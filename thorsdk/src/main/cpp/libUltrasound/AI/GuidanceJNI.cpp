#pragma once
#define LOG_TAG "GuidanceJNI"

#include <GuidanceJNI.h>
#include <ThorUtils.h>

GuidanceJNI::GuidanceJNI(JNIEnv *jenv, jobject context) {
    jvm = nullptr;
    jenv->GetJavaVM(&jvm);
    this->jenv = nullptr; // thread local, this is set on the worker thread
    this->clazz = static_cast<jclass>(jenv->NewGlobalRef(
            jenv->FindClass("com/echonous/ai/guidance/NativeInterface")));
    this->context = jenv->NewGlobalRef(context);

    ctor = jenv->GetMethodID(clazz, "<init>", "(Landroid/content/Context;)V");
    instance = nullptr; // this is slow, initialize on worker
    load = jenv->GetMethodID(clazz, "load", "()V");
    processA4C = jenv->GetMethodID(clazz, "processA4C", "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)V");
    processA2C = jenv->GetMethodID(clazz, "processA2C", "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)V");
    processPLAX = jenv->GetMethodID(clazz, "processPLAX", "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)V");
    // Buffers are also initialized on worker thread
    imageBuffer = nullptr;
    outputBuffer = nullptr;
    imageBufferPLAX = nullptr;
    outputBufferPLAX = nullptr;

    LOGD("Created JNI interface, jvm = %p, jenv = %p, clazz = %p, instance = %p, processA4C = %p, processA2C = %p",
         jvm, jenv, clazz, instance, processA4C, processA2C);
}

void GuidanceJNI::attachToThread() {
    if (jenv != nullptr) {
        LOGE("Should not be attached to any thread!");
        return;
    }
    LOGD("Attaching to thread, creating instance...");
    jvm->AttachCurrentThread(&jenv, nullptr);
    instance = jenv->NewGlobalRef(jenv->NewObject(clazz, ctor, context));
    // instance is now a local ref, no need to add it as global or anything.

    // Create buffers on this thread as well
    imageBufferNative.resize(128*128);
    imageBuffer = jenv->NewGlobalRef(jenv->NewDirectByteBuffer(imageBufferNative.data(), (jlong)imageBufferNative.size() * sizeof(float)));

    // holds both subview and quality score confidences
    outputBufferNative.resize(23 + 5);
    outputBuffer = jenv->NewGlobalRef(jenv->NewDirectByteBuffer(outputBufferNative.data(), (jlong)outputBufferNative.size() * sizeof(float)));

    imageBufferPLAXNative.resize(224*224);
    imageBufferPLAX = jenv->NewGlobalRef(jenv->NewDirectByteBuffer(imageBufferPLAXNative.data(), (jlong)imageBufferPLAXNative.size() * sizeof(float)));

    outputBufferPLAXNative.resize(21+5);
    outputBufferPLAX = jenv->NewGlobalRef(jenv->NewDirectByteBuffer(outputBufferPLAXNative.data(), (jlong)outputBufferPLAXNative.size() * sizeof(float)));

    jenv->CallVoidMethod(instance, load);
    LOGD("Guidance pipeline loaded");
    jenv->DeleteGlobalRef(context);
    context = nullptr;
    LOGD("Attached to thread, instance = %p", instance);
}

GuidanceJNI::~GuidanceJNI() {
    LOGD("Destroying JNI interface");
    jenv->DeleteGlobalRef(outputBufferPLAX);
    jenv->DeleteGlobalRef(imageBufferPLAX);
    jenv->DeleteGlobalRef(outputBuffer);
    jenv->DeleteGlobalRef(imageBuffer);
    jenv->DeleteGlobalRef(instance);
    jenv->DeleteGlobalRef(clazz);
    jvm->DetachCurrentThread();
    jenv = nullptr;
}