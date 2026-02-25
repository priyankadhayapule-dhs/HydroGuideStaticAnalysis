//
// Copyright 2017 EchoNous, Inc.
//

#pragma once

#include <DauContext.h>
#include <DmaBuffer.h>

class UsbDmaBuffer : public DmaBuffer
{
public: // Methods
    UsbDmaBuffer();
    ~UsbDmaBuffer();

    bool open();
    bool open(DauContext* dauContextPtr);
    void close();

    uint64_t    getPhysicalAddr(const DmaBufferType& bufferType);
    uint32_t    getBufferLength(const DmaBufferType& bufferType); // Length in bytes
    uint32_t*   getBufferPtr(const DmaBufferType& bufferType);

private: // Properties
    static uint32_t     mFifoLength;

    uint32_t*           mFifoBufferPtr;
};
