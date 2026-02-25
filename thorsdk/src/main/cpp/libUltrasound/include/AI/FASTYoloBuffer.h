//
// Copyright 2020 EchoNous Inc.
//
#pragma once

// Temporal smoothing buffer for Yolo predictions

#include <array>
#include <vector>
#include <utility>
#include <FASTObjectDetectionConfig.h> // For YoloPrediction

class FASTYoloBuffer
{
public:
    FASTYoloBuffer(size_t yololength, size_t viewlength);

    // Append a single frame's predictions
    void append(const std::vector<float> &view, const std::vector<FASTYoloPrediction> &preds);

    // Get the smoothed boxes
    // Returns a refernce to an internal vector, copy the vector
    // if a longer lifespan is needed
    std::vector<FASTYoloPrediction>& get();
    std::array<float, FAST_OBJ_DETECT_NUM_VIEWS>& getViewscores();

    std::pair<int, float> getView();

    void clear();

public:

    // Update the view predictions from the buffer contents
    void updateView();

    // Update the smoothed predictions with the current buffer contents
    void update();

    // circular buffer of yolo predictions, where mPredBuffer[frame][class] is the prediction for class
    std::vector<std::array<FASTYoloPrediction, FAST_OBJ_DETECT_NUM_OBJECTS>> mFASTYoloBuffer;
    std::vector<std::array<float, FAST_OBJ_DETECT_NUM_VIEWS>> mViewBuffer;
    size_t mPredIndexYolo; // Index of next location in the pred buffers
    size_t mPredIndexView;
    size_t mSizeYolo; ///< Number of valid items in the buffer
    size_t mSizeView;

    // View logits, mean across the buffer
    std::array<float, FAST_OBJ_DETECT_NUM_VIEWS> mViewFlat;

    // output predictions
    std::vector<FASTYoloPrediction> mSmoothPreds;
    int mSmoothView;
    float mSmoothViewProb;
};