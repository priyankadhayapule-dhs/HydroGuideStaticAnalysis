package com.accessvascularinc.hydroguide.mma.ui.plot;

import android.content.Context;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.view.View;

import androidx.core.content.ContextCompat;

import com.accessvascularinc.hydroguide.mma.R;
import com.accessvascularinc.hydroguide.mma.databinding.CaptureLayoutBinding;
import com.accessvascularinc.hydroguide.mma.model.Capture;
import com.androidplot.ui.HorizontalPositioning;
import com.androidplot.ui.Size;
import com.androidplot.ui.SizeMode;
import com.androidplot.ui.VerticalPositioning;
import com.androidplot.xy.BoundaryMode;
import com.androidplot.xy.PointLabelFormatter;
import com.androidplot.xy.StepMode;
import com.androidplot.xy.XYPlot;

import java.util.Locale;
import java.util.Objects;

public class PlotUtils {
  public static final int POINTS_PER_SECOND = 200;
  public static final int CAPTURE_SIZE = (int) (2.5 * POINTS_PER_SECOND);
  public static final float GRAPH_LOWER_BOUND_DEFAULT = -0.5f;
  public static final float GRAPH_UPPER_BOUND_DEFAULT = 1.5f;
  public static final float GRAPH_RANGE_INCREMENT_DEFAULT = 0.5f;

  public static void stylePlot(final XYPlot plot) {
    // style so entire container is used by plot area
    plot.getGraph().getRangeGridLinePaint().setAlpha(80);
    plot.getGraph().setMargins(0, 0, 0, 0);
    plot.getGraph().setSize(new Size(0, SizeMode.FILL, 0, SizeMode.FILL));
    plot.getGraph().position(0, HorizontalPositioning.ABSOLUTE_FROM_LEFT, 0,
        VerticalPositioning.ABSOLUTE_FROM_TOP);
  }

  public static FadeFormatter getFormatter(final Context ctx, final PlotStyle style,
      final int trailSize) {
    final FadeFormatter formatter;
    formatter = (trailSize > 0) ? new FadeFormatter(trailSize) : new FadeFormatter();
    formatter.setLegendIconEnabled(false);
    formatter.setPointLabelFormatter(new PointLabelFormatter());

    switch (style) {
      case External -> {
        formatter.getLinePaint().setColor(ContextCompat.getColor(ctx, R.color.white));
        formatter.setPointIcon(Objects.requireNonNull(
            ContextCompat.getDrawable(ctx, R.drawable.indicator_crosshair)));
      }
      case Intravascular -> {
        formatter.getLinePaint().setColor(ContextCompat.getColor(ctx, R.color.av_yellow));
        formatter.setPointIcon(Objects.requireNonNull(
            ContextCompat.getDrawable(ctx, R.drawable.indicator_crosshair)));
      }
      case Report -> {
        formatter.getLinePaint().setColor(ContextCompat.getColor(ctx, R.color.black));
        final Drawable icon = ContextCompat.getDrawable(ctx, R.drawable.indicator_crosshair_black);
        Objects.requireNonNull(icon).setTint(Color.BLACK);
        formatter.setPointIcon(icon);

        // Adjust to fit in US letter size.
        formatter.getLinePaint().setStrokeWidth(4);
        formatter.getVertexPaint().setStrokeWidth(10);
        formatter.getVertexPaint().setColor(Color.GRAY);
        formatter.getTextPaint().setTextSize(42);
        formatter.getTextPaint().setColor(Color.BLACK);
      }
    }

    return formatter;
  }

  public static void drawCaptureGraph(final XYPlot plot, final Capture capture) {
    final EcgDataSeries capSeries = new EcgDataSeries(capture.getCaptureData().getData(),
        capture.getCaptureData().getMarkedPoints());
    plot.addSeries(capSeries, capture.getFormatter());
    plot.setRangeBoundaries(capture.getLowerBound(), capture.getUpperBound(), BoundaryMode.FIXED);
    plot.setRangeStep(StepMode.INCREMENT_BY_VAL, capture.getIncrement());
    plot.setDomainBoundaries(0, capSeries.size(), BoundaryMode.FIXED);
  }

  public static void setupExternalCapture(final CaptureLayoutBinding layout,
      final Capture extCapture,
      final boolean useExposedLength) {
    layout.captureIcon.setImageResource(R.drawable.logo_external);
    layout.captureId.setText("");
    layout.captureHeaderLabel.setVisibility(useExposedLength ? View.INVISIBLE : View.VISIBLE);
    layout.captureInsertedLength.setVisibility(useExposedLength ? View.INVISIBLE : View.VISIBLE);
    layout.captureFooterLabel.setVisibility(useExposedLength ? View.VISIBLE : View.INVISIBLE);
    layout.captureExposedLength.setVisibility(useExposedLength ? View.VISIBLE : View.INVISIBLE);

    if (extCapture != null) {
      if (useExposedLength) {
        final String newExtText = String.format(Locale.US, "%.1f", extCapture.getExposedLengthCm());
        layout.captureExposedLength.setText(String.format("%s cm", newExtText));
      } else {
        final String newInsText = String.format(Locale.US, "%.1f", extCapture.getInsertedLengthCm());
        layout.captureInsertedLength.setText(String.format("%s cm", newInsText));
      }

      if (extCapture.getFormatter() == null) {
        extCapture.setFormatter(getFormatter(layout.getRoot().getContext(), PlotStyle.External, 0));
      }
    }

    layout.capturePlotGraph.setRangeBoundaries(GRAPH_LOWER_BOUND_DEFAULT, GRAPH_UPPER_BOUND_DEFAULT,
        BoundaryMode.FIXED);
    layout.capturePlotGraph.setRangeStep(StepMode.INCREMENT_BY_VAL, GRAPH_RANGE_INCREMENT_DEFAULT);
    layout.capturePlotGraph.setDomainBoundaries(0, CAPTURE_SIZE, BoundaryMode.FIXED);
    stylePlot(layout.capturePlotGraph);
  }

  public enum PWaveChange {
    External,
    Current,
    LastCapture,
    Maximum,
  }

  @FunctionalInterface
  public interface SwipeListener {
    void onSwipeEnd();
  }
}
