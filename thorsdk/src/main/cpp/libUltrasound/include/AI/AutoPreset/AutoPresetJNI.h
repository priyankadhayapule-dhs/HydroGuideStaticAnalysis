//
// Created by Michael May on 8/27/24.
//
/// Autopreset JNI
// Things needed to call JNI interface
#include "AutoDepthGainPresetConfig.h"
#include <jni.h>
struct AutoPresetJNI  {
    JavaVM *jvm;
    JNIEnv *jenv;
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
    jboolean  jniInit;
    const int AP_MODEL_WIDTH = 224;
    const int AP_MODEL_HEIGHT = 224;
    AutoPresetJNI(JNIEnv *jenv, jobject context) {
        jvm = nullptr;
        jenv->GetJavaVM(&jvm);
        this->jenv = nullptr; // thread local, this is set on the worker thread
        this->clazz = static_cast<jclass>(jenv->NewGlobalRef(
                jenv->FindClass("com/echonous/ai/autopreset/NativeInterface")));
        this->context = jenv->NewGlobalRef(context);

        ctor = jenv->GetMethodID(clazz, "<init>", "(Landroid/content/Context;)V");
        instance = nullptr; // this is slow, initialize on worker
        load = jenv->GetMethodID(clazz, "load", "()V");
        process = jenv->GetMethodID(clazz, "process", "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)I");
        clear = jenv->GetMethodID(clazz, "close", "()V");
        imageBuffer = nullptr;
        viewBuffer = nullptr;
        jniInit = false;
        LOGD("Created JNI interface, jvm = %p, jenv = %p, clazz = %p, instance = %p, process = %p, clear = %p",
             jvm, jenv, clazz, instance, clear);
    }

    void init(JNIEnv *jenv)
    {
        auto makeGlobalRef = [=](jobject local) {
            jobject global = jenv->NewGlobalRef(local);
            jenv->DeleteLocalRef(local);
            return global;
        };
        instance = makeGlobalRef(jenv->NewObject(clazz, ctor, context));
        // set buffer size
        const int VIEW_BUFFER_MAX = 5;
        // Create buffers on this thread as well
        imageBufferNative.resize(VIEW_BUFFER_MAX*AP_MODEL_WIDTH * AP_MODEL_HEIGHT);
        viewBufferNative.resize(VIEW_BUFFER_MAX);
        size_t imageBufferSize = imageBufferNative.size() * sizeof(float);
        size_t viewBufferSize = viewBufferNative.size() * sizeof(float);
        imageBuffer = makeGlobalRef(jenv->NewDirectByteBuffer(imageBufferNative.data(), (jlong)imageBufferSize));
        viewBuffer =  makeGlobalRef(jenv->NewDirectByteBuffer(viewBufferNative.data(), (jlong)viewBufferSize));
        LOGD("[ PRESET ] ImageBufferSize: %d, ViewBufferSize: %d", imageBufferSize, viewBufferSize);

    }

    void detachCurrentThread()
    {
        LOGD("[ AUTO PRESET ] Detaching Thread");
        if(jenv != nullptr) {
            jenv->CallVoidMethod(instance, clear);
        }
        jvm->DetachCurrentThread();
        jenv = nullptr;
        jniInit = false;
    }


    bool attachToThread() {
        if (jenv != nullptr) {
            LOGE("[ AUTO PRESET ] Should not be attached to any thread!");
            jniInit = false;
            return jniInit;
        }
        LOGD("[ AUTO PRESET ] Attaching to thread, creating instance...");
        jvm->AttachCurrentThread(&jenv, nullptr);
        if(jenv->ExceptionCheck())
        {
            auto exc = jenv->ExceptionOccurred();
            jclass excClass = jenv->GetObjectClass(exc);
            jmethodID getMessageMethod = jenv->GetMethodID(excClass, "getMessage", "()Ljava/lang/String;");
            jstring message = (jstring)jenv->CallObjectMethod(exc, getMessageMethod);
            const char *nativeString = jenv->GetStringUTFChars(message, 0);

            LOGD("[ AUTO PRESET ] Exception: %s", nativeString);
            jenv->ExceptionClear();
            jniInit = false;
        }
        else {
            jenv->CallVoidMethod(instance, load);
            jniInit = true;
            LOGD("[ PRESET ] AutoPreset pipeline loaded");
            LOGD("[ AUTO PRESET ] Attached to thread, instance = %p", instance);
        }
        return jniInit;
    }

    ~AutoPresetJNI() {
        LOGD("Destroying JNI interface");
        jenv->DeleteGlobalRef(imageBuffer);
        jenv->DeleteGlobalRef(viewBuffer);
        jenv->DeleteGlobalRef(instance);
        jenv->DeleteGlobalRef(clazz);
        jenv->DeleteGlobalRef(context);
        jvm->DetachCurrentThread();
    }
};