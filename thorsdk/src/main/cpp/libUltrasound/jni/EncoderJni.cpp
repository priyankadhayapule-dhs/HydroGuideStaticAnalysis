//
// Copyright 2018 EchoNous Inc.
//
//  It is assumed that all parameters passed to the JNI functions exist and
//  are valid.  The parameters should be verified by the upper layer
//  Ultrasound Manager (in Java space).
//

#define LOG_TAG "EncoderJni"

#include <jni.h>
#include <android/bitmap.h>

#include <ThorUtils.h>
#include <ThorError.h>
#include <UltrasoundManager.h>
#include <UltrasoundEncoder.h>
#include <CriticalSection.h>

namespace echonous
{

struct EncoderContextIds
{
    jmethodID   onCompletionId;
} gEncoderContextIds;

static JavaVM*              gJavaVmPtr = nullptr;
static jobject              gJavaEncorder = nullptr;
static CriticalSection      gEncoderLock;

extern UltrasoundManager    gUltrasoundManager;

//-----------------------------------------------------------------------------
static void reportCompletion(ThorStatus completionCode)
{
    JNIEnv *jenv = nullptr;

    gEncoderLock.enter();
    ALOGD("Reporting encoding completion: 0x%x", completionCode);
    if (nullptr != gJavaVmPtr && nullptr != gJavaEncorder)
    {
        gJavaVmPtr->AttachCurrentThread(&jenv, NULL);
        if (nullptr == jenv)
        {
            ALOGE("Unable to get Android Runtime");
            goto err_ret;
        }
        jenv->CallVoidMethod(gJavaEncorder,
                             gEncoderContextIds.onCompletionId,
                             (jint)completionCode);
        gJavaVmPtr->DetachCurrentThread();
    }

err_ret:
    gEncoderLock.leave();
}

//-----------------------------------------------------------------------------
static void initEncoder(JNIEnv *jenv, jobject obj, jobject thisObj)
{
    UNUSED(obj);

    jenv->GetJavaVM(&gJavaVmPtr);

    gEncoderLock.enter();
    gJavaEncorder = jenv->NewGlobalRef(thisObj);
    gEncoderLock.leave();

    gUltrasoundManager.getUltrasoundEncoder().attachCompletionHandler(reportCompletion);
}

//-----------------------------------------------------------------------------
static void terminateEncoder(JNIEnv *jenv, jobject obj)
{
    UNUSED(obj);

    gUltrasoundManager.getUltrasoundEncoder().detachCompletionHandler();

    gEncoderLock.enter();
    if (nullptr != gJavaEncorder)
    {
        jenv->DeleteGlobalRef(gJavaEncorder);
        gJavaEncorder = nullptr;
    }
    gEncoderLock.leave();
}

//-----------------------------------------------------------------------------
static void cancelEncoding(JNIEnv *jenv, jobject obj)
{
    UNUSED(jenv);
    UNUSED(obj);

    gUltrasoundManager.getUltrasoundEncoder().requestCancel();
}

//-----------------------------------------------------------------------------
static jint processStillImage(JNIEnv *jenv,
                              jobject obj,
                              jstring path,
                              jfloatArray mat_,
                              jfloatArray win_,
                              jobject bitmap)
{
    jint retVal = THOR_ERROR;

    UNUSED(obj);

    const char *nativePath = jenv->GetStringUTFChars(path, 0);
    jfloat *mat = jenv->GetFloatArrayElements(mat_, NULL);
    jfloat *win = jenv->GetFloatArrayElements(win_, NULL);

    jsize winArrayLen = jenv->GetArrayLength(win_);

    // process bitmap object - rendering offscreen
    AndroidBitmapInfo androidBitmapInfo;
    void *pixels;
    if (AndroidBitmap_getInfo(jenv, bitmap, &androidBitmapInfo) < 0)
    {
        ALOGE("AndroidBitmap_getInfo() failed!");
        goto err_ret;
    }

    if (androidBitmapInfo.format != ANDROID_BITMAP_FORMAT_RGBA_8888)
    {
        ALOGE("Bitmap format is not RGBA_8888!");
        goto err_ret;
    }

    // locking pixels for reading
    if (AndroidBitmap_lockPixels(jenv, bitmap, &pixels) < 0)
    {
        ALOGE("AndroidBitmap_lockPixels() failed!");
        goto err_ret;
    }

    gUltrasoundManager.getUltrasoundEncoder().processStillImage(nativePath, mat, win, winArrayLen,
                                              (uint8_t *)pixels, androidBitmapInfo.width, androidBitmapInfo.height);

    jenv->ReleaseStringUTFChars(path, nativePath);
    jenv->ReleaseFloatArrayElements(mat_, mat, 0);
    jenv->ReleaseFloatArrayElements(win_, win, 0);

    retVal = THOR_OK;

err_ret:
    return (retVal);
}

//-----------------------------------------------------------------------------
static jint encodeStillImage(JNIEnv *jenv,
                             jobject obj,
                             jstring src_path,
                             jstring dst_path,
                             jfloatArray mat_,
                             jfloatArray win_,
                             jobject bitmap,
                             jint desiredFrameNum,
                             jboolean rawOut)
{
    jint retVal = THOR_ERROR;

    UNUSED(obj);

    const char *srcPath = jenv->GetStringUTFChars(src_path, 0);
    const char *dstPath = jenv->GetStringUTFChars(dst_path, 0);
    jfloat *mat = jenv->GetFloatArrayElements(mat_, NULL);
    jfloat *win = jenv->GetFloatArrayElements(win_, NULL);

    jsize winArrayLen = jenv->GetArrayLength(win_);

    // process bitmap object for overlay texture - rendering offscreen
    AndroidBitmapInfo androidBitmapInfo;
    void *pixels;
    if (AndroidBitmap_getInfo(jenv, bitmap, &androidBitmapInfo) < 0)
    {
        ALOGE("AndroidBitmap_getInfo() failed!");
        goto err_ret;
    }

    if (androidBitmapInfo.format != ANDROID_BITMAP_FORMAT_RGBA_8888)
    {
        ALOGE("Bitmap format is not RGBA_8888!");
        goto err_ret;
    }

    // locking pixels for reading
    if (AndroidBitmap_lockPixels(jenv, bitmap, &pixels) < 0)
    {
        ALOGE("AndroidBitmap_lockPixels() failed!");
        goto err_ret;
    }

    retVal = gUltrasoundManager.getUltrasoundEncoder().encodeStillImage(srcPath, dstPath, mat, win, winArrayLen,
                                                                        (uint8_t *)pixels, androidBitmapInfo.width, androidBitmapInfo.height, desiredFrameNum, rawOut);
    if (IS_THOR_ERROR(retVal))
        goto err_ret;

    retVal = THOR_OK;

err_ret:
    jenv->ReleaseStringUTFChars(src_path, srcPath);
    jenv->ReleaseStringUTFChars(dst_path, dstPath);
    jenv->ReleaseFloatArrayElements(mat_, mat, 0);
    jenv->ReleaseFloatArrayElements(win_, win, 0);

    return (retVal);
}

//-----------------------------------------------------------------------------
static jint encodeClip(JNIEnv *jenv,
                       jobject obj,
                       jstring src_path,
                       jstring dst_path,
                       jfloatArray mat_,
                       jfloatArray win_,
                       jobject bitmap,
                       jint startFrame,
                       jint endFrame,
                       jboolean rawOut,
                       jboolean forcedFSR)
{
    jint retVal = THOR_ERROR;

    UNUSED(obj);

    const char *srcPath = jenv->GetStringUTFChars(src_path, 0);
    const char *dstPath = jenv->GetStringUTFChars(dst_path, 0);
    jfloat *mat = jenv->GetFloatArrayElements(mat_, NULL);
    jfloat *win = jenv->GetFloatArrayElements(win_, NULL);

    jsize winArrayLen = jenv->GetArrayLength(win_);

    // process bitmap object for overlay texture - rendering offscreen
    AndroidBitmapInfo androidBitmapInfo;
    void *pixels;
    if (AndroidBitmap_getInfo(jenv, bitmap, &androidBitmapInfo) < 0)
    {
        ALOGE("AndroidBitmap_getInfo() failed!");
        goto err_ret;
    }

    if (androidBitmapInfo.format != ANDROID_BITMAP_FORMAT_RGBA_8888)
    {
        ALOGE("Bitmap format is not RGBA_8888!");
        goto err_ret;
    }

    // locking pixels for reading
    if (AndroidBitmap_lockPixels(jenv, bitmap, &pixels) < 0)
    {
        ALOGE("AndroidBitmap_lockPixels() failed!");
        goto err_ret;
    }

    retVal = gUltrasoundManager.getUltrasoundEncoder().encodeClip(srcPath, dstPath, mat, win, winArrayLen,
                                                                  (uint8_t *)pixels, androidBitmapInfo.width,
                                                                  androidBitmapInfo.height, startFrame, endFrame, rawOut, forcedFSR);
    if (IS_THOR_ERROR(retVal))
        goto err_ret;

    retVal = THOR_OK;

err_ret:
    jenv->ReleaseStringUTFChars(src_path, srcPath);
    jenv->ReleaseStringUTFChars(dst_path, dstPath);
    jenv->ReleaseFloatArrayElements(mat_, mat, 0);
    jenv->ReleaseFloatArrayElements(win_, win, 0);

    return (retVal);
}

//-----------------------------------------------------------------------------
static JNINativeMethod encoder_method_table[] =
    {
        {"nativeInit",
         "(Lcom/echonous/hardware/ultrasound/UltrasoundEncoder;)V",
         (void *)initEncoder},
        {"nativeTerminate",
         "()V",
         (void *)terminateEncoder},
        {"nativeCancelEncoding",
         "()V",
         (void *)cancelEncoding},
        {"nativeProcessStillImage",
         "(Ljava/lang/String;[F[FLandroid/graphics/Bitmap;)I",
         (void *)processStillImage},
        {"nativeEncodeStillImage",
         "(Ljava/lang/String;Ljava/lang/String;[F[FLandroid/graphics/Bitmap;IZ)I",
         (void *)encodeStillImage},
        {"nativeEncodeClip",
         "(Ljava/lang/String;Ljava/lang/String;[F[FLandroid/graphics/Bitmap;IIZZ)I",
         (void *)encodeClip},
};

//-----------------------------------------------------------------------------
int register_encoder_methods(JNIEnv *env)
{
    jclass encoderClass = env->FindClass("com/echonous/hardware/ultrasound/UltrasoundEncoder");
    LOG_FATAL_IF(encoderClass == NULL, "Unable to find class com.echonous.hardware.ultrasound.UltrasoundEncoder");

    gEncoderContextIds.onCompletionId = env->GetMethodID(encoderClass, "onCompletion", "(I)V");
    LOG_FATAL_IF(gEncoderContextIds.onCompletionId == NULL, "Unable to find method onCompletion");

    return env->RegisterNatives(encoderClass,
                                encoder_method_table,
                                NELEM(encoder_method_table));
}

};