package com.accessvascularinc.hydroguide.mma.ui.settings;

import android.Manifest;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.NetworkCapabilities;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiNetworkSpecifier;
import android.net.wifi.WifiNetworkSuggestion;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Switch;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresPermission;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.databinding.FragmentSettingBinding;
import com.accessvascularinc.hydroguide.mma.ui.ConfirmDialog;

import java.util.ArrayList;
import java.util.List;

public class SettingsFragment extends Fragment {

  private FragmentSettingBinding binding ;
  private static final int REQUEST_LOCATION_PERMISSION = 1001;
  private WifiManager wifiManager;
  private RecyclerView recyclerViewWifiNetworks;
  private WifiNetworksAdapter wifiNetworksAdapter;
  private String connectedSsid = null;
  private Switch switchWifi;
  private Switch switchBluetooth;


  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
    binding = FragmentSettingBinding.inflate(inflater, container, false);
    View view = binding.getRoot();
    switchWifi = binding.switchWifi;
    recyclerViewWifiNetworks = binding.recyclerViewWifiNetworks;
    recyclerViewWifiNetworks.setLayoutManager(new LinearLayoutManager(getContext()));
    wifiNetworksAdapter = new WifiNetworksAdapter(new ArrayList<>());
    recyclerViewWifiNetworks.setAdapter(wifiNetworksAdapter);
    wifiNetworksAdapter.setOnWifiNetworkClickListener(this::onWifiNetworkSelected);
    switchBluetooth = binding.switchBluetooth;

    wifiManager = (WifiManager) getContext().getApplicationContext().getSystemService(Context.WIFI_SERVICE);
    // Check Wi-Fi state and connected network on fragment start
    boolean isWifiEnabled = wifiManager.isWifiEnabled();
    switchWifi.setChecked(isWifiEnabled);
    if (isWifiEnabled) {
      // Get currently connected SSID
      String currentSsid = null;
      android.net.wifi.WifiInfo wifiInfo = wifiManager.getConnectionInfo();
      if (wifiInfo != null) {
        String ssid = wifiInfo.getSSID();
        if (ssid != null && !ssid.equals("<unknown ssid>") && !ssid.isEmpty()) {
          // Remove quotes if present
          if (ssid.startsWith("\"") && ssid.endsWith("\"")) {
            ssid = ssid.substring(1, ssid.length() - 1);
          }
          currentSsid = ssid;
        }
      }
      connectedSsid = currentSsid;
      scanWifiNetworks();
      recyclerViewWifiNetworks.setVisibility(View.VISIBLE);
    } else {
      recyclerViewWifiNetworks.setVisibility(View.GONE);
    }
    binding.homeBtn.setOnClickListener(v -> {
      androidx.navigation.NavController navController = androidx.navigation.Navigation
              .findNavController(requireView());
      navController.navigate(R.id.action_navigation_settings_to_home);

    });
    // Add click listener for About button to show license dialog
    binding.aboutBtn.setOnClickListener(v -> showLicenseDialog());

    switchWifi.setOnCheckedChangeListener((buttonView, isChecked) -> {
      if (isChecked) {
        requestLocationPermissionAndScan();
      } else {
        // Try to turn off Wi-Fi (works on Android 9 and below)
        wifiManager.setWifiEnabled(false);
        recyclerViewWifiNetworks.setVisibility(View.GONE);
        showWifiDisableDialog();

      }
    });

    android.bluetooth.BluetoothAdapter bluetoothAdapter = android.bluetooth.BluetoothAdapter.getDefaultAdapter();
    if (bluetoothAdapter != null) {
      switchBluetooth.setChecked(bluetoothAdapter.isEnabled());
      switchBluetooth.setOnCheckedChangeListener((buttonView, isChecked) -> {
        if (isChecked) {
          // Enable Bluetooth
          if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.S) {
            if (ContextCompat.checkSelfPermission(requireContext(), android.Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
              requestPermissions(new String[]{android.Manifest.permission.BLUETOOTH_CONNECT}, 2001);
              switchBluetooth.setChecked(false);
              return;
            }
          }
          if (!bluetoothAdapter.isEnabled()) {
            Intent enableBtIntent = new Intent(android.bluetooth.BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enableBtIntent, 2002);
          }
        } else {
          // Disable Bluetooth: show dialog to instruct user
          if (bluetoothAdapter.isEnabled()) {
            showBluetoothDisableDialog();
          }
        }
      });
    } else {
      switchBluetooth.setEnabled(false); // Device does not support Bluetooth
    }
    return view;
  }

  private void showWifiDisableDialog() {
    Context context = getContext();
    if (context == null) return;
    new android.app.AlertDialog.Builder(context)
            .setTitle("Disable Wi-Fi")
            .setMessage("Due to Android 10+ restrictions, apps cannot turn off Wi-Fi directly. Please disable Wi-Fi manually in system settings.")
            .setPositiveButton("Open Wi-Fi Settings", (dialog, which) -> {
              dialog.dismiss();
              try {
                context.startActivity(new android.content.Intent(android.provider.Settings.ACTION_WIFI_SETTINGS));
              } catch (Exception e) {
                showConnectionStatus("Unable to open Wi-Fi settings.");
              }
            })
            .setNegativeButton("Cancel", (dialog, which) -> {
              switchWifi.setChecked(wifiManager.isWifiEnabled());
            })
            .show();
  }
  // Show custom password dialog for secured Wi-Fi
  private void showWifiPasswordDialog(ScanResult scanResult) {
    Context context = getContext();
    if (context == null) return;
    View dialogView = LayoutInflater.from(context).inflate(R.layout.dialog_wifi_password, null);
    android.widget.EditText editTextPassword = dialogView.findViewById(R.id.editTextPassword);
    android.widget.ImageButton buttonCancel = dialogView.findViewById(R.id.buttonCancel);
    android.widget.ImageButton buttonConnect = dialogView.findViewById(R.id.buttonConnect);

    android.app.AlertDialog dialog = new android.app.AlertDialog.Builder(context, R.style.Theme_HydroGuideApp)
            .setView(dialogView)
            .setCancelable(false)
            .create();

    buttonCancel.setOnClickListener(v -> dialog.dismiss());
    buttonConnect.setOnClickListener(v -> {
      String password = editTextPassword.getText().toString();
      dialog.dismiss();
      connectToWifi(scanResult, password);
    });

    dialog.show();
    // Make dialog appear as a popup (not fullscreen)
    if (dialog.getWindow() != null) {
      dialog.getWindow().setLayout(
              android.view.ViewGroup.LayoutParams.WRAP_CONTENT,
              android.view.ViewGroup.LayoutParams.WRAP_CONTENT
      );
    }
  }

  @Override
  public void onResume() {
    super.onResume();
    if (wifiManager != null && switchWifi != null && recyclerViewWifiNetworks != null) {
      updateWifiUI();
    }

    android.bluetooth.BluetoothAdapter bluetoothAdapter = android.bluetooth.BluetoothAdapter.getDefaultAdapter();
    if (switchBluetooth != null && bluetoothAdapter != null) {
      switchBluetooth.setChecked(bluetoothAdapter.isEnabled());
      // If switch is ON but Bluetooth is OFF, launch intent again
      if (switchBluetooth.isChecked() && !bluetoothAdapter.isEnabled()) {
        Intent enableBtIntent = new Intent(android.bluetooth.BluetoothAdapter.ACTION_REQUEST_ENABLE);
        startActivityForResult(enableBtIntent, 2002);
      }
    }
  }

  // Show dialog to instruct user to disable Bluetooth manually
  private void showBluetoothDisableDialog() {
    Context context = getContext();
    if (context == null) return;
    new android.app.AlertDialog.Builder(context)
            .setTitle("Turn Off Bluetooth")
            .setMessage("Android does not allow apps to turn off Bluetooth directly.\n\nTo turn off Bluetooth, please:\n1. Tap 'Open Bluetooth Settings' below.\n2. In the system Bluetooth screen, switch Bluetooth OFF manually.\n\nIf you do not see the option, please use your device's quick settings or system menu to turn off Bluetooth.")
            .setPositiveButton("Open Bluetooth Settings", (dialog, which) -> {
              dialog.dismiss();
              try {
                context.startActivity(new android.content.Intent(android.provider.Settings.ACTION_BLUETOOTH_SETTINGS));
              } catch (Exception e) {
                android.widget.Toast.makeText(context, "Unable to open Bluetooth settings.", android.widget.Toast.LENGTH_SHORT).show();
              }
            })
            .setNegativeButton("Cancel", (dialog, which) -> {
              switchBluetooth.setChecked(true);
            })
            .show();
  }

  // Helper to update Wi-Fi UI state
  private void updateWifiUI() {
    boolean isWifiEnabled = wifiManager.isWifiEnabled();
    switchWifi.setChecked(isWifiEnabled);
    if (isWifiEnabled) {
      // Get currently connected SSID
      String currentSsid = null;
      android.net.wifi.WifiInfo wifiInfo = wifiManager.getConnectionInfo();
      if (wifiInfo != null) {
        String ssid = wifiInfo.getSSID();
        if (ssid != null && !ssid.equals("<unknown ssid>") && !ssid.isEmpty()) {
          // Remove quotes if present
          if (ssid.startsWith("\"") && ssid.endsWith("\"")) {
            ssid = ssid.substring(1, ssid.length() - 1);
          }
          currentSsid = ssid;
        }
      }
      connectedSsid = currentSsid;
      scanWifiNetworks();
      recyclerViewWifiNetworks.setVisibility(View.VISIBLE);
    } else {
      // Wi-Fi is off: clear network list, connected SSID, and hide RecyclerView
      connectedSsid = null;
      wifiNetworksAdapter.updateNetworks(new ArrayList<>(), null);
      recyclerViewWifiNetworks.setVisibility(View.GONE);
    }
  }

  // Connect to Wi-Fi (open or secured)
  // Connect to Wi-Fi (open or secured)
  private void connectToWifi_Old(ScanResult scanResult, String password) {
    Context context = getContext();
    if (context == null) return;
    // Diagnostic logging for ScanResult and connection parameters
    final String ssid = scanResult.getWifiSsid() != null ? scanResult.getWifiSsid().toString() : "";
    if (ssid == null || ssid.isEmpty()) {
      showConnectionStatus("SSID is empty. Cannot connect.");
      return;
    }
    if (scanResult.capabilities != null && (scanResult.capabilities.contains("WEP") || scanResult.capabilities.contains("WPA"))) {
      if (password == null || password.length() < 8) {
        showConnectionStatus("Password must be at least 8 characters for WPA/WPA2.");
        return;
      }
    }
    android.net.wifi.WifiNetworkSpecifier.Builder builder = new android.net.wifi.WifiNetworkSpecifier.Builder()
            .setSsid(scanResult.SSID)
            .setIsHiddenSsid(false);
    String capabilities = scanResult.capabilities;
    if (capabilities != null) {
      if (capabilities.contains("WPA2")) {
        builder.setWpa2Passphrase(password);
      } else if (capabilities.contains("WPA")) {
        builder.setWpa2Passphrase(password);
      } else if (capabilities.contains("WEP")) {
        showConnectionStatus("WEP networks are not supported for programmatic connection on Android 10+");
        return;
      } else {
      }
    }
    android.net.wifi.WifiNetworkSpecifier wifiNetworkSpecifier = builder.build();
    android.net.NetworkRequest networkRequest = new android.net.NetworkRequest.Builder()
            .addTransportType(android.net.NetworkCapabilities.TRANSPORT_WIFI)
            .setNetworkSpecifier(wifiNetworkSpecifier)
            .build();
    android.net.ConnectivityManager connectivityManager = (android.net.ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);

    connectivityManager.requestNetwork(networkRequest,
            new android.net.ConnectivityManager.NetworkCallback() {
              @Override
              public void onAvailable(android.net.Network network) {
                android.util.Log.d("WIFI", "NetworkCallback: onAvailable for " + ssid);
                showConnectionStatus("Connected to " + ssid);
                connectedSsid = ssid;
                if (getActivity() != null) {
                  getActivity().runOnUiThread(() -> {
                    recyclerViewWifiNetworks.setVisibility(View.GONE);
                  });
                }
              }

              @Override
              public void onUnavailable() {
                android.util.Log.d("WIFI", "NetworkCallback: onUnavailable for " + ssid);
                showConnectionStatus("Failed to connect: " + ssid );
                // showWifiConnectionErrorDialog(scanResult.SSID);
              }

              @Override
              public void onLost(android.net.Network network) {
                android.util.Log.d("WIFI", "NetworkCallback: onLost for " + ssid);
                showConnectionStatus("Lost connection to " + ssid);
              }

              @Override
              public void onCapabilitiesChanged(android.net.Network network, android.net.NetworkCapabilities networkCapabilities) {
                android.util.Log.d("WIFI", "NetworkCallback: onCapabilitiesChanged for " + ssid + ", capabilities: " + networkCapabilities);
              }

              @Override
              public void onLinkPropertiesChanged(android.net.Network network, android.net.LinkProperties linkProperties) {
                android.util.Log.d("WIFI", "NetworkCallback: onLinkPropertiesChanged for " + ssid + ", linkProperties: " + linkProperties);
              }
            });
    showConnectionStatus("Connecting to " + scanResult.SSID);
  }

  private void connectToWifi(ScanResult scanResult, String password) {
    Context context = getContext();
    if (context == null) return;
    // Diagnostic logging for ScanResult and connection parameters
    final String ssid = scanResult.getWifiSsid() != null ? scanResult.getWifiSsid().toString() : "";
    if (ssid == null || ssid.isEmpty()) {
      showConnectionStatus("SSID is empty. Cannot connect.");
      return;
    }
    if (scanResult.capabilities != null && (scanResult.capabilities.contains("WEP") || scanResult.capabilities.contains("WPA"))) {
      if (password == null || password.length() < 8) {
        showConnectionStatus("Password must be at least 8 characters for WPA/WPA2.");
        return;
      }
    }
        final WifiNetworkSuggestion suggestion1;
        String capabilities = scanResult.capabilities;
        if (capabilities == null || (!capabilities.contains("WEP") && !capabilities.contains("WPA"))) {
      // Open network, no password required
      suggestion1 = new WifiNetworkSuggestion.Builder()
        .setSsid(scanResult.SSID)
        .setIsHiddenSsid(true)
        .build();
        } else {
      // Secured network, password required
      suggestion1 = new WifiNetworkSuggestion.Builder()
        .setSsid(scanResult.SSID)
        .setWpa2Passphrase(password)
        .setIsHiddenSsid(true)
        .build();
        }

    final List<WifiNetworkSuggestion> suggestionsList =
            new ArrayList<WifiNetworkSuggestion>();
    suggestionsList.add(suggestion1);
    WifiManager wifiManager =
            (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
    int status = wifiManager.addNetworkSuggestions(suggestionsList);
    if (status == 0 ){
      showConnectionStatus("Connecting to " + scanResult.SSID);
      openWifiSettingsWithDialog(scanResult.SSID);
      Log.i("WIFI", "PSK network added: "+status);
    } else {
      showConnectionStatus("Failed Connecting to " + scanResult.SSID + ". Opening Wi-Fi settings...");
      Log.i("WIFI", "PSK network not added: "+status);
      // Fallback: open Wi-Fi settings so user can connect manually
      openWifiSettingsWithDialog(scanResult.SSID);
    }

  }

  // Fallback: open Wi-Fi settings and show dialog
  private void openWifiSettingsWithDialog(String ssid) {
    Context context = getContext();
    if (context == null) return;
    new android.app.AlertDialog.Builder(context)
            .setTitle("Manual Wi-Fi Connection Required")
            .setMessage("Could not connect to '" + ssid + "' automatically. Please connect manually in system Wi-Fi settings.")
            .setPositiveButton("Open Wi-Fi Settings", (dialog, which) -> {
              dialog.dismiss();
              try {
                context.startActivity(new android.content.Intent(android.provider.Settings.ACTION_WIFI_SETTINGS));
              } catch (Exception e) {
                showConnectionStatus("Unable to open Wi-Fi settings.");
              }
            })
            .setNegativeButton("Cancel", null)
            .show();
  }

  // Show connection status (simple toast for now)
  private void showConnectionStatus(String message) {
    android.widget.Toast.makeText(getContext(), message, android.widget.Toast.LENGTH_SHORT).show();
  }

  // Called when a Wi-Fi network is selected from the list
  @RequiresPermission(Manifest.permission.ACCESS_FINE_LOCATION)
  private void onWifiNetworkSelected(String networkName) {
    // Find the ScanResult for the selected SSID
    if (ActivityCompat.checkSelfPermission(getContext(),
            Manifest.permission.ACCESS_FINE_LOCATION) !=
            PackageManager.PERMISSION_GRANTED) {
      // TODO: Consider calling
      //    ActivityCompat#requestPermissions
      // here to request the missing permissions, and then overriding
      //   public void onRequestPermissionsResult(int requestCode, String[] permissions,
      //                                          int[] grantResults)
      // to handle the case where the user grants the permission. See the documentation
      // for ActivityCompat#requestPermissions for more details.
      return;
    }
    List<ScanResult> results = wifiManager.getScanResults();
    ScanResult selectedResult = null;
    for (ScanResult result : results) {
      if (result.SSID.equals(networkName)) {
        selectedResult = result;
        break;
      }
    }
    if (selectedResult == null) return;

    // Check if the network is open or secured
    String capabilities = selectedResult.capabilities;
    boolean isSecured = capabilities != null && (capabilities.contains("WEP") || capabilities.contains("WPA"));
    if (isSecured) {
      showWifiPasswordDialog(selectedResult);
    } else {
      connectToWifi(selectedResult, null);
    }
  }

  private void requestLocationPermissionAndScan() {
    List<String> permissionsNeeded = new ArrayList<>();
    if (ContextCompat.checkSelfPermission(getContext(), Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
      permissionsNeeded.add(Manifest.permission.ACCESS_FINE_LOCATION);
    }
    if (ContextCompat.checkSelfPermission(getContext(), Manifest.permission.ACCESS_WIFI_STATE) != PackageManager.PERMISSION_GRANTED) {
      permissionsNeeded.add(Manifest.permission.ACCESS_WIFI_STATE);
    }
    if (!permissionsNeeded.isEmpty()) {
      requestPermissions(permissionsNeeded.toArray(new String[0]), REQUEST_LOCATION_PERMISSION);
    } else {
      scanWifiNetworks();
    }
  }

  @Override
  public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
    super.onRequestPermissionsResult(requestCode, permissions, grantResults);
    if (requestCode == REQUEST_LOCATION_PERMISSION) {
      if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
        scanWifiNetworks();
      }
    }
  }

  private void scanWifiNetworks() {
    if (!wifiManager.isWifiEnabled()) {
      wifiManager.setWifiEnabled(true);
    }
    try {
      wifiManager.startScan();
      if (ContextCompat.checkSelfPermission(getContext(), Manifest.permission.ACCESS_WIFI_STATE) != PackageManager.PERMISSION_GRANTED) {
        // Permission not granted, do not proceed
        return;
      }
      List<ScanResult> results = wifiManager.getScanResults();
      List<String> networkNames = new ArrayList<>();
      for (ScanResult result : results) {
        if (result.SSID != null && !result.SSID.isEmpty() && !networkNames.contains(result.SSID)) {
          networkNames.add(result.SSID);
        }
      }
      wifiNetworksAdapter.updateNetworks(networkNames, connectedSsid);
      recyclerViewWifiNetworks.setVisibility(View.VISIBLE);
    } catch (SecurityException e) {
      // Handle the case where permissions are missing
      recyclerViewWifiNetworks.setVisibility(View.GONE);
    }
  }

  // RecyclerView Adapter for Wi-Fi networks
  public static class WifiNetworksAdapter extends RecyclerView.Adapter<WifiNetworksAdapter.WifiViewHolder> {
    private List<String> networks;
    private String connectedSsid = null;
    private OnWifiNetworkClickListener listener;

    public interface OnWifiNetworkClickListener {
      void onWifiNetworkClick(String networkName);
    }

    public WifiNetworksAdapter(List<String> networks) {
      this.networks = networks;
    }

    public void setOnWifiNetworkClickListener(OnWifiNetworkClickListener listener) {
      this.listener = listener;
    }

    // Update networks and move connected SSID to top
    public void updateNetworks(List<String> newNetworks, String connectedSsid) {
      this.connectedSsid = connectedSsid;
      if (connectedSsid != null && newNetworks.contains(connectedSsid)) {
        List<String> sorted = new ArrayList<>();
        sorted.add(connectedSsid);
        for (String ssid : newNetworks) {
          if (!ssid.equals(connectedSsid)) sorted.add(ssid);
        }
        this.networks = sorted;
      } else {
        this.networks = newNetworks;
      }
      notifyDataSetChanged();
    }

    @NonNull
    @Override
    public WifiViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
      View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.item_wifi_network, parent, false);
      return new WifiViewHolder(view);
    }

    @Override
    public void onBindViewHolder(@NonNull WifiViewHolder holder, int position) {
      String ssid = networks.get(position);
      boolean isConnected = connectedSsid != null && connectedSsid.equals(ssid);
      holder.bind(ssid, isConnected, listener);
    }

    @Override
    public int getItemCount() {
      return networks.size();
    }

    static class WifiViewHolder extends RecyclerView.ViewHolder {
      private final android.widget.TextView textViewNetworkName;
      private final android.widget.ImageView imageViewWifiIcon;
      private final android.widget.TextView textViewConnected;
      private final android.widget.ImageView imageViewCheck;

      public WifiViewHolder(@NonNull View itemView) {
        super(itemView);
        textViewNetworkName = itemView.findViewById(R.id.textViewNetworkName);
        imageViewWifiIcon = itemView.findViewById(R.id.imageViewWifiIcon);
        textViewConnected = itemView.findViewById(R.id.textViewConnected);
        imageViewCheck = itemView.findViewById(R.id.imageViewCheck);
      }

      public void bind(String networkName, boolean isConnected, OnWifiNetworkClickListener listener) {
        textViewNetworkName.setText(networkName);
        if (isConnected) {
          textViewConnected.setVisibility(View.VISIBLE);
          imageViewCheck.setVisibility(View.VISIBLE);
        } else {
          textViewConnected.setVisibility(View.GONE);
          imageViewCheck.setVisibility(View.GONE);
        }
        itemView.setOnClickListener(v -> {
          if (listener != null) {
            listener.onWifiNetworkClick(networkName);
          }
        });
      }
    }
  }

  /**
   * Show license and copyright information dialog with dummy content.
   * This content will be replaced with actual content from AVI in a future ticket.
   */
  private void showLicenseDialog() {
    Context context = getContext();
    if (context == null) return;

    View dialogView = LayoutInflater.from(context).inflate(R.layout.dialog_license_info, null);
    
    // Get references to dialog views
    android.widget.TextView textViewDialogCopyright = dialogView.findViewById(R.id.textViewDialogCopyright);
    android.widget.TextView textViewDialogLicense = dialogView.findViewById(R.id.textViewDialogLicense);
    android.widget.TextView textViewDialogVersion = dialogView.findViewById(R.id.textViewDialogVersion);
    android.widget.ImageButton buttonClose = dialogView.findViewById(R.id.buttonCloseLicense);

    // Set content from strings.xml
    textViewDialogCopyright.setText(getString(R.string.copyright_text));
    textViewDialogLicense.setText(getString(R.string.lincense_text));

    // Get app version dynamically
    try {
      String versionName = requireContext().getPackageManager()
              .getPackageInfo(requireContext().getPackageName(), 0).versionName;
      int versionCode = requireContext().getPackageManager()
              .getPackageInfo(requireContext().getPackageName(), 0).versionCode;
      textViewDialogVersion.setText("Version " + versionName + " (Build " + versionCode + ")");
    } catch (PackageManager.NameNotFoundException e) {
      textViewDialogVersion.setText("Version 1.0.0");
    }


    // Create and show dialog
    android.app.AlertDialog dialog = new android.app.AlertDialog.Builder(context, R.style.Theme_HydroGuideApp)
            .setView(dialogView)
            .setCancelable(true)
            .create();

    buttonClose.setOnClickListener(v -> dialog.dismiss());

    dialog.show();

    // Make dialog appear as a popup (not fullscreen)
    if (dialog.getWindow() != null) {
      dialog.getWindow().setLayout(
              (int) (getResources().getDisplayMetrics().widthPixels * 0.8),
              android.view.ViewGroup.LayoutParams.WRAP_CONTENT
      );
    }
  }
}

