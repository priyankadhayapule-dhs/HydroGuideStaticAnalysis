//
// Copyright 2017 EchoNous Inc.
//
//
#define LOG_TAG "DauColorflow"


#include <Colorflow.h>
#include <ThorConstants.h>
#include <ThorError.h>
#include <ThorUtils.h>
#include <DauInputManager.h>
#include "opencv2/imgproc.hpp"

//---------------------------------------------------------------------------------
Colorflow::Colorflow()
{
    Size sz;

    
    sz.width = MAX_COLOR_SAMPLES_PER_LINE;
    sz.height = MAX_COLOR_LINES_PER_FRAME;
    
    mInput.create(sz, CV_32FC4);

    mN.create( sz, CV_32F );
    mD.create( sz, CV_32F );
    mP.create( sz, CV_32F );
    mU.create( sz, CV_32F );

    mNs.create( sz, CV_32F );
    mDs.create( sz, CV_32F );
    mPs.create( sz, CV_32F );
    mUs.create( sz, CV_32F );

    mPostDetFilterBuf.create( sz, CV_32F );
    
    mPowerThreshMask.create       ( sz, CV_32F );
    mClutterPowerThreshMask.create( sz, CV_32F );
    
    mPowerDb.create       ( sz, CV_32F );
    mClutterPowerDb.create( sz, CV_32F );

    mPhaseEst.create( sz, CV_32F );
    mPrevPhaseEst.create( sz, CV_32F );

    mPowerCompressEst.create( sz, CV_32F );
    mPowerCompress.create( sz, CV_32F );
}

//---------------------------------------------------------------------------------
void Colorflow::setParams(const Colorflow::Params &params)
{
    mParams = params;
}

//---------------------------------------------------------------------------------
void Colorflow::setPowerThreshold(float thresh)
{
    float oldVal = mParams.threshold.power;
    mParams.threshold.power = thresh;
}

void Colorflow::calculateSmoothingCoefficient(float lateralCoeff[], float axialCoeff[],float lateralCoeff2[], float axialCoeff2[])
{
    uint32_t lateralSizeVel = mParams.smoothing.lateralSizeVel;
    uint32_t axialSizeVel = mParams.smoothing.axialSizeVel;
    float lateralSigmaVel = mParams.smoothing.lateralSigmaVel;
    float axialSigmaVel = mParams.smoothing.axialSigmaVel;
    uint32_t mode = mParams.smoothing.mode;
    uint32_t lateralSizeVel2 = mParams.smoothing.lateralSizeVel2;
    uint32_t axialSizeVel2 = mParams.smoothing.axialSizeVel2;
    float lateralSigmaVel2 = mParams.smoothing.lateralSigmaVel2;
    float axialSigmaVel2 = mParams.smoothing.axialSigmaVel2;
    uint32_t mode2 = mParams.smoothing.mode2;
    if (lateralSizeVel > 9)
        lateralSizeVel = 9;
    if (axialSizeVel > 9)
        axialSizeVel = 9;
    // calculate lateral coeffs
    if ( (lateralSizeVel & 0x1) && (lateralSizeVel >= 3) ) // filter size must be odd
    {
        if (mode == 2)
        {
            Mat lateralCoeffMat = cv::getGaussianKernel(lateralSizeVel, lateralSigmaVel, CV_32F);
            //lateralCoeff[0] = 0.0;
            for (uint32_t i = 0; i < lateralSizeVel; i++)
            {
                lateralCoeff[i] = lateralCoeffMat.at<float>(i);
            }
        }
        else
        {
            for (uint32_t i = 0; i < lateralSizeVel; i++)
            {
                lateralCoeff[i] =1.0f / lateralSizeVel;
            }
        }
    }

    // calculate axial coeffs
    if (((axialSizeVel & 0x1) == 1) && (axialSizeVel >= 3))
    {
        if (mode == 2)
        {
            Mat axialCoeffMat = cv::getGaussianKernel(axialSizeVel, axialSigmaVel, CV_32F);
            //axialCoeff[0] = 0.0;
            for (uint32_t i = 0; i < axialSizeVel; i++)
            {
                axialCoeff[i] = axialCoeffMat.at<float>(i);
            }
        }
        else{
            for (uint32_t i = 0; i < axialSizeVel; i++)
            {
                axialCoeff[i] = 1.0f / axialSizeVel;
            }
        }
    }

    if (lateralSizeVel2 > 9)
        lateralSizeVel2 = 9;
    if (axialSizeVel2 > 9)
        axialSizeVel2 = 9;
    // calculate lateral coeffs
    if ( (lateralSizeVel2 & 0x1) && (lateralSizeVel2 >= 3) ) // filter size must be odd
    {
        if (mode2 == 2)
        {
            Mat lateralCoeffMat2 = cv::getGaussianKernel(lateralSizeVel2, lateralSigmaVel2, CV_32F);
            //lateralCoeff2[0] = 0.0;
            for (uint32_t i = 0; i < lateralSizeVel2; i++)
            {
                lateralCoeff2[i] = lateralCoeffMat2.at<float>(i);
            }
        }
        else
        {
            for (uint32_t i = 0; i < lateralSizeVel2; i++)
            {
                lateralCoeff2[i] =1.0f / lateralSizeVel2;
            }
        }
    }

    // calculate axial coeffs
    if (((axialSizeVel2 & 0x1) == 1) && (axialSizeVel2 >= 3))
    {
        if (mode2 == 2)
        {
            Mat axialCoeffMat2 = cv::getGaussianKernel(axialSizeVel2, axialSigmaVel2, CV_32F);
            //axialCoeff2[0] = 0.0;
            for (uint32_t i = 0; i < axialSizeVel2; i++)
            {
                axialCoeff2[i] = axialCoeffMat2.at<float>(i);
            }
        }
        else  {
            for (uint32_t i = 0; i < axialSizeVel2; i++)
            {
                axialCoeff2[i] = 1.0f / axialSizeVel2;
            }
        }
    }
}

//---------------------------------------------------------------------------------
void Colorflow::injectTestSignal( int count)
{
    float nval, dval;

    float* nPtr = (float *)mN.data;
    float* dPtr = (float *)mD.data;
    float* pPtr = (float *)mP.data;
    float* uPtr = (float *)mU.data;

    if ( (count >= 0) && (count < 32) )
    {
        nval = count/32.0;
        dval = 1.0;
    }
    else if ( (count >= 32) && (count < 64) )
    {
        nval = 1.0;
        dval = (64 - count)/32.0;
    }
    else if ( (count >= 64) && (count < 96) )
    {
        nval = 1.0;
        dval = (64 - count)/32.0;
    }
    else if ( (count >= 96) && (count < 128) )
    {
        nval = (128-count)/32.0;
        dval = -1.0;
    }
    else if ( (count >= 128) && (count < 160) )
    {
        nval = (128 - count)/32.0;
        dval = -1.0;
    }
    else if ( (count >= 160) && (count < 192) )
    {
        nval = -1.0;
        dval = (count -192)/32.0;
    }
    else if ( (count >= 192) && (count < 223) )
    {
        nval = -1.0;
        dval = (count - 192)/32.0;
    }
    else
    {
        nval = (count - 256)/32.0;
        dval = 1.0;
    }
    const float k = 8192.0; // need this multiplier to fool the thresholding
    nval = k*nval;
    dval = k*dval;

    for (uint32_t lineIndex = 0; lineIndex < mNumLines; lineIndex++)
    {
        for (uint32_t sampleIndex = 0; sampleIndex < mNumSamples; sampleIndex++)
        {
            *nPtr++ = nval;
            *dPtr++ = dval;
            *pPtr++ = k;
            *uPtr++ = k;
        }
    }
}

//---------------------------------------------------------------------------------
void Colorflow::logParams()
{
    LOGI("axialSizeND = %d, lateralSizeND = %d, axialSizeP = %d, lateralSizeP=%d",
         mParams.autoCorr.axialSizeND,
         mParams.autoCorr.lateralSizeND,
         mParams.autoCorr.axialSizeP,
         mParams.autoCorr.lateralSizeP);

    LOGI("axialSigmaND = %f, lateralSigmaND = %f",
         mParams.autoCorr.axialSigmaND,
         mParams.autoCorr.lateralSigmaND );

    LOGI("acFilterMode = %d", mParams.autoCorr.acFilterMode);
    Mat axialKernelND = cv::getGaussianKernel(mParams.autoCorr.axialSizeND,
                                              mParams.autoCorr.axialSigmaND,
                                              CV_32F);
    Mat lateralKernelND = cv::getGaussianKernel(mParams.autoCorr.lateralSizeND,
                                                mParams.autoCorr.lateralSigmaND,
                                                CV_32F);
    Mat axialKernelP = cv::getGaussianKernel(mParams.autoCorr.axialSizeP,
                                              mParams.autoCorr.axialSigmaP,
                                              CV_32F);
    Mat lateralKernelP = cv::getGaussianKernel(mParams.autoCorr.lateralSizeP,
                                                mParams.autoCorr.lateralSigmaP,
                                                CV_32F);
    for (int i = 0; i < axialKernelND.rows; i++)
        LOGI("axialKernelND(%d,1) = %f", i, axialKernelND.at<float>(i,0));
    for (int i = 0; i < lateralKernelND.rows; i++)
        LOGI("lateralKernelND(%d,1) = %f", i, lateralKernelND.at<float>(i,0));
    for (int i = 0; i < axialKernelP.rows; i++)
        LOGI("axialKernelP(%d,1) = %f", i, axialKernelP.at<float>(i,0));
    for (int i = 0; i < lateralKernelP.rows; i++)
        LOGI("lateralKernelP(%d,1) = %f", i, lateralKernelP.at<float>(i,0));

    LOGI("laterSizeVel = %d, axialSizeVel = %d, lateralSizeP = %d, axialSizeP = %d",
            mParams.smoothing.lateralSizeVel,
            mParams.smoothing.axialSizeVel,
            mParams.smoothing.lateralSizeP,
            mParams.smoothing.axialSizeP);
}

//---------------------------------------------------------------------------------
void Colorflow::separateOutNDPU( float *inputPtr, int32_t count)
{
    float *nPtr = (float *)(mN.data);
    float *dPtr = (float *)(mD.data);
    float *pPtr = (float *)(mP.data);
    float *uPtr = (float *)(mU.data);

#if 1
    UNUSED(count);
    for (uint32_t lineIndex = 0; lineIndex < mNumLines; lineIndex++)
    {
        for (uint32_t sampleIndex = 0; sampleIndex < mNumSamples; sampleIndex++)
        {
            *nPtr++ = *inputPtr++;
            *dPtr++ = *inputPtr++;
            *pPtr++ = *inputPtr++;
            *uPtr++ = *inputPtr++;
        }
    }
#else
    injectTestSignal(count%256);
#endif
}

//----------------------------------------------------------------------------------
ThorStatus Colorflow::preprocessInput(uint32_t frameCount)
{
    UNUSED(frameCount);

    ThorStatus st = THOR_OK;
    
    Mat croppedN = Mat(mNumLines, mNumSamples, CV_32F, mN.data);
    Mat croppedD = Mat(mNumLines, mNumSamples, CV_32F, mD.data);
    Mat croppedP = Mat(mNumLines, mNumSamples, CV_32F, mP.data);
    Mat croppedU = Mat(mNumLines, mNumSamples, CV_32F, mU.data);

    Mat smoothedN = Mat(mNumLines, mNumSamples, CV_32F, mNs.data);
    Mat smoothedD = Mat(mNumLines, mNumSamples, CV_32F, mDs.data);
    Mat smoothedP = Mat(mNumLines, mNumSamples, CV_32F, mPs.data);
    Mat smoothedU = Mat(mNumLines, mNumSamples, CV_32F, mUs.data);

    Size szND;
    Size szP;

    szND.height = mParams.autoCorr.lateralSizeND;
    szND.width  = mParams.autoCorr.axialSizeND;
    szP.height  = mParams.autoCorr.lateralSizeP;
    szP.width   = mParams.autoCorr.axialSizeP;

    switch(mParams.autoCorr.acFilterMode)
    {
    case 0: // blur
        cv::boxFilter( croppedN, smoothedN, -1, szND, Point(-1,-1), true, BORDER_REPLICATE );
        cv::boxFilter( croppedD, smoothedD, -1, szND, Point(-1,-1), true, BORDER_REPLICATE );
        cv::boxFilter( croppedP, smoothedP, -1, szP,  Point(-1,-1), true, BORDER_REPLICATE );
        croppedU.copyTo( smoothedU );
        break;

    case 1: // Gaussian blur
        cv::GaussianBlur( croppedN,
                          smoothedN,
                          szND,
                          mParams.autoCorr.axialSigmaND,
                          mParams.autoCorr.lateralSigmaND,
                          BORDER_REPLICATE);
        cv::GaussianBlur( croppedD,
                          smoothedD,
                          szND,
                          mParams.autoCorr.axialSigmaND,
                          mParams.autoCorr.lateralSigmaND,
                          BORDER_REPLICATE);
        cv::GaussianBlur( croppedP,
                          smoothedP,
                          szP,
                          mParams.autoCorr.axialSigmaP,
                          mParams.autoCorr.lateralSigmaP,
                          BORDER_REPLICATE);
        croppedU.copyTo( smoothedU );
        break;

    default: // passthrough
        croppedN.copyTo( smoothedN );
        croppedD.copyTo( smoothedD );
        croppedP.copyTo( smoothedP );
        croppedU.copyTo( smoothedU );
        break;
    }

    Mat croppedPowerDb = Mat(mNumLines, mNumSamples, CV_32F, mPowerDb.data);
    Mat croppedClutterPowerDb = Mat(mNumLines, mNumSamples, CV_32F, mClutterPowerDb.data);

    smoothedP += 1.0e-20;  // add epsilon to prevent having to compute log(0)
    smoothedU += 1.0e-20;
    cv::log(smoothedP, croppedPowerDb);
    cv::log(smoothedU, croppedClutterPowerDb);

    croppedPowerDb        *= 10.0/2.303;
    croppedClutterPowerDb *= 10.0/2.303;
    double minValClutter;
    double maxValClutter;
    double minValPower;
    double maxValPower;
    float clutterMaxRatio;
    float sumClutter;
    float meanClutter;
    float sumPower;
    float meanPower;
    float clutterMeanRatio;
    float clutterThresh;
    Point minLoc;
    Point maxLoc;
    cv::minMaxLoc( croppedClutterPowerDb, &minValClutter, &maxValClutter, &minLoc, &maxLoc );
    cv::minMaxLoc( croppedPowerDb, &minValPower, &maxValPower, &minLoc, &maxLoc );
    clutterMaxRatio = maxValClutter/maxValPower;
    sumClutter = cv::sum( croppedClutterPowerDb )[0];
    meanClutter = sumClutter/(mNumSamples*mNumLines);
    sumPower = cv::sum( croppedPowerDb )[0];
    meanPower = sumPower/(mNumSamples*mNumLines);
    clutterMeanRatio = meanClutter/meanPower;
    clutterThresh = mParams.threshold.clutter;
    clutterThresh = mParams.threshold.clutterFactor*maxValClutter;
     // Clamp power values to powMax
    cv::threshold(croppedPowerDb, croppedPowerDb,
                  mParams.threshold.powMax,
                  mParams.threshold.powMax,
                  THRESH_TRUNC);
    /*
     * Create a mask to discard velocity values at locations
     * where the filtered power is less than power threshold
     */
    Mat powerThreshMask = Mat(mNumLines, mNumSamples, CV_32F, mPowerThreshMask.data);
    cv::threshold(croppedPowerDb, powerThreshMask, mParams.threshold.power, 1.0, cv::THRESH_BINARY);

    Mat clutterPowerThreshMask = Mat(mNumLines, mNumSamples, CV_32F, mClutterPowerThreshMask.data);
    cv::threshold(croppedClutterPowerDb, clutterPowerThreshMask, clutterThresh, 1.0, cv::THRESH_BINARY_INV);

    cv::multiply( powerThreshMask, clutterPowerThreshMask, powerThreshMask);

    cv::multiply(croppedPowerDb, powerThreshMask, croppedPowerDb);

    // for CPD Only
    if (mParams.colorMode == COLOR_MODE_CPD)
    {
        Mat croppedPowerCompress = Mat(mNumLines, mNumSamples, CV_32F, mPowerCompress.data);

        croppedPowerCompress = 255.0f * ( croppedPowerDb / mParams.cpd.cpdCompressDynamicRangeDb -
                mParams.cpd.cpdCompressPivotPtIn + mParams.cpd.cpdCompressPivotPtOut );
    }
    return (st);
}

//----------------------------------------------------------------------------------
ThorStatus Colorflow::persistPhase( Mat& currentFrame, Mat& prevFrame, float alpha)
{
    ThorStatus st = THOR_OK;
    float filteredVal;
    float omalpha = 1.0 - alpha;
    float twopi = 2.0*CV_PI;

    if (mParams.colorMode == COLOR_MODE_CPD)
    {
        for (int row = 0; row < currentFrame.rows; row++)
        {
            float *curPtr = (float *)currentFrame.ptr(row);
            float *prevPtr = (float *)prevFrame.ptr(row);
            for (int col = 0; col < currentFrame.cols; col++)
            {
                float curVal = *curPtr;
                float prevVal = *prevPtr;

                // Apply first order IIR filter
                filteredVal = alpha*prevVal + omalpha*curVal;
                *prevPtr = filteredVal;
                curPtr++;
                prevPtr++;
            }
        }
    } else
    {
    for (int row = 0; row < currentFrame.rows; row++)
    {
        float *curPtr = (float *)currentFrame.ptr(row);
        float *prevPtr = (float *)prevFrame.ptr(row);
        for (int col = 0; col < currentFrame.cols; col++)
        {
            float curVal = *curPtr;
            float prevVal = *prevPtr;

            // Take care of circular interpolation
            if ( (prevVal - curVal) > CV_PI )
            {
                prevVal = prevVal - twopi;
            }
            else if ((curVal - prevVal) > CV_PI)
            {
                curVal = curVal - twopi;
            }

            // Apply first order IIR filter
            filteredVal = alpha*prevVal + omalpha*curVal;

            // Keep the filtered phase in [0, 2pi]
            if (filteredVal < 0.0)
            {
                filteredVal += twopi;
            }

            *prevPtr = filteredVal;
            curPtr++;
            prevPtr++;
        }
    }
        }
    return( st );
}

//---------------------------------------------------------------------------------
void Colorflow::injectTestVelocity(Mat& phaseInp)
{
    float velVal;
#if 0
    for (uint32_t lineIndex = 0; lineIndex < mNumLines/2-4; lineIndex++)
    {
        float *curPtr = (float *)phaseInp.ptr(lineIndex);
        for (uint32_t sampleIndex = 0; sampleIndex < mNumSamples/2; sampleIndex++)
        {

            velVal = 2*CV_PI-3*CV_PI/8;
            *curPtr++ = velVal;
        }
        for (uint32_t sampleIndex = mNumSamples/2; sampleIndex < mNumSamples; sampleIndex++)
        {

            velVal = 2*CV_PI-7*CV_PI/8;
            *curPtr++ = velVal;
        }
    }
    for (uint32_t lineIndex = mNumLines/2-4; lineIndex < mNumLines/2; lineIndex++)
    {
        float *curPtr = (float *)phaseInp.ptr(lineIndex);
        for (uint32_t sampleIndex = 0; sampleIndex < mNumSamples; sampleIndex++)
        {

            velVal = 2*CV_PI;
            *curPtr++ = velVal;
        }
    }
    for (uint32_t lineIndex = mNumLines/2; lineIndex < mNumLines/2+3; lineIndex++)
    {
        float *curPtr = (float *)phaseInp.ptr(lineIndex);
        for (uint32_t sampleIndex = 0; sampleIndex < mNumSamples; sampleIndex++)
        {

            velVal = 0.0;
            *curPtr++ = velVal;
        }
    }

    for (uint32_t lineIndex = mNumLines/2+1; lineIndex < mNumLines/2+2; lineIndex++)
    {
        float *curPtr = (float *)phaseInp.ptr(lineIndex);
        for (uint32_t sampleIndex = 0; sampleIndex < mNumSamples/2; sampleIndex++)
        {

            velVal = 0.0;
            *curPtr++ = velVal;
        }
        for (uint32_t sampleIndex = (7+mNumSamples/2); sampleIndex < (8+mNumSamples/2); sampleIndex++)
        {

            velVal = 1.0;
            *curPtr++ = velVal;
        }
        for (uint32_t sampleIndex = (19+mNumSamples/2); sampleIndex < (20+mNumSamples/2); sampleIndex++)
        {

            velVal = 2.0;
            *curPtr++ = velVal;
        }
    }

    for (uint32_t lineIndex = mNumLines/2+3; lineIndex < mNumLines/2+4; lineIndex++)
    {
        float *curPtr = (float *)phaseInp.ptr(lineIndex);
        for (uint32_t sampleIndex = 0; sampleIndex < mNumSamples/2; sampleIndex++)
        {

            velVal = 2*CV_PI;
            *curPtr++ = velVal;
        }
        for (uint32_t sampleIndex = (10+mNumSamples/2); sampleIndex < (11+mNumSamples/2); sampleIndex++)
        {

            velVal = 4.0;
            *curPtr++ = velVal;
        }
    }

    for (uint32_t lineIndex = mNumLines/2+4; lineIndex < mNumLines/2+10; lineIndex++)
    {
        float *curPtr = (float *)phaseInp.ptr(lineIndex);
        for (uint32_t sampleIndex = 0; sampleIndex < mNumSamples; sampleIndex++)
        {

            velVal = 0.0;
            *curPtr++ = velVal;
        }
    }
    for (uint32_t lineIndex = mNumLines/2+10; lineIndex < mNumLines; lineIndex++)
    {
        float *curPtr = (float *)phaseInp.ptr(lineIndex);
        for (uint32_t sampleIndex = 0; sampleIndex < mNumSamples/2; sampleIndex++)
        {

            velVal = 3*CV_PI/8;
            *curPtr++ = velVal;
        }
        for (uint32_t sampleIndex = mNumSamples/2; sampleIndex < mNumSamples; sampleIndex++)
        {

            velVal = 7*CV_PI/8;
            *curPtr++ = velVal;
        }
    }
#endif
#if 1
    for (uint32_t lineIndex = 0; lineIndex < mNumLines/3; lineIndex++)
    {
        float *curPtr = (float *)phaseInp.ptr(lineIndex);
        for (uint32_t sampleIndex = 0; sampleIndex < mNumSamples/8; sampleIndex++)
        {

            velVal = CV_PI+2.5;
            *curPtr++ = velVal;
        }
        for (uint32_t sampleIndex = mNumSamples/8; sampleIndex < mNumSamples/4; sampleIndex++)
        {

            velVal = CV_PI;
            *curPtr++ = velVal;
        }
        for (uint32_t sampleIndex = mNumSamples/4; sampleIndex < mNumSamples/2; sampleIndex++)
        {

            velVal = CV_PI+2.5;
            *curPtr++ = velVal;
        }
        for (uint32_t sampleIndex = mNumSamples/2; sampleIndex < 3*mNumSamples/4; sampleIndex++)
        {

            velVal = 0.7;
            *curPtr++ = velVal;
        }
        for (uint32_t sampleIndex = 3*mNumSamples/4; sampleIndex < mNumSamples; sampleIndex++)
        {

            velVal = 3.1;
            *curPtr++ = velVal;
        }
    }
    for (uint32_t lineIndex = mNumLines/3; lineIndex < 2*mNumLines/3; lineIndex++)
    {
        float *curPtr = (float *)phaseInp.ptr(lineIndex);
        for (uint32_t sampleIndex = 0; sampleIndex < mNumSamples/8; sampleIndex++)
        {

            velVal = CV_PI+1.5;
            *curPtr++ = velVal;
        }
        for (uint32_t sampleIndex = mNumSamples/8; sampleIndex < mNumSamples/4; sampleIndex++)
        {

            velVal = CV_PI+0.5;
            *curPtr++ = velVal;
        }
        for (uint32_t sampleIndex = mNumSamples/4; sampleIndex < mNumSamples/2; sampleIndex++)
        {

            velVal = CV_PI+1.5;
            *curPtr++ = velVal;
        }
        for (uint32_t sampleIndex = mNumSamples/2; sampleIndex < 3*mNumSamples/4; sampleIndex++)
        {

            velVal = 1.0;
            *curPtr++ = velVal;
        }
        for (uint32_t sampleIndex = 3*mNumSamples/4; sampleIndex < mNumSamples; sampleIndex++)
        {

            velVal = 2.1;
            *curPtr++ = velVal;
        }
    }
    for (uint32_t lineIndex = 2*mNumLines/3; lineIndex < mNumLines; lineIndex++)
    {
        float *curPtr = (float *)phaseInp.ptr(lineIndex);
        for (uint32_t sampleIndex = 0; sampleIndex < mNumSamples/8; sampleIndex++)
        {

            velVal = 3.1;
            *curPtr++ = velVal;
        }
        for (uint32_t sampleIndex = mNumSamples/8; sampleIndex < mNumSamples/4; sampleIndex++)
        {

            //velVal = 0.7;
            velVal = 2.0;
            *curPtr++ = velVal;
        }
        for (uint32_t sampleIndex = mNumSamples/4; sampleIndex < mNumSamples/2; sampleIndex++)
        {

            velVal = CV_PI+2.5;
            *curPtr++ = velVal;
        }
        for (uint32_t sampleIndex = mNumSamples/2; sampleIndex < 3*mNumSamples/4; sampleIndex++)
        {

            velVal = CV_PI;
            *curPtr++ = velVal;
        }
        for (uint32_t sampleIndex = 3*mNumSamples/4; sampleIndex < mNumSamples; sampleIndex++)
        {
            velVal = CV_PI+2.5;
            *curPtr++ = velVal;
        }
    }
#endif
}

//----------------------------------------------------------------------------------
ThorStatus Colorflow::estimatePhase(uint32_t frameCount) {
    ThorStatus st = THOR_OK;

    Mat smoothedN = Mat(mNumLines, mNumSamples, CV_32F, mNs.data);
    Mat smoothedD = Mat(mNumLines, mNumSamples, CV_32F, mDs.data);

    Mat phaseEst = Mat(mNumLines, mNumSamples, CV_32F, mPhaseEst.data);
    Mat prevPhaseEst = Mat(mNumLines, mNumSamples, CV_32F, mPrevPhaseEst.data);

    Mat powerCompress = Mat(mNumLines, mNumSamples, CV_32F, mPowerCompress.data);
    Mat powerCompressEst = Mat(mNumLines, mNumSamples, CV_32F, mPowerCompressEst.data);


    try {
        float alpha = mParams.persistence.vel;

        if (mParams.colorMode == COLOR_MODE_CPD)
        {
            applyNoiseFilter(powerCompress);

            applyPostDetectionFilter(powerCompress);

            if (0 == frameCount)
            {
                powerCompress.copyTo(powerCompressEst);
            }
            persistPhase(powerCompress, powerCompressEst, alpha);

            applyNoiseFilter(powerCompressEst);
        }
        else {
            // Estimate phase
            cv::phase(smoothedN, smoothedD, phaseEst);

            /*
             * Apply power threshold mask to velocity estimates
             */
            Mat powerThreshMask = Mat(mNumLines, mNumSamples, CV_32F, mPowerThreshMask.data);
            for (uint32_t si = 0; si < mNumSamples; si++) {
                for (uint32_t li = 0; li < mNumLines; li++) {
                    if ((phaseEst.at<float>(li, si) < (float) CV_PI) &&
                        (powerThreshMask.at<float>(li, si) == 0.0f)) {
                        phaseEst.at<float>(li, si) = 0.0f;
                    } else if ((phaseEst.at<float>(li, si) >= (float) CV_PI) &&
                               (powerThreshMask.at<float>(li, si) == 0.0f)) {
                        phaseEst.at<float>(li, si) = (float) 2 * CV_PI;
                    }
                }
            }
            //cv::multiply(powerThreshMask, phaseEst, phaseEst);

#if 0
            injectTestVelocity(phaseEst);
#endif
            applyNoiseFilter(phaseEst);

            applyPostDetectionFilter(phaseEst);

            if (0 == frameCount) {
                phaseEst.copyTo(prevPhaseEst);
            }
            persistPhase(phaseEst, prevPhaseEst, alpha);

            applyNoiseFilter(prevPhaseEst);
        }

    }
    catch( cv::Exception& e )
    {
        const char* err_msg = e.what();
        LOGE("Exception caught in Colorflow::estimatePhase() %s", err_msg);
        st = THOR_ERROR;
    }
    return( st );

}

//----------------------------------------------------------------------------------
ThorStatus Colorflow::applyNoiseFilter(Mat& phaseInp)
{
    ThorStatus st = THOR_OK;
    uint32_t filterSize = mParams.noisefilter.filtersize;
    float threshold = mParams.noisefilter.threshold;
    uint32_t noise_count = floor(filterSize * filterSize * threshold);

    Size szFilterSize;
    szFilterSize.height = filterSize;
    szFilterSize.width  = filterSize;

    if (filterSize > 1)
    {
        Mat count = Mat(mNumLines, mNumSamples, CV_32F, Scalar(1));
        Mat mask = Mat(mNumLines, mNumSamples, CV_32F, Scalar(0));
        Mat mask2 = Mat(mNumLines, mNumSamples, CV_32F, Scalar(0));
        cv::threshold(abs(phaseInp), mask, 0, 1.0, cv::THRESH_BINARY);
        cv::boxFilter(mask, count, -1, szFilterSize, Point(-1, -1), false, BORDER_REPLICATE);
        cv::threshold(count, mask2, noise_count, 1, cv::THRESH_BINARY);
        cv::multiply(phaseInp, mask2, phaseInp);
    }
    return (st);
}

//----------------------------------------------------------------------------------
ThorStatus Colorflow::applyPostDetectionFilter(Mat& phaseInp)
{
    ThorStatus st = THOR_OK;

    uint32_t lateralSizeVel = mParams.smoothing.lateralSizeVel;
    uint32_t axialSizeVel = mParams.smoothing.axialSizeVel;
    int32_t mode = mParams.smoothing.mode;
    int32_t mode2 = mParams.smoothing.mode2;
    float velThresh = mParams.smoothing.velThresh;
    float powThresh = mParams.smoothing.powThresh+mParams.threshold.power;
    uint32_t filterSize;

    Mat croppedPowerDb = Mat(mNumLines, mNumSamples, CV_32F, mPowerDb.data);
    Mat powerThreshMask = Mat(mNumLines, mNumSamples, CV_32F, mPowerThreshMask.data);
    Mat powerThreshDb = Mat(mNumLines, mNumSamples, CV_32F, Scalar(0));
    cv::multiply(croppedPowerDb, powerThreshMask, powerThreshDb);


    // Filter laterally
    filterSize = lateralSizeVel;
    if ( (filterSize & 0x1) && (filterSize >= 3) ) // filter size must be odd
    {
        Mat out = Mat(mNumLines, mNumSamples, CV_32F, mPostDetFilterBuf.data);
        Mat inp = Mat(mNumLines, mNumSamples, CV_32F, phaseInp.data);
        uint32_t h = filterSize >> 1;
        for (uint32_t si = 0; si < mNumSamples; si++)
        {
            for (uint32_t li = 0; li < h; li++)
            {
                out.at<float>(li, si) = inp.at<float>(li, si);
            }

            for (uint32_t li = h; li < mNumLines-h; li++) {
                bool allPos = false;   // true if estimated phase [0, pi]
                bool allNeg = false;   // true if estimated phase [pi, >pi]
                bool allPosSmooth = false;   // true if estimated phase [0, pi]
                bool allNegSmooth = false;   // true if estimated phase [pi, >pi]

                float phase = inp.at<float>(li - h, si);
                float ref,refOrig;

                if ((phase >= (float) CV_PI)) {
                    allNeg = true;
                }
                if (phase < (float) CV_PI) {
                    allPos = true;
                }
                for (uint32_t ci = 0; ci < filterSize; ci++) {
                    phase = inp.at<float>((li - h + ci), si);
                    if (allNeg && ((phase < (float) CV_PI))) {
                        allNeg = false;
                    }
                    if (allPos && (phase >= (float) CV_PI)) {
                        allPos = false;
                    }
                }

                phase = inp.at<float>((li - h), si);
                float power = powerThreshDb.at<float>((li - h), si);
                //phase = phase >  (float) 2.0*CV_PI ? (float) 2.0*CV_PI-0.00001 : phase;
                if (((phase  >= (float) (2*CV_PI-velThresh)) || (power<powThresh)))
                {
                    allNegSmooth = true;
                }
                if (((phase  <= velThresh) || (power<powThresh)))
                {
                    allPosSmooth = true;
                }

                for (uint32_t ci = 0; ci < filterSize; ci++) {
                    phase = inp.at<float>((li - h + ci), si);
                    power = powerThreshDb.at<float>((li - h + ci), si);
                    if (allNegSmooth && (phase  < (float) (2*CV_PI-velThresh)) && (power>=powThresh)) {
                        allNegSmooth = false;
                    }
                    if (allPosSmooth && (phase  >velThresh)&& (power>=powThresh)) {
                        allPosSmooth = false;
                    }
                }

                if (velThresh==0.0)
                {
                    allNegSmooth = true;
                    allPosSmooth = true;
                }
                if (mode >=0 )
                // Smooth only if all samples of size lateralSizeVel around li
                // are of the same polarity.
                {
                    /*
                    if (allNeg || allPos) {
                        float val = 0.0f;
                        float valvec[filterSize];
                        for (uint32_t ci = 0; ci < filterSize; ci++) {
                            valvec[ci] = mLateralCoeff[ci] * inp.at<float>(li - h + ci, si);
                        }
                        for (uint32_t ci = 0; ci < filterSize; ci++) {
                            val += valvec[ci];
                        }
                        out.at<float>(li, si) = val;
                    }

                    else if (allNeg ==false and allPos == false)
                     */
                    if (allNegSmooth || allPosSmooth)
                    {

                        float val = 0.0f;
                        float valvec[filterSize];

                        refOrig = inp.at<float>(li, si);
                        phase = inp.at<float>(li, si);
                        if (phase >= (float) CV_PI)
                        {
                            ref = phase - (float) 2 * CV_PI;
                        }
                        else
                            ref=phase;

                        for (uint32_t ci = 0; ci < filterSize; ci++)
                        {
                            float temp = 0.0f;
                            phase = inp.at<float>((li - h + ci), si);
                            if (phase >= (float) CV_PI)
                            {
                                temp = inp.at<float>((li - h + ci), si) - (float) 2 * CV_PI;
                            }
                            else
                                temp=inp.at<float>((li - h + ci), si);

                            temp = temp - ref;

                            if (temp<-(float) CV_PI)
                                temp = (float) 2 * CV_PI + temp;
                            else if (temp>=(float) CV_PI)
                                temp = temp - (float) 2 * CV_PI;
                            valvec[ci] = mLateralCoeff[ci] * temp;
                        }
                        for (uint32_t ci = 0; ci < filterSize; ci++) {
                            val += valvec[ci];
                        }
                        val = val + refOrig;
                        out.at<float>(li, si) = val;
                    }

                   else
                   {
                       out.at<float>(li, si) = inp.at<float>(li, si);
                   }

                   if (out.at<float>(li, si) < 0)
                        out.at<float>(li, si) = out.at<float>(li, si) + (float) 2 * CV_PI;
                    if (out.at<float>(li, si) > (float) 2 * CV_PI)
                        out.at<float>(li, si) = out.at<float>(li, si) - (float) 2 * CV_PI;

                }
            }

            for (uint32_t li = mNumLines-h; li < mNumLines; li++)
            {
                out.at<float>(li, si) = inp.at<float>(li, si);
            }
        }
    }
    else
    {
        Mat out = Mat(mNumLines, mNumSamples, CV_32F, mPostDetFilterBuf.data);
        Mat inp = Mat(mNumLines, mNumSamples, CV_32F, phaseInp.data);
         for (uint32_t li = 0; li < mNumLines; li++)
         {
            for (uint32_t si = 0; si < mNumSamples; si++)
                out.at<float>(li, si) = inp.at<float>(li, si);
         }
    }

    // Filter axially
    filterSize = axialSizeVel;
    if (((filterSize & 0x1) == 1) && (filterSize >= 3))
    {
        Mat out = Mat(mNumLines, mNumSamples, CV_32F, phaseInp.data);
        Mat inp = Mat(mNumLines, mNumSamples, CV_32F, mPostDetFilterBuf.data);
        uint32_t h = filterSize >> 1;

        for (uint32_t li = 0; li < mNumLines; li++)
        {
            for (uint32_t si = 0; si < h; si++)
            {
                out.at<float>(li, si) = inp.at<float>(li, si);
            }

            for (uint32_t si = h; si < mNumSamples-h; si++)
            {
                bool allPos = false;   // true if estimated phase [0, pi]
                bool allNeg = false;   // true if estimated phase [pi, >pi]
                bool allPosSmooth = false;   // true if estimated phase [0, pi]
                bool allNegSmooth = false;   // true if estimated phase [pi, >pi]
                float ref,refOrig;
                float phase = inp.at<float>(li, si-h);
                if ((phase >= (float)CV_PI))
                {
                    allNeg = true;
                }
                if (phase < (float)CV_PI)
                {
                    allPos = true;
                }
                for ( uint32_t ci = 0; ci < filterSize; ci++)
                {
                    phase = inp.at<float>(li, si-h+ci);
                    if (allNeg && ( (phase < (float)CV_PI)))
                    {
                        allNeg = false;
                    }
                    if (allPos && (phase >= (float)CV_PI))
                    {
                        allPos = false;
                    }
                }
                phase = inp.at<float>(li, si-h);
                float power = powerThreshDb.at<float>(li, si-h);
                //phase = phase >  (float) 2.0*CV_PI ? (float) 2.0*CV_PI-0.00001 : phase;
                if (((phase  >= (float) (2*CV_PI-velThresh)) || (power<powThresh)))
                {
                    allNegSmooth = true;
                }
                if (((phase  <= velThresh) || (power<powThresh)))
                {
                    allPosSmooth = true;
                }

                for (uint32_t ci = 0; ci < filterSize; ci++) {
                    phase = inp.at<float>(li, si - h + ci);
                    power = powerThreshDb.at<float>(li, si - h + ci);
                    if (allNegSmooth && (phase  < (float) (2*CV_PI-velThresh)) && (power>=powThresh)) {
                        allNegSmooth = false;
                    }
                    if (allPosSmooth && (phase  >velThresh)&& (power>=powThresh)) {
                        allPosSmooth = false;
                    }
                }
                if (velThresh==0.0)
                {
                    allNegSmooth = true;
                    allPosSmooth = true;
                }
                if (mode>=0)
                    // Smooth only if all samples of size lateralSizeVel around li
                    // are of the same polarity.
                {
                    /*
                    if (allNeg || allPos) {
                        float val = 0.0f;
                        float valvec[filterSize];
                        for (uint32_t ci = 0; ci < filterSize; ci++) {
                            valvec[ci] = mAxialCoeff[ci] * inp.at<float>(li, si - h + ci);
                        }
                        for (uint32_t ci = 0; ci < filterSize; ci++) {
                            val += valvec[ci];
                        }
                        out.at<float>(li, si) = val;
                    }

                    else if (allNeg ==false and allPos == false)
                     */

                    if (allNegSmooth || allPosSmooth)
                    {
                        float val = 0.0f;
                        float valvec[filterSize];

                        refOrig = inp.at<float>(li, si);
                        phase = inp.at<float>(li, si);
                        if (phase >= (float) CV_PI)
                        {
                            ref = phase - (float) 2 * CV_PI;
                        }
                        else
                            ref=phase;

                        for (uint32_t ci = 0; ci < filterSize; ci++)
                        {
                            float temp = 0.0f;
                            phase = inp.at<float>(li, si - h + ci);
                            if (phase >= (float) CV_PI)
                            {
                                temp = inp.at<float>(li, si - h + ci) - (float) 2 * CV_PI;
                            }
                            else
                                temp=inp.at<float>(li, si - h + ci);

                            temp = temp - ref;

                            if (temp<-(float) CV_PI)
                                temp = (float) 2 * CV_PI + temp;
                            else if (temp>=(float) CV_PI)
                                temp = temp - (float) 2 * CV_PI;


                            valvec[ci] = mAxialCoeff[ci] * temp;
                        }
                        for (uint32_t ci = 0; ci < filterSize; ci++) {
                            val += valvec[ci];
                        }

                        val = val + refOrig;
                        out.at<float>(li, si) = val;
                    }

                    else
                    {
                        out.at<float>(li, si) = inp.at<float>(li, si);
                    }

                    if (out.at<float>(li, si) < 0)
                        out.at<float>(li, si) = out.at<float>(li, si) + (float) 2 * CV_PI;
                    if (out.at<float>(li, si) > (float) 2 * CV_PI)
                        out.at<float>(li, si) = out.at<float>(li, si) - (float) 2 * CV_PI;
                }
            }

            for (uint32_t si = mNumSamples-h; si < mNumSamples; si++)
            {
                out.at<float>(li, si) = inp.at<float>(li, si);
            }
        }
    }
    else
    {
        Mat inp = Mat(mNumLines, mNumSamples, CV_32F, mPostDetFilterBuf.data);
        Mat out = Mat(mNumLines, mNumSamples, CV_32F, phaseInp.data);
        for (uint32_t li = 0; li < mNumLines; li++)
        {
            for (uint32_t si = 0; si < mNumSamples; si++)
                out.at<float>(li, si) = inp.at<float>(li, si);
        }
    }

    return (st);
}

//----------------------------------------------------------------------------------
ThorStatus Colorflow::processFrame(uint8_t* inputBuffer,
                                   uint32_t numLines,
                                   uint32_t numSamples,
                                   uint32_t frameCount,
                                   uint8_t* outputBuffer)
{
    ThorStatus st = THOR_OK;

    mNumLines = numLines;
    mNumSamples = numSamples;

    float *phaseEstPtr = (float *)mPhaseEst.data;
    
    try
    {
        uint8_t* inptr = inputBuffer + sizeof(DauInputManager::FrameHeader);
        separateOutNDPU( (float *)(inptr), frameCount );
        if (0 == frameCount)
        {
            LOGI("Colorflow: numLines = %d, numSamples = %d", numLines, numSamples);
            logParams();
            calculateSmoothingCoefficient(mLateralCoeff, mAxialCoeff, mLateralCoeff2, mAxialCoeff2);
            if (THOR_OK != st) return (st);
        }
        st = preprocessInput(frameCount);
        if (THOR_OK != st)
            return (st);

        // output
        mOutput = Mat( numLines, numSamples, CV_8U, outputBuffer);

        if (mParams.colorMode == COLOR_MODE_CPD)
        {
            // For CPD
            //Mat powCompress = Mat(mNumLines, mNumSamples, CV_32F, mPowerCompress.data);
            //powCompress.convertTo(mOutput, CV_8U, 1.0);
            estimatePhase(frameCount);
            Mat powCompressEst = Mat(mNumLines, mNumSamples, CV_32F, mPowerCompressEst.data);
            powCompressEst.convertTo(mOutput, CV_8U, 1.0);
        }
        else
        {
            // For CVD - default
            estimatePhase(frameCount);
            Mat prevPhaseEst = Mat(mNumLines, mNumSamples, CV_32F, mPrevPhaseEst.data);
            prevPhaseEst.convertTo( mOutput, CV_8U, 255.0/(2.0*CV_PI));
        }
    }
    catch( cv::Exception& e )
    {
        const char* err_msg = e.what();
        LOGE("Exception caught in Colorflow::estimatePhase() %s", err_msg);
        st = THOR_ERROR;
    }
    return (st);
}
