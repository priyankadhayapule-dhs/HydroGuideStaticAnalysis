/*
 * Copyright (C) 2016 EchoNous, Inc.
 */

package com.echonous.hardware.ultrasound;

import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteException;

import com.echonous.util.LogUtils;

public class DauEmulated extends Dau {
    private static final String TAG = "DauEmulated";

    private DauContext mDauContext;

    private boolean mIsRunning = false;
    private boolean mIsClosed = false;
    private SQLiteDatabase  mDb = null;

    protected DauEmulated(UltrasoundManager manager) {
        super(manager);
    }

    private DauEmulated() {
        super();
    }

    @Override
    public void close() {
        if (!mIsClosed) {
            nativeEmuClose();
            if (null != mDb) {
                mDb.close();
                mDb = null;
            }

            super.close();

            mIsClosed = true;
            LogUtils.d(TAG, "Emulated Dau closed");
        }
    }

    @Override
    public boolean isOpen() {
        return !mIsClosed;
    }

    @Override
    public boolean isRunning() {
        return mIsRunning;
    }

    @Override
    public String getId() {
        return "DauEmulated";
    }


    @Override
    public boolean supportsEcg() {
        return false;
    }

    @Override
    public boolean supportsDa() {
        return false;
    }

    @Override
    public boolean isElementCheckRunning() {
        return false;
    }

    @Override
    public String getUpsPath() {
        return null;
    }

    @Override
    public ThorError stop() {
        // BUGBUG: Consider throwing exception or returning error code
        if (mIsClosed) {
            LogUtils.e(TAG, "This Dau instance is closed");
            return ThorError.TER_UMGR_DAU_CLOSED;
        }

        if (mIsRunning) {
            LogUtils.d(TAG, "stopping...");
            nativeEmuStop();
            mIsRunning = false;
        }
        else {
            LogUtils.w(TAG, "Dau is already stopped!");
        }
        return ThorError.THOR_OK;
    }

    @Override
    public void start() {
        // BUGBUG: Consider throwing exception or returning error code
        if (mIsClosed) {
            LogUtils.e(TAG, "This Dau instance is closed");
            return;
        }

        if (!mIsRunning) {
            LogUtils.d(TAG, "starting...");
            nativeEmuStart();
            mIsRunning = true;
        }
        else {
            LogUtils.w(TAG, "Dau is already running!");
        }
    }

    @Override
    public ThorError setImagingCase(int imagingCaseId) {
        // NOP
        return ThorError.THOR_OK;
    }

    @Override
    public ThorError setImagingCase(int organId,
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

        if (mIsRunning) {
            LogUtils.e(TAG, "Cannot set Imaging Case while running");
            return ThorError.TER_IE_OPERATION_RUNNING;
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
                LogUtils.e(TAG, retVal.getDescription());
            }
        } catch (SQLiteException ex) {
            retVal = ThorError.TER_IE_UPS_ACCESS;
            LogUtils.e(TAG, retVal.getDescription());
        }

        return retVal;
    }

    @Override
    public ThorError setColorImagingCase(int organId,
                                         int viewId,
                                         int depthId,
                                         int[] colorParams,
                                         float[] roiParams,
                                         float[] apiEstimates,
                                         float[] imagingInfo) {
        // NOP
        return ThorError.THOR_OK;
    }
    
    @Override
    public ThorError setMmodeImagingCase(int organId,
                                         int viewId,
                                         int depthId,
                                         int scrollSpeedId,
                                         float mLinePosition,
                                         float[] apiEstimates,
                                         float[] imagingInfo) {
        // NOP
        return ThorError.THOR_OK;
    }

    @Override
    public synchronized ThorError setPwImagingCase(int imagingCaseId,
                                                   int[] pwIndices,
                                                   float[] gateParams,
                                                   float[] apiEstimates,
                                                   float[] imagingInfo,
                                                   boolean isTDI) {
        // NOP
        return ThorError.THOR_OK;
    }

    @Override
    public synchronized ThorError setCwImagingCase(int imagingCaseId,
                                                   int[] cwIndices,
                                                   float[] gateParams,
                                                   float[] apiEstimates,
                                                   float[] imagingInfo) {
        // NOP
        return ThorError.THOR_OK;
    }

    @Override
    public ThorError setMLine(float mLinePosition) {
        // NOP
        return ThorError.THOR_OK;
    }

    @Override
    public void enableImageProcessing(ImageProcessingType type, boolean enable) {
        // NOP
    }

    @Override
    public ProbeInformation getProbeInformation() {
        return null;
    }

    @Override
    public void setDebugFlag(boolean flag) {
        // NOP
    }

    @Override
    public ThorError setGain(int gain) {
        if (mIsClosed) {
            LogUtils.e(TAG, "This Dau instance is closed");
            return ThorError.TER_IE_DAU_CLOSED;
        }

        if (gain < 0 || gain > 100) {
            LogUtils.e(TAG, "gain value must be between 0 and 100");
            return ThorError.TER_IE_GAIN;
        }

        return ThorError.THOR_OK;
    }

    @Override
    public ThorError setGain(int nearGain, int farGain) {
        if (mIsClosed) {
            LogUtils.e(TAG, "This Dau instance is closed");
            return ThorError.TER_IE_DAU_CLOSED;
        }

        if (nearGain < 0 || nearGain > 100 || farGain < 0 || farGain > 100) {
            LogUtils.e(TAG, "gain value must be between 0 and 100");
            return ThorError.TER_IE_GAIN;
        }

        return ThorError.THOR_OK;
    }

    @Override
    public ThorError setColorGain(int gain) {
        if (mIsClosed) {
            LogUtils.e(TAG, "This Dau instance is closed");
            return ThorError.TER_IE_DAU_CLOSED;
        }

        if (gain < 0 || gain > 100) {
            LogUtils.e(TAG, "color gain value must be between 0 and 100");
            return ThorError.TER_IE_GAIN;
        }

        return ThorError.THOR_OK;
    }

    @Override
    public ThorError setEcgGain(float gain) {
        if (mIsClosed) {
            LogUtils.e(TAG, "This Dau instance is closed");
            return ThorError.TER_IE_DAU_CLOSED;
        }

        if (gain < -100.0 || gain > 100.0) {
            LogUtils.e(TAG, "ecg gain value must be between -100 and 100 dB");
            return ThorError.TER_IE_GAIN;
        }

        return ThorError.THOR_OK;
    }

    @Override
    public ThorError setDaGain(float gain) {
        if (mIsClosed) {
            LogUtils.e(TAG, "This Dau instance is closed");
            return ThorError.TER_IE_DAU_CLOSED;
        }

        if (gain < -100.0 || gain > 100.0) {
            LogUtils.e(TAG, "da gain value must be between -100 and 100 dB");
            return ThorError.TER_IE_GAIN;
        }

        return ThorError.THOR_OK;
    }

    @Override
    public synchronized ThorError setPwGain(int gain) {
        if (mIsClosed) {
            LogUtils.e(TAG, "This Dau instance is closed");
            return ThorError.TER_IE_DAU_CLOSED;
        }

        if (gain < 0 || gain > 100) {
            LogUtils.e(TAG, "pw gain value must be between 0 and 100");
            return ThorError.TER_IE_GAIN;
        }

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

        return ThorError.THOR_OK;
    }

    @Override
    public synchronized ThorError setCwGain(int gain) {
        if (mIsClosed) {
            LogUtils.e(TAG, "This Dau instance is closed");
            return ThorError.TER_IE_DAU_CLOSED;
        }

        if (gain < 0 || gain > 100) {
            LogUtils.e(TAG, "cw gain value must be between 0 and 100");
            return ThorError.TER_IE_GAIN;
        }

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

        return ThorError.THOR_OK;
    }

    @Override
    public void setUSSignal(boolean isUSSignal) {

    }

    @Override
    public void setECGSignal(boolean isECGSignal) {

    }

    @Override
    public void setDASignal(boolean isDASignal) {

    }

    @Override
    public void setExternalEcg(boolean isExternalEcg) {

    }

    @Override
    public void setLeadNumber(int leadNumber) {

    }

    @Override
    public ThorError getFov(int depthId, int imagingMode,float[] fov) {
        ThorError       retVal = ThorError.THOR_OK;

        nativeGetFov(depthId,imagingMode,fov);

        return retVal;
    }

    @Override
    public ThorError getDefaultRoi(int organId, int depthId, float[] roi) {
        ThorError       retVal = ThorError.THOR_OK;

        nativeGetDefaultRoi(organId, depthId, roi);

        return retVal;
    }

    @Override 
    public int getFrameIntervalMs() {
        return nativeGetFrameIntervalMs();
    }

    @Override
    public int getMLinesPerFrame() {
        return nativeGetMLinesPerFrame();
    }
    
    @Override
    public boolean isExternalEcgConnected() {
        return false;
    }

    @Override
    public ThorError runTransducerElementCheck( int[] results )
    {
        return ThorError.THOR_OK;
    }

    public ThorError open(DauContext dauContext) {
        if (mIsClosed) {
            LogUtils.e(TAG, "This Dau instance is closed");
            return ThorError.TER_UMGR_DAU_CLOSED;
        }

        mDauContext = dauContext;

        int retCode = nativeEmuOpen(dauContext);

        ThorError   openError = ThorError.fromCode(retCode);

        if (!openError.isOK()) {
            mIsClosed = true;
        }
        else {
            getManager().attachCineProducer(this);

            try {
                mDb = SQLiteDatabase.openDatabase(getManager().getUpsPath(),
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
    public void setDauSafetyFeatureTestOption(int dauSafetyTestSelectedOption, float dauThresholdTemperatureForTest) {}

    public static native int  nativeEmuOpen(DauContext context);
    public static native void nativeEmuClose();
    public static native void nativeEmuStart();
    public static native void nativeEmuStop();
    public static native void nativeGetFov(int depthId, int imagingMode,float[] fov);
    public static native void nativeGetDefaultRoi(int organId, int depthId, float[] roi);
    public static native int  nativeGetFrameIntervalMs();
    public static native int  nativeGetMLinesPerFrame();

    static {
        System.loadLibrary("EmuUltrasound");
    }
}

