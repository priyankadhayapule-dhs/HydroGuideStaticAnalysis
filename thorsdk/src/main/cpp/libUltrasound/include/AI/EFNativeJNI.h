//
// Created by Tim-Echonous on 2024-04-30.
//

#pragma once
#include <vector>
#include <jni.h>

// Things needed to call JNI interface
struct EFNativeJNI {
    JavaVM *jvm;
    JNIEnv *jenv;
    jclass clazz;
    jobject context;
    jobject instance;
    jmethodID ctor;
    jmethodID load;
    jmethodID runPhaseDetection;
    jmethodID runSegmentationES;
    jmethodID runSegmentationED;
    jmethodID close;
    jobject phaseDetectionImageBuffer;
    std::vector<float> phaseDetectionImageBufferNative;
    jobject segmentationImageBuffer;
    std::vector<float> segmentationImageBufferNative;
    jobject segmentationConfMapBuffer;
    std::vector<float> segmentationConfMapBufferNative;

    // called on main UI thread during init
    EFNativeJNI(JNIEnv *jenv, jobject context);

    // called on native background thread
    void attachToThread();

    ~EFNativeJNI();
};
