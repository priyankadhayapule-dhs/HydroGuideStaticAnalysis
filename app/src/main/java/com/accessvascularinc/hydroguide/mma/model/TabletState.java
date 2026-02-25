package com.accessvascularinc.hydroguide.mma.model;

import android.content.Context;
import android.content.pm.PackageManager;

import androidx.databinding.BaseObservable;
import androidx.databinding.Bindable;

import com.accessvascularinc.hydroguide.mma.BR;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;

import java.util.concurrent.PriorityBlockingQueue;

/**
 * A data binding object for UI elements that keeps track of the current state of the Tablet
 * including free space, volume, and brightness.
 */
public class TabletState extends BaseObservable {
  private int tabletChargePct = -1;
  private long freeSpace_bytes = 0;
  private long totalSpace_bytes = 0;
  private int audioVolume = 5; // Android uses values from 0 to 15.
  private int screenBrightness = 50; // Android uses values from 1 to 255
  private boolean isCharging = false;
  private boolean isMuted = false;
  private PriorityBlockingQueue<AlarmEvent> activeAlarms = new PriorityBlockingQueue<>(); // MinHeap

  @Bindable
  public int getTabletChargePct() {
    return tabletChargePct;
  }

  public void setTabletChargePct(final int chargePct) {
    tabletChargePct = chargePct;
    notifyPropertyChanged(BR.tabletChargePct);
  }

  /**
   * Gets the free space remaining in bytes. A value of 0 indicates the value has not been updated.
   *
   * @return the free space bytes.
   */
  @Bindable
  public long getFreeSpaceBytes() {
    return freeSpace_bytes;
  }

  /**
   * Sets the tablet's remaining free space in bytes.
   *
   * @param newFreeSpaceBytes the latest remaining free space.
   */
  public void setFreeSpaceBytes(final long newFreeSpaceBytes) {
    freeSpace_bytes = newFreeSpaceBytes;
    notifyPropertyChanged(BR.freeSpaceBytes);
  }

  /**
   * Gets the total available space dedicated to the application in bytes. A value of 0 indicates
   * the user has not yet navigated to a screen that requires the
   *
   * @return the total space bytes.
   */
  @Bindable
  public long getTotalSpaceBytes() {
    return totalSpace_bytes;
  }

  /**
   * Sets the total space in
   *
   * @param newTotalSpaceBytes the new total space bytes
   */
  public void setTotalSpaceBytes(final long newTotalSpaceBytes) {
    totalSpace_bytes = newTotalSpaceBytes;
  }

  /**
   * Gets audio volume in a range of 0 to 15
   *
   * @return the audio volume
   */
  @Bindable
  public int getAudioVolume() {
    return audioVolume;
  }

  /**
   * Sets audio volume in a range of 0 to 15
   *
   * @param newVolume the new volume
   */
  public void setAudioVolume(final int newVolume) {
    audioVolume = newVolume;
    notifyPropertyChanged(BR.audioVolume);
  }

  /**
   * Gets screen brightness in a range of 1 to 255
   *
   * @return the screen brightness
   */
  @Bindable
  public int getScreenBrightness() {
    return screenBrightness;
  }

  /**
   * Sets screen brightness in a range of 1 to 255
   *
   * @param newBrightness the new brightness
   */
  public void setScreenBrightness(final int newBrightness) {
    screenBrightness = newBrightness;
    notifyPropertyChanged(BR.screenBrightness);
  }

  @Bindable
  public boolean getIsCharging() {
    return isCharging;
  }

  public void setIsCharging(final boolean chargeState) {
    isCharging = chargeState;
    notifyPropertyChanged(BR.isCharging);
  }

  @Bindable
  public PriorityBlockingQueue<AlarmEvent> getActiveAlarms() {
    return activeAlarms;
  }

  public void addNewAlarm(final AlarmEvent newAlarm) {
    getActiveAlarms().add(newAlarm);
    notifyPropertyChanged(BR.activeAlarms);
  }

  public void removeAlarm(final AlarmEvent targetAlarm) {
    getActiveAlarms().remove(targetAlarm);
    notifyPropertyChanged(BR.activeAlarms);
  }

  public void setActiveAlarms(final PriorityBlockingQueue<AlarmEvent> newPQueue) {
    activeAlarms = newPQueue;
    notifyPropertyChanged(BR.activeAlarms);
  }

  @Bindable
  public boolean getIsTabletMuted() {
    return isMuted;
  }

  public void setTabletMuted(final boolean isMuted) {
    this.isMuted = isMuted;
  }

  public static String getAppVersion(final Context cxt) {
    String appVersion = "???";
    try {
      appVersion = cxt.getPackageManager().getPackageInfo(cxt.getPackageName(), 0).versionName;
    } catch (final PackageManager.NameNotFoundException e) {
      MedDevLog.error("TabletState", "Error getting app version", e);
    }
    return appVersion;
  }

  /**
   * Initializes the tablet with volume and brightness
   *
   * @param volume     the volume (0 to 15)
   * @param brightness the brightness (1 to 255)
   */
  public void initTabletState(final int volume, final int brightness) {
    this.audioVolume = volume;
    this.screenBrightness = brightness;
  }
}
