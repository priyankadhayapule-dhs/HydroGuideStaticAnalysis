
/*
 * Copyright (C) 2017 EchoNous, Inc.
 */

package com.echonous.hardware.ultrasound;

import android.graphics.Bitmap;
import android.os.Handler;
import android.os.Looper;
import android.view.Surface;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.echonous.util.LogUtils;

import java.util.Arrays;

/**
 * <p>The UltrasoundViewer is a "container" for an Android Surface. The 
 * ultrasound imaging engine draws ultrasound images onto this surface.</p> 
 */
public final class UltrasoundViewer {
    private static final String TAG = "UltrasoundViewer";

    private UltrasoundManager   mManager = null;
    private HeartRateCallback   mHeartRateCallback = null;

    private ErrorCallback       mErrorCallback = null;
    private Handler             mErrorHandler = null;


    protected UltrasoundViewer() {
    }

    protected UltrasoundViewer(UltrasoundManager manager) {
        mManager = manager;
    }

    /**
     * Specify a new surface for the Dau to display live imaging.  It is necessary
     * to call this if a new surface is desired, or if the existing surface size
     * changes (i.e. {@code surfaceChanged()}).
     *
     * The Dau must be stopped before calling this method.
     *
     * @param surface The new surface
     *
     * @return <p>
     * <code>THOR_OK</code> on success<p>
     * <code>TER_UMGR_NATIVE_WINDOW</code> if unable to get native window from
     * surface<p>
     * <code>TER_IE_DAU_CLOSED</code> if this Dau instance is closed<p>
     * <code>TER_MISC_PARAM</code> if the passed surface is null<p>
     * <code>TER_IE_OPERATION_RUNNING</code> if currently running.  Can only change
     * the surface when stopped.<p>
     */
    public ThorError setSurface(Surface surface) {
        if (null == surface) {
            LogUtils.e(TAG, "surface cannot be null");
            return ThorError.TER_MISC_PARAM;
        }

        // TODO: Need to finish this
        //if (mIsRunning) {
        //    MedDevLog.error(TAG, "Cannot change the surface while running");
        //    return ThorError.TER_IE_OPERATION_RUNNING;
        //}
        nativeInit(this);
        int retCode = setSurfaceNative(surface);

        return ThorError.fromCode(retCode);
    }

    /**
     * Release the surface
     *  
     */
    public void releaseSurface() {
        releaseSurfaceNative();
    }

    /**
     * The method to provide the layout of the surface used for rendering B-Mode 
     * and M-Mode data in M-Mode.
     *
     * @param bmodeLayout - a float array of length 4 whose values are as follows:
     *                      [0] - x-origin of B-Mode region as a percent of Ultrasound window width
     *                      [1] - y-origin of B-Mode region as a percent of Ultrasound window height
     *                      [2] - width of B-Mode region as a percent of Ultrasound window width
     *                      [3] - height of B-Mode region as a percent of Ultrasound window height
     * @param mmodeLayout - a float array of length 4 whose values are as follows:
     *                      [0] - x-origin of M-Mode region as a percent of Ultrasound window width
     *                      [1] - y-origin of M-Mode region as a percent of Ultrasound window height
     *                      [2] - width of M-Mode region as a percent of Ultrasound window width
     *                      [3] - height of M-Mode region as a percent of Ultrasound window height
     * 
     * @return <p>
     * <code>THOR_OK</code> on success<p>
     * <code>TER_MISC_PARAM</code> if the passed parameters are invalide.<p>
     *
     */
    public ThorError setMmodeLayout(float[] bmodeLayout, float[] mmodeLayout)  {
        int retCode = setMmodeLayoutNative( bmodeLayout, mmodeLayout );
        return ThorError.fromCode(retCode);
    }

    /**
     * The method to enable/disable DA and ECG/EKG.s
     *
     * @param isUSOn-boolean to enable/disable US
     * @param isDAOn - boolean to enable/disable DA
     * @param isECGOn - boolean to enable/disable ECG
     *
     * @return <p>
     * <code>THOR_OK</code> on success<p>
     * <code>TER_MISC_PARAM</code> if the passed parameters are invalide.<p>
     *
     */
    public ThorError setupUSDAECG(boolean isUSOn, boolean isDAOn, boolean isECGOn) {
        int retCode = setupDAECGNative(isUSOn,isDAOn, isECGOn);
        return ThorError.fromCode(retCode);
    }

    /**
     * The method to provide the layout of the surface used for rendering Ultrasound Image
     * and DA/ECG.  This method is not mode specific (independent of modes).
     * The ultrasound imaging region can be B-mode, C-mode, B/M-mode, B/PW-mode and others.
     *
     * @param usLayout - a float array of length 4 whose values are as follows:
     *                      [0] - x-origin of ultrasound imaging region as a percent of Ultrasound window width
     *                      [1] - y-origin of ultrasound imaging region as a percent of Ultrasound window height
     *                      [2] - width of ultrasound imaging region as a percent of Ultrasound window width
     *                      [3] - height of ultrasound imaging region as a percent of Ultrasound window height
     * @param daLayout - a float array of length 4 whose values are as follows:
     *                      [0] - x-origin of DA waveform region as a percent of Ultrasound window width
     *                      [1] - y-origin of DA waveform region as a percent of Ultrasound window height
     *                      [2] - width of DA waveform region as a percent of Ultrasound window width
     *                      [3] - height of DA waveform region as a percent of Ultrasound window height
     * @param ecgLayout - a float array of length 4 whose values are as follows:
     *                      [0] - x-origin of ECG waveform region as a percent of Ultrasound window width
     *                      [1] - y-origin of ECG waveform region as a percent of Ultrasound window height
     *                      [2] - width of ECG waveform region as a percent of Ultrasound window width
     *                      [3] - height of ECG waveform region as a percent of Ultrasound window height
     *
     * @return <p>
     * <code>THOR_OK</code> on success<p>
     * <code>TER_MISC_PARAM</code> if the passed parameters are invalide.<p>
     *
     */
    public ThorError setDAECGLayout(float[] usLayout, float[] daLayout, float[] ecgLayout) {
        int retCode = setDAECGLayoutNative(usLayout, daLayout, ecgLayout);
        return ThorError.fromCode(retCode);
    }

    /**
     * The method to provide the layout of the surface used for rendering PW/CW Doppler Ultrasound Image.
     * This methods is specifically designed to be used for PW/CW and should be used in conjunction.
     * with setDAECGLayout.
     *
     * @param dopplerLayout - a float array of length 4 whose values are as follows:
     *                      [0] - x-origin of ultrasound imaging region as a percent of PW/CW Ultrasound window width
     *                      [1] - y-origin of ultrasound imaging region as a percent of PW/CW Ultrasound window height
     *                      [2] - width of ultrasound imaging region as a percent of PW/CW Ultrasound window width
     *                      [3] - height of ultrasound imaging region as a percent of PW/CW Ultrasound window height
     * @return <p>
     * <code>THOR_OK</code> on success<p>
     * <code>TER_MISC_PARAM</code> if the passed parameters are invalide.<p>
     *
     */
    public ThorError setDopplerLayout(float[] dopplerLayout) {
        int retCode = setDopplerLayoutNative(dopplerLayout);
        return ThorError.fromCode(retCode);
    }

    /**
     * update doppler baseline sift invert
     * @param baselineShiftIndex
     * @param isInvert
     */
    public void updateDopplerBaselineShiftInvert(int baselineShiftIndex, Boolean isInvert){
        LogUtils.d(TAG,"updateDopplerBaselineShiftInvert baselineShiftIndex : " +baselineShiftIndex + " isInvert : " + isInvert);
        updateDopplerBaselineShiftInvertNative(baselineShiftIndex,isInvert);
    }

    /**
     * set peak mean drawing
     * @param peakDrawing
     * @param meanDrawing
     */
    public void setPeakMeanDrawing(Boolean peakDrawing, Boolean meanDrawing){
        LogUtils.d(TAG,"setPeakMeanDrawing : peakDrawing : " + peakDrawing + " meanDrawing : " + meanDrawing);
        setPeakMeanDrawingNative(peakDrawing,meanDrawing);
    }

    /**
     * get doppler peak max
     * @return max peak velocity
     */
    public float getDopplerPeakMax(){
        LogUtils.d(TAG,"getDopplerPeakMax called");
        return getDopplerPeakMaxNative();
    }

    /**
     * doppler peak max info class
     */
    public static class DopplerPeakMaxInfo{
        // define the size
        // max control points => 60
        // index 126, 127 -> center and last index
        public int arraySize = 128;
        public int DESIRED_PEAK_LOC = 121;
        public int MIN_SEARCH_RANGE = 122;
        public int MAX_SEARCH_RANGE = 123;
        public int TRACELINE_POS = 124;
        public int OFFSET_POS = 125;
        public int CENTER_INDEX_POS = 126;
        public int LAST_INDEX_POS = 127;
        public int WF_TYPE = 120;
        public int QUERY_X_COORD_POS = 118;
        public int QUERY_Y_COORD_POS = 119;
        public float[] mapMat = new float[arraySize];
        public int centerIndex = 0;
        public int lastIndex = 0;
        public int offset = 0;
        public int tracelineLoc = 0;

        public float queriedYCoord = 0.0F;

        public int WF_QUERY_Y_COORD = 201;
    }

    /**
     * TimeAvg Matrix should be used to communicate with Native layer and Application layer, for
     * getting TimeAvg data for a particular frame.
     */
    public static class TimeAvgMatrix {
        private final int defaultArraySize = 256;
        public float[] peakMaxPositive = new float[defaultArraySize];
        public float[] peakMeanPositive = new float[defaultArraySize];
        public float[] peakMaxNegative = new float[defaultArraySize];
        public float[] peakMeanNegative = new float[defaultArraySize];
        public int size;
    }

    /**
     * get doppler peak max
     * @param isFlip
     * @return x,y coordinates mapMat
     */
    public DopplerPeakMaxInfo getDopplerPeakMaxCoordinate(Boolean isFlip, Integer offset) {
        LogUtils.d(TAG,"getDopplerPeakMaxCoordinate called isFlip : " + isFlip);
        return getDopplerPeakMaxCoordinate(isFlip, 0, 0, 0, offset);
    }

    /**
     * get doppler peak max
     * @param isFlip
     * @return x,y coordinates mapMat
     */
    public DopplerPeakMaxInfo getDopplerPeakMaxCoordinate(Boolean isFlip, Integer offset, Integer waveformType) {
        LogUtils.d(TAG,"getDopplerPeakMaxCoordinate called isFlip: " + isFlip);
        return getDopplerPeakMaxCoordinate(isFlip, 0, 0, 0, offset, waveformType, 0F);
    }

    /**
     * get doppler peak max
     * @param isFlip
     * @param queryXCoord: pixel coordinate (not normalized)
     * @return x,y coordinates mapMat
     */
    public DopplerPeakMaxInfo getDopplerPeakMaxCoordinate(Boolean isFlip, Integer offset, Integer waveformType, float queryXCoord) {
        LogUtils.d(TAG,"getDopplerPeakMaxCoordinate called isFlip: " + isFlip);
        return getDopplerPeakMaxCoordinate(isFlip, 0, 0, 0, offset, waveformType, queryXCoord);
    }

    /**
     * get doppler peak max
     * @param isFlip (tracelinle Ivert), offset
     * @return x,y coordinates mapMat
     */
    public DopplerPeakMaxInfo getDopplerPeakMaxCoordinate(Boolean isFlip, int desiredPeakLoc,
                                                          int minSearchLoc, int maxSearchLoc, int offset) {
        return getDopplerPeakMaxCoordinate(isFlip, desiredPeakLoc,  minSearchLoc, maxSearchLoc, offset, 0, 0F);
    }

    /**
     * get doppler peak max
     * @param isFlip (traceline Invert), offset, waveformType
     * @return x,y coordinates mapMat
     */
    public DopplerPeakMaxInfo getDopplerPeakMaxCoordinate(Boolean isFlip, int desiredPeakLoc,
                                                          int minSearchLoc, int maxSearchLoc, int offset, int waveformType, float queryXCoord) {
        LogUtils.d(TAG,"getDopplerPeakMaxCoordinate called - Offset: " + offset);
        DopplerPeakMaxInfo mDopplerPeakMaxInfo = new DopplerPeakMaxInfo();
        Arrays.fill(mDopplerPeakMaxInfo.mapMat, 0.0f);
        mDopplerPeakMaxInfo.mapMat[mDopplerPeakMaxInfo.DESIRED_PEAK_LOC] = (float) desiredPeakLoc;
        mDopplerPeakMaxInfo.mapMat[mDopplerPeakMaxInfo.MIN_SEARCH_RANGE] = (float) minSearchLoc;
        mDopplerPeakMaxInfo.mapMat[mDopplerPeakMaxInfo.MAX_SEARCH_RANGE] = (float) maxSearchLoc;
        mDopplerPeakMaxInfo.mapMat[mDopplerPeakMaxInfo.OFFSET_POS] = (float) offset;
        mDopplerPeakMaxInfo.mapMat[mDopplerPeakMaxInfo.WF_TYPE] = (float) waveformType;
        if (waveformType == mDopplerPeakMaxInfo.WF_QUERY_Y_COORD) {
            mDopplerPeakMaxInfo.mapMat[mDopplerPeakMaxInfo.QUERY_X_COORD_POS] = queryXCoord;
        }

        int arrayLength =  mDopplerPeakMaxInfo.mapMat.length;
        getDopplerPeakMaxCoordinateNative(mDopplerPeakMaxInfo.mapMat, arrayLength, isFlip);
        mDopplerPeakMaxInfo.centerIndex = (int) mDopplerPeakMaxInfo.mapMat[mDopplerPeakMaxInfo.CENTER_INDEX_POS];
        mDopplerPeakMaxInfo.lastIndex = (int) mDopplerPeakMaxInfo.mapMat[mDopplerPeakMaxInfo.LAST_INDEX_POS];
        mDopplerPeakMaxInfo.offset = (int) mDopplerPeakMaxInfo.mapMat[mDopplerPeakMaxInfo.OFFSET_POS];
        mDopplerPeakMaxInfo.tracelineLoc = (int) mDopplerPeakMaxInfo.mapMat[mDopplerPeakMaxInfo.TRACELINE_POS];
        if (waveformType == mDopplerPeakMaxInfo.WF_QUERY_Y_COORD) {
            mDopplerPeakMaxInfo.queriedYCoord = mDopplerPeakMaxInfo.mapMat[mDopplerPeakMaxInfo.QUERY_Y_COORD_POS];
        }

        LogUtils.d(TAG,"getDopplerPeakMaxCoordinate called - return form native side mDopplerPeakMaxInfo.offset: " + mDopplerPeakMaxInfo.offset);
        return mDopplerPeakMaxInfo;
    }

    /**
     * Send Time avg co-ordinates to native
     *
     * @return TimeAvgMatrix
     */
    public TimeAvgMatrix setTimeAvgCoOrdinates(TimeAvgMatrix timeAvgMatrix) {

        timeAvgMatrix.size = setTimeAvgCoOrdinatesNative(
                timeAvgMatrix.peakMaxPositive,
                timeAvgMatrix.peakMeanPositive,
                timeAvgMatrix.peakMaxNegative,
                timeAvgMatrix.peakMeanNegative);

        return timeAvgMatrix;
    }

    /**
     * The method to prepare the transition from to B and PW/CW
     * This function sets CineViewer flag to store the latest frame.
     *
     * @return <p>
     * <code>THOR_OK</code> on success<p>
     * <code>TER_MISC_PARAM</code> if the passed parameters are invalide.<p>
     *
     */
    public ThorError prepareDopplerTransition(boolean prepareTransitionFrame) {
        int retCode = prepareDopplerTransitionNative(prepareTransitionFrame);
        return ThorError.fromCode(retCode);
    }
    
    // TODO: Implement this
    public Surface getSurface() {
        return null;
    }

    //
    // methods for zoom and pan
    //

    /**
     * Handle Scroll callback for panning the ultrasound image.  Intended to be 
     * called from the onScroll method of GestureDetector.OnGestureListener. 
     * 
     * @param distX The distance on X axis since the last onScroll().
     * @param distY The distance on Y axis since the last onScroll().
     */
    public void handleOnScroll(float distX, float distY) {
        nativeHandleOnScroll(distX, distY);

        if (mManager.isPlayerAttached()) {
            UltrasoundPlayer    player = mManager.getUltrasoundPlayer();

            if (!player.isRunning()) {
                // It is necessary to redraw the current frame to see
                // the effect of panning.
                player.seekToFrame(player.getCurrentFrameNo(), false);
            }
        }
    }

    /**
     * Handle Scale callback for zooming the ultrasound image.  Intended to be 
     * called from the onScale method of 
     * ScaleGestureDetector.OnScaleGestureListener. 
     * 
     * @param scaleFactor 
     */
    public void handleOnScale(float scaleFactor) {
        nativeHandleOnScale(scaleFactor);

        if (mManager.isPlayerAttached()) {
            UltrasoundPlayer    player = mManager.getUltrasoundPlayer();
            if (!player.isRunning()) {
                // It is necessary to redraw the current frame to see
                // the effect of scaling.
                player.seekToFrame(player.getCurrentFrameNo(), false);
            }
        }
    }

    public void handleOnTouch(float x, float y, boolean isDown) {
        nativeHandleOnTouch(x, y, isDown);

        if (mManager.isPlayerAttached()) {
            UltrasoundPlayer    player = mManager.getUltrasoundPlayer();
            if (!player.isRunning()) {
                // It is necessary to redraw the current frame to see
                // the effect of scaling.
                player.seekToFrame(player.getCurrentFrameNo(), false);
            }
        }
    }

    /**
     * getting DisplayDepth - top and bottom imaging depth in mm scale
     * 
     * @param topBottomDepths 
     */
    public void queryDisplayDepth(float[] topBottomDepths) {
	nativeQueryDisplayDepth(topBottomDepths);
    }

    /**
     * Get Physical to Pixel Map Matrix 
     *  
     * @param mapMat 
     *  
     * <p>
     * 3x3 matrix<p>
     * [mapMat[0] mapMat[3] mapMat[6]]<p>
     * [mapMat[1] mapMat[4] mapMat[7]]<p>
     * [mapMat[2] mapMat[5] mapMat[8]]<p>
     */
    public void queryPhysicalToPixelMap(float[] mapMat) {
	nativeQueryPhysicalToPixelMap(mapMat);
    }

    /**
     * Getting UltrasoundScreenImage that is rendered on the Ultrasound Window
     * without any overlays.  Input param is an empty bitmapImage, which is 
     * then rendered by the native renderer.
     * 
     * @param bitmapImage 
     */
    public void getUltrasoundScreenImage(Bitmap bitmapImage) {
	nativeGetUltrasoundScreenImage(bitmapImage, false, -1);
    }

    /**
     * Getting UltrasoundScreenImage that is rendered on the Ultrasound Window
     * without any overlays.  Input param is an empty bitmapImage, which is
     * then rendered by the native renderer.
     *
     * @param bitmapImage
     * @param isForThumbnail set true if intended to use as a thumbnail
     *                       This is particularly important when DA/ECG is on.
     *
     */
    public void getUltrasoundScreenImage(Bitmap bitmapImage, boolean isForThumbnail) {
    nativeGetUltrasoundScreenImage(bitmapImage, isForThumbnail, -1);
    }

    /**
     * Getting UltrasoundScreenImage that is rendered on the Ultrasound Window
     * without any overlays.  Input param is an empty bitmapImage, which is
     * then rendered by the native renderer.
     *
     * @param bitmapImage
     * @param isForThumbnail set true if intended to use as a thumbnail
     *                       This is particularly important when DA/ECG is on.
     * @param desiredFrameNumber specify frame number, if specific frame needs
     *                           to be used.  Set -1 for the current Frame.
     *
     */
    public void getUltrasoundScreenImage(Bitmap bitmapImage, boolean isForThumbnail,
                                         int desiredFrameNumber) {
    nativeGetUltrasoundScreenImage(bitmapImage, isForThumbnail, desiredFrameNumber);
    }

    /**
     * Get zoom, pan, and flip related parameters.  It's in a float array format.
     * In an order of zoom (scale), Pan (deltaX, deltaY), FlipXY (flipX/LR, flipY/UD).
     *
     * @param zoomPanFlipParams
     */
    public void queryZoomPanFlipParams(float[] zoomPanFlipParams) {
    nativeQueryZoomPanFlipParams(zoomPanFlipParams);
    }

    /**
     * Set zoom, pan, and flip related parameters. It's in a float array format. In
     * an order of zoom (scale), Pan (deltaX, deltaY), FlipXY (flipX/LR, flipY/UD).
     *
     * @param zoomPanFlipParams
     */
    public void setZoomPanFlipParams(float[] zoomPanFlipParams) {
    setZoomPanFlipParamsNative(zoomPanFlipParams);
    }

    /**
     * getting MLine Time
     * This is only needed for M-mode exam review
     * 
     */
    public float getTraceLineTime() {
        return nativeGetTraceLineTime();
    }

    /**
     * getting Frame Interval MS
     * for M-mode exam review
     *
     */
    public int getFrameIntervalMs() {
        return nativeGetFrameIntervalMs();
    }

    /**
     * getting MLine Per Frame
     * for M-mode exam review
     *
     */
    public int getMLinesPerFrame() {
        return nativeGetMLinesPerFrame();
    }

    /**
     * refresh/redraw Layout once parameters updated
     * This only works in Cine Review/Freeze state.
     *
     * @return <p>
     * <code>THOR_OK</code> on success<p>
     * <code>TER_IE_CINE_VIEWER_REFRESH</code> if failed refresh/redraw.<p>
     */
    public ThorError refreshLayout()
    {
        int retCode = refreshLayoutNative();
        return ThorError.fromCode(retCode);
    }

    /**
     * set Color Map with ColorMapId
     *
     * @param colorMapId
     * @param isInverted (inverted colormap or not)
     *
     * @return <p>
     * <code>THOR_OK</code> on success<p>
     * <code>THOR_ERROR</code> if failed set/update.<p>
     */
    public ThorError setColorMap(int colorMapId, boolean isInverted)
    {
        int retCode = setColorMapNative(colorMapId, isInverted);
        return ThorError.fromCode(retCode);
    }

    /**
     *  get zoom factor value of ultrasound view
     * @return <p>
     *     Current zoom factor of the B mode and Color Mode,
     *     Range can be 1.0F to 4.0F
     * </p>
     */
    public float getZoomScale(){

        return getScale();
    }

    /**
     * Set DaEcg scroll speed index.
     *
     * @param scrollSpeedIndex
     */
    public void setDaEcgScrollSpeedIndex(int scrollSpeedIndex)
    {
        setDaEcgScrollSpeedIndexNative(scrollSpeedIndex);
    }

    /**
     *  Set 1.0F to zoom out ultrasound viewer
     *  Set Zoom scale from range of 1.0F to 4.0F
     * @param scaleFactor Pass scale factor from range of 1.0F to 4.0F
     */
    public void setZoomScale(float scaleFactor){
        setScale(scaleFactor);
    }

    public void setZoomPan(float deltaX,float deltaY){
        setPan(deltaX,deltaY);
    }

    protected void onHeartRateUpdate(final float heartRate){
          if(mHeartRateCallback!=null) {
               mHeartRateCallback.onHeartRate(heartRate);
          }
    }

    /**
     * A callback for heart rate update during ECG Enable
     *
     * @see #registerHeartRateCallback
     *
     */
    public static abstract class HeartRateCallback {

        /**
         *  A new playback position occurred
         *
         *  @param heartRate The new value of the Heart Rate
         */
        public void onHeartRate(float heartRate) { }
    }

    public synchronized void registerHeartRateCallback(@NonNull HeartRateCallback heartRateCallBack){

        mHeartRateCallback=heartRateCallBack;
    }

    /**
     * Unregister the existing callback.
     */
    public synchronized void unregisterHeartRateCallback() {
        mHeartRateCallback = null;
    }

    /**
     * A callback for errors that occur while rendering ultrasound and audio data.
     * These can occur during live data aquisition as well as playback.
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
     * Registers a callback for notification of Viewer imaging errors.  Only one
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
     * @return boolean whether native object of cineviewer is initialized or not
     */
    public boolean checkNativeCineViewerIsInitialized() {
        return nativeGetCineViewerInitialisedStatus();
    }

    private static native int setSurfaceNative(Surface surface);

    private static native void releaseSurfaceNative();

    private static native int setMmodeLayoutNative(float[] bmodeLayout, float[] mmodeLayout);
    
    private static native void setZoomPanFlipParamsNative(float[] zoomPanFlipParams);

    private static native void nativeHandleOnScroll(float distX, float distY);

    private static native void nativeHandleOnScale(float scaleFactor);

    private static native void nativeQueryDisplayDepth(float[] topBottomDisplayDepths);

    private static native void nativeHandleOnTouch(float x, float y, boolean isDown);

    private static native void nativeQueryPhysicalToPixelMap(float[] mapMat);

    private static native void nativeQueryZoomPanFlipParams(float[] zoomPanFlipParmas);

    private static native void nativeGetUltrasoundScreenImage(Bitmap bitmapImage, boolean isForThumbnail, int desiredFrameNum);

    private static native float nativeGetTraceLineTime();

    private static native int nativeGetFrameIntervalMs();

    private static native int nativeGetMLinesPerFrame();

    private static native int setupDAECGNative(boolean isUSOn,boolean isDAOn, boolean isECGOn);

    private static native int setDAECGLayoutNative(float[] usLayout, float[] daLayout, float[] ecgLayoust);

    private static native int setDopplerLayoutNative(float[] pwLayout);

    private static native void updateDopplerBaselineShiftInvertNative(int baselineShiftIndex, boolean isInvert);

    private static native void setPeakMeanDrawingNative(boolean peakDrawing, boolean meanDrawing);

    private static native int prepareDopplerTransitionNative(boolean prepareTransitionFrame);

    private static native float getDopplerPeakMaxNative();

    private static native void getDopplerPeakMaxCoordinateNative(float[] mapMat, int arrayLength, boolean isTracelineInvert);

    private static native int refreshLayoutNative();

    private static native int setColorMapNative(int colorMapId, boolean isInverted);

    private static native float getScale();

    private static native void setDaEcgScrollSpeedIndexNative(int scrollSpeedIndex);

    private static native void setScale(float scaleFactor);

    private static native void setPan(float deltaX,float deltaY);

    private static native void nativeInit(UltrasoundViewer ultrasoundViewer);

    private static native boolean nativeGetCineViewerInitialisedStatus();

    private static native int setTimeAvgCoOrdinatesNative(float[] peakMaxPositive_,
                                                          float[] peakMeanPositive_,
                                                          float[] peakMaxNegative_,
                                                          float[] peakMeanNegative_);

}
