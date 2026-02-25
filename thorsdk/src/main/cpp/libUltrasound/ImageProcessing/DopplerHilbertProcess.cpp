//
// Copyright 2020 EchoNous Inc.
//
//
#define LOG_TAG "DopplerHilbertProcess"

#include <DopplerHilbertProcess.h>
#include <ThorUtils.h>

//#define DEBUG_HILBERT_FILTER_COEFFS

DopplerHilbertProcess::DopplerHilbertProcess(const std::shared_ptr<UpsReader> &upsReader) :
    ImageProcess(upsReader)
{
    mHilbertFilterTap = 64;
    mNumDopplerSamplesPerFrame = 48;
}

ThorStatus DopplerHilbertProcess::init()
{
    return THOR_OK;
}

ThorStatus DopplerHilbertProcess::setProcessParams(ProcessParams &params)
{
    mParams = params;
    LOGD("DopplerHilbertProcess::setProcessParams(): numRays = %d", mParams.numRays);
    return THOR_OK;
}

void DopplerHilbertProcess::setProcessIndices(uint32_t prfIndex, uint32_t hilbertFilterIndex, bool isTDI)
{
    // read Hilbert Filter
    mNumDopplerSamplesPerFrame = getUpsReader()->getNumPwSamplesPerFrame(prfIndex, isTDI);
    mHilbertFilterTap = getUpsReader()->getPwHilbertCoefficients(hilbertFilterIndex,
                                                                 mHilbertFilterCoeffs);

    LOGD("DopplerHilbertProcess:prfIndex(%d), HilberFilterIndex(%d), mHilbertFilterTap(%d), "
         "mNumDopplerSamplesPerFrame(%d)", prfIndex, hilbertFilterIndex,
         mHilbertFilterTap, mNumDopplerSamplesPerFrame);

#ifdef DEBUG_HILBERT_FILTER_COEFFS
    for (int i = 0; i < mHilbertFilterTap; i++)
        LOGD("setProcessIndices - Hilbert Filter Coeffs: %d %.10e\n", i, mHilbertFilterCoeffs[i]);
#endif

    // Initialize Hilbert Filter Buffer and processing indices
    for (int i = 0; i < mHilbertFilterTap - 1; i++)
    {
        mHilbertBuffer[i*2] = 0.0f;
        mHilbertBuffer[i*2+1] = 0.0f;
    }

    mHilbertBufferInputIndex = mHilbertFilterTap - 1;
    mHilbertBufferProcessingIndex = 0;
}

void DopplerHilbertProcess::setCwProcessIndices(uint32_t prfIndex, uint32_t hilbertFilterIndex)
{
    // read Hilbert Filter & numSamplesPerFrame
    uint32_t decimationFactor;
    getUpsReader()->getCwDecFactorSamples(prfIndex, decimationFactor, mNumDopplerSamplesPerFrame);
    // TODO: update with CwHilbert coefficient
    mHilbertFilterTap = getUpsReader()->getPwHilbertCoefficients(hilbertFilterIndex,
                                                                 mHilbertFilterCoeffs);

    LOGD("DopplerHilbertProcess(CW): prfIndex(%d), HilberFilterIndex(%d), mHilbertFilterTap(%d), "
         "mNumDopplerSamplesPerFrame-decimated(%d)", prfIndex, hilbertFilterIndex,
         mHilbertFilterTap, mNumDopplerSamplesPerFrame);

#ifdef DEBUG_HILBERT_FILTER_COEFFS
    for (int i = 0; i < mHilbertFilterTap; i++)
        LOGD("setProcessIndices - Hilbert Filter Coeffs: %d %.10e\n", i, mHilbertFilterCoeffs[i]);
#endif

    // Initialize Hilbert Filter Buffer and processing indices
    for (int i = 0; i < mHilbertFilterTap - 1; i++)
    {
        mHilbertBuffer[i*2] = 0.0f;
        mHilbertBuffer[i*2+1] = 0.0f;
    }

    mHilbertBufferInputIndex = mHilbertFilterTap - 1;
    mHilbertBufferProcessingIndex = 0;
}

ThorStatus DopplerHilbertProcess::process(uint8_t* inputPtr, uint8_t* outputPtr)
{
    // input pointer - does not include header
    // Assumption: input is in 32-bit float I and Q. Alternating IQ
    float* inputFloatPtr = (float*) inputPtr;

    // outputPtr does not include the frame header
    // output: Forward & Reverse Alternative
    float* outputFloatPtr = (float*) outputPtr;

    // put data into Hilbert buffer (pre-Filter)
    for (int i = 0; i < mNumDopplerSamplesPerFrame; i++)
    {
        mHilbertBuffer[2*mHilbertBufferInputIndex] = *inputFloatPtr++;
        mHilbertBuffer[2*mHilbertBufferInputIndex + 1] = *inputFloatPtr++;

        mHilbertBufferInputIndex = (mHilbertBufferInputIndex + 1) % HILBERT_BUFFER_SIZE;
    }

    // Hilbert Filter
    for (int i = 0; i < mNumDopplerSamplesPerFrame; i++)
    {
        float fval_I = 0.0f;
        float fval_Q = 0.0f;
        uint32_t current_data_index = mHilbertBufferProcessingIndex;

        for (int j = 0; j < mHilbertFilterTap; j++)
        {
            if (j == (mHilbertFilterTap - 1))
                fval_I = mHilbertBuffer[2*current_data_index];

            fval_Q += mHilbertBuffer[2*current_data_index + 1] * mHilbertFilterCoeffs[j];

            current_data_index = (current_data_index + 1) % HILBERT_BUFFER_SIZE;
        }

        mHilbertBufferProcessingIndex = (mHilbertBufferProcessingIndex + 1) % HILBERT_BUFFER_SIZE;

        // output to the next step
        *outputFloatPtr++ = fval_I - fval_Q;        // forward
        *outputFloatPtr++ = fval_I + fval_Q;        // reverse
    }
    
    return THOR_OK;
}

void DopplerHilbertProcess::terminate() {}
