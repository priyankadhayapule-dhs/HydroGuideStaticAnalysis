package com.accessvascularinc.hydroguide.mma.ui.settings;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.RadioButton;
import android.widget.RadioGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.databinding.DataBindingUtil;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.lifecycle.ViewModelProvider;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.databinding.TimeScaleModeChangeLayoutBinding;
import com.accessvascularinc.hydroguide.mma.model.ChartConfig.AutoscaleAction;
import com.accessvascularinc.hydroguide.mma.ui.MainViewModel;

public class TimeScaleModeChangeFragment extends BaseSettingBottomSheetFragment {
  private TimeScaleModeChangeLayoutBinding binding = null;
  private RadioButton radio_manual_btn = null;

  @Override
  public View onCreateView(@NonNull final LayoutInflater inflater,
                           @Nullable final ViewGroup container,
                           @Nullable final Bundle savedInstanceState) {
    binding = DataBindingUtil.inflate(inflater, R.layout.time_scale_mode_change_layout,
            container, false);
    viewmodel = new ViewModelProvider(requireActivity()).get(MainViewModel.class);
    binding.setChartConfig(viewmodel.getChartConfig());

    radio_manual_btn = binding.radioButtonManual;
    radio_manual_btn.setOnClickListener(view -> showManualTimeScaleDialog());


    final RadioGroup timeScaleModeRadioGroup = binding.timeScaleModeRadioGroup;
    timeScaleModeRadioGroup.check(getRadioIdFromMode(binding.getChartConfig().getTimeScaleMode()));
    radio_manual_btn.requestFocus();

    timeScaleModeRadioGroup.setOnCheckedChangeListener((radioGroup, checkedId) -> {
      if (checkedId != -1) {
        binding.getChartConfig().setTimeScaleMode(getModeFromRadioId(checkedId));
        if (!radio_manual_btn.isChecked()) {
          dismiss();
        }
      }
    });

    binding.timeScaleModeChangeIcon.setOnClickListener(view -> dismiss());
    binding.confirmBtn.setOnClickListener(view -> dismiss());

    return binding.getRoot();
  }

  private void showManualTimeScaleDialog() {
    final TimeScaleValueChangeFragment bottomSheet = new TimeScaleValueChangeFragment();
    bottomSheet.show(getChildFragmentManager(), "TEST");

    // also dismiss this mode selection dialog on manual value selection
    getChildFragmentManager().registerFragmentLifecycleCallbacks(
            new FragmentManager.FragmentLifecycleCallbacks() {
              @Override
              public void onFragmentViewDestroyed(@NonNull final FragmentManager fm,
                                                  @NonNull final Fragment f) {
                super.onFragmentViewDestroyed(fm, f);
                dismiss();
                getChildFragmentManager().unregisterFragmentLifecycleCallbacks(this);
              }
            }, false);
  }

  private int getRadioIdFromMode(final AutoscaleAction action) {
    return switch (action) {
      case Manual -> R.id.radio_button_manual;
      case AutoOnce -> R.id.radio_button_auto_once;
      case AutoContinuous -> R.id.radio_button_auto_cont;
    };
  }

  private AutoscaleAction getModeFromRadioId(final int id) {
    if (id == R.id.radio_button_manual) {
      return AutoscaleAction.Manual;
    } else if (id == R.id.radio_button_auto_once) {
      return AutoscaleAction.AutoOnce;
    } else if (id == R.id.radio_button_auto_cont) {
      return AutoscaleAction.AutoContinuous;
    } else {
      return AutoscaleAction.AutoContinuous;
    }
  }
}
