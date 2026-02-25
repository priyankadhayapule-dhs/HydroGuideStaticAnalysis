package com.accessvascularinc.hydroguide.mma.ui;

import android.Manifest;
import android.app.PendingIntent;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothManager;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.AutoCompleteTextView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.databinding.DataBindingUtil;
import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.databinding.FragmentBluetoothPairingBinding;
import androidx.lifecycle.ViewModelProvider;
import com.accessvascularinc.hydroguide.mma.model.ControllerState;
import com.accessvascularinc.hydroguide.mma.utils.Utility;
import com.accessvascularinc.hydroguide.mma.ui.MainViewModel;
import com.ftdi.j2xx.D2xxManager;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

public class BluetoothPairingFragment extends BaseHydroGuideFragment {
  private androidx.appcompat.app.AlertDialog scanningProgressDialog = null;
  private static final UUID SERVICE_UUID = UUID.fromString("12345678-1234-5678-1234-56789abcdef0");
  private BluetoothLeScanner bleScanner;
  AutoCompleteTextView controllerDropdown;
  private boolean isScanning = false;
  String savedMac = null;
  private TextView statusText;
  private boolean isReceiverRegistered = false;

  private void showScanningProgressDialog() {
    if (getActivity() == null)
      return;
    if (scanningProgressDialog != null && scanningProgressDialog.isShowing())
      return;
    android.content.Context context = getActivity();
    android.widget.LinearLayout progressLayout = new android.widget.LinearLayout(context);
    progressLayout.setOrientation(android.widget.LinearLayout.HORIZONTAL);
    progressLayout.setPadding(60, 60, 60, 60);
    progressLayout.setGravity(android.view.Gravity.CENTER);
    android.widget.ProgressBar progressBar = new android.widget.ProgressBar(context);
    progressBar.setIndeterminate(true);
    android.widget.LinearLayout.LayoutParams progressParams = new android.widget.LinearLayout.LayoutParams(100, 100);
    progressBar.setLayoutParams(progressParams);
    progressLayout.addView(progressBar);
    android.widget.TextView textView = new android.widget.TextView(context);
    textView.setText("  Searching for device...");
    textView.setTextSize(18);
    textView.setPadding(40, 0, 0, 0);
    textView.setTextColor(getResources().getColor(android.R.color.black, null));
    progressLayout.addView(textView);
    scanningProgressDialog = new androidx.appcompat.app.AlertDialog.Builder(context)
        .setView(progressLayout)
        .setCancelable(false)
        .create();
    scanningProgressDialog.show();
  }

  private void dismissScanningProgressDialog() {
    if (scanningProgressDialog != null && scanningProgressDialog.isShowing()) {
      scanningProgressDialog.dismiss();
      scanningProgressDialog = null;
    }
  }

  private FragmentBluetoothPairingBinding binding;

  private void startBleScan() {
    // BLEControllerHardware bleControllerHardware = new
    // BLEControllerHardware(requireContext());
    showScanningProgressDialog();
    if (binding == null) {
      Log.d("BLEEE", "startBleScan() aborted: binding is null");
      return;
    }
    View root = binding.getRoot();
    List<String> deviceNames = new ArrayList<>();
    ArrayAdapter<String> adapter = new ArrayAdapter<String>(requireContext(), R.layout.dropdown_list_item, deviceNames);
    controllerDropdown.setAdapter(adapter);
    controllerDropdown.post(controllerDropdown::showDropDown);

    Log.d("BLEEE", "startBleScan() called");
    // Show progress indicator (wait cursor)
    root.setEnabled(false);
    root.setAlpha(0.5f);
    root.setClickable(true);
    root.setFocusable(true);
    root.setFocusableInTouchMode(true);
    final Runnable[] scanTimeoutRunnable = new Runnable[1];
    android.os.Handler handler = new android.os.Handler(android.os.Looper.getMainLooper());
    scanTimeoutRunnable[0] = () -> {
      Log.d("BLEEE", "Scan timeout reached");
      android.widget.Toast.makeText(requireContext(), "BLE scan timeout", android.widget.Toast.LENGTH_SHORT).show();
      root.setEnabled(true);
      root.setAlpha(1f);
      root.setClickable(false);
      root.setFocusable(false);
      root.setFocusableInTouchMode(false);
      adapter.notifyDataSetChanged();
    };
    handler.postDelayed(scanTimeoutRunnable[0], 30000);
    Log.d("BLEEE", "Calling scanAllDevices now");
    // scanAllDevices now only returns devices advertising SERVICE_UUID
    scanAllDevices(new DeviceScanCallback() {
      @Override
      public void onDeviceFound(String deviceName) {
        Log.d("BLEEE", "onDeviceFound: " + deviceName);
        if (!deviceNames.contains(deviceName)) {
          deviceNames.add(deviceName);
          adapter.notifyDataSetChanged();
          controllerDropdown.showDropDown();
        }
      }

      @Override
      public void onScanComplete() {
        dismissScanningProgressDialog();
        Log.d("BLEEE", "onScanComplete");
        handler.removeCallbacks(scanTimeoutRunnable[0]);
        root.setEnabled(true);
        root.setAlpha(1f);
        root.setClickable(false);
        root.setFocusable(false);
        root.setFocusableInTouchMode(false);
        adapter.notifyDataSetChanged();
        controllerDropdown.showDropDown();
        android.widget.Toast.makeText(requireContext(), "BLE scan complete", android.widget.Toast.LENGTH_SHORT).show();

      }
    }, 5000);
  }

  private static final int REQUEST_BLUETOOTH_SCAN = 1002;
  private static final String ACTION_USB_PERMISSION = "com.accessvascularinc.hydroguide.USB_PERMISSION";

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
      @Nullable Bundle savedInstanceState) {
    binding = DataBindingUtil.inflate(inflater,
        R.layout.fragment_bluetooth_pairing, container, false);

    String title = getString(R.string.connect_controller_title);
    if (getArguments() != null && getArguments().containsKey("title")) {
      String argTitle = getArguments().getString("title");
      if (argTitle != null && !argTitle.isEmpty()) {
        title = argTitle;
      }
    }
    binding.setTitle(title);

    viewmodel = new ViewModelProvider(requireActivity()).get(MainViewModel.class);
    binding.topBar.setTabletState(viewmodel.getTabletState());
    tvBattery = binding.topBar.controllerBatteryLevelPct;
    ivBattery = binding.topBar.remoteBatteryLevelIndicator;
    ivUsbIcon = binding.topBar.usbIcon;
    ivTabletBattery = binding.topBar.tabletBatteryIndicator.tabletBatteryLevelIndicator;
    tvTabletBattery = binding.topBar.tabletBatteryIndicator.tabletBatteryLevelPct;
    hgLogo = binding.topBar.appLogo;
    lpTabletIcon = binding.topBar.tabletBatteryWarningIcon.batteryIcon;
    lpTabletSymbol = binding.topBar.tabletBatteryWarningIcon.batteryWarningImg;
    controllerDropdown = binding.getRoot().findViewById(R.id.controller_name_input);
    statusText = binding.getRoot()
        .findViewById(R.id.ble_connection_label); // replace with your TextView id

    ivBattery.setVisibility(View.GONE);
    binding.topBar.controllerBatteryLabel.setVisibility(View.GONE);
    tvBattery.setVisibility(View.GONE);
    statusText.setVisibility(View.GONE);
    ivUsbIcon.setVisibility(View.GONE);

    View backBtn = binding.getRoot().findViewById(R.id.back_btn);
    backBtn.setOnClickListener(v -> androidx.navigation.Navigation.findNavController(binding.getRoot())
        .navigate(R.id.action_bluetooth_pairing_to_setup_screen));

    // Pair button logic
    View pairBtn = binding.getRoot().findViewById(R.id.pair_btn);
    pairBtn.setOnClickListener(v -> {
      String selected = controllerDropdown.getText().toString();
      if (selected == null || selected.isEmpty()) {
        android.widget.Toast
            .makeText(requireContext(), "Please select a device to pair", android.widget.Toast.LENGTH_SHORT).show();
        return;
      }
      // Extract MAC address (format: MAC (DeviceName) or just MAC)
      String mac = selected.split(" \\(")[0].trim();
      android.bluetooth.BluetoothAdapter bluetoothAdapter = android.bluetooth.BluetoothAdapter.getDefaultAdapter();
      if (bluetoothAdapter == null) {
        android.widget.Toast.makeText(requireContext(), "Bluetooth not available", android.widget.Toast.LENGTH_SHORT)
            .show();
        return;
      }
      if (savedMac != null && savedMac.equals(mac) &&
          viewmodel.getControllerCommunicationManager().checkConnection()) {
        TryNavigateToUltrasound(Utility.FROM_SCREEN_SETUP, true,
            R.id.action_bluetooth_pairing_pattern_to_calibration_screen);

      } else {
        android.bluetooth.BluetoothDevice device = bluetoothAdapter.getRemoteDevice(mac);
        if (device == null) {
          android.widget.Toast.makeText(requireContext(), "Device not found",
              android.widget.Toast.LENGTH_SHORT).show();
          return;
        }
        viewmodel.getControllerCommunicationManager().connectController(mac);
      }
    });

    // BLE device dropdown population with progress indicator and 30s timeout
    // Request all relevant BLE permissions robustly
    // Only request these at runtime; BLUETOOTH and BLUETOOTH_ADMIN are
    // manifest-only
    String[] permissions = new String[] {
        android.Manifest.permission.BLUETOOTH_SCAN,
        android.Manifest.permission.BLUETOOTH_CONNECT,
        android.Manifest.permission.ACCESS_FINE_LOCATION
    };
    List<String> permissionsToRequest = new ArrayList<>();
    for (String perm : permissions) {
      if (androidx.core.content.ContextCompat.checkSelfPermission(requireContext(),
          perm) != android.content.pm.PackageManager.PERMISSION_GRANTED) {
        permissionsToRequest.add(perm);
      }
    }
    viewmodel.getControllerCommunicationManager().configureHardware(true);
    if (!permissionsToRequest.isEmpty()) {
      Log.d("BLEEE", "Requesting permissions: " + permissionsToRequest);
      android.widget.Toast.makeText(requireContext(), "Requesting BLE permissions", android.widget.Toast.LENGTH_SHORT)
          .show();
      requestPermissions(permissionsToRequest.toArray(new String[0]), REQUEST_BLUETOOTH_SCAN);
    } else {
      Log.d("BLEEE", "All permissions granted, about to scanAllDevices");
      android.widget.Toast
          .makeText(requireContext(), "Permissions granted, scanning BLE devices", android.widget.Toast.LENGTH_SHORT)
          .show();
      android.content.SharedPreferences prefs = requireContext().getSharedPreferences("bonded_device_prefs",
          Context.MODE_PRIVATE);
      savedMac = prefs.getString("bonded_mac_address", null);
      Log.d("BLEEE", "Shared" + savedMac);

      if (savedMac != null) {
        // Check if device is still bonded on the tablet
        android.bluetooth.BluetoothAdapter bluetoothAdapter = android.bluetooth.BluetoothAdapter.getDefaultAdapter();
        boolean isBonded = false;
        if (bluetoothAdapter != null) {
          for (android.bluetooth.BluetoothDevice bondedDevice : bluetoothAdapter.getBondedDevices()) {
            if (bondedDevice.getAddress().equals(savedMac)) {
              isBonded = true;
              break;
            }
          }
        }
        if (!isBonded) {
          // Device is not bonded anymore, treat as no savedMac
          savedMac = null;
        }
      }
      if (savedMac != null) {
        List<String> deviceNames = new ArrayList<>();
        deviceNames.add(savedMac);
        ArrayAdapter<String> adapter = new ArrayAdapter<>(requireContext(), R.layout.dropdown_list_item, deviceNames);
        controllerDropdown.setAdapter(adapter);
        controllerDropdown.setText(savedMac, false);
        if (viewmodel.getBleConnectionLiveData().getValue() != MainViewModel.BleConnectionState.CONNECTED) {
          viewmodel.getControllerCommunicationManager().connectController(savedMac);
          checkConnectionStatus(statusText, true);
        }

      } else {
        startBleScan();
      }
    }

    View usbConnectButton = binding.getRoot().findViewById(R.id.usb_alternative_label);
    usbConnectButton.setOnClickListener(v -> {
      this.statusText = statusText;

      // Request USB permission
      UsbManager usbManager = (UsbManager) requireContext().getSystemService(Context.USB_SERVICE);
      if (usbManager != null) {
        boolean found = false;
        for (UsbDevice device : usbManager.getDeviceList().values()) {
          if (device.getVendorId() == 0x0403) { // FTDI vendor ID
            found = true;
            if (!usbManager.hasPermission(device)) {
              Intent intent = new Intent(ACTION_USB_PERMISSION);
              intent.setPackage(requireContext().getPackageName());
              intent.putExtra(UsbManager.EXTRA_DEVICE, device);

              PendingIntent permissionIntent = PendingIntent.getBroadcast(
                  requireContext(),
                  0,
                  intent,
                  PendingIntent.FLAG_IMMUTABLE | PendingIntent.FLAG_UPDATE_CURRENT);
              usbManager.requestPermission(device, permissionIntent);
              Log.i("USB", "Permission request sent for device: " + device.getDeviceName());
            } else {
              // Permission already granted
              viewmodel.getControllerCommunicationManager().configureHardware(false);
              viewmodel.getControllerCommunicationManager().connectController();
              checkConnectionStatus(statusText, false);
            }
            break;
          }
        }
        if (!found) {
          android.widget.Toast.makeText(requireContext(), "USB device not found", android.widget.Toast.LENGTH_SHORT)
              .show();
        }
      }
    });
    return binding.getRoot();
  }

  @Override
  public void onResume() {
    super.onResume();
    if (!isReceiverRegistered) {
      final IntentFilter usbFilter = new IntentFilter();
      usbFilter.addAction(ACTION_USB_PERMISSION);
      usbFilter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
      usbFilter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
      ContextCompat.registerReceiver(requireContext(), mUsbReceiver, usbFilter,
          ContextCompat.RECEIVER_NOT_EXPORTED);
      isReceiverRegistered = true;
    } else {
      Log.i("USB", "Receiver already registered, skipping");
    }
  }

  @Override
  public void onPause() {
    super.onPause();
  }

  @Override
  public void onDestroyView() {
    super.onDestroyView();
    // Unregister USB permission receiver
    if (isReceiverRegistered) {
      try {
        requireContext().unregisterReceiver(mUsbReceiver);
        isReceiverRegistered = false;
      } catch (IllegalArgumentException e) {
      }
    }
  }

  private final BroadcastReceiver mUsbReceiver = new BroadcastReceiver() {
    @Override
    public void onReceive(final Context context, final Intent intent) {
      String action = intent.getAction();

      if (ACTION_USB_PERMISSION.equals(action)) {
        synchronized (this) {
          viewmodel.getControllerCommunicationManager().configureHardware(false);
          viewmodel.getControllerCommunicationManager().connectController();
          checkConnectionStatus(statusText, false);
        }
      } else if (UsbManager.ACTION_USB_DEVICE_DETACHED.equals(action)) {
      }
    }
  };

  private void checkConnectionStatus(TextView statusText, boolean isBLE) {
    viewmodel.getBleConnectionLiveData().observe(getViewLifecycleOwner(), bleState -> {
      statusText.setVisibility(View.VISIBLE);
      if (bleState == MainViewModel.BleConnectionState.CONNECTED) {
        statusText.setText("Connected");
        TryNavigateToUltrasound(Utility.FROM_SCREEN_SETUP, true,
            R.id.action_bluetooth_pairing_pattern_to_calibration_screen);
        statusText.setClickable(false);
        statusText.setOnClickListener(null);
      } else if (bleState == MainViewModel.BleConnectionState.CONNECTING) {
        statusText.setText("Connecting....");
        statusText.setClickable(false);
        statusText.setOnClickListener(null);
      } else if (bleState == MainViewModel.BleConnectionState.FAILED ||
          bleState == MainViewModel.BleConnectionState.NOTCONNECTED) {
        statusText.setText("Connection failed. Click to retry");
        statusText.setClickable(true);
        statusText.setOnClickListener(c -> {
          // Retry logic
          viewmodel.getControllerCommunicationManager().disconnectController();
          viewmodel.getControllerCommunicationManager().configureHardware(isBLE);
          viewmodel.setBleConnection(MainViewModel.BleConnectionState.NOTCONNECTED);
          if (isBLE) {
            Log.d("BLEEE", "checkConnectionStatus: " + savedMac);
            viewmodel.getControllerCommunicationManager().connectController(savedMac);
          } else
            viewmodel.getControllerCommunicationManager().connectController();
        });
      }
    });
  }

  // ...existing code...
  @Override
  public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
    super.onRequestPermissionsResult(requestCode, permissions, grantResults);
    if (requestCode == REQUEST_BLUETOOTH_SCAN) {
      Log.d("BLEEE", "onRequestPermissionsResult: " + java.util.Arrays.toString(grantResults));
      if (grantResults.length > 0 && grantResults[0] == android.content.pm.PackageManager.PERMISSION_GRANTED) {
        android.widget.Toast
            .makeText(requireContext(), "BLE permission granted, scanning now", android.widget.Toast.LENGTH_SHORT)
            .show();
        startBleScan();
      } else {
        android.widget.Toast.makeText(requireContext(), "BLE permission denied", android.widget.Toast.LENGTH_SHORT)
            .show();
      }
    }
  }

  public void scanAllDevices(DeviceScanCallback callback, long... timeoutMillis) {
    this.deviceScanCallback = callback;
    scannedDeviceNames.clear();
    Log.d("BLEEE", "scanAllDevices: ");
    BluetoothManager bluetoothManager = (BluetoothManager) requireContext().getSystemService(Context.BLUETOOTH_SERVICE);
    BluetoothAdapter bluetoothAdapter = bluetoothManager.getAdapter();
    if (bluetoothAdapter == null || !bluetoothAdapter.isEnabled()) {
      if (deviceScanCallback != null)
        deviceScanCallback.onScanComplete();
      return;
    }
    if (ActivityCompat.checkSelfPermission(requireContext(),
        Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED) {
      if (deviceScanCallback != null)
        deviceScanCallback.onScanComplete();
      return;
    }
    bleScanner = bluetoothAdapter.getBluetoothLeScanner();
    if (bleScanner == null) {
      if (deviceScanCallback != null)
        deviceScanCallback.onScanComplete();
      return;
    }
    isScanning = true;
    ScanSettings scanSettings = new ScanSettings.Builder().setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY).build();
    bleScanner.startScan(null, scanSettings, scanCallbackFilteredByService);
    long scanTimeout = (timeoutMillis != null && timeoutMillis.length > 0) ? timeoutMillis[0] : 30000;
    new Handler(Looper.getMainLooper()).postDelayed(() -> {
      if (isScanning) {
        bleScanner.stopScan(scanCallbackFilteredByService);
        isScanning = false;
        if (deviceScanCallback != null)
          deviceScanCallback.onScanComplete();
      }
    }, scanTimeout);
  }

  // ScanCallback for listing all BLE devices (for dropdown)
  private final ScanCallback scanCallbackFilteredByService = new ScanCallback() {
    @Override
    public void onScanResult(int callbackType, ScanResult result) {
      BluetoothDevice device = result.getDevice();
      if (ActivityCompat.checkSelfPermission(requireContext(),
          Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
        return;
      }
      // Only add devices advertising the target service UUID
      if (result.getScanRecord() != null && result.getScanRecord().getServiceUuids() != null) {
        List<android.os.ParcelUuid> serviceUuids = result.getScanRecord().getServiceUuids();
        for (android.os.ParcelUuid uuid : serviceUuids) {
          if (SERVICE_UUID.equals(uuid.getUuid())) {
            String name = device.getName();
            String mac = device.getAddress();
            int rssid = result.getRssi();
            // String display = mac + (name != null ? " (" + name + ")" : "") + "_" + rssid;
            String display = mac + (name != null ? " (" + name + ")" : "");
            if (!scannedDeviceNames.contains(display)) {
              scannedDeviceNames.add(display);
              if (deviceScanCallback != null)
                deviceScanCallback.onDeviceFound(display);
            }
            break;
          }
        }
      }
    }

    @Override
    public void onScanFailed(int errorCode) {
      if (deviceScanCallback != null)
        deviceScanCallback.onScanComplete();
    }
  };

  // Callback interface for device scan results
  public interface DeviceScanCallback {
    void onDeviceFound(String deviceName);

    void onScanComplete();
  }

  private DeviceScanCallback deviceScanCallback;
  private List<String> scannedDeviceNames = new ArrayList<>();

  // ScanCallback for dropdown population
  private final ScanCallback scanCallbackForDropdown = new ScanCallback() {
    @Override
    public void onScanResult(int callbackType, ScanResult result) {
      BluetoothDevice device = result.getDevice();
      if (ActivityCompat.checkSelfPermission(requireContext(),
          Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
        return;
      }
      // Only add devices advertising the target service UUID
      if (result.getScanRecord() != null && result.getScanRecord().getServiceUuids() != null) {
        List<android.os.ParcelUuid> serviceUuids = result.getScanRecord().getServiceUuids();
        for (android.os.ParcelUuid uuid : serviceUuids) {
          if (SERVICE_UUID.equals(uuid.getUuid())) {
            String name = device.getName();
            String mac = device.getAddress();
            String display = mac + (name != null ? " (" + name + ")" : "");
            if (!scannedDeviceNames.contains(display)) {
              scannedDeviceNames.add(display);
              if (deviceScanCallback != null)
                deviceScanCallback.onDeviceFound(display);
            }
            break;
          }
        }
      }
    }

    @Override
    public void onScanFailed(int errorCode) {
      if (deviceScanCallback != null)
        deviceScanCallback.onScanComplete();
    }
  };

}
