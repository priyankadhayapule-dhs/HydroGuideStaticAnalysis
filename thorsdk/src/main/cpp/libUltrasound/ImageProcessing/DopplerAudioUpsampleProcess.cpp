//
// Copyright 2020 EchoNous Inc.
//
//
#define LOG_TAG "DopplerAudioUpsampleProcess"

#include <DopplerAudioUpsampleProcess.h>
#include <ThorUtils.h>
#include <math.h>

#define PI 3.1415926f
#define SINC(x) (x == 0.0f ? 1.0f : sin(PI*x)/(PI*x))

//#define DEBUG_AUDIO_FILTER_COEFFS

DopplerAudioUpsampleProcess::DopplerAudioUpsampleProcess(const std::shared_ptr<UpsReader> &upsReader) :
    ImageProcess(upsReader)
{
    mAudioFilterTap = 128;
    mNumDopplerSamplesPerFrame = 48;
    mUpsampleRatio = 1;
    mAudioFilterCutoff = 0.1f;

    mAudioGainForward = 1.0f;
    mAudioGainReverse = 1.0f;

    // Fade-In related parameters
    mFadeInStartFrameNum = 0;
    mFadeInEndFrameNum = 2;
    mFadeInFrameNum = 0;
    mFadeInGain = 0.0f;
    mFadeInGainInc = 0.001f;

#ifdef DOPPLER_AUDIO_OUTPUT_TEST_PATTERN
    mSineOscL = new SineGenerator();
    mSineOscR = new SineGenerator();
#endif
}

ThorStatus DopplerAudioUpsampleProcess::init()
{
    return THOR_OK;
}

ThorStatus DopplerAudioUpsampleProcess::setProcessParams(ProcessParams &params)
{
    mParams = params;
    LOGD("DopplerAudioFilterProcess::setProcessParams(): numRays = %d", mParams.numRays);
    return THOR_OK;
}

// when upsampleRatio is 0, upsampleRatio is calculated to meet minimum 8k Hz sampling.
// upsample: 1 -> bypass with low-pass filtering
void DopplerAudioUpsampleProcess::setProcessingIndices(uint32_t prfIndex, uint32_t filterIndex,
                                                       bool isTDI, uint32_t upsampleRatio, bool useUPSFilter)
{
    // read coefficients from UPS
    mNumDopplerSamplesPerFrame = getUpsReader()->getNumPwSamplesPerFrame(prfIndex, isTDI);
    getUpsReader()->getPwAudioHilbertLpfParams(filterIndex, mAudioFilterTap, mAudioFilterCutoff);

    float prfHz = getUpsReader()->getGlobalsPtr()->samplingFrequency/
                  ((float) getUpsReader()->getAfeClksPerPwSample(prfIndex, isTDI))*1e6;

    mUpsampleRatio = upsampleRatio;

    // calculate upsampleRatio
    if (upsampleRatio < 1)
    {
        // if prf >= 8k, no need to upsample
        if (prfHz >= 8000)
            mUpsampleRatio = 1;
        else
            mUpsampleRatio = (uint32_t) ceil(8000.0f/prfHz);
    }

    // expected mUpsampleRatio 1 ~ 16
    if (mUpsampleRatio > 16)
    {
        LOGW("UpsampleRatio Too Big: %d", mUpsampleRatio);
        mUpsampleRatio = 16;
    }
    else if (mUpsampleRatio < 1)
    {
        LOGW("UpsampleRatio Too Small: %d", mUpsampleRatio);
        mUpsampleRatio = 1;
    }

    if (!useUPSFilter)
        mAudioFilterCutoff = mAudioFilterCutoff/mUpsampleRatio;

    setAudioFilterCoeffs(filterIndex, useUPSFilter);

    LOGD("DopplerAudioUpsampleProcess: mNumDopplerSamplesPerFrame (%d), mAudioFilterTap (%d), mUpsampleRatio (%d), "
         "mAudioFilterCutoff (%f)", mNumDopplerSamplesPerFrame, mAudioFilterTap, mUpsampleRatio, mAudioFilterCutoff);

    // TODO: AudioGain adjustment - forward and reverse

#ifdef DEBUG_AUDIO_FILTER_COEFFS
    for (int i = 0; i < mAudioFilterTap; i++)
        LOGD("setProcessingIndices - Doppler Audio Upsample - Lowpass Filter Coeffs: %d %.10e\n", i, mAudioFilterCoeffs[i]);
#endif

    // Initialize AudioBuffer and processing indices (worst case: bypass case)
    for (int i = 0; i < mAudioFilterTap - 1; i++)
    {
        mAudioBufferFwd[i] = 0.0f;
        mAudioBufferRev[i] = 0.0f;
    }

    mAudioBufferInputIndex = mAudioFilterTap - 1;

    // for the processing convenience - processing toward the reverse direction
    mAudioBufferProcessingIndex = mAudioFilterTap - 1;

#ifdef DOPPLER_AUDIO_OUTPUT_TEST_PATTERN
    float signalSamplingRate = prfHz * ((float )mUpsampleRatio);
    float testFreqL = 440.0f;
    float testFreqR = 880.0f;
    float testAmp = 0.5f;

    mSineOscL->setup(testFreqL, signalSamplingRate, testAmp);
    mSineOscR->setup(testFreqR, signalSamplingRate, testAmp);

    LOGD("DopplerAudioUpsampleProcess: Using Test Pattern: signalSamplingRate(%f), testSignalFrequencyLR(%f, %f)",
         signalSamplingRate, testFreqL, testFreqR);
#endif

    // Fade-In related parameters
    mFadeInStartFrameNum = 1;
    mFadeInEndFrameNum = 3;
    mFadeInFrameNum = 0;
    mFadeInGain = 0.0f;
    mFadeInGainInc = 1.0f/((float) (mNumDopplerSamplesPerFrame * (mFadeInEndFrameNum - mFadeInStartFrameNum + 1)));
}

// when upsampleRatio is 0, upsampleRatio is calculated to meet minimum 8k Hz sampling.
// upsample: 1 -> bypass with low-pass filtering
void DopplerAudioUpsampleProcess::setCwProcessingIndices(uint32_t prfIndex, uint32_t filterIndex,
                                                       uint32_t upsampleRatio, bool useUPSFilter)
{
    // read coefficients from UPS
    uint32_t decimationFactor;
    getUpsReader()->getCwDecFactorSamples(prfIndex, decimationFactor, mNumDopplerSamplesPerFrame);
    // TODO: udupdate with CW Audio Table
    getUpsReader()->getPwAudioHilbertLpfParams(filterIndex, mAudioFilterTap, mAudioFilterCutoff);

    float prfHz = 195312.5/decimationFactor;

    mUpsampleRatio = upsampleRatio;

    // calculate upsampleRatio
    if (upsampleRatio < 1)
    {
        // if prf >= 8k, no need to upsample
        if (prfHz >= 8000)
            mUpsampleRatio = 1;
        else
            mUpsampleRatio = (uint32_t) ceil(8000.0f/prfHz);
    }

    // expected mUpsampleRatio 1 ~ 8
    if (mUpsampleRatio > 8)
    {
        LOGW("UpsampleRatio Too Big: %d", mUpsampleRatio);
        mUpsampleRatio = 8;
    }
    else if (mUpsampleRatio < 1)
    {
        LOGW("UpsampleRatio Too Small: %d", mUpsampleRatio);
        mUpsampleRatio = 1;
    }

    if (!useUPSFilter)
        mAudioFilterCutoff = mAudioFilterCutoff/mUpsampleRatio;

    setAudioFilterCoeffs(filterIndex, useUPSFilter);

    LOGD("DopplerAudioUpsampleProcess(CW): mNumDopplerSamplesPerFrame(decim) (%d), mAudioFilterTap (%d), mUpsampleRatio (%d), "
         "mAudioFilterCutoff (%f)", mNumDopplerSamplesPerFrame, mAudioFilterTap, mUpsampleRatio, mAudioFilterCutoff);

    // TODO: AudioGain adjustment - forward and reverse

#ifdef DEBUG_AUDIO_FILTER_COEFFS
    for (int i = 0; i < mAudioFilterTap; i++)
        LOGD("setProcessingIndices - Doppler Audio Upsample - Lowpass Filter Coeffs: %d %.10e\n", i, mAudioFilterCoeffs[i]);
#endif

    // Initialize AudioBuffer and processing indices (worst case: bypass case)
    for (int i = 0; i < mAudioFilterTap - 1; i++)
    {
        mAudioBufferFwd[i] = 0.0f;
        mAudioBufferRev[i] = 0.0f;
    }

    mAudioBufferInputIndex = mAudioFilterTap - 1;

    // for the processing convenience - processing toward the reverse direction
    mAudioBufferProcessingIndex = mAudioFilterTap - 1;

#ifdef DOPPLER_AUDIO_OUTPUT_TEST_PATTERN
    float signalSamplingRate = prfHz * ((float )mUpsampleRatio);
    float testFreqL = 440.0f;
    float testFreqR = 880.0f;
    float testAmp = 0.5f;

    mSineOscL->setup(testFreqL, signalSamplingRate, testAmp);
    mSineOscR->setup(testFreqR, signalSamplingRate, testAmp);

    LOGD("DopplerAudioUpsampleProcess: Using Test Pattern: signalSamplingRate(%f), testSignalFrequencyLR(%f, %f)",
         signalSamplingRate, testFreqL, testFreqR);
#endif

    // Fade-In related parameters
    mFadeInStartFrameNum = 3;
    mFadeInEndFrameNum = 13;
    mFadeInFrameNum = 0;
    mFadeInGain = 0.0f;
    mFadeInGainInc = 1.0f/((float) (mNumDopplerSamplesPerFrame * (mFadeInEndFrameNum - mFadeInStartFrameNum + 1)));
}

void DopplerAudioUpsampleProcess::setAudioGain(float dBGain)
{
    mAudioGainForward = pow(10.0f, (dBGain) / 20.0f);;
    mAudioGainReverse = pow(10.0f, (dBGain) / 20.0f);;
}

void DopplerAudioUpsampleProcess::setAudioFilterCoeffs(uint32_t filterIndex, bool useUPSFilter)
{
    if (useUPSFilter)
    {
        // read filter coefficents from UPS
        getUpsReader()->getPwAudioHilbertLpfCoeffs(filterIndex, mAudioFilterCoeffs);
    }
    else
    {
        float halfTap = 0.5f * (mAudioFilterTap - 1);
        float n;
        float filterCoeff;
        float windowCoeff;

        for (int  fi = 0; fi < mAudioFilterTap; fi++)
        {
            // filter coeff (sinc-based)
            n = (((float) fi) - halfTap) * mAudioFilterCutoff;
            filterCoeff = mAudioFilterCutoff * SINC(n);

            // Hanning window coeff
            windowCoeff = 0.5f * (1.0f - cos(2.0f*PI*fi/(mAudioFilterTap - 1)));

            // combined coeffs
            mAudioFilterCoeffs[fi] = windowCoeff * filterCoeff;
         }
    }
}

ThorStatus DopplerAudioUpsampleProcess::process(uint8_t* inputPtr, uint8_t* outputPtr)
{
    // input pointer - does not include header
    // Assumption: input is in 32-bit float I and Q. Alternating IQ
    float* inputFloatPtr = (float*) inputPtr;

    // outputPtr does not include the frame header, L & R channel alternate
    float* outputFloatPtr = (float*) outputPtr;

    // put data into WF buffer (pre-Wall Filter)
    if (mFadeInFrameNum > mFadeInEndFrameNum)
    {
        for (int i = 0; i < mNumDopplerSamplesPerFrame; i++) {
            mAudioBufferFwd[mAudioBufferInputIndex] = *inputFloatPtr++;
            mAudioBufferRev[mAudioBufferInputIndex] = *inputFloatPtr++;

            mAudioBufferInputIndex = (mAudioBufferInputIndex + 1) % AUDIO_BUFFER_SIZE;
        }
    }
    else
    {
        for (int i = 0; i < mNumDopplerSamplesPerFrame; i++) {
            mAudioBufferFwd[mAudioBufferInputIndex] = (*inputFloatPtr++) * mFadeInGain;
            mAudioBufferRev[mAudioBufferInputIndex] = (*inputFloatPtr++) * mFadeInGain;

            mAudioBufferInputIndex = (mAudioBufferInputIndex + 1) % AUDIO_BUFFER_SIZE;

            if (mFadeInFrameNum >= mFadeInStartFrameNum)
                mFadeInGain += mFadeInGainInc;
        }
        // update frame Num
        mFadeInFrameNum++;
    }

    // Upsample - Low Pass FIR
    // num of input samples: mNumDopplerSamplesPerFrame
    // num of output samples: mNumDopplerSamplesPerFrame * mUpsampleRatio
    for (int i = 0; i < mNumDopplerSamplesPerFrame; i++)
    {
        // up-sampling index
        for (int us = 0; us < mUpsampleRatio; us++)
        {
            float fval_L = 0.0f;
            float fval_R = 0.0f;
            int current_data_index = mAudioBufferProcessingIndex;

            // filter index
            for (int fi = mAudioFilterTap - 1 - us; fi >= 0; fi -= mUpsampleRatio)
            {
                fval_L += mAudioBufferFwd[current_data_index] * mAudioFilterCoeffs[fi];
                fval_R += mAudioBufferRev[current_data_index] * mAudioFilterCoeffs[fi];
                current_data_index--;

                if (current_data_index < 0)
                {
                    current_data_index += AUDIO_BUFFER_SIZE;
                }
            }

            // apply gain
            fval_L *= mAudioGainForward;
            fval_R *= mAudioGainReverse;

            // TODO: Clipping & Scaling (if needed)
            if (fval_L > 1.0f)
                fval_L = 1.0f;
            else if (fval_L < -1.0f)
                fval_L = -1.0f;

            if (fval_R > 1.0f)
                fval_R = 1.0f;
            else if (fval_R < -1.0f)
                fval_R = -1.0f;

#ifdef DOPPLER_AUDIO_OUTPUT_TEST_PATTERN
            float test_sigL, test_sigR;

            mSineOscL->render(&test_sigL, 1, 1);
            mSineOscR->render(&test_sigR, 1, 1);

            fval_L = test_sigL;
            fval_R = test_sigR;
#endif

            // output audio (L & R)
            *outputFloatPtr++ = fval_L;
            *outputFloatPtr++ = fval_R;
        }

        mAudioBufferProcessingIndex = (mAudioBufferProcessingIndex + 1) % AUDIO_BUFFER_SIZE;
    }

    return THOR_OK;
}

void DopplerAudioUpsampleProcess::terminate()
{
#ifdef DOPPLER_AUDIO_OUTPUT_TEST_PATTERN
    delete mSineOscL;
    delete mSineOscR;
#endif
}
