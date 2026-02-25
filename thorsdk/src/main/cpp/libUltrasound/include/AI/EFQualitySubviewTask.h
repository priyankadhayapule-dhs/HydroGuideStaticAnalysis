#pragma once

#include <mutex>
#include <condition_variable>
#include <thread>
#include <deque>
#include <jni.h>

#include <CineBuffer.h>
#include <ScanConverterCl.h>
#include <ImguiRenderer.h>
#include <EFWorkflow.h>
#include <EFQualitySubviewConfig.h>
#include <Guidance/AnimationMap.h>
#include <VideoPlayerGL.h>
#include <ThorCapnpTypes.capnp.h>

// Forward declarations
class AIManager;
class Filesystem;
class PLAXPipeline;
class GuidanceDisplay;
class UltrasoundManager;
struct GuidanceJNI;

class EFQualitySubviewTask : public CineBuffer::CineCallback, public ImGuiDrawable
{
public:

    EFQualitySubviewTask(AIManager &ai);
    ~EFQualitySubviewTask();

    ThorStatus init(UltrasoundManager *manager);

    void start(void *javaEnv, void *javaContext);
    void stop();

    void resetQualityBuffers();

    void setView(CardiacView view);
    void setShowQuality(bool quality);
    void setShowGuidance(bool guidance);
    void setAutocaptureEnabled(bool isEnabled);
    void* setCallbackUserData(void *user);
    void* getCallbackUserData();
    void setFlip(float flipX, float flipY);

    void setLanguage(const char *lang);

    // Draw the internal state using ImGui (ImGuiDrawable override)
    virtual void loadDrawable(Filesystem *filesystem) override;
    virtual void openDrawable() override;
    virtual void draw(CineViewer &cineviewer, uint32_t frameNum) override;
    virtual void close() override;

    // CineCallback functions
    virtual ThorStatus onInit() override;
    virtual void onTerminate() override;
    virtual void setParams(CineBuffer::CineParams& params) override;
    virtual void onData(uint32_t frameNum, uint32_t dauDataTypes) override;
    virtual void onSave(uint32_t startFrame, uint32_t endFrame, echonous::capnp::ThorClip::Builder &builder) override;
    virtual void onLoad(echonous::capnp::ThorClip::Reader &reader) override;

    ThorStatus runA4CA2CVerificationOnCinebuffer(JNIEnv *jenv, const char *inputPath, const char* outputPath);
    Json::Value runA4CA2CVerificationOnFrame(JNIEnv *jenv, CardiacView view, uint32_t frameNum, FILE *raw);
    ThorStatus runPLAXVerificationOnCinebuffer(JNIEnv *jenv, const char *inputPath, const char* outputPath);


public: // Made public for testing purposes

    // Internal Structures
    struct DrawableState
    {
        bool loaded = false;
        ScanConverterParams params;
        bool drawQuality = false;
        bool drawGuidance = false;
        bool drawConfig = false;
        bool drawAutocapture = false;
        int qualityScore;
        int guidanceIndex;
        float autocaptureScore;
        float autocaptureMinThreshold;
        int autocaptureIndex;
        int fastProbeFrame = 0;
    };

    // functions
    void drawQuality(CineViewer &cineviewer, const DrawableState &state);
    void drawGuidance(CineViewer &cineviewer, const DrawableState &state, uint32_t frameNum);
    void drawConfig(CineViewer &cineviewer, const DrawableState &state);

    bool workerShouldAwaken();
    void workerThreadFunc(void *jniInterface);

    // Run processing pipeline for a frame
    ThorStatus processFrame(uint32_t framenum, CardiacView view, void *jniInterface);

    // Processing steps
    ThorStatus scanConvert(CineBuffer &cinebuffer, uint32_t frameNum, std::vector<float> &postscan);
    ThorStatus runModel(JNIEnv *jenv, std::vector<float> &postscan, CardiacView view, EFQualitySubviewPred &pred, void *jniInterface);
    void postProcessPred(EFQualitySubviewPred &pred);
    void enqueuePred(const EFQualitySubviewPred &pred);
    EFQualitySubviewPred computeMean();
    int rescaleQuality(int subviewPred, const EFQualitySubviewPred &pred, const EFQualitySubviewConfig &config);

    void storeSmoothResults(uint32_t frameNum, int subview, int quality);
    void checkAutocapture(uint32_t frameNum);
    bool checkFastProbeMotion(uint32_t frameNum);

    // Update mSmartCapturePrevScore and potentially notify listener of update
    void checkSmartCapture(uint32_t frameNum);

    bool isSmartCaptureRange(uint32_t from, uint32_t to, uint32_t *pnext);
    ThorStatus findSmartCaptureRange(uint32_t numFrames, uint32_t *startFrame, uint32_t *endFrame);

    // Member variables
    UltrasoundManager *mUSManager;
    Filesystem *mAssets;
    void *mCallbackUser;
    AIManager &mAIManager;
    CineBuffer &mCineBuffer;
    std::unique_ptr<PLAXPipeline> mPLAXPipeline;

    // Two copies of the configuration params,
    // one for reading and one for writing

    std::thread mWorkerThread;

    // Shared control state, protected by mMutex
    std::mutex mMutex;
    std::condition_variable mCV;
    bool mQualityEnabled;
    bool mGuidanceEnabled;
    bool mAutocaptureEnabled;
    bool mStop;
    uint32_t mLastProcessedQualityFrame;
    uint32_t mProcessedFrame;
    uint32_t mCineFrame;
    CardiacView mView;
    EFQualitySubviewConfig mConfig;

    float mFlipX;
    std::unique_ptr<ScanConverterCL> mScanConverter;

    // Temporal smoothing buffer
    std::deque<EFQualitySubviewPred> mPredictions;
    int mSubviewIndex;

    // Buffer for fast probe motion detection
    std::vector<int> mUnsmoothSubviews;

    // Buffers for autocapture/data save
    float mAutoCapturePrevScore;
    unsigned int mAutoCaptureBufferIndex;
    unsigned int mAutoCaptureBufferSize;
    std::vector<int> mSubviewBuffer;
    std::vector<int> mQualityBuffer;
    int mAutocaptureTimeout;
    float mSmartCapturePrevScore;
    bool mAutocaptureJustFailed;

    std::unique_ptr<GuidanceDisplay> mDisplay;
    GuidanceJNI *mJNI;
};
