package com.accessvascularinc.hydroguide.mma.ui.admin;

import android.animation.LayoutTransition;
import android.app.Dialog;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.GridView;
import android.widget.RelativeLayout;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.databinding.DataBindingUtil;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;
import androidx.navigation.Navigation;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.adapters.DashboardGridAdapter;
import com.accessvascularinc.hydroguide.mma.databinding.FragmentDashboardBinding;
import com.accessvascularinc.hydroguide.mma.model.DashboardGridItems;
import com.accessvascularinc.hydroguide.mma.model.DataFiles;
import com.accessvascularinc.hydroguide.mma.model.ProfileState;
import com.accessvascularinc.hydroguide.mma.model.UserProfile;
import com.accessvascularinc.hydroguide.mma.storage.IStorageManager;
import com.accessvascularinc.hydroguide.mma.ui.BaseHydroGuideFragment;
import com.accessvascularinc.hydroguide.mma.ui.ConfirmDialog;
import com.accessvascularinc.hydroguide.mma.ui.CustomToast;
import com.accessvascularinc.hydroguide.mma.ui.MainViewModel;
import com.accessvascularinc.hydroguide.mma.ui.dialogs.ModeSwitchDialog;
import com.accessvascularinc.hydroguide.mma.ui.dialogs.OfflineToOnlineModeDialog;
import com.accessvascularinc.hydroguide.mma.ui.input.InputUtils;
import com.accessvascularinc.hydroguide.mma.ui.input.OnItemClickListener;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import com.accessvascularinc.hydroguide.mma.utils.NetworkConnectivityLiveData;
import com.accessvascularinc.hydroguide.mma.utils.ScreenRoute;
import com.accessvascularinc.hydroguide.mma.utils.UserType;
import com.accessvascularinc.hydroguide.mma.utils.Utility;
import com.google.android.material.search.SearchView;

import dagger.hilt.android.AndroidEntryPoint;

import javax.inject.Inject;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

@AndroidEntryPoint
public class FragmentDashboard extends BaseHydroGuideFragment
    implements AdapterView.OnItemClickListener, OnItemClickListener {
  private SearchView searchView = null;
  private FragmentDashboardBinding binding = null;
  private GridView grdDashboardItems;
  private DashboardGridAdapter dashboardGridAdapter;
  private List<DashboardGridItems> lstGridItems = new ArrayList<>();
  private boolean isProfileSelected = false;
  private final Dialog closeKbBtn = null;
  private RelativeLayout profileWidget = null;

  @Inject
  IStorageManager storageManager;

  private NetworkConnectivityLiveData networkConnectivityLiveData;
  private boolean isNetworkConnected = false;

  @Override
  public View onCreateView(@NonNull final LayoutInflater inflater, final ViewGroup container,
      final Bundle savedInstanceState) {
    binding = DataBindingUtil.inflate(inflater, R.layout.fragment_dashboard, container, false);
    super.onCreateView(inflater, container, savedInstanceState);
    viewmodel = new ViewModelProvider(requireActivity()).get(MainViewModel.class);
    final ProfileState profileState = viewmodel.getProfileState();
    binding.setProfileState(profileState);
    binding.homeTopBar.setControllerState(viewmodel.getControllerState());
    binding.homeTopBar.setTabletState(viewmodel.getTabletState());
    tvBattery = binding.homeTopBar.controllerBatteryLevelPct;
    ivBattery = binding.homeTopBar.remoteBatteryLevelIndicator;
    ivUsbIcon = binding.homeTopBar.usbIcon;
    ivTabletBattery = binding.homeTopBar.tabletBatteryIndicator.tabletBatteryLevelIndicator;
    tvTabletBattery = binding.homeTopBar.tabletBatteryIndicator.tabletBatteryLevelPct;

    UserProfile[] profiles = getProfilesFromStorage();
    for (final var profile : profiles) {
      Log.d("ProfileFinding", "onCreateView: " + profile.getProfileName());
      viewmodel.getProfileState().setSelectedProfile(profile);
    }
    binding.selectedProfileName.setText(
        viewmodel.getProfileState().getSelectedProfile().getProfileName());
    lowBatteryIndicators.add(binding.batteryStatusPbtn);
    errorIndicators.add(binding.errorIndicatorPbtn);
    profileWidget = binding.profileSelectWidget;
    // binding.selectedProfileName.setOnClickListener(profileSelectBtn_OnClickListener);
    searchView = binding.searchView;
    searchView.setAutoShowKeyboard(false);
    searchView.setLayoutTransition(new LayoutTransition());
    ((ViewGroup) searchView.getParent().getParent()).setLayoutTransition(new LayoutTransition());

    searchView.addTransitionListener((view, previousState, newState) -> {
      if (newState == SearchView.TransitionState.HIDING) {
      }
    });

    if (profileState.getSelectedProfile() == null) {
      isProfileSelected = false;
    } else {
      isProfileSelected = true;
      viewmodel.getTabletState()
          .setTabletMuted(!profileState.getSelectedProfile().getUserPreferences().isEnableAudio());
    }

    // Setup network connectivity listener
    networkConnectivityLiveData = new NetworkConnectivityLiveData(requireContext());
    Observer<Boolean> networkObserver = isConnected -> {
      isNetworkConnected = isConnected != null && isConnected;
      // Always update dashboard items when network state changes
      updateDashboardItems();
    };
    networkConnectivityLiveData.observe(getViewLifecycleOwner(), networkObserver);

    // searchView.getEditText().addTextChangedListener(new TextWatcher() {
    // @Override
    // public void beforeTextChanged(final CharSequence s, final int start, final
    // int count, final int after) {}
    //
    // @Override
    // public void onTextChanged(final CharSequence s, final int start, final int
    // before, final int count)
    // {
    // profileListAdapter.filterList(s.toString());
    // }
    //
    // @Override
    // public void afterTextChanged(final Editable editable) {}
    // });

    final RecyclerView profileListView = binding.profilesListItems;
    profileListView.setLayoutManager(new LinearLayoutManager(getContext()));

    binding.exitAppButton.setOnClickListener(dashboardExitBtnListener);

    final View root = binding.getRoot();

    root.getViewTreeObserver()
        .addOnGlobalLayoutListener(InputUtils.getCloseKeyboardHandler(requireActivity(), root, closeKbBtn));

    grdDashboardItems = binding.grdDashboardItems;
    dashboardGridAdapter = new DashboardGridAdapter(getContext(), R.layout.dashboard_grid_items, prepareGridItems());
    grdDashboardItems.setAdapter(dashboardGridAdapter);
    grdDashboardItems.setOnItemClickListener(this);
    loadAdminDashboard();
    return root;
  }

  private UserProfile[] getProfilesFromStorage() {
    final File storageDir = requireContext().getExternalFilesDir(DataFiles.PROFILES_DIR);
    final String[] fileList = storageDir.list();
    UserProfile[] userProfiles = new UserProfile[0];
    if (fileList != null) {
      userProfiles = new UserProfile[fileList.length];
      for (int i = 0; i < userProfiles.length; i++) {
        userProfiles[i] = DataFiles.getProfileFromFile(new File(storageDir, fileList[i]));
      }
    }

    return userProfiles;
  }

  /**
   * Gets the label for Switch Mode based on current storage mode.
   * If OFFLINE, returns "Switch to Online"
   * If ONLINE, returns "Switch to Offline"
   */
  private String getSwitchModeLabel() {
    if (storageManager != null) {
      IStorageManager.StorageMode currentMode = storageManager.getStorageMode();
      if (currentMode == IStorageManager.StorageMode.OFFLINE) {
        return "Switch to Online";
      } else if (currentMode == IStorageManager.StorageMode.ONLINE) {
        return "Switch to Offline";
      }
    }
    return "Switch Mode";
  }

  /**
   * Determines if Clinician Records should be enabled based on storage mode and
   * network connectivity.
   * 
   * Logic:
   * - OFFLINE mode: Always enabled (feature not implemented for OFFLINE)
   * - ONLINE mode: Enabled only if network is connected
   */
  private boolean isClinicianRecordsEnabled() {
    String storageMode = getStorageMode().name(); // Use the common method
    if (IStorageManager.StorageMode.OFFLINE.toString().equals(storageMode)) {
      return true;
    } else if ("ONLINE".equals(storageMode)) {
      return isNetworkConnected;
    }
    return false;
  }

  /**
   * Updates the dashboard items when network connectivity changes.
   * This ensures real-time updates when user disconnects/connects to network.
   */
  private void updateDashboardItems() {
    if (dashboardGridAdapter != null) {
      lstGridItems.clear();

      boolean isClinicianRecordsEnabled = isClinicianRecordsEnabled();

      DashboardGridItems clinicianItem = new DashboardGridItems(
          getResources().getString(R.string.dashboard_clinician_records),
          R.drawable.dashboard_icon_clinician_records,
          ScreenRoute.CLINICIAN_RECORDS,
          isClinicianRecordsEnabled);
      lstGridItems.add(clinicianItem);

      lstGridItems.add(new DashboardGridItems(getResources().getString(R.string.dashboard_facility_records),
          R.drawable.dashboard_icon_facility_records,
          ScreenRoute.FACILITY_RECORDS, true));

      lstGridItems.add(new DashboardGridItems(getResources().getString(R.string.title_system_update),
          R.drawable.dashboard_icon_system_update,
          ScreenRoute.SYSTEM_UPDATE, isProfileSelected));

      lstGridItems.add(new DashboardGridItems(getResources().getString(R.string.dashboard_settings),
          R.drawable.dashboard_icon_settings,
          ScreenRoute.SETTINGS, isProfileSelected));

      lstGridItems.add(new DashboardGridItems(getResources().getString(R.string.dashboard_change_password),
          R.drawable.button_large_profilesettings,
          ScreenRoute.CHANGE_PASSWORD, true));

      lstGridItems.add(new DashboardGridItems(getSwitchModeLabel(),
          R.drawable.dashboard_icon_settings,
          ScreenRoute.SWITCH_MODE, true));

      dashboardGridAdapter.notifyDataSetChanged();
    }
  }

  private List<DashboardGridItems> prepareGridItems() {
    UserType userType = UserType.ADMIN;
    switch (userType) {
      case ADMIN:
        lstGridItems.clear();

        boolean isClinicianRecordsEnabled = isClinicianRecordsEnabled();

        lstGridItems.add(new DashboardGridItems(getResources().getString(R.string.dashboard_clinician_records),
            R.drawable.dashboard_icon_clinician_records, ScreenRoute.CLINICIAN_RECORDS, isClinicianRecordsEnabled));

        lstGridItems.add(new DashboardGridItems(getResources().getString(R.string.dashboard_facility_records),
            R.drawable.dashboard_icon_facility_records,
            ScreenRoute.FACILITY_RECORDS, true));

        lstGridItems.add(new DashboardGridItems(getResources().getString(R.string.title_system_update),
            R.drawable.dashboard_icon_system_update,
            ScreenRoute.SYSTEM_UPDATE, isProfileSelected));

        lstGridItems.add(new DashboardGridItems(getResources().getString(R.string.dashboard_settings),
            R.drawable.dashboard_icon_settings,
            ScreenRoute.SETTINGS, isProfileSelected));

        lstGridItems.add(new DashboardGridItems(getResources().getString(R.string.dashboard_change_password),
            R.drawable.button_large_profilesettings,
            ScreenRoute.CHANGE_PASSWORD, true));
        lstGridItems.add(new DashboardGridItems(getSwitchModeLabel(),
            R.drawable.dashboard_icon_settings,
            ScreenRoute.SWITCH_MODE, true));
        break;
    }
    return lstGridItems;
  }

  private void loadAdminDashboard() {
    binding.homeTopBar.controllerBatteryLevelPct.setVisibility(View.INVISIBLE);
    binding.homeTopBar.remoteBatteryLevelIndicator.setVisibility(View.INVISIBLE);
    binding.homeTopBar.usbIcon.setVisibility(View.INVISIBLE);
    binding.homeTopBar.controllerBatteryLabel.setVisibility(View.INVISIBLE);
    binding.lnrDashboardControlsSetOne.setVisibility(View.GONE);
    binding.lnrDashboardControlsSetTwo.setVisibility(View.GONE);
    binding.grdDashboardItems.setVisibility(View.VISIBLE);
  }

  @Override
  public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
    DashboardGridItems gridItem = lstGridItems.get(position);

    // Special handling for Clinician Records
    if (gridItem.getDashboardMenu() == ScreenRoute.CLINICIAN_RECORDS) {
      if (storageManager != null) {
        IStorageManager.StorageMode storageMode = storageManager.getStorageMode();

        // ONLINE mode + No Internet: Show needs internet message
        if (storageMode == IStorageManager.StorageMode.ONLINE && !isNetworkConnected) {
          Toast.makeText(requireContext(),
              "User Management needs active Internet connection",
              Toast.LENGTH_SHORT).show();
          return;
        }
      }
    }

    // Check if item is disabled (for other items)
    if (!gridItem.isEnabled()) {
      actionDisabledNP_OnClickListener.onClick(view);
      return;
    }

    // Navigation for enabled items
    Bundle args = new Bundle();
    args.clear();
    switch (gridItem.getDashboardMenu()) {
      case CLINICIAN_RECORDS:
        args.putString(getResources().getString(R.string.dashboard_action_argument_call_source),
            getResources().getString(R.string.caption_clinician_records));
        navigateToScreen(R.id.nav_data_records, args);
        break;
      case FACILITY_RECORDS:
        args.putString(getResources().getString(R.string.dashboard_action_argument_call_source),
            getResources().getString(R.string.caption_facility_records));
        navigateToScreen(R.id.nav_data_records, args);
        break;
      case SYSTEM_UPDATE:
        navigateToScreen(R.id.navigation_software_update, args);
        break;
      case SETTINGS:
        args.putString("fromScreen", Utility.FROM_SCREEN_HOME);
        navigateToScreen(R.id.action_navigation_home_to_settings, args);
        break;
      case CHANGE_PASSWORD:
        args.putString("changeType", "REGULAR_CHANGE");
        navigateToScreen(R.id.navigation_changepassword, args);
        break;
      case SWITCH_MODE:
        handleModeSwitchClick();
        break;
    }
  }

  /**
   * Handles the Switch Mode button click.
   * - If OFFLINE mode: Shows OfflineToOnlineModeDialog (Offline → Online switch)
   * - If ONLINE mode: Shows ModeSwitchDialog (Online → Offline switch) with
   * network validation
   */
  private void handleModeSwitchClick() {
    IStorageManager.StorageMode currentMode = storageManager.getStorageMode();

    if (currentMode == IStorageManager.StorageMode.OFFLINE) {
      // OFFLINE → ONLINE: Show the new OfflineToOnlineModeDialog
      OfflineToOnlineModeDialog dialog = new OfflineToOnlineModeDialog();
      dialog.show(getChildFragmentManager(), "OfflineToOnlineModeDialog");
      return;
    }

    // ONLINE → OFFLINE: Validate network is connected
    if (!isNetworkConnected) {
      Toast.makeText(requireContext(),
          "Internet connection is required to switch to Offline mode",
          Toast.LENGTH_LONG).show();
      return;
    }

    // Show the mode switch dialog (Online → Offline)
    ModeSwitchDialog dialog = ModeSwitchDialog.newInstance();
    dialog.show(getChildFragmentManager(), "ModeSwitchDialog");
  }

  private void navigateToScreen(int layoutIdentifier, Bundle bundl) {
    try {
      android.util.Log.d("FragmentDashboard", "navigateToScreen called with layoutId: " + layoutIdentifier);
      android.util.Log.d("FragmentDashboard", "Checking if layoutId is navigation_changepassword (id="
          + R.id.navigation_changepassword + "), actual=" + layoutIdentifier);

      android.view.View currentView = getView();
      if (currentView != null) {
        android.util.Log.d("FragmentDashboard", "View is attached: " + currentView.hashCode());
        androidx.navigation.NavController navController = Navigation.findNavController(currentView);
        android.util.Log.d("FragmentDashboard", "NavController obtained: " + (navController != null));

        if (navController != null) {
          android.util.Log.d("FragmentDashboard", "Executing navigation with layoutId=" + layoutIdentifier);
          if (bundl != null) {
            android.util.Log.d("FragmentDashboard", "Bundle contains: changeType=" + bundl.getString("changeType"));
          }
          navController.navigate(layoutIdentifier, bundl);
          android.util.Log.d("FragmentDashboard", "Navigation executed successfully");
        } else {
          MedDevLog.error("FragmentDashboard", "ERROR: NavController is null");
        }
      } else {
        MedDevLog.error("FragmentDashboard", "ERROR: View is not attached");
      }
    } catch (IllegalArgumentException e) {
      MedDevLog.error("FragmentDashboard", "ERROR: Navigation action not found - " + e.getMessage());
      CustomToast.show(requireContext(), "Navigation failed: Action not found", CustomToast.Type.ERROR);
    } catch (IllegalStateException e) {
      MedDevLog.error("FragmentDashboard", "ERROR: Fragment state error", e);
      CustomToast.show(requireContext(), "Fragment state error. Try again.", CustomToast.Type.ERROR);
    } catch (Exception e) {
      MedDevLog.error("FragmentDashboard", "ERROR: Fragment state general exception", e);
      CustomToast.show(requireContext(), "Error: " + e.getClass().getSimpleName(), CustomToast.Type.ERROR);
    }
  }

  public void onItemClickListener(final int position) {
    final ProfileState profileState = viewmodel.getProfileState();
    searchView.hide();
  }

  public final View.OnClickListener dashboardExitBtnListener = v -> {
    ConfirmDialog.show(requireContext(), getResources().getString(R.string.dashboard_exit_message), confirmed -> {
      if (confirmed) {
        MedDevLog.audit("Authentication", "Admin user logged out");
        com.accessvascularinc.hydroguide.mma.utils.PrefsManager.clearLoginResponse(requireContext());
        requireActivity().finish();
        MedDevLog.setLoggedInUserId(null);
      }
    });
  };
  private final View.OnClickListener profileSelectBtn_OnClickListener = v -> {
    if (profileWidget.getVisibility() == View.GONE) {
      searchView.show();
      profileWidget.setVisibility(View.VISIBLE);
    } else {
      searchView.hide();
      profileWidget.setVisibility(View.GONE);
    }
  };

  @Override
  public void onDestroyView() {
    super.onDestroyView();
    // Clean up network connectivity listener to prevent memory leaks
    if (networkConnectivityLiveData != null) {
      networkConnectivityLiveData.removeObservers(getViewLifecycleOwner());
    }
    binding = null;
  }
}
