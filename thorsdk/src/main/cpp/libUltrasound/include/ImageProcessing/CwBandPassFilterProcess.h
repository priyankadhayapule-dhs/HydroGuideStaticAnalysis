//
// Copyright 2020 EchoNous Inc.
//
//
#pragma once

#include <ImageProcess.h>

//#define USE_CWBandPassFilter_TEST_PATTERN
//#define LOG_MAX_CW_ADC_VALUES

#ifdef USE_CWBandPassFilter_TEST_PATTERN
#include <SineGenerator.h>
#endif

#define BP_BUFFER_SIZE (MAX_CW_SAMPLES_PER_FRAME_PRE_DECIM + MAX_CW_BANDPASS_FILTER_TAP)

class CwBandPassFilterProcess : public ImageProcess
{
public:
    CwBandPassFilterProcess(const std::shared_ptr<UpsReader> &upsReader);

    static const char *name() { return "CwBandPassFilterProcess"; }

    ThorStatus init() override;
    ThorStatus setProcessParams(ProcessParams &params) override;
    ThorStatus process(uint8_t *inputPtr, uint8_t *outputPtr) override;
    void terminate() override;

    void       setProcessIndices(uint32_t gateIndex, uint32_t prfIndex);
    void       resetIndices();

private:
    ProcessParams mParams;
    uint32_t      mBandPassFilterTap;
    uint32_t      mNumCwSamplesPerFrame;
    uint32_t      mDecimationFactor;
    float         mBandPassFilterCoeffs[MAX_CW_BANDPASS_FILTER_TAP];
    float         mBPBuffer[BP_BUFFER_SIZE * 2];      // complex

    uint32_t      mBPBufferInputIndex;                        // input data index
    uint32_t      mBPBufferProcessingIndex;                   // output processing index

    uint32_t      mNumFramesProcessed;

#ifdef LOG_MAX_CW_ADC_VALUES
    uint32_t      mLogFrameCount;
    uint32_t      mMaxValueCount;
    int32_t       mMaxValue;
    uint32_t      mLogFrameRate;
#endif

#ifdef USE_CWBandPassFilter_TEST_PATTERN
    SineGenerator*  mSineOscI;
    SineGenerator*  mSineOscQ;

    float           mDeltaPhase;
    uint32_t        mPhaseUpdateInterval;
#endif
};
