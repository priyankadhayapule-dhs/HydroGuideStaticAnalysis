//
// Copyright 2017 EchoNous Inc.
//
//

#pragma once

#include <pthread.h>
#include <CineBuffer.h>
#include <ScanConverterParams.h>
#include <ColorModeRecordParams.h>
#include <Event.h>
#include <TriggeredValue.h>
#include <CriticalSection.h>
#include <DopplerPeakMeanProcessHandler.h>

class AIManager;
typedef void (*reportProgressFunc)(uint32_t);
typedef void (*setRunningStatusFunc)(bool);

class CinePlayer
{
public: // Speed enum
    enum Speed
    {
        Normal = 0,
        Half,
        Quarter
    };

public: // Methods
    CinePlayer(CineBuffer& cineBuffer);
    ~CinePlayer();

    CinePlayer() = delete;
    CinePlayer(const CinePlayer& that) = delete;
    CinePlayer& operator=(CinePlayer const&) = delete;

    ThorStatus  init(const std::shared_ptr<UpsReader>& upsReader,
                     const char* appPathPtr);
    void        terminate();

    void        attachCine(reportProgressFunc progressFuncPtr,
                           setRunningStatusFunc runningStatusFuncPtr);
    void        detachCine();

    ThorStatus  openRawFile(const char* srcPath);
    void        closeRawFile();

    void        start();
    void        stop();
    void        pause();

    void        nextFrame();
    void        previousFrame();
    void        seekTo(uint32_t msec);
    void        seekToFrame(uint32_t frame, bool progressCallback);

    uint32_t    getDuration();
    uint32_t    getFrameCount();
    uint32_t    getCurrentPosition();
    uint32_t    getCurrentFrame();
    uint32_t    getCurrentFrameNo();

    void        setStartPosition(uint32_t msec);
    void        setEndPosition(uint32_t msec);
    void        setStartFrame(uint32_t frame);
    void        setEndFrame(uint32_t frame);

    uint32_t    getStartPosition();
    uint32_t    getEndPosition();
    uint32_t    getStartFrame();
    uint32_t    getEndFrame();
    uint32_t    getFrameInterval();

    void        setLooping(bool doLoop);
    bool        isLooping();

    void        setPlaybackSpeed(Speed speed);
    Speed       getPlaybackSpeed();

private: // Methods
    uint32_t    validateFrameNum(uint32_t frame);
    uint32_t    getFrameFromTime(uint32_t msec);

    // special calculation for PW/CW
    void            calculatePeakMean();

    static void*    workerThreadEntry(void* thisPtr);

    void            threadLoop();


private: // Properties
    CineBuffer&                     mCineBuffer;
    std::shared_ptr<UpsReader>      mUpsReaderSPtr;
    reportProgressFunc              mReportProgressFuncPtr;
    setRunningStatusFunc            mSetRunningStatusFuncPtr;
    pthread_t                       mThreadId;

    CriticalSection                 mLock;
    bool                            mIsLooping;
    bool                            mProgressCallback;
    Speed                           mSpeed;
    uint32_t                        mFrameInterval; // time in msec

    uint32_t                        mStartFrame;
    uint32_t                        mEndFrame;

    Event                           mExitEvent;
    Event                           mRefreshEvent;
    Event                           mStopEvent;
    Event                           mStartEvent;
    Event                           mNextEvent;
    Event                           mPreviousEvent;
    TriggeredValue                  mSeekEvent;
    Event                           mSpeedEvent;
    Event                           mGetPosEvent;
    TriggeredValue                  mAckEvent;
    bool                            mFileIsOpen;
    std::string                     mAppPath;

    bool                            mIsInitialized;
};

