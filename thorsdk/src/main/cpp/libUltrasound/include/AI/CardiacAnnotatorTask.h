#pragma once

#include <mutex>
#include <condition_variable>
#include <thread>
#include <deque>

#include <CineBuffer.h>
#include <ScanConverterCl.h>
#include <ImguiRenderer.h>
#include <EFWorkflow.h>
#include <CardiacAnnotator.h>
#include <TDIAnnotator.h>
#include <UltrasoundManager.h>
#include <YoloBuffer.h>
#include <TDIYoloBuffer.h>
#include <AutoDopplerPWAnnotator.h>
#include <AI/Guidance/PhraseMap.h>

#include <AutoDopplerPWYoloBuffer.h>
#include <AutoDopplerPWAnnotatorConfig.h>
class AIManager;
struct PWVerificationResult {
    std::vector<uint8_t> postscan;
    std::vector<float>   viewRaw;
    std::vector<float>   yoloRaw;
    std::vector<float>   yoloProc;
    Json::Value json;
};
struct TDIVerificationResult {
    std::vector<uint8_t> postscan;
    std::vector<float>   viewRaw;
    std::vector<float>   yoloRaw;
    std::vector<float>   yoloProc;

    Json::Value json;
};
/// Target object to use in auto doppler gate placement
/// Set to None for normal autolabel behavior
enum class AutoDopplerTarget {
    None,          // None: show normal autolabels (default)
    PLAX_RVOT_PV,  // Pulmonary valve, PLAX RVOT view
    PLAX_AV_PV,    // Pulmonary valve, PLAX AV view
    A4C_MV,        // Mitral Valve, A4C view
    A4C_TV,        // Tricuspid Valve, A4C view
    A5C_LVOT,      // LVOT, A5C view
    TDI_A4C_MVSA,  // MV Septal Annulus, TDI mode
    TDI_A4C_MVLA,  // MV Lateral Annulus, TDI mode
    TDI_A4C_TVLA   // TV Lateral Annulus, TDI mode
};

/// Type of callback invoked on auto doppler gate location found/location updated
typedef void (*AutoDopplerCallback)(void* user, float x, float y);

#define DOP_BUFFER_LENGTH 30

struct DopplerGateMinMax
{
    float min_x;
    float max_x;
    float min_y;
    float max_y;
    float max_w;
    float max_h;
};

struct DopplerGateBox
{
    float x;
    float y;
    float w;
    float h;
};

class CardiacAnnotatorTask : public CineBuffer::CineCallback, public ImGuiDrawable
{
public:

    CardiacAnnotatorTask(UltrasoundManager *usm);
    ~CardiacAnnotatorTask();

    ThorStatus init();
    void start(void *javaEnv, void *javaContext);
    void stop();

    void setView(CardiacView view);
    void setPause(bool pause, AutoDopplerTarget target = AutoDopplerTarget::None);
    void setFlip(float flipX, float flipY);

    /// Set the callback function to trigger when auto doppler gate location is found. When found, the callback will be called
    /// with the user pointer, and the physical x and y coordinates to place the gate.
    /// If the callback is NULL, then no callback will occur.
    /// Only one callback function may be registered at a time, a new callback function will overwrite the previous.
    ///
    /// Returns previous user data, if any
    void* setAutoDopplerCallback(void* user, /* nullable */ AutoDopplerCallback callback);
    void setMockAutoDoppler(bool mock);

    // ImGuiDrawable functions
    virtual void draw(CineViewer &cineviewer, uint32_t frameNum) override;
    virtual void close() override;

    // CineCallback functions
    virtual ThorStatus onInit() override;
    virtual void onTerminate() override;
    virtual void setParams(CineBuffer::CineParams& params) override;
    virtual void onData(uint32_t frameNum, uint32_t dauDataTypes) override;
    virtual void onSave(uint32_t startFrame, uint32_t endFrame,
                        echonous::capnp::ThorClip::Builder &builder);
    virtual void onLoad(echonous::capnp::ThorClip::Reader &reader);

    // Convert predictions from image space to phsyical space
    void convertToPhysicalSpace();
    void convertToPhysicalSpaceTDI();
    void convertToPhysicalSpacePW();
    const std::vector<YoloPrediction>& getPredictions(uint32_t frameNum) const;
    std::pair<int, float> getView(uint32_t frameNum) const;

    ThorStatus runVerificationOnCineBuffer(JNIEnv *jenv,const char *inputFile, const char *outputRoot, int workflow = 0); // 0 = Autolabel, 1 = PW, 2 = TDI
    ThorStatus runVerificationClip(const char *thor, const char *outdir, JNIEnv *jenv, int workflow);
    ThorStatus verifyFrameAutoLabelMode(uint32_t frameNum, AutoLabelVerificationResult &result, JNIEnv *jenv);

    void addTextRanges(ImFontGlyphRangesBuilder& builder);
    void setLanguage(const char *lang);

private:
    bool isTDIModeTarget(AutoDopplerTarget target) const;
    int autoDopplerTargetIndex(AutoDopplerTarget target) const;
    vec2 autoDopplerAnchorPoint(AutoDopplerTarget target) const;

    bool workerShouldAwaken();

    void workerThreadFunc(void *jni);
    ThorStatus processFrame(uint32_t frameNum, float flipX, void *jni, AutoDopplerTarget target);
    ThorStatus processAutoDopplerGate(uint32_t frameNum);
    const char *getLocalizedString(const char *id) const;

    CineBuffer::CineParams mParams;
    UltrasoundManager *mUSManager;
    CineBuffer *mCB;
    CardiacAnnotator mAnnotator;
    TDIAnnotator mTDIAnnotator;
    AutoDopplerPWAnnotator mPWAnnotator;
    std::vector<YoloPrediction> mPredictions;   // Predictions for current frame
    std::vector<TDIYoloPrediction> mTdiPredictions;   // Predictions for current frame
    YoloBuffer mYoloBuffer;

    std::vector<float> mTDIView;
    std::vector<TDIYoloPrediction> mTDIPredictions;
    TDIYoloBuffer mTDIYoloBuffer;

    std::vector<float> mPWView;
    std::vector<PWYoloPrediction> mPWPredictions;
    AutoDopplerPWYoloBuffer mPWYoloBuffer;
    // Buffer of frame -> predictions for that frame
    std::vector<std::vector<YoloPrediction>> mUnsmoothPredictionBuffer;
    std::vector<std::vector<YoloPrediction>> mPredictionBuffer;
    std::vector<std::pair<int, float>>       mViewCineBuffer;
    std::vector<int>                         mFrameWasSkipped;

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
    Json::Value mLocaleMap;
    const Json::Value *mCurrentLocale;

    bool mIsDevelopMode;
    void *mJNI;
    bool mShouldClear;

    // Doppler Gate Placement Related
    AutoDopplerTarget mTarget;
    void* mAutoDopplerCallbackUser;
    AutoDopplerCallback mAutoDopplerCallback;
    std::vector<int> mTargetObjIndex;
    DopplerGateMinMax mDopMinMax;
    std::vector<DopplerGateBox> mDopplerGateBuffer;
    vec2 mDopGateCoord;
    bool mMockAutoDoppler;

    // verification
    ThorStatus runAutoLabelVerification(const char* outdir, const char* thor, JNIEnv *jenv);
    ThorStatus runPWVerification(const char* outdir, const char* thor, JNIEnv *jenv);
    ThorStatus runTDIVerification(const char* outdir, const char* thor, JNIEnv *jenv);
    ThorStatus verifyFramePWMode(uint32_t frameNum, PWVerificationResult &result, JNIEnv *jenv);
    ThorStatus verifyFrameTDIMode(uint32_t frameNum, TDIVerificationResult &result, JNIEnv *jenv);
    void clearVerificationDataStructures(JNIEnv *jenv);
};
