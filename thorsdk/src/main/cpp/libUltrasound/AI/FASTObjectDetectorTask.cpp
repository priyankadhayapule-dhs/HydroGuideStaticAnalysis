#define LOG_TAG "FASTObjectDetectorTask"

#include <algorithm>
#include <iterator>
#include <numeric>
#include <cmath>
#include <FASTObjectDetectorTask.h>
#include <AIManager.h>
#include <BitfieldMacros.h>
#include <imgui.h>
#include <jni.h>
#include <fstream>
#include <json/json.h>
#include <sys/stat.h>

const char *FAST_VIEW_FORMAT_STRING = "FAST Exam view: %s";
static const int TEMPORAL_SMOOTHING_FRAMES = 16;
static const int TEMPORAL_SMOOTHING_FRAMES_YOLO = 16;
static const int TEMPORAL_SMOOTHING_FRAMES_VIEW = 24;
static const int FRAME_DIVIDER = 2; // Run every N frames
using lock = std::unique_lock<std::mutex>;
// Things needed to call JNI interface
struct FastExamJNI {
    JavaVM *jvm;
    JNIEnv *jenv;
    jclass clazz;
    jobject context;
    jobject instance;
    jmethodID ctor;
    jmethodID load;
    jmethodID process;
    jmethodID clear;
    jobject imageBuffer;
    std::vector<float> imageBufferNative;
    jobject yoloBuffer;
    std::vector<float> yoloBufferNative;
    jobject viewBuffer;
    std::vector<float> viewBufferNative;

    FastExamJNI(JNIEnv *jenv, jobject context) {
        jvm = nullptr;
        jenv->GetJavaVM(&jvm);
        this->jenv = nullptr; // thread local, this is set on the worker thread
        this->clazz = static_cast<jclass>(jenv->NewGlobalRef(
                jenv->FindClass("com/echonous/ai/fast/NativeInterface")));
        this->context = jenv->NewGlobalRef(context);

        ctor = jenv->GetMethodID(clazz, "<init>", "(Landroid/content/Context;)V");
        instance = nullptr; // this is slow, initialize on worker
        load = jenv->GetMethodID(clazz, "load", "()V");
        process = jenv->GetMethodID(clazz, "processOrdered", "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;I)I");
        //process = jenv->GetMethodID(clazz, "process", "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)I");
        clear = jenv->GetMethodID(clazz, "clear", "()V");
        // Buffers are also initialized on worker thread
        imageBuffer = nullptr;
        yoloBuffer = nullptr;
        viewBuffer = nullptr;

        LOGD("Created JNI interface, jvm = %p, jenv = %p, clazz = %p, instance = %p, process = %p, clear = %p",
             jvm, jenv, clazz, instance, clear);
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
        LOGD("Attaching to thread, creating instance...");
        jvm->AttachCurrentThread(&jenv, nullptr);
        instance = makeGlobalRef(jenv->NewObject(clazz, ctor, context));

        // Create buffers on this thread as well
        // set buffer size
        const int YOLO_BUFFER_MAX = 8*8*FAST_OBJ_DETECT_NUM_ANCHORS*FAST_OBJ_DETECT_NUM_CHANNELS;
        const int VIEW_BUFFER_MAX = FAST_OBJ_DETECT_NUM_VIEWS;
        imageBufferNative.resize(FAST_OBJ_DETECT_INPUT_WIDTH*FAST_OBJ_DETECT_INPUT_HEIGHT);
        viewBufferNative.resize(VIEW_BUFFER_MAX);
        yoloBufferNative.resize(YOLO_BUFFER_MAX);
        size_t imageBufferSize = imageBufferNative.size() * sizeof(float);
        size_t viewBufferSize = viewBufferNative.size() * sizeof(float);
        size_t yoloBufferSize = yoloBufferNative.size()*sizeof(float);
        imageBuffer = makeGlobalRef(jenv->NewDirectByteBuffer(imageBufferNative.data(), (jlong)imageBufferSize));
        viewBuffer =  makeGlobalRef(jenv->NewDirectByteBuffer(viewBufferNative.data(), (jlong)viewBufferSize));
        yoloBuffer =  makeGlobalRef(jenv->NewDirectByteBuffer(yoloBufferNative.data(), (jlong)yoloBufferSize));
        LOGD("[ FAST ] ImageBufferSize: %d, ViewBufferSize: %d, YoloBufferSize: %d", imageBufferSize, viewBufferSize, yoloBufferSize);
        //labelBufferNative.resize(CARDIAC_ANNOTATOR_LABELER_NUM_CLASSES);
        //labelBuffer = jenv->NewDirectByteBuffer(labelBufferNative.data(), (jlong)labelBufferNative.size() * sizeof(Label));

        jenv->CallVoidMethod(instance, load);
        LOGD("FAST Exam pipeline loaded");

        jenv->DeleteGlobalRef(context);
        context = nullptr;
        LOGD("Attached to thread, instance = %p", instance);
    }

    ~FastExamJNI() {
        LOGD("Destroying JNI interface");
        jenv->DeleteGlobalRef(yoloBuffer);
        jenv->DeleteGlobalRef(imageBuffer);
        jenv->DeleteGlobalRef(viewBuffer);
        jenv->DeleteGlobalRef(instance);
        jenv->DeleteGlobalRef(clazz);
        jvm->DetachCurrentThread();
        jenv = nullptr;
    }
};
FASTObjectDetectorTask::FASTObjectDetectorTask(UltrasoundManager *usm) :
        mUSManager(usm),
        mCB(&usm->getCineBuffer()),
        mPaused(true),
        mStop(false),
        mProcessedFrame(0xFFFFFFFFu),
        mCineFrame(0xFFFFFFFFu),
        mView(CardiacView::A4C),
        mYoloBuffer(TEMPORAL_SMOOTHING_FRAMES_YOLO, TEMPORAL_SMOOTHING_FRAMES_VIEW),
        mInit(false),
        mMutex(),
        mCV(),
        mWorkerThread(),
        mScanConverter(nullptr),
        mImguiRenderer(usm->getAIManager().getImguiRenderer())
{
    LOGD("Created FASTObjectDetectorTask");
    mUnsmoothPredictionBuffer.resize(CINE_FRAME_COUNT);
    mPredictionBuffer.resize(CINE_FRAME_COUNT);
    mViewCineBuffer.resize(CINE_FRAME_COUNT);
    mFrameWasSkipped.resize(CINE_FRAME_COUNT);
}

FASTObjectDetectorTask::~FASTObjectDetectorTask()
{
    stop();
}

template <class It, class T>
size_t IndexOf(It first, It last, const T& value) {

    if constexpr (std::is_same_v<std::decay_t<T>, const char*>) {
        auto location = std::find_if(first, last, [=](const char* s1, const char* s2){ return strcmp(s1, s2) == 0;});
        if (location == last) {
            std::stringstream sout;
            sout << value;
            LOGE("Failed to find value %s in range", sout.str().c_str());
            std::abort();
        }
        return std::distance(first, location);
    } else {
        auto location = std::find(first, last, value);
        if (location == last) {
            std::stringstream sout;
            sout << value;
            LOGE("Failed to find value %s in range", sout.str().c_str());
            std::abort();
        }
        return std::distance(first, location);
    }
}


ThorStatus FASTObjectDetectorTask::init() {
    ThorStatus status;

    // Create scan converter (destroys old scan converter, if any)
    mScanConverter = std::unique_ptr<ScanConverterCL>(new ScanConverterCL);

    using std::begin;
    using std::end;
    mNoiseIndex = std::distance(
            begin(FAST_OBJ_DETECT_VIEW_NAMES),
            std::find(begin(FAST_OBJ_DETECT_VIEW_NAMES), end(FAST_OBJ_DETECT_VIEW_NAMES), "noise"));

    mViewProb.resize(FAST_OBJ_DETECT_NUM_VIEWS-1);
    std::fill(mViewProb.begin(), mViewProb.end(), 1.0f/(FAST_OBJ_DETECT_NUM_VIEWS-1));

    // load locale map, default to en
    Json::Value trio_locale;
    status = mUSManager->getFilesystem()->readAssetJson("trio_locale.json", trio_locale);
    if (IS_THOR_ERROR(status)) { return status; }
    mLocaleMap = trio_locale["FAST"];
    setLanguage("en");

    return THOR_OK;
}

void FASTObjectDetectorTask::start(void *javaEnv, void *javaContext)
{
    stop();
    mStop = false;

    mInit = true;
    mPaused = true;

    auto l = lock(mMutex);
    mCB->registerCallback(this);
    FastExamJNI* jniInterface = new FastExamJNI((JNIEnv*)javaEnv, (jobject)javaContext);
    mJNI = static_cast<void*>(jniInterface);
    mWorkerThread = std::thread(&FASTObjectDetectorTask::workerThreadFunc, this, jniInterface);
}

void FASTObjectDetectorTask::stop()
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

void FASTObjectDetectorTask::setView(CardiacView view)
{
    mImguiRenderer.setA4C(view == CardiacView::A4C);
}

void FASTObjectDetectorTask::setPause(bool pause)
{
    LOGD("%s(%s)", __func__, pause ? "true":"false");
    // TEMP/TODO: Disable pausing
    //return;

    auto l = lock(mMutex);

    mPaused = pause;
    mCV.notify_one();
}


void FASTObjectDetectorTask::setFlip(float flipX, float flipY)
{
    auto l = lock(mMutex);
    mFlipX = flipX;

    if (mHasParams) {
        mScanConverter->setFlip(flipX, flipY);
    }
}

ThorStatus FASTObjectDetectorTask::onInit()
{
    LOGD("%s", __func__);
    auto l = lock(mMutex);
    mProcessedFrame = 0xFFFFFFFFu;
    mCineFrame = 0xFFFFFFFFu;
    // Clear temporal buffer
    mYoloBuffer.clear();

    // Try reading config
    FILE* config = fopen("/data/data/com.echonous.kosmosapp/FASTEnableDebug.txt", "r");
    if (config) {
        mIsDevelopMode = true;
        fclose(config);
    } else {
        mIsDevelopMode = false;
    }


    // DO NOT reset buffer data, since we can have an onTerminate/onInit pair without clearing the cinebuffer data!
    return THOR_OK;
}
void FASTObjectDetectorTask::onTerminate()
{
    //setPause(true);
}
void FASTObjectDetectorTask::setParams(CineBuffer::CineParams& params)
{
    // Quick exit in Color or M mode
    LOGD("setParams, dauDataTypes = 0x%08x", params.dauDataTypes);
    if (BF_GET(params.dauDataTypes, DAU_DATA_TYPE_COLOR, 1) ||
        BF_GET(params.dauDataTypes, DAU_DATA_TYPE_M, 1)) {
        return;
    }

    auto l = lock(mMutex);
    mHasParams = true;
    mParams = params;
    mScanConverter->setConversionParameters(
            FAST_OBJ_DETECT_INPUT_WIDTH,
            FAST_OBJ_DETECT_INPUT_HEIGHT,
            mFlipX,
            1.0f,
            ScanConverterCL::ScaleMode::NON_UNIFORM_SCALE_TO_FIT,
            params.converterParams);
}

void FASTObjectDetectorTask::onData(uint32_t frameNum, uint32_t dauDataTypes)
{
    // Only activate in purely B mode
    if (dauDataTypes == (1<<DAU_DATA_TYPE_B))
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

bool FASTObjectDetectorTask::workerShouldAwaken()
{
    if (mStop)
        return true;
    if (!mPaused && mCineFrame != mProcessedFrame)
        return true;
    return false;
}

void FASTObjectDetectorTask::workerThreadFunc(void* jniInterface)
{
    LOGD("Starting workerThreadFunc");
    FastExamJNI *jni = (FastExamJNI*)jniInterface;
    jni->attachToThread();
    for (size_t i=0; i != FAST_OBJ_DETECT_ENSEMBLE_SIZE; ++i) {

        mEnsemble[i].init(*mUSManager, FAST_OBJ_DETECT_MODEL_PATHS[i]);
    }
    mEnsembleIndex = 0;

    uint32_t prevFrameNum = 0xFFFFFFFFu;
    uint32_t frameNum = 0xFFFFFFFFu;
    float flipX;
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

            if (mProcessedFrame == 0xFFFFFFFFu)
                prevFrameNum = 0xFFFFFFFFu;
            else
                prevFrameNum = frameNum;
            frameNum = mCineFrame;
            flipX = mFlipX;
        }

        // Record frames that were skipped
        if (prevFrameNum != 0xFFFFFFFFu) {
            for (uint32_t frame = prevFrameNum+1; frame != frameNum; ++frame) {
                mFrameWasSkipped[frame % CINE_FRAME_COUNT] = 1;
            }
        }

        // Process the frame (without holding the lock)
        ThorStatus status = processFrame(frameNum, flipX, jniInterface);
        if (IS_THOR_ERROR(status))
        {
            LOGE("Error processing frame %u: %08x", frameNum, status);
        }
    }
    delete jni;
    mJNI = nullptr;
}

void FASTObjectDetectorTask::convertToPhysicalSpace(float flipX)
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
        pred.normx = physicalPoint.x/CARDIAC_ANNOTATOR_INPUT_WIDTH;
        pred.normy = physicalPoint.y/CARDIAC_ANNOTATOR_INPUT_HEIGHT;
        pred.normw = physicalDim.x/CARDIAC_ANNOTATOR_INPUT_WIDTH;
        pred.normh = physicalDim.y/CARDIAC_ANNOTATOR_INPUT_WIDTH;
    }
}

const std::vector<FASTYoloPrediction>& FASTObjectDetectorTask::getPredictions(uint32_t frameNum) const
{
    return mPredictionBuffer[frameNum % CINE_FRAME_COUNT];
}

std::pair<int, float> FASTObjectDetectorTask::getView(uint32_t frameNum) const
{
    return mViewCineBuffer[frameNum % CINE_FRAME_COUNT];
}
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
static float YoloIOU(const FASTYoloPrediction &a, const FASTYoloPrediction &b) {
    const float intersectionWidth = std::max(std::min(a.x+a.w/2.0f, b.x+b.w/2.0f) - std::max(a.x-a.w/2.0f, b.x-b.w/2.0f), 0.0f);
    const float intersectionHeight = std::max(std::min(a.y+a.h/2.0f, b.y+b.h/2.0f) - std::max(a.y-a.h/2.0f, b.y-b.h/2.0f), 0.0f);
    const float intersection = intersectionWidth*intersectionHeight;
    const float unionArea = a.w*a.h + b.w*b.h - intersection;

    if (unionArea <= 0.0f)
        return 10000000000.0f;
    return intersection / unionArea;
}

static void ObjThresholding(std::vector<FASTYoloPrediction> &boxes) {
    constexpr float OBJ_THRESHOLDS[] = {
        0.25f,
        0.25f,
        0.20f,
        0.15f,
        0.30f,
        0.20f,
        0.20f,
        0.20f,
        0.30f,
        0.30f,
        0.90f,
        0.90f,
        0.10f,
        0.10f,
        0.10f,
        0.10f,
        0.10f
    };

    const auto predicate = [=](const FASTYoloPrediction &box) {
        return box.obj_conf < OBJ_THRESHOLDS[box.class_pred];
    };

    const auto first = std::remove_if(boxes.begin(), boxes.end(), predicate);
    const auto last = boxes.end();
    boxes.erase(first, last);
}

static void EffusionFilter(std::vector<FASTYoloPrediction> &boxes) {
    const int EFFU_INDEX = IndexOf(std::begin(FAST_OBJ_DETECT_LABEL_NAMES), std::end(FAST_OBJ_DETECT_LABEL_NAMES), "effusion");
    const int GALLB_INDEX = IndexOf(std::begin(FAST_OBJ_DETECT_LABEL_NAMES), std::end(FAST_OBJ_DETECT_LABEL_NAMES), "gallbladder");
    const float MAX_IOU = 0.2f;
    const float CONF_THRESHOLD = 0.3f;

    auto effu = std::find_if(boxes.begin(), boxes.end(), [=](const auto& pred) { return pred.class_pred == EFFU_INDEX; });
    auto gallb = std::find_if(boxes.begin(), boxes.end(), [=](const auto& pred) { return pred.class_pred == GALLB_INDEX; });
    if (effu != boxes.end() && gallb != boxes.end()) {
        float iou = YoloIOU(*effu, *gallb);
        float conf_diff = std::abs(effu->obj_conf - gallb->obj_conf);
        if (iou >= MAX_IOU && conf_diff <= CONF_THRESHOLD) {
            boxes.erase(effu);
            gallb = std::find_if(boxes.begin(), boxes.end(), [=](const auto& pred) { return pred.class_pred == GALLB_INDEX; });
            boxes.erase(gallb);
        }
    }
}

static void ObjTopo(std::vector<FASTYoloPrediction> &boxes) {
    const int AORTA_INDEX = IndexOf(std::begin(FAST_OBJ_DETECT_LABEL_NAMES), std::end(FAST_OBJ_DETECT_LABEL_NAMES), "aorta");
    const int IVC_INDEX = IndexOf(std::begin(FAST_OBJ_DETECT_LABEL_NAMES), std::end(FAST_OBJ_DETECT_LABEL_NAMES), "ivc");;
    const float MAX_IOU = 0.1f;
    auto aorta_iter = std::find_if(boxes.begin(), boxes.end(),
                                     [=](const FASTYoloPrediction& pred) { return pred.class_pred == AORTA_INDEX; });
    auto ivc_iter = std::find_if(boxes.begin(), boxes.end(),
                                   [=](const FASTYoloPrediction& pred) { return pred.class_pred == IVC_INDEX; });
    if (aorta_iter != boxes.end() && ivc_iter != boxes.end()) {
        // Both classes exist, enforce minimal iou
        float iou = YoloIOU(*aorta_iter, *ivc_iter);
        if (iou >= MAX_IOU) {
            // Must not overlap too much
            boxes.erase(aorta_iter);
            ivc_iter = std::find_if(boxes.begin(), boxes.end(),
                                      [=](const FASTYoloPrediction& pred) { return pred.class_pred == IVC_INDEX; });
            boxes.erase(ivc_iter);
        }
    } else if (aorta_iter != boxes.end()) {
        // Only aorta_t exists, but aorta_t and ivc_t must coexist
        //boxes.erase(aorta_t_iter);
    } else if (ivc_iter != boxes.end()) {
        // Only ivc_t exists, but aorta_t and ivc_t must coexist
        //boxes.erase(ivc_t_iter);
    }
}

static void MoveFilter(std::vector<FASTYoloPrediction> &preds) {
    const float MOVE_THRESHOLD = 50.0f;
    const auto filter = [=](const FASTYoloPrediction &pred) { return pred.std_dev > MOVE_THRESHOLD; };
    const auto end = std::remove_if(preds.begin(), preds.end(), filter);
    preds.erase(end, preds.end());
}

static int ViewThreshold(const float *viewscore) {
    constexpr float VIEW_THRESHOLDS[] = {
            0.0f,
            0.2f,
            0.3f,
            0.3f,
            0.2f,
            0.2f,
            0.2f,
            0.2f,
            0.2f,
            0.2f,
            0.2f,
            0.2f,
            0.2f,
            0.2f,
            0.3f
    };
    const int NOISE_INDEX = 0;

    int index1 = -1;
    int index2 = -1;
    float conf1 = -1.0f;
    float conf2 = -1.0f;
    for (int i=0; i != FAST_OBJ_DETECT_NUM_VIEWS; ++i) {
        if (viewscore[i] >= conf1) {
            index2 = index1;
            conf2 = conf1;
            index1 = i;
            conf1 = viewscore[i];
        } else if (viewscore[i] >= conf2) {
            index2 = i;
            conf2 = viewscore[i];
        }
    }

    float diff = conf1-conf2;
    if (diff >= VIEW_THRESHOLDS[index1])
        return index1;
    else
        return NOISE_INDEX;
}

static void PericardiumFilter(int viewIndex, std::vector<FASTYoloPrediction> &boxes) {
    const int ALLOWED_VIEWS[] = {8, 9, 10, 11, 12};
    const int PERICARDIUM_INDEX = 16;
    const auto filter = [=](const FASTYoloPrediction &pred) { return pred.class_pred == PERICARDIUM_INDEX; };

    if (std::find(std::begin(ALLOWED_VIEWS), std::end(ALLOWED_VIEWS), viewIndex) == std::end(ALLOWED_VIEWS)) {
        // View not in whitelist
        // Remove all pericardium labels
        const auto end = std::remove_if(boxes.begin(), boxes.end(), filter);
        boxes.erase(end, boxes.end());
    }
}

ThorStatus FASTObjectDetectorTask::processFrame(uint32_t frameNum, float flipX, void* jni)
{
    // This is safe because we will only ever have the one worker thread
    if (!mInit)
    {
        for (size_t i=0; i != FAST_OBJ_DETECT_ENSEMBLE_SIZE; ++i) {
            mEnsemble[i].init(*mUSManager, FAST_OBJ_DETECT_MODEL_PATHS[i]);
        }
        mEnsembleIndex = 0;
        mInit = true;
    }

    ThorStatus status;
    FastExamJNI* fast = (FastExamJNI*)jni;
    if (true /* try to run every frame */)
    {
        // Scan convert image
        uint8_t *prescan = mCB->getFrame(frameNum, DAU_DATA_TYPE_B);
        status = mScanConverter->runCLTex(prescan, fast->imageBufferNative.data());
        if (IS_THOR_ERROR(status))
            return status;

         jint numLabels = fast->jenv->CallIntMethod(fast->instance, fast->process, fast->imageBuffer, fast->yoloBuffer, fast->viewBuffer, mEnsembleIndex);

        mPredictions.clear();
        std::vector<float> view;
        status = mEnsemble[0].postProcess(fast->viewBufferNative, fast->yoloBufferNative, view, mPredictions);
        mEnsembleIndex++;
        if (mEnsembleIndex == FAST_OBJ_DETECT_ENSEMBLE_SIZE)
            mEnsembleIndex = 0;
        if (IS_THOR_ERROR(status))
            return status;

        convertToPhysicalSpace(flipX);

        mUnsmoothPredictionBuffer[frameNum % CINE_FRAME_COUNT] = mPredictions;
        mYoloBuffer.append(view, mPredictions);

        // Copy out viewscores and list of boxes
        auto viewscore = mYoloBuffer.getViewscores();
        auto boxes = mYoloBuffer.get();

        //filterView(viewscore.data());
        multiplyBoxConfs(viewscore.data(), boxes);
        EffusionFilter(boxes);
        ObjThresholding(boxes);
        ObjTopo(boxes);
        MoveFilter(boxes);

        int viewIndex = ViewThreshold(viewscore.data());

        PericardiumFilter(viewIndex, boxes);

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
        mFrameWasSkipped[frameNum % CINE_FRAME_COUNT] = 0;
        auto& bufferBoxes = mPredictionBuffer[frameNum % CINE_FRAME_COUNT];
        bufferBoxes.clear();
        bufferBoxes.resize(boxes.size());
        std::copy(boxes.begin(), boxes.end(), bufferBoxes.begin());


        mViewCineBuffer[frameNum % CINE_FRAME_COUNT] = std::make_pair(viewIndex, viewscore[viewIndex]);
    } else {
        // Copy from previous frame
        size_t prevIndex = (frameNum-1) % CINE_FRAME_COUNT;
        const auto& boxes = mPredictionBuffer[prevIndex];
        auto& bufferBoxes = mPredictionBuffer[frameNum % CINE_FRAME_COUNT];
        bufferBoxes.clear();
        bufferBoxes.resize(boxes.size());
        std::copy(boxes.begin(), boxes.end(), bufferBoxes.begin());

        mViewCineBuffer[frameNum % CINE_FRAME_COUNT] = mViewCineBuffer[prevIndex];
    }

    return THOR_OK;
}

void FASTObjectDetectorTask::addTextRanges(ImFontGlyphRangesBuilder &builder) {
    for (const Json::Value &locale : mLocaleMap) {
        for (const Json::Value &text : locale) {
            //LOGD("Adding text \"%s\" to font", text.asCString());
            builder.AddText(text.asCString());
        }
    }
}
void FASTObjectDetectorTask::setLanguage(const char *lang) {
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

const char *FASTObjectDetectorTask::getLocalizedString(const char *id) const {
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
void FASTObjectDetectorTask::draw(CineViewer &cineviewer, uint32_t frameNum) {

    // Do not draw if we have no processed frames, or if autolabel is paused
    if (mProcessedFrame == 0xFFFFFFFFu || mPaused)
        return;

    // Get conversion matrix to screen space coords
    float physToScreen[9];
    cineviewer.queryPhysicalToPixelMap(physToScreen, 9);

    ImU32 labelColor = ImColor(1.0f, 1.0f, 0.0f);
    ImU32 labelDebugColor = ImColor(1.0f, 0.0f, 0.0f);
    ImU32 labelDebug2Color = ImColor(0.0f, 1.0f, 0.0f);
    // If we are live, we need to display the latest frame, not frameNum
    const auto& preds = mPredictionBuffer[mProcessedFrame % CINE_FRAME_COUNT];

    for (auto &p : preds)
    {
        char labelBuffer[256];
        if (p.class_pred < 0 || p.class_pred >= FAST_OBJ_DETECT_NUM_OBJECTS) {
            // skip prediction, most likely caused by model running slowly?
            LOGW("Skipping prediction with class_pred = %d", (int)p.class_pred);
            continue;
        }

        if (mIsDevelopMode) {
            snprintf(labelBuffer, sizeof(labelBuffer), "%s (%0.2f)",
                     getLocalizedString(FAST_OBJ_DETECT_LABEL_NAMES[p.class_pred]), p.obj_conf);
        } else {
            snprintf(labelBuffer, sizeof(labelBuffer), "%s",
                     getLocalizedString(FAST_OBJ_DETECT_LABEL_NAMES[p.class_pred]));
        }

        float posPhys[3] = {p.x, p.y, 1.0f};
        float posScreen[3];
        ViewMatrix3x3::multiplyMV(posScreen, physToScreen, posPhys);

        auto *drawList = ImGui::GetBackgroundDrawList();
        drawList->AddCircleFilled(ImVec2(posScreen[0], posScreen[1]), 7.0f, labelColor);
        drawList->AddText(ImVec2(posScreen[0]+20.0f, posScreen[1]-18.0f), labelColor, labelBuffer);
    }

    if (mIsDevelopMode) {
        if (ImGui::Begin("FAST Info")) {
            const auto &viewIndexConfPair = mViewCineBuffer[mProcessedFrame % CINE_FRAME_COUNT];

            if (viewIndexConfPair.first < 0 || viewIndexConfPair.first >= FAST_OBJ_DETECT_NUM_VIEWS) {
                LOGW("View index %d out of bounds, model running too slow?", viewIndexConfPair.first);
            } else {
                ImGui::Text("View: %s (%f)", getLocalizedString(FAST_OBJ_DETECT_VIEW_NAMES[viewIndexConfPair.first]),
                            viewIndexConfPair.second);

                for (const auto &p : preds) {
                    float posPhys[3] = {p.x, p.y, 1.0f};
                    float posScreen[3];
                    ViewMatrix3x3::multiplyMV(posScreen, physToScreen, posPhys);

                    ImGui::Text("  %s (%0.2f) at (%0.2f, %0.2f)",
                                getLocalizedString(FAST_OBJ_DETECT_LABEL_NAMES[p.class_pred]),
                                p.obj_conf,
                                posScreen[0], posScreen[1]
                    );
                }
            }
        }
        ImGui::End();
    } else {
        // Show view if not empty or
        char viewDisplay[256];
        auto *drawList = ImGui::GetBackgroundDrawList();
        int viewIndex = mViewCineBuffer[mProcessedFrame % CINE_FRAME_COUNT].first;

        if (viewIndex != 0 && viewIndex != 1 && viewIndex >= 0 && viewIndex < FAST_OBJ_DETECT_NUM_VIEWS) {

            snprintf(viewDisplay, sizeof(viewDisplay),
                     getLocalizedString(FAST_VIEW_FORMAT_STRING),
                     getLocalizedString(FAST_OBJ_DETECT_VIEW_DISPLAY_NAMES[viewIndex]));
            drawList->AddText(ImVec2(20.0f, 25.0f), labelColor, viewDisplay);
        } else if (viewIndex < 0 || viewIndex >= FAST_OBJ_DETECT_NUM_VIEWS) {
            LOGW("View index %d out of bounds, model running too slow?", viewIndex);
        }
    }

#ifdef FAST_MOCK_LABELS
    for (int view = 0; view != FAST_OBJ_DETECT_NUM_VIEWS; ++view) {
        char viewDisplay[256];
        auto *drawList = ImGui::GetBackgroundDrawList();
        snprintf(viewDisplay, sizeof(viewDisplay),
                 getLocalizedString(FAST_VIEW_FORMAT_STRING),
                 getLocalizedString(FAST_OBJ_DETECT_VIEW_DISPLAY_NAMES[view]));
        drawList->AddText(ImVec2(20.0f, 100.0f + 40*view), labelColor, viewDisplay);
    }
#endif
}

void FASTObjectDetectorTask::close() {}

void FASTObjectDetectorTask::onSave(uint32_t startFrame, uint32_t endFrame,
                                    echonous::capnp::ThorClip::Builder &builder)
{
    auto fastexam = builder.initFastexam();
    fastexam.setModelVersion("Unknown");
    uint32_t numFrames = 0;
    for (uint32_t i=startFrame; i != endFrame+1; ++i) {
        if (mFrameWasSkipped[i % CINE_FRAME_COUNT] == 0)
            ++numFrames;
    }

    auto preds = fastexam.initFrames(numFrames);
    uint32_t predIndex = 0;
    for (uint32_t i=startFrame; i != endFrame+1; ++i) {
        if (mFrameWasSkipped[i % CINE_FRAME_COUNT] == 1)
            continue;

        auto capnpLabeledFrame = preds[predIndex++];
        capnpLabeledFrame.setFrameNum(i-startFrame);
        int viewIndex = mViewCineBuffer[i % CINE_FRAME_COUNT].first;
        capnpLabeledFrame.setViewIndex(viewIndex);
        capnpLabeledFrame.setViewName(FAST_OBJ_DETECT_VIEW_DISPLAY_NAMES[viewIndex]);
        capnpLabeledFrame.setViewConf(mViewCineBuffer[i % CINE_FRAME_COUNT].second);

        const auto& rawPreds = mUnsmoothPredictionBuffer[i % mUnsmoothPredictionBuffer.size()];
        const auto& smoothPreds = mPredictionBuffer[i % mPredictionBuffer.size()];

        auto capnpRawLabelsPhysical = capnpLabeledFrame.initRawLabelsPhysical(rawPreds.size());
        auto capnpRawLabelsNormalized = capnpLabeledFrame.initRawLabelsNormalized(rawPreds.size());
        for (size_t j=0; j != rawPreds.size(); ++j) {
            auto capnpRawLabelPhysical = capnpRawLabelsPhysical[j];
            const FASTYoloPrediction& pred = rawPreds[j];
            capnpRawLabelPhysical.setX(pred.x);
            capnpRawLabelPhysical.setY(pred.y);
            capnpRawLabelPhysical.setW(pred.w);
            capnpRawLabelPhysical.setH(pred.h);
            capnpRawLabelPhysical.setLabelId(FAST_OBJ_DETECT_LABEL_NAMES_WITH_VIEW[pred.class_pred]);
            capnpRawLabelPhysical.setDisplayText(FAST_OBJ_DETECT_LABEL_NAMES[pred.class_pred]);
            capnpRawLabelPhysical.setConf(pred.obj_conf);

            auto capnpRawLabelNormalized = capnpRawLabelsNormalized[j];
            capnpRawLabelNormalized.setX(pred.normx);
            capnpRawLabelNormalized.setY(pred.normy);
            capnpRawLabelNormalized.setW(pred.normw);
            capnpRawLabelNormalized.setH(pred.normh);
            capnpRawLabelNormalized.setLabelId(FAST_OBJ_DETECT_LABEL_NAMES_WITH_VIEW[pred.class_pred]);
            capnpRawLabelNormalized.setDisplayText(FAST_OBJ_DETECT_LABEL_NAMES[pred.class_pred]);
            capnpRawLabelNormalized.setConf(pred.obj_conf);
        }

        auto capnpSmoothLabelsPhysical = capnpLabeledFrame.initSmoothLabelsPhysical(smoothPreds.size());
        auto capnpSmoothLabelsNormalized = capnpLabeledFrame.initSmoothLabelsNormalized(smoothPreds.size());
        for (size_t j=0; j != smoothPreds.size(); ++j) {
            auto capnpSmoothLabelPhysical = capnpSmoothLabelsPhysical[j];
            const FASTYoloPrediction& pred = smoothPreds[j];
            capnpSmoothLabelPhysical.setX(pred.x);
            capnpSmoothLabelPhysical.setY(pred.y);
            capnpSmoothLabelPhysical.setW(pred.w);
            capnpSmoothLabelPhysical.setH(pred.h);
            capnpSmoothLabelPhysical.setLabelId(FAST_OBJ_DETECT_LABEL_NAMES_WITH_VIEW[pred.class_pred]);
            capnpSmoothLabelPhysical.setDisplayText(FAST_OBJ_DETECT_LABEL_NAMES[pred.class_pred]);
            capnpSmoothLabelPhysical.setConf(pred.obj_conf);

            auto capnpSmoothLabelNormalized = capnpSmoothLabelsNormalized[j];
            capnpSmoothLabelNormalized.setX(pred.normx);
            capnpSmoothLabelNormalized.setY(pred.normy);
            capnpSmoothLabelNormalized.setW(pred.normw);
            capnpSmoothLabelNormalized.setH(pred.normh);
            capnpSmoothLabelNormalized.setLabelId(FAST_OBJ_DETECT_LABEL_NAMES_WITH_VIEW[pred.class_pred]);
            capnpSmoothLabelNormalized.setDisplayText(FAST_OBJ_DETECT_LABEL_NAMES[pred.class_pred]);
            capnpSmoothLabelNormalized.setConf(pred.obj_conf);
        }
    }
}

void FASTObjectDetectorTask::onLoad(echonous::capnp::ThorClip::Reader &reader) {
}


ThorStatus FASTObjectDetectorTask::filterView(float *viewscore) {
#define CHECK_FINITE(x) do{if (!std::isfinite(x)) { LOGE("Value " #x " is not finite, line %d", __LINE__);}}while(0)

    LOGD("filter view viewscore = [%0.4f, %0.4f, %0.4f, %0.4f, %0.4f, %0.4f, %0.4f, %0.4f, %0.4f, %0.4f, %0.4f, %0.4f, %0.4f, %0.4f, %0.4f]",
         viewscore[0],
         viewscore[1],
         viewscore[2],
         viewscore[3],
         viewscore[4],
         viewscore[5],
         viewscore[6],
         viewscore[7],
         viewscore[8],
         viewscore[9],
         viewscore[10],
         viewscore[11],
         viewscore[12],
         viewscore[13],
         viewscore[14]
    );

    auto safe_xlogx = [](float x) {
        if (x <= 0.0f) { return 0.0f; }
        if (x >= 1.0f) { return 0.0f; }
        double value = static_cast<double>(x) * std::log(static_cast<double>(x));
        if (!std::isfinite(value)) { return 0.0f; }
        return static_cast<float>(value);
    };

    float entropyNew = 0.0f;
    float entropyPrev = 0.0f;
    for (unsigned int i=0, j=0; i != FAST_OBJ_DETECT_NUM_VIEWS; ++i) {
        if (i == mNoiseIndex) continue;
        float viewprobNew = viewscore[i] + viewscore[mNoiseIndex]/FAST_OBJ_DETECT_NUM_VIEWS;
        entropyNew -= safe_xlogx(viewprobNew);
        CHECK_FINITE(entropyNew);
        entropyPrev -= safe_xlogx(mViewProb[j]);
        if (mViewProb[j] < 0.0f || !std::isfinite(mViewProb[j])) {
            LOGE("mViewProb[%d] is %f", j, mViewProb[j]);
        }
        CHECK_FINITE(entropyPrev);
        ++j;
    }
    float weight = 200.0f;
    float alpha = entropyNew * weight / (entropyNew * weight + entropyPrev);
    //CHECK_FINITE(alpha);
    for (unsigned int i=0, j=0; i != FAST_OBJ_DETECT_NUM_VIEWS; ++i) {
        if (i == mNoiseIndex) continue;
        float viewprobNew = viewscore[i] + viewscore[mNoiseIndex]/FAST_OBJ_DETECT_NUM_VIEWS;
        //CHECK_FINITE(viewprobNew);
        float newViewProb = alpha * mViewProb[j] + (1-alpha) * viewprobNew;
        if (newViewProb < 0.0f) {
            LOGE("mViewProb[%d] is %f (previously %f, viewprobNew = %f, entropyNew = %f, entropyPrev = %f, alpha = %f)",
                 j, newViewProb, mViewProb[j], viewprobNew, entropyNew, entropyPrev, alpha);
        }
        mViewProb[j] = newViewProb;

        //CHECK_FINITE(mViewProb[j]);
        ++j;
    }
    float updateMin = *std::min_element(mViewProb.begin(), mViewProb.end());
    for (unsigned int i=0, j=0; i != FAST_OBJ_DETECT_NUM_VIEWS; ++i) {
        if (i == mNoiseIndex) continue;
        viewscore[i] = mViewProb[j] - updateMin;
        //CHECK_FINITE(viewscore[i]);
        ++j;
    }
    viewscore[mNoiseIndex] = updateMin * (FAST_OBJ_DETECT_NUM_VIEWS-1);
    //CHECK_FINITE(viewscore[mNoiseIndex]);
    float sum = std::accumulate(viewscore, viewscore+FAST_OBJ_DETECT_NUM_VIEWS, 0.0f);
    for(unsigned int i=0; i != FAST_OBJ_DETECT_NUM_VIEWS; ++i) {
        viewscore[i] /= sum;
    }
    return THOR_OK;
}

ThorStatus FASTObjectDetectorTask::multiplyBoxConfs(const float *viewscore, std::vector<FASTYoloPrediction> &predictions) {
#define INDEXOF(value, array) IndexOf(std::begin(array), std::end(array), value)
    const int RUQ = INDEXOF("RUQ", FAST_OBJ_DETECT_VIEW_NAMES);
    const int LUQ = INDEXOF("LUQ", FAST_OBJ_DETECT_VIEW_NAMES);
    const int SUP = INDEXOF("SUP", FAST_OBJ_DETECT_VIEW_NAMES);
    const int AortaView = INDEXOF("Aorta", FAST_OBJ_DETECT_VIEW_NAMES);
    const int IVCView = INDEXOF("IVC", FAST_OBJ_DETECT_VIEW_NAMES);

    const int ivc = INDEXOF("ivc", FAST_OBJ_DETECT_LABEL_NAMES);
    const int spleen = INDEXOF("spleen", FAST_OBJ_DETECT_LABEL_NAMES);
    const int liver = INDEXOF("liver", FAST_OBJ_DETECT_LABEL_NAMES);
    const int splenorenal = INDEXOF("splenorenal", FAST_OBJ_DETECT_LABEL_NAMES);
    const int hepatorenal = INDEXOF("hepatorenal", FAST_OBJ_DETECT_LABEL_NAMES);
    const int aortaObj = INDEXOF("aorta", FAST_OBJ_DETECT_LABEL_NAMES);
    const int gallbladder = INDEXOF("gallbladder", FAST_OBJ_DETECT_LABEL_NAMES);
    const int bladder = INDEXOF("bladder", FAST_OBJ_DETECT_LABEL_NAMES);


    for (FASTYoloPrediction& box : predictions) {
        float conf = 0.0f;
        for (int viewIndex=0; viewIndex != FAST_OBJ_DETECT_NUM_VIEWS; ++viewIndex) {
            float matrixValue;

            if (viewIndex == RUQ && box.class_pred == ivc) {
                // ivc in RUQ got suppressed
                matrixValue = 0.6f;
            } else if (viewIndex == RUQ && box.class_pred == spleen) {
                // spleen in RUQ
                matrixValue = -1.0f;
            } else if (viewIndex == RUQ && box.class_pred == splenorenal) {
                // splenorenal in RUQ
                matrixValue = -1.0f;
            }  else if (viewIndex == LUQ && box.class_pred == liver) {
                // liver in LUQ
                matrixValue = -1.0f;
            } else if (viewIndex == LUQ && box.class_pred == hepatorenal) {
                // hepatorenal in LUQ
                matrixValue = -1.0f;
            } else if (viewIndex == SUP && box.class_pred == liver) {
                // liver in SUP
                matrixValue = -1.0f;
            } else if (viewIndex == AortaView && box.class_pred == ivc) {
                // ivc in Aorta
                matrixValue = -1.0f;
            } else if (viewIndex == IVCView && box.class_pred == aortaObj) {
                // aorta in IVC
                matrixValue = -1.0f;
            } else if (viewIndex == SUP && box.class_pred == gallbladder) {
                // gallbladder in SUP
                matrixValue = -1.0f;
            } else if (viewIndex == RUQ && box.class_pred == bladder) {
                // bladder in RUQ
                matrixValue = -1.0f;
            } else if (!FAST_OBJ_DETECT_VIEW_LABEL_MASK[viewIndex][box.class_pred]) {
                matrixValue = 0.0f;
            } else {
                matrixValue = 1.0f;
            }
            conf += box.obj_conf * viewscore[viewIndex] * matrixValue;
        }
        box.obj_conf = conf;
    }
    return THOR_OK;
}

Json::Value ToJson(const FASTYoloPrediction& pred) {
    Json::Value root(Json::ValueType::objectValue);
    root["x"] = pred.x;
    root["y"] = pred.y;
    root["w"] = pred.w;
    root["h"] = pred.h;
    root["obj_conf"] = pred.obj_conf;
    root["class_conf"] = pred.class_conf;
    root["class_pred"] = pred.class_pred;
    root["std_dev"] = pred.std_dev;
    return root;
}

Json::Value ToJson(const std::vector<FASTYoloPrediction>& preds) {
    Json::Value root(Json::ValueType::arrayValue);
    for (const auto& pred : preds) {
        root.append(ToJson(pred));
    }
    return root;
}

Json::Value ToJson(const std::vector<std::vector<FASTYoloPrediction>>& preds) {
    Json::Value root(Json::ValueType::arrayValue);
    for (const auto& pred : preds) {
        root.append(ToJson(pred));
    }
    return root;
}

Json::Value ToJson(const std::vector<std::pair<int, float>> &views) {
    Json::Value root(Json::ValueType::arrayValue);
    for (const auto &view : views) {
        Json::Value obj(Json::ValueType::objectValue);
        obj["index"] = view.first;
        obj["conf"] = view.second;
        root.append(obj);
    }
    return root;
}

#ifdef THOR_TRY
#undef THOR_TRY
#endif
#define THOR_TRY(expr) \
do { \
    ThorStatus status = (expr); \
    if (IS_THOR_ERROR(status)) { \
LOGE("Thor Error code 0x%08x in \"%s\" (%s:%d)\n", static_cast<uint32_t>(status), #expr, __FILE__, __LINE__); \
        return status; \
    } \
} while(0)

ThorStatus FASTObjectDetectorTask::runVerificationTest(JNIEnv *jenv, const char *clip, const char *outRoot) {
    std::string appPath = std::string(outRoot);
    std::string outPath =  appPath + "/FAST";
    // Initialization
    {
        for (size_t i=0; i != FAST_OBJ_DETECT_ENSEMBLE_SIZE; ++i) {
            mEnsemble[i].init(*mUSManager, FAST_OBJ_DETECT_MODEL_PATHS[i]);
        }
        mEnsembleIndex = 0;
        init();
        setFlip(-1.0f, 1.0f);
        LOGD("[ FAST VERIFY ] Init complete");
        mInit = true;
    }

    FastExamJNI *jni = static_cast<FastExamJNI*>(mJNI);

    // Output JSON file
    Json::Value root(Json::ValueType::objectValue);
    root["file"] = clip;

    // Intermediate storage
    std::vector<float> postscan(FAST_OBJ_DETECT_INPUT_WIDTH*FAST_OBJ_DETECT_INPUT_HEIGHT);
    std::vector<FASTYoloPrediction> predictions;
    std::vector<float> view;

    // Main conversion loop
    auto &cinebuffer = mUSManager->getCineBuffer();
    uint32_t maxFrame = cinebuffer.getMaxFrameNum()+1;

    // ******************************************
    // Outputs of test:
    // ******************************************
    // Green values
    // Raw input images (after scan conversion)
    std::vector<float> inputs;
    // Pre-smoothed viewscore
    std::vector<float> viewscore_presmooth;
    // Pre-smoothed yolo tensor
    std::vector<float> yolotensor_presmooth;
    // Pre-smoothed boxes after NMS
    std::vector<std::vector<FASTYoloPrediction>> box_presmooth;

    std::vector<float> viewscore_smooth;
    std::vector<std::vector<FASTYoloPrediction>> box_smooth;
    std::vector<float> objscore_smooth;
    std::vector<float> viewscore_kalman;
    std::vector<std::vector<FASTYoloPrediction>> box_viewmult;
    std::vector<std::vector<FASTYoloPrediction>> box_effusion;
    std::vector<std::vector<FASTYoloPrediction>> box_thresholded;
    std::vector<std::vector<FASTYoloPrediction>> box_topology;
    std::vector<std::pair<int, float>> view_thresholded;
    std::vector<std::vector<FASTYoloPrediction>> box_movement;

    // ******************************************
    // End outputs
    // ******************************************
    std::vector<FASTYoloPrediction> detections_by_label[FAST_OBJ_DETECT_NUM_OBJECTS];
    std::fill(mViewProb.begin(), mViewProb.end(), 1.0f/(FAST_OBJ_DETECT_NUM_VIEWS-1));
    mYoloBuffer.clear();
    uint8_t ensembleIndex = 0;
    for (uint32_t frameNum=0; frameNum != maxFrame; ++frameNum)
    {
        uint8_t *prescan = mCB->getFrame(frameNum, DAU_DATA_TYPE_B);
        auto status = mScanConverter->runCLTex(prescan, jni->imageBufferNative.data());
        predictions.clear();
        jenv->CallIntMethod(jni->instance, jni->process, jni->imageBuffer, jni->yoloBuffer, jni->viewBuffer, frameNum%4);

        std::copy(jni->imageBufferNative.begin(), jni->imageBufferNative.end(), std::back_inserter(inputs));


        FASTObjectDetectorModel *annotator = &mEnsemble[0];

        std::copy(jni->viewBufferNative.begin(), jni->viewBufferNative.end(), std::back_inserter(viewscore_presmooth));
        std::copy(jni->yoloBufferNative.begin(), jni->yoloBufferNative.end(), std::back_inserter(yolotensor_presmooth));

        std::copy(jni->viewBufferNative.begin(), jni->viewBufferNative.end(), annotator->mX0.begin());
        std::copy(jni->yoloBufferNative.begin(), jni->yoloBufferNative.end(),annotator->mX1.begin());
        THOR_TRY(annotator->processClassLayer(view));
        THOR_TRY(annotator->processYoloLayer(jni->yoloBufferNative, FAST_OBJ_DETECT_ANCHOR_INFOS[0]));
        THOR_TRY(annotator->postprocessYoloOutputExt(jni->yoloBufferNative, detections_by_label, predictions,FAST_OBJ_DETECT_ANCHOR_INFOS[0]));
        box_presmooth.push_back(predictions);

        mYoloBuffer.append(view, predictions);
        std::vector<FASTYoloPrediction> predictions_smooth = mYoloBuffer.get();
        const auto& viewflat = mYoloBuffer.mViewFlat;

        box_smooth.push_back(predictions_smooth);
        std::copy(viewflat.begin(), viewflat.end(), std::back_inserter(viewscore_smooth));
        view.resize(viewflat.size());
        std::copy(viewflat.begin(), viewflat.end(), view.begin());

        //filterView(view.data());
        multiplyBoxConfs(view.data(), predictions_smooth);
        std::copy(view.begin(), view.end(), std::back_inserter(viewscore_kalman));
        box_viewmult.push_back(predictions_smooth);

        EffusionFilter(predictions_smooth);
        box_effusion.push_back(predictions_smooth);

        ObjThresholding(predictions_smooth);
        box_thresholded.push_back(predictions_smooth);

        ObjTopo(predictions_smooth);
        box_topology.push_back(predictions_smooth);

        MoveFilter(predictions_smooth);
        box_movement.push_back(predictions_smooth);

        int viewIndex = ViewThreshold(view.data());
        float viewConf = view[viewIndex];
        view_thresholded.emplace_back(viewIndex, viewConf);
    }

    root["box_nms"] = ToJson(box_presmooth);
    root["box_buffer"] = ToJson(box_smooth);
    root["box_multiplication"] = ToJson(box_viewmult);
    root["box_effusion"] = ToJson(box_effusion);
    root["box_obj_thresholding"] = ToJson(box_thresholded);
    root["box_topology"] = ToJson(box_topology);
    root["box_movement"] = ToJson(box_movement);
    root["view_thresholding"] = ToJson(view_thresholded);

    const char *id_begin = strrchr(clip, '/')+1;
    const char *id_end = strrchr(clip, '.');
    char outpath[512];
    snprintf(outpath, sizeof(outpath), "%s/%.*s_box.json",
             outRoot,
             static_cast<int>(id_end - id_begin), id_begin);

    std::ofstream os(outpath);
    os << root << "\n";

    snprintf(outpath, sizeof(outpath), "%s/%.*s_input.raw",
             outRoot,
             static_cast<int>(id_end - id_begin), id_begin);
    printf("Writing input to %s\n", outpath);
    fflush(stdout);
    FILE *file = fopen(outpath, "wb");
    fwrite(inputs.data(), sizeof(float), inputs.size(), file);
    fclose(file);

    snprintf(outpath, sizeof(outpath), "%s/%.*s_viewscore_green.raw",
             outRoot,
             static_cast<int>(id_end - id_begin), id_begin);
    printf("Writing viewscore1 to %s\n", outpath);
    fflush(stdout);
    file = fopen(outpath, "wb");
    fwrite(viewscore_presmooth.data(), sizeof(float), viewscore_presmooth.size(), file);
    fclose(file);

    snprintf(outpath, sizeof(outpath), "%s/%.*s_yolotensor_green.raw",
             outRoot,
             static_cast<int>(id_end - id_begin), id_begin);
    printf("Writing yolotensor1 to %s\n", outpath);
    fflush(stdout);
    file = fopen(outpath, "wb");
    fwrite(yolotensor_presmooth.data(), sizeof(float), yolotensor_presmooth.size(), file);
    fclose(file);

    snprintf(outpath, sizeof(outpath), "%s/%.*s_viewscore_blue.raw",
             outRoot,
             static_cast<int>(id_end - id_begin), id_begin);
    printf("Writing viewscore2 to %s\n", outpath);
    fflush(stdout);
    file = fopen(outpath, "wb");
    fwrite(viewscore_smooth.data(), sizeof(float), viewscore_smooth.size(), file);
    fclose(file);

    snprintf(outpath, sizeof(outpath), "%s/%.*s_viewscore_red.raw",
             outRoot,
             static_cast<int>(id_end - id_begin), id_begin);
    printf("Writing viewscore2 to %s\n", outpath);
    fflush(stdout);
    file = fopen(outpath, "wb");
    fwrite(viewscore_kalman.data(), sizeof(float), viewscore_kalman.size(), file);
    fclose(file);
    return THOR_OK;
}
