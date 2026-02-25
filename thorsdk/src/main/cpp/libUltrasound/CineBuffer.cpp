//
// Copyright 2017 EchoNous Inc.
//
//

#define LOG_TAG "CineBuffer"

#include <ThorUtils.h>
#include <ThorConstants.h>
#include <CineBuffer.h>
#include <BitfieldMacros.h>

//-----------------------------------------------------------------------------
CineBuffer::CineBuffer() :
    mCurFrameNum(UINT32_MAX),
    mMaxFrameNum(0),
    mB_ModeBuffer(nullptr),
    mColorBuffer(nullptr),
    mM_ModeBuffer(nullptr),
    mTexBuffer(nullptr),
    mDaBuffer(nullptr),
    mEcgBuffer(nullptr),
    mDaScrBuffer(nullptr),
    mEcgScrBuffer(nullptr)
{
}

//-----------------------------------------------------------------------------
CineBuffer::~CineBuffer()
{
    ALOGD("%s", __func__);
}

//-----------------------------------------------------------------------------
void CineBuffer::reset()
{
    mLock.enter();
    mCurFrameNum = UINT32_MAX;
    mMaxFrameNum = 0;
    memset(mCompletions, 0, sizeof(mCompletions));
    mLock.leave();
}

//-----------------------------------------------------------------------------
void CineBuffer::setTransitionReadyFlag(bool isReady)
{
    mParams.isTransitionRenderingReady = isReady;
}

//-----------------------------------------------------------------------------
void CineBuffer::setRenderedTransitionImgMode(int imgMode)
{
    mParams.renderedTransitionImgMode = imgMode;
}

//-----------------------------------------------------------------------------
ThorStatus CineBuffer::prepareClients()
{
    ThorStatus  retVal = THOR_OK;

    for (auto it = mCallbackList.begin();
         it != mCallbackList.end();
         it++ )
    {
        retVal = (*it)->onInit();
        if (IS_THOR_ERROR(retVal))
        {
            break;
        }
    }

    return(retVal); 
}

//-----------------------------------------------------------------------------
void CineBuffer::freeClients()
{
    for (auto it = mCallbackList.begin();
         it != mCallbackList.end();
         it++ )
    {
        (*it)->onTerminate();
    }
}

//-----------------------------------------------------------------------------
void CineBuffer::setCineParamsOnly(CineParams &params)
{
    mParams = params;
}

//-----------------------------------------------------------------------------
void CineBuffer::setParams(CineParams& params)
{
    uint8_t*    lastTypeBuffer = nullptr;

    mParams = params;

    ALOGD("mDauDataTypes = %08x", mParams.dauDataTypes);

    mB_ModeBuffer = nullptr;
    mColorBuffer = nullptr;
    mM_ModeBuffer = nullptr;
    mTexBuffer = nullptr;
    mPw_ModeFrameBuffer = nullptr;
    mPw_ModeAdoFrameBuffer = nullptr;
    mCw_ModeFrameBuffer = nullptr;
    mCw_ModeAdoFrameBuffer = nullptr;

    // Setup Texture Buffer & its Header at CineBuffer[0]
    mTexBuffer = &mCineBuffer[0];
    mTexFrameSize = (MAX_TEX_FRAME_SIZE + CINE_HEADER_SIZE);

    // Doppler PeakMax Scr (need this for B/B+C in duplex mode)
    mDopPeakMaxScrFrameBuffer = mTexBuffer + (TEX_FRAME_COUNT * mTexFrameSize);
    lastTypeBuffer = mDopPeakMaxScrFrameBuffer + MAX_DOP_PEAK_MEAN_SCR_SIZE + CINE_HEADER_SIZE;

    // Doppler PeakMean Scr
    mDopPeakMeanScrFrameBuffer = lastTypeBuffer;
    lastTypeBuffer = mDopPeakMeanScrFrameBuffer + MAX_DOP_PEAK_MEAN_SCR_SIZE + CINE_HEADER_SIZE;

    switch (params.imagingMode)
    {
        case IMAGING_MODE_B:
            mB_ModeBuffer = lastTypeBuffer;
            mB_ModeFrameSize = MAX_B_FRAME_SIZE + CINE_HEADER_SIZE;

            lastTypeBuffer = mB_ModeBuffer + (CINE_FRAME_COUNT * mB_ModeFrameSize);
            break;

        case IMAGING_MODE_COLOR:
            mB_ModeBuffer = lastTypeBuffer;
            mB_ModeFrameSize = MAX_B_FRAME_SIZE + CINE_HEADER_SIZE;

            mColorBuffer = mB_ModeBuffer + (CINE_FRAME_COUNT * mB_ModeFrameSize);
            mColorFrameSize = MAX_COLOR_FRAME_SIZE + CINE_HEADER_SIZE;

            lastTypeBuffer = mColorBuffer + (CINE_FRAME_COUNT * mColorFrameSize);
            break;

        case IMAGING_MODE_M:
            mB_ModeBuffer = lastTypeBuffer;
            mB_ModeFrameSize = MAX_B_FRAME_SIZE + CINE_HEADER_SIZE;

            mM_ModeBuffer = mB_ModeBuffer + (CINE_FRAME_COUNT * mB_ModeFrameSize);
            mM_ModeFrameSize = MAX_M_FRAME_SIZE + CINE_HEADER_SIZE;

            lastTypeBuffer = mM_ModeBuffer + (CINE_FRAME_COUNT * mM_ModeFrameSize);
            break;

        case IMAGING_MODE_PW:
            mPw_ModeFrameBuffer = lastTypeBuffer;
            mPw_ModeFrameSize = MAX_PW_FRAME_SIZE + CINE_HEADER_SIZE;

            mPw_ModeAdoFrameBuffer = mPw_ModeFrameBuffer + (CINE_FRAME_COUNT * mPw_ModeFrameSize);
            mPw_ModeAdoFrameSize = MAX_PW_ADO_FRAME_SIZE + CINE_HEADER_SIZE;

            lastTypeBuffer = mPw_ModeAdoFrameBuffer + (CINE_FRAME_COUNT * mPw_ModeAdoFrameSize);
            break;

        case IMAGING_MODE_CW:
            mCw_ModeFrameBuffer = lastTypeBuffer;
            mCw_ModeFrameSize = MAX_CW_FRAME_SIZE + CINE_HEADER_SIZE;

            mCw_ModeAdoFrameBuffer = mCw_ModeFrameBuffer + (CINE_FRAME_COUNT * mCw_ModeFrameSize);
            mCw_ModeAdoFrameSize = MAX_CW_ADO_FRAME_SIZE + CINE_HEADER_SIZE;

            lastTypeBuffer = mCw_ModeAdoFrameBuffer + (CINE_FRAME_COUNT * mCw_ModeAdoFrameSize);
            break;
    }

    // add peak mean buffer if PW or CW
    if ((params.imagingMode == IMAGING_MODE_PW) || (params.imagingMode == IMAGING_MODE_CW))
    {
        mDopPeakMeanFrameBuffer = lastTypeBuffer;
        mDopPeakMeanFrameSize = MAX_DOP_PEAK_MEAN_SIZE + CINE_HEADER_SIZE;

        lastTypeBuffer = mDopPeakMeanFrameBuffer + (CINE_FRAME_COUNT * mDopPeakMeanFrameSize);
    }

    if (0 != BF_GET(mParams.dauDataTypes, DAU_DATA_TYPE_DA, 1))
    {
        mDaBuffer = lastTypeBuffer;
        mDaFrameSize = MAX_DA_FRAME_SIZE + CINE_HEADER_SIZE;

        mDaScrBuffer = mDaBuffer + (CINE_FRAME_COUNT * mDaFrameSize);

        lastTypeBuffer = mDaScrBuffer + MAX_DA_SCR_FRAME_SIZE + CINE_HEADER_SIZE;   //only one frame
    }

    if (0 != BF_GET(mParams.dauDataTypes, DAU_DATA_TYPE_ECG, 1))
    {
        mEcgBuffer = lastTypeBuffer;
        mEcgFrameSize = MAX_ECG_FRAME_SIZE + CINE_HEADER_SIZE;

        mEcgScrBuffer = mEcgBuffer + (CINE_FRAME_COUNT * mEcgFrameSize);
    }

    for (auto it = mCallbackList.begin();
         it != mCallbackList.end();
         it++ )
    {
        (*it)->setParams(params);
    }
}

//-----------------------------------------------------------------------------
CineBuffer::CineParams& CineBuffer::getParams()
{
    return(mParams); 
}

//-----------------------------------------------------------------------------
void CineBuffer::setBoundary(uint32_t minFrameNum, uint32_t maxFrameNum)
{
    // TODO: minFrameNum is being ignored because it is normally calculated
    //       from maxFrameNum.  Need to resolve this so that the contents
    //       of the CineBuffer can be "fenced" to playback only the selected
    //       portion.
    //
    //       Consider putting both values into separate variables and leaving
    //       original min/max values alone.

    ALOGD("Setting minFrame to %u and maxFrame to %u", minFrameNum, maxFrameNum);
    mLock.enter();
    mMaxFrameNum = maxFrameNum;
    mLock.leave();
}

//-----------------------------------------------------------------------------
void CineBuffer::setCurFrame(uint32_t frameNum)
{
    ALOGD("Setting curFrame to %u", frameNum);
    mLock.enter();
    mCurFrameNum = frameNum;
    mLock.leave();
}

//-----------------------------------------------------------------------------
uint32_t CineBuffer::getCurFrameNum()
{
    uint32_t    curFrameNum;

    mLock.enter();
    curFrameNum = mCurFrameNum;
    mLock.leave();

    return(curFrameNum); 
}

//-----------------------------------------------------------------------------
uint32_t CineBuffer::getMinFrameNum()
{
    uint32_t    minFrameNum = 0;
    uint32_t    maxFrameNum;

    maxFrameNum = getMaxFrameNum();
    if (maxFrameNum >= CINE_FRAME_COUNT)
    {
        minFrameNum = maxFrameNum - (CINE_FRAME_COUNT - 1);
    }

    return(minFrameNum);
}

//-----------------------------------------------------------------------------
uint32_t CineBuffer::getMaxFrameNum()
{
    uint32_t    maxFrameNum;

    mLock.enter();
    maxFrameNum = mMaxFrameNum;
    mLock.leave();

    return(maxFrameNum); 
}

//-----------------------------------------------------------------------------
uint8_t* CineBuffer::getCurFrame(uint32_t dauDataType)
{
    uint32_t    curFrameNum;

    mLock.enter();
    curFrameNum = mCurFrameNum;
    mLock.leave();

    return(getFrame(curFrameNum, dauDataType));
}

//-----------------------------------------------------------------------------
uint8_t* CineBuffer::getFrame(uint32_t frameNum,
                              uint32_t dauDataType,
                              FrameContents contents)
{
    uint8_t*    framePtr = nullptr;
    uint8_t     headerOffset = CINE_HEADER_SIZE;
    uint32_t    frameOffset;

    if (FrameContents::IncludeHeader == contents)
    {
        headerOffset = 0;
    }

    switch (dauDataType)
    {
        case DAU_DATA_TYPE_B:
            frameOffset = (frameNum % CINE_FRAME_COUNT) * mB_ModeFrameSize;
            framePtr = &mB_ModeBuffer[frameOffset + headerOffset];
            break;

        case DAU_DATA_TYPE_COLOR:
            frameOffset = (frameNum % CINE_FRAME_COUNT) * mColorFrameSize;
            framePtr = &mColorBuffer[frameOffset + headerOffset];
            break;

        case DAU_DATA_TYPE_PW:
            frameOffset = (frameNum % CINE_FRAME_COUNT) * mPw_ModeFrameSize;
            framePtr = &mPw_ModeFrameBuffer[frameOffset + headerOffset];
            break;

        case DAU_DATA_TYPE_PW_ADO:
            frameOffset = (frameNum % CINE_FRAME_COUNT) * mPw_ModeAdoFrameSize;
            framePtr = &mPw_ModeAdoFrameBuffer[frameOffset + headerOffset];
            break;

        case DAU_DATA_TYPE_M:
            frameOffset = (frameNum % CINE_FRAME_COUNT) * mM_ModeFrameSize;
            framePtr = &mM_ModeBuffer[frameOffset + headerOffset];
            break;

        case DAU_DATA_TYPE_CW:
            frameOffset = (frameNum % CINE_FRAME_COUNT) * mCw_ModeFrameSize;
            framePtr = &mCw_ModeFrameBuffer[frameOffset + headerOffset];
            break;

        case DAU_DATA_TYPE_CW_ADO:
            frameOffset = (frameNum % CINE_FRAME_COUNT) * mCw_ModeAdoFrameSize;
            framePtr = &mCw_ModeAdoFrameBuffer[frameOffset + headerOffset];
            break;

        case DAU_DATA_TYPE_DOP_PM:
            frameOffset = (frameNum % CINE_FRAME_COUNT) * mDopPeakMeanFrameSize;
            framePtr = &mDopPeakMeanFrameBuffer[frameOffset + headerOffset];
            break;

        case DAU_DATA_TYPE_DPMAX_SCR:
            frameOffset = 0;
            framePtr = &mDopPeakMaxScrFrameBuffer[frameOffset + headerOffset];
            break;

        case DAU_DATA_TYPE_DPMEAN_SCR:
            frameOffset = 0;
            framePtr = &mDopPeakMeanScrFrameBuffer[frameOffset + headerOffset];
            break;

        case DAU_DATA_TYPE_TEX:
            frameOffset = (frameNum % TEX_FRAME_COUNT) * mTexFrameSize;
            framePtr = &mTexBuffer[frameOffset + headerOffset];
            break;

        case DAU_DATA_TYPE_DA_SCR:
            frameOffset = 0;
            framePtr = &mDaScrBuffer[frameOffset + headerOffset];
            break;

        case DAU_DATA_TYPE_ECG_SCR:
            frameOffset = 0;
            framePtr = &mEcgScrBuffer[frameOffset + headerOffset];
            break;

        case DAU_DATA_TYPE_DA:
            frameOffset = (frameNum % CINE_FRAME_COUNT) * mDaFrameSize;
            framePtr = &mDaBuffer[frameOffset + headerOffset];
            break;

        case DAU_DATA_TYPE_ECG:
            frameOffset = (frameNum % CINE_FRAME_COUNT) * mEcgFrameSize;
            framePtr = &mEcgBuffer[frameOffset + headerOffset];
            break;

        default:
            break;
    }

    return(framePtr); 
}

//-----------------------------------------------------------------------------
void CineBuffer::setFrameComplete(uint32_t frameNum, uint32_t dauDataType)
{
    bool        completed = false;
    uint32_t    index = frameNum % CINE_FRAME_COUNT;

    mLock.enter();

    if ((frameNum > mCurFrameNum) || (UINT32_MAX == mCurFrameNum))
    {
        mCompletions[index] |= BIT(dauDataType);

        if (mParams.dauDataTypes == mCompletions[index])
        {
            completed = true;
            mCompletions[index] = 0;
            mCurFrameNum = frameNum;
            mMaxFrameNum = mCurFrameNum;
        }

        mLock.leave();

        if (completed)
        {
            for (auto it = mCallbackList.begin();
                 it != mCallbackList.end();
                 it++ )
            {
                (*it)->onData(frameNum, mParams.dauDataTypes);
            }
        }
    }
    else
    {
        mCompletions[index] = 0;
        mLock.leave();
    }
}

//-----------------------------------------------------------------------------
void CineBuffer::replayFrame(uint32_t frameNum)
{
    mLock.enter();
    if (UINT32_MAX == mCurFrameNum)
    {
        // CineBuffer is empty.  Nothing to replay.
        frameNum = UINT32_MAX;
    }
    else
    {
        mCurFrameNum = frameNum;
    }
    mLock.leave();

    if (UINT32_MAX != frameNum)
    {
        for (auto it = mCallbackList.begin();
             it != mCallbackList.end();
             it++ )
        {
            (*it)->onData(frameNum, mParams.dauDataTypes);
        }
    }
}

//-----------------------------------------------------------------------------
void CineBuffer::pausePlayback()
{
    mLock.enter();

    for (auto it = mCallbackList.begin();
         it != mCallbackList.end();
         it++ )
    {
        (*it)->pausePlayback();
    }

    mLock.leave();
}

//-----------------------------------------------------------------------------
void CineBuffer::registerCallback(CineCallback* callbackPtr)
{
    bool    found = false;

    // Ensure this callback is not in the list
    for (auto it = mCallbackList.begin();
         it != mCallbackList.end();
         it++ )
    {
        if (*it == callbackPtr)
        {
            found = true;
            break;
        }
    }

    if (!found)
    {
        mCallbackList.push_back(callbackPtr);
    }
}

//-----------------------------------------------------------------------------
void CineBuffer::unregisterCallback(CineCallback* callbackPtr)
{
    for (auto it = mCallbackList.begin();
         it != mCallbackList.end();
         it++ )
    {
        if (*it == callbackPtr)
        {
            mCallbackList.erase(it);
            break;
        }
    }
}

//-----------------------------------------------------------------------------
void CineBuffer::onSave(uint32_t startFrame, uint32_t endFrame,
                        echonous::capnp::ThorClip::Builder &builder)
{
    for (auto *callback : mCallbackList) {
        callback->onSave(startFrame, endFrame, builder);
    }
}


//-----------------------------------------------------------------------------
void CineBuffer::onLoad(echonous::capnp::ThorClip::Reader &reader)
{
    for (auto *callback : mCallbackList) {
        callback->onLoad(reader);
    }
}