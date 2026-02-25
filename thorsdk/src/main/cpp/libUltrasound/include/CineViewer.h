//
// Copyright 2017 EchoNous Inc.
//
//

#pragma once

#include <memory>
#include <android/native_window.h>
#include <pthread.h>
#include <ThorError.h>
#include <CineBuffer.h>
#include <Event.h>
#include <TriggeredValue.h>
#include <UpsReader.h>
#include <LiveRenderSurface.h>
#include <OffScreenRenderSurface.h>
#include <BModeRenderer.h>
#include <CModeRenderer.h>
#include <WaveformRenderer.h>
#include <ImguiRenderer.h>
#include <ScrollModeRenderer.h>
//#include <OverlayGraphRenderer.h>
//#include <CardiacBounding.h>
#include <ScrollModeRecordParams.h>
#include <DaEcgRecordParams.h>
#include <OverlayRenderer.h>
#include <ThorAudioPlayer.h>
#include <DopplerPeakMeanProcessHandler.h>
#include <DopplerPeakEstimator.h>

class AIManager;
typedef void (*ReportHeartRate)(float);
typedef void (*ReportError)(uint32_t);

class CineViewer : public CineBuffer::CineCallback
{
public: // Methods
    CineViewer(CineBuffer& cineBuffer, AIManager& aiManager);
    ~CineViewer();

    CineViewer() = delete;
    CineViewer(const CineViewer& that) = delete;
    CineViewer& operator=(CineViewer const&) = delete;

    ThorStatus  onInit();
    void        onTerminate();
    void        setWindow(ANativeWindow* windowPtr);
    void        setParams(CineBuffer::CineParams& params);
    void        onData(uint32_t frameNum, uint32_t dauDataTypes);
    void        pausePlayback();

    ThorStatus  setMmodeLayout(float bmodeLayout[], float mmodeLayout[]);
    ThorStatus  setDAECGLayout(float usLayout[], float daLayout[], float ecgLayout[]);
    ThorStatus  setDopplerLayout(float dopplerLayout[]);
    ThorStatus  setupUSDAECG(bool isUSOn,bool isDAOn, bool isECGOn);
    ThorStatus  refreshLayout();

    // prepare Transition for PW/CW duplex view
    ThorStatus  prepareDopplerTransition(bool prepareTransitionFrame);

    void        setDaEcgScrollSpeedIndex(int scrollSpeedIndex);

    // zoom-pan related methods
    void	handleOnScroll(float x, float y);
    void	handleOnScale(float scaleFactor);
    void	queryDisplayDepth(float& x0, float& x1);

    // Raw touch events
    void    handleOnTouch(float x, float y, bool isDown);

    // getPhysicalToPixelMap
    void    queryPhysicalToPixelMap(float* mapMat, int arrayLength);

    // get zoom-pan flip related parameters
    void    queryZoomPanFlipParams(float* zoomPanArray);

    // set zoom-pan flip related parameters
    void    setZoomPanFlipParams(float* zoomPanArray);

    // getUltrasoundScreenImage
    void    getUltrasoundScreenImage(uint8_t* pixels, int imgWidth, int imgHeight,
                                     bool isForThumbnail, int desiredFrameNumber);

    // For passing InferenceEngine bounding box results of cardiac ventricles.
    //void    setBoundingBoxes(BoxPredictionResults& boxResults);

    void    setLive(bool isLive);
    bool    isLive() { return (mIsLive); };
    void    setTintAdjustment(float coeffR, float coeffG, float coeffB);
    void    setPeakMeanDrawing(bool peakDrawing, bool meanDrawing);

    // getScrollMode related parameters
    bool    getScrollModeRecordParams(ScrollModeRecordParams& smParams, uint32_t frameNum = -1u);
    bool    getDARecordParams(DaEcgRecordParams& daParams, uint32_t frameNum = -1u);
    bool    getECGRecordParams(DaEcgRecordParams& ecgParams, uint32_t frameNum = -1u);

    // for getting mline time in the single frame texture mode
    float   getTraceLineTime();
    void    setTraceLineTime(float traceLineTime);
    float   getDopplerPeakMax();
    void    getDopplerPeakMaxCoordinate(float* mapMat, int arrayLength, bool isTracelineInvert);

    int     setTimeAvgCoOrdinates(float* peakMaxPositive, float* peakMeanPositive, float* peakMaxNegative, float* peakMeanNegative);

    uint32_t getFrameIntervalMs();
    uint32_t getMLinesPerFrame();

    float   getECGTimeSpan() { return (mECGTimeSpan);};
    float   getDATimeSpan()  { return (mDATimeSpan);};

    bool    getIsDAOn() { return (mIsDAOn); };
    bool    getIsECGOn() { return (mIsECGOn); };

    // set/update ColorMap with ColorMapId
    ThorStatus  setColorMap(int colorMapId, bool isInverted);

    float getScale();

    void setScale(float scaleFactor);
    void setPan(float deltaX,float deltaY);

    void attachCallbacks(ReportHeartRate updateHeartRate, ReportError errorReporter);

    void updateDopplerBaselineShiftInvert(uint32_t baselineShiftIndex, bool isinvert);

    ImguiRenderer& getImguiRenderer();

    bool isInitialised() const;

private: // Methods
    static void* workerThreadEntry(void* thisPtr);

    void        threadLoop();

    ThorStatus  prepareRenderers();
    ThorStatus  prepareAudioPlayer(ThorAudioPlayer** audioPlayerPtrPtr);
    void        prepareHeartRate();
    void        processDopplerTransition(uint32_t frameNum);
    void        prepareTransitionRendering();
    void        renderTransitionFrame();

private: // Properties
    CineBuffer&                 mCineBuffer;
    AIManager&                  mAIManager;
    ANativeWindow*              mWindowPtr;
    LiveRenderSurface           mRenderSurface;
    BModeRenderer               mBModeRenderer;
    CModeRenderer               mCModeRenderer;
    ImguiRenderer               mImguiRenderer;
    ScrollModeRenderer          mSModeRenderer;     // ScrollMode - M/PW/CW
    WaveformRenderer            mDARenderer;
    WaveformRenderer            mECGRenderer;
    ThorAudioPlayer*            mAudioPlayer;

    TriggeredValue              mDataEvent; // dauDataTypes:FrameNum  32:32 bits
    Event                       mStopEvent;
    TriggeredValue              mRunningEvent;
    Event                       mPausePlaybackEvent;
    pthread_t                   mThreadId;
    uint32_t                    mImagingMode;

    float                       mBModeLayout[4];
    float                       mMModeLayout[4];

    float                       mUSLayout[4];
    float                       mDALayout[4];
    float                       mECGLayout[4];
    float                       mDOPLayout[4];

    float                       mDALineColor[3];
    float                       mECGLineColor[3];

    float                       mECGTimeSpan;
    float                       mDATimeSpan;
    uint32_t                    mDASamplesPerFrame;
    uint32_t                    mDASampleRate;
    uint32_t                    mDopplerAudioSamplesPerFrame;
    uint32_t                    mDopplerAudioSampleRate;

    bool                        mIsDAOn;
    bool                        mIsECGOn;
    bool                        mIsUSOn;

    float                       mTintR;
    float                       mTintG;
    float                       mTintB;

    bool                        mIsInitialized;
    bool                        mIsLive;
    bool                        mPrepareTransitionFrame;
    int                         mTransitionImgMode;
    ReportHeartRate             mReportHeartRate;
    float                       mPrevDisplayHR;
    uint32_t                    mHRUpdateInterval;      // numFrames
    ReportError                 mErrorReporter;

    // zoom pan params
    CriticalSection             mLock;
};
