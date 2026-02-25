package com.accessvascularinc.hydroguide.mma.ui;


import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Color;
import android.graphics.drawable.GradientDrawable;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.databinding.DataBindingUtil;
import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.databinding.FragmentBluetoothPairingPatternBinding;
import com.accessvascularinc.hydroguide.mma.utils.Utility;
import androidx.lifecycle.ViewModelProvider;

import java.util.ArrayList;
import java.util.List;
import android.content.Context;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.util.Log;

import java.util.HashMap;

public class BluetoothPairingPatternFragment extends BaseHydroGuideFragment {

  List<ImageView> arrowImageViews = null;
  int buttonIndexCount = 0;
  String[] directions;
  String deviceName = null;
  private long lastEventTimestamp = 0;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState) {
    FragmentBluetoothPairingPatternBinding binding = DataBindingUtil.inflate(inflater,
            R.layout.fragment_bluetooth_pairing_pattern, container, false);

    // Set up the 10 arrow images at runtime based on the arrowPattern argument
    if (getArguments() != null &&
            getArguments().containsKey(getString(R.string.key_bonded_device))) {
      deviceName = getArguments().getString(getString(R.string.key_bonded_device));
    }
    if (getArguments() != null && getArguments().containsKey(getString(R.string.key_arrowPattern))) {
      String arrowPattern = getArguments().getString(getString(R.string.key_arrowPattern));
      if (arrowPattern != null) {
        // The pattern is a comma-separated string, e.g. "UP,LEFT,DOWN,..."
        directions = arrowPattern.split(",");
        android.view.ViewGroup patternGrid = binding.getRoot().findViewById(R.id.pattern_grid);
        arrowImageViews = new ArrayList<>();
        int childCount = patternGrid.getChildCount();
        for (int i = 0; i < Math.min(directions.length, childCount); i++) {
          String dir = directions[i].trim();
          android.view.View arrowView = patternGrid.getChildAt(i);
          if (arrowView instanceof android.widget.LinearLayout) {
            ImageView img =
                    (android.widget.ImageView) ((android.widget.LinearLayout) arrowView).getChildAt(
                            0);
            img.setTag("arrow_" + i);
            arrowImageViews.add(img);
            if (dir.equals("UP")) {
              img.setImageResource(R.drawable.arrow_up);
            } else if (dir.equals("DOWN")) {
              img.setImageResource(R.drawable.arrow_down);
            } else if (dir.equals("LEFT")) {
              img.setImageResource(R.drawable.arrow_left);
            } else if (dir.equals("RIGHT")) {
              img.setImageResource(R.drawable.arrow_right);
            } else {
              // fallback: hide or set a question mark if needed
              img.setImageResource(android.R.color.transparent);
            }
          }
        }
      }
    }

    String title = getString(R.string.connect_controller_title);
    if (getArguments() != null && getArguments().containsKey("title")) {
      String argTitle = getArguments().getString("title");
      if (argTitle != null && !argTitle.isEmpty()) {
        title = argTitle;
      }
    }
    binding.setTitle(title);

    viewmodel = new ViewModelProvider(requireActivity()).get(MainViewModel.class);
   // ControllerState controllerState = viewmodel.getControllerState();
    //binding.topBar.setControllerState(controllerState);

    viewmodel.getButtonIndex().observe(getViewLifecycleOwner(),buttonIndex ->
    {
      // Ignore stale events - only process fresh button presses
      // Use timestamp to detect new events (allows same button pressed twice)
      long currentTime = System.currentTimeMillis();
      if (buttonIndex != null && (currentTime - lastEventTimestamp) > 50 && buttonIndexCount <= arrowImageViews.size()) {
        lastEventTimestamp = currentTime;
        Log.d("highlightArrow", "Button: " + buttonIndex);
        String pressedDirection = null;
        boolean captureClick = false;
        switch (buttonIndex) {
          case Up:
            pressedDirection = "UP";
            break;
          case Down:
            pressedDirection = "DOWN";
            break;
          case Left:
            pressedDirection = "LEFT";
            break;
          case Right:
            pressedDirection = "RIGHT";
            break;
          case Capture:
            captureClick = true;
            break;
          case Select:
            break;
        }

        if (pressedDirection != null) {
          // Check if pressed button matches expected button in sequence
          if (buttonIndexCount < directions.length) {
            String expectedDirection = directions[buttonIndexCount].trim();
            boolean isCorrect = pressedDirection.equals(expectedDirection);
            highlightArrow(isCorrect);
          }
          if (buttonIndexCount >= directions.length) {

          }
        }
        if (captureClick) {
          resetArrowButton();
        }
      }
    });
    binding.topBar.setTabletState(viewmodel.getTabletState());
    tvBattery = binding.topBar.controllerBatteryLevelPct;
    ivBattery = binding.topBar.remoteBatteryLevelIndicator;
    ivUsbIcon = binding.topBar.usbIcon;
    ivTabletBattery = binding.topBar.tabletBatteryIndicator.tabletBatteryLevelIndicator;
    tvTabletBattery = binding.topBar.tabletBatteryIndicator.tabletBatteryLevelPct;
    hgLogo = binding.topBar.appLogo;
    lpTabletIcon = binding.topBar.tabletBatteryWarningIcon.batteryIcon;
    lpTabletSymbol = binding.topBar.tabletBatteryWarningIcon.batteryWarningImg;

    binding.topBar.remoteBatteryLevelIndicator.setVisibility(View.GONE);
    binding.topBar.controllerBatteryLabel.setVisibility(View.GONE);
    binding.topBar.controllerBatteryLevelPct.setVisibility(View.GONE);
    binding.topBar.usbIcon.setVisibility(View.GONE);

    View backBtn = binding.getRoot().findViewById(R.id.back_btn);
    backBtn.setOnClickListener(v -> androidx.navigation.Navigation.findNavController(binding.getRoot())
                    .navigate(R.id.action_bluetooth_pairing_pattern_to_previous_screen));

    View proceedBtn = binding.getRoot().findViewById(R.id.proceed_btn);
    proceedBtn.setOnClickListener(v -> {      
      TryNavigateToUltrasound(Utility.FROM_SCREEN_SETUP, true, R.id.action_bluetooth_pairing_pattern_to_calibration_screen);
    });
    return binding.getRoot();
  }


  @Override
  public void onStart() {
    super.onStart();
    IntentFilter filter = new IntentFilter(BluetoothDevice.ACTION_BOND_STATE_CHANGED);
    requireContext().registerReceiver(bondStateReceiver, filter);
  }

  @Override
  public void onStop() {
    super.onStop();
    requireContext().unregisterReceiver(bondStateReceiver);
  }

  @Override
  public void onDestroyView() {
    super.onDestroyView();
    // Clear the button index to prevent stale events on next screen
    if (viewmodel != null) {
      viewmodel.setButtonIndex(null);
    }
  }

  void highlightArrow(boolean isCorrect) {
    if (buttonIndexCount >= 0 && buttonIndexCount < arrowImageViews.size()) {
      ImageView img = arrowImageViews.get(buttonIndexCount);
      GradientDrawable border = new GradientDrawable();
      border.setColor(Color.TRANSPARENT); // No fill
      border.setStroke(8, isCorrect ? Color.GREEN : Color.RED);
      img.setBackground(border);
      buttonIndexCount++;
    }
  }

  void resetArrowButton() {
    if (buttonIndexCount > 0 && buttonIndexCount <= arrowImageViews.size()) {
      // Decrement first to get the correct index (0-based)
      buttonIndexCount--;
      if (buttonIndexCount < arrowImageViews.size()) {
        ImageView img = arrowImageViews.get(buttonIndexCount);
        GradientDrawable border = new GradientDrawable();
        border.setColor(Color.TRANSPARENT); // No fill
        Log.d("highlightArrow", "Background Reset " + buttonIndexCount);
        img.setBackground(null); // This removes any custom background, restoring the original look
      }
    }
  }

  private final BroadcastReceiver bondStateReceiver = new BroadcastReceiver() {
    @Override
    public void onReceive(Context context, Intent intent) {
      if (BluetoothDevice.ACTION_BOND_STATE_CHANGED.equals(intent.getAction())) {
        BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
        int bondState = intent.getIntExtra(BluetoothDevice.EXTRA_BOND_STATE, BluetoothDevice.ERROR);
        if (bondState == BluetoothDevice.BOND_BONDED) {
          android.content.SharedPreferences prefs =
                  requireContext().getSharedPreferences("bonded_device_prefs", Context.MODE_PRIVATE);
          prefs.edit().putString("bonded_mac_address", deviceName).apply();
          
          TryNavigateToUltrasound(Utility.FROM_SCREEN_SETUP, true, R.id.action_bluetooth_pairing_pattern_to_calibration_screen);
        }
      }
    }
  };
}
