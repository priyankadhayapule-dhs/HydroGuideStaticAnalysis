//
// Copyright 2018 EchoNous Inc.
//
//

#pragma once

#include <ThorError.h>
#include <ThorRawDataFile.h>

class ThorRawWriter
{
public: // Client Callback for writeData() used for sequential write.
    class WriterCallback
    {
    public: // Methods
        WriterCallback(void* dataPtr) : mDataPtr(dataPtr) {}
        virtual ~WriterCallback() {}

        // Called by ThorRawWriter::writeData() for each individual frame
        // and type.  Should return the number of bytes to write from memory
        // specified by dataPtrPtr.
        virtual uint32_t    onData(uint32_t dataType,
                                   uint32_t frameNum,
                                   uint8_t** dataPtrPtr) = 0;

        void* getDataPtr() { return(mDataPtr); }

    private:
        WriterCallback();

        void* mDataPtr;
    };

public: // Methods
    ThorRawWriter();
    ~ThorRawWriter();

    ThorRawWriter(const ThorRawWriter& that) = delete;
    ThorRawWriter& operator=(ThorRawWriter const&) = delete;

    // Caller should fill in all fields of metadata except for timestamp.
    ThorStatus  create(const char* path, ThorRawDataFile::Metadata& metadata);
    void        close();

    // Sequential based writes
    ThorStatus  writeData(WriterCallback& callback);

    // Access data pointer for random access writes.  Will grow the data file
    // by amount specified in dataSize.
    uint8_t*    getDataPtr(uint32_t dataSize);

    ThorStatus  updateMetadata(ThorRawDataFile::Metadata& metadata);

private: // Methods
    ThorStatus  createHeader(ThorRawDataFile::Metadata& metadata);
    ThorStatus  writeMetadata(ThorRawDataFile::Metadata& metadata);
    ThorStatus  writeData(ThorRawDataFile::DataType dataType,
                          WriterCallback& callback);

private: // Properties
    int             mFd;
    uint32_t        mDataTypes;
    uint32_t        mFrameCount;
    uint8_t*        mDataPtr;
    off_t           mMapSize;
};
