package com.accessvascularinc.hydroguide.mma.model;

import androidx.databinding.BaseObservable;
import androidx.databinding.Bindable;

import com.accessvascularinc.hydroguide.mma.BR;

import java.math.BigDecimal;
import java.math.RoundingMode;

/**
 * A data binding object for UI elements that allow the user to modify various factors that
 * influence how charts are rendered in the procedure screen and resulting captures.
 */
public class ChartConfig extends BaseObservable {
  /**
   * The number of heartbeats to display in the live graph for auto modes.
   */
  public static final float DEFAULT_BEATS_IN_AUTO = 5;
  /**
   * The time in seconds to plotted by the live graph by default.
   */
  public static final float DEFAULT_TIME_SCALE = 6;
  private float timeScale = DEFAULT_TIME_SCALE;
  private AutoscaleAction timeScaleMode = AutoscaleAction.Manual;
  private float plotSpeed = 1;
  private AutoscaleAction verticalScaleMode = AutoscaleAction.Manual;
  private float verticalScale = 0.0f; // Amplitude is in mV.
  private float verticalOffset = 0.0f;

  /**
   * Gets the currently selected time scale mode.
   *
   * @return the current time scale mode.
   */
  @Bindable
  public AutoscaleAction getTimeScaleMode() {
    return timeScaleMode;
  }

  /**
   * Sets a newly selected time scale mode.
   *
   * @param newMode the new time scale mode to use.
   */
  public void setTimeScaleMode(final AutoscaleAction newMode) {
    timeScaleMode = newMode;
    notifyPropertyChanged(BR.timeScaleMode);
  }

  /**
   * Gets currently selected plot speed used to determine how fast a swipe is in the live graph.
   * Speed is a factor that ranges from 0.5x to 3x.
   *
   * @return the current speed set for plotting.
   */
  @Bindable
  public float getPlotSpeed() {
    return plotSpeed;
  }

  /**
   * Sets a plot speed used to determine how fast a swipe is in the live graph, and directly affects
   * the time scale use. Speed is a factor that ranges from 0.5x to 3x.
   *
   * @param newSpeed the new speed used for plotting.
   */
  public void setPlotSpeed(final float newSpeed) {
    plotSpeed = newSpeed;
    notifyPropertyChanged(BR.plotSpeed);
    final float newTimeScale = new BigDecimal(Float.toString(DEFAULT_TIME_SCALE / newSpeed))
            .setScale(1, RoundingMode.HALF_UP).floatValue();
    setTimeScale(newTimeScale);
  }

  /**
   * Gets time scale, which represents the time in seconds to being plotted by the live graph.
   *
   * @return the time in seconds that is plotted by the live graph.
   */
  @Bindable
  public float getTimeScale() {
    return timeScale;
  }

  /**
   * Sets time scale, which represents the time in seconds to be plotted by the live graph.
   *
   * @param newScale the time in seconds to be plotted by the live graph.
   */
  public void setTimeScale(final float newScale) {
    timeScale = newScale;
    notifyPropertyChanged(BR.timeScale);
  }

  /**
   * Gets vertical scale mode, which determines how the vertical bounds of ECG graphs are handled.
   *
   * @return the currently selected vertical scale mode.
   */
  @Bindable
  public AutoscaleAction getVerticalScaleMode() {
    return verticalScaleMode;
  }

  /**
   * Sets vertical scale mode, which determines how the vertical bounds of ECG graphs are handled.
   *
   * @param newMode the new mode scale mode for graphs to use.
   */
  public void setVerticalScaleMode(final AutoscaleAction newMode) {
    verticalScaleMode = newMode;
    notifyPropertyChanged(BR.verticalScaleMode);
  }

  /**
   * Gets how much ECG graphs' vertical bounds are adjusted to visually scale up or down the
   * waveform's amplitude
   *
   * @return the currently set vertical scale adjustment in mV to vertical bounds of ECG graphs.
   */
  @Bindable
  public float getVerticalScale() {
    return verticalScale;
  }

  /**
   * Sets how much in mV the ECG graph's vertical bounds are adjusted to visually scale up or down
   * the waveform's amplitude. This is mainly used to help display the plotted line if it appear too
   * small see well, or if parts of it are going outside the vertical bounds of the graph. Note that
   * this does not adjust actual mV values of ECG points, but simply tightens or inflates the
   * graph's y-range.
   *
   * @param newScale the new vertical scale adjustment to use in mV for the vertical bounds of ECG
   *                 graphs.
   */
  public void setVerticalScale(final float newScale) {
    if (verticalScale != newScale) {
      verticalScale = newScale;
      notifyPropertyChanged(BR.verticalScale);
    }
  }

  /**
   * Gets how much in mV the waveform is shifted up or down in ECG graphs.
   *
   * @return the currently used vertical shift adjustment in mV for ECG graphs.
   */
  @Bindable
  public float getVerticalOffset() {
    return verticalOffset;
  }

  /**
   * Sets how much in mV the waveform is shifted up or down in ECG graphs. This is mainly used to
   * better view the plotted line if parts of it are above or below the vertical bounds of the
   * graph. Note that this does not adjust actual mV values of ECG points, but simply positively or
   * negatively shifts the graph's y-range.
   *
   * @param newOffset the new vertical shift adjustment in mV to be used for ECG graphs.
   */
  public void setVerticalOffset(final float newOffset) {
    if (verticalOffset != newOffset) {
      verticalOffset = newOffset;
      notifyPropertyChanged(BR.verticalOffset);
    }
  }

  /**
   * Updates the plot speed and time scale of ECG graphs based on the patient's BPM, so that a
   * certain amount of beats are visible on the graph in auto scale modes. This is to account for
   * the fact that the heart rate during a procedure and across different procedures may fluctuate.
   *
   * @param bpm the patient's heart rate in BPM.
   */
  public void calcTimeScaleFromBpm(final double bpm) {
    final double beatDurationSecs = 60 / bpm;
    final double newSpeed = DEFAULT_TIME_SCALE / (beatDurationSecs * DEFAULT_BEATS_IN_AUTO);

    setPlotSpeed((float) newSpeed);
  }

  /**
   * This enum defines available auto scaling modes that can be applied to the live ECG graph during
   * a procedure to account for fluctuating heart rates.
   */
  public enum AutoscaleAction {
    /**
     * Swipe speed and time scale of the live ECG graph only change when the user manually adjusts
     * it during a procedure.
     */
    Manual,
    /**
     * Swipe speed and time scale of the live ECG graph is adjusted only once based on the patient's
     * heart rate when enabling this mode and stays constant afterwards.
     */
    AutoOnce,
    /**
     * Swipe speed and time scale of the live ECG graph is continuously adjusted at the end of every
     * swipe during the procedure. This is so the graph always shows the same number of beats no
     * matter how the patient's heart rate may change throughout the procedure.
     */
    AutoContinuous,
  }
}
