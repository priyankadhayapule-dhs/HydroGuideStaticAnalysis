package com.accessvascularinc.hydroguide.mma.ui;

import android.Manifest;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.net.Uri;
import android.os.BatteryManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.os.StatFs;
import android.provider.Settings;
import android.util.Log;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.core.content.ContextCompat;
import androidx.core.app.ActivityCompat;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.lifecycle.ViewModelProvider;
import androidx.navigation.NavController;
import androidx.navigation.fragment.NavHostFragment;
import androidx.navigation.ui.AppBarConfiguration;
import androidx.navigation.ui.NavigationUI;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.databinding.ActivityMainBinding;
import com.accessvascularinc.hydroguide.mma.model.DataFiles;
import com.accessvascularinc.hydroguide.mma.model.Facility;
import com.accessvascularinc.hydroguide.mma.ultrasound.GlobalUltrasoundManager;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import com.echonous.hardware.ultrasound.DauException;
import com.echonous.hardware.ultrasound.ThorError;
import com.echonous.hardware.ultrasound.UltrasoundManager;
import com.ftdi.j2xx.D2xxManager;
import com.google.android.material.bottomnavigation.BottomNavigationView;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import org.json.JSONArray;
import org.json.JSONException;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.Objects;

import dagger.hilt.android.AndroidEntryPoint;

@AndroidEntryPoint
public class MainActivity extends AppCompatActivity {
  private static final long MAX_LOGFILE_SIZE = (long) 90000.0; // 90KB
  private final Handler pollingHandler = new Handler(Looper.getMainLooper());
  private MainViewModel mainViewModel = null;
  private Scheduler memoryPoll;
  private androidx.activity.result.ActivityResultLauncher<String[]> blePermissionLauncher;
  /*********** USB broadcast receiver *******************************************/
  private final BroadcastReceiver mUsbReceiver = new BroadcastReceiver() {
    @Override
    public void onReceive(final Context context, final Intent intent) {
      String action = intent.getAction();
      if (UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(action)) {
        Log.i("ABC", "ATTACHED...");
        UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
        if (device != null && device.getVendorId() == 0x1dbf) {
          // Ultrasound probe attached - initialize SDK now
          // This prevents crash because SDK's internal receiver won't be registered
          // until AFTER we finish processing this USB_DEVICE_ATTACHED event
          Log.d("MainActivity", "Ultrasound probe attached, initializing SDK");
          initializeUltrasoundSDK();
          return;
        }
      }
      
      if (UsbManager.ACTION_USB_DEVICE_DETACHED.equals(intent.getAction())) {
        Log.i("ABC", "DETACHED...");
        mainViewModel.getControllerCommunicationManager().notifyUSBDeviceDetach();
      }
    }
  };
  /*********** DAU Connection broadcast receiver ********************************/
  private final BroadcastReceiver mDauConnectionReceiver = new BroadcastReceiver() {
    @Override
    public void onReceive(final Context context, final Intent intent) {
      String action = intent.getAction();
      if (UltrasoundManager.ACTION_DAU_CONNECTED.equals(action)) {
        int probeId = intent.getIntExtra(UltrasoundManager.EXTRA_PROBE_PID, -1);
        Log.d("MainActivity", "Ultrasound probe connected via SDK: " + probeId);
        // SDK has successfully connected the probe
        // The app can now use the probe normally
      } else if (UltrasoundManager.ACTION_DAU_DISCONNECTED.equals(action)) {
        int probeId = intent.getIntExtra(UltrasoundManager.EXTRA_PROBE_PID, -1);
        Log.d("MainActivity", "Ultrasound probe disconnected: " + probeId);
        // Don't call closeDau() here - SDK's internal error handling has already
        // triggered proper native cleanup via DauNative.onTerminate()
        // Only clean up the manager wrapper to prepare for next connection
        GlobalUltrasoundManager.getInstance().clean();
        GlobalUltrasoundManager.releaseInstance();
        sdkInitialized = false;
      }
    }
  };
  /*********** Battery Change receiver ******************************************/
  private final BroadcastReceiver mBatteryReceiver = new BroadcastReceiver() {
    @Override
    public void onReceive(final Context context, final Intent intent) {
      if (Intent.ACTION_BATTERY_CHANGED.equals(intent.getAction())) {
        final int level = intent.getIntExtra(BatteryManager.EXTRA_LEVEL, -1);
        // int scale = intent.getIntExtra(BatteryManager.EXTRA_LEVEL, -1);
        final int batteryPct = level;
        mainViewModel.getTabletState().setTabletChargePct(batteryPct);

        // CHECKING FOR AND UPDATING CHARGING STATUS //

        // int status = intent.getIntExtra(BatteryManager.EXTRA_STATUS, -1); //
        // Unreliable method
        // boolean batteryCharge = status == BatteryManager.BATTERY_STATUS_CHARGING;

        final int chargePlug = intent.getIntExtra(BatteryManager.EXTRA_PLUGGED, -1);
        final boolean usbCharge = chargePlug == BatteryManager.BATTERY_PLUGGED_USB;
        final boolean acCharge = chargePlug == BatteryManager.BATTERY_PLUGGED_AC;

        final boolean wasCharging = mainViewModel.getTabletState().getIsCharging();
        boolean currentlyCharging = wasCharging;

        // if (batteryCharge) mainViewModel.getTabletState().setIsCharging(true); else
        if (usbCharge) {
          currentlyCharging = true;
        } else if (acCharge) {
          currentlyCharging = true;
        } else if (wasCharging) {
          currentlyCharging = false;
        }

        if (currentlyCharging != wasCharging) {
          mainViewModel.getTabletState().setIsCharging(currentlyCharging);
        }

        // LOGGING BATTERY CHANGES FOR TESTING //
        final String TAG = "BatteryLogging";
        final File root = context.getExternalFilesDir(null);
        if (root == null) {
          MedDevLog.error(TAG, "External files directory is not available. Check storage" +
              " permissions or device state.");
        }
        final String filename = "batteryLogs.txt";
        final File batteryLogFile = new File(root, filename);

        final SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault());
        final String timestamp = sdf.format(new Date());

        try (final FileWriter writer = new FileWriter(batteryLogFile,
            StandardCharsets.UTF_8, true)) {
          final String msg;
          msg = "Timestamp: " + timestamp + " Percentage: " + batteryPct + "\n";
          writer.write(msg);
          writer.close();
          Log.i(TAG, "Tablet battery level recorded");
        } catch (final IOException e) {
          MedDevLog.error(TAG,
              "Error writing to battery log file: " + batteryLogFile.getAbsolutePath(), e);
        } catch (final Exception e) {
          MedDevLog.error(TAG, "Error during battery polling" + batteryLogFile.getAbsolutePath(), e);
        }
      }
    }
  };

  @Override
  protected void onCreate(final Bundle savedInstanceState){
    super.onCreate(savedInstanceState);
    final ActivityMainBinding binding = ActivityMainBinding.inflate(getLayoutInflater());
    setContentView(binding.getRoot());

    mainViewModel = new ViewModelProvider(this).get(MainViewModel.class);

    final BottomNavigationView navView = findViewById(R.id.nav_view);
    final AppBarConfiguration appBarConfiguration = new AppBarConfiguration.Builder(R.id.navigation_procedure,
        R.id.navigation_patient_input, R.id.navigation_summary).build();
    final NavHostFragment navHostFragment = (NavHostFragment) getSupportFragmentManager()
        .findFragmentById(R.id.nav_host_fragment_activity_main);
    final NavController navController = Objects.requireNonNull(navHostFragment).getNavController();
    NavigationUI.setupActionBarWithNavController(this, navController, appBarConfiguration);
    NavigationUI.setupWithNavController(navView, navController);
    Objects.requireNonNull(getSupportActionBar()).hide();

    loadFacilities();

    // Check if ultrasound probe is already connected at startup
    // If yes, initialize SDK now. If no, wait for USB_DEVICE_ATTACHED event
    UsbManager usbManager = (UsbManager) getSystemService(Context.USB_SERVICE);
    if (usbManager != null) {
      for (UsbDevice device : usbManager.getDeviceList().values()) {
        if (device.getVendorId() == 0x1dbf) {
          Log.d("MainActivity", "Ultrasound probe already connected at startup");
          initializeUltrasoundSDK();
          break;
        }
      }
    }
 
    try {
      /*
       * final D2xxManager ftD2xx = D2xxManager.getInstance(this);
       * // Specify a non-default VID and PID combination to match if required.
       * if (!ftD2xx.setVIDPID(0x0403, 0xada1)) {
       * Log.i("ftd2xx-java", "setVIDPID Error");
       * }
       */
      mainViewModel.injectControllerManager(new ControllerManager(this));
    } catch (final D2xxManager.D2xxException ex) {
      MedDevLog.error("MainActivity", "Error initializing D2xxManager", ex);
    }

    final File root = getApplicationContext().getExternalFilesDir(null);
    final File batteryLogs = new File(root, "batteryLogs.txt");
    if (batteryLogs.exists() && batteryLogs.length() >= MAX_LOGFILE_SIZE) { // 20kb
      batteryLogs.delete();
    }

    final IntentFilter usbFilter = new IntentFilter();
    usbFilter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
    usbFilter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
    ContextCompat.registerReceiver(this, mUsbReceiver, usbFilter, ContextCompat.RECEIVER_NOT_EXPORTED);

    final IntentFilter dauFilter = new IntentFilter();
    dauFilter.addAction(UltrasoundManager.ACTION_DAU_CONNECTED);
    dauFilter.addAction(UltrasoundManager.ACTION_DAU_DISCONNECTED);
    ContextCompat.registerReceiver(this, mDauConnectionReceiver, dauFilter, ContextCompat.RECEIVER_NOT_EXPORTED);

    final IntentFilter batteryFilter = new IntentFilter();
    batteryFilter.addAction(Intent.ACTION_BATTERY_CHANGED);
    ContextCompat.registerReceiver(this, mBatteryReceiver, batteryFilter,
        ContextCompat.RECEIVER_NOT_EXPORTED);

    // Needed to change the brightness.
    if (!Settings.System.canWrite(getApplicationContext())) {
      final Intent intent = new Intent(Settings.ACTION_MANAGE_WRITE_SETTINGS);
      // The intent will open asking for permission from user and package name is
      // provided to
      // directly jump to the option of this app in the list of apps.
      intent.setData(Uri.parse("package:" + this.getPackageName()));
      startActivity(intent);
    }

    // TODO : Is this needed?
    // Setting up memory polling
    final long delay = 1000; // 1 second
    memoryPoll = new Scheduler(pollingHandler, delay, () -> {
      final File path = Environment.getDataDirectory();
      final StatFs stat = new StatFs(path.getPath());
      final long blockSize = stat.getBlockSizeLong();
      final long availableBlocks = stat.getAvailableBlocksLong();
      final long totalBlocks = stat.getBlockCountLong();
      mainViewModel.getTabletState().setFreeSpaceBytes(blockSize * availableBlocks);
      mainViewModel.getTabletState().setTotalSpaceBytes(blockSize * totalBlocks);
    });
    memoryPoll.beginPolling();

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.VANILLA_ICE_CREAM) {
      ViewCompat.setOnApplyWindowInsetsListener(binding.getRoot(), (view, windowInsets) -> {
        final Insets insets = windowInsets.getInsets(WindowInsetsCompat.Type.systemBars());
        view.setPadding(0, insets.top, 0, insets.bottom);
        view.setBackgroundColor(getResources().getColor(R.color.av_darkest_blue, getTheme()));

        return windowInsets;
      });
    }

    // Register BLE permission launcher early in lifecycle
    blePermissionLauncher = registerForActivityResult(
        new ActivityResultContracts.RequestMultiplePermissions(),
        result -> {
          final boolean scanGranted = Boolean.TRUE
              .equals(result.getOrDefault(android.Manifest.permission.BLUETOOTH_SCAN,
                  false));
          if(!scanGranted) {
            MedDevLog.error("Bluetooth Permission", "BLE scan permission denied. Cannot proceed with Bluetooth connection.");
          }
          final boolean connectGranted = Boolean.TRUE
              .equals(result.getOrDefault(android.Manifest.permission.BLUETOOTH_CONNECT,
                  false));
          if(!connectGranted) {
            MedDevLog.error("Bluetooth Permission", "BLE connect permission denied. Cannot proceed with Bluetooth connection.");
          }
          final boolean legacyBluetooth = Boolean.TRUE
              .equals(result.getOrDefault(android.Manifest.permission.BLUETOOTH, false));
          if(!legacyBluetooth) {
            MedDevLog.warn("Bluetooth Permission", "Legacy Bluetooth permission denied. May affect compatibility with older Android versions.");
          }
          final boolean legacyAdmin = Boolean.TRUE
              .equals(result.getOrDefault(android.Manifest.permission.BLUETOOTH_ADMIN,
                  false));
          if(!legacyAdmin) {
            MedDevLog.warn("Bluetooth Permission", "Legacy Bluetooth Admin permission denied. May affect compatibility with older Android versions.");
          }
          final boolean fineLocation = Boolean.TRUE
              .equals(result.getOrDefault(android.Manifest.permission.ACCESS_FINE_LOCATION,
                  false));
          if(!fineLocation) {
            MedDevLog.error("Bluetooth Permission", "Fine Location permission denied. May affect BLE scanning on older Android versions.");
          }

          final boolean granted;
          granted = scanGranted && connectGranted;

          if (granted) {
            // Start BLE connection after permissions are granted
            mainViewModel.getControllerCommunicationManager().connectController();
          } else {
            // Inform user and decide whether to re-request or direct to settings
            android.widget.Toast.makeText(this, "BLE permissions denied",
                android.widget.Toast.LENGTH_SHORT).show();
            final boolean canReRequest = canReRequestBlePermissions();
            if (!canReRequest) {
              new MaterialAlertDialogBuilder(this)
                  .setTitle("Permissions required")
                  .setMessage(
                      "Bluetooth permissions are required. Please enable them in Settings → Apps → HydroGuide → Permissions.")
                  .setPositiveButton("Open Settings", (d, w) -> {
                    Intent intent = new Intent(
                        android.provider.Settings.ACTION_APPLICATION_DETAILS_SETTINGS,
                        Uri.fromParts("package", getPackageName(), null));
                    intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    startActivity(intent);
                  })
                  .setNegativeButton("Cancel", null)
                  .show();
            }
          }
        });
  }

  /**
   * Show dialog to choose mode of connection and initiate connection
   */
  public void showConnectionDialog() {
    final boolean isConnected = mainViewModel.getControllerCommunicationManager().checkConnection();
    final String[] hardwareOptions = isConnected
        ? new String[] { "USB", "Bluetooth", "Disconnect" }
        : new String[] { "USB", "Bluetooth" };
    new MaterialAlertDialogBuilder(this)
        .setTitle("Select Hardware Connection")
        .setItems(hardwareOptions, (dialog, which) -> {
          if (which == 0) { // USB
            mainViewModel.getControllerCommunicationManager().configureHardware(false);
            mainViewModel.getControllerCommunicationManager().connectController();
          } else if (which == 1) { // Bluetooth
            android.content.SharedPreferences prefs = this.getSharedPreferences("bonded_device_prefs", Context.MODE_PRIVATE);
            String savedMac = prefs.getString("bonded_mac_address", null);
            if (savedMac != null)
            {
              android.bluetooth.BluetoothAdapter bluetoothAdapter = android.bluetooth.BluetoothAdapter.getDefaultAdapter();
              boolean isBonded = false;
              if (bluetoothAdapter != null) {
                if (ActivityCompat.checkSelfPermission(this,
                        Manifest.permission.BLUETOOTH_CONNECT) !=
                        PackageManager.PERMISSION_GRANTED) {
                  android.widget.Toast.makeText(this, "Bluetooth Permissions missing.",
                          android.widget.Toast.LENGTH_SHORT).show();
                  return;
                }
                for (android.bluetooth.BluetoothDevice bondedDevice : bluetoothAdapter.getBondedDevices()) {
                  if (bondedDevice.getAddress().equals(savedMac)) {
                    isBonded = true;
                    break;
                  }
                }
              }
              if (isBonded)
              {
                mainViewModel.getControllerCommunicationManager().configureHardware(true);
                mainViewModel.getControllerCommunicationManager().connectController(savedMac);
              }
              else {
                android.widget.Toast.makeText(this, "No bonded Bluetooth device found.",
                        android.widget.Toast.LENGTH_SHORT).show();
              }
            }
            else
              android.widget.Toast.makeText(this, "No bonded Bluetooth device found.", android.widget.Toast.LENGTH_SHORT).show();

          } else if (isConnected && which == 2) { // Disconnect
            try {
              mainViewModel.getControllerCommunicationManager().disconnectController();
              dismissPairingCodeDialog();
              dismissScanningProgressDialog();
            } catch (Exception e) {
              MedDevLog.error("MainActivity", "Disconnect failed", e);
            }
          }
        })
        .setCancelable(true)
        .show();
  }

  private boolean isBlePermissionsGranted() {
    final int scan = ContextCompat.checkSelfPermission(this, android.Manifest.permission.BLUETOOTH_SCAN);
    final int connect = ContextCompat.checkSelfPermission(this, android.Manifest.permission.BLUETOOTH_CONNECT);
    return scan == android.content.pm.PackageManager.PERMISSION_GRANTED &&
        connect == android.content.pm.PackageManager.PERMISSION_GRANTED;
  }

  private boolean canReRequestBlePermissions() {
    final boolean scanRationale = ActivityCompat.shouldShowRequestPermissionRationale(this,
        android.Manifest.permission.BLUETOOTH_SCAN);
    final boolean connectRationale = ActivityCompat.shouldShowRequestPermissionRationale(this,
        android.Manifest.permission.BLUETOOTH_CONNECT);
    return scanRationale || connectRationale;
  }

  private void requestBlePermissions() {
    if (isBlePermissionsGranted()) {
      mainViewModel.getControllerCommunicationManager().connectController();
      return;
    }
    blePermissionLauncher.launch(new String[] {
        android.Manifest.permission.BLUETOOTH_SCAN,
        android.Manifest.permission.BLUETOOTH_CONNECT
    });
  }

  private void loadFacilities() {
    final File facilitiesFile = new File(this.getExternalFilesDir(null), DataFiles.FACILITIES_DATA);

    // TODO: Create file if it does not exist.
    try {
      if (facilitiesFile.exists() && facilitiesFile.isFile()) {
        final BufferedReader facilitiesBr = new BufferedReader(
            new InputStreamReader(new FileInputStream(facilitiesFile), StandardCharsets.UTF_8));

        final String facilStr = facilitiesBr.readLine();
        if (facilStr != null) {
          final JSONArray jsonFacilities = new JSONArray(facilStr);
          final Facility[] savedFacilities = new Facility[jsonFacilities.length()];
          for (int i = 0; i < savedFacilities.length; i++) {
            savedFacilities[i] = new Facility(jsonFacilities.getString(i));
          }
          facilitiesBr.close();
          mainViewModel.getProfileState().setFacilityList(savedFacilities);
        }
      } else {
        final boolean fileCreated = facilitiesFile.createNewFile();

        if (fileCreated) {
          // Write an empty array if there is an error reading an empty file
        }

      }
    } catch (final IOException | JSONException e) {
      throw new RuntimeException(e);
    }
  }

  private AlertDialog pairingCodeDialog = null;
  private AlertDialog scanningProgressDialog = null;

  public void showScanningProgressDialog() {
    Log.i("ProgressBar", "Progress bar started");
    runOnUiThread(() -> {
      if (scanningProgressDialog != null && scanningProgressDialog.isShowing()) {
        return; // Already showing
      }

      // Create progress view with proper layout parameters
      LinearLayout progressLayout = new LinearLayout(this);
      progressLayout.setOrientation(LinearLayout.HORIZONTAL);
      progressLayout.setPadding(60, 60, 60, 60);
      progressLayout.setGravity(android.view.Gravity.CENTER);

      // Create progress bar with explicit size
      android.widget.ProgressBar progressBar = new android.widget.ProgressBar(this);
      progressBar.setIndeterminate(true);
      LinearLayout.LayoutParams progressParams = new LinearLayout.LayoutParams(
          100, 100 // width, height in pixels
      );
      progressBar.setLayoutParams(progressParams);
      progressLayout.addView(progressBar);

      // Create text view
      TextView textView = new TextView(this);
      textView.setText("  Searching for device...");
      textView.setTextSize(18);
      textView.setPadding(40, 0, 0, 0);
      textView.setTextColor(getResources().getColor(android.R.color.black, null));
      progressLayout.addView(textView);

      // Build and show dialog
      scanningProgressDialog = new MaterialAlertDialogBuilder(this)
          .setView(progressLayout)
          .setCancelable(false)
          .create();

      scanningProgressDialog.show();

      Log.i("MainActivity", "Scanning progress dialog shown");
    });
  }

  public void dismissScanningProgressDialog() {
    runOnUiThread(() -> {
      if (scanningProgressDialog != null && scanningProgressDialog.isShowing()) {
        scanningProgressDialog.dismiss();
        scanningProgressDialog = null;
        Log.i("MainActivity", "Scanning progress dialog dismissed");
      }
    });
  }
  public void showPairingCodeDialog(String mappedDirections) {
    String[] directions = mappedDirections.split(",");
    // Create the custom view for the code
    PairingCodeView codeView = new PairingCodeView(this, directions);

    // Create a container layout for message + code
    LinearLayout container = new LinearLayout(this);
    container.setOrientation(LinearLayout.VERTICAL);
    container.setPadding(32, 32, 32, 32);

    // Add instruction text
    TextView instruction = new TextView(this);
    instruction.setText("Enter this code on your controller:");
    instruction.setTextSize(18);
    instruction.setPadding(0, 0, 0, 24);
    container.addView(instruction);

    // Add the code view
    container.addView(codeView);

    pairingCodeDialog = new MaterialAlertDialogBuilder(this)
        .setTitle("Pairing Code")
        .setView(container)
        .setPositiveButton("OK", null)
        .show();
  }

  public void dismissPairingCodeDialog() {
    if (pairingCodeDialog != null && pairingCodeDialog.isShowing()) {
      pairingCodeDialog.dismiss();
      pairingCodeDialog = null;
    }
  }

  @Override
  protected void onNewIntent(final Intent intent) {
    super.onNewIntent(intent);
    if (UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(intent.getAction())) {
      UsbDevice usbDevice = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE,UsbDevice.class);
      if(usbDevice != null && usbDevice.getVendorId() == 0x1dbf)
      {       
        Log.d("MainActivity", "Ultrasound probe attached via onNewIntent, SDK will handle connection");
        // Let SDK handle the connection internally
      }else {
        mainViewModel.getControllerCommunicationManager().notifyUSBDeviceAttach();
      }
    }
  }

  private boolean sdkInitialized = false;

  private void initializeUltrasoundSDK() {
    if (sdkInitialized) {
      Log.d("MainActivity", "GlobalUltrasoundManager already initialized, skipping");
      return;
    }

    try {
      GlobalUltrasoundManager.getInstance().init(this, new GlobalUltrasoundManager.ErrorCallback() {
        @Override
        public void onThorError(ThorError error) {
          MedDevLog.error("MainActivity", "ThorError received: " + error);
        }

        @Override
        public void onDauException(DauException dauException) {
          MedDevLog.error("MainActivity", "DauException received: " + dauException);
        }

        @Override
        public void onUPSError() {
          MedDevLog.error("MainActivity", "onUPSError received");
        }

        @Override
        public void onViewerError(ThorError error) {
          MedDevLog.error("MainActivity", "onViewerError received: " + error);
        }

        @Override
        public void onDauOpenError(ThorError error) {
          MedDevLog.error("MainActivity", "onDauOpenError received: " + error);
        }
      });
      sdkInitialized = true;
      Log.d("MainActivity", "GlobalUltrasoundManager initialized successfully");
    } catch (Exception ex) {
      MedDevLog.error("MainActivity", "Failed to initialize GlobalUltrasoundManager", ex);
    }
  }

  @Override
  protected void onStop() {
    super.onStop();
    // Send stop ECG as early as possible when app goes to background
    try {
      if (mainViewModel != null &&
          mainViewModel.getControllerCommunicationManager() != null &&
          mainViewModel.getControllerCommunicationManager().checkConnection()) {
        Log.i("MainActivity", "Stopping ECG on stop (app background/closing)");
        mainViewModel.getControllerCommunicationManager().sendECGCommand(false);
        mainViewModel.getControllerCommunicationManager().disconnectController();
      }
    } catch (Exception e) {
      MedDevLog.error("MainActivity", "Failed to stop ECG on stop", e);
    }
  }

  @Override
  protected void onDestroy() {
    try {
      this.unregisterReceiver(mUsbReceiver);
      this.unregisterReceiver(mDauConnectionReceiver);
      this.unregisterReceiver(mBatteryReceiver);
    } catch (IllegalArgumentException e) {
      MedDevLog.error("MainActivity", "Receiver not registered", e);
    } catch (Exception e) {
      MedDevLog.error("MainActivity", "Unregistering receiver failed", e);
    }
    // Ensure ECG streaming stops if app is closing and controller is connected
    try {
      if (mainViewModel != null &&
          mainViewModel.getControllerCommunicationManager() != null &&
          mainViewModel.getControllerCommunicationManager().checkConnection()) {
        // Stop ECG streaming first, then disconnect the controller
        MedDevLog.info("MainActivity", "Stopping ECG and disconnecting controller on destroy");
        mainViewModel.getControllerCommunicationManager().sendECGCommand(false);
        mainViewModel.getControllerCommunicationManager().disconnectController();
      }
    } catch (Exception e) {
      MedDevLog.error("MainActivity", "Failed to stop ECG/close controller on destroy", e);
    }
    memoryPoll.cleanUp();
    super.onDestroy();
  }
}