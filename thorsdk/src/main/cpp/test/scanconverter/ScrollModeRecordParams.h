//
// Copyright (C) 2018 EchoNous, Inc.
//
//
#pragma once

// Data structure for recording (a) frame/s in scrolling modes.

struct ScrollModeRecordParams
{
    int   drawnIndex;
    int   curLineIndex;
    int   textureFrameWidth;
    int   linesPerBFrame;
    bool  isLive;
    bool  isTextureFrame;
}__attribute__((packed, aligned(1)));

