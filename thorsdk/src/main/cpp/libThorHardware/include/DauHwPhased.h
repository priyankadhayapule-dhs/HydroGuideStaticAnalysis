//
// Copyright 2021 EchoNous Inc.
//
#pragma once

#include <DauHw.h>

class DauHwPhased : public DauHw
{
public:
    DauHwPhased(std::shared_ptr<DauMemory> daumSPtr,
                std::shared_ptr<DauMemory> daum2SPtr);

protected: // Methods
    uint32_t hvMonitorScaleFactor(uint32_t sampleCount);
    uint32_t hvpAdcCode(float volts);
    uint32_t hvnAdcCode(float volts);
    uint32_t hvpiAdcCode(float hvpi);
    uint32_t hvniAdcCode(float hvni);
    float hvp( uint32_t adcCode );
    float hvn( uint32_t adcCode );
    float hvpi( float hvp, uint32_t adcCode );
    float hvni( float hvn, uint32_t adcCode );
};
