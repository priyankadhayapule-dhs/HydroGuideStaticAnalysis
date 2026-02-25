//
// Created by himanshu on 18/7/20.
//
#include <jni.h>

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */
#include "include/dcmtkClientJni.h"
#include "include/dcmtkUtilJni.h"
#include "dcmtk/dcmnet/diutil.h"
#include "dcmtk/dcmnet/scu.h"
#include "dcmtk/dcmdata/dcuid.h"     /* Covers most common dcmdata classes */
#include "include/scu.h"
#include <string>
#include <vector>
#include <iostream>
#include <android/log.h>
#include "jsoncpp/json/json.h"

using namespace std;

#define LOGIFIND(...) \
((void)__android_log_print(ANDROID_LOG_INFO, "dcmtk::", __VA_ARGS__))

DcmClient *mDcmScu = nullptr;

/**
 * Close connection after SCU verification done or fail
 */
void closeClientFindScu(){
    mDcmScu->releaseAssociation();
    if (mDcmScu->getTLSEnabled()) {
        mDcmScu->clearTransportLayer();
    }
    if(mDcmScu->isConnected()){
        LOGIFIND("DICOM isConnected, Release network");
        mDcmScu->freeNetwork();
    }
    delete  mDcmScu;
    LOGIFIND("DICOM CLIENT RELEASE");
}

string cancelRequest(int presentationContextID){
    if(mDcmScu != nullptr){
        LOGIFIND("C-FIND Cancel Request Received");
        //OFCondition cond= mDcmScu->sendCANCELRequest(presentationContextID);
        OFCondition cond= mDcmScu->abortAssociation();
        if(cond.good()){
            LOGIFIND("C-FIND request was being called");
        }else{
            LOGIFIND("NOT ABLE TO CANCEL C-FIND request ");
        }
        closeClientFindScu();
        return string("Success");
    }
    return  string("Fail");
}


JNIEXPORT jstring
DCMTKCLIENT_METHOD(cancelMwl)(JNIEnv* env, jobject thiz,jint id){

    string result=cancelRequest(id);
   return stringTojString(env,result);
}

JNIEXPORT jobject
DCMTKCLIENT_METHOD(queryMWL)(JNIEnv *env, jobject thiz, jobjectArray labels, jobjectArray filterOption, jboolean tlsEncryptionEnabled, jstring tlsCertificatePath, jboolean useClientAuthentication) {

    // Connection with Server
    //jclass jDcmtkClient = env->GetObjectClass(thiz);
    jclass jDcmtkClient = env->GetObjectClass(thiz);
    jmethodID getDate=env->GetMethodID(jDcmtkClient,"getDate",
                                       "(Ljava/lang/String;Ljava/lang/String;)Ljava/util/Date;");
    jmethodID  getbool = env->GetMethodID(jDcmtkClient,"getBool", "(Ljava/lang/Object;)Z");
    //setPresentationContextID
    jmethodID setPresentationContextID=env->GetMethodID(jDcmtkClient,"setPresentationContextID","(I)V");

    jobject mThorAeTitle = env->GetObjectArrayElement(labels, 0);
    jobject mPacsIpAddress = env->GetObjectArrayElement(labels, 1);
    jobject mPacsHostPort = env->GetObjectArrayElement(labels, 2);
    jobject mPacsAeTitle = env->GetObjectArrayElement(labels, 3);
    jobject mRequestTimeout = env->GetObjectArrayElement(labels,4);

    // Get a class reference for java.lang.Integer
    jclass classInteger = env->FindClass("java/lang/Integer");
    // Use Integer.intValue() to retrieve the int
    jmethodID midIntValue = env->GetMethodID(classInteger, "intValue", "()I");

    jint mPortValueC = (env)->CallIntMethod(mPacsHostPort, midIntValue);

    jint mTimeout = env -> CallIntMethod(mRequestTimeout,midIntValue);

    jboolean iscopy;
    const char *thorAeTitle = env->GetStringUTFChars((jstring) mThorAeTitle,
                                                     &iscopy);
    const char *pacsIpAddress = env->GetStringUTFChars((jstring) mPacsIpAddress, &iscopy);
    const char *pacsAeTitle = env->GetStringUTFChars((jstring) mPacsAeTitle, &iscopy);
    const auto pacsHostPort = (Uint16) mPortValueC;

    LOGIFIND("PACS Configuration : \n"
         "Thor AE Title : %s \n"
         "PACS AE Title : %s \n"
         "PACS IP Address : %s \n"
         "PACS Port Number : %d \n"
         "PACS Timeout Number : %d",
         thorAeTitle, pacsAeTitle, pacsIpAddress, pacsHostPort, mTimeout);

    mDcmScu = new DcmClient();
    OFLog::configure(OFLogger::DEBUG_LOG_LEVEL);
    mDcmScu->setAETitle(thorAeTitle);
    mDcmScu->setPeerHostName(pacsIpAddress);
    mDcmScu->setPeerPort(pacsHostPort);
    mDcmScu->setPeerAETitle(pacsAeTitle);
    mDcmScu->setDIMSEBlockingMode(DIMSE_NONBLOCKING);
    mDcmScu->setDIMSETimeout((Uint32)mTimeout);
    mDcmScu->setMaxReceivePDULength(ASC_DEFAULTMAXPDU);

    //mDcmScu.setDIMSEBlockingMode(DIMSE_NONBLOCKING);
    mDcmScu->setVerbosePCMode(OFTrue);

    OFList<OFString> ts;
    ts.emplace_back(UID_LittleEndianExplicitTransferSyntax);
    ts.emplace_back(UID_BigEndianExplicitTransferSyntax);
    ts.emplace_back(UID_LittleEndianImplicitTransferSyntax);
    mDcmScu->addPresentationContext(UID_FINDModalityWorklistInformationModel, ts);

    // Convert the Filter Option Java Object Array to Native
    //jobject mFirstName = env->GetObjectArrayElement(filterOption, 0);
    const char* mPatientName=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(filterOption, 0),&iscopy);
    LOGIFIND("Patient Name Input is %s", mPatientName);
    const char* mMrn = env->GetStringUTFChars((jstring)env->GetObjectArrayElement(filterOption, 1),&iscopy);

    const char* mAccession = env->GetStringUTFChars((jstring)env->GetObjectArrayElement(filterOption, 2),&iscopy);

    const char* mRequestedProId = env->GetStringUTFChars((jstring)env->GetObjectArrayElement(filterOption, 3),&iscopy);

    LOGIFIND("MWL User Query Inputs PatientName %s  \n "
             "MRN %s  Accession %s ReqProId %s",mPatientName,mMrn,mAccession,mRequestedProId);

    const char* mSPSDate = env->GetStringUTFChars((jstring)env->GetObjectArrayElement(filterOption, 4),&iscopy);
    jobject mAssignedToKOSMOS = env->GetObjectArrayElement(filterOption, 5);
    jobject mAssignedToUSModality = env->GetObjectArrayElement(filterOption, 6);
    const char * kosmosThorAETitle =  env->GetStringUTFChars((jstring)env->GetObjectArrayElement(filterOption, 7),&iscopy);

    jobject mMwlResult = nullptr;
    jclass mMwlClass=env->FindClass("com/echonous/kosmosapp/framework/dcmtkclient/MwlResult");
    jmethodID  mMwlResultSetMethod = env->GetMethodID(mMwlClass, "setPatientList","(Ljava/util/ArrayList;)V");
    jmethodID  mAddMwlPatient = env->GetMethodID(jDcmtkClient,"addMwlPatient","(Lcom/echonous/kosmosapp/data/pims/model/PatientMWL;)V");

    //Json::Value root;
    OFCondition result;

    if ((bool) tlsEncryptionEnabled) {
        auto pemTlsCertificatePath = env->GetStringUTFChars(tlsCertificatePath, NULL);
        if (pemTlsCertificatePath && strlen(pemTlsCertificatePath)>0) {
            result = mDcmScu->setTransportLayer(pemTlsCertificatePath, (bool) useClientAuthentication);
            if (result.bad()) {
                string errorMessage;
                errorMessage.append("Error in setting transport layer : ");
                errorMessage.append(result.text());
                LOGE("%s", errorMessage.c_str());
                closeClientFindScu();
                return getResultObject(env, errorMessage, ERROR);
            }
        }
    }

    OFCondition cond = mDcmScu->initNetwork();
    if (cond.good()) {
        LOGIFIND("PACS Configuration init network : %d", cond.good());
        cond = mDcmScu->negotiateAssociation();
        LOGIFIND("PACS Configuration negotiate association : %d  , Error %s", cond.good(), cond.text());
        if (cond.good()) {
           /* T_ASC_PresentationContextID presID = findUncompressedPC(
                    UID_FINDModalityWorklistInformationModel, mDcmScu);*/
            T_ASC_PresentationContextID presID = mDcmScu->findPresentationContextID(UID_FINDModalityWorklistInformationModel, "");
            LOGIFIND("PACS Configuration presID : %d", presID);
            if (presID != 0) {
                env->CallVoidMethod(thiz,setPresentationContextID,presID);
                LOGIFIND("Step 1 PACS Configuration presID : %d", presID);

                DcmDataset req;

                LOGIFIND("Patient name search is %s", mPatientName);
                cond = req.putAndInsertString(DCM_PatientName, mPatientName,OFTrue);
                if(cond.good()){
                    LOGIFIND("Patient name inserted %s", mPatientName);
                }else{
                    LOGIFIND("Patient name Failed");
                }
                cond = req.putAndInsertString(DCM_AccessionNumber, mAccession);
                cond = req.putAndInsertString(DCM_RequestedProcedureID, mRequestedProId);

                cond = req.putAndInsertString(DCM_ReferringPhysicianName, "");

                cond = req.putAndInsertString(DCM_PatientID, mMrn);
                cond = req.putAndInsertString(DCM_PatientBirthDate, "");
                cond = req.putAndInsertString(DCM_PatientSex, "");
                cond = req.putAndInsertString(DCM_StudyInstanceUID, "");
                cond = req.putAndInsertString(DCM_RequestedProcedureDescription, "");
                cond = req.putAndInsertString(DCM_CurrentPatientLocation, "");

                cond = req.putAndInsertString(DCM_RequestedProcedureComments, "");
                cond=  req.putAndInsertString(DCM_EthnicGroup,"");

                cond= req.putAndInsertString(DCM_PatientSize,"");
                cond= req.putAndInsertString(DCM_PatientWeight,"");

                cond= req.putAndInsertString(DCM_AdmittingDiagnosesDescription,"");
                cond= req.putAndInsertString(DCM_MedicalAlerts,"");
                cond= req.putAndInsertString(DCM_AdditionalPatientHistory,"");
                cond =req.putAndInsertString(DCM_PregnancyStatus,"");
                cond= req.putAndInsertString(DCM_PatientComments,"");
                cond= req.putAndInsertString(DCM_Allergies,"");
                LOGIFIND("Step 2 PACS Configuration presID : %d", presID);
                cond= req.putAndInsertString(DCM_RequestingPhysician,"");
                cond= req.putAndInsertString(DCM_ReasonForTheRequestedProcedure,"");
                cond= req.putAndInsertString(DCM_NamesOfIntendedRecipientsOfResults,"");
                cond = req.putAndInsertString(DCM_InstitutionName,"");
                cond = req.putAndInsertString(DCM_PhysiciansOfRecord,"");

                auto *rpCodeSeq = new DcmSequenceOfItems(DCM_RequestedProcedureCodeSequence);
                auto *rpCodeItem = new DcmItem;

                cond = rpCodeItem->putAndInsertString(DCM_CodeValue,"");
                cond = rpCodeItem->putAndInsertString(DCM_CodingSchemeDesignator,"");
                cond = rpCodeItem->putAndInsertString(DCM_CodingSchemeVersion,"");
                cond = rpCodeItem->putAndInsertString(DCM_CodeMeaning,"");

                cond = rpCodeSeq->insert(rpCodeItem);

                cond = req.insert(rpCodeSeq, OFTrue);

                LOGIFIND("Step 3 PACS Configuration presID : %d", presID);
                auto *pSequence = new DcmSequenceOfItems(DCM_ScheduledProcedureStepSequence);
                auto *pDcmItem = new DcmItem;
                LOGIFIND("Step 4 PACS Configuration presID : %d", presID);
                bool  isUsModality=env->CallBooleanMethod(thiz,getbool,mAssignedToUSModality);

                LOGIFIND("Us Modality is  : %d", isUsModality);
                if(isUsModality){
                    cond = pDcmItem->putAndInsertString(DCM_Modality, "US");
                }else{
                    cond = pDcmItem->putAndInsertString(DCM_Modality, "");
                }

                bool assigntoKosmos = env->CallBooleanMethod(thiz, getbool, mAssignedToKOSMOS);
                LOGIFIND("Assign to KOSMOS AE  : %d AE Title is %s", assigntoKosmos,
                         kosmosThorAETitle);

                if (assigntoKosmos) {
                    cond = pDcmItem->putAndInsertString(DCM_ScheduledStationAETitle,
                                                        kosmosThorAETitle);
                } else {
                    cond = pDcmItem->putAndInsertString(DCM_ScheduledStationAETitle, "");
                }

                LOGIFIND(" SPS Date is %s", mSPSDate);
                cond = pDcmItem->putAndInsertString(DCM_ScheduledProcedureStepStartDate, mSPSDate);
                cond = pDcmItem->putAndInsertString(DCM_ScheduledProcedureStepStartTime, "");

                cond = pDcmItem->putAndInsertString(DCM_ScheduledProcedureStepDescription, "");
                cond = pDcmItem->putAndInsertString(DCM_ScheduledProcedureStepID, "");
                cond = pDcmItem->putAndInsertString(DCM_ScheduledPerformingPhysicianName, "");
                cond = pDcmItem->putAndInsertString(DCM_ScheduledStationName, "");


                auto *scheduledProtocolCodeSequenceItem = new DcmItem;
                auto *spCodeSequence=new DcmSequenceOfItems(DCM_ScheduledProtocolCodeSequence);
                //
                cond = scheduledProtocolCodeSequenceItem->putAndInsertString(DCM_CodeValue,"");
                cond = scheduledProtocolCodeSequenceItem->putAndInsertString(DCM_CodingSchemeDesignator,"");
                cond = scheduledProtocolCodeSequenceItem->putAndInsertString(DCM_CodingSchemeVersion,"");
                cond = scheduledProtocolCodeSequenceItem->putAndInsertString(DCM_CodeMeaning,"");

                cond = spCodeSequence->insert(scheduledProtocolCodeSequenceItem);

                cond = pDcmItem->insert(spCodeSequence,OFTrue);

                cond = pSequence->insert(pDcmItem);
                cond = req.insert(pSequence, OFTrue);
                LOGIFIND("Step 5 PACS Configuration presID : %d", presID);


                auto *referStudySeqSeq = new DcmSequenceOfItems(DCM_ReferencedStudySequence);
                auto *refStudyItem = new DcmItem;
                cond = refStudyItem->putAndInsertString(DCM_ReferencedSOPClassUID,"");
                cond = refStudyItem->putAndInsertString(DCM_ReferencedSOPInstanceUID,"");
                cond  = referStudySeqSeq->insert(refStudyItem);
                cond = req.insert(referStudySeqSeq, OFTrue);



                OFList<QRResponse*> *findResponses;
                findResponses=new OFList<QRResponse*>();
                /*if(&findResponses != nullptr){
                    LOGIFIND("Step 7  NOT NULL object PACS Configuration presID : %d", presID);
                }*/
                LOGIFIND("Step 6.0 Init Done PACS Configuration presID : %d", presID);
                cond = mDcmScu->sendFINDRequest(presID, &req, findResponses);
                LOGIFIND("Step 6 PACS Configuration presID : %d", presID);
                bool containData = false;

                if (cond.good())
                {
                    LOGIFIND("PACS Configuration find request Good");
                    OFListIterator(QRResponse*) study;
                    jclass jclassArrayList = env->FindClass("java/util/ArrayList");
                    jmethodID mArrayInit = env->GetMethodID(jclassArrayList, "<init>", "()V");
                    jobject mArrayMwl=env->NewObject(jclassArrayList, mArrayInit);
                    jmethodID  mjMethodId = env->GetMethodID(jclassArrayList, "add", "(Ljava/lang/Object;)Z");

                    for (study = findResponses->begin(); study != findResponses->end(); study++)
                    {
                        Json::Value root;
                        Json::Value spsSequenceData;
                        Json::Value mwlKeyArray;
                        Json::Value keyRoot;

                        //com.echonous.kosmosapp.data.pims.model
                        jclass patientMwlClass = env->FindClass("com/echonous/kosmosapp/data/pims/model/PatientMWL");
                        jmethodID constructorMwl = env->GetMethodID(patientMwlClass, "<init>", "()V");
                        jobject patientObject = env->NewObject(patientMwlClass, constructorMwl);

                        jmethodID setPatientName = env->GetMethodID(patientMwlClass, "setPatientName", "(Ljava/lang/String;)V");
                        //PatientMrn
                        jmethodID setPatientMrn = env->GetMethodID(patientMwlClass, "setPatientMrn", "(Ljava/lang/String;)V");
                        jmethodID setRpId = env->GetMethodID(patientMwlClass, "setRpId", "(Ljava/lang/String;)V");
                        //setStudyInstanceUID
                        jmethodID setStudyInstantUID = env->GetMethodID(patientMwlClass, "setStudyInstanceUID", "(Ljava/lang/String;)V");

                        jmethodID setPatientDOB = env->GetMethodID(patientMwlClass, "setPatientDOB", "(Ljava/lang/String;)V");
                        jmethodID setAccessionNumber = env->GetMethodID(patientMwlClass, "setAccession", "(Ljava/lang/String;)V");

                        jmethodID setSpsDate = env->GetMethodID(patientMwlClass, "setSpsStartDate", "(Ljava/util/Date;)V");
                        jmethodID  setModality = env->GetMethodID(patientMwlClass, "setModality", "(Ljava/lang/String;)V");
                        jmethodID  setPatientSex = env->GetMethodID(patientMwlClass, "setGender", "(Ljava/lang/String;)V");
                        jmethodID  setEthnicity = env->GetMethodID(patientMwlClass, "setEthnicity", "(Ljava/lang/String;)V");

                        jmethodID  setPatientKeys= env->GetMethodID(patientMwlClass, "setPatientKeys", "(Ljava/lang/String;)V");

                        jmethodID  setPatientMwlDatas= env->GetMethodID(patientMwlClass, "setPatientMwlData", "(Ljava/lang/String;)V");

                        jmethodID  setPatientSize = env->GetMethodID(patientMwlClass, "setPatientSize", "(Ljava/lang/String;)V");
                        //patientHeight
                        jmethodID  setPatientWeight = env->GetMethodID(patientMwlClass, "setPatientWeight", "(Ljava/lang/String;)V");
                        //referringPhysicianName
                        jmethodID  setReferringPhysicianName = env->GetMethodID(patientMwlClass, "setReferringPhysicianName", "(Ljava/lang/String;)V");
                        //patientComments
                        jmethodID  setPatientComments = env->GetMethodID(patientMwlClass, "setPatientComments", "(Ljava/lang/String;)V");
                        //physiciansOfRecord
                        jmethodID  setPhysiciansOfRecord = env->GetMethodID(patientMwlClass, "setPhysiciansOfRecord", "(Ljava/lang/String;)V");

                        if(patientObject != nullptr){
                            LOGIFIND("Mwl Object created ");

                        }
                        LOGIFIND("PACS Configuration request found: %d", (*study)->m_dataset == nullptr);
                        if ((*study)->m_dataset != nullptr)
                        {

                            containData = true;

                            DcmItem *mScheduledProcedureStepSequence;

                            auto scheduledProcedureStepSequenceTag=(*study)->m_dataset->
                                    findOrCreateSequenceItem(DCM_ScheduledProcedureStepSequence,mScheduledProcedureStepSequence);

                            if(scheduledProcedureStepSequenceTag.good()){
                                LOGIFIND("ScheduledProcedureStepSequenceTag Found");

                                const char *studyModality;
                                auto modality=mScheduledProcedureStepSequence->findAndGetString(DCM_Modality, studyModality );
                                if (modality.good())
                                {
                                    //cout << "Study Modality : " << studyModality << endl;
                                    LOGIFIND("Study Modality: %s",studyModality);
                                    env->CallVoidMethod(patientObject,setModality,env->NewStringUTF(studyModality));
                                    spsSequenceData["Modality"] = studyModality;
                                }else{
                                    LOGIFIND("Study Modality: Error %s ", modality.text());
                                    spsSequenceData["Modality"]="";
                                }

                                OFString performedProcedureStepStartTime;
                                auto spsTime=mScheduledProcedureStepSequence->findAndGetOFString(DCM_ScheduledProcedureStepStartTime, performedProcedureStepStartTime);
                                if(spsTime.good()){
                                    LOGIFIND("SPS Time: %s",performedProcedureStepStartTime.c_str());
                                    spsSequenceData[ScheduledProcedureStepStartTime] =performedProcedureStepStartTime;
                                }else{
                                    LOGIFIND("SPS Time: Error %s ", spsTime.text());
                                    spsSequenceData[ScheduledProcedureStepStartTime] =performedProcedureStepStartTime;
                                }



                                OFString performedProcedureStepStartDate;
                                auto spsDate=mScheduledProcedureStepSequence->findAndGetOFString(DCM_ScheduledProcedureStepStartDate, performedProcedureStepStartDate);
                                if(spsDate.good()){
                                    LOGIFIND("SPS Date: %s",performedProcedureStepStartDate.c_str());
                                    if(performedProcedureStepStartTime.empty()){
                                        performedProcedureStepStartTime = string("000000");
                                    }
                                    jobject spsDateObject = nullptr;
                                    if(!performedProcedureStepStartDate.empty()){
                                        spsDateObject =env->CallObjectMethod(thiz,getDate,env->NewStringUTF(performedProcedureStepStartDate.c_str()),
                                                                             env->NewStringUTF(performedProcedureStepStartTime.c_str()));
                                    }
                                    env->CallVoidMethod(patientObject,setSpsDate,spsDateObject);
                                    spsSequenceData[ScheduledProcedureStepStartDate] =performedProcedureStepStartDate;

                                }else{
                                    LOGIFIND("SPS Date: Error %s ", spsDate.text());
                                    spsSequenceData[ScheduledProcedureStepStartDate] ="";
                                }



                                OFString scheduledStationAETitle;
                                auto aeTitleTag=mScheduledProcedureStepSequence->findAndGetOFString(DCM_ScheduledStationAETitle,scheduledStationAETitle);
                                if(aeTitleTag.good()){
                                    LOGIFIND("AE Title found: %s",scheduledStationAETitle.c_str());
                                    spsSequenceData[ScheduledStationAETitle] =scheduledStationAETitle;
                                }else{
                                    spsSequenceData[ScheduledStationAETitle] =scheduledStationAETitle;
                                    LOGIFIND("AE Title not found %s", aeTitleTag.text());
                                }

                                OFString strScheduledProcedureStepID;
                                auto scheduledProcedureStepIDTag=mScheduledProcedureStepSequence->findAndGetOFString(DCM_ScheduledProcedureStepID,strScheduledProcedureStepID);
                                if(scheduledProcedureStepIDTag.good()){
                                    LOGIFIND("DCM_ScheduledProcedureStepID found: %s",strScheduledProcedureStepID.c_str());
                                    spsSequenceData[ScheduledProcedureStepID] =strScheduledProcedureStepID;
                                }else{
                                    spsSequenceData[ScheduledProcedureStepID] =strScheduledProcedureStepID;
                                    LOGIFIND("DCM_ScheduledProcedureStepID found %s", scheduledProcedureStepIDTag.text());
                                }

                                //DCM_ScheduledProcedureStepDescription

                                OFString strScheduledProcedureStepDescription;
                                auto scheduledProcedureStepDescriptionTag=mScheduledProcedureStepSequence->findAndGetOFString(DCM_ScheduledProcedureStepDescription,strScheduledProcedureStepDescription);
                                if(scheduledProcedureStepDescriptionTag.good()){
                                    LOGIFIND("DCM_ScheduledProcedureStepDescription found: %s",strScheduledProcedureStepDescription.c_str());
                                    spsSequenceData[ScheduledProcedureStepDescription] =strScheduledProcedureStepDescription;
                                }else{
                                    spsSequenceData[ScheduledProcedureStepDescription] =strScheduledProcedureStepDescription;
                                    LOGIFIND("DCM_ScheduledProcedureStepDescription found %s", scheduledProcedureStepDescriptionTag.text());
                                }

                                Json::Value scheduledProtocolCodeSequenceJson;
                                DcmItem *ScheduledProtocolCodeSequenceItem;
                                auto scheduledProtocolCodeSequenceTag=mScheduledProcedureStepSequence->findOrCreateSequenceItem(DCM_ScheduledProtocolCodeSequence,ScheduledProtocolCodeSequenceItem);
                                if(scheduledProtocolCodeSequenceTag.good()){
                                    LOGIFIND("ScheduledProtocolCodeSequence Found");

                                   OFString dcmCodeValue;
                                   auto dcmCodeValueTag =ScheduledProtocolCodeSequenceItem->findAndGetOFString(DCM_CodeValue,dcmCodeValue);
                                   if(dcmCodeValueTag.good()){
                                       scheduledProtocolCodeSequenceJson[CodeValueJson]=dcmCodeValue;
                                       LOGIFIND("DCM_CodeValue Value is %s",dcmCodeValue.c_str());
                                   }else{
                                       scheduledProtocolCodeSequenceJson[CodeValueJson]=dcmCodeValue;
                                       LOGIFIND("DCM_CodeValue Not found %s",dcmCodeValueTag.text());
                                   }

                                    OFString strCodingSchemeDesignator;
                                    auto codingSchemeDesignatorTag =ScheduledProtocolCodeSequenceItem->findAndGetOFString(DCM_CodingSchemeDesignator,strCodingSchemeDesignator);
                                    if(codingSchemeDesignatorTag.good()){
                                        scheduledProtocolCodeSequenceJson[CodingSchemeDesignatorJson]=strCodingSchemeDesignator;
                                        LOGIFIND("DCM_CodingSchemeDesignator Value is %s",strCodingSchemeDesignator.c_str());
                                    }else{
                                        scheduledProtocolCodeSequenceJson[CodingSchemeDesignatorJson]=strCodingSchemeDesignator;
                                        LOGIFIND("DCM_CodingSchemeDesignator Not found %s",codingSchemeDesignatorTag.text());
                                    }

                                    OFString strCodingSchemeVersion;
                                    auto codingSchemeVersionTag =ScheduledProtocolCodeSequenceItem->findAndGetOFString(DCM_CodingSchemeVersion,strCodingSchemeVersion);
                                    if(codingSchemeVersionTag.good()){
                                        scheduledProtocolCodeSequenceJson[CodingSchemeVersionJson]=strCodingSchemeVersion;
                                        LOGIFIND("DCM_CodingSchemeVersion Value is %s",strCodingSchemeVersion.c_str());
                                    }else{
                                        scheduledProtocolCodeSequenceJson[CodingSchemeVersionJson]=strCodingSchemeVersion;
                                        LOGIFIND("DCM_CodingSchemeVersion Not found %s",codingSchemeVersionTag.text());
                                    }

                                    OFString strCodeMeaning;
                                    auto codeMeaningVersionTag =ScheduledProtocolCodeSequenceItem->findAndGetOFString(DCM_CodeMeaning,strCodeMeaning);
                                    if(codeMeaningVersionTag.good()){
                                        scheduledProtocolCodeSequenceJson[CodeMeaningJson]=strCodeMeaning;
                                        LOGIFIND("DCM_CodeMeaning Value is %s",strCodeMeaning.c_str());
                                    }else{
                                        scheduledProtocolCodeSequenceJson[CodeMeaningJson]=strCodeMeaning;
                                        LOGIFIND("DCM_CodeMeaning Not found %s",codeMeaningVersionTag.text());
                                    }

                                    spsSequenceData[ScheduledProtocolCodeSequence]=scheduledProtocolCodeSequenceJson;

                                }else{
                                    LOGIFIND("ScheduledProtocolCodeSequence Not Found");
                                }

                                root[ScheduledProcedureStepSequence]=spsSequenceData;

                            }else{
                                LOGIFIND("ScheduledProcedureStepSequenceTag Not Found");
                            }
                            //DCM_RequestedProcedureCodeSequence
                            Json::Value requestedProcedureCodeSequenceJson;
                            DcmItem *requestedProcedureCodeSequenceItem;
                            auto requestedProcedureCodeSequenceTag=(*study)->m_dataset->
                                    findOrCreateSequenceItem(DCM_RequestedProcedureCodeSequence,requestedProcedureCodeSequenceItem);
                            if(requestedProcedureCodeSequenceTag.good()){
                                OFString dcmCodeValue;
                                auto dcmCodeValueTag =requestedProcedureCodeSequenceItem->findAndGetOFString(DCM_CodeValue,dcmCodeValue);
                                if(dcmCodeValueTag.good()){
                                    requestedProcedureCodeSequenceJson[CodeValueJson]=dcmCodeValue;
                                    LOGIFIND("DCM_CodeValue Value is %s",dcmCodeValue.c_str());
                                }else{
                                    requestedProcedureCodeSequenceJson[CodeValueJson]=dcmCodeValue;
                                    LOGIFIND("DCM_CodeValue Not found %s",dcmCodeValueTag.text());
                                }

                                OFString strCodingSchemeDesignator;
                                auto codingSchemeDesignatorTag =requestedProcedureCodeSequenceItem->findAndGetOFString(DCM_CodingSchemeDesignator,strCodingSchemeDesignator);
                                if(codingSchemeDesignatorTag.good()){
                                    requestedProcedureCodeSequenceJson[CodingSchemeDesignatorJson]=strCodingSchemeDesignator;
                                    LOGIFIND("DCM_CodingSchemeDesignator Value is %s",strCodingSchemeDesignator.c_str());
                                }else{
                                    requestedProcedureCodeSequenceJson[CodingSchemeDesignatorJson]=strCodingSchemeDesignator;
                                    LOGIFIND("DCM_CodingSchemeDesignator Not found %s",codingSchemeDesignatorTag.text());
                                }

                                OFString strCodingSchemeVersion;
                                auto codingSchemeVersionTag =requestedProcedureCodeSequenceItem->findAndGetOFString(DCM_CodingSchemeVersion,strCodingSchemeVersion);
                                if(codingSchemeVersionTag.good()){
                                    requestedProcedureCodeSequenceJson[CodingSchemeVersionJson]=strCodingSchemeVersion;
                                    LOGIFIND("DCM_CodingSchemeVersion Value is %s",strCodingSchemeVersion.c_str());
                                }else{
                                    requestedProcedureCodeSequenceJson[CodingSchemeVersionJson]=strCodingSchemeVersion;
                                    LOGIFIND("DCM_CodingSchemeVersion Not found %s",codingSchemeVersionTag.text());
                                }

                                OFString strCodeMeaning;
                                auto codeMeaningVersionTag =requestedProcedureCodeSequenceItem->findAndGetOFString(DCM_CodeMeaning,strCodeMeaning);
                                if(codeMeaningVersionTag.good()){
                                    requestedProcedureCodeSequenceJson[CodeMeaningJson]=strCodeMeaning;
                                    LOGIFIND("DCM_CodeMeaning Value is %s",strCodeMeaning.c_str());
                                }else{
                                    requestedProcedureCodeSequenceJson[CodeMeaningJson]=strCodeMeaning;
                                    LOGIFIND("DCM_CodeMeaning Not found %s",codeMeaningVersionTag.text());
                                }

                                root[RequestedProcedureCodeSequence]=requestedProcedureCodeSequenceJson;

                            }else{
                                LOGIFIND("DCM_RequestedProcedureCodeSequence Not Found");
                            }

                            DcmItem *mReferencedStudySequence;

                            auto referencedStudySequenceTag=(*study)->m_dataset->
                                    findAndGetSequenceItem(DCM_ReferencedStudySequence,mReferencedStudySequence);

                            if(referencedStudySequenceTag.good()){
                                Json::Value referencedStudySequenceJson;
                                LOGIFIND("DCM_ReferencedStudySequence  Found");
                                OFString strReferencedSOPClassUID;
                                auto referencedSOPClassUIDTag =mReferencedStudySequence->findAndGetOFString(DCM_ReferencedSOPClassUID,strReferencedSOPClassUID);
                                if(referencedSOPClassUIDTag.good()){
                                    referencedStudySequenceJson[ReferencedSOPClassUID]=strReferencedSOPClassUID;
                                    LOGIFIND("DCM_ReferencedSOPClassUID Value is %s",strReferencedSOPClassUID.c_str());
                                }else{
                                    referencedStudySequenceJson[ReferencedSOPClassUID]=strReferencedSOPClassUID;
                                    LOGIFIND("DCM_ReferencedSOPClassUID Not found %s",referencedSOPClassUIDTag.text());
                                }


                                OFString strReferencedSOPInstanceUID;
                                auto dcmCodeValueTag =mReferencedStudySequence->findAndGetOFString(DCM_ReferencedSOPInstanceUID,strReferencedSOPInstanceUID);
                                if(dcmCodeValueTag.good()){
                                    referencedStudySequenceJson[ReferencedSOPInstanceUID]=strReferencedSOPInstanceUID;
                                    LOGIFIND("DCM_ReferencedSOPInstanceUID Value is %s",strReferencedSOPInstanceUID.c_str());
                                }else{
                                    referencedStudySequenceJson[ReferencedSOPInstanceUID]=strReferencedSOPInstanceUID;
                                    LOGIFIND("DCM_ReferencedSOPInstanceUID Not found %s",dcmCodeValueTag.text());
                                }
                                root[ReferencedStudySequence]=referencedStudySequenceJson;
                            }else{
                                LOGIFIND("DCM_ReferencedStudySequence Not Found");
                            }



                            OFString strAdmittingDiagnosesDescription;
                            auto admittingDiagnosesDescriptionTag=(*study)->m_dataset->findAndGetOFString(DCM_AdmittingDiagnosesDescription,strAdmittingDiagnosesDescription);
                            if(admittingDiagnosesDescriptionTag.good()){
                                root[AdmittingDiagnosesDescription]=strAdmittingDiagnosesDescription;
                            }else{
                                LOGIFIND("DCM_AdmittingDiagnosesDescription Error: %s",admittingDiagnosesDescriptionTag.text());
                            }

                            OFString strMedicalAlerts;
                            auto medicalAlertsTag=(*study)->m_dataset->findAndGetOFString(DCM_MedicalAlerts,strMedicalAlerts);
                            if(medicalAlertsTag.good()){
                                root[MedicalAlerts]=strMedicalAlerts;
                            }else{
                                LOGIFIND("DCM_MedicalAlerts Error: %s",medicalAlertsTag.text());
                            }

                            OFString strAdditionalPatientHistory;
                            auto additionalPatientHistoryTag=(*study)->m_dataset->findAndGetOFString(DCM_AdditionalPatientHistory,strAdditionalPatientHistory);
                            if(additionalPatientHistoryTag.good()){
                                root[AdditionalPatientHistory]=strAdditionalPatientHistory;
                            }else{
                                LOGIFIND("DCM_AdditionalPatientHistory Error: %s",additionalPatientHistoryTag.text());
                            }

                            OFString strPatientComments;
                            auto patientCommentsTag=(*study)->m_dataset->findAndGetOFString(DCM_PatientComments,strPatientComments);
                            if(patientCommentsTag.good()){
                                root[PatientComments]=strPatientComments;
                                if(!strPatientComments.empty()){
                                    mwlKeyArray.append(PatientComments);
                                    env->CallVoidMethod(patientObject,setPatientComments,env->NewStringUTF(strPatientComments.c_str()));
                                }
                            }else{
                                LOGIFIND("DCM_PatientComments Error: %s",patientCommentsTag.text());
                            }

                            OFString strAllergies;
                            auto allergiesTag=(*study)->m_dataset->findAndGetOFString(DCM_Allergies,strAllergies);
                            if(allergiesTag.good()){
                                root[Allergies]=strAllergies;
                            }else{
                                LOGIFIND("DCM_Allergies Error: %s",allergiesTag.text());
                            }

                            OFString strRequestingPhysician;
                            auto requestingPhysicianTag=(*study)->m_dataset->findAndGetOFString(DCM_RequestingPhysician,strRequestingPhysician);
                            if(requestingPhysicianTag.good()){
                                root[RequestingPhysician]=strRequestingPhysician;
                            }else{
                                LOGIFIND("DCM_RequestingPhysician Error: %s",requestingPhysicianTag.text());
                            }

                            OFString strReasonForTheRequestedProcedure;
                            auto reasonForTheRequestedProcedureTag=(*study)->m_dataset->findAndGetOFString(DCM_ReasonForTheRequestedProcedure,strReasonForTheRequestedProcedure);
                            if(reasonForTheRequestedProcedureTag.good()){
                                root[ReasonForTheRequestedProcedure]=strReasonForTheRequestedProcedure;
                            }else{
                                LOGIFIND("DCM_ReasonForTheRequestedProcedure Error: %s",reasonForTheRequestedProcedureTag.text());
                            }

                            OFString strNamesOfIntendedRecipientsOfResults;
                            auto namesOfIntendedRecipientsOfResultsTag=(*study)->m_dataset->findAndGetOFString(DCM_NamesOfIntendedRecipientsOfResults,strNamesOfIntendedRecipientsOfResults);
                            if(namesOfIntendedRecipientsOfResultsTag.good()){
                                root[NamesOfIntendedRecipientsOfResults]=strNamesOfIntendedRecipientsOfResults;
                            }else{
                                LOGIFIND("DCM_NamesOfIntendedRecipientsOfResults Error: %s",namesOfIntendedRecipientsOfResultsTag.text());
                            }

                            OFString strPregnancyStatus;
                            auto pregnancyStatusTag=(*study)->m_dataset->findAndGetOFString(DCM_PregnancyStatus,strPregnancyStatus);
                            if(pregnancyStatusTag.good()){
                                root[PregnancyStatus]=strPregnancyStatus;
                            }else{
                                LOGIFIND("DCM_PregnancyStatus Error: %s",pregnancyStatusTag.text());
                            }


                            OFString strInstitutionName;
                            auto institutionNameTag=(*study)->m_dataset->findAndGetOFString(DCM_InstitutionName,strInstitutionName);
                            if(institutionNameTag.good()){
                                root[InstitutionName]=strInstitutionName;
                            }else{
                                LOGIFIND("DCM_InstitutionName Error: %s",institutionNameTag.text());
                            }

                            OFString strPhysiciansOfRecord;
                            auto physiciansOfRecordTag=(*study)->m_dataset->findAndGetOFString(DCM_PhysiciansOfRecord,strPhysiciansOfRecord );
                            if (physiciansOfRecordTag.good()){
                                LOGIFIND("DCM_PhysiciansOfRecord Id: %s", strPhysiciansOfRecord.c_str());
                                if(!strPhysiciansOfRecord.empty()){
                                    env->CallVoidMethod(patientObject,setPhysiciansOfRecord,env->NewStringUTF(strPhysiciansOfRecord.c_str()));
                                    mwlKeyArray.append("PhysiciansOfRecord");
                                }
                            }else{
                                LOGIFIND("DCM_PhysiciansOfRecord Error: %s",physiciansOfRecordTag.text());
                            }

                            // Received data
                            OFString studyId;
                            if ((*study)->m_dataset->findAndGetOFStringArray(DCM_StudyID,studyId ).good())
                            {
                                //cout << "Study Id is : " << studyId << endl;
                                LOGIFIND("Study Id: %s", studyId.c_str());
                            }
                            OFString strStudyInstantUID;
                            auto studyInstantUid=(*study)->m_dataset->findAndGetOFString(DCM_StudyInstanceUID,strStudyInstantUID );

                            if (studyInstantUid.good()){
                                //cout << "Study Id is : " << studyId << endl;
                                LOGIFIND("DCM_StudyInstanceUID Id: %s", strStudyInstantUID.c_str());
                                env->CallVoidMethod(patientObject,setStudyInstantUID,env->NewStringUTF(strStudyInstantUID.c_str()));
                                mwlKeyArray.append("StudyInstanceUID");
                            }else{
                                LOGIFIND("DCM_StudyInstanceUID Error: %s",studyInstantUid.text());
                            }
                          

                            const char *studyModality;
                            auto modality=(*study)->m_dataset->findAndGetString(DCM_Modality, studyModality );
                            if (modality.good())
                            {
                                //cout << "Study Modality : " << studyModality << endl;
                                LOGIFIND("Study Modality: %s",studyModality);
                            }else{
                                LOGIFIND("Study Modality: Error %s ", modality.text());
                            }

                            OFString studyDate;
                            auto findStudyDate=(*study)->m_dataset->findAndGetOFString(DCM_StudyDate, studyDate);
                            if (findStudyDate.good())
                            {
                                mwlKeyArray.append("StudyDate");
                                // cout << "Study Modality : " << studyDate << endl;
                                LOGIFIND("Study Date: %s", studyDate.c_str());
                            }else{

                                LOGIFIND("Study Date Error: %s", findStudyDate.text());
                            }

                            OFString studyTime;
                            auto findStudyTime=(*study)->m_dataset->findAndGetOFString(DCM_StudyTime, studyTime);
                            if (findStudyTime.good())
                            {
                                mwlKeyArray.append("StudyTime");
                                //  cout << "Study Modality : " << studyTime << endl;
                                LOGIFIND("Study Time: %s", studyTime.c_str());
                            }else{
                                LOGIFIND("Study Time Error: %s", findStudyTime.text());
                            }


                            OFString patientName;

                            if((*study)->m_dataset->findAndGetOFStringArray(DCM_PatientName, patientName).good()){

                                mwlKeyArray.append("PatientName");
                                LOGIFIND("Patient Information Fetched %s",patientName.c_str());
                                //env->SetObjectField(patientMwlClass,setPatientNameField,env->NewStringUTF(patientName.c_str()));
                                env->CallVoidMethod(patientObject,setPatientName,env->NewStringUTF(patientName.c_str()));
                            }else{
                                LOGIFIND("Patient Name not found");
                            }

                            OFString patientMrn;
                            auto patientMrnTag=(*study)->m_dataset->findAndGetOFString(DCM_PatientID, patientMrn);
                            if(patientMrnTag.good()){
                                mwlKeyArray.append("PatientID");
                                LOGIFIND("Patient  MRN Information Fetched %s",patientMrn.c_str());
                                //env->SetObjectField(patientMwlClass,setPatientNameField,env->NewStringUTF(patientName.c_str()));
                                env->CallVoidMethod(patientObject,setPatientMrn,env->NewStringUTF(patientMrn.c_str()));
                            }else{
                                LOGIFIND("Patient MRN not found %s", patientMrnTag.text());
                            }

                            OFString accessionNumber;
                            auto accessionNumberTag=(*study)->m_dataset->findAndGetOFString(DCM_AccessionNumber, accessionNumber);
                            if(accessionNumberTag.good()){
                                mwlKeyArray.append("AccessionNumber");
                                LOGIFIND("AccessionNumber Information Fetched %s",accessionNumber.c_str());
                                //env->SetObjectField(patientMwlClass,setPatientNameField,env->NewStringUTF(patientName.c_str()));
                                env->CallVoidMethod(patientObject,setAccessionNumber,env->NewStringUTF(accessionNumber.c_str()));
                            }else{
                                LOGIFIND("AccessionNumber not found %s", accessionNumberTag.text());
                            }

                            OFString patientDob;
                            auto patientDobTag=(*study)->m_dataset->findAndGetOFString(DCM_PatientBirthDate, patientDob);
                            if(patientDobTag.good()){
                                mwlKeyArray.append("PatientBirthDate");
                                LOGIFIND("Patient  DOB Information Fetched %s",patientDob.c_str());
                                //env->SetObjectField(patientMwlClass,setPatientNameField,env->NewStringUTF(patientName.c_str()));
                                env->CallVoidMethod(patientObject,setPatientDOB,env->NewStringUTF(patientDob.c_str()));
                            }else{
                                LOGIFIND("Patient DOB not found %s", patientDobTag.text());
                            }


                            OFString rpId;
                            auto rpIdTag=(*study)->m_dataset->findAndGetOFString(DCM_RequestedProcedureID, rpId);
                            if(rpIdTag.good()){
                                LOGIFIND("Patient  RPID Information Fetched %s",rpId.c_str());
                                root[RequestedProcedureID]=rpId;
                                env->CallVoidMethod(patientObject,setRpId,env->NewStringUTF(rpId.c_str()));
                            }else{
                                root[RequestedProcedureID]=rpId;
                                LOGIFIND("RPID not found %s", rpIdTag.text());
                            }

                            OFString patientSex;
                            auto patientSexTag=(*study)->m_dataset->findAndGetOFString(DCM_PatientSex, patientSex);
                            if(patientSexTag.good()){
                                if(!patientSex.empty()){
                                    mwlKeyArray.append("PatientSex");
                                }

                                LOGIFIND("Patient  Sex Information Fetched %s",patientSex.c_str());
                                env->CallVoidMethod(patientObject,setPatientSex,env->NewStringUTF(patientSex.c_str()));
                            }else{
                                LOGIFIND("PatientSex not found %s", patientSexTag.text());
                            }

                            OFString patientEthnic;
                            auto patientEthnicTag=(*study)->m_dataset->findAndGetOFString(DCM_EthnicGroup, patientEthnic);
                            if(patientEthnicTag.good()){
                                if(!patientEthnic.empty()){
                                    mwlKeyArray.append("EthnicGroup");
                                }
                                LOGIFIND("DCM_EthnicGroup Information Fetched %s",patientEthnic.c_str());
                                env->CallVoidMethod(patientObject,setEthnicity,env->NewStringUTF(patientEthnic.c_str()));
                            }else{
                                LOGIFIND("DCM_EthnicGroup not found %s", patientEthnicTag.text());
                            }

                            OFString requestedProcedureDescription;
                            auto rpDescp = (*study)->m_dataset->findAndGetOFString(DCM_RequestedProcedureDescription, requestedProcedureDescription);
                            if(rpDescp.good()){
                                LOGIFIND("DCM_RequestedProcedureDescription RPDescr Fetched %s",requestedProcedureDescription.c_str());
                                root[RequestedProcedureDescription]=requestedProcedureDescription;
                            }else{
                                root[RequestedProcedureDescription]="";
                            }

                            OFString strPatientSize;
                            auto patientSizeTag=(*study)->m_dataset->findAndGetOFString(DCM_PatientSize, strPatientSize);
                            if(patientSizeTag.good()){
                                if(!strPatientSize.empty()){
                                    mwlKeyArray.append("PatientSize");
                                }
                                LOGIFIND("Patient Size Fetched %s",strPatientSize.c_str());
                                env->CallVoidMethod(patientObject,setPatientSize,env->NewStringUTF(strPatientSize.c_str()));
                            }else{
                                LOGIFIND("DCM_PatientSize not found %s", patientSizeTag.text());
                            }

                            OFString strPatientWeight;
                            auto patientWeightTag=(*study)->m_dataset->findAndGetOFString(DCM_PatientWeight, strPatientWeight);
                            if(patientWeightTag.good()){
                                if(!strPatientWeight.empty()){
                                    mwlKeyArray.append("PatientWeight");
                                }

                                LOGIFIND("Patient Weight Fetched %s",strPatientWeight.c_str());
                                env->CallVoidMethod(patientObject,setPatientWeight,env->NewStringUTF(strPatientWeight.c_str()));
                            }else{
                                LOGIFIND("DCM_PatientWeight not found %s", patientWeightTag.text());
                            }

                            OFString strReferringPhysicianName;
                            auto referringPhysicianNameTag=(*study)->m_dataset->findAndGetOFString(DCM_ReferringPhysicianName, strReferringPhysicianName);
                            if(referringPhysicianNameTag.good()){
                                if(!strReferringPhysicianName.empty()){
                                    mwlKeyArray.append("ReferringPhysicianName");
                                }
                                LOGIFIND("ReferringPhysicianName Fetched %s",strReferringPhysicianName.c_str());
                                env->CallVoidMethod(patientObject,setReferringPhysicianName,env->NewStringUTF(strReferringPhysicianName.c_str()));
                            }else{
                                LOGIFIND("DCM_ReferringPhysicianName not found %s", referringPhysicianNameTag.text());
                            }

                            keyRoot["MwlKeys"]=mwlKeyArray;

                            Json::StreamWriterBuilder builder;
                            const std::string json_file = Json::writeString(builder, root);

                            const std::string key_list = Json::writeString(builder, keyRoot);

                            env->CallVoidMethod(patientObject,setPatientMwlDatas,env->NewStringUTF(json_file.c_str()));
                            env->CallVoidMethod(patientObject,setPatientKeys,env->NewStringUTF(key_list.c_str()));

                            LOGIFIND("Mwl KEYS is %s", key_list.c_str());
                            LOGIFIND("Mwl JSON is %s", json_file.c_str());

                            //Insert into the Array
                            env->CallBooleanMethod(mArrayMwl, mjMethodId, patientObject);
                            env->CallVoidMethod(thiz,mAddMwlPatient,patientObject);
                            LOGIFIND("Mwl Object inserted ");
                        }
                    }
                    mMwlResult = getMwlResultObject(env,"Success",SUCCESS);
                    if(containData){
                        LOGIFIND("MWl set Array into JNI Object");
                        env->CallVoidMethod(mMwlResult,mMwlResultSetMethod,mArrayMwl);
                        LOGIFIND("MWl set Array into JNI Object Done");
                    }
                }else{
                    //Error in the find request
                    LOGIFIND("MWL::QUERY ERROR %s , Error Code %d  C-FIND ",cond.text(),cond.code());
                    string errorMsg="Error in the C-FIND:" + string(cond.text());
                    mMwlResult = getMwlResultObject(env,errorMsg,ERROR);
                }

                while (!findResponses->empty())
                {
                    delete findResponses->front();
                    findResponses->pop_front();
                }
            }
        }else{
            LOGIFIND("MWL::QUERY ERROR %s , Error Code %d  Association ",cond.text(),cond.code());
            //LOGI("MWL Unable to negotiate association: %s ", cond.text());
            string errorMsg="Unable to get valid PresentationContextID:" + string(cond.text());
            mMwlResult = getMwlResultObject(env,errorMsg,ERROR);
        }
    }else{
        //Network InitFailed
        LOGIFIND("MWL::QUERY ERROR %s , Error Code %d  Network ",cond.text(),cond.code());
        string errorMsg="Unable to set up the network:" + string(cond.text());
        mMwlResult = getMwlResultObject(env,errorMsg,ERROR);
    }
    closeClientFindScu();
    return mMwlResult;
}
