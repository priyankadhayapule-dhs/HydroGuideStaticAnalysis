//
// Copyright 2016 EchoNous Inc.
//
//

#pragma once

#include <sys/timerfd.h>
#include <memory>
#include <vector>
#include <mutex>
#include <android/native_window.h>
#include <android/asset_manager.h>
#include <Event.h>
#include <DauSignalHandler.h>
#include <DauContext.h>
#include <ThorError.h>
#include <DmaBuffer.h>
#include <DauMemory.h>
#include <PciDauMemory.h>
#include <DauSequenceBuilder.h>
#include <DauLutManager.h>
#include <DauTbfManager.h>
#include <DauSseManager.h>
#include <DauInputManager.h>
#include <DauGainManager.h>
#include <DauHw.h>
#include <UpsReader.h>
#include <ThorConstants.h>
#include <UsbDataHandler.h>
#include <TriggeredValue.h>
#include <DauError.h>
#include <ParallelImageProcessor.h>
#include <DeInterleaveProcess.h>
#include <DeInterleaveMProcess.h>
#include <AutoGainProcess.h>
#include <SpeckleReductionProcess.h>
#include <ColorProcess.h>
#include <MFilterProcess.h>
#include <PwLowPassDecimationFilterProcess.h>
#include <CwBandPassFilterProcess.h>
#include <DopplerWallFilterProcess.h>
#include <DopplerSpectralProcess.h>
#include <DopplerSpectralSmoothingProcess.h>
#include <DopplerHilbertProcess.h>
#include <DopplerAudioUpsampleProcess.h>
#include <EcgProcess.h>
#include <DaProcess.h>
#include <ColorAcq.h>
#include <ColorAcqLinear.h>
#include <DopplerAcq.h>
#include <DopplerAcqLinear.h>
#include <CWAcq.h>
#include <MmodeAcq.h>
#include <CineBuffer.h>
#include <CineViewer.h>
#include <ApiManager.h>
#include <DauArrayTest.h>
#include <DopplerPeakMeanProcessHandler.h>
#include <SpatialCompoundProcess.h>
#include <DauRegRWService.h>

// If defined then the DMA Fifo will be populated with test data.
#define GENERATE_TEST_IMAGES 1

// Callback function for reporting errors.  Intended to be used as a callback
// to JNI layer for propogating errors to Java UI.
typedef void (*reportErrorFunc)(uint32_t);

// Callback function for reporting HID input.  Will propogate this input to
// the Java UI via JNI layer.
typedef void (*reportHidFunc)(uint32_t);

// Callback function for reporting external ECG lead connection.  Will propogate
// this to the Java UI via JNI layer.
typedef void (*reportExternalEcgFunc)(bool);

typedef struct
{
    uint32_t        probeType;
    std::string     serialNo;
    std::string     part;
    std::string     fwVersion;
} ProbeInfo;

typedef enum {
    SAFETY_DAU_TEMP_TEST = 1,
    SAFETY_DAU_WD_RESET_TEST,
    SAFET_DATA_OVERFLOW_TEST,
    SAFETY_DATA_UNDERFLOW_TEST,
} SafetyTest;

class Dau
{
public: // Methods
    Dau(DauContext& dauContext,
        CineBuffer& cineBuffer,
        CineViewer& cineViewer,
        const std::string& appPath);

    ~Dau();

    ThorStatus open(const std::shared_ptr<UpsReader>& upsReader, 
                    reportErrorFunc errFuncPtr,
                    reportHidFunc hidFuncPtr,
                    reportExternalEcgFunc extEcgFuncPtr);

    void close();

    bool       start();
    ThorStatus stop();
    ThorStatus reset();

    ThorStatus setImagingCase(uint32_t imagingCaseId,
                                   float apiEstimates[8],
                                   float imagingInfo[8]);
    ThorStatus setColorImagingCase(uint32_t imagingCaseId,
                                   int colorParams[3],
                                   float roiParams[4],
                                   float apiEstimates[8],
                                   float imagingInfo[8]);
    ThorStatus setMmodeImagingCase(uint32_t imagingCaseId,
                                   uint32_t scrollSpeedId,
                                   float mLinePosition,
                                   float apiEstimates[8],
                                   float imagingInfo[8]);
    ThorStatus setPwImagingCase(uint32_t imaginCaseId,
                                int pwParams[3],
                                float gateParams[2],
                                float apiEstimages[8],
                                float imagingInfo[8],
                                bool isTDI);
    ThorStatus setCwImagingCase(uint32_t imaginCaseId,
                                int cwParams[3],
                                float gateParams[2],
                                float apiEstimages[8],
                                float imagingInfo[8]);
    ThorStatus setMLine(float mLinePosition);
    ThorStatus runTransducerElementCheck( int results[] );
    void       enableImageProcessing(int type, bool enable);
    void       setGain(int gain);
    void       setGain(int nearGain, int farGain);
    void       setColorGain(int gain);
    void       setEcgGain(float gain);
    void       setDaGain(float gain);
    void       setPwGain(int gain);
    void       setCwGain(int gain);
    void       setPwAudioGain(int gain);
    void       setCwAudioGain(int gain);
    void       setUSSignal(bool isUsSignal);
    void       setECGSignal(bool isECGSignal);
    void       setDASignal(bool isDASignal);
    void       getFov(int depthId, int imagingMode,float fov[]);
    void       getDefaultRoi(int organId, int depthId, float roi[]);
    uint32_t   getFrameIntervalMs();
    uint32_t   getMLinesPerFrame();
    bool       getExternalEcg();
    void       setExternalEcg(bool isExternalECG);
    void       setLeadNumber(int leadNumber);
    void       getProbeInfo(ProbeInfo *);
    bool       supportsEcg();
    bool       supportsDa();
    const char* getDbName();
    void       setDebuggingFlag(bool flag);
    bool       isTransducerElementCheckRunning();
    void       setDauSafetyFeatureTestOption(int gSafetyTestSelectedOption,
                                             float gDauThresholdTemperatureForTest);

private: // Signal Callback Handler
    class SignalCallback : public DauSignalHandler::SignalCallback
    {
    public:
        SignalCallback(void* thisPtr) : DauSignalHandler::SignalCallback(thisPtr),
                                        mPrevEcgConnect(false) {}

    private: // Methods
        ThorStatus  onInit();
        ThorStatus  onData(uint32_t count);
        void        onHid(uint32_t hidId);
        void        onExternalEcg(bool isConnected);
        ThorStatus  onCheckTemperature();
        void        onError(uint32_t errorCode);
        void        onTest(void* testDataPtr);
        void        onTerminate();

    private: // Properties
        uint8_t*        mDmaFifoPtr;
        uint8_t*        mColorDmaFifoPtr;
        uint8_t*        mMModeDmaFifoPtr;
        uint8_t*        mPwModeDmaFifoPtr;
        uint8_t*        mCwModeDmaFifoPtr;
        uint8_t*        mEcgFifoPtr;
        uint8_t*        mDaFifoPtr;
        uint32_t        mImagingMode;
        uint32_t        mDauDataTypes;
        bool            mPrevEcgConnect;

        typedef CineBuffer::CineFrameHeader CineHeader;
        typedef CineBuffer::FrameContents   FrameContents;
    };

private: // Methods
    Dau();
    ThorStatus      getApiEstimates( uint32_t imagingCaseId, float apiEstimates[8] );
    ThorStatus      getApiEstimatesPW( uint32_t imagingCaseId, uint32_t organId, float gateRange, float apiEstimates[8] );
    void            getImagingInfo( uint32_t imagingCaseId, float imagingInfo[8] );
    void            getImagingInfoLinear( uint32_t imagingCaseId, float imagingInfo[8],float gateAxialStart, uint32_t gateIndex,
                                          float gateAzimuthLoc, float gateAngle);
    void            readProbeInfo();
    void            setDaEcgCineParams(CineBuffer::CineParams& cineParams);
    ThorStatus      bootAppImage(const std::shared_ptr<DauMemory>& daumSPtr);

#ifdef GENERATE_TEST_IMAGES
    void            populateDmaFifo();
#endif

private: // Enumerations
    enum
    {
        IMAGE_PROCESS_AUTO_GAIN = 0,
        IMAGE_PROCESS_DESPECKLE
    };

private: // Properties
    DauContext&                         mDauContext;
    DauSignalHandler*                   mDauSignalHandlerPtr;
    SignalCallback                      mCallback;
    AAssetManager*                      mAssetManagerPtr;
    std::shared_ptr<UpsReader>          mUpsReaderSPtr;
    std::shared_ptr<DauMemory>          mDaumSPtr;
    std::shared_ptr<DauMemory>          mDaumBar2SPtr;
    std::shared_ptr<DauHw>              mDauHwSPtr;
    DauLutManager*                      mLutManagerPtr;
    DauSequenceBuilder*                 mSequenceBuilderPtr;
    DauTbfManager*                      mTbfManagerPtr;
    DauSseManager*                      mSseManagerPtr;
    std::shared_ptr<DauInputManager>    mInputManagerSPtr;
    DauGainManager*                     mGainManagerPtr;
    ApiManager*                         mApiManagerPtr;
    ParallelImageProcessor              mBModeProcessor;
    ParallelImageProcessor              mColorProcessor;
    ParallelImageProcessor              mMModeProcessor;
    ParallelImageProcessor              mPwModeCommonProcessor;
    ParallelImageProcessor              mPwModeSpectralProcessor;
    ParallelImageProcessor              mPwModeAudioProcessor;
    ParallelImageProcessor              mCwModeCommonProcessor;
    ParallelImageProcessor              mCwModeSpectralProcessor;
    ParallelImageProcessor              mCwModeAudioProcessor;
    DauArrayTest*                       mDauArrayTestPtr;
    DopplerPeakMeanProcessHandler*      mDopplerPeakMeanProc;
    ProbeInfo                           mProbeInfo;

    EcgProcess*                         mEcgProcessPtr;
    DaProcess*                          mDaProcessPtr;
    
    ColorAcq*                           mColorAcqPtr;
    ColorAcqLinear*                     mColorAcqLinearPtr;
    MmodeAcq*                           mMmodeAcqPtr;
    DopplerAcq*                         mDopplerAcqPtr;
    DopplerAcqLinear*                   mDopplerAcqLinearPtr;
    CWAcq*                              mCwAcqPtr;
    std::shared_ptr<DmaBuffer>          mDmaBufferSPtr;
    DauError*                           mDauErrorPtr;
    reportErrorFunc                     mReportErrorFuncPtr;
    reportHidFunc                       mReportHidFuncPtr;
    reportExternalEcgFunc               mExternalEcgFuncPtr;
    uint32_t                            mImagingMode;
    uint32_t                            mDauDataTypes;
    int                                 mColorGainPercent;
    uint32_t                            mImagingCase;
    uint32_t                            mMLinesPerFrame;
    int32_t                             mUpsReleaseType;
    float                               mHvVoltage;
    std::shared_ptr<TriggeredValue>     mUsbDataEventSPtr;
    std::shared_ptr<TriggeredValue>     mUsbHidEventSPtr;
    std::shared_ptr<TriggeredValue>     mUsbErrorEventSPtr;
    std::shared_ptr<TriggeredValue>     mUsbExternalEcgEventSPtr;
    std::shared_ptr<UsbDataHandler>     mUsbDataHandlerSPtr;
    DauRegRWService*                    mDauRegRWServicePtr;
    bool                                mIsUsSignal;
    bool                                mIsECGSignal;
    bool                                mIsDASignal;
    bool                                mIsDauStopped;
    bool                                mIsCompounding;
    bool                                mUpdateCompoundFrames;
    bool                                mDebugFlag;
    bool                                mIsDauAttached;
    CineBuffer&                         mCineBuffer;
    CineViewer&                         mCineViewer;
    std::string                         mAppPath;
    std::mutex                          mLock;

    CriticalSection                     mTempExtEcgLock;
    bool                                mTempExtEcg;
    uint32_t                            mLeadNoExternal;

    int                                 mTimerFd;
    struct itimerspec                   mTimerSpec;

    uint8_t                             mDopplerIntBuffer[2][MAX_CW_SAMPLES_PER_FRAME * sizeof(float) * 2 +
                                                            sizeof(CineBuffer::CineFrameHeader)];
    float                               mDauTempThresholdVal;
    bool                                mIsTempSafetyTest;
    bool                                mIsWatchDogResetTest;
    bool                                mIsDataUnderflowTest;
    bool                                mIsDataOverflowTest;
};
