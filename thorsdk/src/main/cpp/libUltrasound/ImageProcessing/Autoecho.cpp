//
// Copyright 2017 EchoNous Inc.
//
//
#define LOG_TAG "DauAutoEcho"

#include <Autoecho.h>
#include <ThorConstants.h>
#include <ThorError.h>
#include <ThorUtils.h>
#include "opencv2/imgproc.hpp"

using namespace std;

#if 0
#include <iostream>
#include <fstream>

void writeUCharMatToFile(cv::Mat& m, const char* filename)
{
    ofstream fout(filename);

    if(!fout)
    {
        cout<<"File Not Opened"<<endl;  return;
    }

    for(int i=0; i<m.rows; i++)
    {
        for(int j=0; j<m.cols; j++)
        {
            fout<<m.at<u_char>(i,j);
        }
        //fout<<endl;
    }

    fout.close();
}

void writeFloatMatToFileBinary(cv::Mat& inMat, const char* filename)
{
    FILE* fp = fopen(filename, "wb");
    for (int row = 0; row < inMat.rows; row++)
    {
        float *inPtr = (float *)inMat.ptr(row);
        fwrite(inPtr, inMat.cols, sizeof(float), fp);
    }
    fclose(fp);

    /* when contiguous
    FILE* fp = fopen(filename, "wb");
    fwrite((float*)(m.data), m.cols * m.rows, sizeof(float), fp);
    fclose(fp);
     */
}
#endif

Autoecho::Autoecho()
{
    // maximum size
    Size sz;
    sz.width = MAX_B_SAMPLES_PER_LINE;
    sz.height = MAX_B_LINES_PER_FRAME;

    Size dSz;   //decimated size
    // assuming decimatation >= 4
    dSz.width = MAX_B_SAMPLES_PER_LINE >> 2;
    dSz.height = MAX_B_LINES_PER_FRAME >> 2;

    // initialize cv Mat
    mInput.create(sz, CV_32F);
    mAutoGain.create(sz, CV_32F);

    // decimated Mean/Std
    mSigMeanDec.create(dSz, CV_32F);
    mSigStdDec.create(dSz, CV_32F);
    mNoiseMeanDec.create(dSz, CV_32F);

    // set default userGain value (1.0f)
    mUserGain = 1.0f;
}

Autoecho::~Autoecho()
{

}

ThorStatus Autoecho::setActiveRegion(int numLines, int numSamples)
{
    ThorStatus status = THOR_OK;

    mActiveRegion.x      = 0;
    mActiveRegion.y      = 0;
    mActiveRegion.width  = numSamples;
    mActiveRegion.height = numLines;

    return( status );
}


ThorStatus Autoecho::setDecimatedRegion(int numLines, int numSamples)
{
    ThorStatus status = THOR_OK;

    mDecimatedRegion.x      = 0;
    mDecimatedRegion.y      = 0;
    mDecimatedRegion.width  = numSamples;
    mDecimatedRegion.height = numLines;

    return( status );
}

ThorStatus Autoecho::meanStdDec(Mat inputImg, Mat &outMeanImg, Mat &outStdImg,
                                uint32_t axiDecFactor, uint32_t latDecFactor)
{
    // outMeanImg and outStdImg should have the same dimensions
    ThorStatus status = THOR_OK;

    Rect curRegion;         // current region
    curRegion.width = axiDecFactor;
    curRegion.height = latDecFactor;

    for (int row = 0; row < outMeanImg.rows; row++)
    {
        curRegion.y = latDecFactor * row;

        float *meanPtr = (float *)outMeanImg.ptr(row);
        float *stdPtr = (float *)outStdImg.ptr(row);
        for (int col = 0; col < outMeanImg.cols; col++) {
            // current region
            curRegion.x = axiDecFactor * col;

            Mat curBlock = Mat(inputImg, curRegion);

            Scalar mean;
            Scalar stdv;

            meanStdDev(curBlock, mean, stdv);

            *meanPtr = (float) mean.val[0];
            *stdPtr = (float) stdv.val[0];

            // update pointer
            meanPtr++;
            stdPtr++;
        }
    }

    return status;
}

ThorStatus Autoecho::meanSnrDec(Mat inputImg, Mat &outMeanImg, Mat &outSnrImg, uint32_t axiDecFactor,
                                uint32_t latDecFactor) {
    // outMeanImg and outStdImg should have the same dimensions
    ThorStatus status = THOR_OK;

    Rect curRegion;         // current region
    curRegion.width = axiDecFactor;
    curRegion.height = latDecFactor;

    for (int row = 0; row < outMeanImg.rows; row++)
    {
        curRegion.y = latDecFactor * row;

        float *meanPtr = (float *)outMeanImg.ptr(row);
        float *sndPtr = (float *)outSnrImg.ptr(row);
        for (int col = 0; col < outMeanImg.cols; col++) {
            // current region
            curRegion.x = axiDecFactor * col;

            Mat curBlock = Mat(inputImg, curRegion);

            Scalar mean;
            Scalar stdv;

            meanStdDev(curBlock, mean, stdv);

            *meanPtr = (float) mean.val[0];
            *sndPtr = ((float) mean.val[0])/((float) stdv.val[0]);

            // update pointer
            meanPtr++;
            sndPtr++;
        }
    }

    return status;
}

// calculate speckleMask in floating point
ThorStatus Autoecho::calculateSpeckleMask(Mat sigMeanDec, Mat sigSnrDec, Mat noiseMeanDec,
                                          Mat &speckleMask, float snrThrDb, float noiseThrDb,
                                          float stdMinDb, float stdMaxDb)
{
    // Assuming sigMeanDec. sigSnrDec, noiseMeanDec have all the same dimensions
    ThorStatus st = THOR_OK;

    float snrThrLi = pow(10, snrThrDb/20.0f);
    float noiseThrLi = pow(10, noiseThrDb/20.0f);

    for (int row = 0; row < sigMeanDec.rows; row++)
    {
        float *meanPtr = (float *)sigMeanDec.ptr(row);
        float *snrPtr = (float *)sigSnrDec.ptr(row);
        float *noisePtr = (float *)noiseMeanDec.ptr(row);
        float *specklePtr = (float *)speckleMask.ptr(row);

        for (int col = 0; col < sigMeanDec.cols; col++) {

            float out_val = 0.0f;

            if ((*meanPtr>snrThrLi*(*noisePtr)) & (*snrPtr>stdMinDb) & (*snrPtr<stdMaxDb))
                out_val = 1.0f;
            else if (*meanPtr<noiseThrLi*(*noisePtr))
                out_val = -1.0f;

            *specklePtr = out_val;

            // update pointer
            meanPtr++;
            snrPtr++;
            noisePtr++;
            specklePtr++;
        }
    }

    return st;
}

ThorStatus Autoecho::setNoiseSignal(uint8_t *inputBuffer) {
    ThorStatus status = THOR_OK;

    // calling setNoiseSignal
    status = setNoiseSignal(inputBuffer, mNumSamples, mNumRays, mAxiDecFactor, mLatDecFactor);

    return status;
}

ThorStatus Autoecho::setNoiseSignal(uint8_t *inputBuffer, uint32_t numSamples, uint32_t numRays,
                                    uint32_t axiDecFactor, uint32_t latDecFactor)
{
    ThorStatus st = THOR_OK;

    try {
        // set active Region
        Mat inputImg_U8 = Mat(mNumRays, mNumSamples, CV_8U, inputBuffer);

        // determine decimated width and height
        int decim_width, decim_height;
        decim_width = numSamples / axiDecFactor;
        decim_height = numRays / latDecFactor;
        setDecimatedRegion(decim_height, decim_width);

        mCroppedNoiseMean = Mat(mNoiseMeanDec, mDecimatedRegion);
        Mat sigStdDec = Mat(mSigStdDec, mDecimatedRegion);      // not needed but to meet the function requirements

        // mean & standard deviation
        meanStdDec(inputImg_U8, mCroppedNoiseMean, sigStdDec, axiDecFactor, latDecFactor);
    }
    catch( cv::Exception& e )
    {
        const char* err_msg = e.what();
        LOGE("Exception caught in Autoecho::setNoiseSignal() %s", err_msg);
        st = THOR_ERROR;
    }

    return st;
}

ThorStatus Autoecho::gainEstimation(Mat sigExt, Mat speckleMask, Mat &gainEst,
                                    float targetGaindB, float noiseThrDb)
{
    // Assuming sigExt, speckleMask, gainEst have all the same dimensions
    ThorStatus st = THOR_OK;

    float gain_coeff = pow(10, targetGaindB/20.0f);
    float noiseThrLi = pow(10, noiseThrDb/20.0f);

    for (int row = 0; row < sigExt.rows; row++)
    {
        float *sigExtPtr = (float *)sigExt.ptr(row);
        float *specklePtr = (float *)speckleMask.ptr(row);
        float *gainEstPtr = (float *)gainEst.ptr(row);

        for (int col = 0; col < sigExt.cols; col++) {

            float out_val = 1.0f;

            if ((*specklePtr) > 0.0f & (*sigExtPtr) != 0.0f)
                out_val = gain_coeff/(*sigExtPtr);
            else if ((*specklePtr) < 0.0f)
                out_val = 1.0f/noiseThrLi;

            *gainEstPtr = out_val;

            // update pointer
            sigExtPtr++;
            specklePtr++;
            gainEstPtr++;
        }
    }

    return st;
}

ThorStatus Autoecho::applyThreshold(Mat inGain, Mat &outGain, float minClip, float maxClip)
{
    // Assuming inGain and outGain have all the same dimensions
    ThorStatus st = THOR_OK;

    float minClipCoeff = pow(10, minClip/20.0f);
    float maxClipCoeff = pow(10, maxClip/20.0f);

    for (int row = 0; row < inGain.rows; row++)
    {
        float *inGainPtr = (float *)inGain.ptr(row);
        float *outGainPtr = (float *)outGain.ptr(row);

        for (int col = 0; col < inGain.cols; col++) {

            float out_val = *inGainPtr;

            if (*inGainPtr < minClipCoeff)
                out_val = minClipCoeff;

            if (*inGainPtr > maxClipCoeff)
                out_val = maxClipCoeff;

            *outGainPtr = out_val;

            // update pointer
            inGainPtr++;
            outGainPtr++;
        }
    }

    return st;
}

void Autoecho::setParams(Autoecho::Params params)
{
    mNumSamples          = params.numSamples;
    mNumRays             = params.numRays;
    mAxiDecFactor        = params.axiDecFactor;
    mLatDecFactor        = params.latDecFactor;
    mTargetGainDb        = params.targetGainDb;
    mSnrThresholdDb      = params.snrThrDb;
    mNoiseThresholdDb    = params.noiseThrDb;
    mStdMinDb            = params.minStdDb;
    mStdMaxDb            = params.maxStdDb;
    mMinClipDb           = params.minClipDb;
    mMaxClipDb           = params.maxClipDb;

    mGaussianKernelSize  = params.gaussianKernelSize;
    mGaussainKernelSigma = params.gaussianKernelSigma;
    mDilationType        = params.dilationType;
}

void Autoecho::setUserGain(float userGain) {
    mUserGain = userGain;
}

ThorStatus Autoecho::estimateGain(uint8_t *inputBuffer) {
    ThorStatus st = THOR_OK;

    try {
        // set active Region
        setActiveRegion(mNumRays, mNumSamples);
        // inputImg
        Mat inputImg_U8 = Mat(mNumRays, mNumSamples, CV_8U, inputBuffer);

        // determine decimated width and height
        int decim_width, decim_height;
        decim_width = mNumSamples / mAxiDecFactor;
        decim_height = mNumRays / mLatDecFactor;
        setDecimatedRegion(decim_height, decim_width);

        // sig Mean/Std decimated images
        Mat sigMeanDec = Mat(mSigMeanDec, mDecimatedRegion);
        Mat sigStdDec = Mat(mSigStdDec, mDecimatedRegion);
        Mat sigSnrDec = sigStdDec;

        // mean & snr calculation
        meanSnrDec(inputImg_U8, sigMeanDec, sigSnrDec, mAxiDecFactor, mLatDecFactor);

        // speckleMask
        Mat speckleMask = sigStdDec;
        calculateSpeckleMask(sigMeanDec, sigSnrDec, mCroppedNoiseMean, speckleMask, mSnrThresholdDb, mNoiseThresholdDb, mStdMinDb, mStdMaxDb);

        // get masked signal (by applying the speckle mask)
        Mat sigExt = sigMeanDec;
#if 1
        // no dilation or extrapolation
        multiply(speckleMask, sigMeanDec, sigExt);
#else
        // apply speckleMask (extrapolate) depending on the dilationType
        applySpeckleMask(speckleMask, sigMeanDec, sigExt, mDilationType);
#endif

        // gain Estimation (calculate required gain adjustment)
        Mat gainEst = sigMeanDec;
        gainEstimation(sigExt, speckleMask, gainEst, mTargetGainDb, mNoiseThresholdDb);

        // applyGain and Smoothing
        Mat autoGain = Mat(mAutoGain, mActiveRegion);

        Size agsz;
        agsz.width = mNumSamples;
        agsz.height = mNumRays;

        // resizing
        resize(gainEst, autoGain, agsz);
        GaussianBlur(autoGain, autoGain, Size( mGaussianKernelSize, mGaussianKernelSize), mGaussainKernelSigma );

        // Thresholding
        applyThreshold(autoGain, autoGain, mMinClipDb, mMaxClipDb);
    }
    catch( cv::Exception& e )
    {
        const char* err_msg = e.what();
        LOGE("Exception caught in Autoecho::estimateGain() %s", err_msg);
        st = THOR_ERROR;
    }

    return st;
}

// applying different types of dilations to the speckle mask to generate
ThorStatus Autoecho::applySpeckleMask(Mat speckleMask, Mat sigMeanDec, Mat &sigExt,
                                      uint32_t dilationType) {
    ThorStatus st = THOR_OK;

    int dilation_size = 1;

    Mat kernel = getStructuringElement(MORPH_RECT, Size( 2*dilation_size + 1, 2*dilation_size+1 ),
                                       Point( dilation_size, dilation_size ) );

    Mat dilation_dst;

    switch (dilationType) {
        case 1:
            // dilate mask then mutilply
            dilate(speckleMask, dilation_dst, kernel);
            multiply(dilation_dst, sigMeanDec, sigExt);
            break;
        case 2:
            // multiply then dilate
            multiply(speckleMask, sigMeanDec, dilation_dst);
            dilate(dilation_dst, sigExt, kernel);
            break;
        case 3:
            // closing
            dilate(speckleMask, dilation_dst, kernel);
            erode(dilation_dst, dilation_dst, kernel);
            multiply(dilation_dst, sigMeanDec, sigExt);
            break;
        case 4:
            // opening
            erode(speckleMask, dilation_dst, kernel);
            dilate(dilation_dst, dilation_dst, kernel);
            multiply(dilation_dst, sigMeanDec, sigExt);
            break;
        default:
            // no dilation
            multiply(speckleMask, sigMeanDec, sigExt);
            break;

    }

    return st;
}

// apply estimated Gain Adjustment to the inputBuffer
ThorStatus Autoecho::applyGain(uint8_t *inputBuffer, uint8_t *outputBuffer) {
    ThorStatus st = THOR_OK;

    try {
        // set active Region
        setActiveRegion(mNumRays, mNumSamples);
        // inputImg
        Mat inputImg_U8 = Mat(mNumRays, mNumSamples, CV_8U, inputBuffer);

        Mat inputImg = Mat(mInput, mActiveRegion);
        // convert to floating point and scale it with UserGain
        inputImg_U8.convertTo(inputImg, CV_32F, mUserGain);

        // applyGain
        Mat autoGain = Mat(mAutoGain, mActiveRegion);
        Mat outImg = inputImg;      // reuse of inputImg mat
        multiply(inputImg, autoGain, outImg);

        Mat output_U8 = Mat(mNumRays, mNumSamples, CV_8U, outputBuffer);
        outImg.convertTo(output_U8, CV_8U);
    }
    catch( cv::Exception& e )
    {
        const char* err_msg = e.what();
        LOGE("Exception caught in Autoecho::applyGain() %s", err_msg);
        st = THOR_ERROR;
    }

    return st;
}

// apply estimated Gain Adjustment to the inputBuffer, float 32-bit output
ThorStatus Autoecho::applyGain(uint8_t *inputBuffer, float32_t *outputBuffer) {
    ThorStatus st = THOR_OK;

    try {
        // set active Region
        setActiveRegion(mNumRays, mNumSamples);
        // inputImg
        Mat inputImg_U8 = Mat(mNumRays, mNumSamples, CV_8U, inputBuffer);

        Mat inputImg = Mat(mInput, mActiveRegion);
        // convert to floating point and scale it with UserGain
        inputImg_U8.convertTo(inputImg, CV_32F, mUserGain);

        // applyGain
        Mat autoGain = Mat(mAutoGain, mActiveRegion);
        Mat outImg = Mat(mNumRays, mNumSamples, CV_32F, outputBuffer);
        multiply(inputImg, autoGain, outImg);
    }
    catch( cv::Exception& e )
    {
        const char* err_msg = e.what();
        LOGE("Exception caught in Autoecho::applyGain() - float %s", err_msg);
        st = THOR_ERROR;
    }

    return st;
}
