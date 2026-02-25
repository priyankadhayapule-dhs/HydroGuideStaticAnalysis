//
// Copyright 2018 EchoNous Inc.
//
//

#define LOG_TAG "CineRecorder"

#include <poll.h>
#include <string.h>
#include <ThorUtils.h>
#include <CineRecorder.h>
#include <BitfieldMacros.h>
#include <ThorCapnpSerialize.h>

#define ENABLE_ELAPSED_TIME
//#define THOR_RAW_FORMAT

#include <ElapsedTime.h>


//-----------------------------------------------------------------------------
CineRecorder::CineRecorder(CineBuffer& cineBuffer) :
    mCineBuffer(cineBuffer),
    mCompletionHandlerPtr(nullptr),
    mIsInitialized(false),
    mIsBusy(false),
    mBFrameSize(0),
    mCFrameSize(0),
    mMFrameSize(0),
    mTexFrameSize(0),
    mDaFrameSize(0),
    mEcgFrameSize(0),
    mDaScrFrameSize(0),
    mEcgScrFrameSize(0),
    mIsDaOn(false),
    mIsEcgOn(false),
    mStillImageFrameNum(-1u)
{
}

//-----------------------------------------------------------------------------
CineRecorder::~CineRecorder()
{
    ALOGD("%s", __func__);

    onTerminate();
}

//-----------------------------------------------------------------------------
ThorStatus CineRecorder::onInit()
{
    ThorStatus      retVal = THOR_ERROR;
    int             ret;

    if (mIsInitialized)
    {
        ALOGE("Already initialized - Cannot re-initialize");
        goto err_ret;
    }

    if (!mStopEvent.init(EventMode::ManualReset, false))
    {
        ALOGE("Unable to initialize StopEvent");
        goto err_ret;
    }
    mStopEvent.reset();

    if (!mRecordRetroEvent.init())
    {
        ALOGE("Unable to initialize RecordRetroEvent");
        goto err_ret;
    }

    if (!mRecordStillEvent.init(EventMode::ManualReset, false))
    {
        ALOGE("Unable to initialize RecordStillEvent");
        goto err_ret;
    }

    if (!mRecordCineEvent.init(EventMode::ManualReset, false))
    {
        ALOGE("Unable to initialize RecordCineEvent");
        goto err_ret;
    }

    if (!mCancelEvent.init(EventMode::AutoReset, false))
    {
        ALOGE("Unable to initialize CancelEvent");
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
void CineRecorder::onTerminate()
{
    if (mIsInitialized)
    {
        mStopEvent.set();
        pthread_join(mThreadId, NULL);

        mIsInitialized = false;
    }
}

//-----------------------------------------------------------------------------
void CineRecorder::setParams(CineBuffer::CineParams& params)
{
    mCineParams = params;

    // TODO: These need to be based on actual imaging params instead of using
    //       maximum sizes.
    mBFrameSize = MAX_B_FRAME_SIZE + sizeof(CineBuffer::CineFrameHeader);
    mCFrameSize = MAX_COLOR_FRAME_SIZE + sizeof(CineBuffer::CineFrameHeader);
    mMFrameSize = MAX_M_FRAME_SIZE + sizeof(CineBuffer::CineFrameHeader);
    mTexFrameSize = MAX_TEX_FRAME_SIZE + sizeof(CineBuffer::CineFrameHeader);
    mDaFrameSize = MAX_DA_FRAME_SIZE + sizeof(CineBuffer::CineFrameHeader);
    mEcgFrameSize = MAX_ECG_FRAME_SIZE + sizeof(CineBuffer::CineFrameHeader);
    mDaScrFrameSize = MAX_DA_SCR_FRAME_SIZE + sizeof(CineBuffer::CineFrameHeader);
    mEcgScrFrameSize = MAX_ECG_SCR_FRAME_SIZE + sizeof(CineBuffer::CineFrameHeader);

}

//-----------------------------------------------------------------------------
void CineRecorder::setScrollModeRecordParams(ScrollModeRecordParams &smrParams)
{
    mScrollModeRecordParams = smrParams;
}

//-----------------------------------------------------------------------------
void CineRecorder::setDaRecordParams(DaEcgRecordParams &daParams)
{
    mDaRecordParams = daParams;
}

//-----------------------------------------------------------------------------
void CineRecorder::setEcgRecordParams(DaEcgRecordParams &ecgParams)
{
    mEcgRecordParams = ecgParams;
}

//-----------------------------------------------------------------------------
void CineRecorder::onData(uint32_t frameNum, uint32_t dauDataTypes)
{
    UNUSED(frameNum);
    UNUSED(dauDataTypes);
}

//-----------------------------------------------------------------------------
void CineRecorder::attachCompletionHandler(CompletionHandler completionHandlerPtr)
{
    mLock.enter();
    mCompletionHandlerPtr = completionHandlerPtr;
    mLock.leave();
}

//-----------------------------------------------------------------------------
void CineRecorder::detachCompletionHandler()
{
    mLock.enter();
    mCompletionHandlerPtr = nullptr;
    mLock.leave();
}

//-----------------------------------------------------------------------------
ThorStatus CineRecorder::recordStillImage(const char* destPath, uint32_t frameNum)
{
    ThorStatus  retVal = THOR_OK;

    if (!getBusyLock())
    {
        ALOGE("Recorder is busy");
        retVal = TER_MISC_BUSY;
        goto err_ret;
    }

    mFilePath = destPath;
    mStillImageFrameNum = frameNum;
    mRecordStillEvent.set();

err_ret:
    return(retVal);
}

//-----------------------------------------------------------------------------
ThorStatus CineRecorder::

recordRetrospectiveVideo(const char* destPath, uint32_t msec)
{
    ThorStatus  retVal = THOR_ERROR;
    uint32_t    desiredFrames;

    if (msec < mCineParams.frameInterval || MAX_RECORD_TIME < msec )
    {
        ALOGE("Invalid time specified.  msec = %u", msec);
        retVal = TER_MISC_PARAM;
        goto err_ret;
    }

    desiredFrames = msec / mCineParams.frameInterval;

    retVal = recordRetrospectiveVideoFrames(destPath, desiredFrames);

err_ret:
    return(retVal);
}

//-----------------------------------------------------------------------------
ThorStatus CineRecorder::

recordRetrospectiveVideoFrames(const char* destPath, uint32_t frames)
{
    ThorStatus  retVal = THOR_ERROR;
    uint32_t    minFrame;
    uint32_t    maxFrame;

    if (!getBusyLock())
    {
        ALOGE("Recorder is busy");
        retVal = TER_MISC_BUSY;
        goto err_ret;
    }

    mFilePath = destPath;

    minFrame = mCineBuffer.getMinFrameNum();
    maxFrame = mCineBuffer.getMaxFrameNum();

    if ((maxFrame - minFrame + 1) < frames)
    {
        frames = maxFrame - minFrame + 1;
        ALOGD("Adjusting recording frame numbers.  Recording %d frames",
              frames);
    }

    mRecordRetroEvent.set(frames);
    retVal = THOR_OK;

err_ret:
    return(retVal);
}

//-----------------------------------------------------------------------------
ThorStatus CineRecorder::

saveVideoFromCine(const char* destPath, uint32_t startMsec, uint32_t stopMsec)
{
    ThorStatus  retVal = THOR_ERROR;
    uint32_t    startFrame;
    uint32_t    stopFrame;

    startFrame = startMsec / mCineParams.frameInterval;
    stopFrame = stopMsec / mCineParams.frameInterval;

    retVal = saveVideoFromCineFrames(destPath, startFrame, stopFrame);

err_ret:
    return(retVal);
}

//-----------------------------------------------------------------------------
ThorStatus CineRecorder::

saveVideoFromCineFrames(const char* destPath, uint32_t reqStartFrame, uint32_t reqStopFrame)
{
    ThorStatus  retVal = THOR_ERROR;
    uint32_t    minFrame;
    uint32_t    maxFrame;
    uint32_t    startFrame;
    uint32_t    stopFrame;

    if (!getBusyLock())
    {
        ALOGE("Recorder is busy");
        retVal = TER_MISC_BUSY;
        goto err_ret;
    }

    minFrame = mCineBuffer.getMinFrameNum();
    maxFrame = mCineBuffer.getMaxFrameNum();

    // adjust startFrame as minFrame could not be zero.
    startFrame = reqStartFrame + minFrame;
    stopFrame = reqStopFrame + minFrame;

    if (stopFrame > maxFrame)
    {
        stopFrame = maxFrame;
        ALOGD("Adjusting stopFrame number: %d", stopFrame);
    }

    if (startFrame >= stopFrame)
    {
        ALOGE("stopFrame must be greater than startFrame");
        retVal = TER_MISC_PARAM;

        // unlock busylock
        mBusyLock.enter();
        mIsBusy = false;
        mBusyLock.leave();

        goto err_ret;
    }

    mFilePath = destPath;
    mStartFrame = startFrame;
    mStopFrame = stopFrame;

    mRecordCineEvent.set();
    retVal = THOR_OK;

err_ret:
    return(retVal);
}

//-----------------------------------------------------------------------------
ThorStatus CineRecorder::
saveVideoFromLiveFrames(const char* destPath, uint32_t startFrame, uint32_t stopFrame)
{
    ThorStatus  retVal = THOR_ERROR;
    uint32_t    minFrame;
    uint32_t    maxFrame;

    if (!getBusyLock())
    {
        ALOGE("Recorder is busy");
        retVal = TER_MISC_BUSY;
        goto err_ret;
    }

    minFrame = mCineBuffer.getMinFrameNum();
    maxFrame = mCineBuffer.getMaxFrameNum();

    if (stopFrame > maxFrame)
    {
        stopFrame = maxFrame;
        ALOGD("Adjusting stopFrame number: %d", stopFrame);
    }

    if (startFrame >= stopFrame)
    {
        ALOGE("stopFrame must be greater than startFrame");
        retVal = TER_MISC_PARAM;

        // unlock busylock
        mBusyLock.enter();
        mIsBusy = false;
        mBusyLock.leave();

        goto err_ret;
    }

    mFilePath = destPath;
    mStartFrame = startFrame;
    mStopFrame = stopFrame;
    LOGD("Recording from frame %u to %u", startFrame, stopFrame);

    mRecordCineEvent.set();
    retVal = THOR_OK;

    err_ret:
    return(retVal);
}

//-----------------------------------------------------------------------------
ThorStatus CineRecorder::updateParamsThorFile(const char* destPath)
{
    ThorStatus  retVal = THOR_OK;

    if (!getBusyLock())
    {
        ALOGE("Recorder is busy");
        retVal = TER_MISC_BUSY;
        goto err_ret;
    }

    retVal = UpdateCineParamsThorCapnpFile(destPath, mCineBuffer);

    /*
    mLock.enter();
    if (nullptr != mCompletionHandlerPtr)
    {
        mCompletionHandlerPtr(retVal);
    }
    mLock.leave();
    */

    mBusyLock.enter();
    mIsBusy = false;
    mBusyLock.leave();

err_ret:
    return(retVal);
}

//-----------------------------------------------------------------------------
bool CineRecorder::getBusyLock()
{
    bool    gotLock = false;

    mBusyLock.enter();
    if (!mIsBusy)
    {
        mIsBusy = true;
        gotLock = true;
    }
    mBusyLock.leave();

    return(gotLock); 
}

//-----------------------------------------------------------------------------
void* CineRecorder::workerThreadEntry(void* thisPtr)
{
    ((CineRecorder*) thisPtr)->threadLoop();

    return(nullptr);
}

//-----------------------------------------------------------------------------
void CineRecorder::threadLoop()
{
    const int       numFd = 4;
    const int       stopEvent = 0;
    const int       recordRetroEvent = 1;
    const int       recordStillEvent = 2;
    const int       recordCineEvent = 3;
    struct pollfd   pollFd[numFd];
    bool            keepLooping  = true;
    int             ret;

    pollFd[stopEvent].fd = mStopEvent.getFd();
    pollFd[stopEvent].events = POLLIN;
    pollFd[stopEvent].revents = 0;

    pollFd[recordRetroEvent].fd = mRecordRetroEvent.getFd();
    pollFd[recordRetroEvent].events = POLLIN;
    pollFd[recordRetroEvent].revents = 0;

    pollFd[recordStillEvent].fd = mRecordStillEvent.getFd();
    pollFd[recordStillEvent].events = POLLIN;
    pollFd[recordStillEvent].revents = 0;

    pollFd[recordCineEvent].fd = mRecordCineEvent.getFd();
    pollFd[recordCineEvent].events = POLLIN;
    pollFd[recordCineEvent].revents = 0;

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
        else if (pollFd[recordRetroEvent].revents & POLLIN)
        {
            uint32_t    desiredFrames;
            uint32_t    maxFrame;
            uint32_t    minFrame;

            desiredFrames = mRecordRetroEvent.read();
            maxFrame = mCineBuffer.getMaxFrameNum();
            minFrame = maxFrame-desiredFrames+1;

#ifdef THOR_RAW_FORMAT
            recordRetrospectiveClip(desiredFrames);
#else
            recordCapnpFrames(minFrame, maxFrame);
#endif
        }
        else if (pollFd[recordStillEvent].revents & POLLIN)
        {
            uint32_t    minFrame = mStillImageFrameNum;
            if (minFrame == -1u)
                minFrame = mCineBuffer.getCurFrameNum();

            uint32_t    maxFrame = minFrame;

            mRecordStillEvent.reset();

#ifdef THOR_RAW_FORMAT
            recordFrames(minFrame, maxFrame);
#else
            recordCapnpFrames(minFrame, maxFrame);
#endif
            mStillImageFrameNum = -1u;
        }
        else if (pollFd[recordCineEvent].revents & POLLIN)
        {
            mRecordCineEvent.reset();

#ifdef THOR_RAW_FORMAT
            recordFrames(mStartFrame, mStopFrame);
#else
            recordCapnpFrames(mStartFrame, mStopFrame);
#endif
        }
        else
        {
            ALOGE("Error occurred in poll()");
            keepLooping = false;
        }

        mBusyLock.enter();
        mIsBusy = false;
        mBusyLock.leave();
    }
}

//-----------------------------------------------------------------------------
void CineRecorder::recordRetrospectiveClip(uint32_t recordFrames)
{
    ELAPSED_FUNC();

    uint32_t                    minFrame;
    uint32_t                    maxFrame;
    ThorRawWriter               writer;
    ThorStatus                  thorRet = THOR_ERROR;
    uint8_t*                    fileDataPtr = nullptr;
    uint32_t                    dataSize = 0;
    const uint32_t              CHECK_FRAME_COUNT = 150;
    DataMap                     dataMap;

    memset(&dataMap, 0, sizeof(dataMap));

    thorRet = createRawFile(writer, recordFrames);
    if (IS_THOR_ERROR(thorRet))
    {
        goto err_ret;
    }

    dataSize = calculateSpace(recordFrames, dataMap);
    ALOGD("recordRetrospectiveClip() - dataSize: %u", dataSize);
    fileDataPtr = writer.getDataPtr(dataSize);

    adjustPointers(dataMap, fileDataPtr);

    // Calculate these values at last possible moment to reduce possiblity of
    // data overflow (corruption).  The above operations take time.  Meanwhile
    // the Cine Buffer is catching up to our start point!
    maxFrame = mCineBuffer.getMaxFrameNum();
    minFrame = maxFrame - recordFrames + 1;
    ALOGD("minFrame: %u,  maxFrame: %u", minFrame, maxFrame);

    // Copy data from Cine Buffer to file
    writeFrames(minFrame, maxFrame, dataMap);

    // TODO: Need to fix up or rebuild new data file if corrupted frames are
    //       found.
    //
    {
        adjustPointers(dataMap, fileDataPtr);

        validateFile(dataMap, minFrame, maxFrame);
    }

    thorRet = THOR_OK;

err_ret:
    writer.close();

    mLock.enter();
    if (nullptr != mCompletionHandlerPtr)
    {
        mCompletionHandlerPtr(thorRet);
    }
    mLock.leave();
}

//-----------------------------------------------------------------------------
void CineRecorder::recordFrames(uint32_t minFrameNum, uint32_t maxFrameNum)
{
    ELAPSED_FUNC();

    ThorRawWriter       writer;
    DataMap             dataMap;
    ThorStatus          thorRet = THOR_ERROR;
    uint32_t            dataSize = 0;
    uint8_t*            fileDataPtr = nullptr;
    uint32_t            recordFrames = maxFrameNum - minFrameNum + 1;

    memset(&dataMap, 0, sizeof(dataMap));

    thorRet = createRawFile(writer, recordFrames);
    if (IS_THOR_ERROR(thorRet))
    {
        goto err_ret; 
    }

    dataSize = calculateSpace(recordFrames, dataMap);
    ALOGD("recordFrames() - dataSize: %u", dataSize);
    fileDataPtr = writer.getDataPtr(dataSize);

    adjustPointers(dataMap, fileDataPtr);
    writeFrames(minFrameNum, maxFrameNum, dataMap);

    thorRet = THOR_OK;

err_ret:
    writer.close();

    mLock.enter();
    if (nullptr != mCompletionHandlerPtr)
    {
        mCompletionHandlerPtr(thorRet);
    }
    mLock.leave();
}

//-----------------------------------------------------------------------------
void CineRecorder::recordCapnpFrames(uint32_t minFrameNum, uint32_t maxFrameNum)
{
    ELAPSED_FUNC();

    std::string capnpPath = mFilePath;

    // prepare scrollModeFrames (+ DA/ECG)
    processScrollModeFrames(maxFrameNum);

    ThorStatus thorRet = WriteThorCapnpFile(capnpPath.c_str(), mCineBuffer, minFrameNum, maxFrameNum);

    mLock.enter();
    if (nullptr != mCompletionHandlerPtr)
    {
        mCompletionHandlerPtr(thorRet);
    }
    mLock.leave();
}

//-----------------------------------------------------------------------------
void CineRecorder::processScrollModeFrames(uint32_t frameNum)
{
    if ( (IMAGING_MODE_M == mCineParams.imagingMode) ||  (IMAGING_MODE_PW == mCineParams.imagingMode)
         ||  (IMAGING_MODE_CW == mCineParams.imagingMode))
    {
        // update CineParams in CineBuffer
        mCineBuffer.getParams().mSingleFrameWidth = mScrollModeRecordParams.scrDataWidth;
        mCineBuffer.getParams().traceIndex = mScrollModeRecordParams.traceIndex;
    }

    if (BF_GET(mCineParams.dauDataTypes, DAU_DATA_TYPE_DA, 1) && mIsDaOn)
    {
        // update CineParams in CineBuffer
        mCineBuffer.getParams().daSingleFrameWidth = mDaRecordParams.frameWidth;
        mCineBuffer.getParams().daTraceIndex = mDaRecordParams.traceIndex;
    }

    if (BF_GET(mCineParams.dauDataTypes, DAU_DATA_TYPE_ECG, 1) && mIsEcgOn)
    {
        // update CineParams in CineBuffer
        mCineBuffer.getParams().ecgSingleFrameWidth = mEcgRecordParams.frameWidth;
        mCineBuffer.getParams().ecgTraceIndex = mEcgRecordParams.traceIndex;
    }
}

//-----------------------------------------------------------------------------
ThorStatus CineRecorder::createRawFile(ThorRawWriter& writer, uint32_t frameCount)
{
    ThorStatus                  thorRet = TER_MISC_CREATE_FILE_FAILED;
    ThorRawDataFile::Metadata   metadata;

    metadata.probeType = 0;
    metadata.imageCaseId = mCineParams.imagingCaseId;
    metadata.upsMajorVer = 2;
    metadata.upsMinorVer = 3;
    metadata.frameCount = frameCount;
    metadata.frameInterval = mCineParams.frameInterval;
    metadata.dataTypes = 0;

    switch (mCineParams.imagingMode)
    {
        case IMAGING_MODE_B:
        case IMAGING_MODE_COLOR:
        case IMAGING_MODE_M:
            metadata.dataTypes |= BIT(ThorRawDataFile::DataType::BMode);

            if (IMAGING_MODE_COLOR == mCineParams.imagingMode)
            {
                metadata.dataTypes |= BIT(ThorRawDataFile::DataType::CMode);
            }
            if (IMAGING_MODE_M == mCineParams.imagingMode)
            {
                metadata.dataTypes |= BIT(ThorRawDataFile::DataType::MMode);
            }
            break;

        default:
            break;
    }

    if (BF_GET(mCineParams.dauDataTypes, DAU_DATA_TYPE_DA, 1) && mIsDaOn)
    {
        metadata.dataTypes |= BIT(ThorRawDataFile::DataType::Auscultation);
    }
    if (BF_GET(mCineParams.dauDataTypes, DAU_DATA_TYPE_ECG, 1) && mIsEcgOn)
    {
        metadata.dataTypes |= BIT(ThorRawDataFile::DataType::ECG);
    }

    thorRet = writer.create(mFilePath.c_str(), metadata);
    if (IS_THOR_ERROR(thorRet))
    {
        ALOGE("Unable to create raw file");
        goto err_ret;
    }

err_ret:
    return(thorRet);
}

//-----------------------------------------------------------------------------
uint32_t CineRecorder::calculateSpace(uint32_t desiredFrames, DataMap& dataMap)
{
    uint32_t    spaceNeeded = 0;

    switch (mCineParams.imagingMode)
    {
        case IMAGING_MODE_B:
        case IMAGING_MODE_COLOR:
        case IMAGING_MODE_M:
            dataMap.bDataSize = sizeof(BModeRecordParams) + (desiredFrames * mBFrameSize);
            spaceNeeded += dataMap.bDataSize;
            dataMap.bDataPtr = nullptr;

            if (IMAGING_MODE_COLOR == mCineParams.imagingMode)
            {
                dataMap.cDataSize = sizeof(ColorModeRecordParams) + (desiredFrames * mCFrameSize);
                spaceNeeded += dataMap.cDataSize;
                dataMap.cDataPtr = nullptr;
                //dataMap.cDataPtr += dataMap.cDataSize;
            }
            if (IMAGING_MODE_M == mCineParams.imagingMode)
            {
                if (desiredFrames == 1)
                {
                    // single frame texture mode
                    dataMap.mDataSize = sizeof(MModeRecordParams) + (mTexFrameSize);
                }
                else
                {
                    dataMap.mDataSize = sizeof(MModeRecordParams) + (desiredFrames * mMFrameSize);
                }
                spaceNeeded += dataMap.mDataSize;
                dataMap.cDataPtr = nullptr;
            }
            break;

        default:
            break;
    }

    if (BF_GET(mCineParams.dauDataTypes, DAU_DATA_TYPE_DA, 1) && mIsDaOn)
    {
        if (desiredFrames == 1)
        {
            // single frame mode
            dataMap.daDataSize = sizeof(DaEcgRecordParams) + mDaScrFrameSize;
        }
        else
        {
            dataMap.daDataSize = sizeof(DaEcgRecordParams) + desiredFrames * mDaFrameSize;
        }
        spaceNeeded += dataMap.daDataSize;
        dataMap.daDataPtr = nullptr;
    }
    if (BF_GET(mCineParams.dauDataTypes, DAU_DATA_TYPE_ECG, 1) && mIsEcgOn)
    {
        if (desiredFrames == 1)
        {
            // single frame mode
            dataMap.ecgDataSize = sizeof(DaEcgRecordParams) + mEcgScrFrameSize;
        }
        else
        {
            dataMap.ecgDataSize = sizeof(DaEcgRecordParams) + desiredFrames * mEcgFrameSize;
        }
        spaceNeeded += dataMap.ecgDataSize;
        dataMap.ecgDataPtr = nullptr;
    }

    return(spaceNeeded);
}

//-----------------------------------------------------------------------------
void CineRecorder::adjustPointers(DataMap& dataMap, uint8_t* basePtr)
{
    uint8_t*    lastDataPtr = nullptr;
    uint32_t    lastDataSize = 0;

    // Assumes that space needed has already been calculated.
    dataMap.bDataPtr = basePtr;
    dataMap.cDataPtr = dataMap.bDataPtr + dataMap.bDataSize;
    //dataMap.pwDataPtr = dataMap.cDataPtr + dataMap.cDataSize;
    dataMap.mDataPtr = dataMap.bDataPtr + dataMap.bDataSize;
    //dataMap.cwDataPtr = dataMap.mDataPtr + dataMap.mDataSize;

    // Assume for now that DA/ECG only happens in B and C modes
    if (BF_GET(mCineParams.dauDataTypes, DAU_DATA_TYPE_DA, 1) && mIsDaOn)
    {
        if (IMAGING_MODE_B == mCineParams.imagingMode)
        {
            dataMap.daDataPtr = dataMap.bDataPtr + dataMap.bDataSize;
            lastDataPtr = dataMap.daDataPtr;
            lastDataSize = dataMap.daDataSize;
        }
        else if (IMAGING_MODE_COLOR == mCineParams.imagingMode)
        {
            dataMap.daDataPtr = dataMap.cDataPtr + dataMap.cDataSize;
            lastDataPtr = dataMap.daDataPtr;
            lastDataSize = dataMap.daDataSize;
        }
    }
    if (BF_GET(mCineParams.dauDataTypes, DAU_DATA_TYPE_ECG, 1) && mIsEcgOn)
    {
        if (IMAGING_MODE_B == mCineParams.imagingMode)
        {
            if (nullptr == lastDataPtr)
            {
                dataMap.ecgDataPtr = dataMap.bDataPtr + dataMap.bDataSize;
            }
            else
            {
                dataMap.ecgDataPtr = lastDataPtr + lastDataSize;
            }
        }
        else if (IMAGING_MODE_COLOR == mCineParams.imagingMode)
        {
            if (nullptr == lastDataPtr)
            {
                dataMap.ecgDataPtr = dataMap.cDataPtr + dataMap.cDataSize;
            }
            else
            {
                dataMap.ecgDataPtr = lastDataPtr + lastDataSize;
            }
        }
    }
}

//-----------------------------------------------------------------------------
uint32_t CineRecorder::writeFrames(uint32_t startFrame,
                                   uint32_t stopFrame,
                                   DataMap& dataMap)
{
    uint32_t    framesWritten = 0;
    uint8_t*    cineFramePtr = nullptr;
    uint32_t    curFrame = startFrame;
    BModeRecordParams     bModeRecordParams;
    ColorModeRecordParams cModeRecordParams;
    CineBuffer::CineFrameHeader* frameHeaderPtr = nullptr;

    ALOGD("startFrame: %u, stopFrame: %u", startFrame, stopFrame);

    // Write any mode specific meta data
    if (BF_GET(mCineParams.dauDataTypes, DAU_DATA_TYPE_B, 1))
    {
        bModeRecordParams.scanConverterParams = mCineParams.converterParams;

        memcpy(dataMap.bDataPtr,
               &bModeRecordParams,
               sizeof(BModeRecordParams));
        dataMap.bDataPtr += sizeof(BModeRecordParams);
    }
    if (IMAGING_MODE_COLOR == mCineParams.imagingMode)
    {
        // colormap data is not updated so update here
        mCineParams.colorMapIndex = mCineBuffer.getParams().colorMapIndex;
        mCineParams.isColorMapInverted = mCineBuffer.getParams().isColorMapInverted;

        cModeRecordParams.scanConverterParams = mCineParams.colorConverterParams;
        cModeRecordParams.colorMapIndex = mCineParams.colorMapIndex;
        cModeRecordParams.isInverted = mCineParams.isColorMapInverted;

        memcpy(dataMap.cDataPtr,
               &cModeRecordParams,
               sizeof(ColorModeRecordParams));
        dataMap.cDataPtr += sizeof(ColorModeRecordParams);
    }
    if (IMAGING_MODE_M == mCineParams.imagingMode)
    {   
        // update Params for the multi frame mode
        if (stopFrame != startFrame)
        {
            mScrollModeRecordParams.isScrFrame = false;
        }

        memcpy(dataMap.mDataPtr,
               &mScrollModeRecordParams,
               sizeof(MModeRecordParams));
        dataMap.mDataPtr += sizeof(MModeRecordParams);
    }


    if (BF_GET(mCineParams.dauDataTypes, DAU_DATA_TYPE_DA, 1) && mIsDaOn)
    {
        // process DA frames for the single frame mode.
        if (stopFrame != startFrame)
        {
            mDaRecordParams.isScrFrame = false;
            mDaRecordParams.traceIndex = -1;
            mDaRecordParams.frameWidth = mCineParams.daSamplesPerFrame;
        }
        else
        {
            // single Frame still texture mode
            mDaRecordParams.isScrFrame = true;
        }

        memcpy(dataMap.daDataPtr,
               &mDaRecordParams,
               sizeof(DaEcgRecordParams));
        dataMap.daDataPtr += sizeof(DaEcgRecordParams);
    }

    if (BF_GET(mCineParams.dauDataTypes, DAU_DATA_TYPE_ECG, 1) && mIsEcgOn)
    {
        // process ECG frames for the single frame mode.
        if (stopFrame != startFrame)
        {
            mEcgRecordParams.isScrFrame = false;
            mEcgRecordParams.traceIndex = -1;
            mEcgRecordParams.frameWidth = mCineParams.ecgSamplesPerFrame;
        }
        else
        {
            // single Frame still texture mode
            mEcgRecordParams.isScrFrame = true;
        }

        // ECG lead location: -1: onboard, 1,2,3: extLeadNum
        if (mCineParams.isEcgExternal)
        {
            mEcgRecordParams.location = mCineParams.ecgLeadNumExternal;
        }
        else
        {
            mEcgRecordParams.location = -1;
        }

        memcpy(dataMap.ecgDataPtr,
               &mEcgRecordParams,
               sizeof(DaEcgRecordParams));
        dataMap.ecgDataPtr += sizeof(DaEcgRecordParams);
    }

    // Copy data from Cine Buffer to file
    do
    {
        switch (mCineParams.imagingMode)
        {
            case IMAGING_MODE_B:
            case IMAGING_MODE_COLOR:
            case IMAGING_MODE_M:
                cineFramePtr = mCineBuffer.getFrame(curFrame,
                                                    DAU_DATA_TYPE_B,
                                                    CineBuffer::FrameContents::IncludeHeader);
                memcpy(dataMap.bDataPtr, cineFramePtr, mBFrameSize);
                dataMap.bDataPtr += mBFrameSize;

                if (IMAGING_MODE_COLOR == mCineParams.imagingMode)
                {
                    cineFramePtr = mCineBuffer.getFrame(curFrame,
                                                        DAU_DATA_TYPE_COLOR,
                                                        CineBuffer::FrameContents::IncludeHeader);
                    memcpy(dataMap.cDataPtr, cineFramePtr, mCFrameSize);
                    dataMap.cDataPtr += mCFrameSize;
                }
                if (IMAGING_MODE_M == mCineParams.imagingMode)
                {
                    if (stopFrame == startFrame)
                    {
                        // single frame texture mode
                        cineFramePtr = mCineBuffer.getFrame(TEX_FRAME_TYPE_M,
                                                            DAU_DATA_TYPE_TEX,
                                                            CineBuffer::FrameContents::IncludeHeader);
                        memcpy(dataMap.mDataPtr, cineFramePtr, mTexFrameSize);
                        dataMap.mDataPtr += mTexFrameSize;
                    }
                    else
                    {
                        // multiple Frames
                        cineFramePtr = mCineBuffer.getFrame(curFrame,
                                                            DAU_DATA_TYPE_M,
                                                            CineBuffer::FrameContents::IncludeHeader);
                        memcpy(dataMap.mDataPtr, cineFramePtr, mMFrameSize);
                        dataMap.mDataPtr += mMFrameSize;
                    }
                }
                break;

            default:
                break;
        }

        if (BF_GET(mCineParams.dauDataTypes, DAU_DATA_TYPE_DA, 1) && mIsDaOn)
        {
            if (stopFrame == startFrame)
            {
                // single frame texture/screen mode
                cineFramePtr = mCineBuffer.getFrame(0,
                                                    DAU_DATA_TYPE_DA_SCR,
                                                    CineBuffer::FrameContents::IncludeHeader);
                memcpy(dataMap.daDataPtr, cineFramePtr, mDaScrFrameSize);
                dataMap.daDataPtr += mDaScrFrameSize;
            }
            else
            {
                // multiple Frames
                cineFramePtr = mCineBuffer.getFrame(curFrame,
                                                    DAU_DATA_TYPE_DA,
                                                    CineBuffer::FrameContents::IncludeHeader);
                memcpy(dataMap.daDataPtr, cineFramePtr, mDaFrameSize);
                dataMap.daDataPtr += mDaFrameSize;
            }
        }
        if (BF_GET(mCineParams.dauDataTypes, DAU_DATA_TYPE_ECG, 1) && mIsEcgOn)
        {
            if (stopFrame == startFrame)
            {
                // single frame texture/screen mode
                cineFramePtr = mCineBuffer.getFrame(0,
                                                    DAU_DATA_TYPE_ECG_SCR,
                                                    CineBuffer::FrameContents::IncludeHeader);
                memcpy(dataMap.ecgDataPtr, cineFramePtr, mEcgScrFrameSize);
                dataMap.ecgDataPtr += mEcgScrFrameSize;
            }
            else
            {
                cineFramePtr = mCineBuffer.getFrame(curFrame,
                                                    DAU_DATA_TYPE_ECG,
                                                    CineBuffer::FrameContents::IncludeHeader);
                memcpy(dataMap.ecgDataPtr, cineFramePtr, mEcgFrameSize);
                dataMap.ecgDataPtr += mEcgFrameSize;
            }
        }

        if (mCancelEvent.wait(0))
        {
            // Stop recording and exit
            //
            // TODO: Is this a cancel and delete or stop recording?
            //       If the later, then need to maybe create second file
            //       to compact space.
            break;
        }
        framesWritten++;
    } while (++curFrame <= stopFrame);

    return(framesWritten);
}

//-----------------------------------------------------------------------------
bool CineRecorder::validateFile(DataMap& dataMap,
                                uint32_t startFrame,
                                uint32_t endFrame)
{
    // Validate enough of the early frames to ensure no overflow corruption
    // 
    // Should we check all frames?
    //
    bool                retVal = false;
    uint32_t            curFrame;
    const uint32_t      CHECK_FRAME_COUNT = 150;

    dataMap.bDataPtr += sizeof(BModeRecordParams);
    dataMap.cDataPtr += sizeof(ColorModeRecordParams);

    for (curFrame = startFrame;
         curFrame < MIN(endFrame, (startFrame + CHECK_FRAME_COUNT));
         curFrame++)
    {
        uint32_t                        frameNum;
        CineBuffer::CineFrameHeader*    frameHeaderPtr = nullptr;

        switch (mCineParams.imagingMode)
        {
            case IMAGING_MODE_B:
            case IMAGING_MODE_COLOR:
            case IMAGING_MODE_M:
                frameHeaderPtr = (CineBuffer::CineFrameHeader*) dataMap.bDataPtr;
                if (frameHeaderPtr->frameNum != curFrame)
                {
                    ALOGE("Data is corrupt in B-Mode frame %u.  Read back: "
                          "%u, expected %u",
                          curFrame - startFrame,
                          frameHeaderPtr->frameNum,
                          curFrame);
                    goto err_ret;
                }
                dataMap.bDataPtr += mBFrameSize;

                if (IMAGING_MODE_COLOR == mCineParams.imagingMode)
                {
                    frameHeaderPtr = (CineBuffer::CineFrameHeader*) dataMap.cDataPtr;
                    if (frameHeaderPtr->frameNum != curFrame)
                    {
                        ALOGE("Data is corrupt in C-Mode frame %u.  Read back: "
                              "%u, expected %u",
                              curFrame - startFrame,
                              frameHeaderPtr->frameNum,
                              curFrame);
                        goto err_ret;
                    }
                    dataMap.cDataPtr += mCFrameSize;
                }
                if (IMAGING_MODE_M == mCineParams.imagingMode)
                {
                    frameHeaderPtr = (CineBuffer::CineFrameHeader*) dataMap.mDataPtr;
                    if (frameHeaderPtr->frameNum != curFrame)
                    {
                        ALOGE("Data is corrupt in M-Mode frame %u.  Read back: "
                              "%u, expected %u",
                              curFrame - startFrame,
                              frameHeaderPtr->frameNum,
                              curFrame);
                        goto err_ret;
                    }
                    dataMap.mDataPtr += mMFrameSize;
                }
                break;

            default:
                break;
        }
    }

    retVal = true;

err_ret:
    return(retVal); 
}


