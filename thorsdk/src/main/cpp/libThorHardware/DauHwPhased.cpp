//
// Copyright 2021 EchoNous Inc.
//
#define LOG_TAG "DauHwPhased"

#include <cstdint>
#include <memory>
#include <cmath>
#include <ThorUtils.h>
#include <ThorConstants.h>
#include <TbfRegs.h>
#include <DauHwPhased.h>
#include <BitfieldMacros.h>

//-----------------------------------------------------------------------------
DauHwPhased::DauHwPhased(std::shared_ptr<DauMemory> daumSPtr,
                         std::shared_ptr<DauMemory> daum2SPtr) :
    DauHw(daumSPtr, daum2SPtr)
{
}

//-----------------------------------------------------------------------------
uint32_t DauHwPhased::hvMonitorScaleFactor( uint32_t sampleCount )
{
    if (sampleCount <= 0.0f)
    {
        return (0);
    }

    float invSampleCount = 1.0f/sampleCount;
    uint32_t sf = std::floor(invSampleCount * (1 << 26));
    return (sf);
}

//-----------------------------------------------------------------------------
uint32_t DauHwPhased::hvpAdcCode( float volts )
{
    return (std::roundf(27.528f*volts - 1.5297f));
}

//-----------------------------------------------------------------------------
uint32_t DauHwPhased::hvnAdcCode( float volts )
{
    return (std::roundf(23.244f*volts + 941.89f));
}

//-----------------------------------------------------------------------------
uint32_t DauHwPhased::hvpiAdcCode( float hvpi )
{
    uint32_t val = (std::roundf(((hvpi/1000.0f)+0.064307f)/0.000552f));
    return (val);
}

//-----------------------------------------------------------------------------
uint32_t DauHwPhased::hvniAdcCode( float hvni )
{
    uint32_t val = std::roundf(((hvni/1000.0f)+0.020525f)/0.000731f);
    return (val);
}

//-----------------------------------------------------------------------------
float DauHwPhased::hvp( uint32_t adcCode )
{
    return(0.0363f*adcCode + 0.0541f);
}

//-----------------------------------------------------------------------------
float DauHwPhased::hvn( uint32_t adcCode )
{
    return(0.043f*adcCode - 40.521f);
}

//-----------------------------------------------------------------------------
float DauHwPhased::hvpi(float hvp, uint32_t adcCode)
{
    float val = (0.000552f*adcCode-0.064307f)*1000.0f;
    if (val < 0.0f)
        val = 0.0f;
    return( val );
}

//-----------------------------------------------------------------------------
float DauHwPhased::hvni(float hvn, uint32_t adcCode)
{
    float val = (0.000731f*adcCode-0.020525f)*1000.0f;
    if (val < 0.0f)
        val = 0.0f;
    return (val);
}
