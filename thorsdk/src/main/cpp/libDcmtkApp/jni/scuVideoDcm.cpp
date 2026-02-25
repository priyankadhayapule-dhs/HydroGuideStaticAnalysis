//
// Created by Echonous on 7/9/2018.
//
#include <jni.h>
#include "include/dcmtkUtilJni.h"
#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmdata/dcpxitem.h"
#include "dcmtk/ofstd/ofstd.h"
#include "dcmtk/ofstd/ofconapp.h"
#include <android/log.h>
#include "jsoncpp/json/json.h"

using namespace std;

#define LOGIVC(...) \
((void)__android_log_print(ANDROID_LOG_INFO, "dcmtk::", __VA_ARGS__))

JNIEXPORT jstring
DCMTKUTIL_METHOD(createVideoDcm)(JNIEnv* env,jobject thiz,jobjectArray dcmInfo,jstring filePath,
        jstring fileName,jbyteArray imgArray,
        jstring fileInput,jint width,jint height,jobjectArray ultrasoundRegionsInfo){

    jboolean isCopy;

    const char* patientName=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, patientNameIndex),&isCopy);
    const char* patientId=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, patientIdIndex),&isCopy);
    const char* patientSex=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, patientSexIndex),&isCopy);
    const char* studyId=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, studyIdIndex),&isCopy);
    const char* studyDate=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, studyDateIndex),&isCopy);
    const char* studyAccessNumber=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, studyAccessNumberIndex),&isCopy);
    const char* filePathC=env->GetStringUTFChars(filePath,&isCopy);
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
    const char* imageGrade=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, imageGradeIndex),&isCopy);
    const char* institutionDepartmentName = env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, institutionDepartmentNameIndex),&isCopy);

    string mJsonString(mwlJsonData);
    bool mIsMwlData;
    bool mIsMwlRead;
    mIsMwlData = !mJsonString.empty();
    LOGIVC("DCM MWL JSON DATA Is %s", mJsonString.c_str());

    std::stringstream jsonData{mJsonString};
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    bool ret = Json::parseFromStream(builder, jsonData, &root, &errs);
    if (!ret || !root.isObject()) {
        std::cout << "cannot convert string to json, err: " << errs << std::endl;
        LOGIVC("JSON Read Error for the MWL Data");
        mIsMwlRead= false;
    }else{
        LOGIVC("JSON Read Success for the MWL Data");
        mIsMwlRead = true;
    }

    //"RegionDataType"
    const char* mRegionSpatialFormat=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(ultrasoundRegionsInfo, RegionSpatialFormatIndex),&isCopy);
    const char* mRegionDataType= env->GetStringUTFChars((jstring)env->GetObjectArrayElement(ultrasoundRegionsInfo, RegionDataTypeIndex),&isCopy);
    const char* mRegionFlags=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(ultrasoundRegionsInfo, RegionFlagsIndex),&isCopy);
    const char* mRegionLocX0=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(ultrasoundRegionsInfo,RegionLocationMinX0Index),&isCopy);
    const char* mRegionLocY0=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(ultrasoundRegionsInfo,RegionLocationMinY0Index),&isCopy);
    const char* mRegionLocX1=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(ultrasoundRegionsInfo,RegionLocationMaxX1Index),&isCopy);
    const char* mRegionLocY1=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(ultrasoundRegionsInfo,RegionLocationMaxY1Index),&isCopy);
    const char* mPhysicalUnitsXDirection=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(ultrasoundRegionsInfo,PhysicalUnitsXDirectionIndex),&isCopy);
    const char* mPhysicalUnitsYDirection=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(ultrasoundRegionsInfo,PhysicalUnitsYDirectionIndex),&isCopy);
    const char* mPhysicalDeltaX=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(ultrasoundRegionsInfo,PhysicalDeltaXIndex),&isCopy);
    const char* mPhysicalDeltaY=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(ultrasoundRegionsInfo,PhysicalDeltaYIndex),&isCopy);
    const char* mFrameTime=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(ultrasoundRegionsInfo,FrameTimeIndex),&isCopy);
    const char* mReferencePixelX0 = env->GetStringUTFChars((jstring)env->GetObjectArrayElement(ultrasoundRegionsInfo, ReferencePixelX0Index),&isCopy);
    const char* mReferencePixelY0 = env->GetStringUTFChars((jstring)env->GetObjectArrayElement(ultrasoundRegionsInfo, ReferencePixelY0Index),&isCopy);
    const char* mReferencePixelPhysicalValueX = env->GetStringUTFChars((jstring)env->GetObjectArrayElement(ultrasoundRegionsInfo, ReferencePixelPhysicalValueXIndex),&isCopy);
    const char* mReferencePixelPhysicalValueY = env->GetStringUTFChars((jstring)env->GetObjectArrayElement(ultrasoundRegionsInfo, ReferencePixelPhysicalValueYIndex),&isCopy);

    jsize  usRegionSize=env->GetArrayLength(ultrasoundRegionsInfo);

    int mSize = usRegionSize;

    LOGIVC("Size of US Region is %d", mSize);

    bool mIsSecondRegion = false;

    if(mSize == UsRegionSize ){
        mIsSecondRegion = true;
    }

    const char* inputVideoFile=env->GetStringUTFChars(fileInput,&isCopy);
    const char* modality="US";
    const auto widthInt=(uint16_t)width;
    const auto heightInt=(uint16_t)height;
   /* Removed as PART of THSW:272 comments
    * if ((patientName != nullptr) && ((patientName[0] == '\0') || (patientName[0] == ' '))){
        patientName = "Not Applicable";
    }*/

    E_TransferSyntax xfer=EXS_MPEG4HighProfileLevel4_1;//EXS_MPEG4HighProfileLevel4_1;
    E_FileWriteMode opt_writeMode = EWM_fileformat;
    E_EncodingType opt_enctype = EET_ExplicitLength;
    E_GrpLenEncoding opt_glenc = EGL_recalcGL;
    E_PaddingEncoding opt_padenc = EPD_withoutPadding;
    OFCmdUnsignedInt opt_filepad = 0;
    OFCmdUnsignedInt opt_itempad = 0;

    char uid[100];
    DcmFileFormat fileformat;
    DcmDataset *dataset = fileformat.getDataset();
    dataset->putAndInsertString(DCM_SOPClassUID, UID_UltrasoundMultiframeImageStorage);
    const char* instanceUID=dcmGenerateUniqueIdentifier(uid, SITE_INSTANCE_UID_ROOT);
    dataset->putAndInsertString(DCM_SOPInstanceUID, instanceUID);
    if(string(calReportComment).compare("NC") != 0){
        dataset->putAndInsertString(DCM_PatientComments,calReportComment,OFTrue);
    }
    const char* instanceMediaStorageUID=dcmGenerateUniqueIdentifier(uid, SITE_INSTANCE_UID_ROOT);
     //char uid1[100]
     /**
      * DICOM library it's self adding MediaStorage so no need to add here
      */
    //dataset->putAndInsertString(DCM_MediaStorageSOPClassUID,UID_VideoPhotographicImageStorage);
    //dataset->putAndInsertString(DCM_MediaStorageSOPInstanceUID,SOPInstanceUID);
    //dataset->putAndInsertString(DCM_TransferSyntaxUID,);

    dataset->putAndInsertString(DCM_SeriesNumber, seriesNumber);
    dataset->putAndInsertString(DCM_StudyTime, studyTime);
    dataset->putAndInsertString(DCM_AcquisitionDate,acquisitionDate);
    string aquisitionDateTime=string();
    aquisitionDateTime.append(acquisitionTime,6);
    dataset->putAndInsertString(DCM_AcquisitionTime,aquisitionDateTime.c_str());
    dataset->putAndInsertString(DCM_AccessionNumber,studyAccessNumber);

    dataset->putAndInsertString(DCM_PatientName,patientName,OFTrue);
    dataset->putAndInsertString(DCM_PatientID,patientId,OFTrue);
    dataset->putAndInsertString(DCM_StudyID,studyId,OFTrue);
    dataset->putAndInsertString(DCM_PatientSex,patientSex,OFTrue);
    dataset->putAndInsertString(DCM_PatientSize,patientHeight,OFTrue);
    dataset->putAndInsertString(DCM_PatientWeight,patientWeight,OFTrue);
    LOGIVC("Patient Wight H264 %s Height %s",patientWeight,patientHeight);
    dataset->putAndInsertString(DCM_StudyDate,studyDate,OFTrue);
    dataset->putAndInsertString(DCM_Modality,modality,OFTrue);

    dataset->putAndInsertString(DCM_PerformedProcedureStepStartDate,studyDate,OFTrue);
    dataset->putAndInsertString(DCM_PerformedProcedureStepStartTime,studyTime,OFTrue);
    //PerformedProcedureStepDescription
    dataset->putAndInsertString(DCM_PerformedProcedureStepDescription,examPurpose,OFTrue);

    dataset->putAndInsertString(DCM_PhotometricInterpretation, "YBR_PARTIAL_420");
    dataset->putAndInsertUint16(DCM_PixelRepresentation, 0);

    dataset->putAndInsertString(DCM_SamplesPerPixel, "3");
    dataset->putAndInsertString(DCM_PlanarConfiguration, "0");

    dataset->putAndInsertUint16(DCM_BitsAllocated, 8);
    dataset->putAndInsertString(DCM_NumberOfFrames, "9165");
    dataset->putAndInsertUint16(DCM_BitsStored, 8);
    dataset->putAndInsertString(DCM_SpecificCharacterSet, "ISO_IR 100");
    dataset->putAndInsertUint16(DCM_Columns, widthInt);
    dataset->putAndInsertUint16(DCM_Rows, heightInt);
    dataset->putAndInsertUint16(DCM_HighBit,7);
    dataset->putAndInsertString(DCM_FrameIncrementPointer,"(0018,1063)");
    dataset->putAndInsertString(DCM_FrameTime,"30");
    dataset->putAndInsertString(DCM_CineRate,"30");
    dataset->putAndInsertString(DCM_LossyImageCompression,"01");
    dataset->putAndInsertString(DCM_Manufacturer,MANUFACTURE);
    dataset->putAndInsertString(DCM_ImageType, DCMImageType);
    dataset->putAndInsertString(DCM_ManufacturerModelName,"KOSMOS");
    dataset->putAndInsertString(DCM_InstitutionName,facilityName);
    dataset->putAndInsertString(DCM_StudyDescription,examPurpose);
    dataset->putAndInsertString(DCM_PatientComments,patientComments);
    dataset->putAndInsertString(DCM_DeviceSerialNumber,deviceSerialNo);
    dataset->putAndInsertString(DCM_SoftwareVersions,softVerNo);

    //Study
    OFCondition resultTag = EC_InvalidValue;
    LOGIVC("StudyInstance is %s",studyInstance);
    resultTag=dataset->putAndInsertString(DCM_StudyInstanceUID, studyInstance,OFTrue);
    if(resultTag.good()){
        LOGIVC("StudyInstance inserted successfully MPEG4");
    }else{
        LOGIVC("StudyInstance inserted Failed MPEG4  %s",resultTag.text());
    }
    //Series
    LOGIVC("SeriesInstance is %s",seriesInstance);
    resultTag=dataset->putAndInsertString(DCM_SeriesInstanceUID, seriesInstance,OFTrue);
    if(resultTag.good()){
        LOGIVC("SeriesInstance inserted successfully MPEG4");
    }else{
        LOGIVC("SeriesInstance inserted Failed MPEG4 %s",resultTag.text());
    }

    dataset->putAndInsertString(DCM_PatientOrientation, patientOrientation);  //anterior  https://dicom.innolitics.com/ciods/us-image/general-image/00200020
    dataset->putAndInsertString(DCM_PatientBirthDate, patientDoB);
    dataset->putAndInsertString(DCM_ReferringPhysicianName, referPhysician);
    LOGIVC("InstanceNumber is  MP4 %s",instanceNumber);
    dataset->putAndInsertString(DCM_InstanceNumber, instanceNumber);
    dataset->putAndInsertString(DCM_TimezoneOffsetFromUTC,timeZoneoffSetFromUTC,OFTrue);

    dataset->putAndInsertString(DCM_ProtocolName,protocolName);
    dataset->putAndInsertString(DCM_EthnicGroup,ethnicGroup);
    dataset->putAndInsertString(DCM_TransducerData,TransducerData);

    dataset->putAndInsertString(DCM_PhysiciansOfRecord,physicianofRecord,OFTrue);
    dataset->putAndInsertString(DCM_PerformingPhysicianName,performPhysician,OFTrue);
    dataset->putAndInsertString(DCM_OperatorsName,performPhysician,OFTrue);
    dataset->putAndInsertString(DCM_StationName,kosmosStationName,OFTrue);

    dataset->putAndInsertString(DCM_SeriesDate,studyDate,OFTrue);
    dataset->putAndInsertString(DCM_SeriesTime,studyTime,OFTrue);

    dataset->putAndInsertString(DCM_ContentDate,acquisitionDate,OFTrue);
    dataset->putAndInsertString(DCM_ContentTime,acquisitionTime,OFTrue);

    if(strlen(institutionDepartmentName) != 0){
        dataset->putAndInsertString(DCM_InstitutionalDepartmentName,institutionDepartmentName,OFTrue);
    }
    dataset->putAndInsertString(DCM_SOPInstanceUID, sopInstantUid, OFTrue);

    DcmItem *mSequenceofUltrasoundRegions;

    //Insert DcmItem tags
    OFCondition result = EC_IllegalCall;
    if(dataset->findOrCreateSequenceItem(DCM_SequenceOfUltrasoundRegions,mSequenceofUltrasoundRegions).good()){

        auto regionSpatial=(Uint16)std::atoi(mRegionSpatialFormat);
        result=mSequenceofUltrasoundRegions->putAndInsertUint16(DCM_RegionSpatialFormat,regionSpatial);
        if (result.bad()){
            LOGIVC("Bad DCM_RegionSpatialFormat");
        }

        auto RegionDataType=(Uint16)std::atoi(mRegionDataType);
        result=mSequenceofUltrasoundRegions->putAndInsertUint16(DCM_RegionDataType,RegionDataType);
        if (result.bad()){
            LOGIVC("Bad DCM_RegionDataType");
        }

        auto RegionFlags=(Uint32)std::atoi(mRegionFlags);
        result=mSequenceofUltrasoundRegions->putAndInsertUint32(DCM_RegionFlags,RegionFlags);
        if (result.bad()){
            LOGIVC("Bad DCM_RegionFlags");
        }


        result=mSequenceofUltrasoundRegions->putAndInsertUint32(DCM_RegionLocationMinX0,(Uint32)std::atoi(mRegionLocX0));
        if (result.bad()){
            LOGIVC("Bad DCM_RegionLocationMinX0");
        }

        result=mSequenceofUltrasoundRegions->putAndInsertUint32(DCM_RegionLocationMinY0,(Uint32)std::atoi(mRegionLocY0));
        if (result.bad()){
            LOGIVC("Bad DCM_RegionLocationMinY0");
        }

        result=mSequenceofUltrasoundRegions->putAndInsertUint32(DCM_RegionLocationMaxX1,(Uint32)std::atoi(mRegionLocX1));
        if (result.bad()){
            LOGIVC("Bad DCM_RegionLocationMaxX1");
        }

        result=mSequenceofUltrasoundRegions->putAndInsertUint32(DCM_RegionLocationMaxY1,(Uint32)std::atoi(mRegionLocY1));
        if (result.bad()){
            LOGIVC("Bad DCM_RegionLocationMaxY1");
        }

        result=mSequenceofUltrasoundRegions->putAndInsertUint16(DCM_PhysicalUnitsXDirection,(Uint16)std::strtol(mPhysicalUnitsXDirection,
                                                                                                                nullptr,10));
        if (result.bad()){
            LOGIVC("Bad DCM_PhysicalUnitsXDirection");
        }

        result=mSequenceofUltrasoundRegions->putAndInsertUint16(DCM_PhysicalUnitsYDirection,(Uint16)std::strtol(mPhysicalUnitsYDirection,
                                                                                                                nullptr,10));
        if (result.bad()){
            LOGIVC("Bad DCM_PhysicalUnitsYDirection");
        }

        result=mSequenceofUltrasoundRegions->putAndInsertFloat64(DCM_PhysicalDeltaX,(Float64)std::strtod(mPhysicalDeltaX,nullptr));
        if (result.bad()){
            LOGIVC("Bad DCM_PhysicalDeltaX");
        }

        result=mSequenceofUltrasoundRegions->putAndInsertFloat64(DCM_PhysicalDeltaY,(Float64) std::strtod(mPhysicalDeltaY,nullptr));
        if (result.bad()){
            LOGIVC("Bad DCM_PhysicalDeltaY");
        }

        result=mSequenceofUltrasoundRegions->putAndInsertSint32(DCM_ReferencePixelX0,(Sint32)std::atoi(mReferencePixelX0));
        if(result.bad()){
            LOGIVC("Bad DCM_ReferencePixelX0");
        }

        result=mSequenceofUltrasoundRegions->putAndInsertSint32(DCM_ReferencePixelY0,(Sint32)std::atoi(mReferencePixelY0));
        if(result.bad()){
            LOGIVC("Bad DCM_ReferencePixelY0");
        }


        result=mSequenceofUltrasoundRegions->putAndInsertFloat64(DCM_ReferencePixelPhysicalValueX,(Float64)std::strtod(mReferencePixelPhysicalValueX,nullptr));
        if(result.bad()){
            LOGIVC("Bad DCM_ReferencePixelPhysicalValueX");
        }

        result=mSequenceofUltrasoundRegions->putAndInsertFloat64(DCM_ReferencePixelPhysicalValueY,(Float64)std::strtod(mReferencePixelPhysicalValueY,nullptr));
        if(result.bad()){
            LOGIVC("Bad DCM_ReferencePixelPhysicalValueY");
        }

    }else{
        LOGIVC("Cannot add SequenceOfUltrasoundRegions");
    }

    if(mIsSecondRegion){

        //"RegionDataType"
        const char* mRegionSpatialFormatSecond=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(ultrasoundRegionsInfo, RegionSpatialFormatIndex+IndexPlus),&isCopy);
        const char* mRegionDataTypeSecond= env->GetStringUTFChars((jstring)env->GetObjectArrayElement(ultrasoundRegionsInfo, RegionDataTypeIndex+IndexPlus),&isCopy);
        const char* mRegionFlagsSecond=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(ultrasoundRegionsInfo, RegionFlagsIndex+IndexPlus),&isCopy);
        const char* mRegionLocX0Second=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(ultrasoundRegionsInfo,RegionLocationMinX0Index+IndexPlus),&isCopy);
        const char* mRegionLocY0Second=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(ultrasoundRegionsInfo,RegionLocationMinY0Index+IndexPlus),&isCopy);
        const char* mRegionLocX1Second=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(ultrasoundRegionsInfo,RegionLocationMaxX1Index+IndexPlus),&isCopy);
        const char* mRegionLocY1Second=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(ultrasoundRegionsInfo,RegionLocationMaxY1Index+IndexPlus),&isCopy);
        const char* mPhysicalUnitsXDirectionSecond=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(ultrasoundRegionsInfo,PhysicalUnitsXDirectionIndex+IndexPlus),&isCopy);
        const char* mPhysicalUnitsYDirectionSecond=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(ultrasoundRegionsInfo,PhysicalUnitsYDirectionIndex+IndexPlus),&isCopy);
        const char* mPhysicalDeltaXSecond=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(ultrasoundRegionsInfo,PhysicalDeltaXIndex+IndexPlus),&isCopy);
        const char* mPhysicalDeltaYSecond=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(ultrasoundRegionsInfo,PhysicalDeltaYIndex+IndexPlus),&isCopy);
        const char* mFrameTimeSecond=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(ultrasoundRegionsInfo,FrameTimeIndex+IndexPlus),&isCopy);
        const char* mReferencePixelX0Second = env->GetStringUTFChars((jstring)env->GetObjectArrayElement(ultrasoundRegionsInfo, ReferencePixelX0Index+IndexPlus),&isCopy);
        const char* mReferencePixelY0Second = env->GetStringUTFChars((jstring)env->GetObjectArrayElement(ultrasoundRegionsInfo, ReferencePixelY0Index+IndexPlus),&isCopy);
        const char* mReferencePixelPhysicalValueXSecond = env->GetStringUTFChars((jstring)env->GetObjectArrayElement(ultrasoundRegionsInfo, ReferencePixelPhysicalValueXIndex+IndexPlus),&isCopy);
        const char* mReferencePixelPhysicalValueYSecond = env->GetStringUTFChars((jstring)env->GetObjectArrayElement(ultrasoundRegionsInfo, ReferencePixelPhysicalValueYIndex+IndexPlus),&isCopy);

        DcmItem *mSequenceofUltrasoundRegionsSecond;

        //Insert DcmItem tags
        //OFCondition result = EC_IllegalCall;
        if(dataset->findOrCreateSequenceItem(DCM_SequenceOfUltrasoundRegions,mSequenceofUltrasoundRegionsSecond,-2).good()){

            auto regionSpatial=(Uint16)std::atoi(mRegionSpatialFormatSecond);
            result=mSequenceofUltrasoundRegionsSecond->putAndInsertUint16(DCM_RegionSpatialFormat,regionSpatial);
            if (result.bad()){
                LOGIVC("Bad DCM_RegionSpatialFormat %s",result.text());
            }

            auto RegionDataType=(Uint16)std::atoi(mRegionDataTypeSecond);
            result=mSequenceofUltrasoundRegionsSecond->putAndInsertUint16(DCM_RegionDataType,RegionDataType);
            if (result.bad()){
                LOGIVC("Bad DCM_RegionDataType %s",result.text());
            }

            auto RegionFlags=(Uint32)std::atoi(mRegionFlagsSecond);
            result=mSequenceofUltrasoundRegionsSecond->putAndInsertUint32(DCM_RegionFlags,RegionFlags);
            if (result.bad()){
                LOGIVC("Bad DCM_RegionFlags %s ",result.text());
            }

            result=mSequenceofUltrasoundRegionsSecond->putAndInsertUint32(DCM_RegionLocationMinX0,(Uint32)std::atoi(mRegionLocX0Second));
            if (result.bad()){
                LOGIVC("Bad DCM_RegionLocationMinX0");
            }

            result=mSequenceofUltrasoundRegionsSecond->putAndInsertUint32(DCM_RegionLocationMinY0,(Uint32)std::atoi(mRegionLocY0Second));
            if (result.bad()){
                LOGIVC("Bad DCM_RegionLocationMinY0");
            }

            result=mSequenceofUltrasoundRegionsSecond->putAndInsertUint32(DCM_RegionLocationMaxX1,(Uint32)std::atoi(mRegionLocX1Second));
            if (result.bad()){
                LOGIVC("Bad DCM_RegionLocationMaxX1");
            }

            result=mSequenceofUltrasoundRegionsSecond->putAndInsertUint32(DCM_RegionLocationMaxY1,(Uint32)std::atoi(mRegionLocY1Second));
            if (result.bad()){
                LOGIVC("Bad DCM_RegionLocationMaxY1");
            }

            result=mSequenceofUltrasoundRegionsSecond->putAndInsertUint16(DCM_PhysicalUnitsXDirection,(Uint16)std::strtol(mPhysicalUnitsXDirectionSecond,
                                                                                                                          nullptr,10));
            if (result.bad()){
                LOGIVC("Bad DCM_PhysicalUnitsXDirection");
            }

            result=mSequenceofUltrasoundRegionsSecond->putAndInsertUint16(DCM_PhysicalUnitsYDirection,(Uint16)std::strtol(mPhysicalUnitsYDirectionSecond,
                                                                                                                          nullptr,10));
            if (result.bad()){
                LOGIVC("Bad DCM_PhysicalUnitsYDirection");
            }

            result=mSequenceofUltrasoundRegionsSecond->putAndInsertFloat64(DCM_PhysicalDeltaX,(Float64)std::strtod(mPhysicalDeltaXSecond,nullptr));
            if (result.bad()){
                LOGIVC("Bad DCM_PhysicalDeltaX");
            }

            result=mSequenceofUltrasoundRegionsSecond->putAndInsertFloat64(DCM_PhysicalDeltaY,(Float64) std::strtod(mPhysicalDeltaYSecond,nullptr));
            if (result.bad()){
                LOGIVC("Bad DCM_PhysicalDeltaY");
            }

            result=mSequenceofUltrasoundRegionsSecond->putAndInsertSint32(DCM_ReferencePixelX0,(Sint32)std::atoi(mReferencePixelX0Second));
            if(result.bad()){
                LOGIVC("Bad DCM_ReferencePixelX0");
            }

            result=mSequenceofUltrasoundRegionsSecond->putAndInsertSint32(DCM_ReferencePixelY0,(Sint32)std::atoi(mReferencePixelY0Second));
            if(result.bad()){
                LOGIVC("Bad DCM_ReferencePixelY0");
            }


            result=mSequenceofUltrasoundRegionsSecond->putAndInsertFloat64(DCM_ReferencePixelPhysicalValueX,(Float64)std::strtod(mReferencePixelPhysicalValueXSecond,nullptr));
            if(result.bad()){
                LOGIVC("Bad DCM_ReferencePixelPhysicalValueX");
            }

            result=mSequenceofUltrasoundRegionsSecond->putAndInsertFloat64(DCM_ReferencePixelPhysicalValueY,(Float64)std::strtod(mReferencePixelPhysicalValueYSecond,nullptr));
            if(result.bad()){
                LOGIVC("Bad DCM_ReferencePixelPhysicalValueY");
            }

        }else{
            LOGIVC("Cannot add SequenceOfUltrasoundRegions");
        }
    }


    DcmItem *AnatomicRegionSequenceItem;


    if (dataset->findOrCreateSequenceItem(DCM_AnatomicRegionSequence, AnatomicRegionSequenceItem).good()){
        result = AnatomicRegionSequenceItem->putAndInsertString(DCM_CodeValue, DCMCodeValue);
        if (result.bad()){
            LOGIVC("Bad DCM_CodeValue");
        }

        result = AnatomicRegionSequenceItem->putAndInsertString(DCM_CodingSchemeDesignator, DCMCodingSchemeDesignator);
        if (result.bad()){
            LOGIVC("Bad DCM_CodingSchemeDesignator");
        }

        result = AnatomicRegionSequenceItem->putAndInsertString(DCM_CodingSchemeVersion, DCMCodingSchemeVersion);
        if (result.bad()){
            LOGIVC("Bad DCM_CodingSchemeVersion");
        }

        result = AnatomicRegionSequenceItem->putAndInsertString(DCM_CodeMeaning, DCMCodeMeaning);
        if (result.bad()){
            LOGIVC("Bad DCM_CodeMeaning");
        }
    } else {
        LOGIVC(" Cannot add DCM_AnatomicRegionSequence");
    }

    DcmItem *AcquisitionContextSequenceItem;
    if (dataset->findOrCreateSequenceItem(DCM_AcquisitionContextSequence,AcquisitionContextSequenceItem).good()){
        result = AcquisitionContextSequenceItem->putAndInsertString(DCM_ValueType, DCMValueType);
        if (result.bad()){
            LOGIVC("Bad DCM_ValueType");
        }

        result = AcquisitionContextSequenceItem->putAndInsertString(DCM_ObservationDateTime,DefaultDate);
        if (result.bad()){
            LOGIVC("Bad DCM_ObservationDateTime");
        }


        DcmItem *ConceptNameCodeSequence;
        if (AcquisitionContextSequenceItem->findOrCreateSequenceItem(DCM_ConceptNameCodeSequence,ConceptNameCodeSequence).good()){
            result = ConceptNameCodeSequence->putAndInsertString(DCM_CodeValue, DCMCodeValue);
            if (result.bad()){
                LOGIVC("Bad ConceptNameCodeSequence DCM_CodeValue");
            }

            result = ConceptNameCodeSequence->putAndInsertString(DCM_CodingSchemeDesignator, DCMCodingSchemeDesignator);
            if (result.bad()){
                LOGIVC("Bad ConceptNameCodeSequence DCM_CodingSchemeDesignator");
            }

            result = ConceptNameCodeSequence->putAndInsertString(DCM_CodingSchemeVersion, DCMCodingSchemeVersion);
            if (result.bad()){
                LOGIVC("Bad ConceptNameCodeSequence DCM_CodingSchemeVersion");
            }

            result = ConceptNameCodeSequence->putAndInsertString(DCM_CodeMeaning, DCMCodeMeaning);
            if (result.bad()){
                LOGIVC("Bad ConceptNameCodeSequence DCM_CodeMeaning");
            }
        } else {
            LOGIVC(" Cannot add DCM_ConceptNameCodeSequence");
        }

        result = AcquisitionContextSequenceItem->putAndInsertUint16(DCM_ReferencedFrameNumber,1);
        if (result.bad()){
            LOGIVC("Bad DCM_ReferencedFrameNumbers");
        }

        result = AcquisitionContextSequenceItem->putAndInsertString(DCM_NumericValue,"1");
        if (result.bad()){
            LOGIVC("Bad DCM_NumericValue");
        }

        result = AcquisitionContextSequenceItem->putAndInsertFloat64(DCM_FloatingPointValue,1.0);
        if (result.bad()){
            LOGIVC("Bad DCM_FloatingPointValue");
        }

        result = AcquisitionContextSequenceItem->putAndInsertString(DCM_Date,DefaultDate);
        if (result.bad()){
            LOGIVC("Bad DCM_Date");
        }

        result = AcquisitionContextSequenceItem->putAndInsertString(DCM_Time,DefaultTime);
        if (result.bad()){
            LOGIVC("Bad DCM_Time");
        }

        result = AcquisitionContextSequenceItem->putAndInsertString(DCM_PersonName,patientName);
        if (result.bad()){
            LOGIVC("Bad DCM_PersonName");
        }

        result = AcquisitionContextSequenceItem->putAndInsertString(DCM_TextValue,"Text");
        if (result.bad()){
            LOGIVC("Bad DCM_TextValue");
        }

        DcmItem *MeasurementUnitsCodeSequence;
        if (AcquisitionContextSequenceItem->findOrCreateSequenceItem(DCM_MeasurementUnitsCodeSequence,MeasurementUnitsCodeSequence).good()){
            result = MeasurementUnitsCodeSequence->putAndInsertString(DCM_CodeValue, DCMCodeValue);
            if (result.bad()){
                LOGIVC("Bad MeasurementUnitsCodeSequence DCM_CodeValue");
            }

            result = MeasurementUnitsCodeSequence->putAndInsertString(DCM_CodingSchemeDesignator, DCMCodingSchemeDesignator);
            if (result.bad()){
                LOGIVC("Bad MeasurementUnitsCodeSequence DCM_CodingSchemeDesignator");
            }

            result = MeasurementUnitsCodeSequence->putAndInsertString(DCM_CodingSchemeVersion, DCMCodingSchemeVersion);
            if (result.bad()){
                LOGIVC("Bad MeasurementUnitsCodeSequence DCM_CodingSchemeVersion");
            }

            result = MeasurementUnitsCodeSequence->putAndInsertString(DCM_CodeMeaning, DCMCodeMeaning);
            if (result.bad()){
                LOGIVC("Bad MeasurementUnitsCodeSequence DCM_CodeMeaning");
            }
        } else {
            LOGIVC(" Cannot add DCM_MeasurementUnitsCodeSequence");
        }
    } else {
        LOGIVC(" Cannot add DCM_AcquisitionContextSequence");
    }
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
            LOGIVC("MWL Encoding DCM_RequestAttributesSequence");
            result=mRequestAttributesSequence->putAndInsertString(DCM_RequestedProcedureDescription, rpDesc.c_str());
            if(result.bad()){
                LOGIVC("Bad DCM_RequestedProcedureDescription RequestAttributesSequence");
            }

            result=mRequestAttributesSequence->putAndInsertString(DCM_ScheduledProcedureStepDescription, spStepDesc.c_str());
            if(result.bad()){
                LOGIVC("Bad DCM_ScheduledProcedureStepDescription RequestAttributesSequence");
            }

            result=mRequestAttributesSequence->putAndInsertString(DCM_RequestedProcedureID, rpId.c_str());
            if(result.bad()){
                LOGIVC("Bad RequestedProcedureID RequestAttributesSequence");
            }

            result=mRequestAttributesSequence->putAndInsertString(DCM_ScheduledProcedureStepID, spStepId.c_str());
            if(result.bad()){
                LOGIVC("Bad DCM_ScheduledProcedureStepID RequestAttributesSequence");
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
                        LOGIVC("Bad DCM_CodeValue ScheduledProtocolCodeSequence");
                    }

                    result=mScheduledProtocolCodeSequence->putAndInsertString(DCM_CodingSchemeDesignator, codingSchemeDesignator.c_str());
                    if(result.bad()){
                        LOGIVC("Bad DCM_CodingSchemeDesignator ScheduledProtocolCodeSequence");
                    }
                    result=mScheduledProtocolCodeSequence->putAndInsertString(DCM_CodeMeaning, codeMeaning.c_str());
                    if(result.bad()){
                        LOGIVC("Bad DCM_CodeMeaning ScheduledProtocolCodeSequence");
                    }

                    result = mScheduledProtocolCodeSequence->putAndInsertString(DCM_CodingSchemeVersion,codingSchemeVersion.c_str());
                    if(result.bad()){
                        LOGIVC("Bad DCM_CodingSchemeVersion ScheduledProtocolCodeSequence");
                    }

                }else{
                    LOGIVC("Bad DCM_ScheduledProtocolCodeSequence RequestAttributesSequence DICOM Sub tag failed");
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
                    LOGIVC("Bad DCM_ReferencedSOPClassUID ReferencedStudySequence");
                }

                auto strReferencedSOPInstanceUID = referencedStudySequenceJson[ReferencedSOPInstanceUID].asString();
                result= mReferencedStudySequence-> putAndInsertString(DCM_ReferencedSOPInstanceUID, strReferencedSOPInstanceUID.c_str());
                if(result.bad()){
                    LOGIVC("Bad DCM_ReferencedSOPInstanceUID ReferencedStudySequence");
                }

            }

        }else{
            LOGIVC("Cannot add DCM_ReferencedStudySequence");
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
    if(!string(patientAlternativeId).empty() || !string(imageGrade).empty()) {
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
            dict.addEntry(new DcmDictEntry(PRIVATE_ELEMENT1_TAG_1, EVR_LO, "ImageGrade", 1, 1, "private", OFTrue, PRIVATE_CREATOR_NAME));
        }
        dcmDataDict.wrunlock();

        //Insert Alternative Id and imageGrade as custom tag
        if (!dataset->tagExists(PRV_PrivateCreator) && !string(patientAlternativeId).empty() && !string(imageGrade).empty()) {
            dataset->putAndInsertString(PRV_PrivateCreator, PRIVATE_CREATOR_NAME);
            dataset->putAndInsertString(PRV_PrivateElement1, patientAlternativeId);
            dataset->putAndInsertString(PRV_PrivateElement2, imageGrade);
        } else if (!dataset->tagExists(PRV_PrivateCreator) && !string(patientAlternativeId).empty()) {
            dataset->putAndInsertString(PRV_PrivateCreator, PRIVATE_CREATOR_NAME);
            dataset->putAndInsertString(PRV_PrivateElement1, patientAlternativeId);
        } else if (!dataset->tagExists(PRV_PrivateCreator) && !string(imageGrade).empty()) {
            dataset->putAndInsertString(PRV_PrivateCreator, PRIVATE_CREATOR_NAME);
            dataset->putAndInsertString(PRV_PrivateElement2, imageGrade);
        }
    }

    DcmElement *mDcmElement= nullptr; //=DcmElement(dataset);

    /* convert tag string */
   // DcmTagKey dcmTagKey;
    DcmTagKey tag(0x7fe0, 0x0010);

    //dcmTagKey.set();

    DcmTag dcmTag(tag);

    const char * vrType="OB";
    /* convert vr string */
    DcmVR dcmVR(OFreinterpret_cast(const char *, vrType));

    dcmTag.setVR(dcmVR);

    /* create DICOM element */
    result = EC_IllegalCall;
    result = DcmItem::newDicomElementWithVR(mDcmElement, dcmTag);

    /* create new pixel sequence */
    //DcmPixelSequence *sequence = new DcmPixelSequence(DCM_PixelSequenceTag);

    if (result.bad())
    {
        LOGIVC("Error occur in creation of DcmElement---> %s",result.text());

        /* delete new element if an error occurred */
        delete mDcmElement;

        mDcmElement = NULL;

    }

    /* insert new sequence element into the dataset */

    result=dataset->insert(mDcmElement,OFTrue);

    string resultData;

    if(result.good()){

        /* special handling for compressed pixel data */
        if (mDcmElement->getTag() == DCM_PixelData)
        {

            /* create new pixel sequence */
            DcmPixelSequence *sequence = new DcmPixelSequence(DCM_PixelSequenceTag);
            if (sequence != NULL)
            {
                /* ... insert it into the dataset and proceed with the pixel items */
                OFstatic_cast(DcmPixelData *, mDcmElement)->putOriginalRepresentation(xfer, NULL, sequence);
               // parsePixelSequence(sequence, current->xmlChildrenNode);

                /* create new pixel item */
                DcmPixelItem *newItemZero = new DcmPixelItem(DCM_PixelItemTag);
                DcmElement *elementZero=newItemZero;

                sequence->insert(newItemZero);
                string dicomVal="";
                /* set the value of the newly created element */
                result = elementZero->putString(OFreinterpret_cast(const char *, dicomVal.c_str()));

                /* create new pixel item */
                DcmPixelItem *newItem = new DcmPixelItem(DCM_PixelItemTag);

                DcmElement *element=newItem;
                if (newItem != NULL)
                {
                    sequence->insert(newItem);
                    /* put pixel data into the item */
                    //putElementContent(current, newItem);
                    /* try to open binary file */
                    FILE *fileOpen = fopen(inputVideoFile, "rb");

                    if(fileOpen!=NULL){

                        /* determine filesize */
                        const size_t fileSize = OFStandard::getFileSize(inputVideoFile);
                        size_t buflen = fileSize;

                        /* if odd then make even (DICOM requires even length values) */
                        if (buflen & 1){
                            buflen++;
                        }

                        Uint8 *buf = NULL;
                        OFCondition fileCreationResult=element->createUint8Array(OFstatic_cast(Uint32, buflen), buf);

                        if(fileCreationResult.good()){

                            /* read binary file into the buffer */
                            if (fread(buf, 1, OFstatic_cast(size_t, fileSize), fileOpen) != fileSize)
                            {
                                LOGIVC("error reading binary data file:");
                                fileCreationResult = EC_CorruptedData;
                            }


                            if(fileCreationResult.good()){

                                /* check whether this is possible */
                                if (fileformat.canWriteXfer(xfer))
                                {

                                    if (DcmXfer(xfer).isEncapsulated())
                                    {

                                        opt_writeMode = EWM_fileformat;
                                    }

                                    /* write DICOM file */

                                    OFCondition dicomResult=fileformat.saveFile(filePathC,xfer,opt_enctype,opt_glenc, opt_padenc,OFstatic_cast(Uint32, opt_filepad),
                                                        OFstatic_cast(Uint32, opt_itempad), opt_writeMode);

                                    if(dicomResult.good()){

                                        LOGIVC("Dicom file created successfully");
                                        resultData="Success";
                                    }else{

                                        resultData="Fail";
                                    }


                                }


                            }

                        }

                        fclose(fileOpen);

                    }

                }

            }
        }

    }else{

        LOGIVC("Error occur in insert of DCmElement---> %s",result.text());

        resultData="Fail";
    }

    return env->NewStringUTF(resultData.c_str());
}
