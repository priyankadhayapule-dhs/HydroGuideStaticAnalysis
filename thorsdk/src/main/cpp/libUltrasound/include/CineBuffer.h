//
// Copyright 2017 EchoNous Inc.
//
//

#pragma once

#include <memory>
#include <vector>
#include <ThorConstants.h>
#include <ThorUtils.h>
#include <ThorError.h>
#include <CriticalSection.h>
#include <UpsReader.h>
#include <ScanConverterParams.h>
#include <DaEcgRecordParams.h>
#include <ScrollModeRecordParams.h>
#include <ThorCapnpTypes.capnp.h>

class CineBuffer
{
public: // CineParams
    typedef struct
    {
        ScanConverterParams                 converterParams;
        ScanConverterParams                 colorConverterParams;
        uint32_t                            imagingCaseId;
        uint32_t                            probeType;
        uint32_t                            organId;
        uint32_t                            dauDataTypes;
        uint32_t                            imagingMode;
        uint32_t                            frameInterval;
        float                               frameRate;
        std::shared_ptr<UpsReader>          upsReader;
        float                               scrollSpeed;        // lines per second
        uint32_t                            targetFrameRate = 30;

        uint32_t                            mlinesPerFrame;
        uint32_t                            mSingleFrameWidth;
        float                               stillTimeShift;
        float                               stillMLineTime;

        uint32_t                            colorMapIndex;
        bool                                isColorMapInverted;

        uint32_t                            ecgSamplesPerFrame;
        float                               ecgSamplesPerSecond;
        uint32_t                            daEcgScrollSpeedIndex = 1;

        uint32_t                            daSamplesPerFrame;
        float                               daSamplesPerSecond;

        int32_t                             ecgSingleFrameWidth;
        int32_t                             ecgTraceIndex;

        int32_t                             daSingleFrameWidth;
        int32_t                             daTraceIndex;

        bool                                isEcgExternal;
        uint32_t                            ecgLeadNumExternal;

        uint32_t                            pwPrfIndex;
        uint32_t                            pwGateIndex;

        uint32_t                            cwPrfIndex;
        uint32_t                            cwDecimationFactor;

        uint32_t                            dopplerBaselineShiftIndex;
        bool                                dopplerBaselineInvert;
        uint32_t                            dopplerSamplesPerFrame;
        uint32_t                            dopplerFFTSize;              // can be different from window size: expect PW(256), CW(512)
        float                               dopplerNumLinesPerFrame;     // FFT output lines
        uint32_t                            dopplerSweepSpeedIndex;
        uint32_t                            dopplerFFTNumLinesToAvg;     // NumLines to Average - decimation
        uint32_t                            dopplerAudioUpsampleRatio;
        float                               dopplerVelocityScale;
        uint32_t                            dopplerFftSmoothingNum;
        float                               dopplerWFGain;
        bool                                dopplerPeakDraw = false;
        bool                                dopplerMeanDraw = false;
        float                               dopplerPeakMax;               // Peak Velocity Max

        bool                                isPWTDI = false;

        // Doppler transition-related parameters
        int                                 colorBThreshold;
        int                                 colorVelThreshold;
        bool                                isTransitionRenderingReady;
        int                                 transitionImagingMode;
        int                                 transitionAltImagingMode;
        int                                 renderedTransitionImgMode;
        // Zoom Pan Flip
        float                               zoomPanParams[5];
        // traceIndex
        int                                 traceIndex;

        // M-mode SweepSpeedIndex
        int                                 mModeSweepSpeedIndex;

        // color CPD (Color Power Doppler)
        uint32_t                            colorDopplerMode = COLOR_MODE_CVD; // CVD / CPD

    } CineParams;

public: // Client Callback
    class CineCallback
    {
    public: // Methods
        CineCallback() {}
        virtual ~CineCallback() {}

        virtual ThorStatus  onInit() { return(THOR_OK); }
        virtual void        onTerminate() {}

        virtual void        setParams(CineParams& params)
        {
            UNUSED(params);
        }

        virtual void        onData(uint32_t frameNum, uint32_t dauDataTypes)
        {
            UNUSED(frameNum);
            UNUSED(dauDataTypes);
        }

        virtual void        pausePlayback() {}

        virtual void        onSave(uint32_t startFrame, uint32_t endFrame, echonous::capnp::ThorClip::Builder &builder)
        {
            UNUSED(startFrame);
            UNUSED(endFrame);
            UNUSED(builder);
        }

        virtual void        onLoad(echonous::capnp::ThorClip::Reader &reader)
        {
            UNUSED(reader);
        }
    };

public: // Frame Header
    typedef struct __attribute__((packed, aligned(1)))
    {
        uint32_t    frameNum;   // Required - Frame number from the Dau
        uint64_t    timeStamp;  // Required - Timestamp of aquisition from the Dau
        uint32_t    numSamples; // Optional - Parameter added by image processing
                                // stage to indicate number of samples in the frame.
        float       heartRate;  // Optional - heartRate
        uint32_t    statusCode; // Optional
    } CineFrameHeader;

    typedef struct __attribute__((packed, aligned(1)))
    {
        uint32_t    frameNum;   // Required - Frame number from the Dau
        uint64_t    timeStamp;  // Required - Timestamp of aquisition from the Dau
        uint32_t    numSamples; // Optional - Parameter added by image processing
        // stage to indicate number of samples in the frame.
    } CineFrameHeaderV3;

public: // To specify what to include in getFrame().
    enum class FrameContents
    {
        IncludeHeader,  // Return pointer to complete frame [header + data]
        DataOnly,       // Only return frame data
    };

public: // Methods
    CineBuffer();
    ~CineBuffer();

    CineBuffer(const CineBuffer& that) = delete;
    CineBuffer& operator=(CineBuffer const&) = delete;

    void        reset();
    ThorStatus  prepareClients();
    void        freeClients();

    void        onSave(uint32_t startFrame, uint32_t endFrame,
                       echonous::capnp::ThorClip::Builder &builder);
    void        onLoad(echonous::capnp::ThorClip::Reader &reader);

    void        setCineParamsOnly(CineParams& params);
    void        setParams(CineParams& params);
    CineParams& getParams();

    void        setBoundary(uint32_t minFrameNum, uint32_t maxFrameNum);
    void        setCurFrame(uint32_t frameNum);

    uint32_t    getCurFrameNum();
    uint32_t    getMinFrameNum();
    uint32_t    getMaxFrameNum();

    uint8_t*    getCurFrame(uint32_t dauDataType);
    uint8_t*    getFrame(uint32_t frameNum,
                         uint32_t dauDataType,
                         FrameContents contents = FrameContents::DataOnly);

    // Used during live imaging to indicate that a frame is complete and ready
    // for post-processing.
    void        setFrameComplete(uint32_t frameNum, uint32_t dauDataType);

    // Used during playback of pre-recorded data.
    void        replayFrame(uint32_t frameNum);

    // Used to pause the playback for AudioPlayer
    void        pausePlayback();

    // It is assumed that registration calls are not made while imaging / playback
    void        registerCallback(CineCallback* callbackPtr);
    void        unregisterCallback(CineCallback* callbackPtr);

    // For Doppler Transition Frames
    void        setTransitionReadyFlag(bool isReady);
    void        setRenderedTransitionImgMode(int imgMode);

private: // Properties
    // Protects mCurFameNum, mMaxFrameNum and mCompletions
    CriticalSection             mLock;

    uint32_t                    mCurFrameNum;
    uint32_t                    mMaxFrameNum;

    CineParams                  mParams;

    static const int CINE_HEADER_SIZE = sizeof(CineFrameHeader);

#define CINE_FRAME_COUNT 1200
#define TEX_FRAME_COUNT 3
#define TEX_FRAME_TYPE_B 0
#define TEX_FRAME_TYPE_C 1
#define TEX_FRAME_TYPE_PW 2
#define TEX_FRAME_TYPE_CW 2
#define TEX_FRAME_TYPE_M 2
#define DOPPLER_PEAK_MEAN_COUNT 2

#define MAX_MODES_SIZE ((MAX_B_FRAME_SIZE + CINE_HEADER_SIZE) + \
                        (MAX_COLOR_FRAME_SIZE + CINE_HEADER_SIZE) + \
                        (MAX_DA_FRAME_SIZE + CINE_HEADER_SIZE) + \
                        (MAX_ECG_FRAME_SIZE + CINE_HEADER_SIZE))

#define CINE_MAX_SIZE ((CINE_FRAME_COUNT * MAX_MODES_SIZE) + \
                       (MAX_TEX_FRAME_SIZE + CINE_HEADER_SIZE) * TEX_FRAME_COUNT + \
                       (MAX_DOP_PEAK_MEAN_SCR_SIZE + CINE_HEADER_SIZE) * DOPPLER_PEAK_MEAN_COUNT +\
                       (MAX_DA_SCR_FRAME_SIZE + CINE_HEADER_SIZE) + \
                       (MAX_ECG_SCR_FRAME_SIZE + CINE_HEADER_SIZE))

    uint32_t                    mCompletions[CINE_FRAME_COUNT];
    uint8_t                     mCineBuffer[CINE_MAX_SIZE];

    uint8_t*                    mB_ModeBuffer;
    uint8_t*                    mColorBuffer;
    uint8_t*                    mM_ModeBuffer;
    uint8_t*                    mTexBuffer;
    uint8_t*                    mDaBuffer;
    uint8_t*                    mEcgBuffer;
    uint8_t*                    mDaScrBuffer;
    uint8_t*                    mEcgScrBuffer;
    uint8_t*                    mPw_ModeFrameBuffer;
    uint8_t*                    mPw_ModeAdoFrameBuffer;
    uint8_t*                    mCw_ModeFrameBuffer;
    uint8_t*                    mCw_ModeAdoFrameBuffer;
    uint8_t*                    mDopPeakMeanFrameBuffer;
    uint8_t*                    mDopPeakMaxScrFrameBuffer;
    uint8_t*                    mDopPeakMeanScrFrameBuffer;

    uint32_t                    mB_ModeFrameSize;
    uint32_t                    mColorFrameSize;
    uint32_t                    mM_ModeFrameSize;
    uint32_t                    mTexFrameSize;
    uint32_t                    mDaFrameSize;
    uint32_t                    mEcgFrameSize;
    uint32_t                    mPw_ModeFrameSize;
    uint32_t                    mPw_ModeAdoFrameSize;
    uint32_t                    mCw_ModeFrameSize;
    uint32_t                    mCw_ModeAdoFrameSize;
    uint32_t                    mDopPeakMeanFrameSize;

    std::vector<CineCallback*>  mCallbackList;
};
