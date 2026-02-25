//
// Copyright 2019 EchoNous Inc.
//
#pragma once
#include <ThorError.h>
#include "FASTObjectDetectionConfig.h"
#include <vector>
class UltrasoundManager;

class FASTObjectDetectorModel
{
public:
    ThorStatus init(UltrasoundManager &manager, const char *path);
    void close();

    ThorStatus annotate(float *postscan, std::vector<float> &view, std::vector<FASTYoloPrediction> &predictions);
    ThorStatus postProcess(std::vector<float>& view, std::vector<float>& yolo, std::vector<float>& outView, std::vector<FASTYoloPrediction>& outYolo);
    ThorStatus setInput(float *postscan);
    ThorStatus setOutputs(float *x0, float *x1);

    ThorStatus runModel();

public:
    ThorStatus loadModel(UltrasoundManager &manager, const char *path);
    ThorStatus createBuffers();

    ThorStatus processClassLayer(std::vector<float> &view);
    ThorStatus processYoloLayer(std::vector<float> &layer, const FASTAnchorInfo &info);
    ThorStatus postprocessYoloOutput(const std::vector<float> &layer1, std::vector<FASTYoloPrediction> &predictions, const FASTAnchorInfo &info);
    ThorStatus postprocessYoloOutputExt(const std::vector<float> &layer1,
                                        std::vector<FASTYoloPrediction> (&detections_by_label)[FAST_OBJ_DETECT_NUM_OBJECTS], std::vector<FASTYoloPrediction> &predictions, const FASTAnchorInfo &info);

    // Memory for the output buffers
    std::vector<float> mX0;
    std::vector<float> mX1;

    // Anchor box info for each Yolo output layer
    std::vector<FASTAnchorInfo> mAnchorInfos;

    // Class layer output
    std::vector<float> mClassProbs;
    std::vector<int>   mClassIndices;
};