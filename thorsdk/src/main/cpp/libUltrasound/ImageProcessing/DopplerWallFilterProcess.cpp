//
// Copyright 2020 EchoNous Inc.
//
//
#define LOG_TAG "DopplerWallFilterProcess"

#include <DopplerWallFilterProcess.h>
#include <ThorUtils.h>

//#define DEBUG_WALL_FILTER_COEFFS

DopplerWallFilterProcess::DopplerWallFilterProcess(const std::shared_ptr<UpsReader> &upsReader) :
    ImageProcess(upsReader)
{
    mWallFilterTap = 127;
    mNumDopplerSamplesPerFrame = 48;
    mWFGain = 1.0f;
}

ThorStatus DopplerWallFilterProcess::init()
{
    return THOR_OK;
}

ThorStatus DopplerWallFilterProcess::setProcessParams(ProcessParams &params)
{
    mParams = params;
    LOGD("DopplerWallFilterProcess::setProcessParams(): numRays = %d", mParams.numRays);
    return THOR_OK;
}

void DopplerWallFilterProcess::setProcessIndices(uint32_t wallFilterIndex, uint32_t prfIndex, bool isTDI)
{
    // read Wall Filter Tap and Coefficients
    mWallFilterTap = getUpsReader()->getPwWallFilterCoefficients(wallFilterIndex, mWallFilterCoeffs, isTDI);
    mNumDopplerSamplesPerFrame = getUpsReader()->getNumPwSamplesPerFrame(prfIndex, isTDI);

    LOGD("DopplerWallFilterProcess:wallFilterIndex(%d), mWallFilterTap = %d, mNumDopplerSamplesPerFrame = %d",
         wallFilterIndex, mWallFilterTap, mNumDopplerSamplesPerFrame);

#ifdef DEBUG_WALL_FILTER_COEFFS
    for (int i = 0; i < mWallFilterTap; i++)
        LOGD("setProcessIndices - Doppler Wall Filter Coeffs: %d %.10e\n", i, mWallFilterCoeffs[i]);
#endif

    // Initialize Wall Filter Buffer and processing indices
    for (int i = 0; i < mWallFilterTap - 1; i++)
    {
        mWFBuffer[i*2] = 0.0f;
        mWFBuffer[i*2+1] = 0.0f;
    }

    mWFBufferInputIndex = mWallFilterTap - 1;
    mWFBufferProcessingIndex = 0;
}

void DopplerWallFilterProcess::setCwProcessIndices(uint32_t wallFilterIndex, uint32_t prfIndex)
{
    // read Wall Filter Tap and Coefficients
    mWallFilterTap = getUpsReader()->getPwWallFilterCoefficients(wallFilterIndex, mWallFilterCoeffs, false);

    // input -> decimatedSamples Per Frame
    uint32_t decimationFactor;
    getUpsReader()->getCwDecFactorSamples(prfIndex, decimationFactor, mNumDopplerSamplesPerFrame);

    LOGD("DopplerWallFilterProcess(CW):wallFilterIndex(%d), mWallFilterTap = %d, mNumDopplerSamplesPerFrame (decimated) = %d",
         wallFilterIndex, mWallFilterTap, mNumDopplerSamplesPerFrame);

#ifdef DEBUG_WALL_FILTER_COEFFS
    for (int i = 0; i < mWallFilterTap; i++)
        LOGD("setProcessIndices - Doppler Wall Filter Coeffs: %d %.10e\n", i, mWallFilterCoeffs[i]);
#endif

    // Initialize Wall Filter Buffer and processing indices
    for (int i = 0; i < mWallFilterTap - 1; i++)
    {
        mWFBuffer[i*2] = 0.0f;
        mWFBuffer[i*2+1] = 0.0f;
    }

    mWFBufferInputIndex = mWallFilterTap - 1;
    mWFBufferProcessingIndex = 0;
}

void DopplerWallFilterProcess::setWFGain(float dBGain)
{
    mWFGain = pow(10.0f, (dBGain) / 20.0f);
}

ThorStatus DopplerWallFilterProcess::process(uint8_t* inputPtr, uint8_t* outputPtr)
{
    // input pointer - does not include header
    // Assumption: input is in 32-bit float I and Q. Alternating IQ
    float* inputFloatPtr = (float*) inputPtr;

    // outputPtr does not include the frame header & float for this step
    float* outputFloatPtr = (float*) outputPtr;

    // put data into WF buffer (pre-Wall Filter)
    for (int i = 0; i < mNumDopplerSamplesPerFrame; i++)
    {
        mWFBuffer[2*mWFBufferInputIndex] = *inputFloatPtr++;
        mWFBuffer[2*mWFBufferInputIndex + 1] = *inputFloatPtr++;

        mWFBufferInputIndex = (mWFBufferInputIndex + 1) % WF_BUFFER_SIZE;
    }

    // Wall Filter
    for (int i = 0; i < mNumDopplerSamplesPerFrame; i++)
    {
        float fval_I = 0.0f;
        float fval_Q = 0.0f;
        uint32_t current_data_index = mWFBufferProcessingIndex;

        for (int j = 0; j < mWallFilterTap; j++)
        {
            fval_I += mWFBuffer[2*current_data_index] * mWallFilterCoeffs[j];
            fval_Q += mWFBuffer[2*current_data_index + 1] * mWallFilterCoeffs[j];

            current_data_index = (current_data_index + 1) % WF_BUFFER_SIZE;
        }

        mWFBufferProcessingIndex = (mWFBufferProcessingIndex + 1) % WF_BUFFER_SIZE;

        // output to the next step
        *outputFloatPtr++ = fval_I * mWFGain;
        *outputFloatPtr++ = fval_Q * mWFGain;
    }
    
    return THOR_OK;
}

void DopplerWallFilterProcess::terminate() {}
