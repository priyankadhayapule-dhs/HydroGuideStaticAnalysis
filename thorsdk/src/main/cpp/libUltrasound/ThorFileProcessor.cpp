//
// Copyright 2019 EchoNous Inc.
//
//

#define LOG_TAG "ThorFileProcessor"

#include <string.h>
#include <ThorFileProcessor.h>
#include <CineBuffer.h>
#include <BitfieldMacros.h>
#include <BModeRecordParams.h>
#include <ColorModeRecordParams.h>
#include <ScrollModeRecordParams.h>
#include <DaEcgRecordParams.h>

//-----------------------------------------------------------------------------
ThorFileProcessor::ThorFileProcessor()
{
}

//-----------------------------------------------------------------------------
ThorFileProcessor::~ThorFileProcessor()
{
}

//-----------------------------------------------------------------------------
ThorStatus  ThorFileProcessor::getModeHeader(ThorRawDataFile::DataType dataType, void* headerPtr)
{
    ThorStatus retVal = THOR_ERROR;

    memcpy(headerPtr,
           mModeHeader[dataType].headerPtr,
           mModeHeader[dataType].headerLength);

    retVal = THOR_OK;

    return (retVal);
}

//-----------------------------------------------------------------------------
ThorStatus  ThorFileProcessor::getData(ThorRawDataFile::DataType dataType, void* dataPtr)
{
    ThorStatus retVal = THOR_ERROR;

    if (mVersion == THOR_RAW_VERSION)
    {
        // current version
        memcpy(dataPtr,
               mModeHeader[dataType].dataPtr,
               mModeHeader[dataType].dataLength);
    }
    else
    {
        retVal = getDataPreVer(dataType, dataPtr);
    }

    retVal = THOR_OK;

    return (retVal);
}

//-----------------------------------------------------------------------------
ThorStatus ThorFileProcessor::getDataPreVer(ThorRawDataFile::DataType dataType, void *dataPtr,
        int dataPtrFrameStride)
{
    ThorStatus retVal = THOR_ERROR;
    size_t frameStride;
    size_t frameHeaderSize;
    size_t frameStrideWithoutHeader;
    size_t dataFrameStrideWithoutHeader;
    size_t currentFrameHeaderSize;
    uint8_t*  curDataPtr;
    uint8_t*  readPtr;

    // frameHeaderSize - older;
    frameHeaderSize = getFrameHeaderSize();
    frameStride = mModeHeader[dataType].dataLength/mFrameCount;
    frameStrideWithoutHeader = frameStride - frameHeaderSize;

    currentFrameHeaderSize = sizeof(CineBuffer::CineFrameHeader);

    if (dataPtrFrameStride < 1)
        dataFrameStrideWithoutHeader = frameStrideWithoutHeader;
    else
        dataFrameStrideWithoutHeader = dataPtrFrameStride;

    // pointers
    curDataPtr = (uint8_t*) dataPtr;
    readPtr = mModeHeader[dataType].dataPtr;

    for (int i = 0; i < mFrameCount; i++)
    {
        // read frame Header
        memcpy(curDataPtr,
               readPtr,
               frameHeaderSize);

        curDataPtr += currentFrameHeaderSize;
        readPtr += frameHeaderSize;

        // read frame data
        memcpy(curDataPtr,
               readPtr,
               frameStrideWithoutHeader);

        curDataPtr += dataFrameStrideWithoutHeader;
        readPtr += frameStrideWithoutHeader;
    }

    retVal = THOR_OK;

    return (retVal);
}

//-----------------------------------------------------------------------------
void* ThorFileProcessor::getDataPtr(ThorRawDataFile::DataType dataType)
{
    return mModeHeader[dataType].dataPtr;
}

//-----------------------------------------------------------------------------
size_t ThorFileProcessor::getFrameHeaderSize()
{
    size_t headerSize = 0;

    switch (mVersion)
    {
        case 0x03:
            headerSize = sizeof(CineBuffer::CineFrameHeaderV3);
            break;
        case 0x04:
        default:
            headerSize = sizeof(CineBuffer::CineFrameHeader);
            break;
    }

    return headerSize;
}

//-----------------------------------------------------------------------------
ThorStatus ThorFileProcessor::processModeHeader(uint8_t *dataPtr, uint32_t dataTypes,
                                                uint32_t frameCount, uint8_t version)
{
    ThorStatus retVal;

    // dataPtr -> first B data address. (filePtr + header)
    mDataPtr = dataPtr;
    mDataTypes = dataTypes;
    mFrameCount = frameCount;
    mVersion = version;

    // initialize modeHeader
    for (int i = 0; i < NUM_DATA_TYPE; i++)
    {
        mModeHeader[i].headerPtr = nullptr;
        mModeHeader[i].headerLength = 0;
        mModeHeader[i].dataPtr = nullptr;
        mModeHeader[i].dataLength = 0;
    }

    switch (mVersion)
    {
        case 0x03:
            retVal = processModeHeaderV3();
            break;
        case 0x04:
            retVal = processModeHeaderV4();
            break;
        default:
            retVal = processModeHeaderCurrent();
            break;
    }

    return(retVal);
}

//-----------------------------------------------------------------------------
ThorStatus ThorFileProcessor::processModeHeaderCurrent()
{
    // this should be called after getDataPtr is invoked.

    uint8_t*        addressPtr;
    ThorStatus      retVal = THOR_ERROR;
    size_t          cineFrameHeaderSize;

    ThorRawDataFile::DataType dataType;

    if (nullptr == mDataPtr)
    {
        ALOGE("DataPtr cannot be null");
        goto err_ret;
    }

    // cineFrameHeaderSize
    cineFrameHeaderSize = getFrameHeaderSize();

    // beginning of B-mode data --> dataPtr
    addressPtr = mDataPtr;

    // B-mode Mode Header
    if (0 != BF_GET(mDataTypes, ThorRawDataFile::DataType::BMode, 1))
    {
        dataType = ThorRawDataFile::DataType::BMode;
        mModeHeader[dataType].headerPtr = addressPtr;
        mModeHeader[dataType].headerLength = sizeof(BModeRecordParams);
        mModeHeader[dataType].dataPtr = addressPtr + mModeHeader[dataType].headerLength;
        mModeHeader[dataType].dataLength = mFrameCount * (MAX_B_FRAME_SIZE + cineFrameHeaderSize);

        addressPtr += mModeHeader[dataType].headerLength + mModeHeader[dataType].dataLength;
    }

    // Get C-Mode scan converter params
    if (0 != BF_GET(mDataTypes, ThorRawDataFile::DataType::CMode, 1))
    {
        dataType = ThorRawDataFile::DataType::CMode;
        mModeHeader[dataType].headerPtr = addressPtr;
        mModeHeader[dataType].headerLength = sizeof(ColorModeRecordParams);
        mModeHeader[dataType].dataPtr = addressPtr + mModeHeader[dataType].headerLength;
        mModeHeader[dataType].dataLength = mFrameCount * (MAX_COLOR_FRAME_SIZE + cineFrameHeaderSize);

        addressPtr += mModeHeader[dataType].headerLength + mModeHeader[dataType].dataLength;
    }

    // Get M-Mode scroll Params
    if (0 != BF_GET(mDataTypes, ThorRawDataFile::DataType::MMode, 1))
    {
        dataType = ThorRawDataFile::DataType::MMode;
        mModeHeader[dataType].headerPtr = addressPtr;
        mModeHeader[dataType].headerLength = sizeof(MModeRecordParams);
        mModeHeader[dataType].dataPtr = addressPtr + mModeHeader[dataType].headerLength;

        if (isTextureFrame(addressPtr))
        {
            mModeHeader[dataType].dataLength = MAX_TEX_FRAME_SIZE + cineFrameHeaderSize;
        }
        else
        {
            mModeHeader[dataType].dataLength = mFrameCount * (MAX_M_FRAME_SIZE + cineFrameHeaderSize);
        }
        addressPtr += mModeHeader[dataType].headerLength + mModeHeader[dataType].dataLength;
    }

    // Get Da record Params
    if (0 != BF_GET(mDataTypes, ThorRawDataFile::DataType::Auscultation, 1))
    {
        dataType = ThorRawDataFile::DataType::Auscultation;
        mModeHeader[dataType].headerPtr = addressPtr;
        mModeHeader[dataType].headerLength = sizeof(DaEcgRecordParams);
        mModeHeader[dataType].dataPtr = addressPtr + mModeHeader[dataType].headerLength;

        if (isScreenFrame(addressPtr))
        {
            mModeHeader[dataType].dataLength = MAX_DA_SCR_FRAME_SIZE + cineFrameHeaderSize;
        }
        else
        {
            mModeHeader[dataType].dataLength = mFrameCount * (MAX_DA_FRAME_SIZE_LEGACY + cineFrameHeaderSize);
        }
        addressPtr += mModeHeader[dataType].headerLength + mModeHeader[dataType].dataLength;
    }

    // Get Ecg record Params
    if (0 != BF_GET(mDataTypes, ThorRawDataFile::DataType::ECG, 1))
    {
        dataType = ThorRawDataFile::DataType::ECG;
        mModeHeader[dataType].headerPtr = addressPtr;
        mModeHeader[dataType].headerLength = sizeof(DaEcgRecordParams);
        mModeHeader[dataType].dataPtr = addressPtr + mModeHeader[dataType].headerLength;

        if (isScreenFrame(addressPtr))
        {
            mModeHeader[dataType].dataLength = MAX_ECG_SCR_FRAME_SIZE + cineFrameHeaderSize;
        }
        else
        {
            mModeHeader[dataType].dataLength = mFrameCount * (MAX_ECG_FRAME_SIZE + cineFrameHeaderSize);
        }
    }

    retVal = THOR_OK;

err_ret:
    return(retVal);
}

//-----------------------------------------------------------------------------
ThorStatus ThorFileProcessor::processModeHeaderV4()
{
    // this should be called after getDataPtr is invoked.

    uint8_t*        addressPtr;
    ThorStatus      retVal = THOR_ERROR;
    size_t          cineFrameHeaderSize;

    ThorRawDataFile::DataType dataType;

    if (nullptr == mDataPtr)
    {
        ALOGE("DataPtr cannot be null");
        goto err_ret;
    }

    // cineFrameHeaderSize
    cineFrameHeaderSize = getFrameHeaderSize();

    // beginning of B-mode data --> dataPtr
    addressPtr = mDataPtr;

    // B-mode Mode Header
    if (0 != BF_GET(mDataTypes, ThorRawDataFile::DataType::BMode, 1))
    {
        dataType = ThorRawDataFile::DataType::BMode;
        mModeHeader[dataType].headerPtr = addressPtr;
        mModeHeader[dataType].headerLength = 0;
        mModeHeader[dataType].dataPtr = addressPtr;
        mModeHeader[dataType].dataLength = mFrameCount * (MAX_B_FRAME_SIZE + cineFrameHeaderSize);

        addressPtr += mModeHeader[dataType].dataLength;
    }

    // Get C-Mode scan converter params
    if (0 != BF_GET(mDataTypes, ThorRawDataFile::DataType::CMode, 1))
    {
        dataType = ThorRawDataFile::DataType::CMode;
        mModeHeader[dataType].headerPtr = addressPtr;
        mModeHeader[dataType].headerLength = sizeof(ColorModeRecordParams);
        mModeHeader[dataType].dataPtr = addressPtr + mModeHeader[dataType].headerLength;
        mModeHeader[dataType].dataLength = mFrameCount * (MAX_COLOR_FRAME_SIZE + cineFrameHeaderSize);

        addressPtr += mModeHeader[dataType].headerLength + mModeHeader[dataType].dataLength;
    }

    // Get M-Mode scroll Params
    if (0 != BF_GET(mDataTypes, ThorRawDataFile::DataType::MMode, 1))
    {
        dataType = ThorRawDataFile::DataType::MMode;
        mModeHeader[dataType].headerPtr = addressPtr;
        mModeHeader[dataType].headerLength = sizeof(MModeRecordParams);
        mModeHeader[dataType].dataPtr = addressPtr + mModeHeader[dataType].headerLength;

        if (isTextureFrame(addressPtr))
        {
            mModeHeader[dataType].dataLength = MAX_TEX_FRAME_SIZE + cineFrameHeaderSize;
        }
        else
        {
            mModeHeader[dataType].dataLength = mFrameCount * (MAX_M_FRAME_SIZE + cineFrameHeaderSize);
        }
        addressPtr += mModeHeader[dataType].headerLength + mModeHeader[dataType].dataLength;
    }

    // Get Da record Params
    if (0 != BF_GET(mDataTypes, ThorRawDataFile::DataType::Auscultation, 1))
    {
        dataType = ThorRawDataFile::DataType::Auscultation;
        mModeHeader[dataType].headerPtr = addressPtr;
        mModeHeader[dataType].headerLength = sizeof(DaEcgRecordParams);
        mModeHeader[dataType].dataPtr = addressPtr + mModeHeader[dataType].headerLength;

        if (isScreenFrame(addressPtr))
        {
            mModeHeader[dataType].dataLength = MAX_DA_SCR_FRAME_SIZE + cineFrameHeaderSize;
        }
        else
        {
            mModeHeader[dataType].dataLength = mFrameCount * (MAX_DA_FRAME_SIZE_LEGACY + cineFrameHeaderSize);
        }
        addressPtr += mModeHeader[dataType].headerLength + mModeHeader[dataType].dataLength;
    }

    // Get Ecg record Params
    if (0 != BF_GET(mDataTypes, ThorRawDataFile::DataType::ECG, 1))
    {
        dataType = ThorRawDataFile::DataType::ECG;
        mModeHeader[dataType].headerPtr = addressPtr;
        mModeHeader[dataType].headerLength = sizeof(DaEcgRecordParams);
        mModeHeader[dataType].dataPtr = addressPtr + mModeHeader[dataType].headerLength;

        if (isScreenFrame(addressPtr))
        {
            mModeHeader[dataType].dataLength = MAX_ECG_SCR_FRAME_SIZE + cineFrameHeaderSize;
        }
        else
        {
            mModeHeader[dataType].dataLength = mFrameCount * (MAX_ECG_FRAME_SIZE + cineFrameHeaderSize);
        }
    }

    retVal = THOR_OK;

err_ret:
    return(retVal);
}

//-----------------------------------------------------------------------------
ThorStatus ThorFileProcessor::processModeHeaderV3()
{
    // this should be called after getDataPtr is invoked.

    uint8_t*        addressPtr;
    ThorStatus      retVal = THOR_ERROR;
    size_t          cineFrameHeaderSize;

    ThorRawDataFile::DataType dataType;

    if (nullptr == mDataPtr)
    {
        ALOGE("DataPtr cannot be null");
        goto err_ret;
    }

    // cineFrameHeaderSize
    cineFrameHeaderSize = getFrameHeaderSize();

    // beginning of B-mode data --> dataPtr
    addressPtr = mDataPtr;

    // B-mode Mode Header
    if (0 != BF_GET(mDataTypes, ThorRawDataFile::DataType::BMode, 1))
    {
        dataType = ThorRawDataFile::DataType::BMode;
        mModeHeader[dataType].headerPtr = addressPtr;
        mModeHeader[dataType].headerLength = 0;
        mModeHeader[dataType].dataPtr = addressPtr;
        mModeHeader[dataType].dataLength = mFrameCount * (MAX_B_FRAME_SIZE + cineFrameHeaderSize);

        addressPtr += mModeHeader[dataType].dataLength;
    }

    // Get C-Mode scan converter params
    if (0 != BF_GET(mDataTypes, ThorRawDataFile::DataType::CMode, 1))
    {
        dataType = ThorRawDataFile::DataType::CMode;
        mModeHeader[dataType].headerPtr = addressPtr;
        mModeHeader[dataType].headerLength = sizeof(ScanConverterParams);
        mModeHeader[dataType].dataPtr = addressPtr + mModeHeader[dataType].headerLength;
        mModeHeader[dataType].dataLength = mFrameCount * (MAX_COLOR_FRAME_SIZE + cineFrameHeaderSize);

        addressPtr += mModeHeader[dataType].headerLength + mModeHeader[dataType].dataLength;
    }

    // Get M-Mode scroll Params
    if (0 != BF_GET(mDataTypes, ThorRawDataFile::DataType::MMode, 1))
    {
        dataType = ThorRawDataFile::DataType::MMode;
        mModeHeader[dataType].headerPtr = addressPtr;
        mModeHeader[dataType].headerLength = sizeof(MModeRecordParams);
        mModeHeader[dataType].dataPtr = addressPtr + mModeHeader[dataType].headerLength;

        if (isTextureFrame(addressPtr))
        {
            mModeHeader[dataType].dataLength = MAX_TEX_FRAME_SIZE + cineFrameHeaderSize;
        }
        else
        {
            mModeHeader[dataType].dataLength = mFrameCount * (MAX_M_FRAME_SIZE + cineFrameHeaderSize);
        }
        addressPtr += mModeHeader[dataType].headerLength + mModeHeader[dataType].dataLength;
    }

    // Get Da record Params
    if (0 != BF_GET(mDataTypes, ThorRawDataFile::DataType::Auscultation, 1))
    {
        dataType = ThorRawDataFile::DataType::Auscultation;
        mModeHeader[dataType].headerPtr = addressPtr;
        mModeHeader[dataType].headerLength = sizeof(DaEcgRecordParamsV3);
        mModeHeader[dataType].dataPtr = addressPtr + mModeHeader[dataType].headerLength;

        if (isScreenFrame(addressPtr))
        {
            mModeHeader[dataType].dataLength = MAX_DA_SCR_FRAME_SIZE + cineFrameHeaderSize;
        }
        else
        {
            mModeHeader[dataType].dataLength = mFrameCount * (MAX_DA_FRAME_SIZE_LEGACY + cineFrameHeaderSize);
        }
        addressPtr += mModeHeader[dataType].headerLength + mModeHeader[dataType].dataLength;
    }

    // Get Ecg record Params
    if (0 != BF_GET(mDataTypes, ThorRawDataFile::DataType::ECG, 1))
    {
        dataType = ThorRawDataFile::DataType::ECG;
        mModeHeader[dataType].headerPtr = addressPtr;
        mModeHeader[dataType].headerLength = sizeof(DaEcgRecordParamsV3);
        mModeHeader[dataType].dataPtr = addressPtr + mModeHeader[dataType].headerLength;

        if (isScreenFrame(addressPtr))
        {
            mModeHeader[dataType].dataLength = MAX_ECG_SCR_FRAME_SIZE + cineFrameHeaderSize;
        }
        else
        {
            mModeHeader[dataType].dataLength = mFrameCount * (MAX_ECG_FRAME_SIZE + cineFrameHeaderSize);
        }
    }

    retVal = THOR_OK;

    err_ret:
    return(retVal);
}

//-----------------------------------------------------------------------------
bool ThorFileProcessor::isTextureFrame(uint8_t *inputPtr)
{
    MModeRecordParams mModeRecordParams;
    memcpy(&mModeRecordParams,
           inputPtr,
           sizeof(MModeRecordParams));

    return mModeRecordParams.isTextureFrame;
}

//-----------------------------------------------------------------------------
bool ThorFileProcessor::isScreenFrame(uint8_t *inputPtr)
{
    DaEcgRecordParams daEcgRecordParams;
    memcpy(&daEcgRecordParams,
           inputPtr,
           sizeof(DaEcgRecordParams));

    return daEcgRecordParams.isScrFrame;
}
