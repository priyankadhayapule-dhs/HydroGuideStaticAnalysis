//
// Copyright 2020 EchoNous Inc.
//
//
#pragma once

#include <ImageProcess.h>
#include <CineBuffer.h>

#define SMOOTH_BUFFER_SIZE (MAX_FFT_SIZE * (MAX_DOPPLER_NUM_LINES_OUT_PER_FRAME + MAX_DOPPLER_SMOOTH_KERNEL_SIZE))

class DopplerSpectralSmoothingProcess : public ImageProcess
{
public:
    DopplerSpectralSmoothingProcess(const std::shared_ptr<UpsReader> &upsReader);

    static const char *name() { return "DopplerSpectralSmoothingProcess"; }

    ThorStatus init() override;
    ThorStatus setProcessParams(ProcessParams &params) override;
    ThorStatus process(uint8_t *inputPtr, uint8_t *outputPtr) override;
    void terminate() override;

    void       setProcessIndices(uint32_t fftSize, uint32_t kernelSize);

private:
    ProcessParams mParams;
    uint32_t      mKernelSize;
    uint32_t      mFFTSize;                           // expected: 256 for PW, 512 for CW
    uint8_t       mImgBuffer[SMOOTH_BUFFER_SIZE];
    float         mLineBuffer[MAX_FFT_SIZE];
    float         mScaleAdj;                          // 1/mKernalSize
};
