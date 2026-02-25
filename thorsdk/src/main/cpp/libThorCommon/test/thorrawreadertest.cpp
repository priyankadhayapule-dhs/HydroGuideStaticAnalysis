//
// Copyright (C) 2018 EchoNous, Inc.
//

#define LOG_TAG "thorrawreadertest"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <BitfieldMacros.h>
#include <ThorUtils.h>
#include <ThorRawReader.h>
#include <ThorConstants.h>

#define ENABLE_ELAPSED_TIME
#include <ElapsedTime.h>

const uint32_t  FrameCount = 900;
const uint32_t  BFrameSize = MAX_B_FRAME_SIZE;
const uint32_t  CFrameSize = MAX_COLOR_FRAME_SIZE;

uint8_t     bData[FrameCount * BFrameSize];
uint8_t     cData[FrameCount * CFrameSize];


//-----------------------------------------------------------------------------
void printBuffer(uint8_t* bufPtr, uint32_t bufSize)
{
    const uint32_t  numElements = 8;  // Elements per row
    uint32_t        index;
    unsigned        pos;

    for (index = 0; index < bufSize; index += numElements)
    {
        printf("0X%016zX: ", (size_t) bufPtr + index);

        for (pos = 0; pos < MIN(numElements, (bufSize - index)); pos++)
        {
            printf("0x%02X  ", *(bufPtr + index + pos));
        }
        printf("\n");
    }
}

//-----------------------------------------------------------------------------
class TestReaderCallback : public ThorRawReader::ReaderCallback
{
public: // Methods
    TestReaderCallback() : ThorRawReader::ReaderCallback(nullptr) {}

    uint32_t    onData(uint8_t mode,
                       uint32_t frameNum,
                       uint8_t** dataPtrPtr)
    {
        uint32_t    frameSize = 0;

        if (ThorRawDataFile::DataMode::BMode == mode)
        {
            printf("Reading B Data frame %u\n", frameNum);
            frameSize = BFrameSize;
            *dataPtrPtr = &bData[frameNum * BFrameSize];
        }
        else if (ThorRawDataFile::DataMode::CMode == mode)
        {
            printf("Reading C Data frame %u\n", frameNum);
            frameSize = CFrameSize;
            *dataPtrPtr = &cData[frameNum * CFrameSize];
        }

        return(frameSize + 500);
    }
};


//-----------------------------------------------------------------------------
int main(int argc, char** argv)
{
    UNUSED(argc);
    UNUSED(argv);
    ELAPSED_FUNC();

    int                         retVal = -1;
    const char*                 filePath = "/data/echonous/test.thor";
    ThorRawReader               reader;
    ThorRawDataFile::Metadata   metadata;
    TestReaderCallback          callback;
    bool                        useSequentialRead = false;

    memset(bData, 0x0, FrameCount * BFrameSize);
    memset(cData, 0x0, FrameCount * CFrameSize);

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
        printf("Data Modes: 0x%X\n", metadata.dataModes);
        printf("Data Payload length: %lu\n", sizeof(bData) + sizeof(cData));

        if (useSequentialRead)
        {
            if (THOR_OK != reader.readData(callback))
            {
                printf("read failed\n");
                goto err_ret;
            }

            //printf("\n\nB-Mode Data:\n\n");
            //printBuffer(bData, sizeof(bData));

            //printf("\n\nC-Mode Data:\n\n");
            //printBuffer(cData, sizeof(cData));
        }
        else
        {
            uint8_t*    addressPtr;

            addressPtr = reader.getDataPtr();
            if (nullptr == addressPtr)
            {
                printf("getDataPtr() failed\n");
                goto err_ret;
            }

            //printf("\n\nB-Mode Data:\n\n");
            //printBuffer(addressPtr, sizeof(bData));
            memcpy(bData, addressPtr, sizeof(bData));

            //printf("\n\nC-Mode Data:\n\n");
            addressPtr += sizeof(bData);
            //printBuffer(addressPtr, sizeof(cData));
            memcpy(cData, addressPtr, sizeof(cData));
        }

        reader.close();
    }
    else
    {
        printf("Failed to open %s\n", filePath);
        goto err_ret;
    }

    printf("read completed\n");
    retVal = 0;

err_ret:
    return(retVal);
}
