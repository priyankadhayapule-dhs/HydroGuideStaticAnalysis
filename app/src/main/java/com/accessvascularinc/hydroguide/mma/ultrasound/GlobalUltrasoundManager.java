// Copyright (C) 2018 EchoNous, Inc.

package com.accessvascularinc.hydroguide.mma.ultrasound;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.sqlite.SQLiteDatabaseCorruptException;
import android.graphics.Bitmap;
import android.hardware.usb.UsbDevice;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;

import androidx.annotation.Nullable;

import com.accessvascularinc.hydroguide.mma.utils.MedDevLog;
import com.echonous.hardware.ultrasound.Dau;
import com.echonous.hardware.ultrasound.DauException;
import com.echonous.hardware.ultrasound.ProbeInformation;
import com.echonous.hardware.ultrasound.Puck;
import com.echonous.hardware.ultrasound.ThorError;
import com.echonous.hardware.ultrasound.UltrasoundEncoder;
import com.echonous.hardware.ultrasound.UltrasoundManager;
import com.echonous.hardware.ultrasound.UltrasoundPlayer;
import com.echonous.hardware.ultrasound.UltrasoundRecorder;
import com.echonous.hardware.ultrasound.UltrasoundViewer;
import com.echonous.kLink.KLinkUsbDevice;
/*import com.echonous.kosmosapp.BuildConfig;
import com.echonous.kosmosapp.app.ui.vmfactory.ViewModelFactory;
import com.echonous.kosmosapp.data.ups.UpsManager;
import com.echonous.kosmosapp.domain.model.imaging.EcgExternalModel;
import com.echonous.kosmosapp.utility.license.OrganListManager;
import com.echonous.kosmosapp.utility.utils.LogUtils;*/

import org.jetbrains.annotations.NotNull;

import java.io.File;
import java.util.Locale;
import java.util.Objects;

public class GlobalUltrasoundManager {

    private static String TAG = "Thor : GlobalUltrasoundManager";

    private UltrasoundManager mUltrasoundManager = null;
    private static GlobalUltrasoundManager singleton = null;
    private static Dau mDau = null;
    private ErrorCallback mCallback = null;
    private UltrasoundRecorder mRecorder = null;
    private UltrasoundViewer mViewer = null;
    private Puck mPuck = null;
    private boolean mIsDauStarted=false;
    private boolean mIsFreeze=false;
    private Locale locale = new Locale("en","United States");
    private String IMAGES_URL = null; // Will be set during init
    public static final String VERSION_NAME = "7.0.0";
    public static final String BUILD_NUMBER = "14";

    private float[] mApiEstimates = new float[8];
    private float[] mImagingInfo = new float[8];

    private GlobalUltrasoundManager() {}

    public static @NotNull
    GlobalUltrasoundManager getInstance() {
        if (singleton == null) {
            singleton = new GlobalUltrasoundManager();
        }
        return singleton;
    }

    public static void releaseInstance() {
        if (singleton != null) {
            singleton = null;
        }
    }

    /**
     * usb probe firmware update
     */
    public void usbProbeFwUpdate(){
        if(mUltrasoundManager != null){
            mUltrasoundManager.usbProbeFwUpdate();
        }
    }

    /* @context : This should always be the application context.*/
    private void init(Context context) {
        if (context == null) {
            return;
        }
        if (mUltrasoundManager == null) {
            // Set IMAGES_URL to clips directory in external files
            File clipsDir = new File(context.getExternalFilesDir(null), "clips");
            if (!clipsDir.exists()) {
                clipsDir.mkdirs();
            }
            IMAGES_URL = clipsDir.getAbsolutePath() + "/";
            Log.d(TAG, "IMAGES_URL set to: " + IMAGES_URL);
            
            mUltrasoundManager = new UltrasoundManager(context, UltrasoundManager.DauCommMode.Usb);
            mRecorder = mUltrasoundManager.getUltrasoundRecorder();
            mRecorder.registerCompletionCallback(recorderCallBack,new Handler(Looper.getMainLooper()) {
                        @Override
                        public void handleMessage(Message msg) {
                            super.handleMessage(msg);
                        }
                    }
            );
            // Locale.getDefault is deprecated, use the following to get the list of locales in order
            // of user preference (https://stackoverflow.com/questions/14389349/android-get-current-locale-not-default)

            Log.i(TAG,"Native Locale is:- "+ locale.getDisplayName() + " Lang Code:- " + locale.getLanguage());
            mUltrasoundManager.setLocale(locale);

            registerReceiverForLocaleChange(context);

            mViewer = mUltrasoundManager.getUltrasoundViewer();
            mViewer.registerErrorCallback(mViewerErrorCallback, new Handler(Looper.getMainLooper()) {
                    }
            );
        }
    }

    private void registerReceiverForLocaleChange(Context context) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.UPSIDE_DOWN_CAKE) {
            context.registerReceiver(new BroadcastReceiver() {
                @Override
                public void onReceive(Context context, Intent intent) {
                    if (mUltrasoundManager != null) {
                        Log.i(TAG, "Native Locale is:- " + locale.getDisplayName() + " Lang Code:- " + locale.getLanguage());
                        mUltrasoundManager.setLocale(locale);
                    }
                }
            }, new IntentFilter(Intent.ACTION_LOCALE_CHANGED), Context.RECEIVER_EXPORTED);
        } else {
            context.registerReceiver(new BroadcastReceiver() {
                @Override
                public void onReceive(Context context, Intent intent) {
                    if (mUltrasoundManager != null) {
                        Log.i(TAG, "Native Locale is:- " + locale.getDisplayName() + " Lang Code:- " + locale.getLanguage());
                        mUltrasoundManager.setLocale(locale);
                    }
                }
            }, new IntentFilter(Intent.ACTION_LOCALE_CHANGED));
        }
    }

    public synchronized boolean setProbeInformation(Integer probeType){
        Log.d(TAG,"setProbeInformation probeType : " +probeType);
        if (null != mUltrasoundManager) {
            return  mUltrasoundManager.setProbeInformation(probeType);
        }
        return false;
    }

    /**
     * run transducer element check
     * @return result[0] number of transducer element failed
     */
    public synchronized int runTransducerElementCheck(UltrasoundManager.DauCommMode mode, UsbDevice device) {
        int[] result = new int[1];

        if (null != mUltrasoundManager) {
            if (null == mDau) {
                try {
                    if (!mUltrasoundManager.isUsbDeviceConnected()) {
                        mUltrasoundManager.isDauConnected(mode, device);
                    }
                    mDau = mUltrasoundManager.openDau(mode);
                    mDau.runTransducerElementCheck(result);
                    mDau.close();
                    mDau = null;
                } catch (DauException dauException) {
                    Handler mHandler =new Handler(Looper.getMainLooper());
                    mHandler.post(() -> {
                        if( mode == UltrasoundManager.DauCommMode.Usb){
                            mCallback.onDauOpenError(dauException.getError());
                        }else{
                            mCallback.onThorError(dauException.getError());
                        }
                    });
                }
            }
            else {
                mDau.runTransducerElementCheck(result);
            }
        }
        return result[0];
    }

    /* MainActivity should call this method during its onCreate() */
    public void init(Context context, ErrorCallback callback) {
        mCallback = callback;
        init(context);
    }

    public void openPuck() {
        try {
            mPuck = mUltrasoundManager.openPuck();
        } catch (DauException e) {
            mCallback.onDauException(e);
        }
    }

    public void closePuck() {
        if (mPuck != null) {
            mPuck.unregisterEventCallback();
            mPuck.close();
            mPuck = null;
        }
    }

    public Puck getPuck() {
        return mPuck;
    }

    public String getUpsPath() {
        if (null != mDau) {
            return mDau.getUpsPath();
        }
        else {
            return mUltrasoundManager.getUpsPath();
        }
    }

    public String getUpsPathProbeWise(Integer probeType) {
        return mUltrasoundManager.getUpsPathProbeWise(probeType);
    }

    public void setUpsDatabaseProbeWise(String databaseName) {
        mUltrasoundManager.setUpsPathProbeWise(databaseName);
    }

    public float getPwPrfFrequency() { return mImagingInfo[5]/1000.0f; }    // kHz

    public float getPwSweepSpeed() { return mImagingInfo[6]; }

    public float getBCenterFrequency() { return mImagingInfo[0]; }

    public float getCCenterFrequency() { return mImagingInfo[1]; }

    public float getFrameRate() { return mImagingInfo[2]; }

    public float getDutyCycle() { return mImagingInfo[3]; }

    public float[] getApiEstimates() { return mApiEstimates; }

    @Deprecated
    public void openDau() {
        if (mDau == null) {
            try {
                mDau = mUltrasoundManager.openDau();
                mDau.registerErrorCallback(mDauErrorCallback, new Handler(Looper.getMainLooper()) {
                            @Override
                            public void handleMessage(Message msg) {
                                super.handleMessage(msg);
                            }
                        }
                );

                /*
                 * As of current implementation External ECG connect/disconnect will only work after Dau Power open
                 */
                mDau.registerExternalEcgCallback(mExternalEcgCallBack,new Handler(Looper.getMainLooper()) {
                    @Override
                    public void handleMessage(Message msg) {
                        super.handleMessage(msg);
                    }
                });

            } catch (DauException e) {
                mCallback.onDauException(e);
            }
        }
    }

    public void openDau(UltrasoundManager.DauCommMode mode) {
        if (mDau == null) {
            try {
                mDau = mUltrasoundManager.openDau(mode);
                mDau.registerErrorCallback(mDauErrorCallback, new Handler(Looper.getMainLooper()) {
                            @Override
                            public void handleMessage(Message msg) {
                                super.handleMessage(msg);
                            }
                        }
                );

                /*
                 * As of current implementation External ECG connect/disconnect will only work after Dau Power open
                 */
                mDau.registerExternalEcgCallback(mExternalEcgCallBack,new Handler(Looper.getMainLooper()) {
                    @Override
                    public void handleMessage(Message msg) {
                        super.handleMessage(msg);
                    }
                });

            } catch (DauException e) {
                mCallback.onDauException(e);
            }
        }
    }

    public void initUps(Context ctx){
        if(ctx != null) {
            try {
                UpsManager.init(ctx);
                UpsManager.getInstance().createUniqueOrgansList(ctx);
                UpsManager.getInstance().createUniqueAiOrgansList(ctx);
                //OrganListManager.INSTANCE.prepareOrganListWithActiveLicense();
            } catch (SQLiteDatabaseCorruptException e) {
                mCallback.onUPSError();
            }
        }
    }

    public boolean isDauConnected(UltrasoundManager.DauCommMode mode, UsbDevice usbDevice) {
        if (null != mUltrasoundManager) {
            return mUltrasoundManager.isDauConnected(mode, usbDevice);
        } else {
            return false;
        }
    }

    /**
     * check for PCIe probe firmware update
     * @return true if firmware update available else false
     */
    public boolean checkFwUpdateAvailablePcie(){
        Log.d(TAG,"checkFwUpdateAvailablePcie");
        if(mUltrasoundManager != null) {
            return mUltrasoundManager.checkFwUpdateAvailableNativePCIe();
        }
        return false;
    }

    /**
     * check for USB probe firmware update
     * @return true if firmware update available else false
     */
    public boolean checkFwUpdateAvailableUsb(){
        Log.d(TAG,"checkFwUpdateAvailableUsb");
        if(mUltrasoundManager != null){
            return mUltrasoundManager.checkFirmwareUpdateUsb();
        }
        return false;
    }

    public synchronized ProbeInformation getProbeInformation() {
        ProbeInformation    probeInfo = null;

        if (null != mUltrasoundManager) {
            try{
                return mUltrasoundManager.getProbeDataFromDau(UltrasoundManager.DauCommMode.Usb);
            }catch (DauException dauException){
                Handler mHandler =new Handler(Looper.getMainLooper());
                mHandler.post(() -> mCallback.onDauOpenError(dauException.getError()));

            }
        }
        return probeInfo;
    }

    /**
     * This method read the probe detail from connected Proprietary and USB mode
     *
     * @param mode Specifies the operational mode: Proprietary, Usb, or Emulation.
     * @return ProbeInformation contains probe property
     */
    public synchronized ProbeInformation getProbeDataFromDau(UltrasoundManager.DauCommMode mode) {
        if(mUltrasoundManager != null) {
            try{
                return mUltrasoundManager.getProbeDataFromDau(mode);
            }catch (DauException dauException){
                Handler mHandler =new Handler(Looper.getMainLooper());
                mHandler.post(new Runnable(){
                    @Override
                    public void run() {
                        mCallback.onDauOpenError(dauException.getError());
                    }
                });

            }
        }
        return null;
    }

    public synchronized void setActiveProbe(int probeType){
        if(mUltrasoundManager != null){
            mUltrasoundManager.setActiveProbe(probeType);
        }
    }

    /**
     * This method read the probe detail from connected Proprietary and USB mode
     *
     * @param probeId Specifies the operational mode: Proprietary, Usb, or Emulation.
     * @return ProbeInformation contains probe property
     */
    public synchronized ProbeInformation getProbeDataFromDau(int probeId) {
        if(mUltrasoundManager != null) {
            try{
                return mUltrasoundManager.getProbeDataFromDau(probeId);
            }catch (DauException dauException){
                Handler mHandler =new Handler(Looper.getMainLooper());
                mHandler.post(new Runnable(){
                    @Override
                    public void run() {
                        mCallback.onDauOpenError(dauException.getError());
                    }
                });

            }
        }
        return null;
    }

    /**
     * Get probe detail from PID
     *
     * @param mode Specifies the operational mode: Proprietary, Usb, or Emulation.
     * @param  probePid probe id
     * @return ProbeInformation
     *
     */
    public ProbeInformation getProbeDataFromPID(UltrasoundManager.DauCommMode mode, int probePid) {
        if(mUltrasoundManager != null) {
            return mUltrasoundManager.getProbeDataFromPID(mode,probePid);
        }
        return null;
    }

    public boolean isDauStart(){
        return mIsDauStarted;
    }


    public boolean isDauFreeze(){
        return mIsFreeze;
    }

    public void startDau() {
       if (mDau != null) {
           // Attach DAU to CineBuffer BEFORE starting
           // This ensures frames are captured into CineBuffer for recordStillImage()
           if (!mDau.isCineAttached()) {
               Log.d(TAG, "Attaching DAU to CineBuffer before starting");
               mDau.attachCine();
           }
           
           mDau.start();

           mIsDauStarted = true;
           mIsFreeze = false;
       }
    }

    public void attachCine() {
        mDau.attachCine();
    }

    public void attachCineIfRequired(){
        if(!mDau.isCineAttached()){
            mDau.attachCine();
        }
    }

    public ThorError stopDau() {
        ThorError mThorError=ThorError.THOR_OK;
        if (mDau != null) {
            if(mIsDauStarted){
                mThorError=mDau.stop();
                mIsDauStarted=false;
                mIsFreeze = false;
                if(!mThorError.isOK()){
                    //Interrupt User
                    mCallback.onThorError(mThorError);
                }
            }
        }
        return mThorError;
    }

    public void exitDau() {
        if (null != mDau) {
            mDau.unregisterErrorCallback();
            mDau.unregisterExternalEcgCallback();
            mDau.close();
            mDau = null;
        }
    }

    public void closeDau() {

        if (null != mDau) {
            mDau.close();
            mDau = null;
        }
        mIsFreeze = false;
    }

    public ThorError freeze() {
        /*
         * Default ThorError will be Thor Ok
         */
        ThorError mThorError=ThorError.THOR_OK;
        if (null == mDau) {
            return ThorError.TER_UMGR_DAU_CLOSED;
        }
        mIsFreeze = true;
        if(mIsDauStarted){
            mThorError=mDau.stop();
            mDau.detachCine();
            mIsDauStarted=false;
            if(!mThorError.isOK()){
             //Interrupt User
                mCallback.onThorError(mThorError);
            }
        }

        return mThorError;
    }

    public void unfreeze() {
        if (null == mDau) {
            return;
        }
        mIsFreeze = false;
        mDau.attachCine();

        mDau.start();

        mIsDauStarted=true;
    }

    /**
     * unfreeze function specific for doppler because where we want to not start
     * dau before calling setImagingCase for B mode
     */
    public void unfreezeDoppler() {
        if (null == mDau) {
            return;
        }
        mIsFreeze = false;
        mIsDauStarted=true;
    }

    public @Nullable
    UltrasoundViewer getUltrasoundViewer(Context context) {
        if (mUltrasoundManager == null) {
            init(context);
        }
        return mUltrasoundManager.getUltrasoundViewer();
    }


    public boolean isExternalEcgConnected(){
        if(mDau!=null){
            //if(FeatureManager.isEnabled(FeatureManager.Feature.DA_ECG)){
                return mDau.isExternalEcgConnected();
            //}else{
            //    return false;
            //}
        }
        return false;
    }

    /**
     * Get TraceLine Position in Second for Scrolling modes
     */
    public float getTraceLinePosition(){
        if (mUltrasoundManager != null) {
            return mUltrasoundManager.getUltrasoundViewer().getTraceLineTime();
        }
        return 0F;
    }


    /**
     * Get B-frame Interval (interval in ms)
     * @return mDau.getFrameIntervalMs()
     */
    public int getBFrameInterval(){
        if(mDau != null){
            return  mDau.getFrameIntervalMs();
        }
        else if (mUltrasoundManager != null) {
            return mUltrasoundManager.getUltrasoundViewer().getFrameIntervalMs();
        }
        return 0;
    }

    /**
     * Get number of M-lines per frame have been created
     * @return  mDau.getMLinesPerFrame()
     */
    public int getNumberOfMLinesPerFrame(){
        if(mDau != null){
            return  mDau.getMLinesPerFrame();
        }
        else if (mUltrasoundManager != null) {
            return mUltrasoundManager.getUltrasoundViewer().getMLinesPerFrame();
        }
        return 0;
    }

    public void setPwImagingCase(int imageCaseId,int[] pwIndices,float[] gateParams) {
        if (mDau != null) {
            mDau.setPwImagingCase(imageCaseId,pwIndices,gateParams, mApiEstimates, mImagingInfo, false);
        }
    }

    public void setTdiImagingCase(int imageCaseId,int[] pwIndices,float[] gateParams) {
        if (mDau != null) {
            // TODO: later we need to update setPwImagingCase to support TDI
            mDau.setPwImagingCase(imageCaseId,pwIndices,gateParams, mApiEstimates, mImagingInfo,true);
        }
    }

    public void setCwImagingCase(int imageCaseId,int[] cwIndices,float[] gateParams) {
        if (mDau != null) {
            mDau.setCwImagingCase(imageCaseId,cwIndices,gateParams, mApiEstimates, mImagingInfo);
        }
    }

    public void setImagingCase(int organId, int viewId, int depthId, int mode) {
        if (mDau != null) {
            var error = mDau.setImagingCase(organId, viewId, depthId, mode, mApiEstimates, mImagingInfo);
            if (error != ThorError.THOR_OK) {
                MedDevLog.error("Ultrasound", "setImagingCase error :" + error);
            }
        }
    }

    public void setMmodeImagingCase(int organId, int viewId, int depthId,int scrollSpeedIndex,float mLinePosition) {
        if (mDau != null) {
            mDau.setMmodeImagingCase(organId, viewId, depthId,scrollSpeedIndex, mLinePosition, mApiEstimates, mImagingInfo);
        }
    }
    public void setMLine(float mLinePosition) {
        if (mDau != null) {
            mDau.setMLine(mLinePosition);
        }
    }

    public void setGain(Integer nearGain, Integer farGain) {
        if (null != mDau) {
            mDau.setGain(nearGain, farGain);
        }
    }

    public void setColorGain(Integer gain) {
        if (null != mDau) {
            mDau.setColorGain(gain);
        }
    }

    public void setEcgGain(Float gain) {
        if (null != mDau) {
            mDau.setEcgGain(gain);
        }
    }

    public void setDaGain(Float gain) {
        if (null != mDau) {
            mDau.setDaGain(gain);
        }
    }

    public void setPwGain(Integer gain) {
        if (null != mDau) {
            mDau.setPwGain(gain);
        }
    }

    public void setPwAudioGain(Integer gain) {
        if (null != mDau) {
            mDau.setPwAudioGain(gain);
        }
    }

    public void setCwGain(Integer gain) {
        if (null != mDau) {
            mDau.setCwGain(gain);
        }
    }

    public void setCwAudioGain(Integer gain) {
        if (null != mDau) {
            mDau.setCwAudioGain(gain);
        }
    }

    public void setUSSignal(Boolean isUSSignalEnable) {
        if (null != mDau) {
            mDau.setUSSignal(isUSSignalEnable);
        }
    }

    public void setECGSignal(Boolean isECGSignalEnable) {
        if (null != mDau) {
            mDau.setECGSignal(isECGSignalEnable);
        }
    }

    public void setDASignal(Boolean isDASignalEnable) {
        if (null != mDau) {
            mDau.setDASignal(isDASignalEnable);
        }
    }

    public void setLeadNumber(int leadNumber){
        if (null != mDau) {
            mDau.setLeadNumber(leadNumber);
        }
    }

    public void setExternalEcg(boolean isExternalEcg){
        if (null != mDau) {
            mDau.setExternalEcg(isExternalEcg);
        }
    }

    /**
     * Torso-one element check
     * @return true when ECG and DA checks are true
     */
    public boolean isTorsoOneConnected() {
        if (null != mUltrasoundManager) {
            if (null != mDau) {
                boolean isEcgSupported = mDau.supportsEcg();
                boolean isDaSupported = mDau.supportsDa();

                return !(isEcgSupported && isDaSupported);
            }
        }
        return false;
    }

    /**
     * @return boolean true/false whether element check is running or not and
     * return true if dau is null or closed
     **/
    public boolean isElementCheckRunning() {
        if (null != mDau) {
            return mDau.isElementCheckRunning();
        }
        return true;
    }

    public ThorError setColorImagingCase(int organId, int viewId, int depthId,int[] colorParams,float[] roiParams){
        if(mDau!=null){
           return mDau.setColorImagingCase(organId,viewId,depthId,colorParams,roiParams,mApiEstimates,mImagingInfo);
        }
        return ThorError.THOR_ERROR;
    }

    public float[] setZoomPanParams(Context ctx, float[] mapMat) {
        Objects.requireNonNull(getUltrasoundViewer(ctx)).setZoomPanFlipParams(mapMat);
        return mapMat;
    }

    public float[] getTransformationMatrix(Context ctx){
        float[] mapMat = new float[]{0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F};
        Objects.requireNonNull(getUltrasoundViewer(ctx)).queryPhysicalToPixelMap(mapMat);
        return mapMat;
    }

    public float[] getZoomPanParams(Context ctx){
        float[] floatArray =  new float[]{0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
        Objects.requireNonNull(GlobalUltrasoundManager.getInstance().getUltrasoundViewer(ctx)).queryZoomPanFlipParams(floatArray);
        return floatArray;
    }

    public float[] getTransformationMatrixForMmode(Context ctx){
        float[] mapMat = new float[]{0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F,0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F};
        Objects.requireNonNull(getUltrasoundViewer(ctx)).queryPhysicalToPixelMap(mapMat);
        return mapMat;
    }

    public float[] getTransformationMatrixForMmodeOnly(Context ctx){
        float[] mapMat = new float[]{0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F,0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F};
        Objects.requireNonNull(getUltrasoundViewer(ctx)).queryPhysicalToPixelMap(mapMat);
        float[] finalMat = new float[]{0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F};
        System.arraycopy(mapMat, 9, finalMat, 0, finalMat.length);
        return finalMat;
    }

    /**
     * get Transformation Matrix For B/C + Doppler Mode
     * @param ctx
     * @return mapMat
     */
    public float[] getTransformationMatrixForDopplerMode(Context ctx){
        float[] mapMat = new float[]{0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F,0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F};
        Objects.requireNonNull(getUltrasoundViewer(ctx)).queryPhysicalToPixelMap(mapMat);
        return mapMat;
    }

    /**
     * get Transformation Matrix For only doppler part
     * @param ctx
     * @return mapMat
     */
    public float[] getTransformationMatrixForDopplerModeOnly(Context ctx){
        float[] mapMat = new float[]{0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F,0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F};
        Objects.requireNonNull(getUltrasoundViewer(ctx)).queryPhysicalToPixelMap(mapMat);
        float[] finalMat = new float[]{0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F};
        System.arraycopy(mapMat, 9, finalMat, 0, finalMat.length);
        return finalMat;
    }

    public float[] getTransformationDaMatrix(Context ctx){
        float[] mapMat = new float[]{0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F,
                0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F,
                0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F};
        Objects.requireNonNull(getUltrasoundViewer(ctx)).queryPhysicalToPixelMap(mapMat);
        float[] finalMat = new float[]{0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F};
        System.arraycopy(mapMat, 9, finalMat, 0, finalMat.length);
        return finalMat;
    }

    public float[] getTransformationEcgMatrix(Context ctx){
        float[] mapMat = new float[]{0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F,
                0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F,
                0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F};
        Objects.requireNonNull(getUltrasoundViewer(ctx)).queryPhysicalToPixelMap(mapMat);
        float[] finalMat = new float[]{0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F};
        System.arraycopy(mapMat, 18, finalMat, 0, finalMat.length);
        return finalMat;
    }

    /*
    * @return       The matrix of FOV parameters.
    *               fov[0] = axial start, fov[1] = axial span,
    *               fov[2] = azimuthStart, fov[3] = azimuthSpan */
    public float[] getFov(int depthId,int imagingMode){
        float[] mapMat = new float[]{0F, 0F, 0F, 0F};
        if (mDau != null) {
            mDau.getFov(depthId, imagingMode, mapMat);
        }
        return mapMat;
    }

    /*
     * @return       The matrix of FOV parameters.
     *               fov[0] = axial start, fov[1] = axial span,
     *               fov[2] = azimuthStart, fov[3] = azimuthSpan */
    public float[] getDefaultRoi(int organIndex, int depthIndex){
        float[] mapMat = new float[]{0F, 0F, 0F, 0F};
        if (mDau != null) {
            mDau.getDefaultRoi(organIndex, depthIndex, mapMat);
        }
        return mapMat;
    }


    public UltrasoundPlayer getUltraSoundPlayer() {
        if (mUltrasoundManager != null) {
            return mUltrasoundManager.getUltrasoundPlayer();
        }
        return null;
    }


    public UltrasoundRecorder getUltraSoundRecorder() {
        if (mUltrasoundManager != null) {
            return mUltrasoundManager.getUltrasoundRecorder();
        }
        return null;
    }


    public UltrasoundEncoder getUltrasoundEncoder(Context context) {
        if (null == mUltrasoundManager && null != context) {
            init(context);
        }
        if (mUltrasoundManager != null) {
            return mUltrasoundManager.getUltrasoundEncoder();
        }
        return null;
    }

    public Boolean isChargerApproved() {
        if (mUltrasoundManager != null) {
            return mUltrasoundManager.isChargerApproved();
        }
        return false;
    }

    /**
     * Gets the KLink device.
     */
    public KLinkUsbDevice getKLink() {
        if (mUltrasoundManager != null) {
            return mUltrasoundManager.getKLink();
        }
        return null;
    }

    public void getUltrasoundScreenImage(Bitmap bitmapImage, boolean isForThumbnail, int desiredFrameNumber) {
        mViewer.getUltrasoundScreenImage(bitmapImage,isForThumbnail,desiredFrameNumber);
    }

    public boolean checkNativeCineViewerIsInitialized() {
        return (null != mViewer && mViewer.checkNativeCineViewerIsInitialized());
    }

    public void openFile(String filename){
        if (null != mDau && mDau.isCineAttached()) {
            mDau.detachCine();
        }

        if (!getUltraSoundPlayer().isCineAttached()) {
            getUltraSoundPlayer().attachCine();
        }
        getUltraSoundPlayer().closeRawFile();

        getUltraSoundPlayer().openRawFile(IMAGES_URL +
                filename);
    }

    public ThorError saveImage(String fileName){
        return mRecorder.recordStillImage(IMAGES_URL +
                fileName + ".thor");
    }

    public ThorError saveClip(String fileName, int clipDuration) {
        return mRecorder.recordRetrospectiveVideo(IMAGES_URL +
                fileName + ".thor", clipDuration);
    }

    public ThorError saveClip(String fileName,int startFrame,
                              int stopFrame) {
        return mRecorder.saveVideoFromCineFrames(IMAGES_URL +
                fileName + ".thor", startFrame,stopFrame);
    }

    public ThorError saveClipRange(String fileName, int startFrame, int stopFrame) {
        return mRecorder.saveVideoFromLiveFrames(IMAGES_URL +
                fileName + ".thor", startFrame,stopFrame);
    }

    /**
     * System may use existing file name to update existing .thor file
     * File name should be with .thor ext
     * @param fileName Pass the file name to be saved
     * @return Return File save status
     */
    public ThorError updateThorFile(String fileName){

        String mThorFile = IMAGES_URL + fileName;
        File mFile = new File(mThorFile);
        if(mFile.exists()){
            Log.i(TAG,"Thor file is exist to be updated "+ fileName);
        }else{
            Log.i(TAG,"Thor file not found");
        }

        return mRecorder.recordUpdateParamsFile(mThorFile);
    }

    public void clean(){

        if(getUltraSoundPlayer() != null) {
            getUltraSoundPlayer().closeRawFile();
        }

        mCallback = null;
        singleton = null;

        if (null != mRecorder) {
            mRecorder.unregisterCompletionCallback();
            mRecorder = null;
        }

        if (null != mViewer) {
            mViewer.unregisterHeartRateCallback();
            mViewer.unregisterErrorCallback();
            mViewer = null;
        }

        if (null != mPuck) {
            closePuck();
        }

        if (null != mDau) {
            exitDau();
        }

        if (mUltrasoundManager != null) {
            mUltrasoundManager.cleanup();
            mUltrasoundManager = null;
        }
    }

    private final Dau.ErrorCallback mDauErrorCallback =
            new Dau.ErrorCallback() {

                @Override
                public void onError(ThorError thorError) {
                    mCallback.onThorError(thorError);
                }
            };

    private final UltrasoundViewer.ErrorCallback mViewerErrorCallback =
        new UltrasoundViewer.ErrorCallback() {

            @Override
            public void onError(ThorError thorError) {
                mCallback.onViewerError(thorError);
            }
        };

    public interface ErrorCallback{
        void onThorError(ThorError error);
        void onDauException(DauException dauException);
        void onUPSError();
        void onViewerError(ThorError error);
        void onDauOpenError(ThorError error);
    }

    public interface CaptureCompletionListener {
        void onCaptureComplete(ThorError result);
    }

    private CaptureCompletionListener mCaptureCompletionListener = null;

    public void setCaptureCompletionListener(CaptureCompletionListener listener) {
        mCaptureCompletionListener = listener;
    }

    private final UltrasoundRecorder.CompletionCallback recorderCallBack = new UltrasoundRecorder.CompletionCallback() {
        @Override
        public void onCompletion(ThorError thorError) {
            //if (BuildConfig.DEBUG) {
                Log.d(TAG, "onCompletion Response Name: "+thorError.getName()+
                        " Code: "+thorError.getCode() +
                        " Description: "+ thorError.getDescription() );
            //}
            
            // Notify listener if registered
            if (mCaptureCompletionListener != null) {
                mCaptureCompletionListener.onCaptureComplete(thorError);
                mCaptureCompletionListener = null; // One-time listener
            }
        }
    };


    private final Dau.ExternalEcgCallback mExternalEcgCallBack=new Dau.ExternalEcgCallback() {
        @Override
        public void onExternalEcg(boolean isConnected) {
            super.onExternalEcg(isConnected);
            //if(FeatureManager.isEnabled(FeatureManager.Feature.DA_ECG)){
                /*try{
                    *//*
                     * It required to be generated when user connect and disconnect ECG during Live imaging scree, Cine Review
                     *//*
                    EcgExternalModel ecgExternalModel=ViewModelFactory.getInstance().getEcgExternalViewModel().getEcgExternalViewModel().getValue();
                    if (ecgExternalModel != null) {
                        ecgExternalModel.setExternalConnected(isConnected);
                        ecgExternalModel.setUserChange(true);
                        LogUtils.d(TAG, "EXT:: Hardware callback ECG " + isConnected);
                        ViewModelFactory.getInstance().getEcgExternalViewModel().setEcgExternalViewModel(ecgExternalModel);
                    }
                }catch (NullPointerException nop){
                    LogUtils.e(TAG,"Fail to set ECG View Model"+ nop);
                }*/
            //}
            Log.i(TAG,"External ECG is"+ isConnected);
        }
    };

    public float getUltrasoundViewerScale(Context context){
        return Objects.requireNonNull(getUltrasoundViewer(context)).getZoomScale();
    }

    public void setUltrasoundViewerScale(Context context,float scaleFactor){
        Objects.requireNonNull(getUltrasoundViewer(context)).setZoomScale(scaleFactor);
    }

    public boolean getIsZoomed(Context context){
        float zoomFactor= Objects.requireNonNull(getUltrasoundViewer(context)).getZoomScale();
        return zoomFactor!=1.0F;
    }

    public boolean isDauOpen(){
        return mDau != null;
    }

    public void setDauSafetyTestOption(int selectedOption, float temperature) {
        mUltrasoundManager.setDauSafetyTestOption(selectedOption, temperature);
    }

    public float getDauThresholdTemperatureForTest() {
        return mUltrasoundManager.getDauThresholdTemperatureForTest();
    }

    public int getSafetTestSelectedOption() {
        return mUltrasoundManager.getSafetTestSelectedOption();
    }

    /**
     * Run ML Verification test(s) on a given input file tree. Output is written to a directory
     * created under [context.getCacheDir()].
     *
     * @param inputRoot Root folder of input file tree
     * @return Root of output file tree
     */
    public File runMLVerification(File inputRoot) {
        String version = VERSION_NAME +
                '.' +
                BUILD_NUMBER;
        return mUltrasoundManager.runMLVerification(version, inputRoot);
    }

    /**
     * Run ML Verification test(s) on a given input file tree. Output is written to the given
     * [outputRoot] folder.
     *
     * @param inputRoot Root folder of input file tree
     * @param outputRoot Root folder of output file tree
     * @return Root of output file tree
     */
    public File runMLVerification(File inputRoot, File outputRoot) {
        String version = VERSION_NAME +
                '.' +
                BUILD_NUMBER;
        return mUltrasoundManager.runMLVerification(version, inputRoot, outputRoot);
    }

    public void enumerateKosmosLink() {
        mUltrasoundManager.enumerateDevices();
    }

}
