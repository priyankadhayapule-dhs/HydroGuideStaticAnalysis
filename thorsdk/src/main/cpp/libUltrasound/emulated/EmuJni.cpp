//
// Copyright 2016 EchoNous Inc.
//
//

#define LOG_TAG "DauEmulatedJni"

#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <jni.h>
#include <android/native_window.h> // requires ndk r5 or newer
#include <android/native_window_jni.h> // requires ndk r5 or newer
#include <android/asset_manager_jni.h>

#include <jni.h>
#include <sys/ioctl.h>

#include <EmuDau.h>
#include <UltrasoundManager.h>

#ifndef NELEM
#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif

namespace echonous
{
struct DauContextIds
{
    jfieldID    androidContextId;
    jmethodID   getDataDirId;
    jmethodID   getAbsolutePathID;
} gDauContextIds;

static EmuDau*              gDauPtr = nullptr;
static class DauContext     gDauContext;
extern UltrasoundManager    gUltrasoundManager;

//-----------------------------------------------------------------------------
static jint openDau(JNIEnv* jenv, jobject obj, jobject assetManager,
                    jobject dauContext)
{
    jint            retVal = THOR_OK;
    AAssetManager*  assetManagerPtr = nullptr;
    jobject         jAndroidContext;
    jobject         jAppPathFile;
    jobject         jAppPath;
    const char*     nativePathPtr;

    UNUSED(obj);

    ALOGD("openDau");

    // appPath
    jAndroidContext = jenv->GetObjectField(dauContext, gDauContextIds.androidContextId);
    jAppPathFile = jenv->CallObjectMethod(jAndroidContext, gDauContextIds.getDataDirId);
    jenv->DeleteLocalRef(jAndroidContext);
    jAppPath = jenv->CallObjectMethod(jAppPathFile, gDauContextIds.getAbsolutePathID);
    jenv->DeleteLocalRef(jAppPathFile);
    nativePathPtr = jenv->GetStringUTFChars((jstring) jAppPath, JNI_FALSE);
    gDauContext.appPath = nativePathPtr;
    jenv->ReleaseStringUTFChars((jstring) jAppPath, nativePathPtr);
    jenv->DeleteLocalRef(jAppPath);

    if (nullptr != gDauPtr)
    {
        delete gDauPtr;
        gDauPtr = nullptr;
    }

    assetManagerPtr = AAssetManager_fromJava(jenv, assetManager);
    if (nullptr == assetManagerPtr)
    {
        ALOGE("Unable to get native Asset Manager");
        retVal = TER_UMGR_ASSETMGR;
        goto err_ret;
    }

    gDauPtr = new EmuDau(gDauContext, gUltrasoundManager.getCineBuffer());

    retVal = gDauPtr->open(assetManagerPtr);
    if (IS_THOR_ERROR(retVal))
    {
        ALOGE("Failed to open Emulated Dau");
        delete gDauPtr;
        gDauPtr = nullptr;
        gDauContext.clear();
    }

err_ret:

    return(retVal);
}

//-----------------------------------------------------------------------------
static void closeDau(JNIEnv* jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    ALOGD("closeDau");

    if (nullptr != gDauPtr)
    {
        gDauPtr->close();
        delete gDauPtr;
        gDauPtr = nullptr;
        gDauContext.clear();
    }

    return;
}

//-----------------------------------------------------------------------------
static void startDau(JNIEnv* jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    ALOGD("startDau");
    if (gDauPtr)
    {
        gDauPtr->start();
    }
    return;
}

//-----------------------------------------------------------------------------
static void stopDau(JNIEnv* jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    ALOGD("stopDau");
    if (gDauPtr)
    {
        gDauPtr->stop();
    }
    return;
}

//-----------------------------------------------------------------------------
static JNINativeMethod dau_method_table[] = {
    { "nativeEmuOpen",
      "(Landroid/content/res/AssetManager;Lcom/echonous/bridge/ultrasound/DauContext;)I",
      (void*)openDau
    },
    { "nativeEmuClose",
      "()V",
      (void*)closeDau
    },
    { "nativeEmuStart",
      "()V",
      (void*)startDau
    },
    { "nativeEmuStop",
      "()V",
      (void*)stopDau
    }
};

//-----------------------------------------------------------------------------
int register_dau_methods(JNIEnv* env)
{
    jclass dauContextClass = env->FindClass("com/echonous/bridge/ultrasound/DauContext");
    LOG_FATAL_IF(dauContextClass == NULL, "Unable to find class com.echonous.bridge.ultrasound.DauContext");

    gDauContextIds.androidContextId = env->GetFieldID(dauContextClass, "mAndroidContext", "Landroid/content/Context;");
    LOG_FATAL_IF(gDauContextIds.androidContextId == NULL, "Unable to find field mAndroidContext");

    jclass contextClass = env->FindClass("android/content/Context");
    LOG_FATAL_IF(contextClass == NULL, "Unable to find class android.content.Context");
    gDauContextIds.getDataDirId = env->GetMethodID(contextClass, "getDataDir", "()Ljava/io/File;");
    LOG_FATAL_IF(gDauContextIds.getDataDirId == NULL, "Unable to find method getDataDir");

    jclass fileClass = env->FindClass("java/io/File");
    LOG_FATAL_IF(fileClass == NULL, "Unable to find class java.io.File");
    gDauContextIds.getAbsolutePathID = env->GetMethodID(fileClass, "getAbsolutePath", "()Ljava/lang/String;");
    LOG_FATAL_IF(gDauContextIds.getAbsolutePathID == NULL, "Unable to find method getAbsolutePath");

    jclass dauEmuClass = env->FindClass("com/echonous/hardware/ultrasound/DauEmulated");
    LOG_FATAL_IF(dauEmuClass == NULL, "Unable to find class com.echonous.hardware.ultrasound.DauEmulated");

    return env->RegisterNatives(dauEmuClass, dau_method_table, NELEM(dau_method_table)); 
}

};

