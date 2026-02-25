//
// Copyright 2017 EchoNous Inc.
//
//

#pragma once

#include <ImageProcess.h>
#include <SpeckleReduction.h>

class SpeckleReductionProcess : public ImageProcess
{
public: // Methods
    SpeckleReductionProcess(const std::shared_ptr<UpsReader>& upsReader);
    ~SpeckleReductionProcess();

    static const char *name() { return "SpeckleReductionProcess"; }


    ThorStatus  init();
    ThorStatus  setProcessParams(ProcessParams& params);
    ThorStatus  process(uint8_t* inputPtr, uint8_t* outputPtr);
    void        terminate();

private: // Properties
    float               mSpecParams[SpeckleReduction::PARAM_ARRAY_SIZE];
    SpeckleReduction    mSpeckleReduction;
};
