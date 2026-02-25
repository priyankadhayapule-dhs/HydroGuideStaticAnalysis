package com.echonous.ai;

public interface BladderWorkflowListener {
    // Core segmentation callback
    void onSegmentationParams(float cX, float cY, float uncertainty, float stability,
                              int area, int frameNum);
}
