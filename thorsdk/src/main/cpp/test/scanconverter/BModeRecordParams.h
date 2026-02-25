//
// Copyright (C) 2019 EchoNous, Inc.
//
//
#pragma once

#include "ScanConverterParams.h"

// Data structure for recording B mode data.
struct BModeRecordParams
{
    ScanConverterParams   scanConverterParams;
}__attribute__((packed, aligned(1)));
