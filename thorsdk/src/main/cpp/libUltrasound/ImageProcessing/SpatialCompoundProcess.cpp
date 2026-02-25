#define LOG_TAG "SpatialCompoundProcess"

#include <SpatialCompoundProcess.h>
#include <opencv2/imgproc.hpp>
#include <ThorUtils.h>

SpatialCompoundProcess::SpatialCompoundProcess(const std::shared_ptr<UpsReader> &ups) :
    ImageProcess(ups)
{}

ThorStatus SpatialCompoundProcess::init()
{
    return THOR_OK;
}

ThorStatus SpatialCompoundProcess::setProcessParams(ImageProcess::ProcessParams &params)
{
    mParams = params;

    // params
    uint32_t    crossWidth;
    uint32_t    numViews;
    float       steeringAngleDegree;

    getUpsReader()->getSCParams(params.imagingCaseId, numViews, steeringAngleDegree, crossWidth);

    getUpsReader()->getBEdgeFilterParams(params.imagingCaseId, mBEdgeFilterParams);
    if (numViews < 2)
    {
        // no compounding
        mBypassCompounding = true;
        return THOR_OK;
    }
    else
        mBypassCompounding = false;

    float steeringRad = steeringAngleDegree/180.0f * 3.1415926f;
    float sinAlpha = sin(steeringRad);
    float cosAlpha = cos(steeringRad);

    // calculate pixel shift
    ScanConverterParams converterParams;
    converterParams.probeShape = PROBE_SHAPE_LINEAR;
    getUpsReader()->getScanConverterParams(params.imagingCaseId, converterParams, params.imagingMode);

    float imgDepth = (mParams.numSamples - 1) * converterParams.sampleSpacingMm;
    float pixelShift = imgDepth * sinAlpha / converterParams.raySpacing;
    float depthShift = imgDepth * cosAlpha / converterParams.sampleSpacingMm;

    mWeightCtr = cv::Mat(mParams.numRays, mParams.numSamples, CV_32F);		// ctr weighting img
    mWeightPos = cv::Mat(mParams.numRays, mParams.numSamples, CV_32F);		// pos weighting img
    mWeightNeg = cv::Mat(mParams.numRays, mParams.numSamples, CV_32F);		// neg weighting img

    mCtrImg    = cv::Mat::ones(mParams.numRays, mParams.numSamples, CV_32F);
    mPosImg    = cv::Mat(mParams.numRays, mParams.numSamples, CV_32F);
    mNegImg    = cv::Mat(mParams.numRays, mParams.numSamples, CV_32F);
    mInOutImg  = cv::Mat(mParams.numRays, mParams.numSamples, CV_32F);

    // intermediate images
    mCtrImg2   = cv::Mat(mParams.numRays, mParams.numSamples, CV_32F);
    mPosImg2   = cv::Mat(mParams.numRays, mParams.numSamples, CV_32F);
    mNegImg2   = cv::Mat(mParams.numRays, mParams.numSamples, CV_32F);
    mMaxImg    = cv::Mat(mParams.numRays, mParams.numSamples, CV_32F);

    // define triangles
    mSrcTri[0] = Point2f( 0.0f, 0.0f );
    mSrcTri[1] = Point2f( (mParams.numSamples - 1.0f), 0.0f );
    mSrcTri[2] = Point2f( 0.0f, mParams.numRays - 1.0f );

    // positive steered triangle
    mPosTri[0] = Point2f( 0.0f, 0.0f );
    mPosTri[1] = Point2f( depthShift, pixelShift );
    mPosTri[2] = Point2f( 0.0f, mParams.numRays - 1.0f );

    // negative steered triangle
    mNegTri[0] = Point2f( 0.0f, 0.0f );
    mNegTri[1] = Point2f( depthShift, -pixelShift );
    mNegTri[2] = Point2f( 0.0f, mParams.numRays - 1.0f );

    // calculate pos and neg transformation matrices
    mPosMatrix = getAffineTransform( mSrcTri, mPosTri );
    mNegMatrix = getAffineTransform( mSrcTri, mNegTri );

    // weighting coeff calculation
    cv::warpAffine(mCtrImg, mPosImg, mPosMatrix, mPosImg.size());
    cv::warpAffine(mCtrImg, mNegImg, mNegMatrix, mNegImg.size());

    // erode and blur to smooth out boundaries
    cv::Mat erElement = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(crossWidth-2, crossWidth-2));
    cv::erode(mPosImg, mWeightPos, erElement);
    cv::erode(mNegImg, mWeightNeg, erElement);
    cv::blur(mWeightPos, mWeightPos, cv::Size(crossWidth, crossWidth));
    cv::blur(mWeightNeg, mWeightNeg, cv::Size(crossWidth, crossWidth));

    mWeightPos = mWeightPos / 3.0f;
    mWeightNeg = mWeightNeg / 3.0f;
    mWeightCtr = 1.0f - (mWeightPos + mWeightNeg);

    return THOR_OK;
}

ThorStatus SpatialCompoundProcess::process(uint8_t *inputPtr, uint8_t *outputPtr)
{
    // input header pointer
    CineBuffer::CineFrameHeader* inputHeaderPtr =
            reinterpret_cast<CineBuffer::CineFrameHeader*>(inputPtr - sizeof(CineBuffer::CineFrameHeader));

    uint32_t frameNum = inputHeaderPtr->frameNum;

    // input & output in uint8
    cv::Mat input_8u(mParams.numRays, mParams.numSamples, CV_8U, inputPtr);
    cv::Mat output_8u(mParams.numRays, mParams.numSamples, CV_8U, outputPtr);

    // bypass
    if (mBypassCompounding)
    {
        input_8u.copyTo(output_8u);
        return THOR_OK;
    }

    if (frameNum == 0)
    {
        input_8u.convertTo(mInOutImg, CV_32F);
        mPosImg = mInOutImg.mul(mWeightPos);
        mNegImg = mInOutImg.mul(mWeightNeg);
    }

    // 0, neg, post (in this order)
    switch (frameNum%3)
    {
        case 1:
            // negative steer
            input_8u.convertTo(mInOutImg, CV_32F);  // converted input
            cv::warpAffine(mInOutImg, mNegImg2, mNegMatrix, mNegImg.size());
            mNegImg = mNegImg2.mul(mWeightNeg);
            break;

        case 2:
            // positive steer
            input_8u.convertTo(mInOutImg, CV_32F);  // converted input
            cv::warpAffine(mInOutImg, mPosImg2, mPosMatrix, mPosImg.size());
            mPosImg = mPosImg2.mul(mWeightPos);
            break;

        default:
            // 0 -> no steering
            input_8u.convertTo(mCtrImg2, CV_32F);   // converted input
            mCtrImg = mCtrImg2.mul(mWeightCtr);
            break;
    }

    // combined with weighing
    mInOutImg = mCtrImg + mPosImg + mNegImg;

    // MaxImg
    mMaxImg = cv::max(cv::max(mCtrImg2, mNegImg2), mPosImg2);

    // Blending
    if (mBEdgeFilterParams.scBlendMode==1)
        cv::addWeighted(mMaxImg, mBEdgeFilterParams.scBlendWeight, mInOutImg,
                1-mBEdgeFilterParams.scBlendWeight, 0, mInOutImg);

    // convert output to uint8
    mInOutImg.convertTo(output_8u, CV_8U);

    return THOR_OK;
}

void SpatialCompoundProcess::terminate()
{}
