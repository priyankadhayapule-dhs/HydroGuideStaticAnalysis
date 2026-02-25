package com.accessvascularinc.hydroguide.mma.ui;

import android.app.Dialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.graphics.PorterDuff;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AutoCompleteTextView;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.DatePicker;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.ImageView;

import androidx.activity.result.ActivityResultLauncher;
import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;
import androidx.annotation.Nullable;
import androidx.core.util.Consumer;
import androidx.databinding.DataBindingUtil;
import androidx.documentfile.provider.DocumentFile;
import androidx.lifecycle.ViewModelProvider;
import androidx.navigation.Navigation;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.databinding.FragmentRecordsBinding;
import com.accessvascularinc.hydroguide.mma.db.HydroGuideDatabase;
import com.accessvascularinc.hydroguide.mma.db.entities.EncounterEntity;
import com.accessvascularinc.hydroguide.mma.db.entities.FacilityEntity;
import com.accessvascularinc.hydroguide.mma.db.repository.EncounterRepository;
import com.accessvascularinc.hydroguide.mma.db.repository.FacilityRepository;
import com.accessvascularinc.hydroguide.mma.model.DataFiles;
import com.accessvascularinc.hydroguide.mma.model.EncounterData;
import com.accessvascularinc.hydroguide.mma.model.EncounterTableRow;
import com.accessvascularinc.hydroguide.mma.model.Facility;
import com.accessvascularinc.hydroguide.mma.model.PatientData;
import com.accessvascularinc.hydroguide.mma.model.SortMode;
import com.accessvascularinc.hydroguide.mma.ui.input.ExportDialogFragment;
import com.accessvascularinc.hydroguide.mma.ui.input.FacilitiesAutoCompleteAdapter;
import com.accessvascularinc.hydroguide.mma.ui.input.OnItemClickListener;
import com.accessvascularinc.hydroguide.mma.ui.input.StorageSelectionDialogFragment;
import com.accessvascularinc.hydroguide.mma.ui.settings.BrightnessChangeFragment;
import com.accessvascularinc.hydroguide.mma.ui.settings.VolumeChangeFragment;
import com.accessvascularinc.hydroguide.mma.utils.DbHelper;
import com.accessvascularinc.hydroguide.mma.utils.SortComparators;
import com.accessvascularinc.hydroguide.mma.utils.Utility;
import com.accessvascularinc.hydroguide.mma.utils.ExportUtility;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import com.google.android.material.tabs.TabLayout;
import com.accessvascularinc.hydroguide.mma.storage.IStorageManager;
import com.accessvascularinc.hydroguide.mma.storage.IStorageManager.StorageMode;
import com.accessvascularinc.hydroguide.mma.utils.NetworkConnectivityLiveData;
import javax.inject.Inject;

import android.widget.TableRow;
import android.view.Gravity;
import android.widget.Toast;

import com.accessvascularinc.hydroguide.mma.adapters.RecordsAdapter;
import com.accessvascularinc.hydroguide.mma.adapters.CloudRecordsAdapter;
import com.google.android.material.textfield.TextInputLayout;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.io.IOException;
import java.nio.file.FileSystems;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.Executors;

import dagger.hilt.android.AndroidEntryPoint;

@AndroidEntryPoint
public class RecordsFragment extends BaseHydroGuideFragment implements OnItemClickListener,View.OnClickListener
{
  @Inject
  IStorageManager storageManager;
  @Inject
  com.accessvascularinc.hydroguide.mma.network.ApiService apiService;
  private RecyclerView recordsTableView = null;
  private TextView tvEmptyText = null;
  private RecordsAdapter recordsAdapter = null;
  private SortMode sortMode = SortMode.None;
  private ImageButton alphaSortBtn = null, dateSortBtn = null;
  private HydroGuideDatabase db;
  private ImageView syncStatusIcon;
  private TabLayout sourceTabs; // Local/Cloud tabs
  private ArrayList<EncounterTableRow> localRecordsRows = new ArrayList<>();
  private ArrayList<EncounterTableRow> cloudRecordsRows = new ArrayList<>();
  // populated from remote service (TODO)
  private boolean isCloudMode = false;
  private TableRow localHeaderRow; // static local header defined in XML
  private TableRow cloudHeaderRow; // dynamic cloud header placeholder
  private CloudRecordsAdapter cloudRecordsAdapter; // adapter for cloud rows
  private ProgressBar loadingIndicator; // cloud loading spinner
  private NetworkConnectivityLiveData networkConnectivityLiveData;
  private boolean isInternetAvailable = true;
  private static final String INTERNET_UNAVAILABLE_MSG = "Internet not available";
  // Delete functionality UI elements
  private ImageButton deleteButton;
  private TextView deleteButtonLabel;
  private TextView selectionCountText;
  private CheckBox localSelectAllCheckbox;

  // Pagination fields for cloud records
  private int currentPageOffset = 0;
  private int PAGE_SIZE = 10; // Made non-final, will be updated by spinner
  private int totalLocalRecords = 0;
  private boolean isLoadingLocalPage = false;
  private boolean isProcessingClick = false;

  // Pagination UI controls
  private android.widget.Button paginationPrevBtn;
  private android.widget.Button paginationNextBtn;
  private TextView paginationInfo;
  private android.widget.Spinner paginationLimitSpinner;
  private android.widget.LinearLayout paginationControls;

  List<String> filesListInDir = new ArrayList<String>();  // Column definition for dynamic cloud header & rows
  private ExportUtility.TransferState transferState = new ExportUtility.TransferState();
  private ActivityResultLauncher<Intent> backupIntentResultLauncher;
  private File zippedFile;
  private Dialog dialog;
  private ExportDialogFragment exptFragment;
  private BroadcastReceiver usbReceiver;
  private FragmentRecordsBinding binding;
  private List<TextView> lstCloudColumHeadersReference = new ArrayList<>();
  private StringBuffer bufferDate = new StringBuffer();
  private String filteredName = "",filteredId = "",selectedFacility = "",filteredFacility = "",filteredInserter = "",filteredDob = "",filteredFromDob = "",filteredToDob = "",filteredInsertionDate = "",filteredFromInsertionDate = "",filteredToInsertionDate = "";
  private int filteredFacilityId = -1;
  private int DATEPICK_SOURCE_DOB = 1;
  private int DATEPICK_SOURCE_INSERTER = 2;
  private Dialog patientFilterDialog,dateFilterDialog;
  private FacilitiesAutoCompleteAdapter facilitySelectAdapter;
  private int facilitySearchMinChar = 3;
  private String TAG = "Logs";

  // Column definition for dynamic cloud header & rows
  private static class ColumnDefinition {
    final String title;
    final float weight;
    final int gravity;
    final int fieldId;

    ColumnDefinition(String title, float weight, int gravity, int fieldId) {
      this.title = title;
      this.weight = weight;
      this.gravity = gravity;
      this.fieldId = fieldId;
    }
  }

  // Initial cloud columns (modify freely in future)
  private final List<ColumnDefinition> cloudColumns = Arrays.<ColumnDefinition>asList(
      new ColumnDefinition("", 0.8f, Gravity.CENTER,0), // checkbox column header left blank
      new ColumnDefinition("Patient Name", 2f, Gravity.START | Gravity.CENTER_VERTICAL,SortComparators.CLOUD_COLUM_PATIENT_NAME),
      new ColumnDefinition("Patient ID#", 1.6f, Gravity.START | Gravity.CENTER_VERTICAL,SortComparators.CLOUD_COLUM_PATIENT_ID),
      new ColumnDefinition("Facility", 2f, Gravity.START | Gravity.CENTER_VERTICAL,SortComparators.CLOUD_COLUM_FACILITY),
      new ColumnDefinition("Date", 1.4f, Gravity.CENTER,SortComparators.CLOUD_COLUM_DATE),
      // Increased weight from 1.8f -> 2.6f to widen the Procedure Done By column
      new ColumnDefinition("Procedure Done By", 2.6f, Gravity.START | Gravity.CENTER_VERTICAL,SortComparators.CLOUD_COLUM_DONE_BY));

  private final View.OnClickListener homeBtn_OnClickListener = v -> Navigation.findNavController(requireView())
      .navigate(R.id.action_navigation_records_to_home);
  private final View.OnClickListener deleteBtn_OnClickListener = v -> {
    if (!isCloudMode) {
      showDeleteConfirmationDialog();
    }
  };
  private final View.OnClickListener exportBtn_OnClickListener = v -> {
    /*
     * 1) locate usb
     * 2) prompt
     * 3) act
     */
    filesListInDir.clear();
    if (isCloudMode) { // Guard: export only for local source
      final MaterialAlertDialogBuilder builder = new MaterialAlertDialogBuilder(requireActivity());
      builder.setMessage("Export is only available for Local records.");
      builder.setNegativeButton("Return", (dialog, id) -> {
      });
      builder.show();
      return;
    }

    StorageManager sm = (StorageManager) requireContext().getSystemService(Context.STORAGE_SERVICE);
    List<StorageVolume> storageVolumes = sm.getStorageVolumes();
    var candidates = new ArrayList<StorageVolume>();
    List<EncounterTableRow> exportRecords = recordsAdapter.getSelectedRecords();

    if (exportRecords.isEmpty()) {
      final MaterialAlertDialogBuilder builder = new MaterialAlertDialogBuilder(requireActivity());
      builder.setMessage("Select one or more records and try again");
      builder.setNegativeButton("Return", (dialog, id) -> {
      });
      builder.show();
      return;
    }

    for (StorageVolume vol : storageVolumes) {
      File vol_path = vol.getDirectory();
      if (vol_path == null) {
        continue;
      }
      Log.i("ExportScanner", "Found volume: " + vol_path.getAbsolutePath() +
          (vol.isPrimary() ? "   primary" : "") + (vol.isRemovable() ? "   removable" : "") +
          (vol.isEmulated() ? "   emulated" : "") + " named " +
          vol.getDescription(requireContext()));
      if (vol.isRemovable() && !vol_path.toPath().getName(0).toString()
          .equals("storage")) { // !vol_path.toPath().getName(0).toString().equals("storage") &&
        candidates.add(vol);
      }
    }

    if (candidates.size() == 1) {
      promptExport(candidates.get(0));
    } else if (candidates.size() > 1) {
      HashMap<String, StorageVolume> usbNaming = new HashMap<>();
      String[] externals = new String[candidates.size()];
      for (int index = 0; index < candidates.size(); index++) {
        String desc = candidates.get(index).getDescription(requireContext());
        usbNaming.put(desc, candidates.get(index));
        externals[index] = desc;
      }

      String[] target = new String[1];

      StorageSelectionDialogFragment extSelect = new StorageSelectionDialogFragment(externals, target, () -> {
        promptExport(Objects.<StorageVolume>requireNonNull(usbNaming.get(target[0])));
      });
      extSelect.show(getChildFragmentManager(), "TAG");
    } else {
      final MaterialAlertDialogBuilder builder = new MaterialAlertDialogBuilder(requireActivity());
      builder.setMessage("No external storage paths can be found at this time");
      builder.setNegativeButton("Return", (dialog, id) -> {
      });
      builder.show();
    }
  };

  private void promptExport(final StorageVolume candidate) {
    List<EncounterTableRow> exportRecords = recordsAdapter.getSelectedRecords();

    final String sb = "Would you like to export " + exportRecords.size() +
        " procedure reports to the external drive named " +
        candidate.getDescription(requireContext());

    exptFragment = new ExportDialogFragment();

    final Runnable onExport = () -> {
      exptFragment.toggleProgressBar(true);
      transferState.setUsbPluggedIn(Utility.isUsbStoragePlugged(requireContext()));
      if(!transferState.isUsbPluggedIn())
      {
        dialog = ConfirmDialog.singleActionPopup(requireContext(),getResources().getString(R.string.sys_update_media_eject_backup_failed),"OK", confirmed -> {},ExportUtility.popupWidth,ExportUtility.popupHeight);
        hideExportProgress();
        showDialogbox(dialog);
        return;
      }
      startExport(candidate, exptFragment);
    };

    final Runnable onDelete = () -> {
      // Delete
      for (final EncounterTableRow record : exportRecords) {
        recordsAdapter.removeProcedureByStartTime(getDate(record.createdOn));
        // TODO NEED TO DISCUSS WITH AJAY
        // deleteProcedureFile(record);
      }
    };

    exptFragment.setRunnables(onExport, onDelete, sb);
    exptFragment.show(getChildFragmentManager(), "TAG");
  }

  private static Date getDate(String createdOn) {
    final long millis = Long.parseLong(createdOn);

    // Convert long milliseconds to Date
    // Convert long milliseconds to Date
    return new Date(millis);
  }

  private void startExport(final StorageVolume target, final ExportDialogFragment exptDialog) {
    final File rootDir = target.getDirectory();
    final File procedureDir = new File(rootDir, "Exported Procedures");
    procedureDir.mkdir();
    final List<EncounterTableRow> exportRecords = recordsAdapter.getSelectedRecords();
    final double max_value = 100.0;
    double progress = 0.0;
    final double progressPerRecord = max_value / exportRecords.size();

    if(!exportRecords.isEmpty())
    {
      progressVisibility(true);
      List<File> includedFiles = new ArrayList<>();
      for(final EncounterTableRow record : exportRecords)
      {
        if(record.dataDirPath != null && !record.dataDirPath.isEmpty())
        {
          includedFiles.add(new File(record.dataDirPath));
        }
      }
      String procedureDirPath = requireContext().getExternalFilesDir(null).getAbsolutePath()+FileSystems.getDefault().getSeparator()+DataFiles.PROCEDURES_DIR;
      String zipDirName = requireContext().getExternalFilesDir(null).getAbsolutePath()+FileSystems.getDefault().getSeparator()+Utility.PROCEDURE_EXPORTS_FILE_NAME;
      File fil = zipDirectory(new File(procedureDirPath),includedFiles,zipDirName);
      if(fil != null)
      {
        initiateExport();
      }
      else
      {
        CustomToast.show(requireContext(),getResources().getString(R.string.zip_creation_failed), CustomToast.Type.ERROR);
      }
    }
  }

  private final View.OnClickListener sortRecordsAlphaBtn_OnClickListener = v -> {
    switch (sortMode) {
      case None, Date -> {
        sortMode = SortMode.Name;
        setButtonColor(alphaSortBtn, R.color.av_tip_confirm_yellow);
      }
      case Name -> {
        sortMode = SortMode.None;
        setButtonColor(alphaSortBtn, R.color.white);
      }
    }
    setButtonColor(dateSortBtn, R.color.white);
    if (isCloudMode && cloudRecordsAdapter != null) {
      cloudRecordsAdapter.sortRecords(sortMode);
    } else if (!isCloudMode && recordsAdapter != null) {
      recordsAdapter.sortRecords(sortMode);
    }
  };
  private final View.OnClickListener sortRecordsDateBtn_OnClickListener = v -> {
    switch (sortMode) {
      case None, Name -> {
        sortMode = SortMode.Date;
        setButtonColor(dateSortBtn, R.color.av_tip_confirm_yellow);
      }
      case Date -> {
        sortMode = SortMode.None;
        setButtonColor(dateSortBtn, R.color.white);
      }
    }
    setButtonColor(alphaSortBtn, R.color.white);
    if (isCloudMode && cloudRecordsAdapter != null) {
      cloudRecordsAdapter.sortRecords(sortMode);
    } else if (!isCloudMode && recordsAdapter != null) {
      recordsAdapter.sortRecords(sortMode);
    }
  };

  private final View.OnClickListener volumeBtn_OnClickListener = v -> {
    VolumeChangeFragment bottomSheet = new VolumeChangeFragment();
    bottomSheet.show(getChildFragmentManager(), "TEST");
  };
  private final View.OnClickListener brightnessBtn_OnClickListener = v -> {
    BrightnessChangeFragment bottomSheet = new BrightnessChangeFragment();
    bottomSheet.show(getChildFragmentManager(), "TEST");
  };

  private void setButtonColor(final ImageButton button, final int color) {
    final int newColor = ContextCompat.getColor(requireContext(), color);
    button.setColorFilter(newColor, PorterDuff.Mode.MULTIPLY);
  }

  /**
   * Hides the Sync Status column (last column) in the header row
   * Used when tablet is in OFFLINE mode since data won't be synced
   */
  private void hideSyncStatusColumn(TableRow headerRow) {
    if (headerRow != null && headerRow.getChildCount() > 0) {
      // Sync Status is the last column
      int lastColumnIndex = headerRow.getChildCount() - 1;
      View syncStatusColumn = headerRow.getChildAt(lastColumnIndex);
      if (syncStatusColumn != null) {
        syncStatusColumn.setVisibility(View.GONE);
      }
    }
  }

  public View onCreateView(@NonNull final LayoutInflater inflater, final ViewGroup container,
      final Bundle savedInstanceState) {
    db = HydroGuideDatabase.buildDatabase(requireContext());
    binding = DataBindingUtil.<FragmentRecordsBinding>inflate(inflater,
        R.layout.fragment_records,
        container,
        false);
    viewmodel = new ViewModelProvider(requireActivity()).<MainViewModel>get(MainViewModel.class);
    setupVolumeAndBrightnessListeners();

    binding.recordsTopBar.setControllerState(viewmodel.getControllerState());
    binding.recordsTopBar.setTabletState(viewmodel.getTabletState());
    boolean showSyncIcon = false;
    try {
      showSyncIcon = storageManager.isSyncAvailable();
    } catch (Exception e) {
      showSyncIcon = false;
    }
    binding.recordsTopBar.setShowSyncIcon(showSyncIcon);
    tvBattery = binding.recordsTopBar.controllerBatteryLevelPct;
    ivBattery = binding.recordsTopBar.remoteBatteryLevelIndicator;
    ivUsbIcon = binding.recordsTopBar.usbIcon;
    ivTabletBattery = binding.recordsTopBar.tabletBatteryIndicator.tabletBatteryLevelIndicator;
    tvTabletBattery = binding.recordsTopBar.tabletBatteryIndicator.tabletBatteryLevelPct;

    syncStatusIcon = binding.recordsTopBar.syncStatusIcon;

    tvEmptyText = binding.recordsEmptyText;
    loadingIndicator = binding.recordsLoadingIndicator;
    recordsTableView = binding.recordsTableItems;
    sourceTabs = binding.recordsSourceTabs;
    localHeaderRow = binding.localHeaderRow;
    cloudHeaderRow = binding.cloudHeaderRow;
    cloudHeaderRow.setVisibility(View.GONE); // default Local view
    localHeaderRow.setVisibility(View.VISIBLE);
    patientFilterDialog = new Dialog(requireContext());
    dateFilterDialog = new Dialog(requireContext());

    // Hide Sync Status column if in OFFLINE mode
    if (storageManager.getStorageMode() == IStorageManager.StorageMode.OFFLINE) {
      hideSyncStatusColumn(localHeaderRow);
    }

    // Setup tabs: Local default selected
    // Modify tab setup to conditionally show tabs based on storage mode
    if (storageManager.getStorageMode() == IStorageManager.StorageMode.ONLINE) {
      sourceTabs.addTab(sourceTabs.newTab().setText("Local"), true);
      sourceTabs.addTab(sourceTabs.newTab().setText("Cloud"));
    } else {
      sourceTabs.addTab(sourceTabs.newTab().setText("Local"), true);
    }

    final ArrayList<EncounterData> savedRecords = new ArrayList<>(
        Arrays.<EncounterData>asList(DataFiles.getSavedRecords(requireContext())));
    recordsTableView.setLayoutManager(
        new LinearLayoutManager(getContext(), LinearLayoutManager.VERTICAL, false));
    // final ArrayList<PatientFacilityInfo> test = getRecords();
    getRecordsAsync(records -> {
      // Local records fetched on main thread
      localRecordsRows = records;
      for (int i = 0; i < records.size(); i++) {
        Log.d("ABC", "Item: " + records.get(i).patientName);
      }
      boolean isOfflineMode = storageManager.getStorageMode() == IStorageManager.StorageMode.OFFLINE;
      recordsAdapter = new RecordsAdapter(records, this, savedRecords, isOfflineMode);
      recordsTableView.setAdapter(recordsAdapter);
      setupSourceTabListener(); // now adapter exists
      initializeSortIcons(recordsAdapter,binding);
    });

    tvEmptyText.setVisibility(savedRecords.isEmpty() ? View.VISIBLE : View.GONE);

    binding.homeButton.setOnClickListener(homeBtn_OnClickListener);
    binding.exportButton.setOnClickListener(exportBtn_OnClickListener);
    binding.brightnessBtn.setOnClickListener(brightnessBtn_OnClickListener);
    binding.volumeBtn.setOnClickListener(volumeBtn_OnClickListener);
    // Initialize delete button and selection count
    deleteButton = binding.getRoot().findViewById(R.id.delete_button);
    deleteButtonLabel = binding.getRoot().findViewById(R.id.delete_button_label);
    selectionCountText = binding.getRoot().findViewById(R.id.selection_count_text);
    localSelectAllCheckbox = binding.getRoot().findViewById(R.id.local_select_all_checkbox);

    deleteButton.setOnClickListener(deleteBtn_OnClickListener);
    setupSelectAllCheckbox();


    alphaSortBtn = binding.sortRecordsAlphaBtn;
    alphaSortBtn.setOnClickListener(sortRecordsAlphaBtn_OnClickListener);
    dateSortBtn = binding.sortRecordsDateBtn;
    dateSortBtn.setOnClickListener(sortRecordsDateBtn_OnClickListener);

    // Initialize pagination controls
    paginationPrevBtn = binding.getRoot().findViewById(R.id.pagination_prev_btn);
    paginationNextBtn = binding.getRoot().findViewById(R.id.pagination_next_btn);
    paginationInfo = binding.getRoot().findViewById(R.id.pagination_info);
    paginationLimitSpinner = binding.getRoot().findViewById(R.id.pagination_limit_spinner);
    paginationControls = binding.getRoot().findViewById(R.id.pagination_controls);

    // Setup pagination controls
    setupPaginationControls();

    // Hide pagination controls by default (only shown in cloud mode)
    if (paginationControls != null) {
      paginationControls.setVisibility(View.GONE);
    }

    backupIntentResultLauncher = instantiateBackupIntent();
    binding.tvPatientNameHeader.setOnClickListener(this);
    binding.tvPatientIdHeader.setOnClickListener(this);
    binding.tvPatientDobHeader.setOnClickListener(this);
    binding.tvInsertionDateHeader.setOnClickListener(this);
    binding.tvDoneByHeader.setOnClickListener(this);
    binding.tvFacilityHeader.setOnClickListener(this);
    binding.lnrShowFilters.setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v)
      {
        if(isFilteringAllowed())
        {
          showFilters();
        }
        else
        {
          CustomToast.show(requireContext(),getResources().getString(R.string.validation_filter_on_selected_row), CustomToast.Type.ERROR);
        }
      }
    });
    binding.lnrClearFilters.setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v)
      {
        if(isFilteringAllowed())
        {
          clearFilters((isCloudMode) ? cloudRecordsAdapter : recordsAdapter);
        }
        else
        {
          CustomToast.show(requireContext(),getResources().getString(R.string.validation_filter_on_selected_row), CustomToast.Type.ERROR);
        }
      }
    });
    binding.sortRecordsAlpha.setVisibility(View.GONE);
    binding.sortRecordsDate.setVisibility(View.GONE);
    return binding.getRoot();
  }

  @Override
  public void onViewCreated(@NonNull final View view, @Nullable final Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);
    // Set up sync button and network observer (see BaseHydroGuideFragment for
    // details)
    setupNetworkSync(syncStatusIcon, getViewLifecycleOwner(), requireContext());
    if (networkConnectivityLiveData == null) {
      networkConnectivityLiveData = new NetworkConnectivityLiveData(requireContext());
      networkConnectivityLiveData.observe(getViewLifecycleOwner(), isConnected -> {
        handleCloudConnectivityChange(Boolean.TRUE.equals(isConnected));
      });
    }
  }

  private void getRecordsAsync(Consumer<ArrayList<EncounterTableRow>> callback) {
    Executors.newSingleThreadExecutor().execute(() -> {
      EncounterRepository repository = new EncounterRepository(db);
      ArrayList<EncounterTableRow> tableRows = new ArrayList<>();
      try {
        List<EncounterEntity> entities = repository.getAllEncounterDataList();
        for (EncounterEntity entity : entities) {
          EncounterTableRow row = new EncounterTableRow();
          if (entity.getPatient() != null) {
            row.patientName = entity.getPatient().getPatientName();
            row.patientId = entity.getPatient().getPatientId();
            row.patientDob = entity.getPatient().getPatientDob();
            row.facilityName = entity.getPatient().getPatientFacility() != null
                ? entity.getPatient().getPatientFacility().getFacilityName()
                : null;
          }
          // Use conductedBy for the 'Procedure Done by' column
          row.createdBy = entity.getConductedBy();
          row.createdOn = entity.getStartTime() != null ? String.valueOf(entity.getStartTime()) : null;
          row.encounterId = entity.getId();
          row.uuid = entity.getUuid();
          row.state = entity.getState() != null ? entity.getState().toString() : null;
          row.dataDirPath = entity.getDataDirPath();
          row.syncStatus = entity.getSyncStatus();
          tableRows.add(row);
        }
        Log.d("LocalDB_FETCH", "Fetched " + tableRows.size() + " records from EncounterEntity");
      } catch (Exception e) {
        MedDevLog.error("LocalDB_FETCH", "Error fetching EncounterEntity records", e);
      }
      new Handler(Looper.getMainLooper()).post(() -> callback.accept(tableRows));
    });
  }

  // Fetch cloud records with pagination
  private void getCloudRecordsAsync(Consumer<ArrayList<EncounterTableRow>> callback) {
    filteredName = "";
    filteredFromInsertionDate = "";
    filteredToInsertionDate = "";
    filteredFacilityId = 0;
    filteredFromDob = "";
    filteredToDob = "";
    fetchCloudRecord(callback);
  }

  private void fetchTotalCloudRecordCount(String orgId, String userId) {
    try {
      // Use Hilt-injected ApiService (includes auth token interceptor)

      // Try raw response first (in case API returns plain integer)
      apiService.getProcedureCountRaw(
          orgId,
              filteredName, // searchTerm
              filteredFromInsertionDate, // fromDate
              filteredToInsertionDate, // toDate
              filteredFacilityId, // facilityId
          "", // patientSide
              filteredFromDob, // dobFromDate
              filteredToDob, // dobToDate
          userId != null ? userId : "").enqueue(
              new retrofit2.Callback<okhttp3.ResponseBody>() {
                @Override
                public void onResponse(
                    retrofit2.Call<okhttp3.ResponseBody> call,
                    retrofit2.Response<okhttp3.ResponseBody> response) {
                  try {
                    if (response.isSuccessful() && response.body() != null) {
                      String responseText = response.body().string().trim();
                      Log.d("CloudRecordCount", "Raw response: " + responseText);

                      try {
                        // Try parsing as plain integer first
                        totalLocalRecords = Integer.parseInt(responseText);
                        Log.d("CloudRecordCount", "Parsed as integer. Total cloud records: " + totalLocalRecords);
                      } catch (NumberFormatException e) {
                        // If not a plain integer, try parsing as JSON
                        try {
                          org.json.JSONObject jsonObj = new org.json.JSONObject(responseText);
                          totalLocalRecords = jsonObj.optInt("totalCount", 0);
                          Log.d("CloudRecordCount", "Parsed as JSON. Total cloud records: " + totalLocalRecords);
                        } catch (Exception jsonEx) {
                          MedDevLog.error("CloudRecordCount", "Failed to parse response as JSON or integer", jsonEx);
                          totalLocalRecords = 0;
                        }
                      }

                      new Handler(Looper.getMainLooper()).post(() -> {
                        updatePaginationInfo();
                        Log.d("PaginationDebug",
                            "Updated pagination info with totalLocalRecords: " + totalLocalRecords);
                      });
                    } else {
                      MedDevLog.warn("CloudRecordCount", "Count response not successful. Code: " + response.code());
                      if (response.errorBody() != null) {
                        MedDevLog.warn("CloudRecordCount", "Error body: " + response.errorBody().string());
                      }
                    }
                  } catch (Exception e) {
                    MedDevLog.error("CloudRecordCount", "Error processing count response", e);
                  }
                }

                @Override
                public void onFailure(
                    retrofit2.Call<okhttp3.ResponseBody> call,
                    Throwable t) {
                  MedDevLog.error("CloudRecordCount", "Failed to fetch count", t);
                }
              });
    } catch (Exception e) {
      MedDevLog.error("CloudRecordCount", "Error fetching record count", e);
    }
  }

  private long parseStartTime(String startTime) {
    if (startTime == null) {
      return System.currentTimeMillis();
    }
    java.text.SimpleDateFormat fmtMs = new java.text.SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSS", java.util.Locale.US);
    java.text.SimpleDateFormat fmtNoMs = new java.text.SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss", java.util.Locale.US);
    try {
      return fmtMs.parse(startTime).getTime();
    } catch (Exception e) {
      try {
        return fmtNoMs.parse(startTime).getTime();
      } catch (Exception ex) {
        return System.currentTimeMillis();
      }
    }
  }

  private void setupSourceTabListener() {
    if (sourceTabs == null) {
      return;
    }
    sourceTabs.addOnTabSelectedListener(new TabLayout.OnTabSelectedListener() {
      @Override
      public void onTabSelected(TabLayout.Tab tab) {
        if (tab.getPosition() == 0) { // Local
          isCloudMode = false;
          if (viewmodel != null) {
            viewmodel.setRecordsIsCloudMode(false);
          }
          cloudHeaderRow.setVisibility(View.GONE);
          localHeaderRow.setVisibility(View.VISIBLE);
          // Ensure recycler visible and spinner hidden when returning to Local
          if (loadingIndicator != null) {
            loadingIndicator.setVisibility(View.GONE);
          }
          if (recordsTableView != null) {
            recordsTableView.setVisibility(View.VISIBLE);
          }
          refreshForSource(localRecordsRows);
          // Clear selections and update UI
          if (recordsAdapter != null) {
            recordsAdapter.clearSelection();
          }
          // Hide pagination controls for local tab
          if (paginationControls != null) {
            paginationControls.setVisibility(View.GONE);
          }
          updateSelectionUI();
        } else { // Cloud
          enterCloudMode();
          if (storageManager.getStorageMode() == StorageMode.ONLINE && !isInternetAvailable) {
            showCloudOfflineState();
            return;
          }
          fetchAndDisplayCloudRecords();
        }
      }

      @Override
      public void onTabUnselected(TabLayout.Tab tab) {
      }

      @Override
      public void onTabReselected(TabLayout.Tab tab) {
        // Optional: force refresh
        onTabSelected(tab);
      }
    });

    // Restore previously selected tab after listener attached
    if (viewmodel != null && viewmodel.getRecordsIsCloudMode()) {
      TabLayout.Tab cloudTab = sourceTabs.getTabAt(1);
      if (cloudTab != null) {
        cloudTab.select();
        enterCloudMode();
        if (storageManager.getStorageMode() == StorageMode.ONLINE && !isInternetAvailable) {
          showCloudOfflineState();
        } else {
          fetchAndDisplayCloudRecords();
        }
      }
    }
  }

  private void enterCloudMode() {
    isCloudMode = true;
    if (viewmodel != null) {
      viewmodel.setRecordsIsCloudMode(true);
    }
    localHeaderRow.setVisibility(View.GONE);
    cloudHeaderRow.setVisibility(View.VISIBLE);
    buildCloudHeader();
    // Show pagination controls for cloud records
    if (paginationControls != null) {
      paginationControls.setVisibility(View.VISIBLE);
    }
  }

  private void setupPaginationControls() {
    // Setup page size spinner
    android.widget.ArrayAdapter<CharSequence> adapter = android.widget.ArrayAdapter.createFromResource(
        requireContext(), R.array.page_sizes, android.R.layout.simple_spinner_item);
    adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);

    paginationLimitSpinner.setAdapter(adapter);

    // Set default to 10
    paginationLimitSpinner.setSelection(0);

    // Set spinner text color to white
    paginationLimitSpinner.setOnItemSelectedListener(new android.widget.AdapterView.OnItemSelectedListener() {
      @Override
      public void onItemSelected(android.widget.AdapterView<?> parent, View view, int position, long id) {
        if (view != null) {
          ((TextView) view).setTextColor(ContextCompat.getColor(requireContext(), R.color.white));
        }
        // Update PAGE_SIZE based on selected item
        String selectedSize = parent.getItemAtPosition(position).toString();
        int newPageSize = Integer.parseInt(selectedSize);

        // Only refresh if page size actually changed
        if (newPageSize != PAGE_SIZE) {
          PAGE_SIZE = newPageSize;
          Log.d("PaginationUI", "Page size changed to: " + PAGE_SIZE);

          // Reset to first page and reset total count cache
          currentPageOffset = 0;
          totalLocalRecords = 0; // Reset count to force re-fetch with new page size

          getCloudRecordsAsync(newRecords -> {
            cloudRecordsRows = newRecords;
            cloudRecordsAdapter.update(new ArrayList<>(cloudRecordsRows));
            updatePaginationInfo(); // Update pagination display with new page size
            recordsTableView.scrollToPosition(0);
          });
        }
      }

      @Override
      public void onNothingSelected(android.widget.AdapterView<?> parent) {
      }
    });

    // Previous button listener
    paginationPrevBtn.setOnClickListener(v -> {
      if (currentPageOffset >= PAGE_SIZE) {
        currentPageOffset -= PAGE_SIZE;
        getCloudRecordsAsync(newRecords -> {
          cloudRecordsRows = newRecords;
          cloudRecordsAdapter.update(new ArrayList<>(cloudRecordsRows));
          updatePaginationInfo(); // Update pagination display with new page
          recordsTableView.scrollToPosition(0);
        });
      }
    });

    // Next button listener
    paginationNextBtn.setOnClickListener(v -> {
      currentPageOffset += PAGE_SIZE;
      getCloudRecordsAsync(newRecords -> {
        cloudRecordsRows = newRecords;
        cloudRecordsAdapter.update(new ArrayList<>(cloudRecordsRows));
        updatePaginationInfo(); // Update pagination display with new page
        recordsTableView.scrollToPosition(0);
      });
    });
  }

  private void updatePaginationInfo() {
    if (paginationInfo != null) {
      if (totalLocalRecords > 0) {
        int currentPage = (currentPageOffset / PAGE_SIZE) + 1;
        int totalPages = (int) Math.ceil((double) totalLocalRecords / PAGE_SIZE);
        // Show format similar to web portal: "Page 1 of 8 (Showing 1-10 of 74 results)"
        int startRecord = currentPageOffset + 1;
        int endRecord = Math.min(currentPageOffset + PAGE_SIZE, totalLocalRecords);
        paginationInfo.setText("Page " + currentPage + " of " + totalPages + " (Showing " + startRecord + "-"
            + endRecord + " of " + totalLocalRecords + ")");

        // Enable/disable buttons based on current page
        if (paginationPrevBtn != null) {
          paginationPrevBtn.setEnabled(currentPageOffset > 0);
        }
        if (paginationNextBtn != null) {
          paginationNextBtn.setEnabled((currentPageOffset + PAGE_SIZE) < totalLocalRecords);
        }
      } else {
        // Show default while loading
        paginationInfo.setText("Page 1 of 1");
        if (paginationPrevBtn != null) {
          paginationPrevBtn.setEnabled(false);
        }
        if (paginationNextBtn != null) {
          paginationNextBtn.setEnabled(false);
        }
      }
    }
  }

  private void showCloudOfflineState() {
    if (tvEmptyText != null) {
      tvEmptyText.setText(INTERNET_UNAVAILABLE_MSG);
      tvEmptyText.setVisibility(View.VISIBLE);
    }
    if (loadingIndicator != null) {
      loadingIndicator.setVisibility(View.GONE);
    }
    if (recordsTableView != null) {
      recordsTableView.setVisibility(View.GONE);
    }
  }

  private void fetchAndDisplayCloudRecords() {
    if (recordsTableView != null) {
      recordsTableView.setAdapter(null);
      recordsTableView.setVisibility(View.GONE);
    }
    if (tvEmptyText != null) {
      tvEmptyText.setVisibility(View.GONE);
    }
    if (loadingIndicator != null) {
      loadingIndicator.setVisibility(View.VISIBLE);
    }

    // Reset pagination for cloud records
    currentPageOffset = 0;
    cloudRecordsRows.clear();
    totalLocalRecords = 0; // Reset total count cache to force re-fetch

    // Fetch total record count first
    try {
      String orgId = com.accessvascularinc.hydroguide.mma.utils.PrefsManager.getOrganizationId(
          requireContext());
      String userId = com.accessvascularinc.hydroguide.mma.utils.PrefsManager.getUserId(requireContext());
      if (orgId != null) {
        applyClearFilters();
        fetchTotalCloudRecordCount(orgId, userId);
      }
    } catch (Exception e) {
      MedDevLog.error("CloudFetch", "Error fetching total count on initial load", e);
    }

    getCloudRecordsAsync(rows -> {
      cloudRecordsRows = rows;
      if (storageManager.getStorageMode() == StorageMode.ONLINE && !isInternetAvailable) {
        showCloudOfflineState();
        return;
      }
      if (cloudRecordsAdapter == null) {
        cloudRecordsAdapter = new CloudRecordsAdapter(new ArrayList<>(rows), RecordsFragment.this);
      } else {
        cloudRecordsAdapter.update(new ArrayList<>(rows));
      }
      if (sortMode != SortMode.None && cloudRecordsAdapter != null) {
        cloudRecordsAdapter.sortRecords(sortMode);
      }
      refreshForSource(cloudRecordsRows);
      // Removed automatic scroll-based pagination - only use Next/Previous buttons
      // Don't call updatePaginationInfo here - it will be called after count is
      // fetched
    });
  }

  private void handleCloudConnectivityChange(boolean connected) {
    isInternetAvailable = connected;
    if (storageManager == null || storageManager.getStorageMode() != StorageMode.ONLINE || !isCloudMode) {
      return;
    }
    if (connected) {
      if (tvEmptyText != null && INTERNET_UNAVAILABLE_MSG.equals(tvEmptyText.getText())) {
        tvEmptyText.setVisibility(View.GONE);
      }
      fetchAndDisplayCloudRecords();
    } else {
      showCloudOfflineState();
    }
  }

  private void refreshForSource(ArrayList<EncounterTableRow> rows) {
    if (!isCloudMode) {
      if (recordsAdapter != null) {
        recordsAdapter.clearSelection();
        recordsAdapter.updateSavedProcedures(rows);
        recordsTableView.setAdapter(recordsAdapter);
        recordsTableView.setVisibility(View.VISIBLE);
        initializeSortIcons(recordsAdapter,binding);
      }
    } else {
      if (cloudRecordsAdapter == null) {
        cloudRecordsAdapter = new CloudRecordsAdapter(rows, this);
      } else {
        cloudRecordsAdapter.update(rows);
      }
      recordsTableView.setAdapter(cloudRecordsAdapter);
      recordsTableView.setVisibility(View.VISIBLE); // show after data fetched
      if (loadingIndicator != null) {
        loadingIndicator.setVisibility(View.GONE);
      }
      initializeCloudSortIcons(lstCloudColumHeadersReference);
    }
    tvEmptyText.setVisibility(rows.isEmpty() ? View.VISIBLE : View.GONE);
  }

  @Override
  public void onItemClickListener(final int position) {
    // Guard against rapid multiple clicks causing duplicate navigation/downloads
    if (isProcessingClick) return;
    isProcessingClick = true;

    if (isCloudMode) {
      EncounterTableRow cloudRow = cloudRecordsRows.get(position);
      EncounterData cloudEncounter = new EncounterData();
      try {
        if (cloudRow.createdOn != null) {
          long millis = Long.parseLong(cloudRow.createdOn);
          cloudEncounter.setStartTime(new Date(millis));
        }
      } catch (Exception ignored) {
      }
      PatientData patientData = new PatientData();
      patientData.setPatientName(cloudRow.patientName != null ? cloudRow.patientName : "");
      patientData.setPatientDob(cloudRow.patientDob != null ? cloudRow.patientDob : "");
      patientData.setPatientId(cloudRow.patientId != null ? cloudRow.patientId : "");
      patientData.setPatientInserter(cloudRow.createdBy != null ? cloudRow.createdBy : "");
      Facility fac = new Facility("", cloudRow.facilityName != null ? cloudRow.facilityName : "",
          cloudEncounter.getStartTimeText());
      patientData.setPatientFacility(fac);
      cloudEncounter.setPatient(patientData);
      cloudEncounter.setFacilityOrLocation(cloudRow.facilityName);
      cloudEncounter.setClinicianId(cloudRow.createdBy);

      // Prepare temp directory under procedures
      File proceduresDir = requireContext().getExternalFilesDir(DataFiles.PROCEDURES_DIR);
      if (proceduresDir == null) {
        Toast.makeText(requireContext(), "Storage unavailable", Toast.LENGTH_SHORT).show();
        isProcessingClick = false;
        return;
      }
      File tempRoot = new File(proceduresDir, "temp");
      if (!tempRoot.exists()) {
        tempRoot.mkdir();
      }
      // Unique subdir per cloud record to avoid collisions
      String dirName = "cloud_" + (cloudRow.createdOn != null ? cloudRow.createdOn : System.currentTimeMillis());
      File recordDir = new File(tempRoot, dirName);
      if (!recordDir.exists()) {
        recordDir.mkdir();
      }
      File pdfFile = new File(recordDir, DataFiles.REPORT_PDF);

      // If already downloaded, skip network call
      if (pdfFile.exists()) {
        cloudEncounter.setDataDirPath(recordDir.getAbsolutePath());
        viewmodel.setProcedureReport(cloudEncounter);
        viewmodel.setEncounterData(cloudEncounter);
        navigateToOpenReports();
        return;
      }

      // Show a quick toast to indicate download start
      Toast.makeText(requireContext(), "Downloading report...", Toast.LENGTH_SHORT).show();

      // Use Hilt-injected ApiService (includes auth token interceptor)
      String fileUrl = cloudRow.fileName;
      if (fileUrl == null || fileUrl.isEmpty()) {
        Toast.makeText(requireContext(), "No file URL provided", Toast.LENGTH_SHORT).show();
        isProcessingClick = false;
        return;
      }
      String blobOrgId = com.accessvascularinc.hydroguide.mma.utils.PrefsManager.getOrganizationId(requireContext());
      if (blobOrgId == null) {
        Toast.makeText(requireContext(), "Organization ID not found", Toast.LENGTH_SHORT).show();
        isProcessingClick = false;
        return;
      }
      Log.d("BLOB URL", "onItemClickListener: " + fileUrl);
      apiService.getBlobStorageFile(blobOrgId, fileUrl).enqueue(new retrofit2.Callback<okhttp3.ResponseBody>() {
        @Override
        public void onResponse(retrofit2.Call<okhttp3.ResponseBody> call,
            retrofit2.Response<okhttp3.ResponseBody> response) {
          Log.d("Response", "onResponse: " + response.message());
          if (!response.isSuccessful() || response.body() == null) {
            isProcessingClick = false;
            Toast.makeText(requireContext(), "Download failed", Toast.LENGTH_SHORT).show();
            return;
          }
          // Save file in background thread
          Executors.newSingleThreadExecutor().execute(() -> {
            try (java.io.InputStream in = response.body().byteStream();
                java.io.FileOutputStream out = new java.io.FileOutputStream(pdfFile)) {
              byte[] buffer = new byte[8192];
              int read;
              while ((read = in.read(buffer)) != -1) {
                out.write(buffer, 0, read);
              }
              out.flush();
              cloudEncounter.setDataDirPath(recordDir.getAbsolutePath());
              // UI updates must run on main thread
              new Handler(Looper.getMainLooper()).post(() -> {
                viewmodel.setProcedureReport(cloudEncounter);
                viewmodel.setEncounterData(cloudEncounter);
                navigateToOpenReports();
              });
            } catch (Exception e) {
              Log.d("File Error", "onResponse: " + e.toString());
              new Handler(Looper.getMainLooper()).post(() -> {
                isProcessingClick = false;
                Toast.makeText(requireContext(), "File save error" + e.getMessage(), Toast.LENGTH_SHORT).show();
              });
            }
          });
        }

        @Override
        public void onFailure(retrofit2.Call<okhttp3.ResponseBody> call, Throwable t) {
          isProcessingClick = false;
          Toast.makeText(requireContext(), "Download error", Toast.LENGTH_SHORT).show();
        }
      });
    } else {
      db = HydroGuideDatabase.buildDatabase(requireContext());
      final EncounterData selectedRecord = recordsAdapter.getRecordAtIdx(position, db);
      viewmodel.setProcedureReport(selectedRecord);
      navigateToOpenReports();
    }
  }

  private void navigateToOpenReports()
  {
    try {
      Bundle bundl = new Bundle();
      bundl.putBoolean(getResources().getString(R.string.report_isSubmitted), true);
      Navigation.findNavController(requireView()).navigate(R.id.nav_records_to_report_view, bundl);
    } catch (IllegalArgumentException | IllegalStateException e) {
      // Already navigated away or fragment not attached — ignore
      Log.w("RecordsFragment", "Navigation skipped: " + e.getMessage());
    } finally {
      isProcessingClick = false;
    }
  }

  // Build cloud header dynamically from definitions (fragment scope)
  private void buildCloudHeader() {
    if (cloudHeaderRow == null) {
      return;
    }
    lstCloudColumHeadersReference.clear();
    cloudHeaderRow.removeAllViews();
    Context ctx = cloudHeaderRow.getContext();
    // Fixed widths to align with data rows. Rebalanced: less Facility width, more
    // Procedure Done By.
    int cbW = ctx.getResources().getDimensionPixelSize(R.dimen.records_column_checkbox_width);
    int nameW = ctx.getResources().getDimensionPixelSize(R.dimen.records_column_name_width);
    int idW = ctx.getResources().getDimensionPixelSize(R.dimen.records_column_id_width);
    int facilityBase = ctx.getResources().getDimensionPixelSize(R.dimen.records_column_facility_width);
    int dobW = ctx.getResources().getDimensionPixelSize(R.dimen.records_column_dob_width);
    int syncW = ctx.getResources().getDimensionPixelSize(R.dimen.records_column_sync_status_width);
    int extraCloud = ctx.getResources().getDimensionPixelSize(R.dimen.records_cloud_extra_width);
    int facilityW = facilityBase + (dobW / 2) + (syncW / 2); // reduced facility
    int dateW = ctx.getResources().getDimensionPixelSize(R.dimen.records_column_date_width);
    int doneByBase = ctx.getResources().getDimensionPixelSize(R.dimen.records_column_done_by_width);
    int doneByW = doneByBase + (dobW / 2) + (syncW / 2) + extraCloud; // widened Procedure Done By
    int rowHeight = ctx.getResources().getDimensionPixelSize(R.dimen.records_table_row_height);
    int pad = (int) ctx.getResources().getDimension(R.dimen.records_table_cell_padding);
    int[] widths = { cbW, nameW, idW, facilityW, dateW, doneByW };
    for (int i = 0; i < cloudColumns.size(); i++) {
      ColumnDefinition def = cloudColumns.get(i);
      TextView tv = new TextView(ctx);
      TableRow.LayoutParams lp = new TableRow.LayoutParams(widths[i], rowHeight);
      tv.setLayoutParams(lp);
      tv.setPadding(pad, 0, pad, 0);
      tv.setBackground(ContextCompat.getDrawable(ctx, R.drawable.thin_white_border));
      tv.setText(def.title); // blank for checkbox
      tv.setGravity(def.gravity);
      tv.setTextColor(ContextCompat.getColor(ctx, R.color.white));
      tv.setTextSize(14);
      tv.setTypeface(tv.getTypeface(), android.graphics.Typeface.BOLD);
      tv.setId(def.fieldId);
      tv.setOnClickListener(this);
      if(i != 0)
      {
        lstCloudColumHeadersReference.add(tv);
      }
      cloudHeaderRow.addView(tv);
    }
    cloudHeaderRow.setBackgroundColor(ContextCompat.getColor(ctx, R.color.av_darkest_blue));
  }

  @Override
  public void onDestroyView() {
    super.onDestroyView();
    if (networkConnectivityLiveData != null) {
      networkConnectivityLiveData.removeObservers(getViewLifecycleOwner());
      networkConnectivityLiveData = null;
    }
    deRegisterUsbReceiver();
    if(dialog != null && dialog.isShowing())
    {
      dialog.dismiss();
    }
    if(exptFragment != null && exptFragment.getDialog() != null && exptFragment.getDialog().isShowing())
    {
      exptFragment.getDialog().dismiss();
    }
    if(patientFilterDialog != null && patientFilterDialog.isShowing())
    {
      patientFilterDialog.dismiss();
    }
    if(dateFilterDialog != null && dateFilterDialog.isShowing())
    {
      dateFilterDialog.dismiss();
    }
  }

  private void setupPaginationScrollListener() {
    if (recordsTableView == null)
      return;

    LinearLayoutManager layoutManager = (LinearLayoutManager) recordsTableView.getLayoutManager();
    recordsTableView.addOnScrollListener(new RecyclerView.OnScrollListener() {
      @Override
      public void onScrolled(@NonNull RecyclerView recyclerView, int dx, int dy) {
        super.onScrolled(recyclerView, dx, dy);

        if (layoutManager != null && isCloudMode && !isLoadingLocalPage) {
          int lastVisiblePosition = layoutManager.findLastVisibleItemPosition();
          int totalItemCount = cloudRecordsAdapter.getItemCount();

          // Load next page when user scrolls near the end (3 items before the end)
          if (lastVisiblePosition >= totalItemCount - 3) {
            int nextOffset = currentPageOffset + PAGE_SIZE;

            // Check if there are more records to load
            if (nextOffset < totalLocalRecords) {
              currentPageOffset = nextOffset;
              getCloudRecordsAsync(newRecords -> {
                // Append new records to existing list
                cloudRecordsRows.addAll(newRecords);
                cloudRecordsAdapter.update(cloudRecordsRows);
                Log.d("Pagination", "Loaded cloud page offset: " + currentPageOffset);
              });
            }
          }
        }
      }
    });
  }

  /**
   * Setup Select All checkbox functionality for both local and cloud modes
   */
  private void setupSelectAllCheckbox() {
    if (localSelectAllCheckbox != null) {
      localSelectAllCheckbox.setOnCheckedChangeListener((buttonView, isChecked) -> {
        if (!isCloudMode && recordsAdapter != null) {
          recordsAdapter.selectAll(isChecked);
          updateSelectionUI();
        }
      });
    }
  }

  /**
   * Update the selection UI elements (count text and delete button visibility)
   */
  public void updateSelectionUI() {
    int selectedCount = 0;

    if (!isCloudMode && recordsAdapter != null) {
      selectedCount = recordsAdapter.getSelectedRecords().size();
    }

    // Update selection count text
    if (selectionCountText != null) {
      if (selectedCount > 0) {
        selectionCountText.setText(selectedCount + " selected");
        selectionCountText.setVisibility(View.VISIBLE);
      } else {
        selectionCountText.setVisibility(View.GONE);
      }
    }

    // Show/hide delete button based on selection
    if (deleteButton != null && deleteButtonLabel != null) {
      if (selectedCount > 0) {
        deleteButton.setVisibility(View.VISIBLE);
        deleteButtonLabel.setVisibility(View.VISIBLE);
      } else {
        deleteButton.setVisibility(View.GONE);
        deleteButtonLabel.setVisibility(View.GONE);
      }
    }

    // Update Select All checkbox state
    updateSelectAllCheckboxState();
  }

  /**
   * Update the Select All checkbox state based on current selections
   */
  private void updateSelectAllCheckboxState() {
      if (localSelectAllCheckbox != null && recordsAdapter != null) {
        int totalRecords = recordsAdapter.getItemCount();
        int selectedCount = recordsAdapter.getSelectedRecords().size();

        // Temporarily remove listener to avoid infinite loop
        localSelectAllCheckbox.setOnCheckedChangeListener(null);

        if (selectedCount == 0) {
          localSelectAllCheckbox.setChecked(false);
        } else if (selectedCount == totalRecords && totalRecords > 0) {
          localSelectAllCheckbox.setChecked(true);
        } else {
          // Indeterminate state (some selected) - for now just uncheck
          localSelectAllCheckbox.setChecked(false);
        }
        // Re-attach listener
        setupSelectAllCheckbox();
      }

  }

  /**
   * Show confirmation dialog before deleting selected records
   */
  private void showDeleteConfirmationDialog() {
    int selectedCount = (recordsAdapter != null ? recordsAdapter.getSelectedRecords().size() : 0);

    if (selectedCount == 0) {
      Toast.makeText(requireContext(), "No records selected", Toast.LENGTH_SHORT).show();
      return;
    }

    String message = "Are you sure you want to delete " + selectedCount +
            " selected record" + (selectedCount > 1 ? "s" : "") + "?";

    String title =  "Delete Local Records";

    final MaterialAlertDialogBuilder builder = new MaterialAlertDialogBuilder(requireActivity());
    builder.setTitle(title);
    builder.setMessage(message);
    builder.setPositiveButton("Delete", (dialog, id) -> {
      // Check storage mode and call appropriate delete method
      if (storageManager.getStorageMode() == IStorageManager.StorageMode.OFFLINE) {
        deleteSelectedLocalRecordsOffline();
      } else {
        deleteSelectedLocalRecords();
      }
    });
    builder.setNegativeButton("Cancel", (dialog, id) -> dialog.dismiss());
    builder.show();
  }

  /**
   * Delete selected local records
   * Only allows deletion of records with "Synced" status in local tab
   */
  private void deleteSelectedLocalRecords() {
    if (recordsAdapter == null) return;

    List<EncounterTableRow> selectedRecords = new ArrayList<>(recordsAdapter.getSelectedRecords());

    if (selectedRecords.isEmpty()) {
      Toast.makeText(requireContext(), "No records selected", Toast.LENGTH_SHORT).show();
      return;
    }

    // Filter records: count synced vs pending
    int syncedCount = 0;
    int pendingCount = 0;
    for (EncounterTableRow record : selectedRecords) {
      if (Objects.equals(record.syncStatus, "Synced")) {
        syncedCount++;
      } else {
        pendingCount++;
      }
    }

    // If there are pending records, show warning
    if (pendingCount > 0) {
      String message = pendingCount + " pending record(s) cannot be deleted. " +
              (syncedCount > 0 ? "Only synced records will be deleted." : "");
      Toast.makeText(requireContext(), message, Toast.LENGTH_LONG).show();

      // If no synced records to delete, return early
      if (syncedCount == 0) {
        return;
      }
    }

    // Delete records in background thread
    Executors.newSingleThreadExecutor().execute(() -> {
      EncounterRepository repository = new EncounterRepository(db);
      int deletedCount = 0;

      for (EncounterTableRow record : selectedRecords) {
        try {
          // Only delete synced records in local tab
          if (record.encounterId > 0 && Objects.equals(record.syncStatus, "Synced")) {
            repository.deleteEncounterByid(record.encounterId);
            deletedCount++;

            // Delete associated files if dataDirPath exists
            if (record.dataDirPath != null && !record.dataDirPath.isEmpty()) {
              File recordDir = new File(record.dataDirPath);
              if (recordDir.exists() && recordDir.isDirectory()) {
                deleteDirectory(recordDir);
              }
            }
          }
        } catch (Exception e) {
          Log.e("DeleteRecord", "Error deleting record: " + record.patientName, e);
        }
      }

      final int finalDeletedCount = deletedCount;

      // Update UI on main thread
      new Handler(Looper.getMainLooper()).post(() -> {
        // Refresh the records list
        getRecordsAsync(records -> {
          localRecordsRows = records;
          recordsAdapter.updateSavedProcedures(records);
          recordsAdapter.clearSelection(); // Clear selection after deletion
          updateSelectionUI();

          MedDevLog.audit("Report", "Deleted " + finalDeletedCount + " procedure record(s)");
          Toast.makeText(requireContext(),
                  finalDeletedCount + " record(s) deleted successfully",
                  Toast.LENGTH_SHORT).show();

          tvEmptyText.setVisibility(records.isEmpty() ? View.VISIBLE : View.GONE);
        });
      });
    });
  }

  /**
   * Delete selected local records in OFFLINE mode
   * Deletes all selected records without checking sync status
   */
  private void deleteSelectedLocalRecordsOffline() {
    if (recordsAdapter == null) return;

    List<EncounterTableRow> selectedRecords = new ArrayList<>(recordsAdapter.getSelectedRecords());

    if (selectedRecords.isEmpty()) {
      Toast.makeText(requireContext(), "No records selected", Toast.LENGTH_SHORT).show();
      return;
    }

    // Delete records in background thread
    Executors.newSingleThreadExecutor().execute(() -> {
      EncounterRepository repository = new EncounterRepository(db);
      int deletedCount = 0;

      for (EncounterTableRow record : selectedRecords) {
        try {
          // In offline mode, delete all selected records regardless of sync status
          if (record.encounterId > 0) {
            repository.deleteEncounterByid(record.encounterId);
            deletedCount++;

            // Delete associated files if dataDirPath exists
            if (record.dataDirPath != null && !record.dataDirPath.isEmpty()) {
              File recordDir = new File(record.dataDirPath);
              if (recordDir.exists() && recordDir.isDirectory()) {
                deleteDirectory(recordDir);
              }
            }
          }
        } catch (Exception e) {
          Log.e("DeleteRecord", "Error deleting record: " + record.patientName, e);
        }
      }

      final int finalDeletedCount = deletedCount;

      // Update UI on main thread
      new Handler(Looper.getMainLooper()).post(() -> {
        // Refresh the records list
        getRecordsAsync(records -> {
          localRecordsRows = records;
          recordsAdapter.updateSavedProcedures(records);
          recordsAdapter.clearSelection(); // Clear selection after deletion
          updateSelectionUI();

          MedDevLog.audit("Report", "Deleted " + finalDeletedCount + " procedure record(s) (offline mode)");
          Toast.makeText(requireContext(),
                  finalDeletedCount + " record(s) deleted successfully",
                  Toast.LENGTH_SHORT).show();

          tvEmptyText.setVisibility(records.isEmpty() ? View.VISIBLE : View.GONE);
        });
      });
    });
  }

  /**
   * Recursively delete a directory and its contents
   */
  private void deleteDirectory(File directory) {
    ExportUtility.deleteDirectory(directory);
  }

  private File zipDirectory(File zipLocation,List<File> includeDirectories, String zipDirName)
  {
    zippedFile = ExportUtility.zipDirectory(zipLocation, includeDirectories, zipDirName);
    if (zippedFile == null)
    {
      progressVisibility(false);
    }
    return zippedFile;
  }

  private void exportDataToUsb(Uri usbTreeUri)
  {
    new Thread(() ->
    {
      try
      {
        DocumentFile root = DocumentFile.fromTreeUri(requireContext(), usbTreeUri);
        if (root == null || !root.canWrite())
        {
          if(!transferState.isUsbPluggedIn())
          {
            requireActivity().runOnUiThread(() -> CustomToast.show(requireContext(), getResources().getString(R.string.sys_backup_folder_write_err), CustomToast.Type.ERROR));
          }
          return;
        }
        String stamp = new java.text.SimpleDateFormat(Utility.FILE_NAME_STAMP_FORMAT, java.util.Locale.US).format(new java.util.Date());
        DocumentFile backupDir = root.createDirectory("PROC_"+Utility.BACKUP_FILE_PREFIX + stamp);
        if (backupDir == null)
        {
          if(!transferState.isUsbPluggedIn())
          {
            requireActivity().runOnUiThread(() -> CustomToast.show(requireContext(), getResources().getString(R.string.sys_backup_cannot_create_folder), CustomToast.Type.ERROR));
          }
          return;
        }
        int copied = 0;
        if (ExportUtility.copyFileToUsbStorage(requireContext(), backupDir, zippedFile, "application/octet-stream"))
        {
          copied++;
          int finalCopied = copied;
          transferState.setTransferComplete(true);
          MedDevLog.audit("Report Export", "Procedure report(s) exported to external storage");
          requireActivity().runOnUiThread(new Runnable()
          {
            @Override
            public void run()
            {
              dialog = ConfirmDialog.singleActionPopup(requireContext(),getResources().getString(R.string.export_success_mesg),"OK", confirmed -> {
                recordsAdapter.clearSelection();
                progressVisibility(false);
                hideExportProgress();
                trashZipExportFile();
              },ExportUtility.popupWidth,ExportUtility.popupHeight);
              showDialogbox(dialog);
            }
          });
        }
        else
        {
          transferState.setTransferComplete(false);
          progressVisibility(false);
          hideExportProgress();
          requireActivity().runOnUiThread(() -> CustomToast.show(requireContext(), getResources().getString(R.string.zip_copy_failed), CustomToast.Type.ERROR));
        }
      }
      catch (Exception e)
      {
        transferState.setTransferComplete(false);
        progressVisibility(false);
        hideExportProgress();
        trashZipExportFile();
        requireActivity().runOnUiThread(() -> CustomToast.show(requireContext(), getResources().getString(R.string.sys_backup_failed) + e.getMessage(), CustomToast.Type.ERROR));
      }
    }).start();
  }

  private void initializeUsbReceiver()
  {
    usbReceiver = ExportUtility.createUsbReceiver(transferState,
            new ExportUtility.UsbReceiverCallback() {
              @Override
              public void onUsbAttached() {}

              @Override
              public void onUsbDetached()
              {
                progressVisibility(false);
                transferState.setUsbPluggedIn(false);
                hideExportProgress();
                if (!transferState.isTransferComplete())
                {
                  dialog = ConfirmDialog.singleActionPopup(requireContext(), getResources().getString(R.string.export_transfer_interrupt_mesg), "OK", confirmed -> {}, ExportUtility.popupWidth, ExportUtility.popupHeight);
                  showDialogbox(dialog);
                }
              }
            });
  }

  public ActivityResultLauncher<Intent> instantiateBackupIntent()
  {
    return ExportUtility.instantiateBackupIntent(this, this::exportDataToUsb);
  }

  private void initiateExport()
  {
    transferState.setTransferComplete(false);
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

  private void showDialogbox(Dialog dialogbox)
  {
    if(dialogbox != null)
    {
      dialogbox.dismiss();
    }
    dialogbox.show();
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

  @Override
  public void onPause() {
    super.onPause();
    deRegisterUsbReceiver();
  }

  @Override
  public void onResume()
  {
    super.onResume();
    registerUsbReceiver();
  }

  public void progressVisibility(boolean visibility)
  {
    if(exptFragment != null && exptFragment.getView() != null)
    {
      if(visibility)
      {
        exptFragment.getView().findViewById(R.id.progress_layout).setVisibility(View.VISIBLE);
      }
      else
      {
        exptFragment.getView().findViewById(R.id.progress_layout).setVisibility(View.GONE);
      }
    }
  }

  public void hideExportProgress()
  {
    requireActivity().runOnUiThread(new Runnable()
    {
      @Override
      public void run()
      {
        if(exptFragment != null && exptFragment.getView() != null)
        {
          exptFragment.getView().findViewById(R.id.export_quit_button).performClick();
        }
      }
    });
  }

  private void trashZipExportFile()
  {
    try
    {
      if(zippedFile != null && zippedFile.exists())
      {
        java.nio.file.Files.delete(zippedFile.toPath());
      }
    }
    catch (IOException e)
    {
      e.printStackTrace();
    }
  }

  private void initializeSortIcons(RecyclerView.Adapter adapterType,FragmentRecordsBinding binding)
  {
    if(adapterType != null && binding != null)
    {
      if(adapterType instanceof RecordsAdapter)
      {
        recordsAdapter.getSortingInstance().toggleSortIndicatorIcon(binding.tvPatientNameHeader,false);
        recordsAdapter.getSortingInstance().toggleSortIndicatorIcon(binding.tvPatientIdHeader,false);
        recordsAdapter.getSortingInstance().toggleSortIndicatorIcon(binding.tvPatientDobHeader,false);
        recordsAdapter.getSortingInstance().toggleSortIndicatorIcon(binding.tvInsertionDateHeader,false);
        recordsAdapter.getSortingInstance().toggleSortIndicatorIcon(binding.tvDoneByHeader,false);
        recordsAdapter.getSortingInstance().toggleSortIndicatorIcon(binding.tvFacilityHeader,false);
      }
    }
  }

  private void initializeCloudSortIcons(List<TextView> cloudColumnHeader)
  {
    if(cloudRecordsAdapter != null && cloudColumnHeader != null)
    {
      for(TextView textField : cloudColumnHeader)
      {
        cloudRecordsAdapter.getSortingInstance().toggleSortIndicatorIcon(textField,false);
      }
    }
  }

  @Override
  public void onClick(View viewId)
  {
    switch (viewId.getId())
    {
      case R.id.tvPatientNameHeader:
        recordsAdapter.sortFields(SortMode.Name,binding.tvPatientNameHeader);
        break;
      case R.id.tvPatientIdHeader:
        recordsAdapter.sortFields(SortMode.Name,binding.tvPatientIdHeader);
        break;
      case R.id.tvPatientDobHeader:
        recordsAdapter.sortFields(SortMode.Date,binding.tvPatientDobHeader);
        break;
      case R.id.tvInsertionDateHeader:
        recordsAdapter.sortFields(SortMode.Date,binding.tvInsertionDateHeader);
        break;
      case R.id.tvDoneByHeader:
        recordsAdapter.sortFields(SortMode.Name,binding.tvDoneByHeader);
        break;
      case R.id.tvFacilityHeader:
        recordsAdapter.sortFields(SortMode.Name,binding.tvFacilityHeader);
        break;
      case SortComparators.CLOUD_COLUM_PATIENT_NAME:
        cloudRecordsAdapter.sortFields(SortMode.Name,(TextView)viewId);
        break;
      case SortComparators.CLOUD_COLUM_PATIENT_ID:
        cloudRecordsAdapter.sortFields(SortMode.Name,(TextView)viewId);
        break;
      case SortComparators.CLOUD_COLUM_DATE:
        cloudRecordsAdapter.sortFields(SortMode.Date,(TextView)viewId);
        break;
      case SortComparators.CLOUD_COLUM_DONE_BY:
        cloudRecordsAdapter.sortFields(SortMode.Name,(TextView)viewId);
        break;
      case SortComparators.CLOUD_COLUM_FACILITY:
        cloudRecordsAdapter.sortFields(SortMode.Name,(TextView)viewId);
        break;
      default:
        return;
    }
  }

  private void showFilters()
  {
    filteredName = "";
    filteredId = "";
    selectedFacility = "";
    filteredFacilityId = 0;
    filteredFacility = "";
    filteredInserter = "";
    filteredDob = "";
    filteredFromDob = "";
    filteredToDob = "";
    filteredInsertionDate = "";
    filteredFromInsertionDate = "";
    filteredToInsertionDate = "";
    patientFilterDialog = ConfirmDialog.patientFilterPopup(patientFilterDialog,requireContext());
    EditText edtPatientName = patientFilterDialog.findViewById(R.id.edtPatientName);
    EditText edtPatientId = patientFilterDialog.findViewById(R.id.edtPatientId);
    View inlcudeFacilityView = patientFilterDialog.findViewById(R.id.includeFacilityInput);
    AutoCompleteTextView edtFacilityInputDropdown = inlcudeFacilityView.findViewById(R.id.edtFacilityInput);
    FacilityRepository facilityDetailsRepository = new FacilityRepository(db.facilityDao());
    List<FacilityEntity> facilityEntities = DbHelper.getAllFacilities(facilityDetailsRepository);
    final String[] faves = new String[0];
    facilitySelectAdapter = new FacilitiesAutoCompleteAdapter(requireContext(), R.layout.facility_list_item,R.id.facility_list_item_text, facilityEntities, faves, true);
    edtFacilityInputDropdown.setThreshold(facilitySearchMinChar);
    edtFacilityInputDropdown.setAdapter(facilitySelectAdapter);
    edtFacilityInputDropdown.setOnItemClickListener(new AdapterView.OnItemClickListener()
    {
      @Override
      public void onItemClick(AdapterView<?> parent, View view, int position, long id)
      {
        if(facilitySelectAdapter != null)
        {
          Facility selected = facilitySelectAdapter.getItem(position);
          if(selected != null)
          {
            selectedFacility = selected.getFacilityName();
          }
        }
      }
    });
    edtFacilityInputDropdown.addTextChangedListener(new TextWatcher()
    {
      @Override
      public void afterTextChanged(Editable s)
      {
        selectedFacility = s.toString();
      }

      @Override
      public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

      @Override
      public void onTextChanged(CharSequence s, int start, int before, int count) {}
    });
    EditText edtInserterName = patientFilterDialog.findViewById(R.id.edtInserterName);
    TextInputLayout inputContainerDob = patientFilterDialog.findViewById(R.id.inputContainerDob);
    EditText edtDob = patientFilterDialog.findViewById(R.id.edtDob);
    TextInputLayout inputContainerInsertionDate = patientFilterDialog.findViewById(R.id.inputContainerInsertionDate);
    EditText edtInsertionDate = patientFilterDialog.findViewById(R.id.edtInsertionDate);
    // In cloud api as of now no filter available for patient id and inserter name, so hiding those fields in filter dialog for cloud mode
    patientFilterDialog.findViewById(R.id.inputLayoutPatientId).setVisibility((isCloudMode) ? View.INVISIBLE : View.VISIBLE);
    patientFilterDialog.findViewById(R.id.inputLayoutInserterName).setVisibility((isCloudMode) ? View.INVISIBLE : View.VISIBLE);

    inlcudeFacilityView.setVisibility(facilityEntities != null && !facilityEntities.isEmpty() ? View.VISIBLE : View.INVISIBLE);

    inputContainerDob.setEndIconOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        getPickedDates(DATEPICK_SOURCE_DOB,edtDob);
      }
    });
    inputContainerInsertionDate.setEndIconOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        getPickedDates(DATEPICK_SOURCE_INSERTER,edtInsertionDate);
      }
    });
    Button btnSearch = patientFilterDialog.findViewById(R.id.btnSearch);
    Button btnDismiss = patientFilterDialog.findViewById(R.id.btnDismiss);
    btnSearch.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        filteredName = edtPatientName.getText().toString().trim();
        filteredId = edtPatientId.getText().toString().trim();;
        filteredFacility = isFacilityAvailable(selectedFacility,facilityEntities) ? selectedFacility.trim() : "";
        filteredInserter = edtInserterName.getText().toString().trim();;
        filteredDob = edtDob.getText().toString().trim();;
        filteredInsertionDate = edtInsertionDate.getText().toString().trim();;
        patientFilterDialog.dismiss();
        JSONObject jsonObject = new JSONObject();
        try
        {
          jsonObject.put("filteredName", filteredName);
          jsonObject.put("filteredId", filteredId);
          jsonObject.put("filteredFacility", filteredFacility);
          jsonObject.put("filteredInserter", filteredInserter);
          jsonObject.put("filteredDob", filteredDob);
          jsonObject.put("filteredInsertionDate", filteredInsertionDate);
        }
        catch (JSONException e)
        {
          MedDevLog.info(TAG,""+e.getMessage());
        }
        if(isCloudMode)
        {
          currentPageOffset = 0;
          totalLocalRecords = 0;
          fetchCloudRecord(records -> {
            cloudRecordsRows = records;
            if(cloudRecordsAdapter != null)
            {
              cloudRecordsAdapter.update(cloudRecordsRows);
            }
          });
        }
        else
        {
          if(recordsAdapter != null)
          {
            recordsAdapter.getFilter().filter(jsonObject.toString());
          }
        }
      }
    });
    btnDismiss.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        patientFilterDialog.dismiss();
      }
    });
    if(isCloudMode)
    {
      if(cloudRecordsAdapter != null)
      {
        btnSearch.setVisibility((cloudRecordsAdapter.fetchActualCollection().isEmpty())? View.INVISIBLE : View.VISIBLE);
      }
    }
    else
    {
      if(recordsAdapter != null)
      {
        btnSearch.setVisibility((recordsAdapter.fetchActualCollection().isEmpty())? View.INVISIBLE : View.VISIBLE);
      }
    }
    showDialogbox(patientFilterDialog);
  }

  private void getPickedDates(int callSource,EditText edtDateField)
  {
    bufferDate.setLength(0);
    dateFilterDialog = ConfirmDialog.dateRangePopup(requireContext());
    DatePicker fromPickedDate = dateFilterDialog.findViewById(R.id.fromPickedDate);
    DatePicker toPickedDate = dateFilterDialog.findViewById(R.id.toPickedDate);
    fromPickedDate.setMaxDate(System.currentTimeMillis());
    toPickedDate.setMaxDate(System.currentTimeMillis());
    Button btnConfirm = dateFilterDialog.findViewById(R.id.btnConfirm);
    Button btnCancel = dateFilterDialog.findViewById(R.id.btnCancel);
    btnConfirm.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        bufferDate.append(String.format("%04d-%02d-%02d",fromPickedDate.getYear(),fromPickedDate.getMonth()+1,fromPickedDate.getDayOfMonth()));
        bufferDate.append("$");
        bufferDate.append(String.format("%04d-%02d-%02d",toPickedDate.getYear(),toPickedDate.getMonth()+1,toPickedDate.getDayOfMonth()));
        if(callSource == DATEPICK_SOURCE_DOB)
        {
          filteredDob = bufferDate.toString();
          edtDateField.setText(filteredDob.replaceAll("\\$"," | "));
          String dobDates[] = Utility.splitDate(edtDateField.getText().toString().trim(),"|");
          if(dobDates != null && dobDates.length == 2)
          {
            filteredFromDob = dobDates[0].trim();
            filteredToDob = dobDates[1].trim();
          }
        }
        else
        {
          filteredInsertionDate = bufferDate.toString();
          edtDateField.setText(filteredInsertionDate.replaceAll("\\$"," | "));
          String insertionDates[] = Utility.splitDate(edtDateField.getText().toString().trim(),"|");
          if(insertionDates != null && insertionDates.length == 2)
          {
            filteredFromInsertionDate = insertionDates[0].trim();
            filteredToInsertionDate = insertionDates[1].trim();
          }
        }
        dateFilterDialog.dismiss();
      }
    });
    btnCancel.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        dateFilterDialog.dismiss();
      }
    });
    showDialogbox(dateFilterDialog);
  }

  private void clearFilters(RecyclerView.Adapter adapterType)
  {
    if(adapterType != null)
    {
      if(adapterType instanceof RecordsAdapter)
      {
        recordsAdapter.clearFilter();
      }
      else
      {
        currentPageOffset = 0;
        totalLocalRecords = 0;
        applyClearFilters();
        fetchCloudRecord(records -> {;
          cloudRecordsRows = records;
          if(cloudRecordsAdapter != null)
          {
            cloudRecordsAdapter.update(cloudRecordsRows);
          }
        });
        //cloudRecordsAdapter.clearFilter();
      }
    }
  }

  private boolean isFilteringAllowed()
  {
    boolean isFilterable = false;
    if(isCloudMode)
    {
      isFilterable = (cloudRecordsAdapter != null && cloudRecordsAdapter.getSelectedRows() != null && cloudRecordsAdapter.getSelectedRows().size() > 0) ? false : true;
    }
    else
    {
      isFilterable = (recordsAdapter != null && recordsAdapter.getSelectedRecords() != null && recordsAdapter.getSelectedRecords().size() > 0) ? false : true;
    }
    return isFilterable;
  }

  private boolean isFacilityAvailable(String facilityName,List<FacilityEntity> dataList)
  {
    for(FacilityEntity facility : dataList)
    {
      if (facility.getFacilityName().equalsIgnoreCase(facilityName.trim()))
      {
        //for filtering facility in online mode use cloud db key colum
        if(isCloudMode)
        {
          if(facility.getCloudDbPrimaryKey() != null)
          {
            filteredFacilityId = facility.getCloudDbPrimaryKey().intValue();
            return true;
          }
        }
        //for filtering facility in offline mode use facility id colum
        filteredFacilityId = facility.getFacilityId().intValue();
        return true;
      }
    }
    return false;
  }

  private void fetchCloudRecord(Consumer<ArrayList<EncounterTableRow>> callback)
  {
    if (isLoadingLocalPage) {
      return; // Prevent concurrent requests
    }
    isLoadingLocalPage = true;

    try {
      String orgId = com.accessvascularinc.hydroguide.mma.utils.PrefsManager.getOrganizationId(
              requireContext());
      String userId = com.accessvascularinc.hydroguide.mma.utils.PrefsManager.getUserId(requireContext());

      if (orgId == null) {
        MedDevLog.warn("CloudFetch", "Organization ID not found; returning empty list");
        isLoadingLocalPage = false;
        new Handler(Looper.getMainLooper()).post(() -> callback.accept(new ArrayList<>()));
        return;
      }

      // Show loading state
      if (tvEmptyText != null) {
        tvEmptyText.setVisibility(View.GONE);
      }
      if (loadingIndicator != null) {
        loadingIndicator.setVisibility(View.VISIBLE);
      }

      // Use Hilt-injected ApiService (includes auth token interceptor)

      // Call paginated API endpoint for cloud records
      apiService.getProcedures(
              orgId,
              filteredName, // searchTerm
              filteredFromInsertionDate, // fromDate
              filteredToInsertionDate, // toDate
              currentPageOffset, // offset - pagination
              PAGE_SIZE, // limit - pagination
              filteredFacilityId, // facilityId
              "", // patientSide
              filteredFromDob, // dobFromDate
              filteredToDob, // dobToDate
              userId != null ? userId : "").enqueue(
              new retrofit2.Callback<com.accessvascularinc.hydroguide.mma.network.ProcedureSyncModel.ProcedureSyncResponse>() {
                @Override
                public void onResponse(
                        retrofit2.Call<com.accessvascularinc.hydroguide.mma.network.ProcedureSyncModel.ProcedureSyncResponse> call,
                        retrofit2.Response<com.accessvascularinc.hydroguide.mma.network.ProcedureSyncModel.ProcedureSyncResponse> response) {
                  ArrayList<EncounterTableRow> rows = new ArrayList<>();
                  try {
                    if (response.isSuccessful() && response.body() != null &&
                            response.body().getData() != null) {
                      for (com.accessvascularinc.hydroguide.mma.network.ProcedureSyncModel.ProcedureSyncResponse.ProcedureData pd : response
                              .body().getData()) {
                        EncounterTableRow row = new EncounterTableRow();
                        if (pd.patient != null) {
                          row.patientName = pd.patient.patientName;
                          // Use patientMrn as ID (matches screenshot)
                          row.patientId = pd.patient.patientMrn;
                          row.patientDob = pd.patient.patientDob;
                        }
                        row.facilityName = pd.facilityOrLocation;
                        // Prefer clinicianEmail; fallback to clinicianId
                        row.createdBy = pd.clinicianEmail != null ? pd.clinicianEmail : pd.clinicianId;
                        row.createdOn = String.valueOf(parseStartTime(pd.startTime));
                        row.syncStatus = "Synced"; // Cloud records are already synced
                        row.fileName = pd.fileUrl;
                        rows.add(row);
                      }
                      Log.d("CloudFetch",
                              "Fetched " + rows.size() + " cloud procedures (offset: " + currentPageOffset + ")");

                      // Fetch total count only on first page if not already fetched
                      if (currentPageOffset == 0 && totalLocalRecords == 0) {
                        Log.d("CloudFetch", "Initiating total count fetch...");
                        fetchTotalCloudRecordCount(orgId, userId);
                      } else {
                        Log.d("CloudFetch", "Skipping count fetch - offset: " + currentPageOffset + ", totalRecords: "
                                + totalLocalRecords);
                      }
                    } else {
                      MedDevLog.warn("CloudFetch", "Unsuccessful response: " + response.code());
                    }
                  } catch (Exception e) {
                    MedDevLog.error("CloudFetch", "Error processing response", e);
                  }
                  new Handler(Looper.getMainLooper()).post(() -> {
                    isLoadingLocalPage = false;
                    if (loadingIndicator != null) {
                      loadingIndicator.setVisibility(View.GONE);
                    }
                    if (tvEmptyText != null) {
                      tvEmptyText.setVisibility(rows.isEmpty() ? View.VISIBLE : View.GONE);
                    }
                    callback.accept(rows);
                  });
                }

                @Override
                public void onFailure(
                        retrofit2.Call<com.accessvascularinc.hydroguide.mma.network.ProcedureSyncModel.ProcedureSyncResponse> call,
                        Throwable t) {
                  MedDevLog.error("CloudFetch", "Failed to fetch cloud procedures", t);
                  new Handler(Looper.getMainLooper()).post(() -> {
                    isLoadingLocalPage = false;
                    if (loadingIndicator != null) {
                      loadingIndicator.setVisibility(View.GONE);
                    }
                    if (tvEmptyText != null) {
                      tvEmptyText.setText("Failed to load cloud records");
                      tvEmptyText.setVisibility(View.VISIBLE);
                    }
                    callback.accept(new ArrayList<>());
                  });
                }
              });
    } catch (Exception e) {
      MedDevLog.error("CloudFetch", "Unexpected error starting cloud fetch", e);
      isLoadingLocalPage = false;
      new Handler(Looper.getMainLooper()).post(() -> {
        if (loadingIndicator != null) {
          loadingIndicator.setVisibility(View.GONE);
        }
        callback.accept(new ArrayList<>());
      });
    }
  }

  private void applyClearFilters()
  {
    filteredName = "";
    filteredFromInsertionDate = "";
    filteredToInsertionDate = "";
    filteredFacilityId = 0;
    filteredFromDob = "";
    filteredToDob = "";
  }
}