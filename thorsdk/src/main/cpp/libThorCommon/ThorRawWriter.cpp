//
// Copyright 2018 EchoNous Inc.
//
//

#define LOG_TAG "ThorRawWriter"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <time.h>
#include <ThorUtils.h>
#include <BitfieldMacros.h>
#include <ThorRawWriter.h>

//-----------------------------------------------------------------------------
ThorRawWriter::ThorRawWriter() :
    mFd(-1),
    mDataTypes(0),
    mFrameCount(0),
    mDataPtr(nullptr),
    mMapSize(0)
{
}

//-----------------------------------------------------------------------------
ThorRawWriter::~ThorRawWriter()
{
    close();
}

//-----------------------------------------------------------------------------
ThorStatus ThorRawWriter::create(const char* path, 
                                 ThorRawDataFile::Metadata& metadata)
{
    ThorStatus  retVal = TER_MISC_CREATE_FILE_FAILED;

    mFd = ::open(path, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

    if (-1 == mFd)
    {
        ALOGE("Unable to create raw data file at path: %s", path);
        ALOGE("errno = %d", errno);
        goto err_ret;
    }

    mDataTypes = metadata.dataTypes;
    mFrameCount = metadata.frameCount;

    retVal = createHeader(metadata);

err_ret:
    return(retVal);
}

//-----------------------------------------------------------------------------
void ThorRawWriter::close()
{
    if (nullptr != mDataPtr)
    {
        msync(mDataPtr, mMapSize, MS_SYNC);
        munmap(mDataPtr, mMapSize);
        mDataPtr = nullptr;
        mMapSize = 0;
    }

    if (-1 != mFd)
    {
        ::fsync(mFd);
        ::close(mFd);
        mFd = -1;
    }
}

//-----------------------------------------------------------------------------
ThorStatus ThorRawWriter::writeData(WriterCallback& callback)
{
    ThorStatus  retVal = THOR_ERROR;

    if (-1 == mFd)
    {
        ALOGE("Cannot write data without first creating a file");
        retVal = TER_MISC_FILE_NOT_OPEN;
        goto err_ret; 
    }

    if (0 != BF_GET(mDataTypes, ThorRawDataFile::DataType::BMode, 1))
    {
        retVal = writeData(ThorRawDataFile::DataType::BMode, callback);
        if (IS_THOR_ERROR(retVal))
        {
            goto err_ret;
        }
    }

    if (0 != BF_GET(mDataTypes, ThorRawDataFile::DataType::CMode, 1))
    {
        retVal = writeData(ThorRawDataFile::DataType::CMode, callback);
        if (IS_THOR_ERROR(retVal))
        {
            goto err_ret;
        }
    }

    if (0 != BF_GET(mDataTypes, ThorRawDataFile::DataType::PW, 1))
    {
        retVal = writeData(ThorRawDataFile::DataType::PW, callback);
        if (IS_THOR_ERROR(retVal))
        {
            goto err_ret;
        }
    }

    if (0 != BF_GET(mDataTypes, ThorRawDataFile::DataType::MMode, 1))
    {
        retVal = writeData(ThorRawDataFile::DataType::MMode, callback);
        if (IS_THOR_ERROR(retVal))
        {
            goto err_ret;
        }
    }

    if (0 != BF_GET(mDataTypes, ThorRawDataFile::DataType::CW, 1))
    {
        retVal = writeData(ThorRawDataFile::DataType::CW, callback);
        if (IS_THOR_ERROR(retVal))
        {
            goto err_ret;
        }
    }

    if (0 != BF_GET(mDataTypes, ThorRawDataFile::DataType::ECG, 1))
    {
        retVal = writeData(ThorRawDataFile::DataType::ECG, callback);
        if (IS_THOR_ERROR(retVal))
        {
            goto err_ret;
        }
    }

    if (0 != BF_GET(mDataTypes, ThorRawDataFile::DataType::Auscultation, 1))
    {
        retVal = writeData(ThorRawDataFile::DataType::Auscultation, callback);
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
uint8_t* ThorRawWriter::getDataPtr(uint32_t dataSize)
{
    uint8_t*    retPtr = nullptr;
    int         headerSize = sizeof(ThorRawDataFile::Identification) +
                             sizeof(ThorRawDataFile::Metadata);

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

    mMapSize = headerSize + dataSize;

    if (ftruncate(mFd, mMapSize))
    {
        ALOGE("Unable to grow file to needed length\n");
        goto err_ret;
    }
    ::lseek(mFd, 0, SEEK_SET);

    mDataPtr = (uint8_t*) mmap(0,
                               mMapSize,
                               PROT_WRITE | PROT_READ,
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
ThorStatus ThorRawWriter::updateMetadata(ThorRawDataFile::Metadata& metadata)
{
    ThorStatus  retVal = THOR_ERROR;
    off_t       metadataOffset = sizeof(ThorRawDataFile::Identification) +
                                 sizeof(ThorRawDataFile::Metadata::timestamp);

    if (-1 == mFd)
    {
        ALOGE("Cannot write data without first creating a file");
        retVal = TER_MISC_FILE_NOT_OPEN;
        goto err_ret; 
    }

    ::lseek(mFd, metadataOffset, SEEK_SET);
    retVal = writeMetadata(metadata);

err_ret:
    return(retVal); 
}

//-----------------------------------------------------------------------------
ThorStatus ThorRawWriter::createHeader(ThorRawDataFile::Metadata& metadata)
{
    ThorStatus  retVal = THOR_ERROR;
    uint32_t    tag = THOR_RAW_TAG;
    uint8_t     version = THOR_RAW_VERSION;
    uint8_t     reserved[3] = { 0x0, 0x0, 0x0 };
    uint64_t    timestamp;

    //
    // Identification
    //
    ::write(mFd, &tag, sizeof(tag));
    ::write(mFd, &version, sizeof(version));
    ::write(mFd, reserved, sizeof(reserved));

    //
    // Metadata
    //
    timestamp = time(nullptr);
    ::write(mFd, &timestamp, sizeof(timestamp));

    retVal = writeMetadata(metadata);

    return(retVal); 
}

//-----------------------------------------------------------------------------
ThorStatus ThorRawWriter::writeMetadata(ThorRawDataFile::Metadata& metadata)
{
    ::write(mFd, &metadata.probeType, sizeof(metadata.probeType));
    ::write(mFd, &metadata.imageCaseId, sizeof(metadata.imageCaseId));
    ::write(mFd, &metadata.upsMajorVer, sizeof(metadata.upsMajorVer));
    ::write(mFd, &metadata.upsMinorVer, sizeof(metadata.upsMinorVer));
    ::write(mFd, &metadata.frameCount, sizeof(metadata.frameCount));
    ::write(mFd, &metadata.frameInterval, sizeof(metadata.frameInterval));
    ::write(mFd, &metadata.dataTypes, sizeof(metadata.dataTypes));

    return(THOR_OK); 
}

//-----------------------------------------------------------------------------
ThorStatus ThorRawWriter::writeData(ThorRawDataFile::DataType dataType,
                                    WriterCallback& callback)
{
    uint32_t    frameNum;
    uint32_t    length;
    uint8_t*    dataPtr;
    ssize_t     bytesWritten;
    ThorStatus  retVal = THOR_OK;

    for (frameNum = 0; frameNum < mFrameCount; frameNum++)
    {
        length = callback.onData(dataType, frameNum, &dataPtr);
        if (length > 0)
        {
            bytesWritten = ::write(mFd, dataPtr, length);
            if ( (uint32_t) bytesWritten != length)
            {
                ALOGE("Write failed.  errno = %d", errno);
                retVal = TER_MISC_WRITE_FAILED;
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

