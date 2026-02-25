package com.accessvascularinc.hydroguide.mma.ui;

import android.animation.LayoutTransition;
import android.app.Dialog;
import android.content.Context;
import android.content.Intent;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.RelativeLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.databinding.DataBindingUtil;
import androidx.lifecycle.ViewModelProvider;
import androidx.navigation.Navigation;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.databinding.FragmentHomeBinding;
import com.accessvascularinc.hydroguide.mma.db.HydroGuideDatabase;
import com.accessvascularinc.hydroguide.mma.db.repository.EncounterRepository;
import com.accessvascularinc.hydroguide.mma.model.DataFiles;
import com.accessvascularinc.hydroguide.mma.model.EncounterData;
import com.accessvascularinc.hydroguide.mma.model.ProfileState;
import com.accessvascularinc.hydroguide.mma.model.UserProfile;
import com.accessvascularinc.hydroguide.mma.sync.SyncManager;
import com.accessvascularinc.hydroguide.mma.ui.input.InputUtils;
import com.accessvascularinc.hydroguide.mma.ui.input.OnItemClickListener;
import com.accessvascularinc.hydroguide.mma.utils.DbHelper;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import com.accessvascularinc.hydroguide.mma.utils.StorageAlertHelper;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import com.google.android.material.search.SearchView;
import com.accessvascularinc.hydroguide.mma.storage.IStorageManager;
import com.accessvascularinc.hydroguide.mma.storage.IStorageManager.StorageMode;
import javax.inject.Inject;
import com.accessvascularinc.hydroguide.mma.utils.NetworkConnectivityLiveData;
import android.widget.ImageView;

import org.json.JSONException;

import java.io.File;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import dagger.hilt.android.AndroidEntryPoint;

@AndroidEntryPoint
public class HomeFragment extends BaseHydroGuideFragment implements OnItemClickListener {
  @Inject
  IStorageManager storageManager;
  private final Dialog closeKbBtn = null;
  FragmentHomeBinding binding = null;

  private ImageView syncStatusIcon;

  private HydroGuideDatabase db;
  private EncounterRepository encounterRepository;

  // Storage alert state tracking
  private boolean procedureBlocked = false;
  private String blockMessage = null;

  private final View.OnClickListener patientInputBtn_OnClickListener = v -> {
    // Check if procedure is blocked due to storage/sync issues
    if (procedureBlocked && blockMessage != null) {
      showStorageBlockedAlert(blockMessage);
      return;
    }

    // Check tablet battery level before allowing Start Procedure
    int tabletBatteryPct = viewmodel.getTabletState().getTabletChargePct();

    if (tabletBatteryPct < 10) {
      // Battery is too low, show alert
      final String content = getString(R.string.alert_content_action_disabled_LP_alarm);
      ConfirmDialog.show(requireContext(), content, confirmed -> {
        // Dialog dismissed
      });
      return;
    }
    
    // Battery level is sufficient, proceed with navigation
    Navigation.findNavController(requireView())
        .navigate(R.id.action_navigation_home_to_patient_input);
  };
  private final View.OnClickListener recordsBtn_OnClickListener = v -> {
    Navigation.findNavController(requireView()).navigate(R.id.action_navigation_home_to_records);
  };

  private final View.OnClickListener ultrasoundBtn_OnClickListener = v -> {    
    TryNavigateToUltrasound("home");
  };

  private final View.OnClickListener procedureBtn_OnClickListener = v -> Navigation.findNavController(requireView())
      .navigate(R.id.action_navigation_home_to_procedure);
  private final View.OnClickListener preferencesBtn_OnClickListener = v -> Navigation.findNavController(requireView())
      .navigate(R.id.action_navigation_home_to_preferences);
  private final View.OnClickListener changePasswordBtn_OnClickListener = v -> {
    try {
      android.os.Bundle args = new android.os.Bundle();
      args.putString("changeType", "REGULAR_CHANGE");
      Navigation.findNavController(requireView())
          .navigate(R.id.action_navigation_home_to_change_password, args);
    } catch (Exception e) {
      MedDevLog.error("HomeFragment", "Error changing password", e);
      CustomToast.show(requireContext(), "Error: " + e.getClass().getSimpleName(), CustomToast.Type.ERROR);
    }
  };

  // private final View.OnClickListener updateNavBtn_OnClickListener = v -> {
  // Intent launchIntent = getContext().getPackageManager()
  // .getLaunchIntentForPackage("com.accessvascularinc.hydroguide.droidmanager");
  // if (launchIntent != null) {
  // startActivity(launchIntent);// null pointer check in case package name was
  // not found
  // }
  // };
  private final View.OnClickListener updateNavBtn_OnClickListener = v -> Navigation.findNavController(requireView())
      .navigate(R.id.navigation_software_update);
  private final View.OnClickListener exitBtn_OnClickListener = v -> {
    // Check if WiFi is enabled
    boolean isWifiEnabled = isWifiEnabledOnHome();
    
    if (!isWifiEnabled) {
      // WiFi is disabled - show WiFi disabled warning dialog
      String wifiDisabledMessage = getString(R.string.wifi_disabled_logout_message);
      ConfirmDialog.show(requireContext(), wifiDisabledMessage,
          getString(R.string.logout_anyway),
          getString(R.string.okay),
          confirmed -> {
            if (confirmed) {
              com.accessvascularinc.hydroguide.mma.utils.PrefsManager.clearLoginResponse(
                  requireContext());
              requireActivity().finish();
            }
          });
    } else {
      // WiFi is enabled - show normal exit confirmation
      ConfirmDialog.show(requireContext(), "Are you sure you want to exit?", confirmed -> {
        if (confirmed) {
          com.accessvascularinc.hydroguide.mma.utils.PrefsManager.clearLoginResponse(
              requireContext());
          requireActivity().finish();
        }
      });
    }
  };

  private boolean isWifiEnabledOnHome() {
    try {
      WifiManager wifiManager = (WifiManager) requireContext().getApplicationContext()
          .getSystemService(Context.WIFI_SERVICE);
      return wifiManager != null && wifiManager.isWifiEnabled();
    } catch (Exception e) {
      Log.e("WiFiStatus", "Error checking WiFi status", e);
      return false;
    }
  }

  @Override
  public View onCreateView(@NonNull final LayoutInflater inflater, final ViewGroup container,
      final Bundle savedInstanceState) {
    db = HydroGuideDatabase.buildDatabase(requireContext());
    encounterRepository = new EncounterRepository(db);
    MedDevLog.info("Navigation", "Home screen entered");
    binding = DataBindingUtil.inflate(inflater, R.layout.fragment_home, container, false);
    boolean showSyncIcon = false;
    try {
      showSyncIcon = storageManager.isSyncAvailable();
    } catch (Exception e) {
      showSyncIcon = false;
    }
    binding.homeTopBar.setShowSyncIcon(showSyncIcon);
    super.onCreateView(inflater, container, savedInstanceState);

    syncStatusIcon = binding.homeTopBar.syncStatusIcon;

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
    restrictedEvents.add(
        new RestrictedEvent(binding.procedureNavButton, procedureBtn_OnClickListener,
            RESTRICTION_TYPE.LOW_POWER));
    restrictedEvents.add(
        new RestrictedEvent(binding.procedureNavButton, procedureBtn_OnClickListener,
            RESTRICTION_TYPE.CONTROLLER_STATUS_TIMEOUT));
    lowBatteryIndicators.add(binding.batteryStatusPbtn);
    errorIndicators.add(binding.errorIndicatorPbtn);

    UserProfile[] profiles = getProfilesFromStorage();
    for (final var profile : profiles) {
      viewmodel.getProfileState().setSelectedProfile(profile);
    }
    binding.selectedProfileName.setText(
        viewmodel.getProfileState().getSelectedProfile().getProfileName());
    binding.patientInputNavButton.setOnClickListener(patientInputBtn_OnClickListener);
    binding.recordsNavButton.setOnClickListener(recordsBtn_OnClickListener);
    binding.procedureNavButton.setOnClickListener(procedureBtn_OnClickListener);
    binding.preferencesNavButton.setOnClickListener(preferencesBtn_OnClickListener);
    binding.changePasswordNavButton.setOnClickListener(changePasswordBtn_OnClickListener);
    binding.exitAppButton.setOnClickListener(exitBtn_OnClickListener);
    binding.systemUpdateNavButton.setOnClickListener(updateNavBtn_OnClickListener);
    binding.ultrasoundNavButton.setOnClickListener(ultrasoundBtn_OnClickListener);

    if (profileState.getSelectedProfile() == null) {
      disableButtons();
    } else {
      if (profileState.getSelectedProfile().getProfileName().isEmpty()) {
        disableButtons();
      } else {
        viewmodel.getTabletState().setTabletMuted(
            !profileState.getSelectedProfile().getUserPreferences().isEnableAudio());
      }

    }

    binding.createProfileButton.setOnClickListener(view -> {
      // Navigate to a "Profile Settings" screen with no data.
      viewmodel.getProfileState().setSelectedProfile(new UserProfile());
      Navigation.findNavController(requireView())
          .navigate(R.id.action_navigation_home_to_preferences);
    });

    final View root = binding.getRoot();

    root.getViewTreeObserver().addOnGlobalLayoutListener(
        InputUtils.getCloseKeyboardHandler(requireActivity(), root, closeKbBtn));
    syncManager.getPreferences();
    return root;
  }

  private void disableButtons() {
    binding.procedureNavButton.setAlpha(0.5f);
    binding.patientInputNavButton.setAlpha(0.5f);
    binding.preferencesNavButton.setAlpha(0.5f);
    binding.ultrasoundNavButton.setAlpha(0.5f);

    binding.procedureNavButton.setOnClickListener(actionDisabledNP_OnClickListener);
    binding.patientInputNavButton.setOnClickListener(actionDisabledNP_OnClickListener);
    binding.preferencesNavButton.setOnClickListener(actionDisabledNP_OnClickListener);
    binding.ultrasoundNavButton.setOnClickListener(actionDisabledNP_OnClickListener);
  }

  /**
   * Check storage/sync status and show appropriate alerts.
   * Called on screen creation and on button click when blocked.
   */
  private void checkStorageAndShowAlert() {
    StorageAlertHelper.checkStorageStatus(
        requireContext(),
        storageManager,
        viewmodel.getTabletState(),
        encounterRepository,
        (level, message) -> {
          switch (level) {
            case BLOCKED:
              procedureBlocked = true;
              blockMessage = message;
              disableStartProcedureButton();
              showStorageBlockedAlert(message);
              break;
            case WARNING:
              procedureBlocked = false;
              blockMessage = null;
              enableStartProcedureButton();
              showStorageWarningAlert(message);
              break;
            case NONE:
              procedureBlocked = false;
              blockMessage = null;
              enableStartProcedureButton();
              break;
          }
        }
    );
  }

  /**
   * Show dismissible warning alert (single Dismiss button).
   */
  private void showStorageWarningAlert(String message) {
    ConfirmDialog.singleActionPopup(
        requireContext(),
        message,
        getString(R.string.dismiss_button),
        confirmed -> { /* Dialog dismissed */ },
        0.8, // 80% of screen width
        0.4  // 40% of screen height
    );
  }

  /**
   * Show blocked alert with two options: Dismiss or Go to Records.
   */
  private void showStorageBlockedAlert(String message) {
    ConfirmDialog.show(
        requireContext(),
        message,
        getString(R.string.go_to_records_button),  // "Yes" button = Go to Records
        getString(R.string.dismiss_button),         // "No" button = Dismiss
        confirmed -> {
          if (confirmed) {
            // Navigate to Records screen
            Navigation.findNavController(requireView())
                .navigate(R.id.action_navigation_home_to_records);
          }
        }
    );
  }

  /**
   * Disable only the Start Procedure button due to storage issues.
   */
  private void disableStartProcedureButton() {
    binding.patientInputNavButton.setAlpha(0.5f);
  }

  /**
   * Enable the Start Procedure button when storage issues are resolved.
   */
  private void enableStartProcedureButton() {
    binding.patientInputNavButton.setAlpha(1.0f);
  }

  @Override
  public void onViewCreated(@NonNull final View view, @Nullable final Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);

    StorageMode mode = storageManager.getStorageMode();
    String modeText = (mode == StorageMode.ONLINE)
        ? "Online Mode"
        : "Offline Mode";
    android.widget.Toast
        .makeText(requireContext(), "Current Storage Mode: " + modeText,
            android.widget.Toast.LENGTH_SHORT)
        .show();

    // Set up sync button and network observer (see BaseHydroGuideFragment for
    // details)
    // Check if controller is connected, show dialog if not
    if (!viewmodel.getControllerCommunicationManager().checkConnection()) {
      // ((MainActivity) requireActivity()).showConnectionDialog();
      ivUsbIcon.setVisibility(View.GONE);
    } else
      ivUsbIcon.setVisibility(View.VISIBLE);
    setupNetworkSync(syncStatusIcon, getViewLifecycleOwner(), requireContext());

    viewmodel.getEncounterData().clearEncounterData();
    viewmodel.getMonitorData().clearMonitorData();

    // Check storage status and show alerts (warning or blocked)
    // Shows alert every time Home screen is entered
    checkStorageAndShowAlert();

    // Scan records for interrupted procedure:
    // We know it is interrupted by the state it is concluded in
    // Interrupted procedure detected. Delete or Resume
    final ArrayList<EncounterData> savedRecords = new ArrayList<>(
        Arrays.asList(DataFiles.getSavedRecords(requireContext())));
    if (savedRecords.isEmpty()) {
      return;
    }

    // Waste of processing, fix later
    if (savedRecords.get(savedRecords.size() - 1).isLive()) {
      // PROMPT
      promptContinueProcedure(savedRecords.get(savedRecords.size() - 1));
    }
  }

  private void promptContinueProcedure(final EncounterData record) {
    final MaterialAlertDialogBuilder builder = new MaterialAlertDialogBuilder(requireActivity());
    builder.setMessage("Interrupted/incomplete procedure record detected.");
    builder.setNegativeButton("Delete", (dialog, id) -> {
      deleteProcedureFile(record);
      dialog.dismiss();
    });
    builder.setPositiveButton("Resume", (dialog, id) -> {
      Log.d("ABC", "promptContinueProcedure: ");

      Log.d("ABC", "2: ");
      viewmodel.setEncounterData(record);
      final String targetProf = viewmodel.getEncounterData().getPatient().getPatientInserter();
      final UserProfile[] profiles = getProfilesFromStorage();
      for (final var profile : profiles) {
        Log.d("Profile", "Selected: " + profile);
        if (profile.getProfileName().equals(targetProf)) {
          viewmodel.getProfileState().setSelectedProfile(profile);
        }
      }
      if (profiles.length == 0 || viewmodel.getProfileState().getSelectedProfile() == null) {
        // This case is not possible: cannot delete a profile between interrupt and
        // rejoin
      }
      final File dirLoc = Paths.get(record.getDataDirPath()).toFile();
      final File pdfFile = new File(dirLoc, DataFiles.REPORT_PDF);
      if (pdfFile.exists()) {
        pdfFile.delete();
      }
      Navigation.findNavController(requireView())
          .navigate(R.id.action_navigation_home_to_procedure);
      dialog.dismiss();
    });
    builder.setCancelable(false);
    builder.show();
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

  public void onItemClickListener(final int position) {
    // Removed profile selection logic
  }

  private static final class ProfileListItemViewHolder extends RecyclerView.ViewHolder {
    public final TextView text;

    private ProfileListItemViewHolder(@NonNull final View view) {
      super(view);
      this.text = itemView.findViewById(R.id.profile_list_item_text);
    }

    @NonNull
    public static ProfileListItemViewHolder create(@NonNull final ViewGroup parent) {
      return new ProfileListItemViewHolder(LayoutInflater.from(parent.getContext())
          .inflate(R.layout.profile_list_item, parent, false));
    }
  }
}