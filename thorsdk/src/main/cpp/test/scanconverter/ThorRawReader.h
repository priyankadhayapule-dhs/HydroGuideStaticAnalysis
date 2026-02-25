//
// Copyright 2018 EchoNous Inc.
//
//

#pragma once

#include "ThorRawDataFile.h"
#include "ThorError.h"
#include <stdio.h>

#define ALOGE(...)  printf(__VA_ARGS__)
#define ALOGD(...)  printf(__VA_ARGS__)

class ThorRawReader
{
public: // Client Callback for readData() used to sequential read.
    class ReaderCallback
    {
    public: // Methods
        ReaderCallback(void* dataPtr) : mDataPtr(dataPtr) {}
        virtual ~ReaderCallback() {}

        // Called by ThorRawReader::readData() for each individual frame
        // and type.  Should return the number of bytes to read to memory
        // specified by dataPtrPtr.
        virtual uint32_t    onData(uint32_t dataType,
                                   uint32_t frameNum,
                                   uint8_t** dataPtrPtr) = 0;

        void* getDataPtr() { return(mDataPtr); }

    private:
        ReaderCallback();

        void* mDataPtr;
    };

public: // Methods
    ThorRawReader();
    ~ThorRawReader();

    ThorRawReader(const ThorRawReader& that) = delete;
    ThorRawReader& operator=(ThorRawReader const&) = delete;

    // On successful return, all fields of metadata will be populated.
    ThorStatus  open(const char* path, ThorRawDataFile::Metadata& metadata);
    void        close();

    // Sequential based reads
    ThorStatus  readData(ReaderCallback& callback);

    // Access data pointer for random access reads.
    uint8_t*    getDataPtr();

    // file version
    uint8_t     getVersion() { return (mVersion); };


private: // Methods
    ThorStatus  readHeader(ThorRawDataFile::Metadata& metadata);
    ThorStatus  readData(ThorRawDataFile::DataType dataType,
                         ReaderCallback& callback);

private: // Properties
    int             mFd;
    uint32_t        mDataTypes;
    uint32_t        mFrameCount;
    uint8_t*        mDataPtr;
    off_t           mMapSize;
    uint8_t         mVersion;
};
