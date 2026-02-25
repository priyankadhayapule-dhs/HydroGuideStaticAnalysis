//
// Copyright 2017 EchoNous Inc.
//
//
#pragma once
#include <stdint.h>
#include "opencv2/core.hpp"
#include <ThorError.h>

using namespace cv;

class Colorflow
{
public:
    enum
    {
        ACFILTER_MODE_PASSTHROUGH=-1,
        ACFILTER_MODE_BLUR=0,
        ACFILTER_MODE_GAUSSIAN=1
    } ACFilterMode;

    typedef struct
    {
        int32_t  acFilterMode;
        float    axialSigmaND;
        float    axialSigmaP;
        float    lateralSigmaND;
        float    lateralSigmaP;
        uint32_t axialSizeND;
        uint32_t axialSizeP;
        uint32_t lateralSizeND;
        uint32_t lateralSizeP;
    } AutoCorrParams;

    typedef struct
    {
        float vel;
        float power;
    } PersistenceParams;

    typedef struct
    {
        float vel;
        float power;
        float var;
        float clutter;
        float powMax;
        float powDynamicRange;
        float gainRange;
        float clutterFactor;
    } ThresholdParams;

    typedef struct
    {
        uint32_t lateralSizeVel;
        uint32_t axialSizeVel;
        uint32_t lateralSizeP;
        uint32_t axialSizeP;
        float lateralSigmaVel;
        float axialSigmaVel;
        uint32_t mode;
        float     velThresh;
        uint32_t lateralSizeVel2;
        uint32_t axialSizeVel2;
        float lateralSigmaVel2;
        float axialSigmaVel2;
        uint32_t mode2;
        float powThresh;
    } SmoothingFilterParams;

    typedef struct
    {
        uint32_t filtersize;
        float threshold;
    } NoiseFilterParams;

    typedef struct
    {
        float powmaxthreshold;
        float velmaxthreshold;
        float velminthreshold;
    } BorderShadeParams;

    typedef struct
    {
        float cpdCompressDynamicRangeDb;
        float cpdCompressPivotPtIn;
        float cpdCompressPivotPtOut;
    } CPDParams;

    typedef struct
    {
//        uint32_t          samplesPerLine;
//        uint32_t          linesPerFrame;
//        uint32_t          ensembleSize;
        AutoCorrParams        autoCorr;
        PersistenceParams     persistence;
        ThresholdParams       threshold;
        SmoothingFilterParams smoothing;
        NoiseFilterParams     noisefilter;
        BorderShadeParams     bordershade;
        CPDParams             cpd;
        uint32_t              colorMode;   // CVD / CPD
    } Params;

public:
    Colorflow ();
    ~Colorflow() {}

    ThorStatus processFrame(uint8_t *inputBuffer,
                            uint32_t numRays,
                            uint32_t numSamples,
                            uint32_t frameCount,
                            uint8_t* outputBuffer);
    void setParams(const Colorflow::Params &params);
    void setPowerThreshold( float threshold );
    
private:
    void       injectTestSignal(int count);
    void       injectTestVelocity(Mat& phaseInp);
    void       separateOutNDPU( float *inputPtr, int32_t frameCount);
    void       logParams(); // for debugging
    void       calculateSmoothingCoefficient(float lateralCoeff[], float axialCoeff[],float lateralCoeff2[], float axialCoeff2[]);
    //void       calculateSmoothingCoefficient(float lateralCoeff[], float axialCoeff[]);
    ThorStatus preprocessInput(uint32_t frameCount);
    ThorStatus estimatePhase(uint32_t frameCount);
    ThorStatus persistPhase(Mat& currentFrame, Mat& prevFrame, float alpha);
    ThorStatus applyNoiseFilter(Mat& phaseEst);
    ThorStatus applyPostDetectionFilter(Mat& phaseInp);
    
private:
    Params mParams;
    Mat mInputFromFpga;
    Mat mInput;
    Mat mN;
    Mat mD;
    Mat mP;
    Mat mU;

    Mat mNs;  // smoothed Numerator
    Mat mDs;  // smoothed Denominator
    Mat mPs;  // smoothed wall filtered Power
    Mat mUs;  // smoothed un(wall)filtered Power
    Mat mPostDetFilterBuf;

    Mat mPowerThreshMask;
    Mat mClutterPowerThreshMask;
    
    Mat mPowerDb;
    Mat mClutterPowerDb;
    Mat mPrevPhaseEst; // for persistence
    Mat mPhaseEst;   
    Mat mOutput;
    Mat mPowerCompress;
    Mat mPowerCompressEst;
    uint32_t mNumLines, mNumSamples;
    float mLateralCoeff[9];
    float mAxialCoeff[9];
    float mLateralCoeff2[9];
    float mAxialCoeff2[9];
};
