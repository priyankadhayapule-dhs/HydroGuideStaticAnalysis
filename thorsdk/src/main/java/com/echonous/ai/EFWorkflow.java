package com.echonous.ai;

import android.content.SharedPreferences;
import android.os.Handler;

import com.echonous.hardware.ultrasound.ThorError;
import com.echonous.util.LogUtils;

import androidx.annotation.Nullable;

public class EFWorkflow
{
	public static final String PREF_KEY_EF_ERROR = "PREF_EF_FORCED_ERROR" ;
	private static final String TAG = "EFWorkflowJava";
	public class ClipRange {
		public ThorError error;
		public int startFrame;
		public int endFrame;

		ClipRange() {
			this.startFrame = 0;
			this.endFrame = 0;
		}
		ClipRange(int errorcode, int start, int end) {
			this.error = ThorError.fromCode(errorcode);
			this.startFrame = start;
			this.endFrame = end;
		}
	}

	public interface AutoDopplerListener {
		/// Callback received on an update to the gate position
		/// x and y are physical space coordinates
		void onAutoDopplerGate(float x, float y);
	}

	public enum AutoDopplerTarget {
		PLAX_RVOT_PV,  // Pulmonary valve, PLAX RVOT view
		PLAX_AV_PV,    // Pulmonary valve, PLAX AV view
		A4C_MV,        // Mitral Valve, A4C view
		A4C_TV,        // Tricuspid Valve, A4C view
		A5C_LVOT,      // LVOT, A5C view
		TDI_A4C_MVSA,  // MV Septal Annulus, TDI mode
		TDI_A4C_MVLA,  // MV Lateral Annulus, TDI mode
		TDI_A4C_TVLA   // TV Lateral Annulus, TDI mode
	}

	public EFWorkflow(SharedPreferences preferences)
	{
		this.preferences = preferences;
		this.listener = null;
		this.handler = null;
		this.mAutoDopplerListener = null;

		init();
		reset();
	}

	/**
	 * Trigger smart capture on the current CineBuffer. Smart capture finds a range of frames to
	 * capture based on estimated quality score. Auto Guidance MUST be enabled for smartCapture to
	 * function.
	 */
	public ClipRange smartCapture(int durationMS) {
		int[] array = nativeSmartCapture(durationMS);
		return new ClipRange(array[0], array[1], array[2]);
	}
	private native int[] nativeSmartCapture(int durationMS);

	// Reset all internal data (both AI determined plus user edited)
	public synchronized native void reset();

	// Register listener class (with a given handler)
	public synchronized void setListener(EFListener listener, Handler handler)
	{
		LogUtils.d(TAG, "setListener to " + listener);
		this.listener = listener;
		this.handler = handler;
	}

	// Set the heart rate. This function should be used when entering the edit/review screen,
	// so that the native layer can calculate CO.
	public synchronized native void setHeartRate(float hr);

	// Kick off operations

	// Start real time quality measurement
	// The Quality score and subview is displayed on screen by the native layer
	public synchronized native void beginQualityMeasure(CardiacView view);
	public synchronized native void endQualityMeasure();

	// Start real time guidance. The guidance text is displayed on screen by the native layer
	public synchronized native void beginGuidance(CardiacView view);
	public synchronized native void endGuidance();

	// Begin real time annotation for the given view
	// Detected objects are displayed on screen by the native layer
	public synchronized native void beginAutoAnnotation(CardiacView view);
	// Begin auto placement of doppler gate for specified target. Disables Autolabel (if currently enabled)
	public synchronized native void beginAutoDoppler(AutoDopplerTarget target);
	public synchronized native void setMockAutoDoppler(boolean mock);
	// End both autolabel and auto doppler gate placement
	public synchronized native void endAutoAnnotationDoppler();

	// Enable/Disable FAST exam module
	public synchronized native void beginFastExam();
	public synchronized native void endFastExam();


	public void setAutoDopplerListener(@Nullable AutoDopplerListener listener) {
		mAutoDopplerListener = listener;
	}

	public synchronized native void setAutocaptureEnabled(boolean isEnabled);

	public synchronized native void beginFrameID(CardiacView view);
	public synchronized native void beginSegmentation(CardiacView view, int frame);

	// Calculate (or recalculate) the EFStatistics
	public synchronized native void beginEFCalculation();

	// Function no longer needed, will be removed
	@Deprecated
	public synchronized native QualityScore getFrameQuality(int frame);

	// Set AI determined frames (for use when reloading data from PIMS)
	public synchronized native void setAIFrames(CardiacView view, CardiacFrames frames);
	// Get AI determined frames
	public synchronized native CardiacFrames getAIFrames(CardiacView view);
	// Set AI determined segmentation (for use when reloading data from PIMS)
	public synchronized native void setAISegmentation(CardiacView view, int frame, CardiacSegmentation segmentation);
	// Get AI determined segmentation
	public synchronized native CardiacSegmentation getAISegmentation(CardiacView view, int frame);

	// Get/set user override results
	public synchronized native void setUserFrames(CardiacView view, CardiacFrames frames);
	public synchronized native CardiacFrames getUserFrames(CardiacView view);
	public synchronized native void setUserSegmentation(CardiacView view, int frame, CardiacSegmentation segmentation);
	public synchronized native CardiacSegmentation getUserSegmentation(CardiacView view, int frame);

	// Get final statistics
	public synchronized native EFStatistics getStatistics();

	// Initialize native layer information
	private synchronized native void init();

	private synchronized void postFrameID(final CardiacView view)
	{
		LogUtils.d(TAG, "postFrameID callback, calling listener " + listener);
		final EFListener l = listener;
		handler.post(new Runnable(){public void run(){l.onAIFramesUpdate(view);}});
	}

	private synchronized void postSegmentation(final CardiacView view, final int frame)
	{
		LogUtils.d(TAG, "postSegmentation callback");
		final EFListener l = listener;
		handler.post(new Runnable(){public void run(){l.onAISegmentationUpdate(view, frame);}});
	}

	private synchronized void postStatistics()
	{
		LogUtils.d(TAG, "postStatistics callback");
		final EFListener l = listener;
		handler.post(new Runnable(){public void run(){l.onStatisticsUpdate(false);}});
	}

	private synchronized void postError(final int code)
	{
		LogUtils.d(TAG, "postError callback");
		final EFListener l = listener;
		handler.post(new Runnable(){public void run(){l.onThorError(ThorError.fromCode(code));}});
	}

	private synchronized void postAutocaptureStart()
	{
		LogUtils.d(TAG, "postAutocapture callback");
		final EFListener l = listener;
		handler.post(new Runnable(){public void run(){l.onAutocaptureStart();}});
	}
	private synchronized void postAutocaptureFail()
	{
		LogUtils.d(TAG, "postAutocapture callback");
		final EFListener l = listener;
		handler.post(new Runnable(){public void run(){l.onAutocaptureFail();}});
	}
	private synchronized void postAutocaptureSucceed()
	{
		LogUtils.d(TAG, "postAutocapture callback");
		final EFListener l = listener;
		handler.post(new Runnable(){public void run(){l.onAutocaptureSucceed();}});
	}

	private synchronized void postAutocaptureProgress(final float progress)
	{
		LogUtils.d(TAG, "postAutocaptureProcess callback");
		final EFListener l = listener;
		handler.post(new Runnable() {public void run() {l.onAutocaptureProgress(progress);}});
	}

	private synchronized void postSmartCaptureProgress(final float progress)
	{
		LogUtils.d(TAG, "postSmartCaptureProcess callback");
		final EFListener l = listener;
		handler.post(new Runnable() {public void run() {l.onSmartCaptureProgress(progress);}});
	}

	private int getForcedErrorEnum() {
		return preferences.getInt(PREF_KEY_EF_ERROR, 0);
	}
	private void clearForcedError() {
		preferences.edit().putInt(PREF_KEY_EF_ERROR, 0).apply();
	}

	private final SharedPreferences preferences;

	private void postAutoDopplerUpdateGatePosition(float x, float y)
	{
		if (mAutoDopplerListener != null) {
			mAutoDopplerListener.onAutoDopplerGate(x, y);
		}
	}

	private EFListener listener;
	private Handler handler;
	private AutoDopplerListener mAutoDopplerListener = null;
}
