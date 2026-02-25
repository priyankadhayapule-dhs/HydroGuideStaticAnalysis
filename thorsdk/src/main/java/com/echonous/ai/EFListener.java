package com.echonous.ai;

import com.echonous.ai.CardiacView;
import com.echonous.hardware.ultrasound.ThorError;

public interface EFListener
{
    @Deprecated
    void onFrameQualityUpdate(int frame);
	void onAIFramesUpdate(CardiacView view);
	void onAISegmentationUpdate(CardiacView view, int frame);
	void onStatisticsUpdate(Boolean isFromThorError);
    void onThorError(ThorError error);
    public void onAutocaptureStart();
    public void onAutocaptureFail();
    public void onAutocaptureSucceed();

    // Called on autocapture progress, where progress will go from 0-1 (1 is fully complete)
    public void onAutocaptureProgress(float progress);
    // Called on Smart Capture progress, where progress will go from 0-1 (1 is fully complete)
    public void onSmartCaptureProgress(float progress);
}