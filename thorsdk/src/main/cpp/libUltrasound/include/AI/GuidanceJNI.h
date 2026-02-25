#pragma once
#include <vector>
#include <jni.h>

struct GuidanceJNI {
    JavaVM *jvm;
    JNIEnv *jenv;
    jclass clazz;
    jobject context;
    jobject instance;
    jmethodID ctor;
    jmethodID load;
    jmethodID processA4C;
    jmethodID processA2C;
    jmethodID processPLAX;
    jobject imageBuffer;
    std::vector<float> imageBufferNative;
    jobject outputBuffer;
    std::vector<float> outputBufferNative;

    jobject imageBufferPLAX;
    std::vector<float> imageBufferPLAXNative;
    jobject outputBufferPLAX;
    std::vector<float> outputBufferPLAXNative;

    GuidanceJNI(JNIEnv *jenv, jobject context);

    void attachToThread();

    ~GuidanceJNI();
};