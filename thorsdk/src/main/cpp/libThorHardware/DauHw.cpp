//
// Copyright 2018 EchoNous Inc.
//
#define LOG_TAG "DauHw"
#include <cstdint>
#include <memory>
#include <cmath>
#include <ThorUtils.h>
#include <ThorConstants.h>
#include <TbfRegs.h>
#include <DauHw.h>
#include <BitfieldMacros.h>

#define I2C_CHECK_STATUS(daumPtr)                                  \
    {                                                              \
        uint32_t    regVal;                                        \
        int         retry = 10;                                    \
                                                                   \
        do                                                         \
        {                                                          \
            daumPtr->read(SENSOR_I2CSTAT_ADDR, &regVal, 1);        \
            usleep(5);                                             \
            retry--;                                               \
        } while (regVal != 0 && retry > 0);                        \
                                                                   \
        if (regVal)                                                \
        {                                                          \
            LOGE("I2C operation failed!  Status: 0x%X\n", regVal); \
            goto err_ret;                                          \
        }                                                          \
    }

DauHw::DauHw(const std::shared_ptr<DauMemory>& daumSPtr,
             const std::shared_ptr<DauMemory>& daum2SPtr) :
    mDaumSPtr(daumSPtr),
    mDaum2SPtr(daum2SPtr)
{
}

void DauHw::getBuildInfo( BuildInfo* infoPtr )
{
    mDaumSPtr->read( SYS_REVID_ADDR, (uint32_t *)infoPtr, sizeof(BuildInfo) >> 2);
}


ThorStatus DauHw::selfTest()
{
    ThorStatus retVal = THOR_OK;

    // place holder for a quick power on self test

    return (retVal);
}

void DauHw::initSysGpio()
{
    uint32_t regVal;

    // Set the direction bits, 1 => input, 0 => output (default input)

    regVal = 0xFFFF;

    regVal &= ~(1 << DauHw::SysGpio::CW_RESET_N_OUT);
    regVal &= ~(1 << DauHw::SysGpio::CW_OVFL_N_OUT);

    mDaumSPtr->write( &regVal, SYS_GPIOD_ADDR, 1 );
    
}

void DauHw::setSysGpioBit(DauHw::SysGpio gpioBit)
{
    uint32_t regVal = 1 << gpioBit;

    mDaumSPtr->write( &regVal, SYS_GPIO_SET_ADDR, 1 );
    
}

void DauHw::clrSysGpioBit(DauHw::SysGpio gpioBit)
{
    uint32_t regVal = 1 << gpioBit;

    mDaumSPtr->write( &regVal, SYS_GPIO_RESET_ADDR, 1 );
}

void DauHw::initExternalEcgDetection()
{
    uint32_t val;

    // Set GPIO to input for external ECG lead connection
    val = 0;
    mDaumSPtr->read(SYS_GPIOD_ADDR, &val, 1);
    val |= BIT(DauHw::ExternalEcg::SYS_EXT_ECG_BIT);
    mDaumSPtr->write(&val, SYS_GPIOD_ADDR, 1);

    // Set MSI interrupt mask for external ECG lead connection
    val = 0;
    mDaumSPtr->read(SYS_MASK_INT1_ADDR, &val, 1);
    val |= BIT(DauHw::ExternalEcg::SYS_EXT_ECG_BIT);
    mDaumSPtr->write(&val, SYS_MASK_INT1_ADDR, 1);
}

bool DauHw::isExternalEcgConnected()
{
    uint32_t    val;

    val = 0;
    mDaumSPtr->read(SYS_GPIOR_ADDR, &val, 1);
    return(!(val & BIT(DauHw::ExternalEcg::SYS_EXT_ECG_BIT)));
}

bool DauHw::handleExternalEcgDetect()
{
    uint32_t    val;
    bool        isConnected = isExternalEcgConnected();

    // Flip the polarity for interrupt generation
    val = 0;
    mDaumSPtr->read(SYS_GPIOP_ADDR, &val, 1);
    if (isConnected)
    {
        val &= ~BIT(SYS_EXT_ECG_BIT);
    }
    else
    {
        val |= BIT(SYS_EXT_ECG_BIT);
    }
    mDaumSPtr->write(&val, SYS_GPIOP_ADDR, 1);

    // Clear ECG as source of previous interrupt
    val = BIT(SYS_EXT_ECG_BIT);
    mDaumSPtr->write(&val, SYS_INT1_ADDR, 1);

    return(isConnected); 
}

ThorStatus DauHw::initTmpSensor( uint32_t slaveAddr )
{
    uint32_t    regVal;
    I2Ccommand  cmd;
    int         retry;

    //
    // Read TI document SBOS473F TMP112x for details
    //

    //
    // Configure for 1/4 HZ sampling
    //
    {
        cmd.byteCount    = 2;  // 3 bytes
        cmd.readBit      = 0;  // write
        cmd.slaveAddress = slaveAddr;
        mDaumSPtr->write( (uint32_t *)&cmd, SENSOR_I2C0CMD_ADDR, 1 );

        regVal = 0x206001; // Write 0x2060 to configuration register (0x01)
        mDaumSPtr->write( &regVal, SENSOR_I2C0WDAT0_ADDR, 1 );

        regVal = 1; // Manual Go
        mDaumSPtr->write( &regVal, SENSOR_I2CMCFG_ADDR, 1 );

        I2C_CHECK_STATUS(mDaumSPtr);

// This is for debugging
#if 0
        // Read the configuration
        cmd.byteCount    = 1;  // 2 bytes
        cmd.readBit      = 1;  // read
        cmd.slaveAddress = slaveAddr;
        daumSPtr->write( (uint32_t *)&cmd, SENSOR_I2C0CMD_ADDR, 1 );

        regVal = 1; // Manual Go
        daumSPtr->write( &regVal, SENSOR_I2CMCFG_ADDR, 1 );

        I2C_CHECK_STATUS(daumSPtr);

        daumSPtr->read( SENSOR_I2CRDAT0_ADDR, &regVal, 1);
        LOGD("Configuration: 0x%X", regVal);
#endif
    }

    //
    // Make obtaining temperature the default for a read
    //
    {
        cmd.byteCount    = 0;  // 1 byte
        cmd.readBit      = 0;  // write
        cmd.slaveAddress = slaveAddr;
        mDaumSPtr->write( (uint32_t *)&cmd, SENSOR_I2C0CMD_ADDR, 1 );

        regVal = 0;  // Set pointer register to 0 to point temperature register
        mDaumSPtr->write( &regVal, SENSOR_I2C0WDAT0_ADDR, 1 );

        regVal = 1; // Manual Go
        mDaumSPtr->write( &regVal, SENSOR_I2CMCFG_ADDR, 1 );
        I2C_CHECK_STATUS(mDaumSPtr);
    }

    return(THOR_OK);

err_ret:
    ALOGE("Failed to initialize Probe temperature sensor!");
    return(TER_THERMAL_PROBE_FAIL);
}

ThorStatus DauHw::readTmpSensor( uint32_t slaveAddr, float& temperature )
{
    uint32_t        regVal;
    uint32_t        tempData;
    const float     celciusConversion = 0.0625;
    const uint32_t  maxTemp = 0x7FF;
    I2Ccommand      cmd;

    cmd.byteCount    = 1;  // 2 bytes
    cmd.readBit      = 1;  // read
    cmd.slaveAddress = slaveAddr;
    mDaumSPtr->write( (uint32_t *)&cmd, SENSOR_I2C0CMD_ADDR, 1 );

    regVal = 1; // Manual Go
    mDaumSPtr->write( &regVal, SENSOR_I2CMCFG_ADDR, 1 );
    I2C_CHECK_STATUS(mDaumSPtr);

    mDaumSPtr->read( SENSOR_I2CRDAT0_ADDR, &regVal, 1);
    //LOGD("Raw Value: 0x%X\n", regVal);

    tempData = (((regVal << 4) & 0xff0) | ((regVal >> 12) & 0xf));
    //LOGD("tempData: 0x%X\n", tempData);

    if (tempData > maxTemp)
    {
        // Negative temperature
        tempData = (~tempData + 1) & 0xFFF;
        temperature = celciusConversion * tempData * -1;
    }
    else
    {
        // Positive temperature
        temperature = celciusConversion * tempData;
    }

    return(THOR_OK);

err_ret:
    LOGE("Failed to read Probe temperature sensor!");
    return(TER_THERMAL_PROBE_FAIL); // Return an unacceptably high value
}

void DauHw::initWatchdogTimer()
{
    uint32_t    val = BIT(FTWDT_WDCR_ENABLE_BIT) | 
                      BIT(FTWDT_WDCR_RESET_BIT) |
                      BIT(FTWDT_WDCR_CLOCK_BIT);

    mDaum2SPtr->write( &val, FTWDT_WDCR_ADDR, 1);
}

void DauHw::resetWatchdogTimer()
{
    uint32_t    val = FTWDT_RESET_MAGIC_NUMBER;

    mDaum2SPtr->write( &val, FTWDT_WDRESTART_ADDR, 1);
}

void DauHw::cancelWatchdogTimer()
{
    uint32_t    val = 0x0;

    mDaum2SPtr->write( &val, FTWDT_WDCR_ADDR, 1);
}

void DauHw::initDmaMemBounds(size_t maxLength)
{
    // This is only relevant for PCIe probes, but is benign for USB-based.
    uint32_t        val = 0x0;

    // The lower bound is preset in ATU by the Dau itself and should be correct.
    mDaumSPtr->read( WDMA_BNDL_ADDR, &val, 1);

    // Set upper bound, because default value is too big (max possible addressing
    // range for PCIe).
    val += maxLength;
    mDaumSPtr->write( &val, WDMA_BNDH_ADDR, 1);
}

void DauHw::convertHvToRegVals(float hvpb, float hvnb, uint32_t& ps1, uint32_t& ps2)
{
    int32_t hvpon, hvpoff;
    int32_t hvnon, hvnoff;

    hvpoff = (int32_t)(-hvpb*3.70611f + 98.9874f);
    hvpoff = (hvpoff < 1) ? 1 : hvpoff;
    hvpoff = (hvpoff > 97) ? 97 : hvpoff;
    hvpon  = 98 - hvpoff;


    hvnon = (int32_t)(hvnb*3.90582f - 12.19956f);
    hvnon = (hvnon < 1) ? 1 : hvnon;
    hvnon = (hvnon > 97) ? 97 : hvnon;
    hvnoff  = 98 - hvnon;

    LOGI("hvpb = %f, hvnb = %f, hvpon = %d, hvnon = %d", hvpb, hvnb, hvpon, hvnon);
    ps1 = 0x0E000000;
    ps2 = 0;
    BFN_SET(ps1, hvpon,  HVCTRLP_ON);
    BFN_SET(ps1, hvpoff, HVCTRLP_OFF);
    BFN_SET(ps2, hvnon,  HVCTRLN_ON);
    BFN_SET(ps2, hvnoff, HVCTRLN_OFF);
}
    
void DauHw::startHvSupply(float voltage)
{
    //return;
    uint32_t v_steps;
    uint32_t val;
    float start_v;
    float v_t;
    uint32_t ps1, ps2;

    start_v = 0.0f;

    // Set HV to MIN_HVPS_VOLTS
    val = 0x00000031;
    mDaumSPtr->write(&val, TBF_REG_HVPS0 + TBF_BASE,1);
    float hvpb = start_v;
    float hvnb = hvpb;
    convertHvToRegVals(hvpb, hvnb, ps1, ps2);
    mDaumSPtr->write( &ps1, TBF_REG_HVPS1 + TBF_BASE, 1);
    mDaumSPtr->write( &ps2, TBF_REG_HVPS2 + TBF_BASE, 1);
    usleep(4*1000); // Wait for stable PWM voltage setting
    // Enable HV power supply
    val = 0x1000000F;
    mDaumSPtr->write( &val, TBF_REG_PS0 + TBF_BASE, 1);
    usleep(2000);

    // Ramp up voltage at a rate of 2mS/V
    if (voltage < MIN_HVPS_VOLTS) {
        voltage = MIN_HVPS_VOLTS;
    }

    v_steps = int(voltage) - (start_v-1);
    // starting from MIN_HVPS_VOLTS
    v_t = start_v;
    LOGD("HV ramp up HV to %f in %d steps", voltage, v_steps);

    for (int i =0; i < v_steps; i++){
        convertHvToRegVals(v_t, v_t, ps1, ps2);
        mDaumSPtr->write( &ps1, TBF_REG_HVPS1 + TBF_BASE, 1);
        usleep(1000);
        mDaumSPtr->write( &ps2, TBF_REG_HVPS2 + TBF_BASE, 1);
        usleep(1000);
        v_t += 1.0f;
     }

     return;
}

void DauHw::stopHvSupply()
{
    hvStopMonitoring(0);
    return;
}

uint32_t DauHw::hvpAdcCode( float volts )
{
    return (std::roundf(27.528f*volts - 1.5297f));
}

uint32_t DauHw::hvnAdcCode( float volts )
{
    return (std::roundf(23.244f*volts + 941.89f));
}

uint32_t DauHw::hvpiAdcCode( float hvpi )
{
    uint32_t val = (std::roundf(((hvpi/1000.0f)+0.064307f)/0.000552f));
    return (val);
}

uint32_t DauHw::hvniAdcCode( float hvni )
{
    uint32_t val = std::roundf(((hvni/1000.0f)+0.020525f)/0.000731f);
    return (val);
}

float DauHw::hvp( uint32_t adcCode )
{
    return(0.0363f*adcCode + 0.0541f);
}

float DauHw::hvn( uint32_t adcCode )
{
    return(0.043f*adcCode - 40.521f);
}

float DauHw::hvpi(float hvp, uint32_t adcCode)
{
    float val = (0.000552f*adcCode-0.064307f)*1000.0f;
    if (val < 0.0f)
        val = 0.0f;
    return( val );
}

float DauHw::hvni(float hvn, uint32_t adcCode)
{
    float val = (0.000731f*adcCode-0.020525f)*1000.0f;
    if (val < 0.0f)
        val = 0.0f;
    return (val);
}

uint32_t DauHw::hvMonitorScaleFactor( uint32_t sampleCount )
{
    if (sampleCount <= 0.0f)
    {
        return (0);
    }

    float invSampleCount = 1.0f/sampleCount;
    uint32_t sf = std::floor(invSampleCount * (1 << 26));
    return (sf);
}


void DauHw::hvMonitorSetSampleCount(uint32_t frameInterval)
{
    uint32_t val;

    if (frameInterval == 0)
    {
        LOGE("%s frameInterval (%d) is invalid", frameInterval);
        return;
    }

    // Reset ADC
    val = 0x2;
    mDaumSPtr->write( &val, PMONCTRL_HVMONADCCFG_ADDR, 1 );

    // Set sampling frequency
    val = (HVMON_ACQ_PERIOD << 16) | HVMON_MCLK_DIVIDER;
    mDaumSPtr->write( &val, PMONCTRL_HVMONADCCLKDIVS_ADDR, 1 );

    uint32_t sampleCount = frameInterval / HVMON_SAMPLE_PERIOD;
    LOGD("%s frameInterval = %d, sampleCount = %d", __func__, frameInterval, sampleCount);

    mDaumSPtr->write( &sampleCount, PMONCTRL_SAMPLCNT_ADDR, 1 );

    uint32_t avgFactor = hvMonitorScaleFactor(sampleCount);
    mDaumSPtr->write( &avgFactor, PMONCTRL_AVGSCALEFACTR_ADDR, 1 );

    // Acquire from lower four channels of ADC (hvp, hvn, hvpi, hvni)
    val = 0xF012c;
    mDaumSPtr->write( &val, PMONCTRL_HVMONADCCFG_ADDR, 1);

    // Configure to start acquiring on PMON_START rising edge
    val = 0x4;
    mDaumSPtr->write( &val, PMONCTRL_PWRCONFIG_ADDR, 1);
}

float DauHw::hvMonitorGetHvpAvg()
{
    float volts;
    uint32_t adcCode;

    mDaumSPtr->read( PMONCTRL_HVPAVG_ADDR, &adcCode, 1 );
    LOGD("addr = %08x, HVP ADCCODE = %08X", PMONCTRL_HVPAVG_ADDR, adcCode);
    volts = hvp( adcCode );

    return (volts);
}

float DauHw::hvMonitorGetHvnAvg()
{
    float volts;
    uint32_t adcCode;

    mDaumSPtr->read( PMONCTRL_HVNAVG_ADDR, &adcCode, 1 );
    LOGD("addr = %08x, HVN ADCCODE = %08X", PMONCTRL_HVNAVG_ADDR, adcCode);
    volts = hvn( adcCode );

    return (volts);
}

float DauHw::hvMonitorGetHvpiAvg(float hvp)
{
    float milliAmps;
    uint32_t adcCode;

    mDaumSPtr->read( PMONCTRL_HVPIAVG_ADDR, &adcCode, 1 );
    LOGD("addr = %08x, HVPI ADCCODE = %08X", PMONCTRL_HVPIAVG_ADDR, adcCode);
    milliAmps = hvpi( hvp, adcCode );

    return (milliAmps);
}

float DauHw::hvMonitorGetHvniAvg(float hvn)
{
    float milliAmps;
    uint32_t adcCode;

    mDaumSPtr->read( PMONCTRL_HVNIAVG_ADDR, &adcCode, 1 );
    LOGD("addr = %08x, HVNI ADCCODE = %08X", PMONCTRL_HVNIAVG_ADDR, adcCode);
    milliAmps = hvni( hvn, adcCode );

    return (milliAmps);
}

void DauHw::hvMonitorDisableInterrupts()
{
    uint32_t val = 0xFFFFFFFF;
    mDaumSPtr->write( &val, PMONCTRL_HVFAULTMASK_ADDR, 1);
}

void DauHw::hvSetLimits(float hv)
{
    float avgUpperLimit = hv + HV_MARGIN_VOLTS;
    float avgLowerLimit = hv - HV_MARGIN_VOLTS;

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

void DauHw::hvStartMonitoring(bool isCwMode)
{
    uint32_t val;

    mDaumSPtr->read(BF_EN0_ADDR, &val, 1);
    if (!isCwMode)
    {
        BFN_SET(val, 1, BF_EN0_HV_FAULT_DIS_MODE);
        BFN_SET(val, 1, BF_EN0_HV_FAULT_RST_MODE);
        BFN_SET(val, 1, BF_EN0_HV_FAULT_EN_MODE);
    }

    mDaumSPtr->write( &val, BF_EN0_ADDR, 1);

    val = 0xffffffff;
    mDaumSPtr->write( &val, PMONCTRL_HVFAULTMASK_ADDR, 1);
    val = 0;
    mDaumSPtr->write( &val, PMONCTRL_PWRSTATUS_ADDR, 1);

    val = 0xffffffff;
    if (!isCwMode)
    {
        BFN_SET(val, 0, PMONCTRL_HVFAULTMASK_HVPMINMASK);
        BFN_SET(val, 0, PMONCTRL_HVFAULTMASK_HVPMAXMASK);
        BFN_SET(val, 0, PMONCTRL_HVFAULTMASK_HVNMINMASK);
        BFN_SET(val, 0, PMONCTRL_HVFAULTMASK_HVNMAXMASK);
        BFN_SET(val, 0, PMONCTRL_HVFAULTMASK_HVPIMAXMASK);
        BFN_SET(val, 0, PMONCTRL_HVFAULTMASK_HVNIMAXMASK);
    }

    mDaumSPtr->write( &val, PMONCTRL_HVFAULTMASK_ADDR, 1);
    LOGI("PMONCTRL_HVFAULTMASK (%08x) = %08x", PMONCTRL_HVFAULTMASK_ADDR, val);
}

void DauHw::hvStopMonitoring(float hv)
{
    uint32_t val;

    mDaumSPtr->read(BF_EN0_ADDR, &val, 1);
    BFN_SET(val, 0, BF_EN0_HV_FAULT_DIS_MODE);
    BFN_SET(val, 0, BF_EN0_HV_FAULT_RST_MODE);
    BFN_SET(val, 0, BF_EN0_HV_FAULT_EN_MODE);
    mDaumSPtr->write( &val, BF_EN0_ADDR, 1);

    val = 0xFFFFFFFF;
    mDaumSPtr->write( &val, PMONCTRL_HVFAULTMASK_ADDR, 1);
}


