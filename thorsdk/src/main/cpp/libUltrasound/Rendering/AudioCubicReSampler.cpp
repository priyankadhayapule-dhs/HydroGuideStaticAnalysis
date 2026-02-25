//
// Copyright 2019 EchoNous Inc.
//
//----------------------------------
// DownSampler for DA/PW/CW audio encoding
//
// input can be mono or stereo
// output: stereo
//

#define LOG_TAG "AudioCubicReSampler"
#define HARD_LIMIT 1.0f
#define EXPORT_SCALE 0.93f

#include <string.h>
#include <AudioCubicReSampler.h>
#include <ThorUtils.h>

//-----------------------------------------------------------------------------
AudioCubicReSampler::AudioCubicReSampler(float inSampleRate, float outSampleRate, int numChannel)
{
    mReadIndex = 0;
    mWriteIndex = 0;
    mSamplingStep = inSampleRate/outSampleRate;

    mNumChannel = numChannel;

    if (mNumChannel < 1)
        mNumChannel = 1;

    if (mNumChannel > 2)
        mNumChannel = 2;

    ALOGD("ReSampling Step: %f", mSamplingStep);

    if (mSamplingStep < 0.5f || mSamplingStep > 2.0f)
    {
        ALOGD("RsSampling Step is out of the desired range!!!");

        if (mSamplingStep < 0.5f)
            mSamplingStep = 0.5f;

        if (mSamplingStep > 2.0f)
            mSamplingStep = 2.0f;
    }

    // initialize three samples in InputBuffer
    for (int i = 0; i < IntShiftSize; i++)
    {
        mInputBufL[i] = 0.0f;
        mInputBufR[i] = 0.0f;
    }

    // set inter index
    mInterIndex = (float) IntShiftSize;

    // reset number of samples stored
    mNumSamplesStored = 0;
}

//-----------------------------------------------------------------------------
AudioCubicReSampler::~AudioCubicReSampler()
{
    ALOGD("AudioCubicReSampler - %s",__func__);
}

bool AudioCubicReSampler::canPutData(int numSamples)
{
    if (mNumSamplesStored + numSamples > ARSBufferSize)
    {
        // TODO: buffer overflow
        ALOGD("cannot put data, buffer overflow");
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
float AudioCubicReSampler::cubicInterpolate(float y0, float y1, float y2, float y3, float mu)
{
    // Catmull-Rom Spline Cubic
    float x, x2, x3, s, rst;
    float a, b, c, d;

    // adjustable param
    s = 0.5;

    x = mu;
    x2 = x*x;
    x3 = x2*x;

    a =      -s*x3 +       2.0*s*x2 - s*x + 0.0;
    b = (2.0-s)*x3 +     (s-3.0)*x2       + 1.0;
    c = (s-2.0)*x3 + (3.0-2.0*s)*x2 + s*x + 0.0;
    d =       s*x3 -           s*x2       + 0.0;
    rst = a*y0 + b*y1 + c*y2 + d*y3;

    return rst;
}

//-----------------------------------------------------------------------------
int AudioCubicReSampler::putDataFloat(float *inData, int numSamples)
{
    int     storedSamples = 0;
    int     intIndex;
    float   interCoeff;
    float   interValL, interValR;

    if ((mNumSamplesStored + numSamples/mSamplingStep) > ARSBufferSize)
    {
        // TODO: handling buffer overflow
        ALOGD("cannot put data, buffer overflow - numFramesStored: %d, numFramesToAdd: %d",
                mNumSamplesStored, numSamples);
        return -1;
    }

    // move data to internal buffers
    for (int i = 0; i < numSamples; i++)
    {
        mInputBufL[i + IntShiftSize] = *inData++;

        // mono
        if (mNumChannel == 1)
        {
            mInputBufR[i + IntShiftSize] = mInputBufL[i + IntShiftSize];
        }
        else
        {
            // stereo
            mInputBufR[i + IntShiftSize] = *inData++;
        }
    }

    // while
    while (mInterIndex < ((float) (numSamples - 2 + IntShiftSize)))
    {
        intIndex = (int) floor(mInterIndex);
        interCoeff = mInterIndex - ((float) intIndex);

        // interpolated value
        interValL = (EXPORT_SCALE) * cubicInterpolate(mInputBufL[intIndex - 1], mInputBufL[intIndex],
                mInputBufL[intIndex + 1], mInputBufL[intIndex + 2], interCoeff);

        interValR = (EXPORT_SCALE) * cubicInterpolate(mInputBufR[intIndex - 1], mInputBufR[intIndex],
                mInputBufR[intIndex + 1], mInputBufR[intIndex + 2], interCoeff);

        // update InterIndex
        mInterIndex += mSamplingStep;

        // clipping
        if (interValL > HARD_LIMIT)
            interValL = HARD_LIMIT;
        else if (interValL < -HARD_LIMIT)
            interValL = -HARD_LIMIT;

        if (interValR > HARD_LIMIT)
            interValR = HARD_LIMIT;
        else if (interValR < -HARD_LIMIT)
            interValR = -HARD_LIMIT;

        // putData
        mARSBufferL[mWriteIndex] = (short) (32767.0f * interValL);
        mARSBufferR[mWriteIndex] = (short) (32767.0f * interValR);

        // update index
        mWriteIndex = (mWriteIndex+1) % ARSBufferSize;

        storedSamples++;
    }

    // update number of samples stored
    mNumSamplesStored += storedSamples;

    // move the last IntShiftSize data to the first 3 location
    for (int i = 0; i < IntShiftSize; i++)
    {
        mInputBufL[i] = mInputBufL[numSamples + i];
        mInputBufR[i] = mInputBufR[numSamples + i];
    }

    // update interpolation index
    mInterIndex -= (float) numSamples;

    return mNumSamplesStored;
}

//-----------------------------------------------------------------------------
bool AudioCubicReSampler::getData(short *outData, int numRequestedSamples)
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
        outData[2*i] = mARSBufferL[mReadIndex];
        outData[2*i+1] = mARSBufferR[mReadIndex];

        mReadIndex = (mReadIndex+1) % ARSBufferSize;
    }

    for (int i = 0; i < numZeroFills; i++)
    {
        outData[numSamplesRead + 2*i] = 0;
        outData[numSamplesRead + 2*i + 1] = 0;
    }

    mNumSamplesStored -= numSamplesRead;

    return true;
}

//-----------------------------------------------------------------------------
int AudioCubicReSampler::getStoredNumFrames()
{
    return mNumSamplesStored;
}
