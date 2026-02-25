package com.accessvascularinc.hydroguide.mma.ui;

import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.DashPathEffect;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.view.View;

/**
 * Center Line Overlay View
 * Displays a vertical line at the center of the ultrasound image for alignment guidance.
 * The center line appears on both live imaging and captured still images.
 */
public class CenterLineOverlay extends View {
    private static final String TAG = "CenterLineOverlay";
    private static final String PREFS_NAME = "ultrasound_display_settings";
    private static final String KEY_CENTER_LINE_ENABLED = "center_line_enabled";
    private static final boolean DEFAULT_CENTER_LINE_ENABLED = false;
    
    private Paint centerLinePaint;
    private boolean centerLineEnabled = false;
    
    // Customizable properties
    private int centerLineColor = 0xFFB3E0FF; // Light blue color (#B3E0FF)
    private float centerLineWidth = 5.0f; // Line thickness in pixels
    
    public CenterLineOverlay(Context context) {
        super(context);
        init(context);
    }
    
    public CenterLineOverlay(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context);
    }
    
    public CenterLineOverlay(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init(context);
    }
    
    private void init(Context context) {
        // Initialize paint for center line
        centerLinePaint = new Paint();
        centerLinePaint.setColor(centerLineColor);
        centerLinePaint.setStrokeWidth(centerLineWidth);
        centerLinePaint.setStyle(Paint.Style.STROKE);
        centerLinePaint.setAntiAlias(true);
        
        // Create dotted line effect: dash length 15px, gap 10px
        centerLinePaint.setPathEffect(new DashPathEffect(new float[]{15, 10}, 0));
        
        // Load saved state
        centerLineEnabled = getCenterLineEnabled(context);
        android.util.Log.d(TAG, "CenterLineOverlay initialized: enabled=" + centerLineEnabled);
        
        // Make the view transparent for touch events
        setWillNotDraw(false);
    }
    
    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        
        if (!centerLineEnabled) {
            return;
        }
        
        // Get view dimensions
        int width = getWidth();
        int height = getHeight();
        
        if (width == 0 || height == 0) {
            return;
        }
        
        // Calculate center X position
        float centerX = width / 2.0f;
        
        // Draw vertical line from top to bottom
        canvas.drawLine(centerX, 0, centerX, height, centerLinePaint);
    }
    
    /**
     * Enable or disable center line display
     * @param enabled true to show center line, false to hide
     */
    public void setCenterLineEnabled(boolean enabled) {
        if (this.centerLineEnabled != enabled) {
            this.centerLineEnabled = enabled;
            
            // Save preference
            saveCenterLineEnabled(getContext(), enabled);
            
            // Trigger redraw
            invalidate();
        }
    }
    
    /**
     * Get current center line state
     * @return true if center line is enabled
     */
    public boolean isCenterLineEnabled() {
        return centerLineEnabled;
    }
    
    /**
     * Set center line color
     * @param color Color value (use Color.argb() for transparency)
     */
    public void setCenterLineColor(int color) {
        this.centerLineColor = color;
        centerLinePaint.setColor(color);
        invalidate();
    }
    
    /**
     * Set center line width
     * @param width Line width in pixels
     */
    public void setCenterLineWidth(float width) {
        this.centerLineWidth = width;
        centerLinePaint.setStrokeWidth(width);
        invalidate();
    }
    
    /**
     * Toggle center line on/off
     * @return new state (true if now enabled)
     */
    public boolean toggleCenterLine() {
        setCenterLineEnabled(!centerLineEnabled);
        return centerLineEnabled;
    }
    
    /**
     * Draw center line onto a bitmap canvas (for captures)
     * @param canvas Canvas to draw on
     * @param width Canvas width
     * @param height Canvas height
     */
    public void drawCenterLineOnCanvas(Canvas canvas, int width, int height) {
        if (!centerLineEnabled) {
            return;
        }
        
        if (width == 0 || height == 0) {
            return;
        }
        
        // Calculate center X position
        float centerX = width / 2.0f;
        
        // Draw vertical line
        canvas.drawLine(centerX, 0, centerX, height, centerLinePaint);
    }
    
    /**
     * Draw center line onto a bitmap canvas with specified bounds (for captures)
     * @param canvas Canvas to draw on
     * @param width Canvas width
     * @param height Canvas height
     * @param top Top boundary (Y coordinate to start the line)
     * @param bottom Bottom boundary (Y coordinate to end the line)
     */
    public void drawCenterLineOnCanvas(Canvas canvas, int width, int height, float top, float bottom) {
        android.util.Log.d(TAG, String.format(
            "drawCenterLineOnCanvas called: enabled=%b, width=%d, height=%d, top=%.1f, bottom=%.1f",
            centerLineEnabled, width, height, top, bottom));
            
        if (!centerLineEnabled) {
            android.util.Log.d(TAG, "Center line not enabled, skipping draw");
            return;
        }
        
        if (width == 0 || height == 0) {
            android.util.Log.w(TAG, "Width or height is 0, skipping draw");
            return;
        }
        
        // Calculate center X position
        float centerX = width / 2.0f;
        
        // Draw vertical line with bounds
        canvas.drawLine(centerX, top, centerX, bottom, centerLinePaint);
        android.util.Log.d(TAG, String.format("Drew bounded center line: x=%.1f, y=%.1f to %.1f", 
            centerX, top, bottom));
    }
    
    // Preference management
    private static void saveCenterLineEnabled(Context context, boolean enabled) {
        SharedPreferences prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
        prefs.edit().putBoolean(KEY_CENTER_LINE_ENABLED, enabled).apply();
    }
    
    public static boolean getCenterLineEnabled(Context context) {
        SharedPreferences prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
        return prefs.getBoolean(KEY_CENTER_LINE_ENABLED, DEFAULT_CENTER_LINE_ENABLED);
    }
}
