//
// Copyright 2023 EchoNous Inc.
//
// Implementation of VTI Calculation Task
#pragma once

#include <vector>
#include <ThorError.h>
#include <AIManager.h>
#include <VTICalculation.h>

class VTICalculationTask : public AITask
{
    void *mUser;
    uint32_t  mClassId;
    VTICalculation* mVTICal;

public:
    VTICalculationTask(AIManager *manager, VTICalculation* vtiCalculation, uint32_t classId);
    ~VTICalculationTask() = default;

    virtual void run();

// private methods
private:
    ThorStatus runEnsemble(uint32_t classId, VtiNativeJni* jni);

    uint32_t findPeakLocation(cv::InputArray input, uint32_t ccIdx, uint32_t yLocation);
    uint32_t findYLocation(cv::InputArray input, uint32_t ccIdx, uint32_t xLocation, uint32_t yMin, uint32_t yMax, int increment);

};