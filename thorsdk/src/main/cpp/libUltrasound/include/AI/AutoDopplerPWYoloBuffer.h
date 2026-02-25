//
// Copyright 2023 EchoNous Inc.
//
#pragma once

// Temporal smoothing buffer for PW Yolo predictions

#include <array>
#include <vector>
#include <utility>
#include <AutoDopplerPWAnnotatorConfig.h> // For YoloPrediction

class AutoDopplerPWYoloBuffer
{
public:
    AutoDopplerPWYoloBuffer(size_t length=30);

    // Append a single frame's predictions
    void append(const std::vector<float> &view, const std::vector<PWYoloPrediction> &preds);

    // Get the smoothed boxes
    // Returns a refernce to an internal vector, copy the vector
    // if a longer lifespan is needed
    std::vector<PWYoloPrediction>& get();

    std::pair<int, float> getView();

    void clear();

public:

    // Update the view predictions from the buffer contents
    void updateView();

    // Update the smoothed predictions with the current buffer contents
    void update();

    // circular buffer of yolo predictions, where mPredBuffer[frame][class] is the prediction for class
    std::vector<std::array<PWYoloPrediction, PW_ANNOTATOR_LABELER_NUM_CLASSES>> mYoloBuffer;
    std::vector<std::array<float, PW_ANNOTATOR_CLASSIFIER_NUM_CLASSES>> mViewBuffer;
    size_t mPredIndex; // Index of next location in the pred buffers

    // View logits, mean across the buffer
    std::array<float, PW_ANNOTATOR_CLASSIFIER_NUM_CLASSES> mViewFlat;

    // output predictions
    std::vector<PWYoloPrediction> mSmoothPreds;
    int mSmoothView;
    float mSmoothViewProb;
};
