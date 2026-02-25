//
// Copyright 2016 EchoNous, Inc.
//
#pragma once

#include <stdint.h>
#include <DauContext.h>

enum class DmaBufferType
{
    Fifo,
    Sequence,
    Diagnostics,
};

class DmaBuffer
{
public: // Methods
    virtual ~DmaBuffer() {}

    // Use this variant for stand-alone tests and utilities.  Will need root
    // permissions for successful access.
    virtual bool open() = 0;

    // Use this variant under normal usage from typical Android applications
    // that have normal access rights.  DauContext has descriptors that have
    // been created from a privileged process. 
    virtual bool open(DauContext* dauContextPtr) = 0;

    virtual void close() = 0;

    virtual uint64_t    getPhysicalAddr(const DmaBufferType& bufferType) = 0;
    virtual uint32_t    getBufferLength(const DmaBufferType& bufferType) = 0; // Length in bytes
    virtual uint32_t*   getBufferPtr(const DmaBufferType& bufferType) = 0;

protected: // Methods
    DmaBuffer() {}
};

