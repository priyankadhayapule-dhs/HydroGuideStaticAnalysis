//
// Copyright 2020 EchoNous Inc.
//
#pragma once
#include <stdint.h>
#include <DauMemory.h>
#include <UpsReader.h>
#include <DauHw.h>

class DauArrayTest
{
public:
    DauArrayTest(const std::shared_ptr<DauMemory>& daum,
                 const std::shared_ptr<UpsReader>& upsReader,
                 const std::shared_ptr<DauHw>& dauHw,
                 uint32_t probe_type);
    ~DauArrayTest();

    
    uint32_t run();

    bool  isElementCheckRunning() const;

private:
    bool      mInitialized;
    uint32_t  mBistwf[16]; // Not used
    uint32_t  bistwf[13];
    uint32_t  mTxrxt;
    uint32_t  mAfetr;
    float     mRmsLow;
    float     mRmsHigh;
    float     rms_element_min;
    float     rms_element_max;
    uint32_t  mHvps1;
    uint32_t  mHvps2;
    uint32_t  mSamples;
    uint32_t  tx_chips;
    uint32_t  error_count;
    uint32_t  probetype = 5;
    bool      mIsElementCheckRunning;

    void  SetHVOff();
    void  SetHV();
    void  AfeOnStandby();
    void  AfeOff();
    void  GetAfeData(uint32_t afe);
    uint32_t  CheckRmsLevel(uint32_t efirst);
    void  TxBaseConfig();
    void  TxWaveformConfig();
    void  TxDelayConfig();
    void  TxDummy();
    void  TxAperture(uint32_t txstart);
    void  TxSWTrig();
    void  DoPings(uint32_t pings);


private:
    std::shared_ptr<DauMemory>  mDaumSPtr;
    std::shared_ptr<UpsReader>  mUpsReaderSPtr;
    std::shared_ptr<DauHw>      mDauHwSPtr;

};
    
