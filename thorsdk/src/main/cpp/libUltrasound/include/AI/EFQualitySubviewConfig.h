//
// Copyright 2020 EchoNous Inc.
//
#pragma once

static const int NUM_SUBVIEWS = 23;

// A single prediction made by the model
struct EFQualitySubviewPred
{
    float subview[NUM_SUBVIEWS]; // Main view and subview prediction
    float quality[5];
};

// All configurable options for post-processing
struct EFQualitySubviewConfig
{
    EFQualitySubviewConfig();

    // Load from a file
    void loadFrom(const char *path);
    // Save to a file
    void saveTo(const char *path);

    void reset();

    float a4cQualityCapTau;
    float a2cQualityCapTau;
    float a4cQualityTau[5];
    float a2cQualityTau[5];
    float subviewTau[NUM_SUBVIEWS];
};

const char * const IQ_SUBVIEW_NAMES[] =
{
    "A4C",
    "Fan Down",
    "Fan Up",
    "Rotate Ctr-Clockwise",
    "Rotate Clockwise",
    "Rock Lateral",
    "Rock Medial",
    "Slide One Rib Up",
    "Slide Medial",
    "Slide Lateral",
    "A2C",
    "NOISE",
    "A3C",
    "A2C Fan Down",
    "A2C Fan Up",
    "A2C Rock Medial",
    "A2C Rock Lateral",
    "Not-Sure",
    "Mirrored-A4C",
    "Mirrored-A2C",
    "A2C Lateral Wall Not Visible",
    "A2C-SUP",
    "A3C Subviews"
};