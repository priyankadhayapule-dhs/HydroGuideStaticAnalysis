//
// Created by Tim-Echonous on 2024-05-21.
//
#include <jni.h>
#include <UltrasoundManager.h>
#include <AIManager.h>
#include <CardiacAnnotatorTask.h>
#include <FASTObjectDetectorTask.h>
#include <EFQualitySubviewTask.h>
#include <AutoDepthGainPresetTask.h>
#include <BladderSegmentationTask.h>
namespace echonous {
    // So, in all the verification functions, I passed in the Java side UltrasoundManager object,
    // thinking we might need it to get the native side equivalent. I forgot that the native side
    // UltrasoundManager is always a singleton stored in echonous::gUltrasoundManager, so the java
    // UltrasoundManager object is redundant.
    //
    // We can (and should) just use echonous::gUltrasoundManager to get the specific ML module and
    // call a member function to do verification through that.

    /**
     *  Singleton global UltrasoundManager (defined in ManagerJni.cpp)
     */
    extern UltrasoundManager gUltrasoundManager;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_echonous_ai_MLManager_nativeAutolabelVerification(JNIEnv *env, jobject thiz,
                                                           jobject manager, jstring input,
                                                           jstring output_root) {

    jboolean outputCStrIsCopy = JNI_FALSE;
    const char *outputCStr = env->GetStringUTFChars(output_root, &outputCStrIsCopy);

    jboolean inputCStrIsCopy = JNI_FALSE;
    const char *inputCStr = env->GetStringUTFChars(input, &inputCStrIsCopy);

    ThorStatus result = echonous::gUltrasoundManager
        .getAIManager()
        .getCardiacAnnotatorTask()
        ->runVerificationOnCineBuffer(env, inputCStr, outputCStr);

    env->ReleaseStringUTFChars(input, inputCStr);
    env->ReleaseStringUTFChars(output_root, outputCStr);
    return static_cast<jint>(result);
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_echonous_ai_MLManager_nativeA4CA2CGuidanceVerification(JNIEnv *env, jobject thiz,
                                                                jobject manager, jstring input,
                                                                jstring output_root) {
    jboolean inputCStrIsCopy = JNI_FALSE;
    const char *inputCStr = env->GetStringUTFChars(input, &inputCStrIsCopy);

    jboolean outputCStrIsCopy = JNI_FALSE;
    const char *outputCStr = env->GetStringUTFChars(output_root, &outputCStrIsCopy);

    ThorStatus result = echonous::gUltrasoundManager
            .getAIManager()
            .getEFQualityTask()
            ->runA4CA2CVerificationOnCinebuffer(env, inputCStr, outputCStr);

    env->ReleaseStringUTFChars(output_root, outputCStr);
    env->ReleaseStringUTFChars(input, inputCStr);
    return static_cast<jint>(result);
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_echonous_ai_MLManager_nativePLAXGuidanceVerification(JNIEnv *env, jobject thiz,
                                                              jobject manager, jstring input,
                                                              jstring output_root) {
    jboolean inputCStrIsCopy = JNI_FALSE;
    const char *inputCStr = env->GetStringUTFChars(input, &inputCStrIsCopy);

    jboolean outputCStrIsCopy = JNI_FALSE;
    const char *outputCStr = env->GetStringUTFChars(output_root, &outputCStrIsCopy);

    ThorStatus result = echonous::gUltrasoundManager
            .getAIManager()
            .getEFQualityTask()
            ->runPLAXVerificationOnCinebuffer(env, inputCStr, outputCStr);

    env->ReleaseStringUTFChars(output_root, outputCStr);
    env->ReleaseStringUTFChars(input, inputCStr);
    return static_cast<jint>(result);
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_echonous_ai_MLManager_nativeFASTVerification(JNIEnv *env, jobject thiz, jobject manager,
                                                      jstring input, jstring output_root) {
    jboolean inputCStrIsCopy = JNI_FALSE;
    const char *inputCStr = env->GetStringUTFChars(input, &inputCStrIsCopy);

    jboolean outputCStrIsCopy = JNI_FALSE;
    const char *outputCStr = env->GetStringUTFChars(output_root, &outputCStrIsCopy);

    ThorStatus result = echonous::gUltrasoundManager
            .getAIManager()
            .getFASTModule()
            ->runVerificationTest(env, inputCStr, outputCStr);

    env->ReleaseStringUTFChars(output_root, outputCStr);
    env->ReleaseStringUTFChars(input, inputCStr);
    return static_cast<jint>(result);
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_echonous_ai_MLManager_nativeEFVerification(JNIEnv *env, jobject thiz, jobject manager,
                                                    jstring inputA4C, jstring inputA2C, jstring output_root) {
    jboolean inputCStrIsCopy = JNI_FALSE;
    const char *inputCStrA4C = env->GetStringUTFChars(inputA4C, &inputCStrIsCopy);

    const char *inputCStrA2C = env->GetStringUTFChars(inputA2C, &inputCStrIsCopy);

    jboolean outputCStrIsCopy = JNI_FALSE;
    const char *outputCStr = env->GetStringUTFChars(output_root, &outputCStrIsCopy);

    ThorStatus result = echonous::gUltrasoundManager
            .getAIManager().runEfVerificationOnCineBuffer(env, inputCStrA4C, inputCStrA2C, outputCStr);

    env->ReleaseStringUTFChars(output_root, outputCStr);
    env->ReleaseStringUTFChars(inputA4C, inputCStrA4C);
    env->ReleaseStringUTFChars(inputA2C, inputCStrA2C);
    return static_cast<jint>(result);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_echonous_ai_MLManager_nativePresetVerification(JNIEnv *env, jobject thiz, jobject manager,
                                                      jstring input, jstring output_root) {
    jboolean inputCStrIsCopy = JNI_FALSE;
    const char *inputCStr = env->GetStringUTFChars(input, &inputCStrIsCopy);

    jboolean outputCStrIsCopy = JNI_FALSE;
    const char *outputCStr = env->GetStringUTFChars(output_root, &outputCStrIsCopy);

    ThorStatus result = echonous::gUltrasoundManager
            .getAIManager()
            .getAutoDepthGainPresetTask()
            ->runVerificationClip(inputCStr, outputCStr, env);

    env->ReleaseStringUTFChars(output_root, outputCStr);
    env->ReleaseStringUTFChars(input, inputCStr);
    return static_cast<jint>(result);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_echonous_ai_MLManager_nativeInit(JNIEnv *env, jclass clazz, jobject context) {
    // Nothing required, but kept in case of future requirements
    // Init here should get all the required jclass, jmethodID, jfield, etc needed for any
    // operations in this file.
}

extern "C"
JNIEXPORT void JNICALL
Java_com_echonous_ai_MLManager_nativePresetAttach(JNIEnv *env, jclass clazz) {
// Nothing required, but kept in case of future requirements
// Init here should get all the required jclass, jmethodID, jfield, etc needed for any
// operations in this file.
echonous::gUltrasoundManager.getAIManager().getAutoDepthGainPresetTask()->verificationInit(env);
}

extern "C"
JNIEXPORT jint JNICALL
        Java_com_echonous_ai_MLManager_nativeAutoDopplerPWVerification(JNIEnv *env, jobject thiz,
jobject manager, jstring input,
jstring output_root) {
const int PW = 1;
jboolean outputCStrIsCopy = JNI_FALSE;
const char *outputCStr = env->GetStringUTFChars(output_root, &outputCStrIsCopy);

jboolean inputCStrIsCopy = JNI_FALSE;
const char *inputCStr = env->GetStringUTFChars(input, &inputCStrIsCopy);

ThorStatus result = echonous::gUltrasoundManager
        .getAIManager()
        .getCardiacAnnotatorTask()
        ->runVerificationOnCineBuffer(env, inputCStr, outputCStr, PW);

env->ReleaseStringUTFChars(input, inputCStr);
env->ReleaseStringUTFChars(output_root, outputCStr);
return static_cast<jint>(result);
}

extern "C"
JNIEXPORT jint JNICALL
        Java_com_echonous_ai_MLManager_nativeAutoDopplerTDIVerification(JNIEnv *env, jobject thiz,
jobject manager, jstring input,
jstring output_root) {
const int TDI = 2;
jboolean outputCStrIsCopy = JNI_FALSE;
const char *outputCStr = env->GetStringUTFChars(output_root, &outputCStrIsCopy);

jboolean inputCStrIsCopy = JNI_FALSE;
const char *inputCStr = env->GetStringUTFChars(input, &inputCStrIsCopy);

ThorStatus result = echonous::gUltrasoundManager
        .getAIManager()
        .getCardiacAnnotatorTask()
        ->runVerificationOnCineBuffer(env, inputCStr, outputCStr, TDI);

env->ReleaseStringUTFChars(input, inputCStr);
env->ReleaseStringUTFChars(output_root, outputCStr);
return static_cast<jint>(result);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_echonous_ai_MLManager_nativeBladderVerification(JNIEnv *env, jobject thiz,
                                                                jobject manager, jstring input,
                                                                jstring output_root) {
    jboolean inputCStrIsCopy = JNI_FALSE;
    const char *inputCStr = env->GetStringUTFChars(input, &inputCStrIsCopy);

    jboolean outputCStrIsCopy = JNI_FALSE;
    const char *outputCStr = env->GetStringUTFChars(output_root, &outputCStrIsCopy);

    ThorStatus result = echonous::gUltrasoundManager
            .getAIManager()
            .getBladderSegmentationTask()->runVerificationTest(env, inputCStr, outputCStr);

    env->ReleaseStringUTFChars(output_root, outputCStr);
    env->ReleaseStringUTFChars(input, inputCStr);
    return static_cast<jint>(result);
}