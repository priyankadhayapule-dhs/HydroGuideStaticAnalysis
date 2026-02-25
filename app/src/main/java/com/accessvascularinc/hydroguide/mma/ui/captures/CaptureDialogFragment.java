package com.accessvascularinc.hydroguide.mma.ui.captures;

import android.media.MediaScannerConnection;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.databinding.DataBindingUtil;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.lifecycle.ViewModelProvider;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.databinding.CaptureDialogLayoutBinding;
import com.accessvascularinc.hydroguide.mma.model.Capture;
import com.accessvascularinc.hydroguide.mma.model.DataFiles;
import com.accessvascularinc.hydroguide.mma.model.EncounterData;
import com.accessvascularinc.hydroguide.mma.model.UserPreferences;
import com.accessvascularinc.hydroguide.mma.ui.MainViewModel;
import com.accessvascularinc.hydroguide.mma.ui.input.BaseValueSelectDialogFragment;
import com.accessvascularinc.hydroguide.mma.ui.input.TrimLengthSelectionFragment;
import com.accessvascularinc.hydroguide.mma.ui.plot.PlotUtils;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;

import org.json.JSONException;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.Locale;
import java.util.OptionalDouble;

public class CaptureDialogFragment extends BaseValueSelectDialogFragment {
  private boolean isExternalTabActive = true;
  private boolean IsExternalCaptureDone= false;
  private Capture internalCapture;
  private Capture externalCapture;
  private boolean useExposedLength = false;
  private double selectedLength = 0;
  private double selectedLength_internal = 0;
  private Runnable onCaptureConfirmedCallback = null;

  private final View.OnClickListener confirmBtn_OnClickListener = view -> confirmSelection();


  // New: allow setting default tab (true=external, false=internal)
  public static CaptureDialogFragment newInstance(Capture internal, Capture external, Runnable onConfirmed, boolean defaultExternalTab) {
    CaptureDialogFragment fragment = new CaptureDialogFragment();
    fragment.internalCapture = internal;
    fragment.externalCapture = external;
    fragment.onCaptureConfirmedCallback = onConfirmed;
    Log.d("Capture", "newInstance: " + defaultExternalTab );
    fragment.IsExternalCaptureDone = defaultExternalTab;
    return fragment;
  }

  public CaptureDialogFragment() {
  }

  public void requestToggle() {
    // Find the binding from the current view
    CaptureDialogLayoutBinding binding = (CaptureDialogLayoutBinding) DataBindingUtil.getBinding(getView());
    if (binding == null) return;

    if (isExternalTabActive) {
      // Switch to internal tab
      binding.tabInternal.setBackgroundResource(R.drawable.tab_selected_bg);
      binding.tabExternal.setBackgroundResource(R.drawable.tab_unselected_bg);
      binding.internalContainer.setVisibility(View.VISIBLE);
      binding.externalContainer.setVisibility(View.GONE);
      isExternalTabActive = false;
      // Focus the internal number picker
      if (binding.catheterInsertionLengthNumberPickerInternal != null) {
        binding.catheterInsertionLengthNumberPickerInternal.requestFocus();
      }
    } else {
      // Switch to external tab
      binding.tabExternal.setBackgroundResource(R.drawable.tab_selected_bg);
      binding.tabInternal.setBackgroundResource(R.drawable.tab_unselected_bg);
      binding.internalContainer.setVisibility(View.GONE);
      binding.externalContainer.setVisibility(View.VISIBLE);
      isExternalTabActive = true;
      // Focus the external number picker
      if (binding.catheterInsertionLengthNumberPicker != null) {
        binding.catheterInsertionLengthNumberPicker.requestFocus();
      }
    }
  }

  @Override
  public void onStart() {
    super.onStart();

    if (getDialog() != null && getDialog().getWindow() != null) {
      int width = dpToPx(600);
      int height = dpToPx(340);

      getDialog().getWindow().setLayout(width, height);
    }
  }

  private int dpToPx(int dp) {
    final float density = requireContext().getResources().getDisplayMetrics().density;
    return Math.round(dp * density);
  }

  @Override
  public View onCreateView(@NonNull final LayoutInflater inflater,
                           @Nullable final ViewGroup container,
                           @Nullable final Bundle savedInstanceState) {
    final CaptureDialogLayoutBinding binding =
            DataBindingUtil.inflate(inflater, R.layout.capture_dialog_layout,
                    container, false);
    viewmodel = new ViewModelProvider(requireActivity()).get(MainViewModel.class);
    final EncounterData procedure = viewmodel.getEncounterData();
    Log.d("ABC", "6: ");
    binding.setEncounterData(procedure);

    // Draw both graphs
    if (externalCapture != null) {
      PlotUtils.drawCaptureGraph(binding.captureGraph, externalCapture);
      PlotUtils.stylePlot(binding.captureGraph);
    }
    if (internalCapture != null) {
      PlotUtils.drawCaptureGraph(binding.captureGraphInternal, internalCapture);
      PlotUtils.stylePlot(binding.captureGraphInternal);
    }

    binding.tabInternal.setOnClickListener(view -> {
      // Change tab backgrounds
      binding.tabInternal.setBackgroundResource(R.drawable.tab_selected_bg);
      binding.tabExternal.setBackgroundResource(R.drawable.tab_unselected_bg);
      // Toggle visibility
      binding.internalContainer.setVisibility(View.VISIBLE);
      binding.externalContainer.setVisibility(View.GONE);
      isExternalTabActive = false;
    });

    binding.tabExternal.setOnClickListener(view -> {
      // Change tab backgrounds
      binding.tabExternal.setBackgroundResource(R.drawable.tab_selected_bg);
      binding.tabInternal.setBackgroundResource(R.drawable.tab_unselected_bg);
      // Toggle visibility
      binding.internalContainer.setVisibility(View.GONE);
      binding.externalContainer.setVisibility(View.VISIBLE);
      isExternalTabActive = true;
    });

    // Prevent button navigation/focus on tabs
    binding.tabInternal.setFocusable(false);
    binding.tabInternal.setClickable(true);
    binding.tabExternal.setFocusable(false);
    binding.tabExternal.setClickable(true);

    // Show selection of inserted or exposed length in dialog based on user set
    // preference.
    // Make sure to adjust calculation of lengths as well so they are consistent.
    useExposedLength = viewmodel.getProfileState().getSelectedProfile().getUserPreferences()
            .getWaveformCaptureLabel() == UserPreferences.WaveformCaptureLabel.ExposedLength;

    final OptionalDouble curTrimLength = procedure.getTrimLengthCm();
    final OptionalDouble curCathInsLength = useExposedLength ? procedure.getExternalCatheterCm()
            : procedure.getInsertedCatheterCm();
    binding.catheterInsertionLengthTitle.setText(ContextCompat.getString(requireContext(),
            (useExposedLength ? R.string.capture_dialog_exposed_length_title
                    : R.string.capture_dialog_inserted_length_title)));
    binding.catheterInsertionLengthTitleInternal.setText(ContextCompat.getString(requireContext(),
            (useExposedLength ? R.string.capture_dialog_exposed_length_title
                    : R.string.capture_dialog_inserted_length_title)));

    // Use internalCapture for intravascular, externalCapture for external
    selectedLength_internal = (procedure.getIntravCaptures().length == 0)
            ? Math.min(EncounterData.DEFAULT_INSERT_LENGTH, curTrimLength.orElse(100))
            : curCathInsLength.getAsDouble();
    selectedLength =
            curCathInsLength.orElse(useExposedLength ? EncounterData.STANDARD_TRIM_LENGTH_CM : 0);

    final double maxLength = curTrimLength.orElse(EncounterData.MAX_TRIM_LENGTH_CM);
    // Removed duplicate declaration. See below for correct usage.
    // Use increment from externalCapture if available, else internalCapture
    final double increment = externalCapture.getIncrement();
    final double increment_internal = internalCapture.getIncrement();

    int numOfValues = (int) Math.round(maxLength / increment) + 1;
    int selectedValueIdx = 0;
    wheelValues = new String[numOfValues];
    for (int i = 0; i < wheelValues.length; i++) {
      final double valueToAdd = increment * i;
      wheelValues[i] = String.format(Locale.US, "%.1f", valueToAdd) + " cm";

      if (valueToAdd == selectedLength) {
        selectedValueIdx = i;
      }
    }

    numOfValues = (int) Math.round(maxLength / increment_internal) + 1;
    int selectedValueIdx_internal = 0;
    wheelValuesInternal = new String[numOfValues];
    for (int i = 0; i < wheelValuesInternal.length; i++) {
      final double valueToAdd = increment_internal * i;
      wheelValuesInternal[i] = String.format(Locale.US, "%.1f", valueToAdd) + " cm";

      if (valueToAdd == selectedLength_internal) {
        selectedValueIdx_internal = i;
      }
    }

    upArrowBtn = binding.catheterInsertionLengthNumberPickerUp;
    downArrowBtn = binding.catheterInsertionLengthNumberPickerDown;
    numberPicker = binding.catheterInsertionLengthNumberPicker;

    upArrowBtn_internal = binding.catheterInsertionLengthNumberPickerUpInternal;
    downArrowBtn_internal = binding.catheterInsertionLengthNumberPickerDownInternal;
    numberPicker_internal = binding.catheterInsertionLengthNumberPickerInternal;

    initNumberPicker(selectedValueIdx);
    initNumberPickerInternal(selectedValueIdx_internal);

    numberPicker.setOnValueChangedListener((numPicker, oldVal, newVal) -> {
      final String newLength = wheelValues[newVal].replace(" cm", "");
      selectedLength = Double.parseDouble(newLength);
    });

    numberPicker.setOnKeyListener((view, i, keyEvent) -> {
      if (keyEvent.getFlags() == KeyEvent.FLAG_LONG_PRESS) {
        MedDevLog.warn("Controller input", "LONG PRESS");
      }
      if ((i == KeyEvent.KEYCODE_ENTER) && (keyEvent.getAction() == KeyEvent.ACTION_DOWN)) {
        confirmSelection();
      }
      return false;
    });

    numberPicker_internal.setOnValueChangedListener((numPicker, oldVal, newVal) -> {
      final String newLength = wheelValuesInternal[newVal].replace(" cm", "");
      selectedLength_internal = Double.parseDouble(newLength);
    });

    numberPicker_internal.setOnKeyListener((view, i, keyEvent) -> {
      if (keyEvent.getFlags() == KeyEvent.FLAG_LONG_PRESS) {
        MedDevLog.warn("Controller input", "LONG PRESS");
      }
      if ((i == KeyEvent.KEYCODE_ENTER) && (keyEvent.getAction() == KeyEvent.ACTION_DOWN)) {
        confirmSelection();
      }
      return false;
    });

    binding.confirmBtn.setOnClickListener(confirmBtn_OnClickListener);
    binding.confirmBtnInternal.setOnClickListener(confirmBtn_OnClickListener);
    binding.cancelButton.setOnClickListener(view -> dismiss());
    binding.cancelButtonInternal.setOnClickListener(view -> dismiss());

    // If external capture was already done on first click, default to internal tab
    if (IsExternalCaptureDone) {
      binding.tabInternal.setBackgroundResource(R.drawable.tab_selected_bg);
      binding.tabExternal.setBackgroundResource(R.drawable.tab_unselected_bg);
      binding.internalContainer.setVisibility(View.VISIBLE);
      binding.externalContainer.setVisibility(View.GONE);
      isExternalTabActive = false;
      if (binding.catheterInsertionLengthNumberPickerInternal != null) {
        binding.catheterInsertionLengthNumberPickerInternal.requestFocus();
      }
    }

    return binding.getRoot();
  }

  private void updateViewModel() {
    final EncounterData procedure = viewmodel.getEncounterData();
    final double newCathInsertionLength =
            isExternalTabActive ? procedure.getTrimLengthCm().orElse(0) - selectedLength
                    : procedure.getTrimLengthCm().orElse(0) - selectedLength_internal;
    // Update both captures
    if (useExposedLength) {
      if (externalCapture != null) {
        externalCapture.setExposedLengthCm(
                isExternalTabActive ? selectedLength : selectedLength_internal);
        procedure.setExternalCatheterCm(
                OptionalDouble.of(isExternalTabActive ? selectedLength : selectedLength_internal));
        externalCapture.setInsertedLengthCm(newCathInsertionLength);
        procedure.setInsertedCatheterCm(OptionalDouble.of(newCathInsertionLength));
      }
      if (internalCapture != null) {
        internalCapture.setExposedLengthCm(
                isExternalTabActive ? selectedLength : selectedLength_internal);
        internalCapture.setInsertedLengthCm(newCathInsertionLength);
      }
    } else {
      if (externalCapture != null) {
        externalCapture.setInsertedLengthCm(
                isExternalTabActive ? selectedLength : selectedLength_internal);
        procedure.setInsertedCatheterCm(
                OptionalDouble.of(isExternalTabActive ? selectedLength : selectedLength_internal));
        externalCapture.setExposedLengthCm(newCathInsertionLength);
        procedure.setExternalCatheterCm(OptionalDouble.of(newCathInsertionLength));
      }
      if (internalCapture != null) {
        internalCapture.setInsertedLengthCm(
                isExternalTabActive ? selectedLength : selectedLength_internal);
        internalCapture.setExposedLengthCm(newCathInsertionLength);
      }
    }

    if (internalCapture != null && !isExternalTabActive) {
      final Capture[] oldIntraCaptures = procedure.getIntravCaptures();
      final Capture[] resizedData = Arrays.copyOf(oldIntraCaptures, oldIntraCaptures.length + 1);
      resizedData[resizedData.length - 1] = internalCapture;
      procedure.setIntravCaptures(resizedData);
      procedure.setLatestCapture(internalCapture);
    }
    if (externalCapture != null && isExternalTabActive) {
      procedure.setExternalCapture(externalCapture);
      procedure.setLatestCapture(externalCapture);
    }

    final File procedureDir = new File(procedure.getDataDirPath());
    final File procData = new File(procedureDir, DataFiles.PROCEDURE_DATA);

    try (final FileWriter writer = new FileWriter(procData, StandardCharsets.UTF_8)) {
      final String[] data = procedure.getProcedureDataText();

      for (final String str : data) {
        writer.write(str);
        writer.write(System.lineSeparator());
        writer.flush();
      }

      MediaScannerConnection.scanFile(requireContext(), new String[] {procData.getAbsolutePath()},
              new String[] {}, null);
    } catch (final IOException | JSONException e) {
      MedDevLog.error("CaptureDialogFragment", "Error writing procedure file", e);
      Toast.makeText(requireContext(), e.getMessage(), Toast.LENGTH_SHORT).show();
      throw new RuntimeException(e);
    }
  }

  private void confirmSelection() {
    MedDevLog.info("Controller Length",
            "New Len" + viewmodel.getEncounterData().getTrimLengthCm().toString());

    if (viewmodel.getEncounterData().getTrimLengthCm().isEmpty()) {
      final TrimLengthSelectionFragment trimSelectDialog = new TrimLengthSelectionFragment();
      trimSelectDialog.show(getChildFragmentManager(), "TEST");

      // Also dismiss this mode selection dialog on manual value selection.
      getChildFragmentManager().registerFragmentLifecycleCallbacks(
              new FragmentManager.FragmentLifecycleCallbacks() {
                @Override
                public void onFragmentViewDestroyed(@NonNull final FragmentManager fm,
                                                    @NonNull final Fragment f) {
                  super.onFragmentViewDestroyed(fm, f);
                  if (viewmodel.getEncounterData().getTrimLengthCm().isPresent()) {
                    updateViewModel();
                    runCallback();
                  }
                  dismiss();
                  getChildFragmentManager().unregisterFragmentLifecycleCallbacks(this);
                }
              }, false);
    } else {
      updateViewModel();
      runCallback();
      dismiss();
    }

  }

  private void runCallback() {
    if (onCaptureConfirmedCallback != null) {
      try {
        requireActivity().runOnUiThread(onCaptureConfirmedCallback);
      } catch (Exception e) {
        MedDevLog.error("CaptureDialogFragment", "Callback execution failed", e);
      }
    }
  }
}