//
// Copyright 2017 EchoNous Inc.
//
//

#pragma once

#include <ImageProcess.h>
#include <DauInputManager.h>

class DeInterleaveMProcess : public ImageProcess
{
public: // Methods
    DeInterleaveMProcess(const std::shared_ptr<UpsReader>& upsReader,
                         const std::shared_ptr<DauInputManager>& inputManager);
    ~DeInterleaveMProcess();

    static const char *name() { return "DeInterleaveMProcess"; }


    ThorStatus  init();
    ThorStatus  setProcessParams(ProcessParams& params);
    void        setMultilineSelection(uint32_t multilineSelection);
    ThorStatus  process(uint8_t* inputPtr, uint8_t* outputPtr);
    void        terminate();

private: // Properties
    std::shared_ptr<DauInputManager>    mInputManagerSPtr;
    uint32_t mMultilineSelection;
};
