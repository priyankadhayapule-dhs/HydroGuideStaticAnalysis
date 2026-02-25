//
// Created by Echonous on 18/7/19.
//
#include <jni.h>
#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/ofstd/ofcond.h"
#include "include/dcmtkUtilJni.h"
#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */
#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmdata/cmdlnarg.h"
#include "dcmtk/ofstd/ofconapp.h"
#include "dcmtk/dcmdata/dcuid.h"       /* for dcmtk version name */ /* Covers most common dcmdata classes */
#include "dcmtk/ofstd/ofstd.h"
#include "dcmtk/ofstd/ofdatime.h"
#include "dcmtk/dcmdata/dccodec.h"
#include "dcmtk/ofstd/ofstdinc.h"
#include "dcmtk/dcmdata/dcitem.h"
#include "jsoncpp/json/json.h"

#include <android/log.h>
#include <dcmtkClientJni.h>

#define ERROR 0
#define SUCCESS 1


BEGIN_EXTERN_C
#ifdef HAVE_FCNTL_H
#include <fcntl.h>       /* for O_RDONLY */
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>   /* required for sys/stat.h */
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>    /* for stat, fstat */
#endif
END_EXTERN_C

using namespace std;


#define  LOGIPDF(...)  __android_log_print(ANDROID_LOG_DEBUG,"dcmtk::",__VA_ARGS__)

static void createIdentifiers(
        OFBool opt_readSeriesInfo,
        const char *opt_seriesFile,
        OFString& studyUID,
        OFString& seriesUID,
        OFString& patientName,
        OFString& patientID,
        OFString& patientBirthDate,
        OFString& patientSex,
        Sint32& incrementedInstance)
{
    char buf[100];
    if (opt_seriesFile)
    {
        DcmFileFormat dfile;
        OFCondition cond = dfile.loadFile(opt_seriesFile, EXS_Unknown, EGL_noChange);
        if (cond.bad())
        {
            //OFLOG_WARN(pdf2dcmLogger, cond.text() << ": reading file: "<< opt_seriesFile);
            LOGIPDF(": reading file: %s",cond.text());
        }
        else
        {
            const char *c = nullptr;
            DcmDataset *dset = dfile.getDataset();
            if (dset)
            {
                // read patient attributes
                c = nullptr;
                if (dset->findAndGetString(DCM_PatientName, c).good() && c) patientName = c;
                c = nullptr;
                if (dset->findAndGetString(DCM_PatientID, c).good() && c) patientID = c;
                c = nullptr;
                if (dset->findAndGetString(DCM_PatientBirthDate, c).good() && c) patientBirthDate = c;
                c = nullptr;
                if (dset->findAndGetString(DCM_PatientSex, c).good() && c) patientSex = c;

                // read study attributes
                c = nullptr;
                if (dset->findAndGetString(DCM_StudyInstanceUID, c).good() && c) studyUID = c;

                // read series attributes
                if (opt_readSeriesInfo)
                {
                    c = nullptr;
                    if (dset->findAndGetString(DCM_SeriesInstanceUID, c).good() && c) seriesUID = c;
                    if (dset->findAndGetSint32(DCM_InstanceNumber, incrementedInstance).good()) ++incrementedInstance; else incrementedInstance = 0;
                }
            }
        }
    }
    if (studyUID.empty())
    {
        dcmGenerateUniqueIdentifier(buf, SITE_STUDY_UID_ROOT);
        studyUID = buf;
    }
    if (seriesUID.empty())
    {
        dcmGenerateUniqueIdentifier(buf, SITE_SERIES_UID_ROOT);
        seriesUID = buf;
    }
}


JNIEXPORT jobject
DCMTKUTIL_METHOD(createPdfDcm)(JNIEnv* env,jobject thiz,jobjectArray dcmInfo,jstring inputFilePath,jstring outPutFilePath){

    jboolean isCopy;
    const char* patientName=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo,0),&isCopy);
    const char* patientId=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo,1),&isCopy);
    const char* patientSex=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo,2),&isCopy);
    const char* studyId=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo,3),&isCopy);
    const char* studyDate=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo,4),&isCopy);
    const char* studyAccessNumber=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo,5),&isCopy);
    const char* timeZoneoffSetFromUTC=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, timezoneIndex),&isCopy);
    const char* patientDoB=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, patientDoBIndex),&isCopy);
    const char* referPhysician=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, referPhysicianIndex),&isCopy);
    const char* patientOrientation=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, patientOrientationIndex),&isCopy);
    const char* instanceNumber=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, instanceNumberIndex),&isCopy);
    const char* seriesNumber=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, seriesNumberIndex),&isCopy);
    const char* studyTime=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, studyTimeIndex),&isCopy);
    const char* facilityName=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, institutionNameIndex),&isCopy);
    const char* examPurpose=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, examPurposeIndex),&isCopy);
    const char* patientComments=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, healthConditionIndex),&isCopy);
    const char* deviceSerialNo=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, deviceSerialIndex),&isCopy);
    const char* softVerNo=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, softVerIndex),&isCopy);
    const char* imageType=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, imageTypeIndex),&isCopy);
    const char* acquisitionDate=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, acquisitionDateIndex),&isCopy);
    const char* acquisitionTime=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, acquisitionTimeIndex),&isCopy);
    const char* protocolName=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, protocolNameIndex),&isCopy);
    const char* ethnicGroup=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, ethnicGroupIndex),&isCopy);
    const char* patientWeight=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, patientWeightIndex),&isCopy);
    const char* patientHeight=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, patientHeightIndex),&isCopy);
    const char* studyInstance=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, studyInstantUID),&isCopy);
    const char* seriesInstance=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, seriesInstantUID),&isCopy);
    const char* performPhysician=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, performPhysicianIndex),&isCopy);
    const char* physicianofRecord=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, physicianOfRecordIndex),&isCopy);
    const char* kosmosStationName=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, kosmosStationNameIndex),&isCopy);
    const char* mwlJsonData = env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, mwlJsonDataIndex),&isCopy);
    const char* sopInstantUid = env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, sopInstantUID),&isCopy);
    const char* calReportComment = env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, calReportCommentIndex),&isCopy);
    const char* patientAlternativeId=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, patientAlternativeIdIndex),&isCopy);

    string mJsonString(mwlJsonData);
    bool mIsMwlData;
    bool mIsMwlRead;
    mIsMwlData = !mJsonString.empty();
    LOGIPDF("DCM MWL JSON DATA Is %s", mJsonString.c_str());

    std::stringstream jsonData{mJsonString};
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    bool ret = Json::parseFromStream(builder, jsonData, &root, &errs);
    if (!ret || !root.isObject()) {
        std::cout << "cannot convert string to json, err: " << errs << std::endl;
        LOGIPDF("JSON Read Error for the MWL Data");
        mIsMwlRead= false;
    }else{
        LOGIPDF("JSON Read Success for the MWL Data");
        mIsMwlRead = true;
    }
    const char* pdfInputFilePath=env->GetStringUTFChars(inputFilePath,&isCopy);
    const char* dcmOutPutFilePath=env->GetStringUTFChars(outPutFilePath,&isCopy);

    E_TransferSyntax opt_oxfer = EXS_LittleEndianExplicit;
    E_GrpLenEncoding opt_oglenc = EGL_withoutGL;
    E_EncodingType opt_oenctype = EET_ExplicitLength;
    E_PaddingEncoding opt_opadenc = EPD_withoutPadding;
    OFCmdUnsignedInt opt_filepad = 0;
    OFCmdUnsignedInt opt_itempad = 0;

    // document specific options
    const char *   opt_seriesFile = nullptr;
    const char *   opt_patientName = nullptr;//patient's name in DICOM PN syntax"
    const char *   opt_patientID = nullptr;//patient identifier
    const char *   opt_patientBirthdate = nullptr;//patient's birth date Format is YYYYMMDD
    const char *   opt_documentTitle = nullptr;
    const char *   opt_conceptCSD = nullptr;
    const char *   opt_conceptCV = nullptr;
    const char *   opt_conceptCM = nullptr;

    OFBool         opt_readSeriesInfo = OFFalse;
    const char *   opt_patientSex = nullptr;//string (M, F or O)
    OFBool         opt_annotation = OFTrue;
    OFCmdSignedInt opt_instance = 1;
    OFBool         opt_increment = OFFalse;

    jobject resultObject;

    //Get Input file name
    //Get Output file name
    dcmEnableGenerationOfNewVRs();
    /*Removed as PART of THSW:272 comments
     * if ((patientName != NULL) && ((patientName[0] == '\0') || (patientName[0] == ' '))){
        patientName = "Not Applicable";
    }*/

    opt_patientName=patientName;
    opt_patientID=patientId;
    opt_patientSex=patientSex;

    // create study and series UID
    OFString studyUID=std::string(studyId);
    OFString seriesUID;
    OFString patientNameLocal=std::string(opt_patientName);
    OFString patientID=std::string(opt_patientID);
    OFString patientBirthDate=std::string(patientDoB);
    OFString patientSexLocal=std::string(opt_patientSex);
    Sint32 incrementedInstance = 0;

    createIdentifiers(opt_readSeriesInfo, opt_seriesFile, studyUID, seriesUID, patientNameLocal, patientID, patientBirthDate, patientSexLocal, incrementedInstance);

    //OFLOG_INFO(pdf2dcmLogger, "creating encapsulated PDF object");
    LOGIPDF("creating encapsulated PDF object");

    DcmFileFormat fileformat;

    OFCondition result = insertPDFFile(fileformat.getDataset(), pdfInputFilePath);
    if (result.bad())
    {
        //OFLOG_ERROR(pdf2dcmLogger, "unable to create PDF DICOM encapsulation");
        string errorMessage="unable to create PDF DICOM encapsulation"+string(result.text());
        resultObject=getResultObject(env,errorMessage,ERROR);
        return resultObject;
    }

    // now we need to generate an instance number that is guaranteed to be unique within a series.

    result = createHeader(fileformat.getDataset(), patientNameLocal.c_str(), patientID.c_str(),
                          patientBirthDate.c_str(), patientSexLocal.c_str(), opt_annotation, studyUID.c_str(),
                          seriesUID.c_str(), opt_documentTitle, opt_conceptCSD, opt_conceptCV, opt_conceptCM, OFstatic_cast(Sint32, opt_instance),
                          timeZoneoffSetFromUTC,studyAccessNumber,studyDate,referPhysician,studyInstance,seriesInstance,sopInstantUid);

    DcmDataset *dataset=fileformat.getDataset();
    dataset->putAndInsertString(DCM_StudyTime, studyTime);
    dataset->putAndInsertString(DCM_InstitutionName,facilityName);
    dataset->putAndInsertString(DCM_StudyDescription,examPurpose);
    dataset->putAndInsertString(DCM_PatientComments,patientComments);
    if(string(calReportComment).compare("NC") != 0){
        dataset->putAndInsertString(DCM_PatientComments,calReportComment,OFTrue);
    }
    dataset->putAndInsertString(DCM_DeviceSerialNumber,deviceSerialNo);
    dataset->putAndInsertString(DCM_SoftwareVersions,softVerNo);
    dataset->putAndInsertString(DCM_AcquisitionDate,acquisitionDate,OFTrue);
    string aquisitionDateTime=string();
    aquisitionDateTime.append(acquisitionTime,6);
    dataset->putAndInsertString(DCM_AcquisitionTime,aquisitionDateTime.c_str(),OFTrue);
    dataset->putAndInsertString(DCM_EthnicGroup,ethnicGroup);
    dataset->putAndInsertString(DCM_PatientSize,patientHeight,OFTrue);
    dataset->putAndInsertString(DCM_PatientWeight,patientWeight,OFTrue);
    dataset->putAndInsertString(DCM_PatientOrientation, patientOrientation);  //anterior  https://dicom.innolitics.com/ciods/us-image/general-image/00200020
    LOGIPDF("InstanceNumber is  PDF %s",instanceNumber);
    dataset->putAndInsertString(DCM_InstanceNumber, instanceNumber);
    dataset->putAndInsertString(DCM_SeriesNumber, seriesNumber);

    dataset->putAndInsertString(DCM_PerformedProcedureStepStartDate,studyDate,OFTrue);
    dataset->putAndInsertString(DCM_PerformedProcedureStepStartTime,studyTime,OFTrue);
    //PerformedProcedureStepDescription
    dataset->putAndInsertString(DCM_PerformedProcedureStepDescription,examPurpose,OFTrue);

    dataset->putAndInsertString(DCM_PhysiciansOfRecord,physicianofRecord,OFTrue);
    dataset->putAndInsertString(DCM_PerformingPhysicianName,performPhysician,OFTrue);
    dataset->putAndInsertString(DCM_OperatorsName,performPhysician,OFTrue);
    dataset->putAndInsertString(DCM_StationName,kosmosStationName,OFTrue);
    dataset->putAndInsertString(DCM_SOPInstanceUID, sopInstantUid, OFTrue);

    dataset->putAndInsertString(DCM_SeriesDate,studyDate,OFTrue);
    dataset->putAndInsertString(DCM_SeriesTime,studyTime,OFTrue);

    dataset->putAndInsertString(DCM_ContentDate,acquisitionDate,OFTrue);
    dataset->putAndInsertString(DCM_ContentTime,acquisitionTime,OFTrue);

    if(mIsMwlRead && mIsMwlData){
        //DcmItem *mRequestAttributesSequence;
        Json::Value scheduledProcedurespJson;
        auto rpId =root[RequestedProcedureID].asString();
        auto rpDesc =root[RequestedProcedureDescription].asString();
        scheduledProcedurespJson = root[ScheduledProcedureStepSequence];
        bool hasReferencedStudySequence = root.isMember(ReferencedStudySequence);

        auto spStepId = scheduledProcedurespJson[ScheduledProcedureStepID].asString();
        auto spStepDesc = scheduledProcedurespJson[ScheduledProcedureStepDescription].asString();
        //Encode following data
        DcmItem *mRequestAttributesSequence;
        if(dataset->findOrCreateSequenceItem(DCM_RequestAttributesSequence,mRequestAttributesSequence).good()){
            LOGIPDF("MWL Encoding DCM_RequestAttributesSequence");
            result=mRequestAttributesSequence->putAndInsertString(DCM_RequestedProcedureDescription, rpDesc.c_str());
            if(result.bad()){
                LOGIPDF("Bad DCM_RequestedProcedureDescription RequestAttributesSequence");
            }

            result=mRequestAttributesSequence->putAndInsertString(DCM_ScheduledProcedureStepDescription, spStepDesc.c_str());
            if(result.bad()){
                LOGIPDF("Bad DCM_ScheduledProcedureStepDescription RequestAttributesSequence");
            }

            result=mRequestAttributesSequence->putAndInsertString(DCM_RequestedProcedureID, rpId.c_str());
            if(result.bad()){
                LOGIPDF("Bad RequestedProcedureID RequestAttributesSequence");
            }

            result=mRequestAttributesSequence->putAndInsertString(DCM_ScheduledProcedureStepID, spStepId.c_str());
            if(result.bad()){
                LOGIPDF("Bad DCM_ScheduledProcedureStepID RequestAttributesSequence");
            }

            bool spcs=scheduledProcedurespJson.isMember(ScheduledProtocolCodeSequence);
            if(spcs){
                Json::Value scheduledProtocolCodeSequenceJson;
                scheduledProtocolCodeSequenceJson = scheduledProcedurespJson[ScheduledProtocolCodeSequence];
                auto codeValue = scheduledProtocolCodeSequenceJson[CodeValueJson].asString();
                auto codeMeaning = scheduledProtocolCodeSequenceJson[CodeMeaningJson].asString();
                auto codingSchemeDesignator = scheduledProtocolCodeSequenceJson[CodingSchemeDesignatorJson].asString();
                auto codingSchemeVersion = scheduledProtocolCodeSequenceJson[CodingSchemeVersionJson].asString();

                DcmItem *mScheduledProtocolCodeSequence;
                if(mRequestAttributesSequence->findOrCreateSequenceItem(DCM_ScheduledProtocolCodeSequence,mScheduledProtocolCodeSequence).good()){

                    result=mScheduledProtocolCodeSequence->putAndInsertString(DCM_CodeValue, codeValue.c_str());
                    if(result.bad()){
                        LOGIPDF("Bad DCM_CodeValue ScheduledProtocolCodeSequence");
                    }

                    result=mScheduledProtocolCodeSequence->putAndInsertString(DCM_CodingSchemeDesignator, codingSchemeDesignator.c_str());
                    if(result.bad()){
                        LOGIPDF("Bad DCM_CodingSchemeDesignator ScheduledProtocolCodeSequence");
                    }
                    result=mScheduledProtocolCodeSequence->putAndInsertString(DCM_CodeMeaning, codeMeaning.c_str());
                    if(result.bad()){
                        LOGIPDF("Bad DCM_CodeMeaning ScheduledProtocolCodeSequence");
                    }

                    result = mScheduledProtocolCodeSequence->putAndInsertString(DCM_CodingSchemeVersion,codingSchemeVersion.c_str());
                    if(result.bad()){
                        LOGIPDF("Bad DCM_CodingSchemeVersion ScheduledProtocolCodeSequence");
                    }

                }else{
                    LOGIPDF("Bad DCM_ScheduledProtocolCodeSequence RequestAttributesSequence DICOM Sub tag failed");
                }
            }

            //Code direct in RequestAttributesSequence as per dcmdump view of lab dicom file


            /* DcmItem *mPerformedProtocolCodeSequence;
             if(mRequestAttributesSequence->findOrCreateSequenceItem(DCM_PerformedProtocolCodeSequence,mPerformedProtocolCodeSequence).good()){

                 result=mPerformedProtocolCodeSequence->putAndInsertString(DCM_CodeValue, studyId);
                 if(result.bad()){
                     LOGIDCM("Bad DCM_CodeValue ScheduledProtocolCodeSequence");
                 }

                 result=mPerformedProtocolCodeSequence->putAndInsertString(DCM_CodingSchemeDesignator, "");
                 if(result.bad()){
                     LOGIDCM("Bad DCM_CodingSchemeDesignator ScheduledProtocolCodeSequence");
                 }
                 result=mPerformedProtocolCodeSequence->putAndInsertString(DCM_CodeMeaning, examPurpose);
                 if(result.bad()){
                     LOGIDCM("Bad DCM_CodeMeaning ScheduledProtocolCodeSequence");
                 }
             }*/

        }

        if(hasReferencedStudySequence){
            Json::Value referencedStudySequenceJson = root[ReferencedStudySequence];
            DcmItem *mReferencedStudySequence;
            if(dataset->findOrCreateSequenceItem(DCM_ReferencedStudySequence,mReferencedStudySequence).good()){
                auto refStudySeq = referencedStudySequenceJson[ReferencedSOPClassUID].asString();
                result= mReferencedStudySequence-> putAndInsertString(DCM_ReferencedSOPClassUID, refStudySeq.c_str());
                if(result.bad()){
                    LOGIPDF("Bad DCM_ReferencedSOPClassUID ReferencedStudySequence");
                }

                auto strReferencedSOPInstanceUID = referencedStudySequenceJson[ReferencedSOPInstanceUID].asString();
                result= mReferencedStudySequence-> putAndInsertString(DCM_ReferencedSOPInstanceUID, strReferencedSOPInstanceUID.c_str());
                if(result.bad()){
                    LOGIPDF("Bad DCM_ReferencedSOPInstanceUID ReferencedStudySequence");
                }

            }

        }else{
            LOGIPDF("Cannot add DCM_ReferencedStudySequence");
        }

        if(root.isMember(InstitutionName)){
            auto strInstitutionName = root[InstitutionName].asString();
            if(!strInstitutionName.empty()){
                dataset->putAndInsertString(DCM_InstitutionName,strInstitutionName.c_str(),OFTrue);
            }
        }

        if(root.isMember(Allergies)){
            auto strAllergies = root[Allergies].asString();
            if(!strAllergies.empty()){
                dataset->putAndInsertString(DCM_Allergies,strAllergies.c_str(),OFTrue);
            }
        }

        if(root.isMember(AdmittingDiagnosesDescription)){
            auto strAdmittingDiagnosesDescription = root[AdmittingDiagnosesDescription].asString();
            if(!strAdmittingDiagnosesDescription.empty()){
                dataset->putAndInsertString(DCM_AdmittingDiagnosesDescription,strAdmittingDiagnosesDescription.c_str(),OFTrue);
            }
        }

        if(root.isMember(MedicalAlerts)){
            auto strMedicalAlerts = root[MedicalAlerts].asString();
            if(!strMedicalAlerts.empty()){
                dataset->putAndInsertString(DCM_MedicalAlerts,strMedicalAlerts.c_str(),OFTrue);
            }
        }

        if(root.isMember(AdditionalPatientHistory)){
            auto strAdditionalPatientHistory = root[AdditionalPatientHistory].asString();
            if(!strAdditionalPatientHistory.empty()){
                dataset->putAndInsertString(DCM_AdditionalPatientHistory,strAdditionalPatientHistory.c_str(),OFTrue);
            }
        }

        if(root.isMember(RequestingPhysician)){
            auto strRequestingPhysician = root[RequestingPhysician].asString();
            if(!strRequestingPhysician.empty()){
                dataset->putAndInsertString(DCM_RequestingPhysician,strRequestingPhysician.c_str(),OFTrue);
            }
        }

        if(root.isMember(ReasonForTheRequestedProcedure)){
            auto strReasonForTheRequestedProcedure = root[ReasonForTheRequestedProcedure].asString();
            if(!strReasonForTheRequestedProcedure.empty()){
                dataset->putAndInsertString(DCM_ReasonForTheRequestedProcedure,strReasonForTheRequestedProcedure.c_str(),OFTrue);
            }
        }

        if(root.isMember(NamesOfIntendedRecipientsOfResults)){
            auto strNamesOfIntendedRecipientsOfResults= root[NamesOfIntendedRecipientsOfResults].asString();
            if(!strNamesOfIntendedRecipientsOfResults.empty()){
                dataset->putAndInsertString(DCM_NamesOfIntendedRecipientsOfResults,strNamesOfIntendedRecipientsOfResults.c_str(),OFTrue);
            }
        }



        if(root.isMember(PregnancyStatus)){
            auto strPregnancyStatus= root[PregnancyStatus].asString();
            if(!strPregnancyStatus.empty()){
                auto pregnancyStatus=(Uint16)std::atoi(strPregnancyStatus.c_str());
                dataset->putAndInsertUint16(DCM_PregnancyStatus,pregnancyStatus);
            }
        }
    }

    //Register private tag if not present
    //Register private tag if not present
    if(!string(patientAlternativeId).empty()) {
        DcmDataDictionary &dict = dcmDataDict.wrlock();
        DcmHashDictIterator iter = dict.normalBegin();
        DcmHashDictIterator end = dict.normalEnd();
        bool isPrivateGroupElePresent = false;
        while (iter != end) {
            const DcmDictEntry* elem = *iter++;
            if(elem->getGroup() == PRIVATE_GROUP && elem->getElement() == PRIVATE_CREATOR_ELE) {
                isPrivateGroupElePresent = true;
            }
        }

        if(isPrivateGroupElePresent == false) {
            dict.addEntry(new DcmDictEntry(PRIVATE_CREATOR_TAG, EVR_LO, "CompanyName", 1, 1, "private", OFTrue, PRIVATE_CREATOR_NAME));
            dict.addEntry(new DcmDictEntry(PRIVATE_ELEMENT1_TAG, EVR_LO, "AlternativeId", 1, 1, "private", OFTrue, PRIVATE_CREATOR_NAME));
        }
        dcmDataDict.wrunlock();

        //Insert Alternative Id as custom tag
        if (!dataset->tagExists(PRV_PrivateCreator)) {
            dataset->putAndInsertString(PRV_PrivateCreator, PRIVATE_CREATOR_NAME);
            dataset->putAndInsertString(PRV_PrivateElement1, patientAlternativeId);
        }
    }

    if (result.bad())
    {
        //OFLOG_ERROR(pdf2dcmLogger, "unable to create DICOM header: " << result.text());
        string errorMessage="unable to create DICOM header: "+string(result.text());
        return getResultObject(env,errorMessage,ERROR);
    }


    //OFLOG_INFO(pdf2dcmLogger, "writing encapsulated PDF object as file " << opt_ofname);

    LOGIPDF("writing encapsulated PDF object as file %s",dcmOutPutFilePath);

    OFCondition error = EC_Normal;

    //OFLOG_INFO(pdf2dcmLogger, "Check if new output transfer syntax is possible");
    LOGIPDF("Check if new output transfer syntax is possible");

    DcmXfer opt_oxferSyn(opt_oxfer);

    if (fileformat.getDataset()->canWriteXfer(opt_oxfer))
    {
        //OFLOG_INFO(pdf2dcmLogger, "Output transfer syntax " << opt_oxferSyn.getXferName() << " can be written");
        LOGIPDF("Output transfer syntax %s can be written",opt_oxferSyn.getXferName());
    } else {
        //OFLOG_ERROR(pdf2dcmLogger, "No conversion to transfer syntax " << opt_oxferSyn.getXferName() << " possible!");
        string errorMessage="No conversion to transfer syntax"+string(opt_oxferSyn.getXferName())+"possible!";
        return getResultObject(env,errorMessage,ERROR);
    }

    //OFLOG_INFO(pdf2dcmLogger, "write converted DICOM file with metaheader");
    LOGIPDF("write converted DICOM file with metaheader");

    error = fileformat.saveFile(dcmOutPutFilePath, opt_oxfer, opt_oenctype, opt_oglenc, opt_opadenc,
                                OFstatic_cast(Uint32, opt_filepad), OFstatic_cast(Uint32, opt_itempad));

    if (error.bad())
    {
        //OFLOG_ERROR(pdf2dcmLogger, error.text() << ": writing file: " << opt_ofname);
        //return 1;
        string errorMessage="Error writing dicom file"+string(error.text());
        return getResultObject(env,errorMessage,ERROR);
    }

    //OFLOG_INFO(pdf2dcmLogger, "conversion successful");
    LOGIPDF("conversion successful pdf dicom");

    return getResultObject(env,"Success",SUCCESS);
}

static OFCondition createHeader(
        DcmItem *dataset,
        const char *opt_patientName,
        const char *opt_patientID,
        const char *opt_patientBirthdate,
        const char *opt_patientSex,
        OFBool opt_burnedInAnnotation,
        const char *opt_studyUID,
        const char *opt_seriesUID,
        const char *opt_documentTitle,
        const char *opt_conceptCSD,
        const char *opt_conceptCV,
        const char *opt_conceptCM,
        Sint32 opt_instanceNumber,
        const char *timeOffset,
        const char *accessionNumber,
        const char *studyDate,
        const char *physicianName,
        const char *studyinstantUID,
        const char *seriesinstantUID,
        const  char *sopInstanceUID)
{
    OFCondition result = EC_Normal;
    char buf[80];

    // insert empty type 2 attributes
    if (result.good()) result = dataset->putAndInsertString(DCM_StudyDate,studyDate,OFTrue);

    if (result.good()) result = dataset->putAndInsertString(DCM_AccessionNumber,accessionNumber,OFTrue);
    if (result.good()) result = dataset->insertEmptyElement(DCM_Manufacturer);

    //In our case both Refer Physical and RequestingPhysician will be same
    if (result.good()) result = dataset->putAndInsertString(DCM_RequestingPhysician,physicianName);
    if (result.good()) result = dataset->putAndInsertString(DCM_ReferringPhysicianName, physicianName);
    if (result.good()) result = dataset->putAndInsertString(DCM_StudyID,opt_studyUID,OFTrue);
    //Empty Content may create issue during DICOMDIR so making this encoding commented as of now
    /*if (result.good()) result = dataset->insertEmptyElement(DCM_ContentDate);
    if (result.good()) result = dataset->insertEmptyElement(DCM_ContentTime);
    if (result.good()) result = dataset->insertEmptyElement(DCM_AcquisitionDateTime);*/

    if (result.good() && opt_conceptCSD && opt_conceptCV && opt_conceptCM)
    {
        result = DcmCodec::insertCodeSequence(dataset, DCM_ConceptNameCodeSequence, opt_conceptCSD, opt_conceptCV, opt_conceptCM);
    }
    else
    {
        result = dataset->insertEmptyElement(DCM_ConceptNameCodeSequence);
    }

    // insert const value attributes
    if (result.good()) result = dataset->putAndInsertString(DCM_SpecificCharacterSet, "ISO_IR 100");
    if (result.good()) result = dataset->putAndInsertString(DCM_SOPClassUID,          UID_EncapsulatedPDFStorage);
    // we are now using "DOC" for the modality, which seems to be more appropriate than "OT" (see CP-749)
    if (result.good()) result = dataset->putAndInsertString(DCM_Modality,             "DOC");
    if (result.good()) result = dataset->putAndInsertString(DCM_ConversionType,       "WSD");
    if (result.good()) result = dataset->putAndInsertString(DCM_MIMETypeOfEncapsulatedDocument, "application/pdf");

    // there is no way we could determine a meaningful series number, so we just use a constant.
    if (result.good()) result = dataset->putAndInsertString(DCM_SeriesNumber,         "1");

    // insert variable value attributes
    if (result.good()) result = dataset->putAndInsertString(DCM_DocumentTitle,        opt_documentTitle,OFTrue);
    if (result.good()) result = dataset->putAndInsertString(DCM_PatientName,          opt_patientName,OFTrue);
    if (result.good()) result = dataset->putAndInsertString(DCM_PatientID,            opt_patientID,OFTrue);
    if (result.good()) result = dataset->putAndInsertString(DCM_PatientBirthDate,     opt_patientBirthdate,OFTrue);
    if (result.good()) result = dataset->putAndInsertString(DCM_PatientSex,           opt_patientSex,OFTrue);
    if (result.good()) result = dataset->putAndInsertString(DCM_BurnedInAnnotation,   opt_burnedInAnnotation ? "YES" : "NO");
    if (result.good())  result = dataset->putAndInsertString(DCM_Manufacturer,MANUFACTURE);
    if (result.good())  result = dataset->putAndInsertString(DCM_ManufacturerModelName,"KOSMOS");
    sprintf(buf, "%ld", OFstatic_cast(long, opt_instanceNumber));
    if (result.good()) result = dataset->putAndInsertString(DCM_InstanceNumber,       buf);

    dcmGenerateUniqueIdentifier(buf, SITE_INSTANCE_UID_ROOT);
    //Study
    //string studyInstance(getStudyInstanceExamUID(opt_studyUID));
    if (result.good()) result = dataset->putAndInsertString(DCM_StudyInstanceUID,     studyinstantUID);
    //string seriesInstance(getSeriesInstanceExamUID(opt_studyUID));
    if (result.good()) result = dataset->putAndInsertString(DCM_SeriesInstanceUID,    seriesinstantUID);
    if (result.good()) result = dataset->putAndInsertString(DCM_SOPInstanceUID,       sopInstanceUID);

    if (result.good()) result = dataset->putAndInsertString(DCM_TimezoneOffsetFromUTC,timeOffset,OFTrue);
    // set instance creation date and time
    OFString s;
    if (result.good()) result = DcmDate::getCurrentDate(s);
    if (result.good()) result = dataset->putAndInsertOFStringArray(DCM_InstanceCreationDate, s);
    if (result.good()) result = DcmTime::getCurrentTime(s);
    if (result.good()) result = dataset->putAndInsertOFStringArray(DCM_InstanceCreationTime, s);

    return result;
}

static OFCondition insertPDFFile(
        DcmItem *dataset,
        const char *filename)
{
    size_t fileSize = 0;
    struct stat fileStat;
    char buf[100];

    if (0 == stat(filename, &fileStat)) fileSize = OFstatic_cast(size_t, fileStat.st_size);
    else
    {
        //OFLOG_ERROR(pdf2dcmLogger, "file " << filename << " not found");
        LOGIPDF("File not found %s",filename);
        return EC_IllegalCall;
    }

    if (fileSize == 0)
    {
        //OFLOG_ERROR(pdf2dcmLogger, "file " << filename << " is empty");
        LOGIPDF("File is empty %s",filename);
        return EC_IllegalCall;
    }

    FILE *pdffile = fopen(filename, "rb");
    if (pdffile == nullptr)
    {
        //OFLOG_ERROR(pdf2dcmLogger, "unable to read file " << filename);
        LOGIPDF("unable to read file %s",filename);
        return EC_IllegalCall;
    }

    size_t buflen = 100;
    if (fileSize < buflen) buflen = fileSize;
    if (buflen != fread(buf, 1, buflen, pdffile))
    {
        //OFLOG_ERROR(pdf2dcmLogger, "read error in file " << filename);
        LOGIPDF("read error in file %s",filename);
        fclose(pdffile);
        return EC_IllegalCall;
    }

    // check magic word for PDF file
    if (0 != strncmp("%PDF-", buf, 5))
    {
        //OFLOG_ERROR(pdf2dcmLogger, "file " << filename << " is not a PDF file");
        LOGIPDF("Is not a pdf file %s",filename);
        fclose(pdffile);
        return EC_IllegalCall;
    }

    // check PDF version number
    char *version = buf + 5;
    OFBool found = OFFalse;
    for (int i = 0; i < 5; ++i)
    {
        if (version[i] == 10 || version[i] == 13)
        {
            version[i] = 0; // insert end of string
            found = OFTrue;
            break;
        }
    }

    if (! found)
    {
        //OFLOG_ERROR(pdf2dcmLogger, "file " << filename << ": unable to decode PDF version number");
        LOGIPDF("Unable to decode PDF version number %s",filename);
        fclose(pdffile);
        return EC_IllegalCall;
    }

    //OFLOG_INFO(pdf2dcmLogger, "file " << filename << ": PDF " << version << ", " << (fileSize + 1023) / 1024 << "kB");
    //LOGIPDF("File %s",filename,"PDF Version %s",version,"KB %d",(fileSize + 1023) / 1024);

    if (0 != fseek(pdffile, 0, SEEK_SET))
    {
        //OFLOG_ERROR(pdf2dcmLogger, "file " << filename << ": seek error");
       // LOGIPDF("file Seek Error ",filename);
        fclose(pdffile);
        return EC_IllegalCall;
    }

    OFCondition result = EC_Normal;
    DcmPolymorphOBOW *elem = new DcmPolymorphOBOW(DCM_EncapsulatedDocument);
    if (elem)
    {
        size_t numBytes = fileSize;
        if (numBytes & 1) ++numBytes;
        Uint8 *bytes = nullptr;
        result = elem->createUint8Array(OFstatic_cast(Uint32, numBytes), bytes);
        if (result.good())
        {
            // blank pad byte
            bytes[numBytes - 1] = 0;

            // read PDF content
            if (fileSize != fread(bytes, 1, fileSize, pdffile))
            {
                //OFLOG_ERROR(pdf2dcmLogger, "read error in file " << filename);
                LOGIPDF("read error in file pdf file %s ",filename);
                result = EC_IllegalCall;
            }
        }
    } else result = EC_MemoryExhausted;

    // if successful, insert element into dataset
    if (result.good()) result = dataset->insert(elem); else delete elem;

    // close file
    fclose(pdffile);

    return result;
}


