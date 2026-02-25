//
// Created by Tim-Echonous on 2024-04-30.
//
#define LOG_TAG "EFNativeJNI"
#include <ThorUtils.h>
#include "EFNativeJNI.h"

EFNativeJNI::EFNativeJNI(JNIEnv *jenv, jobject context) {
    jvm = nullptr;
    jenv->GetJavaVM(&jvm);
    this->jenv = nullptr; // thread local, this is set on the worker thread
    this->clazz = static_cast<jclass>(jenv->NewGlobalRef(
            jenv->FindClass("com/echonous/ai/ef/NativeInterface")));
    this->context = jenv->NewGlobalRef(context);

    ctor = jenv->GetMethodID(clazz, "<init>", "(Landroid/content/Context;)V");
    instance = nullptr; // this is slow, initialize on worker
    load = jenv->GetMethodID(clazz, "load", "()V");
    runPhaseDetection = jenv->GetMethodID(clazz, "runPhaseDetection",
                                          "(Ljava/nio/ByteBuffer;)F");
    runSegmentationES = jenv->GetMethodID(clazz, "runSegmentationES",
                                          "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)V");
    runSegmentationED = jenv->GetMethodID(clazz, "runSegmentationED",
                                          "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)V");
    close = jenv->GetMethodID(clazz, "close", "()V");

    // Buffers are also initialized on worker thread
    phaseDetectionImageBuffer = nullptr;
    segmentationImageBuffer = nullptr;
    segmentationConfMapBuffer = nullptr;

    LOGD("Created JNI interface, jvm = %p, jenv = %p, clazz = %p, instance = %p",
         jvm, jenv, clazz, instance);
}

void EFNativeJNI::attachToThread() {
    if (jenv != nullptr) {
        LOGE("Should not be attached to any thread!");
        return;
    }
    LOGD("Attaching to thread, creating instance...");
    jvm->AttachCurrentThread(&jenv, nullptr);
    instance = jenv->NewGlobalRef(jenv->NewObject(clazz, ctor, context));
    // instance is now a local ref, no need to add it as global or anything.

    // Create buffers on this thread as well
    // TODO the sizes here should come from some #define or other constants?
    phaseDetectionImageBufferNative.resize(128*128);
    phaseDetectionImageBuffer = jenv->NewGlobalRef(jenv->NewDirectByteBuffer(phaseDetectionImageBufferNative.data(),
                                                          (jlong)phaseDetectionImageBufferNative.size() * sizeof(float)));

    segmentationImageBufferNative.resize(128*128*5);
    segmentationImageBuffer = jenv->NewGlobalRef(jenv->NewDirectByteBuffer(segmentationImageBufferNative.data(),
                                                        (jlong)segmentationImageBufferNative.size() * sizeof(float)));

    segmentationConfMapBufferNative.resize(128*128*3*3);
    segmentationConfMapBuffer = jenv->NewGlobalRef(jenv->NewDirectByteBuffer(segmentationConfMapBufferNative.data(),
                                                          (jlong)segmentationConfMapBufferNative.size() * sizeof(float)));

    jenv->CallVoidMethod(instance, load);
    LOGD("EF TFLite backend loaded");

    jenv->DeleteGlobalRef(context);
    context = nullptr;
    LOGD("Attached to thread, instance = %p", instance);
}

EFNativeJNI::~EFNativeJNI() {
    LOGD("Destroying JNI interface");
    jenv->CallVoidMethod(instance, close);
    jenv->DeleteGlobalRef(segmentationConfMapBuffer);
    jenv->DeleteGlobalRef(segmentationImageBuffer);
    jenv->DeleteGlobalRef(phaseDetectionImageBuffer);
    jenv->DeleteGlobalRef(instance);
    jenv->DeleteGlobalRef(clazz);
    jvm->DetachCurrentThread();
    jenv = nullptr;
}