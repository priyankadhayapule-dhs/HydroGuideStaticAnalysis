#pragma once

#include <mutex>
#include <condition_variable>
#include <thread>
#include <deque>

//#include <AIModel.h>
#include <CineBuffer.h>
#include <ScanConverterCl.h>
#include <ImguiRenderer.h>
#include <EFWorkflow.h>
#include <AnatomyClassifierConfig.h>
#include <AutoPreset/AutoControlManager.h>

// threshold parameters
#define CONF_THRESH 0.45
#define TOP2_DIFF_THRESH 0.1
#define CONF_LUNG_THRESH 0.9
#define FREQ_THRESHOLD 0.5

// Forward declarations
class AIManager;
class AssetReader;

class AnatomyClassifierTask : public CineBuffer::CineCallback
{
public:

    AnatomyClassifierTask(AIManager &ai, AutoControlManager &acManager);
    ~AnatomyClassifierTask();

    ThorStatus init(AssetReader *assets);

    void start();
    void stop();

    void setPause(bool pause);
    void setFlip(float flipX, float flipY);

    // CineCallback functions
    virtual ThorStatus onInit() override;
    virtual void onTerminate() override;
    virtual void setParams(CineBuffer::CineParams& params) override;
    virtual void onData(uint32_t frameNum, uint32_t dauDataTypes) override;

public: // Made public for testing purposes

    bool workerShouldAwaken();
    void workerThreadFunc();

    // Run processing pipeline for a frame
    ThorStatus processFrame(uint32_t framenum);

    // Processing steps
    ThorStatus scanConvert(CineBuffer &cinebuffer, uint32_t frameNum, std::vector<float> &postscan);
    ThorStatus runModel(std::vector<float> &postscan, AnatomyClassifierPred &pred);
    void postProcessPred(AnatomyClassifierPred &pred);
    void enqueuePred(const AnatomyClassifierPred &pred);
    AnatomyClassifierPred computeMean();

    int predictMainClass();
    void enqueueFreq(const int class_id);
    float getFrequency(const int class_id);

    // Member variables
    AIManager &mAIManager;
    CineBuffer &mCineBuffer;
    AutoControlManager &mAutoControlManager;
//    std::shared_ptr<AIModel> mModel;

    std::thread mWorkerThread;

    // Shared control state, protected by mMutex
    std::mutex mMutex;
    std::condition_variable mCV;
    bool mStop;
    bool mPaused;

    uint32_t mProcessedFrame;
    uint32_t mCineFrame;

    uint32_t mLastPredFrameNo;

    float mFlipX;
    std::unique_ptr<ScanConverterCL> mScanConverter;

    // Temporal smoothing buffer
    std::deque<AnatomyClassifierPred> mPredictions;

    // Frequency Buffer (updated at the specifed interval)
    std::deque<int> mFrequency;

private:
    ImguiRenderer &mImguiRenderer;
};
