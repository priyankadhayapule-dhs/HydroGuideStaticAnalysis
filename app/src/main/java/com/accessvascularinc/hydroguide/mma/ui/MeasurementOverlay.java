package com.accessvascularinc.hydroguide.mma.ui;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PointF;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;

import androidx.annotation.Nullable;

import com.accessvascularinc.hydroguide.mma.model.MeasurementModel;
import com.accessvascularinc.hydroguide.mma.utils.MatrixConverter;
import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;

import java.util.ArrayList;
import java.util.List;

public class MeasurementOverlay extends View {
    
    private static final String TAG = "MeasurementOverlay";
    
    private Paint linePaint;
    private Paint pointPaint;
    private Paint textPaint;
    private Paint textBackgroundPaint;
    
    private MeasurementModel currentMeasurement;
    private List<MeasurementModel> measurementList;
    private float[] transformationMatrix;
    private boolean isMeasurementMode = false;
    
    // Endpoint dragging state
    private MeasurementModel draggedMeasurement = null;
    private boolean draggingStartPoint = false;
    private float originalEndpointX = 0f; // Store original position to revert if needed
    private float originalEndpointY = 0f;
    private static final float TOUCH_THRESHOLD = 50f; // pixels - radius for touch detection
    private static final float MIN_MEASUREMENT_MM = 0.1f; // Minimum allowed measurement
    
    private OnMeasurementCompleteListener measurementCompleteListener;
    private OnTouchEventListener touchEventListener;
    
    public interface OnMeasurementCompleteListener {
        void onMeasurementComplete(MeasurementModel measurement);
    }
    
    public interface OnTouchEventListener {
        void onTouchEvent(float x, float y, int action);
    }
    
    public MeasurementOverlay(Context context) {
        super(context);
        init();
    }
    
    public MeasurementOverlay(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
        init();
    }
    
    public MeasurementOverlay(Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }
    
    private void init() {
        // Initialize measurement list
        measurementList = new ArrayList<>();
        
        // Setup paint for drawing measurement lines
        linePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        linePaint.setColor(Color.YELLOW);
        linePaint.setStrokeWidth(3.0f);
        linePaint.setStyle(Paint.Style.STROKE);
        
        // Setup paint for drawing control points (endpoints)
        pointPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        pointPaint.setColor(Color.YELLOW);
        pointPaint.setStyle(Paint.Style.FILL);
        
        // Setup paint for measurement value text
        textPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        textPaint.setColor(Color.YELLOW);
        textPaint.setTextSize(40f);
        textPaint.setTextAlign(Paint.Align.CENTER);
        textPaint.setFakeBoldText(true);
        
        // Setup paint for text background
        textBackgroundPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        textBackgroundPaint.setColor(Color.argb(180, 0, 0, 0)); // Semi-transparent black
        textBackgroundPaint.setStyle(Paint.Style.FILL);
    }
    
    public void setTransformationMatrix(float[] matrix) {
        if (matrix == null || matrix.length != 9) {
            MedDevLog.error(TAG, "Invalid transformation matrix. Expected 9 elements, got: " +
                (matrix == null ? "null" : matrix.length));
            return;
        }
        this.transformationMatrix = matrix.clone();
        android.util.Log.d(TAG, "Transformation matrix set: [" + 
            matrix[0] + ", " + matrix[1] + ", " + matrix[2] + ", " + 
            matrix[3] + ", " + matrix[4] + ", " + matrix[5] + ", " + 
            matrix[6] + ", " + matrix[7] + ", " + matrix[8] + "]");
        // Trigger redraw with new matrix so existing measurements update their positions
        invalidate();
    }
    
    public void setMeasurementMode(boolean enabled) {
        this.isMeasurementMode = enabled;
        if (!enabled) {
            // Cancel current measurement if disabling mode
            currentMeasurement = null;
            invalidate();
        }
        android.util.Log.d(TAG, "Measurement mode: " + (enabled ? "enabled" : "disabled"));
    }
    
    public boolean isMeasurementMode() {
        return isMeasurementMode;
    }
    
    public void setOnMeasurementCompleteListener(OnMeasurementCompleteListener listener) {
        this.measurementCompleteListener = listener;
    }
    
    public void setOnTouchEventListener(OnTouchEventListener listener) {
        this.touchEventListener = listener;
    }
    
    public List<MeasurementModel> getMeasurements() {
        return new ArrayList<>(measurementList);
    }
    
    public void addMeasurement(MeasurementModel measurement) {
        if (measurement != null) {
            measurementList.add(measurement);
            invalidate();
        }
    }
    
    public void clearMeasurements() {
        measurementList.clear();
        currentMeasurement = null;
        invalidate();
        android.util.Log.d(TAG, "All measurements cleared");
    }
    
    public void removeLastMeasurement() {
        if (!measurementList.isEmpty()) {
            measurementList.remove(measurementList.size() - 1);
            invalidate();
            android.util.Log.d(TAG, "Last measurement removed");
        }
    }
    
    /**
     * Find if touch is near any measurement endpoint
     * Returns array: [measurement, isStartPoint (1=start, 0=end)] or null
     */
    private Object[] findNearestEndpoint(float pixelX, float pixelY) {
        if (transformationMatrix == null) return null;
        
        // Check in reverse order so most recent measurements have priority
        for (int i = measurementList.size() - 1; i >= 0; i--) {
            MeasurementModel measurement = measurementList.get(i);
            
            // Convert physical coordinates to pixel
            PointF startPixel = MatrixConverter.physicalToPixel(
                transformationMatrix,
                measurement.getStartXPhysical(),
                measurement.getStartYPhysical()
            );
            
            PointF endPixel = MatrixConverter.physicalToPixel(
                transformationMatrix,
                measurement.getEndXPhysical(),
                measurement.getEndYPhysical()
            );
            
            // Check if touch is near start point
            float distToStart = (float) Math.sqrt(
                Math.pow(pixelX - startPixel.x, 2) + Math.pow(pixelY - startPixel.y, 2)
            );
            if (distToStart <= TOUCH_THRESHOLD) {
                return new Object[]{measurement, true};
            }
            
            // Check if touch is near end point
            float distToEnd = (float) Math.sqrt(
                Math.pow(pixelX - endPixel.x, 2) + Math.pow(pixelY - endPixel.y, 2)
            );
            if (distToEnd <= TOUCH_THRESHOLD) {
                return new Object[]{measurement, false};
            }
        }
        
        return null;
    }
    
    @Override
    public boolean onTouchEvent(MotionEvent event) {
        float pixelX = event.getX();
        float pixelY = event.getY();
        
        // Check if transformation matrix is set
        if (transformationMatrix == null) {
            return super.onTouchEvent(event);
        }
        
        switch (event.getAction()) {
            case MotionEvent.ACTION_DOWN:
                // Always check if user is touching an existing endpoint (works even when measurement mode is OFF)
                Object[] endpointInfo = findNearestEndpoint(pixelX, pixelY);
                
                if (endpointInfo != null) {
                    // User touched an endpoint - start dragging
                    draggedMeasurement = (MeasurementModel) endpointInfo[0];
                    draggingStartPoint = (Boolean) endpointInfo[1];
                    
                    // Store original position in case we need to revert
                    if (draggingStartPoint) {
                        originalEndpointX = draggedMeasurement.getStartXPhysical();
                        originalEndpointY = draggedMeasurement.getStartYPhysical();
                    } else {
                        originalEndpointX = draggedMeasurement.getEndXPhysical();
                        originalEndpointY = draggedMeasurement.getEndYPhysical();
                    }
                    
                    android.util.Log.d(TAG, String.format(
                        "Started dragging %s point of measurement",
                        draggingStartPoint ? "start" : "end"
                    ));
                    
                    // Show magnifier during adjustment
                    if (touchEventListener != null) {
                        touchEventListener.onTouchEvent(pixelX, pixelY, event.getAction());
                    }
                    
                    invalidate();
                    return true;
                }
                
                // Only allow creating new measurements when measurement mode is enabled
                if (!isMeasurementMode) {
                    return super.onTouchEvent(event);
                }
                
                // Notify touch listener for magnifier display when creating new measurement
                if (touchEventListener != null) {
                    touchEventListener.onTouchEvent(pixelX, pixelY, event.getAction());
                }
                
                // Not touching endpoint - start new measurement
                PointF physicalStart = MatrixConverter.pixelToPhysical(
                    transformationMatrix, pixelX, pixelY
                );
                
                currentMeasurement = new MeasurementModel();
                currentMeasurement.setStartXPhysical(physicalStart.x);
                currentMeasurement.setStartYPhysical(physicalStart.y);
                currentMeasurement.setEndXPhysical(physicalStart.x);
                currentMeasurement.setEndYPhysical(physicalStart.y);
                
                android.util.Log.d(TAG, String.format(
                    "Measurement started - Pixel: (%.1f, %.1f) -> Physical: (%.2fmm, %.2fmm)",
                    pixelX, pixelY, physicalStart.x, physicalStart.y
                ));
                
                invalidate();
                return true;
                
            case MotionEvent.ACTION_MOVE:
                PointF physicalPoint = MatrixConverter.pixelToPhysical(
                    transformationMatrix, pixelX, pixelY
                );
                
                // Check if dragging an existing endpoint (works even when measurement mode is OFF)
                if (draggedMeasurement != null) {
                    // Temporarily store current position
                    float currentStartX = draggedMeasurement.getStartXPhysical();
                    float currentStartY = draggedMeasurement.getStartYPhysical();
                    float currentEndX = draggedMeasurement.getEndXPhysical();
                    float currentEndY = draggedMeasurement.getEndYPhysical();
                    
                    // Try updating the dragged endpoint
                    if (draggingStartPoint) {
                        draggedMeasurement.setStartXPhysical(physicalPoint.x);
                        draggedMeasurement.setStartYPhysical(physicalPoint.y);
                    } else {
                        draggedMeasurement.setEndXPhysical(physicalPoint.x);
                        draggedMeasurement.setEndYPhysical(physicalPoint.y);
                    }
                    
                    // Check if new position would violate minimum measurement
                    float distanceMm = draggedMeasurement.calculateDistanceMm();
                    if (distanceMm < MIN_MEASUREMENT_MM) {
                        // Revert to previous position - don't allow movement that makes measurement too small
                        draggedMeasurement.setStartXPhysical(currentStartX);
                        draggedMeasurement.setStartYPhysical(currentStartY);
                        draggedMeasurement.setEndXPhysical(currentEndX);
                        draggedMeasurement.setEndYPhysical(currentEndY);
                    }
                    
                    // Update magnifier during drag
                    if (touchEventListener != null) {
                        touchEventListener.onTouchEvent(pixelX, pixelY, event.getAction());
                    }
                    
                    invalidate();
                    return true;
                }
                
                // Update end point of current measurement being drawn (only in measurement mode)
                if (currentMeasurement != null) {
                    currentMeasurement.setEndXPhysical(physicalPoint.x);
                    currentMeasurement.setEndYPhysical(physicalPoint.y);
                    
                    // Update magnifier during drawing
                    if (touchEventListener != null) {
                        touchEventListener.onTouchEvent(pixelX, pixelY, event.getAction());
                    }
                    
                    invalidate();
                    return true;
                }
                
                return super.onTouchEvent(event);
                
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_CANCEL:
                // Check if we were dragging an endpoint (works even when measurement mode is OFF)
                if (draggedMeasurement != null) {
                    PointF physicalEnd = MatrixConverter.pixelToPhysical(
                        transformationMatrix, pixelX, pixelY
                    );
                    
                    // Update final position
                    if (draggingStartPoint) {
                        draggedMeasurement.setStartXPhysical(physicalEnd.x);
                        draggedMeasurement.setStartYPhysical(physicalEnd.y);
                    } else {
                        draggedMeasurement.setEndXPhysical(physicalEnd.x);
                        draggedMeasurement.setEndYPhysical(physicalEnd.y);
                    }
                    
                    // Check if measurement is still valid (above minimum)
                    float distanceMm = draggedMeasurement.calculateDistanceMm();
                    if (distanceMm < MIN_MEASUREMENT_MM) {
                        // Revert to original position - adjustment would make measurement too small
                        if (draggingStartPoint) {
                            draggedMeasurement.setStartXPhysical(originalEndpointX);
                            draggedMeasurement.setStartYPhysical(originalEndpointY);
                        } else {
                            draggedMeasurement.setEndXPhysical(originalEndpointX);
                            draggedMeasurement.setEndYPhysical(originalEndpointY);
                        }
                        
                        android.util.Log.d(TAG, String.format(
                            "Endpoint drag rejected - Distance %.2fmm below minimum %.2fmm",
                            distanceMm, MIN_MEASUREMENT_MM
                        ));
                    } else {
                        android.util.Log.d(TAG, String.format(
                            "Endpoint drag completed - New distance: %.2fmm",
                            distanceMm
                        ));
                        
                        // Notify listener about update (do not toggle mode off for adjustments)
                        if (measurementCompleteListener != null) {
                            measurementCompleteListener.onMeasurementComplete(draggedMeasurement);
                        }
                    }
                    
                    // Hide magnifier after adjustment
                    if (touchEventListener != null) {
                        touchEventListener.onTouchEvent(pixelX, pixelY, event.getAction());
                    }
                    
                    draggedMeasurement = null;
                    draggingStartPoint = false;
                    invalidate();
                    return true;
                }
                
                // Finalize new measurement (only when in measurement mode)
                if (currentMeasurement != null) {
                    PointF physicalEnd = MatrixConverter.pixelToPhysical(
                        transformationMatrix, pixelX, pixelY
                    );
                    
                    currentMeasurement.setEndXPhysical(physicalEnd.x);
                    currentMeasurement.setEndYPhysical(physicalEnd.y);
                    
                    // Calculate distance in mm
                    float distanceMm = currentMeasurement.calculateDistanceMm();
                    
                    // Only save if distance is meaningful (> 0.1mm)
                    if (distanceMm > MIN_MEASUREMENT_MM) {
                        // Keep only one measurement - clear previous if exists
                        // (List structure retained for future multi-measurement support)
                        if (!measurementList.isEmpty()) {
                            measurementList.clear();
                            android.util.Log.d(TAG, "Previous measurement cleared for new measurement");
                        }
                        measurementList.add(currentMeasurement);
                        
                        android.util.Log.d(TAG, String.format(
                            "Measurement completed - Distance: %.2fmm (%.2fcm)",
                            distanceMm, distanceMm / 10.0f
                        ));
                        
                        // Notify listener
                        if (measurementCompleteListener != null) {
                            measurementCompleteListener.onMeasurementComplete(currentMeasurement);
                        }
                    } else {
                        android.util.Log.d(TAG, "Measurement too small, discarded");
                    }
                    
                    // Hide magnifier after creating new measurement
                    if (touchEventListener != null) {
                        touchEventListener.onTouchEvent(pixelX, pixelY, event.getAction());
                    }
                    
                    currentMeasurement = null;
                    invalidate();
                    return true;
                }
                
                return super.onTouchEvent(event);
        }
        
        return super.onTouchEvent(event);
    }
    
    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        
        if (transformationMatrix == null) {
            return;
        }
        
        // Draw all completed measurements
        for (MeasurementModel measurement : measurementList) {
            drawMeasurement(canvas, measurement);
        }
        
        // Draw current measurement being created
        if (currentMeasurement != null) {
            drawMeasurement(canvas, currentMeasurement);
        }
    }
    
    private void drawMeasurement(Canvas canvas, MeasurementModel measurement) {
        PointF startPixel = MatrixConverter.physicalToPixel(
            transformationMatrix,
            measurement.getStartXPhysical(),
            measurement.getStartYPhysical()
        );
        
        PointF endPixel = MatrixConverter.physicalToPixel(
            transformationMatrix,
            measurement.getEndXPhysical(),
            measurement.getEndYPhysical()
        );
        
        // Draw line between start and end points
        canvas.drawLine(
            startPixel.x, startPixel.y,
            endPixel.x, endPixel.y,
            linePaint
        );
        
        // Draw control points (circles at endpoints) - larger for easier dragging
        float endpointRadius = 12f;
        canvas.drawCircle(startPixel.x, startPixel.y, endpointRadius, pointPaint);
        canvas.drawCircle(endPixel.x, endPixel.y, endpointRadius, pointPaint);
        
        // Draw a small border around endpoints to make them stand out
        Paint borderPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        borderPaint.setColor(Color.WHITE);
        borderPaint.setStyle(Paint.Style.STROKE);
        borderPaint.setStrokeWidth(2f);
        canvas.drawCircle(startPixel.x, startPixel.y, endpointRadius + 2f, borderPaint);
        canvas.drawCircle(endPixel.x, endPixel.y, endpointRadius + 2f, borderPaint);
        
        // Draw measurement value label at midpoint
        float midX = (startPixel.x + endPixel.x) / 2;
        float midY = (startPixel.y + endPixel.y) / 2;
        
        // Format distance value in mm with 2 decimal places
        float distanceMm = measurement.calculateDistanceMm();
        String label = String.format("%.2f mm", distanceMm);
        
        // Draw text background
        float textWidth = textPaint.measureText(label);
        float padding = 10f;
        canvas.drawRect(
            midX - textWidth / 2 - padding,
            midY - 25f - padding,
            midX + textWidth / 2 + padding,
            midY + 5f + padding,
            textBackgroundPaint
        );
        
        // Draw text
        canvas.drawText(label, midX, midY, textPaint);
    }
}
