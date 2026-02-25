//
// Copyright 2018 EchoNous Inc.
//
//----------------------------------
// Assuming floating point output
//      Sampling rate >= 8 kHz
//      2 Channels
//      DataType: float
//

#define LOG_TAG "AudioBuffer"

#include <string.h>
#include "AudioBuffer.h"
#include <ThorUtils.h>

// parameters for jitter adjustment
#define STATS_UPDATE_INTERVAL 255

#define CRITFEED_THR 1.0f
#define LOWFEED_THR 1.25f
#define UNDERFEED_THR 1.5f
#define OVERFEED_THR 5.0f
#define CRITOVERFEED_THR 6.0f

// additional debugging log flag
//#define AUDIO_BUFFER_DEBUG

//-----------------------------------------------------------------------------
AudioBuffer::AudioBuffer(int sampleRate, int samplesPerFrame):
        mNumUnderFeed(0),
        mNumOverFeed(0),
        mReadIndex(0),
        mWriteIndex(0),
        mNumFramesStored(0),
        mNumStored(0),
        mAvgIdxDiff(0.0f)
{
    mSampleRate = sampleRate;

    if (mSampleRate < 1)
    {
        ALOGW("AudioBuffer - SampleRate < 1!!!");
        mSampleRate = 8000;
    }

    mCritfeedThreshold = ceil(samplesPerFrame * CRITFEED_THR);
    mLowfeedThreshold = ceil(samplesPerFrame * LOWFEED_THR);
    mUnderfeedThreshold = ceil(samplesPerFrame * UNDERFEED_THR);
    mOverfeedThreshold = ceil(samplesPerFrame * OVERFEED_THR);
    mCriticalOverfeedThreshold = ceil(samplesPerFrame * CRITOVERFEED_THR);

#ifdef AUDIO_BUFFER_DEBUG
    ALOGD("AudioBuffer - sampleRate: %d", sampleRate);
    ALOGD("AudioBuffer - samplesPerFrame: %d", samplesPerFrame);

    ALOGD("AudioBuffer - Threshold - critialLow: %d", mCritfeedThreshold);
    ALOGD("AudioBuffer - Threshold -        Low: %d", mLowfeedThreshold);
    ALOGD("AudioBuffer - Threshold -  underfeed: %d", mUnderfeedThreshold);
    ALOGD("AudioBuffer - Threshold -   overfeed: %d", mOverfeedThreshold);
    ALOGD("AudioBuffer - Threshold -  critialHi: %d", mCriticalOverfeedThreshold);
#endif

}

//-----------------------------------------------------------------------------
AudioBuffer::~AudioBuffer()
{
    ALOGD("AudioBuffer - %s",__func__);
}

//-----------------------------------------------------------------------------
bool AudioBuffer::canPutData(int numFrames)
{
    if (mNumFramesStored + numFrames > AudioBufferSize)
    {
        // TODO: buffer overflow
        ALOGD("cannot put data, buffer overflow");
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
int AudioBuffer::putDataFloat(float *LChannel, float *RChannel, int numFrames, int inIdxStep)
{
    mLock.enter();

    if (mNumFramesStored + numFrames > AudioBufferSize)
    {
        // TODO: handling buffer overflow
        ALOGD("cannot put data, buffer overflow - numFramesStored: %d, numFramesToAdd: %d", mNumFramesStored, numFrames);
        mLock.leave();
        return -1;
    }

    // checking read and write index difference
    int idxDiff = mWriteIndex - mReadIndex;
    if (idxDiff < 0)
        idxDiff += AudioBufferSize;

    int numSampleInsert = 0;
    int insertInterval = 0;
    int insertLoc = 0;
    int storedFrames = 0;

    // idxDiff is based on the idices right before this current set of data is stored.
    if (idxDiff < mUnderfeedThreshold)
    {
        mNumUnderFeed++;
        numSampleInsert += 1;

        if (idxDiff < mLowfeedThreshold)
        {
            numSampleInsert += 2;
        }

        if (idxDiff < mCritfeedThreshold)
        {
            numSampleInsert += 12;
        }
    }
    else if (idxDiff > mOverfeedThreshold)
    {
        // overflow
        mNumOverFeed++;
        numSampleInsert -= 1;

        if (idxDiff > mCriticalOverfeedThreshold)
        {
            numSampleInsert -= 8;
        }
    }   

    if (numSampleInsert > 0)
    {
        insertInterval = (numFrames + numSampleInsert - 1)/(numSampleInsert + 1);
        insertLoc = insertInterval;
    }
    else if (numSampleInsert < 0)
    {
        insertInterval = (numFrames)/(abs(numSampleInsert) + 1);
        insertLoc = insertInterval;
    }

    for (int i = 0; i < numFrames; i++) {
        mAudioBuffer[2*mWriteIndex] = LChannel[ i*inIdxStep ];
        mAudioBuffer[2*mWriteIndex+1] = RChannel[ i*inIdxStep ];

        // find whether we need jitter adjustment
        if (i == insertLoc)
        {
            if (numSampleInsert > 0)
            {
                mAudioBuffer[2*mWriteIndex] += LChannel[ (i-1)*inIdxStep ];
                mAudioBuffer[2*mWriteIndex+1] += RChannel[ (i-1)*inIdxStep ];

                mAudioBuffer[2*mWriteIndex] *= 0.5f;
                mAudioBuffer[2*mWriteIndex+1] *= 0.5f;

                // index update
                i--;
                insertLoc += insertInterval;
                numSampleInsert--;
            }
            else if (numSampleInsert < 0)
            {
                i++;
                insertLoc += insertInterval;
                numSampleInsert++;
            }
        }

        // update index
        mWriteIndex++;

        if (mWriteIndex >= AudioBufferSize)
            mWriteIndex -= AudioBufferSize;

        storedFrames++;
    }

    mNumFramesStored += storedFrames;


    // average idx diff
    mAvgIdxDiff += ((float) idxDiff);

    // index for jitter adjustment interval
    mNumStored++;

    if ((mNumStored > STATS_UPDATE_INTERVAL))
    {
        mAvgIdxDiff = mAvgIdxDiff / ((float)(STATS_UPDATE_INTERVAL + 1));

        if (mNumOverFeed > (STATS_UPDATE_INTERVAL - 10))
        {
            ALOGW("Too many overfeed occurrence: please verify the sampling rate");
        }

        if (mNumUnderFeed > (STATS_UPDATE_INTERVAL - 10))
        {
            ALOGW("Too many underfeed occurrence: please verify the sampling rate");
        }

#ifdef AUDIO_BUFFER_DEBUG
        ALOGD("OverFeed: %d, UnderFeed: %d", mNumOverFeed, mNumUnderFeed);
        ALOGD("Index Diff: %d, current AVG: %f", idxDiff, mAvgIdxDiff);
        ALOGD("Stored Frames: %d", mNumFramesStored);
#endif
        mNumOverFeed = 0;
        mNumUnderFeed = 0;
        mNumStored = 0;

        mAvgIdxDiff = 0.0f;
    }

    mLock.leave();

    return mNumFramesStored;
}

//-----------------------------------------------------------------------------
bool AudioBuffer::getData(float *outData, int numFrames)
{
    mLock.enter();

    bool fadeOut = false;
    int orgNumFrames = numFrames;

    if (mNumFramesStored < 1) 
    {
        mLock.leave();
        return false;
    }
    else if (mNumFramesStored < numFrames)
    {
        // buffer underrun - running out of sample.
        numFrames = mNumFramesStored;
        fadeOut = true;
    }

    // TODO: improve efficiency
    for (int i = 0; i < numFrames; i++)
    {
        outData[2*i] = mAudioBuffer[2*mReadIndex];
        outData[2*i+1] = mAudioBuffer[2*mReadIndex + 1];

        mReadIndex++;

        if (mReadIndex >= AudioBufferSize) {
            mReadIndex -= AudioBufferSize;
        }
    }

    if (fadeOut)
    {
        // fill in the samples
        float lastSample = outData[2*(mNumFramesStored - 1) + 1];
        memset(outData + (2*mNumFramesStored), lastSample, sizeof(float) * 2 * (numFrames - mNumFramesStored));

        // apply gradual attenuation to the signal
        float baseCoeff = 1.0f/(orgNumFrames);
        float adjCoeff;

        for (int i = 0; i < orgNumFrames; i++)
        {
            adjCoeff = (orgNumFrames - 1 - i) * baseCoeff;

            outData[2*i] = outData[2*i] * adjCoeff;
            outData[2*i+1] = outData[2*i+1] * adjCoeff;
        }
    }

    mNumFramesStored -= numFrames;
    mLock.leave();

    return true;
}

//-----------------------------------------------------------------------------
int AudioBuffer::getStoredNumFrames()
{
    return mNumFramesStored;
}

//-----------------------------------------------------------------------------
void AudioBuffer::resetIndices()
{
    mLock.enter();

    // initialize indices.
    mReadIndex = 0;
    mWriteIndex = 0;
    mNumFramesStored = 0;

    mNumOverFeed = 0;
    mNumUnderFeed = 0;
    mNumStored = 0;
    mAvgIdxDiff = 0.0f;

    mLock.leave();
}