//
// Copyright 2020 EchoNous Inc.
//
//
#pragma once

#include <ImageProcess.h>

#define WF_BUFFER_SIZE (MAX_CW_SAMPLES_PER_FRAME + MAX_DOP_WALL_FILTER_TAP)

class DopplerWallFilterProcess : public ImageProcess
{
public:
    DopplerWallFilterProcess(const std::shared_ptr<UpsReader> &upsReader);

    static const char *name() { return "DopplerWallFilterProcess"; }

    ThorStatus init() override;
    ThorStatus setProcessParams(ProcessParams &params) override;
    ThorStatus process(uint8_t *inputPtr, uint8_t *outputPtr) override;
    void terminate() override;

    void       setProcessIndices(uint32_t wallFilterIndex, uint32_t prfIndex, bool isTDI);
    void       setCwProcessIndices(uint32_t wallFilterIndex, uint32_t prfIndex);
    void       setWFGain(float dBGain);

private:
    ProcessParams mParams;
    uint32_t      mWallFilterTap;
    uint32_t      mNumDopplerSamplesPerFrame;
    float         mWallFilterCoeffs[MAX_DOP_WALL_FILTER_TAP];
    float         mWFBuffer[WF_BUFFER_SIZE * 2];    // complex
    float         mWFGain;

    uint32_t      mWFBufferInputIndex;                        // input data index
    uint32_t      mWFBufferProcessingIndex;                   // output processing index
};
