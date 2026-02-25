//
// Copyright 2019 EchoNous Inc.
//
#pragma once
#include <atomic>
#include <json/json.h>
// Configuration constants for the CardiacAnnotator model

extern const char *CARDIAC_ANNOTATOR_MODEL_NAME;

// -- older definition for reference
/*const int CARDIAC_ANNOTATOR_INPUT_WIDTH = 320;
const int CARDIAC_ANNOTATOR_INPUT_HEIGHT = 320;
const int CARDIAC_ANNOTATOR_INPUT_WIDTH_NEW = 224;
const int CARDIAC_ANNOTATOR_INPUT_HEIGHT_NEW = 224;
const int CARDIAC_ANNOTATOR_CLASSIFIER_NUM_CLASSES = 15;
const int CARDIAC_ANNOTATOR_LABELER_NUM_CLASSES = 51;
*/

// - newer definition
const int CARDIAC_ANNOTATOR_INPUT_WIDTH = 224;
const int CARDIAC_ANNOTATOR_INPUT_HEIGHT = 224;
const int CARDIAC_ANNOTATOR_CLASSIFIER_NUM_CLASSES = 20;
const int CARDIAC_ANNOTATOR_LABELER_NUM_CLASSES = 20;

const int CARDIAC_ANNOTATOR_LABELER_NUM_ANCHORS = 3;
const int CARDIAC_ANNOTATOR_LABELER_NUM_CHANNELS =
    (CARDIAC_ANNOTATOR_LABELER_NUM_CLASSES+5);

const float CARDIAC_ANNOTATOR_CONF_THRESHOLD=0.2f;
/////////////////////


struct AnchorInfo
{
    int fsize;
    int stride;
    // Sigmoid indices are {0, 1, 4, 5, ...}
    // x_shift is equal to x index
    // y_shift is equal to y index
    float w_anchors[CARDIAC_ANNOTATOR_LABELER_NUM_ANCHORS];
    float h_anchors[CARDIAC_ANNOTATOR_LABELER_NUM_ANCHORS];
};

struct YoloPrediction
{
    float x;
    float y;
    float w;
    float h;
    float normx;
    float normy;
    float normw;
    float normh;
    float obj_conf;
    float class_conf;
    int class_pred;
};

struct AnnotatorParams
{
    // Defaults
    AnnotatorParams();

    void reset();

    // Base threshold used for all labels
    std::atomic<float> base_label_threshold;
    // Tweak value per label (total threshold is base + tweak)
    std::atomic<float> label_thresholds[CARDIAC_ANNOTATOR_LABELER_NUM_CLASSES];
    // Threshold to be in a view
    std::atomic<float> view_threshold;
    // Fraction of smoothing buffer that a label must be in
    std::atomic<float> label_buffer_frac;
};

struct AutoLabelVerificationResult {
    std::vector<uint8_t> postscan;
    std::vector<float>   viewRaw;
    std::vector<float>   yoloRaw;
    std::vector<float>   yoloProc;

    Json::Value json;
};

extern AnchorInfo CARDIAC_ANNOTATOR_ANCHOR_INFOS[1];

extern AnnotatorParams CARDIAC_ANNOTATOR_PARAMS;
extern const bool CARDIAC_ANNOTATOR_VIEW_LABEL_MASK[CARDIAC_ANNOTATOR_CLASSIFIER_NUM_CLASSES][CARDIAC_ANNOTATOR_LABELER_NUM_CLASSES];
extern const char *CARDIAC_ANNOTATOR_VIEW_NAMES[CARDIAC_ANNOTATOR_CLASSIFIER_NUM_CLASSES];
extern const char *CARDIAC_ANNOTATOR_LABEL_NAMES_WITH_VIEW[CARDIAC_ANNOTATOR_LABELER_NUM_CLASSES];
extern const char *CARDIAC_ANNOTATOR_LABEL_NAMES[CARDIAC_ANNOTATOR_LABELER_NUM_CLASSES];