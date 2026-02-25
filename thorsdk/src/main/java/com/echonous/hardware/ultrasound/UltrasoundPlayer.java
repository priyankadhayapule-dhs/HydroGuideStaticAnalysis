/*
 * Copyright (C) 2017 EchoNous, Inc.
 */

package com.echonous.hardware.ultrasound;

import android.os.Handler;
import android.os.Looper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.echonous.util.LogUtils;

/**
 * <p>Provides methods for playback of data in the Cine Buffer.  Can 
 * playback data left in Cine Buffer from the Dau.  Can also load the Cine 
 * Buffer from a Thor Raw data file.</p> 
 * 
 * @see UltrasoundManager#getUltrasoundPlayer
 */
@SuppressWarnings("unused")
public final class UltrasoundPlayer {
    private static final String TAG = "UltrasoundPlayer";

    private UltrasoundManager   mManager = null;
    private boolean             mIsCineAttached = false;

    private ProgressCallback    mProgressCallback = null;
    private Handler             mProgressHandler = null;

    private boolean             mIsFileOpen = false;
    private boolean             mIsRunning = false;

    /**
     * The possible playback speeds.
     */
    public enum PlaybackSpeed {
        /**
         * Normal speed (i.e. the aquisition speed of ultrasound data).
         */
        Normal,

        /**
         * Half speed.
         */
        Half,

        /**
         * Quarter speed.
         */
        Quarter
    }

    protected UltrasoundPlayer() {
    }

    protected UltrasoundPlayer(UltrasoundManager manager) {
        mManager = manager;
    }

    /**
     * Open a Thor Raw data file into the Cine Buffer for playback.
     * 
     * @param srcPath Full path of the raw file.
     * 
     * @return ThorError 
     */
    public synchronized ThorError openRawFile(String srcPath) {
        if (isCineAttached()) {
            int retCode = nativeOpenRawFile(srcPath);

            ThorError retError = ThorError.fromCode(retCode);

            if (retError.isOK()) {
                mIsFileOpen = true;
            }

            return retError;
        } else {
            return ThorError.TER_UMGR_PLAYER_NOT_ATTACHED;
        }
    }

    /**
     * Close Raw file and clear the Cine Buffer.
     */
    public synchronized void closeRawFile() {
        if (isCineAttached()) {
            nativeCloseRawFile();
            mIsFileOpen = false;
        }
    }

    /**
     * Returns <code>true</code> if a Raw file is open.
     * 
     * @return boolean <code>True</code> if Raw file is open.
     */
    public synchronized boolean isRawFileOpen() {
        return mIsFileOpen;
    }

    /**
     * Attach the UltrasoundPlayer to the Cine Buffer.  The UltrasoundPlayer will 
     * not function unless attached to the Cine Buffer. 
     */
    public synchronized void attachCine() {
        if (mManager.attachCineProducer(this)) {
            nativeInit(this);
            mIsCineAttached = true;
        }
    }

    /**
     * Detach the UltrasoundPlayer from the Cine Buffer.
     */
    public synchronized void detachCine() {
        if (isCineAttached()) {
            if (isRawFileOpen()) {
                closeRawFile();
            }
            mIsRunning = false;
            mManager.detachCineProducer(this);
            nativeTerminate();
            mIsCineAttached = false;
        }
    }

    /**
     * Returns <code>true</code> if the UltrasoundPlayer is attached 
     *         to the Cine Buffer. 
     *  
     * @return boolean <code>True</code> if the UltrasoundPlayer is attached to the
     *         Cine Buffer.
     */
    public synchronized boolean isCineAttached() {
        return mIsCineAttached;
    }

    /**
     * Starts / Resumes playing from current position.
     */
    public synchronized void start() {
        if (isCineAttached()) {
            nativeStart();
            mIsRunning = true;
        }
    }

    /**
     * Stops playback and sets current position to beginning
     */
    public synchronized void stop() {
        if (isCineAttached()) {
            mIsRunning = false;
            nativeStop();
        }
    }

    /**
     * Pauses playback, call start() to resume
     */
    public synchronized void pause() {
        if (isCineAttached()) {
            mIsRunning = false;
            nativePause();
        }
    }

    /**
     * Returns the running status
     * 
     * @return boolean <code>True</code> if is currently running.
     */
    public synchronized boolean isRunning() {
        return mIsRunning;
    }

    /**
     * Sets the starting position when fencing the cine buffer. 
     *  
     * Fencing is a means of restricting the playback of the cine buffer to a subset 
     * of the available data. This subset can be specified in either time in 
     * milliseconds, or frames of data. 
     * 
     * @param msec Position in milliseconds for the virtual start point.
     */
    public synchronized void setStartPosition(int msec) {
        if (isCineAttached()) {
            nativeSetStartPosition(msec);
        }
    }

    /**
     * Sets the ending position when fencing the cine buffer.
     * 
     * Fencing is a means of restricting the playback of the cine buffer to a subset 
     * of the available data. This subset can be specified in either time in 
     * milliseconds, or frames of data. 
     * 
     * @param msec Position in milliseconds for the virtual end point.
     */
    public synchronized void setEndPosition(int msec) {
        if (isCineAttached()) {
            nativeSetEndPosition(msec);
        }
    }

    /**
     * Sets the start frame when fencing the cine buffer.
     * 
     * Fencing is a means of restricting the playback of the cine buffer to a subset 
     * of the available data. This subset can be specified in either time in 
     * milliseconds, or frames of data. 
     * 
     * @param frame Frame number position for the virtual start point.
     */
    public synchronized void setStartFrame(int frame) {
        if (isCineAttached()) {
            nativeSetStartFrame(frame);
        }
    }

    /**
     * Returns the start frame number.
     *
     * @return int start frame number.
     */
    public synchronized int getStartFrame() {
        if (isCineAttached()) {
            return nativeGetStartFrame();
        } else {
            return 0;
        }
    }

    /**
     * Sets the end frame when fencing the cine buffer.
     * 
     * Fencing is a means of restricting the playback of the cine buffer to a subset 
     * of the available data. This subset can be specified in either time in 
     * milliseconds, or frames of data. 
     * 
     * @param frame Frame number position for the virtural end point.
     */
    public synchronized void setEndFrame(int frame) {
        if (isCineAttached()) {
            nativeSetEndFrame(frame);
        }
    }

    /**
     * Returns the end frame number.
     *
     * @return int end frame number.
     */
    public synchronized int getEndFrame() {
        if (isCineAttached()) {
            return nativeGetEndFrame();
        } else {
            return 0;
        }
    }

    /**
     * Returns the frame Interval
     * @return int frame interval
     */
    public synchronized int getFrameInterval() {
        if (isCineAttached()) {
            return nativeGetFrameInterval();
        } else {
            return 0;
        }
    }

    /**
     * Move the current position to the specified time in the Cine Buffer, and 
     * display that frame. 
     * 
     * @param msec Position in milliseconds to move to.
     */
    public synchronized void seekTo(int msec) {
        if (isCineAttached()) {
            nativeSeekTo(msec);
        }
    }

    /**
     * Move the current position to the specified frame in the Cine Buffer, and 
     * display that frame.  gives progress callback to UI.
     * 
     * @param frame Frame to move to.
     */
    public synchronized void seekToFrame(int frame) {
        if (isCineAttached()) {
            nativeSeekToFrame(frame, true);
        }
    }

    /**
     * Move the current position to the specified frame in the Cine Buffer, and
     * display that frame.
     *
     * @param frame Frame to move to.
     * @param progressCallback gives progress callback to UI.
     */
    public synchronized void seekToFrame(int frame, boolean progressCallback) {
        if (isCineAttached()) {
            nativeSeekToFrame(frame, progressCallback);
        }
    }

    /**
     * Advance to the next frame in the Cine Buffer.
     */
    public synchronized void nextFrame() {
        if (isCineAttached()) {
            nativeNextFrame();
        }
    }

    /**
     * Move to the previous frame in the Cine Buffer.
     */
    public synchronized void previousFrame() {
        if (isCineAttached()) {
            nativePreviousFrame();
        }
    }

    /**
     * Returns the current position in milliseconds. 
     *  
     * @return int Current position in milliseconds.
     */
    public synchronized int getCurrentPosition() {
        if (isCineAttached()) {
            return nativeGetCurrentPosition();
        } else {
            return 0;
        }
    }

    /**
     * Returns the current frame number. 
     *  
     * @return int Current frame number.
     */
    public synchronized int getCurrentFrameNo() {
        if (isCineAttached()) {
            return nativeGetCurrentFrameNo();
        } else {
            return 0;
        }
    }

    /**
     * Returns the number of milliseconds of data in the Cine Buffer. 
     *  
     * @return int Number of milliseconds of data in the Cine Buffer.
     */
    public synchronized int getDuration() {
        if (isCineAttached()) {
            return nativeGetDuration();
        } else {
            return 0;
        }
    }

    /**
     * Returns the number of frames of data in the Cine Buffer. 
     *  
     * @return int Number of frames of data in the Cine Buffer.
     */
    public synchronized int getFrameCount() {
        if (isCineAttached()) {
            return nativeGetFrameCount();
        } else {
            return 0;
        }
    }

    /**
     * Returns <code>true</code> if looping is enabled. 
     *  
     * @return boolean <code>True</code> if looping is enabled.
     */
    public synchronized boolean isLooping() {
        if (isCineAttached()) {
            return nativeIsLooping();
        } else {
            return false;
        }
    }

    /**
     * If looping is set then playback will continue from the beginning of the Cine 
     * Buffer after reaching the end. 
     * 
     * @param doLooping <code>true</code> if looping should be enabled.
     */
    public synchronized void setLooping(boolean doLooping) {
        if (isCineAttached()) {
            nativeSetLooping(doLooping);
        }
    }

    /**
     * Set the playback speed. 
     *  
     * @see PlaybackSpeed 
     * 
     * @param speed The new playback speed.
     */
    public synchronized void setPlaybackSpeed(PlaybackSpeed speed) {
        if (isCineAttached()) {
            nativeSetPlaybackSpeed(speed.ordinal());
        }
    }

    /**
     * Returns the current playback speed. 
     *  
     * @return PlaybackSpeed Current playback speed. 
     *  
     * @see PlaybackSpeed 
     */
    public synchronized PlaybackSpeed getPlaybackSpeed() {
        if (isCineAttached()) {
            return PlaybackSpeed.values()[nativeGetPlaybackSpeed()];
        } else {
            return PlaybackSpeed.Normal;
        }
    }

    /** 
     * A callback for progress during playback.
     *  
     * @see #registerProgressCallback 
     *  
     */ 
    public static abstract class ProgressCallback {

        /** 
         *  A new playback position occurred
         *  
         *  @param position The new position of playback in msec.
         */
        public void onProgress(int position) {
        }
    }

    /**
     * Registers a callback to be notified of Player playback progress. Only one 
     * callback can be registered at a time. 
     * 
     * @param callback The new callback to send HID events
     * @param handler The handler on which the callback should be invoked, or 
     *                {@code null} to use the current thread's {@link
     *                android.os.Looper looper}.
     */
    public synchronized void registerProgressCallback(@NonNull ProgressCallback callback, 
                                         @Nullable Handler handler) {
        if (null == handler) {
            Looper looper = Looper.myLooper();

            if (null == looper) {
                throw new IllegalArgumentException(
                    "No handler given, and current thread as no looper!");
            }

            handler = new Handler(looper);
        }

        mProgressCallback = callback;
        mProgressHandler = handler;
    }

    /**
     * Unregister the existing callback.
     */
    public synchronized void unregisterProgressCallback() {
        mProgressCallback = null;
        mProgressHandler = null;
    }

    protected void onProgress(final int position) {
        if (null != mProgressCallback && null != mProgressHandler) {
            mProgressHandler.post(
                new Runnable() {
                    @Override
                    public void run() {
                        try {
                            if (null != mProgressCallback) {
                                mProgressCallback.onProgress(position);
                            }
                        } catch (NullPointerException ex) {
                            LogUtils.e(TAG, "Client callback handler is null", ex);

                            mProgressCallback = null;
                            mProgressHandler = null;
                        }
                    }
                });
        }
    }

    protected void setRunningStatus(final boolean isRunning)
    {
        mIsRunning = isRunning;
    }

    private native void nativeInit(UltrasoundPlayer thisObject);
    private native void nativeTerminate();
    private native int nativeOpenRawFile(String srcPath);
    private native void nativeCloseRawFile();
    private native void nativeStart();
    private native void nativeStop();
    private native void nativePause();
    private native void nativeSetStartPosition(int msec);
    private native void nativeSetEndPosition(int msec);
    private native void nativeSetStartFrame(int frame);
    private native int nativeGetStartFrame();
    private native void nativeSetEndFrame(int frame);
    private native int nativeGetEndFrame();
    private native int nativeGetFrameInterval();
    private native void nativeNextFrame();
    private native void nativePreviousFrame();
    private native void nativeSeekTo(int msec);
    private native void nativeSeekToFrame(int frame, boolean progressCallback);
    private native int nativeGetDuration();
    private native int nativeGetFrameCount();
    private native int nativeGetCurrentPosition();
    private native int nativeGetCurrentFrameNo();
    private native boolean nativeIsLooping();
    private native void nativeSetLooping(boolean doLooping);
    private native void nativeSetPlaybackSpeed(int speed);
    private native int nativeGetPlaybackSpeed();
}
