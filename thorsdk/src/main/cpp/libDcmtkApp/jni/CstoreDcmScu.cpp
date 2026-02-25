
#include <jni.h>

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */
#include "include/dcmtkClientJni.h"
#include "dcmtk/dcmnet/diutil.h"
#include "dcmtk/dcmnet/scu.h"
#include "dcmtk/dcmdata/dcuid.h"     /* Covers most common dcmdata classes */
#include "include/scu.h"
#include <string>
#include <vector>
#include <android/log.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <dcmtk/dcmtls/tlsscu.h>

using namespace std;

#define LOGI(...) \
((void)__android_log_print(ANDROID_LOG_INFO, "dcmtk::", __VA_ARGS__))




jmethodID sendFileProgressMethod;
JNIEnv *jEnv;
jobject jCurrentObject;

void closeSCU(DcmClient& mDcmScu, bool tlsEncryptionEnabled = false){
    mDcmScu.releaseAssociation();
    if (tlsEncryptionEnabled) {
        mDcmScu.clearTransportLayer();
    }
    if(mDcmScu.isConnected()){
        LOGI("DICOM ISConnected, Release network");
        mDcmScu.freeNetwork();
    }
    LOGI("DICOM CLIENT RELEASE");
}

JNIEXPORT jobject
DCMTKCLIENT_METHOD(storeFileOnPACSServer)(JNIEnv* env, jobject thiz,jobject serverSettings,jstring dicomFilePath, jboolean tlsEncryptionEnabled, jstring tlsCertificatePath
        , jboolean useClientAuthentication) {

    jEnv = env;
    jCurrentObject = thiz;

    jclass jDcmtkClient = env->GetObjectClass(thiz);
    sendFileProgressMethod = env->GetMethodID(jDcmtkClient, "sendFileProgress", "(J)V");

    jclass serverSettingClass = env->GetObjectClass(serverSettings);
    jmethodID mGetCallingAETitle = env->GetMethodID(serverSettingClass, "getCallingAETitle", "()Ljava/lang/String;");
    jmethodID mGetPeerHostName = env->GetMethodID(serverSettingClass, "getPeerHostName", "()Ljava/lang/String;");
    jmethodID mGetPeerAETitle = env->GetMethodID(serverSettingClass, "getPeerAETitle", "()Ljava/lang/String;");
    jmethodID mGetPeerHostPort = env->GetMethodID(serverSettingClass, "getPeerPort", "()I");

    jmethodID mGetAbstractSyntaxList = env->GetMethodID(serverSettingClass, "getAbstractSyntaxList", "()Ljava/util/ArrayList;");
    jmethodID mGetTransferSyntaxList = env->GetMethodID(serverSettingClass, "getTransferSyntaxList", "()Ljava/util/ArrayList;");

    jobject mAeTitleObj = (env)->CallObjectMethod(serverSettings,mGetCallingAETitle);
    jobject mPeerHostNameObj = (env) -> CallObjectMethod(serverSettings,mGetPeerHostName);
    jobject mPeerAETitleObj = (env) -> CallObjectMethod(serverSettings,mGetPeerAETitle);
    jint mPeerHostPortObj = (env) -> CallIntMethod(serverSettings,mGetPeerHostPort);
    LOGI("AE HOST PORN JINT %d",mPeerHostPortObj);

    const char* mAeCallingTitleName=env->GetStringUTFChars((jstring)mAeTitleObj, nullptr);
    const char* mPeerHostName=env->GetStringUTFChars((jstring)mPeerHostNameObj, nullptr);
    const char* mPeerAETitle=env->GetStringUTFChars((jstring)mPeerAETitleObj, nullptr);
    const Sint32 connectionTimeOut=90;
    auto peerPortNumber =(Uint16) mPeerHostPortObj;

    LOGI(" AE_TITLE_NAME: %s ",mAeCallingTitleName);
    LOGI(" AE_PEER_HOST_NAME: %s ",mPeerHostName);
    LOGI(" AE_PEER_AE_TITLE: %s ",mPeerAETitle);
    LOGI(" AE_PEER_HOST_PORT: %d ",peerPortNumber);
    DcmClient mDcmScu;
    OFLog::configure(OFLogger::DEBUG_LOG_LEVEL);
    mDcmScu.setAETitle(std::string(mAeCallingTitleName));
    mDcmScu.setPeerHostName(std::string(mPeerHostName));
    mDcmScu.setPeerPort(peerPortNumber);
    mDcmScu.setPeerAETitle(std::string(mPeerAETitle));
   /* Enable these when SCU timeout start working without any issue
    * mDcmScu.setDIMSETimeout(connectionTimeOut);
    mDcmScu.setACSETimeout(connectionTimeOut);
    mDcmScu.setConnectionTimeout(connectionTimeOut);*/

    jobject abstractSyntaxList = (env) -> CallObjectMethod(serverSettings,mGetAbstractSyntaxList);
    auto  *mAbstractSyntaxArray = reinterpret_cast<jobjectArray*>(&abstractSyntaxList);
    LOGI(" Object to Array casting successfully");

    jobject transferSyntaxList = (env) -> CallObjectMethod(serverSettings,mGetTransferSyntaxList);
    auto  *mTransferSyntaxArray = reinterpret_cast<jobjectArray*>(&transferSyntaxList);

    jclass arrayListClass = (env)->FindClass("java/util/ArrayList");
    jmethodID arrayList_size = env->GetMethodID (arrayListClass, "size", "()I");
    jmethodID arrayList_get = env->GetMethodID (arrayListClass, "get", "(I)Ljava/lang/Object;");

    jint abstractSyntaxListLength = env->CallIntMethod(*mAbstractSyntaxArray, arrayList_size);
    jint transferSyntaxListLength = env->CallIntMethod(*mTransferSyntaxArray, arrayList_size);

    OFList<OFString> transferSyntextDefault;
    transferSyntextDefault.emplace_back(UID_LittleEndianExplicitTransferSyntax);
    transferSyntextDefault.emplace_back(UID_BigEndianExplicitTransferSyntax);
    transferSyntextDefault.emplace_back(UID_LittleEndianImplicitTransferSyntax);
    mDcmScu.addPresentationContext(UID_VerificationSOPClass, transferSyntextDefault);

    OFList<OFString> ts;
    for (jint i=0; i<transferSyntaxListLength; i++) {
        auto element = static_cast<jstring>(env->CallObjectMethod(*mTransferSyntaxArray, arrayList_get, i));
        const char* mTransferSyntaxID=env->GetStringUTFChars((jstring)element, nullptr);
        LOGI("Received Transfer syntax: %s ",mTransferSyntaxID);
        ts.emplace_back(mTransferSyntaxID);
    }
    for (jint i=0; i<abstractSyntaxListLength; i++) {
        auto element = static_cast<jstring>(env->CallObjectMethod(*mAbstractSyntaxArray, arrayList_get, i));
        const char* mAbstractSyntaxID=env->GetStringUTFChars((jstring)element, nullptr);
        LOGI("Received Abstract syntax: %s ",mAbstractSyntaxID);
        mDcmScu.addPresentationContext(std::string(mAbstractSyntaxID), ts);
    }
    OFCondition result;
    //* Initialize network *//*
    result = mDcmScu.initNetwork();
    if (result.bad())
    {
        LOGI("Unable to set up the network: %s", result.text());

        string errorMsg="Unable to set up the network:" + string(result.text());
        closeSCU(mDcmScu);
        return  getResultObject(env,errorMsg,ERROR);
    }else{
        string mValue=result.text();
        LOGI("Network init success-->%s",mValue.c_str());
    }

    if ((bool)tlsEncryptionEnabled){
        auto pemTlsCertificatePath = env->GetStringUTFChars(tlsCertificatePath, NULL);
        if (pemTlsCertificatePath && strlen(pemTlsCertificatePath)>0) {
            result = mDcmScu.setTransportLayer(pemTlsCertificatePath, (bool) useClientAuthentication);
            if (result.bad()) {
                string errorMessage;
                errorMessage.append("Error in setting transport layer : ");
                errorMessage.append(result.text());
                LOGE("%s", errorMessage.c_str());
                closeSCU(mDcmScu, (bool) tlsEncryptionEnabled);
                return getResultObject(env, errorMessage, ERROR);
            }
        }
    }

    //* Negotiate Association *//*
    result = mDcmScu.negotiateAssociation();
    if (result.bad())
    {
        LOGI("Unable to negotiate association: %s ", result.text());
         string errorMsg="Unable to negotiate association:" + string(result.text());
        closeSCU(mDcmScu, (bool) tlsEncryptionEnabled);
        return  getResultObject(env,errorMsg,ERROR);
    }

    const char* mFileName=env->GetStringUTFChars(dicomFilePath,NULL);
    DcmFileFormat fileFormat;
    OFCondition status = fileFormat.loadFile(mFileName);
    if (status.bad()) {
        LOGI("ERROR IN LOADING DICOM FILE");
        closeSCU(mDcmScu, (bool) tlsEncryptionEnabled);
        return getResultObject(env,"ERROR IN LOADING DICOM FILE",ERROR);
    }
    DcmDataset* dataset = fileFormat.getDataset();
    mDcmScu.setProgressNotificationMode(OFTrue);
    Uint16 mResponseStatusCode;
    result = mDcmScu.sendSTORERequest(0,mFileName,dataset,mResponseStatusCode);

    if (result.bad()){
        LOGI("Unable to Store file: %s ", result.text());
        LOGI("Unable to Store file with error: %d ", mResponseStatusCode);
        string errorMsg="Unable to Store file: "+string(result.text());
        closeSCU(mDcmScu, (bool) tlsEncryptionEnabled);
        return getResultObject(env,errorMsg,ERROR);
    }
    closeSCU(mDcmScu, (bool) tlsEncryptionEnabled);
    LOGI("DICOM FILE STORED SUCCESSFULLY ON PACS SERVER");
    return getResultObject(env,"Success",SUCCESS);
}

/*void DcmSCU::notifySENDProgress	(const unsigned long byteCount){
    jEnv ->CallVoidMethod(jCurrentObject,sendFileProgressMethod,byteCount);
}*/
