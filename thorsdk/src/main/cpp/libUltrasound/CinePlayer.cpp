//
// Copyright 2017 EchoNous Inc.
//
//

#define LOG_TAG "CinePlayer"

#include <poll.h>
#include <unistd.h>
#include <string.h>
#include <ThorUtils.h>
#include <CinePlayer.h>
#include <ThorRawReader.h>
#include <BitfieldMacros.h>
#include <ScanConverterParams.h>
#include <ScrollModeRecordParams.h>
#include <DaEcgRecordParams.h>
#include <ThorFileProcessor.h>
#include <ThorCapnpSerialize.h>

#define ENABLE_ELAPSED_TIME
#include <ElapsedTime.h>
#include <AIManager.h>
#include <VTICalculation.h>

//-----------------------------------------------------------------------------
CinePlayer::CinePlayer(CineBuffer& cineBuffer) :
    mCineBuffer(cineBuffer),
    mReportProgressFuncPtr(nullptr),
    mSetRunningStatusFuncPtr(nullptr),
    mIsLooping(true),
    mProgressCallback(true),
    mSpeed(Normal),
    mFrameInterval(33),
    mStartFrame(-1),
    mEndFrame(-1),
    mFileIsOpen(false),
    mIsInitialized(false)
{
}

//-----------------------------------------------------------------------------
CinePlayer::~CinePlayer()
{
    ALOGD("%s", __func__);

    terminate();
}

//-----------------------------------------------------------------------------
ThorStatus CinePlayer::init(const std::shared_ptr<UpsReader>& upsReader,
                            const char* appPathPtr)
{
    ThorStatus  retVal = THOR_ERROR;
    int         ret;

    mUpsReaderSPtr = upsReader;
    mAppPath = appPathPtr;

    if (!mExitEvent.init(EventMode::ManualReset, false))
    {
        ALOGE("Unable to initialize ExitEvent");
        goto err_ret;
    }
    mExitEvent.reset();

    if (!mRefreshEvent.init(EventMode::ManualReset, false))
    {
        ALOGE("Unable to initialize RefreshEvent");
        goto err_ret;
    }
    mRefreshEvent.reset();

    if (!mStopEvent.init(EventMode::ManualReset, false))
    {
        ALOGE("Unable to initialize StopEvent");
        goto err_ret;
    }
    mStopEvent.reset();

    if (!mStartEvent.init(EventMode::ManualReset, false))
    {
        ALOGE("Unable to initialize StartEvent");
        goto err_ret;
    }
    mStartEvent.reset();

    if (!mNextEvent.init(EventMode::ManualReset, false))
    {
        ALOGE("Unable to initialize NextEvent");
        goto err_ret;
    }
    mNextEvent.reset();

    if (!mPreviousEvent.init(EventMode::ManualReset, false))
    {
        ALOGE("Unable to initialize PreviousEvent");
        goto err_ret;
    }
    mPreviousEvent.reset();

    if (!mSpeedEvent.init(EventMode::ManualReset, false))
    {
        ALOGE("Unable to initialize SpeedEvent");
        goto err_ret;
    }
    mSpeedEvent.reset();

    if (!mSeekEvent.init())
    {
        ALOGE("Unable to initialize SeekEvent");
        goto err_ret;
    }

    if (!mGetPosEvent.init(EventMode::ManualReset, false))
    {
        ALOGE("Unable to initialize GetPosEvent");
        goto err_ret;
    }
    mGetPosEvent.reset();

    if (!mAckEvent.init())
    {
        ALOGE("Unable to initialize AckEvent");
        goto err_ret;
    }

    ret = pthread_create(&mThreadId, NULL, workerThreadEntry, this);
    if (0 != ret)
    {
        ALOGE("Failed to create worker thread: ret = %d", ret);
        goto err_ret;
    }

    mIsInitialized = true;

    retVal = THOR_OK;

err_ret:
    return(retVal); 
}

//-----------------------------------------------------------------------------
void CinePlayer::terminate()
{
    if (mIsInitialized)
    {
        mExitEvent.set();
        pthread_join(mThreadId, NULL);

        mUpsReaderSPtr = nullptr;
        mReportProgressFuncPtr = nullptr;
        mSetRunningStatusFuncPtr = nullptr;
        mIsInitialized = false;
    }

    if (mFileIsOpen)
    {
        closeRawFile();
    }
}

//-----------------------------------------------------------------------------
void CinePlayer::attachCine(reportProgressFunc progressFuncPtr,
                            setRunningStatusFunc runningStatusFuncPtr)
{
    ALOGD("%s", __func__);

    mLock.enter();
    mReportProgressFuncPtr = progressFuncPtr;
    mSetRunningStatusFuncPtr = runningStatusFuncPtr;
    mLock.leave();

    if (mCineBuffer.getMaxFrameNum() > 0)
    {
        CineBuffer::CineParams  cineParams = mCineBuffer.getParams();

        mFrameInterval = cineParams.frameInterval;
        ALOGD("setting frame interval to %u", mFrameInterval);
        setStartFrame(mCineBuffer.getMinFrameNum());
        setEndFrame(mCineBuffer.getMaxFrameNum());

        mRefreshEvent.set();
        mCineBuffer.prepareClients();
    }
}

//-----------------------------------------------------------------------------
void CinePlayer::detachCine()
{
    ALOGD("%s", __func__);

    mLock.enter();
    mReportProgressFuncPtr = nullptr;
    mSetRunningStatusFuncPtr = nullptr;
    mLock.leave();

    if (mCineBuffer.getMaxFrameNum() > 0)
    {
        mCineBuffer.freeClients();
        mCineBuffer.reset();
    }
}

//-----------------------------------------------------------------------------
ThorStatus CinePlayer::openRawFile(const char* srcPath)
{
    ELAPSED_FUNC();

    ThorStatus                      retVal = THOR_ERROR;
    ThorRawReader                   reader;
    ThorRawDataFile::Metadata       metadata;
    uint8_t*                        addressPtr = nullptr;
    uint8_t*                        cineBufferPtr = nullptr;
    uint32_t                        imagingMode;
    CineBuffer::CineParams          cineParams;
    ColorModeRecordParams           colorModeRecordParams;
    MModeRecordParams               mModeRecordParams;
    DaEcgRecordParams               daRecordParams;
    DaEcgRecordParams               ecgRecordParams;
    uint32_t                        daEcgScrollSpeedIndex;
    ThorFileProcessor               fileProcessor;
    uint8_t                         fileVersion;
    CineBuffer::CineFrameHeader*    cineHeaderPtr;

    ALOGD("%s", __func__);

    // Set default probeType to original probe since earlier builds did not
    // properly support this.  This value will get overwritten from newer
    // variants of the raw file that correctly store this data.
    mCineBuffer.getParams().probeType = 0x3;

    retVal = reader.open(srcPath, metadata);
    if (retVal == TER_MISC_INVALID_FILE)
    {
        // Try new decoding
        LOGD("%s: Trying to decode raw file as Capnp format", __func__);
        // update CineBuffer's upsReader if nullptr
        if (mCineBuffer.getParams().upsReader == nullptr)
            mCineBuffer.getParams().upsReader = mUpsReaderSPtr;

        retVal = ReadThorCapnpFile(srcPath, mCineBuffer, mAppPath.c_str());

        if (IS_THOR_ERROR(retVal))
        {
            LOGE("Unable to parse file as Capnp format");
            goto err_ret;
        }
        else
        {
            LOGD("File successfully read as capnp format");

            // additional process for PW/CW peak/mean
            calculatePeakMean();

            // update mFrameInterval
            mFrameInterval = mCineBuffer.getParams().frameInterval;
            goto ok_ret;
        }
    }
    else if (IS_THOR_ERROR(retVal))
    {
        ALOGE("Unable to open raw file");
        goto err_ret;
    }

    memset(&cineParams, 0, sizeof(cineParams));
    cineParams.probeType = mCineBuffer.getParams().probeType;

    // Ensure that the correct UPS database is used as each probe type
    // has its own UPS.  It is unlikely that this code path will occur...
    retVal = mUpsReaderSPtr->open(mAppPath.c_str(),
                                  mCineBuffer.getParams().probeType);
    if (IS_THOR_ERROR(retVal))
    {
        ALOGE("Unable to open the UPS");
        goto err_ret;
    }
    ALOGD("UPS version : %s", mUpsReaderSPtr->getVersion());

    ALOGD("Getting file data pointer");
    addressPtr = reader.getDataPtr();
    if (nullptr == addressPtr)
    {
        ALOGE("getDataPtr() failed");
        retVal = TER_MISC_OPEN_FILE_FAILED;
        goto err_ret;
    }

    fileVersion = reader.getVersion();

    fileProcessor.processModeHeader(addressPtr, metadata.dataTypes, metadata.frameCount, fileVersion);

    if (mFileIsOpen)
    {
        closeRawFile();
    }
    // If the CineBuffer has data then we can assume that consumers are attached
    // and need to be reset.  This can happen if the CineBuffer was filled from
    // live image data.
    if (mCineBuffer.getMaxFrameNum() > 0)
    {
        mCineBuffer.freeClients();
        mCineBuffer.reset();
    }

    imagingMode = mUpsReaderSPtr->getImagingMode(metadata.imageCaseId);

    // B-mode scan converter params can be read from BMode Header - only applicable to v5 or later
    // Reading from dB for compatibility
    mUpsReaderSPtr->getScanConverterParams(metadata.imageCaseId,
                                           cineParams.converterParams,
                                           imagingMode);
    mFrameInterval = metadata.frameInterval;

    // Get C-Mode scan converter params
    if (0 != BF_GET(metadata.dataTypes, ThorRawDataFile::DataType::CMode, 1))
    {
        // default params for backward compatibility
        colorModeRecordParams.colorMapIndex = 3;
        colorModeRecordParams.isInverted = false;

        fileProcessor.getModeHeader(ThorRawDataFile::DataType::CMode, &colorModeRecordParams);

        ScanConverterParams colorSCParams;
        colorSCParams.numSampleMesh      = colorModeRecordParams.scanConverterParams.numSampleMesh;
        colorSCParams.numRayMesh         = colorModeRecordParams.scanConverterParams.numRayMesh;
        colorSCParams.numSamples         = colorModeRecordParams.scanConverterParams.numSamples;
        colorSCParams.numRays            = colorModeRecordParams.scanConverterParams.numRays;
        colorSCParams.startSampleMm      = colorModeRecordParams.scanConverterParams.startSampleMm;
        colorSCParams.sampleSpacingMm    = colorModeRecordParams.scanConverterParams.sampleSpacingMm;
        colorSCParams.raySpacing         = colorModeRecordParams.scanConverterParams.raySpacing;
        colorSCParams.startRayRadian     = colorModeRecordParams.scanConverterParams.startRayRadian;

        cineParams.colorConverterParams = colorSCParams;
        cineParams.colorMapIndex = colorModeRecordParams.colorMapIndex;
        cineParams.isColorMapInverted = colorModeRecordParams.isInverted;
    }

    // Get M-Mode scroll Params
    if (0 != BF_GET(metadata.dataTypes, ThorRawDataFile::DataType::MMode, 1))
    {
        fileProcessor.getModeHeader(ThorRawDataFile::DataType::MMode, &mModeRecordParams);
    }

    daRecordParams.isScrFrame = false;
    ecgRecordParams.isScrFrame = false;
    daEcgScrollSpeedIndex = 1;

    // Get Da record Params
    if (0 != BF_GET(metadata.dataTypes, ThorRawDataFile::DataType::Auscultation, 1))
    {
        daRecordParams.decimationRatio = 1;
        daRecordParams.location = 0;

        fileProcessor.getModeHeader(ThorRawDataFile::DataType::Auscultation, &daRecordParams);
        daEcgScrollSpeedIndex = daRecordParams.scrollSpeedIndex;
    }

    // Get Ecg record Params
    if (0 != BF_GET(metadata.dataTypes, ThorRawDataFile::DataType::ECG, 1))
    {
        ecgRecordParams.decimationRatio = 1;
        ecgRecordParams.location = 0;

        fileProcessor.getModeHeader(ThorRawDataFile::DataType::ECG, &ecgRecordParams);
        daEcgScrollSpeedIndex = ecgRecordParams.scrollSpeedIndex;
    }

    cineParams.imagingCaseId = metadata.imageCaseId;
    cineParams.dauDataTypes = metadata.dataTypes;
    cineParams.imagingMode = imagingMode;
    cineParams.frameInterval = metadata.frameInterval;
    cineParams.upsReader = mUpsReaderSPtr;
    cineParams.frameRate = 1000.0f/cineParams.frameInterval;

    // m-mode related params
    cineParams.targetFrameRate = 30;
    cineParams.mlinesPerFrame = mModeRecordParams.linesPerBFrame;
    cineParams.scrollSpeed = cineParams.mlinesPerFrame * 1000 / cineParams.frameInterval;
    cineParams.stillTimeShift = 2.0f * ((float)mModeRecordParams.drawnIndex) /
                                ((float)mModeRecordParams.textureFrameWidth);
    cineParams.stillMLineTime = ((float)mModeRecordParams.curLineIndex * cineParams.frameInterval) /
                                ((float)cineParams.mlinesPerFrame * 1000.0f);
    cineParams.traceIndex = mModeRecordParams.curLineIndex;

    // DA/ECG related params
    cineParams.daSingleFrameWidth = daRecordParams.frameWidth;
    cineParams.daTraceIndex = daRecordParams.traceIndex;
    cineParams.ecgSingleFrameWidth = ecgRecordParams.frameWidth;
    cineParams.ecgTraceIndex = ecgRecordParams.traceIndex;

    cineParams.daSamplesPerSecond = daRecordParams.samplesPerSecond;
    cineParams.daSamplesPerFrame = daRecordParams.samplesPerFrame;
    cineParams.ecgSamplesPerSecond = ecgRecordParams.samplesPerSecond;
    cineParams.ecgSamplesPerFrame = ecgRecordParams.samplesPerFrame;
    cineParams.daEcgScrollSpeedIndex = daEcgScrollSpeedIndex;

    mCineBuffer.setParams(cineParams);

    // TODO: Cheating a bit here.
    //       Assuming frame size are maximum allowed instead of packed data.
    //       Also assuming that it is OK to copy many frames directly into
    //       cine buffer (that frames are stored sequentially).
    if (0 != BF_GET(metadata.dataTypes, ThorRawDataFile::DataType::BMode, 1))
    {
        ALOGD("Reading B-Mode data");
        cineBufferPtr = mCineBuffer.getFrame(0,
                                             DAU_DATA_TYPE_B,
                                             CineBuffer::FrameContents::IncludeHeader);

        fileProcessor.getData(ThorRawDataFile::DataType::BMode, cineBufferPtr);
    }

    if (0 != BF_GET(metadata.dataTypes, ThorRawDataFile::DataType::CMode, 1))
    {
        ALOGD("Reading C-Mode data");

        cineBufferPtr = mCineBuffer.getFrame(0,
                                             DAU_DATA_TYPE_COLOR,
                                             CineBuffer::FrameContents::IncludeHeader);

        fileProcessor.getData(ThorRawDataFile::DataType::CMode, cineBufferPtr);
    }

    if (0 != BF_GET(metadata.dataTypes, ThorRawDataFile::DataType::MMode, 1))
    {
        ALOGD("Reading M-Mode data");

        if (mModeRecordParams.isTextureFrame)
        {
            cineBufferPtr = mCineBuffer.getFrame(TEX_FRAME_TYPE_M,
                                                 DAU_DATA_TYPE_TEX,
                                                 CineBuffer::FrameContents::IncludeHeader);
        }
        else
        {
            cineBufferPtr = mCineBuffer.getFrame(0,
                                                 DAU_DATA_TYPE_M,
                                                 CineBuffer::FrameContents::IncludeHeader);
        }

        fileProcessor.getData(ThorRawDataFile::DataType::MMode, cineBufferPtr);

        if (mModeRecordParams.isTextureFrame)
        {
            // insert texture Framewidth into header for the updated renderer
            cineHeaderPtr = reinterpret_cast<CineBuffer::CineFrameHeader*>(cineBufferPtr);
            cineHeaderPtr->numSamples = mModeRecordParams.textureFrameWidth;
        }
    }

    if (BF_GET(metadata.dataTypes, ThorRawDataFile::DataType::Auscultation, 1))
    {
        ALOGD("Reading DA data");

        if (daRecordParams.isScrFrame)
        {
            cineBufferPtr = mCineBuffer.getFrame(0,
                                                 DAU_DATA_TYPE_DA_SCR,
                                                 CineBuffer::FrameContents::IncludeHeader);
        }
        else
        {
            cineBufferPtr = mCineBuffer.getFrame(0,
                                                 DAU_DATA_TYPE_DA,
                                                 CineBuffer::FrameContents::IncludeHeader);
        }

        // Due to the change in MAX_DA_FRAME_SIZE
        fileProcessor.getDataPreVer(ThorRawDataFile::DataType::Auscultation, cineBufferPtr, MAX_DA_FRAME_SIZE);
    }

    if (BF_GET(metadata.dataTypes, ThorRawDataFile::DataType::ECG, 1))
    {
        ALOGD("Reading ECG data");

        if (ecgRecordParams.isScrFrame)
        {
            cineBufferPtr = mCineBuffer.getFrame(0,
                                                 DAU_DATA_TYPE_ECG_SCR,
                                                 CineBuffer::FrameContents::IncludeHeader);
        }
        else
        {
            cineBufferPtr = mCineBuffer.getFrame(0,
                                                 DAU_DATA_TYPE_ECG,
                                                 CineBuffer::FrameContents::IncludeHeader);
        }

        fileProcessor.getData(ThorRawDataFile::DataType::ECG, cineBufferPtr);
    }

    mCineBuffer.setBoundary(0, metadata.frameCount - 1);
    mCineBuffer.setCurFrame(0);

ok_ret:
    setStartFrame(mCineBuffer.getMinFrameNum());
    setEndFrame(mCineBuffer.getMaxFrameNum());

    mCineBuffer.prepareClients();
    mRefreshEvent.set();
    mFileIsOpen = true;

err_ret:
    reader.close();

    return(retVal);
}

//-----------------------------------------------------------------------------
void CinePlayer::closeRawFile()
{
    ALOGD("%s", __func__);

    if (mFileIsOpen)
    {
        mCineBuffer.freeClients();
        mCineBuffer.reset();
        mFileIsOpen = false;
    }
}

//-----------------------------------------------------------------------------
void CinePlayer::start()
{
    mStartEvent.set();
}

//-----------------------------------------------------------------------------
void CinePlayer::stop()
{
    mStopEvent.set();
    seekToFrame(getStartFrame(), true);
}

//-----------------------------------------------------------------------------
void CinePlayer::pause()
{
    mStopEvent.set();
}

//-----------------------------------------------------------------------------
void CinePlayer::nextFrame()
{
    mNextEvent.set();
}

//-----------------------------------------------------------------------------
void CinePlayer::previousFrame()
{
    mPreviousEvent.set();
}

//-----------------------------------------------------------------------------
void CinePlayer::seekTo(uint32_t msec)
{
    seekToFrame(getFrameFromTime(msec), true);
}

//-----------------------------------------------------------------------------
void CinePlayer::seekToFrame(uint32_t frame, bool progressCallback)
{
    mLock.enter();
    mProgressCallback = progressCallback;
    mSeekEvent.set(validateFrameNum(frame));
    mLock.leave();
}

//-----------------------------------------------------------------------------
uint32_t CinePlayer::getDuration()
{
    uint32_t    frameCount = getFrameCount();
    uint32_t    duration = 0;

    if (0 < frameCount)
    {
        duration = (frameCount - 1) * mFrameInterval;
    }
    return(duration);
}

//-----------------------------------------------------------------------------
uint32_t CinePlayer::getFrameCount()
{
    uint32_t        maxFrameNum;
    uint32_t        minFrameNum;
    uint32_t        frameCount;

    maxFrameNum = mCineBuffer.getMaxFrameNum();
    minFrameNum = mCineBuffer.getMinFrameNum();

    if ((0 == maxFrameNum) && (0 == minFrameNum) && 
        (UINT32_MAX == mCineBuffer.getCurFrameNum())) 
    {
        // Corner case of an empty cine buffer
        frameCount = 0;
    }
    else
    {
        frameCount = maxFrameNum - minFrameNum + 1;
    }

    return(frameCount);
}

//-----------------------------------------------------------------------------
uint32_t CinePlayer::getCurrentPosition()
{
    return(getCurrentFrame() * mFrameInterval);
}

//-----------------------------------------------------------------------------
uint32_t CinePlayer::getCurrentFrame()
{
    uint32_t    currentFrame = 0;

    mLock.enter();
    mGetPosEvent.set();
    if (mAckEvent.wait(500))
    {
        currentFrame = mAckEvent.read();
    }
    mLock.leave();

    return(currentFrame);
}

//-----------------------------------------------------------------------------
uint32_t CinePlayer::getCurrentFrameNo()
{
    return (getCurrentFrame() + mCineBuffer.getMinFrameNum());
}

//-----------------------------------------------------------------------------
void CinePlayer::setStartPosition(uint32_t msec)
{
    setStartFrame(getFrameFromTime(msec));
}

//-----------------------------------------------------------------------------
void CinePlayer::setEndPosition(uint32_t msec)
{
    setEndFrame(getFrameFromTime(msec));
}

//-----------------------------------------------------------------------------
void CinePlayer::setStartFrame(uint32_t frame)
{
    mLock.enter();
    mStartFrame = validateFrameNum(frame);
    mLock.leave();

    mRefreshEvent.set();
}

//-----------------------------------------------------------------------------
void CinePlayer::setEndFrame(uint32_t frame)
{
    mLock.enter();
    mEndFrame = validateFrameNum(frame);
    mLock.leave();

    mRefreshEvent.set();
}

uint32_t CinePlayer::getFrameInterval()
{
    return mFrameInterval;
}

//-----------------------------------------------------------------------------
uint32_t CinePlayer::getStartPosition()
{
    return(getStartFrame() * mFrameInterval); 
}

//-----------------------------------------------------------------------------
uint32_t CinePlayer::getEndPosition()
{
    return(getEndFrame() * mFrameInterval); 
}

//-----------------------------------------------------------------------------
uint32_t CinePlayer::getStartFrame()
{
    uint32_t    startFrame;

    mLock.enter();
    startFrame = mStartFrame;
    mLock.leave();

    return(startFrame); 
}

//-----------------------------------------------------------------------------
uint32_t CinePlayer::getEndFrame()
{
    uint32_t    endFrame;

    mLock.enter();
    endFrame = mEndFrame;
    mLock.leave();

    return(endFrame); 
}

//-----------------------------------------------------------------------------
void CinePlayer::setLooping(bool doLoop)
{
    mLock.enter();
    mIsLooping = doLoop;
    mLock.leave();
}

//-----------------------------------------------------------------------------
bool CinePlayer::isLooping()
{
    bool    isLooping;

    mLock.enter();
    isLooping = mIsLooping;
    mLock.leave();

    return(isLooping); 
}

//-----------------------------------------------------------------------------
void CinePlayer::setPlaybackSpeed(Speed speed)
{
    mLock.enter();
    mSpeed = speed;
    mLock.leave();

    mSpeedEvent.set();
}

//-----------------------------------------------------------------------------
CinePlayer::Speed CinePlayer::getPlaybackSpeed()
{
    Speed   speed;

    mLock.enter();
    speed = mSpeed;
    mLock.leave();

    return(speed);
}

//-----------------------------------------------------------------------------
uint32_t CinePlayer::validateFrameNum(uint32_t frame)
{
    uint32_t        maxFrameNum = mCineBuffer.getMaxFrameNum();
    uint32_t        minFrameNum = mCineBuffer.getMinFrameNum();

    if (frame < minFrameNum)
    {
        frame = minFrameNum;
    }

    if (frame > maxFrameNum)
    {
        frame = maxFrameNum;
    }

    return(frame); 
}

//-----------------------------------------------------------------------------
uint32_t CinePlayer::getFrameFromTime(uint32_t msec)
{
    uint32_t        clipLength = getDuration();
    uint32_t        maxFrameNum = mCineBuffer.getMaxFrameNum();
    uint32_t        minFrameNum = mCineBuffer.getMinFrameNum();
    uint32_t        frameNum;

    if (msec >= clipLength)
    {
        frameNum = maxFrameNum;
    }
    else
    {
        float   seekTemp;

        seekTemp = ((float) msec / (float) mFrameInterval) 
                   + minFrameNum;

        frameNum = seekTemp;
    }

    return(frameNum); 
}

//-----------------------------------------------------------------------------
void CinePlayer::calculatePeakMean()
{
    int imagingMode = mCineBuffer.getParams().imagingMode;

    // process peak mean if PW or CW
    if ( ((imagingMode == IMAGING_MODE_PW) || (imagingMode == IMAGING_MODE_CW)) ) {
        DopplerPeakMeanProcessHandler *dopplerPeakMeanProc = new DopplerPeakMeanProcessHandler(
                mUpsReaderSPtr);
        dopplerPeakMeanProc->setCineBuffer(&mCineBuffer);
        dopplerPeakMeanProc->init();

        bool singleFrame = ( mCineBuffer.getMaxFrameNum() == mCineBuffer.getMinFrameNum() );
        int outDataType;

        float refGaindB, peakThreshold, defaultGaindB;
        float gainOffset, gainRange;

        if (singleFrame)
            outDataType = DAU_DATA_TYPE_DPMAX_SCR;
        else
            outDataType = DAU_DATA_TYPE_DOP_PM;

        if (imagingMode == IMAGING_MODE_PW) {
            mUpsReaderSPtr->getPWPeakThresholdParams(mCineBuffer.getParams().organId, refGaindB, peakThreshold);
            dopplerPeakMeanProc->setDataType(DAU_DATA_TYPE_PW, outDataType);

            // Default PW gain
            mUpsReaderSPtr->getPWGainParams(mCineBuffer.getParams().organId,
                                            gainOffset, gainRange, mCineBuffer.getParams().isPWTDI);
            defaultGaindB = gainOffset + (gainRange * 0.5f);
        } else {
            mUpsReaderSPtr->getCWPeakThresholdParams(mCineBuffer.getParams().organId, refGaindB, peakThreshold);
            dopplerPeakMeanProc->setDataType(DAU_DATA_TYPE_CW, outDataType);

            // Default CW gain
            mUpsReaderSPtr->getCWGainParams(mCineBuffer.getParams().organId,
                                            gainOffset, gainRange);
            defaultGaindB = gainOffset + (gainRange * 0.5f);
        }
        dopplerPeakMeanProc->setProcessIndices(mCineBuffer.getParams().dopplerFFTSize, defaultGaindB,
                                               peakThreshold);
        dopplerPeakMeanProc->setWFGain(mCineBuffer.getParams().dopplerWFGain);

        // process all frames in the buffer
        dopplerPeakMeanProc->processAllFrames();

        // clean up
        dopplerPeakMeanProc->terminate();
        if (nullptr != dopplerPeakMeanProc) {
            delete dopplerPeakMeanProc;
        }
    }
}

//-----------------------------------------------------------------------------
void* CinePlayer::workerThreadEntry(void* thisPtr)
{
    ((CinePlayer*) thisPtr)->threadLoop();

    return(nullptr); 
}

//-----------------------------------------------------------------------------
void CinePlayer::threadLoop()
{
    const int       numFd = 9;
    const int       exitEvent = 0;
    const int       stopEvent = 1;
    const int       startEvent = 2;
    const int       seekEvent = 3;
    const int       nextEvent = 4;
    const int       previousEvent = 5;
    const int       speedEvent = 6;
    const int       refreshEvent = 7;
    const int       getPosEvent = 8;
    struct pollfd   pollFd[numFd];
    bool            keepLooping  = true;
    int             ret;
    int             timeout = -1;
    int             tgtTimeout = -1;
    int             timediff = -1;
    uint32_t        frameNum = 0;
    uint32_t        maxFrameNum = 0;
    uint32_t        minFrameNum = 0;
    uint32_t        fenceStart = 0;
    uint32_t        fenceEnd = 0;
    uint32_t        playbackInterval = mFrameInterval;

    struct timespec now;

    double          curTimeMsec;
    double          prevTimeMsec;

    pollFd[exitEvent].fd = mExitEvent.getFd();
    pollFd[exitEvent].events = POLLIN;
    pollFd[exitEvent].revents = 0;

    pollFd[stopEvent].fd = mStopEvent.getFd();
    pollFd[stopEvent].events = POLLIN;
    pollFd[stopEvent].revents = 0;

    pollFd[startEvent].fd = mStartEvent.getFd();
    pollFd[startEvent].events = POLLIN;
    pollFd[startEvent].revents = 0;

    pollFd[seekEvent].fd = mSeekEvent.getFd();
    pollFd[seekEvent].events = POLLIN;
    pollFd[seekEvent].revents = 0;

    pollFd[nextEvent].fd = mNextEvent.getFd();
    pollFd[nextEvent].events = POLLIN;
    pollFd[nextEvent].revents = 0;

    pollFd[previousEvent].fd = mPreviousEvent.getFd();
    pollFd[previousEvent].events = POLLIN;
    pollFd[previousEvent].revents = 0;

    pollFd[speedEvent].fd = mSpeedEvent.getFd();
    pollFd[speedEvent].events = POLLIN;
    pollFd[speedEvent].revents = 0;

    pollFd[refreshEvent].fd = mRefreshEvent.getFd();
    pollFd[refreshEvent].events = POLLIN;
    pollFd[refreshEvent].revents = 0;

    pollFd[getPosEvent].fd = mGetPosEvent.getFd();
    pollFd[getPosEvent].events = POLLIN;
    pollFd[getPosEvent].revents = 0;

    ALOGD("threadLoop entered");

    prevTimeMsec = 0;

    while (keepLooping)
    {
        ret = poll(pollFd, numFd, timeout);
        if (-1 == ret)
        {
            ALOGE("threadLoop: Error occurred during poll(). errno = %d", errno);
            keepLooping = false;
        }
        //
        // Playback Timer fired - draw next frame
        //
        else if (0 == ret)
        {
            clock_gettime(CLOCK_MONOTONIC, &now);
            curTimeMsec =  ((double) now.tv_sec * 1000.0L) + ((double) now.tv_nsec / 1000000.0L);

            timediff = (int) (curTimeMsec - prevTimeMsec);

            prevTimeMsec = curTimeMsec;

            if (timediff > tgtTimeout && tgtTimeout *2 > timediff)
            {
                timeout -= timediff - timeout;

                if (timeout < 0)
                    timeout = 0;
            }
            else if (tgtTimeout > timediff)
            {
                timeout += tgtTimeout - timediff;

                if (timeout > tgtTimeout * 2)
                    timeout = tgtTimeout * 2;
            }

            mCineBuffer.replayFrame(frameNum++);

            if (frameNum > fenceEnd)
            {
                frameNum = getStartFrame();

                mLock.enter();
                if (!mIsLooping)
                {
                    frameNum = getEndFrame();
                    if (nullptr != mSetRunningStatusFuncPtr)
                    {
                        mSetRunningStatusFuncPtr(false);
                    }
                    mCineBuffer.pausePlayback();
                    timeout = -1;
                }
                mLock.leave();
            }

            mLock.enter();
            if (nullptr != mReportProgressFuncPtr)
            {
                mReportProgressFuncPtr((frameNum - minFrameNum) * mFrameInterval);
            }
            mLock.leave();
        }
        //
        // Exit Thread
        //
        else if (pollFd[exitEvent].revents & POLLIN)
        {
            mExitEvent.reset();
            keepLooping = false;
        }
        //
        // Stop Playing
        //
        else if (pollFd[stopEvent].revents & POLLIN)
        {
            mStopEvent.reset();
            mCineBuffer.pausePlayback();
            timeout = -1;
        }
        //
        // Start Playing
        //
        else if (pollFd[startEvent].revents & POLLIN)
        {
            mStartEvent.reset();
            timeout = playbackInterval;
            tgtTimeout = playbackInterval;

            if (frameNum >= fenceEnd)
            {
                frameNum = getStartFrame();

                mLock.enter();
                if (nullptr != mReportProgressFuncPtr)
                {
                    mReportProgressFuncPtr((frameNum - minFrameNum) * mFrameInterval);
                }
                mLock.leave();
            }

            ALOGD("Refresh: playbackInterval: %u, maxFrameNum: %u, "
                  "minFrameNum: %u, fenceStart = %u, fenceEnd = %u, "
                  "frameNum: %u",
                  playbackInterval,
                  maxFrameNum,
                  minFrameNum,
                  fenceStart,
                  fenceEnd,
                  frameNum);
        }
        //
        // Seek to new position
        //
        else if (pollFd[seekEvent].revents & POLLIN)
        {
            // Validation of specified frame within the fencing area is not done.
            // This is intentional to allow the UI to specify display of a frame
            // outside of the fence, thus allowing the user to visually change
            // the fence boundary.

            frameNum = mSeekEvent.read();
            mCineBuffer.replayFrame(frameNum);

            if ((nullptr != mReportProgressFuncPtr) && mProgressCallback)
            {
                mReportProgressFuncPtr((frameNum - minFrameNum) * mFrameInterval);
            }
        }
        //
        // Go to next frame
        //
        else if (pollFd[nextEvent].revents & POLLIN)
        {
            mNextEvent.reset();
            if (-1 == timeout && frameNum < fenceEnd)
            {
                mCineBuffer.replayFrame(++frameNum);

                mLock.enter();
                if (nullptr != mReportProgressFuncPtr)
                {
                    mReportProgressFuncPtr((frameNum - minFrameNum) * mFrameInterval);
                }
                mLock.leave();
            }
        }
        //
        // Go to previous frame
        //
        else if (pollFd[previousEvent].revents & POLLIN)
        {
            mPreviousEvent.reset();
            if (-1 == timeout && frameNum > fenceStart)
            {
                mCineBuffer.replayFrame(--frameNum);

                mLock.enter();
                if (nullptr != mReportProgressFuncPtr)
                {
                    mReportProgressFuncPtr((frameNum - minFrameNum) * mFrameInterval);
                }
                mLock.leave();
            }
        }
        //
        // Change playback speed
        //
        else if (pollFd[speedEvent].revents & POLLIN)
        {
            mSpeedEvent.reset();
            mLock.enter();
            switch (mSpeed)
            {
                case Normal:
                    playbackInterval = mFrameInterval;
                    break;

                case Half:
                    playbackInterval = mFrameInterval * 2;
                    break;

                case Quarter:
                    playbackInterval = mFrameInterval * 4;
                    break;
            }
            mLock.leave();

            if (-1 != timeout)
            {
                timeout = playbackInterval;
                tgtTimeout = playbackInterval;
            }
        }
        //
        // Refresh parameters
        //
        else if (pollFd[refreshEvent].revents & POLLIN)
        {
            mRefreshEvent.reset();
            playbackInterval = mFrameInterval;
            minFrameNum = mCineBuffer.getMinFrameNum();
            maxFrameNum = mCineBuffer.getMaxFrameNum();
            fenceStart = getStartFrame();
            fenceEnd = getEndFrame();
            frameNum = fenceEnd;

            ALOGD("Refresh: playbackInterval: %u, maxFrameNum: %u, "
                  "minFrameNum: %u, fenceStart = %u, fenceEnd = %u, "
                  "frameNum: %u",
                  playbackInterval,
                  maxFrameNum,
                  minFrameNum,
                  fenceStart,
                  fenceEnd,
                  frameNum);

            mLock.enter();
            if (nullptr != mReportProgressFuncPtr)
            {
                mReportProgressFuncPtr((frameNum - minFrameNum) * mFrameInterval);
            }
            mLock.leave();
        }
        //
        // GetPos Request
        //
        else if (pollFd[getPosEvent].revents & POLLIN)
        {
            mGetPosEvent.reset();
            mAckEvent.set(frameNum - minFrameNum);
        }
        else
        {
            ALOGE("Error occurred in poll()");
            keepLooping = false;
        }
    }

    ALOGD("threadLoop exited");
}


