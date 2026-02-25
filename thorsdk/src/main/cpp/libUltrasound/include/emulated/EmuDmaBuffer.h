//
// Copyright 2016 EchoNous, Inc.
//
#pragma once

#include <stdint.h>
#include <linux/ion.h>
#include <CriticalSection.h>
#include <DauContext.h>
#include <DmaBuffer.h>

class EmuDmaBuffer : public DmaBuffer
{
public: // Methods
    ~EmuDmaBuffer();

    static DmaBuffer& getInstance();

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

private: // Methods
    EmuDmaBuffer();

private: // Properties
    static uint32_t     mFifoLength;
    static uint32_t     mSequenceLength;
    static uint32_t     mDiagnosticsLength;
    static uint32_t     mBufferLength;

    int                 mOpenCount; // refcount for open()
    uint32_t*           mBufPtr;
    int                 mIonFd;
    int                 mMapFd;
    ion_user_handle_t   mIonHandle;
    CriticalSection     mLock;
};

