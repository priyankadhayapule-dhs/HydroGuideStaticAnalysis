//
// Copyright 2018 EchoNous Inc.
//
#include <stdint.h>
#pragma once

struct ScanConverterParams
{
    // TODO: Add Probe Type/ID member
    uint32_t    numSampleMesh;
    uint32_t    numRayMesh;
    uint32_t    numSamples;
    uint32_t    numRays;
    float       startSampleMm;
    float       sampleSpacingMm;
    float       raySpacing;
    float       startRayRadian;
}__attribute__((packed, aligned(1)));

