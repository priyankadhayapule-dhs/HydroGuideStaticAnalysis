package com.accessvascularinc.hydroguide.mma.ui.captures;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.model.Capture;
import com.accessvascularinc.hydroguide.mma.ui.plot.PlotUtils;
import com.androidplot.xy.XYPlot;

import java.util.Locale;

public class IntravCaptureListItemViewHolder extends RecyclerView.ViewHolder {
  private final TextView tvCapId, tvInsertedLength, tvExposedLength;
  private final XYPlot captureGraph;
  private Capture capture = null;

  public IntravCaptureListItemViewHolder(@NonNull final View view) {
    super(view);
    tvCapId = itemView.findViewById(R.id.capture_id);
    tvInsertedLength = itemView.findViewById(R.id.capture_inserted_length);
    tvExposedLength = itemView.findViewById(R.id.capture_exposed_length);
    captureGraph = itemView.findViewById(R.id.capture_plot_graph);
  }

  @NonNull
  public static IntravCaptureListItemViewHolder create(@NonNull final ViewGroup parent) {
    return new IntravCaptureListItemViewHolder(LayoutInflater.from(parent.getContext())
            .inflate(R.layout.capture_layout, parent, false));
  }

  public void setCapture(final Capture newCapture) {
    this.capture = newCapture;

    tvCapId.setText(String.format("%s", newCapture.getCaptureId()));
    final String newInsText = String.format(Locale.US, "%.1f", newCapture.getInsertedLengthCm());
    final String newExtText = String.format(Locale.US, "%.1f", newCapture.getExposedLengthCm());
    tvInsertedLength.setText(String.format("%s cm", newInsText));
    tvExposedLength.setText(String.format("%s cm", newExtText));
    captureGraph.clear();
    PlotUtils.drawCaptureGraph(captureGraph, newCapture);
    PlotUtils.stylePlot(captureGraph);
  }
}