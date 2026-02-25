//
// Copyright 2016 EchoNous, Inc.
//
#pragma once

#include <stdint.h>
#include <CriticalSection.h>
#include <DauContext.h>
#include <DmaBuffer.h>

class PciDmaBuffer : public DmaBuffer
{
public: // Methods
    PciDmaBuffer();
    ~PciDmaBuffer();

    // Use this variant for stand-alone tests and utilities.  Will need root
    // permissions for successful access.
    bool open();

    // Use this variant under normal usage from typical Android applications
    // that have normal access rights.  DauContext has descriptors that have
    // been created from a privileged process. 
    bool open(DauContext* dauContextPtr);

    void close();

    uint64_t    getPhysicalAddr(const DmaBufferType& bufferType);
    uint32_t    getBufferLength(const DmaBufferType& bufferType); // Length in bytes
    uint32_t*   getBufferPtr(const DmaBufferType& bufferType);

private: // Structure as defined in EchoNous Ion Buffer (EIB) driver
    struct eib_data
    {
        eib_data() : phys_addr(0), buf_len(0), buf_fd(-1) {}

        uint64_t    phys_addr;
        uint32_t    buf_len;
        int         buf_fd;
    };

private: // Properties
    static uint32_t     mFifoLength;
    static uint32_t     mSequenceLength;
    static uint32_t     mDiagnosticsLength;

    struct eib_data     mEibData;
    uint32_t*           mBufPtr;
    int                 mMapFd;
    CriticalSection     mLock;
};

