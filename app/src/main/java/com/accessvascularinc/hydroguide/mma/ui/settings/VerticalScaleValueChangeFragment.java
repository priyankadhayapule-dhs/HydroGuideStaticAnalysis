package com.accessvascularinc.hydroguide.mma.ui.settings;

import android.os.Bundle;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.databinding.DataBindingUtil;
import androidx.lifecycle.ViewModelProvider;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.databinding.VerticalScaleValueChangeLayoutBinding;
import com.accessvascularinc.hydroguide.mma.ui.MainViewModel;

import java.math.BigDecimal;
import java.math.RoundingMode;

public class VerticalScaleValueChangeFragment extends BaseSettingBottomSheetFragment {
  private VerticalScaleValueChangeLayoutBinding binding = null;

  @Override
  public View onCreateView(@NonNull final LayoutInflater inflater,
                           @Nullable final ViewGroup container,
                           @Nullable final Bundle savedInstanceState) {
    binding = DataBindingUtil.inflate(inflater, R.layout.vertical_scale_value_change_layout,
            container, false);
    viewmodel = new ViewModelProvider(requireActivity()).get(MainViewModel.class);

    binding.setChartConfig(viewmodel.getChartConfig());

    binding.verticalScaleSlider.addOnChangeListener((slider, value, fromUser) -> {
      final float newVal =
              new BigDecimal(Float.toString(value)).setScale(1, RoundingMode.HALF_UP).floatValue();
      binding.getChartConfig().setVerticalScale(newVal);
    });

    binding.verticalScaleSlider.setOnKeyListener((view, i, keyEvent) -> {
      if (i == KeyEvent.KEYCODE_ENTER) {
        dismiss();
      }

      return false;
    });

    return binding.getRoot();
  }
}