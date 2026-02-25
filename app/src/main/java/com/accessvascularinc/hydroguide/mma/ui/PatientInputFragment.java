package com.accessvascularinc.hydroguide.mma.ui;

import android.app.Dialog;
import android.content.Context;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.text.Editable;
import android.text.InputFilter;
import android.text.TextWatcher;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.inputmethod.EditorInfo;
import android.widget.ImageButton;
import android.widget.AdapterView;
import android.widget.AutoCompleteTextView;
import android.widget.EditText;
import android.widget.Toast;

import androidx.activity.OnBackPressedCallback;
import androidx.annotation.NonNull;
import androidx.databinding.DataBindingUtil;
import androidx.lifecycle.ViewModelProvider;
import androidx.navigation.Navigation;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.databinding.FragmentPatientInputBinding;
import com.accessvascularinc.hydroguide.mma.db.HydroGuideDatabase;
import com.accessvascularinc.hydroguide.mma.db.entities.FacilityEntity;
import com.accessvascularinc.hydroguide.mma.db.repository.FacilityRepository;
import com.accessvascularinc.hydroguide.mma.model.DataFiles;
import com.accessvascularinc.hydroguide.mma.model.DisplayedEntryFields;
import com.accessvascularinc.hydroguide.mma.model.EncounterData;
import com.accessvascularinc.hydroguide.mma.model.Facility;
import com.accessvascularinc.hydroguide.mma.model.InputOptions;
import com.accessvascularinc.hydroguide.mma.model.PatientData;
import com.accessvascularinc.hydroguide.mma.model.ProfileState;
import com.accessvascularinc.hydroguide.mma.model.UserPreferences;
import com.accessvascularinc.hydroguide.mma.model.UserProfile;
import com.accessvascularinc.hydroguide.mma.ui.input.EditFacilityDialogFragment;
import com.accessvascularinc.hydroguide.mma.ui.input.FacilitiesAutoCompleteAdapter;
import com.accessvascularinc.hydroguide.mma.ui.input.InputUtils;
import com.accessvascularinc.hydroguide.mma.ultrasound.GlobalUltrasoundManager;
import com.accessvascularinc.hydroguide.mma.utils.DbHelper;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import com.accessvascularinc.hydroguide.mma.utils.PrefsManager;
import com.accessvascularinc.hydroguide.mma.utils.Utility;
import com.echonous.hardware.ultrasound.UltrasoundManager;
import com.google.android.material.datepicker.MaterialDatePicker;
import com.google.android.material.datepicker.CalendarConstraints;
import com.google.android.material.datepicker.DateValidatorPointBackward;
import com.accessvascularinc.hydroguide.mma.ui.settings.VolumeChangeFragment;
import com.accessvascularinc.hydroguide.mma.ui.settings.BrightnessChangeFragment;

import org.json.JSONException;

import java.text.SimpleDateFormat;
import java.util.HashMap;
import java.util.Date;
import java.util.List;
import java.util.Locale;
import java.util.OptionalDouble;

public class PatientInputFragment extends BaseHydroGuideFragment {
  private FragmentPatientInputBinding binding = null;
  private final Dialog closeKbBtn = null;
  private EditText etArmCircum = null, etVeinSize = null;
  private EditText etNoOfLumens = null, etCatheterSize = null;
  private AutoCompleteTextView insertVeinDp = null, insertSideDp = null;
  private AutoCompleteTextView facilitiesDp = null;
  private FacilitiesAutoCompleteAdapter facilitySelectAdapter = null;
  private boolean manualFacilityInput = false;
  private HydroGuideDatabase db;
  private UserProfile profile = null;

  private final AdapterView.OnItemSelectedListener insertVeinDropdown_OnItemSelectedListener = new AdapterView.OnItemSelectedListener() {
    @Override
    public void onItemSelected(final AdapterView<?> adapterView, final View view,
        final int i, final long l) {
    }

    @Override
    public void onNothingSelected(final AdapterView<?> adapterView) {
    }
  };
  private final AdapterView.OnItemClickListener facilityDp_OnItemClickListener = (aV, v, i, l) -> {
    final PatientData patient = viewmodel.getEncounterData().getPatient();

    // Clear any previous error when a facility is selected
    facilitiesDp.setError(null);

    // Hide the warning label when a valid facility is selected
    if (binding != null && binding.facilityWarningLabel != null) {
      binding.facilityWarningLabel.setVisibility(View.GONE);
    }

    Facility facility = facilitySelectAdapter.getItem(i);
    patient.setPatientFacility(facility);

    // Find the FacilityEntity for this facility (by name or id)
    FacilityEntity selectedEntity = null;
    if (facility.getId() != null && !facility.getId().isEmpty()) {
      try {
        long fid = Long.parseLong(facility.getId());
        List<FacilityEntity> allEntities = DbHelper.getAllFacilities(new FacilityRepository(db.facilityDao()));
        for (FacilityEntity entity : allEntities) {
          if (entity.getFacilityId() != null && entity.getFacilityId() == fid) {
            selectedEntity = entity;
            break;
          }
        }
      } catch (NumberFormatException ignore) {
      }
    }
    if (selectedEntity == null) {
      // fallback: match by name
      List<FacilityEntity> allEntities = DbHelper.getAllFacilities(new FacilityRepository(db.facilityDao()));
      for (FacilityEntity entity : allEntities) {
        if (entity.getFacilityName() != null &&
            entity.getFacilityName().equals(facility.getFacilityName())) {
          selectedEntity = entity;
          break;
        }
      }
    }
    if (selectedEntity != null && profile != null) {
      // Set user preferences for displayedEntryFields using setters
      DisplayedEntryFields fields = profile.getUserPreferences().getDisplayedEntryFields();
      fields.setShowName(selectedEntity.isShowPatientName());
      fields.setShowDob(selectedEntity.isShowPatientDob());
      fields.setShowId(selectedEntity.isShowPatientId());
      fields.setShowArmCircumference(selectedEntity.isShowArmCircumference());
      fields.setShowVeinSize(selectedEntity.isShowVeinSize());
      fields.setShowInsertionVein(selectedEntity.isShowInsertionVein());
      fields.setShowPatientSide(selectedEntity.isShowPatientSide());
      fields.setShowReasonForInsertion(selectedEntity.isShowReasonForInsertion());
      fields.setShowNoOfLumens(selectedEntity.isShowNoOfLumens());
      fields.setShowCatheterSize(selectedEntity.isShowCatheterSize());
      // Notify data binding of change
      profile.getUserPreferences().setDisplayedEntryFields(fields);
      binding.setUserProfile(profile);
    }
  };

  private final OnBackPressedCallback backButtonPressed_Listener = new OnBackPressedCallback(true) {
    @Override
    public void handleOnBackPressed() {
      cleanUpInput();
    }
  };
  private final View.OnClickListener homeBtn_OnClickListener = v -> {
    cleanUpInput();
    if (viewmodel.getEncounterData().getPatient().hasAnyData()) {
      onProcedureExitAttempt(() -> {
        viewmodel.getEncounterData().clearEncounterData();
        viewmodel.getMonitorData().clearMonitorData();
        Navigation.findNavController(requireView())
            .navigate(R.id.action_navigation_patient_input_to_home);
      });
    } else {
      viewmodel.getEncounterData().clearEncounterData();
      viewmodel.getMonitorData().clearMonitorData();
      Navigation.findNavController(requireView())
          .navigate(R.id.action_navigation_patient_input_to_home);
    }
  };

  private final View.OnClickListener navToProcedure_OnClickListener = v -> {
    final EncounterData procedure = viewmodel.getEncounterData();

    // Facility validation: must be selected and cannot be changed for this
    // procedure
    Facility procFacility = procedure.getPatient().getPatientFacility();
    String selectedFacilityName = facilitiesDp.getText().toString().trim();
    final String VeinSize = etVeinSize.getText().toString();
    final String ArmCirumference = etArmCircum.getText().toString();
    final String NoOfLumens = etNoOfLumens.getText().toString();
    final String CatheterSize = etCatheterSize.getText().toString();
    // Validate vein size - should not exceed 15mm (1.5cm) and not less than 1mm (0.1cm)
    if (!VeinSize.isEmpty()) {
      try {
        double veinSize = Double.parseDouble(VeinSize);
        if (veinSize < 1 || veinSize > 15) {
          if(veinSize < 1){
            etVeinSize.setError(getString(R.string.patient_vein_size_error_min));
          }else {
            etVeinSize.setError(getString(R.string.patient_vein_size_error_max));
          }
          etVeinSize.requestFocus();
          return;
        }
      } catch (NumberFormatException e) {
        etVeinSize.setError(getString(R.string.patient_vein_size_error_invalid));
        etVeinSize.requestFocus();
        return;
      }
    }
    if(!ArmCirumference.isEmpty()){
      try {
        double armCircumFerence = Double.parseDouble(ArmCirumference);
        if (armCircumFerence < 10 || armCircumFerence > 150) {
          if(armCircumFerence < 10){
            etArmCircum.setError(getString(R.string.patient_arm_circumference_error_min));
          }else {
            etArmCircum.setError(getString(R.string.patient_arm_circumference_error_max));
          }
          etArmCircum.requestFocus();
          return;
        }
      }
      catch (NumberFormatException e){
        etArmCircum.setError(getString(R.string.patient_arm_circumference_error_invalid));
        etArmCircum.requestFocus();
        return;
      }
    }
    //no. of lumens
    if(!NoOfLumens.isEmpty()){
      try {
        double noOfLumens = Double.parseDouble(NoOfLumens);
        if (noOfLumens < 1 || noOfLumens > 5) {
          if(noOfLumens < 1){
            etNoOfLumens.setError(getString(R.string.patient_no_of_lumens_error_min));
          }else {
            etNoOfLumens.setError(getString(R.string.patient_no_of_lumens_error_max));
          }
          etNoOfLumens.requestFocus();
          return;
        }
      }
      catch (NumberFormatException e){
        etNoOfLumens.setError(getString(R.string.patient_no_of_lumens_error_invalid));
        etNoOfLumens.requestFocus();
        return;
      }
    }

    //Catheter Size
    if(!CatheterSize.isEmpty()){
      try {
        double catheterSize = Double.parseDouble(CatheterSize);
        if (catheterSize < 1 || catheterSize > 10) {
          if(catheterSize < 1){
            etCatheterSize.setError(getString(R.string.patient_catheter_size_error_min));
          }else {
            etCatheterSize.setError(getString(R.string.patient_catheter_size_error_max));
          }
          etCatheterSize.requestFocus();
          return;
        }
      }
      catch (NumberFormatException e){
        etCatheterSize.setError(getString(R.string.patient_catheter_size_error_invalid));
        etCatheterSize.requestFocus();
        return;
      }
    }


    if(binding.patientIdInput.getText().toString().isEmpty()){
      binding.patientIdInput.setError(getString(R.string.patient_id_required_error));
      Toast.makeText(requireContext(), getString(R.string.patient_id_required_error), Toast.LENGTH_SHORT).show();
      return;
    }

    if (selectedFacilityName.isEmpty()) {
      facilitiesDp.setError(getString(R.string.patient_facility_required_error));
      Toast.makeText(requireContext(), getString(R.string.patient_facility_required_toast), Toast.LENGTH_SHORT).show();
      return;
    }
    // If manual input, set the facility name
    if (manualFacilityInput) {
      procFacility.setFacilityName(selectedFacilityName);
    }
    // Prevent changing facility after selection for this procedure
    if (facilitiesDp.isEnabled()) {
      facilitiesDp.setEnabled(false);
    }
    procFacility.setDateLastUsed(procedure.getStartTimeText());
    viewmodel.getProfileState().updateFacilityRecency(procFacility, requireContext());

    final String strArmCircum = etArmCircum.getText().toString();
    final OptionalDouble newArmCircum = !strArmCircum.isEmpty() ? OptionalDouble.of(Double.parseDouble(strArmCircum))
        : OptionalDouble.empty();
    procedure.getPatient().setPatientArmCircumference(newArmCircum);

    final String strVeinSize = etVeinSize.getText().toString();
    final OptionalDouble currVeinSize = !strVeinSize.isEmpty() ? OptionalDouble.of(Double.parseDouble(strVeinSize))
        : OptionalDouble.empty();
    procedure.getPatient().setPatientVeinSize(currVeinSize);

    final String strNoOfLumens = etNoOfLumens.getText().toString();
    procedure.getPatient().setPatientNoOfLumens(strNoOfLumens.isEmpty() ? "" : strNoOfLumens);

    final String strCatheterSize = etCatheterSize.getText().toString();
    procedure.getPatient().setPatientCatheterSize(strCatheterSize.isEmpty() ? "" : strCatheterSize);

    if (procedure.isLive()) {
      updateProcedureFile();
    }
    
    MedDevLog.audit("Patient", "Patient profile created for procedure");
    TryNavigateToUltrasound(Utility.FROM_SCREEN_PATIENT_INPUT);
  };

  public View onCreateView(@NonNull final LayoutInflater inflater, final ViewGroup container,
      final Bundle savedInstanceState) {
    db = HydroGuideDatabase.buildDatabase(requireContext());
    MedDevLog.info("Navigation", "Patient input screen entered");
    binding = DataBindingUtil.inflate(inflater, R.layout.fragment_patient_input, container, false);
    viewmodel = new ViewModelProvider(requireActivity()).get(MainViewModel.class);
    // Trigger facility sync with a callback to refresh dropdown on success.
    syncManager.syncFacility(() -> {
      try {
        requireActivity().runOnUiThread(this::refreshFacilitiesDropdown);
      } catch (Exception ignored) {
      }
    });

    final EncounterData procedure = viewmodel.getEncounterData();
    binding.patientIdInput.setFilters(new InputFilter[] { Utility.allowOnlyAlphanumeric() });
    profile = viewmodel.getProfileState().getSelectedProfile();
    Log.d("ABC", "9: ");
    binding.setEncounterData(procedure);
    Log.d("ABC", "profile: " + profile);
    Log.d("ABC", "getProfileName: " + profile.getProfileName());
    binding.setUserProfile(profile);
    binding.inserterNameInput.setText(profile.getProfileName());
    requireActivity().getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

    setupVolumeAndBrightnessListeners();

    // TODO: Use this for when the screen should be allowed to sleep again
    if (!procedure.isLive()) {
      requireActivity().getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }

    binding.patientInputTopBar.setControllerState(viewmodel.getControllerState());
    binding.patientInputTopBar.setTabletState(viewmodel.getTabletState());
    tvBattery = binding.patientInputTopBar.controllerBatteryLevelPct;
    ivBattery = binding.patientInputTopBar.remoteBatteryLevelIndicator;
    ivUsbIcon = binding.patientInputTopBar.usbIcon;
    ivTabletBattery = binding.patientInputTopBar.tabletBatteryIndicator.tabletBatteryLevelIndicator;
    tvTabletBattery = binding.patientInputTopBar.tabletBatteryIndicator.tabletBatteryLevelPct;
    restrictedEvents.add(
        new RestrictedEvent(binding.navToProcedureScreen, navToProcedure_OnClickListener,
            RESTRICTION_TYPE.LOW_POWER));
    restrictedEvents.add(
        new RestrictedEvent(binding.navToProcedureScreen, navToProcedure_OnClickListener,
            RESTRICTION_TYPE.CONTROLLER_STATUS_TIMEOUT));
    lowBatteryIndicators.add(binding.batteryStatusPatientPbtn);
    errorIndicators.add(binding.errorIndicatorPatientPbtn);
    hgLogo = binding.patientInputTopBar.appLogo;
    lpTabletIcon = binding.patientInputTopBar.tabletBatteryWarningIcon.batteryIcon;
    lpTabletSymbol = binding.patientInputTopBar.tabletBatteryWarningIcon.batteryWarningImg;

    binding.patientInputTopBar.remoteBatteryLevelIndicator.setVisibility(View.GONE);
    binding.patientInputTopBar.controllerBatteryLabel.setVisibility(View.GONE);
    binding.patientInputTopBar.controllerBatteryLevelPct.setVisibility(View.GONE);
    binding.patientInputTopBar.usbIcon.setVisibility(View.GONE);

    requireActivity().getOnBackPressedDispatcher().addCallback(backButtonPressed_Listener);

    final PatientData patient = procedure.getPatient();
    final EditText tilDob = binding.patientDobInput;
    tilDob.setText(patient.getPatientDob());
    tilDob.setOnClickListener(view -> {
      final String patientDob = patient.getPatientDob();

      // Create calendar constraints to restrict selection to dates before today
      CalendarConstraints.Builder constraintsBuilder = new CalendarConstraints.Builder();
      constraintsBuilder.setEnd(MaterialDatePicker.todayInUtcMilliseconds());
      constraintsBuilder.setValidator(DateValidatorPointBackward.now());

      final MaterialDatePicker<Long> datePicker = MaterialDatePicker.Builder.datePicker()
          .setTitleText("Select Patient's Date of Birth")
          .setSelection(!patientDob.isEmpty() ? InputUtils.parseDateToMillis(patientDob)
              : null)
          .setCalendarConstraints(constraintsBuilder.build())
          .setInputMode(MaterialDatePicker.INPUT_MODE_TEXT)
          .build();
      datePicker.addOnPositiveButtonClickListener(selection -> {
        Log.d("ABC", "-415152000000: " + selection);
        patient.setPatientDob(InputUtils.getDateStringFromMillis(selection));
      });
      datePicker.show(getChildFragmentManager(), "Date Picker");
    });

    etArmCircum = binding.patientArmCircInput;
    etArmCircum.setFilters(new InputFilter[] { new InputUtils.DecimalDigitsInputFilter(4, 2) });
    final OptionalDouble currArmCirc = patient.getPatientArmCircumference();
    etArmCircum.setText(currArmCirc.isPresent() ? Double.toString(currArmCirc.getAsDouble()) : "");

    etVeinSize = binding.patientVeinSizeInput;
    etVeinSize.setFilters(new InputFilter[] { new InputUtils.DecimalDigitsInputFilter(4, 2) });
    final OptionalDouble currVeinSize = patient.getPatientVeinSize();
    etVeinSize.setText(currVeinSize.isPresent() ? Double.toString(currVeinSize.getAsDouble()) : "");

    etNoOfLumens = binding.patientNoOfLumensInput;
    etNoOfLumens.setText(patient.getPatientNoOfLumens() != null ? patient.getPatientNoOfLumens() : "");

    etCatheterSize = binding.patientCatheterSizeInput;
    etCatheterSize.setText(patient.getPatientCatheterSize() != null ? patient.getPatientCatheterSize() : "");

    facilitiesDp = binding.facilityInput;

    // Add TextWatcher to monitor facility input changes
    facilitiesDp.addTextChangedListener(new TextWatcher() {
      @Override
      public void beforeTextChanged(CharSequence s, int start, int count, int after) {
        // Not needed
      }

      @Override
      public void onTextChanged(CharSequence s, int start, int before, int count) {
        // Not needed
      }

      @Override
      public void afterTextChanged(Editable s) {
        // Show warning label if facility input is empty or null
        if (binding != null && binding.facilityWarningLabel != null) {
          if (s == null || s.toString().trim().isEmpty()) {
            binding.facilityWarningLabel.setVisibility(View.VISIBLE);
          } else {
            binding.facilityWarningLabel.setVisibility(View.GONE);
          }
        }
      }
    });

    if (profile != null) {
      final UserPreferences preferences = profile.getUserPreferences();
      final Facility procedureFacility = patient.getPatientFacility();

      switch (preferences.getFacilitySelectionMode()) {
        case OnceManual -> {
          Log.d("ABC", "OnceManual: ");
          manualFacilityInput = false;
          final Facility manualFacility = preferences.getManualFacility();
          patient.setPatientFacility(manualFacility);
          facilitiesDp.setText((manualFacility != null) ? manualFacility.getFacilityName() : "");
          facilitiesDp.setEnabled(false);
        }
        case Dropdown -> {
          Log.d("ABC", "Dropdown: ");
          manualFacilityInput = false;
          facilitiesDp.setText(procedureFacility.getFacilityName());
          FacilityRepository facilityDetailsRepository = new FacilityRepository(db.facilityDao());
          List<FacilityEntity> facilityEntities = DbHelper.getAllFacilities(facilityDetailsRepository);

          // final Facility[] facilities = viewmodel.getProfileState().getFacilityList();
          final String[] faves = preferences.getFavoriteFacilitiesIds();
          Log.d("ABC", "facilityEntities size: " + facilityEntities.size());
          facilitySelectAdapter = new FacilitiesAutoCompleteAdapter(requireContext(), R.layout.facility_list_item,
              R.id.facility_list_item_text, facilityEntities, faves, true);
          facilitiesDp.setAdapter(facilitySelectAdapter);
          facilitiesDp.setOnItemClickListener(facilityDp_OnItemClickListener);

          // Show/hide warning based on initial facility value
          if (binding.facilityWarningLabel != null) {
            if (procedureFacility.getFacilityName() == null || procedureFacility.getFacilityName().isEmpty()) {
              binding.facilityWarningLabel.setVisibility(View.VISIBLE);
            } else {
              binding.facilityWarningLabel.setVisibility(View.GONE);
            }
          }
        }
        case ManualEachProcedure -> {
          Log.d("ABC", "ManualEachProcedure: ");
          manualFacilityInput = true;
          facilitiesDp.setText(procedureFacility.getFacilityName());
          facilitiesDp.setInputType(EditorInfo.TYPE_CLASS_TEXT);

          // Show/hide warning based on initial facility value
          if (binding.facilityWarningLabel != null) {
            if (procedureFacility.getFacilityName() == null || procedureFacility.getFacilityName().isEmpty()) {
              binding.facilityWarningLabel.setVisibility(View.VISIBLE);
            } else {
              binding.facilityWarningLabel.setVisibility(View.GONE);
            }
          }
        }
      }
    }

    insertVeinDp = binding.patientInsertVeinInput; // @={encounterData.patient.patientInsertionVein}
    insertVeinDp.setAdapter(
        new InputUtils.NonFilterArrayAdapter<>(requireContext(), R.layout.dropdown_list_item,
            InputOptions.VEIN_OPTIONS));

    final View root = binding.getRoot();
    ImageButton volumeBtn = root.findViewById(R.id.volume_btn);
    ImageButton brightnessBtn = root.findViewById(R.id.brightness_btn);
    ImageButton muteBtn = root.findViewById(R.id.mute_btn);

    if (volumeBtn != null) {
      volumeBtn.setOnClickListener(v -> {
        VolumeChangeFragment bottomSheet = new VolumeChangeFragment();
        bottomSheet.show(getChildFragmentManager(), "TEST");
      });
    }
    if (brightnessBtn != null) {
      brightnessBtn.setOnClickListener(v -> {
        BrightnessChangeFragment bottomSheet = new BrightnessChangeFragment();
        bottomSheet.show(getChildFragmentManager(), "TEST");
      });
    }
    if (muteBtn != null) {
      muteBtn.setOnClickListener(v -> {
        viewmodel.getTabletState().setTabletMuted(!viewmodel.getTabletState().getIsTabletMuted());
        updateMuteBtnDrawable(muteBtn);
      });
      // Set initial icon state
      updateMuteBtnDrawable(muteBtn);
    }

    insertVeinDp.setOnItemSelectedListener(insertVeinDropdown_OnItemSelectedListener);

    insertSideDp = binding.patientInsertSideInput;
    insertSideDp.setAdapter(
        new InputUtils.NonFilterArrayAdapter<>(requireContext(), R.layout.dropdown_list_item,
            InputOptions.SIDE_OPTIONS));

    binding.navToProcedureScreen.setOnClickListener(navToProcedure_OnClickListener);
    binding.homeButton.setOnClickListener(homeBtn_OnClickListener);

    root.setOnTouchListener((view, motionEvent) -> {
      InputUtils.hideKeyboard(requireActivity());
      root.performClick();
      return false;
    });

    root.getViewTreeObserver().addOnGlobalLayoutListener(
        InputUtils.getCloseKeyboardHandler(requireActivity(), root, closeKbBtn));

    return root;
  }

  private void updateMuteBtnDrawable(ImageButton muteBtn) {
    muteBtn.setImageDrawable(androidx.core.content.ContextCompat.getDrawable(requireContext(),
        (viewmodel.getTabletState().getIsTabletMuted()) ? R.drawable.button_active_mute
            : R.drawable.button_neutral_mute));
  }

  // TODO : Kept for reference, can be removed later
  private boolean isProbeConnectedAndPermission() {
    try {
      UsbManager usbManager = (UsbManager) requireContext().getSystemService(Context.USB_SERVICE);
      if (usbManager == null) {
        return false;
      }

      // Find the probe device
      HashMap<String, UsbDevice> deviceList = usbManager.getDeviceList();
      for (UsbDevice usbDevice : deviceList.values()) {
        // Check for EchoNous probe (vendor ID 7615 / 0x1dbf)
        if (usbDevice.getVendorId() == 0x1dbf) {
          // Probe device found - return true regardless of permission status
          // ScanningUltrasoundFragment will handle permission requests if needed
          Log.d("PatientInput", "Probe device found, has permission: " + usbManager.hasPermission(usbDevice));

          // If we have permission, check if DAU is actually connected
          if (usbManager.hasPermission(usbDevice)) {
            boolean isConnected = GlobalUltrasoundManager.getInstance()
                .isDauConnected(UltrasoundManager.DauCommMode.Usb, usbDevice);
            return isConnected;
          }

          // If no permission yet, still return true so we navigate to ScanningUltrasound
          // which will request permissions
          return true;
        }
      }
    } catch (Exception e) {
      MedDevLog.error("PatientInput", "Error checking probe connection", e);
    }
    return false;
  }

  @Override
  public void onPause() {
    super.onPause();
    // cleanUpInput();
  }

  private void cleanUpInput() {
    final String strArmCircum = etArmCircum.getText().toString();
    final OptionalDouble newArmCircum = !strArmCircum.isEmpty() ? OptionalDouble.of(Double.parseDouble(strArmCircum))
        : OptionalDouble.empty();
    if (newArmCircum.isPresent()) {
      viewmodel.getEncounterData().getPatient().setPatientArmCircumference(newArmCircum);
    }

    final String strVeinSize = etVeinSize.getText().toString();
    final OptionalDouble currVeinSize = !strVeinSize.isEmpty() ? OptionalDouble.of(Double.parseDouble(strVeinSize))
        : OptionalDouble.empty();
    if (newArmCircum.isPresent()) {
      viewmodel.getEncounterData().getPatient().setPatientVeinSize(currVeinSize);
    }

    final String strNoOfLumens = etNoOfLumens.getText().toString();
    if (!strNoOfLumens.isEmpty()) {
      viewmodel.getEncounterData().getPatient().setPatientNoOfLumens(strNoOfLumens);
    }

    final String strCatheterSize = etCatheterSize.getText().toString();
    if (!strCatheterSize.isEmpty()) {
      viewmodel.getEncounterData().getPatient().setPatientCatheterSize(strCatheterSize);
    }
  }

  @Override
  public void onDestroyView() {
    // cleanUpInput();
    super.onDestroyView();
    binding = null;
  }

  /**
   * Refreshes the facilities dropdown after a successful sync operation.
   * Re-fetches FacilityEntity
   * list and updates or creates the adapter if needed.
   */
  private void refreshFacilitiesDropdown() {
    if (binding == null || facilitiesDp == null || profile == null) {
      return;
    }
    final UserPreferences preferences = profile.getUserPreferences();
    if (preferences.getFacilitySelectionMode() != UserPreferences.FacilitySelectionMode.Dropdown) {
      return; // Only applicable for dropdown mode.
    }
    FacilityRepository repo = new FacilityRepository(db.facilityDao());
    List<FacilityEntity> entities = DbHelper.getAllFacilities(repo);
    String[] faves = preferences.getFavoriteFacilitiesIds();
    if (facilitySelectAdapter == null) {
      facilitySelectAdapter = new FacilitiesAutoCompleteAdapter(requireContext(), R.layout.facility_list_item,
          R.id.facility_list_item_text, entities, faves, true);
      facilitiesDp.setAdapter(facilitySelectAdapter);
      facilitiesDp.setOnItemClickListener(facilityDp_OnItemClickListener);
    } else {
      facilitySelectAdapter.updateFacilities(entities, faves);
    }
    // Preserve current selection text.
    final Facility currentFacility = viewmodel.getEncounterData().getPatient().getPatientFacility();
    if (currentFacility != null && currentFacility.getFacilityName() != null) {
      facilitiesDp.setText(currentFacility.getFacilityName(), false);
    }
    Toast.makeText(requireContext(), getString(R.string.patient_facilities_loaded), Toast.LENGTH_SHORT).show();
  }
}