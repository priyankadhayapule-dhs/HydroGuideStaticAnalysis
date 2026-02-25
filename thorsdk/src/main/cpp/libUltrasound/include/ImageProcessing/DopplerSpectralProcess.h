//
// Copyright 2020 EchoNous Inc.
//
//
#pragma once

#include <ImageProcess.h>
#include <DopplerResamplingBuffer.h>
#include <CineBuffer.h>
#include <pffft.h>

class DopplerSpectralProcess : public ImageProcess
{
public:
    DopplerSpectralProcess(const std::shared_ptr<UpsReader> &upsReader);

    static const char *name() { return "DopplerSpectralProcess"; }

    ThorStatus init() override;
    ThorStatus setProcessParams(ProcessParams &params) override;
    ThorStatus process(uint8_t *inputPtr, uint8_t *outputPtr) override;
    void terminate() override;

    void       setProcessIndices(uint32_t prfIndex, uint32_t numLinesToAvg,
                                 uint32_t compressionCurveIndex, bool isTDI);
    void       setCwProcessIndices(uint32_t prfIndex, uint32_t numLinesToAvg,
                                 uint32_t compressionCurveIndex);

private:
    ProcessParams mParams;
    uint32_t      mNumSamplesFft;
    uint32_t      mNumDopplerSamplesPerFrame;
    uint32_t      mFFTSize;             // FFT size should be bigger than FFTWindowTap (window size)

    float         mFFTWindowCoeffs[MAX_PW_FFT_WINDOW_SIZE];

    // input and output FFT buffer.  must be aligned 32 for the best performance
    float __attribute__ ((aligned(32)))     mFFTInBuffer[MAX_FFT_SIZE * 2];      // complex
    float __attribute__ ((aligned(32)))     mFFTOutBuffer[MAX_FFT_SIZE * 2];     // complex
    float __attribute__ ((aligned(32)))     mFFTTempBuffer[MAX_FFT_SIZE * 2];    // complex temp for PFFFt

    // FFT line average buffer
    float __attribute__ ((aligned(32)))     mFFTAvgBuffer[MAX_FFT_SIZE];         // complex temp for PFFFt

    // Resampling Buffer
    DopplerResamplingBuffer                      mDopplerResamplingBuffer;

    // PFFFT
    PFFFT_Setup* mPFFFTPtr;

    // compression lookup
    float         mCompressionCoeffs[256];
    uint32_t      mNumCompCoeffs;
    uint32_t      getCompressionIndex(float xList[], uint32_t first, uint32_t last, float item);

    uint32_t      mNumLinesToAvg, mLineCnt;
    float         mLinesAvgScale;

    // initial transition related parameters
    uint32_t      mBlankGainStart;
    uint32_t      mBlankFrames;
    float         mBlankGainInc;
    float         mBlankGain;

};
