package com.accessvascularinc.hydroguide.mma.ui.plot;

import android.graphics.Canvas;
import android.util.Log;

import androidx.annotation.Nullable;

import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import com.androidplot.Plot;
import com.androidplot.PlotListener;
import com.androidplot.xy.OrderedXYSeries;

import java.util.Arrays;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.locks.ReentrantReadWriteLock;

/**
 * Primitive simulation of some kind of signal.  For this example, we'll pretend its an ecg.  This
 * class represents the data as a circular buffer; data is added sequentially from left to right.
 * When the end of the buffer is reached, it is reset back to 0 and simulated sampling continues.
 */
public class EcgDataSeries implements OrderedXYSeries, PlotListener {
  private final ConcurrentLinkedQueue<DataPoint> newEcgPoints = new ConcurrentLinkedQueue<>();
  private final ConcurrentLinkedQueue<PMarkPoint> newPeakPoints = new ConcurrentLinkedQueue<>();
  private final ReentrantReadWriteLock lock = new ReentrantReadWriteLock(true);
  private Number[] data, markedPoints;
  // Data is raw waveform data, markedPoints is amplitude = waveform data - amplitude
  private long timestampStart = -1; // offset in timestamp ms value and index
  private int latestIndex = 0, latestMarkedPointIndex = 0;
  private PlotUtils.SwipeListener swipeListener = null;

  public EcgDataSeries(final int dataSize) {
    data = new Number[dataSize];
    markedPoints = new Number[dataSize];

    Arrays.fill(data, null);
    Arrays.fill(markedPoints, null);
  }

  public EcgDataSeries(final Number[] ecgData, final Number[] markedPWavePoints) {
    this.data = ecgData.clone();
    this.markedPoints = markedPWavePoints.clone();
    this.findLatestPMarkIdx();
  }

  public void setSwipeListener(final PlotUtils.SwipeListener newSwipeListener) {
    this.swipeListener = newSwipeListener;
  }

  @Nullable
  @Override
  public XOrder getXOrder() {
    return null;
  }

  @Override
  public String getTitle() {
    return "";
  }

  @Override
  public void onBeforeDraw(final Plot source, final Canvas canvas) {
    // write-lock each active series for writes
    lock.readLock().lock();
  }

  @Override
  public void onAfterDraw(final Plot source, final Canvas canvas) {
    // unlock any locked series
    lock.readLock().unlock();
  }

  @Override
  public int size() {
    return data.length;
  }

  @Override
  public Number getX(final int index) {
    if (index >= data.length) {
      throw new IllegalArgumentException("Provided index larger than last position in data array.");
    }
    return index;
  }

  // getting the raw waveform value at the index
  @Override
  public Number getY(final int index) {
    if (index >= data.length) {
      throw new IllegalArgumentException("Provided index larger than last position in data array.");
    }
    return data[index];
  }

  // getting the pwave amplitude at the index (amplitude = raw - baseline) *in some cases, baseline may be negative
  public Number getMarker(final int index) {
    if (index >= markedPoints.length) {
      throw new IllegalArgumentException(
              "Provided index larger than last position in markedPoints array.");
    }
    return markedPoints[index];
  }

  // Gets raw waveform data
  public Number[] getData() {
    return data.clone();
  }

  // Gets waveform amplitude data at marked p-waves
  public Number[] getMarkedPoints() {
    return markedPoints.clone();
  }

  public boolean isPMarked(final int index) {
    if (index >= data.length) {
      throw new IllegalArgumentException("Provided index larger than last position in data array.");
    }
    return markedPoints[index] != null;
  }

  public void addDataPoint(final long timestamp, final Number newMv) {
    newEcgPoints.add(new DataPoint(timestamp, newMv));
  }

  public void addPMarkPoint(final long timestamp, final Number newMv, final boolean isPWaveMark) {
    newPeakPoints.add(new PMarkPoint(timestamp, newMv, isPWaveMark));
  }

  public int getLatestIndex() {
    return latestIndex;
  }

  public int getLatestMarkedPointIndex() {
    return latestMarkedPointIndex;
  }

  public void findLatestPMarkIdx() {
    for (int i = 0; i < markedPoints.length; i++) {
      if (markedPoints[i] != null) {
        this.latestMarkedPointIndex = i;
      }
    }
  }

  public void updateDataArrays() {
//    if (newEcgPoints.size() > 10){
//      Log.w("New ECG Points Queue", String.format("Size: %s", newEcgPoints.size()));
//    }
//    if (newPeakPoints.size() > 1){
//      Log.w("New Peak Points Queue", String.format("Size: %s", newPeakPoints.size()));
//    }
    while (newEcgPoints.peek() != null) {
      final DataPoint newEcgPoint = newEcgPoints.poll();
      if (newEcgPoint != null) {
        if (timestampStart == -1) {
          timestampStart = newEcgPoint.timestamp;
        }

        // ESG pulse causes timestamps to reset so it would cause negative indexes to be used
        // negative plotIdx value causes live plotting to appear to "stop"
        if (timestampStart > newEcgPoint.timestamp) {
          timestampStart = 0;
        }

        int plotIdx = (int)(newEcgPoint.timestamp - timestampStart) / 5;

        // Log.w("Plot Idx",String.format("%s ms", plotIdx));
        if (plotIdx >= data.length) {
          plotIdx = 0;
          timestampStart = newEcgPoint.timestamp;

          if (swipeListener != null) {
            swipeListener.onSwipeEnd();
          }
        }

        if (plotIdx >= 0) {
          data[plotIdx] = newEcgPoint.value;
          markedPoints[plotIdx] = null;
          latestIndex = plotIdx;
        }
      }
    }

    if (latestIndex < (data.length - 1)) {
      // null out the point following i, to disable connecting i and i+1 with a line:
      data[latestIndex + 1] = null;
      markedPoints[latestIndex + 1] = null;
    }

    while (newPeakPoints.peek() != null) {
      final PMarkPoint newPWavePoint = newPeakPoints.poll();
      if (newPWavePoint != null) {
        final int plotIdx = (int)(newPWavePoint.timestamp - timestampStart) / 5;

        if ((plotIdx > 0) && (plotIdx < markedPoints.length)) {
          markedPoints[plotIdx] = newPWavePoint.isPMarked ? newPWavePoint.value : null;
          Log.i("marker debugging",
                  "Add marker: " + plotIdx + ", " + newPWavePoint.value + ", " + data[plotIdx]);
          if (newPWavePoint.isPMarked) {
            latestMarkedPointIndex = plotIdx;
          }
        }
      }
    }
  }

  public void resize(final int newDataSize) {
    lock.writeLock().lock();
    try {
      updateDataArrays();
      if (data.length < newDataSize) { // if increasing in size
        final Number[] resizedData = Arrays.copyOf(data, newDataSize);
        final Number[] resizedMarkedPoints = Arrays.copyOf(markedPoints, newDataSize);

        for (int i = data.length; i < newDataSize; i++) {
          resizedData[i] = null;
          resizedMarkedPoints[i] = null;
        }

        data = resizedData;
        markedPoints = resizedMarkedPoints;
      } else if (data.length > newDataSize) { // if decreasing in size
        final int idxOffset = data.length - newDataSize; // always be positive
        final Number[] resizedData = Arrays.copyOfRange(data, idxOffset, data.length);
        final Number[] resizedMarkedPoints =
                Arrays.copyOfRange(markedPoints, idxOffset, data.length);
        data = resizedData;
        markedPoints = resizedMarkedPoints;
      } else {
        MedDevLog.warn("Resize ECG", "Data size is the same.");
      }
      this.findLatestPMarkIdx();
    } finally {
      lock.writeLock().unlock();
    }
  }

  public Number[] getRollingDataRange(final Number[] dataArray, final int latestIdx,
                                      final int desiredSize) {
    final int idxOffset = latestIdx - desiredSize;
    if (idxOffset < 0) {
      //final Number[] firstRange =
      //        Arrays.copyOfRange(dataArray, dataArray.length - 1 + idxOffset, dataArray.length - 1);
      //final Number[] secondRange = Arrays.copyOfRange(dataArray, 0, latestIdx);
      final Number[] combinedRange = new Number[desiredSize];
      System.arraycopy(dataArray, dataArray.length + idxOffset, combinedRange, 0, -idxOffset);
      System.arraycopy(dataArray, 0, combinedRange, -idxOffset, latestIdx);
      return combinedRange;
    } else {
      return Arrays.copyOfRange(dataArray, idxOffset, latestIdx);
    }
  }

  public void setData(final Number[] newData, final Number[] newMarkedPoints) {
    lock.writeLock().lock();
    try {
      resize(newData.length);
      data = newData.clone();
      markedPoints = newMarkedPoints.clone();
      this.findLatestPMarkIdx();
    } finally {
      lock.writeLock().unlock();
    }
  }

  public void clearData() {
    lock.writeLock().lock();
    try {
      newEcgPoints.clear();
      newPeakPoints.clear();
      Arrays.fill(data, null);
      Arrays.fill(markedPoints, null);
      timestampStart = -1;
      latestIndex = 0;
      latestMarkedPointIndex = 0;
    } finally {
      lock.writeLock().unlock();
    }
  }

  public EcgDataSeries getCopy() {
    final EcgDataSeries copy = new EcgDataSeries(data.length);
    copy.data = this.data;
    copy.markedPoints = this.markedPoints;
    copy.latestIndex = this.latestIndex;
    copy.latestMarkedPointIndex = this.latestMarkedPointIndex;
    return copy;
  }

  public EcgDataSeries getDataSnapshot(final int snapshotSize) {
    //To start off, just take latest third of data for test captures
    final Number[] dataSnapshot = getRollingDataRange(data, latestIndex, snapshotSize);
    final Number[] markedPointsSnapshot =
            getRollingDataRange(markedPoints, latestIndex, snapshotSize);
    final var sb = new StringBuilder("captured p-waves:");
    for (int i = 0; i < markedPointsSnapshot.length; ++i) {
      final var mark = markedPointsSnapshot[i];
      if (mark != null) {
        sb.append(" (").append(i).append(", ").append(mark).append("),");
      }
    }
    Log.i("marker debugging", sb.toString());
    final EcgDataSeries snapshot = new EcgDataSeries(dataSnapshot, markedPointsSnapshot);
    snapshot.findLatestPMarkIdx();
    return snapshot;
  }

  private static class DataPoint {
    final long timestamp;
    final Number value;

    DataPoint(final long time, final Number val) {
      timestamp = time;
      value = val;
    }
  }

  private static class PMarkPoint {
    final long timestamp;
    final Number value;
    final boolean isPMarked;

    PMarkPoint(final long time, final Number val, final boolean isPMark) {
      timestamp = time;
      value = val;
      isPMarked = isPMark;
    }
  }
}