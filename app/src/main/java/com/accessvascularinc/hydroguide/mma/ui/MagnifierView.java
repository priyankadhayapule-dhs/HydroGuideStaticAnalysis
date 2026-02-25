package com.accessvascularinc.hydroguide.mma.ui;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;

import androidx.annotation.Nullable;

public class MagnifierView extends View {
  private Bitmap imageBitmap;
  private float touchX = -1;
  private float touchY = -1;
  private float displayX = -1; // Fixed display position X
  private float displayY = -1; // Fixed display position Y
  private boolean isShowing = false;
  private float magnifierRadius = 120f; // px
  private float magnification = 2.0f;
  private Paint borderPaint;
  private Path clipPath;
  private boolean useFixedPosition = true; // Always use fixed position at top-right
  private float marginFromEdge = 150f; // Margin from right and top edges
  
  // Store the source view dimensions for accurate coordinate mapping
  private int sourceViewWidth = 0;
  private int sourceViewHeight = 0;

  public MagnifierView(Context context) {
    super(context);
    init();
  }

  public MagnifierView(Context context, @Nullable AttributeSet attrs) {
    super(context, attrs);
    init();
  }

  public MagnifierView(Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
    super(context, attrs, defStyleAttr);
    init();
  }

  private void init() {
    setWillNotDraw(false); // Ensure onDraw is called
    borderPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    borderPaint.setStyle(Paint.Style.STROKE);
    borderPaint.setStrokeWidth(6f);
    borderPaint.setColor(0xFFFFFFFF); // White border
    clipPath = new Path();
    android.util.Log.d("MagnifierView", "init() called, setWillNotDraw(false) set");
  }
  
  private Paint getCrosshairPaint() {
    Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    paint.setColor(0xFFFFFF00); // Yellow
    paint.setStyle(Paint.Style.FILL);
    return paint;
  }

  public void setImageBitmap(Bitmap bitmap) {
    this.imageBitmap = bitmap;
    android.util.Log.d("MagnifierView", "setImageBitmap called: " + (bitmap != null ? bitmap.getWidth() + "x" + bitmap.getHeight() : "null"));
    invalidate();
  }
  
  public void setSourceViewDimensions(int width, int height) {
    this.sourceViewWidth = width;
    this.sourceViewHeight = height;
    android.util.Log.d("MagnifierView", "setSourceViewDimensions: " + width + "x" + height);
  }

  public void showMagnifier(float x, float y) {
    touchX = x;
    touchY = y;
    
    // Calculate fixed position at top-right corner
    if (useFixedPosition) {
      displayX = getWidth() - magnifierRadius - marginFromEdge;
      displayY = magnifierRadius + marginFromEdge;
    } else {
      displayX = x;
      displayY = y;
    }
    
    isShowing = true;
    android.util.Log.d("MagnifierView", "showMagnifier called: touch=(" + x + ", " + y + "), display=(" + displayX + ", " + displayY + "), bitmap=" + (imageBitmap != null) + ", visible=" + (getVisibility() == VISIBLE));
    invalidate();
  }

  public void hideMagnifier() {
    isShowing = false;
    invalidate();
  }

  @Override
  protected void onDraw(Canvas canvas) {
    super.onDraw(canvas);
      if (!isShowing || imageBitmap == null) {
          android.util.Log.d("MagnifierView", "onDraw skipped: isShowing=" + isShowing + ", imageBitmap=" + (imageBitmap != null));
          return;
      }
      
    android.util.Log.d("MagnifierView", "Drawing magnifier at display=(" + displayX + ", " + displayY + "), magnifying touch=(" + touchX + ", " + touchY + ")");

    float diameter = magnifierRadius * 2;
    float left = displayX - magnifierRadius;
    float top = displayY - magnifierRadius;

    // Calculate the source rect in the bitmap based on touch position
    // Use the actual bitmap dimensions and SOURCE VIEW dimensions for accurate mapping
    float scale = magnification;
    float srcRadius = magnifierRadius / scale;
    
    // Map touch coordinates to bitmap coordinates
    // Use source view dimensions if available, otherwise fall back to this view's dimensions
    float viewWidth = sourceViewWidth > 0 ? sourceViewWidth : getWidth();
    float viewHeight = sourceViewHeight > 0 ? sourceViewHeight : getHeight();
    
    float srcCenterX = touchX * imageBitmap.getWidth() / viewWidth;
    float srcCenterY = touchY * imageBitmap.getHeight() / viewHeight;
    
    android.util.Log.d("MagnifierView", "Coordinate mapping: touch=(" + touchX + "," + touchY + "), view=(" + viewWidth + "x" + viewHeight + "), bitmap=(" + imageBitmap.getWidth() + "x" + imageBitmap.getHeight() + "), src=(" + srcCenterX + "," + srcCenterY + ")");
    
    float srcLeft = srcCenterX - srcRadius;
    float srcTop = srcCenterY - srcRadius;
    float srcRight = srcCenterX + srcRadius;
    float srcBottom = srcCenterY + srcRadius;
    
    RectF srcRect = new RectF(srcLeft, srcTop, srcRight, srcBottom);
    RectF dstRect = new RectF(left, top, left + diameter, top + diameter);

    // Clip to circle at display position
    clipPath.reset();
    clipPath.addCircle(displayX, displayY, magnifierRadius, Path.Direction.CW);
    int save = canvas.save();
    canvas.clipPath(clipPath);
    canvas.drawBitmap(imageBitmap,
            new android.graphics.Rect(
                    Math.round(srcRect.left), Math.round(srcRect.top),
                    Math.round(srcRect.right), Math.round(srcRect.bottom)),
            dstRect,
            null);
    canvas.restoreToCount(save);

    // Draw border at display position
    canvas.drawCircle(displayX, displayY, magnifierRadius, borderPaint);
    
    // Draw yellow dot at center to indicate touch point
    Paint dotPaint = getCrosshairPaint();
    canvas.drawCircle(displayX, displayY, 8f, dotPaint);
    
    // Optional: Draw crosshair lines for better precision
    Paint crosshairPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    crosshairPaint.setColor(0xFFFFFF00); // Yellow
    crosshairPaint.setStrokeWidth(2f);
    crosshairPaint.setStyle(Paint.Style.STROKE);
    
    // Horizontal line
    canvas.drawLine(displayX - 15f, displayY, displayX + 15f, displayY, crosshairPaint);
    // Vertical line
    canvas.drawLine(displayX, displayY - 15f, displayX, displayY + 15f, crosshairPaint);
  }

  @Override
  public boolean onTouchEvent(MotionEvent event) {
    // Let parent handle touch events
    return false;
  }
}