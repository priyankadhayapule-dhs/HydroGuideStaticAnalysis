//
// Copyright 2022 EchoNous Inc.
//
#pragma once

// Configuration constants for the CardiacAnnotator model
// to be consistent with other models  -  actual size: 7
//
// output size: yolo - 1*14*14*75
//            : view - 1*20 (from 15)
//            : gain - 1*3
//            : depth - 1*3
//            : anatomy 1*7

// struct without YoLo output - all these need to be processed.
static const int ANATOMY_LABEL_COUNT = 5;
struct AutoDepthGainPresetPred
{
    float view[20];
    float gain[3];
    float depth[3];
    float anatomy[ANATOMY_LABEL_COUNT];
};

// Class label count reduced for new single frame input model
const char* const PRESET_CLASSES[] =
{
    "CARDIAC", "ABDOMINAL", "LUNG", "NOISE", "IVC"
};

const char* const GAIN_CLASSES[] =
{
    "LOW", "OPTIMAL", "HIGH"
};

const char* const DEPTH_CLASSES[] =
{
    "DEEP", "OK", "SHALLOW"
};

enum DepthClass
{
    DEEP, OK, SHALLOW
};

/*
enum PresetClass
{
    CARDIAC, ABDOMINAL, NOISE, NOT_SURE, LUNG, SUBCOSTAL, IVC
};


// TODO: UPDATE - Phased Array ID
enum OrganId
{
    HEART, LUNGS, ABDOMEN
};*/
