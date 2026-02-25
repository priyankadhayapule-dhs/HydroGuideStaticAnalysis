package com.accessvascularinc.hydroguide.mma.model;

import androidx.databinding.BaseObservable;
import androidx.databinding.Bindable;

import com.accessvascularinc.hydroguide.mma.BR;

import java.util.Date;

/**
 * An observable object for keeping track of recent captured information from the monitor device to
 * show the user in the view. It records the heart rate, historical characteristics about the a
 * section of the heart beat (the p-wave amplitude), and the time of the most recent capture.
 */
public class MonitorData extends BaseObservable {
  private double heartRate_bpm = -1;
  private boolean invalidHeartRate = true;
  private TrackedPWaves currentPWaveAmplitudes = new TrackedPWaves();
  private TrackedPWaves maxPWaveAmplitudes = new TrackedPWaves();
  private TrackedPWaves capturedPWaveAmplitudes = new TrackedPWaves();
  private Date captureTime = null;

  /**
   * Gets the latest recorded heart rate in beats per minute
   *
   * @return the heart rate in beats per minute
   */
  @Bindable
  public double getHeartRateBpm() {
    return heartRate_bpm;
  }

  /**
   * Stores the most recent captured heart rate in beats per minute
   *
   * @param newRate_bpm the new rate in bets per minute
   */
  public void setHeartRate(final double newRate_bpm) {
    heartRate_bpm = newRate_bpm;
    notifyPropertyChanged(BR.heartRateBpm);
  }

  public String getDisplayHeartRate() {
    if (heartRate_bpm > 160 || heartRate_bpm < 30 || invalidHeartRate) {
      return "??? ";
    }
    return Double.toString(heartRate_bpm);
  }

  /**
   * Gets current captured p-wave amplitudes
   *
   * @return the amplitude values for the current p-wave
   */
  @Bindable
  public TrackedPWaves getCurrentPWaveAmplitudes() {
    return currentPWaveAmplitudes;
  }

  /**
   * Sets the current captured p-wave amplitudes
   *
   * @param newAmplitudes the new amplitude values for the current p-wave
   */
  public void setCurrentPWaveAmplitudes(final TrackedPWaves newAmplitudes) {
    currentPWaveAmplitudes = newAmplitudes;
    notifyPropertyChanged(BR.currentPWaveAmplitudes);
  }

  /**
   * Gets the maximum captured p-wave amplitudes
   *
   * @return the amplitude values for the maximum p-wave
   */
  @Bindable
  public TrackedPWaves getMaxPWaveAmplitudes() {
    return maxPWaveAmplitudes;
  }

  /**
   * Sets the maximum captured p-wave amplitudes
   *
   * @param newAmplitudes the new amplitude values for the maximum p-wave
   */
  public void setMaxPWaveAmplitudes(final TrackedPWaves newAmplitudes) {
    maxPWaveAmplitudes = newAmplitudes;
    notifyPropertyChanged(BR.maxPWaveAmplitudes);
  }

  /**
   * Gets the captured p-wave amplitudes
   *
   * @return the amplitude values for the captured p-wave
   */
  @Bindable
  public TrackedPWaves getCapturedPWaveAmplitudes() {
    return capturedPWaveAmplitudes;
  }

  /**
   * Sets the captured p-wave amplitudes
   *
   * @param newAmplitudes the new amplitude values for the captured p-wave
   */
  public void setCapturedPWaveAmplitudes(final TrackedPWaves newAmplitudes) {
    capturedPWaveAmplitudes = newAmplitudes;
    notifyPropertyChanged(BR.capturedPWaveAmplitudes);
  }

  /**
   * Gets the most recent capture time
   *
   * @return the capture time
   */
  @Bindable
  public Date getCaptureTime() {
    return captureTime;
  }

  /**
   * Sets the most recent capture time
   *
   * @param newCaptureTime the new capture time
   */
  public void setCaptureTime(final Date newCaptureTime) {
    captureTime = newCaptureTime;
    notifyPropertyChanged(BR.captureTime);
  }

  public boolean isInvalidHeartRate() {
    return invalidHeartRate;
  }

  public void setInvalidHeartRate(final boolean hrValid) {
    invalidHeartRate = hrValid;
  }

  /**
   * Clear monitor data.
   */
  public void clearMonitorData() {
    setHeartRate(-1);
    setCurrentPWaveAmplitudes(new TrackedPWaves());
    setMaxPWaveAmplitudes(new TrackedPWaves());
    setCapturedPWaveAmplitudes(new TrackedPWaves());

  }

  /**
   * A type containing the amplitude in millivolts of a p-wave from different leads' perspectives.
   */
  public static class TrackedPWaves {
    /**
     * The amplitude of the external lead's detected p-wave
     */
    public double external_mV = 0;
    /**
     * The amplitude of the intravascular lead's detected p-wave
     */
    public double intravascular_mV = 0;

    /**
     * Instantiates a new default tracked p-wave object.
     */
    TrackedPWaves() {
    }

    /**
     * Instantiates a new Tracked p-wave object with declared amplitudes.
     *
     * @param ext_Mv the amplitude of the external lead's detected p-wave
     * @param int_Mv the amplitude of the internal lead's detected p-wave
     */
    public TrackedPWaves(final double ext_Mv, final double int_Mv) {
      external_mV = ext_Mv;
      intravascular_mV = int_Mv;
    }
  }
}
