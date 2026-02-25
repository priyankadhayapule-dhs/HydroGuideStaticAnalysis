//
// Copyright 2017 EchoNous Inc.
//
//

#define LOG_TAG "CineViewer"

#include <poll.h>
#include <unistd.h>
#include <ThorUtils.h>
#include <CineViewer.h>
#include <BitfieldMacros.h>
#include <DaEcgRecordParams.h>
#include <EcgProcess.h>
#include <AIManager.h>

#define DA_ECG_LINE_THICKNESS 1

#define CHECK_SKIPPED_FRAMES

#define ENABLE_AUDIO_PLAYER

//-----------------------------------------------------------------------------
CineViewer::CineViewer(CineBuffer& cineBuffer, AIManager& aiManager) :
    CineCallback(),
    mCineBuffer(cineBuffer),
    mAIManager(aiManager),
    mWindowPtr(nullptr),
    mImguiRenderer(*this),
    mAudioPlayer(nullptr),
    mDASamplesPerFrame(0),
    mDASampleRate(0),
    mIsDAOn(false),
    mIsECGOn(false),
    mIsInitialized(false),
    mIsLive(false),
    mPrepareTransitionFrame(false),
    mTransitionImgMode(-1),
    mReportHeartRate(nullptr),
    mPrevDisplayHR(-1.0f),
    mHRUpdateInterval(64),
    mErrorReporter(nullptr)
{
    // default B/MMode Layout
    mBModeLayout[0] = 0.0f;
    mBModeLayout[1] = 50.0f;
    mBModeLayout[2] = 100.0f;
    mBModeLayout[3] = 50.0f;
    mMModeLayout[0] = 0.0f;
    mMModeLayout[1] = 0.0f;
    mMModeLayout[2] = 100.0f;
    mMModeLayout[3] = 50.0f;

    // default US, DA, ECG Layout
    mUSLayout[0] = 0.0f;
    mUSLayout[1] = 30.0f;
    mUSLayout[2] = 100.0f;
    mUSLayout[3] = 70.0f;
    mDALayout[0] = 0.0f;
    mDALayout[1] = 15.0f;
    mDALayout[2] = 100.0f;
    mDALayout[3] = 15.0f;
    mECGLayout[0] = 0.0f;
    mECGLayout[1] = 0.0f;
    mECGLayout[2] = 100.0f;
    mECGLayout[3] = 15.0f;

    // default Doppler (PW/CW) Layout - default 100/44
    mDOPLayout[0] = 0.0f;
    mDOPLayout[1] = 0.0f;
    mDOPLayout[2] = 100.0f;
    mDOPLayout[3] = 44.0f;

    // default DA/ECG Line Color
    mDALineColor[0] = 0.271f;
    mDALineColor[1] = 0.631f;
    mDALineColor[2] = 0.910f;

    mECGLineColor[0] = 0.012f;
    mECGLineColor[1] = 0.941f;
    mECGLineColor[2] = 0.420f;

    //default Tint Value
    mTintR = 0.84f;
    mTintG = 0.88f;
    mTintB = 0.90f;
}

//-----------------------------------------------------------------------------
CineViewer::~CineViewer()
{
    ALOGD("%s", __func__);

    onTerminate();
    if (mAudioPlayer != nullptr)
    {
        delete mAudioPlayer;
        mAudioPlayer = nullptr;
    }
    setWindow(nullptr);
    mRenderSurface.close();
}

//-----------------------------------------------------------------------------
ThorStatus CineViewer::onInit()
{
    ThorStatus      retVal = TER_IE_CINE_VIEWER_INIT;
    int             ret;
    uint64_t        data;

    ALOGD("%s", __func__);

    if (mIsInitialized)
    {
        ALOGE("Already initialized - Cannot re-initialize");
        goto err_ret;
    }

    if (!mDataEvent.init())
    {
        ALOGE("Unable to initialize mDataEvent");
        goto err_ret;
    }

    if (!mStopEvent.init(EventMode::ManualReset, false))
    {
        ALOGE("Unable to initialize StopEvent");
        goto err_ret;
    }
    mStopEvent.reset();

    if (!mPausePlaybackEvent.init(EventMode::ManualReset, false))
    {
        ALOGE("Unable to initialize PausePlaybackEvent");
        goto err_ret;
    }
    mPausePlaybackEvent.reset();

    if (!mRunningEvent.init())
    {
        ALOGE("Unable to initialize RunningEvent");
        goto err_ret;
    }

    ret = pthread_create(&mThreadId, NULL, workerThreadEntry, this);
    if (0 != ret)
    {
        ALOGE("Failed to create worker thread: ret = %d", ret);
        goto err_ret;
    }

    // The various query and UI handler methods cannot be safely called until
    // the worker thread is running.
    mRunningEvent.wait();

    // Check status of worker thread to ensure it initialized correctly.
    data = mRunningEvent.read();
    retVal = data & 0xFFFFFFFF;

    if (!IS_THOR_ERROR(retVal))
    {
        mIsInitialized = true;
        retVal = THOR_OK;
    }

err_ret:
    return(retVal);
}

//-----------------------------------------------------------------------------
void CineViewer::onTerminate()
{
    ALOGD("%s", __func__);

    if (mIsInitialized)
    {
        mStopEvent.set();
        pthread_join(mThreadId, NULL);

        mIsInitialized = false;
    }
}

//-----------------------------------------------------------------------------
ThorStatus CineViewer::refreshLayout()
{
    ThorStatus      retVal = TER_IE_CINE_VIEWER_REFRESH;
    int             ret;
    uint64_t        data;

    ALOGD("%s", __func__);

    if (mIsInitialized)
    {
        mStopEvent.set();
        pthread_join(mThreadId, NULL);

        ret = pthread_create(&mThreadId, NULL, workerThreadEntry, this);
        if (0 != ret)
        {
            ALOGE("Failed to create worker thread: ret = %d", ret);
            goto err_ret;
        }

        mRunningEvent.wait();

        data = mRunningEvent.read();
        retVal = data & 0xFFFFFFFF;

        if (IS_THOR_ERROR(retVal))
        {
            return retVal;
        }

        // call onData to refresh the drawing.
        onData(mCineBuffer.getCurFrameNum(), mCineBuffer.getParams().dauDataTypes);
    }

    retVal = THOR_OK;

err_ret:
    return(retVal);
}

//-----------------------------------------------------------------------------
void CineViewer::setWindow(ANativeWindow* windowPtr)
{
    if (nullptr != mWindowPtr)
    {
        ALOGD("%s: Release window", __func__);
        ANativeWindow_release(mWindowPtr);
    }

    if (nullptr != windowPtr)
    {
        ALOGD("%s Acquire window", __func__);
        ANativeWindow_acquire(windowPtr);
    }
    mWindowPtr = windowPtr;
}

//-----------------------------------------------------------------------------
void CineViewer::setParams(CineBuffer::CineParams& params)
{
    ALOGD("%s", __func__);

    mImagingMode = params.imagingMode;

    float desiredTimeSpan;
    float baselineShiftFracton;
    uint32_t desiredSweepSpeedIndex = params.daEcgScrollSpeedIndex;

    // for older M-mode sweep speed calculation
    bool  updateDesiredTimeSpan = false;

    if ( (mImagingMode == IMAGING_MODE_PW) || (mImagingMode == IMAGING_MODE_CW) ){
        desiredSweepSpeedIndex = params.dopplerSweepSpeedIndex;
    } else if (mImagingMode == IMAGING_MODE_M) {
        if (params.mModeSweepSpeedIndex < 0)
            updateDesiredTimeSpan = true;
        else
            desiredSweepSpeedIndex = params.mModeSweepSpeedIndex;
    }

    // get TimeSpan from ThorUtil
    uint32_t organGlobalId = mCineBuffer.getParams().upsReader->getOrganGlobalId(params.organId);
    desiredTimeSpan = getTimeSpan(desiredSweepSpeedIndex, organGlobalId == TCD_GLOBAL_ID);

    // to support older M-mode data
    if (updateDesiredTimeSpan)
    {
        uint32_t imgWidth = 1520;

        if (mWindowPtr != nullptr)
            imgWidth = ANativeWindow_getWidth(mWindowPtr);

        desiredTimeSpan = imgWidth / params.scrollSpeed;
    }

    // read Tint value from UPS
    mCineBuffer.getParams().upsReader->getTint(mTintR, mTintG, mTintB);

    switch (mImagingMode)
    {
        case IMAGING_MODE_B:
            mBModeRenderer.setParams(params.converterParams);
            mBModeRenderer.setTintAdjustment(mTintR, mTintG, mTintB);
            break;

        case IMAGING_MODE_COLOR:
            int                         bThreshold;
            int                         velThreshold;
            uint32_t                    velocityMap[256];

            params.upsReader->getBEThresholds(params.imagingCaseId, velThreshold, bThreshold);
            params.upsReader->getColorMap(params.colorMapIndex, params.isColorMapInverted, velocityMap);
#if 0
            struct ScanConverterParams  cParams;
            params.upsReader->getCScanConverterParams(params.imagingCaseId, cParams);

            mCModeRenderer.setParams(params.converterParams,
                                     cParams,
                                     bThreshold,
                                     velThreshold,
                                     (uint8_t*) velocityMap);
#else
            mCModeRenderer.setParams(params.converterParams,
                                     params.colorConverterParams,
                                     bThreshold,
                                     velThreshold,
                                     (uint8_t*) velocityMap,
                                     params.colorDopplerMode);
            mCModeRenderer.setTintAdjustment(mTintR, mTintG, mTintB);
#endif
            break;

        case IMAGING_MODE_M:
            mBModeRenderer.setParams(params.converterParams);
            mBModeRenderer.setTintAdjustment(mTintR, mTintG, mTintB);
#if 0
            mMModeRenderer.setParams(params.converterParams.numSamples, params.converterParams.sampleSpacingMm, 0.0f,
                                     735, 30.0f, 25, 33.0f);
#else
            // TODO: update params for Exam Review mode.  Restore Width from Scroll Speed Width
            mSModeRenderer.setInvert(true);     // top to bottom: +ve direction
            mSModeRenderer.setParams(floor(desiredTimeSpan*params.scrollSpeed),
                                     params.converterParams.numSamples,
                                     params.scrollSpeed,
                                     0.5f,  // hard-coded for M-mode
                                     params.targetFrameRate,
                                     params.mlinesPerFrame,
                                     params.frameRate,
                                     params.mlinesPerFrame,
                                     desiredTimeSpan,
                                     params.converterParams.numSamples * params.converterParams.sampleSpacingMm);  // Imaging Depth
            mSModeRenderer.setSingleFrameDrawIndex(params.traceIndex);
            mSModeRenderer.setTintAdjustment(mTintR, mTintG, mTintB);
#endif
            break;

        case IMAGING_MODE_PW:
            params.upsReader->getPwBaselineShiftFraction(params.dopplerBaselineShiftIndex, baselineShiftFracton);
            mSModeRenderer.setInvert(params.dopplerBaselineInvert);
            mSModeRenderer.setParams(floor(desiredTimeSpan*params.scrollSpeed), params.dopplerFFTSize, params.scrollSpeed,
                                      baselineShiftFracton, params.targetFrameRate, params.dopplerNumLinesPerFrame,
                                      params.frameRate, params.dopplerSamplesPerFrame, desiredTimeSpan, params.dopplerVelocityScale*2.0f);
            mSModeRenderer.setTintAdjustment(mTintR, mTintG, mTintB);

            mDopplerAudioSamplesPerFrame = params.dopplerSamplesPerFrame * params.dopplerAudioUpsampleRatio;
            mDopplerAudioSampleRate = ceil(mDopplerAudioSamplesPerFrame * params.frameRate);
            ALOGD("PW AudioPlayer - samplesPerFrame = %u, sampleRate: %u", mDopplerAudioSamplesPerFrame, mDopplerAudioSampleRate);
            break;

        case IMAGING_MODE_CW:
            // TODO: CW specific functions
            params.upsReader->getCwBaselineShiftFraction(params.dopplerBaselineShiftIndex, baselineShiftFracton);
            mSModeRenderer.setInvert(params.dopplerBaselineInvert);
            mSModeRenderer.setParams(floor(desiredTimeSpan*params.scrollSpeed), params.dopplerFFTSize, params.scrollSpeed,
                                      baselineShiftFracton, params.targetFrameRate, params.dopplerNumLinesPerFrame,
                                      params.frameRate, params.dopplerSamplesPerFrame, desiredTimeSpan, params.dopplerVelocityScale*2.0f, 2.0f);
            mSModeRenderer.setTintAdjustment(mTintR, mTintG, mTintB);

            // TODO: CW AUDIO Params
            mDopplerAudioSamplesPerFrame = ((float) params.dopplerSamplesPerFrame)/((float) params.cwDecimationFactor) * params.dopplerAudioUpsampleRatio;
            mDopplerAudioSampleRate = ceil(mDopplerAudioSamplesPerFrame * params.frameRate);
            ALOGD("CW AudioPlayer - samplesPerFrame = %u, sampleRate: %u", mDopplerAudioSamplesPerFrame, mDopplerAudioSampleRate);
            break;
    }

    mRenderSurface.setTargetDisplayFrameRate(params.targetFrameRate);

    // TODO: BUGBUG - need to update the parameters
    float samplesPerSecond = params.ecgSamplesPerSecond;
    uint32_t ecgSamplesPerScreen;
    uint32_t daSamplesPerScreen;

    ecgSamplesPerScreen = (uint32_t)floor(desiredTimeSpan * params.ecgSamplesPerSecond);
    mECGTimeSpan = ecgSamplesPerScreen / samplesPerSecond;
    daSamplesPerScreen = (uint32_t)floor(desiredTimeSpan * params.daSamplesPerSecond);
    mDASamplesPerFrame = params.daSamplesPerFrame;
    mDASampleRate = ceil(params.daSamplesPerSecond);

    LOGD("DA AudioPlayer - samplesPerFrame = %d, sampleRate: %d", mDASamplesPerFrame, mDASampleRate);

    mDARenderer.setParams(daSamplesPerScreen, params.daSamplesPerSecond, params.targetFrameRate, params.daSamplesPerFrame, params.frameRate, desiredTimeSpan);
    mECGRenderer.setParams(ecgSamplesPerScreen, params.ecgSamplesPerSecond, params.targetFrameRate, params.ecgSamplesPerFrame, params.frameRate, desiredTimeSpan);

    LOGD("DA samples per screen = %d, daSamplesPerSecond = %f, targetFrameRate = %d, daSamplesPerFrame = %d, frameRate = %f",
         daSamplesPerScreen, params.daSamplesPerSecond, params.targetFrameRate, params.daSamplesPerFrame, params.frameRate);
    LOGD("ECG samples per screen = %d, ecgSamplesPerSecond = %f, targetFrameRate = %d, ecgSamplesPerFrame = %d, frameRate = %f",
         ecgSamplesPerScreen, params.ecgSamplesPerSecond, params.targetFrameRate, params.ecgSamplesPerFrame, params.frameRate);

    mHRUpdateInterval = (uint32_t) ((float) HEART_RATE_UPDATE_INTERVAL_SAMPLES)/((float) params.ecgSamplesPerFrame) - 1;
    if (mHRUpdateInterval < 2)
        mHRUpdateInterval = 2;

    LOGD("ECG Heart Rate Update Interval: %d", mHRUpdateInterval);

    // DA/ECG single Frame Related Parameters
    mDARenderer.setSingleFrameFrameWidth(params.daSingleFrameWidth);
    mDARenderer.setSingleFrameTraceIndex(params.daTraceIndex);

    mECGRenderer.setSingleFrameFrameWidth(params.ecgSingleFrameWidth);
    mECGRenderer.setSingleFrameTraceIndex(params.ecgTraceIndex);

    // TODO: BUGBUG - Text Color, LineThickness
    mDARenderer.setLineColor(mDALineColor);
    mECGRenderer.setLineColor(mECGLineColor);

    mDARenderer.setLineThickness(DA_ECG_LINE_THICKNESS);
    mECGRenderer.setLineThickness(DA_ECG_LINE_THICKNESS);
}

//-----------------------------------------------------------------------------
void CineViewer::setDaEcgScrollSpeedIndex(int scrollSpeedIndex)
{
    ALOGD("%s", __func__);

    // update CineBuffer scrollSpeedIndex
    mCineBuffer.getParams().daEcgScrollSpeedIndex = scrollSpeedIndex;
}


//-----------------------------------------------------------------------------
void CineViewer::handleOnScroll(float x, float y)
{
    switch (mImagingMode)
    {
        case IMAGING_MODE_B:
            mBModeRenderer.setPanDistance(x, y);
            break;

        case IMAGING_MODE_COLOR:
            mCModeRenderer.setPanDistance(x, y);
            break;

        default:
            break;
    }
}

//-----------------------------------------------------------------------------
void CineViewer::handleOnScale(float scaleFactor)
{
    switch (mImagingMode)
    {
        case IMAGING_MODE_B:
            mBModeRenderer.setScaleFactor(scaleFactor);
            break;

        case IMAGING_MODE_COLOR:
            mCModeRenderer.setScaleFactor(scaleFactor);
            break;

        default:
            break;
    }
}

//-----------------------------------------------------------------------------
void CineViewer::queryDisplayDepth(float& startDepth, float& endDepth)
{
    ALOGD("%s", __func__);

    switch (mImagingMode)
    {
        case IMAGING_MODE_B:
            mBModeRenderer.getDisplayDepth(startDepth, endDepth);
            break;

        case IMAGING_MODE_M:
            mBModeRenderer.getDisplayDepth(startDepth, endDepth);
            break;

        case IMAGING_MODE_COLOR:
            mCModeRenderer.getDisplayDepth(startDepth, endDepth);
            break;

        case IMAGING_MODE_PW:
        case IMAGING_MODE_CW:
            if (mCineBuffer.getParams().renderedTransitionImgMode == IMAGING_MODE_COLOR)
                mCModeRenderer.getDisplayDepth(startDepth, endDepth);
            else
                mBModeRenderer.getDisplayDepth(startDepth, endDepth);
            break;

        default:
            break;
    }
}

//-----------------------------------------------------------------------------
void CineViewer::handleOnTouch(float x, float y, bool isDown)
{
    LOGD("native onTouch(%f, %f, %s)", x, y, isDown ? "down":"up");
    mImguiRenderer.setTouch(x, y, isDown);
}

//-----------------------------------------------------------------------------
float CineViewer::getTraceLineTime()
{
    float retVal = 0.0f;

    if ( (IMAGING_MODE_M == mImagingMode) ||  (IMAGING_MODE_PW == mImagingMode)
         ||  (IMAGING_MODE_CW == mImagingMode))
    {
        if (mCineBuffer.getParams().scrollSpeed > 0.0f)
            retVal = mCineBuffer.getParams().traceIndex / mCineBuffer.getParams().scrollSpeed;
    }

    return(retVal);
}

//-----------------------------------------------------------------------------
float CineViewer::getDopplerPeakMax()
{
    float yExpScale = 1.0f;

    if ( (mCineBuffer.getParams().imagingMode == IMAGING_MODE_CW) ||
         (mCineBuffer.getParams().renderedTransitionImgMode == IMAGING_MODE_CW) )
        yExpScale = 2.0f;

    return (mCineBuffer.getParams().dopplerPeakMax *
            mCineBuffer.getParams().dopplerVelocityScale * yExpScale);
}

//-----------------------------------------------------------------------------
void CineViewer::getDopplerPeakMaxCoordinate(float* mapMat, int arrayLength, bool isTracelineInvert)
{
    ScrollModeRecordParams smParams;
    float    yExpScale = 1.0f;

    // update buffer if needed
    if (!mSModeRenderer.getLastDrawnWholeScreen())
        mSModeRenderer.prepareFreezeMode(-1, false);

    mSModeRenderer.getRecordParams(smParams);

    if ( (mCineBuffer.getParams().imagingMode == IMAGING_MODE_CW) ||
         (mCineBuffer.getParams().renderedTransitionImgMode == IMAGING_MODE_CW) )
        yExpScale = 2.0f;

    // freezeMode - process All Frame
    if ((mCineBuffer.getCurFrameNum() != 0))
    {
        CineBuffer::CineFrameHeader* dpmScrHeader =
                (CineBuffer::CineFrameHeader*) mCineBuffer.getFrame(0, DAU_DATA_TYPE_DPMAX_SCR,
                             CineBuffer::FrameContents::IncludeHeader);

        if (mCineBuffer.getCurFrameNum() !=  dpmScrHeader->frameNum)
        {
            // process peak/mean frame - singleFrameTexture
            DopplerPeakMeanProcessHandler *dopplerPeakMeanProc = new DopplerPeakMeanProcessHandler(
                    mCineBuffer.getParams().upsReader);
            dopplerPeakMeanProc->setCineBuffer(&mCineBuffer);
            dopplerPeakMeanProc->init();

            int outDataType;
            float refGaindB, peakThreshold, defaultGaindB;
            float gainOffset, gainRange;

            outDataType = DAU_DATA_TYPE_DPMAX_SCR;

            if (mCineBuffer.getParams().imagingMode == IMAGING_MODE_PW) {
                mCineBuffer.getParams().upsReader->getPWPeakThresholdParams(
                        mCineBuffer.getParams().organId, refGaindB, peakThreshold);
                dopplerPeakMeanProc->setDataType(DAU_DATA_TYPE_PW, outDataType);
                // Default PW gain
                mCineBuffer.getParams().upsReader->getPWGainParams(mCineBuffer.getParams().organId,
                                                gainOffset, gainRange, mCineBuffer.getParams().isPWTDI);
                defaultGaindB = gainOffset + (gainRange * 0.5f);
            } else {
                mCineBuffer.getParams().upsReader->getCWPeakThresholdParams(
                        mCineBuffer.getParams().organId, refGaindB, peakThreshold);
                dopplerPeakMeanProc->setDataType(DAU_DATA_TYPE_CW, outDataType);
                // Default CW gain
                mCineBuffer.getParams().upsReader->getCWGainParams(mCineBuffer.getParams().organId,
                                                gainOffset, gainRange);
                defaultGaindB = gainOffset + (gainRange * 0.5f);
            }
            dopplerPeakMeanProc->setProcessIndices(mCineBuffer.getParams().dopplerFFTSize,
                                                   defaultGaindB,
                                                   peakThreshold);
            dopplerPeakMeanProc->setWFGain(mCineBuffer.getParams().dopplerWFGain);

            // process all frames in the buffer
            dopplerPeakMeanProc->processAllFrames(true);

            // clean up
            dopplerPeakMeanProc->terminate();
            if (nullptr != dopplerPeakMeanProc) {
                delete dopplerPeakMeanProc;
            }
        }

    }

    // Doppler Peak Estimator
    DopplerPeakEstimator* dopplerPeakEstimator = new DopplerPeakEstimator(&mCineBuffer, &mAIManager);
    dopplerPeakEstimator->setParams(smParams,
                                    mSModeRenderer.getWidth(),
                                    mSModeRenderer.getHeight(),
                                    mSModeRenderer.getYShiftFloat(),
                                    yExpScale,
                                    isTracelineInvert);

    if ( (mCineBuffer.getParams().probeType & DEV_PROBE_MASK) == PROBE_TYPE_LINEAR ) {
        // TODO: running older algorithm for Linear (until it's trained for Linear)
        dopplerPeakEstimator->estimatePeaks(mapMat, arrayLength, mImagingMode);
    }
    else {
        // TODO: update classId
        // classId = 100 -- unknown
        dopplerPeakEstimator->estimatePeaks(mapMat, arrayLength, mImagingMode, 100);
    }

    // clean up
    if (dopplerPeakEstimator != nullptr)
        delete dopplerPeakEstimator;
}

//-----------------------------------------------------------------------------
int CineViewer::setTimeAvgCoOrdinates(float* peakMaxPositive, float* peakMeanPositive,
                                      float* peakMaxNegative, float* peakMeanNegative) {
    // sizeofTimeAvg -> size of point pairs for each output.  point pair -> x and y coordinate.
    // X -> screen location, Y -> normalized coordinate.

    // define sizeOfTimeAvg;
    int sizeOfTimeAvg = 128;

    ScrollModeRecordParams smParams;
    float    yExpScale = 1.0f;

    // update buffer if needed
    if (!mSModeRenderer.getLastDrawnWholeScreen())
        mSModeRenderer.prepareFreezeMode(-1, false);

    mSModeRenderer.getRecordParams(smParams);

    if ( (mCineBuffer.getParams().imagingMode == IMAGING_MODE_CW) ||
         (mCineBuffer.getParams().renderedTransitionImgMode == IMAGING_MODE_CW) )
        yExpScale = 2.0f;

    // freezeMode - process All Frame
    if ((mCineBuffer.getCurFrameNum() != 0))
    {
        CineBuffer::CineFrameHeader* dpmScrHeader =
                (CineBuffer::CineFrameHeader*) mCineBuffer.getFrame(0, DAU_DATA_TYPE_DPMAX_SCR,
                                                                    CineBuffer::FrameContents::IncludeHeader);

        if (mCineBuffer.getCurFrameNum() !=  dpmScrHeader->frameNum)
        {
            // process peak/mean frame - singleFrameTexture
            DopplerPeakMeanProcessHandler *dopplerPeakMeanProc = new DopplerPeakMeanProcessHandler(
                    mCineBuffer.getParams().upsReader);
            dopplerPeakMeanProc->setCineBuffer(&mCineBuffer);
            dopplerPeakMeanProc->init();

            int outDataType;
            float refGaindB, peakThreshold, defaultGaindB;
            float gainOffset, gainRange;

            outDataType = DAU_DATA_TYPE_DPMAX_SCR;

            if (mCineBuffer.getParams().imagingMode == IMAGING_MODE_PW) {
                mCineBuffer.getParams().upsReader->getPWPeakThresholdParams(
                        mCineBuffer.getParams().organId, refGaindB, peakThreshold);
                dopplerPeakMeanProc->setDataType(DAU_DATA_TYPE_PW, outDataType);
                // Default PW gain
                mCineBuffer.getParams().upsReader->getPWGainParams(mCineBuffer.getParams().organId,
                                                gainOffset, gainRange, mCineBuffer.getParams().isPWTDI);
                defaultGaindB = gainOffset + (gainRange * 0.5f);
            } else {
                mCineBuffer.getParams().upsReader->getCWPeakThresholdParams(
                        mCineBuffer.getParams().organId, refGaindB, peakThreshold);
                dopplerPeakMeanProc->setDataType(DAU_DATA_TYPE_CW, outDataType);
                // Default CW gain
                mCineBuffer.getParams().upsReader->getCWGainParams(mCineBuffer.getParams().organId,
                                                gainOffset, gainRange);
                defaultGaindB = gainOffset + (gainRange * 0.5f);
            }

            dopplerPeakMeanProc->setProcessIndices(mCineBuffer.getParams().dopplerFFTSize,
                                                   defaultGaindB,
                                                   peakThreshold);
            dopplerPeakMeanProc->setWFGain(mCineBuffer.getParams().dopplerWFGain);

            // process all frames in the buffer
            dopplerPeakMeanProc->processAllFrames(true);

            // clean up
            dopplerPeakMeanProc->terminate();
            if (nullptr != dopplerPeakMeanProc) {
                delete dopplerPeakMeanProc;
            }
        }
    }

    float *inputPtrMaxFloat, *inputPtrMeanFloat;
    int imgWidth = mSModeRenderer.getWidth();
    int imgHeight = mSModeRenderer.getHeight();
    mSModeRenderer.getRecordParams(smParams);
    float curMaxSamplePos, curMaxSampleNeg;
    float curMeanSamplePos, curMeanSampleNeg;
    float yScaleAdj;
    float curLocOrg, curSampleLocFloat;
    int   curSampleLoc;
    int   startX, endX;

    // limit the size of Time Avg
    if (sizeOfTimeAvg > imgWidth)
        sizeOfTimeAvg = imgWidth;

    startX = 0;                     // coordinate in pixel space
    endX = imgWidth - 1;

    int startXCoord = (int) ((startX / ((float) imgWidth)) * ((float) smParams.scrDataWidth));
    int endXCoord = (int) ((endX / ((float) imgWidth)) * ((float) smParams.scrDataWidth));

    float sampleStepOrg = (endX - startX) / ((float) (sizeOfTimeAvg - 1));
    float sampleStep = (endXCoord - startXCoord) / ((float) (sizeOfTimeAvg - 1));

    int ptsCnt = 0;

    // inputPtr
    inputPtrMaxFloat = (float*) mCineBuffer.getFrame(0, DAU_DATA_TYPE_DPMAX_SCR);
    inputPtrMeanFloat = (float*) mCineBuffer.getFrame(0, DAU_DATA_TYPE_DPMEAN_SCR);

    curSampleLoc = startXCoord;
    curSampleLocFloat = (float) startXCoord;
    curLocOrg = startX;

    // Y-Scale Adjustment
    yScaleAdj = (imgHeight / 2.0f) * yExpScale;

    // nearest neighbor sampling due to original sample is over sampled.
    while ((ptsCnt < sizeOfTimeAvg))
    {
        curMaxSamplePos = *(inputPtrMaxFloat + 2 * curSampleLoc) * yScaleAdj;
        curMaxSampleNeg = *(inputPtrMaxFloat + 2 * curSampleLoc + 1) * yScaleAdj;

        curMeanSamplePos = *(inputPtrMeanFloat + 2 * curSampleLoc) * yScaleAdj;
        curMeanSampleNeg = *(inputPtrMeanFloat + 2 * curSampleLoc + 1) * yScaleAdj;

        *peakMaxPositive++ = curLocOrg;
        *peakMaxPositive++ = curMaxSamplePos;

        *peakMaxNegative++ = curLocOrg;
        *peakMaxNegative++ = curMaxSampleNeg;

        *peakMeanPositive++ = curLocOrg;
        *peakMeanPositive++ = curMeanSamplePos;

        *peakMeanNegative++ = curLocOrg;
        *peakMeanNegative++ = curMeanSampleNeg;

        // sampleLocation in data coordinate
        curSampleLocFloat += sampleStep;
        curSampleLoc = floor(curSampleLocFloat);

        // sampleLocation in display coordinate
        curLocOrg += sampleStepOrg;

        ptsCnt++;
    }

    return sizeOfTimeAvg;
}

//-----------------------------------------------------------------------------
uint32_t CineViewer::getFrameIntervalMs()
{
    return mCineBuffer.getParams().frameInterval;
}

//-----------------------------------------------------------------------------
uint32_t CineViewer::getMLinesPerFrame()
{
    return mCineBuffer.getParams().mlinesPerFrame;
}

//-----------------------------------------------------------------------------
void CineViewer::setTraceLineTime(float traceLineTime)
{
    // TODO: REMOVE
    switch (mImagingMode)
    {
        case IMAGING_MODE_M:
            //mMModeRenderer.setStillMLineTime(traceLineTime);
            break;

        default:
            break;
    }
}

//-----------------------------------------------------------------------------
bool CineViewer::getScrollModeRecordParams(ScrollModeRecordParams& smParams, uint32_t frameNum)
{
    bool retVal = false;

    if ( (mImagingMode == IMAGING_MODE_M) || (mImagingMode == IMAGING_MODE_PW) ||
         (mImagingMode == IMAGING_MODE_CW) )
    {
        // process frame to Tex Memory
        mSModeRenderer.prepareFreezeMode(frameNum, false);
        // get Record params
        mSModeRenderer.getRecordParams(smParams);

        retVal = true;
    }

    return(retVal);
}

//-----------------------------------------------------------------------------
bool CineViewer::getDARecordParams(DaEcgRecordParams &daParams, uint32_t frameNum)
{
    bool retVal = false;
    if (mIsDAOn)
    {
        mDARenderer.prepareFreezeMode(frameNum, false);

        daParams.scrDataWidth = mDARenderer.getInputDataWidth();
        daParams.samplesPerFrame = mDARenderer.getSamplesPerFrame();
        daParams.samplesPerSecond = mDARenderer.getSamplesPerSecond();
        daParams.scrollSpeedIndex = mCineBuffer.getParams().daEcgScrollSpeedIndex;
        daParams.decimationRatio = mDARenderer.getDecimRatio();
        daParams.location = 0;
        daParams.traceIndex = mDARenderer.getSingleFrameTraceIndex();
        daParams.frameWidth = mDARenderer.getSingleFrameFrameWidth();
        retVal = true;
    }

    return(retVal);
}

//-----------------------------------------------------------------------------
bool CineViewer::getECGRecordParams(DaEcgRecordParams &ecgParams, uint32_t frameNum)
{
    bool retVal = false;
    if (mIsECGOn)
    {
        mECGRenderer.prepareFreezeMode(frameNum, false);

        ecgParams.scrDataWidth = mECGRenderer.getInputDataWidth();
        ecgParams.samplesPerFrame = mECGRenderer.getSamplesPerFrame();
        ecgParams.samplesPerSecond = mECGRenderer.getSamplesPerSecond();
        ecgParams.scrollSpeedIndex = mCineBuffer.getParams().daEcgScrollSpeedIndex;
        ecgParams.decimationRatio = mECGRenderer.getDecimRatio();
        ecgParams.location = 0;
        ecgParams.traceIndex = mECGRenderer.getSingleFrameTraceIndex();
        ecgParams.frameWidth = mECGRenderer.getSingleFrameFrameWidth();
        retVal = true;
    }

    return(retVal);
}

//-----------------------------------------------------------------------------
void CineViewer::updateDopplerBaselineShiftInvert(uint32_t baselineShiftIndex, bool isInvert)
{
    ALOGD("%s", __func__);

    bool pwCwTransitionRendered = (mCineBuffer.getParams().renderedTransitionImgMode == IMAGING_MODE_PW)
                                  || (mCineBuffer.getParams().renderedTransitionImgMode == IMAGING_MODE_CW);

    if ( (mImagingMode == IMAGING_MODE_PW) || (mImagingMode == IMAGING_MODE_CW) || pwCwTransitionRendered)
    {
        CineBuffer::CineParams params;
        params = mCineBuffer.getParams();
        float baselineShiftFracton;

        params.dopplerBaselineShiftIndex = baselineShiftIndex;
        params.dopplerBaselineInvert = isInvert;

        if ((mImagingMode == IMAGING_MODE_PW) || (mCineBuffer.getParams().renderedTransitionImgMode == IMAGING_MODE_PW)) {
            params.upsReader->getPwBaselineShiftFraction(params.dopplerBaselineShiftIndex, baselineShiftFracton);
        }
        else {
            params.upsReader->getCwBaselineShiftFraction(params.dopplerBaselineShiftIndex, baselineShiftFracton);
        }
        mSModeRenderer.updateBaselineShiftInvert(baselineShiftFracton, isInvert);

        // update CineParams
        mCineBuffer.setCineParamsOnly(params);

        // transition frame rendererd
        if (pwCwTransitionRendered)
            mSModeRenderer.setFrameNum(0);
        else
            prepareDopplerTransition(true);
    }
}

//-----------------------------------------------------------------------------
void CineViewer::onData(uint32_t frameNum, uint32_t dauDataTypes)
{
    uint64_t    data = dauDataTypes;

    data <<= 32;
    data |= frameNum;

    mDataEvent.set(data);
}

//-----------------------------------------------------------------------------
void* CineViewer::workerThreadEntry(void* thisPtr)
{
    ((CineViewer*) thisPtr)->threadLoop();

    return(nullptr);
}

//-----------------------------------------------------------------------------
void CineViewer::queryPhysicalToPixelMap(float* mapMat, int arrayLength)
{
    // Commenting this log entry for now because the AI auto-labeling is calling
    // this on every frame.  It is creating too much noise in the log and could
    // hurt performance.  Can re-enable later if changes can safely be made to
    // only call this when needed.
    //ALOGD("%s", __func__);

    switch (mImagingMode)
    {
        case IMAGING_MODE_B:
            mBModeRenderer.getPhysicalToPixelMap(mapMat);
            if (arrayLength > 26) {
                // adding Da and Ecg Transformation Matrices
                mDARenderer.getPhysicalToPixelMap(mapMat + 9);
                mECGRenderer.getPhysicalToPixelMap(mapMat + 18);
            }
            break;

        case IMAGING_MODE_COLOR:
            mCModeRenderer.getPhysicalToPixelMap(mapMat);
            if (arrayLength > 26) {
                // adding Da and Ecg Transformation Matrices
                mDARenderer.getPhysicalToPixelMap(mapMat + 9);
                mECGRenderer.getPhysicalToPixelMap(mapMat + 18);
            }
            break;

        case IMAGING_MODE_M:
            mBModeRenderer.getPhysicalToPixelMap(mapMat);
            // THe following length check is to make sure that this call is made
            // with mapMatrix of correct size
            // For M-Mode expected length is 18.
            if (arrayLength > 17)  // 9 for B Mode (3x3 transformation matrix)
                                   // 9 for M Mode
            {
                mSModeRenderer.getPhysicalToPixelMap(mapMat + 9);
            }
            // TODO: Add DA/ECG matrix
            break;

        case IMAGING_MODE_PW:
        case IMAGING_MODE_CW:
            if (mTransitionImgMode == IMAGING_MODE_B)
                mBModeRenderer.getPhysicalToPixelMap(mapMat);
            else if (mTransitionImgMode == IMAGING_MODE_COLOR)
                mCModeRenderer.getPhysicalToPixelMap(mapMat);

            // THe following length check is to make sure that this call is made
            // with mapMatrix of correct size
            // For PW/CW Mode expected length is 18.
            // 9 for B/C Mode (3x3 transformation matrix)
            // 9 for PW/C Mode
            if(arrayLength > 17) {
                mSModeRenderer.getPhysicalToPixelMap(mapMat + 9);
            }
            // TODO: Add DA/ECG matrix
            break;

        default:
            break;
    }


}

//-----------------------------------------------------------------------------
void CineViewer::queryZoomPanFlipParams(float* zoomPanArray)
{
    ALOGD("%s", __func__);
    mLock.enter();

    float scale, deltaX, deltaY, flipX, flipY;
    // default values
    scale = 1.0f;
    deltaX = 0.0f;
    deltaY = 0.0f;
    flipX = 1.0f;
    flipY = 1.0f;

    switch (mImagingMode)
    {
        case IMAGING_MODE_B:
        case IMAGING_MODE_M:
            mBModeRenderer.getScale(scale);
            mBModeRenderer.getPan(deltaX, deltaY);
            mBModeRenderer.getFlip(flipX, flipY);
            break;

        case IMAGING_MODE_COLOR:
            mCModeRenderer.getScale(scale);
            mCModeRenderer.getPan(deltaX, deltaY);
            mCModeRenderer.getFlip(flipX, flipY);
            break;

        case IMAGING_MODE_PW:
        case IMAGING_MODE_CW:
            if (mTransitionImgMode == IMAGING_MODE_B)
            {
                mBModeRenderer.getScale(scale);
                mBModeRenderer.getPan(deltaX, deltaY);
                mBModeRenderer.getFlip(flipX, flipY);
            }
            else if (mTransitionImgMode == IMAGING_MODE_COLOR)
            {
                mCModeRenderer.getScale(scale);
                mCModeRenderer.getPan(deltaX, deltaY);
                mCModeRenderer.getFlip(flipX, flipY);
            }
            break;

        default:
            break;
    }

    zoomPanArray[0] = scale;
    zoomPanArray[1] = deltaX;
    zoomPanArray[2] = deltaY;
    zoomPanArray[3] = flipX;
    zoomPanArray[4] = flipY;

    mLock.leave();
}

//-----------------------------------------------------------------------------
void CineViewer::setZoomPanFlipParams(float *zoomPanArray)
{
    ALOGD("%s", __func__);
    mLock.enter();

    float scale, deltaX, deltaY, flipX, flipY;
    // default values
    scale = zoomPanArray[0];
    deltaX = zoomPanArray[1];
    deltaY = zoomPanArray[2];
    flipX = zoomPanArray[3];
    flipY = zoomPanArray[4];

    switch (mImagingMode)
    {
        case IMAGING_MODE_B:
        case IMAGING_MODE_M:
            mBModeRenderer.setScale(scale);
            mBModeRenderer.setPan(deltaX, deltaY);
            mBModeRenderer.setFlip(flipX, flipY);
            break;

        case IMAGING_MODE_COLOR:
            mCModeRenderer.setScale(scale);
            mCModeRenderer.setPan(deltaX, deltaY);
            mCModeRenderer.setFlip(flipX, flipY);
            break;

        default:
            break;
    }

    // to put the pan params in range
    handleOnScroll(0.0f, 0.0f);

    mLock.leave();

    ALOGI("THSW-755 :: %s,Native :: setZoomPanFlipParams %0.1f,%0.1f,%0.1f,%0.1f,%0.1f", __func__,zoomPanArray[0],zoomPanArray[1],zoomPanArray[2],zoomPanArray[3],zoomPanArray[4]);

    // pass flipX and flipY to AI sub-system
    mAIManager.setFlip(flipX, flipY);
}

//-----------------------------------------------------------------------------
ThorStatus CineViewer::setMmodeLayout(float bmodeLayout[], float mmodeLayout[])
{
    ALOGD("%s", __func__);

    mBModeLayout[0] = bmodeLayout[0];
    mBModeLayout[1] = 100.0f - (bmodeLayout[1] + bmodeLayout[3]);
    mBModeLayout[2] = bmodeLayout[2];
    mBModeLayout[3] = bmodeLayout[3];
    mMModeLayout[0] = mmodeLayout[0];
    mMModeLayout[1] = 100.0f - (mmodeLayout[1] + mmodeLayout[3]);
    mMModeLayout[2] = mmodeLayout[2];
    mMModeLayout[3] = mmodeLayout[3];

    ALOGD("B-Mode Window: Origin = (%f, %f) Width = %f, Height = %f",
          mBModeLayout[0], mBModeLayout[1], mBModeLayout[2], mBModeLayout[3]);
    ALOGD("M-Mode Window: Origin = (%f, %f) Width = %f, Height = %f",
          mMModeLayout[0], mMModeLayout[1], mMModeLayout[2], mMModeLayout[3]);
    return (THOR_OK);
}

//-----------------------------------------------------------------------------
ThorStatus CineViewer::setupUSDAECG(bool isUSOn,bool isDAOn, bool isECGOn) {
    ALOGD("%s", __func__);

    ThorStatus retVal = THOR_OK;

    mIsDAOn = isDAOn;
    mIsECGOn = isECGOn;
    mIsUSOn=isUSOn;

    return (retVal);
}

//-----------------------------------------------------------------------------
ThorStatus CineViewer::setDAECGLayout(float *usLayout, float *daLayout, float *ecgLayout) {
    ALOGD("%s", __func__);

    mUSLayout[0] = usLayout[0];
    mUSLayout[1] = 100.0f - (usLayout[1] + usLayout[3]);
    mUSLayout[2] = usLayout[2];
    mUSLayout[3] = usLayout[3];
    mDALayout[0] = daLayout[0];
    mDALayout[1] = 100.0f - (daLayout[1] + daLayout[3]);
    mDALayout[2] = daLayout[2];
    mDALayout[3] = daLayout[3];
    mECGLayout[0] = ecgLayout[0];
    mECGLayout[1] = 100.0f - (ecgLayout[1] + ecgLayout[3]);
    mECGLayout[2] = ecgLayout[2];
    mECGLayout[3] = ecgLayout[3];

    ALOGD("US Window: Origin = (%f, %f) Width = %f, Height = %f",
          mUSLayout[0], mUSLayout[1], mUSLayout[2], mUSLayout[3]);
    ALOGD("DA Window: Origin = (%f, %f) Width = %f, Height = %f",
          mDALayout[0], mDALayout[1], mDALayout[2], mDALayout[3]);
    ALOGD("ECG Window: Origin = (%f, %f) Width = %f, Height = %f",
          mECGLayout[0], mECGLayout[1], mECGLayout[2], mECGLayout[3]);

    return (THOR_OK);
}

//-----------------------------------------------------------------------------
ThorStatus CineViewer::setDopplerLayout(float *dopplerLayout) {
    ALOGD("%s", __func__);

    mDOPLayout[0] = dopplerLayout[0];
    mDOPLayout[1] = 100.0f - (dopplerLayout[1] + dopplerLayout[3]);
    mDOPLayout[2] = dopplerLayout[2];
    mDOPLayout[3] = dopplerLayout[3];

    ALOGD("Doppler Window: Origin = (%f, %f) Width = %f, Height = %f",
          mDOPLayout[0], mDOPLayout[1], mDOPLayout[2], mDOPLayout[3]);

    return (THOR_OK);
}

//-----------------------------------------------------------------------------
ThorStatus CineViewer::setColorMap(int colorMapId, bool isInverted)
{
    ThorStatus      retVal = THOR_ERROR;
    uint32_t        colorMap[256];
    int             ret;

    ret = mCineBuffer.getParams().upsReader->getColorMap(colorMapId, isInverted, colorMap);

    // TODO: update when 2D ColorMap is implemented.
    if (ret < 0)
    {
        ALOGE("setColorMap: error occurred in getting color map.");
        goto err_ret;
    }

    mCModeRenderer.setColorMap((uint8_t *) colorMap);

    mCineBuffer.getParams().colorMapIndex = colorMapId;
    mCineBuffer.getParams().isColorMapInverted = isInverted;

    retVal = THOR_OK;

err_ret:
    return (retVal);
}

//-----------------------------------------------------------------------------
void CineViewer::getUltrasoundScreenImage(uint8_t* pixels, int imgWidth, int imgHeight,
                                          bool isForThumbnail, int desiredFrameNumber)
{
    ALOGD("%s", __func__);

    if (!mIsInitialized)
    {
        return;
    }

    OffScreenRenderSurface  stillSurface;
    BModeRenderer           bModeStillRenderer;
    CModeRenderer           cModeStillRenderer;
    ScrollModeRenderer      scrollModeStillRenderer;
    WaveformRenderer        daRenderer;
    WaveformRenderer        ecgRenderer;
    int                     frameNumber;

    ScanConverterParams     scParams;
    ScanConverterParams     colorScParams;
    float zoomPanParams[5] = {1.0f, 0.0f, 0.0f, 1.0f, 1.0f};

    frameNumber = desiredFrameNumber;
    CineBuffer::CineParams cineParams = mCineBuffer.getParams();

    // adjust frameNumber
    if (frameNumber > mCineBuffer.getMaxFrameNum() || frameNumber < mCineBuffer.getMinFrameNum())
    {
        frameNumber = mCineBuffer.getCurFrameNum();
    }

    // get zoom, pan, flip params
    queryZoomPanFlipParams(zoomPanParams);

    // if it is thumbnail, prepare DA/ECG drawing
    if (isForThumbnail)
    {
        if (mIsDAOn)
        {
            daRenderer.setParams(mDARenderer.getInputDataWidth(),
                                 mDARenderer.getSamplesPerSecond(),
                                 30,
                                 mDARenderer.getSamplesPerFrame(),
                                 30.00f,
                                 5.454f);

            stillSurface.addRenderer(&daRenderer, mDALayout[0], mDALayout[1], mDALayout[2], mDALayout[3]);
        }
        if (mIsECGOn)
        {
            ecgRenderer.setParams(mECGRenderer.getInputDataWidth(),
                                  mECGRenderer.getSamplesPerSecond(),
                                  30,
                                  mECGRenderer.getSamplesPerFrame(),
                                  30.00f,
                                  5.454f);

            stillSurface.addRenderer(&ecgRenderer, mECGLayout[0], mECGLayout[1], mECGLayout[2], mECGLayout[3]);
        }
    }

    // update PW/CW texture buffer, if not updated.
    if ( ((mImagingMode == IMAGING_MODE_PW)||(mImagingMode == IMAGING_MODE_CW) || (mImagingMode == IMAGING_MODE_M) )
            && (!mIsLive) && (!mSModeRenderer.getLastDrawnWholeScreen()) )
        mSModeRenderer.prepareFreezeMode(-1, false);

    // Transition Frames
    if (cineParams.isTransitionRenderingReady)
    {
        int transitionImgMode = cineParams.renderedTransitionImgMode;
        float baselineShiftFracton;
        float desiredTimeSpan;
        uint32_t organGlobalId = cineParams.upsReader->getOrganGlobalId(cineParams.organId);


        switch (transitionImgMode)
        {
            case IMAGING_MODE_B:
                bModeStillRenderer.setParams(cineParams.converterParams);
                stillSurface.addRenderer(&bModeStillRenderer, mUSLayout[0], mUSLayout[1], mUSLayout[2],
                                         mUSLayout[3]);
                break;

            case IMAGING_MODE_COLOR:
                uint32_t                    velocityMap[256];   // empty CMap - will be updated
                cModeStillRenderer.setParams(cineParams.converterParams, cineParams.colorConverterParams,
                                             cineParams.colorBThreshold, cineParams.colorVelThreshold, (uint8_t*) velocityMap, cineParams.colorDopplerMode);
                stillSurface.addRenderer(&cModeStillRenderer, mUSLayout[0], mUSLayout[1], mUSLayout[2],
                                         mUSLayout[3]);
                break;

            case IMAGING_MODE_PW:
                // set Params
                cineParams.upsReader->getPwBaselineShiftFraction(cineParams.dopplerBaselineShiftIndex, baselineShiftFracton);
                desiredTimeSpan = getTimeSpan(cineParams.dopplerSweepSpeedIndex, organGlobalId == TCD_GLOBAL_ID);
                scrollModeStillRenderer.setParams(cineParams.mSingleFrameWidth, cineParams.dopplerFFTSize,
                                              cineParams.scrollSpeed, baselineShiftFracton, cineParams.targetFrameRate, cineParams.dopplerNumLinesPerFrame,
                                              cineParams.frameRate, cineParams.dopplerSamplesPerFrame, desiredTimeSpan, cineParams.dopplerVelocityScale*2.0f);
                scrollModeStillRenderer.setInvert(cineParams.dopplerBaselineInvert);
                scrollModeStillRenderer.setPeakMeanDrawing(cineParams.dopplerPeakDraw, cineParams.dopplerMeanDraw);

                // Add renderer
                stillSurface.addRenderer(&scrollModeStillRenderer, mDOPLayout[0], mDOPLayout[1], mDOPLayout[2],
                                         mDOPLayout[3]);
                break;

            case IMAGING_MODE_CW:
                // set Params
                cineParams.upsReader->getPwBaselineShiftFraction(cineParams.dopplerBaselineShiftIndex, baselineShiftFracton);
                desiredTimeSpan = getTimeSpan(cineParams.dopplerSweepSpeedIndex, organGlobalId == TCD_GLOBAL_ID);
                scrollModeStillRenderer.setParams(cineParams.mSingleFrameWidth, cineParams.dopplerFFTSize,
                                              cineParams.scrollSpeed, baselineShiftFracton, cineParams.targetFrameRate, cineParams.dopplerNumLinesPerFrame,
                                              cineParams.frameRate, cineParams.dopplerSamplesPerFrame, desiredTimeSpan, cineParams.dopplerVelocityScale*2.0f, 2.0f);
                scrollModeStillRenderer.setInvert(cineParams.dopplerBaselineInvert);
                scrollModeStillRenderer.setPeakMeanDrawing(cineParams.dopplerPeakDraw, cineParams.dopplerMeanDraw);

                // Add renderer
                stillSurface.addRenderer(&scrollModeStillRenderer, mDOPLayout[0], mDOPLayout[1], mDOPLayout[2],
                                         mDOPLayout[3]);
                break;

            default:
                break;
        }
    }

    // get and set SC parameters
    switch (mImagingMode)
    {
        case IMAGING_MODE_B:
            mBModeRenderer.getParams(scParams);
            bModeStillRenderer.setParams(scParams);
            bModeStillRenderer.setTintAdjustment(mTintR, mTintG, mTintB);

            if (isForThumbnail)
            {
                stillSurface.addRenderer(&bModeStillRenderer, mUSLayout[0], mUSLayout[1], mUSLayout[2], mUSLayout[3]);
            }
            else
            {
                stillSurface.addRenderer(&bModeStillRenderer, 0.0f, 0.0f, 100.0f, 100.0f);
            }

            stillSurface.open(imgWidth, imgHeight);

            // set scale and pan parameters
            bModeStillRenderer.setScale(zoomPanParams[0]);
            bModeStillRenderer.setPan(zoomPanParams[1], zoomPanParams[2]);
            bModeStillRenderer.setFlip(zoomPanParams[3], zoomPanParams[4]);

            bModeStillRenderer.setFrame(mCineBuffer.getFrame(frameNumber, DAU_DATA_TYPE_B));
            break;

        case IMAGING_MODE_COLOR:
            int bThreshold;
            int velThreshold;
            uint32_t cDopplerMode;
            uint32_t velocityMap[256];

            mCModeRenderer.getParams(scParams,
                                     colorScParams,
                                     bThreshold,
                                     velThreshold,
                                     (uint8_t *) velocityMap,
                                     cDopplerMode);
            cModeStillRenderer.setParams(scParams,
                                         colorScParams,
                                         bThreshold,
                                         velThreshold,
                                         (uint8_t *) velocityMap,
                                         cDopplerMode);
            cModeStillRenderer.setTintAdjustment(mTintR, mTintG, mTintB);

            if (isForThumbnail)
            {
                stillSurface.addRenderer(&cModeStillRenderer, mUSLayout[0], mUSLayout[1], mUSLayout[2], mUSLayout[3]);
            }
            else
            {
                stillSurface.addRenderer(&cModeStillRenderer, 0.0f, 0.0f, 100.0f, 100.0f);
            }

            stillSurface.open(imgWidth, imgHeight);

            // set scale and pan parameters
            cModeStillRenderer.setScale(zoomPanParams[0]);
            cModeStillRenderer.setPan(zoomPanParams[1], zoomPanParams[2]);
            cModeStillRenderer.setFlip(zoomPanParams[3], zoomPanParams[4]);

            cModeStillRenderer.setFrame(mCineBuffer.getFrame(frameNumber, DAU_DATA_TYPE_B),
                                        mCineBuffer.getFrame(frameNumber, DAU_DATA_TYPE_COLOR));
            break;

        case IMAGING_MODE_M:
            mBModeRenderer.getParams(scParams);
            bModeStillRenderer.setParams(scParams);
            bModeStillRenderer.setTintAdjustment(mTintR, mTintG, mTintB);
            stillSurface.addRenderer(&bModeStillRenderer, mBModeLayout[0], mBModeLayout[1],
                                     mBModeLayout[2], mBModeLayout[3]);

            ScrollModeRecordParams mModeRecParams;
            mSModeRenderer.getRecordParams(mModeRecParams);

            // for single frame texture mode.  therefore, scroll params do not matter.
            scrollModeStillRenderer.setParams(mModeRecParams.scrDataWidth,
                                         mModeRecParams.samplesPerLine,
                                         mModeRecParams.scrollSpeed,
                                         mModeRecParams.baselineShift,
                                         30,
                                         mModeRecParams.numLinesPerFrame,
                                         30,
                                         mModeRecParams.orgNumLinesPerFrame,
                                         mModeRecParams.desiredTimeSpan,
                                         mModeRecParams.yPhysicalScale);
            scrollModeStillRenderer.setInvert(mModeRecParams.isInvert);
            stillSurface.addRenderer(&scrollModeStillRenderer, mMModeLayout[0], mMModeLayout[1],
                                     mMModeLayout[2], mMModeLayout[3]);
            scrollModeStillRenderer.setTintAdjustment(mTintR, mTintG, mTintB);

            stillSurface.open(imgWidth, imgHeight);

            // set scale and pan parameters
            bModeStillRenderer.setScale(zoomPanParams[0]);
            bModeStillRenderer.setPan(zoomPanParams[1], zoomPanParams[2]);
            bModeStillRenderer.setFlip(zoomPanParams[3], zoomPanParams[4]);

            bModeStillRenderer.setFrame(mCineBuffer.getFrame(frameNumber, DAU_DATA_TYPE_B));
            scrollModeStillRenderer.setSingleFrameTextureMode(true);
            scrollModeStillRenderer.setSingleFrameDrawIndex(mModeRecParams.traceIndex);
            scrollModeStillRenderer.prepareRenderer(&mCineBuffer, DAU_DATA_TYPE_M);
            scrollModeStillRenderer.setFrameNum(0);
            break;

        case IMAGING_MODE_PW:
            ScrollModeRecordParams pwModeRecParams;
            mSModeRenderer.getRecordParams(pwModeRecParams);

            // for single frame texture mode.  therefore, scroll params do not matter.
            scrollModeStillRenderer.setParams(pwModeRecParams.scrDataWidth,
                                          pwModeRecParams.samplesPerLine,
                                          pwModeRecParams.scrollSpeed,
                                          pwModeRecParams.baselineShift,
                                         30,
                                          pwModeRecParams.numLinesPerFrame,
                                         30,
                                          pwModeRecParams.orgNumLinesPerFrame,
                                          pwModeRecParams.desiredTimeSpan,
                                          pwModeRecParams.yPhysicalScale);
            scrollModeStillRenderer.setInvert(pwModeRecParams.isInvert);
            scrollModeStillRenderer.setPeakMeanDrawing(cineParams.dopplerPeakDraw, cineParams.dopplerMeanDraw);
            scrollModeStillRenderer.setTintAdjustment(mTintR, mTintG, mTintB);

            stillSurface.addRenderer(&scrollModeStillRenderer, mDOPLayout[0], mDOPLayout[1],
                                     mDOPLayout[2], mDOPLayout[3]);
            stillSurface.open(imgWidth, imgHeight);

            scrollModeStillRenderer.setSingleFrameTextureMode(true);
            scrollModeStillRenderer.setSingleFrameDrawIndex(pwModeRecParams.traceIndex);
            scrollModeStillRenderer.prepareRenderer(&mCineBuffer, DAU_DATA_TYPE_PW);
            scrollModeStillRenderer.setFrameNum(0);
            break;

        case IMAGING_MODE_CW:
            ScrollModeRecordParams cwModeRecParams;
            mSModeRenderer.getRecordParams(cwModeRecParams);

            // for single frame texture mode.  therefore, scroll params do not matter.
            scrollModeStillRenderer.setParams(cwModeRecParams.scrDataWidth,
                                          cwModeRecParams.samplesPerLine,
                                          cwModeRecParams.scrollSpeed,
                                          cwModeRecParams.baselineShift,
                                          30,
                                          cwModeRecParams.numLinesPerFrame,
                                          30,
                                          cwModeRecParams.orgNumLinesPerFrame,
                                          cwModeRecParams.desiredTimeSpan,
                                          cwModeRecParams.yPhysicalScale,
                                          2.0f);
            scrollModeStillRenderer.setInvert(cwModeRecParams.isInvert);
            scrollModeStillRenderer.setPeakMeanDrawing(cineParams.dopplerPeakDraw, cineParams.dopplerMeanDraw);
            scrollModeStillRenderer.setTintAdjustment(mTintR, mTintG, mTintB);

            stillSurface.addRenderer(&scrollModeStillRenderer, mDOPLayout[0], mDOPLayout[1],
                                     mDOPLayout[2], mDOPLayout[3]);
            stillSurface.open(imgWidth, imgHeight);

            scrollModeStillRenderer.setSingleFrameTextureMode(true);
            scrollModeStillRenderer.setSingleFrameDrawIndex(cwModeRecParams.traceIndex);
            scrollModeStillRenderer.prepareRenderer(&mCineBuffer, DAU_DATA_TYPE_CW);
            scrollModeStillRenderer.setFrameNum(0);
            break;

        default:
            break;
    }

    // update PW/CW texture and colorMap if transition frame is available.
    if (cineParams.isTransitionRenderingReady)
    {
        int transitionImgMode = cineParams.renderedTransitionImgMode;

        switch (transitionImgMode)
        {
            case IMAGING_MODE_B:
                //restore ZoomPanFlip
                bModeStillRenderer.setScale(cineParams.zoomPanParams[0]);
                bModeStillRenderer.setPan(cineParams.zoomPanParams[1], cineParams.zoomPanParams[2]);
                bModeStillRenderer.setFlip(cineParams.zoomPanParams[3], cineParams.zoomPanParams[4]);
                // set Frame
                bModeStillRenderer.setFrame(mCineBuffer.getFrame(TEX_FRAME_TYPE_B, DAU_DATA_TYPE_TEX));
                break;

            case IMAGING_MODE_COLOR:
                // Color map
                uint32_t                    velocityMap[256];
                cineParams.upsReader->getColorMap(cineParams.colorMapIndex,
                                                  cineParams.isColorMapInverted, velocityMap);
                cModeStillRenderer.setColorMap((uint8_t *)velocityMap);

                // restore ZoomPanFlip
                cModeStillRenderer.setScale(cineParams.zoomPanParams[0]);
                cModeStillRenderer.setPan(cineParams.zoomPanParams[1], cineParams.zoomPanParams[2]);
                cModeStillRenderer.setFlip(cineParams.zoomPanParams[3], cineParams.zoomPanParams[4]);
                // set Frame
                cModeStillRenderer.setFrame(mCineBuffer.getFrame(TEX_FRAME_TYPE_B, DAU_DATA_TYPE_TEX),
                                            mCineBuffer.getFrame(TEX_FRAME_TYPE_C, DAU_DATA_TYPE_TEX));
                break;

            case IMAGING_MODE_PW:
                scrollModeStillRenderer.setSingleFrameTextureMode(true);
                scrollModeStillRenderer.setSingleFrameDrawIndex(cineParams.traceIndex);
                scrollModeStillRenderer.prepareRenderer(&mCineBuffer, DAU_DATA_TYPE_PW);
                scrollModeStillRenderer.setFrameNum(0);
                break;

            case IMAGING_MODE_CW:
                scrollModeStillRenderer.setSingleFrameTextureMode(true);
                scrollModeStillRenderer.setSingleFrameDrawIndex(cineParams.traceIndex);
                scrollModeStillRenderer.prepareRenderer(&mCineBuffer, DAU_DATA_TYPE_CW);
                scrollModeStillRenderer.setFrameNum(0);
                break;

            default:
                break;
        }
    }

    // for DA/ECG thumbnail
    if (isForThumbnail)
    {
        if (desiredFrameNumber == -1)
        {
            // singleFrame FreezeMode
            if (mIsDAOn)
            {
                daRenderer.setSingleFrameFrameWidth(-1);
                daRenderer.setSingleFrameFreezeMode(true);
                daRenderer.setSingleFrameTraceIndex(mDARenderer.getSingleFrameTraceIndex());
                daRenderer.prepareRenderer(&mCineBuffer, DAU_DATA_TYPE_DA);

                daRenderer.setLineColor(mDALineColor);
                daRenderer.setLineThickness(DA_ECG_LINE_THICKNESS);

                daRenderer.setFrameNum(0);
            }
            if (mIsECGOn)
            {
                ecgRenderer.setSingleFrameFrameWidth(-1);
                ecgRenderer.setSingleFrameFreezeMode(true);
                ecgRenderer.setSingleFrameTraceIndex(mECGRenderer.getSingleFrameTraceIndex());
                ecgRenderer.prepareRenderer(&mCineBuffer, DAU_DATA_TYPE_ECG);

                ecgRenderer.setLineColor(mECGLineColor);
                ecgRenderer.setLineThickness(DA_ECG_LINE_THICKNESS);

                ecgRenderer.setFrameNum(0);
            }
        }
        else
        {
            // reconstruct from frames
            if (mIsDAOn)
            {
                daRenderer.setSingleFrameFreezeMode(false);
                daRenderer.prepareRenderer(&mCineBuffer, DAU_DATA_TYPE_DA);

                daRenderer.setLineColor(mDALineColor);
                daRenderer.setLineThickness(DA_ECG_LINE_THICKNESS);

                daRenderer.setFrameNum(desiredFrameNumber);
            }
            if (mIsECGOn)
            {
                ecgRenderer.setSingleFrameFreezeMode(false);
                ecgRenderer.prepareRenderer(&mCineBuffer, DAU_DATA_TYPE_ECG);

                ecgRenderer.setLineColor(mECGLineColor);
                ecgRenderer.setLineThickness(DA_ECG_LINE_THICKNESS);

                ecgRenderer.setFrameNum(desiredFrameNumber);
            }
        }
    }

    // take screenshot
    stillSurface.takeScreenShotToRGBAPtrUD(pixels);

    stillSurface.close();
    stillSurface.clearRenderList();

}

//-----------------------------------------------------------------------------
void CineViewer::setLive(bool isLive)
{
    mIsLive = isLive;
}

float CineViewer::getScale() {
    float scaleValue = 1.0f;
    switch (mImagingMode)
    {
        case IMAGING_MODE_B:
            mBModeRenderer.getScale(scaleValue);
            break;
        case IMAGING_MODE_COLOR:
            mCModeRenderer.getScale(scaleValue);
            break;
        default:
            break;
    }
    return scaleValue;
}

void CineViewer::setScale(float scaleFactor)
{
    switch (mImagingMode)
    {
        case IMAGING_MODE_B:
            mBModeRenderer.setPan(0.0F,0.0F);
            mBModeRenderer.setScale(scaleFactor);
            break;
        case IMAGING_MODE_COLOR:
            mCModeRenderer.setPan(0.0F,0.0F);
            mCModeRenderer.setScale(scaleFactor);
            break;
        default:
            break;
    }
}

void CineViewer::setPan(float deltaX, float deltaY)
{
    switch (mImagingMode)
    {
        case IMAGING_MODE_B:
            mBModeRenderer.setPan(deltaX,deltaY);
            break;
        case IMAGING_MODE_COLOR:
            mCModeRenderer.setPan(deltaY,deltaY);
            break;
        default:
            break;
    }

}

//-----------------------------------------------------------------------------
void CineViewer::setTintAdjustment(float coeffR, float coeffG, float coeffB)
{
    switch (mImagingMode)
    {
        case IMAGING_MODE_B:
            mBModeRenderer.setTintAdjustment(coeffR, coeffG, coeffB);
            break;
        case IMAGING_MODE_COLOR:
            mCModeRenderer.setTintAdjustment(coeffR, coeffG, coeffB);
            break;
        case IMAGING_MODE_M:
        case IMAGING_MODE_PW:
        case IMAGING_MODE_CW:
            mSModeRenderer.setTintAdjustment(coeffR, coeffG, coeffB);
            break;

        default:
            break;
    }
}

//-----------------------------------------------------------------------------
void CineViewer::setPeakMeanDrawing(bool peakDrawing, bool meanDrawing)
{
    bool pwCwTransitionRendered = (mCineBuffer.getParams().renderedTransitionImgMode == IMAGING_MODE_PW)
            || (mCineBuffer.getParams().renderedTransitionImgMode == IMAGING_MODE_CW);

    if (!mIsLive || pwCwTransitionRendered)
    {
        mSModeRenderer.setPeakMeanDrawing(peakDrawing, meanDrawing);

        // update CineBuffer
        mCineBuffer.getParams().dopplerPeakDraw = peakDrawing;
        mCineBuffer.getParams().dopplerMeanDraw = meanDrawing;
    }

    // when B/B+C live and PW/CW freeze in duplex
    if (pwCwTransitionRendered) {
        // process peak/mean frame - singleFrameTexture
        DopplerPeakMeanProcessHandler *dopplerPeakMeanProc = new DopplerPeakMeanProcessHandler(
                mCineBuffer.getParams().upsReader);
        dopplerPeakMeanProc->setCineBuffer(&mCineBuffer);
        dopplerPeakMeanProc->init();

        int outDataType;
        float refGaindB, peakThreshold, defaultGaindB;
        float gainOffset, gainRange;

        outDataType = DAU_DATA_TYPE_DPMAX_SCR;

        if (mCineBuffer.getParams().renderedTransitionImgMode == IMAGING_MODE_PW) {
            mCineBuffer.getParams().upsReader->getPWPeakThresholdParams(mCineBuffer.getParams().organId, refGaindB, peakThreshold);
            dopplerPeakMeanProc->setDataType(DAU_DATA_TYPE_PW, outDataType);
            // Default PW gain
            mCineBuffer.getParams().upsReader->getPWGainParams(mCineBuffer.getParams().organId,
                                            gainOffset, gainRange, mCineBuffer.getParams().isPWTDI);
            defaultGaindB = gainOffset + (gainRange * 0.5f);
        } else {
            mCineBuffer.getParams().upsReader->getCWPeakThresholdParams(mCineBuffer.getParams().organId, refGaindB, peakThreshold);
            dopplerPeakMeanProc->setDataType(DAU_DATA_TYPE_CW, outDataType);
            // Default CW gain
            mCineBuffer.getParams().upsReader->getCWGainParams(mCineBuffer.getParams().organId,
                                            gainOffset, gainRange);
            defaultGaindB = gainOffset + (gainRange * 0.5f);
        }
        dopplerPeakMeanProc->setProcessIndices(mCineBuffer.getParams().dopplerFFTSize, defaultGaindB,
                                               peakThreshold);
        dopplerPeakMeanProc->setWFGain(mCineBuffer.getParams().dopplerWFGain);

        // process all frames in the buffer
        dopplerPeakMeanProc->processAllFrames(true);

        // clean up
        dopplerPeakMeanProc->terminate();
        if (nullptr != dopplerPeakMeanProc) {
            delete dopplerPeakMeanProc;
        }

        // render the frame
        mSModeRenderer.setFrameNum(0);
    }
}

//-----------------------------------------------------------------------------
ThorStatus CineViewer::prepareDopplerTransition(bool prepareTransitionFrame)
{
    ALOGD("%s", __func__);

    mPrepareTransitionFrame = prepareTransitionFrame;

    return (THOR_OK);
}

//-----------------------------------------------------------------------------
void CineViewer::processDopplerTransition(uint32_t frameNum)
{
    ALOGD("%s", __func__);

    mPrepareTransitionFrame = false;

    // TODO: add US, DA, ECG flags (i.e., mIsUSOn) if necessary

    // update cineParams with the necessary rendering params
    CineBuffer::CineParams cineParams;
    cineParams = mCineBuffer.getParams();

    // store alternate imaging mode.
    if (cineParams.transitionImagingMode != mImagingMode)
        cineParams.transitionAltImagingMode = cineParams.transitionImagingMode;

    cineParams.transitionImagingMode = mImagingMode;

    // query zoom pan flip params (for B & C-mode)
    queryZoomPanFlipParams(cineParams.zoomPanParams);

    ScrollModeRecordParams scrollParams;

    switch (mImagingMode)
    {
        case IMAGING_MODE_B:
            // copy B-mode frame to tex buffer
            memcpy(mCineBuffer.getFrame(TEX_FRAME_TYPE_B, DAU_DATA_TYPE_TEX),
                    mCineBuffer.getFrame(frameNum, DAU_DATA_TYPE_B),
                   cineParams.converterParams.numSamples * cineParams.converterParams.numRays);
            break;

        case IMAGING_MODE_COLOR:
            // update Threshold Vaules
            cineParams.colorBThreshold = mCModeRenderer.getBThreshold();
            cineParams.colorVelThreshold = mCModeRenderer.getVelThreshold();

            // copy B-mode frame to tex buffer
            memcpy(mCineBuffer.getFrame(TEX_FRAME_TYPE_B, DAU_DATA_TYPE_TEX),
                   mCineBuffer.getFrame(frameNum, DAU_DATA_TYPE_B),
                   cineParams.converterParams.numSamples * cineParams.converterParams.numRays);

            memcpy(mCineBuffer.getFrame(TEX_FRAME_TYPE_C, DAU_DATA_TYPE_TEX),
                   mCineBuffer.getFrame(frameNum, DAU_DATA_TYPE_COLOR),
                   cineParams.colorConverterParams.numSamples * cineParams.colorConverterParams.numRays);
            break;

        case IMAGING_MODE_PW:
        case IMAGING_MODE_CW:
            // call prepareFreezeMode to copy the frames to texture buffer.
            mSModeRenderer.prepareFreezeMode(-1);
            // get the updated parameters after the data capture
            mSModeRenderer.getRecordParams(scrollParams);

            // need to update only two params
            cineParams.mSingleFrameWidth = scrollParams.scrDataWidth;
            cineParams.traceIndex = scrollParams.traceIndex;
            break;

        default:
            break;
    }

    // set transition ready Flag
    cineParams.isTransitionRenderingReady = true;

    // update CineParams in CineBuffer
    mCineBuffer.setCineParamsOnly(cineParams);
}

//-----------------------------------------------------------------------------
void CineViewer::prepareTransitionRendering()
{
    ALOGD("%s", __func__);

    // retrieve cineParams
    CineBuffer::CineParams cineParams;
    cineParams = mCineBuffer.getParams();
    float baselineShiftFracton;
    float desiredTimeSpan;
    uint32_t organGlobalId = cineParams.upsReader->getOrganGlobalId(cineParams.organId);

    // check whether transition rendering is ready
    if (!cineParams.isTransitionRenderingReady)
        return;

    // storedImaging Mode
    if (cineParams.transitionImagingMode != mImagingMode)
        mTransitionImgMode = cineParams.transitionImagingMode;
    else
        mTransitionImgMode = cineParams.transitionAltImagingMode;

    // to handle B -> C or C -> B transition in Duplex Screen
    if ( (mImagingMode != mTransitionImgMode)
         && (mImagingMode == IMAGING_MODE_B || mImagingMode == IMAGING_MODE_COLOR)
         && (mTransitionImgMode == IMAGING_MODE_B || mTransitionImgMode == IMAGING_MODE_COLOR) ) {
        // update transition imaging mode
        mTransitionImgMode = cineParams.transitionAltImagingMode;

        // update cineParams
        cineParams.transitionImagingMode = mTransitionImgMode;
        cineParams.transitionAltImagingMode = mImagingMode;
        mCineBuffer.setCineParamsOnly(cineParams);
    }

    switch (mTransitionImgMode)
    {
        case IMAGING_MODE_B:
            // ScanConverter Params
            mBModeRenderer.setParams(cineParams.converterParams);

            // Add renderer
            mRenderSurface.addRenderer(&mBModeRenderer, mUSLayout[0], mUSLayout[1], mUSLayout[2],
                                       mUSLayout[3]);
            break;

        case IMAGING_MODE_COLOR:
            // get ColorMap
            uint32_t                    velocityMap[256];
            mCineBuffer.getParams().upsReader->getColorMap(cineParams.colorMapIndex,
                    cineParams.isColorMapInverted, velocityMap);

            // set Cmode Renderer Params
            mCModeRenderer.setParams(cineParams.converterParams, cineParams.colorConverterParams,
                    cineParams.colorBThreshold, cineParams.colorVelThreshold, (uint8_t*) velocityMap, cineParams.colorDopplerMode);

            // Add renderer
            mRenderSurface.addRenderer(&mCModeRenderer, mUSLayout[0], mUSLayout[1], mUSLayout[2],
                                       mUSLayout[3]);
            break;

        case IMAGING_MODE_PW:
            // set Params
            cineParams.upsReader->getPwBaselineShiftFraction(cineParams.dopplerBaselineShiftIndex, baselineShiftFracton);
            desiredTimeSpan = getTimeSpan(cineParams.dopplerSweepSpeedIndex, organGlobalId == TCD_GLOBAL_ID);

            mSModeRenderer.setParams(cineParams.mSingleFrameWidth, cineParams.dopplerFFTSize,
                    cineParams.scrollSpeed, baselineShiftFracton, cineParams.targetFrameRate, cineParams.dopplerNumLinesPerFrame,
                    cineParams.frameRate, cineParams.dopplerSamplesPerFrame, desiredTimeSpan, cineParams.dopplerVelocityScale*2.0f);
            mSModeRenderer.setInvert(cineParams.dopplerBaselineInvert);
            mSModeRenderer.setPeakMeanDrawing(cineParams.dopplerPeakDraw, cineParams.dopplerMeanDraw);

            // Add renderer
            mRenderSurface.addRenderer(&mSModeRenderer, mDOPLayout[0], mDOPLayout[1], mDOPLayout[2],
                                       mDOPLayout[3]);
            break;

        case IMAGING_MODE_CW:
            // set Params
            cineParams.upsReader->getCwBaselineShiftFraction(cineParams.dopplerBaselineShiftIndex, baselineShiftFracton);
            desiredTimeSpan = getTimeSpan(cineParams.dopplerSweepSpeedIndex, organGlobalId == TCD_GLOBAL_ID);

            mSModeRenderer.setParams(cineParams.mSingleFrameWidth, cineParams.dopplerFFTSize,
                                      cineParams.scrollSpeed, baselineShiftFracton, cineParams.targetFrameRate, cineParams.dopplerNumLinesPerFrame,
                                      cineParams.frameRate, cineParams.dopplerSamplesPerFrame, desiredTimeSpan, cineParams.dopplerVelocityScale*2.0f, 2.0f);
            mSModeRenderer.setInvert(cineParams.dopplerBaselineInvert);
            mSModeRenderer.setPeakMeanDrawing(cineParams.dopplerPeakDraw, cineParams.dopplerMeanDraw);

            // Add renderer
            mRenderSurface.addRenderer(&mSModeRenderer, mDOPLayout[0], mDOPLayout[1], mDOPLayout[2],
                                       mDOPLayout[3]);
            break;

        default:
            break;
    }

    // set transition ready Flag off (should update flag in CineBuffer)
    // mCineBuffer.getParams().isTransitionRenderingReady = false;
    // mCineBuffer.setTransitionReadyFlag(false);
}

//-----------------------------------------------------------------------------
void CineViewer::renderTransitionFrame()
{
    ALOGD("%s", __func__);

    // texture frame header
    CineBuffer::CineParams cineParams;
    cineParams = mCineBuffer.getParams();

    // stored imaging mode
    switch (mTransitionImgMode)
    {
        case IMAGING_MODE_B:
            //restore ZoomPanFlip
            mBModeRenderer.setScale(cineParams.zoomPanParams[0]);
            mBModeRenderer.setPan(cineParams.zoomPanParams[1], cineParams.zoomPanParams[2]);
            mBModeRenderer.setFlip(cineParams.zoomPanParams[3], cineParams.zoomPanParams[4]);

            // set Frame
            mBModeRenderer.setFrame(mCineBuffer.getFrame(TEX_FRAME_TYPE_B, DAU_DATA_TYPE_TEX));
            break;

        case IMAGING_MODE_COLOR:
            // restore ZoomPanFlip
            mCModeRenderer.setScale(cineParams.zoomPanParams[0]);
            mCModeRenderer.setPan(cineParams.zoomPanParams[1], cineParams.zoomPanParams[2]);
            mCModeRenderer.setFlip(cineParams.zoomPanParams[3], cineParams.zoomPanParams[4]);

            // re-bind colormap
            mCModeRenderer.bindColorMap();

            // set Frame
            mCModeRenderer.setFrame(mCineBuffer.getFrame(TEX_FRAME_TYPE_B, DAU_DATA_TYPE_TEX),
                                    mCineBuffer.getFrame(TEX_FRAME_TYPE_C, DAU_DATA_TYPE_TEX));
            break;

        case IMAGING_MODE_PW:
        case IMAGING_MODE_CW:
            mSModeRenderer.setSingleFrameTextureMode(true);
            mSModeRenderer.setSingleFrameDrawIndex(cineParams.traceIndex);
            // scrollModeRenderer loads texture from TexBuffer.
            mSModeRenderer.setFrameNum(0);
            break;

        default:
            break;
    }

    mCineBuffer.setRenderedTransitionImgMode(mTransitionImgMode);
}

//-----------------------------------------------------------------------------
void CineViewer::threadLoop()
{
    const int       numFd = 3;
    const int       stopEvent = 0;
    const int       dataEvent = 1;
    const int       pauseEvent = 2;
    struct pollfd   pollFd[numFd];
    bool            keepLooping  = true;
    int             ret;
    uint64_t        data;
    uint32_t        dauDataTypes;
    uint32_t        frameNum = 0;
    uint32_t        prevFrameNum = 0;
    uint8_t*        bFramePtr;
    uint8_t*        cFramePtr;
    int             bt;
    int             vt;
    ThorStatus      retVal = THOR_OK;

    ALOGD("%s - enter", __func__);

    pollFd[stopEvent].fd = mStopEvent.getFd();
    pollFd[stopEvent].events = POLLIN;
    pollFd[stopEvent].revents = 0;

    pollFd[dataEvent].fd = mDataEvent.getFd();
    pollFd[dataEvent].events = POLLIN;
    pollFd[dataEvent].revents = 0;

    mDataEvent.reset();

    pollFd[pauseEvent].fd = mPausePlaybackEvent.getFd();
    pollFd[pauseEvent].events = POLLIN;
    pollFd[pauseEvent].revents = 0;

    retVal = prepareRenderers();
    if (!IS_THOR_ERROR(retVal))
    {
        retVal = prepareAudioPlayer(&mAudioPlayer);
    }

    data = retVal;
    mRunningEvent.set(data);

    if (IS_THOR_ERROR(retVal))
    {
        keepLooping = false;
    }

    while (keepLooping)
    {
        ret = poll(pollFd, numFd, -1);
        if (-1 == ret)
        {
            ALOGE("threadLoop: Error occurred during poll(). errno = %d", errno);
            keepLooping = false;
        }
        else if (pollFd[stopEvent].revents & POLLIN)
        {
            mStopEvent.reset();
            keepLooping = false;
        }
        else if (pollFd[pauseEvent].revents & POLLIN)
        {
            mPausePlaybackEvent.reset();

            // pause AudioPlayer
            if (mAudioPlayer != nullptr)
                mAudioPlayer->pausePlayback();
        }
        else if (pollFd[dataEvent].revents & POLLIN)
        {
            data = mDataEvent.read();
            dauDataTypes = data >> 32;
            frameNum = data & 0xFFFFFFFF;

#ifdef CHECK_SKIPPED_FRAMES
            if (frameNum > (prevFrameNum + 1))
            {
                ALOGD("Skipping frames: %u", frameNum - prevFrameNum);
            }
            prevFrameNum = frameNum;
#endif

            mImguiRenderer.setFrameNum(frameNum);
            switch (mImagingMode)
            {
                case IMAGING_MODE_B:
                    mBModeRenderer.setFrame(mCineBuffer.getCurFrame(DAU_DATA_TYPE_B));

                    if (mIsDAOn && 0 != BF_GET(dauDataTypes, DAU_DATA_TYPE_DA, 1))
                    {
                        mDARenderer.setFrameNum(frameNum);
#ifdef ENABLE_AUDIO_PLAYER
                        if (mAudioPlayer != nullptr)
                        {
                            mAudioPlayer->setFrameNum(frameNum);
                        }
#endif
                    }
                    if (mIsECGOn && 0 != BF_GET(dauDataTypes, DAU_DATA_TYPE_ECG, 1))
                    {
                        //LOGI("ECG Frame interval");
                        mECGRenderer.setFrameNum(frameNum);
                        prepareHeartRate();
                    }

                    mRenderSurface.draw();
                    break;

                case IMAGING_MODE_COLOR:
                    bFramePtr = mCineBuffer.getFrame(frameNum, DAU_DATA_TYPE_B);
                    cFramePtr = mCineBuffer.getFrame(frameNum, DAU_DATA_TYPE_COLOR);

                    mCModeRenderer.setFrame(bFramePtr, cFramePtr);

                    if (mIsDAOn && 0 != BF_GET(dauDataTypes, DAU_DATA_TYPE_DA, 1))
                    {
                        mDARenderer.setFrameNum(frameNum);
#ifdef ENABLE_AUDIO_PLAYER
                        if (mAudioPlayer != nullptr)
                        {
                            mAudioPlayer->setFrameNum(frameNum);
                        }
#endif
                    }
                    if (mIsECGOn && 0 != BF_GET(dauDataTypes, DAU_DATA_TYPE_ECG, 1))
                    {
                        mECGRenderer.setFrameNum(frameNum);
                        prepareHeartRate();
                    }

                    mRenderSurface.draw();
                    break;

                case IMAGING_MODE_M:
                    mBModeRenderer.setFrame(mCineBuffer.getCurFrame(DAU_DATA_TYPE_B));
                    mSModeRenderer.setFrameNum(frameNum);

                    mRenderSurface.draw();
                    break;

                case IMAGING_MODE_PW:
                    mSModeRenderer.setFrameNum(frameNum);

                    // PW AUDIO
                    if (mAudioPlayer != nullptr)
                    {
                        mAudioPlayer->setFrameNum(frameNum);
                    }
                    mRenderSurface.draw();
                    break;

                case IMAGING_MODE_CW:
                    mSModeRenderer.setFrameNum(frameNum);

                    // CW AUDIO
                    if (mAudioPlayer != nullptr)
                    {
                        mAudioPlayer->setFrameNum(frameNum);
                    }
                    mRenderSurface.draw();
                    break;
            }
        }
        else
        {
            ALOGE("Error occurred in poll()");
            keepLooping = false;
        }
    }

do_exit:
    // AudioPlayer PausePlayback
    if (mAudioPlayer != nullptr)
        mAudioPlayer->pausePlayback();

    // reset TransitionReady flag
    mCineBuffer.setTransitionReadyFlag(false);
    mCineBuffer.setRenderedTransitionImgMode(-1);

    // Prepare Transition Frame if needed
    if (mPrepareTransitionFrame)
        processDopplerTransition(frameNum);

    mRenderSurface.close();
    mRenderSurface.clearRenderList();

    ALOGD("%s - leave", __func__);
}

//-----------------------------------------------------------------------------
ThorStatus CineViewer::prepareRenderers()
{
    ThorStatus  retVal = THOR_OK;

    mLock.enter();

    if ((mImagingMode != IMAGING_MODE_M))
    {
        // prepare transition rendering
        prepareTransitionRendering();
    }

    switch (mImagingMode)
    {
        case IMAGING_MODE_B:
            if (mIsDAOn || mIsECGOn)
            {
                if (mIsDAOn)
                {
                    mRenderSurface.addRenderer(&mDARenderer, mDALayout[0],mDALayout[1], mDALayout[2], mDALayout[3]);
                }

                if (mIsECGOn)
                {
                    mRenderSurface.addRenderer(&mECGRenderer, mECGLayout[0], mECGLayout[1], mECGLayout[2], mECGLayout[3]);
                }
                // use USLayout values, assuming size of (100%, 100%)
                mRenderSurface.addRenderer(&mBModeRenderer, mUSLayout[0],  mUSLayout[1],  mUSLayout[2],  mUSLayout[3]);
                mRenderSurface.addRenderer(&mImguiRenderer, mUSLayout[0],  mUSLayout[1],  mUSLayout[2],  mUSLayout[3]);
            }
            else
            {
                // use USLayout values, assuming size of (100%, 100%)
                mRenderSurface.addRenderer(&mBModeRenderer, mUSLayout[0],  mUSLayout[1],  mUSLayout[2],  mUSLayout[3]);
                mRenderSurface.addRenderer(&mImguiRenderer, mUSLayout[0],  mUSLayout[1],  mUSLayout[2],  mUSLayout[3]);
            }
            break;

        case IMAGING_MODE_COLOR:
            if (mIsDAOn)
            {
                mRenderSurface.addRenderer(&mDARenderer, mDALayout[0],mDALayout[1], mDALayout[2], mDALayout[3]);
            }

            if (mIsECGOn)
            {
                mRenderSurface.addRenderer(&mECGRenderer, mECGLayout[0], mECGLayout[1], mECGLayout[2], mECGLayout[3]);
            }
            // color Renderer
            mRenderSurface.addRenderer(&mCModeRenderer, mUSLayout[0],  mUSLayout[1],  mUSLayout[2],  mUSLayout[3]);
            break;

        case IMAGING_MODE_M:
            mRenderSurface.addRenderer(&mBModeRenderer, mBModeLayout[0], mBModeLayout[1],
                                       mBModeLayout[2], mBModeLayout[3]);
            mRenderSurface.addRenderer(&mSModeRenderer, mMModeLayout[0], mMModeLayout[1],
                                       mMModeLayout[2], mMModeLayout[3]);
            break;

        case IMAGING_MODE_PW:
            //TODO: update - placeholder for layout
            if (mIsDAOn || mIsECGOn)
            {
                if (mIsDAOn) {
                    mRenderSurface.addRenderer(&mDARenderer, mDALayout[0], mDALayout[1],
                                               mDALayout[2], mDALayout[3]);
                }

                if (mIsECGOn) {
                    mRenderSurface.addRenderer(&mECGRenderer, mECGLayout[0], mECGLayout[1],
                                               mECGLayout[2], mECGLayout[3]);
                }
            }

            mRenderSurface.addRenderer(&mSModeRenderer, mDOPLayout[0], mDOPLayout[1],
                                       mDOPLayout[2], mDOPLayout[3]);
            break;

        case IMAGING_MODE_CW:
            mRenderSurface.addRenderer(&mSModeRenderer, mDOPLayout[0], mDOPLayout[1],
                                       mDOPLayout[2], mDOPLayout[3]);
            break;
    }

    mRenderSurface.open(mWindowPtr);

    // prepare additional rendering related tasks.
    // the freeze/live mode switching for M/PW/CW
    if (mImagingMode == IMAGING_MODE_M)
    {
        mSModeRenderer.prepareRenderer(&mCineBuffer, DAU_DATA_TYPE_M);
    }
    else if (mImagingMode == IMAGING_MODE_PW)
    {
        mSModeRenderer.prepareRenderer(&mCineBuffer, DAU_DATA_TYPE_PW);
    }
    else if (mImagingMode == IMAGING_MODE_CW)
    {
        mSModeRenderer.prepareRenderer(&mCineBuffer, DAU_DATA_TYPE_CW);
    }

    // set single frame freeze mode
    if (!mIsLive && (mCineBuffer.getMaxFrameNum() == mCineBuffer.getMinFrameNum()))
    {
        // singleFrame Freeze Mode
        mDARenderer.setSingleFrameFreezeMode(true);
        mECGRenderer.setSingleFrameFreezeMode(true);

        mSModeRenderer.setSingleFrameTextureMode(true);
        mSModeRenderer.setSingleFrameDrawIndex(mCineBuffer.getParams().traceIndex);
    }
    else
    {
        mDARenderer.setSingleFrameFreezeMode(false);
        mECGRenderer.setSingleFrameFreezeMode(false);

        mSModeRenderer.setSingleFrameTextureMode(false);
    }

    if (mIsDAOn)
    {
        mDARenderer.prepareRenderer(&mCineBuffer, DAU_DATA_TYPE_DA);
    }

    if (mIsECGOn)
    {
        mECGRenderer.prepareRenderer(&mCineBuffer, DAU_DATA_TYPE_ECG);
    }

    if (mCineBuffer.getParams().isTransitionRenderingReady)
    {
        renderTransitionFrame();
    }

    if (mIsLive && ((mImagingMode == IMAGING_MODE_PW) || (mImagingMode == IMAGING_MODE_CW)) )
    {
        mSModeRenderer.setPeakMeanDrawing(false, false);

        // CineParams
        mCineBuffer.getParams().dopplerPeakDraw = false;
        mCineBuffer.getParams().dopplerMeanDraw = false;
    }

    mLock.leave();

    return(retVal);
}

//-----------------------------------------------------------------------------
ThorStatus CineViewer::prepareAudioPlayer(ThorAudioPlayer** audioPlayerPtrPtr)
{
    ThorStatus          retVal = THOR_OK;
    ThorAudioPlayer*    audioPlayerPtr = *audioPlayerPtrPtr;

#ifdef ENABLE_AUDIO_PLAYER
    if (audioPlayerPtr != nullptr)
    {
        int32_t audioSampleRate = mAudioPlayer->getDesiredSampleRate();
        int32_t audioSamplesPerFrame = mAudioPlayer->getInputFrameSize();
        uint32_t dataType = mAudioPlayer->getInputDateType();

        bool reInit = true;

        if (mImagingMode == IMAGING_MODE_PW)
        {
            if ( (mDopplerAudioSampleRate == audioSampleRate) && (mDopplerAudioSamplesPerFrame == audioSamplesPerFrame)
                    && (dataType == DAU_DATA_TYPE_PW_ADO) )
                reInit = false;
        }
        else if (mImagingMode == IMAGING_MODE_CW)
        {
            if ( (mDopplerAudioSampleRate == audioSampleRate) && (mDopplerAudioSamplesPerFrame == audioSamplesPerFrame)
                 && (dataType == DAU_DATA_TYPE_CW_ADO) )
                reInit = false;
        }
        else if ((mIsDAOn && mDASampleRate > 7999))
        {
            if ( (mDASampleRate == audioSampleRate) && (mDASamplesPerFrame == audioSamplesPerFrame)
                 && (dataType == DAU_DATA_TYPE_DA) )
                reInit = false;
        }

        if (reInit)
        {
            ALOGD("Delete AudioPlayer for re-init");
            delete audioPlayerPtr;
            audioPlayerPtr = nullptr;
        }
    }

    if (audioPlayerPtr == nullptr)
    {
        // setup Audio Player
        if (mImagingMode == IMAGING_MODE_PW) {
            audioPlayerPtr = new ThorAudioPlayer(&mCineBuffer, mDopplerAudioSampleRate,
                                                 mDopplerAudioSamplesPerFrame,
                                                 DAU_DATA_TYPE_PW_ADO);

            retVal = audioPlayerPtr->init();

            // TODO: Unless specify to a certain device use 0 for auto-select
            // 0 is default value so don't need to call audioPlayer->setDeviceId(0);
            audioPlayerPtr->setInterleavedStereo(true);
        } else if (mImagingMode == IMAGING_MODE_CW) {
            audioPlayerPtr = new ThorAudioPlayer(&mCineBuffer, mDopplerAudioSampleRate,
                                                 mDopplerAudioSamplesPerFrame,
                                                 DAU_DATA_TYPE_CW_ADO);

            retVal = audioPlayerPtr->init();

            // TODO: Unless specify to a certain device use 0 for auto-select
            // 0 is default value so don't need to call audioPlayer->setDeviceId(0);
            audioPlayerPtr->setInterleavedStereo(true);
        } else if ((mIsDAOn && mDASampleRate > 7999)) {
            audioPlayerPtr = new ThorAudioPlayer(&mCineBuffer, mDASampleRate, mDASamplesPerFrame,
                                                 DAU_DATA_TYPE_DA);

            retVal = audioPlayerPtr->init();

            // TODO: Unless specify to a certain device use 0 for auto-select
            // 0 is default value so don't need to call audioPlayer->setDeviceId(0);
        }

        *audioPlayerPtrPtr = audioPlayerPtr;
    }

#endif

    return(retVal);
}

//-----------------------------------------------------------------------------
void CineViewer::pausePlayback()
{
    mPausePlaybackEvent.set();
}

/**
 * Fetch Heart Rate from CineBuffer which was filled by EcgProcess
 */
void CineViewer::prepareHeartRate() {

    uint32_t frameNum = mCineBuffer.getCurFrameNum();

    if((frameNum%mHRUpdateInterval == 0) || !mIsLive)
    {
        uint8_t *outputPtr = mCineBuffer.getFrame(frameNum, DAU_DATA_TYPE_ECG,
                                                  CineBuffer::FrameContents::IncludeHeader);
        CineBuffer::CineFrameHeader *cineHeaderPtr = reinterpret_cast<CineBuffer::CineFrameHeader *>(outputPtr);
        float heartRate = cineHeaderPtr->heartRate;

        if (mPrevDisplayHR != heartRate)
        {
            mPrevDisplayHR = heartRate;

            //Send to Application UI
            mReportHeartRate(heartRate);
        }
    }

}

bool CineViewer::isInitialised() const {
    return mIsInitialized;
}

//-----------------------------------------------------------------------------
void CineViewer::attachCallbacks(ReportHeartRate updateHeartRate,
                                 ReportError errorReporter)
{
    mReportHeartRate = updateHeartRate;
    mErrorReporter = errorReporter;

    // re-init previsouly displayed heartRate
    mPrevDisplayHR = -1.0f;
}

ImguiRenderer& CineViewer::getImguiRenderer()
{
    return mImguiRenderer;
}
