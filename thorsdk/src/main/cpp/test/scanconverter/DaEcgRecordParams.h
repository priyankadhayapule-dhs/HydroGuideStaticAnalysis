//
// Copyright (C) 2018 EchoNous, Inc.
//
//
#pragma once

// Data structure for recording (a) frame/s of waveforms.

struct DaEcgRecordParams
{
    int         traceIndex;             // trace line location
    int         frameWidth;             // recorded frame width
    int         scrDataWidth;           // screen data width (input data width)
    float       samplesPerSecond;       // samples per second
    uint32_t    scrollSpeedIndex;       // scroll speed index
    uint32_t    samplesPerFrame;        // samples per frame
    bool        isScrFrame;             // is screen/single frame or multi frame
    int         decimationRatio;        // drawing decimationRatio
    int         location;               // ecg lead indicator / da mic location
}__attribute__((packed, aligned(1)));


struct DaEcgRecordParamsV3
{
    int         traceIndex;             // trace line location
    int         frameWidth;             // recorded frame width
    int         scrDataWidth;           // screen data width (input data width)
    float       samplesPerSecond;       // samples per second
    uint32_t    scrollSpeedIndex;       // scroll speed index
    uint32_t    samplesPerFrame;        // samples per frame
    bool        isScrFrame;             // is screen/single frame or multi frame
}__attribute__((packed, aligned(1)));