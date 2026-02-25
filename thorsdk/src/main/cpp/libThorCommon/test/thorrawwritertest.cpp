//
// Copyright (C) 2018 EchoNous, Inc.
//

#include <stdio.h>
#include <string.h>
#include <BitfieldMacros.h>
#include <ThorRawWriter.h>
#include <ThorConstants.h>

#define ENABLE_ELAPSED_TIME
#define LOG_TAG "thorrawwritertest"
#include <ElapsedTime.h>

const uint32_t  FrameCount = 900;
const uint32_t  BFrameSize = MAX_B_FRAME_SIZE;
const uint32_t  CFrameSize = MAX_COLOR_FRAME_SIZE;

uint8_t     bData[FrameCount * BFrameSize];
uint8_t     cData[FrameCount * CFrameSize];


//-----------------------------------------------------------------------------
class TestWriterCallback : public ThorRawWriter::WriterCallback
{
public: // Methods
    TestWriterCallback() : ThorRawWriter::WriterCallback(nullptr) {}

    uint32_t    onData(uint8_t mode,
                       uint32_t frameNum,
                       uint8_t** dataPtrPtr)
    {
        uint32_t    frameSize = 0;

        if (ThorRawDataFile::DataMode::BMode == mode)
        {
            printf("Writing B Data frame %u\n", frameNum);
            frameSize = BFrameSize;
            *dataPtrPtr = &bData[frameNum * BFrameSize];
        }
        else if (ThorRawDataFile::DataMode::CMode == mode)
        {
            printf("Writing C Data frame %u\n", frameNum);
            frameSize = CFrameSize;
            *dataPtrPtr = &cData[frameNum * CFrameSize];
        }

        return(frameSize);
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
    ThorRawWriter               writer;
    ThorRawDataFile::Metadata   metadata;
    TestWriterCallback          callback;
    bool                        useSequentialWrite = false;

    metadata.probeType = 0;
    metadata.imageCaseId = 1;
    metadata.upsMajorVer = 2;
    metadata.upsMinorVer = 3;
    metadata.frameCount = FrameCount;
    metadata.dataModes = 0;

    metadata.dataModes = BIT(ThorRawDataFile::DataMode::BMode) |
                         BIT(ThorRawDataFile::DataMode::CMode);

    if (writer.create(filePath, metadata) == THOR_OK)
    {
        printf("Probe Type: %d\n", metadata.probeType);
        printf("Image Case ID: %u\n", metadata.imageCaseId);
        printf("UPS Version: %d.%d\n", metadata.upsMajorVer, metadata.upsMinorVer);
        printf("Frame Count: %u\n", metadata.frameCount);
        printf("Data Modes: 0x%X\n", metadata.dataModes);

        if (useSequentialWrite)
        {
            memset(bData, 0xBB, FrameCount * BFrameSize);
            memset(cData, 0xCC, FrameCount * CFrameSize);

            if (THOR_OK != writer.writeData(callback))
            {
                printf("write failed\n");
                goto err_ret;
            }
        }
        else
        {
            uint8_t*    destAddressPtr = nullptr;

            destAddressPtr = writer.getDataPtr(FrameCount * (BFrameSize + CFrameSize));
            if (nullptr == destAddressPtr)
            {
                printf("getDataPtr() failed\n");
                goto err_ret;
            }
            memset(destAddressPtr, 0xBB, FrameCount * BFrameSize);
            destAddressPtr += FrameCount * BFrameSize;
            memset(destAddressPtr, 0xCC, FrameCount * CFrameSize);
        }

        writer.close();
    }
    else
    {
        printf("Failed to create %s\n", filePath);
        goto err_ret;
    }

    printf("write completed\n");
    retVal = 0;

err_ret:
    return(retVal);
}
