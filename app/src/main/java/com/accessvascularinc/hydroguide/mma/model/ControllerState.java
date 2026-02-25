package com.accessvascularinc.hydroguide.mma.model;

import android.icu.util.DateInterval;

import androidx.databinding.BaseObservable;
import androidx.databinding.Bindable;

import com.accessvascularinc.hydroguide.mma.BR;

/**
 * A data binding object for UI elements that keeps track of the current state of the ECG
 * controller.
 */
public class ControllerState extends BaseObservable {
  private float batteryLevelPct = -1;
  private DateInterval batteryTimeRemaining = null;
  private Boolean batteryIsCharging = false;
  private String firmwareVersion = "";

  /**
   * Gets battery level of the ECG controller as a percentage. A value of -1 would mean no
   * controller is connected.
   *
   * @return the battery level
   */
  @Bindable
  public float getBatteryLevelPct() {
    return batteryLevelPct;
  }

  /**
   * Sets the controller's battery level as a percentage.
   *
   * @param newBattLvl the latest battery level of the ECG controller.
   */
  public void setBatteryLevelPct(final float newBattLvl) {
    batteryLevelPct = newBattLvl;
    notifyPropertyChanged(BR.batteryLevelPct);
  }

  /**
   * Get whether the ECG controller's battery is charging or not. Not to be confused with the
   * charging state of the tablet this app is running on.
   *
   * @return true only if controller is detected and reports that it is charging.
   */
  public Boolean isCharging() {
    return batteryIsCharging;
  }

  /**
   * Gets the approximate amount of battery time remaining, or amount of time the controller can be
   * used before needing to be recharged. It is perhaps most useful to parse into a string that
   * shows the time in hours and minutes.
   *
   * @return the time interval of battery time remaining.
   */
  public DateInterval getBatteryTimeRemaining() {
    return batteryTimeRemaining;
  }

  public String getFirmwareVersion() {
    return firmwareVersion;
  }

  public void setFirmwareVersion(final String newFirmwareVer) {
    firmwareVersion = newFirmwareVer;
  }

  /**
   * Clears or resets the controller state data in this instance to default.
   */
  public void clearControllerState() {
    setBatteryLevelPct(-1);
    batteryTimeRemaining = null;
    batteryIsCharging = null;
    firmwareVersion = "";
  }
}
