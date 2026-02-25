package com.accessvascularinc.hydroguide.mma.ui;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageButton;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.databinding.DataBindingUtil;
import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.databinding.FragmentConnectUltrasoundBinding;
import androidx.lifecycle.ViewModelProvider;
import com.accessvascularinc.hydroguide.mma.model.ControllerState;
import com.accessvascularinc.hydroguide.mma.ui.MainViewModel;
import com.accessvascularinc.hydroguide.mma.utils.Utility;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;

import java.util.HashMap;

public class ConnectUltrasoundFragment extends BaseHydroGuideFragment {
    
    private BroadcastReceiver usbReceiver;
    private boolean probeConnected = false;

    private String fromScreen;

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                             @Nullable Bundle savedInstanceState) {
        FragmentConnectUltrasoundBinding binding = DataBindingUtil.inflate(inflater,
                R.layout.fragment_connect_ultrasound, container, false);

        String title = getString(R.string.connect_ultrsound_probe_title);
        if (getArguments() != null && getArguments().containsKey("title")) {
            String argTitle = getArguments().getString("title");
            if (argTitle != null && !argTitle.isEmpty()) {
                title = argTitle;
            }
        }
        binding.setTitle(title);

        viewmodel = new ViewModelProvider(requireActivity()).get(MainViewModel.class);
        ControllerState controllerState = viewmodel.getControllerState();
        binding.topBar.setControllerState(controllerState);
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

        if (getArguments() != null) {
            fromScreen = getArguments().getString("fromScreen", Utility.FROM_SCREEN_PATIENT_INPUT);
        } else {
            fromScreen = Utility.FROM_SCREEN_PATIENT_INPUT;
        }

        ImageButton backBtn = binding.getRoot().findViewById(R.id.back_btn);
        TextView backText = binding.getRoot().findViewById(R.id.back_btn_text);
        if (Utility.FROM_SCREEN_HOME.equals(fromScreen)) {
            backBtn.setImageResource(R.drawable.white_home);
            backText.setText("Home");
        } else {
            backBtn.setImageResource(R.drawable.left_back);
            backText.setText("Back");
        }
        backBtn.setOnClickListener(v -> {
            if (Utility.FROM_SCREEN_HOME.equals(fromScreen)) {
                androidx.navigation.Navigation.findNavController(binding.getRoot())
                        .navigate(R.id.action_connect_ultrasound_to_home);
            } else {
                androidx.navigation.Navigation.findNavController(binding.getRoot())
                        .navigate(R.id.action_connect_ultrasound_to_patient_input);
            }
        });

        binding.proceedBtn
                .setOnClickListener(v -> {                   
                    TryNavigateToUltrasound(fromScreen, R.id.action_connect_ultrasound_to_setup);
                });

        View proceedMessage = binding.getRoot().findViewById(R.id.proceed_message);
        proceedMessage.setOnClickListener(v -> {
            // Reset ultrasound flags when proceeding without probe
            if (viewmodel != null && viewmodel.getEncounterData() != null) {
                viewmodel.getEncounterData().setIsUltrasoundUsed(false);
                viewmodel.getEncounterData().setIsUltrasoundCaptureSaved(false);
                updateProcedureFile();
                Log.d("ConnectUltrasound", "Reset ultrasound flags when proceeding without probe");
            }
            androidx.navigation.Navigation.findNavController(binding.getRoot())
                    .navigate(R.id.action_connect_ultrasound_to_setup);
        });

        return binding.getRoot();
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        // Check if probe is already connected
        if (isProbeConnected()) {
            Log.d("ConnectUltrasound",
                    "Probe already connected, auto-navigating to scanning screen");
            probeConnected = true;
            // Small delay for UI to settle
            new android.os.Handler(android.os.Looper.getMainLooper()).postDelayed(() -> {                
                navigateToUltrasound(fromScreen);
            }, 300);
        }

        // Register USB receiver to detect probe connection
        usbReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                if (UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(intent.getAction())) {
                    UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                    if (device != null && device.getVendorId() == 0x1dbf) {
                        Log.d("ConnectUltrasound",
                                "Probe connected, auto-navigating to scanning screen");
                        probeConnected = true;
                        // Navigate to scanning screen                       
                        navigateToUltrasound(fromScreen);
                    }
                }
            }
        };

        IntentFilter filter = new IntentFilter(UsbManager.ACTION_USB_DEVICE_ATTACHED);
        requireContext().registerReceiver(usbReceiver, filter, Context.RECEIVER_NOT_EXPORTED);
    }


    @Override
    public void onDestroyView() {
        super.onDestroyView();

        // Unregister USB receiver
        if (usbReceiver != null) {
            try {
                requireContext().unregisterReceiver(usbReceiver);
                usbReceiver = null;
            } catch (Exception e) {
                MedDevLog.error("ConnectUltrasound", "Error unregistering USB receiver", e);
            }
        }
    }
}
