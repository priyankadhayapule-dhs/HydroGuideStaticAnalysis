package com.accessvascularinc.hydroguide.mma.ui.plot;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PointF;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;

import com.androidplot.ui.RenderStack;
import com.androidplot.ui.SeriesRenderer;
import com.androidplot.util.PixelUtils;
import com.androidplot.xy.PointLabelFormatter;
import com.androidplot.xy.PointLabeler;
import com.androidplot.xy.XYPlot;
import com.androidplot.xy.XYRegionFormatter;
import com.androidplot.xy.XYSeriesFormatter;
import com.androidplot.xy.XYSeriesRenderer;

import java.util.Objects;

public class FadeRenderer extends XYSeriesRenderer<EcgDataSeries, FadeRenderer.Formatter> {
  private int latestIndex = 0;

  public FadeRenderer(final XYPlot plot) {
    super(plot);
  }

  @Override
  protected void onRender(final Canvas canvas, final RectF plotArea, final EcgDataSeries series,
                          final Formatter formatter, final RenderStack stack) {
    PointF thisPoint;
    PointF lastPoint = null;
    final boolean hasPointLabelFormatter = formatter.hasPointLabelFormatter();
    final PointLabelFormatter plf =
            hasPointLabelFormatter ? formatter.getPointLabelFormatter() : null;
    final PointLabeler<EcgDataSeries> pointLabeler =
            hasPointLabelFormatter ? formatter.getPointLabeler() : null;

    series.updateDataArrays();
    setLatestIndex(series.getLatestIndex());

    for (int i = 0; i < series.size(); i++) {
      final Number y = series.getY(i);
      final Number x = series.getX(i);

      if ((y != null) && (x != null)) {
        thisPoint = getPlot().getBounds().transformScreen(x, y, plotArea);
      } else {
        thisPoint = null;
      }

      if (thisPoint != null) {
        if ((formatter.getLinePaint() != null) && (lastPoint != null)) {
          canvas.drawLine(lastPoint.x, lastPoint.y, thisPoint.x, thisPoint.y,
                  formatter.getLinePaint(i, latestIndex, series.size()));
        }

        // Only draw point and label if marked as p-wave.
        // Right now its possible for points and labels to be rendered behind the line,
        // May need to update if they need to always be on top, which might impact performance
        if (series.isPMarked(i)) {
          if (formatter.getVertexPaint() != null) {
            final float iconWidth = formatter.getPointIcon().getWidth();
            final float iconHeight = formatter.getPointIcon().getHeight();
            canvas.drawPoint(thisPoint.x, thisPoint.y,
                    formatter.getVertexPaint(i, latestIndex, series.size()));
            canvas.drawBitmap(formatter.getPointIcon(), (thisPoint.x - (iconWidth / 2)),
                    (thisPoint.y - iconHeight),
                    formatter.getVertexPaint(i, latestIndex, series.size()));
          }

          if (formatter.hasPointLabelFormatter() && (i == series.getLatestMarkedPointIndex())) {
            // modify y-param with higher positive offset if 'P' needs to be below the line
            canvas.drawText(Objects.requireNonNull(pointLabeler).getLabel(series, i),
                    (thisPoint.x + plf.hOffset), (thisPoint.y + (plf.vOffset * 5)),
                    formatter.getTextPaint(i, latestIndex, series.size()));
          }
        }
      }
      lastPoint = thisPoint;
    }

    // Draw horizontal pink line at captured P-wave amplitude (set when capture button is clicked)
    if (formatter.isShowHorizontalLines() && formatter.getCapturedPWaveAmplitude() != null &&
            formatter.getPWaveLinePaint() != null) {
      final float capturedY = getPlot().getBounds().transformScreen(
              0, formatter.getCapturedPWaveAmplitude(), plotArea).y;
      canvas.drawLine(plotArea.left, capturedY, plotArea.right, capturedY,
              formatter.getPWaveLinePaint());
    }

    // Draw horizontal blue line at maximum P-wave amplitude (from MonitorData)
    if (formatter.isShowHorizontalLines() && formatter.getMaxPWaveAmplitude() != null &&
            formatter.getMaxPeakLinePaint() != null) {
      final float maxY = getPlot().getBounds().transformScreen(
              0, formatter.getMaxPWaveAmplitude(), plotArea).y;
      canvas.drawLine(plotArea.left, maxY, plotArea.right, maxY,
              formatter.getMaxPeakLinePaint());
    }
  }

  @Override
  protected void doDrawLegendIcon(final Canvas canvas, final RectF rect,
                                  final Formatter formatter) {
    if (formatter.getLinePaint() != null) {
      canvas.drawLine(rect.left, rect.bottom, rect.right, rect.top, formatter.getLinePaint());
    }
  }

  public void setLatestIndex(final int newLatestIndex) {
    this.latestIndex = newLatestIndex;
  }

  public static class Formatter extends XYSeriesFormatter<XYRegionFormatter> {
    private static final int DEFAULT_STROKE_WIDTH = 3;
    // instantiate a default implementation prints point's yVal:
    private final PointLabeler<EcgDataSeries> pointLabeler = (series, index) -> "P";
    private Paint linePaint, vertexPaint = null, textPaint = null, pWaveLinePaint = null, maxPeakLinePaint = null;
    private Bitmap pointIcon = null;
    private int trailSize = 0;
    private Number capturedPWaveAmplitude = null; // Stores captured P-wave amplitude from last capture
    private Number maxPWaveAmplitude = null; // Stores maximum P-wave amplitude from MonitorData
    private boolean showHorizontalLines = false; // Only show lines for intravascular mode

    public Formatter() {
      linePaint = new Paint();
      linePaint.setStrokeWidth(DEFAULT_STROKE_WIDTH);
      linePaint.setColor(Color.RED);
      linePaint.setAntiAlias(false);
      initVertexPaint(Color.CYAN);
      initTextPaint(Color.YELLOW);
      initPWaveLinePaint();
      initMaxPeakLinePaint();
    }

    public Formatter(final Context context, final int xmlConfigId) {
      this();
      configure(context, xmlConfigId);
    }

    @Override
    public Class<? extends SeriesRenderer<XYPlot, EcgDataSeries, Formatter>> getRendererClass() {
      return FadeRenderer.class;
    }

    @Override
    public FadeRenderer doGetRendererInstance(final XYPlot plot) {
      return new FadeRenderer(plot);
    }

    public int getTrailSize() {
      return trailSize;
    }

    public void setTrailSize(final int newSize) {
      trailSize = newSize;
    }

    public Bitmap getPointIcon() {
      return pointIcon;
    }

    public void setPointIcon(final Drawable drawable) {
      final Bitmap bitmap = Bitmap.createBitmap(drawable.getIntrinsicWidth(),
              drawable.getIntrinsicHeight(), Bitmap.Config.ARGB_8888);
      final Canvas canvas = new Canvas(bitmap);
      drawable.setBounds(0, 0, canvas.getWidth(), canvas.getHeight());
      drawable.draw(canvas);

      pointIcon = bitmap;
    }

    public Paint getLinePaint() {
      return linePaint;
    }

    public void setLinePaint(final Paint newLinePaint) {
      this.linePaint = newLinePaint;
    }

    public Paint getLinePaint(final int thisIndex, final int latestIndex, final int seriesSize) {
      return getLinePaint();
    }

    public Paint getVertexPaint(final int thisIndex, final int latestIndex, final int seriesSize) {
      return getVertexPaint();
    }

    public Paint getTextPaint(final int thisIndex, final int latestIndex, final int seriesSize) {
      return getTextPaint();
    }

    protected void initVertexPaint(final Integer vertexColor) {
      if (vertexColor == null) {
        vertexPaint = null;
      } else {
        vertexPaint = new Paint();
        vertexPaint.setStrokeWidth(PixelUtils.dpToPix(5));
        vertexPaint.setColor(vertexColor);
        vertexPaint.setStrokeCap(Paint.Cap.ROUND);
        vertexPaint.setAntiAlias(false);
      }
    }

    public Paint getVertexPaint() {
      if (vertexPaint == null) {
        initVertexPaint(Color.TRANSPARENT);
      }
      return vertexPaint;
    }

    public void setVertexPaint(final Paint newVertexPaint) {
      this.vertexPaint = newVertexPaint;
    }

    public boolean hasVertexPaint() {
      return vertexPaint != null;
    }

    public Paint getTextPaint() {
      if (textPaint == null) {
        initTextPaint(Color.TRANSPARENT);
      }
      return textPaint;
    }

    public void setTextPaint(final Paint newTextPaint) {
      this.textPaint = newTextPaint;
    }

    protected void initTextPaint(final Integer textColor) {
      if (textColor == null) {
        textPaint = null;
      } else {
        textPaint = new Paint();
        textPaint.setColor(textColor);
        textPaint.setTextAlign(Paint.Align.CENTER);
        textPaint.setTextSize(PixelUtils.spToPix(18));
        textPaint.setAntiAlias(false);
      }
    }

    public PointLabeler<EcgDataSeries> getPointLabeler() {
      return pointLabeler;
    }

    protected void initPWaveLinePaint() {
      pWaveLinePaint = new Paint();
      pWaveLinePaint.setStrokeWidth(3);
      pWaveLinePaint.setColor(0xFFFFA6E4); // Pink color
      pWaveLinePaint.setAntiAlias(true);
    }

    public Paint getPWaveLinePaint() {
      if (pWaveLinePaint == null) {
        initPWaveLinePaint();
      }
      return pWaveLinePaint;
    }

    public void setPWaveLinePaint(final Paint newPWaveLinePaint) {
      this.pWaveLinePaint = newPWaveLinePaint;
    }

    protected void initMaxPeakLinePaint() {
      maxPeakLinePaint = new Paint();
      maxPeakLinePaint.setStrokeWidth(3);
      maxPeakLinePaint.setColor(0xFF91CDFF); // Light blue color
      maxPeakLinePaint.setAntiAlias(true);
    }

    public Paint getMaxPeakLinePaint() {
      if (maxPeakLinePaint == null) {
        initMaxPeakLinePaint();
      }
      return maxPeakLinePaint;
    }

    public void setMaxPeakLinePaint(final Paint newMaxPeakLinePaint) {
      this.maxPeakLinePaint = newMaxPeakLinePaint;
    }

    public Number getCapturedPWaveAmplitude() {
      return capturedPWaveAmplitude;
    }

    public void setCapturedPWaveAmplitude(final Number amplitude) {
      this.capturedPWaveAmplitude = amplitude;
    }

    public boolean isShowHorizontalLines() {
      return showHorizontalLines;
    }

    public void setShowHorizontalLines(final boolean show) {
      this.showHorizontalLines = show;
    }

    public Number getMaxPWaveAmplitude() {
      return maxPWaveAmplitude;
    }

    public void setMaxPWaveAmplitude(final Number amplitude) {
      this.maxPWaveAmplitude = amplitude;
    }
  }
}

