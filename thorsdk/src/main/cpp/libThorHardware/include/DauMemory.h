//
// Copyright (C) 2016 EchoNous, Inc.
//

#pragma once

#include <CriticalSection.h>
#include <DauContext.h>

class DauMemory
{
public: // Methods
    virtual ~DauMemory() {}

    virtual bool map() = 0;
    virtual bool map(DauContext* dauContextPtr) = 0;

    virtual void unmap() = 0;

    virtual size_t getMapSize() = 0;

    virtual bool read(uint32_t dauAddress, uint32_t* destAddress, uint32_t numWords) = 0;
    virtual bool write(uint32_t* srcAddress, uint32_t dauAddress, uint32_t numWords) = 0;

protected: // Methods
    DauMemory() {}
};
