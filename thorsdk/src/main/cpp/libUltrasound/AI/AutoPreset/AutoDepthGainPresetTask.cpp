#define LOG_TAG "AutoDepthGainPresetTask"

#include <algorithm>
#include <numeric>
#include <cmath>
#include "AutoPreset/AutoDepthGainPresetTask.h"
#include "AutoPreset/AnatomyClassifierConfig.h"
#include <AIManager.h>
#include <BitfieldMacros.h>
#include <MLOperations.h>
#include <imgui.h>
#include <CineViewer.h>
#include <fstream>
#include <sys/stat.h>
//static const int TEMPORAL_SMOOTHING_FRAMES = 30;
static const int TEMPORAL_SMOOTHING_FRAMES = 30;
static const int TEMPORAL_SMOOTHING_FRAMES_MULTI = 16;
static const int FRAME_DIVIDER = 4;
static const int PROCESS_FRAME_INTERVAL_PRESET = 30;
static const int PROCESS_FRAME_INTERVAL_DEPTH = 60;
static const int PROCESS_FRAME_INTERVAL_GAIN = 10;
static const int FREQ_BUFFER_SIZE = 16;
static const int GAIN_ADJ_VALUE = 5;
static const int FRAME_BUFFER_MIN_SIZE = 5;
// changed from .45 to .55 for new single frame model
#define CONF_THRESH 0.55
// changed from .1 to .15 for new single frame model
#define TOP2_DIFF_THRESH 0.15
using lock = std::unique_lock<std::mutex>;
//#define SAVE_FRAME_DATA
//#define RUN_VERIFICATION_AUTOPRESET
// Model parameters
// Auto Preset model is 224x224
#define MODEL_WIDTH 224
#define MODEL_HEIGHT 224
#define OUTPUT_LAYERS {"Conv_41", "Gemm_68", "Gemm_95", "Gemm_122", "Gemm_149"}
#define OUTPUT_NODES {"yolo", "view", "gain", "depth", "anatomy"}

// output size: yolo - 1*14*14*75
//            : view - 1*20 (from 15)
//            : gain - 1*3
//            : depth - 1*3
//            : anatomy 1*7

AutoDepthGainPresetTask::AutoDepthGainPresetTask(UltrasoundManager* usManager, AIManager &ai, AutoControlManager &acManager) :
        mAIManager(ai),
        mCineBuffer(*ai.getCineBuffer()),
        mAutoControlManager(acManager),
        mFlipX(-1.0f),
        mImguiRenderer(ai.getImguiRenderer()),
        mUSManager(usManager),
        mJniInit(false),
        mVerificationActive(false)
{
    mFrameWasSkipped.resize(CINE_FRAME_COUNT);
    mTotalFrameProcessingTime = 0.0;
    mFramesProcessed = 0;
    mAvgModelProcessingTime = 0.0;
    mModelFramesProcessed = 0;
    mTotalModelProcessingTime = 0.0;
    mMultiFrameHighTime = 0.0;
    mMultiFrameLowTime = 10000.0;
    mSingleFrameLowTime = 10000.0;
    mSingleFrameHighTime = 0.0;
}

AutoDepthGainPresetTask::~AutoDepthGainPresetTask()
{

    stop();
}

ThorStatus AutoDepthGainPresetTask::init(void * javaEnv, void* javaContext) {
    LOGD("[ AUTO PRESET ] Init JNI");
    mJNI = new AutoPresetJNI((JNIEnv*)javaEnv, (jobject)javaContext);
    mJNI->init((JNIEnv*)javaEnv);
    // init scan converter
    mScanConverter = std::unique_ptr<ScanConverterCL>(new ScanConverterCL);

    // since we're not running auto depth for now, do not prepare depth table
    // prepareOptimalDepth();
    mViewCineBuffer.resize(CINE_FRAME_COUNT);
    mHistoricalPreds.resize(CINE_FRAME_COUNT);
    mHistoricalSoftmaxedPreds.resize(CINE_FRAME_COUNT);
    mHistoricalPostProcessedPreds.resize(CINE_FRAME_COUNT);
    mHistoricalMainClassIds.resize(CINE_FRAME_COUNT);
    mHistoricalInferenceFrames.clear();
    mHistoricalFrameData.clear();
    mHistoricalDecisionResults.clear();
    mHistoricalFrequencyBuffer.clear();
    resetSmoothingBuffer();

    return THOR_OK;
}

void AutoDepthGainPresetTask::start()
{
    // Stop any existing worker thread
    stop();
    LOGD("[ AUTO PRESET ] start");
    mCineBuffer.registerCallback(this);
    setWorkerEnabled(true);
}

void AutoDepthGainPresetTask::stop()
{
    LOGD("[ AUTO PRESET ] stop()");
    mFrequency.clear();

    for (int i = 0; i < FREQ_BUFFER_SIZE; i++) {
        enqueueFreq(AnatomyClass::NOISE);
    }

    resetSmoothingBuffer();
    {
        setWorkerEnabled(false);
    }
    LOGD("[ ADGP FRAMETIME ] High: %.3fms Low: %.3fms Avg: %.3fms", mSingleFrameHighTime, mSingleFrameLowTime, mAvgProcessingTime);
}
void AutoDepthGainPresetTask::verificationInit(void* jenv)
{
    stop();
    LOGD("[ AUTO PRESET ] verification init");
    mVerificationActive = true;
    // we should re-init JNI for the verification pipeline here
    if(mJNI->jenv != nullptr) {
        LOGD("[ AUTO PRESET ] re-initializing JNI");
        mJNI->detachCurrentThread();
        mJNI->init((JNIEnv *) jenv);
        mJniInit = mJNI->attachToThread();
    }
    else if(mJniInit != true){
        mJNI->init((JNIEnv *) jenv);
        mJniInit = mJNI->attachToThread();
    }


}
ThorStatus AutoDepthGainPresetTask::workerInit()
{
    LOGD("[ AUTO PRESET ] Worker init");
    //AutoPresetJNI *jni = (AutoPresetJNI*)jniInterface;
    if(mVerificationActive) // we're verifying, attaching to this thread is problematic
        return THOR_OK;
    resetAutoDepthPresetTask();
    std::ostringstream oss;
    oss << "Current: " << std::this_thread::get_id() << "Cached: " << mWorkerThreadId << std::endl;
    mJniInit = mJNI->attachToThread();
    mWorkerThreadId = std::this_thread::get_id();
    if (!mJniInit) {
        LOGE("Failed to init JNI, returning error from workerInit");
        return TER_MISC_ABORT;
    }

#ifdef debug_logs
    else
    {
        LOGD("[ AUTO PRESET ] Already attached");
    }
#endif
    // fill freq buffer
    mFrequency.clear();

    for (int i = 0; i < FREQ_BUFFER_SIZE; i++) {
        enqueueFreq(AnatomyClass::NOISE);
    }

    resetSmoothingBuffer();
    if(mAutoControlManager.getAutoPresetEnabled())
    {
        start();
    }
    return THOR_OK;
}

std::pair<int, float> AutoDepthGainPresetTask::getView(uint32_t frameNum)
{
    return mViewCineBuffer[frameNum % CINE_FRAME_COUNT];
}

void AutoDepthGainPresetTask::setParams(CineBuffer::CineParams& params)
{
    LOGD("setParams, dauDataTypes = 0x%08x", params.dauDataTypes);
    // Removed quick exit in Color/M mode, due to timing issues we may sometimes run
    // for a frame or two on these modes.

    if (BF_GET(params.dauDataTypes, DAU_DATA_TYPE_B, 1) != 0)
    {
        mParams = params;
        mHasParams = true;
        mScanConverter->setConversionParameters(
                MODEL_WIDTH,
                MODEL_HEIGHT,
                *mFlipX.lock(),
                1.0f,
                ScanConverterCL::ScaleMode::FIT_BOTH,
                params.converterParams);
    }

}


static void TweakFloat(const char *label, float &f)
{
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%s.L", label);
    if (ImGui::ArrowButton(buffer, ImGuiDir_Left))
        f -= 0.01f;
    ImGui::SameLine();
    snprintf(buffer, sizeof(buffer), "%s.R", label);
    if (ImGui::ArrowButton(buffer, ImGuiDir_Right))
        f += 0.01f;
    ImGui::SameLine();
    ImGui::SliderFloat(label, &f, 0.0f, 1.0f, "%0.2f");
}

ThorStatus AutoDepthGainPresetTask::workerProcess(uint32_t frameNum, uint32_t dauDataTypes) {
// Process the frame (without holding the lock)
    if (BF_GET(dauDataTypes, DAU_DATA_TYPE_B, 1) == 0) {
        // Do not process unless BMode is set
        return THOR_OK;
    }
    if(!mJniInit){
        // I think this should not happen, since we should always set up JNI in workerInit??
        LOGW("mJniInit is false, reattaching to worker thread in workerProcess");
        mJNI->detachCurrentThread();
        mJniInit = mJNI->attachToThread();
    }
    if(std::this_thread::get_id() != mWorkerThreadId) {
        // I think this should not happen, since we should always set up JNI in workerInit??
        // NOTE: This was happening, through the following:
        //  RealtimeModule::workerFunc calls workerInit()
        //    workerInit attached to current thread, sets mJniInit to true and mWorkerThreadId to X
        //      workerInit calls start()
        //      start calls stop()
        //        stop sets mWorkerThreadId to <empty>
        //    workerInit returns (now mWorkerThreadId is <empty>)
        //  RealtimeModule::workerFunc calls workerProcess
        //    workerProcess notices mWorkerThreadId does not match, re-initializes
        //      this initialization takes a while
        //        In the meantime, imaging stops and restarts in Color + M mode
        //        setParams is called with NO B-Mode data, scan converter should not be used
        //      initialization completes, workerProcess continues
        //      we are now past the check where we ensure the frame carries B-Mode data, but our scan converter is in an invalid state
        //      crash on attempting to process frame
        LOGW("thread id does not match is false, reattaching to worker thread in workerProcess");
        mJNI->detachCurrentThread();
        mJniInit = mJNI->attachToThread();
        mWorkerThreadId = std::this_thread::get_id();
    }
    auto status = THOR_OK;
    if (frameNum % FRAME_DIVIDER == 0) {
        status = processFrame(frameNum, mJNI);
    }
    ++mFramesProcessed;
    if (IS_THOR_ERROR(status))
    {
        LOGE("Error processing frame %u: %08x", frameNum, status);
    }
    return status;
}

void AutoDepthGainPresetTask::workerFramesSkipped(uint32_t first, uint32_t last) {
    for (uint32_t frame = first; frame != last; ++frame) {
        mFrameWasSkipped[frame % CINE_FRAME_COUNT] = 1;
    }
}

ThorStatus AutoDepthGainPresetTask::workerTerminate(){
    LOGD("[ AUTO PRESET ] workerTerminate Call");
    if(mVerificationActive)
        return THOR_OK;
    stop();
    if(mJniInit)
    {
        mWorkerThreadId = std::thread::id();
        mJNI->detachCurrentThread();
        mJniInit = mJNI->jniInit;
    }
#ifdef debug_logs
    else
    {
        LOGD("[ AUTO PRESET ] already detached");
    }
#endif
    return THOR_OK;
}

ThorStatus AutoDepthGainPresetTask::scanConvert(CineBuffer &cinebuffer, uint32_t frameNum, std::vector<float> &postscan)
{
    // Scan convert image
    uint8_t *prescan = mCineBuffer.getFrame(frameNum, DAU_DATA_TYPE_B);
    postscan.resize(MODEL_WIDTH*MODEL_HEIGHT);
    mScanConverter->runCLTex(prescan, postscan.data());

    return THOR_OK;
}
ThorStatus AutoDepthGainPresetTask::runModel(std::vector<uint8_t> &postscan, AutoDepthGainPresetPred &pred)
{
    ThorStatus status = THOR_OK;

    // TODO:: Process from JNI for single frame if necessary

    if (IS_THOR_ERROR(status)) {return status; }
    return THOR_OK;
}

ThorStatus AutoDepthGainPresetTask::runModelMultiFrame(AutoDepthGainPresetPred &pred)
{
    ThorStatus status = THOR_OK;
    std::vector<const float*> orderedInput;
    for(int counter = 0; counter < mAutoPresetBuffer.size(); ++counter)
    {
        int index = mAutoPresetBuffer.currentIndex() + counter;
        if(index >= mAutoPresetBuffer.size())
            index -= mAutoPresetBuffer.size();
#ifdef debug_logs
        LOGD("[ AUTO PRESET DEBUG ] Auto Preset Buffer Index: %d", index);
#endif
        orderedInput.push_back(mAutoPresetBuffer.elementAt(index).data());
    }
#ifdef SAVE_FRAME_DATA
    std::vector<float> frameData;
    for(int i = 0; i < orderedInput.size(); ++i)
    {

        const float* currFrame = orderedInput[i];
        for(int j = 0; j < MODEL_WIDTH * MODEL_HEIGHT; ++j)
        {
            frameData.push_back(currFrame[j]);
        }

    }
    mHistoricalFrameData.push_back(frameData);
#endif
    const int imageSize = MODEL_HEIGHT * MODEL_WIDTH;
    for(int i = 0; i < 5; ++i)
    {
        memcpy(mJNI->imageBufferNative.data() + (i * imageSize) , orderedInput[i], imageSize * sizeof(float));
    }
    mJNI->jenv->CallIntMethod(mJNI->instance, mJNI->process, mJNI->imageBuffer, mJNI->viewBuffer);
    for(int i = 0; i < 5; ++i)
    {
        pred.anatomy[i] = mJNI->viewBufferNative[i];

    }
#ifdef debug_logs
    for(int i = 0 ; i < ANATOMY_LABEL_COUNT; ++i)
    {
        LOGD("[ AUTO PRESET DEBUG ] RAW %s: %.02f", ANATOMY_CLASSES[i], pred.anatomy[i]);
    }
#endif
    if (IS_THOR_ERROR(status)) {return status; }
    return THOR_OK;
}

void AutoDepthGainPresetTask::postProcessPred(AutoDepthGainPresetPred &pred)
{
    // Softmax all outputs
    softmax(std::begin(pred.anatomy), std::end(pred.anatomy));
#ifdef debug_logs
    for(int i = 0 ; i < ANATOMY_LABEL_COUNT; ++i)
    {
        LOGD("[ AUTO PRESET DEBUG ] SOFTMAX %s: %.02f", ANATOMY_CLASSES[i], pred.anatomy[i]);
    }
#endif
}

void AutoDepthGainPresetTask::enqueuePred(const AutoDepthGainPresetPred &pred)
{
    if (mPredictions.size() == TEMPORAL_SMOOTHING_FRAMES_MULTI)
        mPredictions.pop_front();
    mPredictions.push_back(pred);
}

void AutoDepthGainPresetTask::enqueueFreq(const int class_id)
{
    if (mFrequency.size() == FREQ_BUFFER_SIZE)
        mFrequency.pop_front();
    mFrequency.push_back(class_id);
}

float AutoDepthGainPresetTask::getFrequency(const int class_id)
{
    int cnt = std::count (mFrequency.begin(), mFrequency.end(), class_id);

    return ((float) cnt) / ((float) FREQ_BUFFER_SIZE);
}

AutoDepthGainPresetPred AutoDepthGainPresetTask::computeMean()
{
    AutoDepthGainPresetPred mean;
    memset(&mean, 0, sizeof(mean));
    float N = static_cast<float>(mPredictions.size());

    for (auto &pred : mPredictions)
    {
        std::transform(std::begin(mean.anatomy), std::end(mean.anatomy), std::begin(pred.anatomy), std::begin(mean.anatomy), [=](float a, float b){return a + b/N;});
    }
    return mean;
}

static float sigmoid(float f)
{
    return 1/(1+exp(-f));
}

ThorStatus AutoDepthGainPresetTask::processFrame(uint32_t frameNum, void* jni)
{
    ThorStatus status;
    AutoPresetJNI* autoPreset = (AutoPresetJNI*)jni;
    if(!mJniInit){

        mFrameWasSkipped[frameNum % CINE_FRAME_COUNT] = 1;
        return THOR_OK;
    }
    mFrameWasSkipped[frameNum % CINE_FRAME_COUNT] = 0;
    // Clear prediction buffer if imaging has restarted
    if (frameNum == 0) {
        resetSmoothingBuffer();
        mHistoricalInferenceFrames.clear();
        mHistoricalFrameData.clear();
        mHistoricalDecisionResults.clear();
        mHistoricalFrequencyBuffer.clear();
    }
    // Run scan conversion
    std::vector<float> postscan;

    status = scanConvert(mCineBuffer, frameNum, postscan);
    mAutoPresetBuffer.insert(postscan);
    if (IS_THOR_ERROR(status)) { return status; }

    AutoDepthGainPresetPred pred;
    bool runBuffer = mAutoPresetBuffer.isFull();
    if(runBuffer)
    {
        status = processMultiFrame(frameNum, pred);
    }

    return THOR_OK;
}
ThorStatus AutoDepthGainPresetTask::processSingleFrame(uint32_t frameNum, AutoDepthGainPresetPred &pred)
{
#ifdef debug_logs
    LOGD("[ AUTO PRESET MULTI ] RUNNING MODEL FRAME %u", frameNum);
#endif
    mHistoricalInferenceFrames.push_back(frameNum);
    // run the model
    auto status = THOR_OK;

    // enqueue historical data
    mHistoricalPreds[frameNum % CINE_FRAME_COUNT] = pred;
    if (IS_THOR_ERROR(status)) { return status; }

    // Post process (softmax) single prediction (no error conditions)
    postProcessPred(pred);
    // set confidence if mocked
    if(mAutoControlManager.getAutoPresetMockEnabled())
    {
        for(int i = 0; i < ANATOMY_LABEL_COUNT; ++i)
        {
            pred.anatomy[i] = 0.f;
            if(i == mAutoControlManager.getAutoPresetMockClass())
            {
                pred.anatomy[i] = mAutoControlManager.getAutoPresetMockConfidence();
            }
        }
    }
    // Enqueue (and popping) in temporal buffer (for the que to maintain the latest 30 frames)
    enqueuePred(pred);
    // Average temporal buffer
    AutoDepthGainPresetPred meanPred = computeMean();
#ifdef RUN_VERIFICATION_AUTOPRESET
    std::deque<AutoDepthGainPresetPred> temp;
        temp.resize(mPredictions.size());
        ///std::copy(postscan, postscan+TDI_ANNOTATOR_INPUT_WIDTH*TDI_ANNOTATOR_INPUT_HEIGHT, std::back_inserter(result.postscan));
        std::copy(mPredictions.begin(), mPredictions.end(), temp.begin());
        mHistoricalSoftmaxedPreds[frameNum % CINE_FRAME_COUNT] = pred;
        mHistoricalSmoothedPredictions.push_back(temp);
#endif
#ifdef debug_logs
    for(int i = 0 ; i < ANATOMY_LABEL_COUNT; ++i)
        {
            LOGD("[ AUTO PRESET DEBUG ] POST %s: %.02f", ANATOMY_CLASSES[i], meanPred.anatomy[i]);
        }
#endif
    mHistoricalPostProcessedPreds[frameNum % CINE_FRAME_COUNT] = meanPred;
    {
        {
            // believe these are reasonable defaults
            int mainClass = AnatomyClass::NOISE;
            bool changePreset = false;

            // process Auto Preset
            mainClass = predictMainClass(meanPred);
            if(mAutoControlManager.getAutoPresetEnabled())
            {
                changePreset = processAutoPreset(meanPred, mainClass);
            }
            mHistoricalMainClassIds[frameNum % CINE_FRAME_COUNT] = mainClass;
        }

    }
    return status;
}
ThorStatus AutoDepthGainPresetTask::processMultiFrame(uint32_t frameNum, AutoDepthGainPresetPred &pred)
{
#ifdef debug_logs
    LOGD("[ AUTO PRESET MULTI ] RUNNING MODEL FRAME %u", frameNum);
#endif
    mHistoricalInferenceFrames.push_back(frameNum);
    // run the model
    auto status = runModelMultiFrame(pred);

    // enqueue historical data
    mHistoricalPreds[frameNum % CINE_FRAME_COUNT] = pred;
    if (IS_THOR_ERROR(status)) { return status; }

    // Post process (softmax) single prediction (no error conditions)
    postProcessPred(pred);
    // set confidence if mocked
    if(mAutoControlManager.getAutoPresetMockEnabled())
    {
        for(int i = 0; i < ANATOMY_LABEL_COUNT; ++i)
        {
            pred.anatomy[i] = 0.f;
            if(i == mAutoControlManager.getAutoPresetMockClass())
            {
                pred.anatomy[i] = mAutoControlManager.getAutoPresetMockConfidence();
            }
        }
    }
    // Enqueue (and popping) in temporal buffer (for the que to maintain the latest 30 frames)
    enqueuePred(pred);
    // Average temporal buffer
    AutoDepthGainPresetPred meanPred = computeMean();
#ifdef RUN_VERIFICATION_AUTOPRESET
    std::deque<AutoDepthGainPresetPred> temp;
        temp.resize(mPredictions.size());
        ///std::copy(postscan, postscan+TDI_ANNOTATOR_INPUT_WIDTH*TDI_ANNOTATOR_INPUT_HEIGHT, std::back_inserter(result.postscan));
        std::copy(mPredictions.begin(), mPredictions.end(), temp.begin());
        mHistoricalSoftmaxedPreds[frameNum % CINE_FRAME_COUNT] = pred;
        mHistoricalSmoothedPredictions.push_back(temp);
#endif
#ifdef debug_logs
    for(int i = 0 ; i < ANATOMY_LABEL_COUNT; ++i)
        {
            LOGD("[ AUTO PRESET DEBUG ] POST %s: %.02f", ANATOMY_CLASSES[i], meanPred.anatomy[i]);
        }
#endif
    mHistoricalPostProcessedPreds[frameNum % CINE_FRAME_COUNT] = meanPred;
    {
        {
            // believe these are reasonable defaults
            int mainClass = AnatomyClass::NOISE;
            bool changePreset = false;

            // process Auto Preset
            mainClass = predictMainClass(meanPred);
            if(mAutoControlManager.getAutoPresetEnabled())
            {
                changePreset = processAutoPreset(meanPred, mainClass);
            }
            mHistoricalMainClassIds[frameNum % CINE_FRAME_COUNT] = mainClass;
        }

    }
    return status;
}
static void callChangePreset(int deviceClass, AutoControlManager &mAutoControlManager, int mainClass, int organId) {
    uint32_t newOrganId = organId;

#ifdef debug_logs
    LOGD("[ AUTO PRESET SWITCH ] Switching from %s to %s", ANATOMY_CLASSES[deviceClass], ANATOMY_CLASSES[mainClass]);
#endif
    switch(mainClass)
    {
        case AnatomyClass::CARDIAC:
            newOrganId = OrganId::HEART;
            break;
        case AnatomyClass::LUNG:
            newOrganId = OrganId::LUNGS;
            break;
        case AnatomyClass::ABDOMINAL:
            newOrganId = OrganId::ABDOMEN;
            break;
        case AnatomyClass::IVC:
            // if the anatomy class is IVC we switch to cardiac from the lung preset
            newOrganId = OrganId::HEART;
#ifdef debug_logs
            if(deviceClass != AnatomyClass::LUNG)
            {
                LOGD("[ AUTO PRESET ] Error: Switching on IVC but not in Lung preset.");
            }
#endif
            break;
    }

    mAutoControlManager.setAutoOrgan(newOrganId);
}

bool AutoDepthGainPresetTask::processAutoPreset(AutoDepthGainPresetPred meanPred, int mainClass)
{
    // freq enqueue
    enqueueFreq(mainClass);
    float mainClassFreq = getFrequency(mainClass);
    //LOGD("[ AUTO PRESET ] Main Class: %s, Conf: %.02f", ANATOMY_CLASSES[mainClass], mainClassFreq);
#ifdef RUN_VERIFICATION_AUTOPRESET
    std::deque<int> temp;
    temp.resize(mFrequency.size());
    std::copy(mFrequency.begin(), mFrequency.end(), temp.begin());
    mHistoricalFrequencyBuffer.push_back(temp);
    if(mainClassFreq > FREQ_THRESHOLD)
    {
        mHistoricalDecisionResults.push_back(mainClass);
    }
    else {
        mHistoricalDecisionResults.push_back(AnatomyClass::NOISE);
    }
#endif
    int deviceClass = AnatomyClass::CARDIAC;
    int organId = mCineBuffer.getParams().organId;

    if (organId == OrganId::LUNGS)
        deviceClass = AnatomyClass::LUNG;
    else if (organId == OrganId::ABDOMEN)
        deviceClass = AnatomyClass::ABDOMINAL;
    bool changePreset = false;
    /*static int counter = 0;
    if(++counter >= 30)
    {
        LOGD("[ AUTO PRESET ] Detected Anatomies: %s: %.02f, %s: %.02f, %s: %.02f, %s: %.02f, %s: %.02f",
             ANATOMY_CLASSES[0], meanPred.anatomy[0],
             ANATOMY_CLASSES[1], meanPred.anatomy[1],
             ANATOMY_CLASSES[2], meanPred.anatomy[2],
             ANATOMY_CLASSES[3], meanPred.anatomy[3],
             ANATOMY_CLASSES[4], meanPred.anatomy[4]);
    } */
    if (mainClass != deviceClass)
    {
        if (mainClassFreq > FREQ_THRESHOLD)
        {
            // if the main class is IVC and the device preset is lung we will switch to cardiac
            if (( mainClass ==  AnatomyClass::IVC && deviceClass == AnatomyClass::LUNG) || ( mainClass == AnatomyClass::CARDIAC || mainClass == AnatomyClass::ABDOMINAL || mainClass == AnatomyClass::LUNG)) {
                changePreset = true;
                mJniInit = false;
                mJNI->detachCurrentThread();
            }
        }
#ifdef debug_logs
        LOGD("[ AUTO ] mainClass: %s deviceClass: %s", ANATOMY_CLASSES[mainClass], ANATOMY_CLASSES[deviceClass]);
#endif
    }

    // set AutoOrgan to the mainClass -> newOrganId
    if (changePreset)
    {
        //LOGD("[ AUTO PRESET ] Changing preset from %d to %d", mainClass, organId);
        callChangePreset(deviceClass, mAutoControlManager, mainClass, organId);
    }
    /*
    // TODO: REMOVE - for debugging
    if (changePreset)
    {
        mImguiRenderer.setPresetChange(true, mainClass);
    }
    else
        mImguiRenderer.setPresetChange(false, mainClass);
    */

    return changePreset;
}

int AutoDepthGainPresetTask::predictMainClass(AutoDepthGainPresetPred pred)
{
    int mainClass = AnatomyClass::NOISE;
    int maxClassId;
    float maxClassPred;

    // Temporal Buffer Averaging done in processFrame
    std::vector<float> predVec = {pred.anatomy, pred.anatomy + ANATOMY_LABEL_COUNT};

    // TODO: FOR DEBUGGING
    // mImguiRenderer.setAnatomyClassification(predVec);

    maxClassId = argmax(predVec.begin(), predVec.end());
    maxClassPred = predVec.at(maxClassId);

    if (maxClassPred > CONF_THRESH_MULTIFRAME)
    {
        // sort to get 2nd biggest conf
        std::sort(predVec.begin(), predVec.end(), std::greater<float>());

        if ( (maxClassPred - predVec.at(1)) > TOP2_DIFF_THRESH_MULTIFRAME )
        {
            // update mainClass ID
            mainClass = maxClassId;
            // no lung confidence threshold with new single frame model
        }
    }

    return mainClass;
}

void AutoDepthGainPresetTask::setFlip(float flipX, float flipY)
{
    auto l = lock(mMutex);
    *mFlipX.lock() = flipX;

    if (mHasParams) {
        mScanConverter->setFlip(flipX, flipY);
    }
}

void AutoDepthGainPresetTask::onSave(uint32_t frameStart, uint32_t frameEnd, echonous::capnp::ThorClip::Builder &builder)
{
   ///TODO:: port save of module data via capnp
    /*auto preset = builder.initAutopreset();
    preset.setModelVersion("AUTOPRESET_STANDALONE");
    uint32_t inferenceCount = 0;
    for (uint32_t i=0; i < mHistoricalInferenceFrames.size(); ++i) {
        if (mFrameWasSkipped[mHistoricalInferenceFrames[i] % CINE_FRAME_COUNT] == 0)
            ++inferenceCount;
    }
    auto predictions = preset.initFrames(mHistoricalInferenceFrames.size());
    for(uint32_t i = 0u; i < mHistoricalInferenceFrames.size(); ++i)
    {
        uint32_t currentFrame = mHistoricalInferenceFrames[i];
        if (mFrameWasSkipped[currentFrame % CINE_FRAME_COUNT] == 1)
            continue;

        predictions[i].setFrameNum(currentFrame);
        auto postProcessedConfs = predictions[i].initViewPostProcessedConf(ANATOMY_LABEL_COUNT);
        auto softmaxedConfs = predictions[i].initViewSoftmaxedConf(ANATOMY_LABEL_COUNT);
        auto rawConfs = predictions[i].initViewRawConf(ANATOMY_LABEL_COUNT);
        for(uint32_t index = 0; index < ANATOMY_LABEL_COUNT; ++index)
        {
            // copy outputs to list
            auto rawConf = rawConfs[index];
            auto softMaxedConf = softmaxedConfs[index];
            auto postConf = postProcessedConfs[index];
            rawConf = mHistoricalPreds[currentFrame % CINE_FRAME_COUNT].anatomy[index];
            softMaxedConf = mHistoricalSoftmaxedPreds[currentFrame % CINE_FRAME_COUNT].anatomy[index];
            postConf = mHistoricalPostProcessedPreds[currentFrame % CINE_FRAME_COUNT].anatomy[index];
        }
        int presetIndex = mHistoricalMainClassIds[currentFrame % CINE_FRAME_COUNT];
        predictions[i].setViewName(PRESET_CLASSES[presetIndex]);
        predictions[i].setViewIndex(presetIndex);
    }
    LOGD("[ AUTO ] Save complete, %u inferences saved, Begin: %u, End %u", inferenceCount,frameStart, frameEnd);*/
}

void AutoDepthGainPresetTask::onLoad(echonous::capnp::ThorClip::Reader &reader)
{
   /* if (reader.hasAutopreset()) {
        auto preset = reader.getAutopreset();
        const auto preds = preset.getFrames();
        for(uint32_t i = 0; i != preds.size(); ++i)
        {
            auto rawPred = preds[i].getViewRawConf();
            auto softPred = preds[i].getViewSoftmaxedConf();
            auto postPred = preds[i].getViewPostProcessedConf();
            for(uint32_t j = 0; j < ANATOMY_LABEL_COUNT; ++j)
            {
                mHistoricalPreds[preds[i].getFrameNum() % CINE_FRAME_COUNT].anatomy[j] = rawPred[j];
                mHistoricalSoftmaxedPreds[preds[i].getFrameNum() % CINE_FRAME_COUNT].anatomy[j] = softPred[j];
                mHistoricalPostProcessedPreds[preds[i].getFrameNum() % CINE_FRAME_COUNT].anatomy[j] = postPred[j];
            }
            mHistoricalMainClassIds[preds[i].getFrameNum() % CINE_FRAME_COUNT] = preds[i].getViewIndex();
        }
        LOGD("[ AUTO ] Load complete, %u frames loaded", preds.size());
    }*/
}

// Verification Functions
template <class T> void SaveRaw(const std::vector<T> &data, const char *outdir, const char *id_begin, const char *id_end, const char *fmt)
{
    char outpath[512];
    snprintf(outpath, sizeof(outpath), fmt,
             outdir,
             static_cast<int>(id_end - id_begin), id_begin);
    std::ofstream os(outpath);
    os.write((const char*)data.data(), data.size()*sizeof(T));
}

void AutoDepthGainPresetTask::resetAutoDepthPresetTask()
{
    mFrequency.clear();
    mHistoricalPreds.clear();
    mHistoricalSoftmaxedPreds.clear();
    mHistoricalPostProcessedPreds.clear();
    mHistoricalInferenceFrames.clear();
    mHistoricalFrameData.clear();
    mHistoricalSmoothedPredictions.clear();
    mHistoricalFrequencyBuffer.clear();
    mHistoricalDecisionResults.clear();
    mHistoricalPreds.resize(CINE_FRAME_COUNT);
    mHistoricalSoftmaxedPreds.resize(CINE_FRAME_COUNT);
    mHistoricalPostProcessedPreds.resize(CINE_FRAME_COUNT);
    mHistoricalMainClassIds.resize(CINE_FRAME_COUNT);
    mAutoPresetBuffer.clear();
    resetSmoothingBuffer();
    // reset decision buffer
    for (int i = 0; i < FREQ_BUFFER_SIZE; i++) {
        enqueueFreq(AnatomyClass::NOISE);
    }
}

ThorStatus AutoDepthGainPresetTask::runVerificationClip(const char *inputPath, const char *outputPath, void* jni)
{
    LOGD("[ VERIFY ] Auto Preset testing %s", inputPath);
    bool jniCurrent = mJniInit;
    resetAutoDepthPresetTask();
    //bool currentState = mAutoControlManager.getAutoPresetEnabled();
    //mAutoControlManager.setAutoPresetEnabled(true);
    // we've opened the file - get max and min frames?
    // what happens if I just give this some time to attach?
    int waitCounter = 0;
    while(mJNI->jenv == nullptr && waitCounter < 10000)
    {
        ++waitCounter;
        if(waitCounter%10 == 0)
        {
            LOGD("[ AUTO PRESET ] waiting for Godot");
        }

    }
    if(mJNI->jenv != nullptr)
    {
        LOGD("[ AUTO PRESET ] Finally, godot");
        mJniInit = true;
    }
    else
    {
        LOGD("[ AUTO PRESET ] Timeout");
    }
    uint32_t frameMax = mCineBuffer.getMaxFrameNum();
    for(uint32_t i = 0; i <= frameMax; ++i)
    {
        if(i % FRAME_DIVIDER == 0 && i > 1) {
            processFrame(i, jni);
        }
    }
    // once entire file is processed, output to json file
    const char *id_begin = strrchr(inputPath, '/')+1;
    const char *id_end = strrchr(inputPath, '.');
    char outpath[512];
    snprintf(outpath, sizeof(outpath), "%s/%.*s.json",
             outputPath,
             static_cast<int>(id_end - id_begin), id_begin);
    std::ofstream jsonOut(outpath);
    Json::Value adgpVerify;
    LOGD("[ AUTO PRESET ] Verification - preds count: %d, %d, %d, %d", mHistoricalPreds.size(),mHistoricalSoftmaxedPreds.size(), mHistoricalPostProcessedPreds.size(), mHistoricalInferenceFrames.size() );
    adgpVerify["clip"] = id_begin;
    adgpVerify["clip_data"] = ToJson(mHistoricalPreds, mHistoricalSoftmaxedPreds, mHistoricalPostProcessedPreds, mHistoricalInferenceFrames, mHistoricalFrameData);
    adgpVerify["clip_data"]["frame_count"] = frameMax+1;
    //adgpVerify["softmax_outputs"] = ToJson(mSoftmaxedOutputs);
    //adgpVerify["postprocessed_outputs"] = ToJson(mPostProcessedOutputs);
    // we're going to add frame by frame
    // Save results

    jsonOut << adgpVerify << "\n";
    LOGD("[ VERIFY ] Auto Preset verification on %s complete, saving to %s", inputPath, outputPath);
    return THOR_OK;
}
ThorStatus AutoDepthGainPresetTask::runVerification(const char* outputPath)
{
    bool currentState = mAutoControlManager.getAutoPresetEnabled();
    mAutoControlManager.setAutoPresetEnabled(true);

    std::string appPath = std::string(outputPath);//mUSManager->getAppPath();
    std::string outPath = appPath + "/AutoPreset";
    int count = 0;
    ::mkdir(outPath.c_str(), S_IRWXU | S_IRGRP | S_IROTH);

    /// TODO:: rewrite file walk for verification
    /*8@autoreleasepool {
            NSStringEncoding encoding = [NSString defaultCStringEncoding];
            NSFileManager* defMan = [NSFileManager defaultManager];
            NSString* ns_appPath = [defMan URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask].lastObject.path;
            NSString* verificationPath = [ns_appPath stringByAppendingString:@"/AI/verification/AutoPreset/"];
            __autoreleasing NSArray<NSString*> *rawFiles = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:verificationPath error:nil];

            uint64_t fileCount = rawFiles.count;
            uint64_t currentFile = 1;
            for (NSString *raw : rawFiles) {
                NSLog(@"[ VERIFY ] AutoPresetVerifiy: Found resource file %@, %llu of %llu", raw, currentFile++, fileCount);
                NSString* inPath = [verificationPath stringByAppendingString:raw];
                const char *inputPath = [inPath cStringUsingEncoding:encoding];
                ThorStatus status = runVerificationClip(inputPath, outPath.c_str());
                if (IS_THOR_ERROR(status)) {
                    NSLog(@"Error processing clip %@", raw);
                    return status;
                }
                ++count;
            }
    }*/
    LOGD("[ AUTO PRESET ] Verification Over - %d files Verified", count);
    mAutoControlManager.setAutoPresetEnabled(currentState);
    return THOR_OK;
}

void AutoDepthGainPresetTask::resetSmoothingBuffer()
{
    mPredictions.resize(TEMPORAL_SMOOTHING_FRAMES_MULTI);
    for(int i = 0; i < mPredictions.size(); ++i)
    {
        for(int j = 0; j < ANATOMY_LABEL_COUNT; ++j)
        {
            mPredictions[i].anatomy[j] = 0.f;
        }
    }
}

Json::Value AutoDepthGainPresetTask::ToJson(const std::vector<AutoDepthGainPresetPred>& rawPreds, const std::vector<AutoDepthGainPresetPred>& softmaxPreds, const std::vector<AutoDepthGainPresetPred>& postPreds, const std::vector<uint32_t>& inferenceFrames, const std::vector<std::vector<float>>& frameData)
{
    Json::Value json;
    for(int i = 0; i < inferenceFrames.size(); ++i)
    {
        uint32_t inferenceIndex = inferenceFrames[i];
        json["frames"].append(ToJson(rawPreds[inferenceIndex], softmaxPreds[inferenceIndex], postPreds[inferenceIndex], inferenceIndex, frameData[i], i));

    }
    return json;
}

Json::Value AutoDepthGainPresetTask::ToJson(const AutoDepthGainPresetPred& rawPred, const AutoDepthGainPresetPred& softPred, const AutoDepthGainPresetPred& postPred, uint32_t frameNumber, const std::vector<float>& frameData, int inferenceIndex)
{
    Json::Value json;
    json["frame"]["index"].append(frameNumber);
    json["frame"]["decison"].append(mHistoricalDecisionResults[inferenceIndex]);
    for(int i = 0; i < ANATOMY_LABEL_COUNT; ++i)
    {
        json["frame"]["raw_predictions"].append(rawPred.anatomy[i]);
        json["frame"]["softmaxed_predictions"].append(softPred.anatomy[i]);
        json["frame"]["postprocessed_predictions"].append(postPred.anatomy[i]);
    }
    for(int j = 0; j < mHistoricalFrequencyBuffer[inferenceIndex].size(); ++j) {
        json["frame"]["decision_buffer"].append(mHistoricalFrequencyBuffer[inferenceIndex][j]);
    }
    for(int j = 0; j < mHistoricalSmoothedPredictions[inferenceIndex].size(); ++j) {
        for(int x = 0; x < ANATOMY_LABEL_COUNT; ++x){
            json["frame"]["smoothing_buffer"][ANATOMY_CLASSES[x]].append(mHistoricalSmoothedPredictions[inferenceIndex][j].anatomy[x]);
        }
    }
#ifdef SAVE_FRAME_DATA
    for(int i = 0; i < frameData.size(); ++i)
    {
        json["frame"]["frame_data"].append(frameData[i]);
    }
#endif
    return json;
}

