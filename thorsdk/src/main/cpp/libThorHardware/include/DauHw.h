//
// Copyright 2018 EchoNous Inc.
//
#pragma once

#include <DauRegisters.h>
#include <DauMemory.h>
#include <ThorError.h>

class DauHw
{
public:
    DauHw(const std::shared_ptr<DauMemory>& daumSPtr,
          const std::shared_ptr<DauMemory>& daum2SPtr);

    typedef struct
    {
        uint32_t revisionId;
        uint32_t buildId;
        uint32_t dateCode; // YYMMDD
    } BuildInfo;

    virtual void getBuildInfo(BuildInfo *infoPtr);

    //-------------------- I2S -------------------------------------------------

    typedef struct
    {
        uint32_t samplesPerFrame : 16;
        uint32_t wordsPerSample  :  8;  // 0 => 1, 31 => 32, has to multiple of 2
    } I2SframeConfig;
    
    typedef struct
    {
        uint32_t lrClk           : 8;
        uint32_t mClk            : 8;
        uint32_t sClk            : 8;
        uint32_t rsrv            : 8;
    } I2SclkDiv;

    typedef struct
    {
        uint32_t mode            :  1;  // 0 => slave, 1 => master
        uint32_t enable          :  1;
        uint32_t ljMode          :  1;  // 0 => I2S, 1 => LJ
        uint32_t clkEdge         :  1;  // 0 => falling edge of LRCK, 1 => rising edge of LRCK
        uint32_t rsrv            : 27;
        uint32_t reset           :  1;  // 1 => reset, 0 => no reset; logic must be reset before enabling I2S/LS
    } I2Sconfig;

    
    //--------------------  I2C SENSOR -----------------------------------------

    typedef struct
    {
        uint32_t go              :  1;
        uint32_t device          :  2;
        uint32_t reset           :  1;
        uint32_t rsrv            : 28;
    } I2CmanualConfig;
    
    typedef struct
    {
        uint32_t byteCount       :  4;
        uint32_t rsrv0           :  4;
        uint32_t readBit         :  1;
        uint32_t slaveAddress    :  7;
        uint32_t rsrv1           : 16;
    } I2Ccommand;

    static const uint32_t I2C_ADDR_TMP0 = 0x48;
    static const uint32_t I2C_ADDR_TMP1 = 0x49;

    virtual ThorStatus   initTmpSensor(uint32_t slaveAddr);

    virtual ThorStatus   readTmpSensor(uint32_t slaveAddr,
                                       float& temperature);


    //----------------- MOTION SENSORS ------------------------------------

    typedef struct
    {
        uint32_t chainLength     : 7;  // load 1 less than needed length
        uint32_t order           : 1;  // 0 => LSB first, 1 => MSB first
        uint32_t polarity        : 1;  // ??
        uint32_t phase           : 1;  // ??
        uint32_t rsrv            : 6;
        uint32_t clkDiv          : 8;  // 0 => /1, 1 => /2
    } SPIconfig;

    static const uint32_t WHOAMI_ACCELEROMETER = 0x33;
    static const uint32_t WHOAMI_GYROSCOPE     = 0xD3;

    //--------------------  SEQUENCER ------------------------------------------

    typedef struct
    {
        uint32_t busy            :  1;

        // wait status
        uint32_t waitBb          :  1;
        uint32_t waitFb          :  1;
        uint32_t waitTb          :  1;
        uint32_t waitSb          :  1;

        // error status
        uint32_t smpErr          :  1;  // invalid register write
        uint32_t priErr          :  1;  // unexpected PRI
        uint32_t lleErr          :  1;  // wrong magic number in LLE
        uint32_t payloadErr      :  1;  // payload write error

        uint32_t rsrv            :  3;
        
        // sequencer state machine status/error
        uint32_t fsmState        :  4;  // current state
        uint32_t fsmErr          :  4;  // state of state machine when error occurred
    } SeqStatus;

    //-------------------- STREAMING ENGINE ------------------------------------

    typedef struct
    {
        uint32_t smpErr          :  1;
        uint32_t dmaStatus       :  2;
        uint32_t rsrv            :  5;

        uint32_t bufOverflow     :  8;
        uint32_t dataOverflow    :  8;
        uint32_t priSyncErr      :  8;
    } SeStatus;

    typedef struct
    {
        uint32_t reset           :  1;
        uint32_t slave           :  1; // needs to be set for USB i/f
        uint32_t l1PwrMgmt       :  1;
        uint32_t forcePending    :  1;
        uint32_t noSleep         :  4;

        uint32_t bfReset         :  1;
        uint32_t cwReset         :  1;
        uint32_t sensorReset     :  1;

        uint32_t rsrv            : 21;
    } SeControl;

    typedef struct
    {
        uint32_t stride          : 20;
        uint32_t rsrv0           :  4;
        uint32_t buffersLog2     :  3;
        uint32_t rsrv1           :  5;
    } SeFrameStride;

    //-------------------- CLOCK GENERATOR -------------------------------------
    
    //-------------------- SYS GPIO  -------------------------------------------

    typedef enum 
    {
        UNUSED               = 0,   // input by default
        SENSOR_INT1_IN       = 1,  
        SENSOR_INT2_IN       = 2,  
        SENSOR_INT3_IN       = 3,  
        BUTTON_INT0_IN       = 4,  
        SYSTEM_RESET_IN      = 5,  
        BUTTON_INT2_IN       = 6,  
        PCIE_RESET_IN        = 7,  
        CW_RESET_N_OUT       = 8,   
        CW_OVFL_N_OUT        = 9,   
        CLKGEN_I2C_INT_N_IN  = 10,  
        BOOT_SELECT_IN       = 11,  
        USB_SBU1_CONN_IN     = 12,  
        USB_C_MONITOR_IN     = 13,  
        UNUSED2              = 14,  
        SENSOR_INT4_IN       = 15,  
    } SysGpio;

    virtual void initSysGpio();
    virtual void setSysGpioBit(DauHw::SysGpio bit);
    virtual void clrSysGpioBit(DauHw::SysGpio bit);

    //-------------------- CW DOPPLER ------------------------------------------

    //-------------------- DA EKG ----------------------------------------------

    typedef enum
    {
        SYS_EXT_ECG_BIT = SysGpio::SENSOR_INT1_IN,
    } ExternalEcg;

    virtual void initExternalEcgDetection();
    virtual bool isExternalEcgConnected();
    virtual bool handleExternalEcgDetect();


    virtual ThorStatus selfTest();

    //-------------------- Watchdog Timer --------------------------------------

    virtual void initWatchdogTimer();
    virtual void resetWatchdogTimer();
    virtual void cancelWatchdogTimer();

    //-------------------- DMA Memory Bounds -----------------------------------

    virtual void initDmaMemBounds(size_t maxLength);

    //-------------------- HV SUPPLY -------------------------------------------
    static constexpr float HV_MARGIN_VOLTS = 1.5f;
    static constexpr float HV_MARGIN_PEAK_VOLTS = 4.0f;
    static constexpr float HV_MAX_MILLIAMPS = 100.0f;
    static const uint32_t  HVMON_MCLK_DIVIDER = 8;
    static const uint32_t  HVMON_ACQ_PERIOD = 4 * 15 * (HVMON_MCLK_DIVIDER + 1);
    static const uint32_t  HVMON_SAMPLE_PERIOD = HVMON_ACQ_PERIOD / 4;
        
    virtual void startHvSupply(float voltage);
    virtual void stopHvSupply();
    virtual void convertHvToRegVals(float hvpb, float hvnb, uint32_t& ps1, uint32_t& ps2);

    virtual void hvMonitorSetSampleCount(uint32_t frameInterval);
    virtual float hvMonitorGetHvpAvg();
    virtual float hvMonitorGetHvnAvg();
    virtual float hvMonitorGetHvpiAvg(float hvp);
    virtual float hvMonitorGetHvniAvg(float hvn);
    virtual void hvMonitorDisableInterrupts();
    virtual void hvSetLimits(float hv);
    virtual void hvStartMonitoring(bool isCwMode);
    virtual void hvStopMonitoring(float hv);

protected: // Methods
    virtual uint32_t hvMonitorScaleFactor(uint32_t sampleCount);
    virtual uint32_t hvpAdcCode(float volts);
    virtual uint32_t hvnAdcCode(float volts);
    virtual uint32_t hvpiAdcCode(float hvpi);
    virtual uint32_t hvniAdcCode(float hvni);
    virtual float hvp( uint32_t adcCode );
    virtual float hvn( uint32_t adcCode );
    virtual float hvpi( float hvp, uint32_t adcCode );
    virtual float hvni( float hvn, uint32_t adcCode );


protected: // Properties
    std::shared_ptr<DauMemory>          mDaumSPtr;
    std::shared_ptr<DauMemory>          mDaum2SPtr;
};


