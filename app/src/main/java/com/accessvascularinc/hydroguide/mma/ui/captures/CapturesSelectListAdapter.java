package com.accessvascularinc.hydroguide.mma.ui.captures;

import android.content.Context;
import android.support.annotation.NonNull;
import android.view.ViewGroup;

import androidx.core.content.ContextCompat;
import androidx.recyclerview.widget.RecyclerView;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.model.Capture;
import com.accessvascularinc.hydroguide.mma.ui.input.OnCheckedItemClickListener;
import com.accessvascularinc.hydroguide.mma.ui.plot.FadeFormatter;

import java.io.File;
import java.util.Arrays;

public class CapturesSelectListAdapter
        extends RecyclerView.Adapter<CapturesSelectListItemViewHolder> {
  private final Capture[] capturesList;
  private final String[] ultrasoundPaths;
  private final boolean[] ultrasoundSelected;
  private final OnCheckedItemClickListener itemClickListener;
  private boolean useExposedLength = false;

  public CapturesSelectListAdapter(final Capture[] dataSet,
                                   final boolean useExpLength,
                                   final OnCheckedItemClickListener listener) {
    this(null, dataSet, useExpLength, listener);
  }

  public CapturesSelectListAdapter(final String[] ultrasoundPaths,
                                   final Capture[] dataSet,
                                   final boolean useExpLength,
                                   final OnCheckedItemClickListener listener) {
    this.ultrasoundPaths = (ultrasoundPaths != null) ? ultrasoundPaths : new String[0];
    this.ultrasoundSelected = new boolean[this.ultrasoundPaths.length];
    Arrays.fill(ultrasoundSelected, true); // All selected by default
    capturesList = dataSet.clone();
    useExposedLength = useExpLength;
    this.itemClickListener = listener;
  }

  @NonNull
  @Override
  public CapturesSelectListItemViewHolder onCreateViewHolder(@NonNull final ViewGroup parent,
                                                             final int position) {
    return CapturesSelectListItemViewHolder.create(parent);
  }

  @Override
  public void onBindViewHolder(@NonNull final CapturesSelectListItemViewHolder vh,
                               final int position) {
    if (position < ultrasoundPaths.length) {
      // Ultrasound capture
      String path = ultrasoundPaths[position];
      String fileName = new File(path).getName();
      vh.setUltrasound("US " + (position + 1), fileName, ultrasoundSelected[position]);
      vh.itemView.setOnClickListener(view -> vh.cbSelectCapture.toggle());
      vh.cbSelectCapture.setOnCheckedChangeListener(
              (compoundButton, isChecked) -> {
                ultrasoundSelected[position] = isChecked;
                // Notify listener about ultrasound selection change
                if (itemClickListener != null) {
                  itemClickListener.onCheckedItemClickListener(-1, isChecked);
                }
              });
    } else {
      // ECG capture
      final int ecgIndex = position - ultrasoundPaths.length;
      final Capture capture = capturesList[ecgIndex];
      vh.setCapture(capture, useExposedLength);
      vh.cbSelectCapture.setOnCheckedChangeListener(null);

      if (capture == null) {
        return;
      }
      vh.cbSelectCapture.setOnCheckedChangeListener(
              (compoundButton, isChecked) -> {
                if (compoundButton.isPressed()) {
                  itemClickListener.onCheckedItemClickListener(position, isChecked);
                }
              });
      vh.itemView.setOnClickListener(view -> vh.cbSelectCapture.toggle());
    }
  }

  @Override
  public int getItemCount() {
    return ultrasoundPaths.length + capturesList.length;
  }

  public String[] getSelectedUltrasoundPaths() {
    return java.util.stream.IntStream.range(0, ultrasoundPaths.length)
            .filter(i -> ultrasoundSelected[i])
            .mapToObj(i -> ultrasoundPaths[i])
            .toArray(String[]::new);
  }

  public void resetFormatter(final FadeFormatter fmt, final Context cntxt) {

    for (int i = 0; i < capturesList.length; i++) {
      if (capturesList[i].getIsIntravascular()) {
        fmt.getLinePaint().setColor(ContextCompat.getColor(cntxt, R.color.av_yellow));
      } else {
        fmt.getLinePaint().setColor(ContextCompat.getColor(cntxt, R.color.white));
      }
      capturesList[i].setFormatter(fmt);
    }
  }

  public Capture getCaptureFromIdx(final int index) {
    // If index is in ultrasound range, return null (not a Capture)
    if (index < ultrasoundPaths.length) {
      return null;
    }
    // Adjust index to account for ultrasound items
    int captureIndex = index - ultrasoundPaths.length;
    if (captureIndex >= 0 && captureIndex < capturesList.length) {
      return capturesList[captureIndex];
    }
    return null;
  }
}