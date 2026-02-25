//
// Created by Echonous on 28/1/19.
// PACS will be ping by this call, which is called C-ECHO DIMSE
//

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

using namespace std;

/**
 * Close connection after SCU verification done or fail
 * @param mDcmScu
 */
void closeClient(DcmClient& mDcmScu, bool tlsEncryptionEnabled = false){
    mDcmScu.releaseAssociation();
    if (tlsEncryptionEnabled) {
        mDcmScu.clearTransportLayer();
    }
    if(mDcmScu.isConnected()){
        LOGI("DICOM isConnected, Release network");
        mDcmScu.freeNetwork();
    }
    LOGI("DICOM CLIENT RELEASE");
}

JNIEXPORT jobject
DCMTKCLIENT_METHOD(verifySCP)(JNIEnv *env, jobject jObject, jint profileType, jobjectArray labels, jboolean tlsEncryptionEnabled, jstring tlsCertificatePath, jboolean useClientAuthentication) {
    jobject mThorAeTitle = env->GetObjectArrayElement(labels, 0);
    jobject mPacsIpAddress = env->GetObjectArrayElement(labels, 1);
    jobject mPacsHostPort = env->GetObjectArrayElement(labels, 2);
    jobject mPacsAeTitle = env->GetObjectArrayElement(labels, 3);
    // Get a class reference for java.lang.Integer
    jclass classInteger = env->FindClass("java/lang/Integer");
    // Use Integer.intValue() to retrieve the int
    jmethodID midIntValue = env->GetMethodID(classInteger, "intValue", "()I");

    jint mPortValueC = (env)->CallIntMethod(mPacsHostPort, midIntValue);

    jboolean iscopy;
    const char *thorAeTitle = env->GetStringUTFChars((jstring) mThorAeTitle,
                                                     &iscopy);
    const char *pacsIpAddress = env->GetStringUTFChars((jstring) mPacsIpAddress, &iscopy);
    const char *pacsAeTitle = env->GetStringUTFChars((jstring) mPacsAeTitle, &iscopy);
    const Uint16 pacsHostPort = (Uint16) mPortValueC;

    LOGI("PACS Configuration : \n"
         "Thor AE Title : %s \n"
         "PACS AE Title : %s \n"
         "PACS IP Address : %s \n"
         "PACS Port Number : %d \n",
         thorAeTitle, pacsAeTitle,pacsIpAddress,pacsHostPort);

    OFLog::configure(OFLogger::DEBUG_LOG_LEVEL);
    DcmClient scu;

    // set AE titles
    //Set SCU's AETitle to be used in association negotiation.
    scu.setAETitle(std::string(thorAeTitle));
    scu.setPeerHostName(std::string(pacsIpAddress));
    scu.setPeerPort(pacsHostPort);


    //SCP Title
    scu.setPeerAETitle(std::string(pacsAeTitle));
    // Use presentation context for FIND/MOVE in study root, propose all uncompressed transfer syntaxes
    OFList<OFString> ts;
    ts.emplace_back(UID_LittleEndianExplicitTransferSyntax);
    ts.emplace_back(UID_BigEndianExplicitTransferSyntax);
    ts.emplace_back(UID_LittleEndianImplicitTransferSyntax);
    if (profileType == 0) {
        scu.addPresentationContext(UID_VerificationSOPClass, ts);
        scu.addPresentationContext(UID_FINDModalityWorklistInformationModel, ts);
    } else {
        scu.addPresentationContext(UID_FINDPatientRootQueryRetrieveInformationModel, ts);
        scu.addPresentationContext(UID_FINDStudyRootQueryRetrieveInformationModel, ts);
        scu.addPresentationContext(UID_MOVEStudyRootQueryRetrieveInformationModel, ts);
        scu.addPresentationContext(UID_VerificationSOPClass, ts);
        scu.addPresentationContext(UID_UltrasoundImageStorage, ts);
        scu.addPresentationContext(UID_UltrasoundMultiframeImageStorage, ts);
    }

    /* Initialize network */
    OFCondition result = scu.initNetwork();
    if (result.bad()) {
        DCMNET_ERROR("Unable to set up the network: " << result.text());
        LOGI("Unable to set up the network: %s", result.text());
        string errorMessage;
        errorMessage.append("Unable to set up the network : ");
        errorMessage.append(result.text());
        closeClient(scu);
        return getResultObject(env,errorMessage,ERROR);
    }

    if ((bool) tlsEncryptionEnabled) {
        auto pemTlsCertificatePath = env->GetStringUTFChars(tlsCertificatePath, NULL);
        LOGE("pemTlsCertificatePath: %s",pemTlsCertificatePath);
        if (pemTlsCertificatePath && strlen(pemTlsCertificatePath)>0) {
            result = scu.setTransportLayer(pemTlsCertificatePath, (bool)useClientAuthentication);
            if (result.bad()) {
                string errorMessage;
                errorMessage.append("Error in setting transport layer : ");
                errorMessage.append(result.text());
                LOGE("%s", errorMessage.c_str());
                closeClient(scu, (bool) tlsEncryptionEnabled);
                return getResultObject(env,errorMessage,ERROR);
            }
        }
    }

    /* Negotiate Association */
    result = scu.negotiateAssociation();
    if (result.bad()) {
        DCMNET_ERROR("Unable to negotiate association: " << result.text());
        LOGE("Unable to negotiate association: %s ", result.text());

        string errorMessage;
        errorMessage.append("Unable to negotiate association : ");
        errorMessage.append(result.text());
        closeClient(scu, (bool) tlsEncryptionEnabled);
        return getResultObject(env,errorMessage,ERROR);
    }
    /* Let's look whether the server is listening:
       Assemble and send C-ECHO request
     */
    result = scu.sendECHORequest(0);
    if (result.bad()) {
        DCMNET_ERROR("Could not process C-ECHO with the server: " << result.text());
        LOGE("Could not process C-ECHO with the server: %s", result.text());
        string errorMessage;
        errorMessage.append("Could not process C-ECHO with the server : ");
        errorMessage.append(result.text());
        closeClient(scu, (bool) tlsEncryptionEnabled);
        return getResultObject(env,errorMessage,ERROR);
    }

    closeClient(scu, (bool) tlsEncryptionEnabled);
    return getResultObject(env,"success",SUCCESS);
}



jobject integerTojObject(JNIEnv *env,int value) {
    jclass cls = env->FindClass("java/lang/Integer");
    jmethodID midInit = env->GetMethodID(cls, "<init>", "(I)V");
    if (nullptr == midInit) return nullptr;
    jobject object = env->NewObject(cls, midInit, value);
    return  object;
}

jstring stringTojString(JNIEnv *env,string value){
    return env->NewStringUTF(value.c_str());
}

jobject getResultObject(JNIEnv *env,string message,jint status){
    jclass mResultClass=env->FindClass("com/echonous/kosmosapp/framework/dcmtkclient/Result");
    jmethodID  mResultJmethod=env->GetMethodID(mResultClass,"<init>","()V");
    jobject  mResultJObject=env->NewObject(mResultClass,mResultJmethod);
    jfieldID  mResultMessageJFields=env->GetFieldID(mResultClass,"mMessage","Ljava/lang/String;");
    jfieldID  mResultStatusJFields=env->GetFieldID(mResultClass,"mStatus","I");
    env->SetObjectField(mResultJObject, mResultMessageJFields,stringTojString(env,message));
    env->SetIntField(mResultJObject,mResultStatusJFields,status);
    return mResultJObject;
}

jobject getMwlResultObject(JNIEnv *env,string message,jint status){
    jclass mResultMwl=env->FindClass("com/echonous/kosmosapp/framework/dcmtkclient/MwlResult");
    jmethodID  mResultJmethod=env->GetMethodID(mResultMwl,"<init>","()V");
    jobject  mResultJObject=env->NewObject(mResultMwl,mResultJmethod);
    jfieldID  mResultMessageJFields=env->GetFieldID(mResultMwl,"mResult","Ljava/lang/String;");
    jfieldID  mResultStatusJFields=env->GetFieldID(mResultMwl,"mStatus","I");
    env->SetObjectField(mResultJObject, mResultMessageJFields,stringTojString(env,message));
    env->SetIntField(mResultJObject,mResultStatusJFields,status);
    return mResultJObject;
}