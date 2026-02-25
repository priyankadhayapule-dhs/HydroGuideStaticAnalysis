//
// Copyright 2016 EchoNous Inc.
//
//  It is assumed that all parameters passed to the JNI functions exist and
//  are valid.  The parameters should be verified by the upper layer
//  Ultrasound Manager (in Java space).
//

#define LOG_TAG "DauJni"

#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <jni.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <dlfcn.h>

#include <ThorUtils.h>
#include <DauContext.h>
#include <ThorError.h>
#include <Dau.h>
#include <UltrasoundManager.h>
#include <CriticalSection.h>

#ifdef TBENCH_WRITE_LOGGING
#include <cutils/properties.h>
#endif

namespace echonous 
{

struct DauContextIds
{
    jfieldID    androidContextId;
    jfieldID    isUsbContextId;
    jfieldID    eibIoctlFdId;
    jfieldID    ionFdId;
    jfieldID    dauPowerFd;
    jfieldID    dauDataLinkFd;
    jfieldID    dauDataIrqFd;
    jfieldID    dauErrorIrqFd;
    jfieldID    dauResourceFd;
    jfieldID    nativeUsbFd;
    jmethodID   getFdId;
    jmethodID   onErrorId;
    jmethodID   onHidId;
    jmethodID   onExternalEcgId;
    jmethodID   getDataDirId;
    jmethodID   getAbsolutePathID;
} gDauContextIds;

static class DauContext     gDauContext;
static JavaVM*              gJavaVmPtr = nullptr;
static jobject              gJavaDau = nullptr;
static CriticalSection      gDauLock;
static Dau*                 gDauPtr = nullptr;
static bool                 gDebugFlag = false;
static int                  gSafetyTestSelectedOption = 0;
static float                gDauThresholdTemperatureForTest = 0.0f;

extern UltrasoundManager    gUltrasoundManager;

static void closeDau(JNIEnv* jenv, jobject obj);


//-----------------------------------------------------------------------------
static void reportError(uint32_t errorCode)
{
    JNIEnv*     jenv = nullptr;

    gDauLock.enter();
    ALOGE("Reporting error: 0x%x", errorCode);
    if (nullptr != gJavaVmPtr && nullptr != gJavaDau)
    {
        gJavaVmPtr->AttachCurrentThread(&jenv, NULL);
        if (nullptr == jenv)
        {
            ALOGE("Unable to get Android Runtime");
            goto err_ret;
        }
        jenv->CallVoidMethod(gJavaDau, gDauContextIds.onErrorId, (jint) errorCode);
        gJavaVmPtr->DetachCurrentThread();
    }

err_ret:
    gDauLock.leave();
}

//-----------------------------------------------------------------------------
static void reportExternalEcg(bool isConnected)
{
    JNIEnv*     jenv = nullptr;

    gDauLock.enter();
    ALOGD("External ECG Connection Status: %s", 
          isConnected ? "connected" : "disconnected");
    if (nullptr != gJavaVmPtr && nullptr != gJavaDau)
    {
        gJavaVmPtr->AttachCurrentThread(&jenv, NULL);
        if (nullptr == jenv)
        {
            ALOGE("Unable to get Android Runtime");
            goto err_ret;
        }
        jenv->CallVoidMethod(gJavaDau,
                             gDauContextIds.onExternalEcgId,
                             isConnected ? JNI_TRUE : JNI_FALSE);
        gJavaVmPtr->DetachCurrentThread();
    }

err_ret:
    gDauLock.leave();
}

//-----------------------------------------------------------------------------
static void reportHid(uint32_t hidId)
{
    JNIEnv*     jenv = nullptr;

    gDauLock.enter();
    ALOGD("Reporting HID: 0x%x", hidId);
    if (nullptr != gJavaVmPtr && nullptr != gJavaDau)
    {
        gJavaVmPtr->AttachCurrentThread(&jenv, NULL);
        if (nullptr == jenv)
        {
            ALOGE("Unable to get Android Runtime");
            goto err_ret;
        }
        jenv->CallVoidMethod(gJavaDau, gDauContextIds.onHidId, (jint) hidId);
        gJavaVmPtr->DetachCurrentThread();
    }

err_ret:
    gDauLock.leave();
}

//-----------------------------------------------------------------------------
static jobject getProbeInformation(JNIEnv *jenv, jobject obj, jobject thisObj)
{
    ProbeInfo   probeInfo;

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

    gDauPtr->getProbeInfo(&probeInfo);

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
static void setDebugFlag(JNIEnv* jenv, jobject obj, jboolean flag)
{
    UNUSED(jenv);
    UNUSED(obj);

    ALOGD("set Debugging flag");
    gDebugFlag = flag;
}

//-----------------------------------------------------------------------------
static jint openDau(JNIEnv* jenv, jobject obj, jobject thisObj, jobject dauContext)
{
    jint                retVal = THOR_OK;
    jobject             parcelFileDescriptor;
    jobject             fileDescriptor;
    jboolean            jIsUsbContext = JNI_FALSE;
    jobject             jAndroidContext;
    jobject             jAppPathFile;
    jobject             jAppPath;
    int                 fd;
    const char*         nativePathPtr;

    UNUSED(obj);
    ALOGD("openDau");

    jenv->GetJavaVM(&gJavaVmPtr);

    gDauLock.enter();
    gJavaDau = jenv->NewGlobalRef(thisObj);
    gDauLock.leave();

    jIsUsbContext = jenv->GetBooleanField(dauContext, gDauContextIds.isUsbContextId);
    if (JNI_TRUE == jIsUsbContext)
    {
        gDauContext.isUsbContext = true;
        gDauContext.usbFd = dup(jenv->GetIntField(dauContext, gDauContextIds.nativeUsbFd));
    }
    else
    {
        void*   proxyHandle = nullptr;

        gDauContext.isUsbContext = false;

        dlerror();
        proxyHandle = dlopen("libUltrasoundSvcProxy.so", RTLD_LAZY);
        if (nullptr != proxyHandle)
        {
            void*           funcPtr = nullptr;
            const char*     errorMsg;

            dlerror();
            funcPtr = dlsym(proxyHandle, "getDauContext");
            errorMsg = dlerror();
            if (nullptr != errorMsg)
            {
                ALOGE("Unable to get context data: %s", errorMsg);
                retVal = TER_UMGR_SVC_ACCESS;
                goto err_ret;
            }
            else
            {
                typedef ThorStatus (*getDauContextFunc)(DauContext&);
                getDauContextFunc   getDauContext = (getDauContextFunc) funcPtr;
                
                retVal = getDauContext(gDauContext);
            }

            dlclose(proxyHandle);
            if (IS_THOR_ERROR(retVal))
            {
                ALOGE("Failed to get DauContext, unable to open Dau.");
                goto err_ret;
            }
        }
        else
        {
            ALOGE("Unable to open UltrasoundSvcProxy: %s", dlerror());
            retVal = TER_UMGR_SVC_ACCESS;
            goto err_ret;
        }
    }

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

#ifdef TBENCH_WRITE_LOGGING
    gDauContext.enableWriteLogging = property_get_bool("thor.write_log", false);
#endif

    gDauPtr = new Dau(gDauContext, 
                      gUltrasoundManager.getCineBuffer(),
                      gUltrasoundManager.getCineViewer(),
                      gUltrasoundManager.getAppPath());

    if (gDauPtr)
    {
       gDauPtr->setDebuggingFlag(gDebugFlag);
       gDauPtr->setDauSafetyFeatureTestOption(gSafetyTestSelectedOption, gDauThresholdTemperatureForTest);
    }

        retVal = gDauPtr->open(gUltrasoundManager.getUpsReader(),
                           reportError,
                           reportHid,
                           reportExternalEcg);
err_ret:
    if (IS_THOR_ERROR(retVal))
    {
        ALOGE("Failed to open Dau");
        closeDau(jenv, obj);
    }

    return(retVal);
}

//-----------------------------------------------------------------------------
static void closeDau(JNIEnv* jenv, jobject obj)
{
    UNUSED(obj);

    ALOGD("closeDau");

    if (nullptr != gDauPtr)
    {
        gDauPtr->close();
        delete gDauPtr;
        gDauPtr = nullptr;
        gDauContext.clear();
    }
    else
    {
        ALOGD("gDauPtr is null");
    }

    gDauLock.enter();
    if (nullptr != gJavaDau)
    {
        jenv->DeleteGlobalRef(gJavaDau);
        gJavaDau = nullptr;
    }
    gDauLock.leave();

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
static jint stopDau(JNIEnv* jenv, jobject obj)
{
    jint retVal = THOR_ERROR;
    UNUSED(jenv);
    UNUSED(obj);

    ALOGD("stopDau");

    if (gDauPtr)
    {
        retVal = gDauPtr->stop();
    }

    return (retVal);
}

//-----------------------------------------------------------------------------
static jint setImagingCase(JNIEnv* jenv,
                           jobject obj,
                           int imagingCaseId,
                           jfloatArray apiEstimates_,
                           jfloatArray imagingInfo_)
{
    UNUSED(obj);
    jint retVal = THOR_ERROR;
    jfloat *apiEstimates = jenv->GetFloatArrayElements(apiEstimates_, NULL);
    jfloat *imagingInfo = jenv->GetFloatArrayElements(imagingInfo_, NULL);

    ALOGD("setImagingCase");
    if (gDauPtr)
    {
        retVal = gDauPtr->setImagingCase(imagingCaseId, apiEstimates, imagingInfo);
    }
    jenv->ReleaseFloatArrayElements(apiEstimates_, apiEstimates, 0);
    jenv->ReleaseFloatArrayElements(imagingInfo_, imagingInfo, 0);
    return (retVal);
}
    

//-----------------------------------------------------------------------------
static jint setColorImagingCase(JNIEnv* jenv,
                                jobject obj,
                                jint imagingCaseId,
                                jintArray colorparams_,
                                jfloatArray roiparams_,
                                jfloatArray apiEstimates_,
                                jfloatArray imagingInfo_)
{
    UNUSED(obj);
    jint *colorparams = jenv->GetIntArrayElements(colorparams_, NULL);
    jfloat *roiparams = jenv->GetFloatArrayElements(roiparams_, NULL);
    jfloat *apiEstimates = jenv->GetFloatArrayElements(apiEstimates_, NULL);
    jfloat *imagingInfo = jenv->GetFloatArrayElements(imagingInfo_, NULL);

    jint retVal = THOR_ERROR;
    if (gDauPtr)
    {
        retVal = gDauPtr->setColorImagingCase(imagingCaseId, colorparams, roiparams, apiEstimates, imagingInfo);
    }

    jenv->ReleaseIntArrayElements(colorparams_, colorparams, 0);
    jenv->ReleaseFloatArrayElements(roiparams_, roiparams, 0);
    jenv->ReleaseFloatArrayElements(apiEstimates_, apiEstimates, 0);
    jenv->ReleaseFloatArrayElements(imagingInfo_, imagingInfo, 0);
    return (retVal);
}
        
//-----------------------------------------------------------------------------
static jint setMmodeImagingCase(JNIEnv* jenv,
                                jobject obj,
                                jint imagingCaseId,
                                jint scrollSpeedId,
                                jfloatArray apiEstimates_,
                                jfloatArray imagingInfo_,
                                jfloat mLinePosition)
{
    UNUSED(obj);
    jfloat *apiEstimates = jenv->GetFloatArrayElements(apiEstimates_, NULL);
    jfloat *imagingInfo = jenv->GetFloatArrayElements(imagingInfo_, NULL);

    jint retVal = THOR_ERROR;
    if (gDauPtr)
    {
        retVal = gDauPtr->setMmodeImagingCase(imagingCaseId, scrollSpeedId, mLinePosition, apiEstimates, imagingInfo);
    }
    jenv->ReleaseFloatArrayElements(apiEstimates_, apiEstimates, 0);
    jenv->ReleaseFloatArrayElements(imagingInfo_, imagingInfo, 0);
    return (retVal);
}

//-----------------------------------------------------------------------------
    static jint setPwImagingCase(JNIEnv* jenv,
                                 jobject obj,
                                 jint imagingCaseId,
                                 jintArray pwindices_,
                                 jfloatArray gateparams_,
                                 jfloatArray apiEstimates_,
                                 jfloatArray imagingInfo_,
                                 jboolean isTDI)
    {
        UNUSED(obj);
        jint *pwindices = jenv->GetIntArrayElements(pwindices_, NULL);
        jfloat *gateparams = jenv->GetFloatArrayElements(gateparams_, NULL);
        jfloat *apiEstimates = jenv->GetFloatArrayElements(apiEstimates_, NULL);
        jfloat *imagingInfo = jenv->GetFloatArrayElements(imagingInfo_, NULL);

        jint retVal = THOR_ERROR;
        if (gDauPtr)
        {
            retVal = gDauPtr->setPwImagingCase(imagingCaseId, pwindices, gateparams, apiEstimates, imagingInfo, isTDI);
        }

        jenv->ReleaseIntArrayElements(pwindices_, pwindices, 0);
        jenv->ReleaseFloatArrayElements(gateparams_, gateparams, 0);
        jenv->ReleaseFloatArrayElements(apiEstimates_, apiEstimates, 0);
        jenv->ReleaseFloatArrayElements(imagingInfo_, imagingInfo, 0);
        return (retVal);
    }

//-----------------------------------------------------------------------------
    static jint setCwImagingCase(JNIEnv* jenv,
                                 jobject obj,
                                 jint imagingCaseId,
                                 jintArray cwindices_,
                                 jfloatArray gateparams_,
                                 jfloatArray apiEstimates_,
                                 jfloatArray imagingInfo_)
{
    UNUSED(obj);
    jint *cwindices = jenv->GetIntArrayElements(cwindices_, NULL);
    jfloat *gateparams = jenv->GetFloatArrayElements(gateparams_, NULL);
    jfloat *apiEstimates = jenv->GetFloatArrayElements(apiEstimates_, NULL);
    jfloat *imagingInfo = jenv->GetFloatArrayElements(imagingInfo_, NULL);

    jint retVal = THOR_ERROR;
    if (gDauPtr)
    {
        retVal = gDauPtr->setCwImagingCase(imagingCaseId, cwindices, gateparams, apiEstimates, imagingInfo);
    }

    jenv->ReleaseIntArrayElements(cwindices_, cwindices, 0);
    jenv->ReleaseFloatArrayElements(gateparams_, gateparams, 0);
    jenv->ReleaseFloatArrayElements(apiEstimates_, apiEstimates, 0);
    jenv->ReleaseFloatArrayElements(imagingInfo_, imagingInfo, 0);
    return (retVal);
}

//-----------------------------------------------------------------------------
static jint setMLine(JNIEnv* jenv,
                     jobject obj,
                     jfloat mLinePosition)
{
    UNUSED(jenv);
    UNUSED(obj);

    jint retVal = THOR_ERROR;
    if (gDauPtr)
    {
        retVal = gDauPtr->setMLine(mLinePosition);
    }
    return (retVal);
}

//-----------------------------------------------------------------------------
static void enableImageProcessing(JNIEnv* jenv, jobject obj, jint type, jboolean enable)
{
    UNUSED(jenv);
    UNUSED(obj);

    if (gDauPtr)
    {
        gDauPtr->enableImageProcessing(type, JNI_TRUE == enable ? true : false);
    }
}

//-----------------------------------------------------------------------------
static void setGain(JNIEnv* jenv, jobject obj, int gain)
{
    UNUSED(jenv);
    UNUSED(obj);

    ALOGD("setGain");
    if (gDauPtr)
    {
        gDauPtr->setGain(gain);
    }
}

//-----------------------------------------------------------------------------
static void setNearFarGain(JNIEnv* jenv, jobject obj, int nearGain, int farGain)
{
    UNUSED(jenv);
    UNUSED(obj);

    ALOGD("setNearFarGain");
    if (gDauPtr)
    {
        gDauPtr->setGain(nearGain, farGain);
    }
}
    
//-----------------------------------------------------------------------------
static void setColorGain(JNIEnv* jenv, jobject obj, int gain)
{
    UNUSED(jenv);
    UNUSED(obj);

    ALOGD("setColorGain");
    if (gDauPtr)
    {
        gDauPtr->setColorGain(gain);
    }
}

//-----------------------------------------------------------------------------
static void setEcgGain(JNIEnv* jenv, jobject obj, float gain)
{
    UNUSED(jenv);
    UNUSED(obj);

    ALOGD("setEcgGain");
    if (gDauPtr)
    {
        gDauPtr->setEcgGain(gain);
    }
}

//-----------------------------------------------------------------------------
static void setDaGain(JNIEnv* jenv, jobject obj, float gain)
{
    UNUSED(jenv);
    UNUSED(obj);

    ALOGD("setDaGain");
    if (gDauPtr)
    {
        gDauPtr->setDaGain(gain);
    }
}

//-----------------------------------------------------------------------------
static void setPwGain(JNIEnv* jenv, jobject obj, int gain)
{
    UNUSED(jenv);
    UNUSED(obj);

    ALOGD("setPwGain");
    if (gDauPtr)
    {
        gDauPtr->setPwGain(gain);
    }
}

//-----------------------------------------------------------------------------
static void setPwAudioGain(JNIEnv* jenv, jobject obj, int gain)
{
    UNUSED(jenv);
    UNUSED(obj);

    ALOGD("setPwAudioGain");
    if (gDauPtr)
    {
        gDauPtr->setPwAudioGain(gain);
    }
}

//-----------------------------------------------------------------------------
static void setCwGain(JNIEnv* jenv, jobject obj, int gain)
{
    UNUSED(jenv);
    UNUSED(obj);

    ALOGD("setCwGain");
    if (gDauPtr)
    {
        gDauPtr->setCwGain(gain);
    }
}

//-----------------------------------------------------------------------------
static void setCwAudioGain(JNIEnv* jenv, jobject obj, int gain)
{
    UNUSED(jenv);
    UNUSED(obj);

    ALOGD("setCwAudioGain");
    if (gDauPtr)
    {
        gDauPtr->setCwAudioGain(gain);
    }
}

static void setUsSignal(JNIEnv* jenv,jobject obj,jboolean isUSSignal)
{
    UNUSED(jenv);
    UNUSED(obj);
    ALOGD("setUsSignal");

    if(gDauPtr){
        gDauPtr->setUSSignal(isUSSignal);
    }
}

static void setECGSignal(JNIEnv *jenv, jobject obj, jboolean isECGSignal)
{
    UNUSED(jenv);
    UNUSED(obj);
    ALOGD("setECGSignal");

    if (gDauPtr) {
        gDauPtr->setECGSignal(isECGSignal);
    }
}

static void setDASignal(JNIEnv *jenv, jobject obj, jboolean isDASignal)
{
    UNUSED(jenv);
    UNUSED(obj);
    ALOGD("setDASignal");

    if (gDauPtr) {
        gDauPtr->setDASignal(isDASignal);
    }
}

//-----------------------------------------------------------------------------
static void getFov(JNIEnv* jenv, jobject obj, jint depthId, jint imagingMode, jfloatArray fov_)
{
    UNUSED(obj);
    jfloat *fov = jenv->GetFloatArrayElements(fov_, NULL);

    ALOGD("getFov");
    if (gDauPtr)
    {
        gDauPtr->getFov(depthId, imagingMode,fov);
    }
    jenv->ReleaseFloatArrayElements(fov_, fov, 0);
}

//-----------------------------------------------------------------------------
static void getDefaultRoi(JNIEnv* jenv, jobject obj, jint organId, jint depthId, jfloatArray roi_)
{
    UNUSED(obj);
    jfloat *roi = jenv->GetFloatArrayElements(roi_, NULL);

    ALOGD("getDefaultRoi");
    if (gDauPtr)
    {
        gDauPtr->getDefaultRoi(organId, depthId, roi);
    }
    jenv->ReleaseFloatArrayElements(roi_, roi, 0);
}

//-----------------------------------------------------------------------------
static jint getFrameIntervalMs(JNIEnv *jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    jint retVal = 0;
    if (gDauPtr)
    {
        retVal = gDauPtr->getFrameIntervalMs();
    }

    return (retVal);
}

//-----------------------------------------------------------------------------
static jint getMLinesPerFrame(JNIEnv *jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    jint retVal = 0;
    if (gDauPtr)
    {
        retVal = gDauPtr->getMLinesPerFrame();
    }

    return (retVal);
}

//-----------------------------------------------------------------------------
static jboolean getExternalEcg(JNIEnv *jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    jboolean    isConnected = JNI_FALSE;

    if (gDauPtr)
    {
        isConnected = (gDauPtr->getExternalEcg() ? JNI_TRUE : JNI_FALSE);
    }

    return(isConnected); 
}

static void setExternalEcg(JNIEnv *jenv, jobject obj,jboolean isExternalECG)
{
    UNUSED(jenv);
    UNUSED(obj);
   if(gDauPtr){
       LOGI("setExternal ECG %d",isExternalECG);
       gDauPtr->setExternalEcg(isExternalECG);
   }

}

static void setLeadNumber(JNIEnv *jenv, jobject obj,jint leadNumber)
{
    UNUSED(jenv);
    UNUSED(obj);
    if(gDauPtr){
        LOGI("setLeadNumber is ECG %d",leadNumber);
        gDauPtr->setLeadNumber(leadNumber);
    }
}

static jint runTransducerElementCheck(JNIEnv *jenv, jobject obj, jintArray results_)
{
    UNUSED(jenv);
    UNUSED(obj);

    jint retVal = THOR_ERROR;
    jint *results = jenv->GetIntArrayElements(results_, NULL);
    if (gDauPtr)
    {
        retVal = gDauPtr->runTransducerElementCheck(results);
    }
    jenv->ReleaseIntArrayElements(results_, results, 0);
    return(retVal);
}

//-----------------------------------------------------------------------------
static jboolean supportsEcg(JNIEnv *jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    jboolean    retVal = JNI_TRUE;

    if (gDauPtr)
    {
        if (!gDauPtr->supportsEcg())
        {
            retVal = JNI_FALSE;
        }
    }

    return(retVal);
}

//-----------------------------------------------------------------------------
static jboolean isElementCheckRunning(JNIEnv *jenv, jobject obj)
{
    LOGD("%s", __func__);
    UNUSED(jenv);
    UNUSED(obj);

    jboolean    retVal = JNI_TRUE;
    if (gDauPtr)
    {
        if (!gDauPtr->isTransducerElementCheckRunning())
        {
            retVal = JNI_FALSE;
        }
    }

    return(retVal);
}

//-----------------------------------------------------------------------------
static jboolean supportsDa(JNIEnv *jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    jboolean    retVal = JNI_TRUE;

    if (gDauPtr)
    {
        if (!gDauPtr->supportsDa())
        {
            retVal = JNI_FALSE;
        }
    }

    return(retVal);
}

//-----------------------------------------------------------------------------
static jstring getDbName(JNIEnv *jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    if (gDauPtr)
    {
        return(jenv->NewStringUTF(gDauPtr->getDbName()));
    }
    else
    {
        return(NULL);
    }
}

//-----------------------------------------------------------------------------
static void setSafetyFeatureTestOption(JNIEnv *jenv, jobject obj, int dauSafetyTestSelectedOption, float dauThresholdTemperatureForTest)
{
    UNUSED(jenv);
    UNUSED(obj);
    gSafetyTestSelectedOption = dauSafetyTestSelectedOption;
    gDauThresholdTemperatureForTest = dauThresholdTemperatureForTest;

}

//-----------------------------------------------------------------------------
static JNINativeMethod dau_method_table[] = {
    { "nativeOpen",
      "(Lcom/echonous/hardware/ultrasound/DauImpl;Lcom/echonous/hardware/ultrasound/DauContext;)I",
      (void*) openDau
    },
    { "nativeClose",
      "()V",
      (void*) closeDau
    },
    { "nativeStart",
      "()V",
      (void*) startDau
    },
    { "nativeStop",
      "()I",
      (void*) stopDau
    },
    { "nativeSetImagingCase",
      "(I[F[F)I",
      (void*) setImagingCase
    },
    { "nativeSetColorImagingCase",
      "(I[I[F[F[F)I",
      (void *)setColorImagingCase
    },
    { "nativeSetMmodeImagingCase",
      "(IIF[F[F)I",
      (void *)setMmodeImagingCase
    },
    { "nativeSetPwImagingCase",
      "(I[I[F[F[FZ)I",
      (void *)setPwImagingCase
    },
    { "nativeSetCwImagingCase",
      "(I[I[F[F[F)I",
      (void *)setCwImagingCase
    },
    { "nativeSetMLine",
      "(F)I",
      (void *)setMLine
    },
    { "nativeEnableImageProcessing",
      "(IZ)V",
      (void*) enableImageProcessing
    },
    { "nativeSetGain",
      "(I)V",
      (void*) setGain
    },
    { "nativeSetNearFarGain",
      "(II)V",
      (void*) setNearFarGain
    },
    { "nativeSetColorGain",
      "(I)V",
      (void*) setColorGain
    },
    { "nativeSetEcgGain",
      "(F)V",
      (void*) setEcgGain
    },
    { "nativeSetDaGain",
      "(F)V",
      (void*) setDaGain
    },
    { "nativeSetPwGain",
      "(I)V",
      (void*) setPwGain
    },
    { "nativeSetPwAudioGain",
      "(I)V",
      (void*) setPwAudioGain
    },
    { "nativeSetCwGain",
      "(I)V",
      (void*) setCwGain
    },
    { "nativeSetCwAudioGain",
      "(I)V",
      (void*) setCwAudioGain
    },
    { "nativeGetFov",
      "(II[F)V",
      (void*) getFov
    },
    { "nativeGetDefaultRoi",
      "(II[F)V",
      (void*) getDefaultRoi
    },
    { "nativeGetFrameIntervalMs",
      "()I",
      (void*) getFrameIntervalMs
    },
    { "nativeGetMLinesPerFrame",
      "()I",
      (void*) getMLinesPerFrame
    },
    {
      "nativeSetUSSignal",
      "(Z)V",
      (void*)setUsSignal
    },
    {
      "nativeSetDebugFlag",
      "(Z)V",
      (void*)setDebugFlag
    },
    {
      "nativeSetECGSignal",
      "(Z)V",
      (void*)setECGSignal
    },
    {
      "nativeSetDASignal",
      "(Z)V",
      (void*)setDASignal
    },
    {
      "nativeGetExternalEcg",
      "()Z",
      (void*)getExternalEcg
    },
    {
      "nativeSetExternalEcg",
      "(Z)V",
      (void*)setExternalEcg
    },
    {
      "nativeSetLeadNumber",
      "(I)V",
      (void*)setLeadNumber
    },
    {
        "nativeGetProbeInformation",
        "()Lcom/echonous/hardware/ultrasound/ProbeInformation;",
        (void*)getProbeInformation
    },
    {
        "nativeRunTransducerElementCheck",
        "([I)I",
        (void*)runTransducerElementCheck
    },
    {
        "nativeSupportsEcg",
        "()Z",
        (void*)supportsEcg
    },
    {
        "nativeSupportsDa",
        "()Z",
        (void*)supportsDa
    },
    {
        "nativeGetDbName",
        "()Ljava/lang/String;",
        (void*)getDbName
    },
    {
        "nativeIsElementCheckRunning",
        "()Z",
        (void*)isElementCheckRunning
    },
    {
        "nativeSetSafetyFeatureTestOption",
            "(IF)V",
            (void*) setSafetyFeatureTestOption
    }
    };

//-----------------------------------------------------------------------------
int register_dau_methods(JNIEnv* env)
{
    jclass dauContextClass = env->FindClass("com/echonous/hardware/ultrasound/DauContext");
    LOG_FATAL_IF(dauContextClass == NULL, "Unable to find class com.echonous.hardware.ultrasound.DauContext");

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

    gDauContextIds.isUsbContextId = env->GetFieldID(dauContextClass, "mIsUsbContext", "Z");
    LOG_FATAL_IF(gDauContextIds.isUsbContextId == NULL, "Unable to find field mIsUsbContext");

    gDauContextIds.eibIoctlFdId = env->GetFieldID(dauContextClass, "mEibIoctlFd", "Landroid/os/ParcelFileDescriptor;");
    LOG_FATAL_IF(gDauContextIds.eibIoctlFdId == NULL, "Unable to find field mEibIoctlFd");

    gDauContextIds.ionFdId = env->GetFieldID(dauContextClass, "mIonFd", "Landroid/os/ParcelFileDescriptor;");
    LOG_FATAL_IF(gDauContextIds.ionFdId == NULL, "Unable to find field mIonFd");

    gDauContextIds.dauPowerFd = env->GetFieldID(dauContextClass, "mDauPowerFd", "Landroid/os/ParcelFileDescriptor;");
    LOG_FATAL_IF(gDauContextIds.dauPowerFd == NULL, "Unable to find field mDauPowerFd");

    gDauContextIds.dauDataLinkFd = env->GetFieldID(dauContextClass, "mDauDataLinkFd", "Landroid/os/ParcelFileDescriptor;");
    LOG_FATAL_IF(gDauContextIds.dauDataLinkFd == NULL, "Unable to find field mDauDataLinkFd");

    gDauContextIds.dauDataIrqFd = env->GetFieldID(dauContextClass, "mDauDataIrqFd", "Landroid/os/ParcelFileDescriptor;");
    LOG_FATAL_IF(gDauContextIds.dauDataIrqFd == NULL, "Unable to find field mDauDataIrqFd");

    gDauContextIds.dauErrorIrqFd = env->GetFieldID(dauContextClass, "mDauErrorIrqFd", "Landroid/os/ParcelFileDescriptor;");
    LOG_FATAL_IF(gDauContextIds.dauErrorIrqFd == NULL, "Unable to find field mDauErrorIrqFd");

    gDauContextIds.dauResourceFd = env->GetFieldID(dauContextClass, "mDauResourceFd", "Landroid/os/ParcelFileDescriptor;");
    LOG_FATAL_IF(gDauContextIds.dauResourceFd == NULL, "Unable to find field mDauResourceFd");

    gDauContextIds.nativeUsbFd = env->GetFieldID(dauContextClass, "mNativeUsbFd", "I");
    LOG_FATAL_IF(gDauContextIds.nativeUsbFd == NULL, "Unable to find field mNativeUsbFd");

    jclass parcelFileDescClass = env->FindClass("android/os/ParcelFileDescriptor");
    LOG_FATAL_IF(parcelFileDescClass == NULL, "Unable to find class android.os.ParcelFileDescriptor");
    gDauContextIds.getFdId = env->GetMethodID(parcelFileDescClass, "getFd", "()I");
    LOG_FATAL_IF(gDauContextIds.getFdId == NULL, "Unable to find method getFd");

    jclass dauClass = env->FindClass("com/echonous/hardware/ultrasound/Dau");
    LOG_FATAL_IF(dauClass == NULL, "Unable to find class com.echonous.hardware.ultrasound.Dau");
    gDauContextIds.onErrorId = env->GetMethodID(dauClass, "onError", "(I)V");
    LOG_FATAL_IF(gDauContextIds.onErrorId == NULL, "Unable to find method onError");

    gDauContextIds.onHidId = env->GetMethodID(dauClass, "onHid", "(I)V");
    LOG_FATAL_IF(gDauContextIds.onHidId == NULL, "Unable to find method onHid");

    gDauContextIds.onExternalEcgId = env->GetMethodID(dauClass, "onExternalEcg", "(Z)V");
    LOG_FATAL_IF(gDauContextIds.onExternalEcg == NULL, "Unable to find method onExternalEcg");

    jclass dauImplClass = env->FindClass("com/echonous/hardware/ultrasound/DauImpl");
    LOG_FATAL_IF(dauImplClass == NULL, "Unable to find class com.echonous.hardware.ultrasound.DauImpl");

    return env->RegisterNatives(dauImplClass, dau_method_table, NELEM(dau_method_table)); 
}

};

