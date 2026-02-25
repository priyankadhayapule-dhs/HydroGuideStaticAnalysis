//
// Copyright 2018 EchoNous Inc.
//
//
#pragma once

#include "ImageProcess.h"

class MFilterProcess : public ImageProcess
{
public:
    MFilterProcess(const std::shared_ptr<UpsReader> &upsReader);

    static const char *name() { return "MFilterProcess"; }

    ThorStatus init() override;
    ThorStatus setProcessParams(ProcessParams &params) override;
    void       setScrollSpeed(uint32_t imagingCaseId, uint32_t scrollSpeedIndex);
    ThorStatus process(uint8_t *inputPtr, uint8_t *outputPtr) override;
    void terminate() override;

private:
    ProcessParams mParams;
    uint32_t      mLinesToAverage;
    uint32_t      mOutputLinesPerFrame;
    uint32_t      mNumInputLinesPerFrame;
    uint32_t      mAvgBuf[512];
};
