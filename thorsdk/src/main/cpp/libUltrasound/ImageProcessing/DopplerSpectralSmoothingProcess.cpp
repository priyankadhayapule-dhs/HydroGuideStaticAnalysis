//
// Copyright 2020 EchoNous Inc.
//
//
#define LOG_TAG "DopplerSpectralSmoothingProcess"

#include <DopplerSpectralSmoothingProcess.h>
#include <ThorUtils.h>

DopplerSpectralSmoothingProcess::DopplerSpectralSmoothingProcess(const std::shared_ptr<UpsReader> &upsReader) :
    ImageProcess(upsReader)
{
    mKernelSize = 3;
    mFFTSize = 512;
}

ThorStatus DopplerSpectralSmoothingProcess::init()
{
    return THOR_OK;
}

ThorStatus DopplerSpectralSmoothingProcess::setProcessParams(ProcessParams &params)
{
    mParams = params;
    LOGD("DopplerSpectralSmoothingProcess::setProcessParams(): numRays = %d", mParams.numRays);
    return THOR_OK;
}

void DopplerSpectralSmoothingProcess::setProcessIndices(uint32_t fftSize, uint32_t kernelSize)
{
    // set processing Params
    mFFTSize = fftSize;
    mKernelSize = kernelSize;

    if (mKernelSize < 1)
        mKernelSize = 1;

    mScaleAdj = 1.0f/((float) mKernelSize);

    LOGD("DopplerSpectralSmoothingProcess: mFFTSize = %d, mKernelSize = %d", mFFTSize, mKernelSize);

    // Initialize Buffer and processing indices
    for (int i = 0; i < (mKernelSize - 1) * MAX_FFT_SIZE; i++)
    {
        mImgBuffer[i] = 0;
    }
}

ThorStatus DopplerSpectralSmoothingProcess::process(uint8_t* inputPtr, uint8_t* outputPtr)
{
    // input header pointer
    CineBuffer::CineFrameHeader* inputHeaderPtr =
            reinterpret_cast<CineBuffer::CineFrameHeader*>(inputPtr - sizeof(CineBuffer::CineFrameHeader));

    // output header pointer
    CineBuffer::CineFrameHeader* outputHeaderPtr =
            reinterpret_cast<CineBuffer::CineFrameHeader*>(outputPtr - sizeof(CineBuffer::CineFrameHeader));

    // Number of Lines Per Frame
    uint32_t numLinesPerFrame = inputHeaderPtr->numSamples;
    uint32_t index_offset, index_offset_subtract, index_offset_move;

    // put data into Img buffer - pre-Filter
    for (uint32_t j = 0; j < numLinesPerFrame; j++)
    {
        // shift kernel size -1 to preserve older lines
        index_offset = (j + mKernelSize - 1) * mFFTSize;

        for (uint32_t i = 0; i < mFFTSize; i++)
        {
            mImgBuffer[index_offset + i] = *inputPtr++;
        }
    }

    // initialize Line Accumulation Buffer
    for (int i = 0; i < MAX_FFT_SIZE; i++)
        mLineBuffer[i] = 0.0f;

    // first accumulation lines
    for (uint32_t j = 0; j < mKernelSize; j++)
    {
        index_offset = j * mFFTSize;

        for (uint32_t i = 0; i < mFFTSize; i++)
        {
            mLineBuffer[i] += (float) mImgBuffer[index_offset + i];
        }
    }

    // first output
    for (uint32_t i = 0; i < mFFTSize; i++)
    {
        *outputPtr++ = (uint8_t) (mLineBuffer[i] * mScaleAdj);
    }

    // Boxcar Filtering
    for (uint32_t j = mKernelSize; j < (numLinesPerFrame + mKernelSize - 1); j++)
    {
        index_offset_subtract = (j - mKernelSize) * mFFTSize;
        index_offset = j * mFFTSize;

        for (uint32_t i = 0; i < mFFTSize; i++)
        {
            // add current line and subtract older line
            mLineBuffer[i] += (float) mImgBuffer[index_offset + i];
            mLineBuffer[i] -= (float) mImgBuffer[index_offset_subtract + i];

            // store the output
            *outputPtr++ = (uint8_t) (mLineBuffer[i] * mScaleAdj);
        }
    }

    // output header update
    outputHeaderPtr->numSamples = numLinesPerFrame;

    // move the last lines to first lines.
    for (uint32_t j = 0; j < mKernelSize -1; j++)
    {
        index_offset = j * mFFTSize;
        index_offset_move = (j + numLinesPerFrame) * mFFTSize;

        for (uint32_t i = 0; i < mFFTSize; i++)
        {
            mImgBuffer[index_offset + i] = mImgBuffer[index_offset_move + i];
        }
    }

    return THOR_OK;
}

void DopplerSpectralSmoothingProcess::terminate() {}
