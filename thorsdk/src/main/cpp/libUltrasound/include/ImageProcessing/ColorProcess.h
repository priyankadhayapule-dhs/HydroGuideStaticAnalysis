//
// Copyright 2017 EchoNous Inc.
//
//

#pragma once

#include <ImageProcess.h>
#include <Colorflow.h>
#include <ScanConverterParams.h>

class ColorProcess : public ImageProcess
{
public: // Methods
    ColorProcess(const std::shared_ptr<UpsReader>& upsReader);
    ~ColorProcess();

    static const char *name() { return "ColorProcess"; }


    ThorStatus  init();
    ThorStatus  setProcessParams(ProcessParams& params);
    ThorStatus  setProcessParams(uint32_t imagingCaseId, ScanConverterParams scp, uint32_t colorMode);
    void        resetFrameCount();
    ThorStatus  setGain( int gain );
    ThorStatus  process(uint8_t* inputPtr, uint8_t* outputPtr);
    void        terminate();

private: // Properties
    Colorflow                   mColorflow;
    Colorflow::ThresholdParams  mThresholds;
    struct ScanConverterParams  mCConverterParams;
    uint32_t                    mFrameCount;
};


