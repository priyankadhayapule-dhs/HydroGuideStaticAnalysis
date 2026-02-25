//
// Copyright (C) 2018 EchoNous, Inc.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <BitfieldMacros.h>
#include <ThorUtils.h>
#include <ThorRawReader.h>
#include <ThorConstants.h>
#include <ScanConverterParams.h>

typedef struct __attribute__((packed, aligned(1)))
{
    uint32_t    frameNum;
    uint64_t    timeStamp;
} FrameHeader;

const uint32_t  BFrameSize = MAX_B_FRAME_SIZE + sizeof(FrameHeader);
const uint32_t  CFrameSize = MAX_COLOR_FRAME_SIZE + sizeof(FrameHeader);

//-----------------------------------------------------------------------------
void printUsage()
{
    printf("Usage: rawfileinfo <file>\n");
    printf("\t<file>\tPathname of file to inspect.\n");
    exit(-1);
}

//-----------------------------------------------------------------------------
int main(int argc, char** argv)
{
    int                         retVal = -1;
    const char*                 filePath;
    ThorRawReader               reader;
    ThorRawDataFile::Metadata   metadata;
    FrameHeader*                frameHeaderPtr = nullptr;
    uint32_t                    frameCount;

    if (argc < 2)
    {
        printUsage();
        exit(-1);
    }

    filePath = argv[1];
    printf("Inspecting %s\n", filePath);

    if (reader.open(filePath, metadata) == THOR_OK)
    {
        uint64_t    timestamp = metadata.timestamp;
        struct tm*  now = localtime((time_t*) &timestamp);

        printf("Timestamp: %d:%d:%d  %d/%d/%d\n",
               now->tm_hour, now->tm_min, now->tm_sec,
               now->tm_mon + 1, now->tm_mday, now->tm_year + 1900);
        printf("Probe Type: %d\n", metadata.probeType);
        printf("Image Case ID: %u\n", metadata.imageCaseId);
        printf("UPS Version: %d.%d\n", metadata.upsMajorVer, metadata.upsMinorVer);
        printf("Frame Count: %u\n", metadata.frameCount);
        printf("Frame Interval (ms): %u\n", metadata.frameInterval);
        printf("Data Modes: 0x%X\n", metadata.dataModes);

        uint8_t*    addressPtr;

        addressPtr = reader.getDataPtr();
        if (nullptr == addressPtr)
        {
            printf("getDataPtr() failed\n");
            goto err_ret;
        }

        if (0 != BF_GET(metadata.dataModes, ThorRawDataFile::DataMode::BMode, 1))
        {
            printf("\nB-Mode Frames:\n");
            for (frameCount = 0; frameCount < MIN(30, metadata.frameCount); frameCount++)
            {
                frameHeaderPtr = (FrameHeader*) (addressPtr + (frameCount * BFrameSize));
                printf("\tFrame Number: %u    Timecode: %lu\n",
                       frameHeaderPtr->frameNum,
                       frameHeaderPtr->timeStamp);
            }

            addressPtr += metadata.frameCount * BFrameSize;
        }

        if (0 != BF_GET(metadata.dataModes, ThorRawDataFile::DataMode::CMode, 1))
        {
            ScanConverterParams*     paramsPtr = (ScanConverterParams*) addressPtr;

            printf("\nC-Mode metadata\n\n");
            // Color mode metadata
            printf("\tSample Mesh: %u\n", paramsPtr->numSampleMesh);
            printf("\tRay Mesh: %u\n", paramsPtr->numRayMesh);
            printf("\tSamples: %u\n", paramsPtr->numSamples);
            printf("\tRays: %u\n", paramsPtr->numRays);
            printf("\tStart Sample (mm): %f\n", paramsPtr->startSampleMm);
            printf("\tSample Spacing (mm): %f\n", paramsPtr->sampleSpacingMm);
            printf("\tRay Spacing: %f\n", paramsPtr->raySpacing);
            printf("\tStart Ray Radian: %f\n", paramsPtr->startRayRadian);

            addressPtr += sizeof(ScanConverterParams);

            // Color frame headers
            printf("\nC-Mode Frames:\n");
            for (frameCount = 0; frameCount < MIN(30, metadata.frameCount); frameCount++)
            {
                frameHeaderPtr = (FrameHeader*) (addressPtr + (frameCount * CFrameSize));
                printf("\tFrame Number: %u    Timecode: %lu\n",
                       frameHeaderPtr->frameNum,
                       frameHeaderPtr->timeStamp);
            }

            addressPtr += metadata.frameCount * CFrameSize;
        }


        reader.close();
    }
    else
    {
        printf("Failed to open %s\n", filePath);
        goto err_ret;
    }

    printf("\nread completed\n");
    retVal = 0;

err_ret:
    return(retVal);
}
