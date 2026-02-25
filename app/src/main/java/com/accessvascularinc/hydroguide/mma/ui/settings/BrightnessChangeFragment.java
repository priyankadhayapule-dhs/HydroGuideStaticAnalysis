package com.accessvascularinc.hydroguide.mma.ui.settings;

import android.os.Bundle;
import android.provider.Settings;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Toast;
import android.net.Uri;
import android.content.Intent;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.databinding.DataBindingUtil;
import androidx.lifecycle.ViewModelProvider;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.databinding.BrightnessChangeLayoutBinding;
import com.accessvascularinc.hydroguide.mma.ui.MainViewModel;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;

public class BrightnessChangeFragment extends BaseSettingBottomSheetFragment {

  @Override
  public View onCreateView(@NonNull final LayoutInflater inflater,
                           @Nullable final ViewGroup container,
                           @Nullable final Bundle savedInstanceState) {
    final BrightnessChangeLayoutBinding binding =
            DataBindingUtil.inflate(inflater, R.layout.brightness_change_layout, container, false);
    viewmodel = new ViewModelProvider(requireActivity()).get(MainViewModel.class);
    binding.setTabletState(viewmodel.getTabletState());

    // Check if app has WRITE_SETTINGS permission
    if (!Settings.System.canWrite(requireContext())) {
      MedDevLog.warn("Brightness", "WRITE_SETTINGS permission not granted");
      Toast.makeText(requireContext(), "Allow system settings to change brightness", Toast.LENGTH_SHORT).show();
      Intent intent = new Intent(Settings.ACTION_MANAGE_WRITE_SETTINGS);
      intent.setData(Uri.parse("package:" + requireContext().getPackageName()));
      startActivity(intent);
      dismiss();
      return binding.getRoot();
    }

    // Set to manual brightness mode
    try {
      Settings.System.putInt(requireContext().getContentResolver(),
              Settings.System.SCREEN_BRIGHTNESS_MODE, Settings.System.SCREEN_BRIGHTNESS_MODE_MANUAL);
      Log.d("Brightness", "Set to manual brightness mode");
    } catch (Exception e) {
      MedDevLog.error("Brightness", "Error setting brightness mode", e);
    }

    // Initialize slider with current brightness
    try {
      final int currentBrightness = Settings.System.getInt(requireContext().getContentResolver(),
              Settings.System.SCREEN_BRIGHTNESS, 128);
      final float brightnessPercent = BrightnessLookup.getBrightnessPercentage(currentBrightness);
      binding.brightnessSlider.setValue(brightnessPercent * 100);
      Log.d("Brightness", "Initialized slider to " + brightnessPercent * 100 + "% (absolute: " + currentBrightness + ")");
    } catch (Exception e) {
      MedDevLog.error("Brightness", "Error initializing brightness slider", e);
    }

    binding.brightnessChangeIcon.setOnClickListener(view -> dismiss());
    
    // Smooth slider updates
    binding.brightnessSlider.addOnChangeListener((slider, value, fromUser) -> {
      if (!fromUser) return; // Only respond to user input, not programmatic changes
      
      try {
        // Convert percentage (0-100 from slider) to brightness value (0-255)
        // value is 0-100, need to convert to 0-1 for getBrightnessValue()
        final int newBrightness = BrightnessLookup.getBrightnessValue(value / 100f);
        Settings.System.putInt(requireContext().getContentResolver(),
                Settings.System.SCREEN_BRIGHTNESS, newBrightness);
        
        // Update TabletState immediately for smooth UI feedback
        viewmodel.getTabletState().setScreenBrightness((int) value);

        Log.d("Brightness", "Slider changed to " + value + "% (absolute: " + newBrightness + ")");
      } catch (Exception e) {
        MedDevLog.error("Brightness", "Error changing brightness", e);
        Toast.makeText(requireContext(), "Error changing brightness", Toast.LENGTH_SHORT).show();
      }
    });

    binding.brightnessSlider.setOnKeyListener((view, i, keyEvent) -> {
      if (i == KeyEvent.KEYCODE_ENTER) {
        dismiss();
      }
      return false;
    });

    return binding.getRoot();
  }
}