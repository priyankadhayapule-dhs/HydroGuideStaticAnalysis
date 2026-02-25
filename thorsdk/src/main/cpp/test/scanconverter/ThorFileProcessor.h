//
// Copyright (C) 2018 EchoNous, Inc.
//
//
#pragma once

#include <memory>
#include "ThorError.h"
#include "ThorRawReader.h"

#define NUM_DATA_TYPE 10


class ThorFileProcessor
{
public:
    ThorFileProcessor();
    ~ThorFileProcessor();

    // process Mode Header for getting header and data
    ThorStatus      processModeHeader(uint8_t* dataPtr, uint32_t dataTypes,
                                      uint32_t frameCount, uint8_t version);
    // read & copy the modeHeader to headerPtr
    ThorStatus      getModeHeader(ThorRawDataFile::DataType dataType, void* headerPtr);
    // read & copy the data to dataPtr, designed to be used fo copying the data to CineBuffer
    ThorStatus      getData(ThorRawDataFile::DataType dataType, void* dataPtr);
    ThorStatus      getDataPreVer(ThorRawDataFile::DataType dataType, void* dataPtr);

    // get dataptr
    void*           getDataPtr(ThorRawDataFile::DataType dataType);

    // get FrameHeaderSize
    size_t             getFrameHeaderSize();

    // Data structure that describes mode header
    struct ModeHeader
    {
        uint8_t*    headerPtr;
        size_t      headerLength;
        uint8_t*    dataPtr;
        size_t      dataLength;
    }__attribute__((packed, aligned(1)));

    // Copied from CineBuffer to make it standalone fileProcessor
    typedef struct __attribute__((packed, aligned(1)))
    {
        uint32_t    frameNum;   // Required - Frame number from the Dau
        uint64_t    timeStamp;  // Required - Timestamp of aquisition from the Dau
        uint32_t    numSamples; // Optional - Parameter added by image processing
                                // stage to indicate number of samples in the frame.
        float       heartRate;  // Optional - heartRate
        uint32_t    statusCode; // Optional
    } CineFrameHeader;

    typedef struct __attribute__((packed, aligned(1)))
    {
        uint32_t    frameNum;   // Required - Frame number from the Dau
        uint64_t    timeStamp;  // Required - Timestamp of aquisition from the Dau
        uint32_t    numSamples; // Optional - Parameter added by image processing
        // stage to indicate number of samples in the frame.
    } CineFrameHeaderV3;

private:
    ThorStatus      processModeHeaderCurrent();
    ThorStatus      processModeHeaderV4();
    ThorStatus      processModeHeaderV3();

    bool            isTextureFrame(uint8_t* inputPtr);
    bool            isScreenFrame(uint8_t* inputPtr);

private:
    uint8_t*        mDataPtr;
    uint32_t        mDataTypes;
    uint32_t        mFrameCount;
    uint8_t         mVersion;

    ModeHeader     mModeHeader[NUM_DATA_TYPE];
};
