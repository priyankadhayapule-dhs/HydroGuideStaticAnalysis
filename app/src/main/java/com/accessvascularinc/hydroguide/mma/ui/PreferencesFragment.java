package com.accessvascularinc.hydroguide.mma.ui;

import android.app.Dialog;
import android.graphics.PorterDuff;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageButton;
import android.widget.RadioGroup;
import android.widget.Toast;

import androidx.activity.OnBackPressedCallback;
import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;
import androidx.databinding.DataBindingUtil;
import androidx.databinding.Observable;
import androidx.lifecycle.ViewModelProvider;
import androidx.navigation.Navigation;
import androidx.navigation.fragment.NavHostFragment;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.accessvascularinc.hydroguide.mma.BR;
import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.databinding.FragmentPreferencesBinding;
import com.accessvascularinc.hydroguide.mma.db.HydroGuideDatabase;
import com.accessvascularinc.hydroguide.mma.db.repository.FacilityRepository;
import com.accessvascularinc.hydroguide.mma.model.DataFiles;
import com.accessvascularinc.hydroguide.mma.model.Facility;
import com.accessvascularinc.hydroguide.mma.model.ProfileState;
import com.accessvascularinc.hydroguide.mma.model.SortMode;
import com.accessvascularinc.hydroguide.mma.model.UserPreferences;
import com.accessvascularinc.hydroguide.mma.ui.input.EditFacilityDialogFragment;
import com.accessvascularinc.hydroguide.mma.ui.input.FacilityListAdapter;
import com.accessvascularinc.hydroguide.mma.ui.input.InputUtils;
import com.accessvascularinc.hydroguide.mma.ui.settings.BrightnessChangeFragment;
import com.accessvascularinc.hydroguide.mma.ui.settings.VolumeChangeFragment;
import com.accessvascularinc.hydroguide.mma.utils.DbHelper;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import com.accessvascularinc.hydroguide.mma.utils.PrefsManager;
import com.accessvascularinc.hydroguide.mma.db.repository.UsersRepository;
import com.accessvascularinc.hydroguide.mma.db.entities.UsersEntity;
import com.accessvascularinc.hydroguide.mma.security.EncryptionManager;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import org.json.JSONException;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class PreferencesFragment extends BaseHydroGuideFragment {
  private FacilityListAdapter facilityListAdapter = null;
  private final Dialog closeKbBtn = null;
  private SortMode sortMode = SortMode.None;
  private ImageButton nameSortBtn = null, dateSortBtn = null;
  private boolean allFacilitiesSelected = false;
  private HydroGuideDatabase db;
  private FacilityRepository facilityRepository;
  private UsersRepository usersRepository;

  private final View.OnClickListener selectAllFacilitiesBtn_OnClickListener = v -> {
    allFacilitiesSelected = !allFacilitiesSelected;
    if (allFacilitiesSelected) {
      facilityListAdapter.selectAllFacilities();
    } else {
      facilityListAdapter.deselectAllFacilities();
    }
  };
  private final View.OnClickListener addFacilityBtn_OnClickListener = v -> {
    EditFacilityDialogFragment addFacilityDialog = new EditFacilityDialogFragment();
    addFacilityDialog.show(getChildFragmentManager(), "TEST");
  };
  private final View.OnClickListener editFacilityBtn_OnClickListener = v -> {
    final Facility[] selectedFacilities = facilityListAdapter.getSelectedFacilities();

    // Can only edit one facility at a time.
    if (selectedFacilities.length == 1) {
      EditFacilityDialogFragment editFacilityDialog = new EditFacilityDialogFragment(selectedFacilities[0]);
      editFacilityDialog.show(getChildFragmentManager(), "TEST");
    }
  };
  private final View.OnClickListener removeFacilitiesBtn_OnClickListener = v -> {
    deleteRecord();
  };

  private void deleteRecord() {
    final MaterialAlertDialogBuilder builder = new MaterialAlertDialogBuilder(requireActivity());
    builder.setMessage(getString(R.string.pref_action_warning_text));
    builder.setNegativeButton(getString(R.string.pref_action_warning_no), (dialog, id) -> {
    });
    builder.setPositiveButton(getString(R.string.pref_action_warning_yes), (dialog, id) -> {
      final Facility[] selectedFacilities = facilityListAdapter.getSelectedFacilities();
      removeFacilities(selectedFacilities);
      DbHelper.deleteFacility(selectedFacilities, facilityRepository, viewmodel,
          facilityListAdapter);
      NavHostFragment.findNavController(this).navigateUp();
    });
    builder.show();

  }

  private final View.OnClickListener favoriteFacilitiesBtn_OnClickListener = v -> {
    final Facility[] selectedFacilities = facilityListAdapter.getSelectedFacilities();
    final UserPreferences preferences = viewmodel.getProfileState().getSelectedProfile().getUserPreferences();
    final List<String> newFaves = new ArrayList<>(Arrays.asList(preferences.getFavoriteFacilitiesIds()));
    for (Facility facility : selectedFacilities) {
      if (newFaves.contains(facility.getId())) {
        newFaves.remove(facility.getId());
      } else {
        newFaves.add(facility.getId());
      }
    }

    facilityListAdapter.updateFavorites(newFaves.toArray(new String[0]));
    preferences.setFavoriteFacilitiesIds(newFaves.toArray(new String[0]));
  };
  private final View.OnClickListener sortFacilitiesNameBtn_OnClickListener = v -> {
    switch (sortMode) {
      case None, Date -> {
        sortMode = SortMode.Name;
        setButtonColor(nameSortBtn, R.color.av_tip_confirm_yellow);
      }
      case Name -> {
        sortMode = SortMode.None;
        setButtonColor(nameSortBtn, R.color.white);
      }
    }
    setButtonColor(dateSortBtn, R.color.white);
    facilityListAdapter.sortFacilities(sortMode);
  };
  private final View.OnClickListener sortFacilitiesDateBtn_OnClickListener = v -> {
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
    setButtonColor(nameSortBtn, R.color.white);
    facilityListAdapter.sortFacilities(sortMode);
  };

  private void setButtonColor(final ImageButton button, final int color) {
    final int newColor = ContextCompat.getColor(requireContext(), color);
    button.setColorFilter(newColor, PorterDuff.Mode.MULTIPLY);
  }

  private final View.OnClickListener homeBtn_OnClickListener = v -> {
    String filename = viewmodel.getProfileState().getSelectedProfile().getFileName();
    final String profileName = DataFiles.cleanFileNameString(
        viewmodel.getProfileState().getSelectedProfile().getProfileName());
    if (profileName.isEmpty()) {
      Toast.makeText(requireContext(), R.string.invalid_profile_name_message, Toast.LENGTH_SHORT)
          .show();
      return;
    }
    if (!filename.isEmpty()) {
      final File profileFile = new File(requireContext().getExternalFilesDir(DataFiles.PROFILES_DIR), filename);
      viewmodel.getProfileState().setSelectedProfile(DataFiles.getProfileFromFile(profileFile));
    }
    Navigation.findNavController(requireView())
        .navigate(R.id.action_navigation_preferences_to_home);
  };
  private final View.OnClickListener removeProfileBtn_OnClickListener = v -> {
    final MaterialAlertDialogBuilder builder = new MaterialAlertDialogBuilder(requireActivity());
    builder.setMessage(getString(R.string.pref_action_warning_text));
    builder.setNegativeButton(getString(R.string.pref_action_warning_no), (dialog, id) -> {
    });
    builder.setPositiveButton(getString(R.string.pref_action_warning_yes), (dialog, id) -> {
      String filename = viewmodel.getProfileState().getSelectedProfile().getFileName();
      if (!filename.isEmpty()) {
        deleteProfile(filename);
      }

      viewmodel.getProfileState().setSelectedProfile(null);
      Navigation.findNavController(requireView())
          .navigate(R.id.action_navigation_preferences_to_home);
    });
    builder.show();
  };
  private final View.OnClickListener saveProfileBtn_OnClickListener = v -> {
    final MaterialAlertDialogBuilder builder = new MaterialAlertDialogBuilder(requireActivity());
    builder.setMessage(getString(R.string.pref_action_warning_text));
    builder.setNegativeButton(getString(R.string.pref_action_warning_no), (dialog, id) -> {
    });
    builder.setPositiveButton(getString(R.string.pref_action_warning_yes), (dialog, id) -> {
      final String profileName = DataFiles.cleanFileNameString(
          viewmodel.getProfileState().getSelectedProfile().getProfileName());

      if (profileName.isEmpty()) {
        Toast.makeText(requireContext(), R.string.invalid_profile_name_message, Toast.LENGTH_SHORT)
            .show();
        return;
      }
      /*
       * If profile name is changed, then new file is created. This way you can use an
       * existing
       * profile to serve as a template.
       */
      saveProfileToStorage(profileName);
      // Save user preferences to local database
      saveUserPreferencesToDb();
      // Sync user settings to cloud if device is in ONLINE mode
      syncUserSettingsToCloud();
      MedDevLog.audit("Configuration", "User preferences/report settings saved");
      Navigation.findNavController(requireView())
          .navigate(R.id.action_navigation_preferences_to_home);
    });
    builder.show();
  };
  private final View.OnClickListener volumeBtn_OnClickListener = v -> {
    VolumeChangeFragment volumeChangeDialog = new VolumeChangeFragment();
    volumeChangeDialog.show(getChildFragmentManager(), "TEST");
  };
  private final View.OnClickListener brightnessBtn_OnClickListener = v -> {
    BrightnessChangeFragment brightnessChangeDialog = new BrightnessChangeFragment();
    brightnessChangeDialog.show(getChildFragmentManager(), "TEST");
  };

  public View onCreateView(@NonNull final LayoutInflater inflater, final ViewGroup container,
      final Bundle savedInstanceState) {
    db = HydroGuideDatabase.buildDatabase(requireContext());
    usersRepository = new UsersRepository(db, new EncryptionManager(requireContext()));
    final FragmentPreferencesBinding binding = DataBindingUtil.inflate(inflater, R.layout.fragment_preferences,
        container, false);
    viewmodel = new ViewModelProvider(requireActivity()).get(MainViewModel.class);
    setupVolumeAndBrightnessListeners();
    binding.setProfileState(viewmodel.getProfileState());

    binding.preferencesTopBar.setControllerState(viewmodel.getControllerState());
    binding.preferencesTopBar.setTabletState(viewmodel.getTabletState());
    tvBattery = binding.preferencesTopBar.controllerBatteryLevelPct;
    ivBattery = binding.preferencesTopBar.remoteBatteryLevelIndicator;
    ivUsbIcon = binding.preferencesTopBar.usbIcon;
    ivTabletBattery = binding.preferencesTopBar.tabletBatteryIndicator.tabletBatteryLevelIndicator;
    tvTabletBattery = binding.preferencesTopBar.tabletBatteryIndicator.tabletBatteryLevelPct;

    if (PrefsManager.getTabletConfigurationStateIsOffline(getContext())) {
      binding.addFacility.setVisibility(View.VISIBLE);
      binding.editFacility.setVisibility(View.VISIBLE);
      binding.removeFacilities.setVisibility(View.VISIBLE);
    } else {
      binding.addFacility.setVisibility(View.GONE);
      binding.editFacility.setVisibility(View.GONE);
      binding.removeFacilities.setVisibility(View.GONE);
    }

    binding.selectAllFacilitiesBtn.setOnClickListener(selectAllFacilitiesBtn_OnClickListener);
    binding.addFacilityBtn.setOnClickListener(addFacilityBtn_OnClickListener);
    binding.editFacilityBtn.setOnClickListener(editFacilityBtn_OnClickListener);
    binding.removeFacilitiesBtn.setOnClickListener(removeFacilitiesBtn_OnClickListener);
    binding.favoriteFacilitiesBtn.setOnClickListener(favoriteFacilitiesBtn_OnClickListener);

    nameSortBtn = binding.sortFacilitiesNameBtn;
    nameSortBtn.setOnClickListener(sortFacilitiesNameBtn_OnClickListener);
    dateSortBtn = binding.sortFacilitiesDateBtn;
    dateSortBtn.setOnClickListener(sortFacilitiesDateBtn_OnClickListener);

    binding.homeButton.setOnClickListener(homeBtn_OnClickListener);
    binding.removeProfileButton.setOnClickListener(removeProfileBtn_OnClickListener);
    binding.saveProfileButton.setOnClickListener(saveProfileBtn_OnClickListener);

    binding.volumeBtn.setOnClickListener(volumeBtn_OnClickListener);
    binding.brightnessBtn.setOnClickListener(brightnessBtn_OnClickListener);
    facilityRepository = new FacilityRepository(db.facilityDao());
    Facility[] newFacilities = DbHelper.getFacilityName(facilityRepository);
    // final Facility[] facilities = viewmodel.getProfileState().getFacilityList();
    final UserPreferences preferences = viewmodel.getProfileState().getSelectedProfile().getUserPreferences();
    final String[] favIdsList = preferences.getFavoriteFacilitiesIds();
    final RecyclerView recyclerView = binding.facilityListItems;
    recyclerView.setLayoutManager(new LinearLayoutManager(getContext()));
    facilityListAdapter = new FacilityListAdapter(new ArrayList<>(Arrays.asList(newFacilities)), favIdsList);
    recyclerView.setAdapter(facilityListAdapter);
    facilityListAdapter.sortFacilities(sortMode);

    final RadioGroup captureLabelingRadioGroup = binding.captureLabelingRadioGroup;
    captureLabelingRadioGroup.check(getRadioIdFromCapture(preferences.getWaveformCaptureLabel()));
    captureLabelingRadioGroup.setOnCheckedChangeListener((radioGroup, checkedId) -> {
      if (checkedId != -1) {
        preferences.setWaveformCaptureLabel(getCaptureLabelFromRadioId(checkedId));
        // Sync to local DB
        syncWaveformCaptureLabelToDb(preferences.getWaveformCaptureLabel());
      }
    });

    final RadioGroup faciltyModeRadioGroup = binding.facilityModeRadioGroup;
    faciltyModeRadioGroup.check(getRadioIdFromFacilityMode(preferences.getFacilitySelectionMode()));
    faciltyModeRadioGroup.setOnCheckedChangeListener((radioGroup, checkedId) -> {
      if (checkedId != -1) {
        preferences.setFacilitySelectionMode(getFacilityModeFromRadioId(checkedId));
      }
    });

    final RadioGroup enableAudioRadioGroup = binding.enableAudioRadioGroup;
    enableAudioRadioGroup.check(
        preferences.isEnableAudio() ? R.id.enable_audio_on : R.id.enable_audio_off);
    enableAudioRadioGroup.setOnCheckedChangeListener((radioGroup, checkedId) -> {
      if (checkedId != -1) {
        boolean isAudioEnabled = checkedId == R.id.enable_audio_on;
        preferences.setEnableAudio(isAudioEnabled);
        // Sync to local DB
        syncAudioSettingToDb(isAudioEnabled);
      }
    });

    // Fetch and apply user preferences from local database
    fetchAndApplyUserPreferencesFromDb(captureLabelingRadioGroup, enableAudioRadioGroup);

    listenForProfileStateChange(viewmodel.getProfileState());

    final View root = binding.getRoot();

    root.setOnTouchListener((view, motionEvent) -> {
      InputUtils.hideKeyboard(requireActivity());
      root.performClick();
      return false;
    });
    requireActivity().getOnBackPressedDispatcher().addCallback(
        getViewLifecycleOwner(),
        new OnBackPressedCallback(true) {
          @Override
          public void handleOnBackPressed() {
            deleteRecord();
          }
        });

    root.getViewTreeObserver().addOnGlobalLayoutListener(
        InputUtils.getCloseKeyboardHandler(requireActivity(), root, closeKbBtn));

    return root;
  }

  private void deleteProfile(final String fileName) {
    final File profileFile = new File(requireContext().getExternalFilesDir(DataFiles.PROFILES_DIR), fileName);

    if (profileFile.delete()) {
      Toast.makeText(requireContext(), "File deleted successfully", Toast.LENGTH_SHORT).show();
    } else {
      Toast.makeText(requireContext(), "Failed to delete the file", Toast.LENGTH_SHORT).show();
    }
  }

  private int getRadioIdFromCapture(final UserPreferences.WaveformCaptureLabel label) {
    return switch (label) {
      case ExposedLength -> R.id.radio_button_capture_labeling_exposed;
      case InsertedLength -> R.id.radio_button_capture_labeling_inserted;
    };
  }

  private UserPreferences.WaveformCaptureLabel getCaptureLabelFromRadioId(final int id) {
    if (id == R.id.radio_button_capture_labeling_exposed) {
      return UserPreferences.WaveformCaptureLabel.ExposedLength;
    } else if (id == R.id.radio_button_capture_labeling_inserted) {
      return UserPreferences.WaveformCaptureLabel.InsertedLength;
    } else {
      return UserPreferences.WaveformCaptureLabel.InsertedLength;
    }
  }

  private int getRadioIdFromFacilityMode(final UserPreferences.FacilitySelectionMode mode) {
    return switch (mode) {
      case OnceManual -> R.id.radio_button_facility_mode_once_manual;
      case Dropdown -> R.id.radio_button_facility_mode_dropdown;
      case ManualEachProcedure -> R.id.radio_button_facility_mode_manual_each;
    };
  }

  private UserPreferences.FacilitySelectionMode getFacilityModeFromRadioId(final int id) {
    if (id == R.id.radio_button_facility_mode_once_manual) {
      return UserPreferences.FacilitySelectionMode.OnceManual;
    } else if (id == R.id.radio_button_facility_mode_dropdown) {
      return UserPreferences.FacilitySelectionMode.Dropdown;
    } else if (id == R.id.radio_button_facility_mode_manual_each) {
      return UserPreferences.FacilitySelectionMode.ManualEachProcedure;
    } else {
      return UserPreferences.FacilitySelectionMode.Dropdown;
    }
  }

  private Observable.OnPropertyChangedCallback propertyChangedCallback;

  private void listenForProfileStateChange(final ProfileState config) {
    propertyChangedCallback = new Observable.OnPropertyChangedCallback() {
      @Override
      public void onPropertyChanged(final Observable sender, final int propertyId) {
        final ProfileState config = (ProfileState) sender;

        if (propertyId == BR.facilityList) {
          try {
            if (!isAdded() || getActivity() == null) {
              MedDevLog.warn("ABC", "Fragment not attached, skipping facility update");
              return;
            }

            final String writeData = config.getFacilitiesDataText();
            DataFiles.writeFacilitiesToFile(writeData, requireActivity());
            facilityListAdapter.updateSavedFacilities(config.getFacilityList());

          } catch (final JSONException e) {
            MedDevLog.error("WriteLog", "Error writing facilities to file", e);
          }
        }
      }
    };

    config.addOnPropertyChangedCallback(propertyChangedCallback);
  }

  @Override
  public void onDestroyView() {
    super.onDestroyView();

    // ✅ remove callback to stop updates when fragment view is gone
    if (propertyChangedCallback != null && viewmodel != null && viewmodel.getProfileState() != null) {
      viewmodel.getProfileState().removeOnPropertyChangedCallback(propertyChangedCallback);
      propertyChangedCallback = null;
    }
  }

  private void saveProfileToStorage(final String profileName) {
    String alertMsg;
    final File profilesDir = requireContext().getExternalFilesDir(DataFiles.PROFILES_DIR);

    if (!profilesDir.exists()) {
      final boolean dirCreated = profilesDir.mkdir();
      if (dirCreated) {
        Toast.makeText(requireContext(), "Profiles directory created successfully.",
            Toast.LENGTH_SHORT).show();
      } else {
        Toast.makeText(requireContext(), "Profiles directory creation failed.",
            Toast.LENGTH_SHORT).show();
        return;
      }
    }

    // First check if file already exists. For now this is always the inserter name.
    final String fileName = profileName + ".txt";
    viewmodel.getProfileState().getSelectedProfile().setFileName(fileName);
    final File file = new File(profilesDir, fileName);

    try {
      final String[] writeData = viewmodel.getProfileState().getSelectedProfile().getProfileDataText();

      for (int i = 0; i < writeData.length; i++) {
        Log.d("ABC", "getProfileDataText: " + writeData[i]);
      }

      if (file.exists() && file.isFile()) {
        DataFiles.writeArrayToFile(file, writeData, requireContext());
        alertMsg = fileName + " was updated successfully";

      } else {
        final boolean fileCreated = file.createNewFile();

        if (fileCreated) {
          DataFiles.writeArrayToFile(file, writeData, requireContext());
        }

        alertMsg = fileCreated ? (fileName + " was created successfully") : ("Failed to create profile at" + fileName);
      }
    } catch (final Exception e) {
      MedDevLog.error("PreferencesFragment", "Error creating preference file", e);
      alertMsg = e.getMessage();
    }

    Toast.makeText(requireContext(), alertMsg, Toast.LENGTH_SHORT).show();
  }

  private void removeFacilities(final Facility[] rmFacilities) {
    final List<Facility> facilities = new ArrayList<>(Arrays.asList(viewmodel.getProfileState().getFacilityList()));
    for (final Facility rmFacility : rmFacilities) {
      facilities.remove(rmFacility);
    }
    viewmodel.getProfileState().setFacilityList(facilities.toArray(new Facility[0]));
  }

  /**
   * Sync waveform capture label setting to local database
   */
  private void syncWaveformCaptureLabelToDb(final UserPreferences.WaveformCaptureLabel label) {
    new Thread(() -> {
      try {
        String userEmail = viewmodel.getProfileState().getSelectedProfile().getProfileName();
        if (userEmail != null && !userEmail.isEmpty()) {
          UsersRepository repo = usersRepository;
          if (repo == null) {
            repo = new UsersRepository(db, new EncryptionManager(getContext()));
          }
          UsersEntity user = repo.getUserByEmail(userEmail);
          if (user != null) {
            user.setIsInsertedLengthlabellingOn(label == UserPreferences.WaveformCaptureLabel.InsertedLength);
            repo.update(user);
            Log.d("PreferencesSync", "Waveform label synced to DB: " + label);
          }
        }
      } catch (Exception e) {
        MedDevLog.error("PreferencesSync", "Error syncing waveform label to DB", e);
      }
    }).start();
  }

  /**
   * Sync audio setting to local database
   */
  private void syncAudioSettingToDb(final boolean isAudioEnabled) {
    new Thread(() -> {
      try {
        String userEmail = viewmodel.getProfileState().getSelectedProfile().getProfileName();
        if (userEmail != null && !userEmail.isEmpty()) {
          UsersRepository repo = usersRepository;
          if (repo == null) {
            repo = new UsersRepository(db, new EncryptionManager(getContext()));
          }
          UsersEntity user = repo.getUserByEmail(userEmail);
          if (user != null) {
            user.setIsAudioOn(isAudioEnabled);
            repo.update(user);
            Log.d("PreferencesSync", "Audio setting synced to DB: " + isAudioEnabled);
          }
        }
      } catch (Exception e) {
        MedDevLog.error("PreferencesSync", "Error syncing audio setting to DB", e);
      }
    }).start();
  }

  /**
   * Save user preferences to local database for the logged-in user
   */
  private void saveUserPreferencesToDb() {
    new Thread(() -> {
      try {
        String userEmail = viewmodel.getProfileState().getSelectedProfile().getProfileName();
        if (userEmail != null && !userEmail.isEmpty()) {
          UsersRepository repo = usersRepository;
          if (repo == null) {
            repo = new UsersRepository(db, new EncryptionManager(getContext()));
          }

          UsersEntity user = repo.getUserByEmail(userEmail);
          if (user != null) {
            final UserPreferences preferences = viewmodel.getProfileState().getSelectedProfile().getUserPreferences();

            // Save waveform capture label preference
            boolean isInsertedLength = preferences
                .getWaveformCaptureLabel() == UserPreferences.WaveformCaptureLabel.InsertedLength;
            user.setIsInsertedLengthlabellingOn(isInsertedLength);

            // Save audio preference
            user.setIsAudioOn(preferences.isEnableAudio());

            // Update user in database
            repo.update(user);
            Log.d("PreferencesSync", "User preferences saved to DB - Inserted Length: " + isInsertedLength + ", Audio: "
                + preferences.isEnableAudio());
          } else {
            MedDevLog.warn("PreferencesSync", "User not found in database with email: " + userEmail);
          }
        }
      } catch (Exception e) {
        MedDevLog.error("PreferencesSync", "Error saving user preferences to DB", e);
      }
    }).start();
  }

  /**
   * Fetch user preferences from local database and update UI for the logged-in
   * user
   */
  private void fetchAndApplyUserPreferencesFromDb(final RadioGroup captureLabelingRadioGroup,
      final RadioGroup enableAudioRadioGroup) {
    new Thread(() -> {
      try {
        String userEmail = viewmodel.getProfileState().getSelectedProfile().getProfileName();
        if (userEmail != null && !userEmail.isEmpty()) {
          UsersRepository repo = usersRepository;
          if (repo == null) {
            repo = new UsersRepository(db, new EncryptionManager(getContext()));
          }

          UsersEntity user = repo.getUserByEmail(userEmail);
          if (user != null) {
            final UserPreferences preferences = viewmodel.getProfileState().getSelectedProfile()
                .getUserPreferences();

            // Update waveform capture label preference
            Boolean isInsertedLengthFromDb = user.getIsInsertedLengthlabellingOn();
            if (isInsertedLengthFromDb != null) {
              UserPreferences.WaveformCaptureLabel label = isInsertedLengthFromDb
                  ? UserPreferences.WaveformCaptureLabel.InsertedLength
                  : UserPreferences.WaveformCaptureLabel.ExposedLength;
              preferences.setWaveformCaptureLabel(label);

              // Update UI on main thread
              requireActivity().runOnUiThread(() -> {
                captureLabelingRadioGroup.check(getRadioIdFromCapture(label));
                Log.d("PreferencesSync", "Waveform label loaded from DB: " + label);
              });
            }

            // Update audio preference
            Boolean isAudioOnFromDb = user.getIsAudioOn();
            if (isAudioOnFromDb != null) {
              preferences.setEnableAudio(isAudioOnFromDb);

              // Update UI on main thread
              requireActivity().runOnUiThread(() -> {
                enableAudioRadioGroup.check(
                    isAudioOnFromDb ? R.id.enable_audio_on : R.id.enable_audio_off);
                Log.d("PreferencesSync", "Audio setting loaded from DB: " + isAudioOnFromDb);
              });
            }

            Log.d("PreferencesSync", "User preferences loaded from DB successfully");
          } else {
            MedDevLog.warn("PreferencesSync", "User not found in database with email: " + userEmail);
          }
        }
      } catch (Exception e) {
        MedDevLog.error("PreferencesSync", "Error fetching user preferences from DB", e);
      }
    }).start();
  }

  /**
   * Sync user settings to cloud if device is in ONLINE mode
   */
  private void syncUserSettingsToCloud() {
    // Check if storage manager is available and in ONLINE mode
    if (storageManager != null &&
        storageManager
            .getStorageMode() == com.accessvascularinc.hydroguide.mma.storage.IStorageManager.StorageMode.ONLINE) {
      Log.d("PreferencesSync", "Device is in ONLINE mode, syncing user settings to cloud");
      syncManager.syncUserSettings();
    } else {
      Log.d("PreferencesSync", "Device is in OFFLINE mode, skipping cloud sync");
    }
  }
}