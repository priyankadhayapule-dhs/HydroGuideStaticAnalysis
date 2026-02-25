//
// Created by himanshumistri on 08/02/22.
//

#ifndef ECHONOUSAPP_DICOMSR_H
#define ECHONOUSAPP_DICOMSR_H

#define TAG "US2DicomSR"
#define TAG_VAS "VascularSR"

#include <string>
#include <unordered_map>

#define SELECTION_STATUS "selectionStatus"
#define CODE_IMAGING_MODE DSRCodedEntryValue("G-0373","SRT","Image Mode")
#define CODE_IMAGING_MODE_2D DSRCodedEntryValue("G-03A2","SRT","2D mode")
#define CODE_IMAGING_MODE_TDI DSRCodedEntryValue("P5-B0128","SRT","Tissue Doppler Imaging")
#define CODE_IMAGING_MODE_COLOR DSRCodedEntryValue("R-409E2","SRT","Doppler Color Flow")
#define CODE_IMAGING_MODE_CW DSRCodedEntryValue("R-409E3","SRT","Doppler Continuous Wave")
#define CODE_UCUM_Ratio     DSRBasicCodedEntry("{ratio}", "UCUM", "ratio")
#define CODE_UCUM_MMHG DSRBasicCodedEntry("mm[Hg]", "UCUM", "mmHg")

#define UNIT_CM "cm"
#define UNIT_MM "mm"
#define UNIT_M_PER_SEC "m/s"
#define UNIT_CM_PER_SEC "cm/s"
#define FETUS_ID "Fetus ID"
#define FETAL_BIOMETRY "Fetal Biometry"
#define PatientCharacteristicsCode "121118"
#define ProcedureSummaryCode "121111"
#define FetusSummaryCode "125008"
#define FetalBiometryCode "125002"
#define FetalBiometryGroupCode "125005"
#define FetalBiophysicalProfileCode "125006"
#define EarlyGestationCode "125009"
#define AmnioticSacCode "121070"
#define PelvisAndUterusCode "125011"
#define MEAN "Mean"
#define CALCULATED "Calculated"
#define MEASURED "Measured"
#define MAXIMUM "Maximum"


#include <json/value.h>
#include "../../prebuilts/dcmtk/include/dcmtk/dcmsr/dsrdoc.h"
#include "dcmtk/dcmdata/dctk.h"
#include "../../prebuilts/dcmtk/include/dcmtk/dcmsr/dsrtypes.h"
using namespace std;

//Vascular Define
#define Carotid "Carotid"
#define PSV_EDV "PSV_EDV"
#define LEArterial "Lower_Extremity_Arterial"
#define LEVenous "Lower_Extremity_Venous"
#define LTVelRatio "LT_Velocity_Ratio"
#define RTVelRatio "RT_Velocity_Ratio"
#define LT_BRACHIAL_DIASTOLIC "LT_Brachial_Diastolic"
#define LT_BRACHIAL_SYSTOLIC  "LT_Brachial_Systolic"
#define RT_BRACHIAL_DIASTOLIC "RT_Brachial_Diastolic"
#define RT_BRACHIAL_SYSTOLIC  "RT_Brachial_Systolic"

#define Aorta "Aorta"
//Aorta Abdomen
#define CIA             "CIA"
#define TransDiam       "Trans_Diam"
#define APDiam          "AP_Diam"
#define ProxAortaKey    "Prox_Aorta"
#define MidAortaKey     "Mid_Aorta"
#define DistalAortaKey  "Distal_Aorta"
#define LTCIAKey        "LT_CIA"
#define RTCIAKey        "RT_CIA"
#define AneurysmKey     "Aneurysm"

#define CODE_SRT_VesselBranchModifier          DSRBasicCodedEntry("125101", "DCM", "Vessel Branch")


enum LabelKey {
    RTCCAPROX, RTCCAMID, RTCCADIST, RTECA, RTICAPROX, RTICAMID, RTICADIST,
    RTBULB, RTVERT, RTSUBCL, LTCCAPROX, LTCCAMID, LTCCADIST, LTECA,
    LTICAPROX, LTICAMID, LTICADIST, LTBULB, LTVERT, LTSUBCL, RTCFA,
    RTPROFA, RTFAPROX, RTFAMID, RTFADIST, RTPOPA, RTPTA, RTPEROA,
    RTATA, RTDPA, LTCFA, LTPROFA, LTFAPROX, LTFAMID, LTFADIST,
    LTPOPA, LTPTA, LTPEROA, LTATA, LTDPA
};

const std::unordered_map<LabelKey, std::string> labelMap = {
        {RTCCAPROX, "RT_CCA_PROX"}, {RTCCAMID, "RT_CCA_MID"}, {RTCCADIST, "RT_CCA_DIST"},
        {RTECA, "RT_ECA"}, {RTICAPROX, "RT_ICA_PROX"}, {RTICAMID, "RT_ICA_MID"},
        {RTICADIST, "RT_ICA_DIST"}, {RTBULB, "RT_BULB"}, {RTVERT, "RT_VERT"},
        {RTSUBCL, "RT_SUBCL"}, {LTCCAPROX, "LT_CCA_PROX"}, {LTCCAMID, "LT_CCA_MID"},
        {LTCCADIST, "LT_CCA_DIST"}, {LTECA, "LT_ECA"}, {LTICAPROX, "LT_ICA_PROX"},
        {LTICAMID, "LT_ICA_MID"}, {LTICADIST, "LT_ICA_DIST"}, {LTBULB, "LT_BULB"},
        {LTVERT, "LT_VERT"}, {LTSUBCL, "LT_SUBCL"}, {RTCFA, "RT_CFA"},
        {RTPROFA, "RT_PROF_A"}, {RTFAPROX, "RT_FA_PROX"}, {RTFAMID, "RT_FA_MID"},
        {RTFADIST, "RT_FA_DIST"}, {RTPOPA, "RT_POP_A"}, {RTPTA, "RT_PTA"},
        {RTPEROA, "RT_PERO_A"}, {RTATA, "RT_ATA"}, {RTDPA, "RT_DPA"},
        {LTCFA, "LT_CFA"}, {LTPROFA, "LT_PROF_A"}, {LTFAPROX, "LT_FA_PROX"},
        {LTFAMID, "LT_FA_MID"}, {LTFADIST, "LT_FA_DIST"}, {LTPOPA, "LT_POP_A"},
        {LTPTA, "LT_PTA"}, {LTPEROA, "LT_PERO_A"}, {LTATA, "LT_ATA"},
        {LTDPA, "LT_DPA"}
};

inline std::string LabelName(LabelKey key) {
    auto it = labelMap.find(key);
    return it != labelMap.end() ? it->second : "";
}


enum VenousLabelKey {
    RTCFV, RTGSV, RTPROF, RTFVPROX, RTFVMID, RTFVDIST,
    RTPOPV, RTGASTROCV, RTPTV, RTPEROV, RTATV, RTDPV,
    LTCFV, LTGSV, LTPROF, LTFVPROX, LTFVMID, LTFVDIST,
    LTPOPV, LTGASTROCV, LTPTV, LTPEROV, LTATV, LTDPV
};

inline std::string VenousLabelName(VenousLabelKey key) {
    switch (key) {
        case RTCFV: return "RT_CFV";
        case RTGSV: return "RT_GSV";
        case RTPROF: return "RT_PROF_V";
        case RTFVPROX: return "RT_FV_PROX";
        case RTFVMID: return "RT_FV_MID";
        case RTFVDIST: return "RT_FV_DIST";
        case RTPOPV: return "RT_POP_V";
        case RTGASTROCV: return "RT_GASTROC_V";
        case RTPTV: return "RT_PTV";
        case RTPEROV: return "RT_PERO_V";
        case RTATV: return "RT_ATV";
        case RTDPV: return "RT_DPV";
        case LTCFV: return "LT_CFV";
        case LTGSV: return "LT_GSV";
        case LTPROF: return "LT_PROF_V";
        case LTFVPROX: return "LT_FV_PROX";
        case LTFVMID: return "LT_FV_MID";
        case LTFVDIST: return "LT_FV_DIST";
        case LTPOPV: return "LT_POP_V";
        case LTGASTROCV: return "LT_GASTROC_V";
        case LTPTV: return "LT_PTV";
        case LTPEROV: return "LT_PERO_V";
        case LTATV: return "LT_ATV";
        case LTDPV: return "LT_DPV";
        default: return "";
    }
}


enum class AbdomenLabelKey {
    ProxAorta = 0,
    MidAorta,
    DistalAorta,
    LTCIA,
    RTCIA,
    Aneurysm
};

inline std::string AbdomenLabelKeyName(AbdomenLabelKey key) {
    switch (key) {
        case AbdomenLabelKey::ProxAorta: return "ProxAorta";
        case AbdomenLabelKey::MidAorta: return "MidAorta";
        case AbdomenLabelKey::DistalAorta: return "DistalAorta";
        case AbdomenLabelKey::LTCIA: return "LTCIA";
        case AbdomenLabelKey::RTCIA: return "RTCIA";
        case AbdomenLabelKey::Aneurysm: return "Aneurysm";
        default: return "";
    }
}



class DicomSR {
public:
    DSRDocument* createDocument(DSRTypes::E_DocumentType documentType);

    DSRDocument* getDocInstant();

    OFCondition setRootContainer(const DSRCodedEntryValue& conceptName);

    OFCondition addNumMeasurement(const DSRTypes::E_AddMode addMode,const DSRCodedEntryValue &conceptName,const DSRNumericMeasurementValue &numericValue);

    OFCondition addText(const DSRTypes::E_AddMode addMode,const DSRCodedEntryValue &conceptName, const OFString &stringValue);

    OFCondition addDate(const DSRTypes::E_AddMode addMode,const DSRCodedEntryValue &conceptName,const OFString &stringValue);

    OFCondition addConceptModCode(const DSRTypes::E_AddMode addMode,const DSRCodedEntryValue &conceptName,const DSRCodedEntryValue &codeValue);

    OFCondition addCordinates(Json::Value coordinates,const std::string& sopUID);

    OFCondition addProperty(std::string codeValue,const DSRTypes::E_AddMode addMode);

    OFCondition addFindingSite();

    OFCondition addFindingSitePatient();

    OFCondition addContainer(const std::string& containerCode);

    OFCondition addMeasurementEncoding(bool isBelowCurrentNeeded,
                                       const DSRCodedEntryValue &conceptName,
                                       const DSRNumericMeasurementValue &numericValue,
                                       Json::Value coordinates);

    OFCondition addMeasurementEncodingMode(bool isBelowCurrentNeeded,
                                       const DSRCodedEntryValue &conceptName,
                                       const DSRNumericMeasurementValue &numericValue,
                                       Json::Value coordinates,const DSRCodedEntryValue &imagingMode);

    DSRBasicCodedEntry getDistBasicMeasurementUnit(const std::string& unitKey);

    DSRBasicCodedEntry getVelBasicMeasurementUnit(const std::string& unitKey);


    OFCondition addObsContextForOb(const std::string &value);

    void addDerivations(const OFString& derivationType);

    OFCondition addBiometryGroupMeasurement(Json::Value fetalBioMetryArray);

    OFString getLNCodeStringFromMeasurementName(OFString measurementName);

    OFCondition addVascularMeasurement(Json::Value arrayList,bool isBelowCurrentRequired);

    static DSRCodedEntryValue getCodeForContainer(LabelKey key);

    OFCondition addVascularMeasurementContainer(Json::Value jsonObject, LabelKey containerKey,
                                                const DSRCodedEntryValue* dsrCodeEntryValue,
                                                const DSRCodedEntryValue* topographicalValue,bool  isBelow);

    bool isLeftDataForCarotid(const Json::Value& jsonObject);

    OFCondition addVelRatio(const std::string& value);

    bool isRightDataForLowerArterial(const Json::Value &jsonObject);

    bool isLeftDataForLowerArterial(const Json::Value &jsonObject);

    bool isRightDataForLowerVenous(const Json::Value &jsonObject);

    bool isLeftDataForLowerVenous(const Json::Value &jsonObject);

    DSRCodedEntryValue getCodeForVenousContainer(VenousLabelKey key);

    OFCondition generateNodeForVein(Json::Value jsonObject, VenousLabelKey venousLabelKey,
                                    const DSRCodedEntryValue *topographicalValue, bool isBelow);

    DSRCodedEntryValue getCodeValueForVenousSubContainer(const string &stringValue);

    DSRCodedEntryValue getCodeForContainer(AbdomenLabelKey key);

    OFCondition addAbdomen(Json::Value jsonObject, AbdomenLabelKey venousLabelKey,
                           const DSRCodedEntryValue *topographicalValue, const DSRCodedEntryValue* vesselBranchValue,bool isBelow,bool  isVesselBranchNeeded);

    OFCondition addBranchialData(const std::string& value,bool isSystolic);
};


#endif //ECHONOUSAPP_DICOMSR_H
