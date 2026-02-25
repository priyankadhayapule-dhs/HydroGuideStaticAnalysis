//
// Copyright 2020 EchoNous Inc.
//
//----------------------------------
// This function resamples audio data
// It is specifically designed for audio data encoding.
//
//  Input can be mono or stereo in float
//  Output is stereo in 16-bit short
//
#ifndef AUDIO_CUBIC_RESAMPLER_H
#define AUDIO_CUBIC_RESAMPLER_H

#define ARSBufferSize 16384               // buffer size
#define InputBufSize  4096
#define IntShiftSize  3                   // interpolation related shift size

#include <math.h>

class AudioCubicReSampler
{
public:
    AudioCubicReSampler(float inputSampling, float outSampling, int numChannel = 1);
    ~AudioCubicReSampler();

    int putDataFloat(float* inData, int numSample);

    // default float data
    bool getData(short* outData, int numRequestedSamples);

    // check whether the buffer has enough space to accomodate numFrames specified?
    bool canPutData(int numSamples);

    // get a number of frames stored in the buffer
    int getStoredNumFrames();

private:
    // cubic interpolation
    float cubicInterpolate(float y0, float y1, float y2, float y3, float mu);


private:
    float  mInputBufL[InputBufSize];
    float  mInputBufR[InputBufSize];

    short  mARSBufferL[ARSBufferSize];
    short  mARSBufferR[ARSBufferSize];

    int    mNumChannel;
    int    mReadIndex;
    int    mWriteIndex;
    float  mInterIndex;
    int    mNumSamplesStored;

    float  mSamplingStep;
};

#endif /* AUDIO_CUBIC_RESAMPLER_H */
