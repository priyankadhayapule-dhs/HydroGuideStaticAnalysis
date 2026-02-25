//
// Copyright 2020 EchoNous Inc.
//
//
#pragma once

#include <ImageProcess.h>

//#define USE_PWLPDecimFilter_TEST_PATTERN

#ifdef USE_PWLPDecimFilter_TEST_PATTERN
#include <SineGenerator.h>
#endif

class PwLowPassDecimationFilterProcess : public ImageProcess
{
public:
    PwLowPassDecimationFilterProcess(const std::shared_ptr<UpsReader> &upsReader);

    static const char *name() { return "PwLowPassDecimationFilterProcess"; }

    ThorStatus init() override;
    ThorStatus setProcessParams(ProcessParams &params) override;
    ThorStatus process(uint8_t *inputPtr, uint8_t *outputPtr) override;
    void terminate() override;

    void       setProcessIndices(uint32_t gateIndex, uint32_t prfIndex, bool isTDI);

private:
    ProcessParams mParams;
    uint32_t      mGateSamples;
    uint32_t      mNumPwSamplesPerFrame;

#ifdef USE_PWLPDecimFilter_TEST_PATTERN
    SineGenerator*  mSineOscI;
    SineGenerator*  mSineOscQ;

    float           mDeltaPhase;
    uint32_t        mNumFramesProcessed;
    uint32_t        mPhaseUpdateInterval;
#endif

    // summation buffer
    float         mISum[MAX_PW_INPUT_LINES_PER_FRAME];
    float         mQSum[MAX_PW_INPUT_LINES_PER_FRAME];
};
