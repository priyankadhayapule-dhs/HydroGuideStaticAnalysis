#define LOG_TAG "BEdgeFilterProcess"

#include <BEdgeFilterProcess.h>
#include <opencv2/imgproc.hpp>

#include <ThorUtils.h>

BEdgeFilterProcess::BEdgeFilterProcess(const std::shared_ptr<UpsReader> &ups) :
        ImageProcess(ups)
{}

ThorStatus BEdgeFilterProcess::init()
{
    return THOR_OK;
}

ThorStatus BEdgeFilterProcess::setProcessParams(ImageProcess::ProcessParams &params)
{
    getUpsReader()->getBEdgeFilterParams(params.imagingCaseId, mBEdgeFilterParams);
    mParams = params;
    mOutputF  = cv::Mat(mParams.numRays, mParams.numSamples, CV_32F);
    mOutputF2  = cv::Mat(mParams.numRays, mParams.numSamples, CV_32F);
    return THOR_OK;
}

ThorStatus BEdgeFilterProcess::edgeFilter(uint8_t *inputPtr, uint8_t *outputPtr) {
    ThorStatus st = THOR_OK;
    cv::Mat input_8u(mParams.numRays, mParams.numSamples, CV_8U, inputPtr);
    cv::Mat output_8u(mParams.numRays, mParams.numSamples, CV_8U, outputPtr);

    int scBlendMode = mBEdgeFilterParams.scBlendMode;
    float scBlendWeight = mBEdgeFilterParams.scBlendWeight;
    int lpfKernel = mBEdgeFilterParams.lpfKernel;
    float edgeWeight = mBEdgeFilterParams.edgeWeight;
    int order = mBEdgeFilterParams.order;

    if (order==-1)
    {
        input_8u.convertTo(mOutputF, CV_32F);
        mOutputF.convertTo(output_8u, CV_8U);
        return( st );
    }
    input_8u.convertTo(mOutputF, CV_32F);
    input_8u.convertTo(mOutputF2, CV_32F);
    cv::GaussianBlur(mOutputF, mOutputF2, Size(lpfKernel, lpfKernel), 0, 0, BORDER_REPLICATE);
    cv::addWeighted(mOutputF, edgeWeight, mOutputF2, 1-edgeWeight, 0, mOutputF2);
    mOutputF2.convertTo(output_8u, CV_8U);
    return( st );
}

ThorStatus BEdgeFilterProcess::process(uint8_t *inputPtr, uint8_t *outputPtr)
{
    ThorStatus  retVal = THOR_OK;
    cv::Mat input_8u(mParams.numRays, mParams.numSamples, CV_8U, inputPtr);
    cv::Mat output_8u(mParams.numRays, mParams.numSamples, CV_8U, outputPtr);
    edgeFilter(inputPtr,outputPtr);
    return(retVal);
}

void BEdgeFilterProcess::terminate()
{}
