//
// Copyright 2018 EchoNous Inc.
//
//

#pragma once

#include <ThorError.h>
#include <CineBuffer.h>
#include <ScanConverterParams.h>
#include <ThorRawWriter.h>
#include <UpsReader.h>
#include <Event.h>
#include <TriggeredValue.h>
#include <BModeRecordParams.h>
#include <ColorModeRecordParams.h>
#include <ScrollModeRecordParams.h>
#include <DaEcgRecordParams.h>
#include <string>

typedef void (*CompletionHandler)(uint32_t);


class CineRecorder : public CineBuffer::CineCallback
{
public: // Methods
    CineRecorder(CineBuffer& cineBuffer);
    ~CineRecorder();

    CineRecorder() = delete;
    CineRecorder(const CineRecorder& that) = delete;
    CineRecorder& operator=(CineRecorder const&) = delete;

    void        attachCompletionHandler(CompletionHandler completionHandlerPtr);
    void        detachCompletionHandler();

    ThorStatus  recordStillImage(const char* destPath, uint32_t reqFrameNum = -1u);
    ThorStatus  recordRetrospectiveVideo(const char* destPath, uint32_t msec);
    ThorStatus  recordRetrospectiveVideoFrames(const char* destPath, uint32_t frames);

    ThorStatus  saveVideoFromCine(const char* destPath,
                                  uint32_t startMsec,
                                  uint32_t stopMsec);

    ThorStatus  saveVideoFromCineFrames(const char* destPath,
                                        uint32_t startFrame,
                                        uint32_t stopFrame);

    // Save a video with real frame numbers (i.e. startFrame and stopFrame are NOT adjusted by current
    // CineBuffer bounds)
    ThorStatus  saveVideoFromLiveFrames(const char* destPath,
                                        uint32_t startFrame,
                                        uint32_t stopFrame);

    ThorStatus  updateParamsThorFile(const char* destPath);

    void        setScrollModeRecordParams(ScrollModeRecordParams& smrParams);
    void        setDaRecordParams(DaEcgRecordParams& daParams);
    void        setEcgRecordParams(DaEcgRecordParams& ecgParams);

    void        setIsDaOn(bool isDaOn) { mIsDaOn = isDaOn; };
    void        setIsEcgOn(bool isEcgOn) { mIsEcgOn = isEcgOn; };

public: // CineCallback methods
    ThorStatus  onInit();
    void        onTerminate();
    void        setParams(CineBuffer::CineParams& params);
    void        onData(uint32_t frameNum, uint32_t dauDataTypes);

private: // Structures
    typedef struct
    {
        // B-Mode
        uint8_t*    bDataPtr;
        uint32_t    bDataSize;  // Total space needed for B-Mode frames
        // C-Mode
        uint8_t*    cDataPtr;
        uint32_t    cDataSize;
        // PW
        uint8_t*    pwDataPtr;
        uint32_t    pwDataSize;
        // M-Mode
        uint8_t*    mDataPtr;
        uint32_t    mDataSize;
        // CW
        uint8_t*    cwDataPtr;
        uint32_t    cwDataSize;
        // DA
        uint8_t*    daDataPtr;
        uint32_t    daDataSize;
        // ECG
        uint8_t*    ecgDataPtr;
        uint32_t    ecgDataSize;
    } DataMap;


private: // Methods
    static void*    workerThreadEntry(void* thisPtr);

    void            threadLoop();
    bool            getBusyLock();
    void            recordRetrospectiveClip(uint32_t recordFrames);
    void            recordFrames(uint32_t minFrameNum, uint32_t maxFrameNum);
    void            recordCapnpFrames(uint32_t minFrameNum, uint32_t maxFrameNum);
    ThorStatus      createRawFile(ThorRawWriter& writer, uint32_t frameCount);
    uint32_t        calculateSpace(uint32_t desiredFrames, DataMap& dataMap);
    void            adjustPointers(DataMap& dataMap, uint8_t* basePtr);
    uint32_t        writeFrames(uint32_t startFrame, 
                                uint32_t stopFrame,
                                DataMap& dataMap);
    bool            validateFile(DataMap& dataMap,
                                 uint32_t startFrame,
                                 uint32_t endFrame);

    void            processScrollModeFrames(uint32_t frameNum);

private: // Properties
    CineBuffer&             mCineBuffer;
    CriticalSection         mLock;
    CompletionHandler       mCompletionHandlerPtr;
    bool                    mIsInitialized;
    CriticalSection         mBusyLock;
    bool                    mIsBusy;

    Event                   mStopEvent;
    TriggeredValue          mRecordRetroEvent;
    Event                   mRecordStillEvent;
    Event                   mRecordCineEvent;
    Event                   mCancelEvent;

    CineBuffer::CineParams  mCineParams;
    uint32_t                mBFrameSize;
    uint32_t                mCFrameSize;
    uint32_t                mMFrameSize;
    uint32_t                mTexFrameSize;
    uint32_t                mDaFrameSize;
    uint32_t                mEcgFrameSize;
    uint32_t                mDaScrFrameSize;
    uint32_t                mEcgScrFrameSize;
    bool                    mIsDaOn;
    bool                    mIsEcgOn;
    uint32_t                mStillImageFrameNum;

    ScrollModeRecordParams  mScrollModeRecordParams;
    DaEcgRecordParams       mDaRecordParams;
    DaEcgRecordParams       mEcgRecordParams;

    std::string             mFilePath;
    uint32_t                mStartFrame;
    uint32_t                mStopFrame;

    pthread_t               mThreadId;

    static const int        MAX_RECORD_TIME = 200000;
};


// Take a look at adler32 in zlib for a possible efficient checksum.  More
// efficient than crc32 but not as unique.
