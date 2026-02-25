//
// Copyright 2018 EchoNous Inc.
//
//----------------------------------
// Assuming floating point output
//      Sampling rate of 48k
//      2 Channels
//      DataType: float
//
//
#ifndef AUDIO_BUFFER_H
#define AUDIO_BUFFER_H

#include <math.h>
#include <cstdint>
#include <CriticalSection.h>

#define AudioBufferSize 12288                     // represents frames

class AudioBuffer
{
public:
    AudioBuffer(int sampleRate, int samplesPerFrame);
    ~AudioBuffer();

    // numFrames => sample per channel
    int putDataFloat(float* LChannel, float* RChannel, int numFrames, int inIdxStep = 1);

    // default float data
    bool getData(float* outData, int numFrames);

    // check whether the buffer has enough space to accomodate numFrames specified?
    bool canPutData(int numFrames);

    // get a number of frames stored in the buffer
    int getStoredNumFrames();

    // reset indices
    void resetIndices();

private:
    // critical section
    CriticalSection     mLock;

    int                 mNumUnderFeed;
    int                 mNumOverFeed;

    float  mAudioBuffer[AudioBufferSize*2];

    int    mReadIndex;
    int    mWriteIndex;
    int    mNumFramesStored;

    int    mNumStored;

    float  mAvgIdxDiff;

    int    mSampleRate = 12716;

    int    mUnderfeedThreshold = 1670;
    int    mLowfeedThreshold = 1002;
    int    mCritfeedThreshold = 668;
    int    mOverfeedThreshold = 2338;
    int    mCriticalOverfeedThreshold = 3006;
};

#endif /* AUDIO_BUFFER_H */
