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
#define CODE_UCUM_MMHG DSRBasicCodedEntry("mm[Hg]", "UCUM", "mmHg")
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

#define SINUS "Sinus"
#define AVVC "AV VC"
#define ARPHT "AR PHT"
#define AVVTI "AV VTI"
#define RV_BASAL "RV Basal"
#define RV_MID "RV Mid"
#define RV_LENGTH "RV Length"
#define PVACT "PV AcT"
#define MVABYPHT "MVA BY PHT"
#define PAEDP "PAEDP"
#define MVPHT "MVPHT"
#define TR "TR"
#define LVMASS "LVMASS"
#define LVMASSINDEX "LVMASSINDEX"
#define RWT "RWT"
#define STROKEVOLINDEX "Stroke Vol Index"
#define TVS "TVS"
#define TRPG "TRPG"
#define DI "DI"
#define AVA "AVA"
#define MVSV "MVSV"
#define MVVTI "MVVTI"
#define MVVTIPW "MVVTI_PW"
#define MVVC "MVVC"
#define MVAD "MVAD"
#define PR "PR Vmax"
#define MVABYCONTEQA "MVABYCONTEQA"
#define MVVTIPW_VMAX "MVVTI_PW_VMAX"
#define MVVTIPW_VMIN "MVVTI_PW_VMIN"
#define MVVTIPW_MAXPG "MVVTI_PW_MAXPG"
#define MVVTIPW_MEANPG "MVVTI_PW_MEANPG"

#define MVVTICW_VMAX "MVVTI_CW_VMAX"
#define MVVTICW_VMIN "MVVTI_CW_VMIN"
#define MVVTICW_MAXPG "MVVTI_CW_MAXPG"
#define MVVTICW_MEANPG "MVVTI_CW_MEANPG"

#define RVSP "RVSP"

#define LVOT_VTI_VMAX "LVOT_VTI_VMAX"
#define LVOT_VTI_VMIN "LVOT_VTI_VMIN"
#define LVOT_VTI_MAXPG "LVOT_VTI_MAXPG"
#define LVOT_VTI_MEANPG "LVOT_VTI_MEANPG"

#define AV_VTI_VMAX "AV_VTI_VMAX"
#define AV_VTI_VMIN "AV_VTI_VMIN"
#define AV_VTI_MAXPG "AV_VTI_MAXPG"
#define AV_VTI_MEANPG "AV_VTI_MEANPG"
#define AV_PK_VEL "AV_PK_VEL"
#define AVPG "AVPG"

#define VCAVA "Vena Cava"

#define MM_RVIDD "MM RVIDD"
#define MM_IVS "MM IVS"
#define MM_LVIDD "MM LVIDD"
#define MM_LVPW "MM LVPW"
#define MM_LVIDS "MM LVIDS"
#define MM_AO "MM AO"
#define MM_LA "MM LA"

#define MM_EF "MM EF"
#define MM_FS "MM FS"
#define MM_LV_MASS "MM LVMASS"
#define MM_LV_MASS_INDEX "MM LVMASS INDEX"
#define MM_RWT "MM RWT"
#define MM_LA_AO "MM LA:AO"
#define SelectionStatus "selectionStatus"
#define Augmentation "Augmentation"
#define Compression "Compression"
#define Present "Present"
#define Absent "Absent"
#define Reflux "Reflux"










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
DCMTKUTIL_METHOD(createVascularSr)(JNIEnv* env,jobject thiz,jstring jsonReport,jstring outputFilePath){


    //createSR(env,outputFilePath);
    return createVascularSrDocument(env,outputFilePath,jsonReport);
}

static jobject createVascularSrDocument(JNIEnv *env,jstring outputFilePath,jstring jsonReport){

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

    if(isPatientCharateristicAdded){
        doc->getTree().goUp();
    }

    auto isLVRootSet = false;
    auto isRootSet = false;
    LOGSR("%s Root is not null",TAG);
    auto codeCCA = DSRCodedEntryValue("T-45100", "SRT", "Common Carotid Artery");
    const auto proxTopographicalValue = DSRCodedEntryValue("G-A118", "SRT", "Proximal");
    const auto midTopographicalValue = DSRCodedEntryValue("G-A188", "SRT", "Mid-longitudinal");
    const auto distTopographicalValue = DSRCodedEntryValue("G-A119", "SRT", "Distal");
    const auto lateracityRight = DSRCodedEntryValue("G-A100", "SRT", "Right");
    const auto lateracityLeft = DSRCodedEntryValue("G-A101", "SRT", "Left");

    bool  isCarotidExist = root.isMember(Carotid);
    if(isCarotidExist){
        const auto& cartiodJson = root[Carotid];
        cout << "cartiodJson " << "Found"<< endl;
        LOGSR("CartiodJson Found");
        //RT CCA DIST Right CCA Dist
        bool isRightDistExist = cartiodJson.isMember(LabelName(RTCCADIST));




        cout << "isRightDistExist " << isRightDistExist<< endl;
        LOGSR("isRightDistExist Found %d",isRightDistExist);

        doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Container, DSRTypes::AM_belowCurrent);
        doc->getTree().getCurrentContentItem().setConceptName(CODE_DCM_RETIRED_Findings);
        doc->getTree().getCurrentContentItem().setContinuityOfContent(DSRTypes::COC_Separate);

        // Finidng Site Name
        doc->getTree().addContentItem(DSRTypes::RT_hasConceptMod,  DSRTypes::VT_Code,DSRTypes::AM_belowCurrent);
        doc->getTree().getCurrentContentItem().setConceptName(CODE_SRT_FindingSite);
        doc->getTree().getCurrentContentItem().setCodeValue(DSRCodedEntryValue("T-45005", "SRT", "Artery of neck"));
        doc->getTree().getCurrentContentItem().setContinuityOfContent(DSRTypes::COC_Separate);

        bool isAdded = false;
        bool isRightAdded = false;

        bool isRightCCAProxExist = cartiodJson.isMember(LabelName(RTCCAPROX));
        if(isRightCCAProxExist) {
            if(isRightAdded){
                dicomSr->addVascularMeasurementContainer(cartiodJson,RTCCAPROX, nullptr,
                                                         &proxTopographicalValue,isAdded);
            }else{
                dicomSr->addVascularMeasurementContainer(cartiodJson,RTCCAPROX, &lateracityRight,
                                                         &proxTopographicalValue,isAdded);
            }
            isRightAdded = true;
            isAdded = true;
        }

        bool isRightCCAMidExist = cartiodJson.isMember(LabelName(RTCCAMID));
        if(isRightCCAMidExist) {

            if(isRightAdded){
                dicomSr->addVascularMeasurementContainer(cartiodJson,RTCCAMID, nullptr,
                                                         &midTopographicalValue,isAdded);
            }else{
                dicomSr->addVascularMeasurementContainer(cartiodJson,RTCCAMID, &lateracityRight,
                                                         &midTopographicalValue,isAdded);
            }
            isRightAdded = true;
            isAdded = true;
        }

        if(isRightDistExist){
            if(isRightAdded){
                dicomSr->addVascularMeasurementContainer(cartiodJson,RTCCADIST, nullptr,
                                                         &distTopographicalValue,isAdded);
            }else{
                dicomSr->addVascularMeasurementContainer(cartiodJson,RTCCADIST, &lateracityRight,
                                                         &distTopographicalValue,isAdded);
            }
            isAdded = true;
            isRightAdded = true;
        }

        //RTECA
        bool isRightECA = cartiodJson.isMember(LabelName(RTECA));
        LOGSR("isRightECA Found %d",isRightECA);
        if(isRightECA){
            if(isRightAdded){
                dicomSr->addVascularMeasurementContainer(cartiodJson,RTECA, nullptr,
                                                         nullptr,isAdded);
            }else{
                dicomSr->addVascularMeasurementContainer(cartiodJson,RTECA, &lateracityRight,
                                                         nullptr,isAdded);
            }
            isAdded = true;
            isRightAdded = true;
        }

        //RTICAPROX
        bool isRightICAProxExist = cartiodJson.isMember(LabelName(RTICAPROX));
        LOGSR("isRightICAProxExist Found %d",isRightICAProxExist);
        if(isRightICAProxExist){
            if(isRightAdded){
                dicomSr->addVascularMeasurementContainer(cartiodJson,RTICAPROX, nullptr,
                                                         &proxTopographicalValue,isAdded);
            }else{
                dicomSr->addVascularMeasurementContainer(cartiodJson,RTICAPROX, &lateracityRight,
                                                         &proxTopographicalValue,isAdded);
            }
            isRightAdded = true;
            isAdded = true;
        }
        //RTICAMID
        bool isRightICAMidExist = cartiodJson.isMember(LabelName(RTICAMID));
        LOGSR("isRightICAMidExist Found %d",isRightICAMidExist);
        if(isRightICAMidExist){
            if(isRightAdded){
                dicomSr->addVascularMeasurementContainer(cartiodJson,RTICAMID, nullptr,
                                                         &midTopographicalValue,isAdded);
            }else{
                dicomSr->addVascularMeasurementContainer(cartiodJson,RTICAMID, &lateracityRight,
                                                         &midTopographicalValue,isAdded);
            }
            isRightAdded = true;
            isAdded = true;
        }

        bool isRightICADistExist = cartiodJson.isMember(LabelName(RTICADIST));
        LOGSR("isRightICADistExist Found %d",isRightICADistExist);
        if(isRightICADistExist){
            if(isRightAdded){
                dicomSr->addVascularMeasurementContainer(cartiodJson,RTICADIST, nullptr,
                                                         &distTopographicalValue,isAdded);
            }else{
                dicomSr->addVascularMeasurementContainer(cartiodJson,RTICADIST, &lateracityRight,
                                                         &distTopographicalValue,isAdded);
            }
            isRightAdded = true;
            isAdded = true;
        }

        //RTBULB

        bool isRightBulbExist = cartiodJson.isMember(LabelName(RTBULB));
        LOGSR("isRightICADistExist Found %d",isRightBulbExist);
        if(isRightBulbExist){
            if(isRightAdded){
                dicomSr->addVascularMeasurementContainer(cartiodJson,RTBULB, nullptr,
                                                         nullptr,isAdded);
            }else{
                dicomSr->addVascularMeasurementContainer(cartiodJson,RTBULB, &lateracityRight,
                                                         nullptr,isAdded);
            }
            isRightAdded = true;
            isAdded = true;
        }

        // RTVERT
        bool isRightVertExist = cartiodJson.isMember(LabelName(RTVERT));
        LOGSR("isRightVertExist Found %d",isRightVertExist);
        if(isRightVertExist){
            if(isRightAdded){
                dicomSr->addVascularMeasurementContainer(cartiodJson,RTVERT, nullptr,
                                                         nullptr,isAdded);
            }else{
                dicomSr->addVascularMeasurementContainer(cartiodJson,RTVERT, &lateracityRight,
                                                         nullptr,isAdded);
            }
            isRightAdded = true;
            isAdded = true;
        }

        //RTSUBCL
        bool isRightSubClExist = cartiodJson.isMember(LabelName(RTSUBCL));
        LOGSR("isRightSubClExist Found %d",isRightSubClExist);
        if(isRightSubClExist){
            if(isRightAdded){
                dicomSr->addVascularMeasurementContainer(cartiodJson,RTSUBCL, nullptr,
                                                         nullptr,isAdded);
            }else{
                dicomSr->addVascularMeasurementContainer(cartiodJson,RTSUBCL, &lateracityRight,
                                                         nullptr,isAdded);
            }
            isRightAdded = true;
            isAdded = true;
        }

        //Vel Ratio

        bool isRatioExist = cartiodJson.isMember(RTVelRatio);
        LOGSR("isRatioExist Found %d",isRatioExist);
        if(isRatioExist){
            LOGSR("Found RTVelRatio");
            auto valueRatioRight = cartiodJson[RTVelRatio]["value"].asString();
            LOGSR("Found RTVelRatio value %s",valueRatioRight.c_str());
            dicomSr->addVelRatio(valueRatioRight);
        }

        bool  isRtBrachialSys = cartiodJson.isMember(RT_BRACHIAL_SYSTOLIC);
        if(isRtBrachialSys && isRightAdded){
            LOGSR("Found BrachialSys ");
            auto valueRatioRight = cartiodJson[RT_BRACHIAL_SYSTOLIC]["value"].asString();
            LOGSR("Found BrachialSys  value %s",valueRatioRight.c_str());
            dicomSr->addBranchialData(valueRatioRight,true);
        }

        bool  isRtBrachialDia = cartiodJson.isMember(RT_BRACHIAL_DIASTOLIC);
        if(isRtBrachialDia && isRightAdded){
            LOGSR("Found BrachialDia ");
            auto valueRatioRight = cartiodJson[RT_BRACHIAL_DIASTOLIC]["value"].asString();
            LOGSR("Found BrachialDia  value %s",valueRatioRight.c_str());
            dicomSr->addBranchialData(valueRatioRight,false);
        }


        bool isLeftAdded = false;

        bool  isLeftDataForCarotid =dicomSr->isLeftDataForCarotid(cartiodJson);

        if(isLeftDataForCarotid){
            doc->getTree().goUp();
            //doc->getTree().goUp();

            doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Container, DSRTypes::AM_belowCurrent);
            doc->getTree().getCurrentContentItem().setConceptName(CODE_DCM_RETIRED_Findings);
            doc->getTree().getCurrentContentItem().setContinuityOfContent(DSRTypes::COC_Separate);

            // Finidng Site Name
            doc->getTree().addContentItem(DSRTypes::RT_hasConceptMod,  DSRTypes::VT_Code,DSRTypes::AM_belowCurrent);
            doc->getTree().getCurrentContentItem().setConceptName(CODE_SRT_FindingSite);
            doc->getTree().getCurrentContentItem().setCodeValue(DSRCodedEntryValue("T-45005", "SRT", "Artery of neck"));
            doc->getTree().getCurrentContentItem().setContinuityOfContent(DSRTypes::COC_Separate);

            doc->getTree().addContentItem(DSRTypes::RT_hasConceptMod,DSRTypes::VT_Code,DSRTypes::AM_afterCurrent);
            doc->getTree().getCurrentContentItem().setConceptName(CODE_SRT_Laterality);
            doc->getTree().getCurrentContentItem().setCodeValue(lateracityLeft);
        }

        // Left Carotid Measurements

        bool isLeftCCAProxExist = cartiodJson.isMember(LabelName(LTCCAPROX));
        if(isLeftCCAProxExist) {
            dicomSr->addVascularMeasurementContainer(cartiodJson,LTCCAPROX, nullptr,
                                                         &proxTopographicalValue,isLeftAdded);
            isLeftAdded = true;
        }

        bool isLeftCCAMidExist = cartiodJson.isMember(LabelName(LTCCAMID));
        if(isLeftCCAMidExist) {

            dicomSr->addVascularMeasurementContainer(cartiodJson,LTCCAMID, nullptr,
                                                         &midTopographicalValue,isLeftAdded);
            isLeftAdded = true;

        }
        bool isLeftDistExist = cartiodJson.isMember(LabelName(LTCCADIST));

        if(isLeftDistExist){
            dicomSr->addVascularMeasurementContainer(cartiodJson,LTCCADIST, nullptr,
                                                         &distTopographicalValue,isLeftAdded);
            isLeftAdded = true;
        }

        //LTECA
        bool isLeftECAExist = cartiodJson.isMember(LabelName(LTECA));

        if(isLeftECAExist){
            dicomSr->addVascularMeasurementContainer(cartiodJson,LTECA, nullptr,
                                                     nullptr,isLeftAdded);
            isLeftAdded = true;
        }

        //LTICAPROX
        bool  isLeftICAProx = cartiodJson.isMember(LabelName(LTICAPROX));
        if(isLeftICAProx) {
            dicomSr->addVascularMeasurementContainer(cartiodJson,LTICAPROX, nullptr,
                                                     &proxTopographicalValue,isLeftAdded);
            isLeftAdded = true;
        }

        //LTICAMID
        bool  isLeftICAMid = cartiodJson.isMember(LabelName(LTICAMID));
        if(isLeftICAMid) {
            dicomSr->addVascularMeasurementContainer(cartiodJson,LTICAMID, nullptr,
                                                     &midTopographicalValue,isLeftAdded);
            isLeftAdded = true;
        }

        //LTICADIST
        bool  isLeftICADist = cartiodJson.isMember(LabelName(LTICADIST));
        if(isLeftICADist) {
            dicomSr->addVascularMeasurementContainer(cartiodJson,LTICADIST, nullptr,
                                                     &distTopographicalValue,isLeftAdded);
            isLeftAdded = true;
        }

        //LTBULB
        bool isLeftBulbExist = cartiodJson.isMember(LabelName(LTBULB));
        LOGSR("isLeftBulbExist Found %d",isLeftBulbExist);
        if(isLeftBulbExist){

            dicomSr->addVascularMeasurementContainer(cartiodJson,LTBULB, nullptr,
                                                         nullptr,isLeftAdded);

            isLeftAdded = true;

        }

        // LTVERT
        bool isLeftVertExist = cartiodJson.isMember(LabelName(LTVERT));
        LOGSR("isLeftVertExist Found %d",isLeftVertExist);
        if(isLeftVertExist){
            dicomSr->addVascularMeasurementContainer(cartiodJson,LTVERT, nullptr,
                                                     nullptr,isLeftAdded);

            isLeftAdded = true;
        }

        //LTSUBCL
        bool isLeftSubClExist = cartiodJson.isMember(LabelName(LTSUBCL));
        LOGSR("isLeftSubClExist Found %d",isLeftSubClExist);
        if(isLeftSubClExist){
            dicomSr->addVascularMeasurementContainer(cartiodJson,LTSUBCL, nullptr,
                                                     nullptr,isLeftAdded);

            isLeftAdded = true;
        }

        bool isLeftRatioExist = cartiodJson.isMember(LTVelRatio);
        if(isLeftRatioExist){
            // psvObject["value"].asString();
            auto valueRatio = cartiodJson[LTVelRatio]["value"].asString();
            dicomSr->addVelRatio(valueRatio);
        }

        bool  isLtBrachialSys = cartiodJson.isMember(LT_BRACHIAL_SYSTOLIC);
        if(isLtBrachialSys && isLeftAdded){
            LOGSR("Found Left BrachialSys ");
            auto valueBrachialSys = cartiodJson[LT_BRACHIAL_SYSTOLIC]["value"].asString();
            LOGSR("Found Left BrachialSys  value %s",valueBrachialSys.c_str());
            dicomSr->addBranchialData(valueBrachialSys,true);
        }

        bool  isLtBrachialDia = cartiodJson.isMember(LT_BRACHIAL_DIASTOLIC);
        if(isLtBrachialDia && isLeftAdded){
            LOGSR("Found Left BrachialDia ");
            auto valueBrachialDia = cartiodJson[LT_BRACHIAL_DIASTOLIC]["value"].asString();
            LOGSR("Found Left BrachialDia  value %s",valueBrachialDia.c_str());
            dicomSr->addBranchialData(valueBrachialDia,false);
        }


        if(cartiodJson.isMember("Comment") && (isLeftDataForCarotid || isRightAdded)){
            DSRCodedEntryValue code = DSRCodedEntryValue("121106", "DCM", "Comment");
            string comment = cartiodJson["Comment"].asString();
            dicomSr->addText(DSRTypes::AM_belowCurrent, code, comment);
            doc->getTree().goUp();
        }


    }


    if(root.isMember(LEArterial)){
        doc->getTree().goUp();
        const auto& leArterialJson = root[LEArterial];
        LOGSR("LEArterial Found");
        auto findingSite = DSRCodedEntryValue("T-47040", "SRT", "Artery of Lower Extremity");

        doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Container, DSRTypes::AM_belowCurrent);
        doc->getTree().getCurrentContentItem().setConceptName(CODE_DCM_RETIRED_Findings);
        doc->getTree().getCurrentContentItem().setContinuityOfContent(DSRTypes::COC_Separate);

        // Finidng Site Name
        doc->getTree().addContentItem(DSRTypes::RT_hasConceptMod,  DSRTypes::VT_Code,DSRTypes::AM_belowCurrent);
        doc->getTree().getCurrentContentItem().setConceptName(CODE_SRT_FindingSite);
        doc->getTree().getCurrentContentItem().setCodeValue(findingSite);
        doc->getTree().getCurrentContentItem().setContinuityOfContent(DSRTypes::COC_Separate);

        bool isRightArterial = dicomSr->isRightDataForLowerArterial(leArterialJson);

        if(isRightArterial){
            doc->getTree().addContentItem(DSRTypes::RT_hasConceptMod,DSRTypes::VT_Code,DSRTypes::AM_afterCurrent);
            doc->getTree().getCurrentContentItem().setConceptName(CODE_SRT_Laterality);
            doc->getTree().getCurrentContentItem().setCodeValue(lateracityRight);
        }

        //RTCFA
        bool isRightAdded = false;
        bool isRightCFAExist = leArterialJson.isMember(LabelName(RTCFA));
        LOGSR("isRightCFAExist Found %d",isRightCFAExist);
        if(isRightCFAExist){
            dicomSr->addVascularMeasurementContainer(leArterialJson,RTCFA, nullptr,
                                                     nullptr,isRightAdded);
            isRightAdded = true;
        }


        bool isRightProFaExist = leArterialJson.isMember(LabelName(RTPROFA));
        LOGSR("isRightProFaExist Found %d",isRightProFaExist);
        if(isRightProFaExist){
            dicomSr->addVascularMeasurementContainer(leArterialJson,RTPROFA, nullptr,
                                                     nullptr,isRightAdded);
            isRightAdded = true;
        }

        bool  isRightFAProxExist = leArterialJson.isMember(LabelName(RTFAPROX));
        if(isRightFAProxExist) {
            dicomSr->addVascularMeasurementContainer(leArterialJson,RTFAPROX, nullptr,
                                                     &proxTopographicalValue,isRightAdded);
            isRightAdded = true;
        }

        bool  isRightFAMidExist = leArterialJson.isMember(LabelName(RTFAMID));
        if(isRightFAMidExist) {
            dicomSr->addVascularMeasurementContainer(leArterialJson,RTFAMID, nullptr,
                                                     &midTopographicalValue,isRightAdded);
            isRightAdded = true;
        }

        bool  isRightFADistExist = leArterialJson.isMember(LabelName(RTFADIST));
        if(isRightFADistExist) {
            dicomSr->addVascularMeasurementContainer(leArterialJson,RTFADIST, nullptr,
                                                     &distTopographicalValue,isRightAdded);
            isRightAdded = true;
        }

        //RTPOPA

        bool  isRightPopaExist = leArterialJson.isMember(LabelName(RTPOPA));
        if(isRightPopaExist) {
            dicomSr->addVascularMeasurementContainer(leArterialJson,RTPOPA, nullptr,
                                                     nullptr,isRightAdded);
            isRightAdded = true;
        }

        bool  isRightPtaExist = leArterialJson.isMember(LabelName(RTPTA));
        if(isRightPtaExist) {
            dicomSr->addVascularMeasurementContainer(leArterialJson,RTPTA, nullptr,
                                                     nullptr,isRightAdded);
            isRightAdded = true;
        }

        bool  isRightPeroaExist = leArterialJson.isMember(LabelName(RTPEROA));
        if(isRightPeroaExist) {
            dicomSr->addVascularMeasurementContainer(leArterialJson,RTPEROA, nullptr,
                                                     nullptr,isRightAdded);
            isRightAdded = true;
        }

        bool  isRightAtaExist = leArterialJson.isMember(LabelName(RTATA));
        if(isRightAtaExist) {
            dicomSr->addVascularMeasurementContainer(leArterialJson,RTATA, nullptr,
                                                     nullptr,isRightAdded);
            isRightAdded = true;
        }

        //RTDPA

        bool  isRightDpaExist = leArterialJson.isMember(LabelName(RTDPA));
        if(isRightDpaExist) {
            dicomSr->addVascularMeasurementContainer(leArterialJson,RTDPA, nullptr,
                                                     nullptr,isRightAdded);
            isRightAdded = true;
        }

        bool isLeftArterial = dicomSr->isLeftDataForLowerArterial(leArterialJson);

        if(isLeftArterial){

            doc->getTree().goUp();

            doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Container, DSRTypes::AM_belowCurrent);
            doc->getTree().getCurrentContentItem().setConceptName(CODE_DCM_RETIRED_Findings);
            doc->getTree().getCurrentContentItem().setContinuityOfContent(DSRTypes::COC_Separate);

            // Finidng Site Name
            doc->getTree().addContentItem(DSRTypes::RT_hasConceptMod,  DSRTypes::VT_Code,DSRTypes::AM_belowCurrent);
            doc->getTree().getCurrentContentItem().setConceptName(CODE_SRT_FindingSite);
            doc->getTree().getCurrentContentItem().setCodeValue(findingSite);
            doc->getTree().getCurrentContentItem().setContinuityOfContent(DSRTypes::COC_Separate);

            doc->getTree().addContentItem(DSRTypes::RT_hasConceptMod,DSRTypes::VT_Code,DSRTypes::AM_afterCurrent);
            doc->getTree().getCurrentContentItem().setConceptName(CODE_SRT_Laterality);
            doc->getTree().getCurrentContentItem().setCodeValue(lateracityLeft);

            bool isLeftAdded = false;
            bool isLeftCFAExist = leArterialJson.isMember(LabelName(LTCFA));
            LOGSR("isLeftCFAExist Found %d",isLeftCFAExist);
            if(isLeftCFAExist){
                dicomSr->addVascularMeasurementContainer(leArterialJson,LTCFA, nullptr,
                                                         nullptr,isLeftAdded);
                isLeftAdded = true;
            }


            bool isLeftProFaExist = leArterialJson.isMember(LabelName(LTPROFA));
            LOGSR("isLeftProFaExist Found %d",isLeftProFaExist);
            if(isLeftProFaExist){
                dicomSr->addVascularMeasurementContainer(leArterialJson,LTPROFA, nullptr,
                                                         nullptr,isLeftAdded);
                isLeftAdded = true;
            }

            bool  isLeftFAProxExist = leArterialJson.isMember(LabelName(LTFAPROX));
            if(isLeftFAProxExist) {
                dicomSr->addVascularMeasurementContainer(leArterialJson,LTFAPROX, nullptr,
                                                         &proxTopographicalValue,isLeftAdded);
                isLeftAdded = true;
            }

            bool  isLeftFAMidExist = leArterialJson.isMember(LabelName(LTFAMID));
            if(isLeftFAMidExist) {
                dicomSr->addVascularMeasurementContainer(leArterialJson,LTFAMID, nullptr,
                                                         &midTopographicalValue,isLeftAdded);
                isLeftAdded = true;
            }

            bool  isLeftFADistExist = leArterialJson.isMember(LabelName(LTFADIST));
            if(isLeftFADistExist) {
                dicomSr->addVascularMeasurementContainer(leArterialJson,LTFADIST, nullptr,
                                                         &distTopographicalValue,isLeftAdded);
                isLeftAdded = true;
            }

            //RTPOPA

            bool  isLeftPopaExist = leArterialJson.isMember(LabelName(LTPOPA));
            if(isLeftPopaExist) {
                dicomSr->addVascularMeasurementContainer(leArterialJson,LTPOPA, nullptr,
                                                         nullptr,isLeftAdded);
                isLeftAdded = true;
            }

            bool  isLeftPtaExist = leArterialJson.isMember(LabelName(LTPTA));
            if(isLeftPtaExist) {
                dicomSr->addVascularMeasurementContainer(leArterialJson,LTPTA, nullptr,
                                                         nullptr,isLeftAdded);
                isLeftAdded = true;
            }

            bool  isLeftPeroaExist = leArterialJson.isMember(LabelName(LTPEROA));
            if(isLeftPeroaExist) {
                dicomSr->addVascularMeasurementContainer(leArterialJson,LTPEROA, nullptr,
                                                         nullptr,isLeftAdded);
                isLeftAdded = true;
            }

            bool  isLeftAtaExist = leArterialJson.isMember(LabelName(LTATA));
            if(isLeftAtaExist) {
                dicomSr->addVascularMeasurementContainer(leArterialJson,LTATA, nullptr,
                                                         nullptr,isLeftAdded);
                isLeftAdded = true;
            }

            //RTDPA

            bool  isLeftDpaExist = leArterialJson.isMember(LabelName(LTDPA));
            if(isLeftDpaExist) {
                dicomSr->addVascularMeasurementContainer(leArterialJson,LTDPA, nullptr,
                                                         nullptr,isLeftAdded);
                isLeftAdded = true;
            }
        }

        if(leArterialJson.isMember("Comment") && (isRightArterial || isLeftArterial)){
            DSRCodedEntryValue code = DSRCodedEntryValue("121106", "DCM", "Comment");
            string comment = leArterialJson["Comment"].asString();
            dicomSr->addText(DSRTypes::AM_belowCurrent, code, comment);
            doc->getTree().goUp();
        }

    }


    if(root.isMember(LEVenous)){
        doc->getTree().goUp();
        const auto& leVenousJson = root[LEVenous];
        LOGSR("LEVenous Found");

        auto findingSite = DSRCodedEntryValue("T-49403", "SRT", "Vein of Lower Extremity");

        doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Container, DSRTypes::AM_belowCurrent);
        doc->getTree().getCurrentContentItem().setConceptName(CODE_DCM_RETIRED_Findings);
        doc->getTree().getCurrentContentItem().setContinuityOfContent(DSRTypes::COC_Separate);

        // Finidng Site Name
        doc->getTree().addContentItem(DSRTypes::RT_hasConceptMod,  DSRTypes::VT_Code,DSRTypes::AM_belowCurrent);
        doc->getTree().getCurrentContentItem().setConceptName(CODE_SRT_FindingSite);
        doc->getTree().getCurrentContentItem().setCodeValue(findingSite);
        doc->getTree().getCurrentContentItem().setContinuityOfContent(DSRTypes::COC_Separate);

        bool isRightVenous = dicomSr->isRightDataForLowerVenous(leVenousJson);

        if(isRightVenous){
            doc->getTree().addContentItem(DSRTypes::RT_hasConceptMod,DSRTypes::VT_Code,DSRTypes::AM_afterCurrent);
            doc->getTree().getCurrentContentItem().setConceptName(CODE_SRT_Laterality);
            doc->getTree().getCurrentContentItem().setCodeValue(lateracityRight);
        }


        //generateNodeForVein

        bool isRightAdded = false;
        //VenousLabelName(RTCFV)
        bool  isRightCFVExist =leVenousJson.isMember(VenousLabelName(RTCFV));
        if(isRightCFVExist){
            //auto jsonArray = leVenousJson[VenousLabelName(RTCFV)];
            dicomSr->generateNodeForVein(leVenousJson,RTCFV, nullptr,isRightAdded);
            isRightAdded = true;
        }

        bool  isRightGSVExist =leVenousJson.isMember(VenousLabelName(RTGSV));
        if(isRightGSVExist){
            auto jsonArray = leVenousJson[VenousLabelName(RTGSV)];
            dicomSr->generateNodeForVein(leVenousJson,RTGSV, nullptr,isRightAdded);
            isRightAdded = true;
        }

        bool  isRightProxExist =leVenousJson.isMember(VenousLabelName(RTPROF));
        if(isRightProxExist){
            auto jsonArray = leVenousJson[VenousLabelName(RTPROF)];
            dicomSr->generateNodeForVein(leVenousJson,RTPROF, nullptr,isRightAdded);
            isRightAdded = true;
        }

        bool  isRightFVProxExist =leVenousJson.isMember(VenousLabelName(RTFVPROX));
        if(isRightFVProxExist){
            auto jsonArray = leVenousJson[VenousLabelName(RTFVPROX)];
            dicomSr->generateNodeForVein(leVenousJson,RTFVPROX, &proxTopographicalValue,isRightAdded);
            isRightAdded = true;
        }

        bool  isRightFVMidExist =leVenousJson.isMember(VenousLabelName(RTFVMID));
        if(isRightFVMidExist){
            auto jsonArray = leVenousJson[VenousLabelName(RTFVMID)];
            dicomSr->generateNodeForVein(leVenousJson,RTFVMID, &midTopographicalValue,isRightAdded);
            isRightAdded = true;
        }

        bool  isRightFVDistExist =leVenousJson.isMember(VenousLabelName(RTFVDIST));
        if(isRightFVDistExist){
            auto jsonArray = leVenousJson[VenousLabelName(RTFVDIST)];
            dicomSr->generateNodeForVein(leVenousJson,RTFVDIST, &distTopographicalValue,isRightAdded);
            isRightAdded = true;
        }

        //RTPOPV
        bool  isRightPopVExist =leVenousJson.isMember(VenousLabelName(RTPOPV));
        if(isRightPopVExist){
            auto jsonArray = leVenousJson[VenousLabelName(RTPOPV)];
            dicomSr->generateNodeForVein(leVenousJson,RTPOPV, nullptr,isRightAdded);
            isRightAdded = true;
        }

        bool  isRightGasTrocvVExist =leVenousJson.isMember(VenousLabelName(RTGASTROCV));
        if(isRightGasTrocvVExist){
            auto jsonArray = leVenousJson[VenousLabelName(RTGASTROCV)];
            dicomSr->generateNodeForVein(leVenousJson,RTGASTROCV, nullptr,isRightAdded);
            isRightAdded = true;
        }
        //RTPTV

        bool  isRightPtvExist =leVenousJson.isMember(VenousLabelName(RTPTV));
        if(isRightPtvExist){
            auto jsonArray = leVenousJson[VenousLabelName(RTPTV)];
            dicomSr->generateNodeForVein(leVenousJson,RTPTV, nullptr,isRightAdded);
            isRightAdded = true;
        }

        bool  isRightPerovExist =leVenousJson.isMember(VenousLabelName(RTPEROV));
        if(isRightPerovExist){
            auto jsonArray = leVenousJson[VenousLabelName(RTPEROV)];
            dicomSr->generateNodeForVein(leVenousJson,RTPEROV, nullptr,isRightAdded);
            isRightAdded = true;
        }

        bool  isRightAtvExist =leVenousJson.isMember(VenousLabelName(RTATV));
        if(isRightAtvExist){
            auto jsonArray = leVenousJson[VenousLabelName(RTATV)];
            dicomSr->generateNodeForVein(leVenousJson,RTATV, nullptr,isRightAdded);
            isRightAdded = true;
        }

        bool  isRightDpvExist =leVenousJson.isMember(VenousLabelName(RTDPV));
        if(isRightDpvExist){
            auto jsonArray = leVenousJson[VenousLabelName(RTDPV)];
            dicomSr->generateNodeForVein(leVenousJson,RTDPV, nullptr,isRightAdded);
            isRightAdded = true;
        }


        bool isLeftVenous = dicomSr->isLeftDataForLowerVenous(leVenousJson);

        if(isLeftVenous){
            doc->getTree().goUp();

            doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Container, DSRTypes::AM_belowCurrent);
            doc->getTree().getCurrentContentItem().setConceptName(CODE_DCM_RETIRED_Findings);
            doc->getTree().getCurrentContentItem().setContinuityOfContent(DSRTypes::COC_Separate);

            // Finidng Site Name
            doc->getTree().addContentItem(DSRTypes::RT_hasConceptMod,  DSRTypes::VT_Code,DSRTypes::AM_belowCurrent);
            doc->getTree().getCurrentContentItem().setConceptName(CODE_SRT_FindingSite);
            doc->getTree().getCurrentContentItem().setCodeValue(findingSite);
            doc->getTree().getCurrentContentItem().setContinuityOfContent(DSRTypes::COC_Separate);

            doc->getTree().addContentItem(DSRTypes::RT_hasConceptMod,DSRTypes::VT_Code,DSRTypes::AM_afterCurrent);
            doc->getTree().getCurrentContentItem().setConceptName(CODE_SRT_Laterality);
            doc->getTree().getCurrentContentItem().setCodeValue(lateracityLeft);

            bool isLeftAdded = false;
            //VenousLabelName(RTCFV)
            bool  isLeftCFVExist =leVenousJson.isMember(VenousLabelName(LTCFV));
            if(isLeftCFVExist){
                auto jsonArray = leVenousJson[VenousLabelName(LTCFV)];
                dicomSr->generateNodeForVein(leVenousJson,LTCFV, nullptr,isLeftAdded);
                isLeftAdded = true;
            }

            bool  isLeftGSVExist =leVenousJson.isMember(VenousLabelName(LTGSV));
            if(isLeftGSVExist){
                auto jsonArray = leVenousJson[VenousLabelName(LTGSV)];
                dicomSr->generateNodeForVein(leVenousJson,LTGSV, nullptr,isLeftAdded);
                isLeftAdded = true;
            }

            bool  isLeftProxExist =leVenousJson.isMember(VenousLabelName(LTPROF));
            if(isLeftProxExist){
                auto jsonArray = leVenousJson[VenousLabelName(LTPROF)];
                dicomSr->generateNodeForVein(leVenousJson,LTPROF, nullptr,isLeftAdded);
                isLeftAdded = true;
            }

            bool  isLeftFVProxExist =leVenousJson.isMember(VenousLabelName(LTFVPROX));
            if(isLeftFVProxExist){
                auto jsonArray = leVenousJson[VenousLabelName(LTFVPROX)];
                dicomSr->generateNodeForVein(leVenousJson,LTFVPROX, &proxTopographicalValue,isLeftAdded);
                isLeftAdded = true;
            }

            bool  isLeftFVMidExist =leVenousJson.isMember(VenousLabelName(LTFVMID));
            if(isLeftFVMidExist){
                auto jsonArray = leVenousJson[VenousLabelName(LTFVMID)];
                dicomSr->generateNodeForVein(leVenousJson,LTFVMID, &midTopographicalValue,isLeftAdded);
                isLeftAdded = true;
            }

            bool  isLeftFVDistExist =leVenousJson.isMember(VenousLabelName(LTFVDIST));
            if(isLeftFVDistExist){
                auto jsonArray = leVenousJson[VenousLabelName(LTFVDIST)];
                dicomSr->generateNodeForVein(leVenousJson,LTFVDIST, &distTopographicalValue,isLeftAdded);
                isLeftAdded = true;
            }

            //LTPOPV
            bool  isLeftPopVExist =leVenousJson.isMember(VenousLabelName(LTPOPV));
            if(isLeftPopVExist){
                auto jsonArray = leVenousJson[VenousLabelName(LTPOPV)];
                dicomSr->generateNodeForVein(leVenousJson,LTPOPV, nullptr,isLeftAdded);
                isLeftAdded = true;
            }

            bool  isLeftGasTrocvVExist =leVenousJson.isMember(VenousLabelName(LTGASTROCV));
            if(isLeftGasTrocvVExist){
                auto jsonArray = leVenousJson[VenousLabelName(LTGASTROCV)];
                dicomSr->generateNodeForVein(leVenousJson,LTGASTROCV, nullptr,isLeftAdded);
                isLeftAdded = true;
            }
            //LTPTV

            bool  isLeftPtvExist =leVenousJson.isMember(VenousLabelName(LTPTV));
            if(isLeftPtvExist){
               // auto jsonArray = leVenousJson[VenousLabelName(LTPTV)];
                dicomSr->generateNodeForVein(leVenousJson,LTPTV, nullptr,isLeftAdded);
                isLeftAdded = true;
            }

            bool  isLeftPerovExist =leVenousJson.isMember(VenousLabelName(LTPEROV));
            if(isLeftPerovExist){
                //auto jsonArray = leVenousJson[VenousLabelName(LTPEROV)];
                dicomSr->generateNodeForVein(leVenousJson,LTPEROV, nullptr,isLeftAdded);
                isLeftAdded = true;
            }

            bool  isLeftAtvExist =leVenousJson.isMember(VenousLabelName(LTATV));
            if(isLeftAtvExist){
                //auto jsonArray = leVenousJson[VenousLabelName(LTATV)];
                dicomSr->generateNodeForVein(leVenousJson,LTATV, nullptr,isLeftAdded);
                isLeftAdded = true;
            }

            bool  isLeftDpvExist =leVenousJson.isMember(VenousLabelName(LTDPV));
            if(isLeftDpvExist){
                //auto jsonArray = leVenousJson[VenousLabelName(LTDPV)];
                dicomSr->generateNodeForVein(leVenousJson,LTDPV, nullptr,isLeftAdded);
                isLeftAdded = true;
            }

        }

        if(leVenousJson.isMember("Comment") && (isRightVenous || isLeftVenous)){
            DSRCodedEntryValue code = DSRCodedEntryValue("121106", "DCM", "Comment");
            string comment = leVenousJson["Comment"].asString();
            dicomSr->addText(DSRTypes::AM_belowCurrent, code, comment);
            doc->getTree().goUp();
        }

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


DSRCodedEntryValue getCodeForContainer(LabelKey key) {
    switch (key) {
        case RTCCAMID:
        case RTCCADIST:
        case RTCCAPROX:
        case LTCCAMID:
        case LTCCADIST:
        case LTCCAPROX:
            return DSRCodedEntryValue("T-45100", "SRT", "Common Carotid Artery");
        case RTECA:
        case LTECA:
            return DSRCodedEntryValue("T-45200", "SRT", "External Carotid Artery");
        case RTICAPROX:
        case RTICAMID:
        case RTICADIST:
        case LTICAPROX:
        case LTICAMID:
        case LTICADIST:
            return DSRCodedEntryValue("T-45300", "SRT", "Internal Carotid Artery");
        case RTBULB:
        case LTBULB:
            return DSRCodedEntryValue("T-45170", "SRT", "Carotid Bulb");
        case RTVERT:
        case LTVERT:
            return DSRCodedEntryValue("T-45700", "SRT", "Vertebral Artery");
        case RTSUBCL:
        case LTSUBCL:
            return DSRCodedEntryValue("T-45700", "SRT", "Subclavian Artery");
        case RTCFA:
        case LTCFA:
            return DSRCodedEntryValue("T-47400", "SRT", "Common Femoral Artery");
        case RTPROFA:
        case LTPROFA:
            return DSRCodedEntryValue("T-47440", "SRT", "Profunda Femoris Artery");
        case RTFAPROX:
        case RTFAMID:
        case RTFADIST:
        case LTFAPROX:
        case LTFAMID:
        case LTFADIST:
            return DSRCodedEntryValue("T-47403", "SRT", "Superficial Femoral Artery"); // TODO: Confirm with Customer
        case RTPOPA:
        case LTPOPA:
            return DSRCodedEntryValue("T-47500", "SRT", "Popliteal Artery");
        case RTPTA:
        case LTPTA:
            return DSRCodedEntryValue("T-47600", "SRT", "Posterior Tibial Artery");
        case RTPEROA:
        case LTPEROA:
            return DSRCodedEntryValue("T-47630", "SRT", "Peroneal Artery");
        case RTATA:
        case LTATA:
            return DSRCodedEntryValue("T-47700", "SRT", "Anterior Tibial Artery");
        case RTDPA:
        case LTDPA:
            return DSRCodedEntryValue("T-47741", "SRT", "Dorsalis Pedis Artery");

    }
}



static const char * createSR(JNIEnv *env,jstring outputFilePath){

    auto *doc = new DSRDocument();

    doc->createNewDocument(DSRTypes::DT_SimplifiedAdultEchoSR);

    LOGSR("SR:: Empty document created");

    OFBool writeFile = OFTrue;

    OFString studyUID_01;
    jboolean isCopy;

    const char* outputFile=env->GetStringUTFChars(outputFilePath,&isCopy);

    if (!studyUID_01.empty()){
        doc->createNewSeriesInStudy(studyUID_01);
    }

    doc->setStudyDescription("OFFIS Structured Reporting Samples");
    doc->setSeriesDescription("Text Report with CODE, NUM and PNAME content items");
    doc->setPatientName("Osterman^Phillip B.");
    doc->setPatientBirthDate("19220909");
    doc->setPatientSex("M");
    doc->setReferringPhysicianName("Fukuda^Katherine M.^^^M. D.");
    doc->setManufacturer("Echonous");
    doc->setManufacturerModelName("Torso");
    doc->setDeviceSerialNumber("12345");
    doc->setSoftwareVersions("7.0.0");

    doc->getTree().addContentItem(DSRTypes::RT_isRoot, DSRTypes::VT_Container);
    doc->getTree().getCurrentContentItem().setConceptName(DSRCodedEntryValue("DT.06", OFFIS_CODING_SCHEME_DESIGNATOR, "Main Findings"));

    doc->getTree().addContentItem(DSRTypes::RT_hasObsContext, DSRTypes::VT_Code, DSRTypes::AM_belowCurrent);
    doc->getTree().getCurrentContentItem().setConceptName(DSRCodedEntryValue("IHE.04", OFFIS_CODING_SCHEME_DESIGNATOR, "Observer Name"));
    doc->getTree().getCurrentContentItem().setStringValue("Packer^David M.^^^M. D.");

    doc->getTree().addContentItem(DSRTypes::RT_hasObsContext, DSRTypes::VT_Text);
    doc->getTree().getCurrentContentItem().setConceptName(DSRCodedEntryValue("IHE.05", OFFIS_CODING_SCHEME_DESIGNATOR, "Observer Organization Name"));
    doc->getTree().getCurrentContentItem().setStringValue("Redlands Clinic");

    doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Container);
    doc->getTree().getCurrentContentItem().setContinuityOfContent(DSRTypes::COC_Continuous);

    doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Text, DSRTypes::AM_belowCurrent);
    doc->getTree().getCurrentContentItem().setConceptName(DSRCodedEntryValue("CODE_01", OFFIS_CODING_SCHEME_DESIGNATOR, "Description"));
    doc->getTree().getCurrentContentItem().setStringValue("This 78-year-old gentleman referred by");

    doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_PName);
    doc->getTree().getCurrentContentItem().setConceptName(DSRCodedEntryValue("CODE_04", OFFIS_CODING_SCHEME_DESIGNATOR, "Referring Physician"));
    doc->getTree().getCurrentContentItem().setStringValue("Fukuda^^^Dr.");

    doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Text);
    doc->getTree().getCurrentContentItem().setConceptName(DSRCodedEntryValue("CODE_01", OFFIS_CODING_SCHEME_DESIGNATOR, "Description"));
    doc->getTree().getCurrentContentItem().setStringValue("was also seen by");

    doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_PName);
    doc->getTree().getCurrentContentItem().setConceptName(DSRCodedEntryValue("OR.01", OFFIS_CODING_SCHEME_DESIGNATOR, "Physician"));
    doc->getTree().getCurrentContentItem().setStringValue("Mason^^^Dr.");

    doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Text);
    doc->getTree().getCurrentContentItem().setConceptName(DSRCodedEntryValue("CODE_01", OFFIS_CODING_SCHEME_DESIGNATOR, "Description"));
    doc->getTree().getCurrentContentItem().setStringValue("at the");

    doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Code);
    doc->getTree().getCurrentContentItem().setConceptName(DSRCodedEntryValue("CODE_05", OFFIS_CODING_SCHEME_DESIGNATOR, "Hospital Name"));
    doc->getTree().getCurrentContentItem().setCodeValue(DSRCodedEntryValue("CODE_06", OFFIS_CODING_SCHEME_DESIGNATOR, "Redlands Clinic"));

    doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Text);
    doc->getTree().getCurrentContentItem().setConceptName(DSRCodedEntryValue("CODE_01", OFFIS_CODING_SCHEME_DESIGNATOR, "Description"));
    doc->getTree().getCurrentContentItem().setStringValue(". He has been seen in the past by");

    doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_PName);
    doc->getTree().getCurrentContentItem().setConceptName(DSRCodedEntryValue("OR.01", OFFIS_CODING_SCHEME_DESIGNATOR, "Physician"));
    doc->getTree().getCurrentContentItem().setStringValue("Klugman^^^Dr.");

    doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Text);
    doc->getTree().getCurrentContentItem().setConceptName(DSRCodedEntryValue("CODE_01", OFFIS_CODING_SCHEME_DESIGNATOR, "Description"));
    doc->getTree().getCurrentContentItem().setStringValue(".\nThe patient developed a lesion in the concha of the left external ear. Recent biopsy confirmed this as being a squamous cell carcinoma. The patient has had a few other skin cancers.\nOf most significant past history is the fact that this patient has a leukemia that has been treated in standard fashion by");

    doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_PName);
    doc->getTree().getCurrentContentItem().setConceptName(DSRCodedEntryValue("OR.01", OFFIS_CODING_SCHEME_DESIGNATOR, "Physician"));
    doc->getTree().getCurrentContentItem().setStringValue("Klugman^^^Dr.");

    doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Text);
    doc->getTree().getCurrentContentItem().setConceptName(DSRCodedEntryValue("CODE_01", OFFIS_CODING_SCHEME_DESIGNATOR, "Description"));
    doc->getTree().getCurrentContentItem().setStringValue(". The patient was then transferred to the");

    doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Code);
    doc->getTree().getCurrentContentItem().setConceptName(DSRCodedEntryValue("CODE_05", OFFIS_CODING_SCHEME_DESIGNATOR, "Hospital Name"));
    doc->getTree().getCurrentContentItem().setCodeValue(DSRCodedEntryValue("CODE_06", OFFIS_CODING_SCHEME_DESIGNATOR, "Redlands Clinic"));

    doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Text);
    doc->getTree().getCurrentContentItem().setConceptName(DSRCodedEntryValue("CODE_01", OFFIS_CODING_SCHEME_DESIGNATOR, "Description"));
    doc->getTree().getCurrentContentItem().setStringValue("and by some experimental protocol which, I guess, includes some sort of lymphocyte electrophoresis, has been placed into remission. He is not currently on any antileukemia drugs and has responded extremely well to his medical management.\nOn examination, the patient is healthy in general appearance. There is a");

    doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Num);
    doc->getTree().getCurrentContentItem().setConceptName(DSRCodedEntryValue("M-02550", "SNM3", "Diameter"));
    doc->getTree().getCurrentContentItem().setNumericValue(DSRNumericMeasurementValue("1.5", CODE_UCUM_Centimeter));

    doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Text);
    doc->getTree().getCurrentContentItem().setConceptName(DSRCodedEntryValue("CODE_01", OFFIS_CODING_SCHEME_DESIGNATOR, "Description"));
    doc->getTree().getCurrentContentItem().setStringValue("lesion on the concha of the ear, which is seen well on photograph of the left external ear. There are numerous soft lymph nodes in both sides of the neck, which I presume are related to his leukemia.");

    doc->getTree().goUp();

    doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Text);
    doc->getTree().getCurrentContentItem().setConceptName(DSRCodedEntryValue("CODE_02", OFFIS_CODING_SCHEME_DESIGNATOR, "Diagnosis"));
    doc->getTree().getCurrentContentItem().setStringValue("Squamous cell carcinoma, relatively superficial, involving the skin of the left external ear, which is seen well on photograph of the left external ear.");

    doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Text);
    doc->getTree().getCurrentContentItem().setConceptName(DSRCodedEntryValue("CODE_03", OFFIS_CODING_SCHEME_DESIGNATOR, "Treatment"));
    doc->getTree().getCurrentContentItem().setStringValue("The plan of treatment is as follows: 4500 rad, 15 treatment sessions, using 100 kV radiation.\nThe reason for treatment, expected acute reaction, and remote possibility of complication was discussed with this patient at some length, and he accepted therapy as outlined.");

    // add additional information on UCUM coding scheme
    doc->getCodingSchemeIdentification().addItem(US2_CODING_SCHEME_DESIGNATOR);
    doc->getCodingSchemeIdentification().setCodingSchemeUID(CODE_UCUM_CodingSchemeUID);
    doc->getCodingSchemeIdentification().setCodingSchemeName(CODE_UCUM_CodingSchemeDescription);
    doc->getCodingSchemeIdentification().setCodingSchemeResponsibleOrganization("Regenstrief Institute for Health Care, Indianapolis");

    auto *fileformat = new DcmFileFormat();
    DcmDataset *dataset = nullptr;
    if (fileformat != nullptr){
        dataset = fileformat->getDataset();
    }
    OFCondition result=doc->write(*dataset);
    if (result.good()){
        fileformat->saveFile(outputFile, EXS_LittleEndianImplicit);
    }else{
        LOGSR("Error in DICOM SR dataset %s", result.text());
    }

    return "Success";
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
