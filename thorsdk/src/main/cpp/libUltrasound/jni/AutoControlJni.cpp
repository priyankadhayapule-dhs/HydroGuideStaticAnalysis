//
// Copyright 2018 EchoNous Inc.
//
//  It is assumed that all parameters passed to the JNI functions exist and
//  are valid.  The parameters should be verified by the upper layer
//  Ultrasound Manager (in Java space).
//

#define LOG_TAG "AutoControlJni"

#include <jni.h>

#include <ThorUtils.h>
#include <ThorError.h>
#include <UltrasoundManager.h>
#include <AutoControlManager.h>
#include <CriticalSection.h>
#include <MimicAutoController.h>
#include <thread>

namespace echonous
{

struct ACManagerContextIds
{
    jmethodID   onAutoOrganId;
    jmethodID   onAutoGainId;
    jmethodID   onAutoDepthId;
} gACManagerContextIds;

static JavaVM*              gJavaVmPtr = nullptr;
static jobject              gJavaAutoControl = nullptr;
static CriticalSection      gAutoControlLock;
static MimicAutoController  *gMimicAutoController;

extern UltrasoundManager    gUltrasoundManager;

//-----------------------------------------------------------------------------
static void setAutoPresetEnabled(JNIEnv* jenv, jobject obj, jboolean enabled)
{
    UNUSED(obj);
    LOGD("JNI setAutoPresetEnabled %d", enabled);
    jenv->GetJavaVM(&gJavaVmPtr);
    gUltrasoundManager.getAIManager().setAutoPresetEnabled(enabled);
    LOG_FATAL_IF(gUltrasoundManager == NULL, "Unable com.echonous.hardware.ultrasound.UltrasoundManager");
}
//-----------------------------------------------------------------------------
static void setAutoDepthEnabled(JNIEnv* jenv, jobject obj,jboolean enabled)
{
    UNUSED(obj);
    LOGD("JNI setAutoDepthEnabled %d", enabled);
    jenv->GetJavaVM(&gJavaVmPtr);
    gUltrasoundManager.getAutoControlManager().setAutoDepthEnabled(enabled);
    LOG_FATAL_IF(gUltrasoundManager == NULL, "Unable com.echonous.hardware.ultrasound.UltrasoundManager");
}
//-----------------------------------------------------------------------------
static void setAutoGainEnabled(JNIEnv* jenv, jobject obj,jboolean enabled)
{
    UNUSED(obj);
    LOGD("JNI setAutoGainEnabled %d", enabled);
    jenv->GetJavaVM(&gJavaVmPtr);
    gUltrasoundManager.getAutoControlManager().setAutoGainEnabled(enabled);
    LOG_FATAL_IF(gUltrasoundManager == NULL, "Unable com.echonous.hardware.ultrasound.UltrasoundManager");
}
//-----------------------------------------------------------------------------
static void setAutoOrgan(jint organId)
{
    JNIEnv *jenv = nullptr;

    gAutoControlLock.enter();
    ALOGD("callback auto organ: %d", organId);
    if (nullptr != gJavaVmPtr && nullptr != gJavaAutoControl)
    {
        gJavaVmPtr->AttachCurrentThread(&jenv, NULL);
        if (nullptr == jenv)
        {
            ALOGE("Unable to get Android Runtime");
            goto err_ret;
        }
        jenv->CallVoidMethod(gJavaAutoControl,
                             gACManagerContextIds.onAutoOrganId,
                             (jint)organId);
        gJavaVmPtr->DetachCurrentThread();
    }

err_ret:
    gAutoControlLock.leave();
}

//-----------------------------------------------------------------------------
static void setAutoDepth(jint depthAdjVal)
{
    JNIEnv *jenv = nullptr;

    gAutoControlLock.enter();
    ALOGD("callback auto depth adjustment: %d", depthAdjVal);
    if (nullptr != gJavaVmPtr && nullptr != gJavaAutoControl)
    {
        gJavaVmPtr->AttachCurrentThread(&jenv, NULL);
        if (nullptr == jenv)
        {
            ALOGE("Unable to get Android Runtime");
            goto err_ret;
        }
        jenv->CallVoidMethod(gJavaAutoControl,
                             gACManagerContextIds.onAutoDepthId,
                             (jint)depthAdjVal);
        gJavaVmPtr->DetachCurrentThread();
    }

    err_ret:
    gAutoControlLock.leave();
}

//-----------------------------------------------------------------------------
static void setAutoGain(jint gainVal)
{
    JNIEnv *jenv = nullptr;

    gAutoControlLock.enter();
    ALOGD("callback auto gain delta: %d", gainVal);
    if (nullptr != gJavaVmPtr && nullptr != gJavaAutoControl)
    {
        gJavaVmPtr->AttachCurrentThread(&jenv, NULL);
        if (nullptr == jenv)
        {
            ALOGE("Unable to get Android Runtime");
            goto err_ret;
        }
        jenv->CallVoidMethod(gJavaAutoControl,
                             gACManagerContextIds.onAutoGainId,
                             (jint)gainVal);
        gJavaVmPtr->DetachCurrentThread();
    }

    err_ret:
    gAutoControlLock.leave();
}

//-----------------------------------------------------------------------------
static void initAutoControl(JNIEnv *jenv, jobject obj, jobject thisObj)
{
    UNUSED(obj);

    jenv->GetJavaVM(&gJavaVmPtr);

    gAutoControlLock.enter();
    gJavaAutoControl = jenv->NewGlobalRef(thisObj);
    gAutoControlLock.leave();

    gUltrasoundManager.getAutoControlManager().attachOnAutoOrgan(setAutoOrgan);
    gUltrasoundManager.getAutoControlManager().attachOnAutoDepth(setAutoDepth);
    gUltrasoundManager.getAutoControlManager().attachOnAutoGain(setAutoGain);
}

//-----------------------------------------------------------------------------
static void terminateAutoControl(JNIEnv *jenv, jobject obj)
{
    UNUSED(obj);

    gUltrasoundManager.getAutoControlManager().detachOnAutoOrgan();
    gUltrasoundManager.getAutoControlManager().detachOnAutoGain();
    gUltrasoundManager.getAutoControlManager().detachOnAutoDepth();

    gMimicAutoController->killMimicAuto();

    gAutoControlLock.enter();
    if (nullptr != gJavaAutoControl)
    {
        jenv->DeleteGlobalRef(gJavaAutoControl);
        gJavaAutoControl = nullptr;
    }
    gAutoControlLock.leave();
}

//-----------------------------------------------------------------------------
static void mimicAutoControl(JNIEnv *jenv, jobject obj,
                             jboolean mimic_gain, jboolean mimic_depth, jboolean mimic_preset) {
    gMimicAutoController = new MimicAutoController();
    gMimicAutoController->attachTestAutoControls(
            gUltrasoundManager.getAutoControlManager(),
            mimic_gain,
            mimic_depth,
            mimic_preset);
}
static void setDebugPresetJNI(JNIEnv *jenv, jobject obj, jint frameNum, jint preset)
{
    jenv->GetJavaVM(&gJavaVmPtr);
    UNUSED(obj);
    gUltrasoundManager.getAutoControlManager().setAutoOrganDebug(frameNum, preset);
    LOG_FATAL_IF(gUltrasoundManager == NULL, "Unable com.echonous.hardware.ultrasound.UltrasoundManager");
}

static void setDebugGainJNI(JNIEnv *jenv, jclass obj, jint frameNum, jint gain)
{
    jenv->GetJavaVM(&gJavaVmPtr);
    UNUSED(obj);
    gUltrasoundManager.getAutoControlManager().setAutoGainDebug(frameNum, gain);
    LOG_FATAL_IF(gUltrasoundManager == NULL, "Unable com.echonous.hardware.ultrasound.UltrasoundManager");
}

static void setDebugDepthJNI(JNIEnv *jenv, jclass obj, jint frameNum, jint depth)
{
    jenv->GetJavaVM(&gJavaVmPtr);
    UNUSED(obj);
    gUltrasoundManager.getAutoControlManager().setAutoGainDebug(frameNum, depth);
    LOG_FATAL_IF(gUltrasoundManager == NULL, "Unable com.echonous.hardware.ultrasound.UltrasoundManager");
}
//-----------------------------------------------------------------------------
static JNINativeMethod autocontrol_method_table[] =
{
        {"nativeInit",
                "(Lcom/echonous/hardware/ultrasound/AutoControl;)V",
                (void *) initAutoControl
                },
        {"nativeTerminate",
                "()V",
                (void *) terminateAutoControl
                },
        {
         "mimicAutoControl",
                "(ZZZ)V",
                (void *) mimicAutoControl
                },
        {
          "setAutoPresetEnabled",
          "(Z)V",
                (void*) setAutoPresetEnabled
            },
        {
                "setAutoGainEnabled",
                "(Z)V",
                (void*) setAutoGainEnabled
            },
        {
                "setAutoDepthEnabled",
                "(Z)V",
                (void*) setAutoDepthEnabled
            },
        {
                "setDebugPresetJNI",
                "(II)V",
                (void*) setDebugPresetJNI
            },
        {
                "setDebugGainJNI",
                "(II)V",
                (void*) setDebugGainJNI
            },
        {
                "setDebugDepthJNI",
                "(II)V",
                (void*) setDebugDepthJNI
            },
};

//-----------------------------------------------------------------------------
int register_autocontrol_methods(JNIEnv *env)
{
    jclass autoControlClass = env->FindClass("com/echonous/hardware/ultrasound/AutoControl");
    LOG_FATAL_IF(autoControlClass == NULL, "Unable to find class com.echonous.hardware.ultrasound.AutoControl");

    gACManagerContextIds.onAutoOrganId = env->GetMethodID(autoControlClass, "setAutoOrgan", "(I)V");
    LOG_FATAL_IF(gACManagerContextIds.onAutoOrganId == NULL, "Unable to find method onCompletion");

    gACManagerContextIds.onAutoDepthId = env->GetMethodID(autoControlClass, "setAutoDepth", "(I)V");
    LOG_FATAL_IF(gACManagerContextIds.onAutoDepthId == NULL, "Unable to find method onCompletion");

    gACManagerContextIds.onAutoGainId = env->GetMethodID(autoControlClass, "setAutoGain", "(I)V");
    LOG_FATAL_IF(gACManagerContextIds.onAutoGainId == NULL, "Unable to find method onCompletion");

    return env->RegisterNatives(autoControlClass,
                                autocontrol_method_table,
                                NELEM(autocontrol_method_table));
}

}