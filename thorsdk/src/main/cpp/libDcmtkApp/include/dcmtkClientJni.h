//
// Created by himanshu on 14/6/18.
//

#ifndef DCMTKCLIENT_DCMTK_CLIENT_JNI_H
#define DCMTKCLIENT_DCMTK_CLIENT_JNI_H


#include <jni.h>
#include <string>
using namespace std;
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


#define DCMTKCLIENT_METHOD(METHOD_NAME)\
    Java_com_echonous_kosmosapp_framework_dcmtkclient_DcmtkClient_##METHOD_NAME // NOLINT


JNIEXPORT jobject JNICALL
DCMTKCLIENT_METHOD(verifySCP)(JNIEnv* env, jobject jObject, jint profileType, jobjectArray labels, jboolean tlsEncryptionEnabled, jstring tlsCertificatePath, jboolean useClientAuthentication);

JNIEXPORT jobject JNICALL
DCMTKCLIENT_METHOD(storeFileOnPACSServer)(JNIEnv* env, jobject thiz, jobject scpServerSettings, jstring dicomFilePath, jboolean tlsEncryptionEnabled, jstring tlsCertificatePath, jboolean useClientAuthentication);

JNIEXPORT jobject JNICALL
DCMTKCLIENT_METHOD(queryMWL)(JNIEnv* env, jobject thiz, jobjectArray labels, jobjectArray filterOption, jboolean tlsEncryptionEnabled, jstring tlsCertificatePath, jboolean useClientAuthentication);

JNIEXPORT jstring JNICALL
DCMTKCLIENT_METHOD(cancelMwl)(JNIEnv* env, jobject thiz,jint id);

JNIEXPORT jobject JNICALL
DCMTKCLIENT_METHOD(createDicomDir)(JNIEnv* env, jobject thiz,jstring inputDirectory,jstring outputDirectory,jstring writeAction);

jobject integerTojObject(JNIEnv *env,int value);
jstring stringTojString(JNIEnv *env,string value);
jobject getResultObject(JNIEnv *env,string message,int status);
jobject getMwlResultObject (JNIEnv *env,string message,int status);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif //
