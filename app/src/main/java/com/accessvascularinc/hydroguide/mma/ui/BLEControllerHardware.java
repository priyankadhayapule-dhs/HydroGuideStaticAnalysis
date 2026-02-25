package com.accessvascularinc.hydroguide.mma.ui;

import android.Manifest;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanResult;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.ui.interfaces.IControllerHardware;
import com.accessvascularinc.hydroguide.mma.ui.interfaces.IPacketReceiver;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;

import android.bluetooth.le.ScanSettings;
import android.widget.Toast;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import java.util.function.Consumer;

public class BLEControllerHardware implements IControllerHardware {

  private boolean isConnecting = false;

  private static final UUID SERVICE_UUID = UUID.fromString("12345678-1234-5678-1234-56789abcdef0");
  private static final UUID CHARACTERISTIC_UUID =
          UUID.fromString("12345678-1234-5678-1234-56789abcdef1");
  private static final UUID CHARACTERISTIC_UUID_WRITE =
          UUID.fromString("12345678-1234-5678-1234-56789abcdef2");
  private static final int MAX_BLE_RETRIES = 5;
  private int bleRetryCount = 0;
  private static final String TAG = "BLEControllerManager";
  private final Context context;
  private boolean timedOut = false;
  private boolean controllerIsConnected = false;
  private BluetoothGatt bluetoothGatt;
  private BluetoothDevice connectedDevice;
  private BluetoothLeScanner bleScanner;
  private boolean isScanning = false;
  private static final int BLE_PERMISSION_REQUEST_CODE = 1001;
  private IPacketReceiver hardwareClient;
  private String targetDeviceName = null;
  Consumer<Boolean> connectionCallback = b -> {};
  private boolean isBondReceiverRegistered = false;
  private Runnable onDisconnectRequested;
  private Handler mtuTimeoutHandler;
  private Runnable mtuTimeoutRunnable;

  public BLEControllerHardware(Context context) {
    this.context = context;    
  }

  private void RegisterBondReceivers()
  {
    IntentFilter filter = new IntentFilter(BluetoothDevice.ACTION_BOND_STATE_CHANGED);
    context.registerReceiver(bondStateReceiver, filter);
    //isBondReceiverRegistered = true;
    IntentFilter filter2 = new IntentFilter(BluetoothDevice.ACTION_PAIRING_REQUEST);
    filter.setPriority(IntentFilter.SYSTEM_HIGH_PRIORITY); // Override system handler
    context.registerReceiver(bondStateReceiver, filter2);
    isBondReceiverRegistered = true;
  }

  /// TODO - Function to be removed
  @Override
  public void connectController(Consumer<Boolean> onResult) {
    connectionCallback = onResult;
    connectControllerInternal();
  }

  @Override
  public void connectController(Consumer<Boolean> onResult, String deviceName) {
    MedDevLog.info(TAG, "connectController called for device: " + deviceName);
    connectionCallback = onResult;
    targetDeviceName= deviceName;
    android.bluetooth.BluetoothAdapter bluetoothAdapter =
            android.bluetooth.BluetoothAdapter.getDefaultAdapter();
    android.bluetooth.BluetoothDevice device = bluetoothAdapter.getRemoteDevice(deviceName);
    connectToDevice(device);
  }

  @Override
  public boolean checkConnection() {
    return controllerIsConnected;
  }

  @Override
  public void sendMessage(byte[] writeData) {
    sendData(SERVICE_UUID, CHARACTERISTIC_UUID_WRITE, writeData);
  }

  @Override
  public void disconnectController() {
    MedDevLog.info(TAG, "disconnectController called");
    try {
      if (isBondReceiverRegistered) {
        context.unregisterReceiver(bondStateReceiver);
        isBondReceiverRegistered = false;
      }
    } catch (IllegalArgumentException e) {
      MedDevLog.error(TAG, "disconnectController: bondStateReceiver not registered", e);
      isBondReceiverRegistered = false;
    }
    if (bluetoothGatt != null) {
      if (ActivityCompat.checkSelfPermission(context,
              Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
        return;
      }
      Log.d(TAG, "disconnectController: bluetoothGatt not null and passed self permission check");
      bluetoothGatt.disconnect();
      bluetoothGatt.close();
      bluetoothGatt = null;
    }
    controllerIsConnected = false;
    //controllerListeners.forEach((el) -> el.onConnectionStatusChange(false));
    Log.d(TAG, "disconnectController: Disconnected Device");
  }

  @Override
  public void setPacketReceiver(IPacketReceiver client) {
    hardwareClient = client;
  }

  // Allow higher-level manager to handle full disconnect lifecycle
  public void setOnDisconnectRequested(Runnable callback) {
    this.onDisconnectRequested = callback;
  }

  public void connectControllerInternal() {
    // null;
    BluetoothManager bluetoothManager =
            (BluetoothManager) context.getSystemService(Context.BLUETOOTH_SERVICE);
    BluetoothAdapter bluetoothAdapter = bluetoothManager.getAdapter();
    // Check Bluetooth adapter state and warn user if turned off
    if (bluetoothAdapter == null || !bluetoothAdapter.isEnabled()) {
      MedDevLog.warn(TAG, "Bluetooth adapter is null or disabled. Warn user and abort scan.");
      new Handler(Looper.getMainLooper()).post(() ->
              Toast.makeText(context, "Bluetooth is off. Please turn it on.", Toast.LENGTH_LONG).show()
      );
      if (connectionCallback != null) {
        connectionCallback.accept(false);
      }
      return;
    }
    if (ActivityCompat.checkSelfPermission(context,
            Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED) {
      connectionCallback.accept(false);
      return;
    }
    bleScanner = bluetoothAdapter.getBluetoothLeScanner();
    if (bleScanner == null) {
      MedDevLog.error(TAG, "BluetoothLeScanner is null");
      connectionCallback.accept(false);
      return;
    }
    if (isScanning) {
      connectionCallback.accept(false);
      return;
    }
    isScanning = true;
    showScanningProgress();

    // Use explicit ScanSettings to avoid privileged permission requirement
    ScanSettings scanSettings = new ScanSettings.Builder()
            .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
            .build();

    bleScanner.startScan(null, scanSettings, scanCallback);
    // Optionally, stop scan after a timeout (e.g., 10 seconds)
    new Handler(Looper.getMainLooper()).postDelayed(() -> {
      if (isScanning) {
        connectionCallback.accept(false);
        stopScan();
        dismissScanningProgress();
        new Handler(Looper.getMainLooper()).post(() ->
                Toast.makeText(context, "Device Not Found", Toast.LENGTH_LONG).show()
        );
      }
    }, 30000);
  }


  private void dismissScanningProgress() {
    if (context instanceof MainActivity) {
      ((MainActivity) context).dismissScanningProgressDialog();
    }
  }

  private void showScanningProgress() {
    Log.i(TAG, "Progress bar started : " );
    if (context instanceof MainActivity) {
      ((MainActivity) context).showScanningProgressDialog();
    }
  }
  private final BroadcastReceiver bondStateReceiver = new BroadcastReceiver() {
    @Override
    public void onReceive(Context context, Intent intent) {
      final String action = intent.getAction();
      Log.d(TAG, "onReceive: " + action);
      if (BluetoothDevice.ACTION_BOND_STATE_CHANGED.equals(action)) {
        BluetoothDevice device =
                intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE, BluetoothDevice.class);
        int bondState = intent.getIntExtra(BluetoothDevice.EXTRA_BOND_STATE, BluetoothDevice.ERROR);
        if (ActivityCompat.checkSelfPermission(context,
                Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
          return;
        }
        if (device != null) {
          if (bondState == BluetoothDevice.BOND_BONDED) {
            Log.i(TAG, "Bonded with device: " + device.getName());
            controllerIsConnected = true;
            if (connectionCallback != null) {
              connectionCallback.accept(true);
            }
          } else if (bondState == BluetoothDevice.BOND_NONE) {
            MedDevLog.warn(TAG, "Bonding failed or broken for device: " + device.getName());
          }
        }
      }

      if (BluetoothDevice.ACTION_PAIRING_REQUEST.equals(action)) {
        // final BluetoothDevice device =
        // intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE
        // , BluetoothDevice.class);
        final int pairingVariant =
                intent.getIntExtra(BluetoothDevice.EXTRA_PAIRING_VARIANT, BluetoothDevice.ERROR);
        final int passkey2 = intent.getIntExtra(BluetoothDevice.EXTRA_PAIRING_KEY, -1);

        if (passkey2 == -1) {
          Log.i(TAG, "onReceive: Just Works pairing detected, accepting bond");
          return;
        }
        // Convert passkey2 to base 4 string (use long to avoid overflow)
        Log.d(TAG, "onReceive: " + passkey2);
        final String passkeyBase4 =
          String.format("%010d", Long.parseLong(Long.toString(passkey2, 4)));
        Log.d(TAG, "onReceive:Base4 " + passkeyBase4);

        // Map base 4 digits to directions
        final StringBuilder mapped = new StringBuilder();
        for (int i = 0; i < passkeyBase4.length(); i++) {
          final char c = passkeyBase4.charAt(i);
          final String dir = switch (c) {
            case '3' -> "UP";
            case '2' -> "RIGHT";
            case '1' -> "DOWN";
            case '0' -> "LEFT";
            default -> "?";
          };
          if (i > 0) {
            mapped.append(",");
          }
          mapped.append(dir);
        }
        Log.d(TAG, "onReceive: " + pairingVariant + ", passkey2=" + passkey2 + ", base4=" +
                passkeyBase4 + ", mapped="
                + mapped);

        // Navigate to BluetoothPairingPatternFragment and pass the arrow pattern
        android.os.Bundle args = new android.os.Bundle();
        args.putString(context.getString(R.string.key_arrowPattern), mapped.toString());
        args.putString(context.getString(R.string.key_bonded_device), targetDeviceName);
        androidx.navigation.NavController navController = androidx.navigation.Navigation.findNavController(
                        ((MainActivity) context),
                        R.id.nav_host_fragment_activity_main
                );
        int currentDestId = navController.getCurrentDestination() != null ? navController.getCurrentDestination().getId() : -1;
        if (currentDestId == R.id.navigation_bluetooth_pairing) {
          MedDevLog.warn(TAG, "Navigationing to BluetoothPairingFragment");
          navController.navigate(R.id.action_bluetooth_pairing_to_bluetooth_pairing_to_pattern, args);
        } else {
          MedDevLog.warn(TAG, "Navigation skipped: not at BluetoothPairingFragment");
        }

        // Abort broadcast since we are handling pairing
        // May want to comment out when developing locally without the button board
        try {
          // abortBroadcast();
        } catch (Exception e) {
          Log.d(TAG, "onReceive: abortBroadcast failed", e);
        }
      }
    }
  };

  private void stopScan() {
    if (bleScanner != null && isScanning) {
      if (ActivityCompat.checkSelfPermission(context,
              Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED) {
        connectionCallback.accept(false);
        return;
      }
      bleScanner.stopScan(scanCallback);
      isScanning = false;
      Log.i(TAG, "BLE scan stopped.");
    }
  }

  private final ScanCallback scanCallback = new ScanCallback() {
    @Override
    public void onScanResult(int callbackType, ScanResult result) {
      BluetoothDevice device = result.getDevice();
      if (ActivityCompat.checkSelfPermission(context,
              Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
        connectionCallback.accept(false);
        return;
      }

      // Read saved MAC address from SharedPreferences
      android.content.SharedPreferences prefs = context.getSharedPreferences("bonded_device_prefs", Context.MODE_PRIVATE);
      String savedMac = prefs.getString("bonded_mac_address", null);
      Log.d(TAG, "onScanResult: Saved " + savedMac);
      String foundMac = device.getAddress() != null ? device.getAddress().trim().toLowerCase() : "";
      Log.i(TAG,
              "Found device: " + device.getName() + " [" + foundMac + "] RSSI=" + result.getRssi() +
                      " saved mac=" + savedMac + " isConnecting=" + isConnecting +
                      " controllerIsConnected=" + controllerIsConnected);
      // Only connect if MAC matches saved MAC
      if (savedMac != null && foundMac.equals(savedMac.toLowerCase()) &&
              !controllerIsConnected
              && !isConnecting) {
        Log.i(TAG, "Saved MAC matched. Connecting to: " + foundMac);
        stopScan();
        dismissScanningProgress();
        if (device.getBondState() == BluetoothDevice.BOND_BONDED) {
          Log.d(TAG, "onScanResult: Creating bond");
          connectToDevice(device);
        } else {
          Log.i(TAG, "Device not bonded. Initiating bonding...");
          device.createBond();
          // Connection will be handled in bondStateReceiver
        }
      }
    }

    @Override
    public void onScanFailed(int errorCode) {
      MedDevLog.error(TAG, "BLE scan failed with error code: " + errorCode);
    }
  };

  private void connectToDevice(BluetoothDevice device) {
    RegisterBondReceivers();

    if (device == null) {
      return;
    }
    isConnecting = true;
    if (ActivityCompat.checkSelfPermission(context,
            Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
      connectionCallback.accept(false);
      return;
    }
    Log.i(TAG, "Connecting to device: " + device.getName() + " [" + device.getAddress() + "]");
    if (ActivityCompat.checkSelfPermission(context,
            Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
      connectionCallback.accept(false);
      return;
    }
    bluetoothGatt = device.connectGatt(context, false, gattCallback, BluetoothDevice.TRANSPORT_LE);
    connectedDevice = device;
  }

  private final BluetoothGattCallback gattCallback = new BluetoothGattCallback() {
    @Override
    public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
      Log.d(TAG, "onConnectionStateChange: status=" + status + " newState=" + newState);
      if (ActivityCompat.checkSelfPermission(context,
              Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
        connectionCallback.accept(false);
        return;
      }
      if (status == BluetoothGatt.GATT_SUCCESS && newState == BluetoothProfile.STATE_CONNECTED) {

        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.TIRAMISU) {
          gatt.requestConnectionPriority(BluetoothGatt.CONNECTION_PRIORITY_HIGH);
        }

        Log.i(TAG, "Setting PHY 2M");
        gatt.setPreferredPhy(BluetoothDevice.PHY_LE_1M, BluetoothDevice.PHY_LE_1M, BluetoothDevice.PHY_OPTION_NO_PREFERRED);

        Log.i(TAG, "Connected to GATT server. Requesting MTU change to 250...");
        bleRetryCount = 0; // stop retries
        bluetoothGatt.requestMtu(128);
        // Don't request MTU - let device initiate it
        // Set up fallback: if onMtuChanged doesn't fire in 500ms, discover services anyway
        Log.i(TAG, "Waiting for MTU change from device (or timeout)");
        mtuTimeoutHandler = new Handler(Looper.getMainLooper());
        mtuTimeoutRunnable = () -> {
          MedDevLog.warn(TAG, "MTU change callback timeout - proceeding with service discovery");
          if (bluetoothGatt != null && ActivityCompat.checkSelfPermission(context,
                  Manifest.permission.BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED) {
            bluetoothGatt.discoverServices();
          }
        };
        mtuTimeoutHandler.postDelayed(mtuTimeoutRunnable, 500); // 500ms timeout
      } else {
        Log.i(TAG, "Disconnected from GATT server.");
        gatt.close();
        // Perform hardware-level disconnect
        //disconnectController();
        // Notify ControllerManager (if provided) to handle app-level disconnect
        if (onDisconnectRequested != null) {
          try {
            onDisconnectRequested.run();
          } catch (Exception e) {
            MedDevLog.error(TAG, "onDisconnectRequested callback failed", e);
          }
        }
        stopScan();
        dismissScanningProgress();
      }
      /*else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
        Log.i(TAG, "Disconnected from GATT server.");
        gatt.close();
        disconnectController();
        stopScan();
        dismissScanningProgress();
        if (!controllerIsConnected && bleRetryCount < MAX_BLE_RETRIES) {
          bleRetryCount++;
          Log.w(TAG, "Retrying BLE connection in 2 seconds. Attempt " + (bleRetryCount + 1));
          new Handler(Looper.getMainLooper()).postDelayed(
                  BLEControllerHardware.this::connectControllerInternal, 2000);
        } else if (bleRetryCount >= MAX_BLE_RETRIES) {
          MedDevLog.error(TAG, "Max BLE connection retries reached. Giving up.");
        }
      } else if (status != BluetoothGatt.GATT_SUCCESS) {
        MedDevLog.error(TAG, "Connection failed. Status=" + status);
        gatt.close();
        if (!controllerIsConnected && bleRetryCount < MAX_BLE_RETRIES) {
          bleRetryCount++;
          Log.w(TAG, "Retrying BLE connection in 2 seconds. Attempt " + (bleRetryCount + 1));
          new Handler(Looper.getMainLooper()).postDelayed(
                  BLEControllerHardware.this::connectControllerInternal, 2000);
        } else if (bleRetryCount >= MAX_BLE_RETRIES) {
          MedDevLog.error(TAG, "Max BLE connection retries reached. Giving up.");
        }
      }*/
    }

    @Override
    public void onMtuChanged(BluetoothGatt gatt, int mtu, int status) {
      super.onMtuChanged(gatt, mtu, status);
      // Cancel the fallback timeout since MTU callback fired
      if (mtuTimeoutHandler != null && mtuTimeoutRunnable != null) {
        mtuTimeoutHandler.removeCallbacks(mtuTimeoutRunnable);
        mtuTimeoutHandler = null;
        mtuTimeoutRunnable = null;
        Log.d(TAG, "MTU timeout cancelled - callback received");
      }
      if (ActivityCompat.checkSelfPermission(context,
              Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
        connectionCallback.accept(false);
        return;
      }
      if (status == BluetoothGatt.GATT_SUCCESS) {
        Log.i(TAG, "MTU changed successfully to " + mtu);
        gatt.discoverServices();
      } else {
        MedDevLog.error(TAG, "MTU change failed with status: " + status);
        // Optionally, still try to discover services
        gatt.discoverServices();
      }
    }

    @Override
    public void onServicesDiscovered(BluetoothGatt gatt, int status) {
      if (status == BluetoothGatt.GATT_SUCCESS) {
        Log.i(TAG, "Services discovered: " + gatt.getServices().size());
        for (BluetoothGattService service : gatt.getServices()) {
          Log.i(TAG, "Service UUID: " + service.getUuid());
          for (BluetoothGattCharacteristic characteristic : service.getCharacteristics()) {
            Log.i(TAG, "  Characteristic UUID: " + characteristic.getUuid()
                    + ", Properties: " + characteristic.getProperties());
          }
        }
        // Enable notifications on the specified characteristic
        BluetoothGattService targetService = gatt.getService(SERVICE_UUID);
        if (targetService != null) {
          BluetoothGattCharacteristic targetChar =
                  targetService.getCharacteristic(CHARACTERISTIC_UUID);
          if (targetChar != null) {
            if (ActivityCompat.checkSelfPermission(context,
                    Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
              return;
            }
            boolean notifySet = gatt.setCharacteristicNotification(targetChar, true);
            Log.i(TAG, "setCharacteristicNotification returned: " + notifySet);
//            controllerIsConnected = true;
//            connectionCallback.accept(true);
            // Set the CCCD descriptor if present
            if (targetChar.getDescriptor(UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")) !=
                    null) {
              targetChar.getDescriptor(UUID.fromString("00002902-0000-1000-8000-00805f9b34fb"))
                      .setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
              boolean descWrite = gatt
                      .writeDescriptor(targetChar.getDescriptor(
                              UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")));
              Log.i(TAG, "writeDescriptor for notifications returned: " + descWrite);

              if (connectedDevice.getBondState() != BluetoothDevice.BOND_BONDED) {
                connectedDevice.createBond();
              } else {
                controllerIsConnected = true;
                if (connectionCallback != null) {
                  connectionCallback.accept(true);
                }
              }
            } else {
              // connectionCallback.accept(false);
              MedDevLog.warn(TAG, "CCCD descriptor not found for characteristic");
            }
          } else {
            MedDevLog.warn(TAG, "Target characteristic not found in service");
            connectionCallback.accept(false);
          }
        } else {
          MedDevLog.warn(TAG, "Target service not found");
          connectionCallback.accept(false);
        }
      } else {
        MedDevLog.warn(TAG, "onServicesDiscovered received: " + status);
        connectionCallback.accept(false);
      }
    }

    // For API 33+ (Tiramisu), use the new method signature
    @Override
    public void onCharacteristicChanged(@NonNull BluetoothGatt gatt,
                                        BluetoothGattCharacteristic characteristic,
                                        @NonNull byte[] value) {
      // Count notifications
      // notificationCount++;
      // Log.d(TAG, "Notification count: " + notificationCount);

      Log.d(TAG, "onCharacteristicChanged: received");
      if (characteristic.getUuid().equals(CHARACTERISTIC_UUID)) {
        Log.i(TAG, "Notification received from characteristic: " + CHARACTERISTIC_UUID +
                ", data length: "
                + (value != null ? value.length : 0) + ", data : " +
                (value != null ? value[0] : ""));

        hardwareClient.getUsbTimeout().circulateTimeout(false);
        hardwareClient.deserializePacket(value);
      } else {
        Log.i(TAG,
                "Notification received from unknown characteristic: " + characteristic.getUuid());
      }
    }

    // For API < 33, keep the old method for backward compatibility
    @Override
    public void onCharacteristicChanged(BluetoothGatt gatt,
                                        BluetoothGattCharacteristic characteristic) {
      // Count notifications
      // notificationCount++;
      // Log.d(TAG, "Notification count: " + notificationCount);

      if (characteristic.getUuid().equals(CHARACTERISTIC_UUID)) {
        byte[] data = characteristic.getValue();
        Log.i(TAG, "Notification received from characteristic: " + CHARACTERISTIC_UUID +
                ", data length: "
                + (data != null ? data.length : 0));
        // if (notificationListener != null) {
        hardwareClient.getUsbTimeout().circulateTimeout(false);
        hardwareClient.deserializePacket(data);
        // }
      } else {
        Log.i(TAG,
                "Notification received from unknown characteristic: " + characteristic.getUuid());
      }
    }
  };

  public void sendData(UUID serviceUUID, UUID characteristicUUID, byte[] data) {
    if (bluetoothGatt == null) {
      return;
    }
    BluetoothGattService service = bluetoothGatt.getService(serviceUUID);
    if (service == null) {
      return;
    }
    BluetoothGattCharacteristic characteristic = service.getCharacteristic(characteristicUUID);
    if (characteristic == null) {
      return;
    }
    // characteristic.setValue(data);
    if (ActivityCompat.checkSelfPermission(context,
            Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
      return;
    }
    // Use new API on Android 13+ (TIRAMISU) to avoid deprecated call
    if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.TIRAMISU) {
      int writeType = BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT; // or WRITE_TYPE_NO_RESPONSE if needed
      int status = bluetoothGatt.writeCharacteristic(characteristic, data, writeType);
      if(status == 0)
        Log.d(TAG, "sendData: writeCharacteristic (new API) status=" + status + ", length=" + (data != null ? data.length : 0));
      else
        MedDevLog.error(TAG, "sendData: writeCharacteristic (new API) status=" + status + ", length=" + (data != null ? data.length : 0));
    } else {
      // Legacy path
      characteristic.setWriteType(BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT);
      characteristic.setValue(data);
      boolean queued = bluetoothGatt.writeCharacteristic(characteristic);
      Log.d(TAG, "sendData: writeCharacteristic (legacy) queued=" + queued + ", length=" + (data != null ? data.length : 0));
    }
  }
}
