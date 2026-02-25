//
// Copyright 2016 EchoNous Inc.
//
//

#pragma once

#include <sys/timerfd.h>
#include <android/native_window.h>
#include <DauSignalHandler.h>
#include <DauContext.h>
#include <ScanConverterParams.h>
#include <ThorConstants.h>
#include <UpsReader.h>
#include <CineBuffer.h>

class EmuDau
{
public: // Methods
    EmuDau(DauContext& dauContext, CineBuffer& cineBuffer);
    ~EmuDau();

    ThorStatus open();
    void close();

    bool start();
    void stop();

private: // Signal Callback Handler
    class SignalCallback : public DauSignalHandler::SignalCallback
    {
    public:
        SignalCallback(void* thisPtr) : DauSignalHandler::SignalCallback(thisPtr) {}

    private:
        ThorStatus  onInit();
        ThorStatus  onData(int count);
        void        onTerminate();
    };

private: // Methods
    EmuDau();

    void            populateDmaFifo();

private: // Properties
    DauContext&                 mDauContext;
    DauSignalHandler            mDauSignalHandler;
    SignalCallback              mCallback;
    int                         mTimerFd;
    struct itimerspec           mTimerSpec;

    std::shared_ptr<UpsReader>  mUpsReaderSPtr;

#define FIFO_FRAME_COUNT 64
    uint8_t                     mFifoBuffer[FIFO_FRAME_COUNT][MAX_B_FRAME_SIZE];

    CineBuffer&                 mCineBuffer;
};
