//
// Copyright 2020 EchoNous Inc.
//
//
#define LOG_TAG "CwBandPassFilterProcess"

#include <CwBandPassFilterProcess.h>
#include <ThorUtils.h>

//#define DEBUG_CW_BANDPASS_FILTER_COEFFS

CwBandPassFilterProcess::CwBandPassFilterProcess(const std::shared_ptr<UpsReader> &upsReader) :
        ImageProcess(upsReader)
{
    mBandPassFilterTap = 127;
    mNumCwSamplesPerFrame = 256;
    mNumFramesProcessed = 0;

#ifdef LOG_MAX_CW_ADC_VALUES
    mLogFrameCount = 0;
    mLogFrameRate  = 4096;
    mMaxValueCount = 0;
#endif

#ifdef USE_CWBandPassFilter_TEST_PATTERN
    mSineOscI = new SineGenerator();
    mSineOscQ = new SineGenerator();

    mPhaseUpdateInterval = 50;
#endif
}

ThorStatus CwBandPassFilterProcess::init()
{
    return THOR_OK;
}

ThorStatus CwBandPassFilterProcess::setProcessParams(ProcessParams &params)
{
    mParams = params;
    LOGD("CwBandPassFilterProcess::setProcessParams(): numRays = %d", mParams.numRays);
    return THOR_OK;
}

void CwBandPassFilterProcess::setProcessIndices(uint32_t filterIndex, uint32_t prfIndex)
{
    // read BandPass Filter Tap and Coefficients
    mBandPassFilterTap = getUpsReader()->getCwDownsampleCoeffs(prfIndex, mBandPassFilterCoeffs);
    // number of CW samples per frame
    mNumCwSamplesPerFrame = getUpsReader()->getNumCwSamplesPerFrame(prfIndex);

    uint32_t decimatedSamplesPerFrame;
    getUpsReader()->getCwDecFactorSamples(prfIndex, mDecimationFactor, decimatedSamplesPerFrame);

    LOGD("CwBandPassFilterProcess:filterIndex(%d), mFilterTap = %d, mNumCwSamplesPerFrame = %d, "
         "mDecimationFactor: %d, decimatedSamplesPerFrame: %d",
         filterIndex, mBandPassFilterTap, mNumCwSamplesPerFrame, mDecimationFactor, decimatedSamplesPerFrame);

#ifdef DEBUG_CW_BANDPASS_FILTER_COEFFS
    for (int i = 0; i < mBandPassFilterTap; i++)
        LOGD("CwBandPassFilterProcess - CW BandPass Filter Coeffs: %d %.10e\n", i, mBandPassFilterCoeffs[i]);
#endif

    // Initialize Wall Filter Buffer and processing indices
    for (int i = 0; i < mBandPassFilterTap - 1; i++)
    {
        mBPBuffer[i*2] = 0.0f;
        mBPBuffer[i*2+1] = 0.0f;
    }

    mBPBufferInputIndex = mBandPassFilterTap - 1;
    mBPBufferProcessingIndex = 0;

#ifdef USE_CWBandPassFilter_TEST_PATTERN
    // CW Sampling Rate is fixed at
    float signalSamplingRate =  195312.5f;

    float testFreq = signalSamplingRate * 0.005f;
    float testAmp = 0.01f;

    mSineOscI->setup(testFreq, signalSamplingRate, testAmp);
    mSineOscQ->setup(testFreq, signalSamplingRate, testAmp);

    // set phase
    mSineOscI->setPhase(M_PI/2);
    mSineOscQ->setPhase(0);

    // delta phase
    mDeltaPhase = 0.005f * M_PI;

    LOGD("CwBandPassFilterProcess::Test Pattern: signalSamplingRate(%f), testSignalFrequency(%f)",
            signalSamplingRate, testFreq);
#endif
}

ThorStatus CwBandPassFilterProcess::process(uint8_t* inputPtr, uint8_t* outputPtr)
{
    // input pointer - includes header (as this is the first step in the process)
    uint32_t *inputWordPtr = (uint32_t *) inputPtr;

    // outputPtr does not include the frame header & float for this step
    float* outputFloatPtr = (float*) outputPtr;

    // Normalization factor
    float convScale = (1.0f/8388608.0f);

#ifdef USE_CWBandPassFilter_TEST_PATTERN
    uint32_t decimSamplesPerFrame = mNumCwSamplesPerFrame;
    uint32_t outIdx = 0;

    float test_sigI[decimSamplesPerFrame];
    float test_sigQ[decimSamplesPerFrame];

    mSineOscI->render(test_sigI, 1, decimSamplesPerFrame);
    mSineOscQ->render(test_sigQ, 1, decimSamplesPerFrame);

    mNumFramesProcessed++;

    if (mNumFramesProcessed%mPhaseUpdateInterval == 0)
    {
        mSineOscI->addPhaseIncrement(mDeltaPhase);
        mSineOscQ->addPhaseIncrement(mDeltaPhase);
    }

    for (int i = 0; i < mNumCwSamplesPerFrame; i++)
    {
        mBPBuffer[2*mBPBufferInputIndex] = test_sigI[i];
        mBPBuffer[2*mBPBufferInputIndex + 1] = test_sigQ[i];
        mBPBufferInputIndex = (mBPBufferInputIndex + 1) % BP_BUFFER_SIZE;
    }
#else
    // skip past header (header = 4 words)
    inputWordPtr += 4;

    uint32_t uval_I;
    uint32_t uval_Q;
    int      ival_I;
    int      ival_Q;
    float    fval_I;
    float    fval_Q;
    bool     negative_I;
    bool     negative_Q;


    // put data into BP buffer (pre-Filter)
    for (int i = 0; i < mNumCwSamplesPerFrame; i++)
    {
        // input 24-bit unsigned int packed into 32-bit
        // I&Q 2s complement
        uval_I = *inputWordPtr++;
        uval_I = (uval_I & 0x00ffffff);

        uval_Q = *inputWordPtr++;
        uval_Q = (uval_Q & 0x00ffffff);

        negative_I = (uval_I & (1 << 23)) != 0;
        if (negative_I)
            ival_I = uval_I | ~((1 << 24) - 1);
        else
            ival_I = uval_I;

        negative_Q = (uval_Q & (1 << 23)) != 0;
        if (negative_Q)
            ival_Q = uval_Q | ~((1 << 24) - 1);
        else
            ival_Q = uval_Q;

        fval_I = (float) ival_I;
        fval_Q = (float) ival_Q;

        mBPBuffer[2*mBPBufferInputIndex] = fval_I;
        mBPBuffer[2*mBPBufferInputIndex + 1] = fval_Q;
        mBPBufferInputIndex = (mBPBufferInputIndex + 1) % BP_BUFFER_SIZE;

#ifdef LOG_MAX_CW_ADC_VALUES
        if (ival_I>mMaxValue){
            mMaxValue = ival_I;
        }
        if (ival_Q>mMaxValue){
            mMaxValue = ival_Q;
        }
        if (mMaxValue>8000000){
            mMaxValueCount += 1;
        }
        mLogFrameCount += 1;
        if (mLogFrameCount == mLogFrameRate) {
            LOGD("CWLOG max value =  %d max value count = %d", mMaxValue, mMaxValueCount);
            mMaxValue = 0;
            mMaxValueCount = 0;
            mLogFrameCount = 0;
        }
#endif
    }

    mNumFramesProcessed++;

#endif

    // BandPass Filter
    for (int i = 0; i < mNumCwSamplesPerFrame; i++)
    {
        if ((i % mDecimationFactor) == 0)
        {
            float fval_I = 0.0f;
            float fval_Q = 0.0f;
            uint32_t current_data_index = mBPBufferProcessingIndex;

            for (int j = 0; j < mBandPassFilterTap; j++) {
                fval_I += mBPBuffer[2 * current_data_index] * mBandPassFilterCoeffs[j];
                fval_Q += mBPBuffer[2 * current_data_index + 1] * mBandPassFilterCoeffs[j];

                current_data_index = (current_data_index + 1) % BP_BUFFER_SIZE;
            }

            // output to the next step
#ifdef USE_CWBandPassFilter_TEST_PATTERN
            *outputFloatPtr++ = fval_I;
            *outputFloatPtr++ = fval_Q;
#else
            *outputFloatPtr++ = fval_I * convScale;
            *outputFloatPtr++ = fval_Q * convScale;
#endif
        }

        mBPBufferProcessingIndex = (mBPBufferProcessingIndex + 1) % BP_BUFFER_SIZE;
    }

    return THOR_OK;
}

void CwBandPassFilterProcess::resetIndices()
{
    // Initialize Wall Filter Buffer and processing indices
    for (int i = 0; i < mBandPassFilterTap - 1; i++)
    {
        mBPBuffer[i*2] = 0.0f;
        mBPBuffer[i*2+1] = 0.0f;
    }

    mBPBufferInputIndex = mBandPassFilterTap - 1;
    mBPBufferProcessingIndex = 0;

    mNumFramesProcessed = 0;
}

void CwBandPassFilterProcess::terminate()
{
#ifdef USE_CWBandPassFilter_TEST_PATTERN
    // clean up
    delete mSineOscI;
    delete mSineOscQ;
#endif
}
