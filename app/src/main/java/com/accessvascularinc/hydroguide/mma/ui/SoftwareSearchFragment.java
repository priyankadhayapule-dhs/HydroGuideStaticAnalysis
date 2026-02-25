package com.accessvascularinc.hydroguide.mma.ui;

import android.app.Dialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.UriPermission;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.hardware.usb.UsbManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelFileDescriptor;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
import android.provider.Settings;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.NonNull;
import androidx.core.content.FileProvider;
import androidx.databinding.DataBindingUtil;
import androidx.documentfile.provider.DocumentFile;
import androidx.lifecycle.ViewModelProvider;
import androidx.navigation.Navigation;
import androidx.recyclerview.widget.LinearLayoutManager;

import com.accessvascularinc.hydroguide.mma.BuildConfig;
import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.adapters.DataRecordsAdapter;
import com.accessvascularinc.hydroguide.mma.adapters.SysUpdatePackagesAdapter;
import com.accessvascularinc.hydroguide.mma.databinding.SearchUpdateBinding;
import com.accessvascularinc.hydroguide.mma.db.HydroGuideDatabase;
import com.accessvascularinc.hydroguide.mma.db.di.DatabaseModule;
import com.accessvascularinc.hydroguide.mma.fota.FotaClient;
import com.accessvascularinc.hydroguide.mma.fota.FotaManager;
import com.accessvascularinc.hydroguide.mma.model.ProfileState;
import com.accessvascularinc.hydroguide.mma.model.SoftwareUpdateSource;
import com.accessvascularinc.hydroguide.mma.ui.input.OnItemClickListener;
import com.accessvascularinc.hydroguide.mma.utils.ExportUtility;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import com.accessvascularinc.hydroguide.mma.utils.Utility;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.List;
import com.accessvascularinc.hydroguide.mma.fota.FotaClient;
import com.accessvascularinc.hydroguide.mma.fota.FotaConstants;
import androidx.activity.OnBackPressedCallback;
import com.accessvascularinc.hydroguide.mma.fota.FirmwareManifest;
import com.accessvascularinc.hydroguide.mma.fota.FotaException;
import com.accessvascularinc.hydroguide.mma.fota.FotaManager;
import com.accessvascularinc.hydroguide.mma.fota.FotaProgressCallback;
import com.accessvascularinc.hydroguide.mma.fota.ZipUtil;
import com.accessvascularinc.hydroguide.mma.utils.PrefsManager;
import com.google.gson.Gson;
import java.io.InputStreamReader;
import java.util.Comparator;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

public class SoftwareSearchFragment extends BaseHydroGuideFragment implements OnItemClickListener,DataRecordsAdapter.DataRecordsClickListener
{
  private static final String TAG = "SoftwareSearchFragment";
  private SearchUpdateBinding binding = null;
  private SoftwareUpdateSource updateSource = null;
  private File buildFile;
  private ActivityResultLauncher<Intent> installerResultLauncher;
  private ActivityResultLauncher<Intent> storageIntentResultLauncher;
  private ActivityResultLauncher<Intent> backupIntentResultLauncher;
  private List<File> lstBuildFiles = new ArrayList<>();
  private SysUpdatePackagesAdapter adapter;
  private static String CURRENT_OPERATION = "";
  private static String PKG_INSTALL = "Installing Update";;
  private static String DATA_BACKUP = "Backing up Data";
  private HydroGuideDatabase database;
  private Dialog dialog;
  private ExportUtility.TransferState transferState = new ExportUtility.TransferState();
  private BroadcastReceiver usbReceiver;
  private int FIRMWARE_FILE_COUNT = 0;
  private String FIRMWARE_INSTALLATION_MESG = "";
  private ControllerManager controllerManager;
  private FotaManager fotaManager;
  private FotaClient fotaClient;

  private String CURRENT_FIRMWARE_NAME = "";
  private String UPDATED_FIRMWARE_NAME = "";

  Handler intallerLogsHandler = new Handler(Looper.getMainLooper());
  @Override
  public void onItemClickListener(final int position) {}

  @Override
  public View onCreateView(@NonNull final LayoutInflater inflater, final ViewGroup container, final Bundle savedInstanceState)
  {
    binding = DataBindingUtil.inflate(inflater, R.layout.search_update, container, false);
    super.onCreateView(inflater, container, savedInstanceState);
    database = new DatabaseModule().provideDatabase(requireContext());
    viewmodel = new ViewModelProvider(requireActivity()).get(MainViewModel.class);
    controllerManager = (ControllerManager)viewmodel.getControllerCommunicationManager();
    initAndConnectHardware();
    fotaClient = controllerManager.createFotaClient();
    final Bundle args = getArguments();
    if (args != null)
    {
      final Object srcObj = args.getSerializable(getResources().getString(R.string.map_keys_update_src));
      if (srcObj instanceof SoftwareUpdateSource)
      {
        setUpdateSource((SoftwareUpdateSource) srcObj);
      }
    }
    viewmodel = new ViewModelProvider(requireActivity()).get(MainViewModel.class);
    final ProfileState profileState = viewmodel.getProfileState();
    binding.setProfileState(profileState);

    // Initialize ivUsbIcon to avoid NullPointerException in BaseHydroGuideFragment
    binding.includeTopbar.setTabletState(viewmodel.getTabletState());
    ivUsbIcon = binding.includeTopbar.usbIcon;
    tvBattery = binding.includeTopbar.controllerBatteryLevelPct;
    ivBattery = binding.includeTopbar.remoteBatteryLevelIndicator;
    ivTabletBattery = binding.includeTopbar.tabletBatteryIndicator.tabletBatteryLevelIndicator;
    tvTabletBattery = binding.includeTopbar.tabletBatteryIndicator.tabletBatteryLevelPct;
    if (getUpdateSource().equals(SoftwareUpdateSource.ONLINE))
    {
      binding.tvProgressMessage.setText(getString(R.string.searching_updates_online));
      binding.includeTopbar.setTitle(getString(R.string.search_online));
    }
    else if (getUpdateSource().equals(SoftwareUpdateSource.LOCAL_STORAGE))
    {
      binding.tvProgressMessage.setText(getString(R.string.progress_mesg_updates_from_local_storage));
      binding.includeTopbar.setTitle(getString(R.string.toolbar_title_install_local_storage));
      showProgress();
    }
    else
    {
      showProgress();
      binding.tvProgressMessage.setText(getString(R.string.searching_updates_usb));
      binding.includeTopbar.setTitle(getString(R.string.install_usb));
    }
    binding.includeTopbar.controllerBatteryLabel.setVisibility(View.INVISIBLE);
    binding.includeTopbar.controllerBatteryLevelPct.setVisibility(View.INVISIBLE);
    binding.includeTopbar.remoteBatteryLevelIndicator.setVisibility(View.INVISIBLE);
    binding.includeTopbar.usbIcon.setVisibility(View.INVISIBLE);

    binding.inclBottomBar.lnrHome.setVisibility(View.GONE);
    binding.inclBottomBar.lnrVolume.setVisibility(View.GONE);
    binding.inclBottomBar.lnrBrightness.setVisibility(View.GONE);
    binding.inclBottomBar.lnrResetPassword.setVisibility(View.GONE);
    binding.inclBottomBar.lnrRemove.setVisibility(View.GONE);
    binding.inclBottomBar.lnrSave.setVisibility(View.GONE);

    binding.recInstallPackages.setLayoutManager(new LinearLayoutManager(getContext()));
    adapter = new SysUpdatePackagesAdapter(getContext(), lstBuildFiles,this);
    binding.recInstallPackages.setAdapter(adapter);

    binding.inclBottomBar.lnrBack.setOnClickListener(lnrBackClick);

    if (getUpdateSource().equals(SoftwareUpdateSource.ONLINE))
    {
      binding.inclBottomBar.lnrRetry.setVisibility(View.VISIBLE);
      binding.inclBottomBar.lnrRetry.setOnClickListener(v -> {
        // Reset to loading state
        onProcedureExitAttempt(() -> {
          Bundle bundle = new Bundle();
          bundle.putSerializable(getResources().getString(R.string.map_keys_update_src), getUpdateSource());
          Navigation.findNavController(requireView()).navigate(R.id.action_navigation_next_temp, bundle);
        });
        // Trigger your search again
        //performUpdateSearch();
      });
    }

    installerResultLauncher = instantiateInstallerIntent();
    storageIntentResultLauncher = instantiateStorageIntent();
    backupIntentResultLauncher = instantiateBackupIntent();
    new Handler(Looper.getMainLooper()).postDelayed(this::kickOffSystemUpdates, 1000);
    requireActivity().getOnBackPressedDispatcher().addCallback(getViewLifecycleOwner(),handleBackPress);
    displaySoftwareConfiguration();
    return binding.getRoot();
  }

  public ActivityResultLauncher<Intent> instantiateInstallerIntent() {
    return registerForActivityResult(new ActivityResultContracts.StartActivityForResult(), result ->
    {
      if(requireContext().getPackageManager().canRequestPackageInstalls())
      {
        installBuild();
      }
    });
  }

  public ActivityResultLauncher<Intent> instantiateStorageIntent() {
    return registerForActivityResult(new ActivityResultContracts.StartActivityForResult(), result ->
    {
      if(result.getData() != null)
      {
        if(result.getData().getData() != null)
        {
          Uri uriTree = result.getData().getData();
          requireContext().getContentResolver().takePersistableUriPermission(uriTree, Intent.FLAG_GRANT_READ_URI_PERMISSION);
          copyFilesFromUsb(uriTree);
        }
      }
      else
      {
        binding.inclBottomBar.lnrBack.performClick();
      }
    });
  }

  public ActivityResultLauncher<Intent> instantiateBackupIntent()
  {
    return ExportUtility.instantiateBackupIntent(this, this::exportDataToUsb);
  }

  private final View.OnClickListener lnrBackClick = v -> {
    Navigation.findNavController(requireView()).popBackStack();
  };

  public SoftwareUpdateSource getUpdateSource() {
    return updateSource;
  }

  public void setUpdateSource(SoftwareUpdateSource updateSource) {
    this.updateSource = updateSource;
  }

  private void kickOffSystemUpdates()
  {
    switch (getUpdateSource())
    {
      case LOCAL_STORAGE:
        List<File> packageFiles = Utility.getLocalPackageBuilds(requireContext());
        if(packageFiles.size() > 0)
        {
          lstBuildFiles.clear();
          lstBuildFiles.addAll(packageFiles);
          adapter.notifyDataSetChanged();
          showPackageList();
        }
      break;

      case USB:
        if (!Utility.isUsbStoragePlugged(requireContext()))
        {
          binding.lnrProgressContainer.setVisibility(View.GONE);
          dialog = ConfirmDialog.singleActionPopup(requireContext(),getResources().getString(R.string.missing_external_storage),"OK",confirmed2 -> binding.inclBottomBar.lnrBack.performClick(),ExportUtility.popupWidth,ExportUtility.popupHeight);
          showDialogbox(dialog);
        }
        else
        {
          transferState.setUsbPluggedIn(true);
          transferState.setTransferComplete(false);
          Intent storageResultIntent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
          storageResultIntent.putExtra("android.provider.extra.SHOW_ADVANCED", true);
          storageIntentResultLauncher.launch(storageResultIntent);
        }
      break;
    }
  }

  private void installBuild()
  {
    try
    {
      if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O && !requireActivity().getPackageManager().canRequestPackageInstalls())
      {
        Intent initiateInstallIntent = new Intent(Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES);
        initiateInstallIntent.setData(Uri.parse("package:" + requireActivity().getPackageName()));
        installerResultLauncher.launch(initiateInstallIntent);
        showErrorMesg(getResources().getString(R.string.grant_install_rights));
      }
      else
      {
        if(buildFile != null && !buildFile.exists())
        {
          showErrorMesg(getResources().getString(R.string.build_file_missing));
          return;
        }
        else
        {
          Uri apkUri = FileProvider.getUriForFile(requireContext(), requireContext().getPackageName() + ".fileprovider", buildFile);
          Intent installIntent = new android.content.Intent(android.content.Intent.ACTION_VIEW);
          installIntent.setDataAndType(apkUri, "application/vnd.android.package-archive");
          installIntent.addFlags(android.content.Intent.FLAG_GRANT_READ_URI_PERMISSION | android.content.Intent.FLAG_ACTIVITY_NEW_TASK);
          startActivity(installIntent);
        }
      }
    }
    catch (Exception e)
    {
      showErrorMesg(getResources().getString(R.string.build_install_failed));
      MedDevLog.error(TAG, "Error installing build", e);
    }
  }

  private boolean isBuildPackagesSame(File updatedBuild)
  {
    try
    {
      PackageManager packageManager = requireContext().getPackageManager();
      PackageInfo currentPackage = packageManager.getPackageInfo(requireContext().getPackageName(),0);
      PackageInfo updatedPackage = packageManager.getPackageArchiveInfo(updatedBuild.getAbsolutePath(), 0);
      if (currentPackage == null || updatedPackage == null)
      {
        return false;
      }
      return currentPackage.packageName.equals(updatedPackage.packageName) && currentPackage.versionCode == updatedPackage.versionCode;
    }
    catch (Exception e)
    {
      MedDevLog.error("BaseHydroGuideFragment", "Error comparing build packages", e);
      return false;
    }
  }

  private void showErrorMesg(String msg)
  {
    CustomToast.show(requireContext(),msg, CustomToast.Type.ERROR);
  }

  private void copyFilesFromUsb(Uri treeUri)
  {
    showProgress();
    binding.tvProgressMessage.setText(""+getResources().getString(R.string.sys_update_copying_files_progress));
    SoftwareSearchFragment.CURRENT_OPERATION = SoftwareSearchFragment.PKG_INSTALL;
    new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
      @Override
      public void run() {
        DocumentFile documentTree = DocumentFile.fromTreeUri(requireContext(), treeUri);
        try
        {
          if (documentTree != null)
          {
            File destinationDir = new File(requireContext().getExternalFilesDir(null), Utility.PKG_BUILDS_FOLDER_NAME);
            if (!destinationDir.exists())
            {
              destinationDir.mkdirs();
            }
            DocumentFile[] files = documentTree.listFiles();
            for (DocumentFile file : files)
            {
              if (file!=null)
              {
                if(file.exists())
                {
                  if (file.getName().toLowerCase().endsWith(".apk") || file.getName().toLowerCase().endsWith(".zip"))
                  {
                    if (!Environment.MEDIA_MOUNTED.equals(Utility.getRemovableVolumeState(requireContext())))
                    {
                      showErrorMesg(getString(R.string.missing_external_storage));
                      break;
                    }
                    if (!transferState.isUsbPluggedIn()) break;
                    copyFileSafe(file, destinationDir);
                  }
                }
              }
            }
            if (transferState.isUsbPluggedIn())
            {
              transferState.setTransferComplete(true);
              binding.tvProgressMessage.setText(""+getResources().getString(R.string.sys_update_usb_build_transfer_complete));
              initiateBackup();
            }
          }
        }
        catch (Exception e)
        {
          MedDevLog.error(TAG, "Error transferring build file", e);
          if(!transferState.isUsbPluggedIn())
          {
            showErrorMesg(getString(R.string.build_transfer_failed));
          }
          binding.lnrProgressContainer.setVisibility(View.GONE);
        }
      }
    }, 2000);
  }

  private void copyFileSafe(DocumentFile sourceFile, File destDir)
  {
    ParcelFileDescriptor parcelFileDescriptor = null;
    InputStream instream = null;
    FileOutputStream outstream = null;
    File destFile = new File(destDir, sourceFile.getName());
    try
    {
      parcelFileDescriptor = requireContext().getContentResolver().openFileDescriptor(sourceFile.getUri(), "r");
      if (parcelFileDescriptor == null)
      {
        throw new IOException("Cannot open file descriptor");
      }
      instream = new BufferedInputStream(new FileInputStream(parcelFileDescriptor.getFileDescriptor()));
      outstream = new FileOutputStream(destFile);
      byte[] buffer = new byte[64 * 1024];
      int read;
      while ((read = instream.read(buffer)) != -1)
      {
        if (!transferState.isUsbPluggedIn())
        {
          throw new IOException(getResources().getString(R.string.sys_update_usb_deattached_while_read));
        }
        outstream.write(buffer, 0, read);
      }
      outstream.flush();
      lstBuildFiles.add(destFile);
    }
    catch (IOException ioe)
    {
      try
      {
        if (destFile.exists())
        {
          destFile.delete();
        }
      }
      catch (Exception ignore)
      {
        MedDevLog.error(TAG, "Error deleting incomplete file : "+ignore.getMessage());
      }
      if ("EIO (I/O error)".equalsIgnoreCase(ioe.getMessage()) || !transferState.isUsbPluggedIn())
      {
        showErrorMesg(getString(R.string.sys_update_media_eject_pkg_transfer_failed));
      }
      else
      {
        if(!transferState.isUsbPluggedIn())
        {
          showErrorMesg(getString(R.string.build_transfer_failed));
        }
      }
    }
    finally
    {
      try
      {
        if (instream != null)
        {
          instream.close();
        }
      }
      catch (Exception e)
      {
        MedDevLog.error(TAG, "Error closing stream", e);
      }
      try
      {
        if (outstream != null)
        {
          outstream.close();
        }
      }
      catch (Exception e)
      {
        MedDevLog.error(TAG, "Error closing stream", e);
      }
      try
      {
        if (parcelFileDescriptor != null)
        {
          parcelFileDescriptor.close();
        }
      }
      catch (Exception e)
      {
        MedDevLog.error(TAG, "Error closing file", e);
      }
    }
  }

  private void initiateBackup()
  {
    binding.tvProgressMessage.setText(getResources().getString(R.string.sys_update_usb_build_start_data_export));
    SoftwareSearchFragment.CURRENT_OPERATION = SoftwareSearchFragment.DATA_BACKUP;
    transferState.setBackedUp(false);
    Uri tree = ExportUtility.getPersistedUsbTreeOrNull(requireContext());
    if (tree == null)
    {
      backupIntentResultLauncher.launch(ExportUtility.createBackupLocationIntent());
    }
    else
    {
      exportDataToUsb(tree);
    }
  }

  private void exportDataToUsb(Uri usbTreeUri)
  {
    // Collect files to export
    List<File> filesToExport = new ArrayList<>();
    filesToExport.addAll(ExportUtility.findAllPreferenceFiles(requireContext()));
    database.getOpenHelper().getWritableDatabase();
    filesToExport.addAll(ExportUtility.findAllDatabaseFiles(requireContext()));

    ExportUtility.exportDataToUsb(requireContext(), usbTreeUri, filesToExport, Utility.BACKUP_FILE_PREFIX, transferState,
       new ExportUtility.ExportCallback()
       {
          @Override
          public void onExportSuccess(int copiedFilesCount)
          {
            transferState.setBackedUp(true);
            dialog = ConfirmDialog.singleActionPopup(requireContext(), getResources().getString(R.string.sys_update_backup_local) + copiedFilesCount + " copied files", "OK", confirmed -> binding.inclBottomBar.lnrBack.performClick(), ExportUtility.popupWidth, ExportUtility.popupHeight);
            showDialogbox(dialog);
          }

          @Override
          public void onExportFailure(String errorMessage)
          {
            transferState.setBackedUp(false);
            Utility.deletePackageBuilds(requireContext());
            CustomToast.show(requireContext(), errorMessage, CustomToast.Type.ERROR);
          }

          @Override
          public void onExportProgress(int current, int total){}
        });
  }

  private void initializeUsbReceiver()
  {
    usbReceiver = ExportUtility.createUsbReceiver(transferState, new ExportUtility.UsbReceiverCallback()
    {
      @Override
      public void onUsbAttached()
      {
        // USB attached - no action needed
      }

      @Override
      public void onUsbDetached()
      {
        if (updateSource == SoftwareUpdateSource.USB)
        {
          if (!transferState.isTransferComplete() && SoftwareSearchFragment.CURRENT_OPERATION.equals(SoftwareSearchFragment.PKG_INSTALL))
          {
            MedDevLog.warn(TAG, "USB Detached during apk transfer");
            Utility.deletePackageBuilds(requireContext());
            dialog = ConfirmDialog.singleActionPopup(requireContext(), getResources().getString(R.string.sys_update_media_eject_pkg_transfer_failed), "OK", confirmed -> binding.inclBottomBar.lnrBack.performClick(), ExportUtility.popupWidth, ExportUtility.popupHeight);
            showDialogbox(dialog);
          }
          if (!transferState.isBackedUp() && SoftwareSearchFragment.CURRENT_OPERATION.equals(SoftwareSearchFragment.DATA_BACKUP))
          {
            MedDevLog.warn(TAG, "USB Detached during backup");
            Utility.deletePackageBuilds(requireContext());
            dialog = ConfirmDialog.singleActionPopup(requireContext(), getResources().getString(R.string.sys_update_media_eject_backup_failed), "OK",confirmed -> binding.inclBottomBar.lnrBack.performClick(), ExportUtility.popupWidth, ExportUtility.popupHeight);
            showDialogbox(dialog);
          }
        }
        else if(updateSource == SoftwareUpdateSource.LOCAL_STORAGE)
        {
          if(transferState.isFotaInProgress())
          {
            transferState.setFotaInProgress(false);
            dialog = ConfirmDialog.singleActionPopup(requireContext(),getResources().getString(R.string.fota_media_eject),"OK", confirmed -> hideFotaProgress(),ExportUtility.popupWidth, ExportUtility.popupHeight);
            showDialogbox(dialog);
          }
        }
      }
    });
  }

  @Override
  public void onDataRecordClick(int position)
  {
    if(viewmodel != null && viewmodel.getTabletState() != null)
    {
      if (viewmodel.getTabletState().getTabletChargePct() <= Utility.MIN_BATTERY_LEVEL_THRESHOLD)
      {
        dialog = ConfirmDialog.singleActionPopup(requireContext(), getResources().getString(R.string.fota_battery_level_threshold), "OK", confirmed -> hideFotaProgress(), ExportUtility.popupWidth, ExportUtility.popupHeight);
        showDialogbox(dialog);
      }
      else
      {
        buildFile = lstBuildFiles.get(position);
        if (buildFile.getName().endsWith(Utility.PKG_FILE_EXTENSION_CTRL))
        {
          if(ExportUtility.isValidControllerAttached(requireContext()))
          {
            if(fotaClient != null)
            {
              fotaManager = new FotaManager(fotaClient);
              initAndConnectHardware();
              if (viewmodel != null && !viewmodel.getControllerCommunicationManager().checkConnection())
              {
                initAndConnectHardware();
                binding.tvProgressMessage.setText(getResources().getString(R.string.extracting_package));
                initiateFotaProcess(buildFile);
              }
              else
              {
                binding.tvProgressMessage.setText(getResources().getString(R.string.extracting_package));
                initiateFotaProcess(buildFile);
              }
            }
            else
            {
              dialog = ConfirmDialog.singleActionPopup(requireContext(),getResources().getString(R.string.fota_client_not_instantiated),"OK",confirmed -> {}, ExportUtility.popupWidth,ExportUtility.popupHeight);
              showDialogbox(dialog);
            }
          }
          else
          {
            dialog = ConfirmDialog.singleActionPopup(requireContext(),getResources().getString(R.string.invalid_controller_attached),"OK",confirmed -> {}, ExportUtility.popupWidth,ExportUtility.popupHeight);
            showDialogbox(dialog);
          }
        }
        else
        {
          if (isBuildPackagesSame(buildFile))
          {
            showBuildDuplicateMesg();
            binding.tvNoUpdates.setText(getResources().getString(R.string.build_install_duplicate));
            binding.tvNoUpdates.setOnClickListener(new View.OnClickListener() {
              @Override
              public void onClick(View v)
              {
                showPackageList();
              }
            });
          }
          else
          {
            installBuild();
          }
        }
      }
    }
  }

  private void showDialogbox(Dialog dialogbox)
  {
    if(dialogbox != null)
    {
      dialogbox.dismiss();
    }
    dialogbox.show();
  }

  private void showPackageList()
  {
    binding.recInstallPackages.setVisibility(View.VISIBLE);
    binding.lnrSoftwareVersionContainer.setVisibility(View.VISIBLE);
    binding.lnrProgressContainer.setVisibility(View.GONE);
    binding.tvNoUpdates.setVisibility(View.GONE);
    binding.lnrProgressInstaller.setVisibility(View.GONE);
    binding.inclBottomBar.lnrBack.setVisibility(View.VISIBLE);
  }

  private void showProgress()
  {
    binding.lnrProgressContainer.setVisibility(View.VISIBLE);
    binding.recInstallPackages.setVisibility(View.GONE);
    binding.lnrSoftwareVersionContainer.setVisibility(View.GONE);
    binding.tvNoUpdates.setVisibility(View.GONE);
    binding.inclBottomBar.lnrBack.setVisibility(View.GONE);
    binding.lnrProgressInstaller.setVisibility(View.GONE);
  }

  private void showFotaProgress()
  {
    binding.includeTopbar.setTitle(getResources().getString(R.string.toolbar_title_install_firmware));
    binding.lnrProgressContainer.setVisibility(View.GONE);
    binding.recInstallPackages.setVisibility(View.GONE);
    binding.tvNoUpdates.setVisibility(View.GONE);
    binding.inclBottomBar.lnrBack.setVisibility(View.GONE);
    binding.lnrSoftwareVersionContainer.setVisibility(View.GONE);
    binding.lnrProgressInstaller.setVisibility(View.VISIBLE);
  }

  private void hideFotaProgress()
  {
    binding.includeTopbar.setTitle(getResources().getString(R.string.toolbar_title_install_local_storage));
    binding.lnrProgressContainer.setVisibility(View.GONE);
    binding.recInstallPackages.setVisibility(View.VISIBLE);
    binding.tvNoUpdates.setVisibility(View.GONE);
    binding.inclBottomBar.lnrBack.setVisibility(View.VISIBLE);
    binding.lnrSoftwareVersionContainer.setVisibility(View.VISIBLE);
    binding.lnrProgressInstaller.setVisibility(View.GONE);
  }

  private void showBuildDuplicateMesg()
  {
    binding.tvNoUpdates.setVisibility(View.VISIBLE);
    binding.lnrProgressContainer.setVisibility(View.VISIBLE);
    binding.lnrProgress.setVisibility(View.GONE);
    binding.recInstallPackages.setVisibility(View.GONE);
    binding.lnrSoftwareVersionContainer.setVisibility(View.GONE);
    binding.inclBottomBar.lnrBack.setVisibility(View.VISIBLE);
    binding.lnrProgressInstaller.setVisibility(View.GONE);
  }

  @Override
  public void onResume()
  {
    super.onResume();
    MedDevLog.debug(TAG, TAG+" onResume");
    registerUsbReceiver();
  }

  @Override
  public void onPause() {
    super.onPause();
    MedDevLog.debug(TAG,TAG+" onPause");
    deRegisterUsbReceiver();
  }

  @Override
  public void onDestroy()
  {
    super.onDestroy();
    binding = null;
    deRegisterUsbReceiver();
    if(dialog != null && dialog.isShowing())
    {
      dialog.dismiss();
    }
  }

  private void registerUsbReceiver()
  {
    if (usbReceiver == null)
    {
      initializeUsbReceiver();
    }
    ExportUtility.registerUsbReceiver(requireActivity(), usbReceiver, transferState);
  }

  private void deRegisterUsbReceiver()
  {
    if (usbReceiver != null)
    {
      ExportUtility.deRegisterUsbReceiver(requireActivity(), usbReceiver, transferState);
    }
  }

  private void displaySoftwareConfiguration()
  {
    String version = BuildConfig.VERSION_NAME +" :: "+BuildConfig.VERSION_CODE;
    binding.tvCurrentAppVersion.setText(version);
    binding.tvCurrentControllerVersion.setText(PrefsManager.getFirmwareVersion(requireContext()));
    binding.tvCurrentOsVersion.setText(""+displayOsInfo());
  }

  public String displayOsInfo()
  {
    String versionName = Build.VERSION.RELEASE;
    int sdkNumber = Build.VERSION.SDK_INT;
    String codeName = "UNKNOWN";
    for (Field field : Build.VERSION_CODES.class.getFields())
    {
      try
      {
        if (field.getInt(Build.VERSION_CODES.class) == sdkNumber)
        {
          codeName = field.getName();
          break;
        }
      }
      catch (IllegalAccessException e)
      {
        e.printStackTrace();
      }
    }
    String osInfo = "OS Name (Codename): " + codeName + "\nVersion Name (Release): " + versionName + "\nSDK Number (API Level): " + sdkNumber;
    return osInfo;
  }

  public void initiateFotaProcess(File controllerFile)
  {
    FIRMWARE_FILE_COUNT = 0;
    File fotaFolder = new File(requireContext().getExternalFilesDir(null),Utility.PKG_BUILDS_FOLDER_NAME);
    if(fotaFolder.exists())
    {
      File firmwareFile = new File(fotaFolder,controllerFile.getName());
      if(firmwareFile.exists())
      {
        try
        {
          File firmwareContents = new File(fotaFolder,"Firmware Snapshot");
          List<File> firmwareFiles = ZipUtil.locateFirmwareFile(firmwareFile,firmwareContents);
          InputStreamReader metadataReader = null;
          FirmwareManifest firmwareInfo;
          if(firmwareFiles.size() > 0)
          {
            for(File fil : firmwareFiles)
            {
              if(fil.getName().endsWith(".json"))
              {
                metadataReader = new InputStreamReader(new FileInputStream(fil));
                break;
              }
            }
            if(metadataReader != null)
            {
              firmwareInfo = new Gson().fromJson(metadataReader, FirmwareManifest.class);
              UPDATED_FIRMWARE_NAME = firmwareInfo.getName();
              if(PrefsManager.getFirmwareVersion(requireContext()) != null && PrefsManager.getFirmwareVersion(requireContext()).equalsIgnoreCase(firmwareInfo.getName()))
              {
                dialog = ConfirmDialog.singleActionPopup(requireContext(),getResources().getString(R.string.firmware_version_duplicate),"OK",confirmed -> {}, ExportUtility.popupWidth,ExportUtility.popupHeight);
                showDialogbox(dialog);
                return;
              }
              List<FirmwareManifest.FirmwareInfo> lstFirmwareSnapshots = getFirmwareSnapshot(firmwareInfo.getFiles(),firmwareFiles);
              if(fotaManager != null)
              {
                fotaManager.setVerbose(true);
                showFotaProgress();
              }
              ScheduledExecutorService scheduler = Executors.newSingleThreadScheduledExecutor();
              int delaySec = 0;
              for (FirmwareManifest.FirmwareInfo firmwareInfoItem : lstFirmwareSnapshots)
              {
                final FirmwareManifest.FirmwareInfo item = firmwareInfoItem;
                final int scheduledDelay = delaySec;
                scheduler.schedule(() -> {
                  try
                  {
                    MedDevLog.info(TAG,"******* kick off fota file : " + item.getFile() + " (delay " + scheduledDelay + "s) ********");
                    writeFirmwareFiles(fotaManager, item,lstFirmwareSnapshots);
                  }
                  catch (IOException e)
                  {
                    e.printStackTrace();
                  }
                }, scheduledDelay, TimeUnit.SECONDS);
                delaySec += 100;
              }
              scheduler.shutdown();
            }
            else
            {
              CustomToast.show(requireContext(),getResources().getString(R.string.firmware_metadata_load_failure), CustomToast.Type.ERROR);
            }
          }
          else
          {
            CustomToast.show(requireContext(),getResources().getString(R.string.corrupt_firmware_file), CustomToast.Type.ERROR);
          }
        }
        catch (Exception e)
        {
          e.printStackTrace();
        }
      }
      else
      {
        CustomToast.show(requireContext(),getResources().getString(R.string.missing_firmware_file), CustomToast.Type.ERROR);
      }
    }
  }

  private void writeFirmwareFiles(FotaManager fotaManager,FirmwareManifest.FirmwareInfo firmwareInfoItem,List<FirmwareManifest.FirmwareInfo> lstFirmwareSnapshots) throws IOException
  {
    transferState.setFotaInProgress(true);
    FIRMWARE_FILE_COUNT++;
    fotaManager.transferAsync(requireContext(),FotaManager.loadFirmware(firmwareInfoItem.getBinaryFile()), extractImageIndex(firmwareInfoItem.getImage_index()),new FotaProgressCallback() {
              @Override
              public void onPhaseStarted(String phase, String message)
              {
                binding.tvInstallerMessage.setText("Total File : "+FIRMWARE_FILE_COUNT+"/"+lstFirmwareSnapshots.size()+" - Phase : "+phase+" | Message : "+message);
                MedDevLog.info(TAG,"FOTA Phase Started : "+phase+" | Message : "+message);
              }

              @Override
              public void onPhaseCompleted(String phase, String message)
              {
                binding.tvInstallerMessage.setText("Total File : "+FIRMWARE_FILE_COUNT+"/"+lstFirmwareSnapshots.size()+" - Phase : "+phase+" | Message : "+message);
                MedDevLog.info(TAG,"FOTA Phase Completed : "+phase+" | Message : "+message);
                FIRMWARE_INSTALLATION_MESG = message;
              }

              @Override
              public void onTransferProgress(int bytesSent, int totalBytes)
              {
                binding.tvInstallerTransferProgress.setText("Transfer Progress : "+bytesSent+"/"+totalBytes);
              }

              @Override
              public void onSuccess(long elapsedTimeMs, float speedKBps) {
                MedDevLog.info(TAG,"FOTA Success : Elapsed Time (ms) : "+elapsedTimeMs+" | Speed (KBps) : "+speedKBps);
                if(FIRMWARE_FILE_COUNT == lstFirmwareSnapshots.size())
                {
                  PrefsManager.updateFirmwareVersion(requireContext(),UPDATED_FIRMWARE_NAME);
                  displaySoftwareConfiguration();
                  hideFotaProgress();
                  String installMsg = getResources().getString(R.string.fota_update_completed)+"\n New Firmware Version - "+UPDATED_FIRMWARE_NAME;
                  dialog = ConfirmDialog.singleActionPopup(requireContext(),installMsg,"OK",confirmed -> hideFotaProgress(), ExportUtility.popupWidth,ExportUtility.popupHeight);
                  showDialogbox(dialog);
                  UPDATED_FIRMWARE_NAME = "";
                  controllerManager.allowAccess(true);
                }
              }

              @Override
              public void onError(String phase, FotaException error)
              {
                FIRMWARE_INSTALLATION_MESG = error.getMessage()+"\n Failed to install firmware version - "+UPDATED_FIRMWARE_NAME;
                UPDATED_FIRMWARE_NAME = "";
                if(FIRMWARE_FILE_COUNT == lstFirmwareSnapshots.size())
                {
                  MedDevLog.info(TAG,"onError : File Count : "+FIRMWARE_FILE_COUNT);
                  hideFotaProgress();
                  dialog = ConfirmDialog.singleActionPopup(requireContext(),FIRMWARE_INSTALLATION_MESG,"OK",confirmed -> hideFotaProgress(), ExportUtility.popupWidth,ExportUtility.popupHeight);
                  showDialogbox(dialog);
                  transferState.setFotaInProgress(false);
                  controllerManager.allowAccess(true);
                }
                binding.tvInstallerMessage.setText("FOTA Error in Phase : "+phase+" | Message : "+ error.getMessage());
              }
            });
  }

  private List<FirmwareManifest.FirmwareInfo> getFirmwareSnapshot(List<FirmwareManifest.FirmwareInfo> firmwareInfos,List<File> firmwareFiles)
  {
    List<FirmwareManifest.FirmwareInfo> firmwareInfoList = new ArrayList<>();
    for(FirmwareManifest.FirmwareInfo firmwareInfo : firmwareInfos)
    {
      for(File eachFirmwareFile : firmwareFiles)
      {
        if(firmwareInfo.getFile().trim().toLowerCase().equalsIgnoreCase(eachFirmwareFile.getName().trim().toLowerCase()))
        {
          firmwareInfo.setBinaryFile(eachFirmwareFile);
          break;
        }
      }
    }
    firmwareInfoList.addAll(firmwareInfos);
    firmwareInfoList.sort(Comparator.comparing(FirmwareManifest.FirmwareInfo::getImage_index));
    for(FirmwareManifest.FirmwareInfo firmwareInfo : firmwareInfoList)
    {
      MedDevLog.info(TAG,"Firmware Image : "+firmwareInfo.getFile()+"  Image Index : "+firmwareInfo.getImage_index() +"  Binary File : "+(firmwareInfo.getBinaryFile() != null ? firmwareInfo.getBinaryFile().getName() : "No File Found"));
    }
    return firmwareInfoList;
  }

  private final OnBackPressedCallback handleBackPress = new OnBackPressedCallback(true)
  {
    @Override
    public void handleOnBackPressed() {}
  };

  private int extractImageIndex(String imageIndx)
  {
    return imageIndx.matches("\\d+") ? Integer.parseInt(imageIndx) : FotaConstants.IMAGE_ID_APP;
  }

  private void initAndConnectHardware()
  {
    controllerManager.allowAccess(false);
    if (viewmodel != null && !viewmodel.getControllerCommunicationManager().checkConnection())
    {
      viewmodel.getControllerCommunicationManager().configureHardware(false);
      viewmodel.getControllerCommunicationManager().connectController();
    }
  }
}