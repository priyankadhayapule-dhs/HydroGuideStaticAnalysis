//
// Copyright 2020 EchoNous Inc.
//
//----------------------------------
// ReSampling Buffer
// Assuming I&Q float 32-bit (or complex struct)
// 1D circular buffer
// in, out and step sample sizes are not changing.
//
#ifndef DOPPLER_RESAMPLING_BUFFER_H
#define DOPPLER_RESAMPLING_BUFFER_H

#include <math.h>
#include <cstdint>
#include <CriticalSection.h>
#include <ThorConstants.h>
#include <stdio.h>

// TODO: update when CW Numbers are determined.
#define RESAMPLE_BUFFER_SIZE (MAX_CW_SAMPLES_PER_FRAME + MAX_FFT_SIZE)

class DopplerResamplingBuffer
{
public:
    DopplerResamplingBuffer();
    ~DopplerResamplingBuffer();

    // set in, out and step size
    // inNumSamplesPerFrame = SamplesPerFrame (output of WallFilter)
    // inAfeClksPerSample: input AfeClksPerSample
    // numSamplesFft = sample set used for FFT = FFT_Window_Size (<= FFT_Size)
    // fftOutRateAfeClks: fft output rate in terms of AfeClks
    bool setBufferParams(int inNumSamplesPerFrame, int inAfeClksPerSample,
            int numSamplesFft, int fftOutRateAfeClks);

    // numSamples (I&Q)
    int putData(float* inData);

    // getData (I&Q)
    bool getData(float* outData);

    // check whether the buffer has enough space to accommodate numSamples specified
    bool canPutData(int numSamples);

    // get a number of AfeClks stored in the buffer
    int getNumAfeClksStored();

    // reset buffer and indices
    void resetBuffer();

private:
    // critical section
    CriticalSection     mLock;

    float   mResampleBuffer[RESAMPLE_BUFFER_SIZE*2];     // I&Q data

    uint32_t     mReadIndex;
    uint32_t     mWriteIndex;

    int     mInNumSamplesPerFrame;
    int     mInAfeClksPerSample;
    int     mNumSamplesFft;               // FFT window size
    int     mFftOutRateAfeClks;

    int     mNumAfeClksStored;
    int     mNumAfeClksRem;               // remaining AfeClks (can be negative for round up)

    int     mMaxBufferCapacityAfeClks;    // max buffer capacity in terms of AfeClks
};

#endif /* DOPPLER_RESAMPLING_BUFFER_H */
