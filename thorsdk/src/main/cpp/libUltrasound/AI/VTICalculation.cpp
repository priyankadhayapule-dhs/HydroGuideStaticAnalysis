//
// Copyright 2023 EchoNous Inc.
//
// Implementation of VTI Calculation (native side)

#define LOG_TAG "VTICalculation"

#include <ThorUtils.h>
#include <VTICalculation.h>
#include <VTICalculationTask.h>

struct AttachJNIVTITask: public AITask {
    VtiNativeJni *mJNI;
    AttachJNIVTITask(AIManager *manager, VtiNativeJni *jni) : AITask(manager), mJNI(jni) {}
    virtual void run() {
        mJNI->attachToThread();
    }
};
ThorStatus VTICalculation::init(AIManager *manager, void* javaEnv, void* javaContext)
{
    mManager = manager;
    mVtiJni = new VtiNativeJni(static_cast<JNIEnv*>(javaEnv), static_cast<jobject>(javaContext));
    // Send the JNI structure to the worker thread (WHICH NOW MUST BE SINGLETON)
    // Task immediately attaches it to the current thread
    // Worker thread is AIManagerWorker, from AIManager.cpp
    LOGD("[ VTITASK ] Calculation Init");
    manager->push(std::make_unique<AttachJNIVTITask>(manager, mVtiJni));
    mCompletionCallback = nullptr;
    return THOR_OK;
}

VTICalculation::VTICalculation()
{
    mManager == nullptr;
    mUser = nullptr;
    reset();
}

void VTICalculation::reset()
{
    // reset / clear
    mVtiPeaks.clear();
    LOGD("VTICalculation::reset");
}

void VTICalculation::setCompletionCallback(void (*cb)(void*, ThorStatus))
{
    mCompletionCallback = cb;
}

void VTICalculation::notifyComplete(ThorStatus statusCode)
{
    LOGD("Completion Callback with code: %008x", statusCode);
    if (mCompletionCallback)
        mCompletionCallback(mUser, statusCode);
    else
        LOGE("No Completion callback set");
}

std::unique_ptr<AITask> VTICalculation::createVTICalculationTask(uint32_t classId, void* user)
{
    mUser = user;

    // clear result vectors
    mVtiPeaks.clear();

    return std::unique_ptr<AITask>(new VTICalculationTask(mManager, this, classId));
}

void VTICalculation::setCalState(uint32_t imgMode, uint32_t frameNum, uint32_t numSamples, uint32_t baselineShiftIdx,
                                 bool baselineInvert, uint64_t timeStamp, uint32_t classId)
{
    mImgMode = imgMode;
    mFrameNum = frameNum;
    mNumSamples = numSamples;
    mBaselineShiftIndex = baselineShiftIdx;
    mBaselineInvert = baselineInvert;
    mTimeStamp = timeStamp;
    mClassId = classId;
}

// return true when matched
bool VTICalculation::compareCalState(uint32_t imgMode, uint32_t frameNum, uint32_t numSamples,
                                     uint32_t baselineShiftIdx, bool baselineInvert, uint64_t timeStamp, uint32_t classId)
{
    bool ret = false;

    if ( (imgMode == mImgMode) && (frameNum == mFrameNum) && (numSamples == mNumSamples) && (baselineShiftIdx == mBaselineShiftIndex)
        && (timeStamp == mTimeStamp) && (classId == mClassId))
        ret = true;

    return ret;
}

// return true when matched
bool VTICalculation::compareCalState(uint32_t classId)
{
    bool ret = false;
    auto cb = mManager->getCineBuffer();
    auto params = cb->getParams();

    uint32_t imgMode = params.imagingMode;
    uint32_t baselineShiftIdx = params.dopplerBaselineShiftIndex;
    bool baselineInvert = params.dopplerBaselineInvert;

    int inputFrameType;
    if (params.imagingMode == IMAGING_MODE_CW)
        inputFrameType = TEX_FRAME_TYPE_CW;
    else
        inputFrameType = TEX_FRAME_TYPE_PW;

    uint8_t* texPtr = cb->getFrame(inputFrameType, DAU_DATA_TYPE_TEX, CineBuffer::FrameContents::IncludeHeader);
    CineBuffer::CineFrameHeader* cineHeaderPtr = reinterpret_cast<CineBuffer::CineFrameHeader*>(texPtr);
    uint32_t frameNum = cineHeaderPtr->frameNum;
    uint32_t numSamples = cineHeaderPtr->numSamples;
    uint64_t timeStamp = cineHeaderPtr->timeStamp;

    if ( (imgMode == mImgMode) && (frameNum == mFrameNum) && (numSamples == mNumSamples) && (baselineShiftIdx == mBaselineShiftIndex)
         && (timeStamp == mTimeStamp) && (mClassId == classId))
        ret = true;

    return ret;
}

VtiNativeJni* VTICalculation::getNativeJNI()
{
    return mVtiJni;
}

