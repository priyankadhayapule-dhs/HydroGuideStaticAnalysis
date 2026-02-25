//
// Copyright 2017 EchoNous Inc.
//
//

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <SNPE/SNPE.hpp>
#include <DlContainer/IDlContainer.hpp>
#include <DlSystem/ITensor.hpp>
#include <ThorError.h>


class InferenceEngine
{
public: // Methods
    explicit InferenceEngine();
    ~InferenceEngine();

    ThorStatus          loadDlc(std::string& dlcFile);
    ThorStatus          loadDlcLibrary(const char* dlcFilename);
    void                setOutLayers(std::string& outputLayers);

    // Get access to internal buffer for prediction operation
    float*              getInputPtr();

    // Prediction operation against internal buffer.  Assumed that caller
    // filled in buffer from getInputPtr().
    std::unordered_map<std::string, std::vector<float>>  predict();

private: // Methods
    void*       getDlContainer(const char* libraryName);
    ThorStatus  initializeSnpe(std::unique_ptr<zdl::DlContainer::IDlContainer>& containerPtr);

private: // Properties
    std::unique_ptr<zdl::SNPE::SNPE>        mSnpe;
    std::unique_ptr<zdl::DlSystem::ITensor> mInputTensor;
    void*                                   mDlcHandle;
    std::vector<std::string>                mOutputLayers;
};


