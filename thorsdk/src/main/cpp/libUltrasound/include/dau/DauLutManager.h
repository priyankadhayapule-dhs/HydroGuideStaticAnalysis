//
// Copyright (C) 2017 EchoNous, Inc.
//

#pragma once

#include <cstdint>
#include <ThorConstants.h>
#include <memory>
#include <DauMemory.h>
#include <UpsReader.h>
#include <ColorAcq.h>
#include <ColorAcqLinear.h>
#include <DopplerAcq.h>
#include <DopplerAcqLinear.h>
#include <CWAcq.h>

class DauLutManager
{
public: // Methods
    DauLutManager(const std::shared_ptr<DauMemory>& daum,
                  const std::shared_ptr<UpsReader>& upsReader);
    ~DauLutManager();

    bool loadGlobalLuts();
    bool loadImagingCaseLuts(const uint32_t imagingCase, int imagingMode = IMAGING_MODE_B);
    bool loadColorImagingCaseLuts( uint32_t imagingCaseId, ColorAcq* colorAcqPtr, uint32_t wallFilterType );
    bool loadColorImagingCaseLinearLuts( uint32_t imagingCaseId, ColorAcqLinear* colorAcqPtr, uint32_t wallFilterType );
    bool loadDopplerImagingCaseLuts( uint32_t imagingCaseId, DopplerAcq* dopplerAcqPtr );
    bool loadDopplerImagingCaseLinearLuts( uint32_t imagingCaseId, DopplerAcqLinear* dopplerAcqPtr );
    bool loadCwImagingCaseLuts( uint32_t imagingCaseId, CWAcq* cwAcqPtr );
    void wakeupLuts();
    void sleepLuts();
    void TXchipLP();
    void TXchipON();
    void startSpiWrite(uint32_t wordCount);
    const float * getUserGainTable() { return(&mUserGainLut[0]);};

private:
    DauLutManager();
    
private:
    bool loadColorLuts(const uint32_t imagingCase, uint32_t wallFilterType = 1);
    
    uint32_t                    mTransferBuffer[16*1024]; // used for UPS to DAU transfers
    float                       mUserGainLut[1024];
    uint32_t                    mCK0;
    std::shared_ptr<DauMemory>  mDaumSPtr;
    std::shared_ptr<UpsReader>  mUpsReaderSPtr;
};
