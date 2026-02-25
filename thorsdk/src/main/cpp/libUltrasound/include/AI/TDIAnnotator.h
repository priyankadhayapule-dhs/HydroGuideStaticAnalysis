//
// Copyright 2019 EchoNous Inc.
//
#pragma once
#include <ThorError.h>
#include "TDIAnnotatorConfig.h"
#include <vector>
class UltrasoundManager;

class TDIAnnotator
{
public:
    ThorStatus init(UltrasoundManager &manager);
    void close();

    ThorStatus annotate(float *postscan, std::vector<float> &view, std::vector<TDIYoloPrediction> &predictions);

    ThorStatus setInput(float *postscan);
    ThorStatus setOutputs(float *view, float *yolo);

    ThorStatus runModel();

public:
    ThorStatus loadModel(UltrasoundManager &manager);
    ThorStatus createBuffers();

    ThorStatus processClassLayer(std::vector<float> &view);
    ThorStatus processYoloLayer(std::vector<float> &layer, const TDIAnchorInfo &info, std::vector<TDIYoloPrediction> &predictions);

    // Model
    //TODO:: remove SNPE logic
    /*std::unique_ptr<zdl::SNPE::SNPE> mModel;

    // Input buffer descriptor
    std::unique_ptr<zdl::DlSystem::IUserBuffer> mInputBuffer;
    // Output buffer descriptors
    std::unique_ptr<zdl::DlSystem::IUserBuffer> mViewBuffer;
    std::unique_ptr<zdl::DlSystem::IUserBuffer> mYoloBuffer;

    // Map of input buffers and output buffers
    zdl::DlSystem::UserBufferMap mInputMap;
    zdl::DlSystem::UserBufferMap mOutputMap;*/

    // Memory for the output buffers
    std::vector<float> mView;
    std::vector<float> mYolo;
};