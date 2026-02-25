//
// Copyright 2017 EchoNous Inc.
//
//  It is assumed that all parameters passed to the JNI functions exist and
//  are valid.  The parameters should be verified by the upper layer
//  Ultrasound Manager (in Java space).
//

#define LOG_TAG "ViewerJni"

#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/bitmap.h>

#include <ThorUtils.h>
#include <UltrasoundManager.h>

namespace echonous
{

struct ViewerIds
{
    jmethodID onHeartRateUpdateId;
    jmethodID onErrorId;
} gViewerIds;

static JavaVM*              gJavaVmPtr = nullptr;
static jobject              gJavaViewer = nullptr;
static CriticalSection      gViewerLock;
extern UltrasoundManager    gUltrasoundManager;

static void reportHeartUpdate(float heartRate){
    JNIEnv*     jenv = nullptr;

    gViewerLock.enter();
    if (nullptr != gJavaVmPtr && nullptr != gJavaViewer)
    {
        gJavaVmPtr->AttachCurrentThread(&jenv, NULL);
        if (nullptr == jenv)
        {
            ALOGE("Unable to get Android Runtime During HeartRate");
            goto err_ret;
        }
        jenv->CallVoidMethod(gJavaViewer, gViewerIds.onHeartRateUpdateId, (jfloat) heartRate);
        gJavaVmPtr->DetachCurrentThread();
    }
err_ret:
    gViewerLock.leave();
}

//-----------------------------------------------------------------------------
static void reportError(uint32_t errorCode)
{
    JNIEnv*     jenv = nullptr;

    gViewerLock.enter();
    ALOGE("Reporting error: 0x%x", errorCode);
    if (nullptr != gJavaVmPtr && nullptr != gJavaViewer)
    {
        gJavaVmPtr->AttachCurrentThread(&jenv, NULL);
        if (nullptr == jenv)
        {
            ALOGE("Unable to get Android Runtime");
            goto err_ret;
        }
        jenv->CallVoidMethod(gJavaViewer, gViewerIds.onErrorId, (jint) errorCode);
        gJavaVmPtr->DetachCurrentThread();
    }

err_ret:
    gViewerLock.leave();
}

//-----------------------------------------------------------------------------
static void initViewer (JNIEnv* jenv, jobject obj, jobject thisViewer){
    UNUSED(obj);

    jenv->GetJavaVM(&gJavaVmPtr);
    if (nullptr == gJavaViewer) {
        gJavaViewer = jenv->NewGlobalRef(thisViewer);
    }
    gUltrasoundManager.getCineViewer().attachCallbacks(reportHeartUpdate, reportError);
}

//-----------------------------------------------------------------------------
static jint setSurface(JNIEnv* jenv, jobject obj, jobject surface)
{
    jint                retVal = THOR_OK;
    ANativeWindow*      windowPtr = nullptr;

    UNUSED(obj);

    windowPtr = ANativeWindow_fromSurface(jenv, surface);
    if (nullptr == windowPtr)
    {
        ALOGE("Unable to get native window");
        retVal = TER_UMGR_NATIVE_WINDOW;
        goto err_ret;
    }

    gUltrasoundManager.getCineViewer().setWindow(windowPtr);

err_ret:
    if (nullptr != windowPtr)
    {
        ANativeWindow_release(windowPtr);
    }

    return(retVal);
}

//-----------------------------------------------------------------------------
static void releaseSurface(JNIEnv *jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    gUltrasoundManager.getCineViewer().setWindow(nullptr);
}

//-----------------------------------------------------------------------------
static jint setMmodeLayout(JNIEnv* jenv, jobject obj,
                           jfloatArray bmodeLayout_, jfloatArray mmodeLayout_)
{
    UNUSED(obj);
    jfloat *bmodeLayout = jenv->GetFloatArrayElements(bmodeLayout_, NULL);
    jfloat *mmodeLayout = jenv->GetFloatArrayElements(mmodeLayout_, NULL);
    jint retVal = THOR_OK;

    retVal = gUltrasoundManager.getCineViewer().setMmodeLayout(bmodeLayout, mmodeLayout);

    jenv->ReleaseFloatArrayElements(bmodeLayout_, bmodeLayout, 0);
    jenv->ReleaseFloatArrayElements(mmodeLayout_, mmodeLayout, 0);

    return(retVal);
}
        
//-----------------------------------------------------------------------------
static void handleOnScroll(JNIEnv* jenv, jobject obj, jfloat x, jfloat y)
{
    UNUSED(jenv);
    UNUSED(obj);

    gUltrasoundManager.getCineViewer().handleOnScroll(x, y);
}

//-----------------------------------------------------------------------------
static void handleOnScale(JNIEnv* jenv, jobject obj, jfloat scaleFactor)
{
    UNUSED(jenv);
    UNUSED(obj);

    gUltrasoundManager.getCineViewer().handleOnScale(scaleFactor);
}

//-----------------------------------------------------------------------------
static void queryDisplayDepth(JNIEnv* jenv, jobject obj, jfloatArray xy_)
{
    UNUSED(obj);

    jfloat *xy = jenv->GetFloatArrayElements(xy_, NULL);

    gUltrasoundManager.getCineViewer().queryDisplayDepth(xy[0], xy[1]);

    jenv->ReleaseFloatArrayElements(xy_, xy, 0);
}

//-----------------------------------------------------------------------------
static void handleOnTouch(JNIEnv* jenv, jobject obj, jfloat x, jfloat y, jboolean isDown)
{
    UNUSED(jenv);
    UNUSED(obj);

    gUltrasoundManager.getCineViewer().handleOnTouch(x, y, isDown);
}

//-----------------------------------------------------------------------------
static void queryPhysicalToPixelMap(JNIEnv* jenv, jobject obj, jfloatArray mat_)
{
    UNUSED(obj);

    jfloat *mat = jenv->GetFloatArrayElements(mat_, NULL);

    jsize  matArrayLength = jenv->GetArrayLength(mat_);

    gUltrasoundManager.getCineViewer().queryPhysicalToPixelMap(mat, matArrayLength);

    jenv->ReleaseFloatArrayElements(mat_, mat, 0);
}

//-----------------------------------------------------------------------------
static void queryZoomPanFlipParams(JNIEnv *jenv, jobject obj, jfloatArray mat_)
{
    UNUSED(obj);

    jfloat *mat = jenv->GetFloatArrayElements(mat_, NULL);

    gUltrasoundManager.getCineViewer().queryZoomPanFlipParams(mat);

    jenv->ReleaseFloatArrayElements(mat_, mat, 0);
}

//-----------------------------------------------------------------------------
static void setZoomPanFlipParams(JNIEnv *jenv, jobject obj, jfloatArray mat_)
{
    UNUSED(obj);

    jfloat *mat = jenv->GetFloatArrayElements(mat_, NULL);

    gUltrasoundManager.getCineViewer().setZoomPanFlipParams(mat);

    jenv->ReleaseFloatArrayElements(mat_, mat, 0);
}

//-----------------------------------------------------------------------------
static jfloat getTraceLineTime(JNIEnv* jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    return (gUltrasoundManager.getCineViewer().getTraceLineTime());
}

//-----------------------------------------------------------------------------
static jint getFrameIntervalMs(JNIEnv* jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    return (gUltrasoundManager.getCineViewer().getFrameIntervalMs());
}

//-----------------------------------------------------------------------------
static jint getMLinesPerFrame(JNIEnv* jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    return (gUltrasoundManager.getCineViewer().getMLinesPerFrame());
}

//-----------------------------------------------------------------------------
static void getUltrasoundScreenImage(JNIEnv* jenv, jobject obj, jobject bitmap,
                                     jboolean isForThumbnail, jint desiredFrameNum)
{
    UNUSED(obj);

    AndroidBitmapInfo androidBitmapInfo ;
    void* pixels;
    if (AndroidBitmap_getInfo(jenv, bitmap, &androidBitmapInfo)<0)
    {
        ALOGE("AndroidBitmap_getInfo() failed!");
    }

    if (androidBitmapInfo.format != ANDROID_BITMAP_FORMAT_RGBA_8888)
    {        
        ALOGE("Bitmap format is not RGBA_8888!");
        return;
    }

    // reading pixels
    if (AndroidBitmap_lockPixels(jenv, bitmap, &pixels)<0)
    {
        ALOGE("AndroidBitmap_lockPixels() failed!");
        return;
    }

    gUltrasoundManager.getCineViewer().getUltrasoundScreenImage((uint8_t *)pixels,
            androidBitmapInfo.width, androidBitmapInfo.height, isForThumbnail, desiredFrameNum);

    AndroidBitmap_unlockPixels(jenv, bitmap);
}

//-----------------------------------------------------------------------------
static jint setDAECGLayout(JNIEnv* jenv, jobject obj,
                           jfloatArray usLayout_, jfloatArray daLayout_, jfloatArray ecgLayout_)
{
    UNUSED(obj);
    jfloat *usLayout = jenv->GetFloatArrayElements(usLayout_, NULL);
    jfloat *daLayout = jenv->GetFloatArrayElements(daLayout_, NULL);
    jfloat *ecgLayout = jenv->GetFloatArrayElements(ecgLayout_, NULL);

    jint retVal = THOR_OK;

    retVal = gUltrasoundManager.getCineViewer().setDAECGLayout(usLayout, daLayout, ecgLayout);

    jenv->ReleaseFloatArrayElements(usLayout_, usLayout, 0);
    jenv->ReleaseFloatArrayElements(daLayout_, daLayout, 0);
    jenv->ReleaseFloatArrayElements(ecgLayout_, ecgLayout, 0);

    return(retVal);
}

//-----------------------------------------------------------------------------
static jint setDopplerLayout(JNIEnv* jenv, jobject obj,
                           jfloatArray dopplerLayout_)
{
    UNUSED(obj);
    jfloat *dopplerLayout = jenv->GetFloatArrayElements(dopplerLayout_, NULL);

    jint retVal = THOR_OK;

    retVal = gUltrasoundManager.getCineViewer().setDopplerLayout(dopplerLayout);

    jenv->ReleaseFloatArrayElements(dopplerLayout_, dopplerLayout, 0);

    return(retVal);
}

//------------------------------------------------------------------------------
static void updateDopplerBaselineShiftInvert(JNIEnv* jenv, jobject obj,
        jint baselineShiftIndex,jboolean isInvert){
    UNUSED(jenv);
    UNUSED(obj);
    gUltrasoundManager.getCineViewer().updateDopplerBaselineShiftInvert(baselineShiftIndex,isInvert);
}


//------------------------------------------------------------------------------
static void setPeakMeanDrawing(JNIEnv* jenv, jobject obj,
                                   jboolean peakDrawing,jboolean meanDrawing){
     UNUSED(jenv);
     UNUSED(obj);
     gUltrasoundManager.getCineViewer().setPeakMeanDrawing(peakDrawing,meanDrawing);
}

//-----------------------------------------------------------------------------
static jint prepareDopplerTransition(JNIEnv* jenv, jobject obj,jboolean prepareTransitionFrame)
{
    UNUSED(obj);
    jint retVal = THOR_OK;

    // prepareTranstition.
    retVal = gUltrasoundManager.getCineViewer().prepareDopplerTransition(prepareTransitionFrame);

    return(retVal);
}

//------------------------------------------------------------------------------
static jfloat getDopplerPeakMax(JNIEnv *jenv, jobject obj) {
    UNUSED(jenv);
    UNUSED(obj);
    return gUltrasoundManager.getCineViewer().getDopplerPeakMax();
}

//------------------------------------------------------------------------------
static void getDopplerPeakMaxCoordinate(JNIEnv *jenv, jobject obj,
                                        jfloatArray mapMatrix_, jint arrayLength, jboolean isTracelineInvert) {
    UNUSED(obj);
    jfloat *mapMatrix = jenv->GetFloatArrayElements(mapMatrix_, NULL);
    gUltrasoundManager.getCineViewer().getDopplerPeakMaxCoordinate(mapMatrix, arrayLength, isTracelineInvert);
    jenv->ReleaseFloatArrayElements(mapMatrix_, mapMatrix, 0);
}

//------------------------------------------------------------------------------
    static int setTimeAvgCoOrdinates(JNIEnv *jenv,
                                     jobject obj,
                                     jfloatArray peakMaxPositive_,
                                     jfloatArray peakMeanPositive_,
                                     jfloatArray peakMaxNegative_,
                                     jfloatArray peakMeanNegative_) {
        jfloat *peakMaxPositive = jenv->GetFloatArrayElements(peakMaxPositive_, nullptr);
        jfloat *peakMeanPositive = jenv->GetFloatArrayElements(peakMeanPositive_, nullptr);
        jfloat *peakMaxNegative = jenv->GetFloatArrayElements(peakMaxNegative_, nullptr);
        jfloat *peakMeanNegative = jenv->GetFloatArrayElements(peakMeanNegative_, nullptr);

        int sizeOfArray = gUltrasoundManager.getCineViewer().setTimeAvgCoOrdinates(peakMaxPositive, peakMeanPositive,
                                                                                   peakMaxNegative, peakMeanNegative);

        jenv->ReleaseFloatArrayElements(peakMaxPositive_, peakMaxPositive, 0);
        jenv->ReleaseFloatArrayElements(peakMeanPositive_, peakMeanPositive, 0);
        jenv->ReleaseFloatArrayElements(peakMaxNegative_, peakMaxNegative, 0);
        jenv->ReleaseFloatArrayElements(peakMeanNegative_, peakMeanNegative, 0);

        return sizeOfArray;
    }

//-----------------------------------------------------------------------------
static jint setupUSDAECG(JNIEnv* jenv, jobject obj,
                        jboolean isUSOn,jboolean isDAOn, jboolean isECGOn)
{
    UNUSED(jenv);
    UNUSED(obj);

    jint retVal = THOR_OK;

    retVal = gUltrasoundManager.getCineViewer().setupUSDAECG(isUSOn,isDAOn, isECGOn);

    return(retVal);
}

//-----------------------------------------------------------------------------
static jint refreshLayout(JNIEnv* jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    jint retVal = THOR_OK;

    retVal = gUltrasoundManager.getCineViewer().refreshLayout();

    return(retVal);
}

//-----------------------------------------------------------------------------
static jfloat getScale(JNIEnv* jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    return gUltrasoundManager.getCineViewer().getScale();
}

static void setScale(JNIEnv* jenv, jobject obj,jfloat scaleFactor)
{
    UNUSED(jenv);
    UNUSED(obj);

    gUltrasoundManager.getCineViewer().setScale(scaleFactor);

}

static void setPan(JNIEnv* jenv, jobject obj,jfloat deltaX,jfloat deltaY)
{
    UNUSED(jenv);
    UNUSED(obj);
    gUltrasoundManager.getCineViewer().setPan(deltaX,deltaY);
}

//-----------------------------------------------------------------------------
static jint setColorMap(JNIEnv* jenv, jobject obj, jint colorMapId, jboolean isInverted)
{
    UNUSED(jenv);
    UNUSED(obj);

    jint retVal = THOR_OK;

    retVal = gUltrasoundManager.getCineViewer().setColorMap(colorMapId, isInverted);

    return(retVal);
}

//-----------------------------------------------------------------------------
static void setDaEcgScrollSpeedIndex(JNIEnv* jenv, jobject obj, jint speedIndex)
{
    UNUSED(jenv);
    UNUSED(obj);

    gUltrasoundManager.getCineViewer().setDaEcgScrollSpeedIndex(speedIndex);
}

//-----------------------------------------------------------------------------
static jboolean getCineViewerInitialisedStatus(JNIEnv* jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    return gUltrasoundManager.getCineViewer().isInitialised();
}

//-----------------------------------------------------------------------------
static JNINativeMethod viewer_method_table[] =
{
    { "setSurfaceNative",
      "(Landroid/view/Surface;)I",
      (void*) setSurface
    },
    { "releaseSurfaceNative",
      "()V",
      (void*) releaseSurface
    },
    { "setMmodeLayoutNative",
      "([F[F)I",
      (void*) setMmodeLayout
    },
    { "setZoomPanFlipParamsNative",
      "([F)V",
      (void*) setZoomPanFlipParams
    },
    { "nativeHandleOnScroll",
      "(FF)V",
      (void*) handleOnScroll
    },
    { "nativeHandleOnScale",
      "(F)V",
      (void*) handleOnScale
    },
    { "nativeQueryDisplayDepth",
      "([F)V",
      (void*) queryDisplayDepth
    },
    { "nativeHandleOnTouch",
      "(FFZ)V",
      (void*) handleOnTouch
    },
    { "nativeQueryPhysicalToPixelMap",
      "([F)V",
      (void*) queryPhysicalToPixelMap
    },
    { "nativeQueryZoomPanFlipParams",
      "([F)V",
      (void*) queryZoomPanFlipParams
    },
    { "nativeGetUltrasoundScreenImage",
      "(Landroid/graphics/Bitmap;ZI)V",
      (void*) getUltrasoundScreenImage
    },
    { "nativeGetTraceLineTime",
      "()F",
      (void*) getTraceLineTime
    },
    { "nativeGetFrameIntervalMs",
      "()I",
      (void*)getFrameIntervalMs
    },
    { "nativeGetMLinesPerFrame",
      "()I",
      (void*) getMLinesPerFrame
    },
    { "setupDAECGNative",
      "(ZZZ)I",
      (void*) setupUSDAECG
    },
    { "setDAECGLayoutNative",
      "([F[F[F)I",
      (void*) setDAECGLayout
    },
    { "setDopplerLayoutNative",
      "([F)I",
      (void*) setDopplerLayout
    },
    { "updateDopplerBaselineShiftInvertNative",
      "(IZ)V",
      (void*) updateDopplerBaselineShiftInvert
    },
    { "setPeakMeanDrawingNative",
            "(ZZ)V",
            (void*) setPeakMeanDrawing
    },
    { "prepareDopplerTransitionNative",
      "(Z)I",
      (void*) prepareDopplerTransition
    },
    { "refreshLayoutNative",
      "()I",
      (void*) refreshLayout
    },
    { "getDopplerPeakMaxNative",
      "()F",
      (void*) getDopplerPeakMax
    },
    { "getDopplerPeakMaxCoordinateNative",
      "([FIZ)V",
      (void*) getDopplerPeakMaxCoordinate
    },
    { "setTimeAvgCoOrdinatesNative",
            "([F[F[F[F)I",
            (void *) setTimeAvgCoOrdinates
    },
    {
      "setColorMapNative",
      "(IZ)I",
      (void*) setColorMap
    },
    { "getScale",
      "()F",
       (void*) getScale
    },
    {
      "setDaEcgScrollSpeedIndexNative",
      "(I)V",
      (void*) setDaEcgScrollSpeedIndex
    },
    {
      "setScale",
      "(F)V",
      (void*) setScale
    },{
       "setPan",
       "(FF)V",
       (void*) setPan
    },
    { "nativeInit",
      "(Lcom/echonous/hardware/ultrasound/UltrasoundViewer;)V",
      (void*) initViewer
    },
    { "nativeGetCineViewerInitialisedStatus",
            "()Z",
      (void*) getCineViewerInitialisedStatus
    }
};

//-----------------------------------------------------------------------------
int register_viewer_methods(JNIEnv* env)
{
    jclass viewerClass = env->FindClass("com/echonous/hardware/ultrasound/UltrasoundViewer");
    LOG_FATAL_IF(viewerClass == NULL, "Unable to find class com.echonous.hardware.ultrasound.UltrasoundViewer");

    gViewerIds.onHeartRateUpdateId = env->GetMethodID(viewerClass, "onHeartRateUpdate", "(F)V");
    LOG_FATAL_IF(gViewerIds.onHeartRateUpdateId == NULL, "Unable to find method onHeartRateUpdate");

    gViewerIds.onErrorId = env->GetMethodID(viewerClass, "onError", "(I)V");
    LOG_FATAL_IF(gViewerIds.onErrorId == NULL, "Unable to find method onError");

    return env->RegisterNatives(viewerClass,
                                viewer_method_table,
                                NELEM(viewer_method_table));
}


};
