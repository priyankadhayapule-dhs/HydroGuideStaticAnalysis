//
// Copyright 2017 EchoNous Inc.
//
//
#pragma once
#include <stdint.h>
#include "opencv2/core.hpp"
#include <ThorError.h>


using namespace cv;

class Autoecho
{
public:
    Autoecho();
    ~Autoecho();

    struct Params
    {
        uint32_t    numSamples;
        uint32_t    numRays;
        uint32_t    axiDecFactor;
        uint32_t    latDecFactor;
        float       targetGainDb;
        float       snrThrDb;
        float       noiseThrDb;
        float       minStdDb;
        float       maxStdDb;
        float       minClipDb;
        float       maxClipDb;
        uint32_t    gaussianKernelSize;
        float       gaussianKernelSigma;
        uint32_t    dilationType;
        uint32_t    gainCalcInterval;
    };

    // estimate required Gain adjustment
    ThorStatus estimateGain(uint8_t *inputBuffer);

    // apply the estimated Gain adjustment (by estimateGain) to the input frame
    ThorStatus applyGain(uint8_t *inputBuffer, uint8_t *outputBuffer);

    // apply the estimated Gain adjustment (by estimateGain) to the input frame; output 32-bit float
    ThorStatus applyGain(uint8_t *inputBuffer, float *outputBuffer);

    // adjust overall Gain set by a user.  Default: 1.0f
    void setUserGain(float userGain);

    // set parameters for Autoecho.  Default dilation type: 0 - no dilation
    void setParams(Autoecho::Params params);

    // set noiseSignal (needs noise frame data)
    ThorStatus setNoiseSignal(uint8_t *inputBuffer, uint32_t numSamples, uint32_t numRays, uint32_t axiDecFactor, uint32_t latDecFactor);

    // set noiseSignal (override with default parameters)
    ThorStatus setNoiseSignal(uint8_t *inputBuffer);

private:

    // set active region
    ThorStatus setActiveRegion(int numLines, int numSamples);

    // set decimated region
    ThorStatus setDecimatedRegion(int numLines, int numSamples);

    // calculate mean and standard deviation of blocks in the input frame
    ThorStatus meanStdDec(Mat inputImg, Mat& outMeanImg, Mat& outStdImg, uint32_t axiDecFactor, uint32_t latDecFactor);

    // calculate mean and snr (mean/std) of blocks in the input frame
    ThorStatus meanSnrDec(Mat inputImg, Mat& outMeanImg, Mat& outSnrImg, uint32_t axiDecFactor, uint32_t latDecFactor);

    // calculate speckleMask
    ThorStatus calculateSpeckleMask(Mat sigMeanDec, Mat sigSnrDec, Mat noiseMeanDec, Mat& speckleMask, float snrThrDb, float noiseThrDb, float stdMinDb, float stdMaxDb);

    // estimate required gain adjustment
    ThorStatus gainEstimation(Mat sigExt, Mat speckleMask, Mat& gainEst, float targetGaindB, float noiseThrDb);

    // apply threshold both min and max clips
    ThorStatus applyThreshold(Mat inGain, Mat& outGain, float minClip, float maxClip);

    // apply speckle mask to Mean signal with dilation
    ThorStatus applySpeckleMask(Mat speckleMask, Mat sigMeanDec, Mat& sigExt, uint32_t dilationType);

private:
    Mat mInput;
    Mat mAutoGain;

    Mat mSigMeanDec;
    Mat mSigStdDec;

    Mat mNoiseMeanDec;
    Mat mCroppedNoiseMean;

    Rect mActiveRegion;         // defines the region containing valid samples
    Rect mDecimatedRegion;      // defines the region containing valid decimated samples

    // Autoecho Parameters
    uint32_t mAxiDecFactor;
    uint32_t mLatDecFactor;
    uint32_t mNumSamples;
    uint32_t mNumRays;
    float mTargetGainDb;
    float mSnrThresholdDb;
    float mNoiseThresholdDb;
    float mStdMinDb;
    float mStdMaxDb;
    float mMaxClipDb;
    float mMinClipDb;
    uint32_t mGaussianKernelSize;   // Gaussian Blur Kernel Size (used for both lateral and axial directions).
    float mGaussainKernelSigma;

    uint32_t mDilationType;

    // userGain
    float mUserGain;
};
