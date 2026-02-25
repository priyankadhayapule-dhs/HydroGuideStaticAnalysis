//
// Copyright 2023 EchoNous Inc.
//
// Implementation of VTI Calculation
#pragma once

#include <vector>
#include <ThorError.h>
#include <AIManager.h>
#include <VtiNativeJni.h>
#define MAX_INTER_PTS 10

struct VtiPeak
{
    cv::Point2i baseL;
    cv::Point2i baseR;
    cv::Point2i peak;
    cv::Point2i interL[MAX_INTER_PTS];
    cv::Point2i interR[MAX_INTER_PTS];

    uint32_t numInterL;
    uint32_t numInterR;
};

class VTICalculation
{
private:
    // private member data
    AIManager *mManager;

    // completionCallback
    void (*mCompletionCallback)(void*, ThorStatus);

    void* mUser;

    uint32_t mImgMode;
    uint64_t mTimeStamp;
    uint32_t mBaselineShiftIndex;
    bool     mBaselineInvert;
    uint32_t mNumSamples;
    uint32_t mFrameNum;
    uint32_t mClassId;

public:
    // output mVtiPeaks
    std::vector<VtiPeak> mVtiPeaks;

public:
    ThorStatus init(AIManager *manager, void* javaEnv, void* javaContext);

    void reset();

    void setCompletionCallback(void (*cb)(void*, ThorStatus));
    void notifyComplete(ThorStatus statusCode);

    // VTI calculation state
    void setCalState(uint32_t imgMode, uint32_t frameNum, uint32_t numSamples, uint32_t baselineShiftIdx, bool baselineInvert, uint64_t timeStamp, uint32_t classId);
    bool compareCalState(uint32_t imgMode, uint32_t frameNum, uint32_t numSamples, uint32_t baselineShiftIdx, bool baselineInvert, uint64_t timeStamp, uint32_t classId);
    bool compareCalState(uint32_t classId);

    VTICalculation();
    ~VTICalculation() = default;

    // inputDataType: PW or CW
    std::unique_ptr<AITask> createVTICalculationTask(uint32_t classId, void* user);

    //jni
    VtiNativeJni* mVtiJni;
    VtiNativeJni* getNativeJNI();
};