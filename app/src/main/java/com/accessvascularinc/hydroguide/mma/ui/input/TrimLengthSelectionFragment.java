package com.accessvascularinc.hydroguide.mma.ui.input;

import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.databinding.DataBindingUtil;
import androidx.lifecycle.ViewModelProvider;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.databinding.TrimLengthSelectionLayoutBinding;
import com.accessvascularinc.hydroguide.mma.model.EncounterData;
import com.accessvascularinc.hydroguide.mma.model.UserPreferences;
import com.accessvascularinc.hydroguide.mma.model.UserProfile;
import com.accessvascularinc.hydroguide.mma.ui.MainViewModel;

import java.util.Locale;
import java.util.OptionalDouble;

public class TrimLengthSelectionFragment extends BaseValueSelectDialogFragment {
  private double selectedLength = 0;
  private final View.OnClickListener confirmBtn_OnClickListener = v -> confirmSelection();

  @Override
  public View onCreateView(@NonNull final LayoutInflater inflater,
                           @Nullable final ViewGroup container,
                           @Nullable final Bundle savedInstanceState) {
    final TrimLengthSelectionLayoutBinding binding =
            DataBindingUtil.inflate(inflater, R.layout.trim_length_selection_layout, container,
                    false);
    viewmodel = new ViewModelProvider(requireActivity()).get(MainViewModel.class);
    Log.d("ABC", "8: ");
    binding.setEncounterData(viewmodel.getEncounterData());

    selectedLength = EncounterData.STANDARD_TRIM_LENGTH_CM;
    final double increment = 0.5;

    final int numOfValues = (int) Math.round(
            (EncounterData.MAX_TRIM_LENGTH_CM - EncounterData.MIN_TRIM_LENGTH_CM) / increment) + 1;
    int selectedValueIdx = 0;
    wheelValues = new String[numOfValues];
    for (int i = 0; i < wheelValues.length; i++) {
      final double valueToAdd = EncounterData.MIN_TRIM_LENGTH_CM + (increment * i);
      wheelValues[i] = String.format(Locale.US, "%.1f", valueToAdd) + " cm";

      if (valueToAdd == selectedLength) {
        selectedValueIdx = i;
      }
    }

    upArrowBtn = binding.trimLengthNumberPickerUp;
    downArrowBtn = binding.trimLengthNumberPickerDown;
    numberPicker = binding.trimLengthNumberPicker;
    initNumberPicker(selectedValueIdx);

    numberPicker.setOnValueChangedListener((numPicker, oldVal, newVal) -> {
      final String newLength = wheelValues[newVal].replace(" cm", "");
      selectedLength = Double.parseDouble(newLength);
    });

    numberPicker.setOnKeyListener((view, i, keyEvent) -> {
      if ((i == KeyEvent.KEYCODE_ENTER) && (keyEvent.getAction() == KeyEvent.ACTION_DOWN)) {
        confirmSelection();
      }
      return false;
    });

    binding.confirmBtn.setOnClickListener(confirmBtn_OnClickListener);
    binding.cancelButton.setOnClickListener(view -> dismiss());

    return binding.getRoot();
  }

  private void confirmSelection() {
    viewmodel.getEncounterData().setTrimLengthCm(OptionalDouble.of(selectedLength));

    final UserProfile profile = viewmodel.getProfileState().getSelectedProfile();
    final boolean useExposedLength = profile.getUserPreferences().getWaveformCaptureLabel() ==
            UserPreferences.WaveformCaptureLabel.ExposedLength;
    if (useExposedLength) {
      viewmodel.getEncounterData().setInsertedCatheterCm(OptionalDouble.of(selectedLength));
    } else {
      viewmodel.getEncounterData().setExternalCatheterCm(OptionalDouble.of(selectedLength));
    }
    dismiss();
  }
}
