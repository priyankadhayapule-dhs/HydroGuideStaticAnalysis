//
// Copyright 2018 EchoNous Inc.
//
//

#pragma once

#include "ThorConstants.h"

#define THOR_RAW_TAG 0x54484F52     // ASCII for "THOR"
#define THOR_RAW_VERSION 0x5

class ThorRawDataFile
{
public: // Constants & structs
    struct Metadata
    {
        uint64_t    timestamp;
        uint8_t     probeType;
        uint32_t    imageCaseId;
        uint32_t    upsMajorVer;
        uint32_t    upsMinorVer;
        uint32_t    frameCount;
        uint32_t    frameInterval;
        uint32_t    dataTypes;      // Bits set using DataType values
    }__attribute__((packed, aligned(1)));

    // These represent bit positions and are not mutually exclusive.
    enum DataType
    {
        BMode = DAU_DATA_TYPE_B,
        CMode = DAU_DATA_TYPE_COLOR,
        PW = DAU_DATA_TYPE_PW,
        MMode = DAU_DATA_TYPE_M,
        CW = DAU_DATA_TYPE_CW,
        Auscultation = DAU_DATA_TYPE_DA,
        ECG = DAU_DATA_TYPE_ECG
    };

    struct Identification
    {
        uint32_t    tag;
        uint8_t     version;
        uint8_t     reserved[3];
    }__attribute__((packed, aligned(1)));
};
