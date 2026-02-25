package com.accessvascularinc.hydroguide.mma.ui;

import static androidx.navigation.fragment.FragmentKt.findNavController;

import com.accessvascularinc.hydroguide.mma.model.Facility;
import com.accessvascularinc.hydroguide.mma.network.NetworkUtils;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.ContentObserver;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.MediaScannerConnection;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkRequest;
import android.net.Uri;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.Settings;
import android.util.Log;
import android.widget.Toast;

import androidx.annotation.IdRes;
import androidx.annotation.Nullable;
import javax.inject.Inject;
import dagger.hilt.android.AndroidEntryPoint;
import com.accessvascularinc.hydroguide.mma.repository.FacilitySyncRepository;
import com.accessvascularinc.hydroguide.mma.serial.ImpedanceSample;
import com.accessvascularinc.hydroguide.mma.serial.ButtonState;
import com.accessvascularinc.hydroguide.mma.storage.IStorageManager;
import com.accessvascularinc.hydroguide.mma.sync.SyncManager;

import android.view.View;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.view.animation.LinearInterpolator;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;

import com.accessvascularinc.hydroguide.mma.db.repository.FacilityRepository;

import androidx.activity.OnBackPressedCallback;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import androidx.constraintlayout.widget.ConstraintLayout;
import androidx.core.content.ContextCompat;
import androidx.databinding.Observable;
import androidx.databinding.library.baseAdapters.BR;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.ViewModelProvider;
import androidx.navigation.NavController;
import androidx.navigation.Navigation;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.model.AlarmEvent;
import com.accessvascularinc.hydroguide.mma.model.AlarmType;
import com.accessvascularinc.hydroguide.mma.model.DataFiles;
import com.accessvascularinc.hydroguide.mma.model.EncounterData;
import com.accessvascularinc.hydroguide.mma.model.EncounterState;
import com.accessvascularinc.hydroguide.mma.model.TabletState;
import com.accessvascularinc.hydroguide.mma.serial.Button;
import com.accessvascularinc.hydroguide.mma.serial.ControllerStatus;
import com.accessvascularinc.hydroguide.mma.serial.EcgSample;
import com.accessvascularinc.hydroguide.mma.serial.Heartbeat;
import com.accessvascularinc.hydroguide.mma.serial.Packet;
import com.accessvascularinc.hydroguide.mma.serial.PacketId;
import com.accessvascularinc.hydroguide.mma.serial.StartupStatus;
import com.accessvascularinc.hydroguide.mma.serial.flags.BitFlags;
import com.accessvascularinc.hydroguide.mma.serial.flags.ChargerStatusFlags;
import com.accessvascularinc.hydroguide.mma.serial.flags.ControllerStatusFlags;
import com.accessvascularinc.hydroguide.mma.serial.flags.FuelGaugeStatusFlags;
import com.accessvascularinc.hydroguide.mma.serial.flags.SamplingStatusFlags;
import com.accessvascularinc.hydroguide.mma.ui.input.ButtonInputListener;
import com.accessvascularinc.hydroguide.mma.ui.settings.BrightnessLookup;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import com.accessvascularinc.hydroguide.mma.utils.NetworkConnectivityLiveData;
import com.accessvascularinc.hydroguide.mma.utils.PrefsManager;
import com.accessvascularinc.hydroguide.mma.utils.Utility;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import org.json.JSONException;

import java.io.File;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Paths;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Objects;

@AndroidEntryPoint
public class BaseHydroGuideFragment extends Fragment implements ControllerListener,
    ButtonInputListener {
  private final String VOLUME_CHANGED_ACTION = "android.media.VOLUME_CHANGED_ACTION";

  protected MainViewModel viewmodel = null;
  @Inject
  protected FacilitySyncRepository facilitySyncRepository;
  @Inject
  protected SyncManager syncManager;
  @Inject
  protected FacilityRepository facilityRepository;
  @Inject
  protected IStorageManager storageManager;

  protected static class RestrictedEvent {
    public ImageButton graphicButton;
    public View.OnClickListener oldEvent;
    public RESTRICTION_TYPE restrictionType;

    public RestrictedEvent(final ImageButton button, final View.OnClickListener oldClickListener,
        final RESTRICTION_TYPE restrictionType) {
      graphicButton = button;
      oldEvent = oldClickListener;
      this.restrictionType = restrictionType;
    }
  }

  protected Map<AlarmType, Integer> alarmPriorityMap = new HashMap<>();

  protected AlarmEvent[] externalSuspendAlarms = null;
  protected AlarmEvent[] internalSuspendAlarms = null;
  protected AlarmEvent[] fullSuspendAlarms = null;

  protected enum RESTRICTED_EVENT_STATE {
    PROHIBITED, SILENCED, DEFAULT_BEHAVIOR
  }

  protected enum RESTRICTION_TYPE {
    LOW_POWER, CONTROLLER_STATUS_TIMEOUT, NO_PROFILE
  }

  protected ImageView ivBattery = null, ivUsbIcon = null;
  protected TextView tvBattery = null, tvTestUsbText = null; // Optional. Define in subclass to
  // display packet info.
  protected ImageView ivTabletBattery = null;
  protected TextView tvTabletBattery = null;
  protected ConstraintLayout lpTabletIcon = null;
  protected ImageView lpTabletSymbol = null;
  protected ImageView hgLogo = null;
  protected List<RestrictedEvent> restrictedEvents = new ArrayList<>();
  protected List<ImageView> lowBatteryIndicators = new ArrayList<>();
  protected List<ImageView> errorIndicators = new ArrayList<>();
  protected RESTRICTED_EVENT_STATE currentLPState = RESTRICTED_EVENT_STATE.DEFAULT_BEHAVIOR;
  protected RESTRICTED_EVENT_STATE currentCSTState = RESTRICTED_EVENT_STATE.DEFAULT_BEHAVIOR;
  protected BitFlags<ChargerStatusFlags> batterySubsysFault = null;
  private BitFlags<ControllerStatusFlags> controllerStatusFlag = null;
  private Boolean controllerLowPowerFlag = false;
  private Boolean tabletLowPowerFlag = false;
  private AlertDialog modalDialog = null;
  private MediaPlayer notificationPlayer = null;
  private AudioManager audioManager = null;
  private VolumeChangeListener volChangeListener = null;
  private ContentObserver brightnessObserver = null;
  private volatile Boolean transitionAllowed = true;
  private final Handler minimumUptimeHandler = new Handler(Looper.getMainLooper());
  private File logFile = null;

  public static boolean IS_SYNC_PERMITTED = true;

  protected final boolean[] triggeredEvents = new boolean[] { false, false, false, false, false,
      false, false, false };
  // Testing only: {lp controller, lp tablet, charger status, controller status,
  // ecg timeout,
  // charging during procedure}

  private ConnectivityManager connectivityManager;

  protected void setupAlarmArrays() {
    // Initializing Suspension Categories
    if (externalSuspendAlarms != null) {
      return;
    }

    externalSuspendAlarms = new AlarmEvent[] { new AlarmEvent(AlarmType.ExternalLeadOff,
            getInt(R.integer.lead_off_extern_priority)) };
    internalSuspendAlarms = new AlarmEvent[] { new AlarmEvent(AlarmType.InternalLeadOff,
            getInt(R.integer.lead_off_intrav_priority)) };
    fullSuspendAlarms = new AlarmEvent[] {
            new AlarmEvent(AlarmType.EcgDataMissing, getInt(R.integer.ecg_data_priority)),
            new AlarmEvent(AlarmType.ControllerStatusTimeout,
                    getInt(R.integer.ctrl_status_priority)),
            new AlarmEvent(AlarmType.BatterySubsystemFault,
                    getInt(R.integer.battery_subs_priority)),
            new AlarmEvent(AlarmType.FuelGaugeStatusFault, getInt(R.integer.fuel_gauge_priority)),
            new AlarmEvent(AlarmType.SupplyRangeFault, getInt(R.integer.hardware_fault_priority)),

    };
  }

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    viewmodel = new ViewModelProvider(requireActivity()).get(MainViewModel.class);
  }

  @Override
  public void onViewCreated(@NonNull final View view, @Nullable final Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);
    setupAlarmArrays();
    // Make sure subclasses define the viewmodel and UI components above in
    // OnCreateView().
    if (viewmodel != null && viewmodel.getControllerCommunicationManager() != null) {

      viewmodel.getControllerCommunicationManager().addControllerListener(this);
      viewmodel.getControllerCommunicationManager().addButtonInputListener(this);
      viewmodel.getTabletState().addOnPropertyChangedCallback(tabletBatteryChanged_Listener);
      viewmodel.getTabletState().addOnPropertyChangedCallback(tabletChargerStateChanged_Listener);
      viewmodel.getEncounterData().addOnPropertyChangedCallback(encounterStateChanged_Listener);

      // Conditionals
      if (viewmodel.getEncounterData().getState() == EncounterState.Uninitialized) {
        viewmodel.getEncounterData().addOnPropertyChangedCallback(encounterDataChanged_Listener);
        viewmodel.getEncounterData().getPatient()
            .addOnPropertyChangedCallback(patientDataChanged_Listener);
      }
    }

    // Initializing Alarm Priority HT
    alarmPriorityMap.put(AlarmType.EcgDataMissing, getInt(R.integer.ecg_data_priority));
    alarmPriorityMap.put(AlarmType.ControllerStatusTimeout, getInt(R.integer.ctrl_status_priority));
    alarmPriorityMap.put(AlarmType.BatterySubsystemFault, getInt(R.integer.battery_subs_priority));
    alarmPriorityMap.put(AlarmType.InternalLeadOff, getInt(R.integer.lead_off_intrav_priority));
    alarmPriorityMap.put(AlarmType.ExternalLeadOff, getInt(R.integer.lead_off_extern_priority));
    alarmPriorityMap.put(AlarmType.FuelGaugeStatusFault, getInt(R.integer.fuel_gauge_priority));
    alarmPriorityMap.put(AlarmType.SupplyRangeFault, getInt(R.integer.hardware_fault_priority));

    currentLPState = RESTRICTED_EVENT_STATE.DEFAULT_BEHAVIOR;
    currentCSTState = RESTRICTED_EVENT_STATE.DEFAULT_BEHAVIOR;
    controllerLowPowerFlag = false;
    tabletLowPowerFlag = false;

    toggleConnectionIcon(viewmodel.getControllerCommunicationManager().checkConnection());
    // toggleConnectionIcon(viewmodel.getBleControllerManager().checkConnection());
    notificationPlayer = MediaPlayer.create(requireContext(), R.raw.disconnect_alert);
    requireActivity().getOnBackPressedDispatcher().addCallback(backButtonPressed_Listener);
    updateBatteryDisplay();
    updateTabletBatteryDisplay();
    updateControllerTimeoutAlarm();
    updateEcgTimeoutAlarm();
    updateBatterySubsysAlarm();
    checkProcedureSuspension();

    // Set up click listener for USB icon to show connection dialog
    if (ivUsbIcon != null) {
      ivUsbIcon.setOnClickListener(v -> {
        if (getActivity() instanceof MainActivity) {
          ((MainActivity) getActivity()).showConnectionDialog();
        }
      });
    }
    // networkObserver();
    syncManager.setOnUnauthorizedListener(() -> {
      NavController navController = Navigation.findNavController(requireActivity(),
          R.id.nav_host_fragment_activity_main);
      navController.navigate(R.id.navigation_login);
      Toast.makeText(requireContext(), "Session expired. Please login.", Toast.LENGTH_LONG).show();
    });
  }

  // This is needed if volume and brightness are adjustable in the child fragment.
  protected void setupVolumeAndBrightnessListeners() {
    audioManager = (AudioManager) requireActivity().getSystemService(Context.AUDIO_SERVICE);

    volChangeListener = new VolumeChangeListener();
    requireContext().registerReceiver(volChangeListener, new IntentFilter(VOLUME_CHANGED_ACTION));

    final ContentResolver resolver = requireContext().getContentResolver();
    final TabletState tabletState = viewmodel.getTabletState();
    
    // Guard against null tabletState
    if (tabletState == null) {
      MedDevLog.error("BaseHydroGuideFragment", "TabletState is null in setupVolumeAndBrightnessListeners");
      return;
    }
    
    brightnessObserver = new ContentObserver(new Handler(Looper.getMainLooper())) {
      @Override
      public void onChange(final boolean selfChange) {
        try {
          // get system brightness level
          final int brightnessAmount = Settings.System.getInt(resolver, Settings.System.SCREEN_BRIGHTNESS, 0);
          final int brightnessPercent = (int) (BrightnessLookup.getBrightnessPercentage(brightnessAmount) * 100);
          tabletState.setScreenBrightness(brightnessPercent);
          Log.d("Brightness", "onChange: brightness=" + brightnessAmount + ", percent=" + brightnessPercent);
        } catch (Exception e) {
          MedDevLog.error("BaseHydroGuideFragment", "Error updating brightness in observer", e);
        }
      }
    };

    final Uri brightnessUri = Settings.System.getUriFor(Settings.System.SCREEN_BRIGHTNESS);
    // Use notifyForDescendants=true to catch all brightness changes
    resolver.registerContentObserver(brightnessUri, true, brightnessObserver);

    // Initialize with current brightness
    try {
      final int brightness = Settings.System.getInt(resolver, Settings.System.SCREEN_BRIGHTNESS, 0);
      final int brightnessPercent = (int) (BrightnessLookup.getBrightnessPercentage(brightness) * 100);
      tabletState.initTabletState(audioManager.getStreamVolume(AudioManager.STREAM_MUSIC),
          brightnessPercent);
      Log.d("Brightness", "initTabletState: brightness=" + brightness + ", percent=" + brightnessPercent);
    } catch (Exception e) {
      MedDevLog.error("BaseHydroGuideFragment", "Error initializing brightness", e);
    }
  }

  protected void setupNetworkSync(@NonNull final ImageView syncStatusIcon,
      @NonNull final androidx.lifecycle.LifecycleOwner lifecycleOwner,
      @NonNull final Context context) {
    final NetworkConnectivityLiveData networkConnectivityLiveData = new NetworkConnectivityLiveData(context);
    networkConnectivityLiveData.observe(lifecycleOwner, isConnected -> {
      syncStatusIcon
          .setImageResource(Boolean.TRUE.equals(isConnected) ? R.drawable.sync_available : R.drawable.sync_unavailable);
    });
    syncStatusIcon.setOnClickListener(v -> {
      Boolean isConnected = networkConnectivityLiveData.getValue();
      if (Boolean.TRUE.equals(isConnected)) {
        new com.google.android.material.dialog.MaterialAlertDialogBuilder(requireActivity())
            .setMessage(R.string.internet_connection_available_to_sync)
            .setPositiveButton(R.string.yes, (dialog, which) -> performSync())
            .setNegativeButton(R.string.no, (dialog, which) -> dialog.dismiss())
            .show();
      } else {
        new com.google.android.material.dialog.MaterialAlertDialogBuilder(requireActivity())
            .setMessage(R.string.internet_connection_unavailable_to_sync)
            .setPositiveButton(R.string.okay, (dialog, which) -> dialog.dismiss())
            .show();
      }
    });
  }

  protected void performSync() {
    if (!NetworkUtils.isNetworkAvailable(requireContext())) {
      Toast.makeText(requireContext(), "No internet connection", Toast.LENGTH_SHORT).show();
      return;
    }
    if (storageManager != null) {
      storageManager.syncAllData((success, errorMessage) -> {
        if (!success) {
          MedDevLog.error("BaseHydroGuideFragment", "Sync failed: " + errorMessage);
          Toast.makeText(requireContext(), "Sync failed: " + errorMessage, Toast.LENGTH_SHORT).show();
        } else {
          Log.d("BaseHydroGuideFragment", "Sync successful");
          Toast.makeText(requireContext(), "Sync successful", Toast.LENGTH_SHORT).show();
        }
      });
    } else {
      MedDevLog.error("BaseHydroGuideFragment", "IStorageManager injection failed (null)");
      Toast.makeText(requireContext(), "Sync unavailable", Toast.LENGTH_SHORT).show();
    }
  }

  @Override
  public void onDestroyView() {
    viewmodel.getControllerCommunicationManager().removeControllerListener(this);
    viewmodel.getControllerCommunicationManager().removeButtonInputListener(this);
    // viewmodel.getBleControllerManager().removeControllerListener(this);
    // viewmodel.getBleControllerManager().removeButtonInputListener(this);
    viewmodel.getTabletState().removeOnPropertyChangedCallback(tabletBatteryChanged_Listener);
    viewmodel.getTabletState().removeOnPropertyChangedCallback(tabletChargerStateChanged_Listener);
    viewmodel.getEncounterData().removeOnPropertyChangedCallback(encounterStateChanged_Listener);

    final EncounterData encounter = viewmodel.getEncounterData();
    if (encounter.getState() == EncounterState.Uninitialized) {
      encounter.removeOnPropertyChangedCallback(encounterDataChanged_Listener);
      encounter.getPatient().removeOnPropertyChangedCallback(patientDataChanged_Listener);
    }

    notificationPlayer.release();

    if (volChangeListener != null) {
      requireContext().unregisterReceiver(volChangeListener);
    }

    if (brightnessObserver != null) {
      requireContext().getContentResolver().unregisterContentObserver(brightnessObserver);
    }

    super.onDestroyView();
  }

  private void updateSerialDataFile(final String newData, final String filename) {
    // Do not create or update any files if procedure directory has not been set.
    if (viewmodel.getEncounterData().getDataDirPath() == null) {
      return;
    }

    final String serialFilePath = viewmodel.getEncounterData().getDataDirPath() + File.separator + filename;
    final File serialFile = new File(serialFilePath);

    // Either update if file already exists or create it if not.
    final boolean fileExists = serialFile.exists();
    try (final FileOutputStream fos = new FileOutputStream(serialFile, fileExists)) {
      fos.write(newData.getBytes(StandardCharsets.UTF_8));
      fos.write(System.lineSeparator().getBytes(StandardCharsets.UTF_8));
      fos.flush();

      final String alertMsg = filename + " was " + (fileExists ? "updated" : "created") + "successfully.";
      /*
       * FIXME: This does not update files on windows explorer when connected to a PC.
       * MediaScannerConnection.scanFile(requireContext(),
       * new String[] {serialFile.getAbsolutePath()}, new String[] {}, null);
       */

    } catch (final IOException e) {
      MedDevLog.error("BaseHydroGuideFragment", "Error writing serial data file", e);
      Toast.makeText(requireContext(), e.getMessage(), Toast.LENGTH_SHORT).show();
    }
  }

  private void releaseEvents(final RESTRICTION_TYPE releaseType) {
    if (restrictedEvents.isEmpty()) {
      return;
    }
    final boolean profileSelected = viewmodel.getProfileState().getSelectedProfile() != null;
    for (final RestrictedEvent evt : restrictedEvents) {
      if (evt.restrictionType == releaseType) {
        evt.graphicButton.setOnClickListener(
            profileSelected ? evt.oldEvent : actionDisabledNP_OnClickListener);
        if (profileSelected) {
          evt.graphicButton.setAlpha(1.0f);
        }
      }
    }
  }

  private void updateBatteryDisplay() {
    final float pct = Math.round(viewmodel.getControllerState().getBatteryLevelPct());
    if ((10 <= pct) && (pct < 20)) {
      ivBattery.setImageResource(R.drawable.indicator_battery_10);
    } else if ((20 <= pct) && (pct < 30)) {
      ivBattery.setImageResource(R.drawable.indicator_battery_20);
    } else if ((30 <= pct) && (pct < 40)) {
      ivBattery.setImageResource(R.drawable.indicator_battery_30);
    } else if ((40 <= pct) && (pct < 50)) {
      ivBattery.setImageResource(R.drawable.indicator_battery_40);
    } else if ((50 <= pct) && (pct < 60)) {
      ivBattery.setImageResource(R.drawable.indicator_battery_50);
    } else if ((60 <= pct) && (pct < 70)) {
      ivBattery.setImageResource(R.drawable.indicator_battery_60);
    } else if ((70 <= pct) && (pct < 80)) {
      ivBattery.setImageResource(R.drawable.indicator_battery_70);
    } else if ((80 <= pct) && (pct < 90)) {
      ivBattery.setImageResource(R.drawable.indicator_battery_80);
    } else if ((90 <= pct) && (pct < 100)) {
      ivBattery.setImageResource(R.drawable.indicator_battery_90);
    } else if (pct >= 100) {
      ivBattery.setImageResource(R.drawable.indicator_battery_100);
    } else {
      ivBattery.setImageResource(R.drawable.indicator_battery_00);
    }

    if ((0 <= pct) && (pct < 40)) {
      tvBattery.setTextColor(ContextCompat.getColor(requireContext(), R.color.battery_red));
    } else if ((40 <= pct) && (pct < 70)) {
      tvBattery.setTextColor(ContextCompat.getColor(requireContext(), R.color.battery_yellow));
    } else if (pct >= 70) {
      tvBattery.setTextColor(ContextCompat.getColor(requireContext(), R.color.battery_green));
    } else {
      tvBattery.setTextColor(ContextCompat.getColor(requireContext(), R.color.white));
    }
    updateLowPowerAlarm();
  }

  private void updateTabletBatteryDisplay() {
    if (ivTabletBattery == null || tvTabletBattery == null) {
      return;
    }
    final int pct = viewmodel.getTabletState().getTabletChargePct();
    if ((10 <= pct) && (pct < 20)) {
      ivTabletBattery.setImageResource(R.drawable.indicator_battery_10);
    } else if ((20 <= pct) && (pct < 30)) {
      ivTabletBattery.setImageResource(R.drawable.indicator_battery_20);
    } else if ((30 <= pct) && (pct < 40)) {
      ivTabletBattery.setImageResource(R.drawable.indicator_battery_30);
    } else if ((40 <= pct) && (pct < 50)) {
      ivTabletBattery.setImageResource(R.drawable.indicator_battery_40);
    } else if ((50 <= pct) && (pct < 60)) {
      ivTabletBattery.setImageResource(R.drawable.indicator_battery_50);
    } else if ((60 <= pct) && (pct < 70)) {
      ivTabletBattery.setImageResource(R.drawable.indicator_battery_60);
    } else if ((70 <= pct) && (pct < 80)) {
      ivTabletBattery.setImageResource(R.drawable.indicator_battery_70);
    } else if ((80 <= pct) && (pct < 90)) {
      ivTabletBattery.setImageResource(R.drawable.indicator_battery_80);
    } else if ((90 <= pct) && (pct < 100)) {
      ivTabletBattery.setImageResource(R.drawable.indicator_battery_90);
    } else if (pct >= 100) {
      ivTabletBattery.setImageResource(R.drawable.indicator_battery_100);
    } else {
      ivTabletBattery.setImageResource(R.drawable.indicator_battery_00);
    }

    if ((0 <= pct) && (pct < 40)) {
      tvTabletBattery.setTextColor(ContextCompat.getColor(requireContext(), R.color.battery_red));
    } else if ((40 <= pct) && (pct < 70)) {
      tvTabletBattery.setTextColor(ContextCompat.getColor(requireContext(), R.color.battery_yellow));
    } else if (pct >= 70) {
      tvTabletBattery.setTextColor(ContextCompat.getColor(requireContext(), R.color.battery_green));
    } else {
      tvTabletBattery.setTextColor(ContextCompat.getColor(requireContext(), R.color.white));
    }
  }

  private void updateLowPowerAlarm() {
    int batLevel = viewmodel.getTabletState().getTabletChargePct(); // used to be direct,
    if (triggeredEvents[1]) {
      batLevel = 1;
    }
    // now uses viewmodel
    final boolean tabletBatteryLow = (batLevel >= 0 && batLevel <= 10);

    // if (triggeredEvents[0]) {
    // controllerStatusFlag = ControllerStatusFlags.BatteryLow;
    // }

    checkLPRestrictedEvents(ControllerStatusFlags.BatteryLow.isFlaggedIn(controllerStatusFlag)
        || tabletBatteryLow);

    if (ControllerStatusFlags.BatteryLow.isFlaggedIn(controllerStatusFlag)) {
      if (!controllerLowPowerFlag) {
        MedDevLog.warn("Alarm", "Controller low power alarm activated");
        for (final ImageView icon : lowBatteryIndicators) {
          icon.setVisibility(View.VISIBLE);
        }
        if (ivBattery != null) {
          ivBattery.setVisibility(View.VISIBLE);
          toggleBlinkingAnimation(ivBattery, true);
        }
      }
      controllerLowPowerFlag = true;
    } else if (controllerLowPowerFlag) {
      MedDevLog.warn("Alarm", "Controller low power alarm resolved");
      for (final ImageView icon : lowBatteryIndicators) {
        icon.setVisibility(View.GONE);
      }
      if (ivBattery != null) {
        toggleBlinkingAnimation(ivBattery, false);
        ivBattery.setVisibility(View.GONE);
      }
      controllerLowPowerFlag = false;
    }

    if (tabletBatteryLow) {
      if (!tabletLowPowerFlag) {
        MedDevLog.warn("Alarm", "Tablet low power alarm activated");
        if (lpTabletIcon != null) {
          lpTabletIcon.setVisibility(View.VISIBLE);
        }
        if (hgLogo != null) {
          hgLogo.setVisibility(View.GONE);
        }
        for (final ImageView icon : lowBatteryIndicators) {
          icon.setVisibility(View.VISIBLE);
        }
        if (lpTabletSymbol != null) {
          toggleBlinkingAnimation(lpTabletSymbol, true);
        }
        tabletLowPowerFlag = true;
      }
    } else if (tabletLowPowerFlag) {
      MedDevLog.warn("Alarm", "Tablet low power alarm resolved");
      if (lpTabletIcon != null) {
        lpTabletIcon.setVisibility(View.GONE);
      }
      if (hgLogo != null) {
        hgLogo.setVisibility(View.VISIBLE);
      }
      for (final ImageView icon : lowBatteryIndicators) {
        icon.setVisibility(View.GONE);
      }
      if (lpTabletSymbol != null) {
        toggleBlinkingAnimation(lpTabletSymbol, false);
      }
      tabletLowPowerFlag = false;
    }
  }

  private void checkLPRestrictedEvents(final boolean lowBattery) {
    if (lowBattery) {
      if (restrictedEvents != null && currentLPState == RESTRICTED_EVENT_STATE.DEFAULT_BEHAVIOR) {
        MedDevLog.warn("Navigation", "Restricted low power events");
        for (final RestrictedEvent evt : restrictedEvents) {
          evt.graphicButton.setOnClickListener(actionDisabledLP_OnClickListener);
          evt.graphicButton.setAlpha(0.5f);
        }
        currentLPState = RESTRICTED_EVENT_STATE.PROHIBITED;
      }
    } else {
      if (currentLPState == RESTRICTED_EVENT_STATE.PROHIBITED) {
        MedDevLog.warn("Navigation", "Unrestricted low power events");
        releaseEvents(RESTRICTION_TYPE.LOW_POWER);
        currentLPState = RESTRICTED_EVENT_STATE.DEFAULT_BEHAVIOR;
      }
    }
  }

  protected void updateTabletChargingProcedureAlert() {
    if (!viewmodel.getEncounterData().isLive()) {
      return;
    }
    final boolean isCharging = (viewmodel.getTabletState().getIsCharging() || triggeredEvents[5]);
    if (isCharging && modalDialog == null) {
      MedDevLog.warn("Alarm", "Tablet charging in procedure alarm activated");

      checkProcedureSuspension();
      final MaterialAlertDialogBuilder builder = new MaterialAlertDialogBuilder(requireActivity(),
          R.style.ThemeOverlay_App_MaterialAlertDialog);
      final String title = getString(R.string.urgent_alert_title);
      final String content = getString(R.string.charging_in_procedure_instructions);
      builder.setMessage(content);
      builder.setTitle(title);
      modalDialog = builder.create();
      modalDialog.setCancelable(false);
      modalDialog.show();   
    } else if (!isCharging && modalDialog != null) {
      MedDevLog.info("Alarm", "Tablet charging in procedure alarm resolved");

      checkProcedureSuspension();
      if (modalDialog != null) {
        modalDialog.cancel();
        modalDialog = null;
      }

    }
  }

  protected void updateBatterySubsysAlarm() {
    final AlarmEvent batteryFaultEvent = new AlarmEvent(AlarmType.BatterySubsystemFault,
        alarmPriorityMap.get(AlarmType.BatterySubsystemFault));
    final TabletState tabletState = viewmodel.getTabletState();

    if ((batterySubsysFault == null || batterySubsysFault.numVal == 0) &&
        tabletState.getActiveAlarms().contains(batteryFaultEvent)) {
      MedDevLog.info("Alarm", "Battery subsystem fault alarm resolved");
      tabletState.removeAlarm(batteryFaultEvent);
      checkProcedureSuspension();
    } else if (batterySubsysFault != null && batterySubsysFault.numVal != 0 &&
        !tabletState.getActiveAlarms().contains(batteryFaultEvent)) {
      MedDevLog.warn("Alarm", "Battery subsystem fault alarm activated");
      batteryFaultEvent.setCompositeAlerts(
          ChargerStatusFlags.None.getCompositeString(batterySubsysFault));
      tabletState.addNewAlarm(batteryFaultEvent);
      checkProcedureSuspension();
    }
  }

  protected void updateControllerTimeoutAlarm() {
    boolean timedOut = viewmodel.getControllerCommunicationManager().isConnectionTimedOut();
    if (triggeredEvents[3]) {
      timedOut = true;
    }

    final AlarmEvent controllerTimeoutEvent = new AlarmEvent(AlarmType.ControllerStatusTimeout,
        alarmPriorityMap.get(AlarmType.ControllerStatusTimeout));
    if (timedOut) {
      if (!viewmodel.getTabletState().getActiveAlarms().contains(controllerTimeoutEvent)) {
        MedDevLog.warn("Alarm", "Controller status missing alarm activated");
        viewmodel.getTabletState().addNewAlarm(controllerTimeoutEvent);
        checkProcedureSuspension();
      }
      for (final ImageView icon : errorIndicators) {
        icon.setVisibility(View.VISIBLE);
      }
      if (restrictedEvents != null && currentCSTState == RESTRICTED_EVENT_STATE.DEFAULT_BEHAVIOR) {
        for (final RestrictedEvent evt : restrictedEvents) {
          if (evt.restrictionType == RESTRICTION_TYPE.CONTROLLER_STATUS_TIMEOUT) {
            evt.graphicButton.setOnClickListener(actionDisabledCST_OnClickListener);
            evt.graphicButton.setAlpha(0.5f);
          }
        }
      }
      currentCSTState = RESTRICTED_EVENT_STATE.PROHIBITED;
    } else if (viewmodel.getTabletState().getActiveAlarms().contains(controllerTimeoutEvent)) {
      MedDevLog.info("Alarm", "Controller status missing alarm resolved");
      viewmodel.getTabletState().removeAlarm(controllerTimeoutEvent);
      checkProcedureSuspension();
      for (final ImageView icon : errorIndicators) {
        icon.setVisibility(View.GONE);
      }
      if (currentCSTState == RESTRICTED_EVENT_STATE.PROHIBITED) {
        MedDevLog.warn("Navigation", "Unrestricted controller status events");
        releaseEvents(RESTRICTION_TYPE.CONTROLLER_STATUS_TIMEOUT);
      }
    }
  }

  private void updateFuelGaugeAlarm(final BitFlags<FuelGaugeStatusFlags> fgStatusField) {
    final AlarmEvent fuelFaultEvent = new AlarmEvent(AlarmType.FuelGaugeStatusFault,
        alarmPriorityMap.get(AlarmType.FuelGaugeStatusFault));
    final boolean containsFuelFault = viewmodel.getTabletState().getActiveAlarms().contains(
        fuelFaultEvent);
    if ((fgStatusField.toInt() != 0) && !containsFuelFault) {
      MedDevLog.warn("Alarm", "Fuel gauge status fault alarm activated");
      fuelFaultEvent.setCompositeAlerts(
          FuelGaugeStatusFlags.None.getCompositeString(fgStatusField));
      viewmodel.getTabletState().addNewAlarm(fuelFaultEvent);
      checkProcedureSuspension();
    } else if ((fgStatusField.toInt() == 0) && containsFuelFault) {
      MedDevLog.info("Alarm", "Fuel gauge status fault alarm resolved");
      viewmodel.getTabletState().removeAlarm(fuelFaultEvent);
      checkProcedureSuspension();
    }
  }

  private void updateEcgTimeoutAlarm() {
    boolean ecgTimeout = viewmodel.getControllerCommunicationManager().isEcgMissing();
    if (triggeredEvents[4]) {
      ecgTimeout = true;
    }

    final AlarmEvent missingDataEvent = new AlarmEvent(AlarmType.EcgDataMissing,
        alarmPriorityMap.get(AlarmType.EcgDataMissing));
    final boolean containsMissingData = viewmodel.getTabletState().getActiveAlarms().contains(missingDataEvent);

    if (!ecgTimeout && containsMissingData) {
      MedDevLog.debug("ECG Alarm", "ECG data missing alarm resolved");
      viewmodel.getTabletState().removeAlarm(missingDataEvent);
    } else if (ecgTimeout && !containsMissingData) {
      MedDevLog.debug("ECG Alarm", "ECG data missing alarm activated ("
              + ecgTimeout + "=?"
              + viewmodel.getControllerCommunicationManager().isEcgMissing() + ":"
              + containsMissingData + ")");
      viewmodel.getTabletState().addNewAlarm(missingDataEvent);
    }
    checkProcedureSuspension();
  }

  private void toggleBlinkingAnimation(final ImageView icon, final boolean active) {
    if (active) {
      final Animation blinkAnimation = new AlphaAnimation(1.0f, 0.0f);
      blinkAnimation.setDuration(500); // 500ms = 1/2 second
      blinkAnimation.setInterpolator(new LinearInterpolator());
      blinkAnimation.setRepeatMode(Animation.REVERSE);
      blinkAnimation.setRepeatCount(Animation.INFINITE);
      icon.startAnimation(blinkAnimation);
    } else {
      icon.clearAnimation();
    }
  }

  private void toggleConnectionIcon(final boolean isConnected) {
    // Set icon based on connection type (BLE or USB)
    if (viewmodel != null && viewmodel.getControllerCommunicationManager() != null) {
      boolean isBle = false;
      try {
        isBle = viewmodel.getControllerCommunicationManager().isBleConnection();

      } catch (Exception e) {
        // fallback to false
      }
      if (isBle) {
        ivUsbIcon.setImageResource(
            isConnected ? R.drawable.ic_ble : R.drawable.ic_ble_disconnected);
      } else {
        ivUsbIcon.setImageResource(
            isConnected ? R.drawable.logo_usb : R.drawable.logo_usb_disconnected);
      }
    } else {
      ivUsbIcon.setImageResource(
          isConnected ? R.drawable.logo_usb : R.drawable.logo_usb_disconnected);
    }
  }

  protected void checkProcedureSuspension() {
    boolean isSuspended = false;
    boolean isIntraSuspended = false;
    boolean isExternSuspended = false;
    final boolean activeAlarms = !viewmodel.getTabletState().getActiveAlarms().isEmpty();
    if (activeAlarms) {
      for (final AlarmEvent alarm : fullSuspendAlarms) {
        if (viewmodel.getTabletState().getActiveAlarms().contains(alarm)) {
          isSuspended = true;
          break;
        }
      }
      for (final AlarmEvent alarm : externalSuspendAlarms) {
        if (viewmodel.getTabletState().getActiveAlarms().contains(alarm)) {
          isExternSuspended = true;
          break;
        }
      }
      for (final AlarmEvent alarm : internalSuspendAlarms) {
        if (viewmodel.getTabletState().getActiveAlarms().contains(alarm)) {
          isIntraSuspended = true;
          break;
        }
      }
    }

    final EncounterState currentState = viewmodel.getEncounterData().getState();
    EncounterState nextState = currentState;
    if (isIntraSuspended) {
      nextState = EncounterState.IntraSuspended;
    }
    if (isExternSuspended) {
      nextState = isIntraSuspended ? EncounterState.DualSuspended : EncounterState.ExternalSuspended;
    }
    if (isSuspended) {
      nextState = EncounterState.Suspended;
    } else if (!activeAlarms && viewmodel.getEncounterData().isLive() && transitionAllowed) {
      nextState = EncounterState.Active; // Ternary operator for if we return to connected vs active
    }

    if (nextState != currentState) {
      MedDevLog.info("State Change", "State transitioned to " + nextState);
      viewmodel.getEncounterData().setState(nextState);
      transitionAllowed = false;
      final Scheduler timeout = new Scheduler(minimumUptimeHandler, 4000, () -> {
        transitionAllowed = true;
        checkProcedureSuspension();
      });
      timeout.circulateTimeout(false);
    }
  }

  private int getInt(final int resourceInt) {
    return requireContext().getResources().getInteger(resourceInt);
  }

  public void onEcgStatusTimeout() {
    updateEcgTimeoutAlarm();
  }

  public void onConnectionStatusChange(final boolean isConnected) {
    if (!isConnected) {
      viewmodel.getControllerState().clearControllerState();
      if (viewmodel.getEncounterData().isLive()) {
        notificationPlayer.release();
        notificationPlayer = MediaPlayer.create(requireContext(), R.raw.disconnect_alert);
        if (!viewmodel.getTabletState().getIsTabletMuted()) {
          notificationPlayer.start();
        }
      }
    }
    viewmodel.setBleConnection(isConnected ? MainViewModel.BleConnectionState.CONNECTED :
            MainViewModel.BleConnectionState.FAILED);
    MedDevLog.audit("Controller", (isConnected) ? "Controller connected" : "Controller disconnected");
    toggleConnectionIcon(isConnected);
    updateBatteryDisplay();
    updateControllerTimeoutAlarm();
    updateBatterySubsysAlarm();
  }

  public void onConnectionStatusTimeout() {
    updateControllerTimeoutAlarm();
  }

  public void onPacketArrival(final Packet incPacket) {
    final PacketId id = PacketId.valueOfByte(incPacket.packetType);
    if ((id != null) && (tvTestUsbText != null)) {
      final String packetLogMsg = String.format("Time: %s ,Type: %s", new Date(), id);
      // tvTestUsbText.setText(packetLogMsg);
    }
  }

  @Override
  public void onCatheterImpedance(ImpedanceSample impedanceSample, String impedanceLogMsg) {
    Log.d("BaseHydroGuideFragment", "onCatheterImpedance: " + impedanceLogMsg);
  }

  public void onFilteredSamples(final EcgSample sample, final String sampleLogMsg) {
    updateSerialDataFile(sampleLogMsg, DataFiles.SERIAL_ECG_DATA);
    boolean externalDisconnect = false;
    boolean internalDisconnect = false;
    boolean hardwareFault = false;
    if (SamplingStatusFlags.LeftLegLeadOff.isFlaggedIn(sample.samplingStatus) ||
        SamplingStatusFlags.LeftArmLeadOff.isFlaggedIn(sample.samplingStatus) ||
        SamplingStatusFlags.RightArmLeadOff.isFlaggedIn(sample.samplingStatus) ||
        triggeredEvents[6] ||
        SamplingStatusFlags.LimbSaturatedLow.isFlaggedIn(sample.samplingStatus) ||
        SamplingStatusFlags.LimbSaturatedHigh.isFlaggedIn(sample.samplingStatus)) {
      externalDisconnect = true;
      if (!viewmodel.getTabletState().getActiveAlarms().contains(
          new AlarmEvent(AlarmType.ExternalLeadOff,
              alarmPriorityMap.get(AlarmType.ExternalLeadOff)))) {
        // External lead failed, suspend and noise
        MedDevLog.warn("Alarm", "External lead off alarm activated");
        viewmodel.getTabletState().addNewAlarm(new AlarmEvent(AlarmType.ExternalLeadOff,
            alarmPriorityMap.get(AlarmType.ExternalLeadOff)));
        notificationPlayer.release();
        notificationPlayer = MediaPlayer.create(requireContext(), R.raw.lead_off_alert);
        if (!viewmodel.getTabletState().getIsTabletMuted()) {
          notificationPlayer.start();
        }
      }
    } else if (viewmodel.getTabletState().getActiveAlarms().contains(
        new AlarmEvent(AlarmType.ExternalLeadOff,
            alarmPriorityMap.get(AlarmType.ExternalLeadOff)))) {
      MedDevLog.info("Alarm", "External lead off alarm resolved");
      viewmodel.getTabletState().removeAlarm(new AlarmEvent(AlarmType.ExternalLeadOff,
          alarmPriorityMap.get(AlarmType.ExternalLeadOff)));
    }
    final AlarmEvent alarmInternalLeadOff = new AlarmEvent(AlarmType.InternalLeadOff,
        alarmPriorityMap.get(AlarmType.InternalLeadOff));
    final boolean alreadyAlarming = viewmodel.getTabletState().getActiveAlarms().contains(
        alarmInternalLeadOff);
    if (SamplingStatusFlags.InternalLeadOff.isFlaggedIn(sample.samplingStatus) ||
        triggeredEvents[7] ||
        SamplingStatusFlags.InternalSaturatedLow.isFlaggedIn(sample.samplingStatus) ||
        SamplingStatusFlags.InternalSaturatedHigh.isFlaggedIn(sample.samplingStatus)) {
      internalDisconnect = true;
      if (!alreadyAlarming) {
        // Internal lead failed, suspend and noise
        MedDevLog.warn("Alarm", "Internal lead off alarm activated");
        viewmodel.getTabletState().addNewAlarm(alarmInternalLeadOff);
        notificationPlayer.release();
        notificationPlayer = MediaPlayer.create(requireContext(), R.raw.lead_off_alert);
        if (!viewmodel.getTabletState().getIsTabletMuted()) {
          notificationPlayer.start();
        }
      }
    } else if (alreadyAlarming) {
      MedDevLog.warn("Alarm", "Internal lead off alarm activated");
      viewmodel.getTabletState().removeAlarm(alarmInternalLeadOff);
    }
    if (SamplingStatusFlags.SupplyOutOfRange.isFlaggedIn(sample.samplingStatus)) {
      hardwareFault = true;
      if (!viewmodel.getTabletState().getActiveAlarms().contains(
              new AlarmEvent(AlarmType.SupplyRangeFault,
                      alarmPriorityMap.get(AlarmType.SupplyRangeFault)))) {
        // External lead failed, suspend and noise
        MedDevLog.warn("Alarm", "Hardware fault alarm activated");
        viewmodel.getTabletState().addNewAlarm(new AlarmEvent(AlarmType.SupplyRangeFault,
                alarmPriorityMap.get(AlarmType.SupplyRangeFault)));
        notificationPlayer.release();
        notificationPlayer = MediaPlayer.create(requireContext(), R.raw.lead_off_alert);
        if (!viewmodel.getTabletState().getIsTabletMuted()) {
          notificationPlayer.start();
        }
      }
    } else if (viewmodel.getTabletState().getActiveAlarms().contains(
            new AlarmEvent(AlarmType.SupplyRangeFault,
                    alarmPriorityMap.get(AlarmType.SupplyRangeFault)))) {
      MedDevLog.info("Alarm", "Hardware fault alarm alarm resolved");
      viewmodel.getTabletState().removeAlarm(new AlarmEvent(AlarmType.SupplyRangeFault,
              alarmPriorityMap.get(AlarmType.SupplyRangeFault)));
    }
    if (!internalDisconnect && !externalDisconnect &&!hardwareFault) {
      viewmodel.getEncounterData().setProcedureStarted(true);
    }

    updateEcgTimeoutAlarm();
    checkProcedureSuspension();
  }

  public void onPatientHeartbeat(final Heartbeat heartbeat, final String heartbeatLogMsg) {
    updateSerialDataFile(heartbeatLogMsg, DataFiles.SERIAL_HEARTBEAT_DATA);
  }

  public void onButtonsStateChange(final Button newButton, final String buttonLogMsg) {
    updateSerialDataFile(buttonLogMsg, DataFiles.SERIAL_BUTTON_STATE_DATA);
    for (int idx = 0; idx < Button.ButtonIndex.NumButtons.getIntValue(); idx++) {
      final Button.ButtonIndex buttonIdx = Button.ButtonIndex.valueOfInt(idx);
      final ButtonState buttonState = newButton.buttons.getButtonStateAtKey(
              Objects.requireNonNull(buttonIdx));

      if (buttonState == ButtonState.ButtonIdle) {
        continue;
      }

      if (buttonState == ButtonState.ReleaseTransition) {
        Log.d("Feedback", "onButtonsStateChange: " + buttonIdx);
        Toast.makeText(requireContext(),"Pressed " + buttonIdx,Toast.LENGTH_SHORT).show();
        viewmodel.setButtonIndex(buttonIdx);
      } else {

      }
    }
  }

  public void onTabletChargeChange() {
    updateBatteryDisplay();
  }

  public void onTabletChargeStatusChange() {
    updateTabletChargingProcedureAlert();
  }

  protected void deleteProcedureFile(final EncounterData record) {
    // TODO NEED TO DISCUSS WITH AJAY
    if (record.getDataDirPath() == "" || record.getStartTime() == null) {
      return;
    }
    // If no start time, just wipe and call it good no file was generate
    final File dirLoc = Paths.get(record.getDataDirPath()).toFile();
    for (final File item : dirLoc.listFiles()) {
      item.delete();
    }
    dirLoc.delete();
  }

  protected void updateProcedureFile() {
    // final File procedureDir = new
    // File(viewmodel.getEncounterData().getDataDirPath());
    // if(!procedureDir.exists()) procedureDir.mkdir();
    String dirPath = viewmodel.getEncounterData().getDataDirPath();
    if (dirPath == null || dirPath.isEmpty()) {
      MedDevLog.error("updateProcedureFile", "Data directory path is null or empty");
      Toast.makeText(requireContext(), "Cannot update procedure file: data directory not set.", Toast.LENGTH_SHORT)
          .show();
      return;
    }
    final File procedureDir = new File(dirPath);
    final File procData = new File(procedureDir, DataFiles.PROCEDURE_DATA);

    try (final FileWriter writer = new FileWriter(procData, StandardCharsets.UTF_8)) {
      final String[] data = viewmodel.getEncounterData().getProcedureDataText();

      for (final String str : data) {
        Log.d("ABC", "updateProcedureFile: " + str);
        writer.write(str);
        Log.d("ABC", "lineSeparator: " + System.lineSeparator());
        writer.write(System.lineSeparator());
        writer.flush();
      }

      MediaScannerConnection.scanFile(requireContext(), new String[] { procData.getAbsolutePath() },
          new String[] {}, null);
    } catch (final IOException | JSONException e) {
      MedDevLog.error("BaseHydroGuideFragment", "Error updating procedure file", e);
      Toast.makeText(requireContext(), e.getMessage(), Toast.LENGTH_SHORT).show();
      throw new RuntimeException(e);
    }
  }

  public void onControllerStatus(final ControllerStatus ctrlStatus, final String logMsg) {
    updateSerialDataFile(logMsg, DataFiles.SERIAL_CONTROLLER_STATUS_DATA);
    if (currentCSTState == RESTRICTED_EVENT_STATE.PROHIBITED) {
      MedDevLog.info("Navigation", "Unrestricted controller status events");
      releaseEvents(RESTRICTION_TYPE.CONTROLLER_STATUS_TIMEOUT);
    }
    currentCSTState = RESTRICTED_EVENT_STATE.DEFAULT_BEHAVIOR;
    viewmodel.getControllerState().setBatteryLevelPct(Math.round(ctrlStatus.batteryCharge_pct));
    batterySubsysFault = ctrlStatus.chargerStatus;
    controllerStatusFlag = ctrlStatus.controllerStatus;

    updateBatteryDisplay();
    updateControllerTimeoutAlarm();
    updateBatterySubsysAlarm();
    updateFuelGaugeAlarm(ctrlStatus.fuelGaugeStatus);
    updateEcgTimeoutAlarm();
    viewmodel.getMonitorData().setHeartRate(Math.round(ctrlStatus.heartRate_bpm));
  }

  @Override
  public void onStartupStatus(final StartupStatus startupStatus, final String startupStatusLogMsg) {
    // TODO: Do something with the startup message. Get the controller firmware
    // version?
    final String firmwareVer = startupStatus.getControllerAppVersion();
    viewmodel.getControllerState().setFirmwareVersion(firmwareVer);
  }

  protected void onProcedureExitAttempt(final Runnable navigationAction) {
    final EncounterData encounter = viewmodel.getEncounterData();
    MedDevLog.audit("Procedure", "Procedure exit attempted");
    if (encounter.getState() == EncounterState.Uninitialized) {
      navigationAction.run();
      return;
    }
    final MaterialAlertDialogBuilder builder = new MaterialAlertDialogBuilder(requireActivity(),
        R.style.ThemeOverlay_App_MaterialAlertDialog);
    final String title = getString(R.string.alert_title_warning);
    final String content = getString(R.string.alert_content_procedue_cancel_warning);
    builder.setMessage(content);
    builder.setTitle(title);

    builder.setNegativeButton(getString(R.string.tip_location_confirm_yes), (dialog, id) -> {
      MedDevLog.audit("Procedure", "Procedure exited");
      if (!encounter.isConcluded()) {
        deleteProcedureFile(encounter);
      }
      encounter.clearEncounterData();
      navigationAction.run();
    });
    builder.setPositiveButton(getString(R.string.tip_location_confirm_no), (dialog, id) -> {
      // clear popup
      MedDevLog.audit("Procedure", "Procedure exit canceled");
    });
    final AlertDialog dialog = builder.create();
    dialog.show();
  }

  protected final View.OnClickListener actionDisabledNP_OnClickListener = v -> {
    final MaterialAlertDialogBuilder builder = new MaterialAlertDialogBuilder(requireActivity(),
        R.style.ThemeOverlay_App_MaterialAlertDialog);
    builder.setMessage("Select a profile and try again");
    builder.setNegativeButton("Return", (dialog, id) -> {
    });
    builder.show();
  };

  private final View.OnClickListener actionDisabledLP_OnClickListener = v -> {
    // Check if facility is selected in encounter data
    final EncounterData encounterData = viewmodel.getEncounterData();
    final Facility facility = encounterData.getPatient().getPatientFacility();
    
    // If facility is empty, set encounter state to uninitialized and return
    if (facility == null || facility.getFacilityName() == null ||
        facility.getFacilityName().isEmpty()) {
      encounterData.setState(EncounterState.Uninitialized);
      MedDevLog.warn("Navigation", "Facility not selected - cannot bypass low power alarm");
      Toast.makeText(requireContext(), "Please select a facility before proceeding.", 
          Toast.LENGTH_SHORT).show();
      return;
    }
    
    // Check if patient ID is entered
    final String patientId = encounterData.getPatient().getPatientId();
    if (patientId == null || patientId.trim().isEmpty()) {
      encounterData.setState(EncounterState.Uninitialized);
      MedDevLog.warn("Navigation", "Patient ID not entered - cannot bypass low power alarm");
      Toast.makeText(requireContext(), "Please enter patient ID before proceeding.", 
          Toast.LENGTH_SHORT).show();
      return;
    }
    
    // All validations passed, proceed with low power alarm handling
    MedDevLog.info("Navigation", "Restricted action attempted");
    final MaterialAlertDialogBuilder builder = new MaterialAlertDialogBuilder(requireActivity(),
        R.style.ThemeOverlay_App_MaterialAlertDialog);
    final String title = getString(R.string.alert_title_prohibited);
    final String content = getString(R.string.alert_content_action_disabled_LP_alarm);
    builder.setMessage(content);
    builder.setTitle(title);
    builder.setPositiveButton(getString(R.string.alert_button_return), (dialog, id) -> {
      // Clear Popup
    });
    builder.setNegativeButton(getString(R.string.alert_button_bypass), (dialog, id) -> {
      currentLPState = RESTRICTED_EVENT_STATE.SILENCED;
      releaseEvents(RESTRICTION_TYPE.LOW_POWER);
      final var navHostFrag = requireActivity().getSupportFragmentManager()
          .findFragmentById(R.id.nav_host_fragment_activity_main);
      final Fragment currentFrag = navHostFrag.getChildFragmentManager().getFragments().get(0);
      final Class currentClass = currentFrag.getClass();
      if (currentClass == PatientInputFragment.class) {        
        TryNavigateToUltrasound(Utility.FROM_SCREEN_PATIENT_INPUT);
      } else if (currentClass == HomeFragment.class) {
        Navigation.findNavController(requireView())
            .navigate(R.id.action_navigation_home_to_procedure);
      }
    });
    final AlertDialog dialog = builder.create();
    dialog.show();
  };

  private final View.OnClickListener actionDisabledCST_OnClickListener = v -> {
    MedDevLog.info("Navigation", "Restricted action attempted");
    final MaterialAlertDialogBuilder builder = new MaterialAlertDialogBuilder(requireActivity(),
        R.style.ThemeOverlay_App_MaterialAlertDialog);
    final String title = getString(R.string.alert_title_prohibited);
    final String content = getString(R.string.alert_content_action_disabled_CST_alarm);
    builder.setMessage(content);
    builder.setTitle(title);
    builder.setPositiveButton(getString(R.string.alert_button_return), (dialog, id) -> {
      // Clear Popup
    });
    builder.setNegativeButton(getString(R.string.alert_button_bypass), (dialog, id) -> {
      currentCSTState = RESTRICTED_EVENT_STATE.SILENCED;
      releaseEvents(RESTRICTION_TYPE.CONTROLLER_STATUS_TIMEOUT);
      Navigation.findNavController(requireView())
          .navigate(R.id.action_navigation_home_to_procedure);
    });
    final AlertDialog dialog = builder.create();
    dialog.show();
  };

  private final Observable.OnPropertyChangedCallback encounterStateChanged_Listener = new Observable.OnPropertyChangedCallback() {
    @Override
    public void onPropertyChanged(final Observable sender, final int propertyId) {
      if (propertyId == BR.state) {
        // Check charging alert whenever encounter state changes to ensure alert is shown if charging during live procedure
        updateTabletChargingProcedureAlert();
      }
    }
  };

  private final Observable.OnPropertyChangedCallback tabletBatteryChanged_Listener = new Observable.OnPropertyChangedCallback() {
    @Override
    public void onPropertyChanged(final Observable sender, final int propertyId) {
      onTabletChargeChange();
      updateTabletBatteryDisplay();
    }
  };

  private final Observable.OnPropertyChangedCallback tabletChargerStateChanged_Listener = new Observable.OnPropertyChangedCallback() {
    @Override
    public void onPropertyChanged(final Observable sender, final int propertyId) {
      if (propertyId == BR.isCharging) {
        onTabletChargeStatusChange();
      }
    }
  };

  private final Observable.OnPropertyChangedCallback encounterDataChanged_Listener = new Observable.OnPropertyChangedCallback() {
    @Override
    public void onPropertyChanged(final Observable sender, final int propertyId) {
      // Note the change
      MedDevLog.info("State Change", "Initializing state to Idle");
      viewmodel.getEncounterData().initializeState();
      viewmodel.getEncounterData().removeOnPropertyChangedCallback(encounterDataChanged_Listener);
      viewmodel.getEncounterData().getPatient().removeOnPropertyChangedCallback(patientDataChanged_Listener);
    }
  };

  private final Observable.OnPropertyChangedCallback patientDataChanged_Listener = new Observable.OnPropertyChangedCallback() {
    @Override
    public void onPropertyChanged(final Observable sender, final int propertyId) {
      // Note the change
      // - Also This can be freed when its use expires (i.e. on state change and
      // non-reinit)
      MedDevLog.info("State Change", "Initializing state to Idle");
      viewmodel.getEncounterData().initializeState();
      viewmodel.getEncounterData().getPatient().removeOnPropertyChangedCallback(patientDataChanged_Listener);
      viewmodel.getEncounterData().removeOnPropertyChangedCallback(encounterDataChanged_Listener);
    }
  };

  private final OnBackPressedCallback backButtonPressed_Listener = new OnBackPressedCallback(true) {
    @Override
    public void handleOnBackPressed() {
      View fragmentView = getView();
      if (fragmentView == null) {
        // Prevent crash if view is not created
        return;
      }
      
      final NavController navController = Navigation.findNavController(fragmentView);
      final int previousDestinationId = navController.getPreviousBackStackEntry() != null
          ? navController.getPreviousBackStackEntry().getDestination().getId() : -1;
      
      final boolean navigatingToHome = previousDestinationId == R.id.navigation_home;
      final boolean procedureBlocked = viewmodel.getEncounterData().isConcluded()
          && (previousDestinationId == R.id.navigation_procedure);
      final boolean navigatingToRepView = previousDestinationId == R.id.reportViewFragment;
      
      // Blocked cases: don't allow back navigation
      if (procedureBlocked || navigatingToRepView) {
        super.handleOnBackCancelled();
        return;
      }
      
      // Only show procedure exit warning on PatientInputFragment when:
      // 1. Navigating to Home
      // 2. Changes have been made (encounter state is not Uninitialized)
      if (navigatingToHome && BaseHydroGuideFragment.this instanceof PatientInputFragment) {
        final boolean hasChanges = viewmodel.getEncounterData().getPatient().hasAnyData();
        
        if (hasChanges) {
          // Show warning only if there are actual changes
          onProcedureExitAttempt(new Runnable() {
            @Override
            public void run() {
              Navigation.findNavController(fragmentView).popBackStack();
            }
          });
          super.handleOnBackCancelled();
          return;
        }
      }
      
      // For all other cases, allow normal back navigation without warning
      Navigation.findNavController(fragmentView).popBackStack();
    }
  };

  protected class VolumeChangeListener extends BroadcastReceiver {
    @Override
    public void onReceive(final Context context, final Intent intent) {
      if (Objects.equals(intent.getAction(), VOLUME_CHANGED_ACTION)) {
        final int newVolume = audioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
        viewmodel.getTabletState().setAudioVolume(newVolume);
      }
    }
  }

  private void networkObserver() {
    connectivityManager = (ConnectivityManager) getActivity().getSystemService(Context.CONNECTIVITY_SERVICE);
    NetworkRequest.Builder builder = new NetworkRequest.Builder();
    builder.addCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET);
    builder.addTransportType(NetworkCapabilities.TRANSPORT_WIFI);
    if (BaseHydroGuideFragment.IS_SYNC_PERMITTED) {
      connectivityManager.registerNetworkCallback(builder.build(), networkCallback);
    }
  }

  private ConnectivityManager.NetworkCallback networkCallback = new ConnectivityManager.NetworkCallback() {
    @Override
    public void onAvailable(Network network) {
      Log.d("Logs", "Internet Available");
      triggerSync();
    }

    @Override
    public void onLost(Network network) {
      Log.d("Logs", "Internet Lost");
    }
  };

  public void triggerSync() {
    Log.d("Logs", "Invoked Trigger Sync");

    if (!viewmodel.getEncounterData().isLive()) {
      if (!PrefsManager.getSyncStatus(getActivity())) {
        PrefsManager.setSyncStatus(getActivity(), true);
        performSync();
        PrefsManager.setSyncStatus(getActivity(), false);
      }
    }
  }

  private void deregisterNetworkListener() {
    Log.d("Logs", "Network Listener De-register");
    if (connectivityManager != null && networkCallback != null) {
      connectivityManager.unregisterNetworkCallback(networkCallback);
    }
  }

  /**
   * Retrieves the current storage mode using the injected IStorageManager.
   * 
   * @return The current storage mode as an IStorageManager.StorageMode.
   */
  public IStorageManager.StorageMode getStorageMode() {
    if (storageManager != null) {
      return storageManager.getStorageMode(); // Return the StorageMode directly
    }
    return IStorageManager.StorageMode.UNKNOWN; // Default value if storageManager is null
  }

  /**
   * Determines if clinician management is accessible based on storage mode and
   * network connectivity.
   * 
   * @param isNetworkConnected The current network connectivity status.
   * @return True if clinician management is accessible, false otherwise.
   */
  public boolean isClinicianManagementAccessible(boolean isNetworkConnected) {
    String storageMode = getStorageMode().name();
//    if (IStorageManager.StorageMode.ONLINE.equals(storageMode)) {
//      return isNetworkConnected;
//    }
    if(IStorageManager.StorageMode.ONLINE.toString().equals(storageMode))
      return isNetworkConnected;
    return true; // Default behavior for OFFLINE or unknown modes
  }

  protected boolean isProbeConnected() {
    UsbManager usbManager = (UsbManager) requireContext().getSystemService(Context.USB_SERVICE);
    if (usbManager != null) {
      HashMap<String, UsbDevice> deviceList = usbManager.getDeviceList();
      for (UsbDevice device : deviceList.values()) {
        if (device.getVendorId() == 0x1dbf) { // EchoNous vendor ID
          return true;
        }
      }
    }
    return false;
  }

  protected void TryNavigateToUltrasound(String fromScreen) {
    TryNavigateToUltrasound(fromScreen, false, 0);
  }

  protected void TryNavigateToUltrasound(String fromScreen, boolean isVenousAccess) {
    TryNavigateToUltrasound(fromScreen, isVenousAccess,0);
  }

  protected void TryNavigateToUltrasound(String fromScreen, @IdRes int routeIfNotConnected) {
    TryNavigateToUltrasound(fromScreen, false,routeIfNotConnected);
  }

  protected void TryNavigateToUltrasound(String fromScreen, boolean isVenousAccess, @IdRes int routeIfNotConnected)
  {
    Bundle bundle = new Bundle();
    bundle.putString("fromScreen", fromScreen);
    bundle.putBoolean("isVenousAccess", isVenousAccess);

    Log.d("Ultrasound", "TryNavigateToUltrasound" + fromScreen);
    if (isProbeConnected()) {
      Log.d("Ultrasound", "Probe connected, navigating directly to scanning");
      navigateToUltrasound(bundle);
    } else {
      if(routeIfNotConnected == 0) {
        Log.d("Ultrasound", "Probe not connected, navigating to connect screen");
        navigateToConnectUltrasound(bundle);
      }
      else
      {
        findNavController(this)
                .navigate(routeIfNotConnected);
      }
    }
  }

  protected void navigateToUltrasound(String fromScreen){
    Bundle bundle = new Bundle();
    bundle.putString("fromScreen", fromScreen);
    bundle.putBoolean("isVenousAccess", false);
    navigateToUltrasound(bundle);
  }

  protected void navigateToUltrasound(Bundle bundle) {
    NavController navController = findNavController(this);

    Integer currentDestinationId =
            navController.getCurrentDestination() != null
                    ? navController.getCurrentDestination().getId()
                    : null;

    if (currentDestinationId == null ||
            currentDestinationId != R.id.navigation_scanning_ultrasound) {
      navController.navigate(R.id.navigation_scanning_ultrasound, bundle);
    }
  }

  protected void navigateToConnectUltrasound(Bundle bundle) {
    NavController navController = findNavController(this);

    Integer currentDestinationId =
            navController.getCurrentDestination() != null
                    ? navController.getCurrentDestination().getId()
                    : null;

    if (currentDestinationId == null ||
            currentDestinationId != R.id.navigation_connect_ultrasound) {
      navController.navigate(R.id.navigation_connect_ultrasound, bundle);
    }
  }

}