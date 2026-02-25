package com.accessvascularinc.hydroguide.mma.ui.captures;

import android.support.annotation.NonNull;
import android.view.ViewGroup;

import androidx.recyclerview.widget.RecyclerView;

import com.accessvascularinc.hydroguide.mma.model.Capture;
import com.accessvascularinc.hydroguide.mma.ui.plot.FadeFormatter;

import java.util.Arrays;
import java.util.stream.IntStream;

public class IntravCaptureListAdapter
        extends RecyclerView.Adapter<IntravCaptureListItemViewHolder> {
  private Capture[] capturesList;

  public IntravCaptureListAdapter() {
    capturesList = new Capture[0];
  }

  public IntravCaptureListAdapter(final Capture[] dataSet) {
    capturesList = dataSet;
  }

  @NonNull
  @Override
  public IntravCaptureListItemViewHolder onCreateViewHolder(@NonNull final ViewGroup parent,
                                                            final int position) {
    return IntravCaptureListItemViewHolder.create(parent);
  }

  @Override
  public void onBindViewHolder(@NonNull final IntravCaptureListItemViewHolder vh,
                               final int position) {
    vh.setCapture(capturesList[position]);
  }

  @Override
  public int getItemCount() {
    return capturesList.length;
  }

  public void addCapture(final Capture newCapture) {
    final Capture[] resizedData = Arrays.copyOf(capturesList, capturesList.length + 1);
    resizedData[resizedData.length - 1] = newCapture;
    capturesList = resizedData;
    this.notifyItemInserted(capturesList.length - 1);
  }

  public void injectFomatter(final FadeFormatter fmt) {
    for (int i = 0; i < capturesList.length; i++) {
      if (capturesList[i].getFormatter() == null) {
        capturesList[i].setFormatter(fmt);
      }
    }
  }

  public void insertCaptureByNum(final Capture newCapture, final int capNum) {
    final int index = getIdxOfNextCapNum(capNum);
    final Capture[] resizedCaps = new Capture[capturesList.length + 1];
    if (capturesList.length > 0) {
      System.arraycopy(capturesList, 0, resizedCaps, 0, index);
      System.arraycopy(capturesList, index, resizedCaps, index + 1, capturesList.length - index);
    }
    resizedCaps[index] = newCapture;
    capturesList = resizedCaps;
    this.notifyItemInserted(index);
  }

  public void removeCaptureByNum(final int capNum) {
    final int index = getIdxFromCapNum(capNum);
    // Find index of first cap with higher capture number
    final Capture[] resizedCaps = new Capture[capturesList.length - 1];
    System.arraycopy(capturesList, 0, resizedCaps, 0, index);
    System.arraycopy(capturesList, index + 1, resizedCaps, index,
            (capturesList.length - index) - 1);
    capturesList = resizedCaps;
    this.notifyItemRemoved(index);
  }

  public void updateGraphBounds(final Number newLower, final Number newUpper) {
    for (int i = 0; i < getItemCount(); i++) {
      capturesList[i].setLowerBound(newLower);
      capturesList[i].setUpperBound(newUpper);
      this.notifyDataSetChanged();
    }
  }

  public int getIdxFromCapNum(final int capNum) {
    return IntStream.range(0, capturesList.length)
            .filter(i -> (capNum == capturesList[i].getCaptureId()))
            .findFirst() // first occurrence
            .orElse(-1); // No element found
  }

  public int getIdxOfNextCapNum(final int capNum) {
    return IntStream.range(0, capturesList.length)
            .filter(i -> (capNum < capturesList[i].getCaptureId()))
            .findFirst() // first occurrence
            .orElse(capturesList.length); // Return last index if this cap num is largest
  }
}