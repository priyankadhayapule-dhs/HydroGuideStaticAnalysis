//
// Copyright 2018 EchoNous Inc.
//

#pragma once

#include <ThorConstants.h>

// older scan convert params to support older filer format.
struct ScanConverterParamsOld
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

struct ScanConverterParams : ScanConverterParamsOld
{
    // default probe shape -> phased array
    uint32_t    probeShape = PROBE_SHAPE_PHASED_ARRAY;

    float       startRayMm = 0.0f;
    float       steeringAngleRad = 0.0f;
}__attribute__((packed, aligned(1)));