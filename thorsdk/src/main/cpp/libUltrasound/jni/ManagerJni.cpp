//
// Copyright 2017 EchoNous Inc.
//
//  It is assumed that all parameters passed to the JNI functions exist and
//  are valid.  The parameters should be verified by the upper layer
//  Ultrasound Manager (in Java space).
//

#define LOG_TAG "ManagerJni"

#include <jni.h>
#include <android/asset_manager_jni.h>
#include <ThorUtils.h>
#include <CineBuffer.h>
#include <UltrasoundManager.h>
#include <DauController.h>

namespace echonous 
{

static JavaVM*              gJavaVmPtr = nullptr;
static jobject              gJavaManager = nullptr;
static jobject              gJavaAssetManager = nullptr;
UltrasoundManager           gUltrasoundManager;
DauController               mDauController;

//-----------------------------------------------------------------------------
static void openManager(JNIEnv* jenv, jclass managerClass,
                        jobject context,
                        jobject assetManager,
                        jstring appPath)
{
    ThorStatus      retVal = THOR_ERROR;
    AAssetManager*  assetManagerPtr = nullptr;
    const char*     nativeAppPath;

    UNUSED(managerClass);

    ALOGD("openManager");
    if (nullptr == gJavaAssetManager)
    {
        gJavaAssetManager = jenv->NewGlobalRef(assetManager);
    }

    jenv->GetJavaVM(&gJavaVmPtr);

    assetManagerPtr = AAssetManager_fromJava(jenv, assetManager);
    if (nullptr == assetManagerPtr)
    {
        ALOGE("Unable to get native Asset Manager");
        retVal = TER_UMGR_ASSETMGR;
        goto err_ret;
    }

    nativeAppPath = jenv->GetStringUTFChars(appPath, 0);
    gUltrasoundManager.open((void*)jenv, (void*)context, assetManagerPtr, nativeAppPath);

    jenv->ReleaseStringUTFChars(appPath, nativeAppPath);

    retVal = THOR_OK;

err_ret:
    ;
}

//-----------------------------------------------------------------------------
static void closeManager(JNIEnv* jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    ALOGD("closeManager");

    if (nullptr != gJavaAssetManager)
    {
        jenv->DeleteGlobalRef(gJavaAssetManager);
        gJavaAssetManager = nullptr;
    }

    gUltrasoundManager.close();
}

//-----------------------------------------------------------------------------
static void enableIe(JNIEnv* jenv, jobject obj, jboolean enable)
{
    UNUSED(jenv);
    UNUSED(obj);
    UNUSED(enable);

    //gUltrasoundManager.enableIe(enable);
}

//-----------------------------------------------------------------------------
static void setLocale(JNIEnv* jenv, jobject obj, jstring locale)
{
    UNUSED(obj);
    const char *utf = jenv->GetStringUTFChars(locale, nullptr);
    LOGD("JNI setLocale %s", utf);
    UNUSED(utf);
    gUltrasoundManager.setLanguage(utf);
    jenv->ReleaseStringUTFChars(locale, utf);
}


//-----------------------------------------------------------------------------
static jobject getSupportedLocales(JNIEnv* jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    jclass arrayListClass = jenv->FindClass("java/util/ArrayList");
    jmethodID ctor = jenv->GetMethodID(arrayListClass, "<init>", "()V");
    jobject list = jenv->NewObject(arrayListClass, ctor);

    jmethodID add = jenv->GetMethodID(arrayListClass, "add", "(Ljava/lang/Object;)Z");

    // Hard code "en" as a supported locale
    jstring en_US = jenv->NewStringUTF("en");
    jenv->CallBooleanMethod(list, add, en_US);
    jenv->DeleteLocalRef(en_US);

    return list;
}

//--------------------------------------------------------------------------------
static jint bootApplicationImage(JNIEnv* jenv, jobject obj, int fd)
{
    return gUltrasoundManager.bootApplicationImage(fd);
}

//--------------------------------------------------------------------------------
static jint setUsbConfiguration(JNIEnv* jenv, jobject obj, int fd)
{
    return gUltrasoundManager.setUsbConfig(fd);
}

//--------------------------------------------------------------------------------
static jint getProbeTypePcie(JNIEnv *jenv, jobject obj)
{
     UNUSED(jenv);
     UNUSED(obj);
     return mDauController.getProbeType();
}

//-----------------------------------------------------------------------------
static jobject getProbeInformationUsb(JNIEnv *jenv, jobject obj, int fd)
{
    ProbeInfo probeInfo;

    ALOGD("native getProbeInformation called");
    jclass probeInformationClass = jenv->FindClass(
            "com/echonous/hardware/ultrasound/ProbeInformation");
    LOG_FATAL_IF(probeInformationClass == NULL,
                 "Unable to find class com.echonous.hardware.ultrasound.ProbeInformation");
    jmethodID probeInformationMethodId = jenv->GetMethodID(probeInformationClass, "<init>",
                                                           "()V");
    jobject jObjectProbeInformation = jenv->NewObject(probeInformationClass,
                                                      probeInformationMethodId);

    jfieldID typeField = jenv->GetFieldID(probeInformationClass, "type", "I");
    jfieldID serialNumberField = jenv->GetFieldID(probeInformationClass, "serial",
                                                  "Ljava/lang/String;");
    jfieldID partField = jenv->GetFieldID(probeInformationClass, "part", "Ljava/lang/String;");
    jfieldID firmwareVersionField = jenv->GetFieldID(probeInformationClass, "firmwareVersion",
                                                     "Ljava/lang/String;");

    gUltrasoundManager.readProbeInfoUsb(fd,&probeInfo);

    jint type = probeInfo.probeType;
    jstring serialNumber = jenv->NewStringUTF(probeInfo.serialNo.c_str());
    jstring part = jenv->NewStringUTF(probeInfo.part.c_str());
    jstring firmwareVersion = jenv->NewStringUTF(probeInfo.fwVersion.c_str());

    jenv->SetIntField(jObjectProbeInformation, typeField, type);
    jenv->SetObjectField(jObjectProbeInformation, serialNumberField, serialNumber);
    jenv->SetObjectField(jObjectProbeInformation, partField, part);
    jenv->SetObjectField(jObjectProbeInformation, firmwareVersionField, firmwareVersion);

    return jObjectProbeInformation;
}

//-----------------------------------------------------------------------------
static jboolean setProbeInformation(JNIEnv *jenv, jobject obj, int probType,int fd) {
    UNUSED(jenv);
    UNUSED(obj);
    ProbeInfo probeInfo;

    probeInfo.probeType = probType;
    ALOGD("nativeSetProbeInfo FD : %d",fd);
    return gUltrasoundManager.setProbeInfo(&probeInfo, fd);
}

//-------------------------------------------------------------------------------
static jstring getUpsPath(JNIEnv *jenv, jobject obj, int probeType){
    UNUSED(jenv);
    UNUSED(obj);
    jstring upsPath = jenv->NewStringUTF(gUltrasoundManager.getDbName(probeType));
    return upsPath;
}

//--------------------------------------------------------------------------------
static jboolean checkFwUpdateAvailablePCIe(JNIEnv *jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);
    return gUltrasoundManager.isFwUpdateAvailablePCIe();
}

//--------------------------------------------------------------------------------
static jboolean appFwUpdatePCIe(JNIEnv *jenv, jobject obj) {
    UNUSED(jenv);
    UNUSED(obj);
    return gUltrasoundManager.applicationFwUpdatePCIe();
}

//--------------------------------------------------------------------------------
static jboolean checkFwUpdateAvailableUsb(JNIEnv *jenv, jobject obj, int fd , jboolean isAppFd)
{
    ALOGD("checkFwUpdateAvailableUsb");
    UNUSED(jenv);
    UNUSED(obj);
    return gUltrasoundManager.isFwUpdateAvailableUsb(fd, isAppFd);
}

//--------------------------------------------------------------------------------
static jboolean appFwUpdateUsb(JNIEnv *jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);
    return gUltrasoundManager.applicationFwUpdateUsb();
}

//-----------------------------------------------------------------------------
static JNINativeMethod manager_method_table[] =
{
    { "openManagerNative",
      "(Landroid/content/Context;Landroid/content/res/AssetManager;Ljava/lang/String;)V",
      (void*) openManager
    },
    { "closeManagerNative",
      "()V",
      (void*) closeManager
    },
    { "enableIeNative",
      "(Z)V",
      (void*) enableIe
    },
    { "setLocaleNative",
        "(Ljava/lang/String;)V",
        (void*) setLocale
    },
    { "getSupportedLocalesNative",
        "()Ljava/util/List;",
        (void*) getSupportedLocales
    },
    {
      "bootAppImgNative",
      "(I)I",
      (void *) bootApplicationImage
    },
    {
      "nativeSetUsbConfiguration",
      "(I)I",
      (void *) setUsbConfiguration
    },
    {
        "checkFwUpdateAvailableNativeUsb",
        "(IZ)Z",
        (void *) checkFwUpdateAvailableUsb
    },
    {
        "appFwUpdateNativeUsb",
        "()Z",
         (void *) appFwUpdateUsb
    },
    {
        "getProbeTypePcieNative",
        "()I",
        (void *) getProbeTypePcie
    },
    {
        "checkFwUpdateAvailableNativePCIe",
        "()Z",
        (void *) checkFwUpdateAvailablePCIe
    },
    {
        "appFwUpdateNativePCIe",
        "()Z",
        (void *) appFwUpdatePCIe
    },
    {
        "getProbeInformationNativeUsb",
        "(I)Lcom/echonous/hardware/ultrasound/ProbeInformation;",
        (void *) getProbeInformationUsb
    },
    {
        "nativeGetDbName",
        "(I)Ljava/lang/String;",
        (void*) getUpsPath
    },
    {
        "nativeSetProbeInformation",
        "(II)Z",
        (void *) setProbeInformation
    },
};

//-----------------------------------------------------------------------------
int register_manager_methods(JNIEnv* env)
{
    jclass managerClass = env->FindClass("com/echonous/hardware/ultrasound/UltrasoundManager");
    LOG_FATAL_IF(managerClass == NULL, "Unable to find class com.echonous.hardware.ultrasound.UltrasoundManager");

    return env->RegisterNatives(managerClass,
                                manager_method_table,
                                NELEM(manager_method_table)); 
}

};
