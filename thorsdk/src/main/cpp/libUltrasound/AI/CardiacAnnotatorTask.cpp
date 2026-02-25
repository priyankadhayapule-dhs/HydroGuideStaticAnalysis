#define LOG_TAG "Autolabel Task"

#include <algorithm>
#include <iterator>
#include <numeric>
#include <cmath>
#include <CardiacAnnotatorTask.h>
#include <AIManager.h>
#include <BitfieldMacros.h>
#include <imgui.h>
#include <jni.h>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <random>
//#include <TDIAnnotatorConfig.h>
//#define MOCK_AUTOLABEL(X) MockAutolabel(X)

std::vector<YoloPrediction> MockAutolabel(uint32_t frameNum) {
    YoloPrediction p = {0};
    p.x = 10.0f * sin(2.0f*3.14159f / 30.0f * frameNum);
    p.y = 80.0f + 10.0f * sin(3.14159f / 30.0f * frameNum);
    p.class_pred = 5;
    return std::vector<YoloPrediction>{p};
}

static std::pair<float, float> MockDopplerGate(uint32_t frameNum) {
    if (frameNum < 5*30) {
        return std::pair(0,-1);
    }

    std::random_device rand;
    std::mt19937 gen(rand());
    std::uniform_int_distribution<> xRandom(-20, 20);
    std::uniform_int_distribution<> yRandom(0, 80);

    float x = xRandom(gen) + sin(frameNum/30.0f * 3.14159f*2.0f) * 5.0f;
    float y = yRandom(gen) + sin(frameNum/40.0f * 3.14159f*2.0f) * 3.0f;
    return std::pair(x, y);;
}

static const int TEMPORAL_SMOOTHING_FRAMES = 15;
static const int FRAME_DIVIDER = 1; // Run every N frames
static const int PREDS_PER_FRAME = 588;
using lock = std::unique_lock<std::mutex>;

struct Label {
    int id;
    float x;
    float y;
    float w;
    float h;
    float conf;
} __attribute__((packed));

struct Prediction {
    float x;
    float y;
    float w;
    float h;
    float objConf;
    float lblConf[CARDIAC_ANNOTATOR_CLASSIFIER_NUM_CLASSES]; //note this is used for PW / TDI preds, TDI only has 3 classes
}__attribute__((packed));

struct PipelineFullResults {
    float view[CARDIAC_ANNOTATOR_CLASSIFIER_NUM_CLASSES];
    int numLabels;
    Label labels[CARDIAC_ANNOTATOR_LABELER_NUM_CLASSES];
    float smoothView[CARDIAC_ANNOTATOR_CLASSIFIER_NUM_CLASSES];
    int numSmoothLabels;
    Label smoothLabels[CARDIAC_ANNOTATOR_LABELER_NUM_CLASSES];
    Prediction yoloRaw[PREDS_PER_FRAME];
    Prediction yoloProc[PREDS_PER_FRAME];


} __attribute__((packed));

// Things needed to call JNI interface
struct AutolabelJNI {
    JavaVM *jvm;
    JNIEnv *jenv;
    jclass clazz;
    jobject context;
    jobject instance;
    jmethodID ctor;
    jmethodID load;
    jmethodID process;
    jmethodID processWithIntermediates;
    jmethodID processPW;
    jmethodID processPWWithIntermediates;
    jmethodID processTDI;
    jmethodID processTDIWithIntermediates;
    jmethodID clear;
    jobject imageBuffer;
    std::vector<float> imageBufferNative;
    jobject labelBuffer;
    std::vector<Label> labelBufferNative;

    jobject fullResultsBuffer;
    PipelineFullResults fullResults;

    AutolabelJNI(JNIEnv *jenv, jobject context) {
        jvm = nullptr;
        jenv->GetJavaVM(&jvm);
        this->jenv = nullptr; // thread local, this is set on the worker thread
        this->clazz = static_cast<jclass>(jenv->NewGlobalRef(
                jenv->FindClass("com/echonous/ai/autolabel/NativeInterface")));
        this->context = jenv->NewGlobalRef(context);

        ctor = jenv->GetMethodID(clazz, "<init>", "(Landroid/content/Context;)V");
        instance = nullptr; // this is slow, initialize on worker
        load = jenv->GetMethodID(clazz, "load", "()V");
        process = jenv->GetMethodID(clazz, "process", "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)I");
        processPW = jenv->GetMethodID(clazz, "processPW", "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)I");
        processTDI = jenv->GetMethodID(clazz, "processTDI", "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)I");
        processWithIntermediates = jenv->GetMethodID(clazz, "processWithIntermediates",
                                                     "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)V");
        processPWWithIntermediates = jenv->GetMethodID(clazz, "processPWWithIntermediates",
                                                     "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)V");
        processTDIWithIntermediates = jenv->GetMethodID(clazz, "processTDIWithIntermediates",
                                                        "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)V");

        clear = jenv->GetMethodID(clazz, "clear", "()V");
        // Buffers are also initialized on worker thread
        imageBuffer = nullptr;
        labelBuffer = nullptr;

        LOGD("Created JNI interface, jvm = %p, jenv = %p, clazz = %p, instance = %p, process = %p, clear = %p",
             jvm, jenv, clazz, instance, process, clear);
    }

    void attachToThread() {
        auto makeGlobalRef = [=](jobject local) {
            jobject global = this->jenv->NewGlobalRef(local);
            this->jenv->DeleteLocalRef(local);
            return global;
        };
        if (jenv != nullptr) {
            LOGE("Should not be attached to any thread!");
            return;
        }
        LOGD("Attaching autolabel to thread, creating instance...");
        jvm->AttachCurrentThread(&jenv, nullptr);
        instance = makeGlobalRef(jenv->NewObject(clazz, ctor, context));
        // instance is now a local ref, no need to add it as global or anything.

        // Create buffers on this thread as well
        imageBufferNative.resize(CARDIAC_ANNOTATOR_INPUT_HEIGHT*CARDIAC_ANNOTATOR_INPUT_WIDTH);
        imageBuffer = makeGlobalRef(jenv->NewDirectByteBuffer(imageBufferNative.data(), (jlong)imageBufferNative.size() * sizeof(float)));

        labelBufferNative.resize(CARDIAC_ANNOTATOR_LABELER_NUM_CLASSES);
        labelBuffer = makeGlobalRef(jenv->NewDirectByteBuffer(labelBufferNative.data(), (jlong)labelBufferNative.size() * sizeof(Label)));

        fullResultsBuffer = makeGlobalRef(jenv->NewDirectByteBuffer(&fullResults, (jlong)sizeof(fullResults)));

        jenv->CallVoidMethod(instance, load);
        LOGD("Autolabel pipeline loaded");

        jenv->DeleteGlobalRef(context);
        context = nullptr;
        LOGD("Attached to thread, instance = %p", instance);
    }

    ~AutolabelJNI() {
        LOGD("Destroying JNI interface");
        jenv->DeleteGlobalRef(fullResultsBuffer);
        jenv->DeleteGlobalRef(labelBuffer);
        jenv->DeleteGlobalRef(imageBuffer);
        jenv->DeleteGlobalRef(instance);
        jenv->DeleteGlobalRef(clazz);
        jvm->DetachCurrentThread();
        jenv = nullptr;
    }
};

CardiacAnnotatorTask::CardiacAnnotatorTask(UltrasoundManager *usm) :
    mUSManager(usm),
    mCB(&usm->getCineBuffer()),
    mPaused(true),
    mStop(false),
    mProcessedFrame(0xFFFFFFFFu),
    mCineFrame(0xFFFFFFFFu),
    mView(CardiacView::A4C),
    mYoloBuffer(TEMPORAL_SMOOTHING_FRAMES),
    mInit(false),
    mMutex(),
    mCV(),
    mWorkerThread(),
    mScanConverter(nullptr),
    mImguiRenderer(usm->getAIManager().getImguiRenderer()),
    mIsDevelopMode(false),
    mJNI(nullptr),
    mShouldClear(false),
    mTarget(AutoDopplerTarget::None),
    mAutoDopplerCallbackUser(nullptr),
    mAutoDopplerCallback(nullptr),
    mMockAutoDoppler(false)
{
    LOGD("Created CardiacAnnotatorTask");
}

CardiacAnnotatorTask::~CardiacAnnotatorTask()
{
    stop();
}

ThorStatus CardiacAnnotatorTask::init() {
    // Create scan converter (destroys old scan converter, if any)
    mScanConverter = std::unique_ptr<ScanConverterCL>(new ScanConverterCL);
    // Init scan converter with dummy values. setParams will be called before imaging
    // actually starts, and that will overwrite any values set here.
    // Might as well use reasonable default though
    ThorStatus status = mScanConverter->setConversionParameters(
            512, 112,
            CARDIAC_ANNOTATOR_INPUT_WIDTH, CARDIAC_ANNOTATOR_INPUT_HEIGHT,
            0.0f, 120.0f, 0.0f, 512.0f/120.0f, -0.7819f, 0.013962f,
            -1.0f, 1.0f);
    if (IS_THOR_ERROR(status)) { return status; }
    status = mScanConverter->init();
    if (IS_THOR_ERROR(status)) { return status; }

    mUnsmoothPredictionBuffer.resize(CINE_FRAME_COUNT);
    mPredictionBuffer.resize(CINE_FRAME_COUNT);
    mViewCineBuffer.resize(CINE_FRAME_COUNT);
    mFrameWasSkipped.resize(CINE_FRAME_COUNT);
    mDopplerGateBuffer.clear();
    mDopGateCoord = vec2(-1.0f, -1.0f);

    // load locale map, default to en
    Json::Value trio_locale;
    status = mUSManager->getFilesystem()->readAssetJson("trio_locale.json", trio_locale);
    if (IS_THOR_ERROR(status)) { return status; }
    mLocaleMap = trio_locale["autolabel"];
    setLanguage("en");

    return THOR_OK;
}

void CardiacAnnotatorTask::start(void *javaEnv, void *javaContext)
{
    stop();
    mStop = false;
    AutolabelJNI *jniInterface = new AutolabelJNI((JNIEnv*)javaEnv, (jobject)javaContext);
    mJNI = static_cast<void*>(jniInterface);
    auto l = lock(mMutex);
    mCB->registerCallback(this);
    mWorkerThread = std::thread(&CardiacAnnotatorTask::workerThreadFunc, this, (void*)mJNI);
}

void CardiacAnnotatorTask::stop()
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

void CardiacAnnotatorTask::setView(CardiacView view)
{
    mImguiRenderer.setA4C(view == CardiacView::A4C);
}

void CardiacAnnotatorTask::setPause(bool pause, AutoDopplerTarget target)
{
    LOGD("%s(%s)", __func__, pause ? "true":"false");
    auto l = lock(mMutex);

    mPaused = pause;
    mTarget = target;
    mDopplerGateBuffer.clear();
    mDopGateCoord = vec2(-1.0f, -1.0f);
    mCV.notify_one();
}


void CardiacAnnotatorTask::setFlip(float flipX, float flipY)
{
    auto l = lock(mMutex);
    mFlipX = flipX;
    mScanConverter->setFlip(flipX, flipY);
}

void* CardiacAnnotatorTask::setAutoDopplerCallback(void* user, /* nullable */ AutoDopplerCallback callback)
{
    void* prevUser = mAutoDopplerCallbackUser;
    mAutoDopplerCallback = callback;
    mAutoDopplerCallbackUser = user;
    return prevUser;
}
void CardiacAnnotatorTask::setMockAutoDoppler(bool mock) {
    mMockAutoDoppler = mock;
}
ThorStatus CardiacAnnotatorTask::onInit()
{

    LOGD("%s", __func__);
    auto l = lock(mMutex);
    mProcessedFrame = 0xFFFFFFFFu;
    mCineFrame = 0xFFFFFFFFu;

    // Clear temporal buffer
    mYoloBuffer.clear();
    mTDIYoloBuffer.clear();
    mShouldClear = true;

    std::string path = mUSManager->getAppPath();
    if (path.back() != '/')
        path.push_back('/');
    path.append("autolabel_develop.txt");
    FILE *f = fopen(path.c_str(), "r");
    if (f) {
        LOGD("Found %s, enabling develop mode", path.c_str());
        mIsDevelopMode = true;
        fclose(f);
    } else {
        mIsDevelopMode = false;
    }

    // DO NOT reset buffer data, since we can have an onTerminate/onInit pair without clearing the cinebuffer data!
    return THOR_OK;
}
void CardiacAnnotatorTask::onTerminate()
{
    //setPause(true);
}
void CardiacAnnotatorTask::setParams(CineBuffer::CineParams& params)
{
    LOGD("setParams, dauDataTypes = 0x%08x", params.dauDataTypes);

    auto l = lock(mMutex);
    mScanConverter->setConversionParameters(
            CARDIAC_ANNOTATOR_INPUT_WIDTH,
            CARDIAC_ANNOTATOR_INPUT_HEIGHT,
            mFlipX,
            1.0f,
            ScanConverterCL::ScaleMode::NON_UNIFORM_SCALE_TO_FIT,
            params.converterParams);
}

void CardiacAnnotatorTask::onData(uint32_t frameNum, uint32_t dauDataTypes)
{
    // Only activate in B mode
    if (BF_GET(dauDataTypes, DAU_DATA_TYPE_B, 1))
    {
        {
            auto l = lock(mMutex);
            mCineFrame = frameNum;
            if (frameNum == 0) {
                // On first frame, reset the processed frame count to -1 (no processed frames)
                mProcessedFrame = 0xFFFFFFFFu;
            }
            if (mPaused)
            {
                // If paused, overwrite old data with empty data
                mUnsmoothPredictionBuffer[frameNum % CINE_FRAME_COUNT].clear();
                mPredictionBuffer[frameNum % CINE_FRAME_COUNT].clear();
                // TODO: set view to noise/uncertain here (use config to tell which is which)
                mViewCineBuffer[frameNum % CINE_FRAME_COUNT] = std::make_pair(0,0.0f);
            }

        }
        mCV.notify_one();
    }
}

bool CardiacAnnotatorTask::workerShouldAwaken()
{
    if (mStop)
        return true;
    if (!mPaused && mCineFrame != mProcessedFrame)
        return true;
    return false;
}

void CardiacAnnotatorTask::workerThreadFunc(void* jniInterface)
{
    LOGD("Starting workerThreadFunc");
    AutolabelJNI *jni = (AutolabelJNI*)jniInterface;
    jni->attachToThread();

    // This is safe because we will only ever have the one worker thread
    if (!mInit)
    {
        mAnnotator.init(*mUSManager);
        mTDIAnnotator.init(*mUSManager);
        mInit = true;
    }

    uint32_t prevFrameNum = 0xFFFFFFFFu;
    uint32_t frameNum = 0xFFFFFFFFu;
    float flipX;
    AutoDopplerTarget target;
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
                break;

            // Only ever access the java object here on the worker thread
            if (mShouldClear) {
                jni->jenv->CallVoidMethod(jni->instance, jni->clear);
                mShouldClear = false;
            }

            if (mProcessedFrame == 0xFFFFFFFFu)
                prevFrameNum = 0xFFFFFFFFu;
            else
                prevFrameNum = frameNum;
            frameNum = mCineFrame;
            flipX = mFlipX;
            target = mTarget;
        }

        // Record frames that were skipped
        if (prevFrameNum != 0xFFFFFFFFu) {
            for (uint32_t frame = prevFrameNum+1; frame != frameNum; ++frame) {
                mFrameWasSkipped[frame % CINE_FRAME_COUNT] = 1;
            }
        }

        // Process the frame (without holding the lock)
        ThorStatus status = processFrame(frameNum, flipX, (void*)jni, target);
        if (IS_THOR_ERROR(status))
        {
            LOGE("Error processing frame %u: %08x", frameNum, status);
        }
    }
    delete jni;
}

void CardiacAnnotatorTask::convertToPhysicalSpace()
{
    auto transform = mScanConverter->getPixelToPhysicalTransform();

    for(auto &pred : mPredictions)
    {
        auto physicalPoint = transform * vec2(pred.x, pred.y); // Matrix3 * vec2 assumes homogeneous coords (implicit z=1)
        auto physicalDim = transform * vec3(pred.w, pred.h, 0.0f); // No translation on dimensions
        pred.x = physicalPoint.x;
        pred.y = physicalPoint.y;
        pred.w = physicalDim.x;
        pred.h = physicalDim.y;
        pred.normx = pred.x/CARDIAC_ANNOTATOR_INPUT_WIDTH;
        pred.normy = pred.y/CARDIAC_ANNOTATOR_INPUT_HEIGHT;
        pred.normw = pred.w/CARDIAC_ANNOTATOR_INPUT_WIDTH;
        pred.normh = pred.h/CARDIAC_ANNOTATOR_INPUT_WIDTH;
    }
}

void CardiacAnnotatorTask::convertToPhysicalSpacePW()
{
    auto transform = mScanConverter->getPixelToPhysicalTransform();

    for(auto &pred : mPWPredictions)
    {
        auto physicalPoint = transform * vec2(pred.x, pred.y); // Matrix3 * vec2 assumes homogeneous coords (implicit z=1)
        auto physicalDim = transform * vec3(pred.w, pred.h, 0.0f); // No translation on dimensions
        pred.x = physicalPoint.x;
        pred.y = physicalPoint.y;
        pred.w = physicalDim.x;
        pred.h = physicalDim.y;
    }
}

void CardiacAnnotatorTask::convertToPhysicalSpaceTDI()
{
    auto transform = mScanConverter->getPixelToPhysicalTransform();

    for(auto &pred : mTDIPredictions)
    {
        auto physicalPoint = transform * vec2(pred.x, pred.y); // Matrix3 * vec2 assumes homogeneous coords (implicit z=1)
        auto physicalDim = transform * vec3(pred.w, pred.h, 0.0f); // No translation on dimensions
        pred.x = physicalPoint.x;
        pred.y = physicalPoint.y;
        pred.w = physicalDim.x;
        pred.h = physicalDim.y;
    }
}

const std::vector<YoloPrediction>& CardiacAnnotatorTask::getPredictions(uint32_t frameNum) const
{
    return mPredictionBuffer[frameNum % CINE_FRAME_COUNT];
}

std::pair<int, float> CardiacAnnotatorTask::getView(uint32_t frameNum) const
{
    return mViewCineBuffer[frameNum % CINE_FRAME_COUNT];
}

bool CardiacAnnotatorTask::isTDIModeTarget(AutoDopplerTarget target) const {
    switch (target) {
        case AutoDopplerTarget::None:         return false;
        case AutoDopplerTarget::PLAX_RVOT_PV: return false;
        case AutoDopplerTarget::PLAX_AV_PV:   return false;
        case AutoDopplerTarget::A4C_MV:       return false;
        case AutoDopplerTarget::A4C_TV:       return false;
        case AutoDopplerTarget::A5C_LVOT:     return false;
        case AutoDopplerTarget::TDI_A4C_MVSA: return true;
        case AutoDopplerTarget::TDI_A4C_MVLA: return true;
        case AutoDopplerTarget::TDI_A4C_TVLA: return true;
    }
}

int CardiacAnnotatorTask::autoDopplerTargetIndex(AutoDopplerTarget target) const {
    const auto findName = [](const char *name) {
        auto it = std::find(
                std::begin(CARDIAC_ANNOTATOR_LABEL_NAMES_WITH_VIEW),
                std::end(CARDIAC_ANNOTATOR_LABEL_NAMES_WITH_VIEW),
                name);
        if (it == std::end(CARDIAC_ANNOTATOR_LABEL_NAMES_WITH_VIEW)) { return -1; }
        return static_cast<int>(std::distance(std::begin(CARDIAC_ANNOTATOR_LABEL_NAMES_WITH_VIEW), it));
    };

    const auto findNameTDI = [](const char *name) {
        auto it = std::find(
                std::begin(TDI_ANNOTATOR_LABEL_NAMES_WITH_VIEW),
                std::end(TDI_ANNOTATOR_LABEL_NAMES_WITH_VIEW),
                name);
        if (it == std::end(TDI_ANNOTATOR_LABEL_NAMES_WITH_VIEW)) { return -1; }
        return static_cast<int>(std::distance(std::begin(TDI_ANNOTATOR_LABEL_NAMES_WITH_VIEW), it));
    };

    switch (target) {
        case AutoDopplerTarget::None: return -1;
        case AutoDopplerTarget::PLAX_RVOT_PV: return findName("PV");
        case AutoDopplerTarget::PLAX_AV_PV: return findName("PV");
        case AutoDopplerTarget::A4C_MV: return findName("MV");
        case AutoDopplerTarget::A4C_TV: return findName("TV");
        case AutoDopplerTarget::A5C_LVOT: return findName("LVOT");
        case AutoDopplerTarget::TDI_A4C_MVSA: return findNameTDI("MVSA");
        case AutoDopplerTarget::TDI_A4C_MVLA: return findNameTDI("MVLA");
        case AutoDopplerTarget::TDI_A4C_TVLA: return findNameTDI("TVLA");
    }
}


ThorStatus CardiacAnnotatorTask::processFrame(uint32_t frameNum, float flipX, void *jni, AutoDopplerTarget target)
{
    ThorStatus status;
    AutolabelJNI *autolabel = (AutolabelJNI*)jni;

    if (frameNum % FRAME_DIVIDER == 0)
    {
        // Scan convert image
        uint8_t *prescan = mCB->getFrame(frameNum, DAU_DATA_TYPE_B);
        status = mScanConverter->runCLTex(prescan, autolabel->imageBufferNative.data());
        if (IS_THOR_ERROR(status))
            return status;
        if(mTarget == AutoDopplerTarget::None) {
            jint numLabels = autolabel->jenv->CallIntMethod(
                    autolabel->instance, autolabel->process,
                    autolabel->imageBuffer, autolabel->labelBuffer);
            mPredictions.clear();
            for (jint i = 0; i != numLabels; ++i) {
                Label *label = &autolabel->labelBufferNative[i];
                mPredictions.push_back(YoloPrediction{
                        label->x,
                        label->y,
                        label->w,
                        label->h,
                        0, 0, 0, 0,
                        label->conf,
                        1.0f,
                        label->id
                });
            }
            convertToPhysicalSpace();
            mUnsmoothPredictionBuffer[frameNum % CINE_FRAME_COUNT] = mPredictions;
        }

        else if (!isTDIModeTarget(mTarget)) {
            mPWPredictions.clear();

            std::vector<float> view;
            jint numLabels = autolabel->jenv->CallIntMethod(
                    autolabel->instance, autolabel->processPW,
                    autolabel->imageBuffer, autolabel->labelBuffer);
            for (jint i = 0; i != numLabels; ++i) {
                Label *label = &autolabel->labelBufferNative[i];
                mPWPredictions.push_back(PWYoloPrediction{
                        label->x,
                        label->y,
                        label->w,
                        label->h,
                        label->conf,
                        label->id
                });
            }
            if (IS_THOR_ERROR(status))
                return status;

            convertToPhysicalSpacePW();
        } else {
            mTDIPredictions.clear();
            jint numLabels = autolabel->jenv->CallIntMethod(
                    autolabel->instance, autolabel->processTDI,
                    autolabel->imageBuffer, autolabel->labelBuffer);
            for (jint i = 0; i != numLabels; ++i) {
                Label *label = &autolabel->labelBufferNative[i];
                mTDIPredictions.push_back(TDIYoloPrediction{
                        label->x,
                        label->y,
                        label->w,
                        label->h,
                        label->conf,
                        label->id
                });
            }
            if (IS_THOR_ERROR(status))
                return status;
            convertToPhysicalSpaceTDI();
        }

    }

    // Set value in prediction cinebuffer
#ifndef MOCK_AUTOLABEL
    const auto& preds = mPredictions;
#else
    const auto& preds = MOCK_AUTOLABEL(frameNum);
#endif
    mFrameWasSkipped[frameNum % CINE_FRAME_COUNT] = 0;
    auto& yoloBufferItem = mPredictionBuffer[frameNum % CINE_FRAME_COUNT];
    // I'm not certain if vector copy assign operator is guaranteed to reuse existing
    // vector memory. We want to make sure the vector memory is reusued, so instead of
    // copy assign I am clearing the old and copying over the new elements.
    //
    // The goal is that in the steady/long term state, there is no memory allocation happening,
    // since all the vectors have a maximum size larger than the total number of predictions for
    // the frame.
    //
    // Possible future optimization: use a single vector for all predictions, and maintain some way
    //  of quickly finding the predictions for a frame, and for replacing the predictions for a frame
    yoloBufferItem.clear();
    yoloBufferItem.resize(preds.size());
    std::copy(preds.begin(), preds.end(), yoloBufferItem.begin());

    auto& viewBufferItem = mViewCineBuffer[frameNum % CINE_FRAME_COUNT];
    viewBufferItem = mYoloBuffer.getView();

    if (AutoDopplerTarget::None != mTarget) {
        status = processAutoDopplerGate(frameNum);
        if (IS_THOR_ERROR(status)) { return status; }
    }

    return THOR_OK;
}

ThorStatus CardiacAnnotatorTask::processAutoDopplerGate(uint32_t frameNum) {
    // Doppler Gate Placement Logic
    DopplerGateBox currentObj = {};
    bool found = false;

    if (mMockAutoDoppler) {
        std::pair<float, float> gate = MockDopplerGate(frameNum);
        if (gate.second > 0.0f) {
            mDopGateCoord.x = gate.first;
            mDopGateCoord.y = gate.second;
            // Send callback here
            if (nullptr != mAutoDopplerCallback) {
                LOGE("===> mAutoDopplerCallback found", mAutoDopplerCallback, gate.first);
                mAutoDopplerCallback(mAutoDopplerCallbackUser, mDopGateCoord.x, mDopGateCoord.y);
                setPause(true);
            }
            return THOR_OK;
        }
    }

    if (!isTDIModeTarget(mTarget))
    {
        for (const PWYoloPrediction &pred : mPWPredictions)
        {
            if (pred.label == autoDopplerTargetIndex(mTarget)) {
                found = true;
                currentObj = {pred.x, pred.y, pred.w, pred.h};
            }
        }
    } else {
        for (const TDIYoloPrediction &pred : mTDIPredictions) {
            if (pred.label == autoDopplerTargetIndex(mTarget)) {
                found = true;
                currentObj = {pred.x, pred.y, pred.w, pred.h};
            }
        }
    }

    if (found)
    {
        // found the target object
        if (mDopplerGateBuffer.size() < DOP_BUFFER_LENGTH)
        {
            // add the current object values
            mDopplerGateBuffer.push_back(currentObj);
        }
        else
        {
            vec2 min = {10000.0f, 10000.0f};
            vec2 max = {-10000.0f, -10000.0f};

            for (auto it = mDopplerGateBuffer.begin(); it != mDopplerGateBuffer.end(); ++it)
            {
                // convert x/y from center to box origin
                float x = it->x - 0.5f*it->w;
                float y = it->y - 0.5f*it->h;
                // track overall AABB coords
                if (x < min.x) { min.x = x; }
                if (y < min.y) { min.y = y; }
                if (x+it->w > max.x) { max.x = x+it->w; }
                if (y+it->h > max.y) { max.y = y+it->h; }
            }

            // calculate Doppler Gate Coordinate
            vec2 anchor = autoDopplerAnchorPoint(mTarget);
            vec2 extent = max-min;
            vec2 delta = {extent.x * anchor.x, extent.y * anchor.y};
            mDopGateCoord = min + delta;
            // Send callback here
            if (nullptr != mAutoDopplerCallback) {
                mAutoDopplerCallback(mAutoDopplerCallbackUser, mDopGateCoord.x, mDopGateCoord.y);
                setPause(true);
            }
        }
    }
    else
    {
        // target object not in the list
        mDopplerGateBuffer.clear();
        mDopGateCoord.x = -1.0f;
        mDopGateCoord.y = -1.0f;
    }
    return THOR_OK;
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

void CardiacAnnotatorTask::setLanguage(const char *lang) {
    const Json::Value *locale = mLocaleMap.find(lang, lang+strlen(lang));
    if (!locale) {
        const char *fallback = "en";
        LOGE("Failed to find language \"%s\" in locale map, falling back to \"%s\"", lang, fallback);
        locale = mLocaleMap.find(fallback, fallback + strlen(fallback));
        if (!locale) {
            LOGE("Failed to find fallback language \"%s\"", fallback);
        }
    }
    mCurrentLocale = locale;
}
void CardiacAnnotatorTask::addTextRanges(ImFontGlyphRangesBuilder &builder) {
    for (const Json::Value &locale : mLocaleMap) {
        for (const Json::Value &text : locale) {
            LOGD("Adding text \"%s\" to font", text.asCString());
            builder.AddText(text.asCString());
        }
    }
}
const char *CardiacAnnotatorTask::getLocalizedString(const char *id) const {
    if (mCurrentLocale == nullptr) {
        LOGE("No current locale set!");
        return id;
    }
    const Json::Value *localized = mCurrentLocale->find(id, id+strlen(id));
    if (localized == nullptr) {
        LOGE("Could not find string \"%s\" in current locale", id);
        return id;
    }
    return localized->asCString();
}
void CardiacAnnotatorTask::draw(CineViewer &cineviewer, uint32_t frameNum) {

    // Do not draw if we have no processed frames, or if autolabel is paused
    if (mProcessedFrame == 0xFFFFFFFFu || mPaused)
        return;
    // Do not draw if we are looking for auto-doppler gate
    if (mTarget != AutoDopplerTarget::None)
        return;
    // Get conversion matrix to screen space coords
    float physToScreen[9];
    cineviewer.queryPhysicalToPixelMap(physToScreen, 9);

    ImU32 labelColor = ImColor(1.0f, 1.0f, 0.0f);

    // If we are live, we need to display the latest frame, not frameNum
    const auto& preds = mPredictionBuffer[mProcessedFrame % CINE_FRAME_COUNT];
    for (auto &p : preds)
    {
        const char *label = getLocalizedString(CARDIAC_ANNOTATOR_LABEL_NAMES[p.class_pred]);

        float posPhys[3] = {p.x, p.y, 1.0f};
        float posScreen[3];
        ViewMatrix3x3::multiplyMV(posScreen, physToScreen, posPhys);

        auto *drawList = ImGui::GetBackgroundDrawList();
        drawList->AddCircleFilled(ImVec2(posScreen[0], posScreen[1]), 7.0f, labelColor);
        drawList->AddText(ImVec2(posScreen[0]+20.0f, posScreen[1]-18.0f), labelColor, label);
    }

    if (mIsDevelopMode) {
        //    ImGui::SetNextWindowPos(ImVec2(20,20));
        //    ImGui::SetNextWindowSize(ImVec2(500,-1));
        //    if (ImGui::Begin("Autolabel view", NULL, ImGuiWindowFlags_NoDecoration))
        //    {
        //        if (mViewProb.first <= CARDIAC_ANNOTATOR_CLASSIFIER_NUM_CLASSES)
        //            ImGui::Text("View: %s (%0.3f)", CARDIAC_ANNOTATOR_VIEW_NAMES[mViewProb.first], mViewProb.second);
        //        else
        //            ImGui::Text("View index: %d", mViewProb.first);
        //    }
        //    ImGui::End();
        ImGui::SetNextWindowSize(ImVec2(1000, -1));
        if (ImGui::Begin("Autolabel Config")) {
            float base_thr = CARDIAC_ANNOTATOR_PARAMS.base_label_threshold;
            float tweak_thrs[CARDIAC_ANNOTATOR_LABELER_NUM_CLASSES];
            std::copy(
                    std::begin(CARDIAC_ANNOTATOR_PARAMS.label_thresholds),
                    std::end(CARDIAC_ANNOTATOR_PARAMS.label_thresholds),
                    std::begin(tweak_thrs));
            float view_thr = CARDIAC_ANNOTATOR_PARAMS.view_threshold;
            float label_frac = CARDIAC_ANNOTATOR_PARAMS.label_buffer_frac;

            TweakFloat("Base Label Thr", base_thr);
            if (ImGui::CollapsingHeader("Per-label Thresholds")) {
                for (int label = 0; label != CARDIAC_ANNOTATOR_LABELER_NUM_CLASSES; ++label) {
                    tweak_thrs[label] += base_thr;
                    TweakFloat(CARDIAC_ANNOTATOR_LABEL_NAMES_WITH_VIEW[label], tweak_thrs[label]);
                    tweak_thrs[label] -= base_thr;
                }
            }
            TweakFloat("View Thr", view_thr);
            TweakFloat("Label Frac", label_frac);
            if (ImGui::Button("Reset to defaults")) {
                CARDIAC_ANNOTATOR_PARAMS.reset();
            } else {
                CARDIAC_ANNOTATOR_PARAMS.base_label_threshold = base_thr;
                std::copy(
                        std::begin(tweak_thrs), std::end(tweak_thrs),
                        std::begin(CARDIAC_ANNOTATOR_PARAMS.label_thresholds));
                CARDIAC_ANNOTATOR_PARAMS.view_threshold = view_thr;
                CARDIAC_ANNOTATOR_PARAMS.label_buffer_frac = label_frac;
            }

        }
        ImGui::End();
    }
}

void CardiacAnnotatorTask::close() {}

void CardiacAnnotatorTask::onSave(uint32_t startFrame, uint32_t endFrame,
                                  echonous::capnp::ThorClip::Builder &builder) {

    // OTS-2576: this seems to be reading bad data somewhere? But only when we capture via autocapture??
    return;


    auto autolabel = builder.initAutolabel();
    autolabel.setModelVersion("2022_05_23_AutoLabelV3-5.dlc");

    uint32_t numFrames = 0;
    for (uint32_t i=startFrame; i != endFrame+1; ++i) {
        if (mFrameWasSkipped[i % CINE_FRAME_COUNT] == 0)
            ++numFrames;
    }

    auto preds = autolabel.initPredictions(numFrames);
    uint32_t predIndex = 0;
    for (uint32_t i=startFrame; i != endFrame+1; ++i) {
        if (mFrameWasSkipped[i % CINE_FRAME_COUNT] == 1)
            continue;

        auto labeledFrame = preds[predIndex++];
        labeledFrame.setFrameNum(i-startFrame);
        int viewIndex = mViewCineBuffer[i % CINE_FRAME_COUNT].first;
        if (viewIndex < 0 || viewIndex >= 19) {
            LOGE("Found invalid viewIndex = %d", viewIndex);
            viewIndex = 19;
        } // OTHER

        labeledFrame.setViewIndex(viewIndex);
        labeledFrame.setViewName(CARDIAC_ANNOTATOR_VIEW_NAMES[viewIndex]);
        labeledFrame.setViewConf(mViewCineBuffer[i % CINE_FRAME_COUNT].second);

        const auto &realPreds = mUnsmoothPredictionBuffer[i % CINE_FRAME_COUNT];
        const auto &smoothPreds = mPredictionBuffer[i % CINE_FRAME_COUNT];

        auto realLabels = labeledFrame.initRealLabels(realPreds.size());
        auto pixelLabels = labeledFrame.initPixelLabels(realPreds.size());

        auto realLabelsSmooth = labeledFrame.initRealLabelsSmooth(smoothPreds.size());
        auto pixelLabelsSmooth = labeledFrame.initPixelLabelsSmooth(smoothPreds.size());

        for (uint32_t j=0; j != realPreds.size(); ++j) {
            auto capnpRealLabel = realLabels[j];
            const auto &memLabel = realPreds[j];
            capnpRealLabel.setX(memLabel.x);
            capnpRealLabel.setY(memLabel.y);
            capnpRealLabel.setW(memLabel.w);
            capnpRealLabel.setH(memLabel.h);
            capnpRealLabel.setLabelId(CARDIAC_ANNOTATOR_LABEL_NAMES_WITH_VIEW[memLabel.class_pred]);
            capnpRealLabel.setDisplayText(CARDIAC_ANNOTATOR_LABEL_NAMES[memLabel.class_pred]);
            capnpRealLabel.setConf(memLabel.obj_conf);

            auto capnpPixelLabel = pixelLabels[j];
            capnpPixelLabel.setX(memLabel.normx);
            capnpPixelLabel.setY(memLabel.normy);
            capnpPixelLabel.setW(memLabel.normw);
            capnpPixelLabel.setH(memLabel.normh);
            capnpPixelLabel.setLabelId(CARDIAC_ANNOTATOR_LABEL_NAMES_WITH_VIEW[memLabel.class_pred]);
            capnpPixelLabel.setDisplayText(CARDIAC_ANNOTATOR_LABEL_NAMES[memLabel.class_pred]);
            capnpPixelLabel.setConf(memLabel.obj_conf);
        }

        for (uint32_t j=0; j != smoothPreds.size(); ++j) {
            auto capnpRealLabel = realLabelsSmooth[j];
            const auto &memLabel = smoothPreds[j];
            capnpRealLabel.setX(memLabel.x);
            capnpRealLabel.setY(memLabel.y);
            capnpRealLabel.setW(memLabel.w);
            capnpRealLabel.setH(memLabel.h);
            capnpRealLabel.setLabelId(CARDIAC_ANNOTATOR_LABEL_NAMES_WITH_VIEW[memLabel.class_pred]);
            capnpRealLabel.setDisplayText(CARDIAC_ANNOTATOR_LABEL_NAMES[memLabel.class_pred]);
            capnpRealLabel.setConf(memLabel.obj_conf);

            auto capnpPixelLabel = pixelLabelsSmooth[j];
            capnpPixelLabel.setX(memLabel.normx);
            capnpPixelLabel.setY(memLabel.normy);
            capnpPixelLabel.setW(memLabel.normw);
            capnpPixelLabel.setH(memLabel.normh);
            capnpPixelLabel.setLabelId(CARDIAC_ANNOTATOR_LABEL_NAMES_WITH_VIEW[memLabel.class_pred]);
            capnpPixelLabel.setDisplayText(CARDIAC_ANNOTATOR_LABEL_NAMES[memLabel.class_pred]);
            capnpPixelLabel.setConf(memLabel.obj_conf);
        }
    }

}

void CardiacAnnotatorTask::onLoad(echonous::capnp::ThorClip::Reader &reader) {
    if (reader.hasAutolabel()) {
        auto autolabel = reader.getAutolabel();
        auto preds = autolabel.getPredictions();
        uint32_t totalLabels = 0;
        for (uint32_t frameNum=0; frameNum != preds.size(); ++frameNum) {
            auto capnpLabeledFrame = preds[frameNum];
            auto capnpLabels = capnpLabeledFrame.getRealLabelsSmooth();
            // This modulo should not be necessary, as the number of frames stored in the
            // thor file should not exceed the cinebuffer max size.
            // However, better to be cautious
            auto memLabeledFrame = mPredictionBuffer[capnpLabeledFrame.getFrameNum() % CINE_FRAME_COUNT];
            memLabeledFrame.resize(capnpLabels.size());
            totalLabels += capnpLabels.size();
            for (uint32_t labelIndex=0; labelIndex != capnpLabels.size(); ++labelIndex) {
                auto capnpLabel = capnpLabels[labelIndex];
                auto memLabel = memLabeledFrame[labelIndex];

                memLabel.x = capnpLabel.getX();
                memLabel.y = capnpLabel.getY();
                memLabel.w = capnpLabel.getW();
                memLabel.h = capnpLabel.getH();
                // ignore norm values for now?

                auto equals_label_id = [&](const char* str){
                    return 0 == strcmp(str, capnpLabel.getLabelId().cStr());
                };

                auto iter = std::find_if(
                    std::begin(CARDIAC_ANNOTATOR_LABEL_NAMES_WITH_VIEW),
                    std::end(CARDIAC_ANNOTATOR_LABEL_NAMES_WITH_VIEW),
                    equals_label_id);
                if (iter == std::end(CARDIAC_ANNOTATOR_LABEL_NAMES_WITH_VIEW)) {
                    LOGE("Unknown label id: %s", capnpLabel.getLabelId().cStr());
                    // TODO Need a better alternative!
                    memLabel.class_pred = 0;
                } else {
                    memLabel.class_pred = std::distance(
                            std::begin(CARDIAC_ANNOTATOR_LABEL_NAMES_WITH_VIEW), iter);
                }

                memLabel.obj_conf = capnpLabel.getConf();
            }
        }
        LOGD("Loaded %u frames of autolabel data (%u total labels)", preds.size(), totalLabels);
    }
}

ThorStatus CardiacAnnotatorTask::runVerificationOnCineBuffer(JNIEnv *jenv, const char *inputFile, const char *outputRoot, int workflow) {
    // Assume all autolabel clips have -1 for flipX
    setFlip(-1.0f, 1.0f);
    LOGD("[ VERIFY ] Running Verification on %s", inputFile);
    return runVerificationClip(inputFile, outputRoot, jenv, workflow);
}
template <class T> void SaveRaw(const std::vector<T> &data, const char *outdir, const char *fmt)
{
    char outpath[1024];
    snprintf(outpath, sizeof(outpath), fmt, outdir);
    LOGD("[ VERIFY ] Saving: %s", outpath);
    std::ofstream os(outpath);
    os.write((const char*)data.data(), data.size()*sizeof(T));
}

static Json::Value FloatArrayToJson(const float *arr, size_t len) {
    Json::Value json;
    for (size_t i=0; i != len; ++i) {
        json.append(arr[i]);
    }
    return json;
}

static Json::Value LabelArrayToJson(const Label *labels, size_t len) {
    Json::Value jsonList(Json::ValueType::arrayValue);
    for (size_t i=0; i != len; ++i) {
        Json::Value jsonLabel;
        jsonLabel["class_pred"] = labels[i].id;
        jsonLabel["conf"] = labels[i].conf;
        jsonLabel["x"] = labels[i].x;
        jsonLabel["y"] = labels[i].y;
        jsonLabel["w"] = labels[i].w;
        jsonLabel["h"] = labels[i].h;
        jsonList.append(jsonLabel);
    }
    return jsonList;
}

ThorStatus CardiacAnnotatorTask::verifyFrameAutoLabelMode(uint32_t frameNum, AutoLabelVerificationResult &result, JNIEnv *jenv)
{
    ThorStatus status = THOR_OK;

    AutolabelJNI* autolabel = static_cast<AutolabelJNI*>(mJNI);
    // Scan convert image
    uint8_t *prescan = mCB->getFrame(frameNum, DAU_DATA_TYPE_B);
    status = mScanConverter->runCLTex(prescan, autolabel->imageBufferNative.data());

    // process
    jenv->CallVoidMethod(
            autolabel->instance, autolabel->processWithIntermediates,
            autolabel->imageBuffer, autolabel->fullResultsBuffer);

    // Append predicted view and labels to json
    Json::Value frame;
    frame["viewPred"] = FloatArrayToJson(autolabel->fullResults.view, CARDIAC_ANNOTATOR_CLASSIFIER_NUM_CLASSES);
    frame["predictions"] = LabelArrayToJson(autolabel->fullResults.labels, autolabel->fullResults.numLabels);
    frame["smoothView"] = FloatArrayToJson(autolabel->fullResults.smoothView, CARDIAC_ANNOTATOR_CLASSIFIER_NUM_CLASSES);
    frame["smoothed"] = LabelArrayToJson(autolabel->fullResults.smoothLabels, autolabel->fullResults.numSmoothLabels);
    result.json["frames"].append(frame);
    return THOR_OK;
}
void CardiacAnnotatorTask::clearVerificationDataStructures(JNIEnv *jenv) {
    AutolabelJNI* autolabel = static_cast<AutolabelJNI*>(mJNI);
    memset(autolabel->fullResults.view, 0, sizeof(float) * CARDIAC_ANNOTATOR_CLASSIFIER_NUM_CLASSES);
    memset(autolabel->fullResults.smoothView, 0, sizeof(float) * CARDIAC_ANNOTATOR_CLASSIFIER_NUM_CLASSES);
    memset(autolabel->fullResults.yoloProc, 0, sizeof(Prediction) * PREDS_PER_FRAME);
    memset(autolabel->fullResults.yoloRaw, 0, sizeof(Prediction) * PREDS_PER_FRAME);
    memset(autolabel->fullResults.labels, 0, sizeof(Label) * CARDIAC_ANNOTATOR_CLASSIFIER_NUM_CLASSES);
    memset(autolabel->fullResults.smoothLabels, 0, sizeof(Label) * CARDIAC_ANNOTATOR_CLASSIFIER_NUM_CLASSES);
    mPredictions.clear();
    mTDIPredictions.clear();
    mPWPredictions.clear();
    mYoloBuffer.clear();
    mTDIYoloBuffer.clear();
    mPWYoloBuffer.clear();
    mUnsmoothPredictionBuffer.clear();
    mPredictionBuffer.clear();
    mTDIView.clear();
    mPWView.clear();
    mViewCineBuffer.clear();
}
ThorStatus CardiacAnnotatorTask::runVerificationClip(const char *thor, const char *outdir, JNIEnv *jenv, int workflow) {
    LOGD("Running TDI verification on %s", thor);
    setFlip(-1.0f, 1.0f);
    ThorStatus status = THOR_OK;
    clearVerificationDataStructures(jenv);
    // Autolabel verification
    std::string autolabelOutPath = outdir;
    auto mdirResult = ::mkdir(autolabelOutPath.c_str(), S_IRWXU | S_IRGRP | S_IROTH);

    if (mdirResult != 0 && errno != EEXIST) {
        // the directory could not be made and does not exist, return error
        return THOR_ERROR;
    }

    switch(workflow){
        case 0: {
            status = runAutoLabelVerification(outdir, thor, jenv);
            if (IS_THOR_ERROR(status)) {
                LOGE("Error 0x%08x running on frame", status);
                return status;
            }
            break;
        }
        case 1: {
            status = runPWVerification(outdir, thor, jenv);
            if (IS_THOR_ERROR(status)) {
                LOGE("Error 0x%08x running", status);
                return status;
            }
            break;
        }
        case 2: {
            status = runTDIVerification(outdir, thor, jenv);
            if (IS_THOR_ERROR(status)) {
                LOGE("Error 0x%08x running on", status);
                return status;
            }
            break;
        }
        default:
            LOGW("Warning, no case selected for verification");
            break;
    }

    return THOR_OK;
}

vec2 CardiacAnnotatorTask::autoDopplerAnchorPoint(AutoDopplerTarget target) const {
    switch (target) {
        case AutoDopplerTarget::None:         return {0,0};
        case AutoDopplerTarget::PLAX_RVOT_PV: return {0.25f, 0.0f};
        case AutoDopplerTarget::PLAX_AV_PV:   return {0.25f, 0.0f};
        case AutoDopplerTarget::A4C_MV:       return {0.5f, 0.0f};
        case AutoDopplerTarget::A4C_TV:       return {0.5f, 0.0f};
        case AutoDopplerTarget::A5C_LVOT:     return {0.5f, 0.5f};
        case AutoDopplerTarget::TDI_A4C_MVSA: return {0.5f, 0.5f};
        case AutoDopplerTarget::TDI_A4C_MVLA: return {0.5f, 0.5f};
        case AutoDopplerTarget::TDI_A4C_TVLA: return {0.5f, 0.5f};
    }
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

ThorStatus CardiacAnnotatorTask::runAutoLabelVerification(const char* outdir, const char* thor, JNIEnv* jni) {
    ThorStatus status;
    std::string autolabelOutPath = outdir;
    autolabelOutPath.append("/autolabel");
    ::mkdir(autolabelOutPath.c_str(), S_IRWXU | S_IRGRP | S_IROTH);

    AutoLabelVerificationResult result;
    AutolabelJNI *autolabel = (AutolabelJNI*)mJNI;
    jni->CallVoidMethod(
            autolabel->instance, autolabel->clear);
    uint32_t maxFrame = mUSManager->getCineBuffer().getMaxFrameNum()+1;
    for (uint32_t frameNum=0; frameNum != maxFrame; ++frameNum) {
        status = verifyFrameAutoLabelMode(frameNum, result, jni);
        if (IS_THOR_ERROR(status)) {
            LOGE("Error 0x%08x running on frame %u", status, frameNum);
            return status;
        }
    }

    // Save results
    result.json["file"] = thor;
    const char *id_begin = strrchr(thor, '/')+1;
    const char *id_end = strrchr(thor, '.');
    char outpath[512];
    snprintf(outpath, sizeof(outpath), "%s/%.*s.json",
             autolabelOutPath.c_str(),
             static_cast<int>(id_end - id_begin), id_begin);
    std::ofstream jsonOut(outpath);
    jsonOut << result.json << "\n";

    SaveRaw(result.postscan, autolabelOutPath.c_str(), id_begin, id_end, "%s/%.*s_postscan.raw");
    SaveRaw(result.viewRaw, autolabelOutPath.c_str(), id_begin, id_end, "%s/%.*s_viewraw.raw");
    SaveRaw(result.yoloRaw, autolabelOutPath.c_str(), id_begin, id_end, "%s/%.*s_yoloraw.raw");
    SaveRaw(result.yoloProc, autolabelOutPath.c_str(), id_begin, id_end, "%s/%.*s_yoloproc.raw");
    return THOR_OK;
}

ThorStatus CardiacAnnotatorTask::runPWVerification(const char* outdir, const char* thor, JNIEnv* jni) {
    // PW verification
    ThorStatus status = THOR_OK;
    mPWYoloBuffer.clear();
    AutolabelJNI *autodoppler = (AutolabelJNI*)mJNI;
    jni->CallVoidMethod(
            autodoppler->instance, autodoppler->clear);
    mPWView.resize(PW_ANNOTATOR_CLASSIFIER_NUM_CLASSES);
    std::string autolabelOutPath = outdir;
    autolabelOutPath.append("/PW");
    ::mkdir(autolabelOutPath.c_str(),

    S_IRWXU | S_IRGRP | S_IROTH);

    PWVerificationResult result;

    uint32_t maxFrame = mUSManager->getCineBuffer().getMaxFrameNum() + 1;
    for (uint32_t frameNum = 0; frameNum != maxFrame;++frameNum) {
        status = verifyFramePWMode(frameNum, result, jni);
        if (IS_THOR_ERROR(status)) {
        LOGE("Error 0x%08x running on frame %u", status, frameNum);
        return
        status;
    }
    }


    // Save results
    result.json["file"] = thor;
    const char *id_begin = strrchr(thor, '/') + 1;
    const char *id_end = strrchr(thor, '.');
    char outpath[512];
    snprintf(outpath, sizeof(outpath), "%s/%.*s.json",
             autolabelOutPath.c_str(),
             static_cast<int>(id_end - id_begin), id_begin);
    std::ofstream jsonOut(outpath);
    jsonOut << result.json << "\n";

    SaveRaw(result.postscan, autolabelOutPath.c_str(), id_begin, id_end, "%s/%.*s_postscan.raw");
    SaveRaw(result.viewRaw, autolabelOutPath.c_str(), id_begin, id_end, "%s/%.*s_viewraw.raw");
    SaveRaw(result.yoloRaw, autolabelOutPath.c_str(), id_begin, id_end, "%s/%.*s_yoloraw.raw");
    SaveRaw(result.yoloProc, autolabelOutPath.c_str(), id_begin, id_end, "%s/%.*s_yoloproc.raw");
    return THOR_OK;
}

void pushPrediction(std::vector<float>& buffer, Prediction p, int numLblConf)
{
    buffer.push_back(p.x);
    buffer.push_back(p.y);
    buffer.push_back(p.w);
    buffer.push_back(p.h);
    buffer.push_back(p.objConf);
    for(int i = 0; i < numLblConf; ++i) {
        buffer.push_back(p.lblConf[i]);
    }
}
ThorStatus CardiacAnnotatorTask::runTDIVerification(const char* outdir, const char* thor, JNIEnv* jni)
{
    ThorStatus status = THOR_OK;
    TDIVerificationResult result;
    mTDIView.resize(TDI_ANNOTATOR_CLASSIFIER_NUM_CLASSES);
    AutolabelJNI *autodoppler = (AutolabelJNI*)mJNI;
    jni->CallVoidMethod(
            autodoppler->instance, autodoppler->clear);
    uint32_t maxFrame = mUSManager->getCineBuffer().getMaxFrameNum()+1;
    for (uint32_t frameNum=0; frameNum != maxFrame; ++frameNum) {
        LOGD("[ VERIFY ] Running verification on frame %d", frameNum);
        status = verifyFrameTDIMode(frameNum, result, jni);
        if (IS_THOR_ERROR(status)) {
            LOGE("Error 0x%08x running on frame %u", status, frameNum);
            return status;
        }
    }

    std::string TDIOutPath = outdir;
    TDIOutPath.append("/TDI");

    ::mkdir(TDIOutPath.c_str(), S_IRWXU | S_IRGRP | S_IROTH);


    // Save results
    result.json["file"] = thor;
    const char *id_begin = strrchr(thor, '/')+1;
    const char *id_end = strrchr(thor, '.');

    char outpath[512];
    snprintf(outpath, sizeof(outpath), "%s/%.*s.json",
             TDIOutPath.c_str(),
             static_cast<int>(id_end - id_begin), id_begin);
    LOGD("[ VERIFY ] Saving out %s to %s",id_begin, outpath);
    std::ofstream jsonOut(outpath);
    jsonOut << result.json << "\n";
    LOGD("[ VERIFY ] Saving out raw images");
    SaveRaw(result.postscan, TDIOutPath.c_str(), id_begin, id_end, "%s/%.*s_postscan.raw");
    SaveRaw(result.viewRaw, TDIOutPath.c_str(), id_begin, id_end, "%s/%.*s_viewraw.raw");
    SaveRaw(result.yoloRaw, TDIOutPath.c_str(), id_begin, id_end, "%s/%.*s_yoloraw.raw");
    SaveRaw(result.yoloProc, TDIOutPath.c_str(), id_begin, id_end, "%s/%.*s_yoloproc.raw");
    return status;
}

ThorStatus CardiacAnnotatorTask::verifyFramePWMode(uint32_t frameNum, PWVerificationResult &result, JNIEnv* jni)
{
    ThorStatus status = THOR_OK;
    AutolabelJNI *autodoppler = (AutolabelJNI*)mJNI;
    uint8_t *prescan = mCB->getFrame(frameNum, DAU_DATA_TYPE_B);
    status = mScanConverter->runCLTex(prescan, autodoppler->imageBufferNative.data());
    std::copy(autodoppler->imageBufferNative.data(), autodoppler->imageBufferNative.data()+PW_ANNOTATOR_INPUT_HEIGHT*PW_ANNOTATOR_INPUT_WIDTH, std::back_inserter(result.postscan));
    mPWPredictions.clear();

    jni->CallVoidMethod(
            autodoppler->instance, autodoppler->processPWWithIntermediates,
            autodoppler->imageBuffer, autodoppler->fullResultsBuffer);


    if (IS_THOR_ERROR(status)) { return status; }
    Json::Value frame;
    frame["viewPred"] = FloatArrayToJson(autodoppler->fullResults.view, PW_ANNOTATOR_CLASSIFIER_NUM_CLASSES);
    frame["predictions"] = LabelArrayToJson(autodoppler->fullResults.labels, autodoppler->fullResults.numLabels);
    frame["smoothView"] = FloatArrayToJson(autodoppler->fullResults.smoothView, PW_ANNOTATOR_CLASSIFIER_NUM_CLASSES);
    frame["smoothed"] = LabelArrayToJson(autodoppler->fullResults.smoothLabels, autodoppler->fullResults.numSmoothLabels);
    result.json["frames"].append(frame);

    //yoloProc
    for(int i = 0; i < PW_ANNOTATOR_LABELER_NUM_ANCHORS * PW_ANNOTATOR_ANCHOR_INFO.fsize * PW_ANNOTATOR_ANCHOR_INFO.fsize; ++i){
        pushPrediction(result.yoloProc, autodoppler->fullResults.yoloProc[i], PW_ANNOTATOR_CLASSIFIER_NUM_CLASSES);
    }
    //yoloRaw
    for(int i = 0; i < PW_ANNOTATOR_LABELER_NUM_ANCHORS * PW_ANNOTATOR_ANCHOR_INFO.fsize * PW_ANNOTATOR_ANCHOR_INFO.fsize; ++i){
        pushPrediction(result.yoloRaw, autodoppler->fullResults.yoloRaw[i], PW_ANNOTATOR_CLASSIFIER_NUM_CLASSES);
    }
    for(int i = 0; i < 20; ++i){
        result.viewRaw.push_back(autodoppler->fullResults.view[i]);
    }
    return THOR_OK;
}

ThorStatus CardiacAnnotatorTask::verifyFrameTDIMode(uint32_t frameNum, TDIVerificationResult &result, JNIEnv* jni)
{
    ThorStatus status;
    AutolabelJNI *autodoppler = (AutolabelJNI*)mJNI;
    uint8_t *prescan = mCB->getFrame(frameNum, DAU_DATA_TYPE_B);
    status = mScanConverter->runCLTex(prescan, autodoppler->imageBufferNative.data());
    std::copy(autodoppler->imageBufferNative.data(), autodoppler->imageBufferNative.data()+PW_ANNOTATOR_INPUT_HEIGHT*PW_ANNOTATOR_INPUT_WIDTH, std::back_inserter(result.postscan));
    if (IS_THOR_ERROR(status)) { return status; }
    mTDIPredictions.clear();
    jni->CallVoidMethod(
            autodoppler->instance, autodoppler->processTDIWithIntermediates,
            autodoppler->imageBuffer, autodoppler->fullResultsBuffer);

    if (IS_THOR_ERROR(status))
        return status;

    // Append predicted view and labels to json
    Json::Value frame;
    frame["viewPred"] = FloatArrayToJson(autodoppler->fullResults.view, TDI_ANNOTATOR_CLASSIFIER_NUM_CLASSES);
    frame["predictions"] = LabelArrayToJson(autodoppler->fullResults.labels, autodoppler->fullResults.numLabels);
    frame["smoothView"] = FloatArrayToJson(autodoppler->fullResults.smoothView, TDI_ANNOTATOR_CLASSIFIER_NUM_CLASSES);
    frame["smoothed"] = LabelArrayToJson(autodoppler->fullResults.smoothLabels, autodoppler->fullResults.numSmoothLabels);
    result.json["frames"].append(frame);

    //yoloProc
    for(int i = 0; i < TDI_ANNOTATOR_LABELER_NUM_ANCHORS * TDI_ANNOTATOR_ANCHOR_INFO.fsize * TDI_ANNOTATOR_ANCHOR_INFO.fsize; ++i){
        LOGD("[ VERIFY TDI ] Pred - X: %.04f Y: %.04f W: %.04f H: %.04f \nOC: %.04f LC1: %.04f LC2: %.04f LC3: %.04f",
             autodoppler->fullResults.yoloProc[i].x, autodoppler->fullResults.yoloProc[i].y, autodoppler->fullResults.yoloProc[i].w, autodoppler->fullResults.yoloProc[i].h,
                     autodoppler->fullResults.yoloProc[i].objConf, autodoppler->fullResults.yoloProc[i].lblConf[0], autodoppler->fullResults.yoloProc[i].lblConf[1], autodoppler->fullResults.yoloProc[i].lblConf[2]);
        pushPrediction(result.yoloProc, autodoppler->fullResults.yoloProc[i], 3);
    }
    //yoloRaw
    for(int i = 0; i < TDI_ANNOTATOR_LABELER_NUM_ANCHORS * 14 * 14; ++i) {
        pushPrediction(result.yoloRaw, autodoppler->fullResults.yoloRaw[i], 3);
    }
    for(int i = 0; i < 20; ++i){
        result.viewRaw.push_back(autodoppler->fullResults.view[i]);
    }
    return THOR_OK;
}
