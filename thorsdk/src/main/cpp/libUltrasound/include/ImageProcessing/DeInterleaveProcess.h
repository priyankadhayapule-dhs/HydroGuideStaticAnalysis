//
// Copyright 2017 EchoNous Inc.
//
//

#pragma once

#include <ImageProcess.h>
#include <DauInputManager.h>

class DeInterleaveProcess : public ImageProcess
{
public: // Methods
    DeInterleaveProcess(const std::shared_ptr<UpsReader>& upsReader,
                        const std::shared_ptr<DauInputManager>& inputManager);
    ~DeInterleaveProcess();

    static const char *name() { return "DeInterleaveProcess"; }


    ThorStatus  init();
    ThorStatus  setProcessParams(ProcessParams& params);
    ThorStatus  process(uint8_t* inputPtr, uint8_t* outputPtr);
    void        terminate();

private: // Properties
    std::shared_ptr<DauInputManager>    mInputManagerSPtr;
};
