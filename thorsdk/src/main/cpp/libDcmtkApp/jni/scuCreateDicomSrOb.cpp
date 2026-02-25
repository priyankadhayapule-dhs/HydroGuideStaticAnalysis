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

#define CODE_UCUM_cm_per_sec     DSRBasicCodedEntry("cm/s", "UCUM", "cm/s")
#define CODE_UCUM_m2     DSRBasicCodedEntry("m2", "UCUM", "m2")
#define CODE_UCUM_meter     DSRBasicCodedEntry("m", "UCUM", "Meter")
#define CODE_UCUM_kg     DSRBasicCodedEntry("kg", "UCUM", "Kilogram")
#define CODE_UCUM_num     DSRBasicCodedEntry("{#}","UCUM","number")
#define CODE_UCUM_0or2         DSRBasicCodedEntry("{0:2}", "UCUM", "{0:2}")
#define CODE_UCUM_day         DSRBasicCodedEntry("d", "UCUM", "Days")
#define CODE_UCUM_percent         DSRBasicCodedEntry("%", "UCUM", "Percentage")
#define CODE_UCUM_g         DSRBasicCodedEntry("g", "UCUM","Gram")
#define CODE_BPM DSRBasicCodedEntry("bpm", "UCUM", "bpm")
#define CODE_UCUM_MMHG DSRBasicCodedEntry("mm[Hg]", "UCUM", "mmHg")
#define CODE_UCUM_BPM DSRBasicCodedEntry("{H.B.}/min", "UCUM", "BPM")

#define CODE_IMAGING_MODE_PW DSRCodedEntryValue("R-409E4","SRT","Doppler Pulsed")
#define CODE_IMAGING_MODE_M DSRCodedEntryValue("G-0394","SRT","M mode")

#define ERROR 0
#define SUCCESS 1

#define PATIENT_CHARACTERISTICS "Patient Characteristics"
#define PATIENT_HEIGHT "Patient Height"
#define TEXT_COMMENT "Comment"
#define PATIENT_HEIGHT_UNIT_INCH "Inches"
#define PATIENT_WEIGHT "Patient Weight"
#define GRAVIDA "Gravida"
#define PARA "Para"
#define ABORTA "Aborta"
#define ECTOPIC_PREGNANCIES "Ectopic Pregnancies"

#define SUMMARY "Summary"
#define LMP "LMP"
#define EDD "EDD"
#define EDD_FROM_LMP "EDD from LMP"
#define EDD_FROM_AUA "EDD from AUA"
#define NUMBER_OF_FETUSES "Number of Fetuses"
#define FETUS_SUMMARIES_ARRAY "Fetus Summaries"
#define ESTIMATED_WEIGHT "Estimated Weight"
#define ACTUAL_ULTRASOUND_AGE "Actual Ultrasound Age"
#define FETAL_BIOMETRY_ARRAY "Fetal Biometry Array"
#define MEASUREMENTS "Measurements"
#define MEASUREMENT_NAME "measurementName"
#define BIPARIETAL_DIA "BPD"
#define HEAD_CIRCUM "HC"
#define OCCIPITAL_FRONTAL_DIAM "OFD"
#define ABDOMINAL_CIRCUM "AC"
#define FEMUR_LENGTH "FL"
#define ANTERIOR_POSTERIOR_DIAM "Anterior Posterior Diameter"
#define TRANS_ABDOMINAL_DIAM "Trans Abdominal Diameter"
#define TRANSVERSE_THORACIC_DIAMETER "Transverse Thoracic Diameter"
#define DERIVATION_OBJECT "derivationObject"
#define GESTATIONAL_SAC "Gestational Sac"
#define YOLK_SAC "Yolk Sac"
#define CROWN_RUMP_LENGTH "CRL"
#define CERVICAL_LENGTH "Cervical Length"

#define FETAL_BIOPHYSICAL_PROFILE_ARRAY "Fetal Biophysical Profile Array"
#define GROSS_BODY_MOVEMENT "Gross Body Movement"
#define FETAL_BREATHING "Fetal Breathing"
#define FETAL_TONE "Fetal Tone"
#define FETAL_HEART_REACTIVITY "Fetal Heart Reactivity"
#define AMNIOTIC_FLUID_VOLUME "Amniotic Fluid Volume"
#define BIOPHYSICAL_PROFILE_SUM_SCORE "Biophysical Profile Sum Score"

#define EARLY_GESTATION_ARRAY "Early Gestation Array"
#define FINDINGS "Findings"
#define AMNIOTIC_SAC "Amniotic Sac"
#define FIRST_QUADRANT_DIAMETER "First Quadrant Diameter"
#define SECOND_QUADRANT_DIAMETER "Second Quadrant Diameter"
#define THIRD_QUADRANT_DIAMETER "Third Quadrant Diameter"
#define FOURTH_QUADRANT_DIAMETER "Fourth Quadrant Diameter"
#define AMNIOTIC_FLUID_INDEX "Amniotic Fluid Index"
#define AMNIOTIC_FLUID_INDEX_LARGEST_POCKET "Amniotic Fluid Index Largest Pocket"

#define PELVIS_AND_UTERUS "Pelvis and Uterus"
#define SelectionStatus "selectionStatus"
#define valueKey "value"
#define GESTATIONAL_AGE "Gestational Age"
#define GESTATIONAL_GROWTH "Gestational Growth"
#define INFERRED_FROM "Inferred from"
#define LN_CODE "LN Code"
#define PEAK_SYSTOLIC "Peak Systolic"
#define END_DIASTOLIC "End Diastolic"
#define PULSATILITY_INDEX "Pulsatility Index"
#define RESISTIVITY_INDEX "Resistivity Index"
#define DERIVATION "derivation"
#define RI_FROM_JSON "RI"
#define SD_FROM_JSON "S/D"
#define UMBILICAL_ARTERY "Umbilical Artery"
#define UMBILICAL_ARTERY_RATIO "Umbilical Artery Ratio"
#define HEART_RATE "HR"
#define FETAL_PRESENTATION "Fetal Presentation"
#define PLACENTA_LOCATION "Placenta Location"
#define SPATIAL_STATUS "Spatial Status"
#define GA_OVULATION_AGE "gaOvulationAge"


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
DCMTKUTIL_METHOD(createDicomSrOb)(JNIEnv* env,jobject thiz,jstring jsonReport,jstring outputFilePath){


    //createSR(env,outputFilePath);
    return createObSr(env,outputFilePath,jsonReport);
}

auto getDSRCodedEntryValueFromMeasurementName(const OFString& measurementName) {
    OFString codeValue;
    OFString codeMeaning;
    OFString codeScheme = "LN";
    if (measurementName == TEXT_COMMENT) {
        codeValue = "121106";
        codeMeaning = TEXT_COMMENT;
        codeScheme = "DCM";
    } else if (measurementName == PATIENT_HEIGHT) {
        codeValue = "8302-2";
        codeMeaning = PATIENT_HEIGHT;
    } else if (measurementName == PATIENT_WEIGHT) {
        codeValue = "29463-7";
        codeMeaning = PATIENT_WEIGHT;
    } else if (measurementName == GRAVIDA) {
        codeValue = "11996-6";
        codeMeaning = GRAVIDA;
    } else if (measurementName == PARA) {
        codeValue = "11977-6";
        codeMeaning = PARA;
    } else if (measurementName == ABORTA) {
        codeValue = "11612-9";
        codeMeaning = ABORTA;
    } else if (measurementName == ECTOPIC_PREGNANCIES) {
        codeValue = "33065-4";
        codeMeaning = ECTOPIC_PREGNANCIES;
    } else if (measurementName == LMP) {
        codeValue = "11955-2";
        codeMeaning = LMP;
    } else if (measurementName == EDD_FROM_LMP) {
        codeValue = "11779-6";
        codeMeaning = EDD_FROM_LMP;
    } else if (measurementName == EDD_FROM_AUA) {
        codeValue = "11781-2";
        codeMeaning = "EDD from average ultrasound age";
    } else if (measurementName == GA_OVULATION_AGE) {
        codeValue = "11886-9";
        codeMeaning = "Gestational Age by ovulation date";
    } else if (measurementName == EDD) {
        codeValue = "11778-8";
        codeMeaning = EDD;
    }  else if (measurementName == NUMBER_OF_FETUSES) {
        codeValue = "11878-6";
        codeMeaning = NUMBER_OF_FETUSES;
    }  else if (measurementName == ESTIMATED_WEIGHT) {
        codeValue = "11727-5";
        codeMeaning = ESTIMATED_WEIGHT;
    }  else if (measurementName == BIPARIETAL_DIA) {
        codeValue = "11820-8";
        codeMeaning = "Biparietal Diameter";
    } else if (measurementName == HEAD_CIRCUM) {
        codeValue = "11984-2";
        codeMeaning = "Head Circumference";
    } else if (measurementName == OCCIPITAL_FRONTAL_DIAM) {
        codeValue = "11851-3";
        codeMeaning = "Occipital-Frontal Diameter";
    } else if (measurementName == ABDOMINAL_CIRCUM) {
        codeValue = "11979-2";
        codeMeaning = "Abdominal Circumference";
    } else if (measurementName == FEMUR_LENGTH) {
        codeValue = "11963-6";
        codeMeaning = "Femur Length";
    } else if (measurementName == ANTERIOR_POSTERIOR_DIAM) {
        codeValue = "11818-2";
        codeMeaning = "Anterior-Posterior Abdominal Diameter";
    } else if (measurementName == TRANS_ABDOMINAL_DIAM) {
        codeValue = "11862-0";
        codeMeaning = "Transverse Abdominal Diameter";
    } else if (measurementName == TRANSVERSE_THORACIC_DIAMETER) {
        codeValue = "11864-6";
        codeMeaning = "Transverse Thoracic Diameter";
    } else if (measurementName == GESTATIONAL_SAC) {
        codeValue = "11850-5";
        codeMeaning = "Gestational Sac Diameter";
    } else if (measurementName == YOLK_SAC) {
        codeValue = "11816-6";
        codeMeaning = "Yolk Sac length";
    } else if (measurementName == CROWN_RUMP_LENGTH) {
        codeValue = "11957-8";
        codeMeaning = "Crown Rump Length";
    } else if (measurementName == CERVICAL_LENGTH) {
        codeValue = "11961-0";
        codeMeaning = "Cervix Length";
    } else if (measurementName == FIRST_QUADRANT_DIAMETER) {
        codeValue = "11624-4";
        codeMeaning = FIRST_QUADRANT_DIAMETER;
    } else if (measurementName == SECOND_QUADRANT_DIAMETER) {
        codeValue = "11626-9";
        codeMeaning = SECOND_QUADRANT_DIAMETER;
    } else if (measurementName == THIRD_QUADRANT_DIAMETER) {
        codeValue = "11625-1";
        codeMeaning = THIRD_QUADRANT_DIAMETER;
    } else if (measurementName == FOURTH_QUADRANT_DIAMETER) {
        codeValue = "11623-6";
        codeMeaning = FOURTH_QUADRANT_DIAMETER;
    } else if (measurementName == AMNIOTIC_FLUID_INDEX) {
        codeValue = "11627-7";
        codeMeaning = AMNIOTIC_FLUID_INDEX;
    } else if (measurementName == AMNIOTIC_FLUID_INDEX_LARGEST_POCKET) {
        codeValue = "81827009";
        codeScheme = "SCT";
        codeMeaning = AMNIOTIC_FLUID_INDEX_LARGEST_POCKET;
    } else if (measurementName == GROSS_BODY_MOVEMENT) {
        codeValue = "11631-9";
        codeMeaning = GROSS_BODY_MOVEMENT;
    } else if (measurementName == FETAL_BREATHING) {
        codeValue = "11632-7";
        codeMeaning = FETAL_BREATHING;
    } else if (measurementName == FETAL_TONE) {
        codeValue = "11635-0";
        codeMeaning = FETAL_TONE;
    } else if (measurementName == FETAL_HEART_REACTIVITY) {
        codeValue = "11635-5";
        codeMeaning = FETAL_HEART_REACTIVITY;
    } else if (measurementName == AMNIOTIC_FLUID_VOLUME) {
        codeValue = "11630-1";
        codeMeaning = AMNIOTIC_FLUID_VOLUME;
    } else if (measurementName == BIOPHYSICAL_PROFILE_SUM_SCORE) {
        codeValue = "11634-3";
        codeMeaning = BIOPHYSICAL_PROFILE_SUM_SCORE;
    } else if (measurementName == GESTATIONAL_AGE) {
        codeValue = "18185-9";
        codeMeaning = GESTATIONAL_AGE;
    } else if (measurementName == GESTATIONAL_GROWTH) {
        codeScheme = "DCM";
        codeValue = "125012";
        codeMeaning = "Growth Percentile Rank";
    } else if (measurementName == PEAK_SYSTOLIC) {
        codeValue = "11726-7";
        codeMeaning = "Peak Systolic Velocity";
    } else if (measurementName == END_DIASTOLIC) {
        codeValue = "11653-3";
        codeMeaning = "End Diastolic Velocity";
    } else if (measurementName == RESISTIVITY_INDEX) {
        codeValue = "12023-8";
        codeMeaning = RESISTIVITY_INDEX;
    } else if (measurementName == UMBILICAL_ARTERY_RATIO) {
        codeValue = "EN-195-196";
        codeScheme = "EN";
        codeMeaning = UMBILICAL_ARTERY_RATIO;
    } else if (measurementName == ACTUAL_ULTRASOUND_AGE) {
        codeValue = "11888-5";
        codeMeaning = "Composite Ultrasound Age";
    } else if (measurementName == HEART_RATE) {
        codeValue = "11948-7";
        codeMeaning = "Fetal Heart Rate";
    } else if (measurementName == FETAL_PRESENTATION) {
        codeValue = "EN-SP-01";
        codeMeaning = "Fetal Presentation";
        codeScheme = "EN99";
    } else if (measurementName == PLACENTA_LOCATION) {
        codeValue = "EN-SP-02";
        codeMeaning = "Placenta Location";
        codeScheme = "EN99";
    } else {
        codeValue = "";
        codeMeaning = "";
    }
    return std::make_shared<DSRCodedEntryValue>(codeValue, codeScheme, codeMeaning);
}

void addVtiPsvEdvGroupOb(DicomSR* dicomSr, Json::Value object) {
    auto isBelowCurrentNeeded = false;
    if (object.isMember(PEAK_SYSTOLIC) && !object[PEAK_SYSTOLIC].empty()) {
        Json::Value velocityObject = object[PEAK_SYSTOLIC];
        auto lnCode = getDSRCodedEntryValueFromMeasurementName(PEAK_SYSTOLIC);
        auto value = DSRNumericMeasurementValue(
                velocityObject[valueKey].asString(),
                dicomSr->getVelBasicMeasurementUnit(velocityObject["unit"].asString()));
        auto addMode = DSRTypes::AM_afterCurrent;
        if(isBelowCurrentNeeded){
            addMode= DSRTypes::AM_belowCurrent;
        }else{
            isBelowCurrentNeeded = true;
            addMode= DSRTypes::AM_afterCurrent;
        }
        dicomSr->addNumMeasurement(addMode, *lnCode.get(), value);
        dicomSr->addDerivations(velocityObject[DERIVATION].asString());
        dicomSr->getDocInstant()->getTree().goUp();
        dicomSr->getDocInstant()->getTree().goUp();
    }
    if (object.isMember(END_DIASTOLIC) && !object[END_DIASTOLIC].empty()) {
        Json::Value velocityObject = object[END_DIASTOLIC];
        auto lnCode = getDSRCodedEntryValueFromMeasurementName(END_DIASTOLIC);
        auto value = DSRNumericMeasurementValue(
                velocityObject[valueKey].asString(),
                dicomSr->getVelBasicMeasurementUnit(velocityObject["unit"].asString()));
        auto addMode = DSRTypes::AM_afterCurrent;
        if(isBelowCurrentNeeded){
            addMode= DSRTypes::AM_belowCurrent;
        }else{
            isBelowCurrentNeeded = true;
            addMode= DSRTypes::AM_afterCurrent;
        }
        dicomSr->addNumMeasurement(addMode, *lnCode.get(), value);
        dicomSr->addDerivations(velocityObject[DERIVATION].asString());
        dicomSr->getDocInstant()->getTree().goUp();
        dicomSr->getDocInstant()->getTree().goUp();
    }
    if (object.isMember(RI_FROM_JSON) && !object[RI_FROM_JSON].empty()) {
        Json::Value calculationObject = object[RI_FROM_JSON];
        auto lnCode = getDSRCodedEntryValueFromMeasurementName(RESISTIVITY_INDEX);
        auto value = DSRNumericMeasurementValue(
                calculationObject[valueKey].asString(),
                CODE_UCUM_Ratio);
        auto addMode = DSRTypes::AM_afterCurrent;
        if(isBelowCurrentNeeded){
            addMode= DSRTypes::AM_belowCurrent;
        }else{
            isBelowCurrentNeeded = true;
            addMode= DSRTypes::AM_afterCurrent;
        }
        dicomSr->addNumMeasurement(addMode, *lnCode.get(), value);
        dicomSr->addDerivations(calculationObject[DERIVATION].asString());
        dicomSr->getDocInstant()->getTree().goUp();
        dicomSr->getDocInstant()->getTree().goUp();
    }
    if (object.isMember(UMBILICAL_ARTERY_RATIO) && !object[UMBILICAL_ARTERY_RATIO].empty()) {
        Json::Value calculationObject = object[UMBILICAL_ARTERY_RATIO];
        auto lnCode = getDSRCodedEntryValueFromMeasurementName(UMBILICAL_ARTERY_RATIO);
        auto value = DSRNumericMeasurementValue(
                calculationObject[valueKey].asString(),
                CODE_UCUM_Ratio);
        auto addMode = DSRTypes::AM_afterCurrent;
        if(isBelowCurrentNeeded){
            addMode= DSRTypes::AM_belowCurrent;
        }else{
            isBelowCurrentNeeded = true;
            addMode= DSRTypes::AM_afterCurrent;
        }
        dicomSr->addNumMeasurement(addMode, *lnCode.get(), value);
        dicomSr->addDerivations(calculationObject[DERIVATION].asString());
        dicomSr->getDocInstant()->getTree().goUp();
        dicomSr->getDocInstant()->getTree().goUp();
    }
}

OFCondition addBiometryGroupMeasurement(DicomSR* dicomSr, const Json::Value& fetalBiometry) {
    for (const auto &item: fetalBiometry) {
        dicomSr->addContainer(FetalBiometryGroupCode);
        if (item.isMember(MEASUREMENTS)) {
            Json::Value measurements = item[MEASUREMENTS];
            bool isFirstValueAfterContainer = true;
            for (const auto &measurement: measurements) {
                const auto lnCode = getDSRCodedEntryValueFromMeasurementName(
                        measurement[MEASUREMENT_NAME].asString());
                const auto value = DSRNumericMeasurementValue(
                        measurement[valueKey].asString(), dicomSr->getDistBasicMeasurementUnit(measurement["unit"].asString()));
                auto addMode = DSRTypes::AM_afterCurrent;
                if (isFirstValueAfterContainer) {
                    isFirstValueAfterContainer = false;
                    addMode = DSRTypes::AM_belowCurrent;
                }
                auto result = dicomSr->addNumMeasurement(addMode, *lnCode.get(), value);
                if (result.bad()) {
                    return result;
                }
            }
        }
        if (item.isMember(DERIVATION_OBJECT)) {
            Json::Value derivationObject = item[DERIVATION_OBJECT];
            const auto lnCode = getDSRCodedEntryValueFromMeasurementName(
                    derivationObject[MEASUREMENT_NAME].asString());
            const auto value = DSRNumericMeasurementValue(
                    derivationObject[valueKey].asString(), dicomSr->getDistBasicMeasurementUnit(derivationObject["unit"].asString()));
            auto result = dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, *lnCode.get(), value);
            if (result.good()) {
                dicomSr->addDerivations(derivationObject[DERIVATION].asString());
                dicomSr->addProperty(derivationObject[SelectionStatus].asString(), DSRTypes::AM_afterCurrent);
                dicomSr->getDocInstant()->getTree().goUp();
            }
        }
        if (item.isMember(GESTATIONAL_AGE)) {
            Json::Value gestationalAge = item[GESTATIONAL_AGE];
            const auto lnCode = getDSRCodedEntryValueFromMeasurementName(GESTATIONAL_AGE);
            const auto value = DSRNumericMeasurementValue(
                    gestationalAge[valueKey].asString(), CODE_UCUM_day);
            auto result = dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, *lnCode.get(), value);
            if (result.good()) {
                dicomSr->getDocInstant()->getTree().addChildContentItem(DSRTypes::RT_inferredFrom,DSRTypes::VT_Code,DSRCodedEntryValue("12013", "DCM", "Table of Values"));
                auto codeScheme = "LN";
                if (gestationalAge[LN_CODE].asString().find("EN") != std::string::npos ) {
                    codeScheme = "99EN";
                }
                dicomSr->getDocInstant()->getTree().getCurrentContentItem().setCodeValue(DSRCodedEntryValue(gestationalAge[LN_CODE].asString(),codeScheme,gestationalAge[INFERRED_FROM].asString()));
                dicomSr->getDocInstant()->getTree().goUp();
            }
        }
        if (item.isMember(GESTATIONAL_GROWTH)) {
            Json::Value gestationalGrowth = item[GESTATIONAL_GROWTH];
            const auto lnCode = getDSRCodedEntryValueFromMeasurementName(GESTATIONAL_GROWTH);
            const auto value = DSRNumericMeasurementValue(
                    gestationalGrowth[valueKey].asString(), CODE_UCUM_percent);
            auto result = dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, *lnCode.get(), value);
            if (result.good()) {
                dicomSr->getDocInstant()->getTree().addChildContentItem(DSRTypes::RT_inferredFrom,DSRTypes::VT_Code,DSRCodedEntryValue("12015", "DCM", "Table of Values"));
                dicomSr->getDocInstant()->getTree().getCurrentContentItem().setCodeValue(DSRCodedEntryValue(gestationalGrowth[LN_CODE].asString(), "LN", gestationalGrowth[INFERRED_FROM].asString()));
                dicomSr->getDocInstant()->getTree().goUp();
            }
        }
        dicomSr->getDocInstant()->getTree().goUp();
    }
    return OFCondition();
}

static jobject createObSr(JNIEnv *env,jstring outputFilePath,jstring jsonReport) {

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
    dicomSr->getDocInstant()->setStudyDescription("OB/GYN Procedure Report");
    dicomSr->getDocInstant()->setSeriesDescription("OB/GYN with measurements");
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

    dicomSr->setRootContainer(CODE_DCM_OBGYNUltrasoundProcedureReport);
    mResult = dicomSr->getDocInstant()->getTree().setTemplateIdentification("5000","DCMR");
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

    if (root.isMember(PATIENT_CHARACTERISTICS) && !root[PATIENT_CHARACTERISTICS].empty()) {
        if(!isPatientCharateristicAdded){
            dicomSr->addFindingSitePatient();
            isPatientCharateristicAdded = true;
        }
        Json::Value patientCharacteristics = root[PATIENT_CHARACTERISTICS];
        const auto isCommentAdded = root.isMember(TEXT_COMMENT) && !root[TEXT_COMMENT].empty();
        if (isCommentAdded) {
            // Text comment in DICOM SR
            const auto lnCode = getDSRCodedEntryValueFromMeasurementName(TEXT_COMMENT);
            const auto value = root[TEXT_COMMENT].asString();
            dicomSr->addText(DSRTypes::AM_belowCurrent, *lnCode.get(), value);
        }
        if (patientCharacteristics.isMember(PATIENT_HEIGHT) &&
            !patientCharacteristics[PATIENT_HEIGHT].empty()) {
            const auto lnCode = getDSRCodedEntryValueFromMeasurementName(PATIENT_HEIGHT);
            const auto value = DSRNumericMeasurementValue(
                    patientCharacteristics[PATIENT_HEIGHT].asString(), CODE_UCUM_meter);
            if (isCommentAdded) {
                // if the comment is added, we are already 1 step down and therefor adding after current
                dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, *lnCode.get(), value);
            } else {
                dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent, *lnCode.get(), value);
            }
        }
        if (patientCharacteristics.isMember(PATIENT_WEIGHT)) {
            const auto lnCode = getDSRCodedEntryValueFromMeasurementName(PATIENT_WEIGHT);
            const auto value = DSRNumericMeasurementValue(
                    patientCharacteristics[PATIENT_WEIGHT].asString(), CODE_UCUM_kg);
            dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, *lnCode.get(), value);
        }
        if (patientCharacteristics.isMember(GRAVIDA)) {
            const auto lnCode = getDSRCodedEntryValueFromMeasurementName( GRAVIDA);
            const auto value = DSRNumericMeasurementValue(
                    patientCharacteristics[GRAVIDA].asString(), CODE_UCUM_num);
            dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, *lnCode.get(), value);
        }
        if (patientCharacteristics.isMember(PARA)) {
            const auto lnCode = getDSRCodedEntryValueFromMeasurementName( PARA);
            const auto value = DSRNumericMeasurementValue(patientCharacteristics[PARA].asString(),
                                                          CODE_UCUM_num);
            dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, *lnCode.get(), value);
        }
        if (patientCharacteristics.isMember(ABORTA)) {
            const auto lnCode = getDSRCodedEntryValueFromMeasurementName( ABORTA);
            const auto value = DSRNumericMeasurementValue(patientCharacteristics[ABORTA].asString(),
                                                          CODE_UCUM_num);
            dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, *lnCode.get(), value);
        }
        if (patientCharacteristics.isMember(ECTOPIC_PREGNANCIES)) {
            const auto lnCode = getDSRCodedEntryValueFromMeasurementName( ECTOPIC_PREGNANCIES);
            const auto value = DSRNumericMeasurementValue(
                    patientCharacteristics[ECTOPIC_PREGNANCIES].asString(), CODE_UCUM_num);
            dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, *lnCode.get(), value);
        }
        doc->getTree().goUp();
        doc->getTree().goUp();
    }

    if(isPatientCharateristicAdded){
        doc->getTree().goUp();
    }

    if (root.isMember(SUMMARY) && !root[SUMMARY].empty()) {
        dicomSr->addContainer(ProcedureSummaryCode);
        Json::Value procedureSummary = root[SUMMARY];
        if (procedureSummary.isMember(LMP)) {
            const auto lnCode = getDSRCodedEntryValueFromMeasurementName(LMP);
            dicomSr->addDate(DSRTypes::AM_belowCurrent, *lnCode.get(),procedureSummary[LMP].asString());
        }
        if (procedureSummary.isMember(EDD_FROM_LMP)) {
            const auto lnCode = getDSRCodedEntryValueFromMeasurementName(EDD_FROM_LMP);
            dicomSr->addDate(DSRTypes::AM_afterCurrent, *lnCode.get(),procedureSummary[EDD_FROM_LMP].asString());
        }
        if (procedureSummary.isMember(GA_OVULATION_AGE)) {
            const auto lnCode = getDSRCodedEntryValueFromMeasurementName(GA_OVULATION_AGE);
            const auto value = DSRNumericMeasurementValue(
                    procedureSummary[GA_OVULATION_AGE].asString(), CODE_UCUM_day);
            dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, *lnCode.get(), value);
        }
        if (procedureSummary.isMember(EDD)) {
            const auto lnCode = getDSRCodedEntryValueFromMeasurementName(EDD);
            dicomSr->addDate(DSRTypes::AM_belowCurrent, *lnCode.get(),procedureSummary[EDD].asString());
        }
        if (procedureSummary.isMember(NUMBER_OF_FETUSES)) {
            const auto lnCode = getDSRCodedEntryValueFromMeasurementName(NUMBER_OF_FETUSES);
            const auto value = DSRNumericMeasurementValue(
                    procedureSummary[NUMBER_OF_FETUSES].asString(), CODE_UCUM_num);
            dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, *lnCode.get(), value);
        }
        if (procedureSummary.isMember(FETUS_SUMMARIES_ARRAY) && !procedureSummary[FETUS_SUMMARIES_ARRAY].empty()) {
            Json::Value fetusSummariesArray = procedureSummary[FETUS_SUMMARIES_ARRAY];
            if (fetusSummariesArray.isArray()) {
                for (const auto& fetusSummaryObject : fetusSummariesArray) {
                    if (fetusSummaryObject.isObject()) {
                        dicomSr->addContainer(FetusSummaryCode);
                        dicomSr->addObsContextForOb(fetusSummaryObject[FETUS_ID].asString());
                        if (fetusSummaryObject.isMember(ESTIMATED_WEIGHT)) {
                            Json::Value estimatedWeightObject = fetusSummaryObject[ESTIMATED_WEIGHT];
                            const auto lnCode = getDSRCodedEntryValueFromMeasurementName(ESTIMATED_WEIGHT);
                            const auto value = DSRNumericMeasurementValue(
                                    estimatedWeightObject[valueKey].asString(), CODE_UCUM_g);
                            dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, *lnCode.get(),value);
                            doc->getTree().addChildContentItem(DSRTypes::RT_inferredFrom,DSRTypes::VT_Code,DSRCodedEntryValue("12014", "DCM", "Table of Values"));
                            auto codeScheme = "LN";
                            if (estimatedWeightObject[LN_CODE].asString().find("EN") != std::string::npos ) {
                                codeScheme = "99EN";
                            }
                            doc->getTree().getCurrentContentItem().setCodeValue(DSRCodedEntryValue(estimatedWeightObject[LN_CODE].asString(),codeScheme,estimatedWeightObject[INFERRED_FROM].asString()));
                            doc->getTree().goUp();
                        }
                        if (fetusSummaryObject.isMember(ACTUAL_ULTRASOUND_AGE)){
                            const auto lnCode = getDSRCodedEntryValueFromMeasurementName(ACTUAL_ULTRASOUND_AGE);
                            const auto value = DSRNumericMeasurementValue(
                                    fetusSummaryObject[ACTUAL_ULTRASOUND_AGE].asString(), CODE_UCUM_day);
                            dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, *lnCode.get(),value);
                        }
                        if (fetusSummaryObject.isMember(EDD_FROM_AUA)) {
                            const auto lnCode = getDSRCodedEntryValueFromMeasurementName(EDD_FROM_AUA);
                            dicomSr->addDate(DSRTypes::AM_afterCurrent, *lnCode.get(),fetusSummaryObject[EDD_FROM_AUA].asString());
                        }
                        if (fetusSummaryObject.isMember(HEART_RATE)){
                            const auto lnCode = getDSRCodedEntryValueFromMeasurementName(HEART_RATE);
                            const auto value = DSRNumericMeasurementValue(
                                    fetusSummaryObject[HEART_RATE].asString(), CODE_BPM);
                            dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, *lnCode.get(),value);
                        }
                        doc->getTree().goUp();
                    }
                }
            }
        }
        doc->getTree().goUp();
        doc->getTree().goUp();
    }

    if (root.isMember(FETAL_BIOMETRY_ARRAY) && root[FETAL_BIOMETRY_ARRAY].isArray() && !root[FETAL_BIOMETRY_ARRAY].empty()) {
        Json::Value fetalBiometryArray = root[FETAL_BIOMETRY_ARRAY];
        for (const auto &item: fetalBiometryArray) {
            doc->getTree().gotoRoot();
            dicomSr->addContainer(FetalBiometryCode);
            dicomSr->addObsContextForOb(item[FETUS_ID].asString());
            addBiometryGroupMeasurement(dicomSr, item[FETAL_BIOMETRY]);
        }
    }

    if (root.isMember(FETAL_BIOPHYSICAL_PROFILE_ARRAY) && root[FETAL_BIOPHYSICAL_PROFILE_ARRAY].isArray() && !root[FETAL_BIOPHYSICAL_PROFILE_ARRAY].empty()) {
        Json::Value biophysicalProfileArray = root[FETAL_BIOPHYSICAL_PROFILE_ARRAY];
        for (const auto &item: biophysicalProfileArray) {
            doc->getTree().gotoRoot();
            dicomSr->addContainer(FetalBiophysicalProfileCode);
            dicomSr->addObsContextForOb(item[FETUS_ID].asString());
            auto lnCode = getDSRCodedEntryValueFromMeasurementName(GROSS_BODY_MOVEMENT);
            auto value = DSRNumericMeasurementValue(
                    item[GROSS_BODY_MOVEMENT].asString(), CODE_UCUM_0or2);
            dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, *lnCode.get(), value);
            lnCode = getDSRCodedEntryValueFromMeasurementName(FETAL_BREATHING);
            value = DSRNumericMeasurementValue(
                    item[FETAL_BREATHING].asString(), CODE_UCUM_0or2);
            dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, *lnCode.get(), value);
            lnCode = getDSRCodedEntryValueFromMeasurementName(FETAL_TONE);
            value = DSRNumericMeasurementValue(
                    item[FETAL_TONE].asString(), CODE_UCUM_0or2);
            dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, *lnCode.get(), value);
            lnCode = getDSRCodedEntryValueFromMeasurementName(FETAL_HEART_REACTIVITY);
            value = DSRNumericMeasurementValue(
                    item[FETAL_HEART_REACTIVITY].asString(), CODE_UCUM_0or2);
            dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, *lnCode.get(), value);
            lnCode = getDSRCodedEntryValueFromMeasurementName(AMNIOTIC_FLUID_VOLUME);
            value = DSRNumericMeasurementValue(
                    item[AMNIOTIC_FLUID_VOLUME].asString(), CODE_UCUM_0or2);
            dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, *lnCode.get(), value);
            lnCode = getDSRCodedEntryValueFromMeasurementName(BIOPHYSICAL_PROFILE_SUM_SCORE);
            value = DSRNumericMeasurementValue(
                    item[BIOPHYSICAL_PROFILE_SUM_SCORE].asString(), CODE_UCUM_num);
            dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, *lnCode.get(), value);
            LOGSR("Spatial Status Fetus ID --- %s", item[FETUS_ID].asString().c_str());
            LOGSR("Spatial Status Fetus name --- %s", root[SPATIAL_STATUS].isMember(item[FETUS_ID].asString()) ? "True" : "False");
            if (root.isMember(SPATIAL_STATUS) &&
                !root[SPATIAL_STATUS].empty() &&
                root[SPATIAL_STATUS].isMember(item[FETUS_ID].asString()) &&
                !root[SPATIAL_STATUS][item[FETUS_ID].asString()].empty()) {
                auto spatialStatusForFetus = root[SPATIAL_STATUS][item[FETUS_ID].asString()];
                if (spatialStatusForFetus.isMember(FETAL_PRESENTATION) && spatialStatusForFetus[FETAL_PRESENTATION] != "") {
                    // Fetal Presentation
                    auto lnCode = getDSRCodedEntryValueFromMeasurementName(FETAL_PRESENTATION);
                    auto value = spatialStatusForFetus[FETAL_PRESENTATION].asString().c_str();
                    LOGSR("Fetal presentation %s", value);
                    dicomSr->addText(DSRTypes::AM_afterCurrent, *lnCode.get(), value);
                }
                if (spatialStatusForFetus.isMember(PLACENTA_LOCATION) && spatialStatusForFetus[PLACENTA_LOCATION] != "") {
                    // Placenta Location
                    auto lnCode = getDSRCodedEntryValueFromMeasurementName(PLACENTA_LOCATION);
                    auto value = spatialStatusForFetus[PLACENTA_LOCATION].asString().c_str();
                    LOGSR("Placenta %s", value);
                    dicomSr->addText(DSRTypes::AM_afterCurrent, *lnCode.get(), value);
                }
            }
        }
    }

    if (root.isMember(EARLY_GESTATION_ARRAY) && root[EARLY_GESTATION_ARRAY].isArray() && !root[EARLY_GESTATION_ARRAY].empty()) {
        Json::Value earlyGestationArray = root[EARLY_GESTATION_ARRAY];
        for (const auto &item: earlyGestationArray) {
            doc->getTree().gotoRoot();
            dicomSr->addContainer(EarlyGestationCode);
            dicomSr->addObsContextForOb(item[FETUS_ID].asString());
            addBiometryGroupMeasurement(dicomSr, item[FETAL_BIOMETRY]);
        }
    }

    if (root.isMember(FINDINGS) && !root[FINDINGS].empty()) {
        Json::Value findings = root[FINDINGS];
        doc->getTree().gotoRoot();
        dicomSr->addContainer(AmnioticSacCode);
        if (findings.isMember(AMNIOTIC_SAC)) {
            Json::Value amnioticSac = findings[AMNIOTIC_SAC];
            dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent, DSRCodedEntryValue("G-C0E3", "SRT", "Finding Site"),DSRCodedEntryValue("T-F1300", "SRT", "Amniotic Sac"));
            if (amnioticSac.isMember(FIRST_QUADRANT_DIAMETER)) {
                const auto lnCode = getDSRCodedEntryValueFromMeasurementName(FIRST_QUADRANT_DIAMETER);
                const auto value = DSRNumericMeasurementValue(
                        amnioticSac[FIRST_QUADRANT_DIAMETER][valueKey].asString(), dicomSr->getDistBasicMeasurementUnit(amnioticSac[FIRST_QUADRANT_DIAMETER]["unit"].asString()));
                auto result = dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, *lnCode.get(), value);
                if (result.good()) {
                    dicomSr->addDerivations(amnioticSac[FIRST_QUADRANT_DIAMETER][DERIVATION].asString());
                    dicomSr->addProperty(amnioticSac[FIRST_QUADRANT_DIAMETER][SelectionStatus].asString(), DSRTypes::AM_afterCurrent);
                    doc->getTree().goUp();
                }
            }
            if (amnioticSac.isMember(SECOND_QUADRANT_DIAMETER)) {
                const auto lnCode = getDSRCodedEntryValueFromMeasurementName(SECOND_QUADRANT_DIAMETER);
                const auto value = DSRNumericMeasurementValue(
                        amnioticSac[SECOND_QUADRANT_DIAMETER][valueKey].asString(), dicomSr->getDistBasicMeasurementUnit(amnioticSac[SECOND_QUADRANT_DIAMETER]["unit"].asString()));
                auto result = dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, *lnCode.get(), value);
                if (result.good()) {
                    dicomSr->addDerivations(amnioticSac[SECOND_QUADRANT_DIAMETER][DERIVATION].asString());
                    dicomSr->addProperty(amnioticSac[SECOND_QUADRANT_DIAMETER][SelectionStatus].asString(), DSRTypes::AM_afterCurrent);
                    doc->getTree().goUp();
                }
            }
            if (amnioticSac.isMember(THIRD_QUADRANT_DIAMETER)) {
                const auto lnCode = getDSRCodedEntryValueFromMeasurementName(THIRD_QUADRANT_DIAMETER);
                const auto value = DSRNumericMeasurementValue(
                        amnioticSac[THIRD_QUADRANT_DIAMETER][valueKey].asString(), dicomSr->getDistBasicMeasurementUnit(amnioticSac[THIRD_QUADRANT_DIAMETER]["unit"].asString()));
                auto result = dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, *lnCode.get(), value);
                if (result.good()) {
                    dicomSr->addDerivations(amnioticSac[THIRD_QUADRANT_DIAMETER][DERIVATION].asString());
                    dicomSr->addProperty(amnioticSac[THIRD_QUADRANT_DIAMETER][SelectionStatus].asString(), DSRTypes::AM_afterCurrent);
                    doc->getTree().goUp();
                }
            }
            if (amnioticSac.isMember(FOURTH_QUADRANT_DIAMETER)) {
                const auto lnCode = getDSRCodedEntryValueFromMeasurementName(FOURTH_QUADRANT_DIAMETER);
                const auto value = DSRNumericMeasurementValue(
                        amnioticSac[FOURTH_QUADRANT_DIAMETER][valueKey].asString(), dicomSr->getDistBasicMeasurementUnit(amnioticSac[FOURTH_QUADRANT_DIAMETER]["unit"].asString()));
                auto result = dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, *lnCode.get(), value);
                if (result.good()) {
                    dicomSr->addDerivations(amnioticSac[FOURTH_QUADRANT_DIAMETER][DERIVATION].asString());
                    dicomSr->addProperty(amnioticSac[FOURTH_QUADRANT_DIAMETER][SelectionStatus].asString(), DSRTypes::AM_afterCurrent);
                    doc->getTree().goUp();
                }
            }
            if (amnioticSac.isMember(AMNIOTIC_FLUID_INDEX)) {
                const auto lnCode = getDSRCodedEntryValueFromMeasurementName(AMNIOTIC_FLUID_INDEX);
                const auto value = DSRNumericMeasurementValue(
                        amnioticSac[AMNIOTIC_FLUID_INDEX][valueKey].asString(), dicomSr->getDistBasicMeasurementUnit(amnioticSac[AMNIOTIC_FLUID_INDEX]["unit"].asString()));
                auto result = dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, *lnCode.get(), value);
            }
            if (amnioticSac.isMember(AMNIOTIC_FLUID_INDEX_LARGEST_POCKET)) {
                const auto lnCode = getDSRCodedEntryValueFromMeasurementName(AMNIOTIC_FLUID_INDEX_LARGEST_POCKET);
                const auto value = DSRNumericMeasurementValue(
                        amnioticSac[AMNIOTIC_FLUID_INDEX_LARGEST_POCKET][valueKey].asString(), dicomSr->getDistBasicMeasurementUnit(amnioticSac[AMNIOTIC_FLUID_INDEX_LARGEST_POCKET]["unit"].asString()));
                auto result = dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, *lnCode.get(), value);
            }
            doc->getTree().goUp();
        }
        doc->getTree().goUp();
    }

    if (root.isMember(PELVIS_AND_UTERUS) && !root[PELVIS_AND_UTERUS].empty()) {
        doc->getTree().gotoRoot();
        dicomSr->addContainer(PelvisAndUterusCode);
        Json::Value pelvisAndUterus = root[PELVIS_AND_UTERUS];
        if (pelvisAndUterus.isMember(MEASUREMENTS) && pelvisAndUterus[MEASUREMENTS].isArray() && !pelvisAndUterus[MEASUREMENTS].empty()) {
            Json::Value measurements = pelvisAndUterus[MEASUREMENTS];
            bool isFirstValueAfterContainer = true;
            for (const auto &measurement: measurements) {
                const auto lnCode = getDSRCodedEntryValueFromMeasurementName(
                        measurement[MEASUREMENT_NAME].asString());
                const auto value = DSRNumericMeasurementValue(
                        measurement[valueKey].asString(),
                        dicomSr->getDistBasicMeasurementUnit(measurement["unit"].asString()));
                auto addMode = DSRTypes::AM_afterCurrent;
                if (isFirstValueAfterContainer) {
                    isFirstValueAfterContainer = false;
                    addMode = DSRTypes::AM_belowCurrent;
                }
                dicomSr->addNumMeasurement(addMode, *lnCode.get(), value);
            }
            if (pelvisAndUterus.isMember(DERIVATION_OBJECT)) {
                Json::Value derivationObject = pelvisAndUterus[DERIVATION_OBJECT];
                const auto lnCode = getDSRCodedEntryValueFromMeasurementName(
                        derivationObject[MEASUREMENT_NAME].asString());
                const auto value = DSRNumericMeasurementValue(
                        derivationObject[valueKey].asString(),
                        dicomSr->getDistBasicMeasurementUnit(derivationObject["unit"].asString()));
                auto result = dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, *lnCode.get(), value);
                if (result.good()) {
                    dicomSr->addDerivations(derivationObject[DERIVATION].asString());
                    dicomSr->addProperty(derivationObject[SelectionStatus].asString(),
                                         DSRTypes::AM_afterCurrent);
                    doc->getTree().goUp();
                }

            }
        }
    }

    if (root.isMember(UMBILICAL_ARTERY) && root[UMBILICAL_ARTERY].isArray() && !root[UMBILICAL_ARTERY].empty()) {
        Json::Value umbilicalArteryObject = root[UMBILICAL_ARTERY];
        for (const auto &item: umbilicalArteryObject) {
            doc->getTree().gotoRoot();
            auto addMode = DSRTypes::AM_belowCurrent;
            auto container = DSRBasicCodedEntry("59776-5", "LN", "Findings");
            dicomSr->getDocInstant()->getTree().addContentItem(DSRTypes::RT_contains,  DSRTypes::VT_Container,addMode);
            dicomSr->getDocInstant()->getTree().getCurrentContentItem().setConceptName(container);
            dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent,
                                       DSRCodedEntryValue("363698007", "SCT", "Finding Site"),
                                       DSRCodedEntryValue("281496003", "SCT", "Pelvic Vascular Structure"));
            doc->getTree().addContentItem(DSRTypes::RT_contains,  DSRTypes::VT_Container,DSRTypes::AM_afterCurrent);
            doc->getTree().getCurrentContentItem().setContinuityOfContent(DSRTypes::COC_Separate);
            auto subContainer = DSRBasicCodedEntry("50536004", "SCT", UMBILICAL_ARTERY);
            auto result = doc->getTree().getCurrentContentItem().setConceptName(subContainer);
            if (result.good()) {
                result = doc->getTree().addChildContentItem(DSRTypes::RT_hasConceptMod, DSRTypes::VT_Text, DSRCodedEntryValue("112050", "DCM", "Anatomic Identifier"));
                if (result.good()) {
                    result =doc->getTree().getCurrentContentItem().setStringValue(item[FETUS_ID].asString());
                    if (result.good()) {
                        addVtiPsvEdvGroupOb(dicomSr, item[FETAL_BIOMETRY]);
                        doc->getTree().goUp();
                        doc->getTree().goUp();
                    }
                }
            }
        }
    }


    auto *fileFormat = new DcmFileFormat();
    DcmDataset *dataset = nullptr;
    if (fileFormat != nullptr){
        dataset = fileFormat->getDataset();
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
        OFCondition fileResult = fileFormat->saveFile(outputFile, EXS_LittleEndianExplicit);
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
