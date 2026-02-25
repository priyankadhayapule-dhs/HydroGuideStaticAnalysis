#define LOG_TAG "PersistenceProcess"

#include <PersistenceProcess.h>
#include <ThorUtils.h>

PersistenceProcess::PersistenceProcess(const std::shared_ptr<UpsReader> &upsReader) :
    ImageProcess(upsReader) {}

ThorStatus PersistenceProcess::init()
{
    mReset = true;
    return THOR_OK;
}

ThorStatus PersistenceProcess::setProcessParams(ProcessParams &params)
{
    mReset = true;
    mParams = params;
    mPersist = cv::Mat(mParams.numRays, mParams.numSamples, CV_8U);
    mDifference = cv::Mat(mParams.numRays, mParams.numSamples, CV_8U);
    mAlpha = cv::Mat(mParams.numRays, mParams.numSamples, CV_8U);

    mLUT = cv::Mat(256, 1, CV_8U);
    float lut[256];
    getUpsReader()->getBPersistenceLut(params.imagingCaseId, lut);

    cv::Mat tmp(256, 1, CV_32F, &lut);
    tmp.convertTo(mLUT, CV_8U, 255.0);

    return THOR_OK;
}

ThorStatus PersistenceProcess::process(uint8_t *inputPtr, uint8_t *outputPtr)
{
    cv::Mat input(mParams.numRays, mParams.numSamples, CV_8U, inputPtr);
    cv::Mat output(mParams.numRays, mParams.numSamples, CV_8U, outputPtr);


    if (mReset)
    {
        input.copyTo(mPersist);
        mReset = false;
    }
    else
    {
        cv::absdiff(input, mPersist, mDifference);
        cv::LUT(mDifference, mLUT, mAlpha);

        // result is alpha * persist + (1-alpha) * input

        cv::multiply(mPersist, mAlpha, mPersist, 1/255.0, CV_8U);
        cv::bitwise_not(mAlpha, mAlpha);
        cv::multiply(input, mAlpha, mDifference, 1/255.0, CV_8U); // using difference as a temporary here...
        cv::add(mPersist, mDifference, mPersist);
    }

    mPersist.copyTo(output);

    return THOR_OK;
}

void PersistenceProcess::setResetFlag()
{
    mReset = true;
}

void PersistenceProcess::terminate() {}