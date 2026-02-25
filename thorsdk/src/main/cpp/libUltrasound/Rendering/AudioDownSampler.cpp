//
// Copyright 2019 EchoNous Inc.
//
//----------------------------------
// DownSampler for DA audio encoding
//

#define LOG_TAG "AudioDownSampler"

#include <string.h>
#include "AudioDownSampler.h"
#include <ThorUtils.h>

//-----------------------------------------------------------------------------
AudioDownSampler::AudioDownSampler(float inSampleRate, float outSampleRate)
{
    mReadIndex = 0;
    mWriteIndex = 0;
    mInterIndex = 0.0f;
    mSamplingStep = inSampleRate/outSampleRate;
    lastSampleValue = 0.0f;

    ALOGD("Down Sampling Step: %f", mSamplingStep);

    if (mSamplingStep < 0.0f)
    {
        ALOGE("outSampleRate should be smaller than input");
    }
}

//-----------------------------------------------------------------------------
AudioDownSampler::~AudioDownSampler()
{
    ALOGD("AudioDownSampler - %s",__func__);
}

bool AudioDownSampler::canPutData(int numSamples)
{
    if (mNumSamplesStored + numSamples > ADSBufferSize)
    {
        // TODO: buffer overflow
        ALOGD("cannot put data, buffer overflow");
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
int AudioDownSampler::putDataFloat(float *inData, int numSamples)
{
    int     storedSamples = 0;
    int     intIndex;
    float   interCoeff;
    float   interVal;


    if ((mNumSamplesStored + numSamples/mSamplingStep) > ADSBufferSize)
    {
        // TODO: handling buffer overflow
        ALOGD("cannot put data, buffer overflow - numFramesStored: %d, numFramesToAdd: %d",
                mNumSamplesStored, numSamples);
        return -1;
    }

    if (mInterIndex < 0.0f)
    {
        // when interpolation with last sample is needed.
        interCoeff = 1.0f + mInterIndex;
        interVal = interCoeff * inData[0] + (1.0f - interCoeff) * lastSampleValue;

        // update InterIndex
        mInterIndex += mSamplingStep;

        // putData
        mADSBuffer[mWriteIndex] = (short) (32768.0f * interVal);

        // update index
        mWriteIndex++;

        if (mWriteIndex >= ADSBufferSize)
        {
            mWriteIndex -= ADSBufferSize;
        }

        storedSamples++;
    }

    while (mInterIndex < ((float) (numSamples - 1)))
    {
        intIndex = floor(mInterIndex);
        interCoeff = mInterIndex - intIndex;

        interVal = interCoeff * inData[intIndex+1] + (1.0f - interCoeff) * inData[intIndex];

        // update InterIndex
        mInterIndex += mSamplingStep;

        // putData
        mADSBuffer[mWriteIndex] = (short) (32768.0f * interVal);

        // update index
        mWriteIndex++;

        if (mWriteIndex >= ADSBufferSize)
        {
            mWriteIndex -= ADSBufferSize;
        }

        storedSamples++;
    }

    // store last sample values
    lastSampleValue = inData[numSamples - 1];

    // update interpolation index
    mInterIndex -= (float) numSamples;

    // update number of samples stored
    mNumSamplesStored += storedSamples;

    return mNumSamplesStored;
}

//-----------------------------------------------------------------------------
bool AudioDownSampler::getData(short *outData, int numRequestedSamples)
{
    int numSamplesRead = numRequestedSamples;
    int numZeroFills = 0;

    if (mNumSamplesStored < numRequestedSamples)
    {
        // buffer underrun - running out of sample.
        numSamplesRead = mNumSamplesStored;
        numZeroFills = numRequestedSamples - mNumSamplesStored;
        ALOGD("Running out of samples - zeroFill sample size: %d", numZeroFills);
    }

    // TODO: improve efficiency
    for (int i = 0; i < numSamplesRead; i++)
    {
        outData[i] = mADSBuffer[mReadIndex];

        mReadIndex++;

        if (mReadIndex >= ADSBufferSize) {
            mReadIndex -= ADSBufferSize;
        }
    }

    for (int i = 0; i < numZeroFills; i++)
    {
        outData[numSamplesRead + i] = 0;
    }

    mNumSamplesStored -= numSamplesRead;

    return true;
}

//-----------------------------------------------------------------------------
int AudioDownSampler::getStoredNumFrames()
{
    return mNumSamplesStored;
}
