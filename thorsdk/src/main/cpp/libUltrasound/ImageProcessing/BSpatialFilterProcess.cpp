#define LOG_TAG "BSpatialFilterProcess"

#include <BSpatialFilterProcess.h>
#include <opencv2/imgproc.hpp>

#include <ThorUtils.h>

BSpatialFilterProcess::BSpatialFilterProcess(const std::shared_ptr<UpsReader> &ups) :
        ImageProcess(ups)
{}

ThorStatus BSpatialFilterProcess::init()
{
    return THOR_OK;
}

ThorStatus BSpatialFilterProcess::setProcessParams(ImageProcess::ProcessParams &params)
{
    getUpsReader()->getBSpatialFilterParams(params.imagingCaseId, mBSpatialFilterParams);
    mParams = params;
    mOutputF  = cv::Mat(mParams.numRays, mParams.numSamples, CV_32F);
    return THOR_OK;
}

ThorStatus BSpatialFilterProcess::spatialFilter(uint8_t *inputPtr, uint8_t *outputPtr) {
    ThorStatus st = THOR_OK;
    cv::Mat input_8u(mParams.numRays, mParams.numSamples, CV_8U, inputPtr);
    cv::Mat output_8u(mParams.numRays, mParams.numSamples, CV_8U, outputPtr);

    int mode = mBSpatialFilterParams.mode;
    float blackholeaxialthresh=mBSpatialFilterParams.blackholeaxialthresh;
    float blackholelateralthresh=mBSpatialFilterParams.blackholelateralthresh;
    float blackholealpha=mBSpatialFilterParams.blackholealpha; //0=replace
    float blakholeweight = 1-blackholealpha;
    float noiseaxialthresh=mBSpatialFilterParams.noiseaxialthresh;
    float noiselateralthresh=mBSpatialFilterParams.noiselateralthresh;
    float noisealpha=mBSpatialFilterParams.noisealpha; //0=replace
    float noiseweight = noisealpha;
    float axialH,lateralH,blackholeH,maxL,maxA,axialN,lateralN,noiseH;

    if (mode==-1)
    {
        input_8u.convertTo(mOutputF, CV_32F);
        mOutputF.convertTo(output_8u, CV_8U);
        return( st );
    }
    if (mode==0)
    {
        cv::blur(input_8u,input_8u,Size(mBSpatialFilterParams.kernelaxialsize,mBSpatialFilterParams.kernellateralsize),Point(-1,-1),BORDER_REPLICATE);
        input_8u.convertTo(mInputF, CV_32F);
        input_8u.convertTo(mOutputF, CV_32F);
        for (uint32_t li = 1; li < mParams.numRays-1; li++)
        {
            for (uint32_t si = 1; si < mParams.numSamples-1; si++)
            {
                lateralH=round(0.5*(mInputF.at<float>(li-1,si)+mInputF.at<float>(li+1,si)))-blackholelateralthresh;
                axialH=round(0.5*(mInputF.at<float>(li,si-1)+mInputF.at<float>(li,si+1)))-blackholeaxialthresh;
                blackholeH = axialH >= lateralH ? axialH : lateralH;
                if (blackholeH>mInputF.at<float>(li,si))
                {
                    mOutputF.at<float>(li,si) = mInputF.at<float>(li,si) + blakholeweight*(blackholeH-mInputF.at<float>(li,si));
                }
            }
        }
    }
    else if (mode==1)
    {
        cv::GaussianBlur( input_8u,input_8u, Size(mBSpatialFilterParams.kernelaxialsize,mBSpatialFilterParams.kernellateralsize),mBSpatialFilterParams.gaussianaxialsigma, mBSpatialFilterParams.gaussianlateralsigma,BORDER_REPLICATE);
        input_8u.convertTo(mInputF, CV_32F);
        input_8u.convertTo(mOutputF, CV_32F);
        for (uint32_t li = 1; li < mParams.numRays-1; li++)
        {
            for (uint32_t si = 1; si < mParams.numSamples-1; si++)
            {
                lateralH=round(0.5*(mInputF.at<float>(li-1,si)+mInputF.at<float>(li+1,si)))-blackholelateralthresh;
                axialH=round(0.5*(mInputF.at<float>(li,si-1)+mInputF.at<float>(li,si+1)))-blackholeaxialthresh;
                blackholeH = axialH >= lateralH ? axialH : lateralH;
                if (blackholeH>mInputF.at<float>(li,si))
                {
                    mOutputF.at<float>(li,si) = mInputF.at<float>(li,si) + blakholeweight*(blackholeH-mInputF.at<float>(li,si));
                }
            }
        }
    }

    mOutputF.convertTo(output_8u, CV_8U);
    return( st );
}

ThorStatus BSpatialFilterProcess::process(uint8_t *inputPtr, uint8_t *outputPtr)
{
    ThorStatus  retVal = THOR_OK;
    cv::Mat input_8u(mParams.numRays, mParams.numSamples, CV_8U, inputPtr);
    cv::Mat output_8u(mParams.numRays, mParams.numSamples, CV_8U, outputPtr);
    spatialFilter(inputPtr,outputPtr);
    //cv::bilateralFilter( input_8u, output_8u, 5, 71, 71,BORDER_REPLICATE);
    //cv::GaussianBlur( input_8u,output_8u, Size(5,5),0.75, 0.75,BORDER_REPLICATE);
    //cv::medianBlur( input_8u, output_8u, 5);
    return(retVal);
}

void BSpatialFilterProcess::terminate()
{}
