package com.accessvascularinc.hydroguide.mma.ui;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;

/**
 * Custom view to display depth ruler/scale for ultrasound imaging
 * Based on EchoNous app RulerView implementation
 */
public class RulerView extends View {
    private static final String TAG = "RulerView";
    
    private Paint textPaint;
    private Paint linePaint;
    private Paint lineSmallPaint;
    
    private float minValue = 0f;  // Minimum depth in mm
    private float maxValue = 40f; // Maximum depth in mm (default 4cm)
    private float dpmm = 0f;      // Dots (pixels) per millimeter
    private int totalVisibleCM = 4;
    
    private static final float TEXT_SIZE = 24f;
    private static final float LINE_WIDTH_LARGE = 4f;
    private static final float LINE_WIDTH_SMALL = 2f;
    private static final int RULER_COLOR = Color.WHITE;
    
    public RulerView(Context context) {
        super(context);
        init();
    }
    
    public RulerView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }
    
    public RulerView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }
    
    private void init() {
        // Text paint for numbers
        textPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        textPaint.setColor(RULER_COLOR);
        textPaint.setTextSize(TEXT_SIZE);
        textPaint.setTextAlign(Paint.Align.RIGHT);
        
        // Paint for major lines (cm markers)
        linePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        linePaint.setColor(RULER_COLOR);
        linePaint.setStrokeWidth(LINE_WIDTH_LARGE);
        
        // Paint for minor lines (mm markers)
        lineSmallPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        lineSmallPaint.setColor(RULER_COLOR);
        lineSmallPaint.setStrokeWidth(LINE_WIDTH_SMALL);
    }
    
    /**
     * Set min/max depth values in millimeters
     * @param minValue Minimum depth (mm)
     * @param maxValue Maximum depth (mm)
     */
    public void setMinMaxMMValues(float minValue, float maxValue) {
        this.minValue = minValue;
        this.maxValue = maxValue;
        
        // Calculate dots per millimeter based on view height
        int pixelHeight = getHeight();
        if (pixelHeight > 0) {
            float depthRange = maxValue - minValue;
            if (depthRange > 0) {
                // Dynamic DPMM: ruler density adjusts so ruler height matches ultrasound image
                // At all depths, ruler will span the full available height
                dpmm = pixelHeight / depthRange;
                totalVisibleCM = (int) Math.ceil(depthRange / 10f);
                Log.d(TAG, "Dynamic DPMM: " + dpmm + 
                           ", Range: " + minValue + "-" + maxValue + "mm, Visible: " + totalVisibleCM + "cm");
            }
        }
        
        invalidate(); // Trigger redraw
    }
    
    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        super.onSizeChanged(w, h, oldw, oldh);
        // Recalculate DPMM when size changes
        if (h > 0) {
            float depthRange = maxValue - minValue;
            if (depthRange > 0) {
                dpmm = h / depthRange;
                Log.d(TAG, "Size changed - Height: " + h + ", DPMM: " + dpmm);
            }
        }
    }
    
    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        
        if (dpmm <= 0) {
            return; // Not initialized yet
        }
        
        int height = getHeight();
        int width = getWidth();
        float depthRange = maxValue - minValue;
        
        // Adaptive rendering based on visible depth
        if (totalVisibleCM > 12) {
            // > 12 cm: Show markers every 5cm
            drawScaleEvery5CM(canvas, width, height);
        } else if (totalVisibleCM >= 5) {
            // 5-12 cm: Show every 1cm + 5mm subdivisions
            drawScaleEvery1CMWith5MM(canvas, width, height);
        } else {
            // < 5 cm: Show every 1cm + 1mm subdivisions
            drawScaleEvery1CMWith1MM(canvas, width, height);
        }
    }
    
    /**
     * Draw scale markers every 5cm (for deep imaging > 12cm)
     */
    private void drawScaleEvery5CM(Canvas canvas, int width, int height) {
        int cmCount = 0;
        for (float depth = minValue; depth <= maxValue; depth += 10f) { // Every 1cm
            float y = (depth - minValue) * dpmm;
            
            if (y >= 0 && y <= height) {
                if (cmCount % 5 == 0) {
                    // Every 5cm: Draw major line and text
                    canvas.drawLine(width - 50, y, width, y, linePaint);
                    canvas.drawText(cmCount + "", width - 55, y + TEXT_SIZE / 3, textPaint);
                    Log.d(TAG, "Drew 5cm marker at " + cmCount + "cm, y=" + y);
                }
            }
            cmCount++;
        }
    }
    
    /**
     * Draw scale markers every 1cm with 5mm subdivisions (for 5-12cm depth)
     */
    private void drawScaleEvery1CMWith5MM(Canvas canvas, int width, int height) {
        int cmCount = 0;
        for (float depth = minValue; depth <= maxValue; depth += 5f) { // Every 5mm
            float y = (depth - minValue) * dpmm;
            
            if (y >= 0 && y <= height) {
                boolean isCM = (Math.round(depth) % 10) == 0;
                
                if (isCM) {
                    // Every 1cm: Draw major line (8mm) and text
                    canvas.drawLine(width - 50, y, width, y, linePaint);
                    canvas.drawText(cmCount + "", width - 55, y + TEXT_SIZE / 3, textPaint);
                    Log.d(TAG, "Drew 1cm marker at " + cmCount + "cm, y=" + y);
                    cmCount++;
                } else {
                    // Every 5mm: Draw minor line (4mm)
                    canvas.drawLine(width - 30, y, width, y, lineSmallPaint);
                }
            }
        }
    }
    
    /**
     * Draw scale markers every 1cm with 1mm subdivisions (for < 5cm depth)
     */
    private void drawScaleEvery1CMWith1MM(Canvas canvas, int width, int height) {
        int cmCount = 0;
        for (float depth = minValue; depth <= maxValue; depth += 1f) { // Every 1mm
            float y = (depth - minValue) * dpmm;
            
            if (y >= 0 && y <= height) {
                boolean isCM = (Math.round(depth) % 10) == 0;
                boolean is5MM = (Math.round(depth) % 5) == 0;
                
                if (isCM) {
                    // Every 1cm: Draw major line (8mm) and text
                    canvas.drawLine(width - 50, y, width, y, linePaint);
                    canvas.drawText(cmCount + "", width - 55, y + TEXT_SIZE / 3, textPaint);
                    cmCount++;
                } else if (is5MM) {
                    // Every 5mm: Draw medium line (4mm)
                    canvas.drawLine(width - 30, y, width, y, lineSmallPaint);
                } else {
                    // Every 1mm: Draw small line (2mm)
                    canvas.drawLine(width - 20, y, width, y, lineSmallPaint);
                }
            }
        }
    }
}
