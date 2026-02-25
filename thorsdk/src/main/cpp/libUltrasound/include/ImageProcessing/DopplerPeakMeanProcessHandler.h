//
// Copyright 2020 EchoNous Inc.
//
//
#pragma once

#include <DopplerPeakMaxProcess.h>
#include <DopplerPeakMeanPreProcess.h>
#include <DopplerPeakMeanPostProcess.h>
#include <DopplerPeakMeanProcess.h>
#include <CineBuffer.h>

class DopplerPeakMeanProcessHandler
{
public:
    DopplerPeakMeanProcessHandler(const std::shared_ptr<UpsReader> &upsReader);
    ~DopplerPeakMeanProcessHandler();

    static const char *name() { return "DopplerPeakMeanProcessHandler"; }

    ThorStatus init();
    ThorStatus process(uint32_t frameNum);
    void       terminate();

    void       setCineBuffer(CineBuffer* cineBuffer);
    void       setProcessIndices(uint32_t numFFT, float refGaindB = 15.0, float threshold = 0.2);
    void       setDataType(int inputDataType, int outputDataType);
    void       setWFGain(float dBGain);
    void       setFrameLatency(uint32_t);
    void       flush();
    void       updateVMax(bool singleFrameMode = false);
    ThorStatus processAllFrames(bool singleFrameMode = false);

private:
    DopplerPeakMaxProcess           mPeakMaxProcess;
    DopplerPeakMeanPreProcess       mPeakMeanPreProcess;
    DopplerPeakMeanPostProcess      mPeakMeanPostProcess;
    DopplerPeakMeanProcess          mPeakMeanProcess;

    CineBuffer*                     mCineBuffer;

    uint32_t                        mLastFrameNum;
    uint32_t                        mLastProcessedFrameNum;
    uint32_t                        mFrameLatency;

    int                             mInputDataType;
    int                             mOutputDataType;
};
