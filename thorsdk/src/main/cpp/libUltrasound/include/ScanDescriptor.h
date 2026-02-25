//
// Copyright (C) 2018 EchoNous, Inc.
//
//
#pragma once

// Data structure that can be used to describe either field of view or ROI
// in physical coordinates.

typedef struct
{
    float axialStart;     // mm
    float axialSpan;      // mm
    float azimuthStart;   // radians for phased/sector, mm for linear
    float azimuthSpan;    // radians for phased/sector, mm for linear
    float roiA;
} ScanDescriptor;

