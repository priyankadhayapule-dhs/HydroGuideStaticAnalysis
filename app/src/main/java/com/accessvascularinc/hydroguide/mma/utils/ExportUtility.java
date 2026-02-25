package com.accessvascularinc.hydroguide.mma.utils;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.UriPermission;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.net.Uri;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.ViewGroup;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.documentfile.provider.DocumentFile;
import androidx.fragment.app.Fragment;
import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.ui.CustomToast;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

public class ExportUtility
{
  private static final String TAG = "ExportUtility";
  private static final int BUFFER_SIZE = 8192;
  public static final double popupWidth = 0.7f;
  public static final double popupHeight = ViewGroup.LayoutParams.WRAP_CONTENT;
  // Checked with AVI board Ids
  private static final int CONTROLLER_VID = 1027;
  private static final int CONTROLLER_PID = 24597;

  public interface UsbReceiverCallback
  {
    void onUsbAttached();
    void onUsbDetached();
  }

  public interface ExportCallback
  {
    void onExportSuccess(int copiedFilesCount);
    void onExportFailure(String errorMessage);
    void onExportProgress(int current, int total);
  }

  public static class TransferState
  {
    private boolean isBackedUp = false;
    private boolean isTransferComplete = false;
    private boolean isUsbPluggedIn = false;
    private boolean isUsbReceiverRegistered = false;
    private boolean isFotaInProgress = false;

    public boolean isBackedUp()
    {
      return isBackedUp;
    }

    public void setBackedUp(boolean backedUp)
    {
      isBackedUp = backedUp;
    }

    public boolean isTransferComplete()
    {
      return isTransferComplete;
    }

    public void setTransferComplete(boolean transferComplete)
    {
      isTransferComplete = transferComplete;
    }

    public boolean isUsbPluggedIn()
    {
      return isUsbPluggedIn;
    }

    public void setUsbPluggedIn(boolean usbPluggedIn)
    {
      isUsbPluggedIn = usbPluggedIn;
    }

    public boolean isUsbReceiverRegistered()
    {
      return isUsbReceiverRegistered;
    }

    public void setUsbReceiverRegistered(boolean usbReceiverRegistered)
    {
      isUsbReceiverRegistered = usbReceiverRegistered;
    }

    public boolean isFotaInProgress() {
      return isFotaInProgress;
    }

    public void setFotaInProgress(final boolean fotaInProgress) {
      isFotaInProgress = fotaInProgress;
    }

    public void reset()
    {
      isBackedUp = false;
      isTransferComplete = false;
      isUsbPluggedIn = false;
      isFotaInProgress = false;
    }
  }

  public static void exportDataToUsb(@NonNull Context context, @NonNull Uri usbTreeUri, @NonNull List<File> filesToExport, @NonNull String backupPrefix, @NonNull TransferState transferState, @Nullable ExportCallback callback)
  {
    new Thread(() ->
    {
      try
      {
        DocumentFile root = DocumentFile.fromTreeUri(context, usbTreeUri);
        if (root == null || !root.canWrite())
        {
          if (!transferState.isUsbPluggedIn())
          {
            notifyFailure(context, callback, context.getString(R.string.sys_backup_folder_write_err));
          }
          return;
        }

        String stamp = new SimpleDateFormat(Utility.FILE_NAME_STAMP_FORMAT, Locale.US).format(new Date());
        DocumentFile backupDir = root.createDirectory(backupPrefix + stamp);
        if (backupDir == null)
        {
          if (!transferState.isUsbPluggedIn())
          {
            notifyFailure(context, callback, context.getString(R.string.sys_backup_cannot_create_folder));
          }
          return;
        }

        int copied = 0;
        int total = filesToExport.size();

        for (File src : filesToExport)
        {
          String mime = getMimeType(src);
          if (copyFileToUsbStorage(context, backupDir, src, mime))
          {
            copied++;
            if (callback != null)
            {
              int finalCopied = copied;
              runOnUiThread(() -> callback.onExportProgress(finalCopied, total));
            }
          }
        }

        transferState.setTransferComplete(true);
        transferState.setBackedUp(true);

        int finalCopied = copied;
        if (callback != null)
        {
          runOnUiThread(() -> callback.onExportSuccess(finalCopied));
        }
      }
      catch (Exception e)
      {
        transferState.setTransferComplete(false);
        transferState.setBackedUp(false);
        notifyFailure(context, callback, context.getString(R.string.sys_backup_failed) + e.getMessage());
        MedDevLog.error(TAG, "Export failed", e);
      }
    }).start();
  }

  public static boolean copyFileToUsbStorage(@NonNull Context context, @NonNull DocumentFile destDir, @NonNull File src, @NonNull String mime)
  {
    if (!src.exists())
    {
      MedDevLog.warn(TAG, "Source file does not exist: " + src.getAbsolutePath());
      return false;
    }

    DocumentFile out = destDir.createFile(mime, src.getName());
    if (out == null)
    {
      MedDevLog.error(TAG, "Failed to create destination file: " + src.getName());
      return false;
    }

    try (InputStream in = new FileInputStream(src);
         OutputStream os = context.getContentResolver().openOutputStream(out.getUri()))
    {
      if (os == null)
      {
        MedDevLog.error(TAG, "Output stream is null for: " + src.getName());
        return false;
      }

      byte[] buf = new byte[BUFFER_SIZE];
      int bytesRead;
      while ((bytesRead = in.read(buf)) != -1)
      {
        os.write(buf, 0, bytesRead);
      }
      return true;
    }
    catch (Exception e)
    {
      MedDevLog.error(TAG, "Copy failed: " + src.getName(), e);
      return false;
    }
  }

  @Nullable
  public static File zipDirectory(@NonNull File zipLocation, @NonNull List<File> includeDirectories, @NonNull String zipFilePath)
  {
    List<String> filesListInDir = new ArrayList<>();

    try
    {
      for (File dir : includeDirectories)
      {
        populateFilesList(dir, filesListInDir);
      }

      if (filesListInDir.isEmpty())
      {
        MedDevLog.warn(TAG, "No files found to zip");
        return null;
      }

      try (FileOutputStream fos = new FileOutputStream(zipFilePath);
           ZipOutputStream zos = new ZipOutputStream(fos))
      {
        for (String filePath : filesListInDir)
        {
          String relativePath = filePath.substring(zipLocation.getAbsolutePath().length() + 1);
          ZipEntry ze = new ZipEntry(relativePath);
          zos.putNextEntry(ze);

          try (FileInputStream fis = new FileInputStream(filePath))
          {
            byte[] buffer = new byte[1024];
            int len;
            while ((len = fis.read(buffer)) > 0)
            {
              zos.write(buffer, 0, len);
            }
          }
          zos.closeEntry();
        }
      }

      File zipFile = new File(zipFilePath);
      if (zipFile.exists() && zipFile.length() > 0)
      {
        return zipFile;
      }
    }
    catch (IOException e)
    {
      MedDevLog.error(TAG, "Zip operation failed", e);
    }
    return null;
  }

  public static void populateFilesList(@NonNull File dir, @NonNull List<String> filesListInDir)
  {
    if (!dir.exists() || !dir.isDirectory())
    {
      return;
    }

    File[] files = dir.listFiles();
    if (files == null)
    {
      return;
    }

    for (File file : files)
    {
      if (file.isFile())
      {
        filesListInDir.add(file.getAbsolutePath());
      }
      else if (file.isDirectory())
      {
        populateFilesList(file, filesListInDir);
      }
    }
  }

  public static ActivityResultLauncher<Intent> instantiateBackupIntent(@NonNull Fragment fragment, @NonNull java.util.function.Consumer<Uri> onUriSelected)
  {
    return fragment.registerForActivityResult(
        new ActivityResultContracts.StartActivityForResult(),
        result ->
        {
          if (result.getData() != null && result.getData().getData() != null)
          {
            Uri backupUriTree = result.getData().getData();
            fragment.requireContext().getContentResolver().takePersistableUriPermission(
                backupUriTree,
                Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
            onUriSelected.accept(backupUriTree);
          }
        });
  }

  public static void registerUsbReceiver(@NonNull Activity activity, @NonNull BroadcastReceiver receiver, @NonNull TransferState transferState)
  {
    if (!transferState.isUsbReceiverRegistered())
    {
      IntentFilter filter = new IntentFilter();
      filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
      filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
      activity.registerReceiver(receiver, filter);
      transferState.setUsbReceiverRegistered(true);
    }
  }

  public static void deRegisterUsbReceiver(@NonNull Activity activity, @NonNull BroadcastReceiver receiver, @NonNull TransferState transferState)
  {
    try
    {
      if (transferState.isUsbReceiverRegistered())
      {
        activity.unregisterReceiver(receiver);
        transferState.setUsbReceiverRegistered(false);
      }
    }
    catch (IllegalArgumentException e)
    {
      transferState.setUsbReceiverRegistered(false);
      MedDevLog.error(TAG, "Error deregistering USB receiver", e);
    }
  }

  public static BroadcastReceiver createUsbReceiver(@NonNull TransferState transferState, @Nullable UsbReceiverCallback callback)
  {
    return new BroadcastReceiver()
    {
      @Override
      public void onReceive(Context context, Intent intent)
      {
        String action = intent.getAction();
        if (UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(action))
        {
          if(Utility.isUsbStoragePlugged(context))
          {
            transferState.setUsbPluggedIn(true);
            if (callback != null)
            {
              callback.onUsbAttached();
            }
          }
        }
        else if (UsbManager.ACTION_USB_DEVICE_DETACHED.equals(action))
        {
          transferState.setUsbPluggedIn(false);
          if (callback != null)
          {
            callback.onUsbDetached();
          }
        }
      }
    };
  }

  @Nullable
  public static Uri getPersistedUsbTreeOrNull(@NonNull Context context)
  {
    ContentResolver contentResolver = context.getContentResolver();
    for (UriPermission p : contentResolver.getPersistedUriPermissions())
    {
      if (p.isReadPermission())
      {
        return p.getUri();
      }
    }
    return null;
  }

  public static Intent createBackupLocationIntent()
  {
    Intent backupIntent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
    backupIntent.putExtra("android.provider.extra.SHOW_ADVANCED", true);
    backupIntent.addFlags(
        Intent.FLAG_GRANT_READ_URI_PERMISSION |
            Intent.FLAG_GRANT_WRITE_URI_PERMISSION |
            Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
    return backupIntent;
  }

  public static List<File> findAllDatabaseFiles(@NonNull Context context)
  {
    List<File> out = new ArrayList<>();
    File cpDbDir = new File(context.getApplicationInfo().dataDir, "databases");
    fetchExistingDbFiles(cpDbDir, out);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N)
    {
      Context deCtx = context.createDeviceProtectedStorageContext();
      File deDbDir = new File(deCtx.getDataDir(), "databases");
      fetchExistingDbFiles(deDbDir, out);
    }

    return out;
  }

  public static void fetchExistingDbFiles(@Nullable File dir, @NonNull List<File> out)
  {
    if (dir == null || !dir.exists())
    {
      return;
    }

    File[] files = dir.listFiles();
    if (files == null)
    {
      return;
    }

    for (File f : files)
    {
      if (f.isFile())
      {
        out.add(f);
      }
    }
  }

  public static List<File> findAllPreferenceFiles(@NonNull Context context)
  {
    List<File> prefFiles = new ArrayList<>();
    File prefsSrcDir = new File(context.getApplicationInfo().dataDir, "shared_prefs");
    File[] files = prefsSrcDir.listFiles((d, n) -> n.endsWith(".xml"));
    if (files != null)
    {
      for (File file : files)
      {
        prefFiles.add(file);
      }
    }
    return prefFiles;
  }

  private static String getMimeType(@NonNull File file)
  {
    String name = file.getName().toLowerCase(Locale.US);
    if (name.endsWith(".xml"))
    {
      return "application/xml";
    }
    else if (name.endsWith(".db") || name.endsWith(".sqlite"))
    {
      return "application/x-sqlite3";
    }
    else if (name.endsWith(".zip"))
    {
      return "application/zip";
    }
    else if (name.endsWith(".json"))
    {
      return "application/json";
    }
    else if (name.endsWith(".txt") || name.endsWith(".log"))
    {
      return "text/plain";
    }
    return "application/octet-stream";
  }

  private static void runOnUiThread(@NonNull Runnable runnable)
  {
    new Handler(Looper.getMainLooper()).post(runnable);
  }

  private static void notifyFailure(@NonNull Context context, @Nullable ExportCallback callback, @NonNull String errorMessage)
  {
    if (callback != null)
    {
      runOnUiThread(() -> callback.onExportFailure(errorMessage));
    }
    else
    {
      runOnUiThread(() -> CustomToast.show(context, errorMessage, CustomToast.Type.ERROR));
    }
  }

  public static boolean deleteDirectory(@NonNull File directory)
  {
    if (!directory.exists())
    {
      return true;
    }

    File[] files = directory.listFiles();
    if (files != null)
    {
      for (File file : files)
      {
        if (file.isDirectory())
        {
          deleteDirectory(file);
        }
        else
        {
          file.delete();
        }
      }
    }
    return directory.delete();
  }

  public static boolean isValidControllerAttached(Context context) {
    UsbManager manager = (UsbManager) context.getSystemService(Context.USB_SERVICE);
    HashMap<String, UsbDevice> devices = manager.getDeviceList();
    Iterator<String> iterator = devices.keySet().iterator();
    while (iterator.hasNext())
    {
      String deviceName = iterator.next();
      UsbDevice device = devices.get(deviceName);
      if(device.getVendorId() == CONTROLLER_VID && device.getProductId() == CONTROLLER_PID)
      {
        return true;
      }
    }
    return false;
  }
}
