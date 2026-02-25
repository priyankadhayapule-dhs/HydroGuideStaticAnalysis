//
// Copyright 2017 EchoNous Inc.
//
//

#pragma once

#include <ImageProcess.h>
#include <Autoecho.h>
#include <ThorConstants.h>

class AutoGainProcess : public ImageProcess
{
public: // Methods
    AutoGainProcess(const std::shared_ptr<UpsReader>& upsReader);
    ~AutoGainProcess();

    static const char *name() { return "AutoGainProcess"; }

    ThorStatus  init();
    ThorStatus  setProcessParams(ProcessParams& params);
    ThorStatus  process(uint8_t* inputPtr, uint8_t* outputPtr);
    void        terminate();

    void        setUserGain(float userGain);

private: // Properties
    struct Autoecho::Params     mAutoechoParams;
    uint8_t                     mGainNoiseFrame[MAX_B_FRAME_SIZE];
    Autoecho                    mAutoecho;
    uint32_t                    mFrameCount;
};
