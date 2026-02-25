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
DCMTKUTIL_METHOD(createDicomSr)(JNIEnv* env,jobject thiz,jstring jsonReport,jstring outputFilePath){


    //createSR(env,outputFilePath);
    return createUS2Sr(env,outputFilePath,jsonReport);
}

static jobject createUS2Sr(JNIEnv *env,jstring outputFilePath,jstring jsonReport){

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
    dicomSr->getDocInstant()->setStudyDescription("Adult Echocardiography Procedure Report");
    dicomSr->getDocInstant()->setSeriesDescription("Adult Echocardiography with measurements");
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

    dicomSr->setRootContainer(CODE_DCM_AdultEchocardiographyProcedureReport);
    mResult = dicomSr->getDocInstant()->getTree().setTemplateIdentification("5200","DCMR");
    LOGSR("%s setTemplateIdentification Result %s",TAG,mResult.text());

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
    if (root.isMember(LV)) {
        LOGSR("%s Outer Left Ventricle 1 New",TAG);
        Json::Value leftVentricle = root["Left Ventricle"];
        LOGSR("%s Outer Left Ventricle 2",TAG);

        auto isGoUpDoneLVVolume = false;
        if (leftVentricle.isMember("Left Ventricle Volume")) {
            Json::Value lvVolume = leftVentricle["Left Ventricle Volume"];
            dicomSr->addFindingSite();
            auto isBelowCurrentNeeded = true;
            isLVRootSet = true;
            bool  isEFOnly=false;
            for (Json::Value::ArrayIndex i = 0; i != lvVolume.size(); i++) {
                Json::Value::Members members = lvVolume[i].getMemberNames();
                //leftVentricleVolume.push_back(lvVolume[i].asString());
                LOGSR("%s Left Ventricle Volume Array Index %d Members Size %lu",TAG,i,members.size());
                for (auto & member : members) {
                    LOGSR("%s 2 LV Volume label: %s ",TAG, member.c_str());
                    if(member == LVEDV){
                        if(lvVolume[i][member].isMember("value")){
                            string value = lvVolume[i][member]["value"].asString();

                            LOGSR("%s Found LVEDV Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                            const auto lvefCodeValue=DSRCodedEntryValue("18026-5", "LN", "Left Ventricular End Diastolic Volume");
                            const auto lvefNumValue=DSRNumericMeasurementValue(value, CODE_UCUM_ml);
                            if (isBelowCurrentNeeded) {
                                dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent, lvefCodeValue,lvefNumValue);
                            } else {
                                dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, lvefCodeValue,lvefNumValue);
                            }

                            dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_2D);
                            doc->getTree().goUp();
                        }
                    }


                    if(member == LVESV){
                        if(lvVolume[i][member].isMember("value")){
                            string value = lvVolume[i][member]["value"].asString();

                            LOGSR("%s Found LVESV Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                            const auto lvefCodeValue=DSRCodedEntryValue("18148-7", "LN", "Left Ventricular End Systolic Volume");
                            const auto lvefNumValue=DSRNumericMeasurementValue(value, CODE_UCUM_ml);
                            if (isBelowCurrentNeeded) {
                                dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent, lvefCodeValue,lvefNumValue);
                            } else {
                                dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, lvefCodeValue,lvefNumValue);
                            }

                            dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_2D);
                            doc->getTree().goUp();
                        }
                    }


                    if( member == LVEF){
                        if(lvVolume[i][member].isMember("value")){
                            string value = lvVolume[i][member]["value"].asString();
                            LOGSR("%s Found LVEF Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                            const DSRCodedEntryValue lvefCodeValue=DSRCodedEntryValue("18043-0", "LN", "Left Ventricular Ejection Fraction");
                            const  DSRNumericMeasurementValue lvefNumValue=DSRNumericMeasurementValue(value, CODE_US2_PECENT);
                            if(isBelowCurrentNeeded) {
                                dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent,lvefCodeValue,lvefNumValue);
                            } else {
                                dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent,lvefCodeValue,lvefNumValue);
                            }
                            dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_2D);
                            isGoUpDoneLVVolume = true;
                            doc->getTree().goUp();
                        }
                    }

                    if( member == MM_EF){
                        if(lvVolume[i][member].isMember("value")){
                            string value = lvVolume[i][member]["value"].asString();
                            LOGSR("%s Found LVEF Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                            const DSRCodedEntryValue lvefCodeValue=DSRCodedEntryValue("18049-7", "LN", "Left ventricular Ejection fraction by US.M-mode+Calculated by Teichholz method");
                            const  DSRNumericMeasurementValue lvefNumValue=DSRNumericMeasurementValue(value, CODE_US2_PECENT);
                            if(isBelowCurrentNeeded) {
                                dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent,lvefCodeValue,lvefNumValue);
                            } else {
                                dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent,lvefCodeValue,lvefNumValue);
                            }
                            dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_M);
                            isGoUpDoneLVVolume = true;
                            doc->getTree().goUp();
                        }
                    }
                }
                if( i == 0) {
                    isBelowCurrentNeeded = false;
                }
            }
        }

        auto isGoUpDone = false;
        auto isSiteAdded = false;
        if(leftVentricle.isMember("Left Ventricle Linear")){
            Json::Value lvLinear = leftVentricle["Left Ventricle Linear"];
            auto isBelowCurrentNeeded = false;
            if(isLVRootSet && !isGoUpDoneLVVolume){
                doc->getTree().goUp();
            }else{
                if(!isLVRootSet){
                    dicomSr->addFindingSite();
                    isLVRootSet = true;
                    isSiteAdded = true;
                    isBelowCurrentNeeded = true;
                }
            }


            //Loop of LVL
            for (Json::Value::ArrayIndex i = 0; i != lvLinear.size(); i++) {
                Json::Value::Members members = lvLinear[i].getMemberNames();

                //leftVentricleVolume.push_back(lvVolume[i].asString());
                LOGSR("%s Left Ventricle Linear Array Index %d Members Size %lu",TAG,i,members.size());

                for(auto & member : members){

                    if(member == LVIDD){
                        if(lvLinear[i][member].isMember("value")){
                            string value = lvLinear[i][member]["value"].asString();
                            const auto lvLVIDCodeValue=DSRCodedEntryValue("29436-3", "LN", "Left Ventricle Internal End Diastolic Dimension");
                            const auto unitIs= lvLinear[i][member]["unit"].asString();
                            LOGSR("%s Found LVIDD Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                            auto measurementValue =DSRNumericMeasurementValue(value, dicomSr->getDistBasicMeasurementUnit(unitIs));
                            if(isBelowCurrentNeeded){
                                dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent,lvLVIDCodeValue,measurementValue);
                            }else{
                                dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent,lvLVIDCodeValue,measurementValue);
                            }

                            dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_2D);
                            if(lvLinear[i][member].isMember(SELECTION_STATUS)){
                                LOGSR("%s Found LVIDD Value found member is %s value is %s Selection Status Exist", TAG,member.c_str(),value.c_str());
                                string selectionValue = lvLinear[i][member][SELECTION_STATUS].asString();
                                if(!selectionValue.empty()){
                                    dicomSr->addProperty(selectionValue,DSRTypes::AM_afterCurrent);
                                }
                            }
                            dicomSr->addCordinates(lvLinear[i][member],lvLinear[i][member][SOPUID].asString());
                            doc->getTree().goUp();
                            isGoUpDone = true;
                        }

                        fetchCoordinate(lvLinear[i][member]);
                    }

                    if(member == MM_LVIDD){
                        if(lvLinear[i][member].isMember("value")){
                            string value = lvLinear[i][member]["value"].asString();
                            const auto lvLVIDCodeValue=DSRCodedEntryValue("80008-6", "LN", "Left ventricular Minor axis at end diastole [Length] by US.M-mode");
                            const auto unitIs= lvLinear[i][member]["unit"].asString();
                            LOGSR("%s MM Found LVIDD Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                            auto measurementValue =DSRNumericMeasurementValue(value, dicomSr->getDistBasicMeasurementUnit(unitIs));
                            if(isBelowCurrentNeeded){
                                dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent,lvLVIDCodeValue,measurementValue);
                            }else{
                                dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent,lvLVIDCodeValue,measurementValue);
                            }

                            dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_M);
                            if(lvLinear[i][member].isMember(SELECTION_STATUS)){
                                LOGSR("%s Found LVIDD Value found member is %s value is %s Selection Status Exist", TAG,member.c_str(),value.c_str());
                                string selectionValue = lvLinear[i][member][SELECTION_STATUS].asString();
                                if(!selectionValue.empty()){
                                    dicomSr->addProperty(selectionValue,DSRTypes::AM_afterCurrent);
                                }
                            }
                            dicomSr->addCordinates(lvLinear[i][member],lvLinear[i][member][SOPUID].asString());
                            doc->getTree().goUp();
                            isGoUpDone = true;
                        }

                        fetchCoordinate(lvLinear[i][member]);
                    }


                    if(member == LVIDS){
                        if(lvLinear[i][member].isMember("value")){
                            string value = lvLinear[i][member]["value"].asString();
                            const auto lvLVIDSCodeValue=DSRCodedEntryValue("29438-9", "LN", "Left Ventricle Internal Systolic Dimension");
                            const auto unitIs= lvLinear[i][member]["unit"].asString();
                            LOGSR("%s Found LVIDS Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                            auto measurementValue =DSRNumericMeasurementValue(value, dicomSr->getDistBasicMeasurementUnit(unitIs));

                            if(isBelowCurrentNeeded){
                                dicomSr->addNumMeasurement(DSRTypes:: AM_belowCurrent ,lvLVIDSCodeValue,measurementValue);
                            }else{
                                dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent,lvLVIDSCodeValue,measurementValue);
                            }

                            dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_2D);
                            if(lvLinear[i][member].isMember(SELECTION_STATUS)){
                                string selectionValue = lvLinear[i][member][SELECTION_STATUS].asString();
                                if(!selectionValue.empty()){
                                    dicomSr->addProperty(selectionValue,DSRTypes::AM_afterCurrent);
                                }
                            }
                            dicomSr->addCordinates(lvLinear[i][member],lvLinear[i][member][SOPUID].asString());
                            doc->getTree().goUp();
                            isGoUpDone = true;
                        }
                    }

                        if(member == MM_LVIDS){
                            if(lvLinear[i][member].isMember("value")){
                                string value = lvLinear[i][member]["value"].asString();
                                const auto lvLVIDSCodeValue=DSRCodedEntryValue("80012-8", "LN", "Left ventricular Minor axis at end systole [Length] by US.M-mode");
                                const auto unitIs= lvLinear[i][member]["unit"].asString();
                                LOGSR("%s Found MM LVIDS Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                                auto measurementValue =DSRNumericMeasurementValue(value, dicomSr->getDistBasicMeasurementUnit(unitIs));

                                if(isBelowCurrentNeeded){
                                    dicomSr->addNumMeasurement(DSRTypes:: AM_belowCurrent ,lvLVIDSCodeValue,measurementValue);
                                }else{
                                    dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent,lvLVIDSCodeValue,measurementValue);
                                }

                                dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_M);
                                if(lvLinear[i][member].isMember(SELECTION_STATUS)){
                                    string selectionValue = lvLinear[i][member][SELECTION_STATUS].asString();
                                    if(!selectionValue.empty()){
                                        dicomSr->addProperty(selectionValue,DSRTypes::AM_afterCurrent);
                                    }
                                }
                                dicomSr->addCordinates(lvLinear[i][member],lvLinear[i][member][SOPUID].asString());
                                doc->getTree().goUp();
                                isGoUpDone = true;
                            }
                        }

                    if(member == IVSD){
                        if(lvLinear[i][member].isMember("value")){
                            string value = lvLinear[i][member]["value"].asString();
                            const auto lvIVSDCodeValue=DSRCodedEntryValue("18154-5", "LN", "Interventricular Septum Diastolic Thickness");
                            const auto unitIs= lvLinear[i][member]["unit"].asString();
                            LOGSR("%s Found IVSD Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                            auto measurementValue =DSRNumericMeasurementValue(value, dicomSr->getDistBasicMeasurementUnit(unitIs));

                            if(isBelowCurrentNeeded){
                                dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent,lvIVSDCodeValue,measurementValue);

                            }else{
                                dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent,lvIVSDCodeValue,measurementValue);
                            }

                            dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_2D);
                            if(lvLinear[i][member].isMember(SELECTION_STATUS)){
                                string selectionValue = lvLinear[i][member][SELECTION_STATUS].asString();
                                if(!selectionValue.empty()){
                                    dicomSr->addProperty(selectionValue,DSRTypes::AM_afterCurrent);
                                }
                            }
                            dicomSr->addCordinates(lvLinear[i][member],lvLinear[i][member][SOPUID].asString());
                            doc->getTree().goUp();
                            isGoUpDone = true;
                        }

                        //fetchCoordinate(lvLinear[i][member]);
                    }

                        if(member == MM_IVS){
                            if(lvLinear[i][member].isMember("value")){
                                string value = lvLinear[i][member]["value"].asString();
                                const auto lvIVSDCodeValue=DSRCodedEntryValue("79968-4", "LN", "Interventricular septum Thickness at end diastole by US.M-mode");
                                const auto unitIs= lvLinear[i][member]["unit"].asString();
                                LOGSR("%s Found MM IVSD Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                                auto measurementValue =DSRNumericMeasurementValue(value, dicomSr->getDistBasicMeasurementUnit(unitIs));

                                if(isBelowCurrentNeeded){
                                    dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent,lvIVSDCodeValue,measurementValue);

                                }else{
                                    dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent,lvIVSDCodeValue,measurementValue);
                                }

                                dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_M);
                                if(lvLinear[i][member].isMember(SELECTION_STATUS)){
                                    string selectionValue = lvLinear[i][member][SELECTION_STATUS].asString();
                                    if(!selectionValue.empty()){
                                        dicomSr->addProperty(selectionValue,DSRTypes::AM_afterCurrent);
                                    }
                                }
                                dicomSr->addCordinates(lvLinear[i][member],lvLinear[i][member][SOPUID].asString());
                                doc->getTree().goUp();
                                isGoUpDone = true;
                            }
                            //fetchCoordinate(lvLinear[i][member]);
                        }

                    if(member == LVPWd){
                        if(lvLinear[i][member].isMember("value")){
                            string value = lvLinear[i][member]["value"].asString();
                            const auto lvLVPWdCodeValue=DSRCodedEntryValue("18152-9", "LN", "Left Ventricle Posterior Wall Diastolic Thickness");
                            const auto unitIs= lvLinear[i][member]["unit"].asString();
                            LOGSR("%s Found LVPWd Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                            auto measurementValue =DSRNumericMeasurementValue(value, dicomSr->getDistBasicMeasurementUnit(unitIs));

                            if(isBelowCurrentNeeded){
                                dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent,lvLVPWdCodeValue,measurementValue);

                            }else{
                                dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent,lvLVPWdCodeValue,measurementValue);
                            }
                            dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_2D);
                            if(lvLinear[i][member].isMember(SELECTION_STATUS)){
                                string selectionValue = lvLinear[i][member][SELECTION_STATUS].asString();
                                if(!selectionValue.empty()){
                                    dicomSr->addProperty(selectionValue,DSRTypes::AM_afterCurrent);
                                }
                            }
                            dicomSr->addCordinates(lvLinear[i][member],lvLinear[i][member][SOPUID].asString());
                            doc->getTree().goUp();
                            isGoUpDone = true;
                        }
                    }

                    if(member == MM_LVPW){
                            if(lvLinear[i][member].isMember("value")){
                                string value = lvLinear[i][member]["value"].asString();
                                const auto lvLVPWdCodeValue=DSRCodedEntryValue("80031-8", "LN", "Left ventricular posterior wall Thickness at end diastole by US.M-mode");
                                const auto unitIs= lvLinear[i][member]["unit"].asString();
                                LOGSR("%s Found MM LVPWd Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                                auto measurementValue =DSRNumericMeasurementValue(value, dicomSr->getDistBasicMeasurementUnit(unitIs));

                                if(isBelowCurrentNeeded){
                                    dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent,lvLVPWdCodeValue,measurementValue);

                                }else{
                                    dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent,lvLVPWdCodeValue,measurementValue);
                                }
                                dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_M);
                                if(lvLinear[i][member].isMember(SELECTION_STATUS)){
                                    string selectionValue = lvLinear[i][member][SELECTION_STATUS].asString();
                                    if(!selectionValue.empty()){
                                        dicomSr->addProperty(selectionValue,DSRTypes::AM_afterCurrent);
                                    }
                                }
                                dicomSr->addCordinates(lvLinear[i][member],lvLinear[i][member][SOPUID].asString());
                                doc->getTree().goUp();
                                isGoUpDone = true;
                            }
                        }

                    if(member == FS){
                        if(lvLinear[i][member].isMember("value")){
                            string value = lvLinear[i][member]["value"].asString();
                            const auto measurementName=DSRCodedEntryValue("18051-3", "LN", "Left Ventricular Fractional Shortening");
                            LOGSR("%s Found FS Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                            const auto measurementValue=DSRNumericMeasurementValue(value, CODE_US2_PECENT);
                            dicomSr->addMeasurementEncoding(isBelowCurrentNeeded,measurementName,measurementValue,lvLinear[i][member]);
                            doc->getTree().goUp();
                            isGoUpDone = true;
                        }
                    }

                    if(member == MM_FS){
                        if(lvLinear[i][member].isMember("value")){
                            string value = lvLinear[i][member]["value"].asString();
                            const auto measurementName=DSRCodedEntryValue("29435-5", "LN", "Left ventricular Fractional shortening minor axis by US.M-mode");
                            LOGSR("%s Found MM FS Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                            const auto measurementValue=DSRNumericMeasurementValue(value, CODE_US2_PECENT);
                            dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,lvLinear[i][member], CODE_IMAGING_MODE_M);
                            doc->getTree().goUp();
                            isGoUpDone = true;
                        }
                    }

                    if(member == LVOTD){
                        if(lvLinear[i][member].isMember("value")){
                            string value = lvLinear[i][member]["value"].asString();
                            const auto measurementName=DSRCodedEntryValue("80028-4", "LN", "Left ventricular outflow tract dimension (2D)");
                            const auto unitIs= lvLinear[i][member]["unit"].asString();
                            LOGSR("%s Found LVOTD Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                            const auto measurementValue=DSRNumericMeasurementValue(value, dicomSr->getDistBasicMeasurementUnit(unitIs));
                            dicomSr->addMeasurementEncoding(isBelowCurrentNeeded,measurementName,measurementValue,lvLinear[i][member]);
                            doc->getTree().goUp();
                            isGoUpDone = true;
                        }
                    }

                    if(member == LVOT_VTI){
                        if(lvLinear[i][member].isMember("value")){
                            string value = lvLinear[i][member]["value"].asString();
                            const auto measurementName=DSRCodedEntryValue("80030-0", "LN", "Left ventricular outflow tract VTI");
                            LOGSR("%s Found LVOT_VTI Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                            const auto measurementValue=DSRNumericMeasurementValue(value, CODE_UCUM_Centimeter);
                            dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,lvLinear[i][member],CODE_IMAGING_MODE_PW);
                            doc->getTree().goUp();
                            isGoUpDone = true;
                        }
                    }


                    if(member == LVOT_VTI_VMAX){
                        string value = lvLinear[i][member]["value"].asString();
                        const auto measurementName=DSRCodedEntryValue("80029-2", "LN", "Left ventricular outflow tract Vmax");
                        const auto unitIs= lvLinear[i][member]["unit"].asString();
                        LOGSR("%s Found LVOT_VTI_VMAX Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                        auto measurementValue=DSRNumericMeasurementValue(value, dicomSr->getVelBasicMeasurementUnit(unitIs));
                        dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,lvLinear[i][member],CODE_IMAGING_MODE_PW);
                        doc->getTree().goUp();
                        isGoUpDone = true;
                    }

                    if(member == LVOT_VTI_VMIN){
                        string value = lvLinear[i][member]["value"].asString();
                        const auto measurementName=DSRCodedEntryValue("EN-262", "99EN", "LVOT Mean Velocity");
                        const auto unitIs= lvLinear[i][member]["unit"].asString();
                        LOGSR("%s Found LVOT_VTI_VMIN Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                        auto measurementValue=DSRNumericMeasurementValue(value, dicomSr->getVelBasicMeasurementUnit(unitIs));
                        dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,lvLinear[i][member],CODE_IMAGING_MODE_PW);
                        doc->getTree().goUp();
                        isGoUpDone = true;
                    }

                    if(member == LVOT_VTI_MAXPG){
                        string value = lvLinear[i][member]["value"].asString();
                        const auto measurementName=DSRCodedEntryValue("EN-263", "99EN", "LVOT Peak Gradient");
                        LOGSR("%s Found LVOT_VTI_MAXPG Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                        const auto measurementValue=DSRNumericMeasurementValue(value, CODE_UCUM_MMHG);
                        dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,lvLinear[i][member],CODE_IMAGING_MODE_PW);
                        doc->getTree().goUp();
                        isGoUpDone = true;
                    }


                    if(member == LVOT_VTI_MEANPG){
                        string value = lvLinear[i][member]["value"].asString();
                        const auto measurementName=DSRCodedEntryValue("EN-264", "99EN", "LVOT Mean Gradient");
                        LOGSR("%s Found LVOT_VTI_MEANPG Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                        const auto measurementValue=DSRNumericMeasurementValue(value, CODE_UCUM_MMHG);
                        dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,lvLinear[i][member],CODE_IMAGING_MODE_PW);
                        doc->getTree().goUp();
                        isGoUpDone = true;
                    }

                }

                if(isSiteAdded && i==0){
                    isBelowCurrentNeeded = false;
                }

            }

        }

        if(leftVentricle.isMember(COPROPER)){
            Json::Value lvCoProp = leftVentricle[COPROPER];
            auto isBelowCurrentNeeded = false;
            if(isLVRootSet && !isGoUpDone){
                doc->getTree().goUp();
            }{
                if(!isLVRootSet){
                    dicomSr->addFindingSite();
                    isLVRootSet = true;
                    isSiteAdded = true;
                    isBelowCurrentNeeded = true;
                }
            }

            for (Json::Value::ArrayIndex i = 0; i != lvCoProp.size(); i++) {
                Json::Value::Members members = lvCoProp[i].getMemberNames();
                for(auto & member : members){
                    if(member == LVSV){
                        if(lvCoProp[i][member].isMember("value")){
                            string value = lvCoProp[i][member]["value"].asString();
                            const auto lvLVSVCodeValue=DSRCodedEntryValue("F-32120", "SRT", "Stroke Volume");
                            LOGSR("%s Found LVSV Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                            const auto lvLVSVNumValue=DSRNumericMeasurementValue(value, CODE_UCUM_ml);
                            dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,lvLVSVCodeValue,lvLVSVNumValue,lvCoProp[i][member],CODE_IMAGING_MODE_2D);
                            doc->getTree().goUp();
                        }
                    }

                    if(member == CO){
                        if(lvCoProp[i][member].isMember("value")){
                            string value = lvCoProp[i][member]["value"].asString();
                            const auto lvLVSVCodeValue=DSRCodedEntryValue("8741-1", "LN", "Left Ventricular Cardiac Output");
                            LOGSR("%s Found CO Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                            const auto lvLVSVNumValue=DSRNumericMeasurementValue(value, CODE_UCUM_LMIN);
                            dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,lvLVSVCodeValue,lvLVSVNumValue,lvCoProp[i][member],CODE_IMAGING_MODE_2D);
                            doc->getTree().goUp();
                        }
                    }


                    if(member == STROKEVOLINDEX){
                        if(lvCoProp[i][member].isMember("value")){
                            string value = lvCoProp[i][member]["value"].asString();
                            const auto lvLVSVCodeValue=DSRCodedEntryValue("F-00078", "SRT", "Stroke Index");
                            LOGSR("%s Found STROKEVOLINDEX Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                            const auto lvLVSVNumValue=DSRNumericMeasurementValue(value, CODE_UCUM_MLM2);
                            dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,lvLVSVCodeValue,lvLVSVNumValue,lvCoProp[i][member],CODE_IMAGING_MODE_2D);
                            doc->getTree().goUp();
                        }
                    }

                }
                if(isSiteAdded && i==0){
                    isBelowCurrentNeeded = false;
                }
            }

        }

        if(leftVentricle.isMember(LEFT_VENT_OTHER)){
            Json::Value lvCoProp = leftVentricle[LEFT_VENT_OTHER];
            auto isBelowCurrentNeeded = false;
            if(isLVRootSet && !isGoUpDone){
                doc->getTree().goUp();
            }else{
                if(!isLVRootSet){
                    //Json::Value lvVolume = leftVentricle["Left Ventricle Volume"];
                    dicomSr->addFindingSite();
                    isLVRootSet = true;
                    isSiteAdded = true;
                    isBelowCurrentNeeded = true;
                }
            }
            for (Json::Value::ArrayIndex i = 0; i != lvCoProp.size(); i++) {
                Json::Value::Members members = lvCoProp[i].getMemberNames();
                for(auto & member : members){
                    if(member == ELATERAl){
                        if(lvCoProp[i][member].isMember("value")){
                            string value = lvCoProp[i][member]["value"].asString();
                            const auto measurementName=DSRCodedEntryValue("80054-0", "LN", "Mitral lateral e-prime Vmax");
                            const auto unitIs= lvCoProp[i][member]["unit"].asString();
                            LOGSR("%s Found ELATERAl Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                            auto measurementValue=DSRNumericMeasurementValue(value, dicomSr->getVelBasicMeasurementUnit(unitIs));
                            dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,lvCoProp[i][member],CODE_IMAGING_MODE_TDI);
                            doc->getTree().goUp();
                        }
                    }

                    if(member == ALATERAl){
                        if(lvCoProp[i][member].isMember("value")){
                            string value = lvCoProp[i][member]["value"].asString();
                            const auto measurementName=DSRCodedEntryValue("EN-16", "99EN", "a' Lateral");
                            const auto unitIs= lvCoProp[i][member]["unit"].asString();
                            LOGSR("%s Found ALATERAl Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                            auto measurementValue=DSRNumericMeasurementValue(value, dicomSr->getVelBasicMeasurementUnit(unitIs));
                            dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,lvCoProp[i][member],CODE_IMAGING_MODE_TDI);
                            doc->getTree().goUp();
                        }
                    }

                    if(member == ESEPTAL){
                        if(lvCoProp[i][member].isMember("value")){
                            string value = lvCoProp[i][member]["value"].asString();
                            const auto measurementName=DSRCodedEntryValue("78185-6", "LN", "Mitral valve medial annulus Tissue velocity.E-wave.max");
                            const auto unitIs= lvCoProp[i][member]["unit"].asString();
                            LOGSR("%s Found ESEPTAL Value found member is %s value is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                            auto measurementValue=DSRNumericMeasurementValue(value, dicomSr->getVelBasicMeasurementUnit(unitIs));
                            dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,lvCoProp[i][member],CODE_IMAGING_MODE_TDI);
                            doc->getTree().goUp();
                        }
                    }

                    if(member == ASEPTAL){
                        if(lvCoProp[i][member].isMember("value")){
                            string value = lvCoProp[i][member]["value"].asString();
                            const auto measurementName=DSRCodedEntryValue("EN-47", "99EN", "a' Septal");
                            const auto unitIs= lvCoProp[i][member]["unit"].asString();
                            LOGSR("%s Found ASEPTAL Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                            auto measurementValue=DSRNumericMeasurementValue(value, dicomSr->getVelBasicMeasurementUnit(unitIs));
                            dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,lvCoProp[i][member],CODE_IMAGING_MODE_TDI);
                            doc->getTree().goUp();
                        }
                    }
                    if(member == EELATERAL){
                        if(lvCoProp[i][member].isMember("value")){
                            string value = lvCoProp[i][member]["value"].asString();
                            const auto measurementName=DSRCodedEntryValue("G-037B", "SRT", "Ratio of MV Peak Velocity to LV Peak Tissue Velocity E-Wave");
                            LOGSR("%s Found EELATERAL Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                            const auto measurementValue=DSRNumericMeasurementValue(value, CODE_UCUM_Ratio);
                            dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,lvCoProp[i][member],CODE_IMAGING_MODE_TDI);
                            doc->getTree().goUp();
                        }
                    }

                    if(member == SLATERAL){
                        if(lvCoProp[i][member].isMember("value")){
                            string value = lvCoProp[i][member]["value"].asString();
                            const auto measurementName=DSRCodedEntryValue("G-037D", "SRT", "Left Ventricular Peak Systolic Tissue Velocity");
                            const auto unitIs= lvCoProp[i][member]["unit"].asString();
                            LOGSR("%s Found SLATERAL Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                            auto measurementValue=DSRNumericMeasurementValue(value, dicomSr->getVelBasicMeasurementUnit(unitIs));
                            dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,lvCoProp[i][member],CODE_IMAGING_MODE_TDI);
                            doc->getTree().goUp();
                        }
                    }

                    if(member == SSEPTAL) {
                        if (lvCoProp[i][member].isMember("value")) {
                            string value = lvCoProp[i][member]["value"].asString();
                            const auto measurementName = DSRCodedEntryValue("EN-57", "99EN",
                                                                            "s' Septal");
                            const auto unitIs= lvCoProp[i][member]["unit"].asString();
                            LOGSR("%s Found SSEPTAL Value found member is %s value is %s unit is %s", TAG,
                                  member.c_str(), value.c_str(), unitIs.c_str());

                            auto measurementValue=DSRNumericMeasurementValue(value, dicomSr->getVelBasicMeasurementUnit(unitIs));
                            dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,
                                                                measurementName, measurementValue,
                                                                lvCoProp[i][member],
                                                                CODE_IMAGING_MODE_TDI);
                            doc->getTree().goUp();
                        }
                    }



                    if(member == EESEPTAL){
                        if(lvCoProp[i][member].isMember("value")){
                            string value = lvCoProp[i][member]["value"].asString();
                            const auto measurementName=DSRCodedEntryValue("EN-1246", "99EN", "E Velocity to e' Septal Velocity Ratio");
                            LOGSR("%s Found EELATERAL Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                            const auto measurementValue=DSRNumericMeasurementValue(value, CODE_UCUM_Ratio);
                            dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,lvCoProp[i][member],CODE_IMAGING_MODE_TDI);
                            doc->getTree().goUp();
                        }
                    }

                    if(member == HR){
                        if(lvCoProp[i][member].isMember("value")){
                            string value = lvCoProp[i][member]["value"].asString();
                            const auto measurementName=DSRCodedEntryValue("8867-4", "LN", "Heart rate");
                            LOGSR("%s Found HR Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                            const auto measurementValue=DSRNumericMeasurementValue(value, CODE_BMP);
                            dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,lvCoProp[i][member],CODE_IMAGING_MODE_M);
                            doc->getTree().goUp();
                        }
                    }



                    if(member == MAPSE){
                        if(lvCoProp[i][member].isMember("value")){
                            string value = lvCoProp[i][member]["value"].asString();
                            const auto measurementName=DSRCodedEntryValue("EN-36", "99EN", "Mitral Annular Plane Systolic Excursion (MAPSE)");
                            const auto unitIs= lvCoProp[i][member]["unit"].asString();
                            LOGSR("%s Found MAPSE Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                            auto measurementValue=DSRNumericMeasurementValue(value, dicomSr->getDistBasicMeasurementUnit(unitIs));
                            dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,lvCoProp[i][member],CODE_IMAGING_MODE_M);
                            doc->getTree().goUp();
                        }
                    }



                    if(member == RAP_M){
                        if(lvCoProp[i][member].isMember("value")){
                            string value = lvCoProp[i][member]["value"].asString();
                            const auto measurementName=DSRCodedEntryValue("EN-50", "99EN", "Right Atrial Pressure");
                            LOGSR("%s Found RAP_M Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                            const auto measurementValue=DSRNumericMeasurementValue(value, CODE_UCUM_MMHG);
                            dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,lvCoProp[i][member],CODE_IMAGING_MODE_M);
                            doc->getTree().goUp();
                        }
                    }

                    if(member == LVMASS){
                        if(lvCoProp[i][member].isMember("value")){
                            string value = lvCoProp[i][member]["value"].asString();
                            const auto measurementName=DSRCodedEntryValue("80019-3", "LN", "Left ventricular mass (dimension method) 2D");
                            LOGSR("%s Found LVMASS Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                            const auto measurementValue=DSRNumericMeasurementValue(value, CODE_UCUM_G);
                            dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,lvCoProp[i][member],CODE_IMAGING_MODE_2D);
                            doc->getTree().goUp();
                        }
                    }

                    if(member == MM_LV_MASS){
                        if(lvCoProp[i][member].isMember("value")){
                            string value = lvCoProp[i][member]["value"].asString();
                            const auto measurementName=DSRCodedEntryValue("80022-7", "LN", "Left ventricular Myocardial mass at end diastole by US.M-mode+Calculated by dimension method");
                            LOGSR("%s Found LVMASS Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                            const auto measurementValue=DSRNumericMeasurementValue(value, CODE_UCUM_G);
                            dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,lvCoProp[i][member],CODE_IMAGING_MODE_M);
                            doc->getTree().goUp();
                        }
                    }

                    if(member == LVMASSINDEX){
                        if(lvCoProp[i][member].isMember("value")){
                            string value = lvCoProp[i][member]["value"].asString();
                            const auto measurementName=DSRCodedEntryValue("80020-1", "LN", "Left ventricular mass (dimension method) 2D / BSA");
                            LOGSR("%s Found LVMASSINDEX Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                            const auto measurementValue=DSRNumericMeasurementValue(value, CODE_UCUM_GM2);
                            dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,lvCoProp[i][member],CODE_IMAGING_MODE_2D);
                            doc->getTree().goUp();
                        }
                    }

                    if(member == MM_LV_MASS_INDEX){
                        if(lvCoProp[i][member].isMember("value")){
                            string value = lvCoProp[i][member]["value"].asString();
                            const auto measurementName=DSRCodedEntryValue("80023-5", "LN", "Left ventricular Myocardial mass at end diastole (by US.M-mode+Calculated by dimension method)/Body surface area");
                            LOGSR("%s Found MM LVMASS INDEX Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                            const auto measurementValue=DSRNumericMeasurementValue(value, CODE_UCUM_GM2);
                            dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,lvCoProp[i][member],CODE_IMAGING_MODE_M);
                            doc->getTree().goUp();
                        }
                    }

                    if(member == RWT){
                        if(lvCoProp[i][member].isMember("value")){
                            string value = lvCoProp[i][member]["value"].asString();
                            const auto measurementName=DSRCodedEntryValue("EN-234", "99EN", "Relative wall thickness");
                            LOGSR("%s Found RWT Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                            const auto measurementValue=DSRNumericMeasurementValue(value, CODE_UCUM_Ratio);
                            dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,lvCoProp[i][member],CODE_IMAGING_MODE_2D);
                            doc->getTree().goUp();
                        }
                    }

                    if(member == MM_RWT){
                        if(lvCoProp[i][member].isMember("value")){
                            string value = lvCoProp[i][member]["value"].asString();
                            const auto measurementName=DSRCodedEntryValue("EN-66798", "99EN", "Relative Wall Thickness- M-mode");
                            LOGSR("%s Found MM RWT Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                            const auto measurementValue=DSRNumericMeasurementValue(value, CODE_UCUM_Ratio);
                            dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,lvCoProp[i][member],CODE_IMAGING_MODE_M);
                            doc->getTree().goUp();
                        }
                    }

                    if(member == MM_LA_AO){
                        if(lvCoProp[i][member].isMember("value")){
                            string value = lvCoProp[i][member]["value"].asString();
                            const auto measurementName=DSRCodedEntryValue("17985-3", "LN", "Left Atrium to Aortic Root Ratio");
                            LOGSR("%s Found MM RWT Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                            const auto measurementValue=DSRNumericMeasurementValue(value, CODE_UCUM_Ratio);
                            dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,lvCoProp[i][member],CODE_IMAGING_MODE_M);
                            doc->getTree().goUp();
                        }
                    }
                    //E Velocity to e' Lateral Velocity Ratio
                }

                if(isSiteAdded && i==0){
                    isBelowCurrentNeeded = false;
                }
            }

        }


    }

    if(root.isMember(RV)){
        if(isLVRootSet){
            doc->getTree().goUp();
        }else if(isRootSet){
            doc->getTree().goUp();
        }
        doc->getTree().goUp();
        doc->getTree().goUp();

        doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Container, DSRTypes::AM_belowCurrent);
        doc->getTree().getCurrentContentItem().setConceptName(CODE_DCM_RETIRED_Findings);
        doc->getTree().getCurrentContentItem().setContinuityOfContent(DSRTypes::COC_Separate);

        doc->getTree().addContentItem(DSRTypes::RT_hasConceptMod,  DSRTypes::VT_Code,DSRTypes::AM_belowCurrent);
        doc->getTree().getCurrentContentItem().setConceptName(CODE_SRT_FindingSite);
        doc->getTree().getCurrentContentItem().setCodeValue(DSRCodedEntryValue("T-32500", "SRT", "Right Ventricle"));

        doc->getTree().getCurrentContentItem().setContinuityOfContent(DSRTypes::COC_Separate);

        doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Container);
        doc->getTree().getCurrentContentItem().setConceptName(CODE_DCM_MeasurementGroup);
        Json::Value rightVentricle = root[RV];
        isRootSet=true;
        auto isBelowCurrentNeeded = true;
        for (Json::Value::ArrayIndex i = 0; i != rightVentricle.size(); i++) {
            Json::Value::Members members = rightVentricle[i].getMemberNames();

            for(auto & member : members){
                if(member == RVIDD){
                    string value = rightVentricle[i][member]["value"].asString();
                    const auto rvRVIDDCodeValue=DSRCodedEntryValue("20304-2", "LN", "Right Ventricular Internal Diastolic Dimension");
                    const auto unitIs= rightVentricle[i][member]["unit"].asString();
                    LOGSR("%s Found RVIDD Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                    auto measurementValue =DSRNumericMeasurementValue(value, dicomSr->getDistBasicMeasurementUnit(unitIs));
                    if(isBelowCurrentNeeded){
                        dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent,rvRVIDDCodeValue,measurementValue);
                    }else{
                        dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent,rvRVIDDCodeValue,measurementValue);
                    }

                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_2D);
                    dicomSr->addCordinates(rightVentricle[i][member],rightVentricle[i][member][SOPUID].asString());
                    doc->getTree().goUp();
                }

                if(member == MM_RVIDD) {
                    string value = rightVentricle[i][member]["value"].asString();
                    const auto rvRVIDDCodeValue=DSRCodedEntryValue("EN-6678433", "99EN", "RV ED minor axis M-mode");
                    const auto unitIs= rightVentricle[i][member]["unit"].asString();
                    LOGSR("%s Found MM RVIDD Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                    auto measurementValue =DSRNumericMeasurementValue(value, dicomSr->getDistBasicMeasurementUnit(unitIs));
                    if(isBelowCurrentNeeded){
                        dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent,rvRVIDDCodeValue,measurementValue);
                    }else{
                        dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent,rvRVIDDCodeValue,measurementValue);
                    }

                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_M);
                    dicomSr->addCordinates(rightVentricle[i][member],rightVentricle[i][member][SOPUID].asString());
                    doc->getTree().goUp();
                }

                if(member == RV_BASAL){
                    string value = rightVentricle[i][member]["value"].asString();
                    const auto measurementName=DSRCodedEntryValue("80080-5", "LN", "Right ventricular basal dimension 4C");
                    const auto unitIs= rightVentricle[i][member]["unit"].asString();
                    LOGSR("%s Found RV_BASAL Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                    auto measurementValue =DSRNumericMeasurementValue(value, dicomSr->getDistBasicMeasurementUnit(unitIs));
                    dicomSr->addMeasurementEncoding(isBelowCurrentNeeded,measurementName,measurementValue,rightVentricle[i][member]);
                    doc->getTree().goUp();
                }

                if(member == RV_MID){
                    string value = rightVentricle[i][member]["value"].asString();
                    const auto measurementName=DSRCodedEntryValue("80085-4", "LN", "Right ventricular mid-cavity dimension 4C");
                    const auto unitIs= rightVentricle[i][member]["unit"].asString();
                    LOGSR("%s Found RV_MID Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                    auto measurementValue =DSRNumericMeasurementValue(value, dicomSr->getDistBasicMeasurementUnit(unitIs));
                    dicomSr->addMeasurementEncoding(isBelowCurrentNeeded,measurementName,measurementValue,rightVentricle[i][member]);
                    doc->getTree().goUp();
                }

                if(member == RV_LENGTH){
                    string value = rightVentricle[i][member]["value"].asString();
                    const auto measurementName=DSRCodedEntryValue("EN-9", "99EN", "RV Length");
                    const auto unitIs= rightVentricle[i][member]["unit"].asString();
                    LOGSR("%s Found RV_LENGTH Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                    auto measurementValue =DSRNumericMeasurementValue(value, dicomSr->getDistBasicMeasurementUnit(unitIs));
                    dicomSr->addMeasurementEncoding(isBelowCurrentNeeded,measurementName,measurementValue,rightVentricle[i][member]);
                    doc->getTree().goUp();
                }

                if(member == PVACT){
                    string value = rightVentricle[i][member]["value"].asString();
                    const auto measurementName=DSRCodedEntryValue("79928-8", "LN", "Pulmonic valve acceleration time");
                    LOGSR("%s Found PVACT Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                    const auto measurementValue=DSRNumericMeasurementValue(value, CODE_UCUM_mis);
                    dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,rightVentricle[i][member],CODE_IMAGING_MODE_PW);
                    doc->getTree().goUp();
                 }


                if(member == PAEDP){
                    string value = rightVentricle[i][member]["value"].asString();
                    const auto measurementName=DSRCodedEntryValue("79918-9", "LN", "Pulmonic regurgitation end diastolic velocity");
                    const auto unitIs= rightVentricle[i][member]["unit"].asString();
                    LOGSR("%s Found PAEDP Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                    auto measurementValue=DSRNumericMeasurementValue(value, dicomSr->getVelBasicMeasurementUnit(unitIs));
                    dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,rightVentricle[i][member],CODE_IMAGING_MODE_CW);
                    doc->getTree().goUp();
                }


                if(member == TR){
                    string value = rightVentricle[i][member]["value"].asString();
                    const auto measurementName=DSRCodedEntryValue("79921-3", "LN", "Tricuspid regurgitation Vmax");
                    const auto unitIs= rightVentricle[i][member]["unit"].asString();
                    LOGSR("%s Found TR Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                    auto measurementValue=DSRNumericMeasurementValue(value, dicomSr->getVelBasicMeasurementUnit(unitIs));
                    dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,rightVentricle[i][member],CODE_IMAGING_MODE_CW);
                    doc->getTree().goUp();
                }



                if(member == TVS){
                    string value = rightVentricle[i][member]["value"].asString();
                    const auto measurementName=DSRCodedEntryValue("79926-2", "LN", "Tricuspid valve s-prime Vmax");
                    const auto unitIs= rightVentricle[i][member]["unit"].asString();
                    LOGSR("%s Found TVS Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                    auto measurementValue=DSRNumericMeasurementValue(value, dicomSr->getVelBasicMeasurementUnit(unitIs));
                    dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,rightVentricle[i][member],CODE_IMAGING_MODE_TDI);
                    doc->getTree().goUp();
                }

                if(member == TRPG){
                    string value = rightVentricle[i][member]["value"].asString();
                    const auto measurementName=DSRCodedEntryValue("79937-9", "LN", "Tricuspid regurgitation peak gradient");
                    LOGSR("%s Found TRPG Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                    const auto measurementValue=DSRNumericMeasurementValue(value, CODE_UCUM_MMHG);
                    dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,rightVentricle[i][member],CODE_IMAGING_MODE_CW);
                    doc->getTree().goUp();
                }


                if(member == RVSP){
                    string value = rightVentricle[i][member]["value"].asString();
                    const auto measurementName=DSRCodedEntryValue("EN-395", "99EN", "Right Ventricular Systolic Pressure");
                    LOGSR("%s Found RVSP Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                    const auto measurementValue=DSRNumericMeasurementValue(value, CODE_UCUM_MMHG);
                    dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,rightVentricle[i][member],CODE_IMAGING_MODE_CW);
                    doc->getTree().goUp();
                }

                if(member == PR){
                    string value = rightVentricle[i][member]["value"].asString();
                    const auto measurementName=DSRCodedEntryValue("79919-7", "LN", "Pulmonic regurgitation Vmax");
                    const auto unitIs= rightVentricle[i][member]["unit"].asString();
                    LOGSR("%s Found PR Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                    auto measurementValue=DSRNumericMeasurementValue(value, dicomSr->getVelBasicMeasurementUnit(unitIs));
                    dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,rightVentricle[i][member],CODE_IMAGING_MODE_CW);
                    doc->getTree().goUp();
                }

                if(member == TAPSE){
                    if(rightVentricle[i][member].isMember("value")){
                        string value = rightVentricle[i][member]["value"].asString();
                        const auto measurementName=DSRCodedEntryValue("77903-3", "LN", "Tricuspid Annular Plane Systolic Excursion (TAPSE)");
                        const auto unitIs= rightVentricle[i][member]["unit"].asString();
                        LOGSR("%s Found TAPSE Value found member is %s value is %s unit is %s", TAG,member.c_str(),value.c_str(), unitIs.c_str());
                        auto measurementValue=DSRNumericMeasurementValue(value, dicomSr->getDistBasicMeasurementUnit(unitIs));
                        dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,rightVentricle[i][member],CODE_IMAGING_MODE_M);
                        doc->getTree().goUp();
                    }
                }
            }
            if(i==0){
                isBelowCurrentNeeded = false;
            }

        }
    }

    if(root.isMember(LA)){
        if(isLVRootSet){
            doc->getTree().goUp();
        }else if(isRootSet){
            isRootSet = true;
        }
        doc->getTree().goUp();
        doc->getTree().goUp();

        doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Container, DSRTypes::AM_belowCurrent);
        doc->getTree().getCurrentContentItem().setConceptName(CODE_DCM_RETIRED_Findings);
        doc->getTree().getCurrentContentItem().setContinuityOfContent(DSRTypes::COC_Separate);

        doc->getTree().addContentItem(DSRTypes::RT_hasConceptMod,  DSRTypes::VT_Code,DSRTypes::AM_belowCurrent);
        doc->getTree().getCurrentContentItem().setConceptName(CODE_SRT_FindingSite);
        doc->getTree().getCurrentContentItem().setCodeValue(DSRCodedEntryValue("T-32300", "SRT", LA));

        doc->getTree().getCurrentContentItem().setContinuityOfContent(DSRTypes::COC_Separate);

        doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Container);
        doc->getTree().getCurrentContentItem().setConceptName(CODE_DCM_MeasurementGroup);
        Json::Value leftAtrium = root[LA];
        isRootSet = true;
        auto isBelowCurrentNeeded = true;
        for (Json::Value::ArrayIndex i = 0; i != leftAtrium.size(); i++) {
            Json::Value::Members members = leftAtrium[i].getMemberNames();

            for(auto & member : members) {
                auto dsrType = DSRTypes::AM_belowCurrent;
                 if(isBelowCurrentNeeded){
                     dsrType = DSRTypes::AM_belowCurrent;
                }else{
                     dsrType = DSRTypes::AM_afterCurrent;
                }
                if (member == LAESV) {

                    string value = leftAtrium[i][member]["value"].asString();
                    const auto laLAESVCodeValue=DSRCodedEntryValue("17977-0", "LN", "Left Atrium Area A4C view");
                    LOGSR("%s Found LAESV Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                    const auto laRLAESVNumValue=DSRNumericMeasurementValue(value, CODE_UCUM_ml);
                    dicomSr->addNumMeasurement(dsrType,laLAESVCodeValue,laRLAESVNumValue);
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_2D);
                    doc->getTree().goUp();
                }

                if (member == LADIAM) {

                    string value = leftAtrium[i][member]["value"].asString();
                    const auto measurementName=DSRCodedEntryValue("29469-4", "LN", "Left Atrium Antero-posterior Systolic Dimension");
                    const auto unitIs= leftAtrium[i][member]["unit"].asString();
                    LOGSR("%s Found LADIAM Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                    auto measurementValue =DSRNumericMeasurementValue(value, dicomSr->getDistBasicMeasurementUnit(unitIs));
                    dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,leftAtrium[i][member],CODE_IMAGING_MODE_2D);
                    doc->getTree().goUp();
                }

                if (member == MM_LA) {

                    string value = leftAtrium[i][member]["value"].asString();
                    const auto measurementName=DSRCodedEntryValue("18024-0", "LN", "Left atrial Diameter anterior-posterior systole by US.M-mode");
                    const auto unitIs= leftAtrium[i][member]["unit"].asString();
                    LOGSR("%s Found MM LA Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                    auto measurementValue =DSRNumericMeasurementValue(value, dicomSr->getDistBasicMeasurementUnit(unitIs));
                    dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,leftAtrium[i][member],CODE_IMAGING_MODE_M);
                    doc->getTree().goUp();
                }

                if(i==0){
                    isBelowCurrentNeeded = false;
                }
            }
        }

    }

    if(root.isMember(RA)){
        if(isLVRootSet){
            doc->getTree().goUp();
        }else if(isRootSet){
            doc->getTree().goUp();
        }

        doc->getTree().goUp();
        doc->getTree().goUp();
        doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Container, DSRTypes::AM_belowCurrent);
        doc->getTree().getCurrentContentItem().setConceptName(CODE_DCM_RETIRED_Findings);
        doc->getTree().getCurrentContentItem().setContinuityOfContent(DSRTypes::COC_Separate);

        doc->getTree().addContentItem(DSRTypes::RT_hasConceptMod,  DSRTypes::VT_Code,DSRTypes::AM_belowCurrent);
        doc->getTree().getCurrentContentItem().setConceptName(CODE_SRT_FindingSite);
        doc->getTree().getCurrentContentItem().setCodeValue(DSRCodedEntryValue("T-32200", "SRT", RA));

        doc->getTree().getCurrentContentItem().setContinuityOfContent(DSRTypes::COC_Separate);

        doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Container);
        doc->getTree().getCurrentContentItem().setConceptName(CODE_DCM_MeasurementGroup);
        Json::Value rightAtrium = root[RA];
        isRootSet= true;
        auto isBelowCurrentNeeded = true;
        for (Json::Value::ArrayIndex i = 0; i != rightAtrium.size(); i++) {
            Json::Value::Members members = rightAtrium[i].getMemberNames();

            for(auto & member : members) {
                auto dsrType = DSRTypes::AM_belowCurrent;
                if(isBelowCurrentNeeded){
                    dsrType = DSRTypes::AM_belowCurrent;
                }else{
                    dsrType = DSRTypes::AM_afterCurrent;
                }
                if (member == RAAREA) {
                    string value = rightAtrium[i][member]["value"].asString();
                    const auto raRAAREACodeValue=DSRCodedEntryValue("17988-7", "LN", "Right Atrium Area A4C view");
                    LOGSR("%s Found RAAREA Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                    const auto raRAAREANumValue=DSRNumericMeasurementValue(value, CODE_UCUM_cm2);
                    dicomSr->addNumMeasurement(dsrType,raRAAREACodeValue,raRAAREANumValue);
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_2D);
                    dicomSr->addCordinates(rightAtrium[i][member],rightAtrium[i][member][SOPUID].asString());
                    doc->getTree().goUp();
                }



                if (member == RAP_2D) {
                    string value = rightAtrium[i][member]["value"].asString();
                    const auto measurementName=DSRCodedEntryValue("EN-50", "99EN", "Right Atrial Pressure");
                    LOGSR("%s Found RAP_2D Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                    const auto measurementValue=DSRNumericMeasurementValue(value, CODE_UCUM_MMHG);
                    dicomSr->addMeasurementEncoding(isBelowCurrentNeeded,measurementName,measurementValue,rightAtrium[i][member]);
                    doc->getTree().goUp();
                }

                if(i==0){
                    isBelowCurrentNeeded = false;
                }
            }
        }
    }

    if(root.isMember(MV)){
        if(isLVRootSet){
            doc->getTree().goUp();
        }else if(isRootSet){
            doc->getTree().goUp();
        }
        doc->getTree().goUp();
        doc->getTree().goUp();

        doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Container, DSRTypes::AM_belowCurrent);
        doc->getTree().getCurrentContentItem().setConceptName(CODE_DCM_RETIRED_Findings);
        doc->getTree().getCurrentContentItem().setContinuityOfContent(DSRTypes::COC_Separate);

        doc->getTree().addContentItem(DSRTypes::RT_hasConceptMod,  DSRTypes::VT_Code,DSRTypes::AM_belowCurrent);
        doc->getTree().getCurrentContentItem().setConceptName(CODE_SRT_FindingSite);
        doc->getTree().getCurrentContentItem().setCodeValue(DSRCodedEntryValue("T-35300", "SRT", MV));

        doc->getTree().getCurrentContentItem().setContinuityOfContent(DSRTypes::COC_Separate);

        doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Container);
        doc->getTree().getCurrentContentItem().setConceptName(CODE_DCM_MeasurementGroup);
        isRootSet = true;
        Json::Value mitralValve = root[MV];//Mitral Valve
        auto isAfterCurrent = false;
        for (Json::Value::ArrayIndex i = 0; i != mitralValve.size(); i++) {
            Json::Value::Members members = mitralValve[i].getMemberNames();
            for (auto &member : members) {
                DSRTypes::E_AddMode dsrType = DSRTypes::AM_belowCurrent;
                if(isAfterCurrent){
                    dsrType = DSRTypes::AM_afterCurrent;
                }else{
                    dsrType = DSRTypes::AM_belowCurrent;
                }
                if(member == MVE){
                    string value = mitralValve[i][member]["value"].asString();
                    const auto mvECodeValue=DSRCodedEntryValue("18037-2", "LN", "Mitral Valve E-Wave Peak Velocity");
                    const auto unitIs= mitralValve[i][member]["unit"].asString();
                    LOGSR("%s Found MV-E Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                    auto mvENumValue=DSRNumericMeasurementValue(value, dicomSr->getVelBasicMeasurementUnit(unitIs));
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    if(isAfterCurrent){
                        dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent,mvECodeValue,mvENumValue);
                    }else{
                        dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent,mvECodeValue,mvENumValue);
                    }

                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_PW);
                    dicomSr->addCordinates(mitralValve[i][member],mitralValve[i][member][SOPUID].asString());
                    doc->getTree().goUp();

                }



                if(member == MVA){
                    string value = mitralValve[i][member]["value"].asString();
                    const auto mvACodeValue=DSRCodedEntryValue("17978-8", "LN", "Mitral Valve A-Wave Peak Velocity");
                    const auto unitIs= mitralValve[i][member]["unit"].asString();
                    LOGSR("%s Found MV-A Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                    auto mvANumValue=DSRNumericMeasurementValue(value, dicomSr->getVelBasicMeasurementUnit(unitIs));
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    if(isAfterCurrent){
                        dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent,mvACodeValue,mvANumValue);
                    }else{
                        dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent,mvACodeValue,mvANumValue);
                    }

                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_PW);
                    dicomSr->addCordinates(mitralValve[i][member],mitralValve[i][member][SOPUID].asString());
                    doc->getTree().goUp();

                }

                if(member == EARATIO){
                    string value = mitralValve[i][member]["value"].asString();
                    const auto eaRatioCodeValue=DSRCodedEntryValue("18038-0", "LN", "Mitral Valve E to A Ratio");
                    LOGSR("%s Found E A Ratio Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                    const auto eaRatioNumValue=DSRNumericMeasurementValue(value, CODE_UCUM_Ratio);
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    dicomSr->addNumMeasurement(dsrTypeFinal,eaRatioCodeValue,eaRatioNumValue);
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_PW);
                    doc->getTree().goUp();
                }

                if(member == DECT){
                    string value = mitralValve[i][member]["value"].asString();
                    const auto decTCodeValue=DSRCodedEntryValue("G-0384", "SRT", "Mitral Valve E-Wave Deceleration Time");
                    LOGSR("%s Found DecT Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                    const auto decTNumValue=DSRNumericMeasurementValue(value, CODE_UCUM_mis);
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    dicomSr->addNumMeasurement(dsrTypeFinal,decTCodeValue,decTNumValue);
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_PW);
                    dicomSr->addCordinates(mitralValve[i][member],mitralValve[i][member][SOPUID].asString());
                    doc->getTree().goUp();
                }

                if(member == EPSS){
                    string value = mitralValve[i][member]["value"].asString();
                    const auto epssCodeValue=DSRCodedEntryValue("18036-4", "SRT", "Mitral Valve EPSS, E wave");
                    const auto unitIs= mitralValve[i][member]["unit"].asString();
                    LOGSR("%s Found EPSS Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                    auto epssNumValue =DSRNumericMeasurementValue(value, dicomSr->getDistBasicMeasurementUnit(unitIs));
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    dicomSr->addNumMeasurement(dsrTypeFinal,epssCodeValue,epssNumValue);
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_M);
                    dicomSr->addCordinates(mitralValve[i][member],mitralValve[i][member][SOPUID].asString());
                    doc->getTree().goUp();
                }


                if(member == MVABYPHT){
                    string value = mitralValve[i][member]["value"].asString();
                    const auto decTCodeValue=DSRCodedEntryValue("80069-8", "LN", "Mitral valve area (Pressure Half-Time)");
                    LOGSR("%s Found MVABYPHT Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                    const auto decTNumValue=DSRNumericMeasurementValue(value, CODE_UCUM_cm2);
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    dicomSr->addNumMeasurement(dsrTypeFinal,decTCodeValue,decTNumValue);
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_CW);
                    dicomSr->addCordinates(mitralValve[i][member],mitralValve[i][member][SOPUID].asString());
                    doc->getTree().goUp();
                }

                if(member == MVPHT){
                    string value = mitralValve[i][member]["value"].asString();
                    const auto decTCodeValue=DSRCodedEntryValue("79912-2", "LN", "Mitral valve pressure half-time");
                    LOGSR("%s Found MVPHT Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                    const auto decTNumValue=DSRNumericMeasurementValue(value, CODE_UCUM_mis);
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    dicomSr->addNumMeasurement(dsrTypeFinal,decTCodeValue,decTNumValue);
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_CW);
                    dicomSr->addCordinates(mitralValve[i][member],mitralValve[i][member][SOPUID].asString());
                    doc->getTree().goUp();
                }


                if(member == MVSV){
                    string value = mitralValve[i][member]["value"].asString();
                    const auto decTCodeValue=DSRCodedEntryValue("EN-1119", "99EN", "Mitral Valve Stroke Volume");
                    LOGSR("%s Found MVSV Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                    const auto decTNumValue=DSRNumericMeasurementValue(value, CODE_UCUM_ml);
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    dicomSr->addNumMeasurement(dsrTypeFinal,decTCodeValue,decTNumValue);
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_CW);
                    dicomSr->addCordinates(mitralValve[i][member],mitralValve[i][member][SOPUID].asString());
                    doc->getTree().goUp();
                }

                if(member == MVVTI){
                    string value = mitralValve[i][member]["value"].asString();
                    const auto decTCodeValue=DSRCodedEntryValue("79914-8", "LN", "Mitral valve VTI");
                    LOGSR("%s Found MVVTI Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                    const auto decTNumValue=DSRNumericMeasurementValue(value, CODE_UCUM_Centimeter);
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    dicomSr->addNumMeasurement(dsrTypeFinal,decTCodeValue,decTNumValue);
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_CW);
                    dicomSr->addCordinates(mitralValve[i][member],mitralValve[i][member][SOPUID].asString());
                    doc->getTree().goUp();
                }

                if(member == MVVTIPW){
                    string value = mitralValve[i][member]["value"].asString();
                    const auto decTCodeValue=DSRCodedEntryValue("80053-2", "LN", "Mitral annulus VTI");
                    LOGSR("%s Found MVVTIPW Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                    const auto decTNumValue=DSRNumericMeasurementValue(value, CODE_UCUM_Centimeter);
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    dicomSr->addNumMeasurement(dsrTypeFinal,decTCodeValue,decTNumValue);
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_PW);
                    dicomSr->addCordinates(mitralValve[i][member],mitralValve[i][member][SOPUID].asString());
                    doc->getTree().goUp();
                }

                if(member == MVVTIPW_MAXPG){
                    string value = mitralValve[i][member]["value"].asString();
                    const auto decTCodeValue=DSRCodedEntryValue("EN-111", "99EN", "PW MV Peak Gradient");
                    LOGSR("%s Found MVVTIPW_MAXPG Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                    const auto decTNumValue=DSRNumericMeasurementValue(value, CODE_UCUM_MMHG);
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    dicomSr->addNumMeasurement(dsrTypeFinal,decTCodeValue,decTNumValue);
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_PW);
                    dicomSr->addCordinates(mitralValve[i][member],mitralValve[i][member][SOPUID].asString());
                    doc->getTree().goUp();
                }


                if(member == MVVTIPW_MEANPG){
                    string value = mitralValve[i][member]["value"].asString();
                    const auto decTCodeValue=DSRCodedEntryValue("EN-112", "99EN", "PW MV Mean Gradient");
                    LOGSR("%s Found MVVTIPW_MEANPG Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                    const auto decTNumValue=DSRNumericMeasurementValue(value, CODE_UCUM_MMHG);
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    dicomSr->addNumMeasurement(dsrTypeFinal,decTCodeValue,decTNumValue);
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_PW);
                    dicomSr->addCordinates(mitralValve[i][member],mitralValve[i][member][SOPUID].asString());
                    doc->getTree().goUp();
                }


                if(member == MVVTIPW_VMAX){
                    string value = mitralValve[i][member]["value"].asString();
                    const auto mvVtiPwVmaxCodeValue=DSRCodedEntryValue("EN-113", "99EN", "PW MV Peak Velocity");
                    const auto unitIs= mitralValve[i][member]["unit"].asString();
                    LOGSR("%s Found MVVTIPW_VMAX Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                    auto measurementValue=DSRNumericMeasurementValue(value, dicomSr->getVelBasicMeasurementUnit(unitIs));
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    dicomSr->addNumMeasurement(dsrTypeFinal,mvVtiPwVmaxCodeValue,measurementValue);
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_PW);
                    dicomSr->addCordinates(mitralValve[i][member],mitralValve[i][member][SOPUID].asString());
                    doc->getTree().goUp();
                }

                if(member == MVVTIPW_VMIN){
                    string value = mitralValve[i][member]["value"].asString();
                    const auto mvVtiPwVminCodeValue=DSRCodedEntryValue("EN-114", "99EN", "PW MV Mean Velocity");
                    const auto unitIs= mitralValve[i][member]["unit"].asString();
                    LOGSR("%s Found MVVTIPW_VMIN Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                    auto measurementValue=DSRNumericMeasurementValue(value, dicomSr->getVelBasicMeasurementUnit(unitIs));
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    dicomSr->addNumMeasurement(dsrTypeFinal,mvVtiPwVminCodeValue,measurementValue);
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_PW);
                    dicomSr->addCordinates(mitralValve[i][member],mitralValve[i][member][SOPUID].asString());
                    doc->getTree().goUp();
                }



                if(member == MVVTICW_MAXPG){
                    string value = mitralValve[i][member]["value"].asString();
                    const auto decTCodeValue=DSRCodedEntryValue("80074-8", "LN", "Mitral valve peak instantaneous gradient");
                    LOGSR("%s Found MVVTICW_MAXPG Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                    const auto decTNumValue=DSRNumericMeasurementValue(value, CODE_UCUM_MMHG);
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    dicomSr->addNumMeasurement(dsrTypeFinal,decTCodeValue,decTNumValue);
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_CW);
                    dicomSr->addCordinates(mitralValve[i][member],mitralValve[i][member][SOPUID].asString());
                    doc->getTree().goUp();
                }


                if(member == MVVTICW_MEANPG){
                    string value = mitralValve[i][member]["value"].asString();
                    const auto decTCodeValue=DSRCodedEntryValue("80073-0", "LN", "Mitral valve mean gradient");
                    LOGSR("%s Found MVVTIPW_MEANPG Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                    const auto decTNumValue=DSRNumericMeasurementValue(value, CODE_UCUM_MMHG);
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    dicomSr->addNumMeasurement(dsrTypeFinal,decTCodeValue,decTNumValue);
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_CW);
                    dicomSr->addCordinates(mitralValve[i][member],mitralValve[i][member][SOPUID].asString());
                    doc->getTree().goUp();
                }


                if(member == MVVTICW_VMAX){
                    string value = mitralValve[i][member]["value"].asString();
                    const auto vtiCwVmaxCodeValue=DSRCodedEntryValue("79913-0", "LN", "Mitral valve Vmax");
                    const auto unitIs= mitralValve[i][member]["unit"].asString();
                    LOGSR("%s Found MVVTIPW_VMAX Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                    auto measurementValue=DSRNumericMeasurementValue(value, dicomSr->getVelBasicMeasurementUnit(unitIs));
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    dicomSr->addNumMeasurement(dsrTypeFinal,vtiCwVmaxCodeValue,measurementValue);
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_CW);
                    dicomSr->addCordinates(mitralValve[i][member],mitralValve[i][member][SOPUID].asString());
                    doc->getTree().goUp();
                }


                if(member == MVVTICW_VMIN){
                    string value = mitralValve[i][member]["value"].asString();
                    const auto vtiCwVminCodeValue=DSRCodedEntryValue("EN-442", "99EN", "CW MV Mean Velocity");
                    const auto unitIs= mitralValve[i][member]["unit"].asString();
                    LOGSR("%s Found MVVTIPW_VMIN Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                    auto measurementValue=DSRNumericMeasurementValue(value, dicomSr->getVelBasicMeasurementUnit(unitIs));
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    dicomSr->addNumMeasurement(dsrTypeFinal,vtiCwVminCodeValue,measurementValue);
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_CW);
                    dicomSr->addCordinates(mitralValve[i][member],mitralValve[i][member][SOPUID].asString());
                    doc->getTree().goUp();
                }


                if(member == MVVC){
                    string value = mitralValve[i][member]["value"].asString();
                    const auto mvVcCodeValue=DSRCodedEntryValue("80061-5", "LN", "Mitral regurgitation vena contracta width");
                    const auto unitIs = mitralValve[i][member]["unit"].asString();
                    LOGSR("%s Found MVVC Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                    auto measurementValue = DSRNumericMeasurementValue(value, dicomSr->getDistBasicMeasurementUnit(unitIs));
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    dicomSr->addNumMeasurement(dsrTypeFinal,mvVcCodeValue,measurementValue);
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_COLOR);
                    dicomSr->addCordinates(mitralValve[i][member],mitralValve[i][member][SOPUID].asString());
                    doc->getTree().goUp();
                }

                if(member == MVAD){
                    string value = mitralValve[i][member]["value"].asString();
                    const auto mvAdCodeValue=DSRCodedEntryValue("80051-6", "LN", "Mitral annulus diastolic diameter - A4C");
                    const auto unitIs = mitralValve[i][member]["unit"].asString();
                    LOGSR("%s Found MVAD Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                    auto measurementValue = DSRNumericMeasurementValue(value, dicomSr->getDistBasicMeasurementUnit(unitIs));
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    dicomSr->addNumMeasurement(dsrTypeFinal,mvAdCodeValue,measurementValue);
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_2D);
                    dicomSr->addCordinates(mitralValve[i][member],mitralValve[i][member][SOPUID].asString());
                    doc->getTree().goUp();
                }

                if(member == MVABYCONTEQA){
                    string value = mitralValve[i][member]["value"].asString();
                    const auto measurementName=DSRCodedEntryValue("80067-2", "LN", "Mitral valve area (PISA)");
                    LOGSR("%s Found MVABYCONTEQA Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                    const auto measurementValue=DSRNumericMeasurementValue(value, CODE_UCUM_cm2);
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    dicomSr->addNumMeasurement(dsrTypeFinal,measurementName,measurementValue);
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_CW);
                    dicomSr->addCordinates(mitralValve[i][member],mitralValve[i][member][SOPUID].asString());
                    doc->getTree().goUp();
                }

            }
            if(i==0){
                isAfterCurrent = true;
                LOGSR("%s MV isAfterCurrent true", TAG);
            }
        }
    }


    if(root.isMember(AORTA)){
        if(isLVRootSet){
            doc->getTree().goUp();
        }else if( isRootSet){
            doc->getTree().goUp();
        }
        doc->getTree().goUp();
        doc->getTree().goUp();

        doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Container, DSRTypes::AM_belowCurrent);
        doc->getTree().getCurrentContentItem().setConceptName(CODE_DCM_RETIRED_Findings);
        doc->getTree().getCurrentContentItem().setContinuityOfContent(DSRTypes::COC_Separate);

        doc->getTree().addContentItem(DSRTypes::RT_hasConceptMod,  DSRTypes::VT_Code,DSRTypes::AM_belowCurrent);
        doc->getTree().getCurrentContentItem().setConceptName(CODE_SRT_FindingSite);
        doc->getTree().getCurrentContentItem().setCodeValue(DSRCodedEntryValue("T-35400", "SRT", AORTICVALVE));

        doc->getTree().getCurrentContentItem().setContinuityOfContent(DSRTypes::COC_Separate);

        doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Container);
        doc->getTree().getCurrentContentItem().setConceptName(CODE_DCM_MeasurementGroup);
        isRootSet=true;
        Json::Value aorta = root[AORTA];
        auto isAfterCurrent = false;
        bool isBelowCurrentNeeded = true;
        for (Json::Value::ArrayIndex i = 0; i != aorta.size(); i++) {
            Json::Value::Members members = aorta[i].getMemberNames();
            for(auto & member : members) {
                auto dsrType = DSRTypes::AM_belowCurrent;
                if(isAfterCurrent){
                    dsrType = DSRTypes::AM_afterCurrent;
                }else{
                    dsrType = DSRTypes::AM_belowCurrent;
                }
                if (member == ASCAO) {
                    string value = aorta[i][member]["value"].asString();
                    const auto ascAoCodeValue=DSRCodedEntryValue("18012-5", "LN", "Ascending Aortic Diameter");
                    const auto unitIs = aorta[i][member]["unit"].asString();
                    LOGSR("%s Found AsccAo Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                    auto measurementValue = DSRNumericMeasurementValue(value, dicomSr->getDistBasicMeasurementUnit(unitIs));
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    dicomSr->addNumMeasurement(dsrTypeFinal,ascAoCodeValue,measurementValue);
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_2D);
                    dicomSr->addCordinates(aorta[i][member],aorta[i][member][SOPUID].asString());
                    doc->getTree().goUp();
                }

                if (member == STJUN) {
                    string value = aorta[i][member]["value"].asString();
                    const auto raRAAREACodeValue=DSRCodedEntryValue("79955-1", "LN", "Aortic sinotubular junction dimension");
                    const auto unitIs = aorta[i][member]["unit"].asString();
                    LOGSR("%s Found STJUN Value found member is %s value is %s unit is %s", TAG,member.c_str(),value.c_str(),unitIs.c_str());
                    const auto raRAAREANumValue=DSRNumericMeasurementValue(value, dicomSr->getDistBasicMeasurementUnit(unitIs));
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    dicomSr->addNumMeasurement(dsrTypeFinal,raRAAREACodeValue,raRAAREANumValue);
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_2D);
                    dicomSr->addCordinates(aorta[i][member],aorta[i][member][SOPUID].asString());
                    doc->getTree().goUp();
                }

                if(member == ANNULUS){
                    string value = aorta[i][member]["value"].asString();
                    const auto raRAAREACodeValue=DSRCodedEntryValue("79940-3", "LN", "Aortic annulus diameter");
                    const auto unitIs = aorta[i][member]["unit"].asString();
                    LOGSR("%s Found ANNULUS Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                    const auto raRAAREANumValue=DSRNumericMeasurementValue(value, dicomSr->getDistBasicMeasurementUnit(unitIs));
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    dicomSr->addNumMeasurement(dsrTypeFinal,raRAAREACodeValue,raRAAREANumValue);
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_2D);
                    dicomSr->addCordinates(aorta[i][member],aorta[i][member][SOPUID].asString());
                    doc->getTree().goUp();
                }


                if(member == SINUS){
                    string value = aorta[i][member]["value"].asString();
                    const auto raRAAREACodeValue=DSRCodedEntryValue("79953-6", "LN", "Aortic root diameter");
                    const auto unitIs = aorta[i][member]["unit"].asString();
                    LOGSR("%s Found SINUS Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                    const auto raRAAREANumValue=DSRNumericMeasurementValue(value, dicomSr->getDistBasicMeasurementUnit(unitIs));
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    dicomSr->addNumMeasurement(dsrTypeFinal,raRAAREACodeValue,raRAAREANumValue);
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_2D);
                    dicomSr->addCordinates(aorta[i][member],aorta[i][member][SOPUID].asString());
                    doc->getTree().goUp();
                }

                if(member == AVVC){
                    string value = aorta[i][member]["value"].asString();
                    const auto raRAAREACodeValue=DSRCodedEntryValue("79948-6", "LN", "Aortic regurgitation vena contracta width");
                    const auto unitIs = aorta[i][member]["unit"].asString();
                    LOGSR("%s Found AVVC Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                    const auto raRAAREANumValue=DSRNumericMeasurementValue(value, dicomSr->getDistBasicMeasurementUnit(unitIs));
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    dicomSr->addNumMeasurement(dsrTypeFinal,raRAAREACodeValue,raRAAREANumValue);
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_COLOR);
                    dicomSr->addCordinates(aorta[i][member],aorta[i][member][SOPUID].asString());
                    doc->getTree().goUp();
                }

                if(member == ARPHT){
                    string value = aorta[i][member]["value"].asString();
                    const auto measurement=DSRCodedEntryValue("79947-8", "LN", "Aortic regurgitation pressure half-time");
                    LOGSR("%s Found ARPHT Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                    const auto decTNumValue=DSRNumericMeasurementValue(value, CODE_UCUM_mis);
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    dicomSr->addNumMeasurement(dsrTypeFinal,measurement,decTNumValue);
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_CW);
                    dicomSr->addCordinates(aorta[i][member],aorta[i][member][SOPUID].asString());
                    doc->getTree().goUp();
                }

                if(member == AVVTI){
                    string value = aorta[i][member]["value"].asString();
                    const auto measurement=DSRCodedEntryValue("79965-0", "LN", "Aortic valve VTI");
                    LOGSR("%s Found AVVC Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                    const auto measurementValue=DSRNumericMeasurementValue(value, CODE_UCUM_Centimeter);
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    dicomSr->addNumMeasurement(dsrTypeFinal,measurement,measurementValue);
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_CW);
                    dicomSr->addCordinates(aorta[i][member],aorta[i][member][SOPUID].asString());
                    doc->getTree().goUp();
                }

                if(member == AV_VTI_VMAX){
                    string value = aorta[i][member]["value"].asString();
                    const auto measurementName=DSRCodedEntryValue("79964-3", "LN", "AV Vmax sys");
                    const auto unitIs = aorta[i][member]["unit"].asString();
                    LOGSR("%s Found AV_VTI_VMAX Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                    const auto measurementValue=DSRNumericMeasurementValue(value, dicomSr->getVelBasicMeasurementUnit(unitIs));
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    dicomSr->addNumMeasurement(dsrTypeFinal,measurementName,measurementValue);
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_CW);
                    dicomSr->addCordinates(aorta[i][member],aorta[i][member][SOPUID].asString());
                    doc->getTree().goUp();

                }

                if(member == AV_VTI_VMIN){
                    string value = aorta[i][member]["value"].asString();
                    const auto measurementName=DSRCodedEntryValue("79961-9", "LN", "AV Mean flow vel sys");
                    const auto unitIs = aorta[i][member]["unit"].asString();
                    LOGSR("%s Found AV_VTI_VMIN Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                    const auto measurementValue=DSRNumericMeasurementValue(value, dicomSr->getVelBasicMeasurementUnit(unitIs));
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    dicomSr->addNumMeasurement(dsrTypeFinal,measurementName,measurementValue);
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_CW);
                    dicomSr->addCordinates(aorta[i][member],aorta[i][member][SOPUID].asString());
                    doc->getTree().goUp();

                }

                if(member == AV_VTI_MAXPG){
                    string value = aorta[i][member]["value"].asString();
                    const auto measurementName=DSRCodedEntryValue("79963-5", "LN", "AV Max grad sys Simp Bern");
                    LOGSR("%s Found AV_VTI_MAXPG Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                    const auto measurementValue=DSRNumericMeasurementValue(value, CODE_UCUM_MMHG);
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    dicomSr->addNumMeasurement(dsrTypeFinal,measurementName,measurementValue);
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_CW);
                    dicomSr->addCordinates(aorta[i][member],aorta[i][member][SOPUID].asString());
                    doc->getTree().goUp();

                }


                if(member == AV_VTI_MEANPG){
                    string value = aorta[i][member]["value"].asString();
                    const auto measurementName=DSRCodedEntryValue("79962-7", "LN", "Aortic Valve Mean Gradient");
                    LOGSR("%s Found AV_VTI_MEANPG Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                    const auto measurementValue=DSRNumericMeasurementValue(value, CODE_UCUM_MMHG);
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    dicomSr->addNumMeasurement(dsrTypeFinal,measurementName,measurementValue);
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_CW);
                    dicomSr->addCordinates(aorta[i][member],aorta[i][member][SOPUID].asString());
                    doc->getTree().goUp();

                }

                if(member == AVA){
                    string value = aorta[i][member]["value"].asString();
                    const auto measurement=DSRCodedEntryValue("79958-5", "LN", "Aortic valve area (Continuity VTI)");
                    LOGSR("%s Found AVA Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                    const auto measurementValue=DSRNumericMeasurementValue(value, CODE_UCUM_cm2);
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    dicomSr->addNumMeasurement(dsrTypeFinal,measurement,measurementValue);
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_CW);
                    dicomSr->addCordinates(aorta[i][member],aorta[i][member][SOPUID].asString());
                    doc->getTree().goUp();
                }

                if(member == DI){
                    string value = aorta[i][member]["value"].asString();
                    const auto measurement=DSRCodedEntryValue("EN-2627", "99EN", "Dimensionless Index (DI)");
                    LOGSR("%s Found DI Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                    const auto measurementValue=DSRNumericMeasurementValue(value, CODE_UCUM_Ratio);
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    dicomSr->addNumMeasurement(dsrTypeFinal,measurement,measurementValue);
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_CW);
                    dicomSr->addCordinates(aorta[i][member],aorta[i][member][SOPUID].asString());
                    doc->getTree().goUp();
                }

                if(member == AV_PK_VEL){
                    string value = aorta[i][member]["value"].asString();
                    const auto measurementName=DSRCodedEntryValue("EN-58", "99EN", "AV Peak Velocity");
                    const auto unitIs = aorta[i][member]["unit"].asString();
                    LOGSR("%s Found AV_PK_VEL Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                    const auto measurementValue=DSRNumericMeasurementValue(value, dicomSr->getVelBasicMeasurementUnit(unitIs));
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    if(isAfterCurrent){
                        dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent,measurementName,measurementValue);
                    }else{
                        dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent,measurementName,measurementValue);
                    }

                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_PW);
                    dicomSr->addCordinates(aorta[i][member],aorta[i][member][SOPUID].asString());
                    doc->getTree().goUp();
                }

                if(member == AVPG){
                    string value = aorta[i][member]["value"].asString();
                    const auto measurementName=DSRCodedEntryValue("EN-5818", "99EN", "AV Peak Gradient");
                    LOGSR("%s Found AVPG Value found member is %s value is %s", TAG,member.c_str(),value.c_str());
                    const auto measurementValue=DSRNumericMeasurementValue(value, CODE_UCUM_MMHG);
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    if(isAfterCurrent){
                        dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent,measurementName,measurementValue);
                    }else{
                        dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent,measurementName,measurementValue);
                    }
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_PW);
                    dicomSr->addCordinates(aorta[i][member],aorta[i][member][SOPUID].asString());
                    doc->getTree().goUp();
                }

                if(member == MM_AO){
                    string value = aorta[i][member]["value"].asString();
                    const auto measurementName=DSRCodedEntryValue("18015-8", "LN", "Aorta root annulo-aortic junction Diameter by US");
                    const auto unitIs = aorta[i][member]["unit"].asString();
                    LOGSR("%s Found MM AO Value found member is %s value is %s", TAG,member.c_str(),value.c_str(), unitIs.c_str());
                    const auto measurementValue=DSRNumericMeasurementValue(value, dicomSr->getDistBasicMeasurementUnit(unitIs));
                    const DSRTypes::E_AddMode dsrTypeFinal = dsrType;
                    if(isAfterCurrent){
                        dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent,measurementName,measurementValue);
                    }else{
                        dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent,measurementName,measurementValue);
                    }
                    dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_M);
                    dicomSr->addCordinates(aorta[i][member],aorta[i][member][SOPUID].asString());
                    doc->getTree().goUp();
                }

                if(i==0){
                    isAfterCurrent = true;
                }
            }
        }
    }

    if(root.isMember(VCAVA)){
        if(isLVRootSet || isRootSet){
            doc->getTree().goUp();
        }

        doc->getTree().goUp();
        doc->getTree().goUp();

        doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Container, DSRTypes::AM_belowCurrent);
        doc->getTree().getCurrentContentItem().setConceptName(CODE_DCM_RETIRED_Findings);
        doc->getTree().getCurrentContentItem().setContinuityOfContent(DSRTypes::COC_Separate);

        doc->getTree().addContentItem(DSRTypes::RT_hasConceptMod,  DSRTypes::VT_Code,DSRTypes::AM_belowCurrent);
        doc->getTree().getCurrentContentItem().setConceptName(CODE_SRT_FindingSite);
        doc->getTree().getCurrentContentItem().setCodeValue(DSRCodedEntryValue("T-48600", "SRT", VCAVA));

        doc->getTree().getCurrentContentItem().setContinuityOfContent(DSRTypes::COC_Separate);

        doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Container);
        doc->getTree().getCurrentContentItem().setConceptName(CODE_DCM_MeasurementGroup);
        isRootSet=true;
        Json::Value vccava = root[VCAVA];

        auto isBelowCurrentNeeded = true;

        for (Json::Value::ArrayIndex i = 0; i != vccava.size(); i++) {
            Json::Value::Members members = vccava[i].getMemberNames();

            for (auto &member: members) {
                if (member == IVC_MIN_2D) {
                    string value = vccava[i][member]["value"].asString();
                    const auto measurementName=DSRCodedEntryValue("18006-7", "LN", "Inferior vena cava Diameter");
                    const auto unitIs= vccava[i][member]["unit"].asString();
                    LOGSR("%s Found IVC_MIN_2D Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                    auto measurementValue=DSRNumericMeasurementValue(value, dicomSr->getDistBasicMeasurementUnit(unitIs));
                    dicomSr->addMeasurementEncoding(isBelowCurrentNeeded,measurementName,measurementValue,vccava[i][member]);
                    doc->getTree().goUp();
                }

                if (member == IVC_MAX_2D) {
                    string value = vccava[i][member]["value"].asString();
                    const auto measurementName=DSRCodedEntryValue("EN-32", "99EN", "Inferior Vena Cava Max Diameter");
                    const auto unitIs= vccava[i][member]["unit"].asString();
                    LOGSR("%s Found IVC_MAX_2D Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                    auto measurementValue=DSRNumericMeasurementValue(value, dicomSr->getDistBasicMeasurementUnit(unitIs));
                    dicomSr->addMeasurementEncoding(isBelowCurrentNeeded,measurementName,measurementValue,vccava[i][member]);
                    doc->getTree().goUp();
                }

                if(member == IVC_MIN_M){
                    if(vccava[i][member].isMember("value")){
                        string value = vccava[i][member]["value"].asString();
                        const auto measurementName=DSRCodedEntryValue("EN-48", "99EN", "Inferior Vena Cava Min Diameter");
                        const auto unitIs= vccava[i][member]["unit"].asString();
                        LOGSR("%s Found IVC_MIN_M Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                        auto measurementValue=DSRNumericMeasurementValue(value, dicomSr->getDistBasicMeasurementUnit(unitIs));
                        dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,vccava[i][member],CODE_IMAGING_MODE_M);
                        doc->getTree().goUp();
                    }
                }

                if(member == IVC_MAX_M){
                    if(vccava[i][member].isMember("value")){
                        string value = vccava[i][member]["value"].asString();
                        const auto measurementName=DSRCodedEntryValue("EN-49", "99EN", "Inferior Vena Cava Max Diameter");
                        const auto unitIs= vccava[i][member]["unit"].asString();
                        LOGSR("%s Found IVC_MAX_M Value found member is %s value is %s unit is %s", TAG,member.c_str(), value.c_str(), unitIs.c_str());
                        auto measurementValue=DSRNumericMeasurementValue(value, dicomSr->getDistBasicMeasurementUnit(unitIs));
                        dicomSr->addMeasurementEncodingMode(isBelowCurrentNeeded,measurementName,measurementValue,vccava[i][member],CODE_IMAGING_MODE_M);
                        doc->getTree().goUp();
                    }
                }

            }
            if(i==0){
                isBelowCurrentNeeded = false;
            }
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
