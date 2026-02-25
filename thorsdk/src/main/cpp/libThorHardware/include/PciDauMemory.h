//
// Copyright (C) 2016 EchoNous, Inc.
//

#pragma once

#include <stdio.h>
#include <DauMemory.h>
#include <CriticalSection.h>
#include <DauContext.h>

class PciDauMemory : public DauMemory
{
public: // Methods
    PciDauMemory();
    ~PciDauMemory();

    // Note: Select bar before mapping.
    void selectBar(uint8_t bar);

    // If true, then read/write methods take internal addresses which are
    // automatically mapped to BAR0 offsets.  Default is to use internal
    // addresses.  Can be called before or after mapping operation.
    void useInternalAddress(bool useInternal);

    bool map();
    bool map(DauContext* dauContextPtr);

    void unmap();

    size_t getMapSize();

    bool read(uint32_t dauAddress, uint32_t* destAddress, uint32_t numWords);
    bool write(uint32_t* srcAddress, uint32_t dauAddress, uint32_t numWords);

private: // Methods
    size_t getMemLength(int fd);

private: // Properties
    DauContext*         mDauContextPtr;
    uint32_t*           mMemoryPtr;
    size_t              mMemLength;
    int                 mBar;
    int                 mFd;
    bool                mUseInternalAddress;
    FILE*               mLogStream;
    CriticalSection     mLock;

    static const char* const    mBarResources[6];
    static const uint32_t       mBar0Offset;
};
