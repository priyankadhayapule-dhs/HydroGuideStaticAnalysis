//
// Copyright 2023 EchoNous Inc.
//

#pragma once

// Temporal smoothing buffer for TDI Yolo predictions
#include <array>
#include <vector>
#include <utility>
#include <TDIAnnotatorConfig.h> // For YoloPrediction

class TDIYoloBuffer
{
public:
    TDIYoloBuffer(size_t length=30);
    // Append a single frame's predictions
    void append(const std::vector<float> &view, const std::vector<TDIYoloPrediction> &preds);
    // Get the smoothed boxes
    // Returns a refernce to an internal vector, copy the vector
    // if a longer lifespan is needed
    std::vector<TDIYoloPrediction>& get();
    std::pair<int, float> getView();
    void clear();
public:
    // Update the view predictions from the buffer contents
    void updateView();
    // Update the smoothed predictions with the current buffer contents
    void update();
    // circular buffer of yolo predictions, where mPredBuffer[frame][class] is the prediction for class
    std::vector<std::array<TDIYoloPrediction, TDI_ANNOTATOR_LABELER_NUM_CLASSES>> mYoloBuffer;
    std::vector<std::array<float, TDI_ANNOTATOR_CLASSIFIER_NUM_CLASSES>> mViewBuffer;
    size_t mPredIndex; // Index of next location in the pred buffers
    // View logits, mean across the buffer
    std::array<float, TDI_ANNOTATOR_CLASSIFIER_NUM_CLASSES> mViewFlat;
    // output predictions
    std::vector<TDIYoloPrediction> mSmoothPreds;
    int mSmoothView;
    float mSmoothViewProb;
};