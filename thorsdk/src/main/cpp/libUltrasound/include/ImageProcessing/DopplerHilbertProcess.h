//
// Copyright 2020 EchoNous Inc.
//
//
#pragma once

#include <ImageProcess.h>

#define HILBERT_BUFFER_SIZE (MAX_CW_SAMPLES_PER_FRAME + MAX_PW_HILBERT_FILTER_TAP)

class DopplerHilbertProcess : public ImageProcess
{
public:
    DopplerHilbertProcess(const std::shared_ptr<UpsReader> &upsReader);

    static const char *name() { return "DopplerHilbertProcess"; }

    ThorStatus init() override;
    ThorStatus setProcessParams(ProcessParams &params) override;
    ThorStatus process(uint8_t *inputPtr, uint8_t *outputPtr) override;
    void terminate() override;

    void       setProcessIndices(uint32_t prfIndex, uint32_t hilbertFilterIndex, bool isTDI);
    void       setCwProcessIndices(uint32_t prfIndex, uint32_t hilbertFilterIndex);

private:
    ProcessParams mParams;
    uint32_t      mHilbertFilterTap;
    uint32_t      mNumDopplerSamplesPerFrame;
    float         mHilbertFilterCoeffs[MAX_PW_HILBERT_FILTER_TAP];

    float         mHilbertBuffer[HILBERT_BUFFER_SIZE * 2];    // complex

    uint32_t      mHilbertBufferInputIndex;                        // input data index
    uint32_t      mHilbertBufferProcessingIndex;                   // output processing index
};
