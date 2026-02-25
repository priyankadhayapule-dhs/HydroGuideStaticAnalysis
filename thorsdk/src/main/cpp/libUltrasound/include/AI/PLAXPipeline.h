#pragma once

#include <ThorError.h>
#include <CircularBuffer.h>
#include <memory>
#include <vector>
#include <string>
#include <mutex>
#include <json/value.h>
#include <jni.h>

class AIModel;
class ScanConverterCL;
class UltrasoundManager;
class CineBuffer;
struct ScanConverterParams;
struct GuidanceJNI;

struct PLAXAutocaptureStatus {
    bool start;
    bool trigger;
    bool failure;
    float score;
    float min;
};

class PLAXPipeline {
public:
    PLAXPipeline();
    PLAXPipeline(const PLAXPipeline&) = delete;
    ~PLAXPipeline();
    PLAXPipeline& operator=(const PLAXPipeline&) = delete;

    ThorStatus init(UltrasoundManager *manager, GuidanceJNI *jni);
    void uninit();

    void reset();
    ThorStatus setFlipX(float flipx);
    ThorStatus setParams(const ScanConverterParams& params);
    ThorStatus process(uint32_t frameNum, int* quality, int* subview);
    Json::Value processIntoJson(JNIEnv *jenv, uint32_t frameNum);
    Json::Value getIQViewJson();

    int numViews() const;
    int quality() const;
    int view() const;
    bool isOptimal() const;

    /// Gets autocapture progress, from 0-1, and optional boolean indicating whether autocapture
    /// should trigger or not
    PLAXAutocaptureStatus getAutocapture(int maxFrames);


public:
    struct IQTransition {
        int from;
        int to;
        float threshold;
    };
    struct Config {
        std::string modelName;
        std::vector<std::string> modelOutputLayers;
        std::vector<std::string> modelOutputTensors;
        int numIQScores;
        int numViewClasses;
        int optimalView;
        int unsureView;
        std::vector<std::string> viewClassNames;

        int smoothingFilterFrames;
        std::vector<IQTransition> transitions;
        std::vector<int> maxQualities;

        /// Minimum number of frames required for autocapture
        int autocaptureMin;
        /// Maximum number of frames required for autocapture
        int autocaptureMax;

        int hysteresisK;
        float hysteresisDiff;

        float minViewConf;
    };

private:

    ThorStatus scanConvert(uint32_t frameNum);
    ThorStatus runModel(JNIEnv *jenv);
    void applySoftmax();
    void runSmoothingFilter();
    void applyIQTransitions();
    void capIQ();
    void hysteresisFilter();
    void checkMinViewConf();

    bool inTopKViews(int view) const;

    /// Get the count of current "good" quality frames
    int getCountGoodFrames() const;
    bool isCurrentIQGood() const;

    Config mConfig;

    std::mutex mMutex;
    bool mIsInit;
    bool mNeedsSetParams;

    CineBuffer *mCineBuffer;
    std::unique_ptr<ScanConverterCL> mScanConverter;
    bool mScanConverterIsInit;

    float mFlipX;
    std::vector<float> mIQConfBuffer;
    std::vector<float> mViewConfBuffer;
    std::vector<float> mIQSmoothingBuffer;
    std::vector<float> mViewSmoothingBuffer;
    int mSmoothingBufferStartIndex;
    int mSmoothingBufferSize;
    std::vector<float> mRunningIQConf;
    std::vector<float> mRunningViewConf;

    /// Previous view for hysteresis filter
    int mPrevView;

    int mIQ;
    int mView;

    CircularBuffer<int> mIQAutocaptureBuffer;
    int mAutocaptureTimeout;

public:
    GuidanceJNI *mJNI;
};