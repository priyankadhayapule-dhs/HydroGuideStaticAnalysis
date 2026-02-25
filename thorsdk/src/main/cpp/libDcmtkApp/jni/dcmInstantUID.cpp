//
// Created by Echonous on 8/5/20.
//
#include <jni.h>
#include <dcmtkClientJni.h>
#include "include/dcmtkUtilJni.h"
#include <android/log.h>
#define LOGI(...) \
((void)__android_log_print(ANDROID_LOG_INFO, "dcmtk::", __VA_ARGS__))
using namespace std;

JNIEXPORT jstring JNICALL
DCMTKUTIL_METHOD(generateInstanceUID)(JNIEnv* env,jobject thiz,jint instantType){

    const auto mInstantType=(Uint16)instantType;

    string mfinalInstantUID;

    if(mInstantType==TypeStudyInstanceUID){
        //Generate StudyUID
        char uid[100];
        auto instantUID=dcmGenerateUniqueIdentifier(uid,SITE_STUDY_UID_ROOT);
        mfinalInstantUID=string(instantUID);
        LOGI("Create new  Instance UID for Study %s", mfinalInstantUID.c_str());

    }else if(mInstantType==TypeSeriesInstanceUID){
        char uid[100];
        auto instantUID=dcmGenerateUniqueIdentifier(uid,  SITE_SERIES_UID_ROOT);
        mfinalInstantUID=string(instantUID);
        LOGI("Create new  Series UID for Series %s",mfinalInstantUID.c_str());
    } else if (mInstantType == TypeSopInstanceUID) {
        char uid[100];
        auto instantUID = dcmGenerateUniqueIdentifier(uid, SITE_INSTANCE_UID_ROOT);
        mfinalInstantUID = string(instantUID);
        LOGI("Create new SOP UID for dicom file %s", mfinalInstantUID.c_str());
    }
    return stringTojString(env,mfinalInstantUID);
}