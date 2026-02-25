#define LOG_TAG "DopplerPeakMeanPostProcess"

#include <DopplerPeakMeanPostProcess.h>
#include <opencv2/imgproc.hpp>
#include <ThorUtils.h>


DopplerPeakMeanPostProcess::DopplerPeakMeanPostProcess(const std::shared_ptr<UpsReader> &ups) :
    ImageProcess(ups)
{}

ThorStatus DopplerPeakMeanPostProcess::init()
{
    mDataType = DAU_DATA_TYPE_PW;
    return THOR_OK;
}

ThorStatus DopplerPeakMeanPostProcess::setProcessParams(ImageProcess::ProcessParams &params)
{
    mParams = params;

    return THOR_OK;
}

void DopplerPeakMeanPostProcess::setDataType(int dataType)
{
    mDataType = dataType;
}

void DopplerPeakMeanPostProcess::setProcessIndices(uint32_t numFFT, float relThreshold, float absThreshold)
{
    // Number of FFT:
    // Expected: PW: 256, CW: 512
    mNumFFT = numFFT;

    mRelThreshold = relThreshold;
    mAbsThreshold = absThreshold;
}

ThorStatus DopplerPeakMeanPostProcess::process(uint8_t *inputPtr, uint8_t *outputPtr)
{
    // input header pointer
    CineBuffer::CineFrameHeader* inputHeaderPtr =
            reinterpret_cast<CineBuffer::CineFrameHeader*>(inputPtr - sizeof(CineBuffer::CineFrameHeader));

    // output header pointer
    CineBuffer::CineFrameHeader* outputHeaderPtr =
            reinterpret_cast<CineBuffer::CineFrameHeader*>(outputPtr - sizeof(CineBuffer::CineFrameHeader));

    // Number of Lines Per Frame from header
    uint32_t numLinesPerFrame = inputHeaderPtr->numSamples;
    // output header update
    outputHeaderPtr->numSamples = numLinesPerFrame;
    outputHeaderPtr->frameNum = inputHeaderPtr->frameNum;

    // assumption: inputA without preprocessing, inputB with preprocessing
    float*  inputFloatPtr = (float*) inputPtr;
    float*  outputFloatPtr = (float*) outputPtr;
    float   inputA, inputB, diff, rel_diff;
    int lineIdx;

    // positive and negative
    for (lineIdx = 0; lineIdx < numLinesPerFrame * 2; lineIdx++)
    {
        inputA = *inputFloatPtr;
        inputB = *outputFloatPtr;

        diff = abs(inputA - inputB);
        rel_diff = abs(diff/inputB);

        if (rel_diff > mRelThreshold)
            *outputFloatPtr = inputB;
        else
            *outputFloatPtr = inputA;

        // update pointer
        inputFloatPtr++;
        outputFloatPtr++;
    }

    return THOR_OK;
}

void DopplerPeakMeanPostProcess::terminate()
{}
