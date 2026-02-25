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
#define CODE_UCUM_day         DSRBasicCodedEntry("d", "UCUM", "days")
#define CODE_UCUM_MMHG DSRBasicCodedEntry("mm[Hg]", "UCUM", "mmHg")

#define CODE_IMAGING_MODE_PW DSRCodedEntryValue("R-409E4","SRT","Doppler Pulsed")
#define CODE_IMAGING_MODE_M DSRCodedEntryValue("G-0394","SRT","M mode")
#define CODE_UCUM_BPM DSRBasicCodedEntry("{H.B.}/min", "UCUM", "BPM")

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
#define MEASUREMENTS "Measurements"
#define MEASUREMENT_NAME "measurementName"
#define SelectionStatus "selectionStatus"
#define valueKey "value"
#define INFERRED_FROM "Inferred from"
#define DERIVATION_OBJECT "derivationObject"

#define PELVIS_AND_UTERUS "Pelvis and Uterus"
#define SelectionStatus "selectionStatus"
#define ENDOMETRIUM "Endometrium"
#define CERVICAL_LENGTH "Cervical Length"
#define UTERUS "Uterus"
#define VOLUME "Volume"
#define LENGTH "Length"
#define WIDTH "Width"
#define HEIGHT "Height"
#define OVARIES "Ovaries"
#define RIGHT_OVARY_VOLUME_GROUP "Right Ovary Volume Group"
#define LEFT_OVARY_VOLUME_GROUP "Left Ovary Volume Group"
#define RIGHT_UTERINE_ARTERY "Right Uterine Artery"
#define LEFT_UTERINE_ARTERY "Left Uterine Artery"
#define RIGHT_OVARIAN_ARTERY "Right Ovarian Artery"
#define LEFT_OVARIAN_ARTERY "Left Ovarian Artery"
#define VTI "VTI"
#define PEAK_VELOCITY "Peak Vel"
#define MEAN_VELOCITY "Mean Vel"
#define PEAK_GRADIENT "Peak Gradient"
#define MEAN_GRADIENT "Mean Gradient"
#define PEAK_SYSTOLIC "Peak Systolic"
#define END_DIASTOLIC "End Diastolic"
#define PULSATILITY_INDEX "Pulsatility Index"
#define RESISTIVITY_INDEX "Resistivity Index"
#define SYSTOLIC_TO_DIASTOLIC_VELOCITY_RATIO "Systolic to Diastolic Velocity Ratio"
#define LATERALITY "Laterality"
#define MEASUREMENTID "MeasurementId"
#define PRE_VOID_BLADDER "Pre-Void Bladder"
#define POST_VOID_BLADDER "Post-Void Bladder"
#define DERIVATION "derivation"
#define PI_FROM_JSON "PI"
#define RI_FROM_JSON "RI"
#define SD_FROM_JSON "S/D"
#define FIBROMA_ONE "Fibroma-1"
#define FIBROMA_TWO "Fibroma-2"
#define RIGHT_MASS_ONE "Right Mass 1"
#define RIGHT_MASS_TWO "Right Mass 2"
#define RIGHT_OVARIAN_CYST_ONE "Right Ovarian Cyst 1"
#define RIGHT_OVARIAN_CYST_TWO "Right Ovarian Cyst 2"
#define LEFT_MASS_ONE "Left Mass 1"
#define LEFT_MASS_TWO "Left Mass 2"
#define LEFT_OVARIAN_CYST_ONE "Left Ovarian Cyst 1"
#define LEFT_OVARIAN_CYST_TWO "Left Ovarian Cyst 2"
#define SOLID_MASS "Solid mass"
#define COMPLEX_CYST "Complex cyst"




using namespace std;

#define LOGSR(...) \
((void)__android_log_print(ANDROID_LOG_INFO, "dcmtk::", __VA_ARGS__))

OFCondition addFindingContainer(DicomSR* dicomSr, const DSRCodedEntryValue& dsrCodedEntryValue) {
    auto addMode = DSRTypes::AM_belowCurrent;
    auto container = DSRBasicCodedEntry("59776-5", "LN", "Findings");
    dicomSr->getDocInstant()->getTree().addContentItem(DSRTypes::RT_contains,  DSRTypes::VT_Container,addMode);
    auto result = dicomSr->getDocInstant()->getTree().getCurrentContentItem().setConceptName(container);
    if (result.bad()) {
        return result;
    }
    return dicomSr->addConceptModCode(DSRTypes::AM_belowCurrent, DSRCodedEntryValue("363698007", "SCT", "Finding Site"),dsrCodedEntryValue);

}

auto getDSRCodedEntryValueFromMeasurementNames(const OFString& measurementName, const OFString& measurementId = "") {
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
    } else if (measurementName == CERVICAL_LENGTH) {
        codeValue = "11961-0";
        codeMeaning = "Cervix Length";
    } else if (measurementName == ENDOMETRIUM) {
        codeValue = "12145-9";
        codeMeaning = "Endometrium Thickness";
    } else if (measurementName == VTI) {
        codeValue = "20354-7";
        codeMeaning = "Velocity Time Integral";
    } else if (measurementName == PEAK_VELOCITY) {
        codeValue = "EN-"+measurementId;
        codeScheme = "99EN";
        codeMeaning = PEAK_VELOCITY;
    } else if (measurementName == MEAN_VELOCITY) {
        codeValue = "EN-"+measurementId;
        codeScheme = "99EN";
        codeMeaning = MEAN_VELOCITY;
    } else if (measurementName == PEAK_GRADIENT) {
        codeValue = "EN-"+measurementId;
        codeScheme = "99EN";
        codeMeaning = PEAK_GRADIENT;
    } else if (measurementName == MEAN_GRADIENT) {
        codeValue = "EN-"+measurementId;
        codeScheme = "99EN";
        codeMeaning = MEAN_GRADIENT;
    } else if (measurementName == PEAK_SYSTOLIC) {
        codeValue = "11726-7";
        codeMeaning = "Peak Systolic Velocity";
    } else if (measurementName == END_DIASTOLIC) {
        codeValue = "11653-3";
        codeMeaning = "End Diastolic Velocity";
    } else if (measurementName == PULSATILITY_INDEX) {
        codeValue = "12008-9";
        codeMeaning = PULSATILITY_INDEX;
    } else if (measurementName == RESISTIVITY_INDEX) {
        codeValue = "12023-8";
        codeMeaning = RESISTIVITY_INDEX;
    } else if (measurementName == SYSTOLIC_TO_DIASTOLIC_VELOCITY_RATIO) {
        codeValue = "12144-2";
        codeMeaning = SYSTOLIC_TO_DIASTOLIC_VELOCITY_RATIO;
    } else {
        codeValue = "";
        codeMeaning = "";
    }
    return std::make_shared<DSRCodedEntryValue>(codeValue, codeScheme, codeMeaning);
}

void addPelvisUterusMeasurement(DicomSR *dicomSr, Json::Value measurementObject, bool isFirstValueAfterContainer) {
    if (measurementObject.isMember(MEASUREMENTS) && measurementObject[MEASUREMENTS].isArray() && !measurementObject[MEASUREMENTS].empty()) {
        Json::Value measurements = measurementObject[MEASUREMENTS];
        for (const auto &measurement: measurements) {
            const auto lnCode = getDSRCodedEntryValueFromMeasurementNames(
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
        if (measurementObject.isMember(DERIVATION_OBJECT)) {
            Json::Value derivationObject = measurementObject[DERIVATION_OBJECT];
            const auto lnCode = getDSRCodedEntryValueFromMeasurementNames(
                    derivationObject[MEASUREMENT_NAME].asString());
            const auto value = DSRNumericMeasurementValue(
                    derivationObject[valueKey].asString(),
                    dicomSr->getDistBasicMeasurementUnit(derivationObject["unit"].asString()));
            auto result = dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, *lnCode.get(), value);
            if (result.good()) {
                dicomSr->addDerivations(derivationObject[DERIVATION].asString());
                dicomSr->addProperty(derivationObject[SelectionStatus].asString(),
                                     DSRTypes::AM_afterCurrent);
                dicomSr->getDocInstant()->getTree().goUp();
            }

        }
    }
}

void addLWHMeasurements(DicomSR* dicomSr, Json::Value object, const DSRCodedEntryValue& lnCode, const bool itemHasToBeAddedBelow = false) {
    bool itemAddedBelow = false;
    if (object.isMember(MEASUREMENTS) && object[MEASUREMENTS].isArray() && !object[MEASUREMENTS].empty()) {
        Json::Value measurementsObject = object[MEASUREMENTS];
        for (const auto &measurement: measurementsObject) {
            const auto value = DSRNumericMeasurementValue(
                    measurement[valueKey].asString(),
                    dicomSr->getDistBasicMeasurementUnit(measurement["unit"].asString()));
            auto addMode = DSRTypes::AM_afterCurrent;
            if (itemHasToBeAddedBelow && !itemAddedBelow) {
                addMode = DSRTypes::AM_belowCurrent;
                itemAddedBelow = true;
            }
            dicomSr->addNumMeasurement(addMode, lnCode, value);
        }
    }
    if (object.isMember(DERIVATION_OBJECT)) {
        Json::Value derivationObject = object[DERIVATION_OBJECT];
        const auto value = DSRNumericMeasurementValue(
                derivationObject[valueKey].asString(),
                dicomSr->getDistBasicMeasurementUnit(derivationObject["unit"].asString()));
        auto result = dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, lnCode, value);
        if (result.good()) {
            dicomSr->addDerivations(derivationObject[DERIVATION].asString());
            dicomSr->addProperty(derivationObject[SelectionStatus].asString(),
                                 DSRTypes::AM_afterCurrent);
            dicomSr->getDocInstant()->getTree().goUp();
        }

    }
}

auto getLateralityValue(const OFString& laterality) {
    if (laterality == "Right") {
        return std::make_shared<DSRCodedEntryValue>("24028007", "SCT", "Right");
    } else {
        return std::make_shared<DSRCodedEntryValue>("7771000", "SCT", "Left");
    }
}

void addVtiPsvEdvGroup(DicomSR* dicomSr, Json::Value object) {
    auto isBelowCurrentNeeded = false;
    dicomSr->addConceptModCode(
            DSRTypes::AM_belowCurrent,
            DSRCodedEntryValue("272741003", "SCT", LATERALITY),
            *getLateralityValue(object[LATERALITY].asString()).get());
    if (object.isMember(VTI) && !object[VTI].empty()) {
        Json::Value velocityObject = object[VTI];
        auto lnCode = getDSRCodedEntryValueFromMeasurementNames(VTI);
        auto value = DSRNumericMeasurementValue(
                velocityObject[valueKey].asString(),
                dicomSr->getDistBasicMeasurementUnit(velocityObject["unit"].asString()));
        auto addMode = DSRTypes::AM_afterCurrent;
        if (isBelowCurrentNeeded) {
            addMode= DSRTypes::AM_belowCurrent;
        } else {
            isBelowCurrentNeeded = true;
            addMode= DSRTypes::AM_afterCurrent;
        }
        dicomSr->addNumMeasurement(addMode, *lnCode.get(), value);
        dicomSr->addDerivations(velocityObject[DERIVATION].asString());
        dicomSr->getDocInstant()->getTree().goUp();
        dicomSr->getDocInstant()->getTree().goUp();
    }
    if (object.isMember(PEAK_VELOCITY) && !object[PEAK_VELOCITY].empty()) {
        Json::Value velocityObject = object[PEAK_VELOCITY];
        auto lnCode = getDSRCodedEntryValueFromMeasurementNames(PEAK_VELOCITY, velocityObject[MEASUREMENTID].asString());
        auto value = DSRNumericMeasurementValue(
                velocityObject[valueKey].asString(),
                dicomSr->getVelBasicMeasurementUnit(velocityObject["unit"].asString()));
        auto addMode = DSRTypes::AM_afterCurrent;
        if (isBelowCurrentNeeded) {
            addMode= DSRTypes::AM_belowCurrent;
        } else {
            isBelowCurrentNeeded = true;
            addMode= DSRTypes::AM_afterCurrent;
        }
        dicomSr->addNumMeasurement(addMode, *lnCode.get(), value);
        dicomSr->addDerivations(velocityObject[DERIVATION].asString());
        dicomSr->getDocInstant()->getTree().goUp();
        dicomSr->getDocInstant()->getTree().goUp();
    }
    if (object.isMember(MEAN_VELOCITY) && !object[MEAN_VELOCITY].empty()) {
        Json::Value velocityObject = object[MEAN_VELOCITY];
        auto lnCode = getDSRCodedEntryValueFromMeasurementNames(MEAN_VELOCITY, velocityObject[MEASUREMENTID].asString());
        auto value = DSRNumericMeasurementValue(
                velocityObject[valueKey].asString(),
                dicomSr->getVelBasicMeasurementUnit(velocityObject["unit"].asString()));
        auto addMode = DSRTypes::AM_afterCurrent;
        if (isBelowCurrentNeeded) {
            addMode= DSRTypes::AM_belowCurrent;
        } else {
            isBelowCurrentNeeded = true;
            addMode= DSRTypes::AM_afterCurrent;
        }
        dicomSr->addNumMeasurement(addMode, *lnCode.get(), value);
        dicomSr->addDerivations(velocityObject[DERIVATION].asString());
        dicomSr->getDocInstant()->getTree().goUp();
        dicomSr->getDocInstant()->getTree().goUp();
    }
    if (object.isMember(PEAK_GRADIENT) && !object[PEAK_GRADIENT].empty()) {
        Json::Value velocityObject = object[PEAK_GRADIENT];
        auto lnCode = getDSRCodedEntryValueFromMeasurementNames(PEAK_GRADIENT, velocityObject[MEASUREMENTID].asString());
        auto value = DSRNumericMeasurementValue(
                velocityObject[valueKey].asString(),
                CODE_UCUM_MMHG);
        auto addMode = DSRTypes::AM_afterCurrent;
        if (isBelowCurrentNeeded) {
            addMode= DSRTypes::AM_belowCurrent;
        } else {
            isBelowCurrentNeeded = true;
            addMode= DSRTypes::AM_afterCurrent;
        }
        dicomSr->addNumMeasurement(addMode, *lnCode.get(), value);
        dicomSr->addDerivations(velocityObject[DERIVATION].asString());
        dicomSr->getDocInstant()->getTree().goUp();
        dicomSr->getDocInstant()->getTree().goUp();
    }
    if (object.isMember(MEAN_GRADIENT) && !object[MEAN_GRADIENT].empty()) {
        Json::Value velocityObject = object[MEAN_GRADIENT];
        auto lnCode = getDSRCodedEntryValueFromMeasurementNames(MEAN_GRADIENT, velocityObject[MEASUREMENTID].asString());
        auto value = DSRNumericMeasurementValue(
                velocityObject[valueKey].asString(),
                CODE_UCUM_MMHG);
        auto addMode = DSRTypes::AM_afterCurrent;
        if (isBelowCurrentNeeded) {
            addMode= DSRTypes::AM_belowCurrent;
        } else {
            isBelowCurrentNeeded = true;
            addMode= DSRTypes::AM_afterCurrent;
        }
        dicomSr->addNumMeasurement(addMode, *lnCode.get(), value);
        dicomSr->addDerivations(velocityObject[DERIVATION].asString());
        dicomSr->getDocInstant()->getTree().goUp();
        dicomSr->getDocInstant()->getTree().goUp();
    }
    if (object.isMember(PEAK_SYSTOLIC) && !object[PEAK_SYSTOLIC].empty()) {
        Json::Value velocityObject = object[PEAK_SYSTOLIC];
        auto lnCode = getDSRCodedEntryValueFromMeasurementNames(PEAK_SYSTOLIC);
        auto value = DSRNumericMeasurementValue(
                velocityObject[valueKey].asString(),
                dicomSr->getVelBasicMeasurementUnit(velocityObject["unit"].asString()));
        auto addMode = DSRTypes::AM_afterCurrent;
        if (isBelowCurrentNeeded) {
            addMode= DSRTypes::AM_belowCurrent;
        } else {
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
        auto lnCode = getDSRCodedEntryValueFromMeasurementNames(END_DIASTOLIC);
        auto value = DSRNumericMeasurementValue(
                velocityObject[valueKey].asString(),
                dicomSr->getVelBasicMeasurementUnit(velocityObject["unit"].asString()));
        auto addMode = DSRTypes::AM_afterCurrent;
        if (isBelowCurrentNeeded) {
            addMode= DSRTypes::AM_belowCurrent;
        } else {
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
        auto lnCode = getDSRCodedEntryValueFromMeasurementNames(RESISTIVITY_INDEX);
        auto value = DSRNumericMeasurementValue(
                calculationObject[valueKey].asString(),
                CODE_UCUM_Ratio);
        auto addMode = DSRTypes::AM_afterCurrent;
        if (isBelowCurrentNeeded) {
            addMode= DSRTypes::AM_belowCurrent;
        } else {
            isBelowCurrentNeeded = true;
            addMode= DSRTypes::AM_afterCurrent;
        }
        dicomSr->addNumMeasurement(addMode, *lnCode.get(), value);
        dicomSr->addDerivations(calculationObject[DERIVATION].asString());
        dicomSr->getDocInstant()->getTree().goUp();
        dicomSr->getDocInstant()->getTree().goUp();
    }
    if (object.isMember(PI_FROM_JSON) && !object[PI_FROM_JSON].empty()) {
        Json::Value calculationObject = object[PI_FROM_JSON];
        auto lnCode = getDSRCodedEntryValueFromMeasurementNames(PULSATILITY_INDEX);
        auto value = DSRNumericMeasurementValue(
                calculationObject[valueKey].asString(),
                CODE_UCUM_Ratio);
        auto addMode = DSRTypes::AM_afterCurrent;
        if (isBelowCurrentNeeded) {
            addMode= DSRTypes::AM_belowCurrent;
        } else {
            isBelowCurrentNeeded = true;
            addMode= DSRTypes::AM_afterCurrent;
        }
        dicomSr->addNumMeasurement(addMode, *lnCode.get(), value);
        dicomSr->addDerivations(calculationObject[DERIVATION].asString());
        dicomSr->getDocInstant()->getTree().goUp();
        dicomSr->getDocInstant()->getTree().goUp();
    }
    if (object.isMember(SD_FROM_JSON) && !object[SD_FROM_JSON].empty()) {
        Json::Value calculationObject = object[SD_FROM_JSON];
        auto lnCode = getDSRCodedEntryValueFromMeasurementNames(SYSTOLIC_TO_DIASTOLIC_VELOCITY_RATIO);
        auto value = DSRNumericMeasurementValue(
                calculationObject[valueKey].asString(),
                CODE_UCUM_Ratio);
        auto addMode = DSRTypes::AM_afterCurrent;
        if (isBelowCurrentNeeded) {
            addMode= DSRTypes::AM_belowCurrent;
        } else {
            isBelowCurrentNeeded = true;
            addMode= DSRTypes::AM_afterCurrent;
        }
        dicomSr->addNumMeasurement(addMode, *lnCode.get(), value);
        dicomSr->addDerivations(calculationObject[DERIVATION].asString());
        dicomSr->getDocInstant()->getTree().goUp();
        dicomSr->getDocInstant()->getTree().goUp();
    }
}

static jobject createGynSr(JNIEnv *env,jstring outputFilePath,jstring jsonReport){

    Json::Value root;
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
    mResult =doc->createNewSeriesInStudy(studyUID);//setcreateNewSeriesInStudy(studyUID_01);
        LOGSR("%s New Series Created Result %s",TAG,mResult.text());
    //}

    if (!weight.empty()) {
        dicomSr->getDocInstant()->setPatientWeight(weight);
    }

    if (!height.empty()) {
        dicomSr->getDocInstant()->setPatientSize(height);
    }

    if(!accessNumber.empty()){
        doc->setAccessionNumber(accessNumber);
    }

    if(!timeZoneOffSet.empty()){
        doc->setTimezoneOffsetFromUTC(timeZoneOffSet);
    }

    if(!studyDate.empty()){
        doc->setStudyDate(studyDate);
    }

    if(!studyTime.empty()){
        doc->setStudyTime(studyTime);
    }

    doc->setStudyID(studyId);
    doc->setStudyDescription("OB/GYN Procedure Report");
    doc->setSeriesDescription("OB/GYN with measurements");
    doc->setManufacturer(MANUFACTURE);
    doc->setManufacturerModelName("Torso");
    doc->setDeviceSerialNumber(deviceSerial);
    doc->setSoftwareVersions(softwareVersion);
    doc->setSeriesNumber("2", OFTrue);
    doc->setInstanceNumber(instanceNumber, OFTrue);


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
    mResult = doc->getTree().setTemplateIdentification("5000","DCMR");
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
        // Check if the comment is already added into SR
        const auto isCommentAdded = root.isMember(TEXT_COMMENT) && !root[TEXT_COMMENT].empty();

        if (isCommentAdded) {
            // Text comment in DICOM SR
            const auto lnCode = getDSRCodedEntryValueFromMeasurementNames(TEXT_COMMENT);
            const auto value = root[TEXT_COMMENT].asString();
            dicomSr->addText(DSRTypes::AM_belowCurrent, *lnCode.get(), value);
        }

        if (patientCharacteristics.isMember(PATIENT_HEIGHT) &&
            !patientCharacteristics[PATIENT_HEIGHT].empty()) {
            const auto lnCode = getDSRCodedEntryValueFromMeasurementNames(PATIENT_HEIGHT);
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
            const auto lnCode = getDSRCodedEntryValueFromMeasurementNames(PATIENT_WEIGHT);
            const auto value = DSRNumericMeasurementValue(
                    patientCharacteristics[PATIENT_WEIGHT].asString(), CODE_UCUM_kg);
            dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, *lnCode.get(), value);
        }
        if (patientCharacteristics.isMember(GRAVIDA)) {
            const auto lnCode = getDSRCodedEntryValueFromMeasurementNames( GRAVIDA);
            const auto value = DSRNumericMeasurementValue(
                    patientCharacteristics[GRAVIDA].asString(), CODE_UCUM_num);
            dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, *lnCode.get(), value);
        }
        if (patientCharacteristics.isMember(PARA)) {
            const auto lnCode = getDSRCodedEntryValueFromMeasurementNames( PARA);
            const auto value = DSRNumericMeasurementValue(patientCharacteristics[PARA].asString(),
                                                          CODE_UCUM_num);
            dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, *lnCode.get(), value);
        }
        if (patientCharacteristics.isMember(ABORTA)) {
            const auto lnCode = getDSRCodedEntryValueFromMeasurementNames( ABORTA);
            const auto value = DSRNumericMeasurementValue(patientCharacteristics[ABORTA].asString(),
                                                          CODE_UCUM_num);
            dicomSr->addNumMeasurement(DSRTypes::AM_afterCurrent, *lnCode.get(), value);
        }
        if (patientCharacteristics.isMember(ECTOPIC_PREGNANCIES)) {
            const auto lnCode = getDSRCodedEntryValueFromMeasurementNames( ECTOPIC_PREGNANCIES);
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
            const auto lnCode = getDSRCodedEntryValueFromMeasurementNames(LMP);
            dicomSr->addDate(DSRTypes::AM_belowCurrent, *lnCode.get(),procedureSummary[LMP].asString());
        }
        doc->getTree().goUp();
        doc->getTree().goUp();
    }

    if (root.isMember(PELVIS_AND_UTERUS) && !root[PELVIS_AND_UTERUS].empty()) {
        dicomSr->addContainer(PelvisAndUterusCode);
        Json::Value pelvisAndUterus = root[PELVIS_AND_UTERUS];

        bool isFirstValueAfterContainer = true;

        if (pelvisAndUterus.isMember(ENDOMETRIUM) && !pelvisAndUterus[ENDOMETRIUM].empty()) {
            Json::Value endometriumObject = pelvisAndUterus[ENDOMETRIUM];
            addPelvisUterusMeasurement(dicomSr, endometriumObject, isFirstValueAfterContainer);
            isFirstValueAfterContainer = false;
        }

        if (pelvisAndUterus.isMember(CERVICAL_LENGTH) && !pelvisAndUterus[CERVICAL_LENGTH].empty()) {
            Json::Value endometriumObject = pelvisAndUterus[CERVICAL_LENGTH];
            addPelvisUterusMeasurement(dicomSr, endometriumObject, isFirstValueAfterContainer);
            isFirstValueAfterContainer = false;
        }

        if (pelvisAndUterus.isMember(UTERUS) && !pelvisAndUterus[UTERUS].empty()) {
            Json::Value uterusObject = pelvisAndUterus[UTERUS];
            doc->getTree().addContentItem(DSRTypes::RT_contains,  DSRTypes::VT_Container,DSRTypes::AM_afterCurrent);
            auto container = DSRBasicCodedEntry("35039007", "SCT", UTERUS);
            auto result = doc->getTree().getCurrentContentItem().setConceptName(container);
            if (result.good()) {
                bool itemHasToBeAddedBelow = true;
                if (uterusObject.isMember(VOLUME) && !uterusObject[VOLUME].empty()) {
                    const auto lnCode = DSRCodedEntryValue("33192-6", "LN", "Uterus Volume");
                    const auto value = DSRNumericMeasurementValue(
                            uterusObject[VOLUME][valueKey].asString(),
                            CODE_UCUM_ml);
                    dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent, lnCode, value);
                    dicomSr->addDerivations(uterusObject[VOLUME][DERIVATION].asString());
                    doc->getTree().goUp();
                    itemHasToBeAddedBelow = false;
                }
                if (uterusObject.isMember(LENGTH) && !uterusObject[LENGTH].empty()) {
                    Json::Value lengthObject = uterusObject[LENGTH];
                    auto lnCode = DSRCodedEntryValue("11842-2", "LN", "Uterus Length");
                    addLWHMeasurements(dicomSr, lengthObject, lnCode, itemHasToBeAddedBelow);
                    itemHasToBeAddedBelow = false;
                }
                if (uterusObject.isMember(HEIGHT) && !uterusObject[HEIGHT].empty()) {
                    Json::Value heightObject = uterusObject[HEIGHT];
                    auto lnCode = DSRCodedEntryValue("11859-6", "LN", "Uterus Height");
                    addLWHMeasurements(dicomSr, heightObject, lnCode, itemHasToBeAddedBelow);
                    itemHasToBeAddedBelow = false;
                }
                if (uterusObject.isMember(WIDTH) && !uterusObject[WIDTH].empty()) {
                    Json::Value widthObject = uterusObject[WIDTH];
                    auto lnCode = DSRCodedEntryValue("11865-3", "LN", "Uterus Width");
                    addLWHMeasurements(dicomSr, widthObject, lnCode, itemHasToBeAddedBelow);
                    itemHasToBeAddedBelow = false;
                }
                doc->getTree().goUp();
            }
            doc->getTree().goUp();
        }
        doc->getTree().goUp();
    }

    if (root.isMember(OVARIES) && !root[OVARIES].empty()) {
        addFindingContainer(dicomSr, DSRCodedEntryValue("15497006", "SCT", "Ovary"));
        Json::Value ovariesObject = root[OVARIES];
        if (ovariesObject.isMember(LEFT_OVARY_VOLUME_GROUP) && !ovariesObject[LEFT_OVARY_VOLUME_GROUP].empty()) {
            Json::Value leftOvaryVolumeGroup = ovariesObject[LEFT_OVARY_VOLUME_GROUP];
            doc->getTree().addContentItem(DSRTypes::RT_contains,  DSRTypes::VT_Container,DSRTypes::AM_afterCurrent);
            auto container = DSRBasicCodedEntry("15497006", "SCT", "Ovary");
            auto result = doc->getTree().getCurrentContentItem().setConceptName(container);
            if (result.good()) {
                bool itemHasToBeAddedBelow = true;
                if (leftOvaryVolumeGroup.isMember(VOLUME) && !leftOvaryVolumeGroup[VOLUME].empty()) {
                    const auto lnCode = DSRCodedEntryValue("12164-0", "LN", "Left Ovary Volume");
                    const auto value = DSRNumericMeasurementValue(
                            leftOvaryVolumeGroup[VOLUME][valueKey].asString(),
                            CODE_UCUM_ml);
                    itemHasToBeAddedBelow = false;
                    dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent, lnCode, value);
                    dicomSr->addDerivations(leftOvaryVolumeGroup[VOLUME][DERIVATION].asString());
                    doc->getTree().goUp();
                }
                if (leftOvaryVolumeGroup.isMember(LENGTH) && !leftOvaryVolumeGroup[LENGTH].empty()) {
                    Json::Value lengthObject = leftOvaryVolumeGroup[LENGTH];
                    auto lnCode = DSRCodedEntryValue("11840-6", "LN", "Left Ovary Length");
                    addLWHMeasurements(dicomSr, lengthObject, lnCode, itemHasToBeAddedBelow);
                    itemHasToBeAddedBelow = false;
                }
                if (leftOvaryVolumeGroup.isMember(HEIGHT) && !leftOvaryVolumeGroup[HEIGHT].empty()) {
                    Json::Value heightObject = leftOvaryVolumeGroup[HEIGHT];
                    auto lnCode = DSRCodedEntryValue("11857-0", "LN", "Left Ovary Height");
                    addLWHMeasurements(dicomSr, heightObject, lnCode, itemHasToBeAddedBelow);
                    itemHasToBeAddedBelow = false;
                }
                if (leftOvaryVolumeGroup.isMember(WIDTH) && !leftOvaryVolumeGroup[WIDTH].empty()) {
                    Json::Value widthObject = leftOvaryVolumeGroup[WIDTH];
                    auto lnCode = DSRCodedEntryValue("11829-9", "LN", "Left Ovary Width");
                    addLWHMeasurements(dicomSr, widthObject, lnCode, itemHasToBeAddedBelow);
                    itemHasToBeAddedBelow = false;
                }
                doc->getTree().goUp();
            }
        }
        if (ovariesObject.isMember(RIGHT_OVARY_VOLUME_GROUP) && !ovariesObject[RIGHT_OVARY_VOLUME_GROUP].empty()) {
            Json::Value rightOvaryVolumeGroup = ovariesObject[RIGHT_OVARY_VOLUME_GROUP];
            doc->getTree().addContentItem(DSRTypes::RT_contains,  DSRTypes::VT_Container,DSRTypes::AM_afterCurrent);
            auto container = DSRBasicCodedEntry("15497006", "SCT", "Ovary");
            auto result = doc->getTree().getCurrentContentItem().setConceptName(container);
            if (result.good()) {
                bool itemHasToBeAddedBelow = true;
                if (rightOvaryVolumeGroup.isMember(VOLUME) && !rightOvaryVolumeGroup[VOLUME].empty()) {
                    const auto lnCode = DSRCodedEntryValue("12165-7", "LN", "Right Ovary Volume");
                    const auto value = DSRNumericMeasurementValue(
                            rightOvaryVolumeGroup[VOLUME][valueKey].asString(),
                            CODE_UCUM_ml);
                    dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent, lnCode, value);
                    itemHasToBeAddedBelow = false;
                    dicomSr->addDerivations(rightOvaryVolumeGroup[VOLUME][DERIVATION].asString());
                    doc->getTree().goUp();
                }
                if (rightOvaryVolumeGroup.isMember(LENGTH) && !rightOvaryVolumeGroup[LENGTH].empty()) {
                    Json::Value lengthObject = rightOvaryVolumeGroup[LENGTH];
                    auto lnCode = DSRCodedEntryValue("11841-4", "LN", "Right Ovary Length");
                    addLWHMeasurements(dicomSr, lengthObject, lnCode, itemHasToBeAddedBelow);
                    itemHasToBeAddedBelow = false;
                }
                if (rightOvaryVolumeGroup.isMember(HEIGHT) && !rightOvaryVolumeGroup[HEIGHT].empty()) {
                    Json::Value heightObject = rightOvaryVolumeGroup[HEIGHT];
                    auto lnCode = DSRCodedEntryValue("11858-8", "LN", "Right Ovary Height");
                    addLWHMeasurements(dicomSr, heightObject, lnCode, itemHasToBeAddedBelow);
                    itemHasToBeAddedBelow = false;
                }
                if (rightOvaryVolumeGroup.isMember(WIDTH) && !rightOvaryVolumeGroup[WIDTH].empty()) {
                    Json::Value widthObject = rightOvaryVolumeGroup[WIDTH];
                    auto lnCode = DSRCodedEntryValue("11830-7", "LN", "Right Ovary Width");
                    addLWHMeasurements(dicomSr, widthObject, lnCode, itemHasToBeAddedBelow);
                    itemHasToBeAddedBelow = false;
                }
                doc->getTree().goUp();
            }
            doc->getTree().goUp();
        }
        doc->getTree().goUp();
    }

    if (root.isMember(RIGHT_UTERINE_ARTERY) && !root[RIGHT_UTERINE_ARTERY].empty()) {
        addFindingContainer(dicomSr, DSRCodedEntryValue("281496003", "SCT", "Pelvic Vascular Structure"));
        doc->getTree().addContentItem(DSRTypes::RT_contains,  DSRTypes::VT_Container,DSRTypes::AM_afterCurrent);
        auto container = DSRBasicCodedEntry("91079009", "SCT", "Uterine Artery");
        auto result = doc->getTree().getCurrentContentItem().setConceptName(container);
        if (result.good()) {
            addVtiPsvEdvGroup(dicomSr, root[RIGHT_UTERINE_ARTERY]);
            doc->getTree().goUp();
            doc->getTree().goUp();
        }
    }
    if (root.isMember(LEFT_UTERINE_ARTERY) && !root[LEFT_UTERINE_ARTERY].empty()) {
        addFindingContainer(dicomSr, DSRCodedEntryValue("281496003", "SCT", "Pelvic Vascular Structure"));
        doc->getTree().addContentItem(DSRTypes::RT_contains,  DSRTypes::VT_Container,DSRTypes::AM_afterCurrent);
        auto container = DSRBasicCodedEntry("91079009", "SCT", "Uterine Artery");
        auto result = doc->getTree().getCurrentContentItem().setConceptName(container);
        if (result.good()) {
            addVtiPsvEdvGroup(dicomSr, root[LEFT_UTERINE_ARTERY]);
            doc->getTree().goUp();
            doc->getTree().goUp();
        }
    }
    if (root.isMember(RIGHT_OVARIAN_ARTERY) && !root[RIGHT_OVARIAN_ARTERY].empty()) {
        addFindingContainer(dicomSr, DSRCodedEntryValue("281496003", "SCT", "Pelvic Vascular Structure"));
        doc->getTree().addContentItem(DSRTypes::RT_contains,  DSRTypes::VT_Container,DSRTypes::AM_afterCurrent);
        auto container = DSRBasicCodedEntry("12052000", "SCT", "Ovarian Artery");
        auto result = doc->getTree().getCurrentContentItem().setConceptName(container);
        if (result.good()) {
            addVtiPsvEdvGroup(dicomSr, root[RIGHT_OVARIAN_ARTERY]);
            doc->getTree().goUp();
            doc->getTree().goUp();
        }
    }
    if (root.isMember(LEFT_OVARIAN_ARTERY) && !root[LEFT_OVARIAN_ARTERY].empty()) {
        addFindingContainer(dicomSr, DSRCodedEntryValue("281496003", "SCT", "Pelvic Vascular Structure"));
        doc->getTree().addContentItem(DSRTypes::RT_contains,  DSRTypes::VT_Container,DSRTypes::AM_afterCurrent);
        auto container = DSRBasicCodedEntry("12052000", "SCT", "Ovarian Artery");
        auto result = doc->getTree().getCurrentContentItem().setConceptName(container);
        if (result.good()) {
            addVtiPsvEdvGroup(dicomSr, root[LEFT_OVARIAN_ARTERY]);
            doc->getTree().goUp();
            doc->getTree().goUp();
        }
    }

    if (root.isMember(PRE_VOID_BLADDER) && !root[PRE_VOID_BLADDER].empty()) {
        addFindingContainer(dicomSr, DSRCodedEntryValue("89837001", "SCT", "Bladder"));
        Json::Value bladderObject = root[PRE_VOID_BLADDER];
        doc->getTree().addContentItem(DSRTypes::RT_contains,  DSRTypes::VT_Container,DSRTypes::AM_afterCurrent);
        auto container = DSRBasicCodedEntry("89837001", "SCT", "Pre-Void Bladder");
        doc->getTree().getCurrentContentItem().setConceptName(container);
        bool itemHasToBeAddedBelow = true;
        if (bladderObject.isMember(VOLUME) && !bladderObject[VOLUME].empty()) {
            const auto lnCode = DSRCodedEntryValue("EN-161-162-163", "99EN", "Pre-Void Bladder Volume");
            const auto value = DSRNumericMeasurementValue(
                    bladderObject[VOLUME][valueKey].asString(),
                    CODE_UCUM_ml);
            dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent, lnCode, value);
            itemHasToBeAddedBelow = false;
            dicomSr->addDerivations(bladderObject[VOLUME][DERIVATION].asString());
            doc->getTree().goUp();
        }
        if (bladderObject.isMember(LENGTH) && !bladderObject[LENGTH].empty()) {
            Json::Value lengthObject = bladderObject[LENGTH];
            auto lnCode = DSRCodedEntryValue("EN-161", "99EN", "Pre-Void Bladder Length");
            addLWHMeasurements(dicomSr, lengthObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        if (bladderObject.isMember(HEIGHT) && !bladderObject[HEIGHT].empty()) {
            Json::Value heightObject = bladderObject[HEIGHT];
            auto lnCode = DSRCodedEntryValue("EN-163", "99EN", "Pre-Void Bladder Height");
            addLWHMeasurements(dicomSr, heightObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        if (bladderObject.isMember(WIDTH) && !bladderObject[WIDTH].empty()) {
            Json::Value widthObject = bladderObject[WIDTH];
            auto lnCode = DSRCodedEntryValue("EN-162", "99EN", "Pre-Void Bladder Width");
            addLWHMeasurements(dicomSr, widthObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
    }
    if (root.isMember(POST_VOID_BLADDER) && !root[POST_VOID_BLADDER].empty()) {
        doc->getTree().gotoRoot();
        addFindingContainer(dicomSr, DSRCodedEntryValue("89837001", "SCT", "Bladder"));
        Json::Value bladderObject = root[POST_VOID_BLADDER];
        doc->getTree().addContentItem(DSRTypes::RT_contains,  DSRTypes::VT_Container,DSRTypes::AM_afterCurrent);
        auto container = DSRBasicCodedEntry("89837001", "SCT", "Post-Void Bladder");
        doc->getTree().getCurrentContentItem().setConceptName(container);
        bool itemHasToBeAddedBelow = true;
        if (bladderObject.isMember(VOLUME) && !bladderObject[VOLUME].empty()) {
            const auto lnCode = DSRCodedEntryValue("EN-164-165-166", "99EN", "Post-Void Bladder Volume");
            const auto value = DSRNumericMeasurementValue(
                    bladderObject[VOLUME][valueKey].asString(),
                    CODE_UCUM_ml);
            dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent, lnCode, value);
            itemHasToBeAddedBelow = false;
            dicomSr->addDerivations(bladderObject[VOLUME][DERIVATION].asString());
            doc->getTree().goUp();
        }
        if (bladderObject.isMember(LENGTH) && !bladderObject[LENGTH].empty()) {
            Json::Value lengthObject = bladderObject[LENGTH];
            auto lnCode = DSRCodedEntryValue("EN-164", "99EN", "Post-Void Bladder Length");
            addLWHMeasurements(dicomSr, lengthObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        if (bladderObject.isMember(HEIGHT) && !bladderObject[HEIGHT].empty()) {
            Json::Value heightObject = bladderObject[HEIGHT];
            auto lnCode = DSRCodedEntryValue("EN-166", "99EN", "Post-Void Bladder Height");
            addLWHMeasurements(dicomSr, heightObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        if (bladderObject.isMember(WIDTH) && !bladderObject[WIDTH].empty()) {
            Json::Value widthObject = bladderObject[WIDTH];
            auto lnCode = DSRCodedEntryValue("EN-165", "99EN", "Post-Void Bladder Width");
            addLWHMeasurements(dicomSr, widthObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        doc->getTree().goUp();
        doc->getTree().goUp();
    }

    if (root.isMember(FIBROMA_ONE) && !root[FIBROMA_ONE].empty()) {
        doc->getTree().gotoRoot();
        addFindingContainer(dicomSr, DSRCodedEntryValue("89837001", "SCT", "Bladder"));
        Json::Value bladderObject = root[FIBROMA_ONE];
        doc->getTree().addContentItem(DSRTypes::RT_contains,  DSRTypes::VT_Container,DSRTypes::AM_afterCurrent);
        auto container = DSRBasicCodedEntry("89837001", "SCT", FIBROMA_ONE);
        doc->getTree().getCurrentContentItem().setConceptName(container);
        bool itemHasToBeAddedBelow = true;
        if (bladderObject.isMember(VOLUME) && !bladderObject[VOLUME].empty()) {
            const auto lnCode = DSRCodedEntryValue("EN-125-126-127", "99EN", (OFString)FIBROMA_ONE + " Volume");
            const auto value = DSRNumericMeasurementValue(
                    bladderObject[VOLUME][valueKey].asString(),
                    CODE_UCUM_ml);
            dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent, lnCode, value);
            itemHasToBeAddedBelow = false;
            dicomSr->addDerivations(bladderObject[VOLUME][DERIVATION].asString());
            doc->getTree().goUp();
        }
        if (bladderObject.isMember(LENGTH) && !bladderObject[LENGTH].empty()) {
            Json::Value lengthObject = bladderObject[LENGTH];
            auto lnCode = DSRCodedEntryValue("EN-125", "99EN", (OFString)FIBROMA_ONE + " Length");
            addLWHMeasurements(dicomSr, lengthObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        if (bladderObject.isMember(HEIGHT) && !bladderObject[HEIGHT].empty()) {
            Json::Value heightObject = bladderObject[HEIGHT];
            auto lnCode = DSRCodedEntryValue("EN-127", "99EN", (OFString)FIBROMA_ONE + " Height");
            addLWHMeasurements(dicomSr, heightObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        if (bladderObject.isMember(WIDTH) && !bladderObject[WIDTH].empty()) {
            Json::Value widthObject = bladderObject[WIDTH];
            auto lnCode = DSRCodedEntryValue("EN-126", "99EN", (OFString)FIBROMA_ONE + " Width");
            addLWHMeasurements(dicomSr, widthObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        doc->getTree().goUp();
        doc->getTree().goUp();
    }

    if (root.isMember(FIBROMA_TWO) && !root[FIBROMA_TWO].empty()) {
        doc->getTree().gotoRoot();
        addFindingContainer(dicomSr, DSRCodedEntryValue("89837001", "SCT", "Bladder"));
        Json::Value bladderObject = root[FIBROMA_TWO];
        doc->getTree().addContentItem(DSRTypes::RT_contains,  DSRTypes::VT_Container,DSRTypes::AM_afterCurrent);
        auto container = DSRBasicCodedEntry("89837001", "SCT", FIBROMA_TWO);
        doc->getTree().getCurrentContentItem().setConceptName(container);
        bool itemHasToBeAddedBelow = true;
        if (bladderObject.isMember(VOLUME) && !bladderObject[VOLUME].empty()) {
            const auto lnCode = DSRCodedEntryValue("EN-128-129-130", "99EN", (OFString)FIBROMA_TWO + " Volume");
            const auto value = DSRNumericMeasurementValue(
                    bladderObject[VOLUME][valueKey].asString(),
                    CODE_UCUM_ml);
            dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent, lnCode, value);
            itemHasToBeAddedBelow = false;
            dicomSr->addDerivations(bladderObject[VOLUME][DERIVATION].asString());
            doc->getTree().goUp();
        }
        if (bladderObject.isMember(LENGTH) && !bladderObject[LENGTH].empty()) {
            Json::Value lengthObject = bladderObject[LENGTH];
            auto lnCode = DSRCodedEntryValue("EN-128", "99EN", (OFString)FIBROMA_TWO + " Length");
            addLWHMeasurements(dicomSr, lengthObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        if (bladderObject.isMember(HEIGHT) && !bladderObject[HEIGHT].empty()) {
            Json::Value heightObject = bladderObject[HEIGHT];
            auto lnCode = DSRCodedEntryValue("EN-130", "99EN", (OFString)FIBROMA_TWO + " Height");
            addLWHMeasurements(dicomSr, heightObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        if (bladderObject.isMember(WIDTH) && !bladderObject[WIDTH].empty()) {
            Json::Value widthObject = bladderObject[WIDTH];
            auto lnCode = DSRCodedEntryValue("EN-129", "99EN", (OFString)FIBROMA_TWO + " Width");
            addLWHMeasurements(dicomSr, widthObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        doc->getTree().goUp();
        doc->getTree().goUp();
    }

    if (root.isMember(RIGHT_MASS_ONE) && !root[RIGHT_MASS_ONE].empty()) {
        doc->getTree().gotoRoot();
        addFindingContainer(dicomSr, DSRBasicCodedEntry("15497006", "SCT", "Ovary"));
        Json::Value bladderObject = root[RIGHT_MASS_ONE];
        doc->getTree().addContentItem(DSRTypes::RT_contains,  DSRTypes::VT_Container,DSRTypes::AM_afterCurrent);
        auto container = DSRBasicCodedEntry("111462", "DCM", SOLID_MASS);
        doc->getTree().getCurrentContentItem().setConceptName(container);
        bool itemHasToBeAddedBelow = true;
        if (bladderObject.isMember(LATERALITY)) {
            dicomSr->addConceptModCode(
                    DSRTypes::AM_belowCurrent,
                    DSRCodedEntryValue("272741003", "SCT", LATERALITY),
                    *getLateralityValue(bladderObject[LATERALITY].asString()).get());
            doc->getTree().goUp();
        }
        if (bladderObject.isMember(VOLUME) && !bladderObject[VOLUME].empty()) {
            const auto lnCode = DSRCodedEntryValue("EN-134-135-136", "99EN", (OFString)RIGHT_MASS_ONE + " Volume");
            const auto value = DSRNumericMeasurementValue(
                    bladderObject[VOLUME][valueKey].asString(),
                    CODE_UCUM_ml);
            dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent, lnCode, value);
            itemHasToBeAddedBelow = false;
            dicomSr->addDerivations(bladderObject[VOLUME][DERIVATION].asString());
            doc->getTree().goUp();
        }
        if (bladderObject.isMember(LENGTH) && !bladderObject[LENGTH].empty()) {
            Json::Value lengthObject = bladderObject[LENGTH];
            auto lnCode = DSRCodedEntryValue("EN-134", "99EN", (OFString)RIGHT_MASS_ONE + " Length");
            addLWHMeasurements(dicomSr, lengthObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        if (bladderObject.isMember(HEIGHT) && !bladderObject[HEIGHT].empty()) {
            Json::Value heightObject = bladderObject[HEIGHT];
            auto lnCode = DSRCodedEntryValue("EN-136", "99EN", (OFString)RIGHT_MASS_ONE + " Height");
            addLWHMeasurements(dicomSr, heightObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        if (bladderObject.isMember(WIDTH) && !bladderObject[WIDTH].empty()) {
            Json::Value widthObject = bladderObject[WIDTH];
            auto lnCode = DSRCodedEntryValue("EN-135", "99EN", (OFString)RIGHT_MASS_ONE + " Width");
            addLWHMeasurements(dicomSr, widthObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        doc->getTree().goUp();
        doc->getTree().goUp();
    }

    if (root.isMember(RIGHT_MASS_TWO) && !root[RIGHT_MASS_TWO].empty()) {
        doc->getTree().gotoRoot();
        addFindingContainer(dicomSr, DSRBasicCodedEntry("15497006", "SCT", "Ovary"));
        Json::Value bladderObject = root[RIGHT_MASS_TWO];
        doc->getTree().addContentItem(DSRTypes::RT_contains,  DSRTypes::VT_Container,DSRTypes::AM_afterCurrent);
        auto container = DSRBasicCodedEntry("111462", "DCM", SOLID_MASS);
        doc->getTree().getCurrentContentItem().setConceptName(container);
        bool itemHasToBeAddedBelow = true;
        if (bladderObject.isMember(LATERALITY)) {
            dicomSr->addConceptModCode(
                    DSRTypes::AM_belowCurrent,
                    DSRCodedEntryValue("272741003", "SCT", LATERALITY),
                    *getLateralityValue(bladderObject[LATERALITY].asString()).get());
            doc->getTree().goUp();
        }
        if (bladderObject.isMember(VOLUME) && !bladderObject[VOLUME].empty()) {
            const auto lnCode = DSRCodedEntryValue("EN-137-138-139", "99EN", (OFString)RIGHT_MASS_TWO + " Volume");
            const auto value = DSRNumericMeasurementValue(
                    bladderObject[VOLUME][valueKey].asString(),
                    CODE_UCUM_ml);
            dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent, lnCode, value);
            itemHasToBeAddedBelow = false;
            dicomSr->addDerivations(bladderObject[VOLUME][DERIVATION].asString());
            doc->getTree().goUp();
        }
        if (bladderObject.isMember(LENGTH) && !bladderObject[LENGTH].empty()) {
            Json::Value lengthObject = bladderObject[LENGTH];
            auto lnCode = DSRCodedEntryValue("EN-137", "99EN", (OFString)RIGHT_MASS_TWO + " Length");
            addLWHMeasurements(dicomSr, lengthObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        if (bladderObject.isMember(HEIGHT) && !bladderObject[HEIGHT].empty()) {
            Json::Value heightObject = bladderObject[HEIGHT];
            auto lnCode = DSRCodedEntryValue("EN-139", "99EN", (OFString)RIGHT_MASS_TWO + " Height");
            addLWHMeasurements(dicomSr, heightObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        if (bladderObject.isMember(WIDTH) && !bladderObject[WIDTH].empty()) {
            Json::Value widthObject = bladderObject[WIDTH];
            auto lnCode = DSRCodedEntryValue("EN-138", "99EN", (OFString)RIGHT_MASS_TWO + " Width");
            addLWHMeasurements(dicomSr, widthObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        doc->getTree().goUp();
        doc->getTree().goUp();
    }

    if (root.isMember(RIGHT_OVARIAN_CYST_ONE) && !root[RIGHT_OVARIAN_CYST_ONE].empty()) {
        doc->getTree().gotoRoot();
        addFindingContainer(dicomSr, DSRBasicCodedEntry("15497006", "SCT", "Ovary"));
        Json::Value bladderObject = root[RIGHT_OVARIAN_CYST_ONE];
        doc->getTree().addContentItem(DSRTypes::RT_contains,  DSRTypes::VT_Container,DSRTypes::AM_afterCurrent);
        auto container = DSRBasicCodedEntry("111460", "DCM", COMPLEX_CYST);
        doc->getTree().getCurrentContentItem().setConceptName(container);
        bool itemHasToBeAddedBelow = true;
        if (bladderObject.isMember(LATERALITY)) {
            dicomSr->addConceptModCode(
                    DSRTypes::AM_belowCurrent,
                    DSRCodedEntryValue("272741003", "SCT", LATERALITY),
                    *getLateralityValue(bladderObject[LATERALITY].asString()).get());
            doc->getTree().goUp();
        }
        if (bladderObject.isMember(VOLUME) && !bladderObject[VOLUME].empty()) {
            const auto lnCode = DSRCodedEntryValue("EN-140-141-142", "99EN", (OFString)RIGHT_OVARIAN_CYST_ONE + " Volume");
            const auto value = DSRNumericMeasurementValue(
                    bladderObject[VOLUME][valueKey].asString(),
                    CODE_UCUM_ml);
            dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent, lnCode, value);
            itemHasToBeAddedBelow = false;
            dicomSr->addDerivations(bladderObject[VOLUME][DERIVATION].asString());
            doc->getTree().goUp();
        }
        if (bladderObject.isMember(LENGTH) && !bladderObject[LENGTH].empty()) {
            Json::Value lengthObject = bladderObject[LENGTH];
            auto lnCode = DSRCodedEntryValue("EN-140", "99EN", (OFString)RIGHT_OVARIAN_CYST_ONE + " Length");
            addLWHMeasurements(dicomSr, lengthObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        if (bladderObject.isMember(HEIGHT) && !bladderObject[HEIGHT].empty()) {
            Json::Value heightObject = bladderObject[HEIGHT];
            auto lnCode = DSRCodedEntryValue("EN-142", "99EN", (OFString)RIGHT_OVARIAN_CYST_ONE + " Height");
            addLWHMeasurements(dicomSr, heightObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        if (bladderObject.isMember(WIDTH) && !bladderObject[WIDTH].empty()) {
            Json::Value widthObject = bladderObject[WIDTH];
            auto lnCode = DSRCodedEntryValue("EN-141", "99EN", (OFString)RIGHT_OVARIAN_CYST_ONE + " Width");
            addLWHMeasurements(dicomSr, widthObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        doc->getTree().goUp();
        doc->getTree().goUp();
    }

    if (root.isMember(RIGHT_OVARIAN_CYST_TWO) && !root[RIGHT_OVARIAN_CYST_TWO].empty()) {
        doc->getTree().gotoRoot();
        addFindingContainer(dicomSr, DSRBasicCodedEntry("15497006", "SCT", "Ovary"));
        Json::Value bladderObject = root[RIGHT_OVARIAN_CYST_TWO];
        doc->getTree().addContentItem(DSRTypes::RT_contains,  DSRTypes::VT_Container,DSRTypes::AM_afterCurrent);
        auto container = DSRBasicCodedEntry("111460", "DCM", COMPLEX_CYST);
        doc->getTree().getCurrentContentItem().setConceptName(container);
        bool itemHasToBeAddedBelow = true;
        if (bladderObject.isMember(LATERALITY)) {
            dicomSr->addConceptModCode(
                    DSRTypes::AM_belowCurrent,
                    DSRCodedEntryValue("272741003", "SCT", LATERALITY),
                    *getLateralityValue(bladderObject[LATERALITY].asString()).get());
            doc->getTree().goUp();
        }
        if (bladderObject.isMember(VOLUME) && !bladderObject[VOLUME].empty()) {
            const auto lnCode = DSRCodedEntryValue("EN-143-144-145", "99EN", (OFString)RIGHT_OVARIAN_CYST_TWO + " Volume");
            const auto value = DSRNumericMeasurementValue(
                    bladderObject[VOLUME][valueKey].asString(),
                    CODE_UCUM_ml);
            dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent, lnCode, value);
            itemHasToBeAddedBelow = false;
            dicomSr->addDerivations(bladderObject[VOLUME][DERIVATION].asString());
            doc->getTree().goUp();
        }
        if (bladderObject.isMember(LENGTH) && !bladderObject[LENGTH].empty()) {
            Json::Value lengthObject = bladderObject[LENGTH];
            auto lnCode = DSRCodedEntryValue("EN-143", "99EN", (OFString)RIGHT_OVARIAN_CYST_TWO + " Length");
            addLWHMeasurements(dicomSr, lengthObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        if (bladderObject.isMember(HEIGHT) && !bladderObject[HEIGHT].empty()) {
            Json::Value heightObject = bladderObject[HEIGHT];
            auto lnCode = DSRCodedEntryValue("EN-145", "99EN", (OFString)RIGHT_OVARIAN_CYST_TWO + " Height");
            addLWHMeasurements(dicomSr, heightObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        if (bladderObject.isMember(WIDTH) && !bladderObject[WIDTH].empty()) {
            Json::Value widthObject = bladderObject[WIDTH];
            auto lnCode = DSRCodedEntryValue("EN-144", "99EN", (OFString)RIGHT_OVARIAN_CYST_TWO + " Width");
            addLWHMeasurements(dicomSr, widthObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        doc->getTree().goUp();
        doc->getTree().goUp();
    }

    if (root.isMember(LEFT_MASS_ONE) && !root[LEFT_MASS_ONE].empty()) {
        doc->getTree().gotoRoot();
        addFindingContainer(dicomSr, DSRBasicCodedEntry("15497006", "SCT", "Ovary"));
        Json::Value bladderObject = root[LEFT_MASS_ONE];
        doc->getTree().addContentItem(DSRTypes::RT_contains,  DSRTypes::VT_Container,DSRTypes::AM_afterCurrent);
        auto container = DSRBasicCodedEntry("111462", "DCM", SOLID_MASS);
        doc->getTree().getCurrentContentItem().setConceptName(container);
        bool itemHasToBeAddedBelow = true;
        if (bladderObject.isMember(LATERALITY)) {
            dicomSr->addConceptModCode(
                    DSRTypes::AM_belowCurrent,
                    DSRCodedEntryValue("272741003", "SCT", LATERALITY),
                    *getLateralityValue(bladderObject[LATERALITY].asString()).get());
            doc->getTree().goUp();
        }
        if (bladderObject.isMember(VOLUME) && !bladderObject[VOLUME].empty()) {
            const auto lnCode = DSRCodedEntryValue("EN-149-150-151", "99EN", (OFString)LEFT_MASS_ONE + " Volume");
            const auto value = DSRNumericMeasurementValue(
                    bladderObject[VOLUME][valueKey].asString(),
                    CODE_UCUM_ml);
            dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent, lnCode, value);
            itemHasToBeAddedBelow = false;
            dicomSr->addDerivations(bladderObject[VOLUME][DERIVATION].asString());
            doc->getTree().goUp();
        }
        if (bladderObject.isMember(LENGTH) && !bladderObject[LENGTH].empty()) {
            Json::Value lengthObject = bladderObject[LENGTH];
            auto lnCode = DSRCodedEntryValue("EN-149", "99EN", (OFString)LEFT_MASS_ONE + " Length");
            addLWHMeasurements(dicomSr, lengthObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        if (bladderObject.isMember(HEIGHT) && !bladderObject[HEIGHT].empty()) {
            Json::Value heightObject = bladderObject[HEIGHT];
            auto lnCode = DSRCodedEntryValue("EN-151", "99EN", (OFString)LEFT_MASS_ONE + " Height");
            addLWHMeasurements(dicomSr, heightObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        if (bladderObject.isMember(WIDTH) && !bladderObject[WIDTH].empty()) {
            Json::Value widthObject = bladderObject[WIDTH];
            auto lnCode = DSRCodedEntryValue("EN-150", "99EN", (OFString)LEFT_MASS_ONE + " Width");
            addLWHMeasurements(dicomSr, widthObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        doc->getTree().goUp();
        doc->getTree().goUp();
    }

    if (root.isMember(LEFT_MASS_TWO) && !root[LEFT_MASS_TWO].empty()) {
        doc->getTree().gotoRoot();
        addFindingContainer(dicomSr, DSRBasicCodedEntry("15497006", "SCT", "Ovary"));
        Json::Value bladderObject = root[LEFT_MASS_TWO];
        doc->getTree().addContentItem(DSRTypes::RT_contains,  DSRTypes::VT_Container,DSRTypes::AM_afterCurrent);
        auto container = DSRBasicCodedEntry("111462", "DCM", SOLID_MASS);
        doc->getTree().getCurrentContentItem().setConceptName(container);
        bool itemHasToBeAddedBelow = true;
        if (bladderObject.isMember(LATERALITY)) {
            dicomSr->addConceptModCode(
                    DSRTypes::AM_belowCurrent,
                    DSRCodedEntryValue("272741003", "SCT", LATERALITY),
                    *getLateralityValue(bladderObject[LATERALITY].asString()).get());
            doc->getTree().goUp();
        }
        if (bladderObject.isMember(VOLUME) && !bladderObject[VOLUME].empty()) {
            const auto lnCode = DSRCodedEntryValue("EN-152-153-154", "99EN", (OFString)LEFT_MASS_TWO + " Volume");
            const auto value = DSRNumericMeasurementValue(
                    bladderObject[VOLUME][valueKey].asString(),
                    CODE_UCUM_ml);
            dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent, lnCode, value);
            itemHasToBeAddedBelow = false;
            dicomSr->addDerivations(bladderObject[VOLUME][DERIVATION].asString());
            doc->getTree().goUp();
        }
        if (bladderObject.isMember(LENGTH) && !bladderObject[LENGTH].empty()) {
            Json::Value lengthObject = bladderObject[LENGTH];
            auto lnCode = DSRCodedEntryValue("EN-152", "99EN", (OFString)LEFT_MASS_TWO + " Length");
            addLWHMeasurements(dicomSr, lengthObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        if (bladderObject.isMember(HEIGHT) && !bladderObject[HEIGHT].empty()) {
            Json::Value heightObject = bladderObject[HEIGHT];
            auto lnCode = DSRCodedEntryValue("EN-154", "99EN", (OFString)LEFT_MASS_TWO + " Height");
            addLWHMeasurements(dicomSr, heightObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        if (bladderObject.isMember(WIDTH) && !bladderObject[WIDTH].empty()) {
            Json::Value widthObject = bladderObject[WIDTH];
            auto lnCode = DSRCodedEntryValue("EN-153", "99EN", (OFString)LEFT_MASS_TWO + " Width");
            addLWHMeasurements(dicomSr, widthObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        doc->getTree().goUp();
        doc->getTree().goUp();
    }

    if (root.isMember(LEFT_OVARIAN_CYST_ONE) && !root[LEFT_OVARIAN_CYST_ONE].empty()) {
        doc->getTree().gotoRoot();
        addFindingContainer(dicomSr, DSRBasicCodedEntry("15497006", "SCT", "Ovary"));
        Json::Value bladderObject = root[LEFT_OVARIAN_CYST_ONE];
        doc->getTree().addContentItem(DSRTypes::RT_contains,  DSRTypes::VT_Container,DSRTypes::AM_afterCurrent);
        auto container = DSRBasicCodedEntry("111460", "DCM", COMPLEX_CYST);
        doc->getTree().getCurrentContentItem().setConceptName(container);
        bool itemHasToBeAddedBelow = true;
        if (bladderObject.isMember(LATERALITY)) {
            dicomSr->addConceptModCode(
                    DSRTypes::AM_belowCurrent,
                    DSRCodedEntryValue("272741003", "SCT", LATERALITY),
                    *getLateralityValue(bladderObject[LATERALITY].asString()).get());
            doc->getTree().goUp();
        }
        if (bladderObject.isMember(VOLUME) && !bladderObject[VOLUME].empty()) {
            const auto lnCode = DSRCodedEntryValue("EN-155-156-157", "99EN", (OFString)LEFT_OVARIAN_CYST_ONE + " Volume");
            const auto value = DSRNumericMeasurementValue(
                    bladderObject[VOLUME][valueKey].asString(),
                    CODE_UCUM_ml);
            dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent, lnCode, value);
            itemHasToBeAddedBelow = false;
            dicomSr->addDerivations(bladderObject[VOLUME][DERIVATION].asString());
            doc->getTree().goUp();
        }
        if (bladderObject.isMember(LENGTH) && !bladderObject[LENGTH].empty()) {
            Json::Value lengthObject = bladderObject[LENGTH];
            auto lnCode = DSRCodedEntryValue("EN-155", "99EN", (OFString)LEFT_OVARIAN_CYST_ONE + " Length");
            addLWHMeasurements(dicomSr, lengthObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        if (bladderObject.isMember(HEIGHT) && !bladderObject[HEIGHT].empty()) {
            Json::Value heightObject = bladderObject[HEIGHT];
            auto lnCode = DSRCodedEntryValue("EN-157", "99EN", (OFString)LEFT_OVARIAN_CYST_ONE + " Height");
            addLWHMeasurements(dicomSr, heightObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        if (bladderObject.isMember(WIDTH) && !bladderObject[WIDTH].empty()) {
            Json::Value widthObject = bladderObject[WIDTH];
            auto lnCode = DSRCodedEntryValue("EN-156", "99EN", (OFString)LEFT_OVARIAN_CYST_ONE + " Width");
            addLWHMeasurements(dicomSr, widthObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        doc->getTree().goUp();
        doc->getTree().goUp();
    }

    if (root.isMember(LEFT_OVARIAN_CYST_TWO) && !root[LEFT_OVARIAN_CYST_TWO].empty()) {
        doc->getTree().gotoRoot();
        addFindingContainer(dicomSr, DSRBasicCodedEntry("15497006", "SCT", "Ovary"));
        Json::Value bladderObject = root[LEFT_OVARIAN_CYST_TWO];
        doc->getTree().addContentItem(DSRTypes::RT_contains,  DSRTypes::VT_Container,DSRTypes::AM_afterCurrent);
        auto container = DSRBasicCodedEntry("111460", "DCM", COMPLEX_CYST);
        doc->getTree().getCurrentContentItem().setConceptName(container);
        bool itemHasToBeAddedBelow = true;
        if (bladderObject.isMember(LATERALITY)) {
            dicomSr->addConceptModCode(
                    DSRTypes::AM_belowCurrent,
                    DSRCodedEntryValue("272741003", "SCT", LATERALITY),
                    *getLateralityValue(bladderObject[LATERALITY].asString()).get());
            doc->getTree().goUp();
        }
        if (bladderObject.isMember(VOLUME) && !bladderObject[VOLUME].empty()) {
            const auto lnCode = DSRCodedEntryValue("EN-158-159-160", "99EN", (OFString)LEFT_OVARIAN_CYST_TWO + " Volume");
            const auto value = DSRNumericMeasurementValue(
                    bladderObject[VOLUME][valueKey].asString(),
                    CODE_UCUM_ml);
            dicomSr->addNumMeasurement(DSRTypes::AM_belowCurrent, lnCode, value);
            itemHasToBeAddedBelow = false;
            dicomSr->addDerivations(bladderObject[VOLUME][DERIVATION].asString());
            doc->getTree().goUp();
        }
        if (bladderObject.isMember(LENGTH) && !bladderObject[LENGTH].empty()) {
            Json::Value lengthObject = bladderObject[LENGTH];
            auto lnCode = DSRCodedEntryValue("EN-158", "99EN", (OFString)LEFT_OVARIAN_CYST_TWO + " Length");
            addLWHMeasurements(dicomSr, lengthObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        if (bladderObject.isMember(HEIGHT) && !bladderObject[HEIGHT].empty()) {
            Json::Value heightObject = bladderObject[HEIGHT];
            auto lnCode = DSRCodedEntryValue("EN-160", "99EN", (OFString)LEFT_OVARIAN_CYST_TWO + " Height");
            addLWHMeasurements(dicomSr, heightObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        if (bladderObject.isMember(WIDTH) && !bladderObject[WIDTH].empty()) {
            Json::Value widthObject = bladderObject[WIDTH];
            auto lnCode = DSRCodedEntryValue("EN-159", "99EN", (OFString)LEFT_OVARIAN_CYST_TWO + " Width");
            addLWHMeasurements(dicomSr, widthObject, lnCode, itemHasToBeAddedBelow);
            itemHasToBeAddedBelow = false;
        }
        doc->getTree().goUp();
        doc->getTree().goUp();
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
DCMTKUTIL_METHOD(createDicomSrGyn)(JNIEnv* env,jobject thiz,jstring jsonReport,jstring outputFilePath){


    //createSR(env,outputFilePath);
    return createGynSr(env,outputFilePath,jsonReport);
}
