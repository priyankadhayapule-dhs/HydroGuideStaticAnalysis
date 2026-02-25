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
#include <DauMemory.h>

#define DA_PROCESSING_BUFFER_SIZE 2400
#define DA_FILTER_MAX_COEFFS 1024

//#define DA_SIGNAL_TESTING
//#define BYPASS_FILTER

#ifdef DA_SIGNAL_TESTING
#include <SineGenerator.h>
#endif

class DaProcess
{
public:
    DaProcess(const std::shared_ptr<DauMemory>& daum,
              const std::shared_ptr<UpsReader> &upsReader );
    ~DaProcess();

    static const char *name() { return "DaProcess"; }
    ThorStatus init();
    ThorStatus setProcessParams(uint32_t samplesPerFrame, uint32_t organId);
    ThorStatus process(uint8_t* inputPtr, uint32_t count);
    ThorStatus setCineBuffer(CineBuffer* cineBuffer);
    ThorStatus initializeProcessIndices();
    ThorStatus setGain(float dBGain);
    
private:
    uint32_t mSamplesPerFrame;
    uint32_t mInputDataIndex;           // input data index
    uint32_t mProcessingIndex;          // output processing index
    uint32_t mFilterSize;
    std::shared_ptr<UpsReader> mUpsReaderSPtr;
    std::shared_ptr<DauMemory>  mDaumSPtr;
    CineBuffer*  mCineBuffer;
    float    mGain;

    float    mXn_1, mXn_2;
    float    mYn_1, mYn_2;
    float    mHardLimit;

    float        mProcessingBuffer[DA_PROCESSING_BUFFER_SIZE];

    float        mDaCoeffs[DA_FILTER_MAX_COEFFS];

#ifdef DA_SIGNAL_TESTING
    SineGenerator *mSineOsc;
#endif

};
