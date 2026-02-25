//
// Copyright 2017 EchoNous Inc.
//
//
// Inference Engine Handler
//

#pragma once

#include <ScanConverterParams.h>
#include <ScanConverterCl.h>
#include <InferenceEngine.h>
#include <Event.h>
#include <CriticalSection.h>
#include <TriggeredValue.h>
#include <UpsReader.h>
#include <CineBuffer.h>
#include <CineViewer.h>
#include <CardiacView.h>
#include <CardiacBounding.h>

class IeHandler : public CineBuffer::CineCallback
{
public: // Methods
    IeHandler(CineBuffer& cineBuffer, CineViewer& cineViewer);
    ~IeHandler();

    ThorStatus      onInit();
    void            onTerminate();
    void            setParams(CineBuffer::CineParams& params);
    void            onData(uint32_t frameNum, uint32_t dauDataTypes);

    void            enableIe(bool enable);

private: // Methods
    IeHandler();

    typedef std::unordered_map<std::string, std::vector<float>> InferenceResults;

    static void*    workerThreadEntry(void* thisPtr);
    void            threadLoop();
    void            handleCardiacClassifier(InferenceResults& results);
    void            handleCardiacBounding(InferenceResults& results);

private: // Properties
    CineBuffer&         mCineBuffer;
    CineViewer&         mCineViewer;
    ScanConverterCL     mScanConverter;
    InferenceEngine     mInferenceEngine;
    uint32_t            mImagingMode;
    Event               mStopEvent;
    TriggeredValue      mFrameEvent;
    Classifier*         mCurClassifierPtr;
    CardiacView         mCardiacView;
    CardiacBounding     mCardiacBounding;
    bool                mEnabled;
    CriticalSection     mLock;
    pthread_t           mThreadId;
    bool                mIsInitialized;
};
