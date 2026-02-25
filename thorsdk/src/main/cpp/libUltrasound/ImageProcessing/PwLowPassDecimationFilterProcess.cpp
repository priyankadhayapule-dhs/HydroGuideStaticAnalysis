//
// Copyright 2018 EchoNous Inc.
//
//
#define LOG_TAG "PwLowPassDecimationFilterProcess"

#include <PwLowPassDecimationFilterProcess.h>
#include <ThorUtils.h>

//#define PWLP_OUTPUT_DEBUG

PwLowPassDecimationFilterProcess::PwLowPassDecimationFilterProcess(const std::shared_ptr<UpsReader> &upsReader) :
    ImageProcess(upsReader)
{
    mGateSamples = 10;
    mNumPwSamplesPerFrame = 48;

#ifdef USE_PWLPDecimFilter_TEST_PATTERN
    mSineOscI = new SineGenerator();
    mSineOscQ = new SineGenerator();

    mNumFramesProcessed = 0;
    mPhaseUpdateInterval = 10;
#endif
}

ThorStatus PwLowPassDecimationFilterProcess::init()
{
    return THOR_OK;
}

ThorStatus PwLowPassDecimationFilterProcess::setProcessParams(ProcessParams &params)
{
    mParams = params;
    LOGD("PwLowPassDecimationFilterProcess::setProcessParams(): numRays = %d", mParams.numRays);
    return THOR_OK;
}

void PwLowPassDecimationFilterProcess::setProcessIndices(uint32_t gateIndex, uint32_t prfIndex, bool isTDI)
{
    // TODO:
    mGateSamples = getUpsReader()->getGateSizeSamples(gateIndex);
    mNumPwSamplesPerFrame = getUpsReader()->getNumPwSamplesPerFrame(prfIndex, isTDI);

    LOGD("PwLowPassDecimationFilterProcess:setGateIndex(%d), mGateSamples = %d, mNumPwSamplesPerFrame = %d",
            gateIndex, mGateSamples, mNumPwSamplesPerFrame);

#ifdef USE_PWLPDecimFilter_TEST_PATTERN
    float samplingFrequency = getUpsReader()->getGlobalsPtr()->samplingFrequency;
    int afeClksPerPwSample = getUpsReader()->getAfeClksPerPwSample(prfIndex);

    float signalSamplingRate = samplingFrequency*1e6/afeClksPerPwSample;

    float testFreq = signalSamplingRate * 0.25f;
    float testAmp = 0.5f;

    mSineOscI->setup(testFreq, signalSamplingRate, testAmp);
    mSineOscQ->setup(testFreq, signalSamplingRate, testAmp);

    // set phase
    mSineOscI->setPhase(M_PI/2);
    mSineOscQ->setPhase(0);

    // delta phase
    mDeltaPhase = 0.05f * M_PI;

    LOGD("Using Test Pattern: signalSamplingRate(%f), testSignalFrequency(%f)",
            signalSamplingRate, testFreq);
#endif
}

ThorStatus PwLowPassDecimationFilterProcess::process(uint8_t *inputPtr, uint8_t *outputPtr)
{
    // input pointer - includes header (as this is the first step in the process)
    uint32_t *inputWordPtr = (uint32_t *) inputPtr;

    // outputPtr does not include the frame header
    float *outputFloatPtr = (float *)outputPtr;

    // TODO: update
    float convScale = (1.0f/32768.0f) ;    //U->F conversion scale

    convScale *= 1;

    // initialize buffers (NumPwSamplesPerFrame)
    for (uint32_t si = 0; si < mNumPwSamplesPerFrame; si++)
    {
        mISum[si] = 0.0f;
        mQSum[si] = 0.0f;
    }

    // skip past header (header =  4 words)
    inputWordPtr += 4;

    for (uint32_t si = 0; si < mNumPwSamplesPerFrame; si++)  // sample / frame loop
    {
        uint32_t uval;
        uint32_t uval_I;
        uint32_t uval_Q;
        int      ival_I;
        int      ival_Q;
        float    fval_I;
        float    fval_Q;

        for (uint32_t gs = 0; gs < mGateSamples; gs++)      // gate sample loop
        {
            uval = *inputWordPtr;

            // uval = 32-bit packed, 16-bit I&Q
            // I&Q = 2s complement
            uval_I = (uval & 0x0000ffff);
            uval_Q = (uval & 0xffff0000) >> 16;

            if (uval_I > 32767)
                ival_I = ((int) uval_I) - 65536;
            else
                ival_I = (int) uval_I;

            if (uval_Q > 32767)
                ival_Q = ((int) uval_Q) - 65536;
            else
                ival_Q = (int) uval_Q;

            fval_I = (float) ival_I;
            fval_Q = (float) ival_Q;

            mISum[si] += fval_I;
            mQSum[si] += fval_Q;

            // 4 words per sample
            inputWordPtr += 4;
        }
    }

#ifdef USE_PWLPDecimFilter_TEST_PATTERN
    float test_sigI[mNumPwSamplesPerFrame];
    float test_sigQ[mNumPwSamplesPerFrame];

    mSineOscI->render(test_sigI, 1, mNumPwSamplesPerFrame);
    mSineOscQ->render(test_sigQ, 1, mNumPwSamplesPerFrame);

    if (mNumFramesProcessed%mPhaseUpdateInterval == 0)
    {
        mSineOscI->addPhaseIncrement(mDeltaPhase);
        mSineOscQ->addPhaseIncrement(mDeltaPhase);
    }

    mNumFramesProcessed++;
#endif

    // output to outputPtr
    for (uint32_t si = 0; si < mNumPwSamplesPerFrame; si++)
    {
#ifdef USE_PWLPDecimFilter_TEST_PATTERN
        *outputFloatPtr++ = test_sigI[si];
        *outputFloatPtr++ = test_sigQ[si];
#else
        *outputFloatPtr++ = mISum[si] * convScale;
        *outputFloatPtr++ = mQSum[si] * convScale;
#endif
    }

#ifdef PWLP_OUTPUT_DEBUG
    float maxVal = 0.0;
    float minVal = 0.0;

    for (uint32_t si = 0; si < mNumPwSamplesPerFrame; si++)
    {
        if (maxVal < mISum[si] * convScale)
            maxVal = mISum[si] * convScale;

        if (minVal > mISum[si] * convScale)
            minVal = mISum[si] * convScale;
    }

    LOGD("I output value in a frame: max(%f), min(%f)", maxVal, minVal);
#endif

    return THOR_OK;
}

void PwLowPassDecimationFilterProcess::terminate()
{
#ifdef USE_PWLPDecimFilter_TEST_PATTERN
    // clean up
    delete mSineOscI;
    delete mSineOscQ;
#endif
}
