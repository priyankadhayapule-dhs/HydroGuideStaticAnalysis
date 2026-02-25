#define LOG_TAG "EFQualitySubviewTask"

#include <algorithm>
#include <numeric>
#include <cmath>
#include <EFQualitySubviewTask.h>
#include <AIManager.h>
#include <BitfieldMacros.h>
#include <MLOperations.h>
#include <imgui.h>
#include <CineBuffer.h>
#include <CineViewer.h>
#include <Filesystem.h>
#include <PLAXPipeline.h>
#include <UltrasoundManager.h>
#include <Guidance/GuidanceDisplay.h>
#include <GuidanceJNI.h>
#include <fstream>

//#define THOR_MOCK_QUALITY(x)  MockQuality(x)
//#define THOR_MOCK_SUBVIEW(x)  MockSubview(x)

static const float AUTOCAPTURE_HIGH_THRESHOLD = 0.99f;
static const float AUTOCAPTURE_LOW_THRESHOLD = 0.7f;
static const int AUTOCAPTURE_TIMEOUT_EXTRA = 30;
static const int TEMPORAL_SMOOTHING_FRAMES = 30;
static const int FAST_PROBE_DETECT_FRAMES = 30; // must be smaller than autocapture num frames (uses same buffer)
static const int FAST_PROBE_MAX_SUBVIEWS  = 6;
static const int FAST_PROBE_WARNING_FRAMES = 60;
static const uint32_t AUTOCAPTURE_NUM_FRAMES = (30*3);
static const int FRAME_DIVIDER = 1;
static const int MAX_FRAME_QUALITY_FILL = 15; // up too N skipped frames will be filled in the quality buffer
constexpr float AUTOCAPTURE_NOTIFY_EVERY = 0.05f;
constexpr float SMARTCAPTURE_NOTIFY_EVERY = 0.10f;

using lock = std::unique_lock<std::mutex>;


static int MockQuality(int frameNum) {
    if (frameNum < 120)
        return 1;
    else if (frameNum < 150)
        return 4;
    else if (frameNum < 180)
        return 1;
    else
        return 5;
}

static int MockSubview(int frameNum) {
    return (frameNum/30)%NUM_SUBVIEWS;
}

// Model parameters
#define A4C_MODEL_NAME "grading-guidance-A4C_finetuned_subv_finetuned_iq_A4C.dlc"
#define A4C_OUTPUT_LAYERS {}
#define A4C_OUTPUT_NODES {"213", "233"}

#define A2C_MODEL_NAME "grading-guidance-A2C_finetuned_subv_finetuned_iq_A2C.dlc"
#define A2C_OUTPUT_LAYERS {}
#define A2C_OUTPUT_NODES {"213", "233"}
#define MODEL_WIDTH 128
#define MODEL_HEIGHT 128


EFQualitySubviewTask::EFQualitySubviewTask(AIManager &ai) :
    mAssets(nullptr),
    mCallbackUser(nullptr),
    mAIManager(ai),
    mCineBuffer(*ai.getCineBuffer()),
    mQualityEnabled(false),
    mGuidanceEnabled(false),
    mAutocaptureEnabled(false),
    mStop(false),
    mProcessedFrame(0xFFFFFFFFu),
    mLastProcessedQualityFrame(0xFFFFFFFFu),
    mCineFrame(0xFFFFFFFFu),
    mView(CardiacView::A4C),
    mMutex(),
    mCV(),
    mWorkerThread(),
    mScanConverter(nullptr),
    mSubviewIndex(11), // Noise
    mAutoCaptureBufferIndex(0),
    mAutoCaptureBufferSize(CINE_FRAME_COUNT),
    mSubviewBuffer(mAutoCaptureBufferSize),
    mQualityBuffer(mAutoCaptureBufferSize),
    mAutocaptureTimeout(30*6),
    mDisplay(nullptr)
{
}

EFQualitySubviewTask::~EFQualitySubviewTask()
{
    stop();
}

ThorStatus EFQualitySubviewTask::init(UltrasoundManager *manager) {
    ThorStatus status;
    mUSManager = manager;

    mPLAXPipeline.reset(new PLAXPipeline);

    //mA5CPipeline.reset(new A5CPipeline);
    //status = mA5CPipeline->init(manager);
    //if (IS_THOR_ERROR(status)) return status;

    mAssets = manager->getFilesystem();

    // Create scan converter (destroys old scan converter, if any)
    mScanConverter = std::unique_ptr<ScanConverterCL>(new ScanConverterCL(ScanConverterCL::OutputType::FLOAT_UNSIGNED_SCALE_255));

    mUnsmoothSubviews.resize(FAST_PROBE_DETECT_FRAMES);

    mAutocaptureEnabled = false;
    mFlipX = -1.0f;

    mDisplay.reset(new GuidanceDisplay);
    status = mDisplay->init(manager);
    if (IS_THOR_ERROR(status)) {
        LOGE("Error initializing animations");
        return status;
    }

    mPredictions.clear();
    std::fill(mSubviewBuffer.begin(), mSubviewBuffer.end(), -1); // no data
    std::fill(mQualityBuffer.begin(), mQualityBuffer.end(), -1);  // no data
    mAutocaptureTimeout = 0;

    mSmartCapturePrevScore = -1.0f; // always notify
    return THOR_OK;
}

void EFQualitySubviewTask::start(void *javaEnv, void *javaContext)
{
    // Stop any existing worker thread
    stop();
    mStop = false;
    mQualityEnabled = false;
    mGuidanceEnabled = false;

    auto l = lock(mMutex);
    mCineBuffer.registerCallback(this);
    mJNI = new GuidanceJNI((JNIEnv*)javaEnv, (jobject)javaContext);
    mWorkerThread = std::thread(&EFQualitySubviewTask::workerThreadFunc, this, (void*)(mJNI));
}

void EFQualitySubviewTask::stop()
{
    LOGD("stop()");
    {
        auto l = lock(mMutex);
        if (!mWorkerThread.joinable())
        {
            LOGD("Worker thread not joinable, stop complete.");
            return;
        }
        mStop = true;
    }
    LOGD("Worker thread joinable, awaiting done");
    mCV.notify_one();
    mWorkerThread.join();
    LOGD("Joined worker thread, stop complete.");
}

void EFQualitySubviewTask::resetQualityBuffers() {
    std::fill(mSubviewBuffer.begin(), mSubviewBuffer.end(), -1); // no data
    std::fill(mQualityBuffer.begin(), mQualityBuffer.end(), -1); // no data
    mPLAXPipeline->reset();
}

void EFQualitySubviewTask::setView(CardiacView view)
{
    const char* viewName = "unknown";
    switch (view) {
    case CardiacView::A4C: viewName = "A4C"; break;
    case CardiacView::A2C: viewName = "A2C"; break;
    case CardiacView::A5C: viewName = "A5C"; break;
    case CardiacView::PLAX: viewName = "PLAX"; break;
    }
    LOGD("SetView(%s)", viewName);

    if (mView != view) {
        mPredictions.clear();
        std::fill(mSubviewBuffer.begin(), mSubviewBuffer.end(), -1); // no data
        std::fill(mQualityBuffer.begin(), mQualityBuffer.end(), -1);  // no data
        mAutocaptureTimeout = 0;

        if (view == CardiacView::PLAX)
            mPLAXPipeline->reset();
        else if (view == CardiacView::A5C) {
            // TODO re-enable A5C pipeline
            //mA5CPipeline->reset();
        }
        mView = view;
    }
}

void EFQualitySubviewTask::setShowQuality(bool quality)
{
    {
        auto l = lock(mMutex);
        mQualityEnabled = quality;
        mDisplay->setEnableQuality(quality);
    }
    mCV.notify_one();
}

void EFQualitySubviewTask::setShowGuidance(bool guidance)
{
    {
        auto l = lock(mMutex);
        mGuidanceEnabled = guidance;
        mDisplay->setEnableGuidance(guidance);
    }
    mCV.notify_one();
}

void EFQualitySubviewTask::setAutocaptureEnabled(bool isEnabled)
{
    {
        auto l = lock(mMutex);
        mAutocaptureEnabled = isEnabled;
        mAutocaptureTimeout = AUTOCAPTURE_NUM_FRAMES + AUTOCAPTURE_TIMEOUT_EXTRA;
    }
    mCV.notify_one();
}

void EFQualitySubviewTask::setLanguage(const char *lang)
{
    auto l = lock(mMutex);
    mDisplay->setLocale(lang);
}


void EFQualitySubviewTask::loadDrawable(Filesystem *filesystem) {
    mDisplay->loadFont(*filesystem);
}


void EFQualitySubviewTask::openDrawable() {
    mDisplay->openDrawable();
}

void EFQualitySubviewTask::draw(CineViewer &cineviewer, uint32_t frameNum)
{
    auto l = lock(mMutex);
    mDisplay->draw(cineviewer, mCineBuffer.getParams().converterParams);
}

// Destroy graphics resources
void EFQualitySubviewTask::close()
{
    mDisplay->closeDrawable();
}

ThorStatus EFQualitySubviewTask::onInit()
{
    auto l = lock(mMutex);
    mProcessedFrame = 0xFFFFFFFFu;
    mCineFrame = 0xFFFFFFFFu;
    mAutoCapturePrevScore = 0.0f;

    LOGD("onInit()");

    std::fill(mSubviewBuffer.begin(), mSubviewBuffer.end(), -1); // no data
    std::fill(mQualityBuffer.begin(), mQualityBuffer.end(), -1); // no data
    mPLAXPipeline->reset();
    mAutocaptureJustFailed = false;

    return THOR_OK;
}
void EFQualitySubviewTask::onTerminate()
{
    LOGD("onTerminate()");
    auto l = lock(mMutex);
    // autocapture end
    if (mAutocaptureEnabled) {
        mAIManager.getEFWorkflow()->notifyAutocaptureFail(mCallbackUser);
        mAutocaptureJustFailed = true;
    }

    // Reset the fast probe timer
    // Even if this is just a pause and imaging will resume, we can hide the probe too fast message
//    mDrawableState.fastProbeFrame = 0;

    //setPause(true);
}
void EFQualitySubviewTask::setParams(CineBuffer::CineParams& params)
{
    // Quick exit in Color or M mode
    LOGD("setParams, dauDataTypes = 0x%08x", params.dauDataTypes);
    if (BF_GET(params.dauDataTypes, DAU_DATA_TYPE_COLOR, 1) ||
        BF_GET(params.dauDataTypes, DAU_DATA_TYPE_M, 1)) {
        return;
    }

    mPLAXPipeline->setParams(params.converterParams);
    //mA5CPipeline->setParams(params.converterParams);

    {
        auto l = lock(mMutex);
        mScanConverter->setConversionParameters(
                MODEL_WIDTH,
                MODEL_HEIGHT,
                mFlipX,
                1.0f,
                ScanConverterCL::ScaleMode::NON_UNIFORM_SCALE_TO_FIT,
                params.converterParams);
    }

}

void EFQualitySubviewTask::onData(uint32_t frameNum, uint32_t dauDataTypes)
{
    // Only activate in purely B mode
    if (dauDataTypes == (1<<DAU_DATA_TYPE_B))
    {
        {
            auto l = lock(mMutex);
            mCineFrame = frameNum;
            if (!mGuidanceEnabled && !mQualityEnabled && !mAutocaptureEnabled)
            {
                // Not enabled, overwrite with empty data
                mUnsmoothSubviews[frameNum % FAST_PROBE_DETECT_FRAMES] = -1;
                mSubviewBuffer[frameNum % CINE_FRAME_COUNT] = -1;
                mQualityBuffer[frameNum % CINE_FRAME_COUNT] = -1;
            }
        }
        mCV.notify_one();
    }
}

void EFQualitySubviewTask::drawQuality(CineViewer &cineviewer, const DrawableState &state)
{
    float depth = state.params.startSampleMm +
                  (state.params.sampleSpacingMm * (state.params.numSamples - 1));
    float lineLen = depth / 5.0f; // length of each line in physical space
    float physToScreen[9];
    cineviewer.queryPhysicalToPixelMap(physToScreen, 9);
    float screenHeight = cineviewer.getImguiRenderer().getHeight();
    float screenToPhysScale = depth/screenHeight;

    float lineX = lineLen * cos(-state.params.startRayRadian);
    float lineY = lineLen * sin(-state.params.startRayRadian);

    // The values below used to be expressed in screen space coordinates,
    // but that led to issues when the image was flipped/not flipped. So instead, specify them
    // in physical coordinates and let physToScreen handle the flipping.
    // The odd values here approximately equal to the old values of 10 pixels and 5 pixels.
    float xDisplacement = 10.0f * screenToPhysScale; // physical space distance to offset lines so they don't overlap the cone
    float lineTrim = 5.0f * screenToPhysScale; // physical space distance that lines are shortened by on either end

    ImU32 qualityColor;
    if (state.qualityScore <= 2)
        qualityColor = ImColor(1.0f, 0.0f, 0.0f);
    else
        qualityColor =  ImColor(0.0f, 1.0f, 0.0f);
    // Quality lines on sides
    for (int l = 0; l < state.qualityScore; ++l) {
        // Physical coords
        float phys1[3] = {l * lineX + lineTrim + xDisplacement, l * lineY + lineTrim, 1.0f};
        float phys2[3] = {(l + 1) * lineX - lineTrim + xDisplacement, (l + 1) * lineY - lineTrim, 1.0f};

        float screen1[3];
        float screen2[3];
        ViewMatrix3x3::multiplyMV(screen1, physToScreen, phys1);
        ViewMatrix3x3::multiplyMV(screen2, physToScreen, phys2);

        ImVec2 o = ImVec2(screen1[0], screen1[1]);
        ImVec2 d = ImVec2(screen2[0], screen2[1]);
        ImGui::GetBackgroundDrawList()->AddLine(o, d, qualityColor, 10.0f);

        // draw flipped around x axis
        phys1[0] = -phys1[0];
        phys2[0] = -phys2[0];
        ViewMatrix3x3::multiplyMV(screen1, physToScreen, phys1);
        ViewMatrix3x3::multiplyMV(screen2, physToScreen, phys2);
        o = ImVec2(screen1[0], screen1[1]);
        d = ImVec2(screen2[0], screen2[1]);
        ImGui::GetBackgroundDrawList()->AddLine(o, d, qualityColor, 10.0f);
    }
}
void EFQualitySubviewTask::drawGuidance(CineViewer &cineviewer, const DrawableState &state, uint32_t frameNum)
{
    // Draw animations
    int viewIndex = 0;
    if (mView == CardiacView::A4C) viewIndex = 0;
    else if (mView == CardiacView::A2C) viewIndex = 1;
    else if (mView == CardiacView::PLAX) viewIndex = 2;
    else if (mView == CardiacView::A5C) viewIndex = 3;
//    mAnimationPlayer.setState(viewIndex, state.guidanceIndex);
//    mAnimationPlayer.draw(cineviewer, frameNum);

    if (state.fastProbeFrame > frameNum) {
        // Draw fast probe motion warning
        const char *text = "Slow down";
        auto size = ImGui::CalcTextSize(text);
        auto width = cineviewer.getImguiRenderer().getWidth();
        auto height = cineviewer.getImguiRenderer().getHeight();
        auto pos = ImVec2(width/2.0f - size.x/2.0f, height/2.0f - size.y/2.0f);
        ImGui::GetBackgroundDrawList()->AddText(pos, IM_COL32(0xF7, 0x94, 0x1D, 0xFF), text);
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

void EFQualitySubviewTask::drawConfig(CineViewer &cineviewer, const DrawableState &state)
{
    auto l = lock(mMutex);
    auto config = mConfig;
    l.unlock();

    ImGui::SetNextWindowSize(ImVec2(1000, -1));
    if (ImGui::Begin("IQSubview Details"))
    {
        if (!mPredictions.empty()) {
            auto pred = mPredictions.back();
            ImGui::PlotHistogram("Main view/subview", pred.subview, NELEM(pred.subview), 0, nullptr, 0.0f, 1.0f);
            ImGui::PlotHistogram("Quality", pred.quality, NELEM(pred.quality), 0, nullptr, 0.0f, 1.0f);
        }

        if (ImGui::CollapsingHeader("A4C Quality Thresholds"))
        {
            TweakFloat("A4C Quality 3", config.a4cQualityTau[2]);
            TweakFloat("A4C Quality 4", config.a4cQualityTau[3]);
            TweakFloat("A4C Quality 5", config.a4cQualityTau[4]);
        }
        if (ImGui::CollapsingHeader("A2C Quality Thresholds"))
        {
            TweakFloat("A2C Quality 3", config.a2cQualityTau[2]);
            TweakFloat("A2C Quality 4", config.a2cQualityTau[3]);
            TweakFloat("A2C Quality 5", config.a2cQualityTau[4]);
        }
        if (ImGui::CollapsingHeader("Subview Thresholds"))
        {
            for (unsigned int i=0; i < NELEM(IQ_SUBVIEW_NAMES); ++i)
            {
                TweakFloat(IQ_SUBVIEW_NAMES[i], config.subviewTau[i]);
            }
        }
        if (ImGui::Button("Reset to defaults"))
            config.reset();

        l = lock(mMutex);
        mConfig = config;
        l.unlock();
    }
    ImGui::End();
}


bool EFQualitySubviewTask::workerShouldAwaken()
{
    // Should always awaken if told to stop
    if (mStop)
        return true;
    // Otherwise, should awaken if either toggle is enabled, and there is a new frame
    return (mQualityEnabled || mGuidanceEnabled || mAutocaptureEnabled) && mCineFrame != mProcessedFrame;
}

void EFQualitySubviewTask::workerThreadFunc(void *jniInterface)
{
    LOGD("Starting workerThreadFunc");
    GuidanceJNI *jni = (GuidanceJNI*)jniInterface;
    jni->attachToThread();

//    mA4CModel = mAIManager.getModel(A4C_MODEL_NAME, A4C_OUTPUT_LAYERS, A4C_OUTPUT_NODES);
//    if (!mA4CModel) {
//        LOGE("Failed to load A4C model");
//        return;
//    }
//    mA2CModel = mAIManager.getModel(A2C_MODEL_NAME, A2C_OUTPUT_LAYERS, A2C_OUTPUT_NODES);
//    if (!mA2CModel) {
//        LOGE("Failed to load A2C model");
//        return;
//    }

    ThorStatus status = mPLAXPipeline->init(mUSManager, jni);
    if (IS_THOR_ERROR(status)) {
        LOGE("Error in PLAXPipeline->init(): 0x%08x", status);
        delete jni;
        return;
    }

    mDisplay->loadAnimations(*mAssets);
    LOGD("Loaded animations");

    uint32_t prevFrameNum = 0xFFFFFFFFu;
    uint32_t frameNum = 0xFFFFFFFFu;
    CardiacView view;
    while (true)
    {
        // Get next frame number to process
        {
            auto l = lock(mMutex);
            // Update the processed frame number
            mProcessedFrame = frameNum;

            // Wait while paused, or until either mStop or we have a frame to process
            while (!workerShouldAwaken())
                mCV.wait(l);
            if (mStop)
                return;
            prevFrameNum = frameNum;
            frameNum = mCineFrame;
            view = mView;
        }

        if (mAutocaptureTimeout > 0 && frameNum > prevFrameNum) {
            mAutocaptureTimeout -= static_cast<int>(frameNum-prevFrameNum);
            if (mAutocaptureTimeout < 0) {
                mAutocaptureTimeout = 0;
            }
        }

        // Process the frame (without holding the lock)
        if (view == CardiacView::PLAX) {
            ThorStatus status = mPLAXPipeline->process(frameNum, nullptr, nullptr);
            if (IS_THOR_ERROR(status))
            {
                LOGE("Error processing frame %u: %08x", frameNum, status);
            }
#if defined(THOR_MOCK_QUALITY)
            int quality = THOR_MOCK_QUALITY(frameNum);
#else
            int quality = mPLAXPipeline->quality();
#endif

            mQualityBuffer[frameNum % mQualityBuffer.size()] = quality;
            mSubviewBuffer[frameNum % mSubviewBuffer.size()] = mPLAXPipeline->view();


            if (mAutocaptureEnabled) {
                checkAutocapture(frameNum);
                checkSmartCapture(frameNum);
            }
            mLastProcessedQualityFrame = frameNum;

            {
                auto l = lock(mMutex);
                mDisplay->setQuality(Quality(mPLAXPipeline->quality()));
                if (frameNum % 30 == 0)
                    mDisplay->setSubview(Subview(mPLAXPipeline->view(), View::PLAX));
            }
        }
        else if (view == CardiacView::A5C) {
            // Todo Re-enanble A5C pipeline
//            ThorStatus status = mA5CPipeline->process(frameNum, nullptr, nullptr);
//            if (IS_THOR_ERROR(status))
//            {
//                LOGE("Error processing frame %u: %08x", frameNum, status);
//            }
//
//            auto l = lock(mDrawableMutex);
//            mDrawableState.loaded = true;
//            mDrawableState.qualityScore = mA5CPipeline->quality();
//            if (mA5CPipeline->isOptimal() || frameNum % 60 == 0)
//                mDrawableState.guidanceIndex = mA5CPipeline->view();
        }
        else if (frameNum % FRAME_DIVIDER == 0)
        {
            ThorStatus status = processFrame(frameNum, view, (void*)jni);
            if (IS_THOR_ERROR(status))
            {
                LOGE("Error processing frame %u: %08x", frameNum, status);
            }
        }
    }
    delete jni;
}

ThorStatus EFQualitySubviewTask::scanConvert(CineBuffer &cinebuffer, uint32_t frameNum, std::vector<float> &postscan)
{
    ThorStatus status;
    // Scan convert image
    uint8_t *prescan = cinebuffer.getFrame(frameNum, DAU_DATA_TYPE_B);
    postscan.resize(MODEL_WIDTH*MODEL_HEIGHT);
    status = mScanConverter->runCLTex(prescan, &postscan[0]);
    if (IS_THOR_ERROR(status)) {return status; }
    return THOR_OK;
}
ThorStatus EFQualitySubviewTask::runModel(JNIEnv *jenv, std::vector<float> &postscan, CardiacView view, EFQualitySubviewPred &pred, void *jniInterface)
{
    ThorStatus status;
    GuidanceJNI *guidance = (GuidanceJNI*)(jniInterface);

    // Run through model
    jmethodID processViewMethod = nullptr;
    if (view == CardiacView::A4C) {
        processViewMethod = guidance->processA4C;
    } else if (view == CardiacView::A2C) {
        processViewMethod = guidance->processA2C;
    } else {
        const char *viewName;
        switch (view) {
            case CardiacView::A4C: viewName = "A4C"; break; // unreachable, just here for completeness
            case CardiacView::A2C: viewName = "A2C"; break; // unreachable, just here for completeness
            case CardiacView::A5C: viewName = "A5C"; break;
            case CardiacView::PLAX: viewName = "PLAX"; break;
            default: viewName = "Unknown"; break;
        }
        LOGE("Invalid view for EFQualitySubviewModel: %s", viewName);
        return THOR_ERROR;
    }
    jenv->CallVoidMethod(guidance->instance, processViewMethod, guidance->imageBuffer, guidance->outputBuffer);
    std::copy_n(guidance->outputBufferNative.begin(), NUM_SUBVIEWS, pred.subview);
    std::copy_n(guidance->outputBufferNative.begin()+NUM_SUBVIEWS, 5, pred.quality);
    if (IS_THOR_ERROR(status)) {return status; }
    return THOR_OK;
}

void EFQualitySubviewTask::postProcessPred(EFQualitySubviewPred &pred)
{
    // Softmax all outputs
    softmax(std::begin(pred.subview), std::end(pred.subview));
    softmax(std::begin(pred.quality), std::end(pred.quality));
}

void EFQualitySubviewTask::enqueuePred(const EFQualitySubviewPred &pred)
{
    if (mPredictions.size() == TEMPORAL_SMOOTHING_FRAMES)
        mPredictions.pop_front();
    mPredictions.push_back(pred);
}

EFQualitySubviewPred EFQualitySubviewTask::computeMean()
{
    EFQualitySubviewPred mean;
    memset(&mean, 0, sizeof(mean));
    float N = static_cast<float>(mPredictions.size());
    for (auto &pred : mPredictions)
    {
        std::transform(std::begin(mean.subview), std::end(mean.subview), std::begin(pred.subview), std::begin(mean.subview), [=](float a, float b){return a + b/N;});
        std::transform(std::begin(mean.quality), std::end(mean.quality), std::begin(pred.quality), std::begin(mean.quality), [=](float a, float b){return a + b/N;});
    }
    return mean;
}

int EFQualitySubviewTask::rescaleQuality(int subviewPred, const EFQualitySubviewPred &pred, const EFQualitySubviewConfig &config)
{
    const float *iqThresholds;
    const float *iqPred;
    float subviewTau;
    int subviewTarget;
    int subviewOther;

    int excludeFromCapping[9];
    std::fill(std::begin(excludeFromCapping), std::end(excludeFromCapping), -1);
    float targetTau;

    if (mView == CardiacView::A4C)
    {
        iqThresholds = config.a4cQualityTau;
        iqPred = pred.quality;
        subviewTarget = 0;
        subviewOther = 10;
        targetTau = config.a4cQualityCapTau;
        excludeFromCapping[0] = 1; // Tilt down
        excludeFromCapping[1] = 2; // Tilt up
        excludeFromCapping[2] = 3; // CTR
        excludeFromCapping[3] = 4; // CLK
        excludeFromCapping[4] = 5; // LAT
        excludeFromCapping[5] = 6; // MED
        excludeFromCapping[6] = 7; // SUP
        excludeFromCapping[7] = 8; // SMED
        excludeFromCapping[8] = 17; // Not Sure
    }
    else
    {
        iqThresholds = config.a2cQualityTau;
        iqPred = pred.quality;
        subviewTarget = 10;
        subviewOther = 0;
        targetTau = config.a2cQualityCapTau;
        excludeFromCapping[0] = 3;  // CTR
        excludeFromCapping[1] = 13; // A2C-TDN
        excludeFromCapping[2] = 14; // A2C-TUP
        excludeFromCapping[3] = 15; // A2C-MED
        excludeFromCapping[4] = 16; // A2C-LAT
        excludeFromCapping[5] = 17; // Not sure
    }

    subviewTau = config.subviewTau[subviewTarget];
    int quality = static_cast<int>(argmax(iqPred, iqPred+5));

    // If we are in target subview and the confidence is high enough, rescale the quality higher
    if (subviewPred == subviewTarget && pred.subview[subviewPred] > subviewTau)
    {
        if (iqPred[4] > iqThresholds[4] && quality == 3)
            quality = 4;
        else if (iqPred[3] > iqThresholds[3] && quality == 2)
            quality = 3;
        else if (iqPred[2] > iqThresholds[2] & quality == 1)
            quality = 2;
    }

    if (subviewPred == subviewTarget)
        return quality;

    // Cap Quality by subview
    if (subviewPred == 11)
        quality = 0; // Noise
    else if (subviewPred == subviewOther)
        quality = 1; // A2C when looking for A4C

    if (std::find(std::begin(excludeFromCapping), std::end(excludeFromCapping), subviewPred) != std::end(excludeFromCapping)) {
        if (quality > 1 && pred.subview[subviewTarget] >= targetTau)
            quality = 2;
        else
            quality = 1;

        return quality;
    }

    return min(1, quality);
}

static bool IsA2CSubview(int subview) {
    return subview == 10 || subview == 12 || subview == 13 || subview == 14 || subview == 15 || subview == 16;
}

ThorStatus EFQualitySubviewTask::processFrame(uint32_t frameNum, CardiacView view, void *jniInterface)
{
    ThorStatus status;
    GuidanceJNI *guidance = (GuidanceJNI*)(jniInterface);
    // Clear prediction buffer if imaging has restarted
    if (frameNum == 0) {
        mPredictions.clear();
        std::fill(mQualityBuffer.begin(), mQualityBuffer.end(), -1);
        std::fill(mSubviewBuffer.begin(), mSubviewBuffer.end(), -1);
    }

    // Run scan conversion
    status = scanConvert(mCineBuffer, frameNum, guidance->imageBufferNative);
    if (IS_THOR_ERROR(status)) { return status; }

    // Run SNPE model
    EFQualitySubviewPred pred;
    status = runModel(guidance->jenv, guidance->imageBufferNative, view, pred, jniInterface);
    if (IS_THOR_ERROR(status)) { return status; }

    // Post process single prediction (no error conditions)
    postProcessPred(pred);

    // Store predicted subview in unsmooth subviews for fast probe motion detection
    auto subviewIter = std::max_element(std::begin(pred.subview), std::end(pred.subview));
    auto subview = std::distance(std::begin(pred.subview), subviewIter);
    mUnsmoothSubviews[frameNum % FAST_PROBE_DETECT_FRAMES] = static_cast<int>(subview);

    // Enqueue in temporal buffer
    enqueuePred(pred);

    // Average temporal buffer
    EFQualitySubviewPred meanPred = computeMean();

    // unique_elem in python: find max confidence subview
    auto subviewIt = std::max_element(std::begin(meanPred.subview), std::end(meanPred.subview));
    int subviewIdx = static_cast<int>(std::distance(std::begin(meanPred.subview), subviewIt));
    float subviewConf = *subviewIt;

    // rescale_iq in python
    auto l = lock(mMutex);
    auto config = mConfig;
    l.unlock();
    // State logic for subview
    if (subviewConf >= config.subviewTau[subviewIdx])
        mSubviewIndex = subviewIdx;
    else
        mSubviewIndex = 17; // Uncertain
    int quality = rescaleQuality(mSubviewIndex, meanPred, config) + 1;

#ifdef THOR_MOCK_QUALITY
    quality = THOR_MOCK_QUALITY(frameNum);
#endif
#ifdef THOR_MOCK_SUBVIEW
    mSubviewIndex = THOR_MOCK_SUBVIEW(frameNum);
#endif

    storeSmoothResults(frameNum, mSubviewIndex, quality);
    if (mAutocaptureEnabled) {
        checkAutocapture(frameNum);
        checkSmartCapture(frameNum);
    }
    mLastProcessedQualityFrame = frameNum;

    // Check for fast probe motion
    bool fastProbeMotion = checkFastProbeMotion(frameNum);

    {
        auto l = lock(mMutex);
        mDisplay->setQuality(Quality(quality));
        View v = view == CardiacView::A4C ? View::A4C : View::A2C;
        if (frameNum % 30 == 0)
            mDisplay->setSubview(Subview(mSubviewIndex, v));
    }

//    l = lock(mDrawableMutex);
//    mDrawableState.loaded = true;
//    mDrawableState.qualityScore = quality;
//    // A2C views update every 2 seconds, otherwise once per second
//    bool a2csubview = IsA2CSubview((mSubviewIndex));
//    if ((a2csubview && frameNum % 60 == 0) || (!a2csubview && frameNum % 30 == 0)) {
//        mDrawableState.guidanceIndex = mSubviewIndex;
//    }
//    if (fastProbeMotion) {
//        LOGI("Too many subviews in a short time, displaying probe motion warning (at frame %u, until frame %u)", frameNum, frameNum+FAST_PROBE_WARNING_FRAMES);
//        mDrawableState.fastProbeFrame = frameNum + FAST_PROBE_WARNING_FRAMES;
//    }

    return THOR_OK;
}

void EFQualitySubviewTask::setFlip(float flipX, float flipY) {
    auto l = lock(mMutex);
    // copy sc params out of drawable state
    ScanConverterParams converterParams = mCineBuffer.getParams().converterParams;

    mPLAXPipeline->setFlipX(flipX);
    //mA5CPipeline->setFlipX(flipX);
    mFlipX = flipX;
    mScanConverter->setFlip(flipX, flipY);
}

void EFQualitySubviewTask::storeSmoothResults(uint32_t frameNum, int subview, int quality) {
    auto index = frameNum % mAutoCaptureBufferSize;
    mSubviewBuffer[index] = subview;
    mQualityBuffer[index] = quality;
}

static int findMedian(const std::vector<int>& subviews) {
    int counts[NUM_SUBVIEWS] = {0};
    for (int subview : subviews) {
        if (subview >= 0)
            ++counts[subview];
    }
    return static_cast<int>(std::max_element(std::begin(counts), std::end(counts)) - counts);
}

void EFQualitySubviewTask::checkAutocapture(uint32_t frameNum) {
    // if the mean quality is above a threshold, and the median subview is a4c/a2c,
    // then send autocapture event (with a timeout?)
    constexpr int AUTOCAPTURE_QUALITY = 4;
    int prevQuality = mQualityBuffer[mLastProcessedQualityFrame % mQualityBuffer.size()];
    if (mLastProcessedQualityFrame == 0xFFFFFFFFu) {
        prevQuality = 0;
    }

    // It seems when imaging restarts, we get the following:
    //  onData(N)
    //  onTerminate()
    //  onInit() -> this resets mProcessedFrame to 0xFFFFFFFFu
    //  onData(N) -> repeated frame N means this runs again, with mLastProcessedQualityFrame == frameNum
    //     if we then looped from mLastProcessedQualityFrame+1 to frameNum, we would be looping over
    //     the entire range of a uint32_t, which blocked the worker thread for a while and caused
    //     the display to keep displaying the last quality.
    if ((frameNum <= mLastProcessedQualityFrame) ||
        (frameNum - mLastProcessedQualityFrame) > MAX_FRAME_QUALITY_FILL)
        mLastProcessedQualityFrame = frameNum - 1;

    for (uint32_t skipped = mLastProcessedQualityFrame + 1; (skipped != frameNum); ++skipped) {
        // treat skipped frames as same quality as just measured
        mQualityBuffer[skipped % mQualityBuffer.size()] = prevQuality;
    }

    // Earliest frame in the buffer that we should check
    uint32_t earliestFrame = frameNum - min(frameNum, AUTOCAPTURE_NUM_FRAMES + 1);

    // find most recent frame with quality 4-5 (end of our consecutive range of good frames)
    // This is a "one past the end" index
    uint32_t rangeEnd = frameNum + 1;
    for (; rangeEnd > earliestFrame; --rangeEnd) {
        if (mQualityBuffer[(rangeEnd - 1) % mAutoCaptureBufferSize] >= AUTOCAPTURE_QUALITY)
            break;
    }
    // Find start of range of consecutive good frames
    // (first index where the previous frame is quality < 3)
    uint32_t rangeBegin = rangeEnd;
    for (; rangeBegin > earliestFrame; --rangeBegin) {
        if (mQualityBuffer[(rangeBegin - 1) % mAutoCaptureBufferSize] < AUTOCAPTURE_QUALITY)
            break;
    }

    uint32_t rangeSize = rangeEnd - rangeBegin;
    float qualityRatio = static_cast<float>(rangeSize) / AUTOCAPTURE_NUM_FRAMES;
    bool recentFrameIsBad = rangeEnd != frameNum + 1;
    bool triggerAutocapture =
            (qualityRatio >= AUTOCAPTURE_HIGH_THRESHOLD) ||
            (qualityRatio >= AUTOCAPTURE_LOW_THRESHOLD && recentFrameIsBad);

    // If we lost our streak, then reset the display progress bar
    if (recentFrameIsBad)
        qualityRatio = 0.0f;

    int medianSubview = findMedian(mSubviewBuffer);
    int targetSubview = (mView == CardiacView::A4C ? 0 : 10);

    //LOGD("quality streak = %f, subview = %d", qualityStreakRatio, medianSubview);

//    auto l = lock(mDrawableMutex);
//    float prevScore = mDrawableState.autocaptureScore;
//    mDrawableState.autocaptureScore = qualityRatio;
//    mDrawableState.autocaptureIndex = medianSubview;
//    mDrawableState.autocaptureMinThreshold = AUTOCAPTURE_LOW_THRESHOLD;
//    l.unlock();

    // Notify progress
    {
        auto l = lock(mMutex);
        int prevSection = static_cast<int>(mAutoCapturePrevScore / AUTOCAPTURE_NOTIFY_EVERY);
        int curSection = static_cast<int>(qualityRatio / AUTOCAPTURE_NOTIFY_EVERY);
        if (curSection != prevSection) {
            LOGD("notifyAutocaptureProgress(%0.4f), mAutocaptureTimeout = %d, recentFrameIsBad = %s",
                 qualityRatio, mAutocaptureTimeout, recentFrameIsBad ? "true" : "false");
            mAIManager.getEFWorkflow()->notifyAutocaptureProgress(mCallbackUser, qualityRatio);
        }

        if (rangeSize == 1 && !recentFrameIsBad) {
            // autocapture start
            mAIManager.getEFWorkflow()->notifyAutocaptureStart(mCallbackUser);
        } else if (recentFrameIsBad && mAutoCapturePrevScore != 0.0f) {
            // autocapture end
            mAIManager.getEFWorkflow()->notifyAutocaptureFail(mCallbackUser);
            mAutocaptureJustFailed = true;
        } else if (triggerAutocapture /* && medianSubview == targetSubview */
                   /* && mAutocaptureTimeout < 0 */) {
            LOGD("Triggering autocapture on frame %u", frameNum);
            mAutocaptureTimeout = AUTOCAPTURE_NUM_FRAMES +
                                  AUTOCAPTURE_TIMEOUT_EXTRA; // do not trigger again for at least as long as the buffer
            resetQualityBuffers();
            mAIManager.getEFWorkflow()->notifyAutocaptureSucceed(mCallbackUser);
        }
        // implicit unlock mMutex here
    }

    mAutoCapturePrevScore = qualityRatio;
}

bool EFQualitySubviewTask::checkFastProbeMotion(uint32_t frameNum) {
    auto start = frameNum < FAST_PROBE_DETECT_FRAMES ? 0u : frameNum - FAST_PROBE_DETECT_FRAMES;
    // count unique subviews in last N frames
    bool present[NUM_SUBVIEWS];
    std::fill(std::begin(present), std::end(present), false);
    for (uint32_t i=start; i != frameNum; ++i ) {
        int subview = mUnsmoothSubviews[i % FAST_PROBE_DETECT_FRAMES];
        if (0 <= subview && subview < NUM_SUBVIEWS)
            present[subview] = true;
    }
    int numUnique = std::count(std::begin(present), std::end(present), true);
    bool result = numUnique > FAST_PROBE_MAX_SUBVIEWS;
    if (result) {
        LOGD("Triggering probe too fast message, numUnique = %d, start=%u, end=%u",
                numUnique, start, frameNum);
    }
    return result;
}

void* EFQualitySubviewTask::setCallbackUserData(void *user) {
    auto l = lock(mMutex);
    void *old = mCallbackUser;
    mCallbackUser = user;
    return old;
}

void* EFQualitySubviewTask::getCallbackUserData() {
    auto l = lock(mMutex);
    return mCallbackUser;
}

static echonous::capnp::Guidance::View convertToCapnpView(CardiacView view) {
    using View = echonous::capnp::Guidance::View;
    switch (view) {
    case CardiacView::A4C : return View::A4C;
    case CardiacView::A2C : return View::A2C;
    case CardiacView::PLAX: return View::PLAX;
    case CardiacView::A5C : return View::A5C;
    default: return View::NO_INFORMATION;
    }
}

static CardiacView convertToThorView(echonous::capnp::Guidance::View view) {
    using View = echonous::capnp::Guidance::View;
    switch (view) {
        case View::A4C : return CardiacView::A4C;
        case View::A2C : return CardiacView::A2C;
        case View::PLAX: return CardiacView::PLAX;
        case View::A5C : return CardiacView::A5C;
        default:
            LOGW("No matching view for capnp enum value 0x%08x", static_cast<uint32_t>(view));
            return CardiacView::A4C;
    }
}

static echonous::capnp::Guidance::Subview convertToCapnpSubview(int subview) {
    using Subview = echonous::capnp::Guidance::Subview;
    // use mapping from model subview -> capnp subview here
    // for now, hard code the conversion?
    switch(subview) {
    case -1: return Subview::NO_INFORMATION;
    case 0:  return Subview::A4C;
    case 1:  return Subview::TILT_DOWN;
    case 2:  return Subview::TILT_UP;
    case 3:  return Subview::COUNTER_CLK;
    case 4:  return Subview::CLOCKWISE;
    case 5:  return Subview::ANGLE_LATERAL;
    case 6:  return Subview::ANGLE_MEDIAL;
    case 7:  return Subview::RIB_SPACE_UP;
    case 8:  return Subview::SLIDE_MEDIAL;
    case 9:  return Subview::SLIDE_LATERAL;
    case 10: return Subview::A2C;
    case 11: return Subview::NOISE;
    case 12: return Subview::A3C;
    case 13: return Subview::TILT_DOWN_A2C;
    case 14: return Subview::TILT_UP_A2C;
    case 15: return Subview::ANGLE_MEDIAL_A2C;
    case 16: return Subview::ANGLE_LATERAL_A2C;
    case 17: return Subview::UNCERTAIN;
    case 18: return Subview::FLIPPED_A4C;
    //case 19: return Subview::FLIPPED_A2C;
    default: return Subview::NO_INFORMATION;
    }
}

static echonous::capnp::Guidance::Subview convertPLAXToCapnpSubview(int subview) {
    using Subview = echonous::capnp::Guidance::Subview;
    // use mapping from model subview -> capnp subview here
    // for now, hard code the conversion?
    switch(subview) {
        case -1: return Subview::NO_INFORMATION;
        case 0:  return Subview::PLAX_APICAL;
        case 1:  return Subview::PLAX_PSAX_AV;
        case 2:  return Subview::PLAX_PSAX_MV;
        case 3:  return Subview::PLAX_PSAX_PM;
        case 4:  return Subview::PLAX_PSAX_AP;
        case 5:  return Subview::PLAX_OTHER;
        case 6:  return Subview::PLAX_PLAX_BEST;
        case 7:  return Subview::PLAX_PLAX_ALAT;
        case 8:  return Subview::PLAX_PLAX_AMED;
        case 9:  return Subview::PLAX_PLAX_SLAT;
        case 10: return Subview::PLAX_PLAX_SMED;
        case 11: return Subview::PLAX_PLAX_SUP;
        case 12: return Subview::PLAX_PLAX_SDN;
        case 13: return Subview::PLAX_PLAX_CLK;
        case 14: return Subview::PLAX_PLAX_CTR;
        case 15: return Subview::PLAX_PLAX_TDN;
        case 16: return Subview::PLAX_RVIT;
        case 17: return Subview::PLAX_PLAX_TUP;
        case 18: return Subview::PLAX_RVOT;
        case 19: return Subview::PLAX_PLAX_UNSURE;
        case 20: return Subview::PLAX_FLIP;
        default: return Subview::NO_INFORMATION;
    }
}

static int convertToThorSubview(echonous::capnp::Guidance::Subview subview) {
    using Subview = echonous::capnp::Guidance::Subview;
    // TODO: This should use some config file to match stored subview to runtime int index
    switch(subview) {
        case Subview::NO_INFORMATION : return -1;
        case Subview::A4C: return 0;
        case Subview::TILT_DOWN: return 1;
        case Subview::TILT_UP: return 2;
        case Subview::COUNTER_CLK: return 3;
        case Subview::CLOCKWISE: return 4;
        case Subview::ANGLE_LATERAL: return 5;
        case Subview::ANGLE_MEDIAL: return 6;
        case Subview::RIB_SPACE_UP: return 7;
        case Subview::SLIDE_MEDIAL: return 8;
        case Subview::SLIDE_LATERAL: return 9;
        case Subview::A2C: return 10;
        case Subview::NOISE: return 11;
        case Subview::A3C: return 12;
        case Subview::TILT_DOWN_A2C: return 13;
        case Subview::TILT_UP_A2C: return 14;
        case Subview::ANGLE_MEDIAL_A2C: return 15;
        case Subview::ANGLE_LATERAL_A2C: return 16;
        case Subview::UNCERTAIN: return 17;
        case Subview::FLIPPED_A4C: return 18;
        //case Subview::FLIPPED_A2C: return 19;
        default: return -1;
    }
}

void EFQualitySubviewTask::onSave(uint32_t startFrame, uint32_t endFrame,
                                  echonous::capnp::ThorClip::Builder &builder) {
    LOGD("onSave(%u, %u)", startFrame, endFrame);

    // OTS-2576: this seems to be reading bad data somewhere? But only when we capture via autocapture??
    return;

    auto guidance = builder.initGuidance();
    if (mView == CardiacView::A4C)
        guidance.setModelVersion(A4C_MODEL_NAME);
    else if (mView == CardiacView::A2C)
        guidance.setModelVersion(A2C_MODEL_NAME);
    else if (mView == CardiacView::PLAX)
        guidance.setModelVersion("2022_01_19_PLAX_SingleBranch.dlc");
    guidance.setTargetView(convertToCapnpView(mView));
    auto predictions = guidance.initPredictions(endFrame-startFrame+1);
    for (uint32_t i=0u; i != endFrame-startFrame+1; ++i) {
        auto index = (i+startFrame) % mAutoCaptureBufferSize;
        predictions[i].setQualityScore(mQualityBuffer[index]); // quality needs no conversion
        if (mView == CardiacView::PLAX)
            predictions[i].setSubview(convertPLAXToCapnpSubview(mSubviewBuffer[index]));
        else
            predictions[i].setSubview(convertToCapnpSubview(mSubviewBuffer[index]));
    }
}
void EFQualitySubviewTask::onLoad(echonous::capnp::ThorClip::Reader &reader)
{
    if (reader.hasGuidance()) {
        auto guidance = reader.getGuidance();
        auto version = guidance.getModelVersion(); // type kj::StringPtr
        auto view = guidance.getTargetView();
        LOGD("Loading guidance version %s, view %s",
             version.cStr(), convertToThorView(view) == CardiacView::A4C ? "A4C" : "A2C");

        //mView = convertToThorView(view);
        const auto& preds = guidance.getPredictions();
        for (uint32_t i=0; i !=preds.size() && i <= mAutoCaptureBufferSize; ++i) {
            //mQualityBuffer[i] = preds[i].getQualityScore();
            //mSubviewBuffer[i] = convertToThorSubview(preds[i].getSubview());
        }
    }
}

bool EFQualitySubviewTask::isSmartCaptureRange(uint32_t from, uint32_t to, uint32_t *pnext) {
    //LOGD("Smart capture: Checking frame range %u-%u...", from, to);
    for (uint32_t i=from; i != to; ++i) {
        if (mQualityBuffer[i%mAutoCaptureBufferSize] < 3) {
            //LOGE("Smart capture: Failed, next range starts at %u", i+1);
            *pnext = i+1;
            return false;
        }
    }
    return true;
}

ThorStatus EFQualitySubviewTask::findSmartCaptureRange(uint32_t numFrames,
                                                 uint32_t *startFrame, uint32_t *endFrame)
{
    LOGD("Smart capture: mProcessedFrame = %u, numFrames = %u", mProcessedFrame, numFrames);

    // fake the quality scores
//    for (uint32_t frameNum=0; frameNum < 20; ++frameNum) {
//        mQualityBuffer[frameNum % mAutoCaptureBufferSize] = 4;
//    }
//    for (uint32_t frameNum=20; frameNum < 120; ++frameNum) {
//        mQualityBuffer[frameNum % mAutoCaptureBufferSize] = 2;
//    }
//    for (uint32_t frameNum=120; frameNum < mProcessedFrame; ++frameNum) {
//        mQualityBuffer[frameNum % mAutoCaptureBufferSize] = 5;
//    }

    uint32_t frameNum = mProcessedFrame;
    if (frameNum < numFrames) {
        LOGD("Smart capture: Not enough frames, returning false (needed %u, have %u)", numFrames, frameNum);
        return TER_AI_SC_NOFRAMES;
    }
    auto start = frameNum < mAutoCaptureBufferSize ? 0u : frameNum-mAutoCaptureBufferSize;
    start += 10; // give a 10 frame buffer to make sure UI has time to capture the range
    auto end = max(frameNum, numFrames)-numFrames+1;
    for (uint32_t i = start; i < end;) {
        uint32_t next;
        // TODO This currently only selects the FIRST range than meets the minimum criteria,
        // need to find the BEST range
        if (isSmartCaptureRange(i, i+numFrames, &next)) {
            *startFrame = i;
            *endFrame = i+numFrames;
            return THOR_OK;
        } else {
            i = next;
        }
    }
    LOGD("Smart capture: failed to find any valid range");
    return TER_AI_SC_NO_VALID_RANGE;
}

void EFQualitySubviewTask::checkSmartCapture(uint32_t frameNum) {
    // Get count of current frames that are 3+
    uint32_t currentCount = 0;
    for (uint32_t i=0; i < AUTOCAPTURE_NUM_FRAMES; ++i) {
        if (i > frameNum) {
            break;
        }
        uint32_t frame = frameNum-i;
        int quality = mQualityBuffer[frame % mAutoCaptureBufferSize];
        if (quality < 3)
            break;
        ++currentCount;
    }
    // If currently progress is 1.0 (meaning we had a good range somewhere in the CineBuffer),
    // Check if we still do
    bool hasRange = false;
    if (mSmartCapturePrevScore == 1.0f) {
        auto start = frameNum < mAutoCaptureBufferSize ? 0u : frameNum-mAutoCaptureBufferSize;
        start += 20; // give a 20 frame buffer to make sure UI has time to capture the range
        // (NOTE: larger than the buffer used in findSmartCaptureRange to make this more strict)
        auto end = max(frameNum, AUTOCAPTURE_NUM_FRAMES)-AUTOCAPTURE_NUM_FRAMES+1;
        for (uint32_t i = start; i < end;) {
            uint32_t next;
            if (isSmartCaptureRange(i, i+AUTOCAPTURE_NUM_FRAMES, &next)) {
                hasRange = true;
                break;
            } else {
                i = next;
            }
        }
    }
    float newScore = mSmartCapturePrevScore;
    if (hasRange) {
        // If we have a valid range in the CineBuffer (regardless of current status), then the score
        // is 1.0f
        newScore = 1.0f;
    } else if (currentCount == 0) {
        // If current count is 0 (meaning we are currently scanning at less than 3 quality)
        // and we do not have a range of valid frames, then we should immediately set the score to 0
        newScore = 0.0f;
    } else {
        // Currently scanning 3+, and no good range in the CineBuffer
        newScore = static_cast<float>(currentCount)/AUTOCAPTURE_NUM_FRAMES;
    }
    auto section = [](float score) {
        if (score <= 0.0f) {
            // lowest score is section 0
            return 0;
        } else if (score >= 1.0f) {
            // highest score is highest section (total number of sections includes extra two for <=0 and >=1)
            return static_cast<int>(1.0f/SMARTCAPTURE_NOTIFY_EVERY)+2;
        } else {
            return static_cast<int>(score/SMARTCAPTURE_NOTIFY_EVERY)+1;
        }
    };
    int prevSection = section(mSmartCapturePrevScore);
    int curSection  = section(newScore);
    if (prevSection != curSection || mAutocaptureJustFailed) {
        // Notify UI of change
        auto l = lock(mMutex);
        mAutocaptureJustFailed = false;
        mAIManager.getEFWorkflow()->notifySmartCaptureProgress(mCallbackUser, newScore);
    }
    mSmartCapturePrevScore = newScore;
}

static void fwriteall(const void *ptr, size_t size, size_t count, FILE *stream)
{
    while (count)
    {
        size_t written = fwrite(ptr, size, count, stream);
        if (written == 0)
        {
            perror("fwrite failed");
            std::abort();
        }
        ptr = (const void*)((const char*)ptr + written*size);
        count -= written;
    }
}

template <typename Iterable>
Json::Value json_list(Iterable &&iterable) {
    using std::begin;
    using std::end;
    auto first = begin(iterable);
    auto last  = end(iterable);

    Json::Value list;
    while (first != last) {
        list.append(*first++);
    }
    return list;
}

static Json::Value json_pred(const EFQualitySubviewPred &pred) {
    Json::Value root;
    root["subview"] = json_list(pred.subview);
    root["quality"] = json_list(pred.quality);
    return root;
}

Json::Value EFQualitySubviewTask::runA4CA2CVerificationOnFrame(JNIEnv *jenv, CardiacView view, uint32_t frameNum, FILE *raw) {
    // All intermediate values
    std::vector<float> scanframe;
    EFQualitySubviewPred rawPred;
    EFQualitySubviewPred procPred;
    EFQualitySubviewPred smoothPred;

    // Scan convert frame
    scanConvert(mCineBuffer, frameNum, scanframe);
    scanConvert(mCineBuffer, frameNum, mJNI->imageBufferNative);
    // Output scan converted frame
    if (raw) {
        fwriteall(scanframe.data(), sizeof(float), scanframe.size(), raw);
    }

    // Run model
    runModel(jenv, scanframe, view, rawPred, (void*)mJNI);

    // Post process and smooth prediction
    procPred = rawPred;
    postProcessPred(procPred);
    enqueuePred(procPred);
    smoothPred = computeMean();

    // Get thresholded subview and rescaled quality
    auto subview = std::max_element(std::begin(smoothPred.subview), std::end(smoothPred.subview));
    auto subviewIndex = std::distance(std::begin(smoothPred.subview), subview);
    int resultSubview;
    if (*subview > mConfig.subviewTau[subviewIndex])
        resultSubview = subviewIndex;
    else
        resultSubview = 17; // uncertain

    int resultQuality = rescaleQuality(resultSubview, smoothPred, mConfig);

    // Jsonify output
    Json::Value root;
    root["rawPred"]      = json_pred(rawPred);
    root["procPred"]     = json_pred(procPred);
    root["smoothPred"]   = json_pred(smoothPred);
    root["subviewIndex"] = resultSubview;
    root["qualityScore"] = resultQuality+1;

    return root;
}

ThorStatus EFQualitySubviewTask::runA4CA2CVerificationOnCinebuffer(JNIEnv *jenv, const char *inputPath,
                                                                   const char *outputPath)
{
    setFlip(-1.0f, 1.0f);
    char path[256];
    const char *clip = strrchr(inputPath, '/');
    int clipIDLen = static_cast<int>(strrchr(clip, '.')-clip);
    setView(CardiacView::A4C);
    snprintf(path, sizeof(path), "%s/%.*s.raw", outputPath, clipIDLen, clip);
    FILE *raw = fopen(path, "wb");
    Json::Value a4c;
    uint32_t maxFrame = mCineBuffer.getMaxFrameNum();
    for (uint32_t frameNum = 0; frameNum != maxFrame; ++frameNum) {
        a4c["frames"].append(runA4CA2CVerificationOnFrame(jenv, CardiacView::A4C, frameNum, raw));
    }
    if(raw != nullptr)
        fclose(raw);

    snprintf(path, sizeof(path), "%s/%.*s-target-a4c.json", outputPath, clipIDLen, clip);
    std::ofstream a4cout(path);
    a4cout << a4c << "\n";
    setView(CardiacView::A2C);
    Json::Value a2c;
    maxFrame = mCineBuffer.getMaxFrameNum();
    for (uint32_t frameNum = 0; frameNum != maxFrame; ++frameNum) {
        a2c["frames"].append(runA4CA2CVerificationOnFrame(jenv, CardiacView::A2C, frameNum, NULL));
    }

    snprintf(path, sizeof(path), "%s/%.*s-target-a2c.json", outputPath, clipIDLen, clip);
    std::ofstream a2cout(path);
    a2cout << a2c << "\n";

    return THOR_OK;
}


ThorStatus EFQualitySubviewTask::runPLAXVerificationOnCinebuffer(JNIEnv *jenv, const char *inputPath,
                                                                   const char *outputPath)
{
    ThorStatus status;
    setFlip(-1.0f, 1.0f);
    mPLAXPipeline->setFlipX(-1.0f);
    resetQualityBuffers();
    Json::Value json;
    const char* fname_path = strrchr(inputPath, '/');
    int offset = 1;
    if(fname_path == nullptr)
    {
        // not a path, just a filename
        fname_path = inputPath;
        offset = 0;
    }
    const char *fname = fname_path + offset;
    const char *ext = strrchr(inputPath, '.');

    std::string rawpath = outputPath;
    rawpath.push_back('/');
    rawpath.append(fname, ext-fname);

    std::string rawRealPath = rawpath;
    rawRealPath.append(".raw");
    std::ofstream raw(rawRealPath);

    {
        json["file"] = fname;
        mPLAXPipeline->reset();
        for (uint32_t frameNum = mCineBuffer.getMinFrameNum(); frameNum != mCineBuffer.getMaxFrameNum(); ++frameNum) {
            json["frames"].append(mPLAXPipeline->processIntoJson(jenv, frameNum));

            std::stringstream pgmPath;
            pgmPath << rawpath << "_frame" << frameNum << ".pgm";
            std::ofstream pgm(pgmPath.str());
            pgm << "P5\n" << 224 << " " << 224 << "\n";
            pgm << "255\n";
            pgm.write(
                reinterpret_cast<const char*>(mPLAXPipeline->mJNI->imageBufferNative.data()),
                mPLAXPipeline->mJNI->imageBufferNative.size());
            raw.write(
                    reinterpret_cast<const char*>(mPLAXPipeline->mJNI->imageBufferNative.data()),
                    mPLAXPipeline->mJNI->imageBufferNative.size());
        }
    }

    // Write JSON to output file
    std::string outpath = outputPath;
    outpath.push_back('/');
    outpath.append(fname, ext-fname);
    outpath.append(".json");
    std::ofstream outfile(outpath);
    outfile << json;
    return THOR_OK;
}