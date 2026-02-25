//
// Created by himanshumistri on 08/02/22.
//

#include "../include/DicomSR.h"
#include <android/log.h>
#include <json/value.h>
#include <dcmtk/dcmsr/codes/dcm.h>
#include <dcmtk/dcmsr/codes/srt.h>
#include <dcmtk/dcmsr/codes/ucum.h>

#define LOGSR(...) \
((void)__android_log_print(ANDROID_LOG_INFO, "dcmtk::", __VA_ARGS__))
#define MRVC "Most recent value chosen"
#define UCV "User chosen value"
#define MVC "Mean value chosen"
#define PSV "PSV"
#define EDV "EDV"
#define Derivation "derivation"
#define Augmentation "Augmentation"
#define Compression "Compression"

#define CODE_UCUM_m_per_sec     DSRBasicCodedEntry("m/s", "UCUM", "m/s")
#define CODE_UCUM_cm_per_sec     DSRBasicCodedEntry("cm/s", "UCUM", "cm/s")

DSRDocument *doc;


DSRDocument* DicomSR::createDocument(DSRTypes::E_DocumentType documentType){

    doc = new DSRDocument();
    doc->createNewDocument(documentType);

    return doc;
}


DSRDocument* DicomSR::getDocInstant(){

    return doc;
}

//CODE_DCM_AdultEchocardiographyProcedureReport
OFCondition DicomSR::setRootContainer(const DSRCodedEntryValue& conceptName){

    OFCondition result;
     auto size = doc->getTree().addContentItem(DSRTypes::RT_isRoot, DSRTypes::VT_Container);
     if(size != 0){
         result = doc->getTree().getCurrentContentItem().setConceptName(conceptName);
         return result;
     }else{
         return SR_EC_IncompatibleDocumentTree;
     }

}

OFCondition DicomSR::addNumMeasurement(const DSRTypes::E_AddMode addMode,const DSRCodedEntryValue &conceptName,const DSRNumericMeasurementValue &numericValue){
    OFCondition result;
    LOGSR("%s addNumMeasurement",TAG);
    auto size = doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Num, addMode);
    LOGSR("%s addContentItem",TAG);
    if(size != 0){
        result = doc->getTree().getCurrentContentItem().setConceptName(conceptName);
        LOGSR("%s setConceptName",TAG);
        result = doc->getTree().getCurrentContentItem().setNumericValue(numericValue);
        LOGSR("%s setNumericValue",TAG);
        return result;
    }else{
        return SR_EC_CannotAddContentItem;
    }
}

OFCondition DicomSR::addText(const DSRTypes::E_AddMode addMode, const DSRCodedEntryValue &conceptName, const OFString &stringValue){
    OFCondition result;
    LOGSR("%s addText",TAG);
    auto size = doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Text, addMode);
    LOGSR("%s addContentItem",TAG);
    if(size != 0){
        result = doc->getTree().getCurrentContentItem().setConceptName(conceptName);
        LOGSR("%s setConceptName %s",TAG, conceptName.getCodeValue().c_str());
        result = doc->getTree().getCurrentContentItem().setStringValue(stringValue);
        LOGSR("%s setTextComment %s", TAG, stringValue.c_str());
        return result;
    }else{
        return SR_EC_CannotAddContentItem;
    }
}

OFCondition DicomSR::addDate(const DSRTypes::E_AddMode addMode,const DSRCodedEntryValue &conceptName,const OFString &stringValue){
    OFCondition result;
    LOGSR("%s addDate",TAG);
    auto size = doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Date,addMode);
    LOGSR("%s addContentItem",TAG);
    if(size != 0){
        result = doc->getTree().getCurrentContentItem().setConceptName(conceptName);
        LOGSR("%s setConceptName",TAG);
        result = doc->getTree().getCurrentContentItem().setStringValue(stringValue);
        LOGSR("%s setNumericValue",TAG);
        return result;
    }else{
        return SR_EC_CannotAddContentItem;
    }
}


OFCondition DicomSR::addConceptModCode(const DSRTypes::E_AddMode addMode,const DSRCodedEntryValue &conceptName,const DSRCodedEntryValue &codeValue){
    OFCondition result;
    auto size = doc->getTree().addContentItem(DSRTypes::RT_hasConceptMod, DSRTypes::VT_Code,addMode);
    if(size != 0){
        result = doc->getTree().getCurrentContentItem().setConceptName(conceptName);
        result = doc->getTree().getCurrentContentItem().setCodeValue(codeValue);
        return result;
    }else{
        return SR_EC_CannotAddContentItem;
    }
}

OFCondition DicomSR::addCordinates(Json::Value coordinates,const std::string& sopUID) {

    //Json::Value coordinates = lvLinear[i][member];
    if (coordinates.isMember("coordinates")) {
        Json::Value coordinateMembers = coordinates["coordinates"];
        doc->getTree().addContentItem(DSRTypes::RT_inferredFrom,DSRTypes::VT_SCoord);
        DSRSpatialCoordinatesValue coord = DSRSpatialCoordinatesValue(DSRTypes::GT_Polyline);
        LOGSR("\nUS2DicomSR %d", coordinateMembers.size());
        for (Json::Value::ArrayIndex index = 0; index != coordinateMembers.size(); index++) {
            auto xValue = coordinateMembers[index]["x"].asString();
            auto yValue = coordinateMembers[index]["y"].asString();
            coord.getGraphicDataList().addItem(std::stof(xValue),std::stof(yValue));
            LOGSR("\nUS2DicomSR X: %s and Y: %s", xValue.c_str(), yValue.c_str());
        }
        doc->getTree().getCurrentContentItem().setSpatialCoordinates(coord);

        //Add Selected from Image
        doc->getTree().addContentItem(DSRTypes::RT_selectedFrom,DSRTypes::VT_Image,DSRTypes::AM_belowCurrent);
        const auto imageSourceName=DSRCodedEntryValue("121112", "DCM", "Source of Measurement");
        doc->getTree().getCurrentContentItem().setConceptName(imageSourceName);

        doc->getTree().getCurrentContentItem().setImageReference(DSRImageReferenceValue(UID_UltrasoundImageStorage, sopUID));
        doc->getTree().goUp();
    }


    return OFCondition();
}

OFCondition DicomSR::addProperty( std::string value,const DSRTypes::E_AddMode addMode) {
    OFCondition result;
    auto size = doc->getTree().addContentItem(DSRTypes::RT_hasProperties,DSRTypes::VT_Code,addMode);
    if(size != 0){
        //
        result = doc->getTree().getCurrentContentItem().setConceptName(CODE_DCM_SelectionStatus);
        if(result.good()){
            if(value == MRVC){
                doc->getTree().getCurrentContentItem().setCodeValue(CODE_DCM_MostRecentValueChosen);
            }else if(value == UCV){
                doc->getTree().getCurrentContentItem().setCodeValue(CODE_DCM_UserChosenValue);
            }else if(value == MVC){
                doc->getTree().getCurrentContentItem().setCodeValue(CODE_DCM_MeanValueChosen);
            } else if (value == MAXIMUM) {
                doc->getTree().getCurrentContentItem().setCodeValue(DSRBasicCodedEntry("G-A437", "SRT", MAXIMUM));
            }else if(value == MEASURED){
                doc->getTree().getCurrentContentItem().setCodeValue(CODE_DCM_MostRecentValueChosen);
            }else if(value == MEAN){
                doc->getTree().getCurrentContentItem().setCodeValue(CODE_DCM_MeanValueChosen);
            }
        }else{
            return SR_EC_CannotAddContentItem;
        }

    }else{
        return SR_EC_CannotAddContentItem;
    }
    return OFCondition();
}

OFCondition DicomSR::addFindingSite() {
    OFCondition result;

    doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Container, DSRTypes::AM_belowCurrent);
    doc->getTree().getCurrentContentItem().setConceptName(CODE_DCM_RETIRED_Findings);
    doc->getTree().getCurrentContentItem().setContinuityOfContent(DSRTypes::COC_Separate);

    doc->getTree().addContentItem(DSRTypes::RT_hasConceptMod,  DSRTypes::VT_Code,DSRTypes::AM_belowCurrent);
    doc->getTree().getCurrentContentItem().setConceptName(CODE_SRT_FindingSite);
    doc->getTree().getCurrentContentItem().setCodeValue(DSRCodedEntryValue("T-32600", "SRT", "Left Ventricle"));

    doc->getTree().getCurrentContentItem().setContinuityOfContent(DSRTypes::COC_Separate);

    doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Container);
    result = doc->getTree().getCurrentContentItem().setConceptName(CODE_DCM_MeasurementGroup);

    return result;
}


OFCondition DicomSR::addFindingSitePatient() {
    OFCondition result;
    doc->getTree().addContentItem(DSRTypes::RT_contains,  DSRTypes::VT_Container,DSRTypes::AM_belowCurrent);
    //EV (121118, DCM, "Patient Characteristics")
    auto patientContainer=DSRBasicCodedEntry("121118", "DCM", "Patient Characteristics");
    result = doc->getTree().getCurrentContentItem().setConceptName(patientContainer);

    return result;
}

OFCondition DicomSR::addContainer(const std::string& containerCode) {
    DSRCodedEntryValue   container;
    auto addMode = DSRTypes::AM_belowCurrent;

    if (containerCode == PatientCharacteristicsCode) {
        container = DSRBasicCodedEntry(PatientCharacteristicsCode, "DCM", "Patient Characteristics");
    } else if (containerCode == ProcedureSummaryCode){
        container = DSRBasicCodedEntry(ProcedureSummaryCode, "DCM", "Summary");
    } else if (containerCode == FetusSummaryCode){
        addMode = DSRTypes::AM_afterCurrent;
        container = DSRBasicCodedEntry(FetusSummaryCode, "DCM", "Fetus Summary");
    } else if (containerCode == FetalBiometryCode) {
        container = DSRBasicCodedEntry(FetalBiometryCode, "DCM", "Fetal Biometry");
    } else if (containerCode == FetalBiometryGroupCode) {
        addMode = DSRTypes::AM_afterCurrent;
        container = DSRBasicCodedEntry(FetalBiometryGroupCode, "DCM", "Biometry Group");
    } else if (containerCode == FetalBiophysicalProfileCode) {
        container = DSRBasicCodedEntry(FetalBiophysicalProfileCode, "DCM", "Biophysical Profile");
    } else if (containerCode == EarlyGestationCode) {
        container = DSRBasicCodedEntry(EarlyGestationCode, "DCM", "Early Gestation");
    } else if (containerCode == AmnioticSacCode) {
        container = DSRBasicCodedEntry(AmnioticSacCode, "DCM", "Findings");
    } else if (containerCode == PelvisAndUterusCode) {
        container = DSRBasicCodedEntry(PelvisAndUterusCode, "DCM", "Pelvis and Uterus");
    }
    doc->getTree().addContentItem(DSRTypes::RT_contains,  DSRTypes::VT_Container,addMode);
    return doc->getTree().getCurrentContentItem().setConceptName(container);
}

OFCondition DicomSR::addMeasurementEncoding(bool isBelowCurrentNeeded,
                                            const DSRCodedEntryValue &conceptName,
                                            const DSRNumericMeasurementValue &numericValue,
                                            Json::Value coordinates){
    OFCondition result;
    if(isBelowCurrentNeeded){
        result= addNumMeasurement(DSRTypes::AM_belowCurrent,conceptName,numericValue);

    }else{
        result =addNumMeasurement(DSRTypes::AM_afterCurrent,conceptName,numericValue);
    }
    if(result.good()){
        result = addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,CODE_IMAGING_MODE_2D);
    }
    if(coordinates.isMember(SELECTION_STATUS)){
        std::string selectionValue = coordinates[SELECTION_STATUS].asString();
        if(!selectionValue.empty()){
            addProperty(selectionValue,DSRTypes::AM_afterCurrent);
        }
    }
    return  result;
}


OFCondition DicomSR::addMeasurementEncodingMode(bool isBelowCurrentNeeded,
                                            const DSRCodedEntryValue &conceptName,
                                            const DSRNumericMeasurementValue &numericValue,
                                            Json::Value coordinates,const DSRCodedEntryValue &imagingMode){
    OFCondition result;
    if(isBelowCurrentNeeded){
        result= addNumMeasurement(DSRTypes::AM_belowCurrent,conceptName,numericValue);
    }else{
        result =addNumMeasurement(DSRTypes::AM_afterCurrent,conceptName,numericValue);
    }
    if(result.good()){
        result = addConceptModCode(DSRTypes::AM_belowCurrent,CODE_IMAGING_MODE,imagingMode);
    }
    if(coordinates.isMember(SELECTION_STATUS)){
        std::string selectionValue = coordinates[SELECTION_STATUS].asString();
        if(!selectionValue.empty()){
            addProperty(selectionValue,DSRTypes::AM_afterCurrent);
        }
    }
    return  result;
}

DSRBasicCodedEntry DicomSR::getDistBasicMeasurementUnit(const std::string& unitKey) {
    if(unitKey == UNIT_MM){
        return CODE_UCUM_Millimeter;
    }else{
        return CODE_UCUM_Centimeter;
    }
}

DSRBasicCodedEntry DicomSR::getVelBasicMeasurementUnit(const std::string& unitKey) {
    if(unitKey == UNIT_CM_PER_SEC){
        return CODE_UCUM_cm_per_sec;
    }else{
        return CODE_UCUM_m_per_sec;
    }
}

OFCondition DicomSR::addObsContextForOb(const std::string& value) {
    OFCondition result = doc->getTree().addChildContentItem(DSRTypes::RT_hasObsContext, DSRTypes::VT_Text, DSRCodedEntryValue("11951-1", "LN", FETUS_ID));
    if (result.good()) {
        return doc->getTree().getCurrentContentItem().setStringValue(value);
    } else {
        return result;
    }
}

void DicomSR::addDerivations(const OFString &derivationType) {
    if (derivationType == CALCULATED) {
        addConceptModCode(DSRTypes::AM_belowCurrent,
                          DSRCodedEntryValue("121401", "DCM", "Derivation"),
                          DSRCodedEntryValue("R-41D2D", "SRT", CALCULATED));
    } else if (derivationType == MAXIMUM) {
        addConceptModCode(DSRTypes::AM_belowCurrent,
                          DSRCodedEntryValue("121401", "DCM", "Derivation"),
                          DSRCodedEntryValue("G-A437", "SRT", MAXIMUM));
    } else if (derivationType == MEASURED) {
        addConceptModCode(DSRTypes::AM_belowCurrent,
                          DSRCodedEntryValue("121401", "DCM", "Derivation"),
                          DSRCodedEntryValue("R-41D41", "SRT", MEASURED));
    } else {
        addConceptModCode(DSRTypes::AM_belowCurrent,
                          DSRCodedEntryValue("121401", "DCM", "Derivation"),
                          DSRCodedEntryValue("R-00317", "SRT", MEAN));
    }
}

OFCondition DicomSR::addVascularMeasurement(Json::Value pdvEdvArray,bool isBelowCurrentRequired) {
    const auto psvCodeValue = DSRCodedEntryValue("1726-7", "LN", "Peak Systolic Velocity");
    const auto edvCodeValue = DSRCodedEntryValue("11653-3", "LN", "End Diastolic Velocity");
    auto codeCCA = DSRCodedEntryValue("T-45100", "SRT", "Common Carotid Artery");

    OFCondition result;
    bool isBelowRequired = isBelowCurrentRequired;
    //Loop to read all PSV and EDV
    for(int i=0;i< pdvEdvArray.size();i++){
        Json::Value::Members members = pdvEdvArray[i].getMemberNames();
        //std::cout << "Members Size: " << members.size() << endl;
        LOGSR( "Members Size: %zu", members.size());
        //for(auto & member : members) {
            //std::cout << "Member: " << member << endl;

            auto isPsvFund = pdvEdvArray[i].isMember(PSV);
        LOGSR( "Member: %d", isPsvFund);
            string selectionStatus = "";
            string derivationStatus ="";
            if(isPsvFund){
                //std::cout << "Member: 1 " << member << endl;
                LOGSR("Member: 1 %s",PSV);
                auto psvObject= pdvEdvArray[i][PSV];
                auto psvData = psvObject["value"].asString();
                const std::string unitStr = psvObject["unit"].asString();
                if(isBelowRequired){
                    //std::cout << "Member: 55 " << psvData << endl;
                    LOGSR("Member: 55 %s",psvData.c_str());
                    doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Num,DSRTypes::AM_belowCurrent);
                    isBelowRequired = false;
                }else{
                    isBelowRequired = false;
                    doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Num, DSRTypes::AM_afterCurrent);
                }
                //std::cout << "Member: 2 " << member << endl;
                doc->getTree().getCurrentContentItem().setConceptName(psvCodeValue);
                //std::cout << "Member: 3 " << member << endl;
                doc->getTree().getCurrentContentItem().setNumericValue(DSRNumericMeasurementValue(psvData.c_str(), getVelBasicMeasurementUnit(unitStr)));
                //std::cout << "Member: 4 " << member << endl;
                if(psvObject.isMember(SELECTION_STATUS)){
                    auto derivationString = psvObject[Derivation].asString();
                    addDerivations(derivationString);
                    auto selectionStatus = psvObject[SELECTION_STATUS].asString();
                    addProperty(selectionStatus,DSRTypes::AM_afterCurrent);
                    doc->getTree().goUp();
                }

            }
            auto isedvFund = pdvEdvArray[i].isMember(EDV);
            if(isedvFund){
                auto edvObject = pdvEdvArray[i][EDV];
                auto edvData = edvObject["value"].asString();
                const std::string unitStr = edvObject["unit"].asString();
                //std::cout << "Member: 5 " << edvData << endl;
                if(isBelowRequired){
                    doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Num,DSRTypes::AM_belowCurrent);
                    isBelowRequired = false;
                    //std::cout << "Member: 66 " << edvData << endl;
                }else{
                    doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Num, DSRTypes::AM_afterCurrent);
                    isBelowRequired = false;
                    //std::cout << "Member: 56 " << edvData << endl;
                }
                //std::cout << "Member: 6 " << member << endl;

                doc->getTree().getCurrentContentItem().setConceptName(edvCodeValue);
                //std::cout << "Member: 7 " << member << endl;
                doc->getTree().getCurrentContentItem().setNumericValue(DSRNumericMeasurementValue(edvData.c_str(), getVelBasicMeasurementUnit(unitStr)));
                //std::cout << "Member: 8 " << member << endl;

                if(edvObject.isMember(SELECTION_STATUS)){
                    auto derivationString = edvObject[Derivation].asString();
                    addDerivations(derivationString);
                    auto selectionStatus = edvObject[SELECTION_STATUS].asString();
                    addProperty(selectionStatus,DSRTypes::AM_afterCurrent);
                    doc->getTree().goUp();
                    LOGSR("Member: EDV selection Status found %d pos is %d",pdvEdvArray.size(),i);
                    if(i == pdvEdvArray.size()-1){
                        doc->getTree().goUp();
                    }

                }

            }
            

        //}

    }
    return OFCondition();
}

OFCondition DicomSR::addVascularMeasurementContainer(Json::Value jsonObject,LabelKey containerKey,const DSRCodedEntryValue* dsrCodeEntryValue,const DSRCodedEntryValue* topographicalValue,bool  isBelow){
    auto jsonObjectRightDistExt = jsonObject[LabelName(containerKey)];


    if(dsrCodeEntryValue != nullptr){
        doc->getTree().addContentItem(DSRTypes::RT_hasConceptMod,DSRTypes::VT_Code,DSRTypes::AM_afterCurrent);
        doc->getTree().getCurrentContentItem().setConceptName(CODE_SRT_Laterality);
        doc->getTree().getCurrentContentItem().setCodeValue(dsrCodeEntryValue->getValue());
    }


    //isAdded = true;
    if(isBelow){
        doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Container,DSRTypes::AM_belowCurrent);
    }else{
        doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Container);
    }

    doc->getTree().getCurrentContentItem().setConceptName(getCodeForContainer(containerKey));
    doc->getTree().getCurrentContentItem().setContinuityOfContent(DSRTypes::COC_Separate);

    bool isBelowCurrentRequired = false;
    if(topographicalValue!= nullptr){
        doc->getTree().addContentItem(DSRTypes::RT_hasConceptMod,  DSRTypes::VT_Code,DSRTypes::AM_belowCurrent);
        doc->getTree().getCurrentContentItem().setConceptName(CODE_SRT_TopographicalModifier);
        doc->getTree().getCurrentContentItem().setCodeValue(DSRCodedEntryValue(topographicalValue->getValue()));
        isBelowCurrentRequired = false;
    }else{
        isBelowCurrentRequired = true;
    }


    auto isPdvEdvExists = jsonObjectRightDistExt.isMember(PSV_EDV);
    cout << "isPdvEdvExists " << isPdvEdvExists<< endl;

    if(isPdvEdvExists){
        //Fetch Measurements
        Json::Value pdvEdvArray = jsonObjectRightDistExt[PSV_EDV];
        addVascularMeasurement(pdvEdvArray,isBelowCurrentRequired);
    }

    doc->getTree().goUp();

    return OFCondition();
}

OFCondition DicomSR::generateNodeForVein(Json::Value jsonObject,VenousLabelKey venousLabelKey,const DSRCodedEntryValue* topographicalValue,bool isBelow){
    auto jsonVein = jsonObject[VenousLabelName(venousLabelKey)];

    if(!jsonVein.empty()){

        //isAdded = true;
        if(isBelow){
            doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Container,DSRTypes::AM_belowCurrent);
        }else{
            doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Container);
        }

        doc->getTree().getCurrentContentItem().setConceptName(getCodeForVenousContainer(venousLabelKey));
        doc->getTree().getCurrentContentItem().setContinuityOfContent(DSRTypes::COC_Separate);

        bool isBelowCurrentRequired = false;
        if(topographicalValue!= nullptr){
            doc->getTree().addContentItem(DSRTypes::RT_hasConceptMod,  DSRTypes::VT_Code,DSRTypes::AM_belowCurrent);
            doc->getTree().getCurrentContentItem().setConceptName(CODE_SRT_TopographicalModifier);
            doc->getTree().getCurrentContentItem().setCodeValue(DSRCodedEntryValue(topographicalValue->getValue()));
            isBelowCurrentRequired = false;
        }else{
            isBelowCurrentRequired = true;
        }

        //Loop to read all PSV and EDV
        for(int i=0;i< jsonVein.size();i++){
            //
            auto isAugmentation = jsonVein[i].isMember(Augmentation);
            if(isAugmentation){

                if (isBelowCurrentRequired) {
                    doc->getTree().addContentItem(DSRTypes::RT_hasConceptMod,  DSRTypes::VT_Code,DSRTypes::AM_belowCurrent);
                    isBelowCurrentRequired = false;
                } else {
                    doc->getTree().addContentItem(DSRTypes::RT_hasConceptMod,  DSRTypes::VT_Code,DSRTypes::AM_afterCurrent);

                }
                const auto augmentationConceptData = DSRBasicCodedEntry("V-400", "99EN", "Augmentation");
                auto augmentationValue = jsonVein[i][Augmentation].asString();
                doc->getTree().getCurrentContentItem().setConceptName(augmentationConceptData);
                doc->getTree().getCurrentContentItem().setCodeValue(getCodeValueForVenousSubContainer(augmentationValue));
            }

            auto isCompression = jsonVein[i].isMember(Compression);
            if(isCompression){

                if (isBelowCurrentRequired) {
                    doc->getTree().addContentItem(DSRTypes::RT_hasConceptMod,  DSRTypes::VT_Code,DSRTypes::AM_belowCurrent);
                    isBelowCurrentRequired = false;
                } else {
                    doc->getTree().addContentItem(DSRTypes::RT_hasConceptMod,  DSRTypes::VT_Code,DSRTypes::AM_afterCurrent);

                }
                const auto augmentationConceptData = DSRBasicCodedEntry("V-401", "99EN", "Compression");
                auto augmentationValue = jsonVein[i][Compression].asString();
                doc->getTree().getCurrentContentItem().setConceptName(augmentationConceptData);
                doc->getTree().getCurrentContentItem().setCodeValue(getCodeValueForVenousSubContainer(augmentationValue));
            }
        }

        doc->getTree().goUp();
        doc->getTree().goUp();
    }

    return OFCondition();
}

OFCondition DicomSR::addAbdomen(Json::Value jsonObject,AbdomenLabelKey venousLabelKey,const DSRCodedEntryValue* topographicalValue,const DSRCodedEntryValue* vesselBranchValue,bool isBelow,bool  isVesselBranchNeeded){

    doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Container);
    doc->getTree().getCurrentContentItem().setConceptName(getCodeForContainer(venousLabelKey));
    doc->getTree().getCurrentContentItem().setContinuityOfContent(DSRTypes::COC_Separate);

    bool isBelowCurrentRequired = (topographicalValue == nullptr);

    if(topographicalValue != nullptr) {
        doc->getTree().addContentItem(DSRTypes::RT_hasConceptMod, DSRTypes::VT_Code,
                                      DSRTypes::AM_belowCurrent);
        doc->getTree().getCurrentContentItem().setConceptName(CODE_SRT_TopographicalModifier);
        doc->getTree().getCurrentContentItem().setCodeValue(
                DSRCodedEntryValue(topographicalValue->getValue()));
    }

    if(isVesselBranchNeeded){
        doc->getTree().addContentItem(DSRTypes::RT_hasConceptMod,  DSRTypes::VT_Code,DSRTypes::AM_belowCurrent);
        doc->getTree().getCurrentContentItem().setConceptName(CODE_SRT_VesselBranchModifier);
        doc->getTree().getCurrentContentItem().setCodeValue(DSRCodedEntryValue(vesselBranchValue->getValue()));
        if(isBelowCurrentRequired){
            isBelowCurrentRequired = false;
        }
    }

    bool isApDiam = jsonObject.isMember(APDiam);
    if(isApDiam){
        auto apDiamJson = jsonObject[APDiam];
        for(int i=0;i<apDiamJson.size();i++){
            auto apDiamData = apDiamJson[i];
            if(apDiamData.isMember("value")){
                const std::string value = apDiamData["value"].asString();
                const std::string unitStr = apDiamData["unit"].asString();

                const auto apDiamCodeValue = DSRCodedEntryValue("A-402", "99EN", "Anterior Posterior Diameter");
                const auto measurementValue = DSRNumericMeasurementValue(value, getDistBasicMeasurementUnit(unitStr));

                if(isBelowCurrentRequired){
                    //std::cout << "Member: 55 " << psvData << endl;
                    doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Num,DSRTypes::AM_belowCurrent);
                    isBelowCurrentRequired = false;
                }else{
                    doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Num, DSRTypes::AM_afterCurrent);
                }
                //std::cout << "Member: 2 " << member << endl;
                doc->getTree().getCurrentContentItem().setConceptName(apDiamCodeValue);
                //std::cout << "Member: 3 " << member << endl;
                doc->getTree().getCurrentContentItem().setNumericValue(measurementValue);
                //std::cout << "Member: 4 " << member << endl;
                if(apDiamData.isMember(SELECTION_STATUS)){
                    auto derivationString = apDiamData[Derivation].asString();
                    addDerivations(derivationString);
                    auto selectionStatus = apDiamData[SELECTION_STATUS].asString();
                    addProperty(selectionStatus,DSRTypes::AM_afterCurrent);
                    doc->getTree().goUp();
                }

            }

        }

    }


    bool isTransDiam = jsonObject.isMember(TransDiam);
    if(isTransDiam){
        auto transDiamJson = jsonObject[TransDiam];
        for(int i=0;i<transDiamJson.size();i++){
            auto transDiamData = transDiamJson[i];
            if(transDiamData.isMember("value")){
                const std::string value = transDiamData["value"].asString();
                const std::string unitStr = transDiamData["unit"].asString();

                const auto transDiamCodeValue=DSRCodedEntryValue("A-401", "99EN", "Trans Abdominal Diameter");
                const auto measurementValue = DSRNumericMeasurementValue(value, getDistBasicMeasurementUnit(unitStr));

                if(isBelowCurrentRequired){
                    //std::cout << "Member: 55 " << psvData << endl;
                    doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Num,DSRTypes::AM_belowCurrent);
                    isBelowCurrentRequired = false;
                }else{
                    doc->getTree().addContentItem(DSRTypes::RT_contains, DSRTypes::VT_Num, DSRTypes::AM_afterCurrent);
                }
                //std::cout << "Member: 2 " << member << endl;
                doc->getTree().getCurrentContentItem().setConceptName(transDiamCodeValue);
                //std::cout << "Member: 3 " << member << endl;
                doc->getTree().getCurrentContentItem().setNumericValue(measurementValue);
                //std::cout << "Member: 4 " << member << endl;
                if(transDiamData.isMember(SELECTION_STATUS)){
                    auto derivationString = transDiamData[Derivation].asString();
                    addDerivations(derivationString);
                    auto selectionStatus = transDiamData[SELECTION_STATUS].asString();
                    addProperty(selectionStatus,DSRTypes::AM_afterCurrent);
                    doc->getTree().goUp();
                }

            }

        }
    }

    auto isPdvEdvExists = jsonObject.isMember(PSV_EDV);
    cout << "isPdvEdvExists " << isPdvEdvExists<< endl;

    if(isPdvEdvExists){
        //Fetch Measurements
        Json::Value pdvEdvArray = jsonObject[PSV_EDV];
        addVascularMeasurement(pdvEdvArray,isBelowCurrentRequired);
    }
    if(!isPdvEdvExists){
        doc->getTree().goUp();
    }


    return OFCondition();
}


OFCondition DicomSR::addVelRatio(const std::string& value){
    const auto ratioCode=DSRCodedEntryValue("33868-1", "LN", "ICA/CCA velocity ratio");
    const auto lvefNumValue = DSRNumericMeasurementValue(value, CODE_UCUM_Ratio);
    addNumMeasurement(DSRTypes::AM_belowCurrent,ratioCode,lvefNumValue);
    doc->getTree().goUp();
    return OFCondition();
}

OFCondition DicomSR::addBranchialData(const std::string& value,bool isSystolic){
    DSRCodedEntryValue code;
    if (isSystolic) {
        code = DSRCodedEntryValue("V-405", "99EN", "Brachial Systolic");
    } else {
        code = DSRCodedEntryValue("V-404", "99EN", "Brachial Diastolic");
    }
    const auto dsrValue = DSRNumericMeasurementValue(value, CODE_UCUM_MMHG);
    addNumMeasurement(DSRTypes::AM_belowCurrent, code,dsrValue);
    doc->getTree().goUp();
    return OFCondition();
}


DSRCodedEntryValue DicomSR::getCodeValueForVenousSubContainer(const std::string& stringValue) {
    if (stringValue == "Present") {
        return DSRCodedEntryValue("52101004", "SCT", "Present");
    } else if (stringValue == "Absent") {
        return DSRCodedEntryValue("272519000", "SCT", "Absent");
    } else if (stringValue == "Reflux") {
        return DSRCodedEntryValue("V-402", "99EN", "Reflux");
    }
    // Default to "Present"
    return DSRCodedEntryValue("52101004", "SCT", "Present");
}





bool DicomSR::isLeftDataForCarotid(const Json::Value& jsonObject) {
    return jsonObject.isMember(LabelName(LTCCAPROX)) ||
           jsonObject.isMember(LabelName(LTCCAMID)) ||
           jsonObject.isMember(LabelName(LTCCADIST)) ||
           jsonObject.isMember(LabelName(LTECA)) ||
           jsonObject.isMember(LabelName(LTICAPROX)) ||
           jsonObject.isMember(LabelName(LTICAMID)) ||
           jsonObject.isMember(LabelName(LTICADIST)) ||
           jsonObject.isMember(LabelName(LTBULB)) ||
           jsonObject.isMember(LabelName(LTVERT)) ||
           jsonObject.isMember(LabelName(LTSUBCL));
}



bool DicomSR::isRightDataForLowerArterial(const Json::Value& jsonObject) {
    return jsonObject.isMember(LabelName(RTCFA)) ||
           jsonObject.isMember(LabelName(RTPROFA)) ||
           jsonObject.isMember(LabelName(RTFAPROX)) ||
           jsonObject.isMember(LabelName(RTFAMID)) ||
           jsonObject.isMember(LabelName(RTFADIST)) ||
           jsonObject.isMember(LabelName(RTPOPA)) ||
           jsonObject.isMember(LabelName(RTPTA)) ||
           jsonObject.isMember(LabelName(RTPEROA)) ||
           jsonObject.isMember(LabelName(RTATA)) ||  // RTATA repeated in original, kept once here
           jsonObject.isMember(LabelName(RTDPA));
}



bool DicomSR::isLeftDataForLowerArterial(const Json::Value& jsonObject) {
    return jsonObject.isMember(LabelName(LTCFA)) ||
           jsonObject.isMember(LabelName(LTPROFA)) ||
           jsonObject.isMember(LabelName(LTFAPROX)) ||
           jsonObject.isMember(LabelName(LTFAMID)) ||
           jsonObject.isMember(LabelName(LTFADIST)) ||
           jsonObject.isMember(LabelName(LTPOPA)) ||
           jsonObject.isMember(LabelName(LTPTA)) ||
           jsonObject.isMember(LabelName(LTPEROA)) ||
           jsonObject.isMember(LabelName(LTATA)) ||  // LTATA repeated in original, kept once here
           jsonObject.isMember(LabelName(LTDPA));
}



bool DicomSR::isRightDataForLowerVenous(const Json::Value& jsonObject) {
    return jsonObject.isMember(VenousLabelName(RTCFV)) ||
           jsonObject.isMember(VenousLabelName(RTGSV)) ||
           jsonObject.isMember(VenousLabelName(RTPROF)) ||
           jsonObject.isMember(VenousLabelName(RTFVPROX)) ||
           jsonObject.isMember(VenousLabelName(RTFVMID)) ||
           jsonObject.isMember(VenousLabelName(RTFVDIST)) ||
           jsonObject.isMember(VenousLabelName(RTPOPV)) ||
           jsonObject.isMember(VenousLabelName(RTGASTROCV)) ||
           jsonObject.isMember(VenousLabelName(RTPTV)) ||
           jsonObject.isMember(VenousLabelName(RTPEROV)) ||
           jsonObject.isMember(VenousLabelName(RTATV)) ||
           jsonObject.isMember(VenousLabelName(RTDPV));
}


bool DicomSR::isLeftDataForLowerVenous(const Json::Value& jsonObject) {
    return jsonObject.isMember(VenousLabelName(LTCFV)) ||
           jsonObject.isMember(VenousLabelName(LTGSV)) ||
           jsonObject.isMember(VenousLabelName(LTPROF)) ||
           jsonObject.isMember(VenousLabelName(LTFVPROX)) ||
           jsonObject.isMember(VenousLabelName(LTFVMID)) ||
           jsonObject.isMember(VenousLabelName(LTFVDIST)) ||
           jsonObject.isMember(VenousLabelName(LTPOPV)) ||
           jsonObject.isMember(VenousLabelName(LTGASTROCV)) ||
           jsonObject.isMember(VenousLabelName(LTPTV)) ||
           jsonObject.isMember(VenousLabelName(LTPEROV)) ||
           jsonObject.isMember(VenousLabelName(LTATV)) ||
           jsonObject.isMember(VenousLabelName(LTDPV));
}




DSRCodedEntryValue DicomSR::getCodeForVenousContainer(VenousLabelKey key) {
    switch (key) {
        case RTCFV:
        case LTCFV:
            return DSRCodedEntryValue("G-035B", "SRT", "Common Femoral Vein");
        case RTGSV:
        case LTGSV:
            return DSRCodedEntryValue("T-49530", "SRT", "Great Saphenous Vein");
        case RTPROF:
        case LTPROF:
            return DSRCodedEntryValue("T-49660", "SRT", "Profunda Femoris Vein");
        case RTFVPROX:
        case RTFVMID:
        case RTFVDIST:
        case LTFVPROX:
        case LTFVMID:
        case LTFVDIST:
            return DSRCodedEntryValue("G-035A", "SRT", "Superficial Femoral Vein"); // TODO: Confirm with Customer
        case RTPOPV:
        case LTPOPV:
            return DSRCodedEntryValue("T-49650", "SRT", "Popliteal Vein");
        case RTGASTROCV:
        case LTGASTROCV:
            return DSRCodedEntryValue("T-4942D", "SRT", "Gastrocnemius vein");
        case RTPTV:
        case LTPTV:
            return DSRCodedEntryValue("T-49620", "SRT", "Posterior Tibial Vein");
        case RTPEROV:
        case LTPEROV:
            return DSRCodedEntryValue("T-49640", "SRT", "Peroneal Vein");
        case RTATV:
        case LTATV:
            return DSRCodedEntryValue("T-49630", "SRT", "Anterior Tibial Vein");
        case RTDPV:
        case LTDPV:
            return DSRCodedEntryValue("V-403", "EN99", "Dorsalis Pedis Venous");
    }
}

DSRCodedEntryValue DicomSR::getCodeForContainer(LabelKey key) {
        switch (key)
        {
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


DSRCodedEntryValue DicomSR::getCodeForContainer(AbdomenLabelKey key) {
    switch (key) {
        case AbdomenLabelKey::ProxAorta:
        case AbdomenLabelKey::MidAorta:
        case AbdomenLabelKey::DistalAorta:
            return DSRCodedEntryValue("T-42000", "SRT", "Aorta");
        case AbdomenLabelKey::LTCIA:
        case AbdomenLabelKey::RTCIA:
            return DSRCodedEntryValue("T-46710", "SRT", "Common Iliac Artery");
        case AbdomenLabelKey::Aneurysm:
            return DSRCodedEntryValue("A-400", "99EN", "Aneurysm");
    }
}


