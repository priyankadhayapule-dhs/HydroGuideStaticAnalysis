//
// Created by Michael May on 5/13/24.
//
#pragma once

#include <jni.h>

struct VtiNativeJni {
    JavaVM *jvm;
    JNIEnv *jenv;
    jclass clazz;
    jobject context;
    jobject instance;
    jmethodID ctor;
    jmethodID load;
    jmethodID process_38;
    jmethodID process_36;
    jmethodID process_50;
    jmethodID clear;
    jobject imageBuffer;
    std::vector<float> imageBufferNative;
    jobject kernelBuffer;
    std::vector<float> kernelBufferNative;
    jobject outputBuffer;
    std::vector<float> outputBufferNative;
    jobject outputBuffer2nd;
    std::vector<float> outputBuffer2ndNative;

    VtiNativeJni(JNIEnv *jenv, jobject context) {
        jvm = nullptr;
        jenv->GetJavaVM(&jvm);
        this->jenv = nullptr; // thread local, this is set on the worker thread
        this->clazz = static_cast<jclass>(jenv->NewGlobalRef(
                jenv->FindClass("com/echonous/ai/vti/NativeInterface")));
        this->context = jenv->NewGlobalRef(context);

        ctor = jenv->GetMethodID(clazz, "<init>", "(Landroid/content/Context;)V");
        instance = nullptr; // this is slow, initialize on worker
        load = jenv->GetMethodID(clazz, "load", "()V");
        process_38 = jenv->GetMethodID(clazz, "process_38", "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)V");
        process_36 = jenv->GetMethodID(clazz, "process_36", "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)V");
        process_50 = jenv->GetMethodID(clazz, "process_50", "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)V");
        clear = jenv->GetMethodID(clazz, "close", "()V");
        // Buffers are also initialized on worker thread
        imageBuffer = nullptr;
        kernelBuffer = nullptr;
        outputBuffer = nullptr;
        outputBuffer2nd = nullptr;

        LOGD("Created JNI interface, jvm = %p, jenv = %p, clazz = %p, instance = %p, process = %p, clear = %p",
             jvm, jenv, clazz, instance, clear);
    }

    void attachToThread() {
        LOGD("[ VTITASK attachToThread called");
        if(jenv != nullptr) {
            LOGE("[ VTITASK ] Should not be attached to any thread.");
            return;
        }
        LOGD("[ VTITASK ] Attaching to thread, creating instance...");
        jvm->AttachCurrentThread(&jenv, nullptr);
        instance = jenv->NewObject(clazz, ctor, context);
        imageBufferNative.resize(256*256);
        imageBuffer = jenv->NewDirectByteBuffer(imageBufferNative.data(), (jlong)imageBufferNative.size() * sizeof(float));
        kernelBufferNative.resize(9);
        kernelBuffer = jenv->NewDirectByteBuffer(kernelBufferNative.data(), (jlong)kernelBufferNative.size() * sizeof(float));
        outputBufferNative.resize(2*256*256);
        outputBuffer = jenv->NewDirectByteBuffer(outputBufferNative.data(), (jlong)outputBufferNative.size() * sizeof(float));
        outputBuffer2ndNative.resize(2*256*256);
        outputBuffer2nd = jenv->NewDirectByteBuffer(outputBuffer2ndNative.data(), (jlong)outputBuffer2ndNative.size() * sizeof(float));

        jenv->CallVoidMethod(instance, load);
        LOGD("[ VTI ] TFLite backend loaded");

        jenv->DeleteGlobalRef(context);
        context = nullptr;
        LOGD("[ VTI ] Attached to thread, instance = %p", instance);
    }

    ~VtiNativeJni() {
        LOGD("[ VTITASK ] Destroying VTI JNI interface");
        jenv->DeleteLocalRef(imageBuffer);
        jenv->DeleteLocalRef(kernelBuffer);
        jenv->DeleteLocalRef(outputBuffer);
        jenv->DeleteLocalRef(outputBuffer2nd);
    }
};