//
// Copyright 2018 EchoNous Inc.
//
//

#pragma once

#include <ThorError.h>
#include <UpsReader.h>
#include <ThorRawReader.h>
#include <Event.h>
#include <TriggeredValue.h>
#include <ThorUtils.h>
#include <OffScreenRenderSurface.h>
#include <OverlayRenderer.h>
#include <BModeRenderer.h>
#include <CModeRenderer.h>
#include <ScrollModeRenderer.h>
#include <ScanConverterParams.h>
#include <ColorModeRecordParams.h>
#include <ScrollModeRecordParams.h>
#include <DaEcgRecordParams.h>
#include <WaveformRenderer.h>
#include <OverlayHRRenderer.h>
#include <CineBuffer.h>
#include <capnp/serialize.h>

typedef void (*CompletionHandler)(uint32_t);

class UltrasoundEncoder
{
public: // Methods
    explicit UltrasoundEncoder();
    ~UltrasoundEncoder();

    void attachCompletionHandler(CompletionHandler completionHandlerPtr);
    void detachCompletionHandler();

    ThorStatus  init(const char* appPathPtr);
    void        terminate();

    ThorStatus  processStillImage(const char* srcPath,
                                  float* zoomPanArray,
                                  float* winArray,
                                  int winArrayLen,
                                  uint8_t* outPixels,
                                  int imgWidth, int imgHeight);

    ThorStatus  encodeClip(const char* srcPath,
                           const char* dstPath,
                           float* zoomPanArray,
                           float* winArray,
                           int winArrayLen,
                           uint8_t* overlayPixels,
                           int imgWidth, int imgHeight,
                           int startFrame = 0, int endFrame = 0, bool rawOut = false, bool forcedFSR = false);

    ThorStatus  encodeStillImage(const char* srcPath,
                                 const char* dstPath,
                                 float* zoomPanArray,
                                 float* winArray,
                                 int winArrayLen,
                                 uint8_t* overlayPixels,
                                 int imgWidth, int imgHeight,
                                 int desiredFrameNum,
                                 bool rawOut);

    void        requestCancel();


private: // Methods
    static void*    workerThreadEntry(void* thisPtr);

    ThorStatus  processInput(const char* srcPath, bool tryNewFormat = true);
    ThorStatus  processInputCapnp(const char* srcPath);
    ThorStatus  prepareRenderSurface(ANativeWindow *inputSurface, bool addOverlay = true);
    ThorStatus  prepareTransitionRendering(int transitionImgMode, float desiredTimeSpan);
    ThorStatus  renderTransitionFrame(int transitionImgMode);

    // Get frame data (readonly data, but not const for compatibility)
    uint8_t*    getFrame(uint32_t frameNum, uint32_t dataType, bool isSingleFrameTexture = false);
    uint32_t    getNumSamplesInFrameHeader(uint32_t frameNum, uint32_t dataType);
    float       getHeartRateInFrameHeader(uint32_t frameNum);      // ECG data only

    // close input file and surface.  clean up
    void        close();

    void        threadLoop();
    bool        getBusyLock();

    ThorStatus  encodeClipCore();
    ThorStatus  encodeStillCore();

private: // Properties
    std::unique_ptr<UpsReader>      mUpsReaderUPtr;
    CriticalSection                 mLock;
    CompletionHandler               mCompletionHandlerPtr;
    bool                            mIsInitialized;
    CriticalSection                 mBusyLock;
    bool                            mIsBusy;
    bool                            mCancelRequested;

    // consolidated cineParam
    CineBuffer::CineParams          mCineParams;

    bool                            mIsDaEnabled;
    bool                            mIsEcgEnabled;
    bool                            mRenderHR;
    bool                            mIsRawOut;

    // for ScrollMode Full Screen Render (for S9)
    bool                            mIsForcedFSR;

    ThorRawReader                   mReader;
    uint32_t                        mImagingMode;
    uint32_t                        mImagingCaseID;
    uint32_t                        mFrameCount;
    uint32_t                        mFrameInterval;
    uint32_t                        mTargetFrameRate;
    size_t                          mCineFrameHeaderSize;

    bool                            mUseCapnpReader;
    int                             mCapnpFd;
    std::unique_ptr<capnp::StreamFdMessageReader> mCapnpReader;
    uint8_t*                        mBaseAddressPtr;
    uint8_t*                        mBaseAddressPtrColor;
    uint8_t*                        mBaseAddressPtrM;
    uint8_t*                        mBaseAddressPtrDa;
    uint8_t*                        mBaseAddressPtrEcg;

    OffScreenRenderSurface          mOffSCreenRenderSurface;

    // Renderer
    BModeRenderer                   mBModeRenderer;
    CModeRenderer                   mCModeRenderer;
    ScrollModeRenderer              mSModeRenderer;         // for M, PW, CW
    OverlayRenderer                 mOverlayRenderer;
    WaveformRenderer                mDaRenderer;
    WaveformRenderer                mEcgRenderer;
    OverlayHRRenderer               mHRRenderer;

    // Rendering related parameters
    ColorModeRecordParams           mColorModeParams;
    MModeRecordParams               mMModeParams;
    DaEcgRecordParams               mDaRecordParams;
    DaEcgRecordParams               mEcgRecordParams;

    // Event
    Event                           mEncodeStillEvent;
    Event                           mEncodeClipEvent;
    Event                           mStopEvent;

    // DA/ECG LineColor
    float                           mDALineColor[3];
    float                           mECGLineColor[3];

    // Encode Parameters
    std::string                     mInputFilePath;
    std::string                     mOutputFilePath;
    uint32_t                        mStartFrame;
    uint32_t                        mEndFrame;
    uint32_t                        mStillFrameNum;
    float                           mScale;
    float                           mDeltaX;
    float                           mDeltaY;
    float                           mFlipX;
    float                           mFlipY;
    float                           mWinXPct;
    float                           mWinYPct;
    float                           mWinWidthPct;
    float                           mWinHeightPct;
    float                           mWinXPct2;
    float                           mWinYPct2;
    float                           mWinWidthPct2;
    float                           mWinHeightPct2;
    float                           mWinXPct3;
    float                           mWinYPct3;
    float                           mWinWidthPct3;
    float                           mWinHeightPct3;
    uint32_t                        mImgWidth;
    uint32_t                        mImgHeight;
    uint8_t*                        mOverlayImgPrt;

    pthread_t                       mThreadId;

    // Private UPS Reader Objects
    std::string                     mAppPath;

};