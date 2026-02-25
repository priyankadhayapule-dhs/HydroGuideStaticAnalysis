//
// Copyright 2021 EchoNous Inc.
//
//

#pragma once

#include <ThorUtils.h>
#include <ThorError.h>
#include <CriticalSection.h>

typedef void (*CallbackHandler)(int32_t);

class AutoControlManager
{
public: // Methods
    explicit AutoControlManager();
    ~AutoControlManager();

    void attachOnAutoOrgan(CallbackHandler callbackHandlerPtr);
    void detachOnAutoOrgan();

    void attachOnAutoDepth(CallbackHandler callbackHandlerPtr);
    void detachOnAutoDepth();

    void attachOnAutoGain(CallbackHandler callbackHandlerPtr);
    void detachOnAutoGain();

    void setAutoOrgan(int32_t organId);
    void setAutoGain(int32_t gainVal);
    void setAutoDepth(int32_t depthAdjVal);

    void setAutoGainEnabled(bool enabled);
    void setAutoDepthEnabled(bool enabled);
    void setAutoPresetEnabled(bool enabled);

    bool getAutoGainEnabled();
    bool getAutoDepthEnabled();
    bool getAutoPresetEnabled();
    bool getActive();
    
    bool getAutoPresetMockEnabled();
    int getAutoPresetMockClass();
    float getAutoPresetMockConfidence();

    ThorStatus init();
    void terminate();

    void setAutoPresetMock(int label, float confidence);
    void disableAutoPresetMock();

    void setAutoOrganDebug(uint32_t frameCount, int preset);
    void setAutoDepthDebug(uint32_t frameCount, int depth);
    void setAutoGainDebug(uint32_t frameCount, int gain);
private: // Methods


private: // Properties
    CallbackHandler         mOnAutoOrganHandlerPtr;
    CallbackHandler         mOnAutoDepthHandlerPtr;
    CallbackHandler         mOnAutoGainHandlerPtr;

    CriticalSection         mLock;

    bool                    mAutoDepthEnabled;
    bool                    mAutoDepthDebugEnabled;
    bool                    mAutoGainEnabled;
    bool                    mAutoGainDebugEnabled;
    bool                    mAutoPresetEnabled;
    bool                    mAutoPresetDebugEnabled;

    bool                    mAutoPresetMockEnabled;
    int                     mAutoPresetMockLabel;
    float                   mAutoPresetMockConfidence;

    int                     mAutoGainDebugFrameCount;
    int                     mAutoDepthDebugFrameCount;
    int                     mAutoPresetDebugFrameCount;
    int                     mAutoGainDebugValue;
    int                     mAutoDepthDebugValue;
    int                     mAutoPresetDebugValue;
};

