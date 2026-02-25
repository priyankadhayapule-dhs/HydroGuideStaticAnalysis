//
// Copyright (C) 2020 EchoNous, Inc.
//
//
#pragma once

// Data structure for recording (a) frame/s in scrolling modes.

// older M-mode Record Parameters
struct MModeRecordParams
{
    int   drawnIndex;
    int   curLineIndex;
    int   textureFrameWidth;
    int   linesPerBFrame;
    bool  isLive;
    bool  isTextureFrame;
}__attribute__((packed, aligned(1)));


// ScrollMode Record Parameters (M, PW, CW)
struct ScrollModeRecordParams
{
    int         traceIndex;             // trace line location
    int         frameWidth;             // recorded frame width
    int         scrDataWidth;           // screen data width (input data width)
    int         samplesPerLine;         // fftSize (in PW/CW)
    uint32_t    orgNumLinesPerFrame;    // orgNumLinesPerFrame (before re-sample)
    float       numLinesPerFrame;       // lines per frame
    float       baselineShift;          // baselineShift (float)
    float       scrollSpeed;            // input lines per second
    float       desiredTimeSpan;        // desiredTimeSpan
    float       yPhysicalScale;         // Y scale mapped to -1.0 ~ 1.0
    bool        isScrFrame;             // is screen/single frame or multi frame
    bool        isInvert;               // invert
}__attribute__((packed, aligned(1)));