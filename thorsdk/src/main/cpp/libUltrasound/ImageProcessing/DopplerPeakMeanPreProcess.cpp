#define LOG_TAG "DopplerPeakMeanPreProcess"

#include <DopplerPeakMeanPreProcess.h>
#include <opencv2/imgproc.hpp>
#include <ThorUtils.h>

#define CROSS_WIDTH_CW 25
#define CROSS_WIDTH_PW 10

DopplerPeakMeanPreProcess::DopplerPeakMeanPreProcess(const std::shared_ptr<UpsReader> &ups) :
    ImageProcess(ups)
{}

ThorStatus DopplerPeakMeanPreProcess::init()
{
    mDataType = DAU_DATA_TYPE_PW;
    return THOR_OK;
}

ThorStatus DopplerPeakMeanPreProcess::setProcessParams(ImageProcess::ProcessParams &params)
{
    mParams = params;

    return THOR_OK;
}

void DopplerPeakMeanPreProcess::setDataType(int dataType)
{
    mDataType = dataType;
}

ThorStatus DopplerPeakMeanPreProcess::process(uint8_t *inputPtr, uint8_t *outputPtr)
{
    int crossWidth = CROSS_WIDTH_PW;

    if (mDataType == DAU_DATA_TYPE_CW)
        crossWidth = CROSS_WIDTH_CW;

    cv::Mat input_8u(mParams.numRays, mParams.numSamples, CV_8U, inputPtr);
    cv::Mat output_8u(mParams.numRays, mParams.numSamples, CV_8U, outputPtr);

    cv::Mat inImg  = cv::Mat(mParams.numRays, mParams.numSamples, CV_32F);
    input_8u.convertTo(inImg, CV_32F);

    cv::Mat outImg = cv::Mat(mParams.numRays, mParams.numSamples, CV_32F);
    cv::Mat erElement = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(crossWidth, crossWidth));

    cv::morphologyEx(inImg, outImg, cv::MORPH_CLOSE, erElement);
    outImg.convertTo(output_8u, CV_8U);

    return THOR_OK;
}

void DopplerPeakMeanPreProcess::terminate()
{}
