//
// Copyright 2020 EchoNous Inc.
//
//
#pragma once

#include <ImageProcess.h>
#include <CineBuffer.h>

class DopplerPeakMaxProcess : public ImageProcess
{
public:
    DopplerPeakMaxProcess(const std::shared_ptr<UpsReader> &upsReader);

    static const char *name() { return "DopplerPeakMaxProcess"; }

    ThorStatus init() override;
    ThorStatus setProcessParams(ProcessParams &params) override;
    ThorStatus process(uint8_t *inputPtr, uint8_t *outputPtr) override;
    void terminate() override;

    void       setProcessIndices(uint32_t numFFT, float refGaindB = 15.0, float threshold = 0.2);
    void       setWFGain(float dBGain);
    void       resetCounter();

    uint32_t   getThreshold() { return {mThreshold}; };

private:
    ProcessParams mParams;

    uint32_t      mNumFFT;
    float         mWFGain;
    float         mWFGainRef;
    float         mThresholdFloat;
    uint32_t      mThreshold;
    int           mPrevDistIdx;
};
