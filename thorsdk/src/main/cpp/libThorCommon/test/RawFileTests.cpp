#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <gtest/gtest.h>

#include <BitfieldMacros.h>
#include <ThorRawWriter.h>
#include <ThorRawReader.h>

namespace android
{

const char*     testDir = "/data/echonous";
const char*     testFile = "/data/echonous/rawtest.thor";

const uint32_t  FrameCount = 4;
const uint32_t  BFrameSize = 512;
const uint32_t  CFrameSize = 128;

uint8_t     bDataWrite[FrameCount * BFrameSize];
uint8_t     cDataWrite[FrameCount * CFrameSize];

uint8_t     bDataRead[FrameCount * BFrameSize];
uint8_t     cDataRead[FrameCount * CFrameSize];


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
            frameSize = BFrameSize;
            *dataPtrPtr = &bDataWrite[frameNum * BFrameSize];
        }
        else if (ThorRawDataFile::DataMode::CMode == mode)
        {
            frameSize = CFrameSize;
            *dataPtrPtr = &cDataWrite[frameNum * CFrameSize];
        }

        return(frameSize);
    }
};

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
            frameSize = BFrameSize;
            *dataPtrPtr = &bDataRead[frameNum * BFrameSize];
        }
        else if (ThorRawDataFile::DataMode::CMode == mode)
        {
            frameSize = CFrameSize;
            *dataPtrPtr = &cDataRead[frameNum * CFrameSize];
        }

        return(frameSize);
    }
};

//-------------------------------------------------------------------------
TEST(RawFileTest, Setup)
{
    int             ret;
    struct stat     fileStat;

    memset(bDataWrite, 0xBB, FrameCount * BFrameSize);
    memset(cDataWrite, 0xCC, FrameCount * CFrameSize);

    memset(bDataRead, 0x0, FrameCount * BFrameSize);
    memset(cDataRead, 0x0, FrameCount * CFrameSize);

    // Check for test directory and create if necessary
    ret = stat(testDir, &fileStat);
    if (0 == ret)
    {
        ASSERT_TRUE(S_IFDIR & fileStat.st_mode);
    }
    else
    {
        ASSERT_TRUE(0 == mkdir(testDir, S_IRWXU | S_IRWXG | S_IRWXO));
    }
}

//-------------------------------------------------------------------------
TEST(RawFileTest, WriteTest)
{
    ThorRawWriter               writer;
    ThorRawDataFile::Metadata   metadata;
    TestWriterCallback          callback;

    metadata.probeType = 0;
    metadata.imageCaseId = 1;
    metadata.upsMajorVer = 2;
    metadata.upsMinorVer = 3;
    metadata.frameCount = FrameCount;
    metadata.dataModes = 0;

    metadata.dataModes = BIT(ThorRawDataFile::DataMode::BMode) |
                         BIT(ThorRawDataFile::DataMode::CMode);

    ASSERT_TRUE(THOR_OK == writer.create(testFile, metadata));
    writer.writeData(callback);
    writer.close();
}

//-------------------------------------------------------------------------
TEST(RawFileTest, ReadTest)
{
    ThorRawReader               reader;
    ThorRawDataFile::Metadata   metadata;
    TestReaderCallback          callback;

    ASSERT_TRUE(THOR_OK == reader.open(testFile, metadata));
    reader.readData(callback);
    reader.close();
}

//-------------------------------------------------------------------------
TEST(RawFileTest, CompareTest)
{
    EXPECT_TRUE(0 == memcmp(bDataWrite, bDataRead, FrameCount * BFrameSize));
    EXPECT_TRUE(0 == memcmp(cDataWrite, cDataRead, FrameCount * CFrameSize));
}

}
