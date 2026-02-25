package com.echonous.hardware.ultrasound;

import android.os.Handler;


public class AutoControl
{
	private static final String TAG = "AutoControlJava";

	public AutoControl(boolean mimicGain, boolean mimicDepth, boolean mimicPreset)
	{
		this.listener = null;
		this.handler = null;
		this.delayUS = 50000; // 50 ms delay by default

		nativeInit(this);

		mimicAutoControl(mimicGain, mimicDepth, mimicPreset);
	}


	public synchronized native void nativeInit(AutoControl thisObject);
	public synchronized native void nativeTerminate();
	public synchronized native void setAutoPresetEnabled(boolean enabled);
	public synchronized native void setAutoGainEnabled(boolean enabled);
	public synchronized native void setAutoDepthEnabled(boolean enabled);
	public synchronized native void setDebugPresetJNI(int frameNum, int preset);
	public synchronized native void setDebugGainJNI(int frameNum, int gain);
	public synchronized native void setDebugDepthJNI(int frameNum, int depth);
	public synchronized native void mimicAutoControl(boolean mimicGain, boolean mimicDepth, boolean mimicPreset);

	// Register listener class (with a given handler)
	public synchronized void setListener(AutoControlListener listener, Handler handler)
	{
		this.listener = listener;
		this.handler = handler;
	}

	// just in case we need delay here
	public synchronized void debugSetDelay(int delayUS)
	{
		this.delayUS = delayUS;
	}

	private synchronized void setAutoOrgan(final int organId)
	{
		final AutoControlListener l = listener;
		handler.post(() -> l.onAutoOrgan(organId));
	}

	private synchronized void setAutoGain(final int gainVal)
	{
		final AutoControlListener l = listener;
		handler.post(() -> l.onAutoGain(gainVal));
	}

	private synchronized void setAutoDepth(final int depthIndex)
	{
		final AutoControlListener l = listener;
		handler.post(() -> l.onAutoDepth(depthIndex));
	}

	private AutoControlListener listener;
	private Handler handler;
	private int delayUS;
}