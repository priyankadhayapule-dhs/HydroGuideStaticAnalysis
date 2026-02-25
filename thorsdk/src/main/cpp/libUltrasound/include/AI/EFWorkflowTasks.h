//
// Copyright 2019 EchoNous Inc.
//
// Implementation of EFWorkflow Tasks (native side)
#pragma once

#include <vector>
#include <ThorError.h>
#include <AIManager.h>
#include <EFWorkflow.h>


class FrameIDTask : public AITask
{
    void *mUser;
    EFWorkflow *mWorkflow;
    CardiacView mView;
    std::vector<float> mPhase;
    int mPeriod;
    std::vector<int> mEDFramesMapped;
    std::vector<int> mESFramesMapped;
    std::vector<float> mEstimatedEF;
    std::vector<float> mEDUncertainties;
    std::vector<float> mESUncertainties;
    std::vector<float> mEDLengths;
    std::vector<float> mESLengths;
    MultiCycleFrames mFrames;
public:
    FrameIDTask(AIManager *manager, void *user, EFWorkflow *workflow, CardiacView view);

    // For running segmentation as part of phase detection
    ThorStatus doSegmentationAndUncertaintyCheck(MultiCycleFrames *frames);
    ThorStatus doSegmentationAndUncertaintyCheck(MultiCycleFrames *frames, void *jenv);
    ThorStatus segment(uint32_t frameNum, std::unordered_map<int, CardiacSegmentation> &segs, bool isEDFrame);
    ThorStatus segment(uint32_t frameNum, std::unordered_map<int, CardiacSegmentation> &segs, bool isEDFrame, void *jenv);
    virtual void run();
    // For testing purposes
    const std::vector<float>& getPhaseAreas() const;
    ThorStatus runImmediate(FILE* rawOut, void *jenv, const char* debugOut);
    int getPeriod() const;
    const std::vector<int>& getEDFramesMapped() const;
    const std::vector<int>& getESFramesMapped() const;
    const std::vector<float>& getEstimatedEFs() const;
    const std::vector<float>& getEDUncertainties() const;
    const std::vector<float>& getESUncertainties() const;
    const std::vector<float>& getEDLengths() const;
    const std::vector<float>& getESLengths() const;
    const MultiCycleFrames &getFrames() const;
    void setDebugOutPath(const char* debugOut);
    const char* mDebugOutPath;
};

class SegmentationTask : public AITask
{
    void *mUser;
    EFWorkflow *mWorkflow;
    CardiacView mView;
    int mFrame;
    bool mIsEDFrame;
    bool mIsUserEdit;
    const char* mDebugOutPath;
public:
    SegmentationTask(AIManager *manager, void *user, EFWorkflow *workflow, CardiacView view, int frame, bool isEDFrame,
            bool isUserEdit);

    ThorStatus segment(uint32_t frameNum, std::unordered_map<int, CardiacSegmentation> &segs);
    ThorStatus segment(uint32_t frameNum, std::unordered_map<int, CardiacSegmentation> &segs, void *jenv);

    virtual void run();
    ThorStatus runImmediate(void *jenv, const char* debugOut);
    void setDebugOutPath(const char* debugOut);
};

class EFCalculationTask : public AITask
{
    void *mUser;
    EFWorkflow *mWorkflow;
public:
    EFCalculationTask(AIManager *manager, void *user, EFWorkflow *workflow);
    virtual void run();
};