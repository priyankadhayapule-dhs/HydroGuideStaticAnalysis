//
// Copyright 2020 EchoNous Inc.
//
//
#pragma once

#include <ImageProcess.h>

//#define DOPPLER_AUDIO_OUTPUT_TEST_PATTERN

#ifdef DOPPLER_AUDIO_OUTPUT_TEST_PATTERN
#include <SineGenerator.h>
#endif

#define AUDIO_BUFFER_SIZE (MAX_CW_SAMPLES_PER_FRAME + MAX_PW_AUDIO_LP_FILTER_TAP)

class DopplerAudioUpsampleProcess : public ImageProcess
{
public:
    DopplerAudioUpsampleProcess(const std::shared_ptr<UpsReader> &upsReader);

    static const char *name() { return "DopplerAudioUpsampleProcess"; }

    ThorStatus init() override;
    ThorStatus setProcessParams(ProcessParams &params) override;
    ThorStatus process(uint8_t *inputPtr, uint8_t *outputPtr) override;
    void terminate() override;

    void       setProcessingIndices(uint32_t prfIndex, uint32_t filterIndex, bool isTDI,
                                    uint32_t upsampleRatio = 0, bool useUPSFilter = false);
    void       setCwProcessingIndices(uint32_t prfIndex, uint32_t filterIndex,
                                    uint32_t upsampleRatio = 0, bool useUPSFilter = false);
    void       setAudioGain(float dBGain);

private:
    void       setAudioFilterCoeffs(uint32_t filterIndex, bool useUPSFilter);

private:
    ProcessParams mParams;
    uint32_t      mAudioFilterTap;
    uint32_t      mNumDopplerSamplesPerFrame;
    uint32_t      mUpsampleRatio;
    float         mAudioFilterCutoff;                   // normalized freq

    float         mAudioGainForward;
    float         mAudioGainReverse;

    float         mAudioFilterCoeffs[MAX_PW_AUDIO_LP_FILTER_TAP];

    float         mAudioBufferFwd[AUDIO_BUFFER_SIZE];     // Forward
    float         mAudioBufferRev[AUDIO_BUFFER_SIZE];     // Reverse

    uint32_t      mAudioBufferInputIndex;                        // input data index
    uint32_t      mAudioBufferProcessingIndex;                   // output processing index

    // Fade-In related parameters
    uint32_t      mFadeInStartFrameNum;
    uint32_t      mFadeInEndFrameNum;
    uint32_t      mFadeInFrameNum;
    float         mFadeInGain;
    float         mFadeInGainInc;

#ifdef DOPPLER_AUDIO_OUTPUT_TEST_PATTERN
    SineGenerator*  mSineOscL;
    SineGenerator*  mSineOscR;
#endif

};
