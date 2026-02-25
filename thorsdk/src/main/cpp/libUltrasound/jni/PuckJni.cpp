//
// Copyright 2018 EchoNous Inc.
//
//

#define LOG_TAG "PuckJni"

#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <jni.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <dlfcn.h>
#include <pthread.h>
#include <string.h>

#include <ThorUtils.h>
#include <ThorError.h>
#include <thorpuck.h>
#include <CriticalSection.h>

#define CHECK_CONFIG_RESULT(result) \
    if (!(result)) \
    { \
        ALOGE("Unable to configure the Puck."); \
        retVal = TER_TABLET_PUCK_CONFIG; \
        goto err_ret; \
    }

namespace echonous 
{

static jmethodID        onPuckEventId;

static JavaVM*          gJavaVmPtr = nullptr;
static jobject          gJavaPuck = nullptr;
static CriticalSection  gPuckLock;
static thorpuck_t*      gPuckHandle = nullptr;
static bool             gFirmwareInProgress = false;

//-----------------------------------------------------------------------------
static void onPuckEvent(thorpuck_t* puckHandle,
                        void* priv,
                        thorpuck_event_t* puckEventPtr)
{
    JNIEnv*     jenv = nullptr;

    UNUSED(puckHandle);
    UNUSED(priv);

    gPuckLock.enter();
    if (nullptr != gJavaVmPtr && nullptr != gJavaPuck)
    {
        gJavaVmPtr->AttachCurrentThread(&jenv, NULL);
        if (nullptr == jenv)
        {
            ALOGE("Unable to get Android Runtime");
            goto err_ret;
        }
        jenv->CallVoidMethod(gJavaPuck,
                             onPuckEventId,
                             (jint) puckEventPtr->leftButtonAction,
                             (jint) puckEventPtr->rightButtonAction,
                             (jint) puckEventPtr->palmAction,
                             (jint) puckEventPtr->scrollAction,
                             (jint) puckEventPtr->scrollAmount,
                             (jint) puckEventPtr->updateCurPos,
                             (jint) puckEventPtr->updateMaxPos,
                             (jint) puckEventPtr->updateStatus);

        gJavaVmPtr->DetachCurrentThread();
    }

err_ret:
    gPuckLock.leave();
}

//-----------------------------------------------------------------------------
static jint openPuck(JNIEnv* jenv, jobject obj, jobject thisObj)
{
    jint    retVal = THOR_OK;
    void*   proxyHandle = nullptr;

    UNUSED(obj);

    ALOGD("openPuck");

    jenv->GetJavaVM(&gJavaVmPtr);

    dlerror();
    proxyHandle = dlopen("libUltrasoundSvcProxy.so", RTLD_LAZY);
    if (nullptr != proxyHandle)
    {
        void*           funcPtr = nullptr;
        const char*     errorMsg;

        dlerror();
        funcPtr = dlsym(proxyHandle, "getPuckContext");
        errorMsg = dlerror();
        if (nullptr != errorMsg)
        {
            ALOGE("Unable to get context data: %s", errorMsg);
            goto err_ret;
        }
        else
        {
            typedef ThorStatus      (*getPuckContextFunc)(puck_context_t&);
            getPuckContextFunc      getPuckContext = (getPuckContextFunc) funcPtr;
            puck_context_t          puckContext;
            thorpuck_open_code_t    retCode;

            retVal = getPuckContext(puckContext);
            if (IS_THOR_ERROR(retVal))
            {
                goto err_ret;
            }

            retCode = thorpuck_open(&puckContext, onPuckEvent, NULL, false, &gPuckHandle);
            if (THORPUCK_OPEN_FAIL == retCode)
            {
                ALOGE("Unable to access the Puck, trying bootloader for firmware update.");
                getPuckContext(puckContext);
                thorpuck_open(&puckContext, onPuckEvent, NULL, true, &gPuckHandle);
                retVal = TER_TABLET_PUCK_OPEN;
                goto err_ret;
            }
            else if (THORPUCK_OPEN_VER == retCode)
            {
                ALOGE("The Puck firmware version does not match expected value.");
                retVal = TER_TABLET_PUCK_FIRMWARE;
            }

            if (!thorpuck_start_scanning(gPuckHandle))
            {
                ALOGE("Unable to start scanning the Puck.");
                retVal = TER_TABLET_PUCK_SCAN;
                goto err_ret;
            }
        }
    }
    else
    {
        ALOGE("Unable to open UltrasoundSvcProxy: %s", dlerror());
        retVal = TER_UMGR_SVC_ACCESS;
        goto err_ret;
    }

err_ret:
    if (nullptr != proxyHandle)
    {
        dlclose(proxyHandle);
        proxyHandle = nullptr;
    }

    gPuckLock.enter();
    gJavaPuck = jenv->NewGlobalRef(thisObj);
    gPuckLock.leave();

    return(retVal);
}

//-----------------------------------------------------------------------------
static void closePuck(JNIEnv* jenv, jobject obj)
{
    UNUSED(obj);

    ALOGD("closePuck");

    if (nullptr != gPuckHandle)
    {
        thorpuck_stop_scanning(gPuckHandle);
        thorpuck_close(gPuckHandle);
        gPuckHandle = nullptr;
    }

    gPuckLock.enter();
    if (nullptr != gJavaPuck)
    {
        jenv->DeleteGlobalRef(gJavaPuck);
        gJavaPuck = nullptr;
    }
    gPuckLock.leave();

    return;
}

//-----------------------------------------------------------------------------
static jstring getFirmwareVersion(JNIEnv* jenv, jobject obj)
{
    char   version[] = "---";

    if (nullptr != gPuckHandle)
    {
        thorpuck_get_fw_version(gPuckHandle,
                                (uint8_t*) &version[0],
                                (uint8_t*) &version[1],
                                (uint8_t*) &version[2]);
    }

    return(jenv->NewStringUTF(version));
}

//-----------------------------------------------------------------------------
static jstring getLibraryVersion(JNIEnv* jenv, jobject obj)
{
    char   version[] = "---";

    thorpuck_get_lib_version( (uint8_t*) &version[0],
                              (uint8_t*) &version[1],
                              (uint8_t*) &version[2]);

    return(jenv->NewStringUTF(version));
}

//-----------------------------------------------------------------------------
static void* firmwareWorkerThread(void* dataPtr)
{
    UNUSED(dataPtr);

    if (nullptr != gPuckHandle)
    {
        thorpuck_stop_scanning(gPuckHandle);
        thorpuck_update_firmware(gPuckHandle);
    }
    else
    {
        thorpuck_event_t event;

        ALOGE("Unable to update firmware as the Puck is not opened.");

        memset(&event, 0, sizeof(event));
        event.updateStatus = TER_TABLET_PUCK_UPDATE_FIRMWARE;

        onPuckEvent(NULL, NULL, &event);
    }

    gPuckLock.enter();
    gFirmwareInProgress = false;
    gPuckLock.leave();

    return(nullptr);
}

//-----------------------------------------------------------------------------
static jint updateFirmware(JNIEnv* jenv, jobject obj)
{
    jint        retVal = THOR_OK;
    pthread_t   threadId;
    int         ret;
    bool        startUpdate = true;

    gPuckLock.enter();
    if (gFirmwareInProgress)
    {
        ALOGE("A Puck firmware update is already in progress");
        startUpdate = false;
        retVal = TER_TABLET_PUCK_UPDATE_FIRMWARE_BUSY;
    }
    gPuckLock.leave();

    if (startUpdate)
    {
        ret = pthread_create(&threadId, NULL, firmwareWorkerThread, NULL);
        if (0 != ret)
        {
            ALOGE("Failed to create worker thread for Puck firmware update: ret = %d", ret);
            retVal = TER_TABLET_PUCK_UPDATE_FIRMWARE;
        }
        else
        {
            gPuckLock.enter();
            gFirmwareInProgress = true;
            gPuckLock.leave();
        }
    }

    return(retVal);
}

//-----------------------------------------------------------------------------
static void setSingleClick(JNIEnv* jenv, jobject obj, jint min, jint max)
{
    UNUSED(jenv);
    UNUSED(obj);

    if (nullptr != gPuckHandle)
    {
        thorpuck_set_single_click(gPuckHandle, min, max);
    }
}

//-----------------------------------------------------------------------------
static void getSingleClick(JNIEnv* jenv, jobject obj, jintArray params)
{
    UNUSED(obj);

    uint16_t    minVal = 0;
    uint16_t    maxVal = 0;
    jint*       paramArray = jenv->GetIntArrayElements(params, NULL);

    if (nullptr != gPuckHandle)
    {
        thorpuck_get_single_click(gPuckHandle, &minVal, &maxVal);
    }

    paramArray[0] = minVal;
    paramArray[1] = maxVal;

    jenv->ReleaseIntArrayElements(params, paramArray, 0);
}

//-----------------------------------------------------------------------------
static void setDoubleClick(JNIEnv* jenv, jobject obj, jint min, jint max)
{
    UNUSED(jenv);
    UNUSED(obj);

    if (nullptr != gPuckHandle)
    {
        thorpuck_set_double_click(gPuckHandle, min, max);
    }
}

//-----------------------------------------------------------------------------
static void getDoubleClick(JNIEnv* jenv, jobject obj, jintArray params)
{
    UNUSED(obj);

    uint16_t    minVal = 0;
    uint16_t    maxVal = 0;
    jint*       paramArray = jenv->GetIntArrayElements(params, NULL);

    if (nullptr != gPuckHandle)
    {
        thorpuck_get_double_click(gPuckHandle, &minVal, &maxVal);
    }

    paramArray[0] = minVal;
    paramArray[1] = maxVal;

    jenv->ReleaseIntArrayElements(params, paramArray, 0);
}

//-----------------------------------------------------------------------------
static void setLongClick(JNIEnv* jenv, jobject obj, jint max)
{
    UNUSED(jenv);
    UNUSED(obj);

    if (nullptr != gPuckHandle)
    {
        thorpuck_set_long_press_max(gPuckHandle, max);
    }
}

//-----------------------------------------------------------------------------
static void getLongClick(JNIEnv* jenv, jobject obj, jintArray params)
{
    UNUSED(obj);

    uint16_t    maxVal = 0;
    jint*       paramArray = jenv->GetIntArrayElements(params, NULL);

    if (nullptr != gPuckHandle)
    {
        thorpuck_get_long_press_max(gPuckHandle, &maxVal);
    }

    paramArray[0] = maxVal;

    jenv->ReleaseIntArrayElements(params, paramArray, 0);
}

//-----------------------------------------------------------------------------
static void setScrollPos(JNIEnv* jenv, jobject obj, jintArray params)
{
    UNUSED(obj);

    jint* paramArray = jenv->GetIntArrayElements(params, NULL);

    uint8_t pos1 = paramArray[0];
    uint8_t pos2 = paramArray[1];
    uint8_t pos3 = paramArray[2];
    uint8_t pos4 = paramArray[3];

    if (nullptr != gPuckHandle)
    {
        thorpuck_set_scroll_positions(gPuckHandle, pos1, pos2, pos3, pos4);
    }

    jenv->ReleaseIntArrayElements(params, paramArray, 0);
}

//-----------------------------------------------------------------------------
static void getScrollPos(JNIEnv* jenv, jobject obj, jintArray params)
{
    UNUSED(obj);

    uint8_t pos1 = 0;
    uint8_t pos2 = 0;
    uint8_t pos3 = 0;
    uint8_t pos4 = 0;
    jint*   paramArray = jenv->GetIntArrayElements(params, NULL);

    if (nullptr != gPuckHandle)
    {
        thorpuck_get_scroll_positions(gPuckHandle, &pos1, &pos2, &pos3, &pos4);
    }

    paramArray[0] = pos1;
    paramArray[1] = pos2;
    paramArray[2] = pos3;
    paramArray[3] = pos4;

    jenv->ReleaseIntArrayElements(params, paramArray, 0);
}

//-----------------------------------------------------------------------------
static void setScrollSteps(JNIEnv* jenv, jobject obj, jintArray params)
{
    UNUSED(obj);

    jint* paramArray = jenv->GetIntArrayElements(params, NULL);

    uint8_t step1 = paramArray[0];
    uint8_t step2 = paramArray[1];
    uint8_t step3 = paramArray[2];
    uint8_t step4 = paramArray[3];

    if (nullptr != gPuckHandle)
    {
        thorpuck_set_scroll_steps(gPuckHandle, step1, step2, step3, step4);
    }

    jenv->ReleaseIntArrayElements(params, paramArray, 0);
}

//-----------------------------------------------------------------------------
static void getScrollSteps(JNIEnv* jenv, jobject obj, jintArray params)
{
    UNUSED(obj);

    uint8_t step1 = 0;
    uint8_t step2 = 0;
    uint8_t step3 = 0;
    uint8_t step4 = 0;
    jint*   paramArray = jenv->GetIntArrayElements(params, NULL);

    if (nullptr != gPuckHandle)
    {
        thorpuck_get_scroll_steps(gPuckHandle, &step1, &step2, &step3, &step4);
    }

    paramArray[0] = step1;
    paramArray[1] = step2;
    paramArray[2] = step3;
    paramArray[3] = step4;

    jenv->ReleaseIntArrayElements(params, paramArray, 0);
}

//-----------------------------------------------------------------------------
static void setDefault(JNIEnv* jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    if (nullptr != gPuckHandle)
    {
        thorpuck_reset_factory(gPuckHandle);
    }
}

//-----------------------------------------------------------------------------
static void reset(JNIEnv* jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    if (nullptr != gPuckHandle)
    {
        thorpuck_psoc_reset(gPuckHandle);
    }
}

//-----------------------------------------------------------------------------
static JNINativeMethod puck_method_table[] = {
    { "nativeOpen",
      "(Lcom/echonous/hardware/ultrasound/Puck;)I",
      (void*) openPuck
    },
    { "nativeClose",
      "()V",
      (void*) closePuck
    },
    { "nativeGetFirmwareVersion",
      "()Ljava/lang/String;",
      (void*) getFirmwareVersion
    },
    { "nativeGetLibraryVersion",
      "()Ljava/lang/String;",
      (void*) getLibraryVersion
    },
    { "nativeUpdateFirmware",
      "()I",
      (void*) updateFirmware
    },
    { "nativeSetSingleClick",
      "(II)V",
      (void*) setSingleClick
    },
    { "nativeGetSingleClick",
      "([I)V",
      (void*) getSingleClick
    },
    { "nativeSetDoubleClick",
      "(II)V",
      (void*) setDoubleClick
    },
    { "nativeGetDoubleClick",
      "([I)V",
      (void*) getDoubleClick
    },
    { "nativeSetLongClick",
      "(I)V",
      (void*) setLongClick
    },
    { "nativeGetLongClick",
      "([I)V",
      (void*) getLongClick
    },
    { "nativeSetScrollPos",
      "([I)V",
      (void*) setScrollPos
    },
    { "nativeGetScrollPos",
      "([I)V",
      (void*) getScrollPos
    },
    { "nativeSetScrollSteps",
      "([I)V",
      (void*) setScrollSteps
    },
    { "nativeGetScrollSteps",
      "([I)V",
      (void*) getScrollSteps
    },
    { "nativeSetDefault",
      "()V",
      (void*) setDefault
    },
    { "nativeReset",
      "()V",
      (void*) reset
    },
};

//-----------------------------------------------------------------------------
int register_puck_methods(JNIEnv* env)
{
    jclass puckClass = env->FindClass("com/echonous/hardware/ultrasound/Puck");
    LOG_FATAL_IF(puckClass == NULL, "Unable to find class com.echonous.hardware.ultrasound.Puck");

    onPuckEventId = env->GetMethodID(puckClass, "onPuckEvent", "(IIIIIIII)V");
    LOG_FATAL_IF(onPuckEventId == NULL, "Unable to find method onPuckEvent");

    return env->RegisterNatives(puckClass, puck_method_table, NELEM(puck_method_table)); 
}

};

