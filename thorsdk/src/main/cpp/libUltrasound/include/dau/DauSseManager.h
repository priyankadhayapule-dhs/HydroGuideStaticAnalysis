//
// Copyright 2017 EchoNous Inc.
//
//
#pragma once
#include <memory>
#include <DauMemory.h>
#include <DmaBuffer.h>
#include <ThorError.h>

class DauSseManager
{
public:
    DauSseManager(const std::shared_ptr<DauMemory>& daum,
                  const std::shared_ptr<DmaBuffer>& dmaBuffer);
    ~DauSseManager();

    ThorStatus init();
    ThorStatus start();
    ThorStatus stop(uint32_t waitTimeMs = 500);

private:
    DauSseManager();

private:
    std::shared_ptr<DauMemory>  mDaumSPtr;
    std::shared_ptr<DmaBuffer>  mDmaBufferSPtr;
};



