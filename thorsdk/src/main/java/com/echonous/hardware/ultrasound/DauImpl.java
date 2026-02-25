/*
 * Copyright (C) 2016 EchoNous, Inc.
 */

package com.echonous.hardware.ultrasound;

import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteException;

import com.echonous.util.LogUtils;


public final class DauImpl extends Dau {
    private static final String TAG = "Dau";

    private DauContext mDauContext;

    private boolean mIsRunning = false;
    private boolean mIsClosed = false;
    private SQLiteDatabase  mDb = null;

    protected DauImpl(UltrasoundManager manager) {
        super(manager);
    }

    private DauImpl() {
        super();
    }

    @Override
    public synchronized void close() {
        if (!mIsClosed) {
            nativeClose();
            if (null != mDb) {
                mDb.close();
                mDb = null;
            }

            super.close();

            mIsClosed = true;
            LogUtils.d(TAG, "Dau closed");
        }
    }

    @Override
    public synchronized boolean isOpen() {
        return !mIsClosed;
    }

    @Override
    public synchronized boolean isRunning() {
        return mIsRunning;
    }

    @Override
    public String getId() {
        return "DauImpl";
    }

    @Override
    public boolean supportsEcg() {
        return nativeSupportsEcg();
    }

    @Override
    public boolean supportsDa() {
        return nativeSupportsDa();
    }

    @Override
    public boolean isElementCheckRunning() {
        return nativeIsElementCheckRunning();
    }

    @Override
    public String getUpsPath() {
        String  upsPath;

        upsPath = mDauContext.mAndroidContext.getDataDir().getAbsolutePath();
        upsPath += "/" + nativeGetDbName();

        return upsPath;
    }

    @Override
    public synchronized ThorError stop() {
        ThorError retVal = ThorError.THOR_OK;
        if (mIsClosed) {
            LogUtils.e(TAG, "This Dau instance is closed");
            return ThorError.TER_UMGR_DAU_CLOSED;
        }

        if (mIsRunning) {
            LogUtils.d(TAG, "stopping...");
            int retCode = nativeStop();
            retVal = ThorError.fromCode(retCode);
            mIsRunning = false;
        }
        else {
            LogUtils.w(TAG, "Dau is already stopped!");
        }
        return retVal;
    }

    @Override
    public synchronized void start() {
        if (mIsClosed) {
            LogUtils.e(TAG, "This Dau instance is closed");
            return;
        }

        if (!isCineAttached()) {
            LogUtils.e(TAG, "Cannot start imaging unless Cine is attached");
            return;
        }

        if (!mIsRunning) {
            LogUtils.d(TAG, "starting...");
            nativeStart();
            mIsRunning = true;
        }
        else {
            LogUtils.w(TAG, "Dau is already running!");
        }
    }

    @Override
    public synchronized ThorError setImagingCase(int imagingCaseId) {
        float[] apiEstimates = new float[8];
        float[] imagingInfo = new float[8];
        if (mIsClosed) {
            LogUtils.e(TAG, "This Dau instance is closed");
            return ThorError.TER_UMGR_DAU_CLOSED;
        }
        int retCode = nativeSetImagingCase(imagingCaseId, apiEstimates, imagingInfo);
        return ThorError.fromCode(retCode);
    }

    @Override
    public synchronized ThorError setImagingCase(int organId,
                                                 int viewId,
                                                 int depthId,
                                                 int modeId,
                                                 float[] apiEstimates,
                                                 float[] imagingInfo) {
        Cursor          cursor;
        ThorError       retVal = ThorError.THOR_OK;

        if (mIsClosed) {
            LogUtils.e(TAG, "This Dau instance is closed");
            return ThorError.TER_IE_DAU_CLOSED;
        }

        try {
            cursor = mDb.query("ImagingCases",
                               new String[] {"ID"},
                    "Organ=? and View=? and Depth=? and Mode=?",
                               new String[] {String.valueOf(organId),
                                             String.valueOf(viewId),
                                             String.valueOf(depthId),
                                             String.valueOf(modeId)},
                               null,
                               null,
                               null);
            if (cursor.getCount() != 1) {
                retVal = ThorError.TER_IE_IMAGE_PARAM;
            }
            else {
                cursor.moveToFirst();
                int retCode = nativeSetImagingCase(cursor.getInt(0), apiEstimates, imagingInfo);
                retVal = ThorError.fromCode(retCode);
            }
        } catch (SQLiteException ex) {
            retVal = ThorError.TER_IE_UPS_ACCESS;
        }

        if (!retVal.isOK()) {
            LogUtils.e(TAG, retVal.getDescription());
        }

        return retVal;
    }

    @Override
    public synchronized ThorError setColorImagingCase(int organId,
                                                      int viewId,
                                                      int depthId,
                                                      int[] colorParams,
                                                      float[] roiParams,
                                                      float[] apiEstimates,
                                                      float[] imagingInfo) {
        Cursor          cursor;
        ThorError       retVal = ThorError.THOR_OK;

        if (mIsClosed) {
            LogUtils.e(TAG, "This Dau instance is closed");
            return ThorError.TER_IE_DAU_CLOSED;
        }

        try {
            // The following reversal was needed to address THR-1037
            if (colorParams[1] == 0) {
                colorParams[1] = 2;
            }
            else if (colorParams[1] == 2) {
                colorParams[1] = 0;
            }
            cursor = mDb.query("ImagingCases",
                               new String[] {"ID"},
                    "Organ=? and View=? and Depth=? and FlowStateLevelID=? and Mode=1",
                               new String[] {String.valueOf(organId),
                                             String.valueOf(viewId),
                                             String.valueOf(depthId),
                                             String.valueOf(colorParams[1])},
                               null,
                               null,
                               null);
            if (cursor.getCount() != 1) {
                retVal = ThorError.TER_IE_IMAGE_PARAM;
            }
            else {
                cursor.moveToFirst();
                int retCode = nativeSetColorImagingCase(cursor.getInt(0),
                                                        colorParams,
                                                        roiParams,
                                                        apiEstimates,
                                                        imagingInfo);
                retVal = ThorError.fromCode(retCode);
            }
        } catch (SQLiteException ex) {
            retVal = ThorError.TER_IE_UPS_ACCESS;
        }

        if (!retVal.isOK()) {
            LogUtils.e(TAG, retVal.getDescription());
        }

        return retVal;
    }

    @Override
    public synchronized ThorError setPwImagingCase(int imagingCaseId,
                                                   int[] pwIndices,
                                                   float[] gateParams,
                                                   float[] apiEstimates,
                                                   float[] imagingInfo,
                                                   boolean isTDI) {
        Cursor          cursor;
        ThorError       retVal = ThorError.THOR_OK;

        if (mIsClosed) {
            LogUtils.e(TAG, "This Dau instance is closed");
            return ThorError.TER_IE_DAU_CLOSED;
        }

        int retCode = nativeSetPwImagingCase(imagingCaseId,
                                                pwIndices,
                                                gateParams,
                                                apiEstimates,
                                                imagingInfo,
                                                isTDI);
        retVal = ThorError.fromCode(retCode);

        if (!retVal.isOK()) {
            LogUtils.e(TAG, retVal.getDescription());
        }
        return retVal;
    }

    @Override
    public synchronized ThorError setCwImagingCase(int imagingCaseId,
                                                   int[] cwIndices,
                                                   float[] gateParams,
                                                   float[] apiEstimates,
                                                   float[] imagingInfo) {
        Cursor          cursor;
        ThorError       retVal = ThorError.THOR_OK;

        if (mIsClosed) {
            LogUtils.e(TAG, "This Dau instance is closed");
            return ThorError.TER_IE_DAU_CLOSED;
        }

        int retCode = nativeSetCwImagingCase(imagingCaseId,
                cwIndices,
                gateParams,
                apiEstimates,
                imagingInfo);
        retVal = ThorError.fromCode(retCode);

        if (!retVal.isOK()) {
            LogUtils.e(TAG, retVal.getDescription());
        }
        return retVal;
    }

    @Override
    public synchronized ThorError setMmodeImagingCase(int organId,
                                                      int viewId,
                                                      int depthId,
                                                      int scrollSpeedId,
                                                      float mLinePosition,
                                                      float[] apiEstimates,
                                                      float[] imagingInfo) {
        Cursor          cursor;
        ThorError       retVal = ThorError.THOR_OK;

        if (mIsClosed) {
            LogUtils.e(TAG, "This Dau instance is closed");
            return ThorError.TER_IE_DAU_CLOSED;
        }

        try {
            cursor = mDb.query("ImagingCases",
                               new String[] {"ID"},
                    "Organ=? and View=? and Depth=? and Mode=3",
                               new String[] {String.valueOf(organId),
                                             String.valueOf(viewId),
                                             String.valueOf(depthId)},
                               null,
                               null,
                               null);
            if (cursor.getCount() != 1) {
                retVal = ThorError.TER_IE_IMAGE_PARAM;
            }
            else {
                cursor.moveToFirst();
                int retCode = nativeSetMmodeImagingCase(cursor.getInt(0),
                                                        scrollSpeedId,
                                                        mLinePosition,
                                                        apiEstimates,
                                                        imagingInfo);
                retVal = ThorError.fromCode(retCode);
            }
        } catch (SQLiteException ex) {
            retVal = ThorError.TER_IE_UPS_ACCESS;
        }

        if (!retVal.isOK()) {
            LogUtils.e(TAG, retVal.getDescription());
        }

        return retVal;
    }

    @Override
    public synchronized ThorError setMLine(float mLinePosition) {
        ThorError retVal = ThorError.THOR_OK;
        int retCode = 0;
        
        if (mIsClosed) {
            LogUtils.e(TAG, "This Dau instance is closed");
            return ThorError.TER_IE_DAU_CLOSED;
        }

        retCode = nativeSetMLine(mLinePosition);

        return ThorError.fromCode(retCode);
    }

    @Override
    public synchronized void enableImageProcessing(ImageProcessingType type, boolean enable) {
        boolean restart = false;

        if (mIsRunning) {
            restart = true;
            stop();
        }

        nativeEnableImageProcessing(type.ordinal(), enable); 

        if (restart) {
            start();
        }
    }

    @Override
    public synchronized ThorError setGain(int gain) {
        if (mIsClosed) {
            LogUtils.e(TAG, "This Dau instance is closed");
            return ThorError.TER_IE_DAU_CLOSED;
        }

        if (gain < 0 || gain > 100) {
            LogUtils.e(TAG, "gain value must be between 0 and 100");
            return ThorError.TER_IE_GAIN;
        }

        nativeSetGain(gain);

        return ThorError.THOR_OK;
    }

    @Override
    public synchronized ThorError setGain(int nearGain, int farGain) {
        if (mIsClosed) {
            LogUtils.e(TAG, "This Dau instance is closed");
            return ThorError.TER_IE_DAU_CLOSED;
        }

        if (nearGain < 0 || nearGain > 100 || farGain < 0 || farGain > 100) {
            LogUtils.e(TAG, "gain value must be between 0 and 100");
            return ThorError.TER_IE_GAIN;
        }

        nativeSetNearFarGain(nearGain, farGain);

        return ThorError.THOR_OK;
    }

    @Override
    public synchronized ThorError setColorGain(int gain) {
        if (mIsClosed) {
            LogUtils.e(TAG, "This Dau instance is closed");
            return ThorError.TER_IE_DAU_CLOSED;
        }

        if (gain < 0 || gain > 100) {
            LogUtils.e(TAG, "color gain value must be between 0 and 100");
            return ThorError.TER_IE_GAIN;
        }

        nativeSetColorGain(gain);

        return ThorError.THOR_OK;
    }

    @Override
    public synchronized ThorError setEcgGain(float gain) {
        if (mIsClosed) {
            LogUtils.e(TAG, "This Dau instance is closed");
            return ThorError.TER_IE_DAU_CLOSED;
        }

        if (gain < -100.0 || gain > 100.0) {
            LogUtils.e(TAG, "ecg gain value must be between -100 and 100 dB");
            return ThorError.TER_IE_GAIN;
        }

        nativeSetEcgGain(gain);

        return ThorError.THOR_OK;
    }

    @Override
    public synchronized ThorError setDaGain(float gain) {
        if (mIsClosed) {
            LogUtils.e(TAG, "This Dau instance is closed");
            return ThorError.TER_IE_DAU_CLOSED;
        }

        if (gain < -100.0 || gain > 100.0) {
            LogUtils.e(TAG, "da gain value must be between -100 and 100 dB");
            return ThorError.TER_IE_GAIN;
        }

        nativeSetDaGain(gain);

        return ThorError.THOR_OK;
    }

    @Override
    public synchronized ThorError setPwGain(int gain) {
        if (mIsClosed) {
            LogUtils.e(TAG, "This Dau instance is closed");
            return ThorError.TER_IE_DAU_CLOSED;
        }

        if (gain < 0 || gain > 100)
        {
            LogUtils.e(TAG, "pw gain value must be between 0 and 100");
            return ThorError.TER_IE_GAIN;
        }

        nativeSetPwGain(gain);

        return ThorError.THOR_OK;
    }

    @Override
    public synchronized ThorError setPwAudioGain(int gain) {
        if (mIsClosed) {
            LogUtils.e(TAG, "This Dau instance is closed");
            return ThorError.TER_IE_DAU_CLOSED;
        }

        if (gain < 0 || gain > 100)
        {
            LogUtils.e(TAG, "pw audio gain value must be between 0 and 100");
            return ThorError.TER_IE_GAIN;
        }

        nativeSetPwAudioGain(gain);

        return ThorError.THOR_OK;
    }

    @Override
    public synchronized ThorError setCwGain(int gain) {
        if (mIsClosed) {
            LogUtils.e(TAG, "This Dau instance is closed");
            return ThorError.TER_IE_DAU_CLOSED;
        }

        if (gain < 0 || gain > 100)
        {
            LogUtils.e(TAG, "cw gain value must be between 0 and 100");
            return ThorError.TER_IE_GAIN;
        }

        nativeSetCwGain(gain);

        return ThorError.THOR_OK;
    }

    @Override
    public synchronized ThorError setCwAudioGain(int gain) {
        if (mIsClosed) {
            LogUtils.e(TAG, "This Dau instance is closed");
            return ThorError.TER_IE_DAU_CLOSED;
        }

        if (gain < 0 || gain > 100)
        {
            LogUtils.e(TAG, "cw audio gain value must be between 0 and 100");
            return ThorError.TER_IE_GAIN;
        }

        nativeSetCwAudioGain(gain);

        return ThorError.THOR_OK;
    }

    @Override
    public synchronized void setUSSignal(boolean isUSSignal) {
        if (mIsClosed) {
            LogUtils.e(TAG, "This Dau instance is closed");
        }else{
            nativeSetUSSignal(isUSSignal);
        }
    }

    @Override
    public synchronized void setECGSignal(boolean isECGSignal) {
        if (mIsClosed) {
            LogUtils.e(TAG, "This Dau instance is closed");
        }else{
            nativeSetECGSignal(isECGSignal);
        }
    }

    @Override
    public synchronized void setDASignal(boolean isDASignal) {
        if (mIsClosed) {
            LogUtils.e(TAG, "This Dau instance is closed");
        }else{
            nativeSetDASignal(isDASignal);
        }
    }

    @Override
    public void setExternalEcg(boolean isExternalEcg) {
        if(mIsClosed){
            LogUtils.e(TAG, "This Dau instance is closed");
        }else{
            nativeSetExternalEcg(isExternalEcg);
        }
    }

    @Override
    public synchronized ThorError runTransducerElementCheck( int[] results )
    {
        int retCode = nativeRunTransducerElementCheck(results);
        ThorError retVal = ThorError.fromCode(retCode);
        return retVal;
    }

    @Override
    public void setLeadNumber(int leadNumber) {
        if(mIsClosed){
            LogUtils.e(TAG, "This Dau instance is closed");
        }else{
            nativeSetLeadNumber(leadNumber);
        }
    }

    @Override
    public synchronized ThorError getFov(int depthId, int imagingMode,float[] fov) {
        ThorError       retVal = ThorError.THOR_OK;


        nativeGetFov(depthId, imagingMode,fov);

        return retVal;
    }

    @Override
    public synchronized ThorError getDefaultRoi(int organId, int depthId, float[] roi) {
        ThorError       retVal = ThorError.THOR_OK;


        nativeGetDefaultRoi(organId, depthId, roi);

        return retVal;
    }

    @Override
    public synchronized int getFrameIntervalMs() {
        return nativeGetFrameIntervalMs();
    }

    @Override
    public synchronized int getMLinesPerFrame() {
        return nativeGetMLinesPerFrame();
    }
    
    @Override
    public boolean isExternalEcgConnected() {
        return nativeGetExternalEcg();
    }


    /** 
     * For use by the UltrasoundManager for creating new instances.
     *  
     * @hide
     */
    protected ThorError open(DauContext dauContext) {
        if (mIsClosed) {
            LogUtils.e(TAG, "This Dau instance is closed");
            return ThorError.TER_UMGR_DAU_CLOSED;
        }

        mDauContext = dauContext;

        int retCode = nativeOpen(this, mDauContext);

        ThorError   openError = ThorError.fromCode(retCode);

        if (!openError.isOK()) {
            close();
        }
        else {
            attachCine();

            try {
                mDb = SQLiteDatabase.openDatabase(getUpsPath(),
                                                  null,
                                                  SQLiteDatabase.OPEN_READONLY);
            } catch (SQLiteException ex) {
                LogUtils.e(TAG, "SQLiteException", ex);
                openError = ThorError.TER_IE_UPS_OPEN;
            }

        }

        return openError;
    }

    @Override
    public ProbeInformation getProbeInformation() {
        return nativeGetProbeInformation();
    }

    @Override
    public void setDebugFlag(boolean flag) {
        nativeSetDebugFlag(flag);
    }

    @Override
    public void setDauSafetyFeatureTestOption(int dauSafetyTestSelectedOption, float dauThresholdTemperatureForTest) {
        nativeSetSafetyFeatureTestOption(dauSafetyTestSelectedOption, dauThresholdTemperatureForTest);
    }

    private static native ProbeInformation nativeGetProbeInformation();

    private static native int  nativeOpen(DauImpl thisObject,
                                          DauContext context);
    private static native void nativeClose();
    private static native void nativeStart();
    private static native int  nativeStop();
    private static native int  nativeSetImagingCase(int imagingCaseId,
                                                    float[] apiEstimates,
                                                    float[] imagingInfo);
    private static native int  nativeSetColorImagingCase(int imagingCaseId,
                                                         int[] colorParams,
                                                         float[] roiParams,
                                                         float[] apiEstimates,
                                                         float[] imagingInfo);
    private static native int  nativeSetPwImagingCase(int imagingCaseId,
                                                      int[] pwIndices,
                                                      float[] gateParams,
                                                      float[] apiEstimates,
                                                      float[] imagingInfo,
                                                      boolean isTDI);
    private static native int  nativeSetCwImagingCase(int imagingCaseId,
                                                      int[] cwIndices,
                                                      float[] gateParams,
                                                      float[] apiEstimates,
                                                      float[] imagingInfo);
    private static native int  nativeSetMmodeImagingCase(int imagingCaseId,
                                                         int scrollSpeedId,
                                                         float mLinePosition,
                                                         float[] apiEstimates,
                                                         float[] imagingInfo);
    private static native int  nativeSetMLine(float mLinePosition);
    private static native void nativeEnableImageProcessing(int type,
                                                           boolean enable);
    private static native void nativeSetGain(int gain);
    private static native void nativeSetNearFarGain(int nearGain, int farGain);
    private static native void nativeSetColorGain(int gain);
    private static native void nativeSetEcgGain(float gain);
    private static native void nativeSetDaGain(float gain);
    private static native void nativeGetFov(int depthId, int imagingMode,float[] fov);
    private static native void nativeSetPwGain(int gain);
    private static native void nativeSetPwAudioGain(int gain);
    private static native void nativeSetCwGain(int gain);
    private static native void nativeSetCwAudioGain(int gain);
    private static native void nativeGetDefaultRoi(int organId, int depthId, float[] roi);    
    private static native int  nativeGetFrameIntervalMs();
    private static native int  nativeGetMLinesPerFrame();
    private static native void nativeSetUSSignal(boolean isUsEnable);
    private static native void nativeSetECGSignal(boolean isECGEnable);
    private static native void nativeSetDASignal(boolean isDAEnable);
    private static native boolean nativeGetExternalEcg();
    private static native void nativeSetExternalEcg(boolean isExternalEcg);
    private static native void nativeSetLeadNumber(int leadNumber);
    private static native int nativeRunTransducerElementCheck(int[] results);
    private static native boolean nativeSupportsEcg();
    private static native boolean nativeSupportsDa();
    private static native String nativeGetDbName();
    private static native void nativeSetDebugFlag(boolean flag);
    private static native boolean nativeIsElementCheckRunning();

    private static native void nativeSetSafetyFeatureTestOption(int dauSafetyTestSelectedOption, float dauThresholdTemperatureForTest);
}


