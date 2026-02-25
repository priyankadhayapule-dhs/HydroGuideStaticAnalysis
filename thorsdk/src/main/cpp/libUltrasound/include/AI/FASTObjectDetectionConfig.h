//
// Copyright 2019 EchoNous Inc.
//
#pragma once
#include <atomic>
// Configuration constants for the FASTExam object detection model

const int FAST_OBJ_DETECT_INPUT_WIDTH = 128;
const int FAST_OBJ_DETECT_INPUT_HEIGHT = 128;
const int FAST_OBJ_DETECT_NUM_VIEWS = 15;
const int FAST_OBJ_DETECT_NUM_OBJECTS = 17;
const int FAST_OBJ_DETECT_NUM_ANCHORS = 3;
const int FAST_OBJ_DETECT_NUM_CHANNELS =
        (FAST_OBJ_DETECT_NUM_OBJECTS+5);

struct FASTAnchorInfo
{
    int fsize;
    int stride;
    // Sigmoid indices are {0, 1, 4, 5, ...}
    // x_shift is equal to x index
    // y_shift is equal to y index
    float w_anchors[FAST_OBJ_DETECT_NUM_ANCHORS];
    float h_anchors[FAST_OBJ_DETECT_NUM_ANCHORS];
};

struct FASTYoloPrediction
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

    float coords[4];
    float sumerrors[4];
    int frames;

    float std_dev;

    FASTYoloPrediction() :
        x(0), y(0), w(0), h(0),
        normx(0), normy(0), normw(0), normh(0),
        obj_conf(0), class_conf(0),
        class_pred(0), std_dev(0) {}
    FASTYoloPrediction(float x, float y, float w, float h,
                       float normx, float normy, float normw, float normh,
                       float obj_conf, float class_conf, int class_pred, float std_dev) :
            x(x), y(y), w(w), h(h),
            normx(normx), normy(normy), normw(normw), normh(normh),
            obj_conf(obj_conf), class_conf(class_conf),
            class_pred(class_pred), std_dev(std_dev) {}
};

struct FASTObjectDetectorParams
{
    // Defaults
    FASTObjectDetectorParams();

    void reset();

    // Base threshold used for all labels
    std::atomic<float> base_label_threshold;
    // Tweak value per label (total threshold is base + tweak)
    std::atomic<float> label_thresholds[FAST_OBJ_DETECT_NUM_OBJECTS];
    // Threshold to be in a view
    std::atomic<float> view_threshold;
    // Fraction of smoothing buffer that a label must be in
    std::atomic<float> label_buffer_frac;
};

extern FASTAnchorInfo FAST_OBJ_DETECT_ANCHOR_INFOS[1];

const size_t FAST_OBJ_DETECT_ENSEMBLE_SIZE = 4;
extern const char *FAST_OBJ_DETECT_MODEL_PATHS[FAST_OBJ_DETECT_ENSEMBLE_SIZE];
extern FASTObjectDetectorParams FAST_OBJ_DETECT_PARAMS;
extern const bool FAST_OBJ_DETECT_VIEW_LABEL_MASK[FAST_OBJ_DETECT_NUM_VIEWS][FAST_OBJ_DETECT_NUM_OBJECTS];
extern const char *FAST_OBJ_DETECT_VIEW_NAMES[FAST_OBJ_DETECT_NUM_VIEWS];
extern const char *FAST_OBJ_DETECT_VIEW_DISPLAY_NAMES[FAST_OBJ_DETECT_NUM_VIEWS];
extern const char *FAST_OBJ_DETECT_LABEL_NAMES_WITH_VIEW[FAST_OBJ_DETECT_NUM_OBJECTS];
extern const char *FAST_OBJ_DETECT_LABEL_NAMES[FAST_OBJ_DETECT_NUM_OBJECTS];