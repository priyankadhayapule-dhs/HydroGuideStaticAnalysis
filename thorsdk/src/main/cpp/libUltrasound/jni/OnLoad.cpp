/*
 * Copyright (C) 2016 EchoNous Inc.
 *
 */

#include <jni.h>
#include <ThorUtils.h>

namespace echonous {
int register_manager_methods(JNIEnv* env);
int register_dau_methods(JNIEnv* env);
int register_viewer_methods(JNIEnv* env);
int register_player_methods(JNIEnv* env);
int register_recorder_methods(JNIEnv* env);
int register_encoder_methods(JNIEnv* env);
int register_puck_methods(JNIEnv* env);
int register_autocontrol_methods(JNIEnv* env);
int register_bladderSeg_methods(JNIEnv* env);
};

using namespace echonous;

extern "C" jint JNI_OnLoad(JavaVM* vm, void* /* reserved */)
{
    JNIEnv* env = NULL;
    jint result = -1;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("GetEnv failed!");
        return result;
    }
    ALOG_ASSERT(env, "Could not retrieve the env!");

    ALOGD("Registering Ultrasound native methods");

    register_manager_methods(env);
    register_dau_methods(env);
    register_viewer_methods(env);
    register_player_methods(env);
    register_recorder_methods(env);
    register_encoder_methods(env);
    register_puck_methods(env);
    register_autocontrol_methods(env);
    register_bladderSeg_methods(env);
    ALOGD("Register Ultrasound native methods completed");

    return JNI_VERSION_1_4;
}
