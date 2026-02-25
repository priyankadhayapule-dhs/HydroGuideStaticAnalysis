//
// Copyright 2020 EchoNous Inc.
//
//
#define LOG_TAG "DopplerPeakMeanProcessHandler"

#include <DopplerPeakMeanProcessHandler.h>
#include <ThorUtils.h>

#define MAX_ALLOWED_DIFFERENCE 18

// post process
#define REL_THRESHOLD 0.1f
#define ABS_THRESHOLD 255.0f

DopplerPeakMeanProcessHandler::DopplerPeakMeanProcessHandler(const std::shared_ptr<UpsReader> &upsReader) :
        mPeakMaxProcess(upsReader),
        mPeakMeanPreProcess(upsReader),
        mPeakMeanPostProcess(upsReader),
        mPeakMeanProcess(upsReader)
{

    mCineBuffer = nullptr;

    // dataType default
    mInputDataType = DAU_DATA_TYPE_PW;
    mOutputDataType = DAU_DATA_TYPE_DOP_PM;     // Doppler peak mean
}

DopplerPeakMeanProcessHandler::~DopplerPeakMeanProcessHandler()
{
}

ThorStatus DopplerPeakMeanProcessHandler::init()
{
    mPeakMaxProcess.init();
    mPeakMeanPreProcess.init();
    mPeakMeanPostProcess.init();
    mPeakMeanProcess.init();

    mLastProcessedFrameNum = -1u;
    mLastFrameNum = 0;
    mFrameLatency = 5;
    return THOR_OK;
}

void DopplerPeakMeanProcessHandler::setCineBuffer(CineBuffer *cineBuffer)
{
    mCineBuffer = cineBuffer;
}

void DopplerPeakMeanProcessHandler::setProcessIndices(uint32_t numFFT, float refGaindB, float threshold)
{
    mPeakMaxProcess.setProcessIndices(numFFT, refGaindB, threshold);
    mPeakMeanPostProcess.setProcessIndices(numFFT, REL_THRESHOLD, ABS_THRESHOLD);
    mPeakMeanProcess.setProcessIndices(numFFT, refGaindB, threshold);
}

void DopplerPeakMeanProcessHandler::setDataType(int inputType, int outputType)
{
    mInputDataType = inputType;
    mOutputDataType = outputType;
}

void DopplerPeakMeanProcessHandler::setWFGain(float dBGain)
{
    mPeakMaxProcess.setWFGain(dBGain);
    mPeakMeanProcess.setWFGain(dBGain);
}

void DopplerPeakMeanProcessHandler::setFrameLatency(uint32_t frameLatency)
{
    mFrameLatency = frameLatency;
}

ThorStatus DopplerPeakMeanProcessHandler::process(uint32_t frameNum)
{
    mLastFrameNum = frameNum;

    if (frameNum < mFrameLatency)
        return THOR_OK;

    // current frame with latency
    uint32_t curFrame = frameNum - mFrameLatency;

    mPeakMaxProcess.process(mCineBuffer->getFrame(curFrame, mInputDataType),
                             mCineBuffer->getFrame(curFrame, mOutputDataType));

    mLastProcessedFrameNum = curFrame;

    return THOR_OK;
}

ThorStatus DopplerPeakMeanProcessHandler::processAllFrames(bool singleFrameMode)
{
    uint8_t* outputPtr;
    int      frameWidth = 0;
    mLastFrameNum = mCineBuffer->getMaxFrameNum();

    if (singleFrameMode)
        mLastFrameNum = 0;

    // reset pos/neg counter
    mPeakMaxProcess.resetCounter();

    if (mLastFrameNum > 0)
    {
        for (int i = mCineBuffer->getMinFrameNum(); i < (mLastFrameNum + 1); i++)
        {
            mPeakMaxProcess.process(mCineBuffer->getFrame(i, mInputDataType),
                                    mCineBuffer->getFrame(i, mOutputDataType));
        }
    }
    else
    {
        // singleFrame from Texture Memory
        int inputFrameType = TEX_FRAME_TYPE_PW;
        if (mInputDataType == DAU_DATA_TYPE_CW)
            inputFrameType = TEX_FRAME_TYPE_CW;

        uint8_t* orgInPtr = mCineBuffer->getFrame(inputFrameType, DAU_DATA_TYPE_TEX);
        uint8_t* processInPtr = orgInPtr;
        uint8_t* tmpInPtr = nullptr;
        uint8_t* tmpOutPtr = nullptr;
        uint8_t* tmpOutPtrImg = nullptr;
        uint8_t* outPtrOrg = nullptr;
        uint8_t* outPtrOrgImg = nullptr;            // output ptr with unprocessed input
        CineBuffer::CineFrameHeader* inputHeaderPtr = nullptr;
        CineBuffer::CineFrameHeader* outputHeaderPtr = nullptr;

        if (mOutputDataType == DAU_DATA_TYPE_DPMAX_SCR)
        {
            frameWidth = mCineBuffer->getParams().mSingleFrameWidth;

            if (frameWidth < 1500)
                frameWidth = 1500;

            if (frameWidth > MAX_DOP_PEAK_MEAN_SCR_SAMPLE_SIZE)
                frameWidth = MAX_DOP_PEAK_MEAN_SCR_SAMPLE_SIZE;

            outputPtr = mCineBuffer->getFrame(0, DAU_DATA_TYPE_DPMAX_SCR);
            memset(outputPtr, 0, frameWidth * sizeof(float) * 2);

            // preprocess - making binary image
            ImageProcess::ProcessParams pParams;

            pParams.imagingCaseId = mCineBuffer->getParams().imagingCaseId;
            pParams.imagingMode = mCineBuffer->getParams().imagingMode;
            pParams.numSamples = mCineBuffer->getParams().dopplerFFTSize;
            pParams.numRays = frameWidth;

            mPeakMeanPreProcess.setProcessParams(pParams);
            mPeakMeanPreProcess.setDataType(mInputDataType);

            // prepare output
            tmpInPtr = mCineBuffer->getFrame(inputFrameType, DAU_DATA_TYPE_TEX);
            tmpOutPtr = new uint8_t[pParams.numSamples * (frameWidth+1)];
            tmpOutPtrImg = tmpOutPtr + pParams.numSamples;                      // without header

            // pre process - closing operation
            mPeakMeanPreProcess.process(tmpInPtr, tmpOutPtrImg);

            // input header pointer
            inputHeaderPtr =
                    reinterpret_cast<CineBuffer::CineFrameHeader*>(tmpInPtr - sizeof(CineBuffer::CineFrameHeader));

            // output header pointer
            outputHeaderPtr =
                    reinterpret_cast<CineBuffer::CineFrameHeader*>(tmpOutPtrImg - sizeof(CineBuffer::CineFrameHeader));

            // copy header contents
            memcpy(outputHeaderPtr, inputHeaderPtr, sizeof(CineBuffer::CineFrameHeader));

            // update processInPtr - tmpOutPtrImg = input with pre-Processing
            processInPtr = tmpOutPtrImg;

            mPeakMaxProcess.process(processInPtr,
                                    mCineBuffer->getFrame(0, mOutputDataType));

            //////////////////////////////////
            // output without pre-processing
            outPtrOrg = new uchar[MAX_DOP_PEAK_MEAN_SCR_SIZE + sizeof(CineBuffer::CineFrameHeader)];
            outPtrOrgImg = outPtrOrg + sizeof(CineBuffer::CineFrameHeader);

            // outputHeaderPtr
            outputHeaderPtr =
                    reinterpret_cast<CineBuffer::CineFrameHeader*>(outPtrOrgImg - sizeof(CineBuffer::CineFrameHeader));

            // copy header contents
            memcpy(outputHeaderPtr, inputHeaderPtr, sizeof(CineBuffer::CineFrameHeader));

            // process with original image (without pre-processing)
            mPeakMaxProcess.process(orgInPtr, outPtrOrgImg);

            //////////////////////////////////
            // Post process
            mPeakMeanPostProcess.setProcessParams(pParams);
            mPeakMeanPostProcess.setDataType(mInputDataType);

            mPeakMeanPostProcess.process(outPtrOrgImg, mCineBuffer->getFrame(0, mOutputDataType));

            // PeakMean process
            // copy MAX data to MEAN data (as output overwrites)
            memcpy(mCineBuffer->getFrame(0, DAU_DATA_TYPE_DPMEAN_SCR, CineBuffer::FrameContents::IncludeHeader),
                   mCineBuffer->getFrame(0, mOutputDataType, CineBuffer::FrameContents::IncludeHeader),
                   MAX_DOP_PEAK_MEAN_SCR_SIZE + sizeof(CineBuffer::CineFrameHeader));
            mPeakMeanProcess.setProcessParams(pParams);
            mPeakMeanProcess.process(orgInPtr, mCineBuffer->getFrame(0, DAU_DATA_TYPE_DPMEAN_SCR));
        }
        else
        {
            mPeakMaxProcess.process(processInPtr,
                                    mCineBuffer->getFrame(0, mOutputDataType));
        }

        if (outPtrOrg != nullptr)
            delete[] outPtrOrg;

        if (tmpOutPtr != nullptr)
            delete[] tmpOutPtr;

    }   // end else - single frame texture

    mLastProcessedFrameNum = mLastFrameNum;

    updateVMax(singleFrameMode);

    return THOR_OK;
}

void DopplerPeakMeanProcessHandler::updateVMax(bool singleFrameMode)
{
    float vMax = -100.0f;
    CineBuffer::CineFrameHeader* inputHeaderPtr;
    uint32_t   numLinesPerFrame;
    float*     inputPtrFloat;
    float      curVal;
    int        startFrameNum, endFrameNum;

    if (singleFrameMode)
    {
        startFrameNum = 0;
        endFrameNum = 0;
    }
    else
    {
        startFrameNum = mCineBuffer->getMinFrameNum();
        endFrameNum = mCineBuffer->getMaxFrameNum();
    }

    for (int i = startFrameNum; i < (endFrameNum + 1); i++)
    {
        inputHeaderPtr = reinterpret_cast<CineBuffer::CineFrameHeader*>(mCineBuffer->getFrame(i, mOutputDataType,
                CineBuffer::FrameContents::IncludeHeader));
        numLinesPerFrame = inputHeaderPtr->numSamples;

        inputPtrFloat = (float*) mCineBuffer->getFrame(i, mOutputDataType);

        for (int j = 0; j < numLinesPerFrame; j++)
        {
            curVal = *inputPtrFloat;

            if (vMax < curVal)
                vMax = curVal;

            // peak/mean interleaved
            inputPtrFloat += 2;
        }
    }

    mCineBuffer->getParams().dopplerPeakMax = vMax;
}

void DopplerPeakMeanProcessHandler::flush()
{
    // when no frame is processed
    if (mLastProcessedFrameNum == -1u)
        return;

    // process remaining frames
    for (int i = mLastProcessedFrameNum + 1; i < (mLastFrameNum + 1); i++)
        mPeakMaxProcess.process(mCineBuffer->getFrame(i, mInputDataType),
                                 mCineBuffer->getFrame(i, mOutputDataType));

    updateVMax();

    // reset LastProcessedFrameNum
    mLastProcessedFrameNum = -1u;
}

void DopplerPeakMeanProcessHandler::terminate()
{
    mPeakMaxProcess.terminate();
    mPeakMeanPreProcess.terminate();
    mPeakMeanPostProcess.terminate();
    mPeakMeanProcess.terminate();
}
