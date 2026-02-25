/*
 * Copyright (C) 2018 EchoNous, Inc.
 */

package com.echonous.hardware.ultrasound;

import com.echonous.util.LogUtils;

import android.os.Handler;
import android.os.Looper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

/**
 * <p>Object that represents an instance of the Thor Puck.  Used to get 
 * notifications of Puck touch events.  The UltrasoundManager is used to 
 * obtain and instance of a Puck.</p> 
 *  
 * @see UltrasoundManager#openPuck
 */
public final class Puck {
    private static final String TAG = "Puck";

    private UltrasoundManager   mManager = null;
    private boolean             mIsClosed = false;

    private PuckEventCallback   mCallback = null;
    private Handler             mHandler = null;

    private ThorError           mOpenStatus = ThorError.THOR_OK;


    protected Puck() {
    }

    protected Puck(UltrasoundManager manager) {
        mManager = manager;
    }

    protected ThorError open() {
        int retCode = nativeOpen(this);

        mOpenStatus = ThorError.fromCode(retCode);

        return mOpenStatus;
    }

    /**
     * Returns the status of opening the Puck instance.  This status is used to 
     * determine if the Puck opened successfully for scanning, or if the firmware is 
     * out of date, or if unable to communicate with the Puck.  In the later case it 
     * may be possible to initiate a firmware update to restore functionality. 
     * 
     * @return ThorError open status of the Puck
     */
    public ThorError getOpenStatus() {
        return mOpenStatus;
    }

    /**
     * Close this Puck instance.  It expected that an instance of an active Puck 
     * only exists if needed. 
     */
    public void close() {
        if (!mIsClosed) {
            nativeClose();

            mIsClosed = true;
            LogUtils.d(TAG, "Puck closed");
        }

        if (null != mManager)
        {
            mManager.releasePuck(this);
            mManager = null;
        }
    }

    /**
     * Returns the firmware version of the Puck.
     * 
     * @return String firmware version
     */
    public String getFirmwareVersion() throws DauException {
        if (mIsClosed) {
            LogUtils.e(TAG, "This Puck instance is closed");
            throw new DauException(ThorError.TER_UMGR_PUCK_CLOSED);
        }

        return nativeGetFirmwareVersion();
    }

    /**
     * Returns the firmware of the Puck library.  The library has an embedded 
     * firmware image for updating the Puck.  Generally the firmware of the Puck 
     * should match the library. 
     * 
     * @return String library version
     */
    public String getLibraryVersion() {
        return nativeGetLibraryVersion();
    }

    /**
     * Update the firmware of the Puck to the one embedded in the library.  This 
     * method is asynchronous, and a worker thread is created that does the update. 
     * As the update progresses, events will be sent through the normal PuckEvent 
     * mechanism.  See getUpdateCurPos() and getUpdateMaxPos() in PuckEvent.  The 
     * firmware update is complete once curPos matches maxPos. 
     * 
     * @return ThorError <p> 
     * <code>THOR_OK</code> If firmware update was successfully initiated 
     * <code>TER_TABLET_PUCK_UPDATE_FIRMWARE</code> If the worker thread failed to 
     * run or otherwise the update failed 
     * <code>TER_TABLET_PUCK_UPDATE_FIRMWARE_BUSY</code> If a firmware update is 
     * already in progress. 
     */
    public ThorError updateFirmware() {
        int retCode = nativeUpdateFirmware();
        ThorError updateError = ThorError.fromCode(retCode);

        return updateError;
    }

    /**
     * Set the parameters that control detection of button clicks and scrolling. See 
     * PuckParams for more information. 
     * 
     * @param puckParams the new parameters
     */
    public void setPuckParams(PuckParams puckParams) throws DauException {
        if (mIsClosed) {
            LogUtils.e(TAG, "This Puck instance is closed");
            throw new DauException(ThorError.TER_UMGR_PUCK_CLOSED);
        }

        int[] params = new int[4];

        nativeSetSingleClick(puckParams.getSingleClickMin(), puckParams.getSingleClickMax());

        nativeSetDoubleClick(puckParams.getDoubleClickMin(), puckParams.getDoubleClickMax());

        nativeSetLongClick(puckParams.getLongClickMax());

        params[0] = puckParams.getScrollThreshold(PuckParams.ScrollPosition.POS1);
        params[1] = puckParams.getScrollThreshold(PuckParams.ScrollPosition.POS2);
        params[2] = puckParams.getScrollThreshold(PuckParams.ScrollPosition.POS3);
        params[3] = puckParams.getScrollThreshold(PuckParams.ScrollPosition.POS4);
        nativeSetScrollPos(params);

        params[0] = puckParams.getScrollStep(PuckParams.ScrollPosition.POS1);
        params[1] = puckParams.getScrollStep(PuckParams.ScrollPosition.POS2);
        params[2] = puckParams.getScrollStep(PuckParams.ScrollPosition.POS3);
        params[3] = puckParams.getScrollStep(PuckParams.ScrollPosition.POS4);
        nativeSetScrollSteps(params);
    }

    /**
     * Returns the parameters that control detection of button clicks and scrolling. 
     * See PuckParams for more information. 
     * 
     * @return PuckParams the current parameters
     */
    public PuckParams getPuckParams() throws DauException {
        if (mIsClosed) {
            LogUtils.e(TAG, "This Puck instance is closed");
            throw new DauException(ThorError.TER_UMGR_PUCK_CLOSED);
        }

        PuckParams puckParams = new PuckParams();
        int[] params = new int[4];

        nativeGetSingleClick(params);
        puckParams.setSingleClickMin(params[0]);
        puckParams.setSingleClickMax(params[1]);

        nativeGetDoubleClick(params);
        puckParams.setDoubleClickMin(params[0]);
        puckParams.setDoubleClickMax(params[1]);

        nativeGetLongClick(params);
        puckParams.setLongClickMax(params[0]);

        nativeGetScrollPos(params);
        puckParams.setScrollThreshold(PuckParams.ScrollPosition.POS1, params[0]);
        puckParams.setScrollThreshold(PuckParams.ScrollPosition.POS2, params[1]);
        puckParams.setScrollThreshold(PuckParams.ScrollPosition.POS3, params[2]);
        puckParams.setScrollThreshold(PuckParams.ScrollPosition.POS4, params[3]);

        nativeGetScrollSteps(params);
        puckParams.setScrollStep(PuckParams.ScrollPosition.POS1, params[0]);
        puckParams.setScrollStep(PuckParams.ScrollPosition.POS2, params[1]);
        puckParams.setScrollStep(PuckParams.ScrollPosition.POS3, params[2]);
        puckParams.setScrollStep(PuckParams.ScrollPosition.POS4, params[3]);

        return puckParams;
    }

    /**
     * Resets all the parameters for detection of button clicks and scrolling to 
     * factory default values. 
     * 
     */
    public void setDefaultParams() throws DauException {
        if (mIsClosed) {
            LogUtils.e(TAG, "This Puck instance is closed");
            throw new DauException(ThorError.TER_UMGR_PUCK_CLOSED);
        }

        nativeSetDefault();
    }

    /**
     * Resets the Handle Controller.
     */
    public void reset() throws DauException {
        if (mIsClosed) {
            LogUtils.e(TAG, "This Puck instance is closed");
            throw new DauException(ThorError.TER_UMGR_PUCK_CLOSED);
        }

        nativeReset();
    }

    /**
     * A callback for Puck touch events. 
     *  
     * @see #registerEventCallback 
     */
    public static abstract class PuckEventCallback {
        /**
         * A new Puck event occurred.
         * 
         * @param event The new PuckEvent that describes the most recent touch event or 
         *              gesture.
         */
        public void onPuckEvent(PuckEvent event) {
        }
    }

    /**
     * Registers a callback for notification of Puck touch events. Only one callback 
     * can be registered at a time. 
     * 
     * @param callback The new callback to send Puck touch events
     * @param handler The handler on which the callback should be invoked, or 
     *                {@code null} to use the current thread's {@link
     *                android.os.Looper looper}.
     */
    public void registerEventCallback(@NonNull PuckEventCallback callback,
                                      @Nullable Handler handler) {
        if (null == handler) {
            Looper looper = Looper.myLooper();

            if (null == looper) {
                throw new IllegalArgumentException(
                    "No handler given, and current thread as no looper!");
            }

            handler = new Handler(looper);
        }

        mCallback = callback;
        mHandler = handler;
    }

    /**
     * Unregister the existing callback.
     */
    public void unregisterEventCallback() {
        mCallback = null;
        mHandler = null;
    }

    protected void onPuckEvent(final int leftButtonAction,
                               final int rightButtonAction,
                               final int palmAction,
                               final int scrollAction,
                               final int scrollAmount,
                               final int updateCurPos,
                               final int updateMaxPos,
                               final int updateStatus) {
        if (null != mCallback && null != mHandler) {
            mHandler.post(
                new Runnable() {
                    @Override
                    public void run() {
                        try {
                            mCallback.onPuckEvent(new PuckEvent(leftButtonAction,
                                                                rightButtonAction,
                                                                palmAction,
                                                                scrollAction,
                                                                scrollAmount,
                                                                updateCurPos,
                                                                updateMaxPos,
                                                                updateStatus));
                        } catch (NullPointerException ex) {
                            LogUtils.e(TAG, "Client callback handler is null", ex);

                            mCallback = null;
                            mHandler = null;
                        }
                    }
                });
        }
    }

    private static native int       nativeOpen(Puck thisObject);
    private static native void      nativeClose();
    private static native String    nativeGetFirmwareVersion();
    private static native String    nativeGetLibraryVersion();
    private static native int       nativeUpdateFirmware();
    private static native void      nativeSetSingleClick(int min, int max);
    private static native void      nativeGetSingleClick(int[] params);
    private static native void      nativeSetDoubleClick(int min, int max);
    private static native void      nativeGetDoubleClick(int[] params);
    private static native void      nativeSetLongClick(int max);
    private static native void      nativeGetLongClick(int[] params);
    private static native void      nativeSetScrollPos(int[] params);
    private static native void      nativeGetScrollPos(int[] params);
    private static native void      nativeSetScrollSteps(int[] params);
    private static native void      nativeGetScrollSteps(int[] params);
    private static native void      nativeSetDefault();
    private static native void      nativeReset();
}


