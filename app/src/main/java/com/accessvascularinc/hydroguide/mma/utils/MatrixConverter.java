package com.accessvascularinc.hydroguide.mma.utils;

import android.graphics.PointF;

import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;

public class MatrixConverter {
    
    private static final String TAG = "MatrixConverter";
    
    public static PointF pixelToPhysicalWithZoom(float[] matrix, float pixelX, float pixelY,
                                                   float zoomScale, int viewWidth, int viewHeight) {
        if (matrix == null || matrix.length != 9) {
            MedDevLog.error(TAG, "Invalid matrix for pixelToPhysicalWithZoom conversion");
            return new PointF(0, 0);
        }
        
        float centerX = viewWidth / 2.0f;
        float centerY = viewHeight / 2.0f;
        
        // Adjust for zoom: translate to center, scale, translate back
        float adjustedX = ((pixelX - centerX) / zoomScale) + centerX;
        float adjustedY = ((pixelY - centerY) / zoomScale) + centerY;
        
        android.util.Log.d(TAG, String.format(
            "Zoom adjustment: scale=%.2f, original=(%.1f,%.1f), adjusted=(%.1f,%.1f)",
            zoomScale, pixelX, pixelY, adjustedX, adjustedY
        ));
        
        // Now convert adjusted pixel coordinates to physical coordinates
        return pixelToPhysical(matrix, adjustedX, adjustedY);
    }
    
    public static PointF pixelToPhysical(float[] matrix, float pixelX, float pixelY) {
        if (matrix == null || matrix.length != 9) {
            MedDevLog.error(TAG, "Invalid matrix for pixelToPhysical conversion");
            return new PointF(0, 0);
        }
        
        float[][] invertedMatrix = getInvertedMatrix(matrix);
        
        if (invertedMatrix == null) {
            MedDevLog.error(TAG, "Failed to invert matrix");
            return new PointF(0, 0);
        }
        
        float physicalX = invertedMatrix[0][0] * pixelX + 
                          invertedMatrix[0][1] * pixelY + 
                          invertedMatrix[0][2];
                          
        float physicalY = invertedMatrix[1][0] * pixelX + 
                          invertedMatrix[1][1] * pixelY + 
                          invertedMatrix[1][2];
        
        return new PointF(physicalX, physicalY);
    }
    
    public static PointF physicalToPixelWithZoom(float[] matrix, float physicalX, float physicalY,
                                                   float zoomScale, int viewWidth, int viewHeight) {
        if (matrix == null || matrix.length != 9) {
            MedDevLog.error(TAG, "Invalid matrix for physicalToPixelWithZoom conversion");
            return new PointF(0, 0);
        }
        
        PointF basePixel = physicalToPixel(matrix, physicalX, physicalY);
        
        float centerX = viewWidth / 2.0f;
        float centerY = viewHeight / 2.0f;
        
        // Apply zoom: translate to center, scale, translate back
        float zoomedX = ((basePixel.x - centerX) * zoomScale) + centerX;
        float zoomedY = ((basePixel.y - centerY) * zoomScale) + centerY;
        
        return new PointF(zoomedX, zoomedY);
    }
    
    public static PointF physicalToPixel(float[] matrix, float physicalX, float physicalY) {
        if (matrix == null || matrix.length != 9) {
            MedDevLog.error(TAG, "Invalid matrix for physicalToPixel conversion");
            return new PointF(0, 0);
        }
        
        float pixelX = matrix[0] * physicalX + 
                       matrix[3] * physicalY + 
                       matrix[6];
                       
        float pixelY = matrix[1] * physicalX + 
                       matrix[4] * physicalY + 
                       matrix[7];
        
        return new PointF(pixelX, pixelY);
    }
    
    private static float[][] getInvertedMatrix(float[] matrix) {
        float m0 = matrix[0], m1 = matrix[1], m2 = matrix[2];
        float m3 = matrix[3], m4 = matrix[4], m5 = matrix[5];
        float m6 = matrix[6], m7 = matrix[7], m8 = matrix[8];
        
        float det = m0 * (m4 * m8 - m5 * m7) -
                    m3 * (m1 * m8 - m2 * m7) +
                    m6 * (m1 * m5 - m2 * m4);
        
        if (Math.abs(det) < 1e-10) {
            MedDevLog.error(TAG, "Matrix is singular, cannot invert");
            return null;
        }
        
        float invDet = 1.0f / det;
        
        float[][] inverted = new float[3][3];
        
        inverted[0][0] = (m4 * m8 - m5 * m7) * invDet;
        inverted[0][1] = (m6 * m5 - m3 * m8) * invDet;
        inverted[0][2] = (m3 * m7 - m6 * m4) * invDet;
        
        inverted[1][0] = (m7 * m2 - m1 * m8) * invDet;
        inverted[1][1] = (m0 * m8 - m6 * m2) * invDet;
        inverted[1][2] = (m6 * m1 - m0 * m7) * invDet;
        
        inverted[2][0] = (m1 * m5 - m4 * m2) * invDet;
        inverted[2][1] = (m3 * m2 - m0 * m5) * invDet;
        inverted[2][2] = (m0 * m4 - m3 * m1) * invDet;
        
        return inverted;
    }
    
    public static float getScaleX(float[] matrix) {
        if (matrix == null || matrix.length != 9) {
            return 1.0f;
        }
        return (float) Math.sqrt(matrix[0] * matrix[0] + matrix[1] * matrix[1]);
    }
    
    public static float getScaleY(float[] matrix) {
        if (matrix == null || matrix.length != 9) {
            return 1.0f;
        }
        return (float) Math.sqrt(matrix[3] * matrix[3] + matrix[4] * matrix[4]);
    }
    
    public static boolean isFlippedHorizontally(float[] matrix) {
        if (matrix == null || matrix.length != 9) {
            return false;
        }
        float det = matrix[0] * matrix[4] - matrix[1] * matrix[3];
        return det < 0;
    }
    
    public static boolean isValidMatrix(float[] matrix) {
        if (matrix == null || matrix.length != 9) {
            return false;
        }
        
        for (float value : matrix) {
            if (Float.isNaN(value) || Float.isInfinite(value)) {
                return false;
            }
        }
        
        float m0 = matrix[0], m1 = matrix[1], m2 = matrix[2];
        float m3 = matrix[3], m4 = matrix[4], m5 = matrix[5];
        float m6 = matrix[6], m7 = matrix[7], m8 = matrix[8];
        
        float det = m0 * (m4 * m8 - m5 * m7) -
                    m3 * (m1 * m8 - m2 * m7) +
                    m6 * (m1 * m5 - m2 * m4);
        
        return Math.abs(det) > 1e-10;
    }
    
    public static void printMatrix(String label, float[] matrix) {
        if (matrix == null || matrix.length != 9) {
            android.util.Log.d(TAG, label + ": Invalid matrix");
            return;
        }
        
        android.util.Log.d(TAG, label + ":");
        android.util.Log.d(TAG, String.format("  [%.4f  %.4f  %.4f]", matrix[0], matrix[3], matrix[6]));
        android.util.Log.d(TAG, String.format("  [%.4f  %.4f  %.4f]", matrix[1], matrix[4], matrix[7]));
        android.util.Log.d(TAG, String.format("  [%.4f  %.4f  %.4f]", matrix[2], matrix[5], matrix[8]));
    }
}
