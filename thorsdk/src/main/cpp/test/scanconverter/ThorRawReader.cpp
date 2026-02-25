//
// Copyright 2018 EchoNous Inc.
//
//

#define LOG_TAG "ThorRawReader"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include "BitfieldMacros.h"
#include "ThorRawReader.h"


//-----------------------------------------------------------------------------
ThorRawReader::ThorRawReader() :
    mFd(-1),
    mDataTypes(0),
    mFrameCount(0),
    mDataPtr(nullptr),
    mMapSize(0),
    mVersion(0)
{
}

//-----------------------------------------------------------------------------
ThorRawReader::~ThorRawReader()
{
    close();
}

//-----------------------------------------------------------------------------
ThorStatus ThorRawReader::open(const char* path, ThorRawDataFile::Metadata& metadata)
{
    ThorStatus  retVal = TER_MISC_OPEN_FILE_FAILED;

    mFd = ::open(path, O_RDONLY);

    if (-1 == mFd)
    {
        ALOGE("Unable to open raw data file at path: %s", path);
        ALOGE("errno = %d", errno);
        goto err_ret;
    }

    retVal = readHeader(metadata);
    if (IS_THOR_ERROR(retVal))
    {
        close();
        goto err_ret;
    }

    mDataTypes = metadata.dataTypes;
    mFrameCount = metadata.frameCount;

err_ret:
    return(retVal);
}

//-----------------------------------------------------------------------------
void ThorRawReader::close()
{
    if (nullptr != mDataPtr)
    {
        munmap(mDataPtr, mMapSize);
        mDataPtr = nullptr;
        mMapSize = 0;
    }

    if (-1 != mFd)
    {
        ::close(mFd);
        mFd = -1;
    }
}

//-----------------------------------------------------------------------------
ThorStatus ThorRawReader::readData(ReaderCallback& callback)
{
    ThorStatus  retVal = THOR_ERROR;

    if (-1 == mFd)
    {
        ALOGE("Cannot read data without first opening a file");
        retVal = TER_MISC_FILE_NOT_OPEN;
        goto err_ret;
    }

    if (0 != BF_GET(mDataTypes, ThorRawDataFile::DataType::BMode, 1))
    {
        retVal = readData(ThorRawDataFile::DataType::BMode, callback);
        if (IS_THOR_ERROR(retVal))
        {
            goto err_ret;
        }
    }

    if (0 != BF_GET(mDataTypes, ThorRawDataFile::DataType::CMode, 1))
    {
        retVal = readData(ThorRawDataFile::DataType::CMode, callback);
        if (IS_THOR_ERROR(retVal))
        {
            goto err_ret;
        }
    }

    if (0 != BF_GET(mDataTypes, ThorRawDataFile::DataType::PW, 1))
    {
        retVal = readData(ThorRawDataFile::DataType::PW, callback);
        if (IS_THOR_ERROR(retVal))
        {
            goto err_ret;
        }
    }

    if (0 != BF_GET(mDataTypes, ThorRawDataFile::DataType::MMode, 1))
    {
        retVal = readData(ThorRawDataFile::DataType::MMode, callback);
        if (IS_THOR_ERROR(retVal))
        {
            goto err_ret;
        }
    }

    if (0 != BF_GET(mDataTypes, ThorRawDataFile::DataType::CW, 1))
    {
        retVal = readData(ThorRawDataFile::DataType::CW, callback);
        if (IS_THOR_ERROR(retVal))
        {
            goto err_ret;
        }
    }

    if (0 != BF_GET(mDataTypes, ThorRawDataFile::DataType::ECG, 1))
    {
        retVal = readData(ThorRawDataFile::DataType::ECG, callback);
        if (IS_THOR_ERROR(retVal))
        {
            goto err_ret;
        }
    }

    if (0 != BF_GET(mDataTypes, ThorRawDataFile::DataType::Auscultation, 1))
    {
        retVal = readData(ThorRawDataFile::DataType::Auscultation, callback);
        if (IS_THOR_ERROR(retVal))
        {
            goto err_ret;
        }
    }

    retVal = THOR_OK;

err_ret:
    return(retVal);
}

//-----------------------------------------------------------------------------
uint8_t* ThorRawReader::getDataPtr()
{
    uint8_t*    retPtr = nullptr;
    int         ret;
    int         headerSize = sizeof(ThorRawDataFile::Identification) +
                             sizeof(ThorRawDataFile::Metadata);
    struct stat statBuf;

    if (-1 == mFd)
    {
        ALOGE("Cannot write data without first creating a file");
        goto err_ret;
    }

    if (nullptr != mDataPtr)
    {
        retPtr = mDataPtr + headerSize;
        goto ok_ret;
    }

    ret = fstat(mFd, &statBuf);
    if (ret)
    {
        ALOGE("Unable to get size of file\n");
        goto err_ret;
    }
    ::lseek(mFd, 0, SEEK_SET);

    mMapSize = statBuf.st_size;

    mDataPtr = (uint8_t*) mmap(0,
                               mMapSize,
                               PROT_READ,
                               MAP_SHARED,
                               mFd,
                               0);
    if ((void*) -1 == mDataPtr)
    {
        ALOGD("Failed to map data file. errno = %d\n", errno);
        mDataPtr = nullptr;
        goto err_ret;
    }

    retPtr = mDataPtr + headerSize;

ok_ret:
err_ret:
    return(retPtr);
}

//-----------------------------------------------------------------------------
ThorStatus ThorRawReader::readHeader(ThorRawDataFile::Metadata& metadata)
{
    ThorStatus      retVal = THOR_ERROR;
    struct stat     fileStat;
    int             ret;
    uint32_t        tag;
    uint8_t         reserved[3] = { 0x0, 0x0, 0x0 };

    // Verify file length is at least long enough to hold the header
    ret = fstat(mFd, &fileStat);
    if (-1 == ret)
    {
        ALOGE("fstat failed.  errno = %d", errno);
        goto err_ret;
    }

    if ((sizeof(ThorRawDataFile::Identification) +
         sizeof(ThorRawDataFile::Metadata)) > (uint32_t) fileStat.st_size)
    {
        ALOGE("Invalid file: Missing header");
        retVal = TER_MISC_INVALID_FILE;
        goto err_ret;
    }


    //
    // Identification
    //
    ::read(mFd, &tag, sizeof(tag));
    if (THOR_RAW_TAG != tag)
    {
        ALOGE("Invalid file: Missing tag");
        retVal = TER_MISC_INVALID_FILE;
        goto err_ret;
    }

    ::read(mFd, &mVersion, sizeof(mVersion));
    if (mVersion < 0x03)
    {
        ALOGE("Invalid file: Unsupported version - %d", mVersion);
        retVal = TER_MISC_INVALID_FILE;
        goto err_ret;
    }

    ::read(mFd, &reserved, sizeof(reserved));


    //
    // Metadata
    //
    ::read(mFd, &metadata.timestamp, sizeof(metadata.timestamp));
    ::read(mFd, &metadata.probeType, sizeof(metadata.probeType));
    ::read(mFd, &metadata.imageCaseId, sizeof(metadata.imageCaseId));
    ::read(mFd, &metadata.upsMajorVer, sizeof(metadata.upsMajorVer));
    ::read(mFd, &metadata.upsMinorVer, sizeof(metadata.upsMinorVer));
    ::read(mFd, &metadata.frameCount, sizeof(metadata.frameCount));
    ::read(mFd, &metadata.frameInterval, sizeof(metadata.frameInterval));
    ::read(mFd, &metadata.dataTypes, sizeof(metadata.dataTypes));

    retVal = THOR_OK;

err_ret:
    return(retVal);
}

//-----------------------------------------------------------------------------
ThorStatus ThorRawReader::readData(ThorRawDataFile::DataType dataType,
                                   ReaderCallback& callback)
{
    uint32_t    frameNum;
    uint32_t    length;
    uint8_t*    dataPtr;
    ssize_t     bytesRead;
    ThorStatus  retVal = THOR_OK;

    for (frameNum = 0; frameNum < mFrameCount; frameNum++)
    {
        length = callback.onData(dataType, frameNum, &dataPtr);
        if (length > 0)
        {
            bytesRead = ::read(mFd, dataPtr, length);
            if ( (uint32_t) bytesRead != length)
            {
                ALOGE("File is too short.");
                retVal = TER_MISC_INVALID_FILE;
                break;
            }
        }
        else
        {
            // Bail if callback returns no data
            ALOGE("File operation aborted");
            retVal = TER_MISC_ABORT;
            break;
        }
    }

    return(retVal);
}
