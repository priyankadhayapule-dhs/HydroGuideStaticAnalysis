//
// Copyright 2017 EchoNous Inc.
//
//

#define LOG_TAG "PlayerJni"

#include <jni.h>
#include <ThorUtils.h>
#include <UltrasoundManager.h>
#include <CriticalSection.h>

namespace echonous
{

struct PlayerIds
{
    jmethodID   onProgressId;
    jmethodID   setRunningStatusId;
} gPlayerIds;

static JavaVM*              gJavaVmPtr = nullptr;
static jobject              gJavaPlayer = nullptr;
static CriticalSection      gPlayerLock;
extern UltrasoundManager    gUltrasoundManager;

//-----------------------------------------------------------------------------
static void reportProgress(uint32_t position)
{
    JNIEnv*     jenv = nullptr;

    gPlayerLock.enter();
    if (nullptr != gJavaVmPtr && nullptr != gJavaPlayer)
    {
        gJavaVmPtr->AttachCurrentThread(&jenv, NULL);
        if (nullptr == jenv)
        {
            ALOGE("Unable to get Android Runtime");
            goto err_ret; 
        }
        jenv->CallVoidMethod(gJavaPlayer, gPlayerIds.onProgressId, (jint) position);
        gJavaVmPtr->DetachCurrentThread();
    }

err_ret:
    gPlayerLock.leave();
}

//-----------------------------------------------------------------------------
static void setRunningStatus(bool isRunning)
{
    JNIEnv *jenv = nullptr;

    gPlayerLock.enter();
    if (nullptr != gJavaVmPtr && nullptr != gJavaPlayer)
    {
        gJavaVmPtr->AttachCurrentThread(&jenv, NULL);
        if (nullptr == jenv)
        {
            ALOGE("Unable to get Android Runtime");
            goto err_ret; 
        }
        jenv->CallVoidMethod(gJavaPlayer, gPlayerIds.setRunningStatusId, (jboolean)isRunning);
        gJavaVmPtr->DetachCurrentThread();
    }

err_ret:
    gPlayerLock.leave();
}

//-----------------------------------------------------------------------------
static void initPlayer(JNIEnv* jenv, jobject obj, jobject thisObj)
{
    UNUSED(obj);

    jenv->GetJavaVM(&gJavaVmPtr);

    gPlayerLock.enter();
    gJavaPlayer = jenv->NewGlobalRef(thisObj);
    gPlayerLock.leave();

    gUltrasoundManager.getCinePlayer().attachCine(reportProgress, setRunningStatus);
}

//-----------------------------------------------------------------------------
static void terminatePlayer(JNIEnv* jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    gUltrasoundManager.getCinePlayer().detachCine();

    gPlayerLock.enter();
    if (nullptr != gJavaPlayer)
    {
        jenv->DeleteGlobalRef(gJavaPlayer);
        gJavaPlayer = nullptr;
    }
    gPlayerLock.leave();
}

//-----------------------------------------------------------------------------
static jint openRawFile(JNIEnv* jenv, jobject obj, jstring srcPath)
{
    ThorStatus  retVal;

    UNUSED(obj);

    const char* nativePath = jenv->GetStringUTFChars(srcPath, 0);

    retVal = gUltrasoundManager.getCinePlayer().openRawFile(nativePath);

    jenv->ReleaseStringUTFChars(srcPath, nativePath);

    return(retVal);
}

//-----------------------------------------------------------------------------
static void closeRawFile(JNIEnv* jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    gUltrasoundManager.getCinePlayer().closeRawFile();
}

//-----------------------------------------------------------------------------
static void startPlayer(JNIEnv* jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    gUltrasoundManager.getCinePlayer().start();
}

//-----------------------------------------------------------------------------
static void stopPlayer(JNIEnv* jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    gUltrasoundManager.getCinePlayer().stop();
}


//-----------------------------------------------------------------------------
static void pausePlayer(JNIEnv* jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    gUltrasoundManager.getCinePlayer().pause();
}

//-----------------------------------------------------------------------------
static void setStartPosition(JNIEnv* jenv, jobject obj, jint msec)
{
    UNUSED(jenv);
    UNUSED(obj);

    gUltrasoundManager.getCinePlayer().setStartPosition(msec);
}

//-----------------------------------------------------------------------------
static void setEndPosition(JNIEnv* jenv, jobject obj, jint msec)
{
    UNUSED(jenv);
    UNUSED(obj);

    gUltrasoundManager.getCinePlayer().setEndPosition(msec);
}

//-----------------------------------------------------------------------------
static void setStartFrame(JNIEnv* jenv, jobject obj, jint frame)
{
    UNUSED(jenv);
    UNUSED(obj);

    gUltrasoundManager.getCinePlayer().setStartFrame(frame);
}

//-----------------------------------------------------------------------------
static jint getStartFrame(JNIEnv* jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    uint32_t startFrame = gUltrasoundManager.getCinePlayer().getStartFrame();
    return(startFrame);
}

//-----------------------------------------------------------------------------
static void setEndFrame(JNIEnv* jenv, jobject obj, jint frame)
{
    UNUSED(jenv);
    UNUSED(obj);

    gUltrasoundManager.getCinePlayer().setEndFrame(frame);
}



//-----------------------------------------------------------------------------
static jint getEndFrame(JNIEnv* jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    uint32_t endFrame = gUltrasoundManager.getCinePlayer().getEndFrame();
    return(endFrame);
}


//-----------------------------------------------------------------------------
static jint getFrameInterval(JNIEnv* jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);
    uint32_t frameInterval = gUltrasoundManager.getCinePlayer().getFrameInterval();
    return(frameInterval);
}

//-----------------------------------------------------------------------------
static void nextFrame(JNIEnv* jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    gUltrasoundManager.getCinePlayer().nextFrame();
}

//-----------------------------------------------------------------------------
static void previousFrame(JNIEnv* jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    gUltrasoundManager.getCinePlayer().previousFrame();
}

//-----------------------------------------------------------------------------
static void seekTo(JNIEnv* jenv, jobject obj, jint msec)
{
    UNUSED(jenv);
    UNUSED(obj);

    gUltrasoundManager.getCinePlayer().seekTo(msec);
}

//-----------------------------------------------------------------------------
static void seekToFrame(JNIEnv* jenv, jobject obj, jint frame, jboolean progressCallbck)
{
    UNUSED(jenv);
    UNUSED(obj);

    gUltrasoundManager.getCinePlayer().seekToFrame(frame, progressCallbck);
}


//-----------------------------------------------------------------------------
static jint getDuration(JNIEnv* jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    uint32_t    duration = gUltrasoundManager.getCinePlayer().getDuration();

    ALOGD("Duration is %u", duration);

    //return(gUltrasoundManager.getCinePlayer().getDuration());
    return(duration);
}

//-----------------------------------------------------------------------------
static jint getFrameCount(JNIEnv* jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    uint32_t    frameCount = gUltrasoundManager.getCinePlayer().getFrameCount();

    ALOGD("FrameCount is %u", frameCount);

    //return(gUltrasoundManager.getCinePlayer().getFrameCount());
    return(frameCount);
}

//-----------------------------------------------------------------------------
static jint getCurrentPosition(JNIEnv* jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    return(gUltrasoundManager.getCinePlayer().getCurrentPosition());
}

//-----------------------------------------------------------------------------
static jint getCurrentFrameNo(JNIEnv* jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    return(gUltrasoundManager.getCinePlayer().getCurrentFrameNo());
}

//-----------------------------------------------------------------------------
static jboolean isLooping(JNIEnv* jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    return(gUltrasoundManager.getCinePlayer().isLooping() ? JNI_TRUE : JNI_FALSE);
}

//-----------------------------------------------------------------------------
static void setLooping(JNIEnv* jenv, jobject obj, jboolean doLooping)
{
    UNUSED(jenv);
    UNUSED(obj);

    gUltrasoundManager.getCinePlayer().setLooping(JNI_TRUE == doLooping ? true : false);
}

//-----------------------------------------------------------------------------
static void setPlaybackSpeed(JNIEnv* jenv, jobject obj, jint speed)
{
    UNUSED(jenv);
    UNUSED(obj);

    gUltrasoundManager.getCinePlayer().setPlaybackSpeed((CinePlayer::Speed) speed);
}

//-----------------------------------------------------------------------------
static jint getPlaybackSpeed(JNIEnv* jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    return(gUltrasoundManager.getCinePlayer().getPlaybackSpeed());
}

//-----------------------------------------------------------------------------
static JNINativeMethod player_method_table[] =
{
    { "nativeInit",
      "(Lcom/echonous/hardware/ultrasound/UltrasoundPlayer;)V",
      (void*) initPlayer
    },
    { "nativeTerminate",
      "()V",
      (void*) terminatePlayer
    },
    { "nativeOpenRawFile",
      "(Ljava/lang/String;)I",
      (void*) openRawFile
    },
    { "nativeCloseRawFile",
      "()V",
      (void*) closeRawFile
    },
    { "nativeStart",
      "()V",
      (void*) startPlayer
    },
    { "nativeStop",
      "()V",
      (void*) stopPlayer
    },
    { "nativePause",
      "()V",
      (void*) pausePlayer
    },
    { "nativeSetStartPosition",
      "(I)V",
      (void*) setStartPosition
    },
    { "nativeSetEndPosition",
      "(I)V",
      (void*) setEndPosition
    },
    { "nativeSetStartFrame",
      "(I)V",
      (void*) setStartFrame
    },
    { "nativeGetStartFrame",
      "()I",
      (void*) getStartFrame
    },
    { "nativeSetEndFrame",
      "(I)V",
      (void*) setEndFrame
    },
    { "nativeGetEndFrame",
      "()I",
      (void*) getEndFrame
    },
    { "nativeGetFrameInterval",
      "()I",
      (void*) getFrameInterval
    },
    { "nativeNextFrame",
      "()V",
      (void*) nextFrame
    },
    { "nativePreviousFrame",
      "()V",
      (void*) previousFrame
    },
    { "nativeSeekTo",
      "(I)V",
      (void*) seekTo
    },
    { "nativeSeekToFrame",
      "(IZ)V",
      (void*) seekToFrame
    },
    { "nativeGetDuration",
      "()I",
      (void*) getDuration
    },
    { "nativeGetFrameCount",
      "()I",
      (void*) getFrameCount
    },
    { "nativeGetCurrentPosition",
      "()I",
      (void*) getCurrentPosition
    },
    { "nativeGetCurrentFrameNo",
      "()I",
      (void*) getCurrentFrameNo
    },
    { "nativeIsLooping",
      "()Z",
      (void*) isLooping
    },
    { "nativeSetLooping",
      "(Z)V",
      (void*) setLooping
    },
    { "nativeSetPlaybackSpeed",
      "(I)V",
      (void*) setPlaybackSpeed
    },
    { "nativeGetPlaybackSpeed",
      "()I",
      (void*) getPlaybackSpeed
    },
};

//-----------------------------------------------------------------------------
int register_player_methods(JNIEnv* env)
{
    jclass playerClass = env->FindClass("com/echonous/hardware/ultrasound/UltrasoundPlayer");
    LOG_FATAL_IF(playerClass == NULL, "Unable to find class com.echonous.hardware.ultrasound.UltrasoundPlayer");

    gPlayerIds.onProgressId = env->GetMethodID(playerClass, "onProgress", "(I)V");
    LOG_FATAL_IF(gPlayerIds.onProgressId == NULL, "Unable to find method onProgress");

    gPlayerIds.setRunningStatusId = env->GetMethodID(playerClass, "setRunningStatus", "(Z)V");
    LOG_FATAL_IF(gPlayerIds.setRunningStatusId == NULL, "Unable to find method setRunningStatus");

    return env->RegisterNatives(playerClass,
                                player_method_table,
                                NELEM(player_method_table)); 
}

};

