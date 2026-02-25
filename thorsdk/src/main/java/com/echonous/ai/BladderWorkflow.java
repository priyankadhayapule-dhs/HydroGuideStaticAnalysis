package com.echonous.ai;

import android.content.Context;

import com.echonous.util.LogUtils;

public class BladderWorkflow {
    private static final String TAG = "BladderWorkflowJava";
    private BladderWorkflowListener listener;
    private BladderVerificationListener verificationListener;
    
    public BladderWorkflow() {
        listener = null;
        verificationListener = null;
        nativeInit(this);
    }

    public void setSegmentationParams(float centroidX, float centroidY, float uncertainty, float stability, int area, int frameNumber) {
        
        // Route to verification listener first (preferred for verification workflows)
        if (verificationListener != null) {
            verificationListener.onVerificationSegmentationParams(centroidX, centroidY, uncertainty,
                    stability, area, frameNumber);
        } else if (listener != null) {
            listener.onSegmentationParams(centroidX, centroidY, uncertainty,
                    stability, area, frameNumber);
        }
    }

    // ROI (Region of Interest) verification methods
    public boolean getCentroidRegionOfInterestStatus(int frameNumber) {
        if (verificationListener != null) {
            return verificationListener.getCentroidRegionOfInterestStatus(frameNumber);
        }
        // BladderWorkflowListener no longer has verification methods - return false as fallback
        return false;
    }

    public boolean getFrameInRoIMap(int frameNumber) {
        if (verificationListener != null) {
            return verificationListener.getFrameInRoIMap(frameNumber);
        }
        // BladderWorkflowListener no longer has verification methods - return false as fallback
        return false;
    }

    public void clearCentroidRegionOfInterestStatus() {
        if (verificationListener != null) {
            verificationListener.clearCentroidRegionOfInterestData();
        }
        // BladderWorkflowListener no longer has verification methods - no fallback action needed
    }

    public int getTotalFramesProcessed() {
        if (verificationListener != null) {
            return verificationListener.getTotalFramesProcessed();
        }
        // BladderWorkflowListener no longer has verification methods - return 0 as fallback
        return 0;
    }

    // Verification configuration methods
    public void setVerificationDepth(int depth) {
        if (verificationListener != null) {
            verificationListener.setVerificationDepth(depth);
        }
        // BladderWorkflowListener no longer has verification methods - no fallback action needed
    }

    public void setVerificationLoadInfo(int flowtype, int viewtype, int depth, int probetype) {
        if (verificationListener != null) {
            verificationListener.setVerificationLoadData(flowtype, viewtype, depth, probetype);
        }
        // BladderWorkflowListener no longer has verification methods - no fallback action needed
    }

    public void setVerificationLoadData(int flowType, int viewType, int depth, int probeType) {
        if (verificationListener != null) {
            verificationListener.setVerificationLoadData(flowType, viewType, depth, probeType);
        }
        // BladderWorkflowListener no longer has verification methods - no fallback action needed
    }

    // MaxArea tracking methods (for optimal frame selection during verification)
    public int getMaxAreaFrame() {
        if (verificationListener != null) {
            return verificationListener.getMaxAreaFrame();
        }
        // BladderWorkflowListener no longer has verification methods - return 0 as fallback
        return 0;
    }

    public float getMaxArea() {
        if (verificationListener != null) {
            return verificationListener.getMaxArea();
        }
        // BladderWorkflowListener no longer has verification methods - return 0.0f as fallback
        return 0.0f;
    }

    public synchronized void setListener(BladderWorkflowListener listener) {
        this.listener = listener;
    }

    public synchronized void setVerificationListener(BladderVerificationListener verificationListener) {
        this.verificationListener = verificationListener;
    }

    // start processing bladder operations
    public synchronized native void beginBladderExam(Context context);

    public synchronized native void endBladderExam();

    public synchronized native void nativeInit(BladderWorkflow thisObj);

    public synchronized native void nativeTerminate();

    public synchronized native BVSegmentation getContour(int frameNum, int viewType);

    public synchronized native void setSegmentationColor(long color);
}
