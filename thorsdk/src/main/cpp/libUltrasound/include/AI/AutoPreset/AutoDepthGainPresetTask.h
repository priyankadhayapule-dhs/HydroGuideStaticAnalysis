#pragma once

#include <mutex>
#include <condition_variable>
#include <thread>
#include <deque>
#include <unordered_map>

#include <UltrasoundManager.h>
#include <CineBuffer.h>
#include <ScanConverterCl.h>
#include <ImguiRenderer.h>
#include <EFWorkflow.h>
#include <YoloBuffer.h>
#include "AutoPreset/AutoControlManager.h"
#include "AutoPreset/AutoDGPModelAbstractor.h"
#include <RealtimeModule.h>
#include "ThorRingBuffer.hpp"
#include <json/json.h>
#include <jni.h>
#include <Sync.h>
#include <AutoPresetJNI.h>
// threshold parameters
// changed from .45 to .55 for new single frame model
#define CONF_THRESH 0.55
#define CONF_THRESH_MULTIFRAME 0.65
// changed from .1 to .15 for new single frame model
#define TOP2_DIFF_THRESH 0.15
#define TOP2_DIFF_THRESH_MULTIFRAME 0.2
// unused for single frame model
#define CONF_LUNG_THRESH 0.9
#define FREQ_THRESHOLD 0.8
// Forward declarations
class AIManager;

class AutoDepthGainPresetTask : public RealtimeModule
{
public:

    AutoDepthGainPresetTask(UltrasoundManager* usManager, AIManager &ai, AutoControlManager &acManager);
    ~AutoDepthGainPresetTask();

    ThorStatus init(void * javaEnv, void* javaContext);
    void verificationInit(void* jenv);
    void start();
    void stop();

    void setFlip(float flipX, float flipY);

    // RealtimeModule functions
    virtual void setParams(CineBuffer::CineParams& params) override;
    std::pair<int, float> getView(uint32_t frameNum);

    virtual ThorStatus workerProcess(uint32_t frameNum, uint32_t dauDataTypes) override;
    virtual void workerFramesSkipped(uint32_t first, uint32_t last) override;
    virtual ThorStatus workerInit() override;
    virtual ThorStatus workerTerminate() override;

    virtual void onSave(uint32_t frameStart, uint32_t frameEnd, echonous::capnp::ThorClip::Builder& builder) override;
    virtual void onLoad(echonous::capnp::ThorClip::Reader& reader) override;
private:

    // Run processing pipeline for a frame
    ThorStatus processFrame(uint32_t framenum, void* jni);
    ThorStatus processMultiFrame(uint32_t frameNum, AutoDepthGainPresetPred &pred);
    ThorStatus processSingleFrame(uint32_t frameNum, AutoDepthGainPresetPred &pred);
    // Processing steps
    ThorStatus scanConvert(CineBuffer &cinebuffer, uint32_t frameNum, std::vector<float> &postscan);
    ThorStatus runModel(std::vector<uint8_t> &postscan, AutoDepthGainPresetPred &pred);
    ThorStatus runModelMultiFrame(AutoDepthGainPresetPred &pred);
    void postProcessPred(AutoDepthGainPresetPred &pred);
    void enqueuePred(const AutoDepthGainPresetPred &pred);
    AutoDepthGainPresetPred computeMean();

    // anatomy (preset)
    int predictMainClass(AutoDepthGainPresetPred pred);
    void enqueueFreq(const int class_id);
    float getFrequency(const int class_id);

    // process functions
    bool processAutoPreset(AutoDepthGainPresetPred meanPred, int mainClass);
    bool processAutoPresetMultiFrame(AutoDepthGainPresetPred meanPred, int mainClass);

    // verification functions
public:
    void resetAutoDepthPresetTask();
    ThorStatus runVerification(const char* outputPath);
    ThorStatus runVerificationClip(const char* inputPath, const char* outputPath, void* jni);
    Json::Value ToJson(const std::vector<AutoDepthGainPresetPred>& rawPreds, const std::vector<AutoDepthGainPresetPred>& softmaxPreds, const std::vector<AutoDepthGainPresetPred>& postPreds, const std::vector<uint32_t>& inferenceFrames, const std::vector<std::vector<float>>& mHistoricalFrameData);
    Json::Value ToJson(const AutoDepthGainPresetPred& rawPred, const AutoDepthGainPresetPred& softPred, const AutoDepthGainPresetPred& postPred, uint32_t frameNum, const std::vector<float>& frameData, int inferenceIndex);
private:
    // Member variables
    AIManager &mAIManager;
    CineBuffer &mCineBuffer;
    std::unique_ptr<ScanConverterCL> mScanConverter;
    std::vector<std::pair<int, float>>       mViewCineBuffer;

    Sync<float> mFlipX;
    // Temporal smoothing buffer
    std::deque<AutoDepthGainPresetPred> mPredictions;
    // Historical Data for onSave
    std::vector<AutoDepthGainPresetPred> mHistoricalPreds;
    std::vector<AutoDepthGainPresetPred> mHistoricalSoftmaxedPreds;
    std::vector<AutoDepthGainPresetPred> mHistoricalPostProcessedPreds;
    std::vector<uint32_t> mHistoricalInferenceFrames;
    std::vector<std::vector<float>> mHistoricalFrameData;
    std::vector<int> mHistoricalMainClassIds;
    std::vector<int> mHistoricalDecisionResults;
    std::vector<std::deque<int>> mHistoricalFrequencyBuffer;
    std::vector<std::deque<AutoDepthGainPresetPred>> mHistoricalSmoothedPredictions;
    bool mVerificationActive; // When active, do not init JNI for worker threads
    // Frequency Buffer (updated at the specifed interval)
    std::deque<int> mFrequency;

    static constexpr int AUTO_PRESET_BUFFER_SIZE = 5;
private:
    void resetSmoothingBuffer();
    AutoPresetJNI* mJNI;
    ImguiRenderer &mImguiRenderer;
    CineBuffer::CineParams mParams;
    UltrasoundManager* mUSManager;
    AutoControlManager &mAutoControlManager;
    std::vector<int> mFrameWasSkipped;
    std::mutex mMutex;
    std::thread::id mWorkerThreadId;
    bool mHasParams;
    ThorRingBuffer<std::vector<float>, AUTO_PRESET_BUFFER_SIZE> mAutoPresetBuffer;
    double mAvgProcessingTime;
    uint64_t mFramesProcessed;
    double mTotalFrameProcessingTime;
    double mAvgModelProcessingTime;
    uint64_t mModelFramesProcessed;
    double mTotalModelProcessingTime;
    double mSingleFrameHighTime;
    double mSingleFrameLowTime;
    double mMultiFrameHighTime;
    double mMultiFrameLowTime;
    bool mJniInit;
};
