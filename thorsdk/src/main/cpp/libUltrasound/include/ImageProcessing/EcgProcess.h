//
// Copyright 2019 EchoNous Inc.
//
//

#pragma once
#include <cstdint>
#include <memory>
#include <ThorConstants.h>
#include <ThorError.h>
#include <UpsReader.h>
#include <CineBuffer.h>

// FIR filter buffer size
#define ECG_PROCESSING_BUFFER_SIZE 512

// IIR filter order
#define IIR_FILTER_ORDER 2      // should be  >= 2

// Heart Rate Update Interval
#define HEART_RATE_UPDATE_INTERVAL_SAMPLES 910   // number of samples

//#define ECG_PERFORMANCE_TEST

#ifdef ECG_PERFORMANCE_TEST
#include <DmaBuffer.h>
#endif

class EcgProcess
{
public:
    EcgProcess(const std::shared_ptr<UpsReader> &upsReader, bool isUsb);
    ~EcgProcess();

    static const char *name() { return "EcgProcess"; }
    ThorStatus  init();
    ThorStatus setProcessParams(uint32_t samplesPerFrame, float samplesPerSecond);
    ThorStatus setADCMax(uint32_t adcMax);
    ThorStatus process(uint8_t* inputPtr, uint32_t count);
    ThorStatus setCineBuffer(CineBuffer* cineBuffer);
    ThorStatus initializeProcessIndices();
    ThorStatus setGain(float dBGain);

#ifdef ECG_PERFORMANCE_TEST
    void       setDMABufferSPtr(const std::shared_ptr<DmaBuffer> &dmaBufferSPtr) { mDmaBufferSPtr = dmaBufferSPtr; };
#endif

private:
    void       estimateHR(uint32_t frameNum);
    
private:
    uint32_t mSamplesPerFrame;
    uint32_t mInputDataIndex;           // input data index
    uint32_t mProcessingIndex;          // output processing index (FIR filter)
    uint32_t mFilterSize;
    uint32_t mNotReadyCount;            // variable used to keep track of invalid samples
    float    mLastValidSample;          // variable used to replace invalid sample
    float    mADCMax;
    std::shared_ptr<UpsReader> mUpsReaderSPtr;
    CineBuffer*  mCineBuffer;
    float    mGain;
    float    mSamplesPerSecond;
    uint32_t mMinSeparation;
    float    mMinQRSSample;

    float    mProcessingBuffer[ECG_PROCESSING_BUFFER_SIZE];
    float    mHeartRate;
    uint32_t mHeartRateUpdateIndex;
    int      mPrevPeakLoc;
    bool     mPrevPositive;
    float    mPrevEstimatedHR;

    uint32_t mHeartRateUpdateInterval;
    bool     mIsUsbContext;

    // IIR filter order
#if IIR_FILTER_ORDER > 2
    float    mX[IIR_FILTER_ORDER];
    float    mY[IIR_FILTER_ORDER];
#else
    float    mXn_1, mXn_2;
    float    mYn_1, mYn_2;
#endif

    // for performance testing
#ifdef ECG_PERFORMANCE_TEST
    std::shared_ptr<DmaBuffer> mDmaBufferSPtr;
#endif

};
