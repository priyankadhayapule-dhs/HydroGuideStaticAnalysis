//
// Copyright (C) 2017 EchoNous, Inc.
//

#pragma once

#include <cstdint>
#include <memory>
#include <DauMemory.h>
#include <UpsReader.h>
#include <DauLutManager.h>

class DauGainManager
{
public: // Methods
    DauGainManager(const std::shared_ptr<DauMemory>& daum,
                   const std::shared_ptr<UpsReader>& upsReader);
    ~DauGainManager();

    bool applyBModeGain( DauLutManager& lutm, float nearGain, float farGain, uint32_t imagingCase );

private:
    DauGainManager();
    
    uint32_t quantizeGain(float gain);

private:
    uint32_t                    mGainTable[1024];
    std::shared_ptr<DauMemory>  mDaumSPtr;
    std::shared_ptr<UpsReader>  mUpsReaderSptr;
};
