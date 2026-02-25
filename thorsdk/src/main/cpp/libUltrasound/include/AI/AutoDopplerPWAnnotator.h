//
// Copyright 2023 EchoNous Inc.
//
#pragma once
#include <ThorError.h>
#include <vector>
#include "AutoDopplerPWAnnotatorConfig.h"
// TODO::Include Model

struct PWVerificationResult;
struct AutoDopplerPWJNI;
class AutoDopplerPWAnnotator
{
public:
    ThorStatus init();
    void close();

    ThorStatus annotate(const unsigned char *postscan, std::vector<float> &view, std::vector<PWYoloPrediction> &predictions);
    ThorStatus annotate(const unsigned char *postscan, std::vector<float> &view, std::vector<PWYoloPrediction> &predictions, PWVerificationResult &result);

    void processClassLayer(std::vector<float> viewIn, std::vector<float> &viewOut);
    void processYoloLayer(std::vector<float> yoloIn, const PWAnchorInfo &info, std::vector<PWYoloPrediction> &predictions);
    void processYoloLayer(std::vector<float> yoloIn, const PWAnchorInfo &info, std::vector<PWYoloPrediction> &predictions, PWVerificationResult &result);

private:
    AutoDopplerPWJNI *mModel;
};
