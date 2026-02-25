//
// Copyright 2021 EchoNous Inc.
//
//

#define LOG_TAG "AutoControlHandler"

#include <AutoControlManager.h>
#include <MimicAutoController.h>

//-----------------------------------------------------------------------------
AutoControlManager::AutoControlManager() :
    mOnAutoOrganHandlerPtr(nullptr),
    mOnAutoDepthHandlerPtr(nullptr),
    mOnAutoGainHandlerPtr(nullptr),
    mAutoPresetDebugEnabled(false),
    mAutoGainDebugEnabled(false),
    mAutoDepthDebugEnabled(false),
    mAutoPresetEnabled(true),
    mAutoGainEnabled(false),
    mAutoDepthEnabled(false),
    mAutoPresetDebugValue(0),
    mAutoGainDebugValue(0),
    mAutoDepthDebugValue(0),
    mAutoPresetDebugFrameCount(0),
    mAutoGainDebugFrameCount(0),
    mAutoDepthDebugFrameCount(0)
{
}

//-----------------------------------------------------------------------------
AutoControlManager::~AutoControlManager()
{
    ALOGD("%s", __func__);
    terminate();
}

//-----------------------------------------------------------------------------
ThorStatus AutoControlManager::init()
{
    ThorStatus      retVal = THOR_ERROR;

    // placeholder for init
    retVal = THOR_OK;

err_ret:
    return(retVal);
}

//-----------------------------------------------------------------------------
void AutoControlManager::terminate()
{
    ALOGD("%s", __func__);
}

//-----------------------------------------------------------------------------
void AutoControlManager::attachOnAutoOrgan(CallbackHandler callbackHandlerPtr)
{
    mLock.enter();
    mOnAutoOrganHandlerPtr = callbackHandlerPtr;
    mLock.leave();
}

//-----------------------------------------------------------------------------
void AutoControlManager::detachOnAutoOrgan()
{
    mLock.enter();
    mOnAutoOrganHandlerPtr = nullptr;
    mLock.leave();
}

//-----------------------------------------------------------------------------
void AutoControlManager::attachOnAutoDepth(CallbackHandler callbackHandlerPtr)
{
    mLock.enter();
    mOnAutoDepthHandlerPtr = callbackHandlerPtr;
    mLock.leave();
}

//-----------------------------------------------------------------------------
void AutoControlManager::detachOnAutoDepth()
{
    mLock.enter();
    mOnAutoDepthHandlerPtr = nullptr;
    mLock.leave();
}

//-----------------------------------------------------------------------------
void AutoControlManager::attachOnAutoGain(CallbackHandler callbackHandlerPtr)
{
    mLock.enter();
    mOnAutoGainHandlerPtr = callbackHandlerPtr;
    mLock.leave();
}

//-----------------------------------------------------------------------------
void AutoControlManager::detachOnAutoGain()
{
    mLock.enter();
    mOnAutoGainHandlerPtr = nullptr;
    mLock.leave();
}

//-----------------------------------------------------------------------------
void AutoControlManager::setAutoOrgan(int32_t organId)
{
    if(mAutoPresetDebugEnabled) {
        mAutoPresetDebugEnabled = false;
        mAutoPresetDebugFrameCount = 0;
    }
    if (mOnAutoOrganHandlerPtr != nullptr)
        mOnAutoOrganHandlerPtr(organId);
    else
        ALOGD("%s - nullptr", __func__);
}

//-----------------------------------------------------------------------------
void AutoControlManager::setAutoGain(int32_t gainVal)
{
    if(mAutoGainDebugEnabled) {
        mAutoGainDebugEnabled = false;
        mAutoGainDebugFrameCount = 0;
    }
    if (mOnAutoGainHandlerPtr != nullptr) {
        mOnAutoGainHandlerPtr(gainVal);
    }
    else
        ALOGD("%s - nullptr", __func__);
}

//-----------------------------------------------------------------------------
void AutoControlManager::setAutoDepth(int32_t depthAdjVal)
{
    if(mAutoDepthDebugEnabled) {
        mAutoDepthDebugEnabled = false;
        mAutoDepthDebugFrameCount = 0;
    }
    if (mOnAutoDepthHandlerPtr != nullptr)
        mOnAutoDepthHandlerPtr(depthAdjVal);
    else
        ALOGD("%s - nullptr", __func__);
}

//-----------------------------------------------------------------------------
void AutoControlManager::setAutoDepthEnabled(bool enabled)
{
    mLock.enter();
    mAutoDepthEnabled = enabled;
    mLock.leave();
}

//-----------------------------------------------------------------------------
void AutoControlManager::setAutoGainEnabled(bool enabled)
{
    mLock.enter();
    mAutoGainEnabled = enabled;
    mLock.leave();
}

//-----------------------------------------------------------------------------
void AutoControlManager::setAutoOrganEnabled(bool enabled)
{
    mLock.enter();
    mAutoPresetEnabled = enabled;
    mLock.leave();
}

//-----------------------------------------------------------------------------
// Disable preset processing for frameCount frames, then trigger setAutoPreset call with
// given preset
void AutoControlManager::setAutoOrganDebug(uint32_t frameCount, int preset)
{
    mLock.enter();
    mAutoPresetDebugEnabled = true;
    mAutoPresetDebugFrameCount = frameCount;
    mAutoPresetDebugValue = preset;
    mLock.leave();
}

//-----------------------------------------------------------------------------
// Disable depth processing for frameCount frames, then trigger setAutoDepth call with
// given depth
void AutoControlManager::setAutoDepthDebug(uint32_t frameCount, int depth)
{
    mLock.enter();
    mAutoDepthDebugEnabled = true;
    mAutoDepthDebugFrameCount = frameCount;
    mAutoDepthDebugValue = depth;
    mLock.leave();
}

//-----------------------------------------------------------------------------
// Disable gain processing for frameCount frames, then trigger setAutoGain call with
// given gain
void AutoControlManager::setAutoGainDebug(uint32_t frameCount, int gain)
{
    mLock.enter();
    mAutoGainDebugEnabled = true;
    mAutoGainDebugFrameCount = frameCount;
    mAutoGainDebugValue = gain;
    mLock.leave();
}