package com.accessvascularinc.hydroguide.mma.ui;

import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.ImageButton;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.databinding.DataBindingUtil;
import androidx.lifecycle.ViewModelProvider;
import androidx.navigation.Navigation;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.databinding.FragmentSetupBinding;
import com.accessvascularinc.hydroguide.mma.model.ControllerState;
import com.accessvascularinc.hydroguide.mma.utils.Utility;
import com.accessvascularinc.hydroguide.mma.model.EncounterData;
import com.accessvascularinc.hydroguide.mma.ui.settings.BrightnessChangeFragment;
import com.accessvascularinc.hydroguide.mma.ui.settings.VolumeChangeFragment;

import java.util.OptionalDouble;

import dagger.hilt.android.AndroidEntryPoint;

/**
 * Setup Fragment - Displays the setup screen for electrode placement and catheter configuration
 * This screen appears after Home and before Patient Input in the procedure flow
 */
@AndroidEntryPoint
public class SetupFragment extends BaseHydroGuideFragment {

  private FragmentSetupBinding binding = null;
  private EditText etDistance1 = null;
  private EditText etDistance2 = null;
  private EditText etDistance3 = null;
  private EditText etTrimmedLength = null;
  double MaxDistanceCm = 150.0; // Maximum allowed distance in cm (1 meter)
  double MinDistanceCm = 10.0;
  /**
   * Click listener for the Back button - returns to Home screen
   */
  private final View.OnClickListener backBtn_OnClickListener = v ->
        Navigation.findNavController(requireView()).popBackStack();

  /**
   * Click listener for the Next button - proceeds to Patient Input screen
   * Saves all setup values to EncounterData before navigating
   */
  private final View.OnClickListener nextBtn_OnClickListener = v -> {

    String strDistance1 = etDistance1.getText().toString().trim();
    String strDistance2 = etDistance2.getText().toString().trim();
    String strDistance3 = etDistance3.getText().toString().trim();
    String strTrimLength = etTrimmedLength.getText().toString().trim();

    if (strDistance1.isEmpty() && strDistance2.isEmpty() && strDistance3.isEmpty()) {
      etDistance1.setError(getString(R.string.setupscreen_distance_required_error));
      etDistance1.requestFocus();
      return;
    }

    if (!strDistance1.isEmpty()) {
        double distance1 = Double.parseDouble(strDistance1);
        if (distance1 > MaxDistanceCm || distance1 < MinDistanceCm) {
            etDistance1.setError(getString(R.string.setupscreen_distance_error_message));
            etDistance1.requestFocus();
            return;
        }
    }

    if (!strDistance2.isEmpty()) {
        double distance2 = Double.parseDouble(strDistance2);
        if (distance2 > MaxDistanceCm || distance2 < MinDistanceCm) {
            etDistance2.setError(getString(R.string.setupscreen_distance_error_message));
            etDistance2.requestFocus();
            return;
        }
    }

    if (!strDistance3.isEmpty()) {
        double distance3 = Double.parseDouble(strDistance3);
        if (distance3 > MaxDistanceCm || distance3 < MinDistanceCm) {
            etDistance3.setError(getString(R.string.setupscreen_distance_error_message));
            etDistance3.requestFocus();
            return;
        }
    }
    if(!strTrimLength.isEmpty()){
      double trimLength = Double.parseDouble(strTrimLength);
      if (trimLength < 0 || trimLength > 200) {
        etTrimmedLength.setError(getString(R.string.setupscreen_trimlength_error_message));
        etTrimmedLength.requestFocus();
        return;
      }
    }

    // Save all input values to EncounterData
    saveSetupDataToEncounter();

    if (viewmodel.getControllerCommunicationManager().checkConnection()) {
      TryNavigateToUltrasound(Utility.FROM_SCREEN_SETUP, true, R.id.action_navigation_setup_to_calibration_screen);
    } else {
      // Navigate to Patient Input screen
      Navigation.findNavController(requireView())
              .navigate(R.id.action_navigation_setup_to_bluetooth_pairing);
    }
  };

  /**
   * Click listener for the Home button - returns to Ultrasound Capture Screen
   */
  private final View.OnClickListener homeBtn_OnClickListener = v ->
          TryNavigateToUltrasound(Utility.FROM_SCREEN_PATIENT_INPUT, false
                  , R.id.action_navigation_setup_to_patient_input);

  /**
   * Click listener for the Volume button - shows volume control bottom sheet
   */
  private final View.OnClickListener volumeBtn_OnClickListener = v -> {
    VolumeChangeFragment bottomSheet = new VolumeChangeFragment();
    bottomSheet.show(getChildFragmentManager(), "VolumeChange");
  };

  /**
   * Click listener for the Brightness button - shows brightness control bottom sheet
   */
  private final View.OnClickListener brightnessBtn_OnClickListener = v -> {
    BrightnessChangeFragment bottomSheet = new BrightnessChangeFragment();
    bottomSheet.show(getChildFragmentManager(), "BrightnessChange");
  };

  /**
   * Click listener for the Mute button - toggles audio muting
   */
  private final View.OnClickListener muteBtn_OnClickListener = v -> {
    // Toggle mute functionality (reusing from BaseHydroGuideFragment if available)
    if (viewmodel != null) {
      boolean currentMuteState = viewmodel.getTabletState().getIsTabletMuted();
      viewmodel.getTabletState().setTabletMuted(!currentMuteState);
    }
  };

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState) {
    // Inflate the layout using data binding
    binding = DataBindingUtil.inflate(inflater, R.layout.fragment_setup, container, false);

    // Get the MainViewModel instance shared across fragments
    viewmodel = new ViewModelProvider(requireActivity()).get(MainViewModel.class);

    // Get the EncounterData from the view model
    EncounterData encounterData = viewmodel.getEncounterData();
    binding.setEncounterData(encounterData);

    // Set up the controller state for the top bar
    ControllerState controllerState = viewmodel.getControllerState();
    binding.setupTopBar.setControllerState(controllerState);
    binding.setupTopBar.setTabletState(viewmodel.getTabletState());

    // Initialize references to UI elements for BaseHydroGuideFragment
    tvBattery = binding.setupTopBar.controllerBatteryLevelPct;
    ivBattery = binding.setupTopBar.remoteBatteryLevelIndicator;
    ivUsbIcon = binding.setupTopBar.usbIcon;
    ivTabletBattery = binding.setupTopBar.tabletBatteryIndicator.tabletBatteryLevelIndicator;
    tvTabletBattery = binding.setupTopBar.tabletBatteryIndicator.tabletBatteryLevelPct;
    hgLogo = binding.setupTopBar.appLogo;
    lpTabletIcon = binding.setupTopBar.tabletBatteryWarningIcon.batteryIcon;
    lpTabletSymbol = binding.setupTopBar.tabletBatteryWarningIcon.batteryWarningImg;

    binding.setupTopBar.remoteBatteryLevelIndicator.setVisibility(View.GONE);
    binding.setupTopBar.controllerBatteryLabel.setVisibility(View.GONE);
    binding.setupTopBar.controllerBatteryLevelPct.setVisibility(View.GONE);
    binding.setupTopBar.usbIcon.setVisibility(View.GONE);

    return binding.getRoot();
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);

    // Initialize input field references
    etDistance1 = binding.etDistance1;
    etDistance2 = binding.etDistance2;
    etDistance3 = binding.etDistance3;
    etTrimmedLength = binding.etTrimmedLength;

    // Set up button click listeners
//        ImageButton backButton = binding.backButton;
    ImageButton nextButton = binding.nextButton;
    ImageButton homeButton = binding.homeButton;
    ImageButton volumeBtn = binding.volumeBtn;
    ImageButton brightnessBtn = binding.brightnessBtn;
    ImageButton muteBtn = binding.muteBtn;

//        backButton.setOnClickListener(backBtn_OnClickListener);
        nextButton.setOnClickListener(nextBtn_OnClickListener);
        homeButton.setOnClickListener(homeBtn_OnClickListener);
        volumeBtn.setOnClickListener(volumeBtn_OnClickListener);
        brightnessBtn.setOnClickListener(brightnessBtn_OnClickListener);
        muteBtn.setOnClickListener(muteBtn_OnClickListener);

        // Load any existing values from EncounterData if available
        loadSetupDataFromEncounter();
    }

    /**
     * Loads existing setup data from EncounterData into the input fields.
     * This allows users to see previously entered values if they navigate back to this screen.
     */
    private void loadSetupDataFromEncounter() {
        if (viewmodel == null || viewmodel.getEncounterData() == null) {
            return;
        }

        final EncounterData encounterData = viewmodel.getEncounterData();

        // Load electrode distance 1
        if (encounterData.getElectrodeDistance1Cm().isPresent()) {
            etDistance1.setText(String.valueOf(encounterData.getElectrodeDistance1Cm().getAsDouble()));
        }

        // Load electrode distance 2
        if (encounterData.getElectrodeDistance2Cm().isPresent()) {
            etDistance2.setText(String.valueOf(encounterData.getElectrodeDistance2Cm().getAsDouble()));
        }

        // Load electrode distance 3
        if (encounterData.getElectrodeDistance3Cm().isPresent()) {
            etDistance3.setText(String.valueOf(encounterData.getElectrodeDistance3Cm().getAsDouble()));
        }

        // Load trimmed length (using existing field in EncounterData)
        if (encounterData.getTrimLengthCm().isPresent()) {
            etTrimmedLength.setText(String.valueOf(encounterData.getTrimLengthCm().getAsDouble()));
        }
    }

    /**
     * Saves all setup input values to EncounterData.
     * This ensures the data persists across screen navigation and app crashes.
     * Pattern follows PatientInputFragment's approach of using OptionalDouble for numeric values.
     */
    private void saveSetupDataToEncounter() {
        if (viewmodel == null || viewmodel.getEncounterData() == null) {
            return;
        }

        final EncounterData encounterData = viewmodel.getEncounterData();

        // Save electrode distance 1
        final String strDistance1 = etDistance1.getText().toString();
        final OptionalDouble distance1 = !strDistance1.isEmpty() 
            ? OptionalDouble.of(Double.parseDouble(strDistance1))
            : OptionalDouble.empty();
        encounterData.setElectrodeDistance1Cm(distance1);

        // Save electrode distance 2
        final String strDistance2 = etDistance2.getText().toString();
        final OptionalDouble distance2 = !strDistance2.isEmpty() 
            ? OptionalDouble.of(Double.parseDouble(strDistance2))
            : OptionalDouble.empty();
        encounterData.setElectrodeDistance2Cm(distance2);

        // Save electrode distance 3
        final String strDistance3 = etDistance3.getText().toString();
        final OptionalDouble distance3 = !strDistance3.isEmpty() 
            ? OptionalDouble.of(Double.parseDouble(strDistance3))
            : OptionalDouble.empty();
        encounterData.setElectrodeDistance3Cm(distance3);

        // Save trimmed length (using existing trimLengthCm field)
        final String strTrimLength = etTrimmedLength.getText().toString();
        final OptionalDouble trimLength = !strTrimLength.isEmpty() 
            ? OptionalDouble.of(Double.parseDouble(strTrimLength))
            : OptionalDouble.empty();
        encounterData.setTrimLengthCm(trimLength);
        //log the newly set trim length
        Log.d("SetupFragment", "Set Trim Length Cm: " + trimLength);
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        
        // Note: We save data on Next button click, not on destroy
        // This prevents saving incomplete data when user navigates back
        
        // Clear binding reference to prevent memory leaks
        binding = null;
        etDistance1 = null;
        etDistance2 = null;
        etDistance3 = null;
        etTrimmedLength = null;
    }
}
