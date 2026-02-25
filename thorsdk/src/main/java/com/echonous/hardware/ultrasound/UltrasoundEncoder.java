/*
 * Copyright (C) 2018 EchoNous, Inc.
 */

package com.echonous.hardware.ultrasound;

import android.graphics.Bitmap;
import android.os.Handler;
import android.os.Looper;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.echonous.util.LogUtils;

/**
 * <p>The UltrasoundEncoder is a collection of functions related to encode
 * still images and clips.</p> 
 */
public final class UltrasoundEncoder {
    private static final String TAG = "UltrasoundEncoder";

    private CompletionCallback mCompletionCallback = null;
    private Handler mCompletionHandler = null;

    protected UltrasoundEncoder() {
        nativeInit(this);
    }


    /**
     * A callback for encoding completion events.
     * 
     * @see #registerCompletionCallback
     * 
     */
    public static abstract class CompletionCallback {
        /**
         * Enconding has completed.
         * 
         * @param result Completion result.
         *               <p>
         *               <code>THOR_OK</code> if encoding finished successfully.
         *               <p>
         *               <code>TER_UMGR_XXX</code> if error occurred during the encoding.
         *               <p>
         */
        public void onCompletion(ThorError result) {
        }
    }

    /**
     * Registers a callback for notification of the encoding progress. Only one
     * callback can be registered at a time.
     * 
     * @param completion The new callback to send notifications.
     * @param handler    The handler on which the callback should be invoked, or
     *                   {@code null} to use the current thread's
     *                   {@link android.os.Looper looper}.
     */
    public void registerCompletionCallback(@NonNull CompletionCallback completion, @Nullable Handler handler) {
        if (null == handler) {
            Looper looper = Looper.myLooper();

            if (null == looper) {
                throw new IllegalArgumentException("No handler given, and current thread as no looper!");
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
     * Internal handler for receiving and relaying completion events from the native
     * layer.
     * 
     * @param result The result of the recording operation.
     */
    private void onCompletion(final int result) {
        if (null != mCompletionCallback && null != mCompletionHandler) {
            mCompletionHandler.post(new Runnable() {
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
            });
        }
    }

    protected void cleanup() {
        nativeTerminate();
    }

    /**
     * Getting a rendered UltrasoundScreenImage via Bitmap object for JPEG export.
     * The image is rendered from a save file offsreen.
     * The rendered image does not contain any overlays.
     * 
     * @param srcPath Full pathname of the source file
     * @param zoomPanParams zoom, pan, and flip related params (float[5])
     * @param windowParams origin location (x, y) and window size (width, height).
     *                     all described in percentage wrt bitmap size(float[4])
     * @param bitmapImage bitmap Object. Image size needs to be defined.
     * 
     * @return ThorError 
     * <code>S_OK</code> on successful rendering.
     */

    public ThorError processStillImage(String srcPath, float[] zoomPanParams, float[] windowParams, Bitmap bitmapImage) {
        int retCode = nativeProcessStillImage(srcPath, zoomPanParams, windowParams, bitmapImage);
        ThorError retError = ThorError.fromCode(retCode);

        return retError;
    }

    /**
     * Override of processStillImage (above) with default windowParams.
     * 
     * Default window params -> {0.0f, 0.0f, 100.0f, 100.0f}.
     * 
     */
    public ThorError processStillImage(String srcPath, float[] zoomPanParams, Bitmap bitmapImage) {
        // default windows params
        float[] windowParams = new float[] {0.0f, 0.0f, 100.0f, 100.0f};
        ThorError retError = processStillImage(srcPath, zoomPanParams, windowParams, bitmapImage);

        return retError;
    }


    /**
     * Render a still image with an overlay from a Thor file and encode it to a JPEG file.
     * 
     * @param srcPath       Full pathname of the source file (.thor)
     * @param dstPath       Full pathname of the destination file (.mp4)
     * @param zoomPanParams zoom, pan, and flip related params (float[5])
     * @param windowParams  origin location (x, y) and window size (width, height).
     *                      all described in percentage wrt bitmap size (float[4]).
     *                      For M-mode, add float[4] for M-window location and size (total, 8)
     *                      For DA/ECG, add float[8] for DA and ECG layout (total, 12)
     * @param overlayImage  bitmap Object containing the overlay image. Image size
     *                      needs to be defined.
     * @param desiredFrameNum frameNum when extracting a frame from a clip (should use 0 for still)
     *
     * @return ThorError <code>S_OK</code> on successful rendering.
     */
    public ThorError encodeStillImage(String srcPath, String dstPath, float[] zoomPanParams, float[] windowParams,
                                      Bitmap overlayImage, int desiredFrameNum, Boolean rawOut) {
        int retCode = nativeEncodeStillImage(srcPath, dstPath, zoomPanParams, windowParams, overlayImage, desiredFrameNum, rawOut);
        ThorError retError = ThorError.fromCode(retCode);

        return retError;
    }


    /**
     * Encode a clip from a Thor file to a MP4 file.
     * It can be canceled during the encoding process.
     * 
     * @param srcPath       Full pathname of the source file (.thor)
     * @param dstPath       Full pathname of the destination file (.mp4)
     * @param zoomPanParams zoom, pan, and flip related params (float[5])
     * @param windowParams  origin location (x, y) and window size (width, height).
     *                      all described in percentage wrt bitmap size(float[4]).
     *                      For M-mode, add float[4] for M-window location and size (total, 8)
     *                      For DA/ECG, add float[8] for DA and ECG layout (total, 12)
     * @param overlayImage  bitmap Object containing the overlay image.
     *                      Image size needs to be defined.
     * @param startFrame    startFrame no. (0 indicates the beginning of the file)
     * @param endFrame      endFrame no. (0 indicates the ending of the file)
     * @param forcedFSR     Boolean whether device contains MP4 rendering issue or not
     *
     * @return ThorError <code>S_OK</code> on successful rendering.
     */
    public ThorError encodeClip(String srcPath, String dstPath, float[] zoomPanParams, float[] windowParams,
                                Bitmap overlayImage, int startFrame, int endFrame, boolean forcedFSR) {
        int retCode = nativeEncodeClip(srcPath, dstPath, zoomPanParams, windowParams, overlayImage, startFrame, endFrame, false, forcedFSR);
        ThorError retError = ThorError.fromCode(retCode);

        return retError;
    }

    /**
     * Override of encodeClip with the default start and end frames.
     * 
     * Default start and end frames -> 0.
     * 
     */
    public ThorError encodeClip(String srcPath, String dstPath, float[] zoomPanParams, float[] windowParams, Bitmap overlayImage) {
        int retCode = nativeEncodeClip(srcPath, dstPath, zoomPanParams, windowParams, overlayImage, 0, 0, false, false);
        ThorError retError = ThorError.fromCode(retCode);

        return retError;
    }

    /**
     * Produce a raw clip file from a Thor file (in RGBA format).
     * It can be canceled during the encoding process.
     *
     * @param srcPath       Full pathname of the source file (.thor)
     * @param dstPath       Full pathname of the destination file (.raw)
     * @param zoomPanParams zoom, pan, and flip related params (float[5])
     * @param windowParams  origin location (x, y) and window size (width, height).
     *                      all described in percentage wrt bitmap size(float[4]).
     *                      For M-mode, add float[4] for M-window location and size (total, 8)
     *
     *                      For DA/ECG, add float[8] for DA and ECG layout (total, 12)
     * @param overlayImage  bitmap Object containing the overlay image.
     *                      Image size needs to be defined.
     * @param startFrame    startFrame no. (0 indicates the beginning of the file)
     * @param endFrame      endFrame no. (0 indicates the ending of the file)
     *
     * @return ThorError <code>S_OK</code> on successful rendering.
     */
    public ThorError processClipRaw(String srcPath, String dstPath, float[] zoomPanParams, float[] windowParams,
                                Bitmap overlayImage, int startFrame, int endFrame) {
        int retCode = nativeEncodeClip(srcPath, dstPath, zoomPanParams, windowParams, overlayImage, startFrame, endFrame, true, false);
        ThorError retError = ThorError.fromCode(retCode);

        return retError;
    }

    /**
     * Request to cancel the encoding process.
     * 
     */
    public void requestCancelEncoding() {
        nativeCancelEncoding();
    }


    private native void nativeInit(UltrasoundEncoder thisObject);
    private native void nativeTerminate();

    private native void nativeCancelEncoding();

    private static native int nativeProcessStillImage(String path, 
                                                      float[] zoomPanParams,
                                                      float[] windowParams,
                                                      Bitmap bitmapImage);


    /**
     * Render a still image with an overlay from a Thor file and encode it to a JPEG file.
     *
     * @param srcPath       Full pathname of the source file (.thor)
     * @param dstPath       Full pathname of the destination file (.mp4)
     * @param zoomPanParams zoom, pan, and flip related params (float[5])
     * @param windowParams  origin location (x, y) and window size (width, height).
     *                      all described in percentage wrt bitmap size (float[4]).
     *                      For M-mode, add float[4] for M-window location and size (total, 8)
     *                      For DA/ECG, add float[8] for DA and ECG layout (total, 12)
     * @param overlayImage  bitmap Object containing the overlay image. Image size
     *                      needs to be defined.
     *
     * @param desiredFrameNum frameNum when extracting a frame from a clip (should use 0 for still)
     *
     * @return ThorError <code>S_OK</code> on successful rendering.
     */
    public ThorError processStillRaw(String srcPath, String dstPath, float[] zoomPanParams, float[] windowParams,
                                     Bitmap overlayImage, int desiredFrameNum) {
        int retCode = nativeEncodeStillImage(srcPath, dstPath, zoomPanParams, windowParams, overlayImage, desiredFrameNum, true);
        ThorError retError = ThorError.fromCode(retCode);
        return retError;
    }

    private static native int nativeEncodeStillImage(String srcPath,
                                                     String dstPath,
                                                     float[] zoomPanParams,
                                                     float[] windowParams,
                                                     Bitmap overlayImage,
                                                     int desiredFrameNum,
                                                     boolean rawOut);

    private static native int nativeEncodeClip(String srcPath,
                                               String dstPath,
                                               float[] zoomPanParams,
                                               float[] windowParams,
                                               Bitmap overlayImage,
                                               int startFrame,
                                               int endFrame,
                                               boolean rawOut,
                                               boolean forcedFSR);
}
