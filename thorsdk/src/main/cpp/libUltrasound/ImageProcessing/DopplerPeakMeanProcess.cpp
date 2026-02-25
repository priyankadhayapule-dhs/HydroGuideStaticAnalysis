//
// Copyright 2020 EchoNous Inc.
//
//
#define LOG_TAG "DopplerPeakMeanProcess"

#include <DopplerPeakMeanProcess.h>
#include <ThorUtils.h>

DopplerPeakMeanProcess::DopplerPeakMeanProcess(const std::shared_ptr<UpsReader> &upsReader) :
        ImageProcess(upsReader)
{
    mNumFFT = 256;
    mWFGain = 5.62f;
    mWFGainRef = 5.62f;
    mThreshold = 255;
    mThresholdFloat = 0.2f;
}

ThorStatus DopplerPeakMeanProcess::init()
{
    return THOR_OK;
}

ThorStatus DopplerPeakMeanProcess::setProcessParams(ProcessParams &params)
{
    mParams = params;
    LOGD("DopplerPeakMeanProcess::setProcessParams(): numRays = %d", mParams.numRays);
    return THOR_OK;
}

void DopplerPeakMeanProcess::setProcessIndices(uint32_t numFFT, float refGaindB, float thresholdFloat)
{
    // Number of FFT:
    // Expected: PW: 256, CW: 512
    mNumFFT = numFFT;

    // Reference Gain
    mWFGainRef = pow(10.0f, (refGaindB) / 20.0f);

    // threshold 0 ~ 1 (default 0.2)
    mThresholdFloat = thresholdFloat;
    float gainAdj = (mWFGain / mWFGainRef);
    if (gainAdj > 1.0f)
        gainAdj = sqrt(gainAdj);

    mThreshold = (uint32_t) (255.0f * gainAdj * mThresholdFloat);

    mPrevDistIdx = 127;

    LOGD("DopplerPeakMeanProcess::setProcessIndices(): mWFGainRef: %f, numFFT: %d, threshold: %d",
         mWFGainRef, mNumFFT, mThreshold);
}


void DopplerPeakMeanProcess::setWFGain(float dBGain)
{
    // TODO: update WFGain for the normalization process
    mWFGain = pow(10.0f, (dBGain) / 20.0f);
    float gainAdj = (mWFGain / mWFGainRef);
    if (gainAdj > 1.0f)
        gainAdj = sqrt(gainAdj);

    mThreshold = (uint32_t) (255.0f * gainAdj * mThresholdFloat);
    LOGD("DopplerPeakMeanProcess::setWFGain(): mWFGain: %f, threshold: %d", mWFGain, mThreshold);
}


ThorStatus DopplerPeakMeanProcess::process(uint8_t* inputPtr, uint8_t* outputPtr)
{
    // input: 8-bit unsigned integer
    // output float between 0 ~ 1.
    // Assumption: output carries initial peak max results
    // Mean Results -> output overwrite

    // input header pointer
    CineBuffer::CineFrameHeader* inputHeaderPtr =
            reinterpret_cast<CineBuffer::CineFrameHeader*>(inputPtr - sizeof(CineBuffer::CineFrameHeader));

    // output header pointer
    CineBuffer::CineFrameHeader* outputHeaderPtr =
            reinterpret_cast<CineBuffer::CineFrameHeader*>(outputPtr - sizeof(CineBuffer::CineFrameHeader));

    // Number of Lines Per Frame from header
    uint32_t numLinesPerFrame = inputHeaderPtr->numSamples;
    // output header update
    outputHeaderPtr->numSamples = numLinesPerFrame;
    outputHeaderPtr->frameNum = inputHeaderPtr->frameNum;

    float*   outputFloatPtr = (float*) outputPtr;

    int   halfDistance = (mNumFFT >> 1);    // distance to check the threshold
    int   lineIdx, distIdx, idxShift;
    float scaleAdj = 1.0f/((float) halfDistance);

    int   currentPeakMax;
    int   positiveSum, negativeSum;
    int   positiveCnt, negativeCnt;
    float positiveRst, negativeRst;

    for (lineIdx = 0; lineIdx < numLinesPerFrame; lineIdx++)
    {
        idxShift = mNumFFT * lineIdx;

        // reset counter
        positiveSum = 0;
        negativeSum = 0;
        positiveCnt = 0;
        negativeCnt = 0;
        positiveRst = 0.0f;
        negativeRst = 0.0f;

        // calculate positive side
        currentPeakMax = round( (*outputFloatPtr) * halfDistance );

        if (currentPeakMax > halfDistance)
            currentPeakMax = halfDistance;

        distIdx = currentPeakMax;

        while (distIdx >= 0)
        {
            if (inputPtr[idxShift + distIdx] > mThreshold)
            {
                positiveSum += distIdx;
                positiveCnt++;
            }
            distIdx--;
        }


        // store positive-side results
        if (positiveCnt > 0)
            positiveRst = ((float) positiveSum) / ((float) positiveCnt) * scaleAdj;

        *outputFloatPtr++ = positiveRst;

        // calculate negative side - currentPeakMax (negative)
        currentPeakMax = mNumFFT + round( (*outputFloatPtr) * halfDistance ) - 1;

        if (currentPeakMax > mNumFFT - 1)
            currentPeakMax = mNumFFT - 1;
        else if (currentPeakMax <= halfDistance)
            currentPeakMax = halfDistance + 1;

        distIdx = currentPeakMax;

        while (distIdx < mNumFFT)
        {
            if (inputPtr[idxShift + distIdx] > mThreshold)
            {
                negativeSum += -(mNumFFT - distIdx);
                negativeCnt++;
            }
            distIdx++;
        }
        // store negative-side results
        if (negativeCnt > 0)
            negativeRst = ((float) negativeSum) / ((float) negativeCnt) * scaleAdj;

        *outputFloatPtr++ = negativeRst;
    }

    return THOR_OK;
}

void DopplerPeakMeanProcess::terminate() {}
