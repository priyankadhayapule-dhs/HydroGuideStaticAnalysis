//
// Copyright 2023 EchoNous Inc.
//
#pragma once

// Configuration constants for the PWAnnotator model
const int PW_ANNOTATOR_INPUT_WIDTH = 224;
const int PW_ANNOTATOR_INPUT_HEIGHT = 224;
const int PW_ANNOTATOR_CLASSIFIER_NUM_CLASSES = 20;
const int PW_ANNOTATOR_LABELER_NUM_CLASSES = 20;
const int PW_ANNOTATOR_LABELER_NUM_ANCHORS = 3;


struct PWAnchorInfo
{
    int fsize;
    int stride;
    float anchors[PW_ANNOTATOR_LABELER_NUM_ANCHORS][2];
};

struct PWYoloPrediction
{
    float x;
    float y;
    float w;
    float h;
    float conf;
    int label;
};

struct PWAnnotatorParams
{
    // Base threshold used for all labels
    float base_label_threshold = 0.0f;
    // Tweak value per label (total threshold is base + tweak)
    float label_thresholds[PW_ANNOTATOR_LABELER_NUM_CLASSES] = {
        0.3,
        0.4,
        0.4,
        0.3,
        0.4,
        0.4,
        0.4,
        0.4,
        0.4,
        0.4,
        0.4,
        0.2,
        0.3,
        0.4,
        0.4,
        0.2,
        0.4,
        0.4,
        0.4,
        0.4
    };
    // Threshold to be in a view
    float view_threshold = 0.4f;
    // Fraction of smoothing buffer that a label must be in
    float label_buffer_frac = 0.4f;
};

extern PWAnchorInfo PW_ANNOTATOR_ANCHOR_INFO;

extern PWAnnotatorParams PW_ANNOTATOR_PARAMS;
extern const bool PW_ANNOTATOR_VIEW_LABEL_MASK[PW_ANNOTATOR_CLASSIFIER_NUM_CLASSES][PW_ANNOTATOR_LABELER_NUM_CLASSES];
extern const char *PW_ANNOTATOR_VIEW_NAMES[PW_ANNOTATOR_CLASSIFIER_NUM_CLASSES];
extern const char *PW_ANNOTATOR_LABEL_NAMES_WITH_VIEW[PW_ANNOTATOR_LABELER_NUM_CLASSES];
extern const char *PW_ANNOTATOR_LABEL_NAMES[PW_ANNOTATOR_LABELER_NUM_CLASSES];
