/*
 * Copyright (C) 2018 EchoNous, Inc.
 */

package com.echonous.hardware.ultrasound;

import android.os.Handler;
import android.os.Looper;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.echonous.util.LogUtils;

/**
 * <p>Saves raw ultrasound data from the Cine Buffer to a file.</p> 
 *  
 * @see UltrasoundManager#getUltrasoundRecorder
 */
public final class UltrasoundRecorder {
    private static final String TAG = "UltrasoundRecorder";

    private UltrasoundManager   mManager = null;
    private CompletionCallback  mCompletionCallback = null;
    private Handler             mCompletionHandler = null;

    protected UltrasoundRecorder(UltrasoundManager manager) {
        mManager = manager;
        nativeInit(this);
    }


    /**
     * A callback for recording completion events. 
     *  
     * @see #registerCompletionCallback 
     * 
     */
    public static abstract class CompletionCallback {
        /** 
         *  Recording has completed.
         *   
         *  @param result Completion result.<p>
         *<code>THOR_OK</code> if recording finished successfully.<p> 
         *<code>TER_UMGR_XXX</code> if error occurred during recording.<p>
         */
        public void onCompletion(ThorError result) {
        }
    }

    /**
     * Registers a callback for notification of the recording progress.  Only one 
     * callback can be registered at a time. 
     * 
     * @param completion The new callback to send notifications.
     * @param handler The handler on which the callback should be invoked, or 
     *                {@code null} to use the current thread's {@link
     *                android.os.Looper looper}.
     */
    public void registerCompletionCallback(@NonNull CompletionCallback completion, 
                                           @Nullable Handler handler) {
        if (null == handler) {
            Looper looper = Looper.myLooper();

            if (null == looper) {
                throw new IllegalArgumentException(
                    "No handler given, and current thread as no looper!");
            }

            handler = new Handler(looper);
        }

        mCompletionCallback = completion;
        mCompletionHandler = handler;
    }

    /**
     * Unregister the existing callback.
     * 
     */
    public void unregisterCompletionCallback() {
        mCompletionCallback = null;
        mCompletionHandler = null;
    }

    /**
     * Will save the current frame in the Cine Buffer to file.  Will work under live
     * imaging or frozen state. 
     *  
     * @param destPath Full pathname of the image file to be created.
     * 
     * @return ThorError <p> 
     * <code>S_OK</code> on successful initiation.<p> 
     * <code>TER_MISC_BUSY</code> if recording is already in progress.<p>
     * <code>TER_MISC_PARAM</code> if parameters are null or out of range.<p>
     */
    public ThorError recordStillImage(@NonNull String destPath) {
        int errorRet = nativeRecordStillImage(destPath);

        return ThorError.fromCode(errorRet);
    }

    /**
     * Save video clip from Cine Buffer from current position back by msec.  Will 
     * work under live imaging.  The recorded clip may be shorter than the specified
     * length if Cine Buffer does not contain the desired data. 
     * 
     * This is an asynchronous operation.  Use RecordingCallback for notification of 
     * completion. 
     *  
     * @param destPath Full pathname of the video file to be created.
     * @param msec Number of milliseconds to record.
     * 
     * @return ThorError <p> 
     * <code>S_OK</code> on successful initiation.<p>
     * <code>TER_MISC_BUSY</code> if recording is already in progress.<p>
     * <code>TER_MISC_PARAM</code> if parameters are null or out of range.<p>
     */
    public ThorError recordRetrospectiveVideo(@NonNull String destPath, int msec) {
        int errorRet = nativeRecordRetrospectiveVideo(destPath, msec);

        return ThorError.fromCode(errorRet);
    }

    /**
     * Save video clip from Cine Buffer from current position back by frames.  Will 
     * work under live imaging.  The recorded clip may be shorter than the specified
     * length if Cine Buffer does not contain the desired data. 
     * 
     * This is an asynchronous operation.  Use RecordingCallback for notification of 
     * completion.
     * 
     * @param destPath Full pathname of the video file to be created.
     * @param frames Number of frames to record.
     * 
     * @return ThorError <p> 
     * <code>S_OK</code> on successful initiation.<p>
     * <code>TER_MISC_BUSY</code> if recording is already in progress.<p>
     * <code>TER_MISC_PARAM</code> if parameters are null or out of range.<p>
     */
    public ThorError recordRetrospectiveVideoFrames(@NonNull String destPath, int frames) {
        int errorRet = nativeRecordRetrospectiveVideoFrames(destPath, frames);

        return ThorError.fromCode(errorRet);
    }

    /**
     * Save video clip from Cine Buffer from startMsec to stopMsec.  Will only work 
     * when the UltrasoundPlayer is attached to CineBuffer. 
     * 
     * @param destPath Full pathname of the video file to be created.
     * @param startMsec Start of video clip: Number of milliseconds from beginning 
     *                  of Cine Buffer.
     * @param stopMsec End of video clip: Number of milliseconds from beginning of 
     *                 Cine Buffer.
     * 
     * @return ThorError <p> 
     * <code>S_OK</code> on successful initiation.<p>
     * <code>TER_MISC_BUSY</code> if recording is already in progress.<p>
     * <code>TER_MISC_PARAM</code> if parameters are null or out of range.<p>
     * <code>TER_IE_OPERATION_RUNNING</code> is called during live imaging.<p>
     */
    public ThorError saveVideoFromCine(@NonNull String destPath,
                                       int startMsec,
                                       int stopMsec) {
        if (mManager.isPlayerAttached()) {
            int errorRet = nativeSaveVideoFromCine(destPath, startMsec, stopMsec);

            return ThorError.fromCode(errorRet);
        }
        else {
            return ThorError.TER_IE_OPERATION_RUNNING;
        }
    }

    /**
     * Save video clip from Cine Buffer from startMsec to stopMsec.  Will only work 
     * when the UltrasoundPlayer is attached to CineBuffer.
     * 
     * @param destPath Full pathname of the video file to be created.
     * @param startFrame Start of video clip: Number of frames from beginning 
     *                  of Cine Buffer.
     * @param stopFrame End of video clip: Number of frames from beginning of 
     *                 Cine Buffer.
     * 
     * @return ThorError <p> 
     * <code>S_OK</code> on successful initiation.<p>
     * <code>TER_MISC_BUSY</code> if recording is already in progress.<p>
     * <code>TER_MISC_PARAM</code> if parameters are null or out of range.<p>
     * <code>TER_IE_OPERATION_RUNNING</code> is called during live imaging.<p>
     */
    public ThorError saveVideoFromCineFrames(@NonNull String destPath,
                                             int startFrame,
                                             int stopFrame) {
        if (mManager.isPlayerAttached()) {
            int errorRet = nativeSaveVideoFromCineFrames(destPath, startFrame, stopFrame);

            return ThorError.fromCode(errorRet);
        }
        else {
            return ThorError.TER_IE_OPERATION_RUNNING;
        }
    }

    /**
     * Save video clip from Cine Buffer from startFrame to stopFrame.
     *
     * @param destPath Full pathname of the video file to be created.
     * @param startFrame Start of video clip: Raw frame number
     * @param stopFrame End of video clip: Raw frame number
     *
     * @return ThorError <p>
     * <code>S_OK</code> on successful initiation.<p>
     * <code>TER_MISC_BUSY</code> if recording is already in progress.<p>
     * <code>TER_MISC_PARAM</code> if parameters are null or out of range.<p>
     * <code>TER_IE_OPERATION_RUNNING</code> is called during live imaging.<p>
     */
    public ThorError saveVideoFromLiveFrames(@NonNull String destPath,
                                             int startFrame,
                                             int stopFrame) {
        int errorRet = nativeSaveVideoFromLiveFrames(destPath, startFrame, stopFrame);
        return ThorError.fromCode(errorRet);
    }

    /**
     * Will update cineParams in Thor File with the one in CineBuffer
     * Works for both stills and clips
     *
     * @param destPath Full pathname of the image file to be created.
     *
     * @return ThorError <p>
     * <code>S_OK</code> on successful initiation.<p>
     * <code>TER_MISC_BUSY</code> if recording is already in progress.<p>
     * <code>TER_MISC_PARAM</code> if parameters are null or out of range.<p>
     */
    public ThorError recordUpdateParamsFile(@NonNull String destPath) {
        int errorRet = nativeUpdateParamsFile(destPath);

        return ThorError.fromCode(errorRet);
    }

    protected void cleanup() {
        nativeTerminate();
    }

    /**
     * Internal handler for receiving and relaying completion events from the native 
     * layer. 
     * 
     * @param result The result of the recording operation.
     */
    private void onCompletion(final int result) {
        if (null != mCompletionCallback && null != mCompletionHandler) {
            mCompletionHandler.post(
                new Runnable() {
                    @Override
                    public void run() {
                        try {
                            ThorError thorError = ThorError.fromCode(result);
                            mCompletionCallback.onCompletion(thorError);
                        } catch (NullPointerException ex) {
                            LogUtils.e(TAG, "Client callback handler is null", ex);

                            mCompletionCallback = null;
                            mCompletionHandler = null;
                        }
                    }
                }
                );
        }
    }

    
    private native void nativeInit(UltrasoundRecorder thisObject);
    private native void nativeTerminate();
    private native int nativeRecordStillImage(String path);
    private native int nativeRecordRetrospectiveVideo(String path, int msec);
    private native int nativeRecordRetrospectiveVideoFrames(String path, int frames);
    private native int nativeSaveVideoFromCine(String path,
                                               int startMsec,
                                               int stopMsec);
    private native int nativeSaveVideoFromCineFrames(String path,
                                                     int startFrame,
                                                     int stopFrame);
    private native int nativeSaveVideoFromLiveFrames(String path,
                                                     int startFrame,
                                                     int stopFrame);
    private native int nativeUpdateParamsFile(String path);
}
