//
// Copyright 2020 EchoNous Inc.
//
// 1D circular Buffer for Doppler processing
//
//

#define LOG_TAG "DopplerResamplingBuffer"

#include <string.h>
#include <DopplerResamplingBuffer.h>
#include <ThorUtils.h>

#define SAMPLE_TO_ROUND     // rounding to the nearest sample (if not defined then floor)

//-----------------------------------------------------------------------------
DopplerResamplingBuffer::DopplerResamplingBuffer():
        mReadIndex(0),
        mWriteIndex(0),
        mInNumSamplesPerFrame(48),
        mInAfeClksPerSample(24576),
        mNumSamplesFft(10),
        mFftOutRateAfeClks(26041),
        mNumAfeClksStored(0),
        mNumAfeClksRem(0)
{
    mMaxBufferCapacityAfeClks = RESAMPLE_BUFFER_SIZE * mInAfeClksPerSample;
}

//-----------------------------------------------------------------------------
DopplerResamplingBuffer::~DopplerResamplingBuffer()
{
    ALOGD("DopplerResamplingBuffer - %s",__func__);
}

//-----------------------------------------------------------------------------
bool DopplerResamplingBuffer::setBufferParams(int inNumSamplesPerFrame, int inAfeClksPerSample,
        int numSamplesFft, int fftOutRateAfeClks)
{
    mInNumSamplesPerFrame = inNumSamplesPerFrame;
    mNumSamplesFft = numSamplesFft;
    mInAfeClksPerSample = inAfeClksPerSample;
    mFftOutRateAfeClks = fftOutRateAfeClks;

    mMaxBufferCapacityAfeClks = RESAMPLE_BUFFER_SIZE * mInAfeClksPerSample;

    // put 0s so that the first sample can be the last in the FFT window.
    for (int i = 0; i < numSamplesFft - 1; i++)
    {
        mResampleBuffer[2*i] = 0.0f;
        mResampleBuffer[2*i+1] = 0.0f;
    }

    // update read and write indices
    mWriteIndex = numSamplesFft - 1;
    mReadIndex = 0;

    // initially stored num of AfeClks
    mNumAfeClksStored = mWriteIndex * mInAfeClksPerSample;

    ALOGD("DopplerResamplingBuffer: NumSamplesPerFrame: %d, NumSamplesFft: %d, InAfeClkPerSample: %d, FftOutRateAfeClk: %d",
          mInNumSamplesPerFrame, mNumSamplesFft, mInAfeClksPerSample, mFftOutRateAfeClks);

    return true;
}

//-----------------------------------------------------------------------------
bool DopplerResamplingBuffer::canPutData(int numSamples)
{
    if (mNumAfeClksStored + numSamples * mInAfeClksPerSample > mMaxBufferCapacityAfeClks)
    {
        // TODO: buffer overflow
        ALOGD("cannot put data, buffer overflow");
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
int DopplerResamplingBuffer::putData(float* inData)
{
    mLock.enter();

    // number of samples input = mInNumSamplesPerFrame
    if (mNumAfeClksStored + mInNumSamplesPerFrame * mInAfeClksPerSample > mMaxBufferCapacityAfeClks)
    {
        // TODO: handling buffer overflow
        ALOGD("cannot put data, buffer overflow - NumAfeClksStored: %d, NumAfeClksToAdd: %d",
              mNumAfeClksStored, mInNumSamplesPerFrame * mInAfeClksPerSample);
        mLock.leave();
        return -1;
    }

    for (int i = 0; i < mInNumSamplesPerFrame; i++)
    {
        mResampleBuffer[2*mWriteIndex] = *inData++;
        mResampleBuffer[2*mWriteIndex+1] = *inData++;

        // update index
        mWriteIndex = (mWriteIndex + 1) % RESAMPLE_BUFFER_SIZE;
    }

    mNumAfeClksStored += (mInNumSamplesPerFrame * mInAfeClksPerSample);

    mLock.leave();

    return mNumAfeClksStored;
}

//-----------------------------------------------------------------------------
bool DopplerResamplingBuffer::getData(float *outData) {
    mLock.enter();

    // number of samples output = mNumSamplesFft
    // need to check whether the buffer stored enough data
    if (mNumSamplesFft * mInAfeClksPerSample > mNumAfeClksStored) {
        // This could happen when the last set of samples were read from the buffer.
        mLock.leave();
        return false;
    }

    // read from mReadIndex
    uint32_t cur_readIndex = mReadIndex;

    for (int i = 0; i < mNumSamplesFft; i++)
    {
        *outData++ = mResampleBuffer[2*cur_readIndex];
        *outData++ = mResampleBuffer[2*cur_readIndex+1];

        // update index
        cur_readIndex = (cur_readIndex + 1) % RESAMPLE_BUFFER_SIZE;
    }

    // update AfeClksCounter and make adjustment for the remaining counter
    int actualSampleCnt;
    int adjustAfeClks = mNumAfeClksRem + mFftOutRateAfeClks;    // previous remaining clks + outrate

    actualSampleCnt = adjustAfeClks/mInAfeClksPerSample;
    mNumAfeClksRem = adjustAfeClks - actualSampleCnt * mInAfeClksPerSample;

#ifdef SAMPLE_TO_ROUND
    // round to the nearest valid sample
    if (mNumAfeClksRem >= (mInAfeClksPerSample >> 1))
    {
        mNumAfeClksRem -= mInAfeClksPerSample;
        actualSampleCnt++;
    }
#endif

    // update number of AfeClks Stored
    mNumAfeClksStored -= (actualSampleCnt * mInAfeClksPerSample);

    // update readIndex
    mReadIndex = (mReadIndex + actualSampleCnt) % RESAMPLE_BUFFER_SIZE;

    mLock.leave();

    return true;
}

//-----------------------------------------------------------------------------
int DopplerResamplingBuffer::getNumAfeClksStored()
{
    return mNumAfeClksStored;
}

//-----------------------------------------------------------------------------
void DopplerResamplingBuffer::resetBuffer()
{
    mLock.enter();

    // initialize indices.
    mReadIndex = 0;
    mWriteIndex = 0;

    mInNumSamplesPerFrame = 48;
    mNumSamplesFft = 10;
    mInAfeClksPerSample = 24576;
    mFftOutRateAfeClks = 26041;

    mMaxBufferCapacityAfeClks = RESAMPLE_BUFFER_SIZE * mInAfeClksPerSample;
    mNumAfeClksRem = 0;

    // reset Buffers
    for (int i = 0; i < RESAMPLE_BUFFER_SIZE*2; i++)
    {
        mResampleBuffer[i] = 0.0f;
    }

    mLock.leave();
}