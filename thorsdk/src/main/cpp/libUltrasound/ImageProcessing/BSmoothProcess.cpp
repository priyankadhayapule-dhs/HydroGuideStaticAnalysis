#define LOG_TAG "BSmoothProcess"

#include <BSmoothProcess.h>
#include <opencv2/imgproc.hpp>

#include <ThorUtils.h>

BSmoothProcess::BSmoothProcess(const std::shared_ptr<UpsReader> &ups) :
    ImageProcess(ups)
{}

ThorStatus BSmoothProcess::init()
{
    return THOR_OK;
}

ThorStatus BSmoothProcess::setProcessParams(ImageProcess::ProcessParams &params)
{    
    getUpsReader()->getBSmoothParams(params.imagingCaseId, mBSmoothParams);
    // Sanity check of coeffs?

    /*
    for (auto i=0u; i != mBSmoothParams.aCoeffs.size(); ++i)
        LOGD("aCoeffs[%u] = %f", i, mBSmoothParams.aCoeffs[i]);
    for (auto i=0u; i != mBSmoothParams.bCoeffs.size(); ++i)
        LOGD("bCoeffs[%u] = %f", i, mBSmoothParams.bCoeffs[i]);
    */

    mXs.resize(mBSmoothParams.numLPFtaps);
    mYs.resize(mBSmoothParams.numLPFtaps);

    mParams = params;

    mForward  = cv::Mat(mParams.numRays, mParams.numSamples, CV_32F);
    mBackward = cv::Mat(mParams.numRays, mParams.numSamples, CV_32F);
    mBlended  = cv::Mat(mParams.numRays, mParams.numSamples, CV_32F);

    return THOR_OK;
}

ThorStatus BSmoothProcess::process(uint8_t *inputPtr, uint8_t *outputPtr)
{
    cv::Mat input_8u(mParams.numRays, mParams.numSamples, CV_8U, inputPtr);
    input_8u.convertTo(mInputF, CV_32F);

    // set up signal history buffers for forwards scan
    for (auto &x : mXs)
        x = mInputF.row(0);
    for (auto &y : mYs)
        y = mInputF.row(0);

    for (uint32_t ray=0; ray < mParams.numRays; ++ray)
    {
        // shift x signal history
        for (auto i=mXs.size(); i != 1u; --i)
            mXs[i-1] = mXs[i-2];
        mXs[0] = mInputF.row(ray);

        // shift y signal history
        for (auto i=mYs.size(); i != 1u; --i)
            mYs[i-1] = mYs[i-2];
        mYs[0] = mForward.row(ray);
        mYs[0].setTo(0);

        // scale x * b coeffs
        for (auto i=0u; i != mXs.size(); ++i)
            cv::scaleAdd(mXs[i], mBSmoothParams.bCoeffs[i], mYs[0], mYs[0]);

        // scale y * a coeffs
        for (auto i=1u; i != mYs.size(); ++i)
            cv::scaleAdd(mYs[i], -mBSmoothParams.aCoeffs[i], mYs[0], mYs[0]);

        // final scaling
        mYs[0] /= mBSmoothParams.aCoeffs[0];
    }

    // set up signal history buffers for backwards scan
    for (auto &x : mXs)
        x = mInputF.row(mParams.numRays-1);
    for (auto &y : mYs)
        y = mInputF.row(mParams.numRays-1);

    for (uint32_t ray=mParams.numRays; ray != 0; --ray)
    {
        // shift x signal history
        for (auto i=mXs.size(); i != 1u; --i)
            mXs[i-1] = mXs[i-2];
        mXs[0] = mInputF.row(ray-1);

        // shift y signal history
        for (auto i=mYs.size(); i != 1u; --i)
            mYs[i-1] = mYs[i-2];
        mYs[0] = mBackward.row(ray-1);
        mYs[0].setTo(0);

        // scale x * b coeffs
        for (auto i=0u; i != mXs.size(); ++i)
            cv::scaleAdd(mXs[i], mBSmoothParams.bCoeffs[i], mYs[0], mYs[0]);

        // scale y * a coeffs
        for (auto i=1u; i != mYs.size(); ++i)
            cv::scaleAdd(mYs[i], -mBSmoothParams.aCoeffs[i], mYs[0], mYs[0]);

        // final scaling
        mYs[0] /= mBSmoothParams.aCoeffs[0];
    }

    // blend together
    for (uint32_t ray=0; ray != mParams.numRays; ++ray)
    {
        float blend = static_cast<float>(ray)/(mParams.numRays-1);
        mForward.row(ray) *= blend;
        mBackward.row(ray) *= (1.0f-blend);
    }

    cv::add(mForward, mBackward, mBlended);

    // convert to output
    cv::Mat output_8u(mParams.numRays, mParams.numSamples, CV_8U, outputPtr);
    mBlended.convertTo(output_8u, CV_8U);

    return THOR_OK;
}

void BSmoothProcess::terminate()
{}