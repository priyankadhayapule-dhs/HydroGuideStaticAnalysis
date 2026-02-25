#pragma once

#include <mutex>
#include <condition_variable>
#include <thread>
#include <deque>

#include <CineBuffer.h>
#include <ScanConverterCl.h>
#include <ImguiRenderer.h>
#include <EFWorkflow.h>
#include <FASTObjectDetectorModel.h>
#include <UltrasoundManager.h>
#include <FASTYoloBuffer.h>

class AIManager;
struct FastExamJNI;
class FASTObjectDetectorTask : public CineBuffer::CineCallback, public ImGuiDrawable
{
public:

    FASTObjectDetectorTask(UltrasoundManager *usm);
    ~FASTObjectDetectorTask();

    ThorStatus init();
    void start(void *javaEnv, void *javaContext);
    void stop();

    void setView(CardiacView view);
    void setPause(bool pause);
    void setFlip(float flipX, float flipY);

    // ImGuiDrawable functions
    void addTextRanges(ImFontGlyphRangesBuilder& builder);
    void setLanguage(const char *lang);
    virtual void draw(CineViewer &cineviewer, uint32_t frameNum) override;
    virtual void close() override;

    // CineCallback functions
    virtual ThorStatus onInit() override;
    virtual void onTerminate() override;
    virtual void setParams(CineBuffer::CineParams& params) override;
    virtual void onData(uint32_t frameNum, uint32_t dauDataTypes) override;
    virtual void onSave(uint32_t startFrame, uint32_t endFrame,
                        echonous::capnp::ThorClip::Builder &builder) override;
    virtual void onLoad(echonous::capnp::ThorClip::Reader &reader) override;

    // Convert predictions from image space to phsyical space
    void convertToPhysicalSpace(float flipX);

    const std::vector<FASTYoloPrediction>& getPredictions(uint32_t frameNum) const;
    std::pair<int, float> getView(uint32_t frameNum) const;

    ThorStatus filterView(float *viewscore);
    ThorStatus multiplyBoxConfs(const float *viewscore, std::vector<FASTYoloPrediction> &predictions);

    //Verification
    ThorStatus runVerificationTest(JNIEnv *jenv, const char *clip, const char *outRoot);

private:

    bool workerShouldAwaken();

    void workerThreadFunc(void* jniInterface);
    ThorStatus processFrame(uint32_t frameNum, float flipX, void* jni);
    const char *getLocalizedString(const char *id) const;

    bool mHasParams = false;
    CineBuffer::CineParams mParams;
    UltrasoundManager *mUSManager;
    CineBuffer *mCB;
    FASTObjectDetectorModel mEnsemble[FAST_OBJ_DETECT_ENSEMBLE_SIZE];
    int mEnsembleIndex;
    std::vector<FASTYoloPrediction> mPredictions;   // Predictions for current frame
    FASTYoloBuffer mYoloBuffer;

    // Buffer of frame -> predictions for that frame
    std::vector<std::vector<FASTYoloPrediction>> mUnsmoothPredictionBuffer;
    std::vector<std::vector<FASTYoloPrediction>> mPredictionBuffer;
    std::vector<std::pair<int, float>>           mViewCineBuffer;
    std::vector<int>                             mFrameWasSkipped;

    bool mInit;
    bool mPaused;
    bool mStop;
    uint32_t mProcessedFrame;
    uint32_t mCineFrame;
    CardiacView mView;

    std::mutex mMutex;
    std::condition_variable mCV;
    std::thread mWorkerThread;

    float mFlipX;
    std::unique_ptr<ScanConverterCL> mScanConverter;
    ImguiRenderer &mImguiRenderer;

    bool mIsDevelopMode = false;

    // View probabilities for kalman style filtering
    std::vector<float> mViewProb;
    // Index of view noise class
    unsigned int mNoiseIndex;

    Json::Value mLocaleMap;
    const Json::Value *mCurrentLocale;

    void *mJNI;
};
