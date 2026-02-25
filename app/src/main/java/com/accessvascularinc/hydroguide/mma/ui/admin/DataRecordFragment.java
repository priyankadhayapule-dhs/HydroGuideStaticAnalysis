package com.accessvascularinc.hydroguide.mma.ui.admin;

import android.os.Bundle;
import android.os.Handler;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.graphics.Color;
import android.graphics.PorterDuff;
import android.widget.ImageView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.widget.SearchView;
import androidx.databinding.DataBindingUtil;
import androidx.lifecycle.ViewModelProvider;
import androidx.navigation.Navigation;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.adapters.DataRecordsAdapter;
import com.accessvascularinc.hydroguide.mma.databinding.FragmentDataRecordsBinding;
import com.accessvascularinc.hydroguide.mma.db.HydroGuideDatabase;
import com.accessvascularinc.hydroguide.mma.db.di.DatabaseModule;
import com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianApiModel;

import com.accessvascularinc.hydroguide.mma.model.DataRecords;
import com.accessvascularinc.hydroguide.mma.model.SortMode;
import com.accessvascularinc.hydroguide.mma.ui.BaseHydroGuideFragment;
import com.accessvascularinc.hydroguide.mma.ui.MainViewModel;
import com.accessvascularinc.hydroguide.mma.utils.DbHelper;
import com.accessvascularinc.hydroguide.mma.utils.PrefsManager;
import com.accessvascularinc.hydroguide.mma.utils.RecordsMode;
import com.accessvascularinc.hydroguide.mma.utils.ScreenRoute;
import com.accessvascularinc.hydroguide.mma.utils.Utility;
import com.accessvascularinc.hydroguide.mma.storage.IStorageManager;
import com.accessvascularinc.hydroguide.mma.utils.NetworkConnectivityLiveData;

import dagger.hilt.android.AndroidEntryPoint;
import javax.inject.Inject;
import com.google.gson.Gson;

import com.accessvascularinc.hydroguide.mma.ui.settings.BrightnessChangeFragment;
import com.accessvascularinc.hydroguide.mma.ui.settings.VolumeChangeFragment;

import java.util.ArrayList;
import java.util.List;

@AndroidEntryPoint
public class DataRecordFragment extends BaseHydroGuideFragment
    implements DataRecordsAdapter.DataRecordsClickListener, View.OnClickListener {
  private FragmentDataRecordsBinding binding;
  private SortMode sortMode = SortMode.None;
  private RecyclerView recDataRecords;
  private SearchView searchView;
  private DataRecordsAdapter adapter;
  private List<Object> lstDataRecords = new ArrayList<>();
  private ScreenRoute screenRoute;
  private RecordsMode recordsMode = RecordsMode.CREATE;
  private boolean isSortedByAlphabet, isSortedByDate;
  private HydroGuideDatabase database;

  @Inject
  IStorageManager storageManager;

  private NetworkConnectivityLiveData networkConnectivityLiveData;
  private boolean isNetworkConnected = false;
  private boolean isFetchingClinicians = false;
  private boolean isFetchingFacilities = false;

  @Nullable
  @Override
  public View onCreateView(@NonNull final LayoutInflater inflater,
      @Nullable final ViewGroup container,
      @Nullable final Bundle savedInstanceState) {
    binding = DataBindingUtil.inflate(inflater, R.layout.fragment_data_records, container, false);
    viewmodel = new ViewModelProvider(requireActivity()).get(MainViewModel.class);
    database = new DatabaseModule().provideDatabase(requireContext());
    binding.includeTopbar.setTabletState(viewmodel.getTabletState());
    tvBattery = binding.includeTopbar.controllerBatteryLevelPct;
    ivBattery = binding.includeTopbar.remoteBatteryLevelIndicator;
    ivUsbIcon = binding.includeTopbar.usbIcon;
    ivTabletBattery = binding.includeTopbar.tabletBatteryIndicator.tabletBatteryLevelIndicator;
    tvTabletBattery = binding.includeTopbar.tabletBatteryIndicator.tabletBatteryLevelPct;
    // Topbar search field, clear icon reference
    searchView = binding.includeTopbar.includeSearch.searchField;
    int searchCloseButtonId = searchView.findViewById(androidx.appcompat.R.id.search_close_btn).getId();
    ImageView closeButton = searchView.findViewById(searchCloseButtonId);
    closeButton.setOnClickListener(searchFieldClearListener());
    binding.includeTopbar.includeSearch.searchField.setOnQueryTextListener(searchListener());

    tvBattery.setVisibility(View.INVISIBLE);
    ivBattery.setVisibility(View.INVISIBLE);
    ivUsbIcon.setVisibility(View.INVISIBLE);
    binding.includeTopbar.controllerBatteryLabel.setVisibility(View.INVISIBLE);

    binding.includeTopbar.includeSearch.getRoot().setVisibility(View.VISIBLE);
    binding.inclBottomBar.lnrBack.setVisibility(View.GONE);
    binding.inclBottomBar.lnrResetPassword.setVisibility(View.GONE);
    binding.inclBottomBar.lnrRemove.setVisibility(View.GONE);
    binding.inclBottomBar.lnrSave.setVisibility(View.GONE);
    binding.inclBottomBar.lnrExport.setVisibility(View.VISIBLE);
    binding.inclBottomBar.lnrHome.setVisibility(View.VISIBLE);
    binding.inclBottomBar.lnrVolume.setVisibility(View.VISIBLE);
    binding.inclBottomBar.lnrBrightness.setVisibility(View.VISIBLE);
    binding.inclBottomBar.lnrVolume.setOnClickListener(this);
    binding.inclBottomBar.lnrBrightness.setOnClickListener(this);

    setupVolumeAndBrightnessListeners();
    binding.includeTopbar.setControllerState(viewmodel.getControllerState());
    binding.inclBottomBar.lnrHome.setOnClickListener(this);

    Bundle bundl = getArguments();
    String callSource = bundl.getString(
        getResources().getString(R.string.dashboard_action_argument_call_source));

    // Determine screen route based on the source of the call (Clinician or Facility
    // Records).
    // Note: This compares the callSource against a string resource. Consider
    // whether this should
    // use an enum constant instead of calling getResources().getString() in the
    // comparison
    // to avoid repeated resource lookups and improve performance/readability.
    screenRoute = callSource.equalsIgnoreCase(
        getResources().getString(R.string.caption_clinician_records)) ? ScreenRoute.CLINICIAN_RECORDS
            : ScreenRoute.FACILITY_RECORDS;

    adapter = new DataRecordsAdapter(this, lstDataRecords, screenRoute);
    binding.recDataRecords.setLayoutManager(new LinearLayoutManager(getContext()));
    binding.recDataRecords.setAdapter(adapter);

    binding.inclTaskbar.lnrSortByName.setOnClickListener(this);
    binding.inclTaskbar.lnrSortByDate.setOnClickListener(this);

    // Setup network connectivity listener for Clinician Records
    if (screenRoute == ScreenRoute.CLINICIAN_RECORDS) {
      networkConnectivityLiveData = new NetworkConnectivityLiveData(requireContext());
      networkConnectivityLiveData.observe(getViewLifecycleOwner(), isConnected -> {
        isNetworkConnected = isConnected != null && isConnected;
        updateScreenStateForClinicianRecords();
      });
    }

    // Data loading is handled in onResume() to ensure it always runs
    // when the fragment becomes visible (initial load, back navigation, etc.)
    return binding.getRoot();
  }

  private SearchView.OnQueryTextListener searchListener() {
    return new SearchView.OnQueryTextListener() {
      @Override
      public boolean onQueryTextChange(String newText) {
        if (newText.trim().isEmpty()) {
          // Use synchronous refreshList() instead of async filter("")
          // to avoid race condition with data loading callbacks
          adapter.refreshList();
        } else {
          adapter.getFilter().filter(newText);
        }
        return false;
      }

      @Override
      public boolean onQueryTextSubmit(String query) {
        adapter.getFilter().filter(query);
        return false;
      }

    };
  }

  private View.OnClickListener searchFieldClearListener() {
    return new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        searchView.setQuery("", false);
        adapter.refreshList();
      }
    };
  }

  private void manageClinicianWigets() {
    binding.includeTopbar.setTitle(getResources().getString(R.string.caption_clinician_records));
    binding.inclBottomBar.lnrAddClinician.setOnClickListener(this);

    // Check if clinician records are accessible (requires internet in ONLINE mode)
    boolean isAccessible = isClinicianRecordsAccessible();

    if (!isAccessible) {
      // Not accessible - show message, hide table and buttons, CLEAR data
      binding.inclBottomBar.lnrAddClinician.setVisibility(View.GONE);
      binding.inclBottomBar.lnrExport.setVisibility(View.GONE);
      binding.recordsTable.setVisibility(View.GONE);
      binding.tvNoRecordsFound.setVisibility(View.VISIBLE);
      binding.tvNoRecordsFound.setTextSize(22);
      binding.tvNoRecordsFound.setText("User Management needs active Internet connection");

      // Clear cached data to prevent old records from showing
      lstDataRecords.clear();
      adapter.refreshList();

      hideLoader();
      isFetchingClinicians = false;
      return; // Exit early - don't fetch data
    }

    // Prevent concurrent fetches — if already fetching, skip
    if (isFetchingClinicians) {
      return;
    }
    isFetchingClinicians = true;

    // Accessible - proceed with fetching data
    binding.inclBottomBar.lnrAddClinician.setVisibility(View.VISIBLE);
    binding.inclBottomBar.lnrExport.setVisibility(View.VISIBLE);

    // Show loader, hide table, hide message
    showLoader();
    binding.recordsTable.setVisibility(View.GONE);
    binding.tvNoRecordsFound.setVisibility(View.GONE);

    lstDataRecords.clear();

    // Fetch clinician data directly from storage manager
    String orgId = com.accessvascularinc.hydroguide.mma.utils.PrefsManager.getOrganizationId(
        requireContext());

    storageManager.getAllOrganizationUsers(orgId, (clinicians, errorMessage) -> {
      isFetchingClinicians = false;

      if (!isAdded() || binding == null) {
        return;
      }

      // CRITICAL: Check if clinician records are still accessible before updating UI
      boolean isStillAccessible = isClinicianRecordsAccessible();

      if (!isStillAccessible) {
        return; // Don't update UI if not accessible
      }

      if (clinicians != null) {
        // Get the currently logged-in user ID
        String loggedInUserId = PrefsManager.getUserId(requireContext());

        // Filter out the logged-in user from the clinicians list
        for (com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianApiModel clinician : clinicians) {
          if (loggedInUserId == null || !loggedInUserId.equals(clinician.getUserId())) {
            lstDataRecords.add(clinician);
          }
        }
      }

      hasRecords();
      // refreshList() auto-applies sort if one is active
      adapter.refreshList();
      adapter.fetchActualCollection(lstDataRecords);

      // Hide loader and show table
      hideLoader();
      if (binding != null) {
        binding.recordsTable.setVisibility(View.VISIBLE);
      }
    });
  }

  private void manageFacilityWigets() {
    binding.includeTopbar.setTitle(getResources().getString(R.string.caption_facility_records));
    binding.inclBottomBar.lnrAddFacility.setVisibility(View.VISIBLE);
    binding.inclBottomBar.lnrAddFacility.setOnClickListener(this);

    // Prevent concurrent fetches
    if (isFetchingFacilities) {
      return;
    }
    isFetchingFacilities = true;

    lstDataRecords.clear();
    String orgId = PrefsManager.getOrganizationId(
        requireContext());

    storageManager.getAllFacilities(orgId, (facilities, errorMessage) -> {
      isFetchingFacilities = false;

      if (!isAdded() || binding == null) {
        return;
      }

      if (facilities != null) {
        lstDataRecords.addAll(facilities);
      } else {
        if (errorMessage != null && !errorMessage.isEmpty()) {
          Toast.makeText(requireContext(), errorMessage, Toast.LENGTH_SHORT).show();
        }
      }

      hasRecords();
      // refreshList() auto-applies sort if one is active
      adapter.refreshList();
      adapter.fetchActualCollection(lstDataRecords);
    });
  }

  @Override
  public void onDataRecordClick(int position) {
    recordsMode = RecordsMode.EDIT;
    Bundle argSelectedRecord = new Bundle();
    argSelectedRecord.putString(getResources().getString(R.string.caption_record_type_mode),
        getResources().getString(R.string.caption_record_type_mode_edit));
    Object selected = adapter.getFilteredItem(position);
    if (selected == null) return;
    if (screenRoute == ScreenRoute.CLINICIAN_RECORDS &&
        selected instanceof com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianApiModel) {
      com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianApiModel clinician = (com.accessvascularinc.hydroguide.mma.network.ClinicianModel.ClinicianApiModel) selected;
      argSelectedRecord.putString("clinician_id", clinician.getUserId());
      argSelectedRecord.putString(
          getResources().getString(R.string.dashboard_action_argument_call_source),
          getResources().getString(R.string.caption_clinician_records));
      navigateToScreen(R.id.nav_to_clinicians_manager, argSelectedRecord);
    } else if (screenRoute == ScreenRoute.FACILITY_RECORDS) {
      argSelectedRecord.putString(getResources().getString(R.string.caption_selected_record_data),
          new Gson().toJson(selected));
      argSelectedRecord.putString(
          getResources().getString(R.string.dashboard_action_argument_call_source),
          getResources().getString(R.string.caption_facility_records));
      navigateToScreen(R.id.nav_to_manage_facilities, argSelectedRecord);
    }
  }

  @Override
  public void onClick(View view) {
    switch (view.getId()) {
      case R.id.lnrHome:
        Navigation.findNavController(requireView()).popBackStack();
        break;
      case R.id.lnrSortByName:
        isSortedByAlphabet = !isSortedByAlphabet;
        isSortedByDate = false; // only one sort active at a time
        adapter.sortRecords(SortMode.Name, isSortedByAlphabet, isSortedByDate);
        updateSortButtonHighlight();
        break;
      case R.id.lnrSortByDate:
        isSortedByDate = !isSortedByDate;
        isSortedByAlphabet = false; // only one sort active at a time
        adapter.sortRecords(SortMode.Date, isSortedByAlphabet, isSortedByDate);
        updateSortButtonHighlight();
        break;
      case R.id.lnrVolume:
        VolumeChangeFragment volumeChange = new VolumeChangeFragment();
        volumeChange.show(getChildFragmentManager(), volumeChange.getClass().getSimpleName());
        break;
      case R.id.lnrBrightness:
        BrightnessChangeFragment brightnessChange = new BrightnessChangeFragment();
        brightnessChange.show(getChildFragmentManager(), brightnessChange.getClass().getSimpleName());
        break;
      case R.id.lnrAddClinician:
        recordsMode = RecordsMode.CREATE;
        Bundle argsClincian = new Bundle();
        argsClincian.putString(
            getResources().getString(R.string.dashboard_action_argument_call_source),
            getResources().getString(R.string.caption_clinician_records));
        argsClincian.putString(getResources().getString(R.string.caption_record_type_mode),
            RecordsMode.CREATE.toString());
        navigateToScreen(R.id.nav_to_clinicians_manager, argsClincian);
        break;
      case R.id.lnrAddFacility:
        recordsMode = RecordsMode.CREATE;
        Bundle argFacility = new Bundle();
        argFacility.putString(
            getResources().getString(R.string.dashboard_action_argument_call_source),
            getResources().getString(R.string.caption_facility_records));
        argFacility.putString(getResources().getString(R.string.caption_record_type_mode),
            RecordsMode.CREATE.toString());
        navigateToScreen(R.id.nav_to_manage_facilities, argFacility);
        break;
    }
  }

  private void navigateToScreen(int layoutIdentifier, Bundle bundl) {
    Navigation.findNavController(requireView()).navigate(layoutIdentifier, bundl);
  }

  private void hasRecords() {
    /*
     * if(lstDataRecords.size() <= 1)
     * {
     * binding.recordsTable.setVisibility(View.GONE);
     * binding.tvNoRecordsFound.setVisibility(View.VISIBLE);
     * }
     * else {
     * binding.recordsTable.setVisibility(View.VISIBLE);
     * binding.tvNoRecordsFound.setVisibility(View.GONE);
     * }
     */
  }

  @Override
  public void onResume() {
    super.onResume();

    if (screenRoute == ScreenRoute.CLINICIAN_RECORDS) {
      manageClinicianWigets();
    } else {
      manageFacilityWigets();
    }
  }

  /**
   * Determines if Clinician Records is accessible based on storage mode and
   * network connectivity.
   * 
   * Logic:
   * - For ONLINE mode: Only accessible if internet is connected
   * - Data is auto-fetched when accessible
   */
  private boolean isClinicianRecordsAccessible() {
    if (storageManager == null) {
      return true; // Fallback to accessible if manager is null
    }

    IStorageManager.StorageMode storageMode = getStorageMode();

    // For ONLINE mode, only accessible if internet connected
    if (storageMode == IStorageManager.StorageMode.ONLINE) {
      boolean result = isNetworkConnected;
      return result;
    }

    // For OFFLINE mode or unknown, assume accessible
    return true;
  }

  /**
   * Updates the screen state for Clinician Records based on connectivity and
   * storage mode.
   * 
   * When accessible: Shows table, Add, Export buttons and fetches data
   * When not accessible: Hides table, hides Add/Export buttons, shows connection
   * message
   */
  private void updateScreenStateForClinicianRecords() {
    boolean isAccessible = isClinicianRecordsAccessible();

    if (isAccessible) {
      // Show table and buttons
      binding.recordsTable.setVisibility(View.VISIBLE);
      binding.tvNoRecordsFound.setVisibility(View.GONE);
      binding.inclBottomBar.lnrAddClinician.setVisibility(View.VISIBLE);
      binding.inclBottomBar.lnrExport.setVisibility(View.VISIBLE);
      hideLoader();

      // Only fetch data if not already fetching and data hasn't been loaded yet
      if (!isFetchingClinicians) {
        fetchClinicianDataIfAccessible();
      }
    } else {
      // Hide table and buttons
      binding.recordsTable.setVisibility(View.GONE);
      binding.inclBottomBar.lnrAddClinician.setVisibility(View.GONE);
      binding.inclBottomBar.lnrExport.setVisibility(View.GONE);
      hideLoader();

      // Show no internet message
      binding.tvNoRecordsFound.setVisibility(View.VISIBLE);
      binding.tvNoRecordsFound.setTextSize(22);
      binding.tvNoRecordsFound.setText("User Management needs active Internet connection");

      // Clear data
      lstDataRecords.clear();
      adapter.notifyDataSetChanged();
    }
  }

  /**
   * Fetches clinician data if the feature is accessible.
   * This is called automatically when:
   * - Fragment first loads
   * - Internet connection is restored
   */
  private void fetchClinicianDataIfAccessible() {
    if (!isClinicianRecordsAccessible()) {
      return; // Don't fetch if not accessible
    }

    manageClinicianWigets();
  }

  /**
   * Highlights the active sort button in yellow and resets the inactive one.
   */
  private void updateSortButtonHighlight() {
    if (binding == null) return;
    if (isSortedByAlphabet) {
      binding.inclTaskbar.imgSoryByName.setColorFilter(Color.YELLOW, PorterDuff.Mode.SRC_IN);
    } else {
      binding.inclTaskbar.imgSoryByName.clearColorFilter();
    }
    if (isSortedByDate) {
      binding.inclTaskbar.imgSotyByDate.setColorFilter(Color.YELLOW, PorterDuff.Mode.SRC_IN);
    } else {
      binding.inclTaskbar.imgSotyByDate.clearColorFilter();
    }
  }

  private void showLoader() {
    binding.loaderOverlay.setVisibility(View.VISIBLE);
    binding.progressBarLoader.setVisibility(View.VISIBLE);
  }

  private void hideLoader() {
    if (binding == null)
      return;
    binding.loaderOverlay.setVisibility(View.GONE);
    binding.progressBarLoader.setVisibility(View.GONE);
  }

  @Override
  public void onDestroyView() {
    super.onDestroyView();
    // Clean up network connectivity listener to prevent memory leaks
    if (networkConnectivityLiveData != null && screenRoute == ScreenRoute.CLINICIAN_RECORDS) {
      networkConnectivityLiveData.removeObservers(getViewLifecycleOwner());
    }
    // Reset fetching guards so onResume can re-fetch after view recreation
    isFetchingClinicians = false;
    isFetchingFacilities = false;
    binding = null;
  }
}
