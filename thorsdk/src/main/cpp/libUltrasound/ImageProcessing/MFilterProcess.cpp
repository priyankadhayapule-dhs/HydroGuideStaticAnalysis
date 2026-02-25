//
// Copyright 2018 EchoNous Inc.
//
//
#define LOG_TAG "MFilterProcess"

#include <MFilterProcess.h>
#include <ThorUtils.h>

MFilterProcess::MFilterProcess(const std::shared_ptr<UpsReader> &upsReader) :
    ImageProcess(upsReader)
{
    mLinesToAverage = 2;
}

ThorStatus MFilterProcess::init()
{
    return THOR_OK;
}

ThorStatus MFilterProcess::setProcessParams(ProcessParams &params)
{
    mParams = params;
    LOGD("MFilterProcess::setProcessParams(): numRays = %d", mParams.numRays);
    return THOR_OK;
}

void MFilterProcess::setScrollSpeed(uint32_t imagingCaseId, uint32_t scrollSpeedIndex)
{
    
    mLinesToAverage = getMLinesToAverage(scrollSpeedIndex);
    mNumInputLinesPerFrame = getUpsReader()->getNumTxBeamsB(imagingCaseId, IMAGING_MODE_M);
    mOutputLinesPerFrame = mNumInputLinesPerFrame / mLinesToAverage;
    LOGD("MFilterProcess:setScrollSpeed(%d), mInputLinesPerFrame = %d, mLinesToAverage = %d, mOutputLinesPerFrame = %d",
         scrollSpeedIndex,
         mNumInputLinesPerFrame,
         mLinesToAverage,
         mOutputLinesPerFrame);
}

//#define USE_MMODE_TEST_PATTERN

ThorStatus MFilterProcess::process(uint8_t *inputPtr, uint8_t *outputPtr)
{
    float avgFactor = 1.0 / mLinesToAverage;

    for (uint32_t bi = 0; bi < mOutputLinesPerFrame; bi++)  // block loop
    {
        // clear averaging buffer
        memset( mAvgBuf, 0, sizeof(mAvgBuf)); 

        uint8_t* iPtr = inputPtr + (bi*mParams.numSamples*mLinesToAverage);
        for (uint32_t li = 0; li < mLinesToAverage; li++) // lines per block
        {
            for (uint32_t si = 0; si < mParams.numSamples; si++)  // samples per line
            {
                mAvgBuf[si] += *iPtr++;
            }
        }

        // normalize & clamp output
        uint8_t* oPtr = outputPtr + bi*mParams.numSamples;
#ifndef USE_MMODE_TEST_PATTERN            
        for (uint32_t si = 0; si < mParams.numSamples; si++)
        {
            uint32_t avgVal = mAvgBuf[si]*avgFactor;
            *oPtr++ = (avgVal > 255) ? 255 : avgVal;
        }
#else
        static uint8_t startValPersist = 255;
        uint32_t startVal = startValPersist;
        startValPersist--;
        if (startValPersist == 0) startValPersist = 255;
        for (uint32_t si = 0; si < mParams.numSamples; si++)
        {
            uint8_t val;
            if ( (si >> 1) > startVal )
                val = 255 + startVal - (si >> 1);
            else
                val = startVal - (si >> 1);
            *oPtr++ = val;
        }
#endif
    }
    
    return THOR_OK;
}

void MFilterProcess::terminate() {}
