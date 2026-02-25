//
// Copyright 2018 EchoNous Inc.
//

#pragma once

#include <stdint.h>
#include <memory>
#include <UpsReader.h>
#include <ScanDescriptor.h>
#include <DauRegisters.h>


class MmodeAcq
{

public:
    MmodeAcq(const std::shared_ptr<UpsReader>& upsReader);
    MmodeAcq() {};
    ~MmodeAcq();

    void init( const std::shared_ptr<UpsReader>& upsReader) { mUpsReaderSPtr = upsReader; };

    void getNearestBBeam( uint32_t imagingCaseId, float mLinePosition, uint32_t &beamNumber, uint32_t &multilinePosition, bool isLinearProbe = false);

private:
    std::shared_ptr<UpsReader> mUpsReaderSPtr;
    
};
