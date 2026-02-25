package com.echonous.ai;

/**
 * Verification-specific interface for bladder ML verification workflows.
 * This interface extracts only the essential methods needed for verification,
 * removing dependencies on UI components and complex workflow logic.
 */
public interface BladderVerificationListener {
    
    /**
     * Process segmentation parameters from the native ML verification pipeline
     */
    void onVerificationSegmentationParams(
        float cX, // Centroid X in physical coordinates
        float cY, // Centroid Y in physical coordinates
        float uncertainty,
        float stability,
        int area,
        int frameNum
    );
    
    /**
     * Get the region of interest status for a specific frame
     * @param frameNumber The frame number to check
     * @return True if the centroid was in the region of interest for that frame, false otherwise
     */
    boolean getCentroidRegionOfInterestStatus(int frameNumber);
    
    /**
     * Check if a frame exists in the ROI tracking map
     * @param frameNumber The frame number to check
     * @return True if the frame has been processed and exists in the map
     */
    boolean getFrameInRoIMap(int frameNumber);
    
    /**
     * Get the total number of frames that have been processed
     * @return The total count of processed frames
     */
    int getTotalFramesProcessed();
    
    /**
     * Clear all region of interest tracking data
     */
    void clearCentroidRegionOfInterestData();
    
    /**
     * Set the verification depth (called from native verification workflow)
     * @param depth The depth ID to set
     */
    void setVerificationDepth(int depth);
    
    /**
     * Initialize verification with load data (called from native verification workflow)
     * @param flowType The flow type (PRE_VOID, POST_VOID)
     * @param viewType The view type (TRANSVERSE, SAGITTAL)  
     * @param depth The depth ID
     * @param probeType The probe type
     */
    void setVerificationLoadData(int flowType, int viewType, int depth, int probeType);
    
    /**
     * Get the frame number that had the maximum area during verification
     * @return The frame number with maximum area, or 0 if none found
     */
    int getMaxAreaFrame();
    
    /**
     * Get the maximum area value tracked during verification
     * @return The maximum area value, or 0.0f if none found
     */
    float getMaxArea();
}
