//
// Copyright 2021 EchoNous Inc.
//
//

#define LOG_TAG "AutoControlHandler"

#include "AutoPreset/AutoControlManager.h"

//-----------------------------------------------------------------------------
AutoControlManager::AutoControlManager() :
    mOnAutoOrganHandlerPtr(nullptr),
    mOnAutoDepthHandlerPtr(nullptr),
    mOnAutoGainHandlerPtr(nullptr),
    mAutoPresetEnabled(false),
    mAutoPresetMockLabel(4), //noise
    mAutoPresetMockConfidence(0.f)
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
    if (mOnAutoOrganHandlerPtr != nullptr)
        mOnAutoOrganHandlerPtr(organId);
    else
        ALOGD("[ ADGP ] %s - nullptr (%d)", __func__, organId);
}

//-----------------------------------------------------------------------------
void AutoControlManager::setAutoGain(int32_t gainVal)
{
    if (mOnAutoGainHandlerPtr != nullptr)
        mOnAutoGainHandlerPtr(gainVal);
    else
        ALOGD("[ ADGP ] %s - nullptr (%d)", __func__, gainVal);
}

//-----------------------------------------------------------------------------
void AutoControlManager::setAutoDepth(int32_t depthAdjVal)
{
    if (mOnAutoDepthHandlerPtr != nullptr)
        mOnAutoDepthHandlerPtr(depthAdjVal);
    else
        ALOGD("[ ADGP ] %s - nullptr (%d)", __func__, depthAdjVal);
}

//-----------------------------------------------------------------------------
void AutoControlManager::setAutoDepthEnabled(bool enabled)
{
    mAutoDepthEnabled = enabled;
}

//-----------------------------------------------------------------------------
void AutoControlManager::setAutoGainEnabled(bool enabled)
{
    mAutoGainEnabled = enabled;
}

//-----------------------------------------------------------------------------
void AutoControlManager::setAutoPresetEnabled(bool enabled)
{
    mAutoPresetEnabled = enabled;
}

//-----------------------------------------------------------------------------
bool AutoControlManager::getAutoGainEnabled()
{
    return mAutoGainEnabled;
}

//-----------------------------------------------------------------------------
bool AutoControlManager::getAutoDepthEnabled()
{
    return mAutoDepthEnabled;
}

//-----------------------------------------------------------------------------
bool AutoControlManager::getAutoPresetEnabled()
{
    return mAutoPresetEnabled;
}

void AutoControlManager::setAutoPresetMock(int anatomyClass, float certainty)
{
    mAutoPresetMockLabel = anatomyClass;
    mAutoPresetMockConfidence = certainty;
    mAutoPresetMockEnabled = true;
}

void AutoControlManager::disableAutoPresetMock()
{
    mAutoPresetMockEnabled = false;
}

bool AutoControlManager::getAutoPresetMockEnabled()
{
    return mAutoPresetMockEnabled;
}

int AutoControlManager::getAutoPresetMockClass()
{
    return mAutoPresetMockLabel;
}

float AutoControlManager::getAutoPresetMockConfidence()
{
    return mAutoPresetMockConfidence;
}

bool AutoControlManager::getActive()
{
    return mAutoGainEnabled || mAutoDepthEnabled || mAutoPresetEnabled;
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