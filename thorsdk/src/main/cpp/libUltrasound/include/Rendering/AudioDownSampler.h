//
// Copyright 2019 EchoNous Inc.
//
//----------------------------------
// This DownSampling Buffer
//      is designed for simple downsampling
//      audio data for encoding.
//
//      support only Mono
//
#ifndef AUDIO_DOWN_SAMPLER_H
#define AUDIO_DOWN_SAMPLER_H

#define ADSBufferSize 4096                   // buffer size

#include <math.h>

class AudioDownSampler
{
public:
    AudioDownSampler(float inputSampling, float outSampling);
    ~AudioDownSampler();

    int putDataFloat(float* inData, int numSample);

    // default float data
    bool getData(short* outData, int numRequestedSamples);

    // check whether the buffer has enough space to accomodate numFrames specified?
    bool canPutData(int numSamples);

    // get a number of frames stored in the buffer
    int getStoredNumFrames();


private:
    short  mADSBuffer[ADSBufferSize];

    int    mReadIndex;
    int    mWriteIndex;
    float  mInterIndex;
    int    mNumSamplesStored;

    float lastSampleValue;

    float  mSamplingStep;
};

#endif /* AUDIO_DOWN_SAMPLER_H */
