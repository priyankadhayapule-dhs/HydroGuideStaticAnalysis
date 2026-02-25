/*
 * Copyright (C) 2016 EchoNous, Inc.
 */

package com.echonous.hardware.ultrasound;

import android.os.Handler;
import android.os.Looper;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.echonous.util.LogUtils;

/**
 * <p>The Dau class represents a single DAU (Data Aquisition Unit), i.e., 
 * ultrasound probe. This is the primary interface for controlling 
 * ultrasound imaging.  The UltrasoundManager is used to obtain an instance 
 * of a Dau.</p> 
 *  
 * @see UltrasoundManager#openDau 
 */
public abstract class Dau implements AutoCloseable {
    private static final String TAG = "DauAbstract";

    private ErrorCallback       mErrorCallback = null;
    private Handler             mErrorHandler = null;

    private HidCallback         mHidCallback = null;
    private Handler             mHidHandler = null;

    private ExternalEcgCallback mExternalEcgCallback = null;
    private Handler             mExternalEcgHandler = null;

    private UltrasoundManager   mManager = null;
    private boolean             mIsCineAttached = false;


    /** 
     * To be inherited by com.echonous.hardware.ultrasound.* only. 
     *  
     * @hide
     */
    protected Dau() {
    }

    protected Dau(UltrasoundManager manager) {
        mManager = manager;
    }

    /**
     * Close all resources for this Dau instance.  After doing so, this instance 
     * cannot be "re-opened" for ultrasound image processing. 
     */
    @Override
    public void close() {
        detachCine();
        mManager.releaseDau(this);
        mManager = null;
    }

    /** 
     * Returns true if this instance is open. 
     *  
     * @return boolean <code>True</code> if this instance is open.
     */
    public abstract boolean isOpen();

    /** 
     * Returns true if running (actively imaging). 
     *  
     * @return boolean <code>True</code> if running.
     */
    public abstract boolean isRunning();

    protected UltrasoundManager getManager() {
        return mManager;
    }

    /**
     * Get the type of Dau in the form of a model number.
     * 
     * @return The identifier of this Dau
     */
    public abstract String getId();

    /**
     * Returns true if the Dau supports ECG
     *
     * @return boolean <code>True</code> if ECG supported
     */
    public abstract boolean supportsEcg();

    /**
     * Returns true if the Dau supports DA
     *
     * @return boolean <code>True</code> if DA supported
     */
    public abstract boolean supportsDa();

    /**
     * Returns true if the element check is running
     *
     * @return boolean <code>True</code> if element check running
     */
    public abstract boolean isElementCheckRunning();

    /**
     * Returns the path to the UPS for this Dau.
     *
     * @return String Ups Path
     */
    public abstract String getUpsPath();

    /**
     * Registers a callback for notification of Dau imaging errors.  Only one 
     * callback can be registered at a time. 
     * 
     * @param callback The new callback to send errors to
     * @param handler The handler on which the callback should be invoked, or 
     *                {@code null} to use the current thread's {@link
     *                android.os.Looper looper}.
     */
    public void registerErrorCallback(@NonNull ErrorCallback callback, 
                                      @Nullable Handler handler) {
        if (null == handler) {
            Looper looper = Looper.myLooper();

            if (null == looper) {
                throw new IllegalArgumentException(
                    "No handler given, and current thread as no looper!");
            }

            handler = new Handler(looper);
        }

        mErrorCallback = callback;
        mErrorHandler = handler;
    }

    /**
     * Unregister the existing callback.
     */
    public void unregisterErrorCallback() {
        mErrorCallback = null;
        mErrorHandler = null;
    }

    /**
     * Registers a callback to be notified of Dau HID input. Only one callback can 
     * be registered at a time. 
     * 
     * @param callback The new callback to send HID events
     * @param handler The handler on which the callback should be invoked, or 
     *                {@code null} to use the current thread's {@link
     *                android.os.Looper looper}.
     */
    public void registerHidCallback(@NonNull HidCallback callback, 
                                    @Nullable Handler handler) {
        if (null == handler) {
            Looper looper = Looper.myLooper();

            if (null == looper) {
                throw new IllegalArgumentException(
                    "No handler given, and current thread as no looper!");
            }

            handler = new Handler(looper);
        }

        mHidCallback = callback;
        mHidHandler = handler;
    }

    /**
     * Unregister the existing callback.
     */
    public void unregisterHidCallback() {
        mHidCallback = null;
        mHidHandler = null;
    }

    /**
     * Registers a callback to be notified of Dau External ECG lead connection. Only
     * one callback can be registered at a time. 
     * 
     * @param callback The new callback to send External ECG connection events
     * @param handler The handler on which the callback should be invoked, or 
     *                {@code null} to use the current thread's {@link
     *                android.os.Looper looper}.
     */
    public void registerExternalEcgCallback(@NonNull ExternalEcgCallback callback, 
                                    @Nullable Handler handler) {
        if (null == handler) {
            Looper looper = Looper.myLooper();

            if (null == looper) {
                throw new IllegalArgumentException(
                    "No handler given, and current thread as no looper!");
            }

            handler = new Handler(looper);
        }

        mExternalEcgCallback = callback;
        mExternalEcgHandler = handler;
    }

    /**
     * Unregister the existing callback.
     */
    public void unregisterExternalEcgCallback() {
        mExternalEcgCallback = null;
        mExternalEcgHandler = null;
    }

    /**
     * Open this instance and initialize.  This method is intended for use by the 
     * UltrasoundManager. 
     *  
     * @hide 
     */
    protected abstract ThorError open(DauContext context);

    /**
     * Get probe information (type, serial number, etc.)
     * 
     */
    public abstract ProbeInformation getProbeInformation();

    /**
     * Set the Debug flag to enable the DauRegRWService
     *
     */
    public abstract void setDebugFlag(boolean flag);

    public abstract void setDauSafetyFeatureTestOption(int dauSafetyTestSelectedOption, float dauThresholdTemperatureForTest);


    /**
     * Stop ultrasound image processing.
     */
    public abstract ThorError stop();

    /**
     * Start ultrasound image processing.  Will paint the resulting images onto the 
     * surface provided when this Dau instance was created. 
     */
    public abstract void start();

    /**
     * Set up the imaging engine parameters based on the specified 
     * imaging ID.  The ID comes from the UPS (Ultrasound Parameter Storage) 
     * database. 
     * <p> 
     * Note: It is currently assumed that imaging is stopped before calling this 
     * method. It is also assumed that the supplied imageCaseId is valid.
     * 
     * @param imagingCaseId 
     *  
     * @see UltrasoundManager#getUpsPath 
     *  
     * @deprecated The four parameter variant supercedes this one. 
     */
    public abstract ThorError setImagingCase(int imagingCaseId);

    /**
     * Sets the image case based on the supplied parameters.  The parameters 
     * are used to determine the correct imaging ID. The various supplies IDs come 
     * from the UPS (Ultrasound Parameter Storage). 
     * <p> 
     * Note: It is currently assumed that imaging is stopped before calling this 
     * method. 
     *  
     * @param organId 
     * @param viewId 
     * @param depthId 
     * @param modeId
     * @param apiEstimates[8] - Predictions from the AP&I Black Box
     *        On return, this array is expected to contain the following:
     *        [0] : MI
     *        [1] : TIS
     *        [2] : TIB
     *        [3] : TIC
     *        [4:7]  content to be determined
     * @params imagingInfo[8]
     *        On return, this array is expected to contain the following:
     *        [0] : frame rate
     *        [1] : center frequency
     *        [2] : imaging duty cycle (for engineering use only)
     *        [3] : transmit voltage (for engineering use only)
     *        [4:7] : content to be determined
     * @return <p> 
     * <code>THOR_OK</code> on success<p>
     * <code>TER_IE_IMAGE_PARAM</code> if one or more supplied IDs are invalid.<p> 
     * <code>TER_IE_UPS_ACCESS</code> if failed to access the UPS database. 
     *  
     * @see UltrasoundManager#getUpsPath
     */
    public abstract ThorError setImagingCase(int organId,
                                             int viewId,
                                             int depthId,
                                             int modeId,
                                             float[] apiEstimates,
                                             float[] imagingInfo);

    /**
     * Set up the imaging case for colorflow imaging.
     * <p> 
     * Note: It is currently assumed that the supplied combination of
     * oranId, viewId and depthId for colorflow mode exists in the UPS.
     *
     * @param organId - index of the selected organ
     * @param viewId  - index of the selected BMI
     * @param depthId - index of the selected depth
     * @param colorParams[] - {prfIndex, flowSpeedIndex, wallFilterIndex, maxPRF(ret), ColorMode}
     *        On return, prfIndex will be the actual PRF index used for acquisition.
     * @param roiParams[] - {axialStart, axialSpan, azimuthStart, azimuthSpan
     *        On return, values contain in roiParams[] will contain actual values
     * @param apiEstimates[8] - Predictions from the AP&I Black Box
     *      On return, this array is expected to contain the following:
     *      [0] : MI
     *      [1] : TI
     *      [2:7] : content to be determined
     * @param imagingInfo[8]
     *      On return, this array is expected to contain the following:
     *      [0] : frame rate
     *      [1] : center frequency
     *      [2] : imaging duty cycle (for engineering use only)
     *      [3] : transmit voltage (for engineering use only)
     *      [4:7] : content to be determned
     * 
     * @return <p>
     * <code>THOR_IK</code> on success<p>
     * <code>TER_IE_IMAGE_PARAM</code> if one or more supplied IDs are invalid<p>
     * <code>TER_IE_UPS_ACCESS</code> if failed to access the UPS database.
     *
     */
    public abstract ThorError setColorImagingCase(int organId,
                                                  int viewId,
                                                  int depthId,
                                                  int[] colorParams,
                                                  float[] roiParams,
                                                  float[] apiEstimates,
                                                  float[] imagingInfo);

    /**
     * Set up the imaging case for colorflow imaging.
     * <p>
     * Note: It is currently assumed that the supplied combination of
     * oranId, viewId and depthId for colorflow mode exists in the UPS.
     *
     * @param imagingCaseId - index of the imaging case to be used with PW
     * @param pwIndices[] - {prfIndex, wallFilterIndex, gateIndex, baseline index, sweep speed index,
     *                       baseline invert}
     *        On return, prfIndex will be the actual PRF index used for acquisition.
     * @param gateParams[] - {gateStart, beam Angle, gate Angle}
     *        On return, values contained in gateParams[] will contain actual values
     * @param apiEstimates[8] - Predictions from the AP&I Black Box
     *      On return, this array is expected to contain the following:
     *      [0] : MI
     *      [1] : TI
     *      [2:7] : content to be determined
     * @param imagingInfo[8]
     *      On return, this array is expected to contain the following:
     *      [0] : PW center frequency
     *      [1] : N/A
     *      [2] : N/A
     *      [3] : N/A
     *      [4] : Tx voltage
     *      [5] : Doppler PRF
     *      [6] : Doppler Sweep Speed
     *      [7] : content to be determned
     *@param isTDI - set true if PW TDI
     *
     * @return <p>
     * <code>THOR_IK</code> on success<p>
     * <code>TER_IE_IMAGE_PARAM</code> if one or more supplied IDs are invalid<p>
     * <code>TER_IE_UPS_ACCESS</code> if failed to access the UPS database.
     *
     */
    public abstract ThorError setPwImagingCase(int imagingCaseId,
                                               int[] pwIndices,
                                               float[] gateParams,
                                               float[] apiEstimates,
                                               float[] imagingInfo,
                                               boolean isTDI);

    /**
     * Set up the imaging case for CW imaging.
     * <p>
     * Note: It is currently assumed that the supplied combination of
     * oranId, viewId and depthId for CW mode exists in the UPS.
     *
     * @param imagingCaseId - index of the imaging case to be used with CW
     * @param cwIndices[] - {prfIndex (Unused), wallFilterIndex, gateIndex (Unused), baseline index, sweep speed index,
     *                       baseline invert}
     *        On return, prfIndex will be the actual PRF index used for acquisition.
     * @param gateParams[] - {gateStart, beam Angle, gate Angle}
     *        On return, values contained in gateParams[] will contain actual values
     * @param apiEstimates[8] - Predictions from the AP&I Black Box
     *      On return, this array is expected to contain the following:
     *      [0] : MI
     *      [1] : TI
     *      [2:7] : content to be determined
     * @param imagingInfo[8]
     *      On return, this array is expected to contain the following:
     *      [0] : CW center frequency
     *      [1] : N/A
     *      [2] : N/A
     *      [3] : N/A
     *      [4] : Tx voltage
     *      [5] : Doppler PRF
     *      [6] : Doppler Sweep Speed
     *      [7] : content to be determined
     *
     * @return <p>
     * <code>THOR_IK</code> on success<p>
     * <code>TER_IE_IMAGE_PARAM</code> if one or more supplied IDs are invalid<p>
     * <code>TER_IE_UPS_ACCESS</code> if failed to access the UPS database.
     *
     */
    public abstract ThorError setCwImagingCase(int imagingCaseId,
                                               int[] cwIndices,
                                               float[] gateParams,
                                               float[] apiEstimates,
                                               float[] imagingInfo);


    /**
     * Set up the imaging case for M-Mode imaging.
     * <p> 
     * Note: It is currently assumed that the supplied combination of
     * oranId, viewId and depthId for colorflow mode exists in the UPS.
     *
     * @param organId       - index of the selected organ
     * @param viewId        - index of the selected BMI
     * @param depthId       - index of the selected depth
     * @param scrollSpeedId - index of the selected scroll speed
     * @param mLinePosition - position of M-line in the field of view expressed in radians for the phased array
     * @param apiEstimates[8] - Predictions from the AP&I Black Box
     *        On return, this array is expected to contain the following:
     *        [0] : MI
     *        [1] : TI
     *        [2:7]  content to be determined
     * @params imagingInfo[8]
     *        On return, this array is expected to contain the following:
     *        [0] : frame rate
     *        [1] : center frequency
     *        [2] : imaging duty cycle (for engineering use only)
     *        [3] : transmit voltage (for engineering use only)
     *        [4:7] : content to be determined
     * @return <p>
     * <code>THOR_IK</code> on success<p>
     * <code>TER_IE_IMAGE_PARAM</code> if one or more supplied IDs are invalid<p>
     * <code>TER_IE_UPS_ACCESS</code> if failed to access the UPS database.
     *
     */
    public abstract ThorError setMmodeImagingCase(int organId,
                                                  int viewId,
                                                  int depthId,
                                                  int scrollSpeedId,
                                                  float mLinePosition,
                                                  float[] apiEstimates,
                                                  float[] imagingInfo);
    /**
     * Set M Line position
     * <p> 
     * This method should be called after setMmodeImagingCase is called.
     *
     * @param mLinePosition - position of the M line in the field of view.
     * 
     * @return <p>
     * <code>THOR_IK</code> on success<p>
     * <code>TER_IE_IMAGE_PARAM</code> if one or more supplied IDs are invalid<p>
     * <code>TER_IE_UPS_ACCESS</code> if failed to access the UPS database.
     *
     */
    public abstract ThorError setMLine(float mLinePosition);

    /**
     * Available imaging processing that can be selectively enabled.
     */
    public enum ImageProcessingType {
        /**
         * If selected, then gain will automatically be set in software.  setGain() will 
         * then alter this gain algorithm.  If disabled, setGain() will manually control
         * the gain value in hardware. 
         */
        IMAGE_PROCESS_AUTO_GAIN,

        /**
         * If enabled, then speckles will be reduced.
         */
        IMAGE_PROCESS_DESPECKLE
    }

    /**
     * Enable / Disable an imaging process.
     * 
     * @param type The image process to enable
     * @param enable <code>true</code> to enable, <code>false</code> to disable
     */
    public abstract void enableImageProcessing(ImageProcessingType type,
                                               boolean enable);

    /**
     * Set the overall gain in B-Mode.
     * 
     * 
     * @param gain Accepts a value from 0 to 100. 
     *  
     * @return {@code THOR_OK} if successful.  Will return {@code TER_IE_GAIN} if 
     *         specified gain is out of range.
     */
    public abstract ThorError setGain(int gain);

    /**
     * Set the near and far gains in B-Mode.
     * 
     * 
     * @param nearGain Accepts a value from 0 to 100. 
     * @param farGain Accepts a value from 0 to 100. 
     *  
     * @return {@code THOR_OK} if successful.  Will return {@code TER_IE_GAIN} if 
     *         specified gain is out of range.
     */
    public abstract ThorError setGain(int nearGain, int farGain);

    /**
     * Set the overall Colorflow gain
     * 
     * 
     * @param gain Accepts a value from 0 to 100. 
     *  
     * @return {@code THOR_OK} if successful.  Will return {@code TER_IE_GAIN} if 
     *         specified gain is out of range.
     */
    public abstract ThorError setColorGain(int gain);

    /**
     * Set the overall Ecg gain
     *
     *
     * @param gain Accepts a float value from -100 to 100 in dB.
     *
     * @return {@code THOR_OK} if successful.  Will return {@code TER_IE_GAIN} if
     *         specified gain is out of range.
     */
    public abstract ThorError setEcgGain(float gain);

    /**
     * Set the overall Da gain
     *
     *
     * @param gain Accepts a float value from -100 to 100 in dB.
     *
     * @return {@code THOR_OK} if successful.  Will return {@code TER_IE_GAIN} if
     *         specified gain is out of range.
     */
    public abstract ThorError setDaGain(float gain);

    /**
     * Set the overall Pw gain
     *
     *
     * @param gain Accepts a value from 0 to 100.
     *
     * @return {@code THOR_OK} if successful.  Will return {@code TER_IE_GAIN} if
     *         specified gain is out of range.
     */
    public abstract ThorError setPwGain(int gain);

    /**
     * Set the overall Pw Audio gain
     *
     *
     * @param gain Accepts a value from 0 to 100.
     *
     * @return {@code THOR_OK} if successful.  Will return {@code TER_IE_GAIN} if
     *         specified gain is out of range.
     */
    public abstract ThorError setPwAudioGain(int gain);

    /**
     * Set the overall Cw gain
     *
     *
     * @param gain Accepts a value from 0 to 100.
     *
     * @return {@code THOR_OK} if successful.  Will return {@code TER_IE_GAIN} if
     *         specified gain is out of range.
     */
    public abstract ThorError setCwGain(int gain);

    /**
     * Set the overall Cw Audio gain
     *
     *
     * @param gain Accepts a value from 0 to 100.
     *
     * @return {@code THOR_OK} if successful.  Will return {@code TER_IE_GAIN} if
     *         specified gain is out of range.
     */
    public abstract ThorError setCwAudioGain(int gain);

    /**
     *  Set US Signal to be on and off when DAU start
     *  If set as false than DAU will not capture US Signal
     *
     * @param isUSSignal
     */
    public abstract void setUSSignal(boolean isUSSignal);

    /**
     *
     * @param isECGSignal
     */
    public abstract void setECGSignal(boolean isECGSignal);

    /**
     *
     * @param isDASignal
     */
    public abstract void setDASignal(boolean isDASignal);

    /**
     *
     * @param isExternalEcg pass true if using External Electrodes else it will use internal Electrodes
     */
    public abstract void setExternalEcg(boolean isExternalEcg);

    /**
     * @param leadNumber Set lead number when External ECG electrode are connected, It must be configure before Dau.start() take place
     */
    public abstract void setLeadNumber(int leadNumber);

    /**
     * Get B-Mode FOV for the specified imaging case
     * 
     * @param depthId
     * @param imagingMode
     * @param fov[4] 
     *        fov[0] - axialStart
     *        fov[1] - axialSpan
     *        fov[2] - azimuthStart
     *        fov[3] - azimuthSpan
     *
     * @return {@code THOR_OK} if successful.  Will return {@code TER_IE_INVALID_FOV} if 
     *         specified gain is out of range.
     */
    public abstract ThorError getFov(int depthId, int imagingMode, float[] fov );

    /**
     * Get Default ROI for the specified organ and imaging depth
     * 
     * @param organId
     * @param depthId
     * @param roi[4] 
     *        roi[0] - axialStart
     *        roi[1] - axialSpan
     *        roi[2] - azimuthStart
     *        roi[3] - azimuthSpan
     *
     * @return {@code THOR_OK} if successful.  Will return {@code TER_IE_INVALID_FOV} if 
     *         specified gain is out of range.
     */
    public abstract ThorError getDefaultRoi(int organId, int depthId, float[] roi );

    /**
     * Get FrameInterval in ms
     * 
     */
    public abstract int getFrameIntervalMs();

    /**
     * Get M-lines per frame paramter 
     *  only valid in M-mode
     * 
     */
    public abstract int getMLinesPerFrame();

    /**
     * Get external ECG lead connection status. 
     *  
     */
    public abstract boolean isExternalEcgConnected();

    /**
     * Execute transducer element check and get results
     */
    public abstract ThorError runTransducerElementCheck( int[] results );

    /** 
     * A callback for errors that occur while imaging.
     *  
     * @see #registerErrorCallback 
     *  
     */ 
    public static abstract class ErrorCallback {

        /** 
         *  An error has occurred while imaging.
         *  
         *  @param thorError The cause of the error.
         */
        public void onError(ThorError thorError) {
        }
    }

    /** 
     * A callback for HID (Human Interface Device) input from the Dau.
     *  
     * @see #registerHidCallback 
     *  
     */ 
    public static abstract class HidCallback {

        /** 
         *  A HID input has occurred.
         *  
         *  @param hidId The ID of the HID input.
         */
        public void onHid(int hidId) {
        }
    }

    /** 
     * A callback for External ECG connection from the Dau.
     *  
     * @see #registerExternalEcgCallback 
     *  
     */ 
    public static abstract class ExternalEcgCallback {

        /** 
         *  An external ECG connection event has occurred.
         *  
         *  @param isConnected The external ECG connection status.
         */
        public void onExternalEcg(boolean isConnected) {
        }
    }

    /**
     * Internal handler for receiving errors from the native layer.
     * 
     * @param errorCode The error code. 
     *  
     * @hide 
     */
    protected void onError(final int errorCode) {
        LogUtils.d(TAG, "errorCode = 0x" + Integer.toHexString(errorCode));

        if (null != mErrorCallback && null != mErrorHandler) {
            mErrorHandler.post(
                new Runnable() {
                    @Override
                    public void run() {
                        try {
                            ThorError thorError = ThorError.fromCode(errorCode);
                            mErrorCallback.onError(thorError);
                        } catch (NullPointerException ex) {
                            LogUtils.e(TAG, "Client error callback handler is null", ex);

                            mErrorCallback = null;
                            mErrorHandler = null;
                        }
                    }
                });
        }
    }

    /**
     * Internal handler for receiving HID input the native layer.
     * 
     * @param hidID The ID of the HID input.
     *  
     * @hide 
     */
    protected void onHid(final int hidId) {
        LogUtils.d(TAG, "hid ID = " + hidId);

        if (null != mHidCallback && null != mHidHandler) {
            mHidHandler.post(
                new Runnable() {
                    @Override
                    public void run() {
                        try {
                            mHidCallback.onHid(hidId);
                        } catch (NullPointerException ex) {
                            LogUtils.e(TAG, "Client HID callback handler is null", ex);

                            mHidCallback = null;
                            mHidHandler = null;
                        }
                    }
                });
        }
    }

    /**
     * Internal handler for receiving external ECG connection status from the native
     * layer. 
     * 
     * @param isConnected The connection status of the external ECG leads.
     *  
     * @hide 
     */
    protected void onExternalEcg(final boolean isConnected) {
        LogUtils.d(TAG, "isConnected = " + isConnected);

        if (null != mExternalEcgCallback && null != mExternalEcgHandler) {
            mExternalEcgHandler.post(
                new Runnable() {
                    @Override
                    public void run() {
                        try {
                            mExternalEcgCallback.onExternalEcg(isConnected);
                        } catch (NullPointerException ex) {
                            LogUtils.e(TAG, "Client External ECG callback handler is null", ex);

                            mExternalEcgCallback = null;
                            mExternalEcgHandler = null;
                        }
                    }
                });
        }
    }

    /**
     * Attach the Dau to the Cine Buffer.  The Dau will not function unless attached 
     * to the Cine Buffer. 
     */
    public void attachCine() {
        if (mManager.attachCineProducer(this)) {
            mIsCineAttached = true;
        }
    }

    /**
     * Detach the Dau from the Cine Buffer.
     */
    public void detachCine() {
        mManager.detachCineProducer(this);
        mIsCineAttached = false;
    }

    /** 
     * Returns <code>true</code> if the Dau is attached to the Cine 
     *         Buffer. 
     *  
     * @return boolean <code>True</code> if the Dau is attached to the Cine Buffer.
     */
    public boolean isCineAttached() {
        return mIsCineAttached;
    }
}
