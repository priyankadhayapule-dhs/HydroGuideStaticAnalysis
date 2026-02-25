//
// Copyright 2017 EchoNous Inc.
//
//

#pragma once

#include <stdint.h>
#include <memory>
#include <ThorError.h>
#include <UpsReader.h>

class ImageProcess
{
public: // ProcessParams
    struct ProcessParams
    {
        uint32_t    imagingCaseId;
        uint32_t    imagingMode;
        uint32_t    numSamples;
        uint32_t    numRays;
    };

public: // Methods
    ImageProcess(const std::shared_ptr<UpsReader>& upsReader) :
        mUpsReaderSPtr(upsReader), mIsEnabled(true) {}

    virtual ~ImageProcess() {}

    virtual ThorStatus  init() = 0;
    virtual ThorStatus  setProcessParams(ProcessParams& params) = 0;
    virtual ThorStatus  process(uint8_t* inputPtr, uint8_t* outputPtr) = 0;
    virtual void        terminate() = 0;

    void setEnabled(bool isEnabled) { mIsEnabled = isEnabled; }
    bool isEnabled() { return(mIsEnabled); }

protected: // Methods
    UpsReader*  getUpsReader() { return mUpsReaderSPtr.get(); }

private: // Properties
    std::shared_ptr<UpsReader>      mUpsReaderSPtr;
    bool                            mIsEnabled;
};
