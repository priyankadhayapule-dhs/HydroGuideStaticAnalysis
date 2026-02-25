//
// Created by Michael May on 4/24/25.
//

#pragma once
#include <jni.h>
#include <vector>
struct BladderSegmentationJNI {
    JavaVM* jvm;
    JNIEnv* jenv;
    jclass clazz;
    jobject context;
    jobject instance;
    jmethodID ctor;
    jmethodID load;
    jmethodID process;
    jmethodID clear;
    jobject imageBuffer;
    std::vector<float> imageBufferNative;
    jobject viewBuffer;
    std::vector<float> viewBufferNative;
    jobject pMapBuffer;
    std::vector<float> pMapBufferNative;
    const int BVW_MODEL_WIDTH = 128;
    const int BVW_MODEL_HEIGHT = 128;
    const int VIEW_BUFFER_SIZE = 5;
    const int PMAP_BUFFER_SIZE = 128*128*2;
    bool isAttached;
    BladderSegmentationJNI(JNIEnv *jenv, jobject context) {
        jvm = nullptr;
        jenv->GetJavaVM(&jvm);
        this->jenv = nullptr; // thread local, this is set on the worker thread
        this->clazz = static_cast<jclass>(jenv->NewGlobalRef(
                jenv->FindClass("com/echonous/ai/bladder/NativeInterface")));
        this->context = jenv->NewGlobalRef(context);
        ctor = jenv->GetMethodID(clazz, "<init>", "(Landroid/content/Context;)V");
        instance = nullptr; // this is slow, initialize on worker
        load = jenv->GetMethodID(clazz, "load", "()V");
        process = jenv->GetMethodID(clazz, "processOrdered", "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;I)V");
        imageBuffer = nullptr;
        viewBuffer = nullptr;
        pMapBuffer = nullptr;

        LOGD("[ BLADDER ] Created Bladder JNI interface, jvm = %p, jenv = %p, clazz = %p, instance = %p, process = %p, clear = %p",
             jvm, jenv, clazz, instance, clear);
        isAttached = false;
    }

    void attachToThread() {
        if (jenv != nullptr) {
            LOGE("Should not be attached to any thread!");
            return;
        }
        LOGD("Attaching to thread, creating instance...");
        jvm->AttachCurrentThread(&jenv, nullptr);
        instance = jenv->NewGlobalRef(jenv->NewObject(clazz, ctor, context));
        // instance is now a local ref, no need to add it as global or anything.

        // Create buffers on this thread as well
        imageBufferNative.resize(BVW_MODEL_WIDTH*BVW_MODEL_HEIGHT);
        imageBuffer = jenv->NewGlobalRef(jenv->NewDirectByteBuffer(imageBufferNative.data(), (jlong)imageBufferNative.size() * sizeof(float)));
        viewBufferNative.resize(VIEW_BUFFER_SIZE);
        viewBuffer = jenv->NewGlobalRef(jenv->NewDirectByteBuffer(viewBufferNative.data(), jlong(viewBufferNative.size() * sizeof(float))));
        pMapBufferNative.resize(PMAP_BUFFER_SIZE);
        pMapBuffer = jenv->NewGlobalRef(jenv->NewDirectByteBuffer(pMapBufferNative.data(), jlong(pMapBufferNative.size() * sizeof(float))));
        jenv->CallVoidMethod(instance, load);
        jenv->DeleteGlobalRef(context);
        context = nullptr;
        LOGD("[ BLADDER ] Attached to thread, instance = %p", instance);
        isAttached = true;
    }

    ~BladderSegmentationJNI(){
        LOGD("Destroying JNI interface");
        jenv->DeleteGlobalRef(imageBuffer);
        jenv->DeleteGlobalRef(viewBuffer);
        jenv->DeleteGlobalRef(pMapBuffer);
        jenv->DeleteGlobalRef(instance);
        jenv->DeleteGlobalRef(clazz);
        jvm->DetachCurrentThread();
        LOGD("[ BLADDER ] Detaching thread");
        isAttached = false;
        jenv = nullptr;
    }
};

