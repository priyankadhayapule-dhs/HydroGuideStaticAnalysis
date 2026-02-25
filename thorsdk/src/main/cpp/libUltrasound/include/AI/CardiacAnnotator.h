//
// Copyright 2019 EchoNous Inc.
//
#pragma once
#include <ThorError.h>
#include "CardiacAnnotatorConfig.h"
#include <vector>
class UltrasoundManager;

class CardiacAnnotator
{
public:
    ThorStatus init(UltrasoundManager &manager);
    void close();

public:
    ThorStatus createBuffers();

    ThorStatus processClassLayer(std::vector<float> &view);
    ThorStatus processYoloLayer(std::vector<float> &layer, const AnchorInfo &info);
    ThorStatus postprocessYoloOutput(const std::vector<float> &layer1, std::vector<YoloPrediction> &predictions);
    ThorStatus postprocessYoloOutputExt(const std::vector<float> &layer1,
            std::vector<YoloPrediction> (&detections_by_label)[CARDIAC_ANNOTATOR_LABELER_NUM_CLASSES], std::vector<YoloPrediction> &predictions);

    // Memory for the output buffers
    std::vector<float> mX0;
    std::vector<float> mX1;

    // Anchor box info for each Yolo output layer
    std::vector<AnchorInfo> mAnchorInfos;

    // Class layer output
    std::vector<float> mClassProbs;
    std::vector<int>   mClassIndices;
};