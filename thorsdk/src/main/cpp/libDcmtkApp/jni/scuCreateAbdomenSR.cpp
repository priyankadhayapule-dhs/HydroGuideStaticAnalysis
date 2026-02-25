//
// Created by himanshumistri on 24/01/22.
//

#include <jni.h>
#include <vector>
#include "dcmtk/dcmdata/dctk.h"
#include "include/dcmtkUtilJni.h"
#include "include/dcmtkClientJni.h"
#include "include/DicomSR.h"
#include <android/log.h>

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */
#include <dcmtk/dcmsr/dsrdoc.h>       /* for main interface class DSRDocument */
#include <dcmtk/dcmsr/codes/ucum.h>
#include <dcmtk/dcmdata/dctk.h>       /* for typical set of "dcmdata" headers */
#include <dcmtk/dcmsr/codes/dcm.h>
#include <dcmtk/dcmsr/codes/srt.h>
#include "dcmtk/dcmsr/dsrcodvl.h"

#include "jsoncpp/json/json.h"
#include <string>
#include <unordered_map>

#define CODE_US2_PECENT   DSRBasicCodedEntry("%", "UCUM", "Percent")
#define CODE_BMP DSRBasicCodedEntry("bpm", "UCUM", "bpm")
#define CODE_UCUM_cm2     DSRBasicCodedEntry("cm2", "UCUM", "cm2")
#define CODE_UCUM_ms     DSRBasicCodedEntry("m/s", "UCUM", "m/s")
#define CODE_UCUM_cm_per_sec     DSRBasicCodedEntry("cm/s", "UCUM", "cm/s")
#define CODE_UCUM_mis     DSRBasicCodedEntry("ms", "UCUM", "MilliSecond")

#define CODE_UCUM_LMIN DSRBasicCodedEntry("liter per minute", "UCUM", "L/min")
#define CODE_UCUM_G DSRBasicCodedEntry("g", "UCUM", "g")
#define CODE_UCUM_GM2 DSRBasicCodedEntry("g/m2", "UCUM", "g/m2")
#define CODE_UCUM_MLM2 DSRBasicCodedEntry("ml/m2", "UCUM", "ml/m2")
#define CODE_UCUM_m2     DSRBasicCodedEntry("m2", "UCUM", "m2")
#define CODE_UCUM_BPM DSRBasicCodedEntry("{H.B.}/min", "UCUM", "BPM")
//(ml/m2, UCUM, "ml/m2")
//(cm2, UCUM, "cm2")

#define CODE_IMAGING_MODE_PW DSRCodedEntryValue("R-409E4","SRT","Doppler Pulsed")
#define CODE_IMAGING_MODE_M DSRCodedEntryValue("G-0394","SRT","M mode")
#define US2_CODING_SCHEME_DESIGNATOR "Us2.ai"



#define ERROR 0
#define SUCCESS 1


#define LVEF "LVEF MOD biplane"
#define LVEDV "LVEDV MOD biplane"
#define LVESV "LVESV MOD biplane"
#define LVIDD "LVIDd"
#define LVIDS "LVIDs"
#define IVSD "IVSd"
#define LVPWd "LVPWd"
#define FS "FS"

#define COPROPER "Cardiac Output Properties"
#define LVSV "LVSV MOD biplane"
#define LV "Left Ventricle"
#define RV "Right Ventricle"
#define RVIDD "RVIDd (basal)"
#define LA "Left Atrium"
#define LAESV "LAESV MOD biplane"
#define RA "Right Atrium"
#define RAAREA "RA area A4C (s)"
#define MV "Mitral Valve"
#define MVE "MV-E"
#define MVA "MV-A"
#define EARATIO "E/A ratio"
#define DECT "DecT"
#define SOPUID "SopInstanceUID"
#define EPSS "EPSS"
#define LADIAM "LA diam"
#define AORTA "Aorta"
#define AORTICVALVE "Aortic Valve"
#define ASCAO "AscAo"
#define STJUN "STJuction"
#define ANNULUS "Annulus"
#define LVOTD "LVOTD"
#define LVOT_VTI "LVOTVTI"
#define LEFT_VENT_OTHER "Left Ventricle Other"
#define ELATERAl "E Lateral"
#define ALATERAl "A Lateral"
#define ESEPTAL "E Septal"
#define ASEPTAL "A Septal"
#define SLATERAL "S Lateral"
#define SSEPTAL "S Septal"
#define EELATERAL "EE Lateral"
#define EESEPTAL "EE Septal"
#define HR "HR"
#define TAPSE "TAPSE"
#define MAPSE "MAPSE"
#define IVC_MIN_M "IVC Min"
#define IVC_MAX_M "IVC Max"
#define RAP_M "RAP M"
#define CO "CO"
#define IVC_MIN_2D "IVC Min 2D"
#define IVC_MAX_2D "IVC Max 2D"
#define RAP_2D "RAP 2D"












using namespace std;

#define LOGSR(...) \
((void)__android_log_print(ANDROID_LOG_INFO, "dcmtk::", __VA_ARGS__))
/**
 * For the Calc Measurement Report
 * We have taken Finding detail from this link
 * https://dicom.nema.org/dicom/2013/output/chtml/part16/sect_EchocardiographyProcedureReportTemplates.html#sect_TID_5201
 *
 * @param env           Env available of the JNIEnv
 * @param thiz          Name of caller function
 * @param jsonReport    A valid JSON Report which is common for the US2 and Calc report
 * @param outputFilePath File path for the DICOM SR with .dcm file extension
 * @return
 */
JNIEXPORT jobject
DCMTKUTIL_METHOD(createAbdomenSr)(JNIEnv* env,jobject thiz,jstring jsonReport,jstring outputFilePath){


    //createSR(env,outputFilePath);
    return createAbdomenSrDocument(env,outputFilePath,jsonReport);
}

static jobject createAbdomenSrDocument(JNIEnv *env,jstring outputFilePath,jstring jsonReport){

    Json::Value root;
    //doc->createNewDocument(DSRTypes::DT_SimplifiedAdultEchoSR);
    auto dicomSr = new DicomSR();
    dicomSr->createDocument(DSRTypes::DT_ComprehensiveSR);
    auto *doc = dicomSr->getDocInstant();

    LOGSR("%s Empty document created",TAG);
    //OFString studyUID_01;
    jboolean isCopy;

    const char* outputFile=env->GetStringUTFChars(outputFilePath,&isCopy);





    // Convert JSON jString to native cpp string
    const char* reportJsonData = env->GetStringUTFChars(jsonReport, &isCopy);
    string mJsonString(reportJsonData);
    LOGSR("%s JSON SR is %s", TAG, mJsonString.c_str());

    std::stringstream jsonData{mJsonString};
    Json::CharReaderBuilder builder;
    std::string errs;

    // Patient Details
    string patientName;
    string birthday;
    string sex;
    string referringPhysician;
    string patientId;
    string studyId;
    string accessNumber;
    string timeZoneOffSet;
    string studyDate;
    string studyTime;
    string studyUID;
    string examBSA;
    string calReportComment;
    string alternativeId;
    string height;
    string weight;
    string systolicBloodPressure;
    string diastolicBloodPressure;
    string heartRate;

    // Device and Software details
    string deviceSerial;
    string softwareVersion;

    // Report details
    string instanceNumber;
    //Json::Value leftVentricle;

    string reportComment;

    // Ready JSON from the string
    bool isReportEmpty = mJsonString.empty();
    bool isPatientCharateristicAdded = false;

    if (!isReportEmpty) {
        bool parseSuccess =Json::parseFromStream(builder, jsonData, &root, &errs); //reader.parse(mJsonString, root, false);
        if (parseSuccess) {
            // Patient Name details
            if (root.isMember("name")) {
                patientName = root["name"].asString();
            }

            if(root.isMember("mrn")){
                patientId = root["mrn"].asString();
            }

            if(root.isMember("studyId")){
                studyId = root["studyId"].asString();
            }

            if(root.isMember("accessNumber")){
                accessNumber = root["accessNumber"].asString();
            }

            if(root.isMember("timeZoneOffSet")){
                timeZoneOffSet = root["timeZoneOffSet"].asString();
            }

            if(root.isMember("studyDate")){
                studyDate = root["studyDate"].asString();
            }

            if(root.isMember("studyTime")){
                studyTime = root["studyTime"].asString();
            }

            if(root.isMember("studyUID")){
                studyUID = root["studyUID"].asString();
            }

            if (root.isMember("birthday")) {
                birthday = root["birthday"].asString();
            }
            if (root.isMember("sex")) {
                sex = root["sex"].asString();
            }
            if (root.isMember("referringPhysician")) {
                referringPhysician = root["referringPhysician"].asString();
            }

            // Device and Software details
            if (root.isMember("deviceSerialNumber")) {
                deviceSerial = root["deviceSerialNumber"].asString();
            }
            if (root.isMember("softwareVersion")) {
                softwareVersion = root["softwareVersion"].asString();
            }

            if (root.isMember("BSA")) {
                examBSA = root["BSA"].asString();
            }

            if(root.isMember("calReportComment")) {
                calReportComment = root["calReportComment"].asString();
            }

            if(root.isMember("alternativeId")) {
                alternativeId = root["alternativeId"].asString();
            }
            LOGSR("%s Result is %s", TAG,deviceSerial.c_str());

            if(root.isMember("instanceNumber")) {
                instanceNumber = root["instanceNumber"].asString();
            }

            if(root.isMember("Height")) {
                height = root["Height"].asString();
            }

            if(root.isMember("Weight")) {
                weight = root["Weight"].asString();
            }

            if (root.isMember("sysBloodPress")) {
                systolicBloodPressure = root["sysBloodPress"].asString();
            }
            LOGSR("%s Sys Blood Pressure %s", TAG, systolicBloodPressure.c_str());

            if (root.isMember("diaBloodPress")) {
                diastolicBloodPressure = root["diaBloodPress"].asString();
            }
            LOGSR("%s Dia Blood Pressure %s", TAG, diastolicBloodPressure.c_str());

            if (root.isMember("heartRate")) {
                heartRate = root["heartRate"].asString();
            }

            if (root.isMember("Comment")) {
                reportComment = root["Comment"].asString();
            }
        }
    }


    OFCondition mResult;

    //if (!studyUID_01.empty()){
    mResult =dicomSr->getDocInstant()->createNewSeriesInStudy(studyUID);//setcreateNewSeriesInStudy(studyUID_01);
        LOGSR("%s New Series Created Result %s",TAG,mResult.text());
    //}

    if (!weight.empty()) {
        dicomSr->getDocInstant()->setPatientWeight(weight);
    }

    if (!height.empty()) {
        dicomSr->getDocInstant()->setPatientSize(height);
    }

    if(!accessNumber.empty()){
        dicomSr->getDocInstant()->setAccessionNumber(accessNumber);
    }

    if(!timeZoneOffSet.empty()){
        dicomSr->getDocInstant()->setTimezoneOffsetFromUTC(timeZoneOffSet);
    }

    if(!studyDate.empty()){
        dicomSr->getDocInstant()->setStudyDate(studyDate);
    }

    if(!studyTime.empty()){
        dicomSr->getDocInstant()->setStudyTime(studyTime);
    }

    dicomSr->getDocInstant()->setStudyID(studyId);
    dicomSr->getDocInstant()->setStudyDescription("Vascular Ultrasound Report");
    dicomSr->getDocInstant()->setSeriesDescription("Vascular Ultrasound Report with measurements");
    dicomSr->getDocInstant()->setManufacturer(MANUFACTURE);
    dicomSr->getDocInstant()->setManufacturerModelName("Torso");
    dicomSr->getDocInstant()->setDeviceSerialNumber(deviceSerial);
    dicomSr->getDocInstant()->setSoftwareVersions(softwareVersion);
    dicomSr->getDocInstant()->setSeriesNumber("2", OFTrue);
    dicomSr->getDocInstant()->setInstanceNumber(instanceNumber, OFTrue);


    if(!patientName.empty()){
        doc->setPatientName(patientName);
    }

    if(!birthday.empty()){
        doc->setPatientBirthDate(birthday);
    }

    if(!sex.empty()){
        doc->setPatientSex(sex);
    }

    if(!referringPhysician.empty()){
        doc->setReferringPhysicianName(referringPhysician);
    }

    if(!patientId.empty()){
        doc->setPatientID(patientId);
    }

    dicomSr->setRootContainer(CODE_DCM_RETIRED_VascularUltrasoundProcedureReport);
    mResult = dicomSr->getDocInstant()->getTree().setTemplateIdentification("5100","DCMR");
    LOGSR("%s setTemplateIdentification Result %s",TAG_VAS,mResult.text());

    if(!examBSA.empty()){
        dicomSr->addFindingSitePatient();
        isPatientCharateristicAdded = true;
        const auto lvefCodeValue = DSRCodedEntryValue("8277-6", "LN", "Body Surface Area");
        const auto lvefNumValue = DSRNumericMeasurementValue(examBSA, CODE_UCUM_m2);
        dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent,lvefCodeValue,lvefNumValue);
        doc->getTree().goUp();
        doc->getTree().goUp();
    }

    if(!heartRate.empty()){
        if(!isPatientCharateristicAdded){
            dicomSr->addFindingSitePatient();
            isPatientCharateristicAdded = true;
        }
        const auto heartRateCodeValue = DSRCodedEntryValue("8867-4", "LN", "Heart Rate");
        const auto heartRateNumValue = DSRNumericMeasurementValue(heartRate, CODE_UCUM_BPM);
        dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent,heartRateCodeValue,heartRateNumValue);
        doc->getTree().goUp();
    }
    if(!systolicBloodPressure.empty()){
        if(!isPatientCharateristicAdded){
            dicomSr->addFindingSitePatient();
            isPatientCharateristicAdded = true;
        }
        const auto systolicCodeValue = DSRCodedEntryValue("271649006", "SCT", "Systolic Blood Pressure");
        const auto systolicNumValue = DSRNumericMeasurementValue(systolicBloodPressure, CODE_UCUM_MMHG);
        dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent,systolicCodeValue,systolicNumValue);
        doc->getTree().goUp();
    }

    if(!diastolicBloodPressure.empty()){
        if(!isPatientCharateristicAdded){
            dicomSr->addFindingSitePatient();
            isPatientCharateristicAdded = true;
        }
        const auto diastolicCodeValue = DSRCodedEntryValue("271650006", "SCT", "Diastolic Blood Pressure");
        const auto diastolicNumValue = DSRNumericMeasurementValue(diastolicBloodPressure, CODE_UCUM_MMHG);
        dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent,diastolicCodeValue,diastolicNumValue);
        doc->getTree().goUp();
    }

    if(!reportComment.empty()){
        if(!isPatientCharateristicAdded){
            dicomSr->addFindingSitePatient();
            isPatientCharateristicAdded = true;
        }
        const auto commentCodeValue = DSRCodedEntryValue("121106", "DCM", "Comment");
        dicomSr->addText(DSRTypes::AM_afterCurrent, commentCodeValue, reportComment);
        doc->getTree().goUp();
    }

    if(isPatientCharateristicAdded){
        doc->getTree().goUp();
    }

    auto isLVRootSet = false;
    auto isRootSet = false;
    LOGSR("%s Root is not null",TAG);

    const auto proxTopographicalValue = DSRCodedEntryValue("G-A118", "SRT", "Proximal");
    const auto midTopographicalValue = DSRCodedEntryValue("G-A188", "SRT", "Mid-longitudinal");
    const auto distTopographicalValue = DSRCodedEntryValue("G-A119", "SRT", "Distal");
    const auto lateracityRight = DSRCodedEntryValue("G-A100", "SRT", "Right");
    const auto lateracityLeft = DSRCodedEntryValue("G-A101", "SRT", "Left");
    const auto unilateral= DSRCodedEntryValue("G-A103", "SRT", "Unilateral");


    auto findingSite = DSRCodedEntryValue("T-46002", "SRT", "Artery of Abdomen");

    doc->getTree().addContentItem(DSRTypes::RT_contains,DSRTypes::VT_Container, DSRTypes::AM_belowCurrent);
    doc->getTree().getCurrentContentItem().setConceptName(CODE_DCM_RETIRED_Findings);
    doc->getTree().getCurrentContentItem().setContinuityOfContent(DSRTypes::COC_Separate);

    doc->getTree().addContentItem(DSRTypes::RT_hasConceptMod,  DSRTypes::VT_Code,DSRTypes::AM_belowCurrent);
    doc->getTree().getCurrentContentItem().setConceptName(CODE_SRT_FindingSite);
    doc->getTree().getCurrentContentItem().setCodeValue(findingSite);
    doc->getTree().getCurrentContentItem().setContinuityOfContent(DSRTypes::COC_Separate);


    doc->getTree().addContentItem(DSRTypes::RT_hasConceptMod,DSRTypes::VT_Code,DSRTypes::AM_afterCurrent);
    doc->getTree().getCurrentContentItem().setConceptName(CODE_SRT_Laterality);
    doc->getTree().getCurrentContentItem().setCodeValue(unilateral);

    bool isAortaAdded;
    if(root.isMember(Aorta)){
        isAortaAdded = true;
        const auto& aortaJson = root[Aorta];
        bool  isAdded = false;
        for (int i = 0; i < aortaJson.size(); i++) {
            auto jsonData = aortaJson[i];

            if(jsonData.isMember(ProxAortaKey)){
                dicomSr->addAbdomen(jsonData[ProxAortaKey],AbdomenLabelKey::ProxAorta,&proxTopographicalValue,
                                    nullptr,isAdded,false);
                isAdded = true;
            }

            if(jsonData.isMember(MidAortaKey)){
                dicomSr->addAbdomen(jsonData[MidAortaKey],AbdomenLabelKey::MidAorta,&midTopographicalValue,nullptr,isAdded,false);
                isAdded = true;
            }

            if(jsonData.isMember(DistalAortaKey)){
                dicomSr->addAbdomen(jsonData[DistalAortaKey],AbdomenLabelKey::DistalAorta,&distTopographicalValue,nullptr,isAdded,false);
                isAdded = true;
            }

        }
        //

    }

    if(root.isMember(CIA)) {

        //doc->getTree().goUp();

        const auto &ciaJson = root[CIA];
        bool isAdded = false;
        for (int i = 0; i < ciaJson.size(); i++) {
            auto jsonCIAData = ciaJson[i];
            //LTCIA

            if(jsonCIAData.isMember(LTCIAKey)){
                dicomSr->addAbdomen(jsonCIAData[LTCIAKey],AbdomenLabelKey::LTCIA, nullptr,
                                    &lateracityLeft,isAdded,true);
                isAdded = true;
            }

            //RTCIA
            if(jsonCIAData.isMember(RTCIAKey)){
                dicomSr->addAbdomen(jsonCIAData[RTCIAKey],AbdomenLabelKey::RTCIA, nullptr,
                                    &lateracityRight,isAdded,true);
                isAdded = true;
            }
        }
    }

   // if ([root objectForKey:AbdomenLabelKeyName(Aneurysm)]) {
   if(root.isMember(AneurysmKey)){
       //doc->getTree().goUp();
        //Json Array
        auto dataArrayJson = root[AneurysmKey];
        dicomSr->addAbdomen(dataArrayJson,AbdomenLabelKey::Aneurysm, nullptr,
                           nullptr,false,false);
   }


    auto *fileformat = new DcmFileFormat();
    DcmDataset *dataset = nullptr;
    if (fileformat != nullptr){
        dataset = fileformat->getDataset();
    }

    if(!calReportComment.empty())
    {
        OFCondition mResultComment= dataset->putAndInsertString(DCM_PatientComments,calReportComment.c_str(),OFTrue);
        LOGSR("%s Patient Comment Status %s",TAG,mResultComment.text());
    }

    //Register private tag if not present
    if(!string(alternativeId).empty()) {
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
            dataset->putAndInsertString(PRV_PrivateElement1, alternativeId.c_str());
        }
    }

    jobject resultObject;

    OFCondition result=doc->write(*dataset);
    if (result.good()){
        OFCondition fileResult = fileformat->saveFile(outputFile, EXS_LittleEndianExplicit);
        if (fileResult.bad()) {
            string errorMessage="Unable to create DICOM SR file Error is"+string(fileResult.text());
            resultObject=getResultObject(env,errorMessage,ERROR);
        } else {
            resultObject=getResultObject(env,"Success",SUCCESS);
        }
    }else{
        LOGSR("%s Error in DICOM SR dataset %s",TAG ,result.text());
        string errorMessage = "Error in DICOM SR dataset "+string (result.text());
        resultObject=getResultObject(env,errorMessage,ERROR);
    }
    //OFFIS_CODING_SCHEME_DESIGNATOR

    return resultObject;
}





/**
 * Fetch the co-ordinates from Json
 * @param coordinates
 */
static void fetchCoordinate(Json::Value coordinates) {
    if (coordinates.isMember("coordinates")) {
        Json::Value coordinateMembers = coordinates["coordinates"];
        LOGSR("\nUS2DicomSR %d", coordinateMembers.size());
        for (Json::Value::ArrayIndex index = 0; index != coordinateMembers.size(); index++) {
            string xValue = coordinateMembers[index]["x"].asString();
            string yValue = coordinateMembers[index]["y"].asString();
            LOGSR("\nUS2DicomSR X: %s and Y: %s", xValue.c_str(), yValue.c_str());
        }
    }
}
