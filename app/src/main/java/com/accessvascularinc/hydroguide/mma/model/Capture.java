package com.accessvascularinc.hydroguide.mma.model;

import android.util.Log;

import com.accessvascularinc.hydroguide.mma.ui.plot.EcgDataSeries;
import com.accessvascularinc.hydroguide.mma.ui.plot.FadeFormatter;

import org.json.JSONArray;
import org.json.JSONException;

import java.util.Objects;

/**
 * A capture represents a "snapshot" of the live ECG waveform being plotted in the procedure screen.
 * A capture can be of either the external or the intravascular waveform, and includes the data
 * points and graph configuration needed to draw a graph. Inserted and exposed lengths of the
 * catheter used in the procedure at the time of the capture are recorded and whether the data here
 * is visible in the final report.
 */
public class Capture {
  private int captureId = -1;
  private boolean isIntravascular = false; // This capture is designated as external by default.
  private boolean shownInReport = true;
  private Double insertedLengthCm;
  private Double exposedLengthCm;
  private EcgDataSeries captureData;
  private FadeFormatter formatter = null;
  private Number upperBound;
  private Number lowerBound;
  private double increment = 0;

  /**
   * Instantiates a new Capture.
   *
   * @param capId         The capture's ID number, which is unique in a procedure's report.
   * @param isIntrav      Designates whether this capture is of an intravascular waveform or not.
   * @param insLength     The inserted length of the catheter in cm.
   * @param expLength     The exposed length of the catheter in cm.
   * @param data          An instance of {@link EcgDataSeries} with the ECG data points that make up
   *                      the plotted line in the graph.
   * @param upBound       The maximum mV value for the upper y-range of the graph.
   * @param lowBound      The minimum mV value for the lower y-range of the graph.
   * @param plotIncrement The increments at which to draw horizontal guide lines on the graph.
   * @param fmt           An instance of {@link FadeFormatter} with visual information needed to
   *                      draw the graph, used mainly for the line color and unique crosshair.
   */
  public Capture(final int capId, final boolean isIntrav, final Double insLength,
                 final Double expLength, final EcgDataSeries data, final Number upBound,
                 final Number lowBound, final double plotIncrement, final FadeFormatter fmt) {
    this.captureId = capId;
    this.isIntravascular = isIntrav;
    this.insertedLengthCm = insLength;
    this.exposedLengthCm = expLength;
    this.captureData = data;
    this.upperBound = upBound;
    this.lowerBound = lowBound;
    this.increment = plotIncrement;
    this.formatter = fmt;
  }

  /**
   * Parses capture data as a JSON string to create a new instance. This is used for reading data
   * from a text file.
   *
   * @param dataString the JSON string that represents capture data.
   *
   * @throws JSONException a JSON exception for when the string provided does not result in a
   *                       JSONArray.
   */
  public Capture(final String dataString) throws JSONException {
    if (dataString.isEmpty()) {
      return;
    }
    final JSONArray dataStrArray = new JSONArray(dataString);
    captureId = dataStrArray.getInt(0);
    isIntravascular = dataStrArray.getBoolean(1);
    shownInReport = dataStrArray.getBoolean(2);
    insertedLengthCm = dataStrArray.getDouble(3);
    exposedLengthCm = dataStrArray.getDouble(4);

    final String[] dataArray = DataFiles.getArrayFromString(dataStrArray.getString(5));
    final Number[] dataPoints = new Number[dataArray.length];
    for (int i = 0; i < dataPoints.length; i++) {
      dataPoints[i] = Objects.equals(dataArray[i], "null") ? null :
              Double.parseDouble(dataArray[i]);
    }

    final String[] peaksArray = DataFiles.getArrayFromString(dataStrArray.getString(6));
    final Number[] peakPoints = new Number[peaksArray.length];
    for (int i = 0; i < peakPoints.length; i++) {
      peakPoints[i] = Objects.equals(peaksArray[i], "null") ? null :
              Double.parseDouble(peaksArray[i]);
    }
    captureData = new EcgDataSeries(dataPoints, peakPoints);

    upperBound = dataStrArray.getDouble(7);
    lowerBound = dataStrArray.getDouble(8);
    increment = dataStrArray.getDouble(9);
  }

  /**
   * Gets the capture's ID, which is unique in a procedure's report.
   *
   * @return The capture's ID number.
   */
  public int getCaptureId() {
    return captureId;
  }

  /**
   * Sets the capture's Id.
   *
   * @param newCapId The new Id number to be used.
   */
  public void setCaptureId(final int newCapId) {
    captureId = newCapId;
  }

  /**
   * Gets whether this capture is of an intravascular or external waveform.
   *
   * @return whether the capture is intravascular or not.
   */
  public boolean getIsIntravascular() {
    return isIntravascular;
  }

  /**
   * Sets whether this capture is of an intravascular or external waveform.
   *
   * @param isIntrav whether the capture is intravascular or not.
   */
  public void setIsIntravascular(final boolean isIntrav) {
    isIntravascular = isIntrav;
  }

  /**
   * Gets whether to show or hide this capture in the procedure's report.
   *
   * @return whether this capture is visible in the report.
   */
  public boolean getShowInReport() {
    return shownInReport;
  }

  /**
   * Sets whether to show or hide this capture in the procedure's report.
   *
   * @param isShown whether this capture is visible in the report.
   */
  public void setShownInReport(final boolean isShown) {
    shownInReport = isShown;
  }

  /**
   * Gets the inserted length of the catheter in cm.
   *
   * @return the inserted inserted length of the catheter in cm.
   */
  public double getInsertedLengthCm() {
    return (insertedLengthCm == null) ? 0.0 : insertedLengthCm;
  }

  /**
   * Sets the inserted length of the catheter in cm.
   *
   * @param newLength the new inserted length of the catheter in cm.
   */
  public void setInsertedLengthCm(final double newLength) {
    insertedLengthCm = newLength;
  }

  /**
   * Gets the exposed length of the catheter in cm.
   *
   * @return the exposed length of the catheter in cm.
   */
  public double getExposedLengthCm() {
    return (exposedLengthCm == null) ? 0.0 : exposedLengthCm;
  }

  /**
   * Sets the exposed length of the catheter in cm.
   *
   * @param newLength the new exposed length of the catheter in cm.
   */
  public void setExposedLengthCm(final double newLength) {
    exposedLengthCm = newLength;
  }

  /**
   * Gets an instance of {@link EcgDataSeries} with the ECG data points that make up the plotted
   * line in the graph.
   *
   * @return an instance of ECG data.
   */
  public EcgDataSeries getCaptureData() {
    return captureData;
  }

  /**
   * Sets an instance of {@link EcgDataSeries} with the ECG data points that make up the plotted
   * line in the graph.
   *
   * @param newCapData A new instance ECG data to use.
   */
  public void setCaptureData(final EcgDataSeries newCapData) {
    captureData = newCapData;
  }

  /**
   * Gets instance of {@link FadeFormatter} with visual information needed to draw the graph, used
   * mainly for the line color and unique crosshair.
   *
   * @return an instance formatter being used to draw the graph.
   */
  public FadeFormatter getFormatter() {
    return formatter;
  }

  /**
   * Sets instance of {@link FadeFormatter} with visual information needed to draw the graph, used
   * mainly for the line color and unique crosshair.
   *
   * @param newFormatter a new formatter to be used to draw the graph.
   */
  public void setFormatter(final FadeFormatter newFormatter) {
    formatter = newFormatter;
  }

  /**
   * Gets the maximum mV value for the upper y-range of the graph.
   *
   * @return the upper bound of the graph.
   */
  public Number getUpperBound() {
    return upperBound;
  }

  /**
   * Sets the maximum mV value for the upper y-range of the graph.
   *
   * @param newUpperBound the new upper bound of the graph.
   */
  public void setUpperBound(final Number newUpperBound) {
    upperBound = newUpperBound;
  }

  /**
   * Gets the minimum mV value for the lower y-range of the graph.
   *
   * @return the lower bound of the graph.
   */
  public Number getLowerBound() {
    return lowerBound;
  }

  /**
   * Sets the minimum mV value for the lower y-range of the graph.
   *
   * @param newLowerBound the new lower bound of the graph.
   */
  public void setLowerBound(final Number newLowerBound) {
    lowerBound = newLowerBound;
  }

  /**
   * Gets the increments at which to draw horizontal guide lines on the graph.
   *
   * @return the increment of the horizontal lines.
   */
  public double getIncrement() {
    return increment;
  }

  /**
   * Sets the increments at which to draw horizontal guide lines on the graph.
   *
   * @param newIncrement the new increment for horizontal lines.
   */
  public void setIncrement(final double newIncrement) {
    increment = newIncrement;
  }

  /**
   * Get the capture data as readable text.
   *
   * @return the data in this capture as a string.
   *
   * @throws JSONException a JSON exception when trying to format an array into a string.
   */
  public String[] getCaptureDataText() throws JSONException {
    String getData= new JSONArray(captureData.getData()).toString();
    String getMarkedPoints= new JSONArray(captureData.getMarkedPoints()).toString();
    Log.d("ABC", "Prasanna+getData: "+getData);
    Log.d("ABC", "Prasanna+getData: "+getMarkedPoints);
    Log.d("ABC", "Prasanna+upperBound: "+upperBound);
    Log.d("ABC", "Prasanna+lowerBound: "+lowerBound);
    Log.d("ABC", "Prasanna+increment: "+increment);
    return new String[] {
            Integer.toString(captureId),
            Boolean.toString(isIntravascular),
            Boolean.toString(shownInReport),
            Double.toString(insertedLengthCm),
            Double.toString(exposedLengthCm),
            new JSONArray(captureData.getData()).toString(),
            new JSONArray(captureData.getMarkedPoints()).toString(),
            upperBound.toString(),
            lowerBound.toString(),
            Double.toString(increment),
    };
  }
}
