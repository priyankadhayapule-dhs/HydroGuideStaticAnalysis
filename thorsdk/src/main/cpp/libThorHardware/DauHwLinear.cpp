//
// Copyright 2021 EchoNous Inc.
//
#define LOG_TAG "DauHwLinear"

#include <cstdint>
#include <memory>
#include <cmath>
#include <ThorUtils.h>
#include <ThorConstants.h>
#include <TbfRegs.h>
#include <DauHwLinear.h>
#include <BitfieldMacros.h>

//-----------------------------------------------------------------------------
DauHwLinear::DauHwLinear(std::shared_ptr<DauMemory> daumSPtr,
                         std::shared_ptr<DauMemory> daum2SPtr) :
    DauHw(daumSPtr, daum2SPtr)
{
}

//-----------------------------------------------------------------------------
void DauHwLinear::convertHvToRegVals(float hvpb, float hvnb, uint32_t& ps1, uint32_t& ps2)
{
    int32_t hvpon, hvpoff;
    int32_t hvnon, hvnoff;

    hvpon  = (int32_t)((hvpb+0.8812f)/0.515f);
    hvpon  = (hvpon < 1) ? 1 : hvpon;
    hvpon  = (hvpon > 97) ? 97 : hvpon;
    hvpoff = 98-hvpon;

    hvnon  = (int32_t)((hvnb-0.7951f)/0.494f);
    hvnon  = (hvnon < 1) ? 1 : hvnon;
    hvnon  = (hvnon > 97) ? 97 : hvnon;
    hvnoff = 98-hvnon;

    LOGI(" TWHV hvpb = %f hvnb = %f hvpon = %d, hvpoff = %d hvnon = %d hvnoff = %d", hvpb, hvnb, hvpon, hvpoff,  hvnon, hvnoff);
    ps1 = 0x0E000000;
    ps2 = 0;
    BFN_SET(ps1, hvpon,  HVCTRLP_ON);
    BFN_SET(ps1, hvpoff, HVCTRLP_OFF);
    BFN_SET(ps2, hvnon,  HVCTRLN_ON);
    BFN_SET(ps2, hvnoff, HVCTRLN_OFF);
}

//-----------------------------------------------------------------------------
void DauHwLinear::hvSetLimits(float hv)
{
    float avgUpperLimit = hv + HV_MARGIN_VOLTS;
    float avgLowerLimit = hv - (HV_MARGIN_VOLTS + hv*0.05);

    float peakUpperLimit = hv + HV_MARGIN_PEAK_VOLTS;

    uint32_t avgUpperLimitCode = hvpAdcCode(avgUpperLimit);
    uint32_t avgLowerLimitCode = hvpAdcCode(avgLowerLimit);
    mDaumSPtr->write( &avgUpperLimitCode, PMONCTRL_HVPUPLIMIT_ADDR, 1);
    mDaumSPtr->write( &avgLowerLimitCode, PMONCTRL_HVPLWRLIMIT_ADDR, 1);
    LOGI("PMONCTRL_HVPUPLIMIT (%08x) = %08x (%f)",  PMONCTRL_HVPUPLIMIT_ADDR, avgUpperLimitCode, avgUpperLimit);
    LOGI("PMONCTRL_HVPLWRLIMIT (%08x) = %08x (%f)",  PMONCTRL_HVPLWRLIMIT_ADDR, avgLowerLimitCode, avgLowerLimit);

    avgUpperLimitCode = hvnAdcCode(-avgLowerLimit);
    avgLowerLimitCode = hvnAdcCode(-avgUpperLimit);
    mDaumSPtr->write( &avgUpperLimitCode, PMONCTRL_HVNUPLIMIT_ADDR, 1);
    mDaumSPtr->write( &avgLowerLimitCode, PMONCTRL_HVNLWRLIMIT_ADDR, 1);
    LOGI("PMONCTRL_HVNUPLIMIT (%08x) = %08x (%f)",  PMONCTRL_HVNUPLIMIT_ADDR, avgUpperLimitCode, -avgLowerLimit);
    LOGI("PMONCTRL_HVNLWRLIMIT (%08x) = %08x (%f)",  PMONCTRL_HVNLWRLIMIT_ADDR, avgLowerLimitCode, -avgUpperLimit);

    uint32_t peakUpperLimitCode = hvpAdcCode(peakUpperLimit);
    mDaumSPtr->write( &peakUpperLimitCode, PMONCTRL_HVPPEAKTHV_ADDR, 1);
    LOGI("PMONCTRL_HVPPEAKTHV (%08x) = %08x (%f)", PMONCTRL_HVPPEAKTHV_ADDR, peakUpperLimitCode, peakUpperLimit);

    peakUpperLimitCode = hvnAdcCode(-peakUpperLimit);
    mDaumSPtr->write( &peakUpperLimitCode, PMONCTRL_HVNPEAKTHV_ADDR, 1);
    LOGI("PMONCTRL_HVNPEAKTHV (%08x) = %08x (%f)", PMONCTRL_HVNPEAKTHV_ADDR,  peakUpperLimitCode, -peakUpperLimit);

    uint32_t currentUpperLimitCode = hvpiAdcCode(HV_MAX_MILLIAMPS);
    mDaumSPtr->write( &currentUpperLimitCode, PMONCTRL_HVPIUPLIMIT_ADDR, 1);
    LOGI("PMONCTRL_HVPIUPLIMIT (%08x) = %08x", PMONCTRL_HVPIUPLIMIT_ADDR,  currentUpperLimitCode);

    currentUpperLimitCode = hvniAdcCode(HV_MAX_MILLIAMPS);
    mDaumSPtr->write( &currentUpperLimitCode, PMONCTRL_HVNIUPLIMIT_ADDR, 1);
    LOGI("PMONCTRL_HVNIUPLIMIT (%08x) = %08x", PMONCTRL_HVNIUPLIMIT_ADDR,  currentUpperLimitCode);
}

//-----------------------------------------------------------------------------
uint32_t DauHwLinear::hvMonitorScaleFactor( uint32_t sampleCount )
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
uint32_t DauHwLinear::hvpAdcCode( float volts )
{
    return (std::roundf(volts*18.228));
}

//-----------------------------------------------------------------------------
uint32_t DauHwLinear::hvnAdcCode( float volts )
{
    return (std::roundf(997.0-19.6*-1.0f*volts));
}

//-----------------------------------------------------------------------------
uint32_t DauHwLinear::hvpiAdcCode( float hvpi )
{
    uint32_t val = (std::roundf(hvpi*1.36f));
    return (val);
}

//-----------------------------------------------------------------------------
uint32_t DauHwLinear::hvniAdcCode( float hvni )
{
    uint32_t val = (std::roundf(hvni*1.36f));
    return (val);
}

//-----------------------------------------------------------------------------
float DauHwLinear::hvp( uint32_t adcCode )
{
    return(adcCode/18.228f);
}

//-----------------------------------------------------------------------------
float DauHwLinear::hvn( uint32_t adcCode )
{
    return(-1.0*(997.0f-adcCode)/19.6f);
}

//-----------------------------------------------------------------------------
float DauHwLinear::hvpi(float hvp, uint32_t adcCode)
{
    float val = (adcCode/1.36f);
    if (val < 0.0f)
        val = 0.0f;
    return( val );
}

//-----------------------------------------------------------------------------
float DauHwLinear::hvni(float hvn, uint32_t adcCode)
{
    float val = (adcCode/1.36f);
    if (val < 0.0f)
        val = 0.0f;
    return (val);
}
