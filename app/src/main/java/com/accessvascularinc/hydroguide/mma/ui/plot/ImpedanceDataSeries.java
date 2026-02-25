package com.accessvascularinc.hydroguide.mma.ui.plot;

import android.graphics.Canvas;

import androidx.annotation.NonNull;

import com.accessvascularinc.hydroguide.mma.serial.ImpedanceSample;
import com.androidplot.Plot;
import com.androidplot.PlotListener;
import com.androidplot.xy.OrderedXYSeries;

import java.util.ArrayList;
import java.util.List;

public class ImpedanceDataSeries implements OrderedXYSeries, PlotListener {

  private int MAX_NUM_IMPEDANCE_POINTS_TO_AVG = 50;
  private List<ImpedanceDataPoint> data = new ArrayList<ImpedanceDataPoint>();

  public void addDataPoint(long timestamp, ImpedanceSample newImpedanceSample) {
    data.add(new ImpedanceDataPoint(timestamp, newImpedanceSample));
  }

  @Override
  public int size() {
    return data.size();
  }

  @Override
  public Number getX(int index) {
    return data.get(index).calculateX();
  }

  @Override
  public Number getY(int index) {
    return data.get(index).calculateY();
  }

  @Override
  public void onBeforeDraw(Plot source, Canvas canvas) {

  }

  @Override
  public void onAfterDraw(Plot source, Canvas canvas) {

  }

  @Override
  public XOrder getXOrder() {
    return null;
  }

  @Override
  public String getTitle() {
    return "";
  }

  public String getDataPointString(int index) {
    return data.get(index).toString();
  }

  public long getAverageRA() {
    int count = Math.min(MAX_NUM_IMPEDANCE_POINTS_TO_AVG, data.size());
    if (count == 0) return 0;
    long sum = 0;
    for (int i = data.size() - count; i < data.size(); i++) {
      sum += data.get(i).sample.impedanceRAWhite_milliohms;
    }
    return sum / count;
  }

  public long getAverageLA() {
    int count = Math.min(MAX_NUM_IMPEDANCE_POINTS_TO_AVG, data.size());
    if (count == 0) return 0;
    long sum = 0;
    for (int i = data.size() - count; i < data.size(); i++) {
      sum += data.get(i).sample.impedanceLABlack_milliohms;
    }
    return sum / count;
  }

  public long getAverageLL() {
    int count = Math.min(MAX_NUM_IMPEDANCE_POINTS_TO_AVG, data.size());
    if (count == 0) return 0;
    long sum = 0;
    for (int i = data.size() - count; i < data.size(); i++) {
      sum += data.get(i).sample.impedanceLLRed_milliohms;
    }
    return sum / count;
  }

  private static class ImpedanceDataPoint {
    final long timestamp;
    final ImpedanceSample sample;

    ImpedanceDataPoint(final long time, final ImpedanceSample value) {
      timestamp = time;
      sample = value;
    }

    /*
      Algorithm for calculating X and Y from 3 impedance sensors. These are expecting kiloohms.
     */
    public double calculateX() {
      return (sample.impedanceRAWhite_milliohms * -0.0742) + (sample.impedanceLLRed_milliohms * 0.8360) + (sample.impedanceLABlack_milliohms * -0.85327) + 8.6984;
    }
    public double calculateY() {
      return (sample.impedanceRAWhite_milliohms * -0.2400) + (sample.impedanceLLRed_milliohms * 0.0580) + (sample.impedanceLABlack_milliohms * -0.0140) + 17.4143;
    }

    @NonNull
    @Override
    public String toString() {
      return "ImpedanceDataPoint{" +
              "timestamp=" + timestamp +
              ", impedanceRAWhite_Kohms=" + sample.impedanceRAWhite_milliohms +
              ", impedanceLABlack_Kohms=" + sample.impedanceLABlack_milliohms +
              ", impedanceLLRed_Kohms=" + sample.impedanceLLRed_milliohms +
              '}';
    }
  }
}
