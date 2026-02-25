package com.accessvascularinc.hydroguide.mma.ui.plot;

import android.graphics.Paint;

import com.androidplot.xy.AdvancedLineAndPointRenderer;

/**
 * Original: Special {@link AdvancedLineAndPointRenderer.Formatter} that draws a line that fades
 * over time.  Designed to be used in conjunction with a circular buffer model. - Modified to use
 * custom renderer.
 */

public class FadeFormatter extends FadeRenderer.Formatter {
  private final boolean enableFade;

  public FadeFormatter() {
    enableFade = false;
  }

  public FadeFormatter(final int trailSize) {
    enableFade = true;
    this.setTrailSize(trailSize);
  }

  @Override
  public Class<FadeRenderer> getRendererClass() {
    return FadeRenderer.class;
  }

  @Override
  public Paint getLinePaint(final int thisIndex, final int latestIndex, final int seriesSize) {
    final int alpha = getFadeAlpha(thisIndex, latestIndex, seriesSize);
    getLinePaint().setAlpha(Math.max(alpha, 0));
    return getLinePaint();
  }

  @Override
  public Paint getVertexPaint(final int thisIndex, final int latestIndex, final int seriesSize) {
    final int alpha = getFadeAlpha(thisIndex, latestIndex, seriesSize);
    getVertexPaint().setAlpha(Math.max(alpha, 0));
    return getVertexPaint();
  }

  @Override
  public Paint getTextPaint(final int thisIndex, final int latestIndex, final int seriesSize) {
    final int alpha = getFadeAlpha(thisIndex, latestIndex, seriesSize);
    getTextPaint().setAlpha(Math.max(alpha, 0));
    return getTextPaint();
  }

  public int getFadeAlpha(final int thisIndex, final int latestIndex, final int seriesSize) {
    if (!enableFade) {
      return 255;
    }
    // offset from the latest index:
    final int offset;
    if (thisIndex > latestIndex) {
      offset = latestIndex + (seriesSize - thisIndex);
    } else {
      offset = latestIndex - thisIndex;
    }

    final float scale = 255.0f / getTrailSize();
    return (int) (255 - (offset * scale));
  }
}
