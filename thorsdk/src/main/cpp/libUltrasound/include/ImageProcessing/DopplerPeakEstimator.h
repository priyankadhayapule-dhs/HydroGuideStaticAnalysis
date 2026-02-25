//
// Copyright 2020 EchoNous Inc.
//
//
#pragma once

#include <CineBuffer.h>
#include <ScrollModeRecordParams.h>
#include <AIManager.h>
#include <VTICalculation.h>
#include <Event.h>

struct keyCtrlPts
{
    int offset;
    int minLeftLoc;
    int peakLoc;
    int minRightLoc;
};

class DopplerPeakEstimator
{
public:
    DopplerPeakEstimator(CineBuffer* cineBuffer, AIManager* aiManager);
    ~DopplerPeakEstimator();

    static const char *name() { return "DopplerPeakEstimator"; }

    void    setParams(ScrollModeRecordParams smParams, int imgWidth, int imgHeight, float yShift,
            float yExpScale, bool traceInvert);

    void    estimatePeaks(float* mapMat, int arrayLen, int imgMode);
    void    estimatePeaks(float* mapMat, int arrayLen, int imgMode, uint32_t classId);

    static void completionCallback(void* userPtr, ThorStatus statusCode);

private:
    bool    findKeyControlPoints(float* inData, std::vector<std::pair<int,float>> topPts,
                                 std::vector<int> separationPts, int searchRange, float sampleMin, int offset,
                                 int searchRangeMin, int searchRangeMax, int desiredPeakLoc,
                                 bool isLinearProbe, float rightMultiplier, keyCtrlPts* keyPts);

    void    findPeakPoints(std::vector<std::pair<int,float>> topPts, std::vector<int> separationPts,
                           std::vector<std::pair<int,float>>& peakPts, float& avgDist);

    int     insertControlPoints(float* inData, float* tmpMat, keyCtrlPts* keyPts, int searchRange);
    void    getDefaultControlPoints(float* inData, float* mapMat, int searchRange);

    void    findMinLocs(float* inData, float peakVal, float sampleMin,
                        int peakLoc, int startIdx, int endIdx, int minMaintainDuration,
                        int& minL, int& minR, float rightMultiplier, bool disableDeltaThreshold = false);
    void    medianFilter(float* inData, float* outData, int arraySize, int windowSize);

    // Hole Fill Filter - inData is overwritten
    void    holeFillFilter(float* inData, int arraySize, int threshold, int windowSize);

private:
    CineBuffer*                 mCineBuffer;
    AIManager*                  mAIManager;

    ScrollModeRecordParams      mSMParams;
    int                         mImgWidth;
    int                         mImgHeight;
    float                       mYShift;
    float                       mYExpScale;
    bool                        mTraceInvert;

    Event                       mCompletionEvent;
};
