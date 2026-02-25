package com.accessvascularinc.hydroguide.mma.ui;

import android.app.Dialog;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.media.MediaScannerConnection;
import android.os.Bundle;
import android.text.InputFilter;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.widget.AdapterView;
import android.widget.AutoCompleteTextView;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RadioGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.lifecycle.DefaultLifecycleObserver;
import androidx.lifecycle.LifecycleOwner;
import androidx.lifecycle.ViewModelProvider;
import androidx.navigation.Navigation;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.databinding.FragmentSummaryBinding;
import com.accessvascularinc.hydroguide.mma.db.HydroGuideDatabase;
import com.accessvascularinc.hydroguide.mma.db.repository.FacilityRepository;
import com.accessvascularinc.hydroguide.mma.db.repository.ProcedureCaptureRepository;
import com.accessvascularinc.hydroguide.mma.model.Capture;
import com.accessvascularinc.hydroguide.mma.model.DataFiles;
import com.accessvascularinc.hydroguide.mma.model.EncounterData;
import com.accessvascularinc.hydroguide.mma.model.Facility;
import com.accessvascularinc.hydroguide.mma.model.InputOptions;
import com.accessvascularinc.hydroguide.mma.model.PatientData;
import com.accessvascularinc.hydroguide.mma.model.ProfileState;
import com.accessvascularinc.hydroguide.mma.model.TabletState;
import com.accessvascularinc.hydroguide.mma.model.UserPreferences;
import com.accessvascularinc.hydroguide.mma.model.UserProfile;
import com.accessvascularinc.hydroguide.mma.serial.Button;
import com.accessvascularinc.hydroguide.mma.serial.ButtonState;
import com.accessvascularinc.hydroguide.mma.ui.captures.CapturesSelectListAdapter;
import com.accessvascularinc.hydroguide.mma.ui.captures.IntravCaptureListAdapter;
import com.accessvascularinc.hydroguide.mma.ui.input.ConfirmProcedureDialogFragment;
import com.accessvascularinc.hydroguide.mma.ui.input.EditFacilityDialogFragment;
import com.accessvascularinc.hydroguide.mma.ui.input.FacilitiesAutoCompleteAdapter;
import com.accessvascularinc.hydroguide.mma.ui.input.InputUtils;
import com.accessvascularinc.hydroguide.mma.ui.input.OnCheckedItemClickListener;
import com.accessvascularinc.hydroguide.mma.ui.settings.BrightnessChangeFragment;
import com.accessvascularinc.hydroguide.mma.ui.settings.VolumeChangeFragment;
import com.accessvascularinc.hydroguide.mma.ui.plot.PlotStyle;
import com.accessvascularinc.hydroguide.mma.ui.plot.PlotUtils;
import com.accessvascularinc.hydroguide.mma.utils.DbHelper;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import com.accessvascularinc.hydroguide.mma.utils.Utility;
import com.androidplot.xy.XYPlot;
import com.google.android.material.datepicker.CalendarConstraints;
import com.google.android.material.datepicker.DateValidatorPointBackward;
import com.google.android.material.datepicker.MaterialDatePicker;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import com.google.android.material.snackbar.Snackbar;
import com.google.android.material.textfield.TextInputEditText;
import com.google.android.material.textfield.TextInputLayout;

import org.json.JSONException;
import org.w3c.dom.Text;

import java.io.File;
import java.io.FileWriter;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.Objects;
import java.util.OptionalDouble;

public class SummaryFragment extends BaseHydroGuideFragment implements OnCheckedItemClickListener {
  private FragmentSummaryBinding binding = null;
  private EditText etTrimLength = null, etExtLength = null, etArmCircum = null, etVeinSize = null;
  private EditText etNoOfLumens = null, etCatheterSize = null;
  private ConfirmProcedureDialogFragment confirmationDialog = null;
  private RecyclerView capturesSelectListView = null, rptIntravCapturesListView = null;
  private CapturesSelectListAdapter capturesSelectListAdapter = null;
  private XYPlot reportExtPCapture = null;
  private ImageView ultrasoundImage = null;
  private final Dialog closeKbBtn = null;
  private IntravCaptureListAdapter rptIntravCapsListAdapter = null;
  private AutoCompleteTextView facilitiesDp = null;
  private FacilitiesAutoCompleteAdapter facilitySelectAdapter = null;
  private boolean manualFacilityInput = false;
  private BaseInputConnection inputConnection = null;

  private HydroGuideDatabase db;
  private MaterialDatePicker<Long> datePicker;

  private final View.OnClickListener navToProcedure_OnClickListener = v -> {
    saveInputsToVm();
    if (viewmodel.getEncounterData().isLive()) {
      updateProcedureFile();
    }
    Navigation.findNavController(requireView())
        .navigate(R.id.action_navigation_summary_to_procedure);
  };
  private final View.OnClickListener navToFinish_OnClickListener = v -> {

    // External Length Validation
    String extLength = etExtLength.getText().toString();
    if (!extLength.isEmpty()) {
      try {
        double externalLength = Double.parseDouble(extLength);
        if (externalLength < 0 || externalLength > 50) {
          etExtLength.setError("External Length should be in the range of 0 - 50cm");
          etExtLength.requestFocus();
          return;
        }
      } catch (NumberFormatException e) {
        etExtLength.setError("Invalid External Length value");
        etExtLength.requestFocus();
        return;
      }
    }

    // Trim Length Validation
    String trimLength = etTrimLength.getText().toString();
    if (!trimLength.isEmpty()) {
      try {
        double trimmedLength = Double.parseDouble(trimLength);
        if (trimmedLength < 0 || trimmedLength > 200) {
          etTrimLength.setError("Trim Length should be in the range of 0 - 200cm");
          etTrimLength.requestFocus();
          return;
        }
      } catch (NumberFormatException e) {
        etTrimLength.setError("Invalid Trim Length value");
        etTrimLength.requestFocus();
        return;
      }
    }

    // Arm Circumference Validation
    String armCircum = etArmCircum.getText().toString();
    if (!armCircum.isEmpty()) {
      try {
        double armCircumference = Double.parseDouble(armCircum);
        if (armCircumference < 10 || armCircumference > 150) {
          if (armCircumference < 10) {
            etArmCircum.setError("Arm Circumference cannot be less than 10cm");
          } else {
            etArmCircum.setError("Arm Circumference cannot exceed 150cm");
          }
          etArmCircum.requestFocus();
          return;
        }
      } catch (NumberFormatException e) {
        etArmCircum.setError("Invalid Arm Circumference value");
        etArmCircum.requestFocus();
        return;
      }
    }

    // Vein Size Validation
    String veinSize = etVeinSize.getText().toString();
    if (!veinSize.isEmpty()) {
      try {
        double veinSizeValue = Double.parseDouble(veinSize);
        if (veinSizeValue < 1 || veinSizeValue > 15) {
          if (veinSizeValue < 1) {
            etVeinSize.setError("Vein size cannot be less than 1mm");
          } else {
            etVeinSize.setError("Vein size cannot exceed 15mm");
          }
          etVeinSize.requestFocus();
          return;
        }
      } catch (NumberFormatException e) {
        etVeinSize.setError("Invalid Vein Size value");
        etVeinSize.requestFocus();
        return;
      }
    }

    // No. of Lumens Validation
    String noOfLumens = etNoOfLumens.getText().toString();
    if (!noOfLumens.isEmpty()) {
      try {
        double lumensValue = Double.parseDouble(noOfLumens);
        if (lumensValue < 1 || lumensValue > 5) {
          if (lumensValue < 1) {
            etNoOfLumens.setError("No. of Lumens cannot be less than 1");
          } else {
            etNoOfLumens.setError("No. of Lumens cannot exceed 5");
          }
          etNoOfLumens.requestFocus();
          return;
        }
      } catch (NumberFormatException e) {
        etNoOfLumens.setError("Invalid No. of Lumens value");
        etNoOfLumens.requestFocus();
        return;
      }
    }

    // Catheter Size Validation
    String catheterSize = etCatheterSize.getText().toString();
    if (!catheterSize.isEmpty()) {
      try {
        double catheterSizeValue = Double.parseDouble(catheterSize);
        if (catheterSizeValue < 1 || catheterSizeValue > 10) {
          if (catheterSizeValue < 1) {
            etCatheterSize.setError("Catheter Size cannot be less than 1");
          } else {
            etCatheterSize.setError("Catheter Size cannot exceed 10");
          }
          etCatheterSize.requestFocus();
          return;
        }
      } catch (NumberFormatException e) {
        etCatheterSize.setError("Invalid Catheter Size value");
        etCatheterSize.requestFocus();
        return;
      }
    }

    EncounterData currentProcedure = viewmodel.getEncounterData();
    // Enforce only first 20 checked internal captures are shown in report
    int internalCheckedCount = 0;
    if (capturesSelectListAdapter != null) {
      // Collect all checked internal captures
      java.util.List<Capture> checkedInternal = new java.util.ArrayList<>();
      for (int i = 0; i < capturesSelectListAdapter.getItemCount(); i++) {
        try {
          Capture cap = capturesSelectListAdapter.getCaptureFromIdx(i);
          if (cap != null && cap.getIsIntravascular() && cap.getShowInReport()) {
            checkedInternal.add(cap);
          }
        } catch (Exception e) {
          // Skip items that can't be retrieved (e.g., ultrasound items)
          continue;
        }
      }
      if (checkedInternal.size() > 20) {
        new MaterialAlertDialogBuilder(requireActivity())
            .setMessage("Internal capture cannot be selected greater than 20.")
            .setPositiveButton("OK", (dialog, id) -> dialog.dismiss())
            .show();
        return;
      }
    }
    if (!currentProcedure.isLive() && !currentProcedure.isConcluded()) {
      final MaterialAlertDialogBuilder builder = new MaterialAlertDialogBuilder(requireActivity());
      builder.setMessage("Please use the HydroGUIDE controller to begin a procedure");
      builder.setNegativeButton("Return", (dialog, id) -> {
      });
      builder.show();
      return;
    }
    saveInputsToVm();

    Runnable onDismissal = () -> {
      if (currentProcedure.isConcluded()) {
        updateNavigation();
        saveReport();
        viewmodel.setProcedureReport(currentProcedure);
        Navigation.findNavController(requireView()).navigate(R.id.nav_summary_to_report_view);
      }
    };

    if (!currentProcedure.isConcluded()) { // Removed: && viewmodel.getEncounterData().isLive()
      confirmationDialog = new ConfirmProcedureDialogFragment(onDismissal);
      confirmationDialog.show(getChildFragmentManager(), "TAG");
    } else {
      File dataDir = new File(currentProcedure.getDataDirPath());
      File repPdf = new File(dataDir, DataFiles.REPORT_PDF);
      if (repPdf.exists()) {
        repPdf.delete();
      }
      onDismissal.run();
    }
  };

  private final AdapterView.OnItemClickListener facilityDp_OnItemClickListener = (aV, v, i, l) -> {
    final PatientData patient = viewmodel.getEncounterData().getPatient();

    Facility facility = facilitySelectAdapter.getItem(i);
    patient.setPatientFacility(facility);
  };

  public View onCreateView(@NonNull final LayoutInflater inflater, final ViewGroup container,
      final Bundle savedInstanceState) {
    MedDevLog.info("Navigation", "Summary screen entered");
    binding = FragmentSummaryBinding.inflate(inflater, container, false);
    viewmodel = new ViewModelProvider(requireActivity()).get(MainViewModel.class);
    setupVolumeAndBrightnessListeners();
    requireActivity().getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

    final TabletState tablet = viewmodel.getTabletState();
    final double scale_factor = (1.0 / 1048576.0); // Byte -> MegaByte
    final int mb_available = (int) ((tablet.getFreeSpaceBytes()) * scale_factor);
    final int mb_init = (int) ((tablet.getTotalSpaceBytes()) * scale_factor);
    final TextView spaceRemaining = binding.freeByteValue;
    spaceRemaining.setText(mb_available + " MB / " + mb_init + " MB");

    final EncounterData procedure = viewmodel.getEncounterData();
    final UserProfile profile = viewmodel.getProfileState().getSelectedProfile();
    // Encounter insertion is now handled in DbHelper.insertDataInDb

    Log.d("ABC", "1: ");
    binding.setEncounterData(procedure);
    binding.setUserProfile(profile);
    binding.setChartConfig(viewmodel.getChartConfig());
    binding.setControllerState(viewmodel.getControllerState());
    binding.setTabletState(viewmodel.getTabletState());
    binding.summaryTopBar.setTabletState(viewmodel.getTabletState());

    tvBattery = binding.controllerBatteryLevelPct;
    ivBattery = binding.remoteBatteryLevelIndicator;
    ivUsbIcon = binding.usbIcon;
    ivTabletBattery = binding.tabletBatteryLevelIndicator;
    tvTabletBattery = binding.tabletBatteryLevelPct;

    ultrasoundImage = binding.ultrasoundCapture.ultrasoundCaptureImage;
    reportExtPCapture = binding.externalCapture.capturePlotGraph;
    rptIntravCapturesListView = binding.intravCapturesContainer.capturesList;

    binding.intravCapturesContainer.intravInstruction.setVisibility(View.GONE);
    rptIntravCapturesListView = binding.intravCapturesContainer.capturesList;
    rptIntravCapturesListView.setVisibility(View.VISIBLE);

    updateNavigation();

    capturesSelectListView = binding.capSelectList;

    final PatientData patientData = procedure.getPatient();
    binding.patientIdInput.setFilters(new InputFilter[] { Utility.allowOnlyAlphanumeric() });
    final EditText tilDob = binding.patientDobInput;
    tilDob.setText(patientData.getPatientDob());
    tilDob.setOnClickListener(view -> {
      final String patientDob = patientData.getPatientDob();
      CalendarConstraints.Builder constraintsBuilder = new CalendarConstraints.Builder();
      constraintsBuilder.setEnd(MaterialDatePicker.todayInUtcMilliseconds());
      constraintsBuilder.setValidator(DateValidatorPointBackward.now());

      final MaterialDatePicker<Long> datePicker = MaterialDatePicker.Builder.datePicker()
          .setTitleText("Select Patient's Date of Birth")
          .setSelection(!patientDob.isEmpty() ? InputUtils.parseDateToMillis(patientDob) : null)
          .setCalendarConstraints(constraintsBuilder.build())
          .setInputMode(MaterialDatePicker.INPUT_MODE_TEXT)
          .build();
      datePicker.addOnPositiveButtonClickListener(selection -> {
        final String newDate = InputUtils.getDateStringFromMillis(selection);
        patientData.setPatientDob(newDate);
      });
      datePicker.show(getChildFragmentManager(), "Date Picker");
      datePicker.getLifecycle().addObserver(new DefaultLifecycleObserver() {
        @Override
        public void onResume(@NonNull LifecycleOwner owner) {
          View view = datePicker.getView();
          if (view != null) {
            TextInputLayout dateInputLayout = (TextInputLayout) view.findViewById(com.google.android.material.R.id.mtrl_picker_text_input_date);
            TextInputEditText editText =(TextInputEditText) dateInputLayout.getEditText();
            if (editText != null) {
              editText.post(() -> editText.setSelection(editText.length()));
            }
          }
        }
      });
    });

    facilitiesDp = binding.facilityInput;
    if (profile != null) {
      final UserPreferences preferences = profile.getUserPreferences();
      patientData.setPatientInserter(profile.getProfileName());

      switch (preferences.getFacilitySelectionMode()) {
        case OnceManual -> {
          manualFacilityInput = false;
          final Facility manualFacility = preferences.getManualFacility();
          patientData.setPatientFacility(manualFacility);
          facilitiesDp.setText((manualFacility != null) ? manualFacility.getFacilityName() : "");
          facilitiesDp.setEnabled(false);
        }
        case Dropdown -> {
          manualFacilityInput = false;
          facilitiesDp.setText(patientData.getPatientFacility().getFacilityName());
          final Facility[] facilities = viewmodel.getProfileState().getFacilityList();
          final String[] faves = preferences.getFavoriteFacilitiesIds();
          facilitySelectAdapter = new FacilitiesAutoCompleteAdapter(requireContext(), R.layout.facility_list_item,
              R.id.facility_list_item_text, facilities, faves);
          facilitiesDp.setAdapter(facilitySelectAdapter);
          facilitiesDp.setOnItemClickListener(facilityDp_OnItemClickListener);
        }
        case ManualEachProcedure -> {
          manualFacilityInput = true;
          facilitiesDp.setText(patientData.getPatientFacility().getFacilityName());
          facilitiesDp.setInputType(EditorInfo.TYPE_CLASS_TEXT);
        }
      }
    }

    etTrimLength = binding.patientCathTrimInput;
    etTrimLength.setFilters(new InputFilter[] { new InputUtils.DecimalDigitsInputFilter(4, 1) });
    final OptionalDouble curTrimLength = procedure.getTrimLengthCm();
    etTrimLength.setText(
        curTrimLength.isPresent() ? Double.toString(curTrimLength.getAsDouble()) : "");

    etExtLength = binding.patientCathExtInput;
    etExtLength.setFilters(new InputFilter[] { new InputUtils.DecimalDigitsInputFilter(4, 1) });
    final OptionalDouble curExtLength = procedure.getExternalCatheterCm();
    etExtLength.setText(
        curExtLength.isPresent() ? Double.toString(curExtLength.getAsDouble()) : "");

    etArmCircum = binding.patientArmCircInput;
    etArmCircum.setFilters(new InputFilter[] { new InputUtils.DecimalDigitsInputFilter(4, 2) });
    final OptionalDouble curArmCircum = patientData.getPatientArmCircumference();
    etArmCircum.setText(
        curArmCircum.isPresent() ? Double.toString(curArmCircum.getAsDouble()) : "");

    etVeinSize = binding.patientVeinSizeInput;
    etVeinSize.setFilters(new InputFilter[] { new InputUtils.DecimalDigitsInputFilter(4, 2) });
    final OptionalDouble curVeinSize = patientData.getPatientVeinSize();
    etVeinSize.setText(curVeinSize.isPresent() ? Double.toString(curVeinSize.getAsDouble()) : "");

    etNoOfLumens = binding.patientNoOfLumensInput;
    etNoOfLumens.setText(
        patientData.getPatientNoOfLumens() != null ? patientData.getPatientNoOfLumens() : "");

    etCatheterSize = binding.patientCatheterSizeInput;
    etCatheterSize.setText(
        patientData.getPatientCatheterSize() != null ? patientData.getPatientCatheterSize() : "");

    final AutoCompleteTextView insertSideDp = binding.patientInsertSideInput;
    insertSideDp.setAdapter(
        new InputUtils.NonFilterArrayAdapter<>(requireContext(), R.layout.dropdown_list_item,
            InputOptions.SIDE_OPTIONS));

    final AutoCompleteTextView insertVeinDp = binding.patientInsertVeinInput;
    insertVeinDp.setAdapter(
        new InputUtils.NonFilterArrayAdapter<>(requireContext(), R.layout.dropdown_list_item,
            InputOptions.VEIN_OPTIONS));

    final RadioGroup rgTipLocConfirm = binding.tipLocationConfirmRadioGroup;

    final Boolean isHgConfirmed = procedure.getHydroGuideConfirmed();
    if (isHgConfirmed != null) {
      rgTipLocConfirm.check(
          isHgConfirmed ? R.id.tip_location_confirm_yes : R.id.tip_location_confirm_no);
    }

    initCaptures();
    PlotUtils.stylePlot(reportExtPCapture);

    rgTipLocConfirm.setOnCheckedChangeListener((radioGroup, checkedId) -> {
      if (checkedId != -1) {
        if (radioGroup.getCheckedRadioButtonId() == R.id.tip_location_confirm_yes) {
          binding.getEncounterData().setHydroGuideConfirmed(true);
        } else if (radioGroup.getCheckedRadioButtonId() == R.id.tip_location_confirm_no) {
          binding.getEncounterData().setHydroGuideConfirmed(false);
        } else {
          binding.getEncounterData().setHydroGuideConfirmed(null);
        }
      }
    });

    final File directory = requireContext().getFilesDir();
    final String[] files = requireContext().fileList();
    MedDevLog.warn("Location: ", directory.toString());
    MedDevLog.warn("Files List:", Arrays.toString(files));

    binding.navToProcedureScreen.setOnClickListener(navToProcedure_OnClickListener);
    binding.navToFinish.setOnClickListener(navToFinish_OnClickListener);

    // Setup volume, brightness, and mute controls
    final View root = binding.getRoot();
    ImageButton volumeBtn = root.findViewById(R.id.volume_btn);
    ImageButton brightnessBtn = root.findViewById(R.id.brightness_btn);
    ImageButton muteBtn = root.findViewById(R.id.mute_btn);

    if (volumeBtn != null) {
      volumeBtn.setOnClickListener(v -> {
        VolumeChangeFragment bottomSheet = new VolumeChangeFragment();
        bottomSheet.show(getChildFragmentManager(), "VolumeChange");
      });
    }
    if (brightnessBtn != null) {
      brightnessBtn.setOnClickListener(v -> {
        BrightnessChangeFragment bottomSheet = new BrightnessChangeFragment();
        bottomSheet.show(getChildFragmentManager(), "BrightnessChange");
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

  @Override
  public void onViewCreated(@NonNull final View view, @Nullable final Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);
    final View rootView = requireActivity().getWindow().getDecorView().getRootView();
    inputConnection = new BaseInputConnection(rootView, true);
  }

  private void initCaptures() {
    // TODO: Show either inserted or exposed length in capture window based on user
    // set preference.
    final EncounterData procedure = viewmodel.getEncounterData();
    Capture[] allCaptures = new Capture[0];

    final Capture extCapData = procedure.getExternalCapture();
    final UserProfile profile = viewmodel.getProfileState().getSelectedProfile();
    final boolean useExposedLength = profile.getUserPreferences()
        .getWaveformCaptureLabel() == UserPreferences.WaveformCaptureLabel.ExposedLength;
    PlotUtils.setupExternalCapture(binding.externalCapture, extCapData, useExposedLength);

    if (extCapData != null) {
      allCaptures = new Capture[] { extCapData };
      PlotUtils.drawCaptureGraph(reportExtPCapture, extCapData);
    }

    final Capture[] intravCaps = procedure.getIntravCaptures();
    for (final Capture intravCap : intravCaps) {
      intravCap.setFormatter(PlotUtils.getFormatter(requireContext(), PlotStyle.Intravascular, 0));
    }

    if (intravCaps.length > 0) {
      final Capture[] resizedCaps = Arrays.copyOf(allCaptures, allCaptures.length + intravCaps.length);
      System.arraycopy(intravCaps, 0, resizedCaps, allCaptures.length, intravCaps.length);
      allCaptures = resizedCaps;
    }

    // Get ultrasound captures
    String[] ultrasoundPaths = procedure.getUltrasoundCapturePaths();

    // Hide ultrasound capture control if no captures exist
    if (ultrasoundPaths == null || ultrasoundPaths.length == 0) {
      binding.ultrasoundCapture.getRoot().setVisibility(View.GONE);
    } else {
      binding.ultrasoundCapture.getRoot().setVisibility(View.VISIBLE);
    }

    // TODO: Make sure labels in list reflect exposed or inserted length based on
    // profile setting.
    capturesSelectListView.setLayoutManager(
        new LinearLayoutManager(requireContext(), LinearLayoutManager.VERTICAL, false));

    // Use adapter with ultrasound support
    capturesSelectListAdapter = new CapturesSelectListAdapter(
        ultrasoundPaths, allCaptures, useExposedLength, this);
    capturesSelectListView.setAdapter(capturesSelectListAdapter);

    // Display ultrasound images in report area (all selected by default initially)
    if (ultrasoundPaths != null && ultrasoundPaths.length > 0) {
      displayUltrasoundImages(ultrasoundPaths);
    }

    updateReportExtCapVisibility(extCapData);

    rptIntravCapturesListView.setLayoutManager(
        new LinearLayoutManager(requireContext(), LinearLayoutManager.HORIZONTAL, false));
    rptIntravCapsListAdapter = new IntravCaptureListAdapter();

    final Capture[] capsToShow = Arrays.stream(intravCaps).filter(Capture::getShowInReport).toArray(Capture[]::new);

    if (capsToShow.length > 0) {
      rptIntravCapsListAdapter = new IntravCaptureListAdapter(capsToShow);
      rptIntravCapturesListView.smoothScrollToPosition(rptIntravCapsListAdapter.getItemCount() - 1);
    }
    rptIntravCapturesListView.setAdapter(rptIntravCapsListAdapter);
  }

  @Override
  public void onCheckedItemClickListener(final int position, final boolean isChecked) {
    // Position -1 indicates ultrasound selection changed
    if (position == -1) {
      updateUltrasoundDisplay();
      return;
    }

    final Capture toggledCapture = capturesSelectListAdapter.getCaptureFromIdx(position);
    if (toggledCapture == null) {
      // Ultrasound item - already handled by position == -1 check
      return;
    }
    toggledCapture.setShownInReport(isChecked);

    final EncounterData procedure = viewmodel.getEncounterData();

    if (toggledCapture.getIsIntravascular()) {
      procedure.getIntravCaptures()[toggledCapture.getCaptureId() - 1].setShownInReport(isChecked);
      if (isChecked) {
        rptIntravCapsListAdapter.insertCaptureByNum(toggledCapture, toggledCapture.getCaptureId());
      } else {
        rptIntravCapsListAdapter.removeCaptureByNum(toggledCapture.getCaptureId());
      }
    } else {
      procedure.getExternalCapture().setShownInReport(isChecked);
      updateReportExtCapVisibility(procedure.getExternalCapture());
    }
  }

  /**
   * Update ultrasound display based on current selection state
   */
  private void updateUltrasoundDisplay() {
    if (capturesSelectListAdapter == null) {
      return;
    }

    String[] selectedPaths = capturesSelectListAdapter.getSelectedUltrasoundPaths();

    // Hide ultrasound capture view if no ultrasounds are selected
    if (selectedPaths == null || selectedPaths.length == 0) {
      binding.ultrasoundCapture.getRoot().setVisibility(View.GONE);
    } else {
      binding.ultrasoundCapture.getRoot().setVisibility(View.VISIBLE);
      displayUltrasoundImages(selectedPaths);
    }
  }

  private void updateReportExtCapVisibility(final Capture extCapture) {
    // Draw graph if external capture exists and is selected to be shown
    final boolean showExtCap = (extCapture != null) && (extCapture.getShowInReport());
    binding.externalCapture.captureLayout.setVisibility(showExtCap ? View.VISIBLE : View.GONE);
  }

  /**
   * Display ultrasound images in the report captures area
   */
  private void displayUltrasoundImages(String[] selectedPaths) {
    if (selectedPaths == null || selectedPaths.length == 0) {
      return;
    }

    LinearLayout reportCapturesContainer = binding.reportCaptures;

    // Add ultrasound images at the beginning (before ECG captures)
    for (int i = 0; i < selectedPaths.length; i++) {
      String imagePath = selectedPaths[i];

      // Load image from file
      Bitmap bitmap = BitmapFactory.decodeFile(imagePath);
      if (bitmap != null) {
        // Crop any transparent/blank space from bottom of image
        Bitmap croppedBitmap = cropBitmapTransparentBottom(bitmap);
        ultrasoundImage.setImageBitmap(croppedBitmap);
        if (croppedBitmap != bitmap) {
          bitmap.recycle(); // Free original bitmap if we created a new one
        }
      }
    }
  }

  /**
   * Crops transparent or blank space from the bottom of a bitmap
   */
  private Bitmap cropBitmapTransparentBottom(Bitmap source) {
    int width = source.getWidth();
    int height = source.getHeight();

    // Find the last row with non-transparent/non-black pixels
    int lastNonEmptyRow = height - 1;
    for (int y = height - 1; y >= 0; y--) {
      boolean rowHasContent = false;
      for (int x = 0; x < width; x++) {
        int pixel = source.getPixel(x, y);
        int alpha = (pixel >> 24) & 0xFF;
        int red = (pixel >> 16) & 0xFF;
        int green = (pixel >> 8) & 0xFF;
        int blue = pixel & 0xFF;

        // Check if pixel is not transparent and not black
        if (alpha > 10 && (red > 10 || green > 10 || blue > 10)) {
          rowHasContent = true;
          break;
        }
      }

      if (rowHasContent) {
        lastNonEmptyRow = y;
        break;
      }
    }

    // If we found blank space at bottom, crop it
    if (lastNonEmptyRow < height - 1) {
      int newHeight = lastNonEmptyRow + 1;
      return Bitmap.createBitmap(source, 0, 0, width, newHeight);
    }

    return source; // No cropping needed
  }

  private void updateNavigation() {
    final boolean isConcluded = viewmodel.getEncounterData().isConcluded();
    binding.navToProcedureScreenContainer.setVisibility(isConcluded ? View.GONE : View.VISIBLE);
    binding.concludedProcedureIndicator.setVisibility(isConcluded ? View.VISIBLE : View.GONE);
  }

  @Override
  public void onButtonsStateChange(final Button newButton, final String buttonLogMsg) {
    super.onButtonsStateChange(newButton, buttonLogMsg);

    for (int idx = 0; idx < Button.ButtonIndex.NumButtons.getIntValue(); idx++) {
      final Button.ButtonIndex buttonIdx = Button.ButtonIndex.valueOfInt(idx);
      final ButtonState buttonState = newButton.buttons.getButtonStateAtKey(
          Objects.requireNonNull(buttonIdx));

      if (buttonState == ButtonState.ButtonIdle) {
        continue;
      } else {
        InputUtils.simulateKeyInput(buttonIdx, buttonState, inputConnection);
      }
    }
  }

  private void saveReport() {
    // Save the selected ultrasound paths for PDF generation
    // but don't permanently modify the EncounterData paths
    if (capturesSelectListAdapter != null) {
      String[] selectedUltrasoundPaths = capturesSelectListAdapter.getSelectedUltrasoundPaths();
      // Store selected paths temporarily for PDF generation
      viewmodel.getEncounterData().setSelectedUltrasoundPathsForReport(selectedUltrasoundPaths);
    }

    // Update file with latest procedure data
    final String procedurePath = viewmodel.getEncounterData().getDataDirPath() + File.separator +
        DataFiles.PROCEDURE_DATA;
    final File procedureFile = new File(procedurePath);
    Log.d("ABC", "procedurePath: " + procedurePath);
    String alertMsg;
    if (procedureFile.exists()) {
      try (final FileWriter writer = new FileWriter(procedureFile, StandardCharsets.UTF_8)) {
        final String[] data = viewmodel.getEncounterData().getProcedureDataText();
        final HydroGuideDatabase db = HydroGuideDatabase.buildDatabase(requireContext());
        final FacilityRepository facilityRepository = new FacilityRepository(db.facilityDao());
        final ProcedureCaptureRepository procedureCaptureRepository = new ProcedureCaptureRepository(
            db.procedureCaptureDao());

        DbHelper.insertDataInDb(
            requireContext(),
            data,
            viewmodel.getEncounterData().getPatient(),
            facilityRepository,
            viewmodel.getEncounterData().isLive(),
            procedureCaptureRepository,
            viewmodel.getEncounterData().getDataDirPath());
        // Invoke Sync Method
        // triggerSync();
        for (int i = 0; i < data.length; i++) {
          // for (final String str : data) {
          writer.write(data[i]);
          Log.d("ABC", "data[" + i + "]" + data[i]);
          Log.d("ABC", "lineSeparator: " + System.lineSeparator());
          writer.write(System.lineSeparator());
          writer.flush();
        }
        alertMsg = DataFiles.PROCEDURE_DATA + " was updated successfully";
        MedDevLog.audit("Report", "Report data modified for encounter: " + viewmodel.getEncounterData().getStartTimeText());
        MediaScannerConnection.scanFile(requireContext(),
            new String[] { procedureFile.getAbsolutePath() }, new String[] {}, null);
      } catch (final Exception e) {
        MedDevLog.error("SummaryFragment", "Error saving procedure data", e);
        alertMsg = e.getMessage();
      }
    } else {
      alertMsg = "Error: Procedure file is missing.";
    }

    Snackbar.make(requireView(), Objects.requireNonNull(alertMsg), Snackbar.LENGTH_LONG)
        .setAction("OK", v -> {
        }).show();
  }

  private void saveInputsToVm() {
    final EncounterData procedure = viewmodel.getEncounterData();
    if (manualFacilityInput) {
      final String manualFacilityName = facilitiesDp.getText().toString();
      procedure.getPatient().getPatientFacility().setFacilityName(manualFacilityName);
    }

    final Facility procFacility = procedure.getPatient().getPatientFacility();
    procFacility.setDateLastUsed(procedure.getStartTimeText());
    viewmodel.getProfileState().updateFacilityRecency(procFacility, requireContext());

    final String strTrimLength = etTrimLength.getText().toString();
    final OptionalDouble newTrimLength = !strTrimLength.isEmpty() ? OptionalDouble.of(Double.parseDouble(strTrimLength))
        : OptionalDouble.empty();
    if (newTrimLength.isPresent()) {
      procedure.setTrimLengthCm(newTrimLength);
    }

    final String txtExtLength = etExtLength.getText().toString();
    final OptionalDouble newExtLength = !txtExtLength.isEmpty() ? OptionalDouble.of(Double.parseDouble(txtExtLength))
        : OptionalDouble.empty();
    if (newExtLength.isPresent()) {
      procedure.setExternalCatheterCm(newExtLength);
    }

    final String txtArmCirc = etArmCircum.getText().toString();
    final OptionalDouble newArmCirc = !txtArmCirc.isEmpty() ? OptionalDouble.of(Double.parseDouble(txtArmCirc))
        : OptionalDouble.empty();
    if (newArmCirc.isPresent()) {
      procedure.getPatient().setPatientArmCircumference(newArmCirc);
    }

    final String txtVeinSize = etVeinSize.getText().toString();
    final OptionalDouble currVeinSize = !txtVeinSize.isEmpty() ? OptionalDouble.of(Double.parseDouble(txtVeinSize))
        : OptionalDouble.empty();
    if (currVeinSize.isPresent()) {
      procedure.getPatient().setPatientVeinSize(currVeinSize);
    }

    final String strNoOfLumens = etNoOfLumens.getText().toString();
    procedure.getPatient().setPatientNoOfLumens(strNoOfLumens.isEmpty() ? "" : strNoOfLumens);
    Log.d("SummaryFragment_Data", "Set No. of Lumens: '" + strNoOfLumens + "' -> Patient value: '"
        + procedure.getPatient().getPatientNoOfLumens() + "'");

    final String strCatheterSize = etCatheterSize.getText().toString();
    procedure.getPatient().setPatientCatheterSize(strCatheterSize.isEmpty() ? "" : strCatheterSize);
    Log.d("SummaryFragment_Data", "Set Catheter Size: '" + strCatheterSize + "' -> Patient value: '"
        + procedure.getPatient().getPatientCatheterSize() + "'");

    Log.d("SummaryFragment_Data", "Final PatientData - NoOfLumens: '" + procedure.getPatient().getPatientNoOfLumens() +
        "', CatheterSize: '" + procedure.getPatient().getPatientCatheterSize() + "'");
  }

  @Override
  public void onDestroyView() {
    super.onDestroyView();
    binding = null;
    if(datePicker != null && datePicker.getDialog() != null && datePicker.getDialog().isShowing())
    {
      datePicker.dismiss();
    }
  }
}