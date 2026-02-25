//
// Copyright 2023 EchoNous Inc.
//

#pragma once

// Configuration constants for the TDIAnnotator model

extern const char *TDI_ANNOTATOR_MODEL_PATH;
const int TDI_ANNOTATOR_INPUT_WIDTH = 224;
const int TDI_ANNOTATOR_INPUT_HEIGHT = 224;
const int TDI_ANNOTATOR_CLASSIFIER_NUM_CLASSES = 20;
const int TDI_ANNOTATOR_LABELER_NUM_CLASSES = 3;
const int TDI_ANNOTATOR_LABELER_NUM_ANCHORS = 3;
const int TDI_ANNOTATOR_LABELER_NUM_CHANNELS = TDI_ANNOTATOR_LABELER_NUM_CLASSES+5;
struct TDIAnchorInfo
{
    int fsize;
    int stride;
    float anchors[TDI_ANNOTATOR_LABELER_NUM_ANCHORS][2];
};

struct TDIYoloPrediction
{
    float x;
    float y;
    float w;
    float h;
    float conf;
    int label;
};

struct TDIAnnotatorParams
{
    // Base threshold used for all labels
    float base_label_threshold = 0.2f;
    // Tweak value per label (total threshold is base + tweak)
    float label_thresholds[TDI_ANNOTATOR_LABELER_NUM_CLASSES] = {0.0f, 0.0f, 0.0f};
    // Threshold to be in a view
    float view_threshold = 0.4f;
    // Fraction of smoothing buffer that a label must be in
    float label_buffer_frac = 0.4f;
};

extern TDIAnchorInfo TDI_ANNOTATOR_ANCHOR_INFO;
extern TDIAnnotatorParams TDI_ANNOTATOR_PARAMS;
extern const bool TDI_ANNOTATOR_VIEW_LABEL_MASK[TDI_ANNOTATOR_CLASSIFIER_NUM_CLASSES][TDI_ANNOTATOR_LABELER_NUM_CLASSES];
extern const char *TDI_ANNOTATOR_VIEW_NAMES[TDI_ANNOTATOR_CLASSIFIER_NUM_CLASSES];
extern const char *TDI_ANNOTATOR_LABEL_NAMES_WITH_VIEW[TDI_ANNOTATOR_LABELER_NUM_CLASSES];
extern const char *TDI_ANNOTATOR_LABEL_NAMES[TDI_ANNOTATOR_LABELER_NUM_CLASSES];