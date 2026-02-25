//
// Copyright (C) 2016 EchoNous, Inc.
//

#pragma once

#include <pthread.h>
#include <ThorUtils.h>
#include <Event.h>
#include <CriticalSection.h>
#include <DauContext.h>
#include <ThorError.h>

class DauSignalHandler
{
public: // Methods
    DauSignalHandler(DauContext& dauContext);
    ~DauSignalHandler();

    class SignalCallback;

    bool init(SignalCallback* callbackPtr);
    bool start();
    void stop();
    void terminate();

public: // Callback Class
    class SignalCallback
    {
    public:
        SignalCallback(void* dataPtr) : mDataPtr(dataPtr) {}
        virtual ~SignalCallback() {}

        virtual ThorStatus  onInit() { return(THOR_OK); }
        virtual ThorStatus  onData(uint32_t count) {UNUSED(count); return(THOR_OK);}
        virtual void        onHid(uint32_t hidId) {UNUSED(hidId);}
        virtual void        onExternalEcg(bool isConnected) {UNUSED(isConnected);}
        virtual ThorStatus  onCheckTemperature() {}
        virtual void        onError(uint32_t errorCode) {UNUSED(errorCode);}
        virtual void        onTest(void* testDataPtr) {UNUSED(testDataPtr);}
        virtual void        onTerminate() {}

        void* getDataPtr() { return(mDataPtr); }

    private:
        SignalCallback();

        void* mDataPtr;
    };

private: // Poll Fd Indexes
    // These are used as indexes for a fd array, and are listed in priority
    // order.  Do not re-order these without modifying the code that uses them!
    enum PollFdTypes
    {
        StartFd = 0,
        StopFd,
        ExitFd,
        DataFd,
        HidFd,
        ErrorFd,
        DauMemFd,
        ExtEcgFd,
        ProbeTempFd,
        FdCount
    };

private: // State Definitions
    enum State
    {
        Unknown = 0,
        Idle,
        Running,
        StateCount
    };

private: // Methods
    DauSignalHandler();

    static void* workerThreadEntry(void* thisPtr);

    void threadLoop();

    void initPollFd(struct pollfd (&pollArray)[FdCount]);
    void logPollResults(struct pollfd (&pollArray)[FdCount]);

private: // Properties
    DauContext&         mDauContext;
    SignalCallback*     mCallbackPtr;
    State               mCurState;

    Event               mStartEvent;
    Event               mStopEvent;
    Event               mExitEvent;

    Event               mAckEvent;

    pthread_t           mThreadId;

    CriticalSection     mLock;

    static const char* const mStateNames[StateCount];
    static const char* const mPollFdNames[FdCount];
};
