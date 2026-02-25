//
// Copyright 2018 EchoNous Inc.
//
//

#define LOG_TAG "RecorderJni"

#include <jni.h>
#include <ThorUtils.h>
#include <ThorError.h>
#include <UltrasoundManager.h>
#include <CineRecorder.h>
#include <CriticalSection.h>

namespace echonous
{

struct RecorderContextIds
{
    jmethodID   onCompletionId;
} gRecorderContextIds;

static JavaVM*              gJavaVmPtr = nullptr;
static jobject              gJavaRecorder = nullptr;
static CriticalSection      gRecorderLock;

extern UltrasoundManager    gUltrasoundManager;

//-----------------------------------------------------------------------------
static void reportCompletion(ThorStatus completionCode)
{
    JNIEnv*     jenv = nullptr;

    gRecorderLock.enter();
    ALOGD("Reporting recording completion: 0x%x", completionCode);
    // THIS IS NOT NEEDED ANYMORE AFTER M_MODE BEHAVIOR CHANGE
    // get and set TraceLineTime for ScrollMode
    // gUltrasoundManager.getCineViewer().setTraceLineTime(
    //        gUltrasoundManager.getCineRecorder().getTraceLineTime());

    if (nullptr != gJavaVmPtr && nullptr != gJavaRecorder)
    {
        gJavaVmPtr->AttachCurrentThread(&jenv, NULL);
        if (nullptr == jenv)
        {
            ALOGE("Unable to get Android Runtime");
            goto err_ret;
        }
        jenv->CallVoidMethod(gJavaRecorder,
                             gRecorderContextIds.onCompletionId,
                             (jint) completionCode);
        gJavaVmPtr->DetachCurrentThread();
    }

err_ret:
    gRecorderLock.leave();
}

//-----------------------------------------------------------------------------
static void initRecorder(JNIEnv* jenv, jobject obj, jobject thisObj)
{
    UNUSED(obj);

    jenv->GetJavaVM(&gJavaVmPtr);

    gRecorderLock.enter();
    gJavaRecorder = jenv->NewGlobalRef(thisObj);
    gRecorderLock.leave();

    gUltrasoundManager.getCineRecorder().attachCompletionHandler(reportCompletion);
}

//-----------------------------------------------------------------------------
static void terminateRecorder(JNIEnv* jenv, jobject obj)
{
    UNUSED(obj);

    gUltrasoundManager.getCineRecorder().detachCompletionHandler();

    gRecorderLock.enter();
    if (nullptr != gJavaRecorder)
    {
        jenv->DeleteGlobalRef(gJavaRecorder);
        gJavaRecorder = nullptr;
    }
    gRecorderLock.leave();
}

//-----------------------------------------------------------------------------
static jint recordStillImage(JNIEnv* jenv, jobject obj, jstring path)
{
    ThorStatus  retVal = TER_MISC_NOT_IMPLEMENTED;

    UNUSED(obj);

    const char* nativePath = jenv->GetStringUTFChars(path, 0);

    ScrollModeRecordParams  scrollModeRecParams;
    DaEcgRecordParams       daRecordParams;
    DaEcgRecordParams       ecgRecordParams;

    uint32_t curFrameNum = gUltrasoundManager.getCineBuffer().getCurFrameNum();

    if (gUltrasoundManager.getCineViewer().getScrollModeRecordParams(scrollModeRecParams, curFrameNum))
    {
        //set scrollModeParams;
        gUltrasoundManager.getCineRecorder().setScrollModeRecordParams(scrollModeRecParams);
    }

    // set DaOn and EcgOn
    gUltrasoundManager.getCineRecorder().setIsDaOn(gUltrasoundManager.getCineViewer().getIsDAOn());
    gUltrasoundManager.getCineRecorder().setIsEcgOn(gUltrasoundManager.getCineViewer().getIsECGOn());

    if (gUltrasoundManager.getCineViewer().getDARecordParams(daRecordParams, curFrameNum))
    {
        gUltrasoundManager.getCineRecorder().setDaRecordParams(daRecordParams);
    }

    if (gUltrasoundManager.getCineViewer().getECGRecordParams(ecgRecordParams, curFrameNum))
    {
        gUltrasoundManager.getCineRecorder().setEcgRecordParams(ecgRecordParams);
    }

    retVal = gUltrasoundManager.getCineRecorder().recordStillImage(nativePath, curFrameNum);

    jenv->ReleaseStringUTFChars(path, nativePath);

    return(retVal); 
}

//-----------------------------------------------------------------------------
static jint recordRetrospectiveVideo(JNIEnv* jenv,
                                     jobject obj,
                                     jstring path,
                                     jint msec)
{
    ThorStatus  retVal = TER_MISC_NOT_IMPLEMENTED;

    UNUSED(obj);

    const char* nativePath = jenv->GetStringUTFChars(path, 0);

    ScrollModeRecordParams  scrollModeRecParams;
    DaEcgRecordParams       daRecordParams;
    DaEcgRecordParams       ecgRecordParams;

    if (gUltrasoundManager.getCineViewer().getScrollModeRecordParams(scrollModeRecParams))
    {
        //set scrollModeParams;
        gUltrasoundManager.getCineRecorder().setScrollModeRecordParams(scrollModeRecParams);
    }

    // set DaOn and EcgOn
    gUltrasoundManager.getCineRecorder().setIsDaOn(gUltrasoundManager.getCineViewer().getIsDAOn());
    gUltrasoundManager.getCineRecorder().setIsEcgOn(gUltrasoundManager.getCineViewer().getIsECGOn());

    if (gUltrasoundManager.getCineViewer().getDARecordParams(daRecordParams))
    {
        gUltrasoundManager.getCineRecorder().setDaRecordParams(daRecordParams);
    }

    if (gUltrasoundManager.getCineViewer().getECGRecordParams(ecgRecordParams))
    {
        gUltrasoundManager.getCineRecorder().setEcgRecordParams(ecgRecordParams);
    }

    retVal = gUltrasoundManager.getCineRecorder().recordRetrospectiveVideo(nativePath,
                                                                           msec);

    jenv->ReleaseStringUTFChars(path, nativePath);

    return(retVal); 
}

//-----------------------------------------------------------------------------
static jint recordRetrospectiveVideoFrames(JNIEnv* jenv,
                                           jobject obj,
                                           jstring path,
                                           jint frames)
{
    ThorStatus  retVal = TER_MISC_NOT_IMPLEMENTED;

    UNUSED(obj);

    const char* nativePath = jenv->GetStringUTFChars(path, 0);

    ScrollModeRecordParams  scrollModeRecParams;
    DaEcgRecordParams daRecordParams;
    DaEcgRecordParams ecgRecordParams;

    if (gUltrasoundManager.getCineViewer().getScrollModeRecordParams(scrollModeRecParams))
    {
        //set scrollModeParams;
        gUltrasoundManager.getCineRecorder().setScrollModeRecordParams(scrollModeRecParams);
    }

    // set DaOn and EcgOn
    gUltrasoundManager.getCineRecorder().setIsDaOn(gUltrasoundManager.getCineViewer().getIsDAOn());
    gUltrasoundManager.getCineRecorder().setIsEcgOn(gUltrasoundManager.getCineViewer().getIsECGOn());

    if (gUltrasoundManager.getCineViewer().getDARecordParams(daRecordParams))
    {
        gUltrasoundManager.getCineRecorder().setDaRecordParams(daRecordParams);
    }

    if (gUltrasoundManager.getCineViewer().getECGRecordParams(ecgRecordParams))
    {
        gUltrasoundManager.getCineRecorder().setEcgRecordParams(ecgRecordParams);
    }

    retVal = gUltrasoundManager.getCineRecorder().recordRetrospectiveVideoFrames(nativePath,
                                                                                 frames);

    jenv->ReleaseStringUTFChars(path, nativePath);

    return(retVal); 
}

//-----------------------------------------------------------------------------
static jint saveVideoFromCine(JNIEnv* jenv,
                              jobject obj,
                              jstring path,
                              jint startMsec,
                              jint stopMsec)
{
    ThorStatus  retVal = TER_MISC_NOT_IMPLEMENTED;

    UNUSED(obj);

    const char* nativePath = jenv->GetStringUTFChars(path, 0);

    ScrollModeRecordParams  scrollModeRecParams;
    DaEcgRecordParams daRecordParams;
    DaEcgRecordParams ecgRecordParams;

    if (gUltrasoundManager.getCineViewer().getScrollModeRecordParams(scrollModeRecParams))
    {
        //set scrollModeParams;
        gUltrasoundManager.getCineRecorder().setScrollModeRecordParams(scrollModeRecParams);
    }

    // set DaOn and EcgOn
    gUltrasoundManager.getCineRecorder().setIsDaOn(gUltrasoundManager.getCineViewer().getIsDAOn());
    gUltrasoundManager.getCineRecorder().setIsEcgOn(gUltrasoundManager.getCineViewer().getIsECGOn());

    if (gUltrasoundManager.getCineViewer().getDARecordParams(daRecordParams))
    {
        gUltrasoundManager.getCineRecorder().setDaRecordParams(daRecordParams);
    }

    if (gUltrasoundManager.getCineViewer().getECGRecordParams(ecgRecordParams))
    {
        gUltrasoundManager.getCineRecorder().setEcgRecordParams(ecgRecordParams);
    }

    retVal = gUltrasoundManager.getCineRecorder().saveVideoFromCine(nativePath,
                                                                    startMsec,
                                                                    stopMsec);

    jenv->ReleaseStringUTFChars(path, nativePath);

    return(retVal); 
}

//-----------------------------------------------------------------------------
static jint saveVideoFromCineFrames(JNIEnv* jenv,
                                    jobject obj,
                                    jstring path,
                                    jint startFrame,
                                    jint stopFrame)
{
    ThorStatus  retVal = TER_MISC_NOT_IMPLEMENTED;

    UNUSED(obj);

    const char* nativePath = jenv->GetStringUTFChars(path, 0);

    ScrollModeRecordParams  scrollModeRecParams;
    DaEcgRecordParams daRecordParams;
    DaEcgRecordParams ecgRecordParams;

    if (gUltrasoundManager.getCineViewer().getScrollModeRecordParams(scrollModeRecParams))
    {
        //set scrollModeParams;
        gUltrasoundManager.getCineRecorder().setScrollModeRecordParams(scrollModeRecParams);
    }

    // set DaOn and EcgOn
    gUltrasoundManager.getCineRecorder().setIsDaOn(gUltrasoundManager.getCineViewer().getIsDAOn());
    gUltrasoundManager.getCineRecorder().setIsEcgOn(gUltrasoundManager.getCineViewer().getIsECGOn());

    if (gUltrasoundManager.getCineViewer().getDARecordParams(daRecordParams))
    {
        gUltrasoundManager.getCineRecorder().setDaRecordParams(daRecordParams);
    }

    if (gUltrasoundManager.getCineViewer().getECGRecordParams(ecgRecordParams))
    {
        gUltrasoundManager.getCineRecorder().setEcgRecordParams(ecgRecordParams);
    }

    retVal = gUltrasoundManager.getCineRecorder().saveVideoFromCineFrames(nativePath,
                                                                          startFrame,
                                                                          stopFrame);

    jenv->ReleaseStringUTFChars(path, nativePath);

    return(retVal); 
}

//-----------------------------------------------------------------------------
static jint saveVideoFromLiveFrames(JNIEnv *jenv,
                                    jobject obj,
                                    jstring path,
                                    jint startFrame,
                                    jint stopFrame) {
    ThorStatus retVal = TER_MISC_NOT_IMPLEMENTED;

    UNUSED(obj);

    const char *nativePath = jenv->GetStringUTFChars(path, 0);

    ScrollModeRecordParams scrollModeRecParams;
    DaEcgRecordParams daRecordParams;
    DaEcgRecordParams ecgRecordParams;

    if (gUltrasoundManager.getCineViewer().getScrollModeRecordParams(scrollModeRecParams)) {
        //set scrollModeParams;
        gUltrasoundManager.getCineRecorder().setScrollModeRecordParams(scrollModeRecParams);
    }

    // set DaOn and EcgOn
    gUltrasoundManager.getCineRecorder().setIsDaOn(
            gUltrasoundManager.getCineViewer().getIsDAOn());
    gUltrasoundManager.getCineRecorder().setIsEcgOn(
            gUltrasoundManager.getCineViewer().getIsECGOn());

    if (gUltrasoundManager.getCineViewer().getDARecordParams(daRecordParams)) {
        gUltrasoundManager.getCineRecorder().setDaRecordParams(daRecordParams);
    }

    if (gUltrasoundManager.getCineViewer().getECGRecordParams(ecgRecordParams)) {
        gUltrasoundManager.getCineRecorder().setEcgRecordParams(ecgRecordParams);
    }

    retVal = gUltrasoundManager.getCineRecorder().saveVideoFromLiveFrames(nativePath,
                                                                          startFrame,
                                                                          stopFrame);

    jenv->ReleaseStringUTFChars(path, nativePath);

    return (retVal);
}

//-----------------------------------------------------------------------------
static jint updateParamsFile(JNIEnv* jenv, jobject obj, jstring path)
{
    // update cineParams in ThorFile
    ThorStatus  retVal = TER_MISC_NOT_IMPLEMENTED;

    UNUSED(obj);

    const char* nativePath = jenv->GetStringUTFChars(path, 0);

    retVal = gUltrasoundManager.getCineRecorder().updateParamsThorFile(nativePath);

    jenv->ReleaseStringUTFChars(path, nativePath);

    return(retVal);
}

//-----------------------------------------------------------------------------
static JNINativeMethod recorder_method_table[] =
{
    { "nativeInit",
      "(Lcom/echonous/hardware/ultrasound/UltrasoundRecorder;)V",
      (void*) initRecorder
    },
    { "nativeTerminate",
      "()V",
      (void*) terminateRecorder
    },
    { "nativeRecordStillImage",
      "(Ljava/lang/String;)I",
      (void*) recordStillImage
    },
    { "nativeRecordRetrospectiveVideo",
      "(Ljava/lang/String;I)I",
      (void*) recordRetrospectiveVideo
    },
    { "nativeRecordRetrospectiveVideoFrames",
      "(Ljava/lang/String;I)I",
      (void*) recordRetrospectiveVideoFrames
    },
    { "nativeSaveVideoFromCine",
      "(Ljava/lang/String;II)I",
      (void*) saveVideoFromCine
    },
    { "nativeSaveVideoFromLiveFrames",
            "(Ljava/lang/String;II)I",
            (void*) saveVideoFromLiveFrames
    },
    { "nativeSaveVideoFromCineFrames",
      "(Ljava/lang/String;II)I",
      (void*) saveVideoFromCineFrames
    },
    { "nativeUpdateParamsFile",
      "(Ljava/lang/String;)I",
      (void*) updateParamsFile
    },
};

//-----------------------------------------------------------------------------
int register_recorder_methods(JNIEnv* env)
{
    jclass recorderClass = env->FindClass("com/echonous/hardware/ultrasound/UltrasoundRecorder");
    LOG_FATAL_IF(recorderClass == NULL, "Unable to find class com.echonous.hardware.ultrasound.UltrasoundRecorder");

    gRecorderContextIds.onCompletionId = env->GetMethodID(recorderClass, "onCompletion", "(I)V");
    LOG_FATAL_IF(gRecorderContextIds.onCompletionId == NULL, "Unable to find method onCompletion");

    return env->RegisterNatives(recorderClass,
                                recorder_method_table,
                                NELEM(recorder_method_table)); 
}

};

