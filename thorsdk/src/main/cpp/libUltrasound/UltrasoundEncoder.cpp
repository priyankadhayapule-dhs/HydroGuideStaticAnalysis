//
// Copyright 2018 EchoNous Inc.
//
//

#define LOG_TAG "UltrasoundEncoder"

#include <poll.h>
#include <ThorUtils.h>
#include <UltrasoundEncoder.h>
#include <CineBuffer.h>
#include <BitfieldMacros.h>
#include <ClipEncoderCore.h>
#include <ThorFileProcessor.h>
#include <AudioCubicReSampler.h>
#include <unistd.h>

// For open()/close()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define ENABLE_ELAPSED_TIME
#include <ElapsedTime.h>
#include <ThorCapnpTypes.capnp.h>
#include <ThorCapnpSerialize.h>

#include <memory>

#define DA_AUDIO_OUT_SAMPLERATE 8000

// DA ECG Line Drawing Thickness for Encoding
#define DA_ECG_LINE_THICKNESS_ENC 2

// Intentional delay of audio data feed, since video encoding takes more time.  -1 for no delay.
#define AUDIO_FRAME_ENCODE_DELAY 4

//-----------------------------------------------------------------------------
UltrasoundEncoder::UltrasoundEncoder() :
    mUpsReaderUPtr(nullptr),
    mCompletionHandlerPtr(nullptr),
    mIsInitialized(false),
    mIsBusy(false),
    mCancelRequested(false),
    mIsDaEnabled(false),
    mIsEcgEnabled(false),
    mRenderHR(false),
    mFrameCount(0),
    mFrameInterval(0),
    mTargetFrameRate(30),       // for scrolling mode or waveform renderer
    mUseCapnpReader(false),
    mCapnpFd(-1),
    mCapnpReader(nullptr),
    mBaseAddressPtr(nullptr),
    mBaseAddressPtrColor(nullptr),
    mBaseAddressPtrM(nullptr),
    mBaseAddressPtrDa(nullptr),
    mBaseAddressPtrEcg(nullptr),
    mStartFrame(0),
    mEndFrame(0),
    mStillFrameNum(0),
    mIsForcedFSR(false)
{
}

//-----------------------------------------------------------------------------
UltrasoundEncoder::~UltrasoundEncoder()
{
    ALOGD("%s", __func__);
    terminate();
}

//-----------------------------------------------------------------------------
ThorStatus UltrasoundEncoder::init(const char* appPathPtr)
{
    ThorStatus      retVal = THOR_ERROR;
    int             ret;

    // AppPath and UpsReaderPtr
    mAppPath = appPathPtr;
    mUpsReaderUPtr = std::make_unique<UpsReader>();

    if (!mStopEvent.init(EventMode::ManualReset, false))
    {
        ALOGE("Unable to initialize StopEvent");
        goto err_ret;
    }
    mStopEvent.reset();

    if (!mEncodeStillEvent.init(EventMode::ManualReset, false))
    {
        ALOGE("Unable to initialize RecordStillEvent");
        goto err_ret;
    }

    if (!mEncodeClipEvent.init(EventMode::ManualReset, false))
    {
        ALOGE("Unable to initialize RecordCineEvent");
        goto err_ret;
    }

    ret = pthread_create(&mThreadId, NULL, workerThreadEntry, this);
    if (0 != ret)
    {
        ALOGE("Failed to create worker thread: ret = %d", ret);
        goto err_ret;
    }

    // default DA/ECG Line Color
    mDALineColor[0] = 0.271f;
    mDALineColor[1] = 0.631f;
    mDALineColor[2] = 0.910f;

    mECGLineColor[0] = 0.012f;
    mECGLineColor[1] = 0.941f;
    mECGLineColor[2] = 0.420f;

    // set OverlayHRRenderer path
    mHRRenderer.setAppPath(appPathPtr);

    mIsInitialized = true;
    retVal = THOR_OK;

err_ret:
    return(retVal);
}

//-----------------------------------------------------------------------------
void UltrasoundEncoder::terminate()
{
    // terminate
    if (mIsInitialized)
    {
        mStopEvent.set();
        pthread_join(mThreadId, NULL);

        mIsInitialized = false;
    }

    if (nullptr != mUpsReaderUPtr)
    {
        mUpsReaderUPtr->close();
        mUpsReaderUPtr = nullptr;
    }
}

//-----------------------------------------------------------------------------
void UltrasoundEncoder::close()
{
    // close input file and off rendering surface
    if (mCapnpFd > 0)
    {
        ::close(mCapnpFd);
        mCapnpFd = -1;
    }
    mCapnpReader.reset();
    mUseCapnpReader = false;
    mReader.close();
    mOffSCreenRenderSurface.close();
    mOffSCreenRenderSurface.clearRenderList();
}

//-----------------------------------------------------------------------------
void UltrasoundEncoder::attachCompletionHandler(CompletionHandler completionHandlerPtr)
{
    mLock.enter();
    mCompletionHandlerPtr = completionHandlerPtr;
    mLock.leave();
}

//-----------------------------------------------------------------------------
void UltrasoundEncoder::detachCompletionHandler()
{
    mLock.enter();
    mCompletionHandlerPtr = nullptr;
    mLock.leave();
}

//-----------------------------------------------------------------------------
bool UltrasoundEncoder::getBusyLock()
{
    bool gotLock = false;

    mBusyLock.enter();
    if (!mIsBusy)
    {
        mIsBusy = true;
        gotLock = true;
    }
    mBusyLock.leave();

    return (gotLock);
}

//-----------------------------------------------------------------------------
void UltrasoundEncoder::requestCancel()
{
    mBusyLock.enter();
    mCancelRequested = true;
    mBusyLock.leave();
}

//-----------------------------------------------------------------------------
void *UltrasoundEncoder::workerThreadEntry(void *thisPtr)
{
    ((UltrasoundEncoder *)thisPtr)->threadLoop();

    return (nullptr);
}

//-----------------------------------------------------------------------------
void UltrasoundEncoder::threadLoop()
{
    const int       numFd = 3;
    const int       stopEvent = 0;
    const int       encodeStill = 1;
    const int       encodeClip = 2;
    struct pollfd   pollFd[numFd];

    bool            keepLooping  = true;
    int             ret;
    ThorStatus      retVal = THOR_OK;

    pollFd[stopEvent].fd = mStopEvent.getFd();
    pollFd[stopEvent].events = POLLIN;
    pollFd[stopEvent].revents = 0;

    pollFd[encodeStill].fd = mEncodeStillEvent.getFd();
    pollFd[encodeStill].events = POLLIN;
    pollFd[encodeStill].revents = 0;

    pollFd[encodeClip].fd = mEncodeClipEvent.getFd();
    pollFd[encodeClip].events = POLLIN;
    pollFd[encodeClip].revents = 0;

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
        else if (pollFd[encodeStill].revents & POLLIN)
        {
            // encode Still
            mEncodeStillEvent.reset();
            retVal = encodeStillCore();

            mLock.enter();
            if (nullptr != mCompletionHandlerPtr)
            {
                mCompletionHandlerPtr(retVal);
            }
            mLock.leave();
        }
        else if (pollFd[encodeClip].revents & POLLIN)
        {
            // encode Clip
            mEncodeClipEvent.reset();
            retVal = encodeClipCore();

            mLock.enter();
            if (nullptr != mCompletionHandlerPtr)
            {
                if (mCancelRequested)
                {
                    ALOGD("Clip encoding has been canceled.");
                    mCompletionHandlerPtr(TER_MISC_ABORT);
                }
                else
                {
                    mCompletionHandlerPtr(retVal);
                }
            }
            mLock.leave();
        }
        else
        {
            ALOGE("Error occurred in poll()");
            keepLooping = false;
        }

        mBusyLock.enter();
        mCancelRequested = false;
        mIsBusy = false;
        mBusyLock.leave();
    }
}

//-----------------------------------------------------------------------------
ThorStatus UltrasoundEncoder::processInputCapnp(const char *srcPath)
{
    ThorStatus  retVal;

    try {
        // I don't think this should get triggered, we should always call UltrasoundEncoder::close first
        // Still, better to be safe
        if (mCapnpFd > 0)
        {
            ::close(mCapnpFd);
        }
        mCapnpFd = ::open(srcPath, O_RDONLY);
        capnp::ReaderOptions options;
        options.traversalLimitInWords =
                1024 * 1024 * 128; // expand traversal limit to allow up to 1GiB files
        mCapnpReader = std::make_unique<capnp::StreamFdMessageReader>(mCapnpFd, options);
    } catch (std::exception &e) {
        LOGE("Exception while trying to read thor file in Capnp format: %s", e.what());
        return TER_MISC_INVALID_FILE;
    }

    auto clip = mCapnpReader->getRoot<echonous::capnp::ThorClip>();
    float    desiredTimeSpan;
    uint32_t desiredSweepSpeedIndex;

    // update variables
    ReadCineParams(clip.getCineParams(), mCineParams);

    // read thor db before updating cineParams
    retVal = mUpsReaderUPtr->open(mAppPath.c_str(), mCineParams.probeType);
    if (IS_THOR_ERROR(retVal))
    {
        ALOGE("Unable to open the UPS");
        return retVal;
    }
    ALOGD("UPS version : %s", mUpsReaderUPtr->getVersion());

    mFrameCount = clip.getNumFrames();
    mFrameInterval = mCineParams.frameInterval;
    mImagingCaseID = mCineParams.imagingCaseId;

    mImagingMode = mCineParams.imagingMode;

    mIsDaEnabled = clip.hasDa();
    mIsEcgEnabled = clip.hasEcg();
    mRenderHR = false;

    // Unused variables
    mCineFrameHeaderSize = 0;
    mBaseAddressPtr = nullptr;
    mBaseAddressPtrColor = nullptr;
    mBaseAddressPtrM = nullptr;
    mBaseAddressPtrDa = nullptr;
    mBaseAddressPtrEcg = nullptr;

    LOGD("Capnp file additional: %s %s", mIsDaEnabled ? "DA":"", mIsEcgEnabled? "ECG":"");

    if (mIsEcgEnabled || mIsDaEnabled)
    {
        desiredSweepSpeedIndex = mCineParams.daEcgScrollSpeedIndex;
        uint32_t organGlobalId = mUpsReaderUPtr->getOrganGlobalId(mCineParams.organId);
        desiredTimeSpan = getTimeSpan(desiredSweepSpeedIndex, organGlobalId == TCD_GLOBAL_ID);

        mEcgRecordParams.scrDataWidth = (uint32_t)floor(desiredTimeSpan * mCineParams.ecgSamplesPerSecond);
        mEcgRecordParams.samplesPerSecond = mCineParams.ecgSamplesPerSecond;
        mEcgRecordParams.samplesPerFrame = mCineParams.ecgSamplesPerFrame;
        mEcgRecordParams.traceIndex = mCineParams.ecgTraceIndex;
        mEcgRecordParams.frameWidth = mCineParams.ecgSingleFrameWidth;
        mEcgRecordParams.decimationRatio = 1;
        mEcgRecordParams.location = mCineParams.ecgLeadNumExternal;

        mDaRecordParams.scrDataWidth = (uint32_t)floor(desiredTimeSpan * mCineParams.daSamplesPerSecond);
        mDaRecordParams.samplesPerSecond = mCineParams.daSamplesPerSecond;
        mDaRecordParams.samplesPerFrame = mCineParams.daSamplesPerFrame;
        mDaRecordParams.traceIndex = mCineParams.daTraceIndex;
        mDaRecordParams.frameWidth = mCineParams.daSingleFrameWidth;
        mDaRecordParams.decimationRatio = 1;
    }

    if (mFrameCount > 1)
        mRenderHR = mIsEcgEnabled && (mEcgRecordParams.location != -1);

    LOGI("Opened Capnp clip containing %u frames", mFrameCount);

    retVal = THOR_OK;
    mUseCapnpReader = true;
    return retVal;
}

//-----------------------------------------------------------------------------
ThorStatus UltrasoundEncoder::processInput(const char* srcPath, bool tryNewFormat)
{
    ThorStatus                      retVal = THOR_ERROR;
    ThorRawDataFile::Metadata       metadata;
    uint8_t*                        addressPtr = nullptr;
    ThorFileProcessor               fileProcessor;
    uint8_t                         fileVersion;
    ScanConverterParams             scParams;
    uint32_t                        probeType;

    LOGD("Opening input path %s", srcPath);
    // open Reader
    retVal = mReader.open(srcPath, metadata);
    if (IS_THOR_ERROR(retVal))
    {
        LOGD("ThorFileReader open failed with code 0x%08x", retVal);
        if (tryNewFormat)
        {
            LOGD("Trying to process input file as Capnp format");
            retVal = processInputCapnp(srcPath);

            if (IS_THOR_ERROR(retVal))
            {
                LOGE("Unable to parse file as Capnp format");
                goto err_ret;
            }
            else
            {
                LOGD("File successfully read as Capnp format");
                goto ok_ret;
            }

        } else
        {
            ALOGE("Unable to open raw file");
            goto err_ret;
        }
    }

    // This routine is to support older file formats.
    // Supported Mode: B, B+C, M (still) + DA/ECG only
    mUseCapnpReader = false;
    addressPtr = mReader.getDataPtr();
    if (nullptr == addressPtr)
    {
        ALOGE("getDataPtr() failed");
        retVal = TER_MISC_OPEN_FILE_FAILED;
        goto err_ret;
    }

    // prepare UPS reader for older data format support.
    // probeType = 0x3 (default);
    probeType = 0x3;
    retVal = mUpsReaderUPtr->open(mAppPath.c_str(), probeType);
    if (IS_THOR_ERROR(retVal))
    {
        ALOGE("Unable to open the UPS");
        goto err_ret;
    }
    ALOGD("UPS version : %s", mUpsReaderUPtr->getVersion());

    fileVersion = mReader.getVersion();

    mFrameCount = metadata.frameCount;
    mFrameInterval = metadata.frameInterval;
    mImagingCaseID = metadata.imageCaseId;
    mCineParams.frameRate = 1000.0f/((float) mFrameInterval);

    fileProcessor.processModeHeader(addressPtr, metadata.dataTypes, mFrameCount, fileVersion);

    mImagingMode = mUpsReaderUPtr->getImagingMode(mImagingCaseID);

    mIsDaEnabled = false;
    mIsEcgEnabled = false;
    mRenderHR = false;

    mCineFrameHeaderSize = fileProcessor.getFrameHeaderSize();

    // older format - must have B-mode SC Params
    mUpsReaderUPtr->getScanConverterParams(mImagingCaseID, scParams, mImagingMode);
    // update cineParams
    mCineParams.converterParams = scParams;

    // if there an error in UPS Reader
    if ( 0 != mUpsReaderUPtr->getErrorCount() )
    {
        ALOGE("%s UPS error count was not zero on entry", __func__);
        retVal = TER_IE_UPS_READ_FAILURE;
        goto err_ret;
    }

    // read Imaging Mode
    if (0 != BF_GET(metadata.dataTypes, ThorRawDataFile::DataType::BMode, 1))
    {
        mBaseAddressPtr = (uint8_t*)fileProcessor.getDataPtr(ThorRawDataFile::DataType::BMode)
                          + mCineFrameHeaderSize;
    }

    if (0 != BF_GET(metadata.dataTypes, ThorRawDataFile::DataType::CMode, 1))
    {
        // add default params for backward compatibility
        mColorModeParams.colorMapIndex = 3;
        mColorModeParams.isInverted = false;

        fileProcessor.getModeHeader(ThorRawDataFile::DataType::CMode, &mColorModeParams);

        mBaseAddressPtrColor = (uint8_t*)fileProcessor.getDataPtr(ThorRawDataFile::DataType::CMode)
                               + mCineFrameHeaderSize;

        // copy color mode params to cineParams
        ScanConverterParams colorSCParams;
        colorSCParams.numSampleMesh      = mColorModeParams.scanConverterParams.numSampleMesh;
        colorSCParams.numRayMesh         = mColorModeParams.scanConverterParams.numRayMesh;
        colorSCParams.numSamples         = mColorModeParams.scanConverterParams.numSamples;
        colorSCParams.numRays            = mColorModeParams.scanConverterParams.numRays;
        colorSCParams.startSampleMm      = mColorModeParams.scanConverterParams.startSampleMm;
        colorSCParams.sampleSpacingMm    = mColorModeParams.scanConverterParams.sampleSpacingMm;
        colorSCParams.raySpacing         = mColorModeParams.scanConverterParams.raySpacing;
        colorSCParams.startRayRadian     = mColorModeParams.scanConverterParams.startRayRadian;

        mCineParams.colorConverterParams = colorSCParams;
        mCineParams.colorMapIndex = mColorModeParams.colorMapIndex;
        mCineParams.isColorMapInverted = mColorModeParams.isInverted;
    }

    if (0 != BF_GET(metadata.dataTypes, ThorRawDataFile::DataType::MMode, 1))
    {
        fileProcessor.getModeHeader(ThorRawDataFile::DataType::MMode, &mMModeParams);

        mBaseAddressPtrM = (uint8_t*)fileProcessor.getDataPtr(ThorRawDataFile::DataType::MMode)
                           + mCineFrameHeaderSize;

        // copy Mmode params to cineParams
        mCineParams.mlinesPerFrame = mMModeParams.linesPerBFrame;
        mCineParams.mSingleFrameWidth = mMModeParams.textureFrameWidth;
        mCineParams.traceIndex = mMModeParams.curLineIndex;

        // older M-mode supports only still image - bogus numbers for comparability
        // TODO: update when M-mode sweeping time span is determined.
        mCineParams.scrollSpeed = 400.0f;
        mCineParams.targetFrameRate = 30;
        mCineParams.frameRate = 30.0f;
    }

    if (0 != BF_GET(metadata.dataTypes, ThorRawDataFile::DataType::Auscultation, 1))
    {
        mIsDaEnabled = true;

        fileProcessor.getModeHeader(ThorRawDataFile::DataType::Auscultation, &mDaRecordParams);

        mBaseAddressPtrDa = (uint8_t*)fileProcessor.getDataPtr(ThorRawDataFile::DataType::Auscultation)
                            + mCineFrameHeaderSize;

        mCineParams.daEcgScrollSpeedIndex = mDaRecordParams.scrollSpeedIndex;
    }

    if (0 != BF_GET(metadata.dataTypes, ThorRawDataFile::DataType::ECG, 1))
    {
        mIsEcgEnabled = true;

        fileProcessor.getModeHeader(ThorRawDataFile::DataType::ECG, &mEcgRecordParams);

        mBaseAddressPtrEcg = (uint8_t*)fileProcessor.getDataPtr(ThorRawDataFile::DataType::ECG)
                             + mCineFrameHeaderSize;

        mCineParams.daEcgScrollSpeedIndex = mEcgRecordParams.scrollSpeedIndex;
    }

    if (mIsEcgEnabled && (mEcgRecordParams.location != -1) && (mFrameCount > 1))
        mRenderHR = true;

    // TODO: ADD OTHER MODES

    retVal = THOR_OK;

err_ret:
ok_ret:
    return (retVal);
}

//-----------------------------------------------------------------------------
ThorStatus UltrasoundEncoder::prepareRenderSurface(ANativeWindow *inputSurface, bool addOverlay)
{
    ThorStatus retVal = THOR_ERROR;

    // zoom, pan related parameters
    mTargetFrameRate = 30;
    float bFrameRate = 1000.0f / ((float) mFrameInterval);
    float hrrYNorm = 90.0f;

    uint32_t desiredSweepSpeedIndex;
    float    desiredTimeSpan;
    float    baselineShiftFracton;

    // TimeSpan update
    desiredSweepSpeedIndex = mCineParams.daEcgScrollSpeedIndex;

    if ( (mImagingMode == IMAGING_MODE_PW) || (mImagingMode == IMAGING_MODE_CW) )
    {
        desiredSweepSpeedIndex = mCineParams.dopplerSweepSpeedIndex;
    }
    else if (mImagingMode == IMAGING_MODE_M)
    {
        desiredSweepSpeedIndex = mCineParams.mModeSweepSpeedIndex;
    }

    // get desired timespan
    uint32_t organGlobalId = mUpsReaderUPtr->getOrganGlobalId(mCineParams.organId);
    desiredTimeSpan = getTimeSpan(desiredSweepSpeedIndex, organGlobalId == TCD_GLOBAL_ID);

    // To support older data format, which does not have mModeSweepSpeedIndex
    if (mImagingMode == IMAGING_MODE_M  && (desiredSweepSpeedIndex < 0))
    {
        desiredTimeSpan = ((float) mCineParams.mSingleFrameWidth) / mCineParams.scrollSpeed;
    }

    // prepare transition frame rendering
    if (mCineParams.isTransitionRenderingReady)
        prepareTransitionRendering(mCineParams.renderedTransitionImgMode, desiredTimeSpan);

    // Tint adjustment
    float r,g,b;
    if (!mUpsReaderUPtr->getTint(r,g,b))
    {
        r = 0.84f;
        g = 0.88f;
        b = 0.90f;
    }

    switch (mImagingMode)
    {
        case IMAGING_MODE_B:
            mBModeRenderer.setParams(mCineParams.converterParams);
            mBModeRenderer.setTintAdjustment(r, g, b);
            mOffSCreenRenderSurface.addRenderer(&mBModeRenderer, mWinXPct, 100.0f - (mWinYPct + mWinHeightPct), mWinWidthPct, mWinHeightPct);
            break;

        case IMAGING_MODE_COLOR:
            int      bThreshold;
            int      velThreshold;
            uint32_t velocityMap[256];

            // read velThreshold and bThreshold (to support older formats)
            mUpsReaderUPtr->getBEThresholds(mImagingCaseID, velThreshold, bThreshold);
            mUpsReaderUPtr->getColorMap(mCineParams.colorMapIndex, mCineParams.isColorMapInverted, velocityMap);

            mCModeRenderer.setParams(mCineParams.converterParams,
                                     mCineParams.colorConverterParams,
                                     bThreshold,
                                     velThreshold,
                                     (uint8_t *)velocityMap,
                                     mCineParams.colorDopplerMode);
            mCModeRenderer.setTintAdjustment(r, g, b);
            mOffSCreenRenderSurface.addRenderer(&mCModeRenderer, mWinXPct, 100.0f - (mWinYPct + mWinHeightPct), mWinWidthPct, mWinHeightPct);
            break;

        case IMAGING_MODE_M:
            mBModeRenderer.setParams(mCineParams.converterParams);
            mBModeRenderer.setTintAdjustment(r, g, b);
            mOffSCreenRenderSurface.addRenderer(&mBModeRenderer, mWinXPct, 100.0f - (mWinYPct + mWinHeightPct), mWinWidthPct, mWinHeightPct);

            mSModeRenderer.setParams(floor(desiredTimeSpan*mCineParams.scrollSpeed),
                                     mCineParams.converterParams.numSamples,
                                     mCineParams.scrollSpeed,
                                     0.5f,  // hard-coded for M-mode
                                     mCineParams.targetFrameRate,
                                     mCineParams.mlinesPerFrame,
                                     mCineParams.frameRate,
                                     mCineParams.mlinesPerFrame,
                                     desiredTimeSpan,
                                     mCineParams.converterParams.numSamples * mCineParams.converterParams.sampleSpacingMm);  // Imaging Depth
            mSModeRenderer.setInvert(true);     // top to bottom: +ve direction
            mSModeRenderer.setTintAdjustment(r, g, b);
            mOffSCreenRenderSurface.addRenderer(&mSModeRenderer, mWinXPct2, 100.0f - (mWinYPct2 + mWinHeightPct2), mWinWidthPct2, mWinHeightPct2);
            break;

        case IMAGING_MODE_PW:
            mUpsReaderUPtr->getPwBaselineShiftFraction(mCineParams.dopplerBaselineShiftIndex, baselineShiftFracton);
            mSModeRenderer.setParams(floor(desiredTimeSpan*mCineParams.scrollSpeed), mCineParams.dopplerFFTSize, mCineParams.scrollSpeed,
                                      baselineShiftFracton, mCineParams.targetFrameRate, mCineParams.dopplerNumLinesPerFrame,
                                      mCineParams.frameRate, mCineParams.dopplerSamplesPerFrame, desiredTimeSpan, mCineParams.dopplerVelocityScale*2.0f);
            mSModeRenderer.setInvert(mCineParams.dopplerBaselineInvert);
            mSModeRenderer.setTintAdjustment(r, g, b);
            mSModeRenderer.setPeakMeanDrawing(mCineParams.dopplerPeakDraw, mCineParams.dopplerMeanDraw);
            mOffSCreenRenderSurface.addRenderer(&mSModeRenderer, mWinXPct2, 100.0f - (mWinYPct2 + mWinHeightPct2), mWinWidthPct2, mWinHeightPct2);
            break;

        case IMAGING_MODE_CW:
            mUpsReaderUPtr->getCwBaselineShiftFraction(mCineParams.dopplerBaselineShiftIndex, baselineShiftFracton);
            mSModeRenderer.setParams(floor(desiredTimeSpan*mCineParams.scrollSpeed), mCineParams.dopplerFFTSize, mCineParams.scrollSpeed,
                                      baselineShiftFracton, mCineParams.targetFrameRate, mCineParams.dopplerNumLinesPerFrame,
                                      mCineParams.frameRate, mCineParams.dopplerSamplesPerFrame, desiredTimeSpan, mCineParams.dopplerVelocityScale*2.0f, 2.0f);
            mSModeRenderer.setInvert(mCineParams.dopplerBaselineInvert);
            mSModeRenderer.setTintAdjustment(r, g, b);
            mSModeRenderer.setPeakMeanDrawing(mCineParams.dopplerPeakDraw, mCineParams.dopplerMeanDraw);
            mOffSCreenRenderSurface.addRenderer(&mSModeRenderer, mWinXPct2, 100.0f - (mWinYPct2 + mWinHeightPct2), mWinWidthPct2, mWinHeightPct2);
            break;
    }

    // Report if there's any error in UPS reader
    if ( 0 != mUpsReaderUPtr->getErrorCount() )
    {
        ALOGE("%s UPS error count was not zero on entry", __func__);
        retVal = TER_IE_UPS_READ_FAILURE;
        goto err_ret;
    }

    if (mIsEcgEnabled)
    {
        mEcgRenderer.setParams(mEcgRecordParams.scrDataWidth, mEcgRecordParams.samplesPerSecond, mTargetFrameRate,
                               mEcgRecordParams.samplesPerFrame, bFrameRate, desiredTimeSpan, mEcgRecordParams.decimationRatio);
        mOffSCreenRenderSurface.addRenderer(&mEcgRenderer, mWinXPct2, 100.0f - (mWinYPct2 + mWinHeightPct2), mWinWidthPct2, mWinHeightPct2);

        // For HeartRate rendering (when enabled)
        if (mRenderHR)
        {
            if (mWinHeightPct2 > 13.0f)
            {
                hrrYNorm = 200.0f;
            }

            mOffSCreenRenderSurface.addRenderer(&mHRRenderer, mWinXPct2 - 0.1f,
                                                              (100.0f - mWinYPct2) - ((14+18)/hrrYNorm * mWinHeightPct2),
                                                              2.9f,
                                                              3.0f);
            mHRRenderer.setTextSize(60.0f * mImgHeight / 1080.0f * 0.71f);
        }
    }

    if (mIsDaEnabled)
    {
        mDaRenderer.setParams(mDaRecordParams.scrDataWidth, mDaRecordParams.samplesPerSecond, mTargetFrameRate,
                              mDaRecordParams.samplesPerFrame, bFrameRate, desiredTimeSpan, mDaRecordParams.decimationRatio);
        mOffSCreenRenderSurface.addRenderer(&mDaRenderer, mWinXPct3, 100.0f - (mWinYPct3 + mWinHeightPct3), mWinWidthPct3, mWinHeightPct3);
    }

    if (addOverlay)
    {
        mOffSCreenRenderSurface.addRenderer(&mOverlayRenderer, 0.0f, 0.0f, 100.0f, 100.0f);     // for overlay
    }

    if (inputSurface == nullptr)
    {
        // still imaging mode
        mOffSCreenRenderSurface.open(mImgWidth, mImgHeight);
    }
    else
    {
        // clip mode
        mOffSCreenRenderSurface.open(inputSurface);
    }

    if ((mImagingMode == IMAGING_MODE_B) | (mImagingMode == IMAGING_MODE_M))
    {
        // set scale and pan parameters
        mBModeRenderer.setScale(mScale);
        mBModeRenderer.setPan(mDeltaX, mDeltaY);
        mBModeRenderer.setFlip(mFlipX, mFlipY);
    }
    else if (mImagingMode == IMAGING_MODE_COLOR)
    {
        mCModeRenderer.setScale(mScale);
        mCModeRenderer.setPan(mDeltaX, mDeltaY);
        mCModeRenderer.setFlip(mFlipX, mFlipY);

        // rebindColorMap
        mCModeRenderer.bindColorMap();
    }

    retVal = THOR_OK;

err_ret:
    return (retVal);
}

//-----------------------------------------------------------------------------
ThorStatus UltrasoundEncoder::prepareTransitionRendering(int transitionImgMode, float desiredTimeSpan)
{
    ThorStatus retVal = THOR_ERROR;
    float baselineShiftFracton;

    // Transition ImgMode should be differnet from ImagingMode
    if (transitionImgMode == mImagingMode)
        ALOGW("Transition ImagingMode and ImagingMode should be different!");

    switch (transitionImgMode)
    {
        case IMAGING_MODE_B:
            mBModeRenderer.setParams(mCineParams.converterParams);
            mOffSCreenRenderSurface.addRenderer(&mBModeRenderer, mWinXPct, 100.0f - (mWinYPct + mWinHeightPct), mWinWidthPct, mWinHeightPct);
            break;

        case IMAGING_MODE_COLOR:
            uint32_t                    velocityMap[256];   // empty CMap - will be updated
            mUpsReaderUPtr->getColorMap(mCineParams.colorMapIndex,
                                        mCineParams.isColorMapInverted, velocityMap);
            mCModeRenderer.setParams(mCineParams.converterParams, mCineParams.colorConverterParams,
                                     mCineParams.colorBThreshold, mCineParams.colorVelThreshold, (uint8_t*) velocityMap, mCineParams.colorDopplerMode);
            mOffSCreenRenderSurface.addRenderer(&mCModeRenderer, mWinXPct, 100.0f - (mWinYPct + mWinHeightPct), mWinWidthPct, mWinHeightPct);
            break;

        case IMAGING_MODE_PW:
            // set Params
            mUpsReaderUPtr->getPwBaselineShiftFraction(mCineParams.dopplerBaselineShiftIndex, baselineShiftFracton);
            mSModeRenderer.setParams(mCineParams.mSingleFrameWidth, mCineParams.dopplerFFTSize,
                                     mCineParams.scrollSpeed, baselineShiftFracton, mCineParams.targetFrameRate, mCineParams.dopplerNumLinesPerFrame,
                                     mCineParams.frameRate, mCineParams.dopplerSamplesPerFrame, desiredTimeSpan, mCineParams.dopplerVelocityScale*2.0f);
            mSModeRenderer.setInvert(mCineParams.dopplerBaselineInvert);
            mSModeRenderer.setPeakMeanDrawing(mCineParams.dopplerPeakDraw, mCineParams.dopplerMeanDraw);
            mOffSCreenRenderSurface.addRenderer(&mSModeRenderer, mWinXPct2, 100.0f - (mWinYPct2 + mWinHeightPct2), mWinWidthPct2, mWinHeightPct2);
            break;

        case IMAGING_MODE_CW:
            // set Params
            mUpsReaderUPtr->getPwBaselineShiftFraction(mCineParams.dopplerBaselineShiftIndex, baselineShiftFracton);
            mSModeRenderer.setParams(mCineParams.mSingleFrameWidth, mCineParams.dopplerFFTSize,
                                          mCineParams.scrollSpeed, baselineShiftFracton, mCineParams.targetFrameRate, mCineParams.dopplerNumLinesPerFrame,
                                          mCineParams.frameRate, mCineParams.dopplerSamplesPerFrame, desiredTimeSpan, mCineParams.dopplerVelocityScale*2.0f);
            mSModeRenderer.setInvert(mCineParams.dopplerBaselineInvert);
            mSModeRenderer.setPeakMeanDrawing(mCineParams.dopplerPeakDraw, mCineParams.dopplerMeanDraw);
            mOffSCreenRenderSurface.addRenderer(&mSModeRenderer, mWinXPct2, 100.0f - (mWinYPct2 + mWinHeightPct2), mWinWidthPct2, mWinHeightPct2);
            break;

        default:
            break;
    }

    return retVal;
}

//-----------------------------------------------------------------------------
ThorStatus UltrasoundEncoder::renderTransitionFrame(int transitionImgMode)
{
    ThorStatus retVal = THOR_ERROR;

    uint8_t *framePtr;
    uint8_t *framePtrColor;
    uint8_t *framePtrPw;
    uint8_t *framePtrCw;
    uint8_t *framePtrDopPM;
    uint32_t textureWidth;

    switch (transitionImgMode)
    {
        case IMAGING_MODE_B:
            //restore ZoomPanFlip
            mBModeRenderer.setScale(mCineParams.zoomPanParams[0]);
            mBModeRenderer.setPan(mCineParams.zoomPanParams[1], mCineParams.zoomPanParams[2]);
            mBModeRenderer.setFlip(mCineParams.zoomPanParams[3], mCineParams.zoomPanParams[4]);
            // set Frame
            framePtr = getFrame(0, DAU_DATA_TYPE_TEX);
            mBModeRenderer.setFrame(framePtr);
            break;

        case IMAGING_MODE_COLOR:
            // restore ZoomPanFlip
            mCModeRenderer.setScale(mCineParams.zoomPanParams[0]);
            mCModeRenderer.setPan(mCineParams.zoomPanParams[1], mCineParams.zoomPanParams[2]);
            mCModeRenderer.setFlip(mCineParams.zoomPanParams[3], mCineParams.zoomPanParams[4]);
            // re-bind colormap
            mCModeRenderer.bindColorMap();
            // set Frame
            framePtr = getFrame(0, DAU_DATA_TYPE_TEX);
            framePtrColor = getFrame(1, DAU_DATA_TYPE_TEX);
            mCModeRenderer.setFrame(framePtr, framePtrColor);
            break;

        case IMAGING_MODE_PW:
            mSModeRenderer.setSingleFrameTextureMode(true);
            mSModeRenderer.setSingleFrameDrawIndex(mCineParams.traceIndex);
            // set Frame
            framePtrPw = getFrame(0, DAU_DATA_TYPE_TEX);
            mSModeRenderer.prepareRendererOffScreen(0, 0, DAU_DATA_TYPE_PW);
            mSModeRenderer.setSingleFrameTextureMode(true);
            mSModeRenderer.setSingleFrameDrawIndex(mCineParams.traceIndex);
            textureWidth = getNumSamplesInFrameHeader(0, DAU_DATA_TYPE_TEX);
            mSModeRenderer.setSingleFrameTexturePtr(framePtrPw, textureWidth);
            if (mCineParams.dopplerPeakDraw || mCineParams.dopplerMeanDraw)
            {
                framePtrDopPM = getFrame(0, DAU_DATA_TYPE_DPMAX_SCR);
                mSModeRenderer.setSingleFramePeakMeanTexturePtr(framePtrDopPM, textureWidth);
            }
            break;

        case IMAGING_MODE_CW:
            mSModeRenderer.setSingleFrameTextureMode(true);
            mSModeRenderer.setSingleFrameDrawIndex(mCineParams.traceIndex);
            // set Frame
            framePtrCw = getFrame(0, DAU_DATA_TYPE_TEX);     // singleFrameTexture
            // CW-mode Renderer
            mSModeRenderer.prepareRendererOffScreen(0, 0, DAU_DATA_TYPE_CW);
            mSModeRenderer.setSingleFrameTextureMode(true);
            mSModeRenderer.setSingleFrameDrawIndex(mCineParams.traceIndex);
            textureWidth = getNumSamplesInFrameHeader(0, DAU_DATA_TYPE_TEX);
            mSModeRenderer.setSingleFrameTexturePtr(framePtrCw, textureWidth);
            if (mCineParams.dopplerPeakDraw || mCineParams.dopplerMeanDraw)
            {
                framePtrDopPM = getFrame(0, DAU_DATA_TYPE_DPMAX_SCR);
                mSModeRenderer.setSingleFramePeakMeanTexturePtr(framePtrDopPM, textureWidth);
            }
            break;

        default:
            break;
    }

    return retVal;
}

//-----------------------------------------------------------------------------
// TODO: not actively used.  Can be defucnted soon.
ThorStatus UltrasoundEncoder::
    processStillImage(const char *srcPath, float *zoomPanArray, float *winParams,
                      int winArrayLen, uint8_t *outPixels, int imgWidth, int imgHeight)
{
    ThorStatus retVal = THOR_ERROR;
    uint8_t *framePtr = nullptr;
    uint8_t *framePtrColor = nullptr;
    uint8_t *framePtrM = nullptr;
    float   *framePtrDa = nullptr;
    float   *framePtrEcg = nullptr;

    // set parameters
    mStartFrame = 0;
    mEndFrame = 0;
    mScale = zoomPanArray[0];
    mDeltaX = zoomPanArray[1];
    mDeltaY = zoomPanArray[2];
    mFlipX = zoomPanArray[3];
    mFlipY = zoomPanArray[4];

    mWinXPct = winParams[0];
    mWinYPct = winParams[1];
    mWinWidthPct = winParams[2];
    mWinHeightPct = winParams[3];

    if (winArrayLen > 7)
    {
        mWinXPct2 = winParams[4];
        mWinYPct2 = winParams[5];
        mWinWidthPct2 = winParams[6];
        mWinHeightPct2 = winParams[7];
    }

    if (winArrayLen > 11)
    {
        mWinXPct3 = winParams[8];
        mWinYPct3 = winParams[9];
        mWinWidthPct3 = winParams[10];
        mWinHeightPct3 = winParams[11];
    }

    mImgWidth = imgWidth;
    mImgHeight = imgHeight;

    retVal = processInput(srcPath, false);
    if (IS_THOR_ERROR(retVal))
    {
        ALOGE("Error in Processing Input File.");
        goto err_ret;
    }

    retVal = prepareRenderSurface(nullptr, false);
    if (IS_THOR_ERROR(retVal))
    {
        ALOGE("Error in Preparing RenderSurface.");
        goto err_ret;
    }

    switch (mImagingMode)
    {
        case IMAGING_MODE_B:
            // frame data pointer
            framePtr = getFrame(0, DAU_DATA_TYPE_B);
            mBModeRenderer.setFrame(framePtr);
            break;

        case IMAGING_MODE_COLOR:
            framePtr = getFrame(0, DAU_DATA_TYPE_B);
            framePtrColor = getFrame(0, DAU_DATA_TYPE_COLOR);
            mCModeRenderer.setFrame(framePtr, framePtrColor);
            break;

        case IMAGING_MODE_M:
            framePtr = getFrame(0, DAU_DATA_TYPE_B);
            framePtrM = getFrame(0, DAU_DATA_TYPE_M, true);
            mBModeRenderer.setFrame(framePtr);
            mSModeRenderer.setSingleFrameTexturePtr(framePtrM, mCineParams.mSingleFrameWidth);
            break;
    }

    if (mIsDaEnabled)
    {
        mDaRenderer.setSingleFrameFrameWidth(mDaRecordParams.frameWidth);
        mDaRenderer.setSingleFrameFreezeMode(true);
        mDaRenderer.setSingleFrameTraceIndex(mDaRecordParams.traceIndex);

        mDaRenderer.prepareRendererOffScreen(0, 0, DAU_DATA_TYPE_DA);

        mDaRenderer.setLineColor(mDALineColor);
        mDaRenderer.setLineThickness(DA_ECG_LINE_THICKNESS_ENC);

        framePtrDa = (float*) getFrame(0, DAU_DATA_TYPE_DA_SCR);
        mDaRenderer.prepareFreezeModeOffScreen(framePtrDa);
    }

    if (mIsEcgEnabled)
    {
        mEcgRenderer.setSingleFrameFrameWidth(mEcgRecordParams.frameWidth);
        mEcgRenderer.setSingleFrameFreezeMode(true);
        mEcgRenderer.setSingleFrameTraceIndex(mEcgRecordParams.traceIndex);

        mEcgRenderer.prepareRendererOffScreen(0, 0, DAU_DATA_TYPE_ECG);

        mEcgRenderer.setLineColor(mECGLineColor);
        mEcgRenderer.setLineThickness(DA_ECG_LINE_THICKNESS_ENC);

        framePtrEcg = (float*) getFrame(0, DAU_DATA_TYPE_ECG_SCR);
        mEcgRenderer.prepareFreezeModeOffScreen(framePtrEcg);
    }

    // Take Snapshot
    mOffSCreenRenderSurface.takeScreenShotToRGBAPtrUD(outPixels);

    retVal = THOR_OK;

err_ret:
    close();
    return (retVal);

}

//-----------------------------------------------------------------------------
ThorStatus UltrasoundEncoder::
    encodeStillImage(const char *srcPath, const char *dstPath, float *zoomPanArray, float *winParams,
                     int winArrayLen, uint8_t *overlayImg, int imgWidth, int imgHeight, int desiredFrameNum,
                     bool rawOut)
{
    ThorStatus retVal = THOR_OK;

    if (!getBusyLock())
    {
        retVal = TER_MISC_BUSY;
        goto err_ret;
    }

    // set parameters
    mInputFilePath = srcPath;
    mOutputFilePath = dstPath;
    mStartFrame = 0;
    mEndFrame = 0;
    mScale = zoomPanArray[0];
    mDeltaX = zoomPanArray[1];
    mDeltaY = zoomPanArray[2];
    mFlipX = zoomPanArray[3];
    mFlipY = zoomPanArray[4];

    mWinXPct = winParams[0];
    mWinYPct = winParams[1];
    mWinWidthPct = winParams[2];
    mWinHeightPct = winParams[3];

    if (winArrayLen > 7)
    {
        mWinXPct2 = winParams[4];
        mWinYPct2 = winParams[5];
        mWinWidthPct2 = winParams[6];
        mWinHeightPct2 = winParams[7];
    }

    if (winArrayLen > 11)
    {
        mWinXPct3 = winParams[8];
        mWinYPct3 = winParams[9];
        mWinWidthPct3 = winParams[10];
        mWinHeightPct3 = winParams[11];
    }

    mImgWidth = imgWidth;
    mImgHeight = imgHeight;

    mOverlayImgPrt = overlayImg;

    mStillFrameNum = desiredFrameNum;
    mEndFrame = desiredFrameNum;

    mIsRawOut = rawOut;
    // set encodeStill Event
    mEncodeStillEvent.set();

err_ret:
    return (retVal);
}

//-----------------------------------------------------------------------------
ThorStatus UltrasoundEncoder::
    encodeClip(const char *srcPath, const char *dstPath, float *zoomPanArray, float *winParams,
               int winArrayLen, uint8_t *overlayImg, int imgWidth, int imgHeight, int startFrame,
               int endFrame, bool rawOut, bool forcedFSR)
{
    ThorStatus retVal = THOR_OK;

    if (!getBusyLock())
    {
        retVal = TER_MISC_BUSY;
        goto err_ret;
    }

    // set parameters
    mInputFilePath = srcPath;
    mOutputFilePath = dstPath;
    mStartFrame = startFrame;
    mEndFrame = endFrame;
    mScale = zoomPanArray[0];
    mDeltaX = zoomPanArray[1];
    mDeltaY = zoomPanArray[2];
    mFlipX = zoomPanArray[3];
    mFlipY = zoomPanArray[4];

    mWinXPct = winParams[0];
    mWinYPct = winParams[1];
    mWinWidthPct = winParams[2];
    mWinHeightPct = winParams[3];

    if (winArrayLen > 7)
    {
        mWinXPct2 = winParams[4];
        mWinYPct2 = winParams[5];
        mWinWidthPct2 = winParams[6];
        mWinHeightPct2 = winParams[7];
    }

    if (winArrayLen > 11)
    {
        mWinXPct3 = winParams[8];
        mWinYPct3 = winParams[9];
        mWinWidthPct3 = winParams[10];
        mWinHeightPct3 = winParams[11];
    }

    mImgWidth = imgWidth;
    mImgHeight = imgHeight;

    LOGE("mImgWidth : %d, mImgHeight : %d",mImgWidth,mImgHeight);

    mOverlayImgPrt = overlayImg;

    if (rawOut)
        mIsRawOut = true;
    else
        mIsRawOut = false;

    mIsForcedFSR = forcedFSR;

    mEncodeClipEvent.set();

err_ret:
    return (retVal);
}

//-----------------------------------------------------------------------------
uint8_t* UltrasoundEncoder::getFrame(uint32_t frameNum, uint32_t dataType, bool isSFT)
{
    if ((mStartFrame > frameNum || frameNum > mEndFrame) && (dataType != DAU_DATA_TYPE_TEX))
    {
        LOGE("FrameNum %u out of bounds [%u,%u]", frameNum, mStartFrame, mEndFrame);
        return nullptr;
    }
    if (mUseCapnpReader)
    {
        // Read Capnp frames directly from the Capnp Reader
        auto clip = mCapnpReader->getRoot<echonous::capnp::ThorClip>();
        uint8_t* tmp_ptr;

        auto checkDataSize = [](const capnp::Data::Reader &data, size_t size) -> uint8_t* {
            if (data.size() != size) { LOGE("Bad data size (expected %zu, got %zu)", size, data.size()); return nullptr; }
            return const_cast<uint8_t*>(data.begin());
        };
        switch (dataType)
        {
            // TODO: If any of the structs in the file are missing, they will fill in with default values
            // For types containing "Data" such as the raw frames, they will return a nullptr
            // Check this and fail somehow!!
            case DAU_DATA_TYPE_B:
                return checkDataSize(clip.getBmode().getRawFrames()[frameNum].getFrame(), MAX_B_FRAME_SIZE);
            case DAU_DATA_TYPE_COLOR:
                return checkDataSize(clip.getCmode().getRawFrames()[frameNum].getFrame(), MAX_COLOR_FRAME_SIZE);
            case DAU_DATA_TYPE_M:
                if (!isSFT)
                    return checkDataSize(clip.getMmode().getRawFrames()[frameNum].getFrame(), MAX_M_FRAME_SIZE);
                else
                    return checkDataSize(clip.getMmode().getRawFrames()[frameNum].getFrame(), MAX_TEX_FRAME_SIZE);
            case DAU_DATA_TYPE_PW:
                if (!isSFT)
                    return checkDataSize(clip.getPwmode().getRawFrames()[frameNum].getFrame(), MAX_PW_FRAME_SIZE);
                else
                    return checkDataSize(clip.getPwmode().getRawFrames()[frameNum].getFrame(), MAX_TEX_FRAME_SIZE);
            case DAU_DATA_TYPE_CW:
                if (!isSFT)
                    return checkDataSize(clip.getCwmode().getRawFrames()[frameNum].getFrame(), MAX_CW_FRAME_SIZE);
                else
                    return checkDataSize(clip.getCwmode().getRawFrames()[frameNum].getFrame(), MAX_TEX_FRAME_SIZE);
            case DAU_DATA_TYPE_PW_ADO:
                return checkDataSize(clip.getPwaudio().getRawFrames()[frameNum].getFrame(), MAX_PW_ADO_FRAME_SIZE);
            case DAU_DATA_TYPE_CW_ADO:
                return checkDataSize(clip.getCwaudio().getRawFrames()[frameNum].getFrame(), MAX_CW_ADO_FRAME_SIZE);
            case DAU_DATA_TYPE_DA:
                tmp_ptr = checkDataSize(clip.getDa().getRawFrames()[frameNum].getFrame(), MAX_DA_FRAME_SIZE);
                if (tmp_ptr == nullptr)
                    tmp_ptr = checkDataSize(clip.getDa().getRawFrames()[frameNum].getFrame(), MAX_DA_FRAME_SIZE_LEGACY);
                return tmp_ptr;
            case DAU_DATA_TYPE_DA_SCR:
                return checkDataSize(clip.getDa().getRawFrames()[frameNum].getFrame(), MAX_DA_SCR_FRAME_SIZE);
            case DAU_DATA_TYPE_ECG:
                return checkDataSize(clip.getEcg().getRawFrames()[frameNum].getFrame(), MAX_ECG_FRAME_SIZE);
            case DAU_DATA_TYPE_ECG_SCR:
                return checkDataSize(clip.getEcg().getRawFrames()[frameNum].getFrame(), MAX_ECG_SCR_FRAME_SIZE);
            case DAU_DATA_TYPE_TEX: // TransitionFrame is stored in Tex memory
                return checkDataSize(clip.getTransitionframe().getRawFrames()[frameNum].getFrame(), MAX_TEX_FRAME_SIZE);
            case DAU_DATA_TYPE_DOP_PM: // Doppler Peak Mean frames
                return checkDataSize(clip.getDopplerpeakmean().getRawFrames()[frameNum].getFrame(), MAX_DOP_PEAK_MEAN_SIZE);
            case DAU_DATA_TYPE_DPMAX_SCR: // Doppler Peak Max Screen Frame
                return checkDataSize(clip.getDopplerpeakmean().getRawFrames()[frameNum].getFrame(), MAX_DOP_PEAK_MEAN_SCR_SIZE);
            default:
                LOGE("Unsupported data type in UltrasoundEncoder: 0x%08x", dataType);
                return nullptr;
        }
    }
    else /* Not Capnp */
    {
        switch (dataType) {
            case DAU_DATA_TYPE_B:
                return mBaseAddressPtr + (mCineFrameHeaderSize + MAX_B_FRAME_SIZE) * frameNum;
            case DAU_DATA_TYPE_COLOR:
                return mBaseAddressPtrColor +
                       (mCineFrameHeaderSize + MAX_COLOR_FRAME_SIZE) * frameNum;
            case DAU_DATA_TYPE_M: /* fallthrough */
            case DAU_DATA_TYPE_TEX:
                return mBaseAddressPtrM + (mCineFrameHeaderSize * MAX_M_FRAME_SIZE) * frameNum;
            case DAU_DATA_TYPE_DA:
                return mBaseAddressPtrDa + (mCineFrameHeaderSize + MAX_DA_FRAME_SIZE_LEGACY) * frameNum;
            case DAU_DATA_TYPE_DA_SCR:
                return mBaseAddressPtrDa + (mCineFrameHeaderSize + MAX_DA_SCR_FRAME_SIZE) * frameNum;
            case DAU_DATA_TYPE_ECG:
                return mBaseAddressPtrEcg + (mCineFrameHeaderSize + MAX_ECG_FRAME_SIZE) * frameNum;
            case DAU_DATA_TYPE_ECG_SCR:
                return mBaseAddressPtrEcg + (mCineFrameHeaderSize + MAX_ECG_SCR_FRAME_SIZE) * frameNum;
            default:
                LOGE("Unsupported data type in UltrasoundEncoder: 0x%08x", dataType);
                return nullptr;
        }
    }
}

//-----------------------------------------------------------------------------
uint32_t UltrasoundEncoder::getNumSamplesInFrameHeader(uint32_t frameNum, uint32_t dataType)
{
    if ((mStartFrame > frameNum || frameNum > mEndFrame))
    {
        LOGE("FrameNum %u out of bounds [%u,%u]", frameNum, mStartFrame, mEndFrame);
        return 0;
    }
    if (!mUseCapnpReader)
    {
        if (dataType == DAU_DATA_TYPE_M) {
            // To support M-mode data in older formats
            return mCineParams.mSingleFrameWidth;
        }
        else {
            LOGE("Should be in Capnp format!");
            return 0;
        }
    }

    // Read Capnp frames directly from the Capnp Reader
    auto clip = mCapnpReader->getRoot<echonous::capnp::ThorClip>();

    switch (dataType)
    {
        // TODO: If any of the structs in the file are missing, they will fill in with default values
        case DAU_DATA_TYPE_M:
            return clip.getMmode().getRawFrames()[frameNum].getHeader().getNumSamples();
        case DAU_DATA_TYPE_PW:
            return clip.getPwmode().getRawFrames()[frameNum].getHeader().getNumSamples();
        case DAU_DATA_TYPE_CW:
            return clip.getCwmode().getRawFrames()[frameNum].getHeader().getNumSamples();
        case DAU_DATA_TYPE_TEX:
            return clip.getTransitionframe().getRawFrames()[frameNum].getHeader().getNumSamples();

        default:
            return 0;
    }
}

//-----------------------------------------------------------------------------
float UltrasoundEncoder::getHeartRateInFrameHeader(uint32_t frameNum)
{
    float heartRate = 0.0f;

    if ((mStartFrame > frameNum || frameNum > mEndFrame))
    {
        LOGE("FrameNum %u out of bounds [%u,%u]", frameNum, mStartFrame, mEndFrame);
        return heartRate;
    }

    if (mUseCapnpReader)
    {
        auto clip = mCapnpReader->getRoot<echonous::capnp::ThorClip>();
        heartRate = clip.getEcg().getRawFrames()[frameNum].getHeader().getHeartRate();
    }
    else
    {
        // to support older version
        uint8_t* framePtrEcg = getFrame(frameNum, DAU_DATA_TYPE_ECG);
        uint8_t* frameHDRPtr = ((uint8_t*) framePtrEcg) -  mCineFrameHeaderSize;
        CineBuffer::CineFrameHeader* cineHeaderPtr =
                reinterpret_cast<CineBuffer::CineFrameHeader *>(frameHDRPtr);

        heartRate = cineHeaderPtr->heartRate;
    }

    return heartRate;
}

//-----------------------------------------------------------------------------
ThorStatus UltrasoundEncoder::encodeClipCore()
{
    ELAPSED_FUNC();

    // prepare clipEncodercore
    ClipEncoderCore *clipEnc = nullptr;
    ThorStatus retVal = THOR_ERROR;
    uint8_t* framePtr;
    uint8_t* framePtrColor;
    uint8_t* framePtrM;
    uint8_t* framePtrPw;
    uint8_t* framePtrCw;
    uint8_t* framePtrDopPM;
    float*   framePtrDa;
    float*   framePtrEcg;
    float*   framePtrADO;


    uint32_t frameIdx = 0;
    bool     keepLooping = true;
    int      numFramesToEncode = 0;
    int      numEncodedFrame = 0;
    uint32_t numSamplesInFrame;
    float    heartRate = -1.0f;
    bool     encodeScroll = false;
    bool     encodeAudio = false;
    bool     encodeDopPeakMean = false;

    // time tracker - encoder and acquisition
    float   encTime = 0.0f;
    float   acqTime = 0.0f;
    float   clipDuration;

    float   encInterval;
    float   acqInterval;

    // DA Audio related variables
    uint64_t audioPresentationTime;
    uint64_t audioFrameIntervalUs;
    float    inSampleRate;
    uint32_t inSamplesPerFrame;
    float    outSampleRate = 8000.0f;       // audio samplerate
    uint32_t outSamplesPerFrame;
    int      lastSamplesPerFrame;
    int      numAudioChannels;
    int      audioBitRate = 96000;

    // File Descriptor for RawOutput
    FILE*   outFile;
    u_char* frameImg;

    AudioCubicReSampler*  audioReSampleBuffer;
    short* inAudioData;

    retVal = processInput(mInputFilePath.c_str());
    if (IS_THOR_ERROR(retVal))
    {
        ALOGE("Error in Processing Input File.");
        goto err_ret;
    }

    // prepare start and end index for clip encoding
    if (mStartFrame < 1)
    {
        mStartFrame = 0;
    }

    if ((mEndFrame < 1) || (mEndFrame > mFrameCount - 1))
    {
        mEndFrame = mFrameCount - 1;
    }

    numFramesToEncode = mEndFrame - mStartFrame + 1;
    frameIdx = mStartFrame;

    // minimum number of frames = 2
    if ((mFrameCount < 2) || (numFramesToEncode < 2))
    {
        ALOGE("Not Enough Frames to Encode a Clip.");
        retVal = TER_MISC_PARAM;
        goto err_ret;
    }

    // encoding interval
    encInterval = 1000.0f / ((float) mTargetFrameRate);
    // acquisition interval (ideally == mFrameInterval)
    acqInterval = 1000.0f / mCineParams.frameRate;

    if (abs(acqInterval - ((float) mFrameInterval) ) > 1.0f )
    {
        ALOGW("acquisition interval does not match with FrameInterval: %f, %d", acqInterval, mFrameInterval);
    }

    if (acqInterval > 350.0f || acqInterval < 1.0f)
    {
        ALOGE("acquisition interval out of bounds: acqInterval: %f", acqInterval);
        retVal = TER_MISC_PARAM;
        goto err_ret;
    }

    if (encInterval > acqInterval)
    {
        ALOGW("encInterval: %f > acqInterval: %f.  Please verify!!!", encInterval, acqInterval);
        encInterval = acqInterval;
    }

    // when waveform or scroll renderer is not enabled, set encInterval to acqInterval
    encodeScroll = mIsDaEnabled || mIsEcgEnabled || (mImagingMode == IMAGING_MODE_PW) ||
            (mImagingMode == IMAGING_MODE_CW) || (mImagingMode == IMAGING_MODE_M);
    if (!encodeScroll)
    {
        encInterval = acqInterval;
    }

    // check whether encode Audio
    encodeAudio = mIsDaEnabled || (mImagingMode == IMAGING_MODE_PW) || (mImagingMode == IMAGING_MODE_CW);

    if (((mImagingMode == IMAGING_MODE_PW) || (mImagingMode == IMAGING_MODE_CW)) &&
            (mCineParams.dopplerPeakDraw || mCineParams.dopplerMeanDraw))
        encodeDopPeakMean = true;


    if (mIsRawOut)
    {
        // no audio for Rawout
        encodeAudio = false;
        // allocate frameImg
        frameImg = new u_char[mImgWidth * mImgHeight * 3];

        // open output file
        outFile = fopen(mOutputFilePath.c_str(), "wb");
        if (outFile == NULL)
        {
            ALOGE("Unable to create raw output file: %s", mOutputFilePath.c_str());
            goto err_ret;
        }
    }

    // calculate audio encoding params
    if (encodeAudio)
    {
        // DA audio related variables
        audioPresentationTime = (uint64_t) 0;
        audioFrameIntervalUs = (uint64_t) (acqInterval * 1000.0f);

        if ((mImagingMode == IMAGING_MODE_PW) || (mImagingMode == IMAGING_MODE_CW))
        {
            inSamplesPerFrame = mCineParams.dopplerSamplesPerFrame *
                                mCineParams.dopplerAudioUpsampleRatio;

            // PW/CW audio encoding
            if (mImagingMode == IMAGING_MODE_CW)
                inSamplesPerFrame = inSamplesPerFrame/mCineParams.cwDecimationFactor;

            inSampleRate = inSamplesPerFrame * mCineParams.frameRate;

            if (inSampleRate > 44100.0f)
                outSampleRate = 44100.0f;
            else if (inSampleRate > 22050.0f)
                outSampleRate = 22050.0f;
            else if (inSampleRate > 11025.0f)
                outSampleRate = 11025.0f;
            else
                outSampleRate = 8000.0f;

            numAudioChannels = 2;
        }
        else
        {
            // DA Audio
            inSamplesPerFrame = mDaRecordParams.samplesPerFrame;
            inSampleRate = mDaRecordParams.samplesPerSecond;
            outSampleRate = (float) DA_AUDIO_OUT_SAMPLERATE;

            numAudioChannels = 1;
        }

        outSamplesPerFrame = ceil(outSampleRate/ mCineParams.frameRate);
        audioReSampleBuffer = new AudioCubicReSampler(inSampleRate, outSampleRate, numAudioChannels);

        ALOGD("Encode-Audio: inSamplesPerFrame(%u), InSampleRate(%f), OutSampleRate(%f), OutSamplesPerFrame(%d)",
              inSamplesPerFrame, inSampleRate, outSampleRate, outSamplesPerFrame);

        // ReSampler Output -> 2 channel
        inAudioData = new short[outSamplesPerFrame * 2];
    }

    if (mIsRawOut)
    {
        retVal = prepareRenderSurface(nullptr, true);  // need to add overlay
    }
    else
    {
        // setup a ClipEncoder
        // TODO: Encoder Params - bitrate, Iframe Interval
        clipEnc = new ClipEncoderCore(mImgWidth, mImgHeight, 5000000, 1000.0f / encInterval, 5,
                                      (int) outSampleRate, audioBitRate, mOutputFilePath.c_str(), encodeAudio);
        retVal = prepareRenderSurface(clipEnc->getInputSurface(), true);  // need to add overlay
    }

    // check error in preparing render surface
    if (IS_THOR_ERROR(retVal))
    {
        ALOGE("Error in Preparing RenderSurface.");
        goto err_ret;
    }

    // forced full screen render for scroll mode
    if (mIsForcedFSR)
        mSModeRenderer.setForcedFullScreenRender(true);

    // overlay texture
    mOverlayRenderer.setTextureBmpObjAlphaX(mOverlayImgPrt, 3);

    if (mIsDaEnabled)
    {
        mDaRenderer.setSingleFrameFreezeMode(false);
        mDaRenderer.prepareRendererOffScreen(mStartFrame, mEndFrame, DAU_DATA_TYPE_DA);

        mDaRenderer.setLineColor(mDALineColor);
        mDaRenderer.setLineThickness(DA_ECG_LINE_THICKNESS_ENC);
    }

    if (mIsEcgEnabled)
    {
        mEcgRenderer.setSingleFrameFreezeMode(false);
        mEcgRenderer.prepareRendererOffScreen(mStartFrame, mEndFrame, DAU_DATA_TYPE_ECG);

        mEcgRenderer.setLineColor(mECGLineColor);
        mEcgRenderer.setLineThickness(DA_ECG_LINE_THICKNESS_ENC);
    }

    // encode time in ms
    clipDuration = acqInterval * ((float) numFramesToEncode);

    ALOGD("Encode interval: %f ms, acquisition interval: %f, clip duration: %f", encInterval,
           acqInterval, clipDuration);

    if (!mIsRawOut)
        clipEnc->drainEncoder(false);

    // drainEncoder for format change
    if (encodeAudio)
    {
        // drainAudioEncoder for format change
        clipEnc->drainAudioEncoder(false);
    }

    // Render Transition Frame
    if (mCineParams.isTransitionRenderingReady)
        renderTransitionFrame(mCineParams.renderedTransitionImgMode);

    while ((encTime < clipDuration) && (keepLooping))
    {
        if (encTime >= acqTime)
        {
            // renderer imaging screen.
            switch (mImagingMode)
            {
                case IMAGING_MODE_B:
                    // frame data pointer
                    framePtr = getFrame(frameIdx, DAU_DATA_TYPE_B);
                    mBModeRenderer.setFrame(framePtr);
                    break;

                case IMAGING_MODE_COLOR:
                    framePtr = getFrame(frameIdx, DAU_DATA_TYPE_B);
                    framePtrColor = getFrame(frameIdx, DAU_DATA_TYPE_COLOR);
                    mCModeRenderer.setFrame(framePtr, framePtrColor);
                    break;

                case IMAGING_MODE_M:
                    framePtr = getFrame(frameIdx, DAU_DATA_TYPE_B);
                    framePtrM = getFrame(frameIdx, DAU_DATA_TYPE_M);
                    mBModeRenderer.setFrame(framePtr);
                    mSModeRenderer.setFrame(framePtrM, mCineParams.mlinesPerFrame);
                    break;

                case IMAGING_MODE_PW:
                    framePtrPw = getFrame(frameIdx, DAU_DATA_TYPE_PW);
                    numSamplesInFrame = getNumSamplesInFrameHeader(frameIdx, DAU_DATA_TYPE_PW);
                    mSModeRenderer.setFrame(framePtrPw, numSamplesInFrame);
                    if (encodeDopPeakMean)
                    {
                        framePtrDopPM = getFrame(frameIdx, DAU_DATA_TYPE_DOP_PM);
                        mSModeRenderer.setPeakMeanFrame(framePtrDopPM, numSamplesInFrame);
                    }
                    if (encodeAudio)
                        framePtrADO = (float*) getFrame(frameIdx, DAU_DATA_TYPE_PW_ADO);
                    break;

                case IMAGING_MODE_CW:
                    framePtrCw = getFrame(frameIdx, DAU_DATA_TYPE_CW);
                    numSamplesInFrame = getNumSamplesInFrameHeader(frameIdx, DAU_DATA_TYPE_CW);
                    mSModeRenderer.setFrame(framePtrCw, numSamplesInFrame);
                    if (encodeDopPeakMean)
                    {
                        framePtrDopPM = getFrame(frameIdx, DAU_DATA_TYPE_DOP_PM);
                        mSModeRenderer.setPeakMeanFrame(framePtrDopPM, numSamplesInFrame);
                    }
                    if (encodeAudio)
                        framePtrADO = (float*) getFrame(frameIdx, DAU_DATA_TYPE_CW_ADO);
                    break;
            }

            // Da/Ecg drawing
            if (mIsDaEnabled)
            {
                framePtrDa = (float*) getFrame(frameIdx, DAU_DATA_TYPE_DA);
                mDaRenderer.setFrame(framePtrDa);

                if (encodeAudio)
                    framePtrADO = framePtrDa;
            }
            if (mIsEcgEnabled)
            {
                framePtrEcg = (float*) getFrame(frameIdx, DAU_DATA_TYPE_ECG);
                mEcgRenderer.setFrame(framePtrEcg);

                if (mRenderHR)
                {
                    heartRate = getHeartRateInFrameHeader(frameIdx);
                    mHRRenderer.setHeartRate(heartRate);
                }
            }

            // encoding audio data
            if (encodeAudio)
            {
                // audio encoding
                audioReSampleBuffer->putDataFloat(framePtrADO, inSamplesPerFrame);

                if (frameIdx > AUDIO_FRAME_ENCODE_DELAY)
                {
                    audioReSampleBuffer->getData(inAudioData, outSamplesPerFrame);
                    clipEnc->putAudioData(inAudioData, outSamplesPerFrame*2*2, audioPresentationTime,
                            audioFrameIntervalUs, false);  // 2ch short (*4)
                    audioPresentationTime += audioFrameIntervalUs;
                }
            }

            // update index
            frameIdx++;

            if (frameIdx > mEndFrame)
                frameIdx = mEndFrame;

            // next acqTime
            acqTime += acqInterval;

            mOverlayRenderer.setScissorUpdate(true);
        }

        // S8 case
        if (mIsForcedFSR)
        {
            if (mCineParams.isTransitionRenderingReady)
                renderTransitionFrame(mCineParams.renderedTransitionImgMode);
        }

        if (mCancelRequested)
        {
            keepLooping = false;
        }

        if (mIsRawOut)
        {
            // draw/render and get screenshot
            mOffSCreenRenderSurface.takeScreenShotToRGBPtrUD(frameImg);

            // write the frameImg to the output file
            fwrite(frameImg, sizeof(u_char), mImgWidth * mImgHeight * 3, outFile);
        }
        else
        {
            // set encoder bit and draw
            clipEnc->drainEncoder(false);
            mOffSCreenRenderSurface.draw();
        }

        numEncodedFrame++;
        encTime += encInterval;
        // make this true only for ForcedFSR scroll mode
        mOverlayRenderer.setScissorUpdate(mIsForcedFSR);
    }

    if (!mIsRawOut)
        clipEnc->drainEncoder(true);

    if (encodeAudio)
    {
        // to make sure all the audio samples are encoded.
        while(audioReSampleBuffer->getStoredNumFrames() > outSamplesPerFrame)
        {
            audioReSampleBuffer->getData(inAudioData, outSamplesPerFrame);
            clipEnc->putAudioData(inAudioData, outSamplesPerFrame*2*2, audioPresentationTime,
                    audioFrameIntervalUs, false);
            audioPresentationTime += audioFrameIntervalUs;
        }

        // drain audioEncoder
        clipEnc->drainAudioEncoder(false);

        // last samples and frame
        lastSamplesPerFrame = audioReSampleBuffer->getStoredNumFrames();
        audioReSampleBuffer->getData(inAudioData, lastSamplesPerFrame);
        clipEnc->putAudioData(inAudioData, lastSamplesPerFrame*2*2, audioPresentationTime, audioFrameIntervalUs, true);

        // last frame with EoS flag
        clipEnc->drainAudioEncoder(true);
    }

    ALOGD("NumFramesToEncode: %d, Actual Encoded Frames: %d", numFramesToEncode, numEncodedFrame);

    retVal = THOR_OK;

err_ret:
    if (clipEnc != nullptr)
        delete clipEnc;

    if (encodeAudio)
    {
        delete[] inAudioData;
        delete audioReSampleBuffer;
    }

    if (mIsRawOut)
    {
        delete[] frameImg;

        // close output file
        if (outFile != NULL) {
            fclose(outFile);
        }
    }

    // set Forced Mode to False
    mSModeRenderer.setForcedFullScreenRender(false);

    close();
    return retVal;
}

//-----------------------------------------------------------------------------
ThorStatus UltrasoundEncoder::encodeStillCore() {
    // encodeStill core
    ThorStatus retVal = THOR_ERROR;
    uint8_t *framePtr;
    uint8_t *framePtrColor;
    uint8_t *framePtrM;
    uint8_t *framePtrPw;
    uint8_t *framePtrCw;
    uint8_t *framePtrDopPM;
    float   *framePtrDa;
    float   *framePtrEcg;
    uint32_t textureWidth;
    uint32_t encodeFrameNum;

    // File Descriptor for RawOutput
    FILE*   outFile;
    u_char* frameImg;

    retVal = processInput(mInputFilePath.c_str());
    if (IS_THOR_ERROR(retVal)) {
        ALOGE("Error in Processing Input File.");
        goto err_ret;
    }

    retVal = prepareRenderSurface(nullptr, true); // need to add overlay
    if (IS_THOR_ERROR(retVal)) {
        ALOGE("Error in Preparing RenderSurface.");
        goto err_ret;
    }

    // Render Transition Frame
    if (mCineParams.isTransitionRenderingReady)
        renderTransitionFrame(mCineParams.renderedTransitionImgMode);

    // overlay texture
    mOverlayRenderer.setTextureBmpObjAlphaX(mOverlayImgPrt, 3);

    encodeFrameNum = mStillFrameNum;

    if (encodeFrameNum > (mFrameCount - 1))
        encodeFrameNum = (mFrameCount - 1);

    // adjust endframe number
    if (encodeFrameNum > mEndFrame)
        mEndFrame = encodeFrameNum;

    ALOGD("Still Image Encoded with frameNum: %d", encodeFrameNum);

    switch (mImagingMode) {
        case IMAGING_MODE_B:
            // frame data pointer
            framePtr = getFrame(encodeFrameNum, DAU_DATA_TYPE_B);
            mBModeRenderer.setFrame(framePtr);
            break;

        case IMAGING_MODE_COLOR:
            framePtr = getFrame(encodeFrameNum, DAU_DATA_TYPE_B);
            framePtrColor = getFrame(encodeFrameNum, DAU_DATA_TYPE_COLOR);
            mCModeRenderer.setFrame(framePtr, framePtrColor);
            break;

        case IMAGING_MODE_M:
            framePtr = getFrame(0, DAU_DATA_TYPE_B);
            framePtrM = getFrame(0, DAU_DATA_TYPE_M, true);     // singleFrameTexture
            mBModeRenderer.setFrame(framePtr);
            // M-mode Renderer
            mSModeRenderer.setSingleFrameTextureMode(true);
            mSModeRenderer.setSingleFrameDrawIndex(mCineParams.traceIndex);
            mSModeRenderer.prepareRendererOffScreen(0, 0, DAU_DATA_TYPE_M);
            textureWidth = getNumSamplesInFrameHeader(0, DAU_DATA_TYPE_M);
            mSModeRenderer.setSingleFrameTexturePtr(framePtrM, textureWidth);
            break;

        case IMAGING_MODE_PW:
            framePtrPw = getFrame(0, DAU_DATA_TYPE_PW, true);     // singleFrameTexture
            // PW-mode Renderer
            mSModeRenderer.prepareRendererOffScreen(0, 0, DAU_DATA_TYPE_PW);
            mSModeRenderer.setSingleFrameTextureMode(true);
            mSModeRenderer.setSingleFrameDrawIndex(mCineParams.traceIndex);
            textureWidth = getNumSamplesInFrameHeader(0, DAU_DATA_TYPE_PW);
            mSModeRenderer.setSingleFrameTexturePtr(framePtrPw, textureWidth);
            if (mCineParams.dopplerPeakDraw || mCineParams.dopplerMeanDraw)
            {
                framePtrDopPM = getFrame(0, DAU_DATA_TYPE_DPMAX_SCR);
                mSModeRenderer.setSingleFramePeakMeanTexturePtr(framePtrDopPM, textureWidth);
            }
            break;

        case IMAGING_MODE_CW:
            framePtrCw = getFrame(0, DAU_DATA_TYPE_CW, true);     // singleFrameTexture
            // CW-mode Renderer
            mSModeRenderer.prepareRendererOffScreen(0, 0, DAU_DATA_TYPE_CW);
            mSModeRenderer.setSingleFrameTextureMode(true);
            mSModeRenderer.setSingleFrameDrawIndex(mCineParams.traceIndex);
            textureWidth = getNumSamplesInFrameHeader(0, DAU_DATA_TYPE_CW);
            mSModeRenderer.setSingleFrameTexturePtr(framePtrCw, textureWidth);
            if (mCineParams.dopplerPeakDraw || mCineParams.dopplerMeanDraw)
            {
                framePtrDopPM = getFrame(0, DAU_DATA_TYPE_DPMAX_SCR);
                mSModeRenderer.setSingleFramePeakMeanTexturePtr(framePtrDopPM, textureWidth);
            }
            break;
    }

    if (mIsDaEnabled)
    {
        mDaRenderer.setSingleFrameFrameWidth(mDaRecordParams.frameWidth);
        mDaRenderer.setSingleFrameFreezeMode(true);
        mDaRenderer.setSingleFrameTraceIndex(mDaRecordParams.traceIndex);

        mDaRenderer.prepareRendererOffScreen(0, 0, DAU_DATA_TYPE_DA);

        mDaRenderer.setLineColor(mDALineColor);
        mDaRenderer.setLineThickness(DA_ECG_LINE_THICKNESS_ENC);

        framePtrDa = (float*) getFrame(0, DAU_DATA_TYPE_DA_SCR);
        mDaRenderer.prepareFreezeModeOffScreen(framePtrDa);
    }

    if (mIsEcgEnabled)
    {
        mEcgRenderer.setSingleFrameFrameWidth(mEcgRecordParams.frameWidth);
        mEcgRenderer.setSingleFrameFreezeMode(true);
        mEcgRenderer.setSingleFrameTraceIndex(mEcgRecordParams.traceIndex);

        mEcgRenderer.prepareRendererOffScreen(0, 0, DAU_DATA_TYPE_ECG);

        mEcgRenderer.setLineColor(mECGLineColor);
        mEcgRenderer.setLineThickness(DA_ECG_LINE_THICKNESS_ENC);

        framePtrEcg = (float*) getFrame(0, DAU_DATA_TYPE_ECG_SCR);
        mEcgRenderer.prepareFreezeModeOffScreen(framePtrEcg);
    }

    // write the data
    if (mIsRawOut)
    {
        // allocate frameImg
        frameImg = new u_char[mImgWidth * mImgHeight * 3];
        // open output file
        outFile = fopen(mOutputFilePath.c_str(), "wb");
        if (outFile == NULL)
        {
            ALOGE("Unable to create raw output file: %s", mOutputFilePath.c_str());
            goto err_ret;
        }
        // draw/render and get screenshot
        mOffSCreenRenderSurface.takeScreenShotToRGBPtrUD(frameImg);
        // write the frameImg to the output file
        fwrite(frameImg, sizeof(u_char), mImgWidth * mImgHeight * 3, outFile);
        // close output file
        if (outFile != NULL) {
            fclose(outFile);
        }
        delete[] frameImg;
    }
    else
    {
        // JPEG output
        mOffSCreenRenderSurface.takeScreenShotToJpegFileUD(mOutputFilePath.c_str());
    }

    retVal = THOR_OK;

err_ret:
    close();
    return retVal;
}