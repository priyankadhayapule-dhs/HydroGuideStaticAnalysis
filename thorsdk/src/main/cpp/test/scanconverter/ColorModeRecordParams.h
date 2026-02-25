//
// Copyright (C) 2018 EchoNous, Inc.
//
//
#pragma once

#include "ScanConverterParams.h"

// Data structure for recording color modes.

struct ColorModeRecordParams
{
    ScanConverterParams   scanConverterParams;
    uint32_t              colorMapIndex;
    bool                  isInverted;
}__attribute__((packed, aligned(1)));
