#define LOG_TAG "PLAXPipeline"

#include <PLAXPipeline.h>
#include <UltrasoundManager.h>
#include <Filesystem.h>
#include <MLOperations.h>
#include <cassert>
#include <json/json.h>
#include <GuidanceJNI.h>

#define MOCK_PLAX_RESULTS 0

#if defined(MOCK_PLAX_RESULTS) && MOCK_PLAX_RESULTS
namespace {
    int MockPLAXIQ(uint32_t frameNum) {
        if (frameNum < 60)
            return 1;
        else if (frameNum < 90)
            return 5;
        else if (frameNum < 120)
            return 2;
        else
            return 3;
    }
    int MockPLAXView(uint32_t frameNum) {
        if (frameNum < 60)
            return 7;
        else if (frameNum < 90)
            return 6;
        else if (frameNum < 120)
            return 4;
        else
            return 6;
    }
}
#endif

// Hard coded default config, could swap to reading from a JSON or something
static const PLAXPipeline::Config CONFIG = PLAXPipeline::Config {
    "2022_06_10_PLAX_SingleBranch.dlc",
    {"Gemm_66", "Gemm_43"},
    {"iq", "view"},
    /* num IQ Classes */ 5, /* num View Classes */ 21,
    /* optimal view index */ 6,
    /* unsure view index */ 19,
    /* View Class names */
    {
        "APICAL",
        "PSAX-AV",
        "PSAX-MV",
        "PSAX-PM",
        "PSAX-AP",
        "OTHER",
        "PLAX-BEST",
        "PLAX-ALAT",
        "PLAX-AMED",
        "PLAX-SLAT",
        "PLAX-SMED",
        "PLAX-SUP",
        "PLAX-SDN",
        "PLAX-CLK",
        "PLAX-CTR",
        "PLAX-TDN",
        "RVIT",
        "PLAX-TUP",
        "RVOT",
        "PLAX-UNSURE",
        "FLIP"
    },
    /* Smoothing filter frames */
    30,
    /* IQ transitions */
    {
        {1,2, 0.425},
        {3,4, 0.4},
        {4,5, 0.3}
    },
    /* max qualities */
    {
        1,
        1,
        1,
        1,
        1,
        1,
        5,
        3,
        3,
        3,
        3,
        3,
        4,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2
    },
    /* autocapture min */
    90,
    /* autocapture max */
    90,
    /* hysteresisK */
    10,
    /* hysteresisDiff */
    0.2f,
    /* minViewConf */
    0.2f,
};

static const char* name(Json::ValueType type) {
    switch(type) {
    case Json::nullValue: return "<null>";
    case Json::intValue: return "int";
    case Json::uintValue: return "uint";
    case Json::realValue: return "real";
    case Json::stringValue: return "string";
    case Json::booleanValue: return "bool";
    case Json::arrayValue: return "array";
    case Json::objectValue: return "object";
    default: return "<Unknown>";
    }
}

std::string PrintVec(const std::vector<std::string> &list) {
    if (list.empty())
        return "[]";
    std::string result;
    result.append("[\"");
    result.append(list.front());
    result.append("\"");
    for (unsigned int i=1; i < list.size(); ++i) {
        result.append(", \"");
        result.append(list[i]);
        result.append("\"");
    }
    result.append("]");
    return result;
}

/// Reads optional overrides for config parameters
static void ReadConfigJson(Filesystem* filesystem, PLAXPipeline::Config& config) {
    ThorStatus status;
    std::vector<unsigned char> buffer;
    status = filesystem->readAsset("plax_config.json", buffer);
    if (IS_THOR_ERROR(status))
        return; // Fine if no config file is present
    std::string configString(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    std::istringstream configStream(configString);
    Json::Value json;
    configStream >> json;

    if (json["modelName"].isString()) {
        config.modelName = json["modelName"].asString();
        LOGD("Overwrote model name to %s", config.modelName.c_str());
    }
    if (json["outputLayers"].isArray()) {
        std::vector<std::string> layers;
        bool ok = true;
        for (const Json::Value& v : json["outputLayers"]) {
            if (v.isString())
                layers.push_back(v.asString());
            else {
                LOGE("outputLayers should be a list of strings!");
                ok = false;
                break;
            }
        }
        if (ok) {
            config.modelOutputLayers = std::move(layers);
            LOGD("Overwrote model output layers to %s", PrintVec(config.modelOutputLayers).c_str());
        }
    }
    if (json["outputTensors"].isArray()) {
        std::vector<std::string> tensors;
        bool ok = true;
        for (const Json::Value& v : json["outputTensors"]) {
            if (v.isString())
                tensors.push_back(v.asString());
            else {
                LOGE("outputTensors should be a list of strings!");
                ok = false;
                break;
            }
        }
        if (ok) {
            config.modelOutputTensors = std::move(tensors);
            LOGD("Overwrote model output tensors to %s", PrintVec(config.modelOutputTensors).c_str());
        }
    }
    if (json["numIQScores"].isInt()) {
        LOGW("Config does not support changing number of IQ scores");
    }
    if (json["views"].isArray()) {
        int optimal = -1;
        int unsure = -1;
        std::vector<std::string> views;
        bool ok = true;
        for (const Json::Value& value : json["views"]) {
            if (!value.isString()) {
                LOGE("view must be a list of strings");
                ok = false;
                break;
            }
            std::string view = value.asString();
            if (view == "PLAX-BEST") {
                optimal = static_cast<int>(views.size());
            } else if (view == "PLAX-UNSURE") {
                unsure = static_cast<int>(views.size());
            }
            views.push_back(view);
        }
        if (ok && optimal != -1 && unsure != -1) {
            LOGD("Overwrote view class names to %s", PrintVec(views).c_str());
            if (config.viewClassNames.size() != views.size()) {
                LOGW("Warning: view class names has different size than hard coded. "
                     "Make sure guidanceConfig.json is also updated to have the correct animations!");
            }
            config.viewClassNames = std::move(views);
        } else if (ok) {
            if (optimal == -1) {
                LOGE("Did not find \"PLAX-BEST\" class in view list, one view MUST have this name");
            }
            if (unsure == -1) {
                LOGE("Did not find \"PLAX-UNSURE\" class in view list, one view MUST have this name");
            }
        }
    }
    if (json["smoothingFilterFrames"].isInt()) {
        config.smoothingFilterFrames = json["smoothingFilterFrames"].asInt();
        LOGD("Overwrote smoothing filter frames to %d", config.smoothingFilterFrames);
    }
    if (json["iqTransitions"].isArray()) {
        std::vector<PLAXPipeline::IQTransition> transitions;
        bool ok = true;
        for (const Json::Value& value : json["iqTransitions"]) {
            if (!value.isObject()) {
                LOGE("(not object) iqTransitions should be a list of objects {from: N, to: N, threshold: Q}");
                ok = false;
                break;
            }
            const Json::Value &from = value["from"];
            const Json::Value &to = value["to"];
            const Json::Value &threshold = value["threshold"];
            if (!from.isInt() || !to.isInt() || !threshold.isDouble()) {
                LOGE("iqTransitions should be a list of objects {from: N, to: N, threshold: Q}");
                LOGE("from = %s", name(from.type()));
                LOGE("to = %s", name(to.type()));
                LOGE("threshold = %s", name(threshold.type()));
                ok = false;
                break;
            }
            transitions.push_back({from.asInt(), to.asInt(), static_cast<float>(threshold.asDouble())});
        }
        if (ok) {
            config.transitions = std::move(transitions);
            LOGD("Overwrote iq transitions");
        }
    }
    const auto& maxQualities = json["maxQualities"];
    if (maxQualities.isObject()) {
        std::vector<int> qualities(config.viewClassNames.size());
        bool ok = true;

        const auto first = maxQualities.begin();
        const auto last = maxQualities.end();
        for (auto it = first; it != last; ++it) {
            if (!it->isInt()) {
                LOGE("maxQualities should be a map of 'view name' => max quality");
                ok = false;
                break;
            }
            const std::string& view = it.key().asString();
            const auto viewIter = std::find(
                    config.viewClassNames.begin(),
                    config.viewClassNames.end(),
                    view);
            if (viewIter == config.viewClassNames.end()) {
                LOGE("View not found in view name list: %s", view.c_str());
                ok = false;
                break;
            }
            int index = static_cast<int>(viewIter-config.viewClassNames.begin());
            qualities[index] = it->asInt();
        }
        if (ok) {
            config.maxQualities = std::move(qualities);
            LOGD("Overwrote max qualities");
        }
    }
    if (json["autocaptureMin"].isInt()) {
        config.autocaptureMin = json["autocaptureMin"].asInt();
        LOGD("Overwrote autocapture min to %d", config.autocaptureMin);
    }
    if (json["autocaptureMax"].isInt()) {
        config.autocaptureMax = json["autocaptureMax"].asInt();
        LOGD("Overwrote autocapture max to %d", config.autocaptureMax);
    }
    if (json["hysteresisK"].isInt()) {
        config.hysteresisK = json["hysteresisK"].asInt();
        LOGD("Overwrote hysteresisK to %d", config.hysteresisK);
    }
    if (json["hysteresisDiff"].isDouble()) {
        config.hysteresisDiff = json["hysteresisDiff"].asFloat();
        LOGD("Overwrote hysteresisDiff to %f", config.hysteresisDiff);
    }
    if (json["minViewConf"].isDouble()) {
        config.minViewConf = json["minViewConf"].asFloat();
        LOGD("Overwrote minViewConf to %f", config.minViewConf);
    }
}

PLAXPipeline::PLAXPipeline() :
    mConfig(CONFIG),
    mIsInit(false),
    mNeedsSetParams(false),
    mCineBuffer(nullptr),
    mScanConverter(nullptr),
    mScanConverterIsInit(false),
    mFlipX(-1.0f),
    mSmoothingBufferStartIndex(0),
    mSmoothingBufferSize(0),
    mIQAutocaptureBuffer(0)
{}
PLAXPipeline::~PLAXPipeline() { uninit(); }

ThorStatus PLAXPipeline::init(UltrasoundManager *manager, GuidanceJNI *jni) {
    ThorStatus status;
    LOGD("init");
    mCineBuffer = &manager->getCineBuffer();
    mScanConverter.reset(new ScanConverterCL);
    mScanConverterIsInit = false;

    LOGD("Created AIModel and ScanConverter objects");

    Filesystem* filesystem = manager->getFilesystem();
    ReadConfigJson(filesystem, mConfig);

    LOGD("AIModel built");

    mFlipX = -1.0f;
    mIQConfBuffer.resize(mConfig.numIQScores);
    mViewConfBuffer.resize(mConfig.numViewClasses);
    mIQSmoothingBuffer.resize(mConfig.numIQScores*mConfig.smoothingFilterFrames);
    mViewSmoothingBuffer.resize(mConfig.numViewClasses*mConfig.smoothingFilterFrames);
    mRunningIQConf.resize(mConfig.numIQScores);
    mRunningViewConf.resize(mConfig.numViewClasses);
    mIQAutocaptureBuffer.resize(mConfig.autocaptureMax);

    LOGD("Buffers sized according to config");

    mJNI = jni;

    reset();

    std::unique_lock<std::mutex> lock(mMutex);
    mIsInit = true;
    bool needsSetParams = mNeedsSetParams;
    mNeedsSetParams = false;
    lock.unlock();
    if (needsSetParams)
    {
        setParams(mCineBuffer->getParams().converterParams);
    }

    return THOR_OK;
}
void PLAXPipeline::uninit() {
    reset();

    mIsInit = false;

    mCineBuffer = nullptr;
    mScanConverter = nullptr;
    mScanConverterIsInit = false;
    std::exchange(mIQConfBuffer, {});
    std::exchange(mViewConfBuffer, {});
    std::exchange(mIQSmoothingBuffer, {});
    std::exchange(mViewSmoothingBuffer, {});
    std::exchange(mRunningIQConf, {});
    std::exchange(mRunningViewConf, {});
}

void PLAXPipeline::reset() {
    LOGD("reset");
    mSmoothingBufferStartIndex = 0;
    mSmoothingBufferSize = 0;
    mIQ = -1;
    mView = -1;
    mPrevView = -1;
    mAutocaptureTimeout = 0;

    for (size_t i=0; i != mIQAutocaptureBuffer.size(); ++i) {
        mIQAutocaptureBuffer[i] = 0;
    }
}
ThorStatus PLAXPipeline::setFlipX(float flipx) {
    std::unique_lock<std::mutex> lock(mMutex);
    mFlipX = flipx;
    if (!mIsInit) {
        return THOR_OK;
    }
    lock.unlock();

    // Only set scan converter params if the scan converter is already initialized,
    // since otherwise the CineBuffer might not yet have valid scan converter params
    if (mScanConverterIsInit)
        mScanConverter->setFlip(flipx, 1.0f);

    return THOR_OK;
}
ThorStatus PLAXPipeline::setParams(const ScanConverterParams& params) {
    LOGD("setParams");
    //assert(mModel);
    //assert(mScanConverter);
    ThorStatus status;

    // Reset the smoothing filter when changing parameters
    reset();

    {
        std::unique_lock<std::mutex> lock(mMutex);
        // delay set params until we are initialized
        if (!mIsInit) {
            mNeedsSetParams = true;
            return THOR_OK;
        }
    }

    if (mIsInit) {

        // Want to convert so that the edges are in frame
        status = mScanConverter->setConversionParameters(
                224,
                224,
                mFlipX,
                1.0f,
                ScanConverterCL::ScaleMode::NON_UNIFORM_SCALE_TO_FIT,
                params);
        if (IS_THOR_ERROR(status)) return status;

        // Potentially delayed scan converter initialization
        if (!mScanConverterIsInit) {
            status = mScanConverter->init();
            if (IS_THOR_ERROR(status)) return status;
            mScanConverterIsInit = true;
        }
    }
    return THOR_OK;
}
ThorStatus PLAXPipeline::process(uint32_t frameNum, int *quality, int *subview) {
    ThorStatus status;

    // WARNING: `process` should only be called after `init`, so this check
    // should never trigger.
    {
        std::unique_lock<std::mutex> lock(mMutex);
        if (!mIsInit) {
            LOGW("PLAXPipeline has not finished init, but process was called");
            return THOR_OK;
        }
    }


    status = scanConvert(frameNum);
    if (IS_THOR_ERROR(status)) return status;
    status = runModel(mJNI->jenv);
    if (IS_THOR_ERROR(status)) return status;

    applySoftmax();
    runSmoothingFilter();
    applyIQTransitions();
    hysteresisFilter();
    checkMinViewConf();
    capIQ();

#if defined(MOCK_PLAX_RESULTS) && MOCK_PLAX_RESULTS
    mIQ = MockPLAXIQ(frameNum);
    mView = MockPLAXView(frameNum);
#endif

    mIQAutocaptureBuffer.push(mIQ);

    //LOGD("  iq (final): %d (%f)", mIQ, mRunningIQConf[mIQ-1]);
    //LOGD("  view (final): %d (%f)", mView, mRunningViewConf[mView]);

    if (quality) *quality = mIQ;
    if (subview) *subview = mView;

    if (mAutocaptureTimeout != 0)
        --mAutocaptureTimeout;
    return THOR_OK;
}

static Json::Value ToJson(const std::vector<float> vec) {
    Json::Value json;
    for (float value : vec)
        json.append(value);
    return json;
}

Json::Value PLAXPipeline::processIntoJson(JNIEnv *jenv, uint32_t frameNum) {
    Json::Value json(Json::ValueType::objectValue);
    ThorStatus status;
    status = scanConvert(frameNum);
    if (IS_THOR_ERROR(status)) {
        json["error"] = status;
        return json;
    }
    status = runModel(jenv);
    if (IS_THOR_ERROR(status)) {
        json["error"] = status;
        return json;
    }
    json["raw"]["iq"] = ToJson(mIQConfBuffer);
    json["raw"]["view"] = ToJson(mViewConfBuffer);

    applySoftmax();
    json["softmax"]["iq"] = ToJson(mIQConfBuffer);
    json["softmax"]["view"] = ToJson(mViewConfBuffer);

    runSmoothingFilter();
    json["smoothed"]["iq"] = ToJson(mRunningIQConf);
    json["smoothed"]["view"] = ToJson(mRunningViewConf);

    applyIQTransitions();
    json["afterIQTransitions"] = getIQViewJson();
    hysteresisFilter();
    json["afterHysteresis"] = getIQViewJson();
    checkMinViewConf();
    json["afterMinViewConf"] = getIQViewJson();
    capIQ();
    json["afterIQCapping"] = getIQViewJson();
    return json;
}
Json::Value PLAXPipeline::getIQViewJson() {
    Json::Value json;
    json["iq"] = mIQ;
    json["iqConf"] = mRunningIQConf[mIQ-1];
    json["view"] = mView;
    json["viewConf"] = mRunningViewConf[mView];
    return json;
}
int PLAXPipeline::numViews() const {
    return mConfig.numViewClasses;
}
int PLAXPipeline::quality() const {
    //assert(1 <= mIQ && mIQ <= mConfig.numIQScores);
    return mIQ;
}
int PLAXPipeline::view() const {
    //assert(0 <= mView && mView < mConfig.numViewClasses);
    return mView;
}
bool PLAXPipeline::isOptimal() const {
    return view() == mConfig.optimalView;
}
ThorStatus PLAXPipeline::scanConvert(uint32_t frameNum) {
    //assert(mCineBuffer);
    //assert(mScanConverter);
    //assert(mScanConverterIsInit);
    //assert(!mFrameBuffer.empty()); // Don't bother checking the actual size, just assume it's good

    if (!mScanConverterIsInit) {
        ThorStatus status = setParams(mCineBuffer->getParams().converterParams);
        if (IS_THOR_ERROR(status)) {
            LOGE("Error setting scan converter params: 0x%08x", status);
            return status;
        }
    }

    unsigned char* input = mCineBuffer->getFrame(frameNum, DAU_DATA_TYPE_B);
    return mScanConverter->runCLTex(input, mJNI->imageBufferPLAXNative.data());
}
ThorStatus PLAXPipeline::runModel(JNIEnv *jenv) {
    jenv->CallVoidMethod(
            mJNI->instance,
            mJNI->processPLAX,
            mJNI->imageBufferPLAX,
            mJNI->outputBufferPLAX);

    std::copy_n(mJNI->outputBufferPLAXNative.data(), CONFIG.numViewClasses, mViewConfBuffer.data());
    std::copy_n(mJNI->outputBufferPLAXNative.data() + CONFIG.numViewClasses, CONFIG.numIQScores, mIQConfBuffer.data());

    return THOR_OK;
}
void PLAXPipeline::applySoftmax() {
    softmax(mIQConfBuffer.begin(), mIQConfBuffer.end());
    softmax(mViewConfBuffer.begin(), mViewConfBuffer.end());
}
void PLAXPipeline::runSmoothingFilter() {
    // Sanity check the size, start index, and buffer sizes
    //assert(0 <= mSmoothingBufferSize && mSmoothingBufferSize <= mConfig.smoothingFilterFrames);
    //assert(0 <= mSmoothingBufferStartIndex && mSmoothingBufferStartIndex < mConfig.smoothingFilterFrames);
    //assert(mIQConfBuffer.size() == mConfig.numIQScores);
    //assert(mViewConfBuffer.size() == mConfig.numViewClasses);
    //assert(mIQSmoothingBuffer.size() == mConfig.smoothingFilterFrames * mConfig.numIQScores);
    //assert(mViewSmoothingBuffer.size() == mConfig.smoothingFilterFrames * mConfig.numViewClasses);
    //assert(mRunningIQConf.size() == mConfig.numIQScores);
    //assert(mRunningViewConf.size() == mConfig.numViewClasses);

    if (mSmoothingBufferSize != mConfig.smoothingFilterFrames) {
        // Adding a new frame without removing a frame
        const int i = mSmoothingBufferStartIndex + mSmoothingBufferSize;
        const int offsetIQ = i*mConfig.numIQScores;
        const int offsetView = i*mConfig.numViewClasses;
        std::copy(mIQConfBuffer.begin(), mIQConfBuffer.end(), mIQSmoothingBuffer.begin() + offsetIQ);
        std::copy(mViewConfBuffer.begin(), mViewConfBuffer.end(), mViewSmoothingBuffer.begin() + offsetView);

        for (int j=0; j != mConfig.numIQScores; ++j) {
            float prev = mRunningIQConf[j];
            mRunningIQConf[j] = (mRunningIQConf[j]*mSmoothingBufferSize + mIQConfBuffer[j])/(mSmoothingBufferSize+1);
            if (mRunningIQConf[j] < 0.0f || 1.0f < mRunningIQConf[j]) {
                LOGW("Running IQ[%d] = %f, prev = %f, next = %f, size = %d",
                     j, mRunningIQConf[j], prev, mIQConfBuffer[j], mSmoothingBufferSize);
            }
        }
        for (int j=0; j != mConfig.numViewClasses; ++j) {
            mRunningViewConf[j] = (mRunningViewConf[j]*mSmoothingBufferSize + mViewConfBuffer[j])/(mSmoothingBufferSize+1);
        }
        mSmoothingBufferSize++;
    } else {
        // Both adding a new frame and removing an old frame
        const int i = mSmoothingBufferStartIndex;
        const int offsetIQ = i*mConfig.numIQScores;
        const int offsetView = i*mConfig.numViewClasses;

        for (int j=0; j != mConfig.numIQScores; ++j) {
            mRunningIQConf[j] += (mIQConfBuffer[j] - mIQSmoothingBuffer[offsetIQ+j])/mSmoothingBufferSize;
        }
        for (int j=0; j != mConfig.numViewClasses; ++j) {
            mRunningViewConf[j] += (mViewConfBuffer[j] - mViewSmoothingBuffer[offsetView+j])/mSmoothingBufferSize;
        }
        std::copy(mIQConfBuffer.begin(), mIQConfBuffer.end(), mIQSmoothingBuffer.begin() + offsetIQ);
        std::copy(mViewConfBuffer.begin(), mViewConfBuffer.end(), mViewSmoothingBuffer.begin()+offsetView);


        mSmoothingBufferStartIndex++;
        if (mSmoothingBufferStartIndex == mConfig.smoothingFilterFrames)
            mSmoothingBufferStartIndex = 0;
    }

    // Then determine current IQ and view class
    mIQ = argmax(mRunningIQConf.begin(), mRunningIQConf.end())+1;
    mView = argmax(mRunningViewConf.begin(), mRunningViewConf.end());

    // Sanity check postconditions
    //assert(1 <= mSmoothingBufferSize && mSmoothingBufferSize <= mConfig.smoothingFilterFrames);
    //assert(0 <= mSmoothingBufferStartIndex && mSmoothingBufferStartIndex < mConfig.smoothingFilterFrames);
    //assert(1 <= mIQ && mIQ <= mConfig.numIQScores);
    //assert(0 <= mView && mView < mConfig.numViewClasses);
}
void PLAXPipeline::applyIQTransitions() {
    for (const IQTransition& t : mConfig.transitions) {
        if (mIQ == t.from && mRunningIQConf[t.to-1] >= t.threshold) {
            mIQ = t.to;
            return; // exit on first matching transition
        }
    }
}
void PLAXPipeline::capIQ() {
    if (mIQ > mConfig.maxQualities[mView]) {
        mIQ = mConfig.maxQualities[mView];
    }
}
void PLAXPipeline::hysteresisFilter() {
    if (mView == mPrevView)
        return;
    if (inTopKViews(mPrevView)) {
        float diff = mRunningViewConf[mView] - mRunningViewConf[mPrevView];
        if (diff <= mConfig.hysteresisDiff) {
            LOGD("Hysteresis filter prevented change from class %d (conf: %0.2f) to class %d (conf: %0.2f)",
                 mPrevView, mRunningViewConf[mPrevView],
                 mView, mRunningViewConf[mView]);
            mView = mPrevView;
        }
    }
    mPrevView = mView;
}
void PLAXPipeline::checkMinViewConf() {
    float maxConf = *std::max_element(mRunningViewConf.begin(), mRunningViewConf.end());
    if (maxConf < mConfig.minViewConf) {
        LOGD("Max view conf of %0.3f did not pass minimum threshold (%0.3f), setting view to unsure",
             maxConf, mConfig.minViewConf);
        mView = mConfig.unsureView;
    }
}
bool PLAXPipeline::inTopKViews(int view) const {
    // When pipeline is reset, we set the previous view to -1, so this should always
    // return false
    if (view < 0 || view >= mConfig.numViewClasses)
        return false;

    // Count how many classes are strictly higher conf,
    // then return true if the number of higher conf classes is strictly less than K
    float score = mRunningViewConf[view];
    int higher = 0;
    for (int i=0; i != mConfig.numViewClasses; ++i) {
        if (mRunningViewConf[i] > score)
            ++higher;
    }
    return higher < mConfig.hysteresisK;
}

int PLAXPipeline::getCountGoodFrames() const {
    int count = 0;
    if (isCurrentIQGood())
        ++count;
    for (size_t i = mIQAutocaptureBuffer.size()-1; i != 0; --i) {
        if (mIQAutocaptureBuffer[i-1] >= 4)
            ++count;
        else
            break;
    }
    return count;
}
bool PLAXPipeline::isCurrentIQGood() const {
    return mIQAutocaptureBuffer[mIQAutocaptureBuffer.size()-1] >= 4;
}

PLAXAutocaptureStatus PLAXPipeline::getAutocapture(int maxFrames) {
    int count = getCountGoodFrames() - mAutocaptureTimeout;
    if (count > maxFrames)
        count = std::max(0, maxFrames);
    bool current = isCurrentIQGood();

//    LOGD("getAutocapture count = %d, current = %s",
//         count, current ? "true" : "false");

    bool triggerLow = count >= mConfig.autocaptureMin && !current;
    bool triggerHigh = count >= mConfig.autocaptureMax;

    // We are starting if the current frame is good, and there is exactly one good frame
    bool start = current && count == 1;
    // Trigger if the current frame is bad and we passed the low threshold, or if
    // we passed the high threshold (always make sure timeout has expired as well)
    bool trigger = triggerLow || triggerHigh;
    // Failure if current frame is bad, we had a non zero count, and we failed to reach the low
    // threshold
    bool failure = !current && count > 0 && count < mConfig.autocaptureMin;
    float score = static_cast<float>(count) / static_cast<float>(mConfig.autocaptureMax);
    float min = static_cast<float>(mConfig.autocaptureMin) / static_cast<float>(mConfig.autocaptureMax);

    if (trigger)
        mAutocaptureTimeout = mConfig.autocaptureMin;

//    auto tostr = [](bool b) { return b ? "true":"false"; };
//    LOGD("  start = %s, trigger = %s, failure = %s",
//         tostr(start), tostr(trigger), tostr(failure));

    return PLAXAutocaptureStatus {start, trigger, failure, score, min};
}
