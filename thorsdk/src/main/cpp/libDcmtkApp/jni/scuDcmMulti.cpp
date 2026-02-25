//
// Created by Echonous on 6/29/2018.
//
#include <jni.h>
#include "include/dcmtkUtilJni.h"
#include "include/djencoder.h"
#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmdata/dcpxitem.h"
#include <android/log.h>
#include <dcmtk/dcmjpeg/djencode.h>
#include <dcmtk/dcmjpeg/djrplol.h>
#include <dcmtk/dcmjpeg/djencode.h>
#include <dcmtk/dcmjpeg/dipijpeg.h>
#include <dcmtk/dcmjpeg/djrplol.h>
#include <dcmtk/dcmjpeg/djrploss.h>
#include <dcmtk/dcmjpeg/djencode.h>
#include "dcmtk/dcmdata/dcrleerg.h"  /* for DcmRLEEncoderRegistration */
#include "dcmtk/dcmimage/diregist.h"  /* include to support color images */
#include "dcmtk/dcmjpls/djlsutil.h"   /* for dcmjpls typedefs */
#include "dcmtk/dcmjpls/djencode.h"   /* for class DJLSEncoderRegistration */
#include "dcmtk/dcmjpls/djrparam.h"   /* for class DJLSRepresentationParameter */
#include <cmath>
#include <malloc.h>
#include <cstdlib>
#include <cstring>
#include <dcmtk/dcmjpeg/djcodece.h>
#include <dcmtk/dcmjpeg/djencbas.h>
#include <dcmtk/dcmjpeg/djencext.h>
#include <dcmtk/ofstd/ofcmdln.h>

// dcmjpeg includes
#include "dcmtk/dcmjpeg/djcparam.h"   /* for class DJCodecParameter */
#include "dcmtk/dcmjpeg/djencabs.h"   /* for class DJEncoder */

// dcmimgle includes
#include "dcmtk/dcmimgle/dcmimage.h"  /* for class DicomImage */

// dcmdata includes
#include "dcmtk/dcmdata/dcpxitem.h"   /* for class DcmPixelItem */

#include "dcmtk/dcmjpeg/djcodece.h"
#include "dcmtk/dcmjpeg/djencabs.h"

// dcmdata includes
#include "dcmtk/dcmdata/dcdatset.h"   /* for class DcmDataset */
#include "dcmtk/dcmdata/dcdeftag.h"   /* for tag constants */
#include "dcmtk/dcmdata/dcovlay.h"    /* for class DcmOverlayData */
#include "dcmtk/dcmdata/dcpixseq.h"   /* for class DcmPixelSequence */
#include "dcmtk/dcmdata/dcpxitem.h"   /* for class DcmPixelItem */
#include "dcmtk/dcmdata/dcuid.h"      /* for dcmGenerateUniqueIdentifer()*/
#include "dcmtk/dcmdata/dcvrcs.h"     /* for class DcmCodeString */
#include "dcmtk/dcmdata/dcvrds.h"     /* for class DcmDecimalString */
#include "dcmtk/dcmdata/dcvrlt.h"     /* for class DcmLongText */
#include "dcmtk/dcmdata/dcvrst.h"     /* for class DcmShortText */
#include "dcmtk/dcmdata/dcvrus.h"     /* for class DcmUnsignedShort */
#include "dcmtk/dcmdata/dcswap.h"     /* for swapIfNecessary */
#include "../../libThorCommon/include/ThorUtils.h"
#include "jsoncpp/json/json.h"


using namespace std;

static string sReferenceStudyId;
static string sReferenceInstanceUid;

jstring
DCMTKUTIL_METHOD(createMutiDcm)(JNIEnv* env,jobject thiz,jobjectArray dcmInfo,jstring filePath,jstring fileName,
        jobjectArray ultrasoundRegionsInfo,jint width,jint height, jint jpegCompressionType){

    jboolean isCopy;
    const char* filePathC=env->GetStringUTFChars(filePath,&isCopy);
    const char* fileInput=env->GetStringUTFChars(fileName,&isCopy);
    char uid[100];
    const Uint16 mUintWidth=width;
    const Uint16 mUintHeight=height;
    const float mMiliSeconds=1000.0F;
    LOGI("DCM Bitmap width %d Bitmap height %d", mUintWidth,mUintHeight);

    const char* patientName=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, patientNameIndex),&isCopy);
    const char* patientId=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, patientIdIndex),&isCopy);
    const char* patientSex=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, patientSexIndex),&isCopy);
    const char* studyId=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, studyIdIndex),&isCopy);
    const char* studyDate=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, studyDateIndex),&isCopy);
    const char* studyAccessNumber=env->GetStringUTFChars((jstring)env->GetObjectArrayElement(dcmInfo, studyAccessNumberIndex),&isCopy);
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

    bool  isJpegBaseline = true;
    if(jpegCompressionType == JPEG_BASELINE){
        isJpegBaseline = true;
    }else if(jpegCompressionType == JPEG_LOSSLESS){
        isJpegBaseline = false;
    }

    string mJsonString(mwlJsonData);
    bool mIsMwlData;
    bool mIsMwlRead;
    mIsMwlData = !mJsonString.empty();
    LOGI("DCM MWL JSON DATA Is %s", mJsonString.c_str());

    std::stringstream jsonData{mJsonString};
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    bool ret = Json::parseFromStream(builder, jsonData, &root, &errs);
    if (!ret || !root.isObject()) {
        std::cout << "cannot convert string to json, err: " << errs << std::endl;
        LOGI("JSON Read Error for the MWL Data");
        mIsMwlRead= false;
    }else{
        LOGI("JSON Read Success for the MWL Data");
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

    LOGI("Size of US Region is %d", mSize);

    bool mIsSecondRegion = false;

    if(mSize == UsRegionSize ){
        mIsSecondRegion = true;
    }


    DcmFileFormat fileformat;
    const char* modality="US";
    DcmDataset *dataset = fileformat.getDataset();
    dataset->putAndInsertString(DCM_SOPClassUID, UID_UltrasoundMultiframeImageStorage);
    char * sopInstanceUID=dcmGenerateUniqueIdentifier(uid, SITE_INSTANCE_UID_ROOT);
    dataset->putAndInsertString(DCM_SOPInstanceUID, sopInstanceUID);

    //Insert DcmItem tags
    OFCondition resultTag = EC_InvalidValue;
    //Study
    LOGI("StudyInstance is %s",studyInstance);
    resultTag=dataset->putAndInsertString(DCM_StudyInstanceUID,studyInstance);
    if(resultTag.good()){
        LOGI("StudyInstance inserted successfully");
    }else{
        LOGI("StudyInstance inserted Failed %s",resultTag.text());
    }
    resultTag=dataset->putAndInsertString(DCM_StudyTime, studyTime);

    LOGI("SeriesInstance is %s",seriesInstance);
    resultTag=dataset->putAndInsertString(DCM_SeriesInstanceUID,seriesInstance);
    if(resultTag.good()){
        LOGI("SeriesInstance inserted successfully");
    }else{
        LOGI("SeriesInstance inserted Failed %s",resultTag.text());
    }
    resultTag=dataset->putAndInsertString(DCM_SeriesNumber, seriesNumber);

    dataset->putAndInsertString(DCM_PatientName,patientName,OFTrue);
    dataset->putAndInsertString(DCM_PatientID,patientId,OFTrue);
    dataset->putAndInsertString(DCM_StudyID,studyId,OFTrue);
    dataset->putAndInsertString(DCM_PatientSex,patientSex,OFTrue);
    dataset->putAndInsertString(DCM_PatientSize,patientHeight,OFTrue);
    dataset->putAndInsertString(DCM_PatientWeight,patientWeight,OFTrue);
    LOGI("Patient Wight MJPEG %s Height %s",patientWeight,patientHeight);

    // general study module attributes
    dataset->putAndInsertString(DCM_StudyDate,studyDate,OFTrue);
    dataset->putAndInsertString(DCM_Modality,modality,OFTrue);
    dataset->putAndInsertString(DCM_ImageType, imageType);
    dataset->putAndInsertString(DCM_AccessionNumber,studyAccessNumber,OFTrue);
    dataset->putAndInsertString(DCM_Manufacturer,MANUFACTURE);
    dataset->putAndInsertString(DCM_TimezoneOffsetFromUTC,timeZoneoffSetFromUTC,OFTrue);

    dataset->putAndInsertString(DCM_SeriesDate,studyDate,OFTrue);
    dataset->putAndInsertString(DCM_SeriesTime,studyTime,OFTrue);

    dataset->putAndInsertString(DCM_PatientOrientation, patientOrientation);  //anterior  https://dicom.innolitics.com/ciods/us-image/general-image/00200020
    dataset->putAndInsertString(DCM_PatientBirthDate, patientDoB);
    dataset->putAndInsertString(DCM_ReferringPhysicianName, referPhysician);
    LOGI("InstanceNumber is  MJPEG %s",instanceNumber);
    dataset->putAndInsertString(DCM_InstanceNumber, instanceNumber);


    dataset->putAndInsertString(DCM_PhotometricInterpretation, "YBR_FULL_422");
    dataset->putAndInsertUint16(DCM_PixelRepresentation, 0);

    dataset->putAndInsertString(DCM_SamplesPerPixel, "3");
    dataset->putAndInsertString(DCM_PlanarConfiguration, "0");

    dataset->putAndInsertUint16(DCM_BitsAllocated, 8);

    dataset->putAndInsertUint16(DCM_BitsStored, 8);
    dataset->putAndInsertUint16(DCM_HighBit, 7);
    dataset->putAndInsertUint16(DCM_Columns,mUintWidth);
    dataset->putAndInsertUint16(DCM_Rows, mUintHeight);
    //Point this to DCM_FrameTime
    dataset->putAndInsertString(DCM_FrameIncrementPointer,"(0018,1063)");
    dataset->putAndInsertString(DCM_SpecificCharacterSet, "ISO_IR 100");
    dataset->putAndInsertString(DCM_InstanceCreationDate,studyDate,OFTrue);
    dataset->putAndInsertString(DCM_InstanceCreationTime,studyTime,OFTrue);
    dataset->putAndInsertString(DCM_ContentDate,acquisitionDate,OFTrue);
    dataset->putAndInsertString(DCM_ContentTime,acquisitionTime,OFTrue);
    dataset->putAndInsertString(DCM_PresentationIntentType,"FOR PRESENTATION");
    dataset->putAndInsertString(DCM_InstitutionName,facilityName);
    dataset->putAndInsertString(DCM_StationName,"");
    dataset->putAndInsertString(DCM_ManufacturerModelName,"KOSMOS");
    dataset->putAndInsertString(DCM_StudyDescription,examPurpose);

    //In our case both Refer Physical and RequestingPhysician will be same
    dataset->putAndInsertString(DCM_RequestingPhysician,referPhysician);
    //Lossy Compression
    dataset->putAndInsertString(DCM_DerivationDescription,"Lossy JPEG");

    dataset->putAndInsertString(DCM_PatientComments,patientComments);

    dataset->putAndInsertString(DCM_DeviceSerialNumber,deviceSerialNo);

    dataset->putAndInsertString(DCM_SoftwareVersions,softVerNo);

    dataset->putAndInsertString(DCM_DigitalImageFormatAcquired,"Bitmap Import");

    dataset->putAndInsertString(DCM_ProtocolName,protocolName);

    dataset->putAndInsertString(DCM_TransducerData,TransducerData);
    dataset->putAndInsertString(DCM_ProcessingFunction,"CARD_ADULT_GEN_CV");
    dataset->putAndInsertUint16(DCM_UltrasoundColorDataPresent,01);
    dataset->putAndInsertString(DCM_BurnedInAnnotation,"YES");

    dataset->putAndInsertString(DCM_PerformedProcedureStepStartDate,studyDate,OFTrue);
    dataset->putAndInsertString(DCM_PerformedProcedureStepStartTime,studyTime,OFTrue);
    //PerformedProcedureStepDescription
    dataset->putAndInsertString(DCM_PerformedProcedureStepDescription,examPurpose,OFTrue);

    dataset->putAndInsertString(DCM_EthnicGroup,ethnicGroup);

    dataset->putAndInsertString(DCM_PhysiciansOfRecord,physicianofRecord,OFTrue);
    dataset->putAndInsertString(DCM_PerformingPhysicianName,performPhysician,OFTrue);
    dataset->putAndInsertString(DCM_OperatorsName,performPhysician,OFTrue);
    dataset->putAndInsertString(DCM_StationName,kosmosStationName,OFTrue);
    dataset->putAndInsertString(DCM_SOPInstanceUID, sopInstantUid, OFTrue);
    if(strlen(institutionDepartmentName) != 0){
        LOGI("institutionDepartmentName is %s",institutionDepartmentName);
        dataset->putAndInsertString(DCM_InstitutionalDepartmentName,institutionDepartmentName,OFTrue);
    }
    if(string(calReportComment).compare("NC") != 0){
        dataset->putAndInsertString(DCM_PatientComments,calReportComment,OFTrue);
    }

    string stepId=string(studyDate);
    LOGI("Study Date %s", studyDate);
    stepId.append(".",1);
    LOGI("Study Append Date %s",stepId.c_str());
    stepId.append(studyTime,6);
    LOGI("Study Append time %s",stepId.c_str());

    dataset->putAndInsertString(DCM_PerformedProcedureStepID,stepId.c_str(),OFTrue);
    dataset->putAndInsertString(DCM_CommentsOnThePerformedProcedureStep,"Adult Echo",OFTrue);

    string aquisitionDateTime=string();
    aquisitionDateTime.append(acquisitionTime,6);
    dataset->putAndInsertString(DCM_AcquisitionTime,aquisitionDateTime.c_str(),OFTrue);
    LOGI("DCM_AcquisitionTime %s",aquisitionDateTime.c_str());
    dataset->putAndInsertString(DCM_AcquisitionDate,acquisitionDate,OFTrue);
    LOGI("DCM_AcquisitionDate %s",acquisitionDate);
    dataset->putAndInsertString(DCM_PresentationLUTShape,"IDENTITY",OFTrue);

    auto transferSyntax = EXS_JPEGProcess1;
    if(isJpegBaseline){
        transferSyntax = EXS_JPEGProcess1;
    }else{
        transferSyntax = EXS_JPEGProcess14SV1;
    }

    E_TransferSyntax opt_oxfer = transferSyntax;
    OFCmdUnsignedInt opt_selection_value = 6;
    OFCmdUnsignedInt opt_point_transform = 0;
    OFCmdUnsignedInt opt_quality = JPEG_LOSSY_QUALITY;
    OFBool           opt_huffmanOptimize = OFTrue;
    int              opt_smoothing = 0;
    int              opt_compressedBits = 0; // 0=auto, 8/12/16=force
    E_CompressionColorSpaceConversion opt_compCSconversion = ECC_lossyYCbCr;
    E_DecompressionColorSpaceConversion opt_decompCSconversion = EDC_photometricInterpretation;
    E_SubSampling    opt_sampleFactors = ESS_422;
    OFBool           opt_useYBR422 = OFTrue;
    Uint32           opt_fragmentSize = 0; // 0=unlimited
    OFBool           opt_createOffsetTable = OFTrue;
    size_t              opt_windowType = 0;  /* default: no windowing; 1=Wi, 2=Wl, 3=Wm, 4=Wh, 5=Ww, 6=Wn, 7=Wr */
    OFCmdUnsignedInt opt_windowParameter = 0;
    OFCmdFloat       opt_windowCenter=0.0, opt_windowWidth=0.0;
    E_UIDCreation    opt_uidcreation = EUC_default;
    OFBool           opt_secondarycapture = OFFalse;
    OFCmdUnsignedInt opt_roiLeft = 0, opt_roiTop = 0, opt_roiWidth = 0, opt_roiHeight = 0;
    OFBool           opt_usePixelValues = OFTrue;
    OFBool           opt_useModalityRescale = OFFalse;

    auto optTrueLossLess = OFFalse;
    if(isJpegBaseline){
        optTrueLossLess = OFFalse;
    }else{
        optTrueLossLess = OFTrue;
    }
    OFBool           opt_trueLossless = optTrueLossLess;

    if(mFrameTime!= nullptr){

        float frameTime=std::stof(string(mFrameTime));
        int cineFrameRate = (int) frameTime;
        dataset->putAndInsertString(DCM_CineRate,to_string(cineFrameRate).c_str());
        LOGI("DICOM Frame Time per second %d", cineFrameRate);
        float mFrameRoundValue = floorf((mMiliSeconds/frameTime) * 100) / 100;
        string mFrameTimeDicom=to_string(mFrameRoundValue);
        LOGI("DICOM Frame time is %s",mFrameTimeDicom.c_str());
        dataset->putAndInsertString(DCM_FrameTime,mFrameTimeDicom.c_str());
    }

    DcmItem *mSequenceofUltrasoundRegions;

    //Insert DcmItem tags
    OFCondition result = EC_IllegalCall;
    if(dataset->findOrCreateSequenceItem(DCM_SequenceOfUltrasoundRegions,mSequenceofUltrasoundRegions).good()){

        auto regionSpatial=(Uint16)std::atoi(mRegionSpatialFormat);
        result=mSequenceofUltrasoundRegions->putAndInsertUint16(DCM_RegionSpatialFormat,regionSpatial);
        if (result.bad()){
            LOGI("Bad DCM_RegionSpatialFormat");
        }

        auto RegionDataType=(Uint16)std::atoi(mRegionDataType);
        result=mSequenceofUltrasoundRegions->putAndInsertUint16(DCM_RegionDataType,RegionDataType);
        if (result.bad()){
            LOGI("Bad DCM_RegionDataType");
        }

        auto RegionFlags=(Uint32)std::atoi(mRegionFlags);
        result=mSequenceofUltrasoundRegions->putAndInsertUint32(DCM_RegionFlags,RegionFlags);
        if (result.bad()){
            LOGI("Bad DCM_RegionFlags");
        }


        result=mSequenceofUltrasoundRegions->putAndInsertUint32(DCM_RegionLocationMinX0,(Uint32)std::atoi(mRegionLocX0));
        if (result.bad()){
            LOGI("Bad DCM_RegionLocationMinX0");
        }

        result=mSequenceofUltrasoundRegions->putAndInsertUint32(DCM_RegionLocationMinY0,(Uint32)std::atoi(mRegionLocY0));
        if (result.bad()){
            LOGI("Bad DCM_RegionLocationMinY0");
        }

        result=mSequenceofUltrasoundRegions->putAndInsertUint32(DCM_RegionLocationMaxX1,(Uint32)std::atoi(mRegionLocX1));
        if (result.bad()){
            LOGI("Bad DCM_RegionLocationMaxX1");
        }

        result=mSequenceofUltrasoundRegions->putAndInsertUint32(DCM_RegionLocationMaxY1,(Uint32)std::atoi(mRegionLocY1));
        if (result.bad()){
            LOGI("Bad DCM_RegionLocationMaxY1");
        }

        result=mSequenceofUltrasoundRegions->putAndInsertUint16(DCM_PhysicalUnitsXDirection,(Uint16)std::strtol(mPhysicalUnitsXDirection,
                                                                                                                nullptr,10));
        if (result.bad()){
            LOGI("Bad DCM_PhysicalUnitsXDirection");
        }

        result=mSequenceofUltrasoundRegions->putAndInsertUint16(DCM_PhysicalUnitsYDirection,(Uint16)std::strtol(mPhysicalUnitsYDirection,
                                                                                                                nullptr,10));
        if (result.bad()){
            LOGI("Bad DCM_PhysicalUnitsYDirection");
        }

        result=mSequenceofUltrasoundRegions->putAndInsertFloat64(DCM_PhysicalDeltaX,(Float64)std::strtod(mPhysicalDeltaX,nullptr));
        if (result.bad()){
            LOGI("Bad DCM_PhysicalDeltaX");
        }

        result=mSequenceofUltrasoundRegions->putAndInsertFloat64(DCM_PhysicalDeltaY,(Float64) std::strtod(mPhysicalDeltaY,nullptr));
        if (result.bad()){
            LOGI("Bad DCM_PhysicalDeltaY");
        }

        result=mSequenceofUltrasoundRegions->putAndInsertSint32(DCM_ReferencePixelX0,(Sint32)std::atoi(mReferencePixelX0));
        if(result.bad()){
            LOGI("Bad DCM_ReferencePixelX0");
        }

        result=mSequenceofUltrasoundRegions->putAndInsertSint32(DCM_ReferencePixelY0,(Sint32)std::atoi(mReferencePixelY0));
        if(result.bad()){
            LOGI("Bad DCM_ReferencePixelY0");
        }


        result=mSequenceofUltrasoundRegions->putAndInsertFloat64(DCM_ReferencePixelPhysicalValueX,(Float64)std::strtod(mReferencePixelPhysicalValueX,nullptr));
        if(result.bad()){
            LOGI("Bad DCM_ReferencePixelPhysicalValueX");
        }

        result=mSequenceofUltrasoundRegions->putAndInsertFloat64(DCM_ReferencePixelPhysicalValueY,(Float64)std::strtod(mReferencePixelPhysicalValueY,nullptr));
        if(result.bad()){
            LOGI("Bad DCM_ReferencePixelPhysicalValueY");
        }

    }else{
        LOGI("Cannot add SequenceOfUltrasoundRegions");
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
                LOGI("Bad DCM_RegionSpatialFormat %s",result.text());
            }

            auto RegionDataType=(Uint16)std::atoi(mRegionDataTypeSecond);
            result=mSequenceofUltrasoundRegionsSecond->putAndInsertUint16(DCM_RegionDataType,RegionDataType);
            if (result.bad()){
                LOGI("Bad DCM_RegionDataType %s",result.text());
            }

            auto RegionFlags=(Uint32)std::atoi(mRegionFlagsSecond);
            result=mSequenceofUltrasoundRegionsSecond->putAndInsertUint32(DCM_RegionFlags,RegionFlags);
            if (result.bad()){
                LOGI("Bad DCM_RegionFlags %s ",result.text());
            }

            result=mSequenceofUltrasoundRegionsSecond->putAndInsertUint32(DCM_RegionLocationMinX0,(Uint32)std::atoi(mRegionLocX0Second));
            if (result.bad()){
                LOGI("Bad DCM_RegionLocationMinX0");
            }

            result=mSequenceofUltrasoundRegionsSecond->putAndInsertUint32(DCM_RegionLocationMinY0,(Uint32)std::atoi(mRegionLocY0Second));
            if (result.bad()){
                LOGI("Bad DCM_RegionLocationMinY0");
            }

            result=mSequenceofUltrasoundRegionsSecond->putAndInsertUint32(DCM_RegionLocationMaxX1,(Uint32)std::atoi(mRegionLocX1Second));
            if (result.bad()){
                LOGI("Bad DCM_RegionLocationMaxX1");
            }

            result=mSequenceofUltrasoundRegionsSecond->putAndInsertUint32(DCM_RegionLocationMaxY1,(Uint32)std::atoi(mRegionLocY1Second));
            if (result.bad()){
                LOGI("Bad DCM_RegionLocationMaxY1");
            }

            result=mSequenceofUltrasoundRegionsSecond->putAndInsertUint16(DCM_PhysicalUnitsXDirection,(Uint16)std::strtol(mPhysicalUnitsXDirectionSecond,
                                                                                                                    nullptr,10));
            if (result.bad()){
                LOGI("Bad DCM_PhysicalUnitsXDirection");
            }

            result=mSequenceofUltrasoundRegionsSecond->putAndInsertUint16(DCM_PhysicalUnitsYDirection,(Uint16)std::strtol(mPhysicalUnitsYDirectionSecond,
                                                                                                                    nullptr,10));
            if (result.bad()){
                LOGI("Bad DCM_PhysicalUnitsYDirection");
            }

            result=mSequenceofUltrasoundRegionsSecond->putAndInsertFloat64(DCM_PhysicalDeltaX,(Float64)std::strtod(mPhysicalDeltaXSecond,nullptr));
            if (result.bad()){
                LOGI("Bad DCM_PhysicalDeltaX");
            }

            result=mSequenceofUltrasoundRegionsSecond->putAndInsertFloat64(DCM_PhysicalDeltaY,(Float64) std::strtod(mPhysicalDeltaYSecond,nullptr));
            if (result.bad()){
                LOGI("Bad DCM_PhysicalDeltaY");
            }

            result=mSequenceofUltrasoundRegionsSecond->putAndInsertSint32(DCM_ReferencePixelX0,(Sint32)std::atoi(mReferencePixelX0Second));
            if(result.bad()){
                LOGI("Bad DCM_ReferencePixelX0");
            }

            result=mSequenceofUltrasoundRegionsSecond->putAndInsertSint32(DCM_ReferencePixelY0,(Sint32)std::atoi(mReferencePixelY0Second));
            if(result.bad()){
                LOGI("Bad DCM_ReferencePixelY0");
            }


            result=mSequenceofUltrasoundRegionsSecond->putAndInsertFloat64(DCM_ReferencePixelPhysicalValueX,(Float64)std::strtod(mReferencePixelPhysicalValueXSecond,nullptr));
            if(result.bad()){
                LOGI("Bad DCM_ReferencePixelPhysicalValueX");
            }

            result=mSequenceofUltrasoundRegionsSecond->putAndInsertFloat64(DCM_ReferencePixelPhysicalValueY,(Float64)std::strtod(mReferencePixelPhysicalValueYSecond,nullptr));
            if(result.bad()){
                LOGI("Bad DCM_ReferencePixelPhysicalValueY");
            }

        }else{
            LOGI("Cannot add SequenceOfUltrasoundRegions");
        }
    }

    DcmItem *mSourceImageSequence;
    if(dataset->findOrCreateSequenceItem(DCM_SourceImageSequence,mSourceImageSequence).good()){
        result=mSourceImageSequence->putAndInsertString(DCM_ReferencedSOPClassUID, UID_UltrasoundMultiframeImageStorage);
        if(result.bad()){
            LOGI("Bad DCM_SourceImageSequence");
        }
        result=mSourceImageSequence->putAndInsertString(DCM_ReferencedSOPInstanceUID, sopInstanceUID);
        if(result.bad()){
            LOGI("Bad DCM_ReferencedSOPInstanceUID");
        }
    }


/*
 *  Disable this as it will be added when we have  ProcedureCodeSequence available
 *  DcmItem *mProcedureCodeSequence;
    if(dataset->findOrCreateSequenceItem(DCM_ProcedureCodeSequence,mProcedureCodeSequence).good()){

        result=mProcedureCodeSequence->putAndInsertString(DCM_CodeValue, studyId);
        if(result.bad()){
            LOGI("Bad DCM_CodeValue mProcedureCodeSequence");
        }

        result=mProcedureCodeSequence->putAndInsertString(DCM_CodingSchemeDesignator, "");
        if(result.bad()){
            LOGI("Bad DCM_CodingSchemeDesignator mProcedureCodeSequence");
        }

        result=mProcedureCodeSequence->putAndInsertString(DCM_CodeMeaning, examPurpose);
        if(result.bad()){
            LOGI("Bad DCM_CodeMeaning mProcedureCodeSequence");
        }
    }*/

    DcmItem *mReferencedPerformedProcedureStepSequence;
    if(dataset->findOrCreateSequenceItem(DCM_ReferencedPerformedProcedureStepSequence,mReferencedPerformedProcedureStepSequence).good()){
        result=mReferencedPerformedProcedureStepSequence->putAndInsertString(DCM_ReferencedSOPClassUID, UID_ModalityPerformedProcedureStepSOPClass);
        if(result.bad()){
            LOGI("Bad DCM_ReferencedSOPClassUID ReferencedPerformedProcedureStepSequence");
        }
        result=mReferencedPerformedProcedureStepSequence->putAndInsertString(DCM_ReferencedSOPInstanceUID, getReferenceInstanceExamUID(studyId));
        if(result.bad()){
            LOGI("Bad DCM_ReferencedSOPInstanceUID ReferencedPerformedProcedureStepSequence");
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

   /* Disable this as it will be added when we have  PerformedProtocolCodeSequence available
    * DcmItem *mPerformedProtocolCodeSequenceOuter;
    if(dataset->findOrCreateSequenceItem(DCM_PerformedProtocolCodeSequence,mPerformedProtocolCodeSequenceOuter).good()){
        result=mPerformedProtocolCodeSequenceOuter->putAndInsertString(DCM_CodeValue, studyId);
        if(result.bad()){
            LOGI("Bad DCM_CodeValue PerformedProtocolCodeSequence");
        }

        result=mPerformedProtocolCodeSequenceOuter->putAndInsertString(DCM_CodingSchemeDesignator, "");
        if(result.bad()){
            LOGI("Bad DCM_CodingSchemeDesignator PerformedProtocolCodeSequence");
        }

        result=mPerformedProtocolCodeSequenceOuter->putAndInsertString(DCM_CodeMeaning, examPurpose);
        if(result.bad()){
            LOGI("Bad DCM_CodeMeaning PerformedProtocolCodeSequence");
        }
    }*/



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
            LOGI("MWL Encoding DCM_RequestAttributesSequence");
            result=mRequestAttributesSequence->putAndInsertString(DCM_RequestedProcedureDescription, rpDesc.c_str());
            if(result.bad()){
                LOGI("Bad DCM_RequestedProcedureDescription RequestAttributesSequence");
            }

            result=mRequestAttributesSequence->putAndInsertString(DCM_ScheduledProcedureStepDescription, spStepDesc.c_str());
            if(result.bad()){
                LOGI("Bad DCM_ScheduledProcedureStepDescription RequestAttributesSequence");
            }

            result=mRequestAttributesSequence->putAndInsertString(DCM_RequestedProcedureID, rpId.c_str());
            if(result.bad()){
                LOGI("Bad RequestedProcedureID RequestAttributesSequence");
            }

            result=mRequestAttributesSequence->putAndInsertString(DCM_ScheduledProcedureStepID, spStepId.c_str());
            if(result.bad()){
                LOGI("Bad DCM_ScheduledProcedureStepID RequestAttributesSequence");
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
                        LOGI("Bad DCM_CodeValue ScheduledProtocolCodeSequence");
                    }

                    result=mScheduledProtocolCodeSequence->putAndInsertString(DCM_CodingSchemeDesignator, codingSchemeDesignator.c_str());
                    if(result.bad()){
                        LOGI("Bad DCM_CodingSchemeDesignator ScheduledProtocolCodeSequence");
                    }
                    result=mScheduledProtocolCodeSequence->putAndInsertString(DCM_CodeMeaning, codeMeaning.c_str());
                    if(result.bad()){
                        LOGI("Bad DCM_CodeMeaning ScheduledProtocolCodeSequence");
                    }

                    result = mScheduledProtocolCodeSequence->putAndInsertString(DCM_CodingSchemeVersion,codingSchemeVersion.c_str());
                    if(result.bad()){
                        LOGI("Bad DCM_CodingSchemeVersion ScheduledProtocolCodeSequence");
                    }

                }else{
                    LOGI("Bad DCM_ScheduledProtocolCodeSequence RequestAttributesSequence DICOM Sub tag failed");
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
                    LOGI("Bad DCM_ReferencedSOPClassUID ReferencedStudySequence");
                }

                auto strReferencedSOPInstanceUID = referencedStudySequenceJson[ReferencedSOPInstanceUID].asString();
                result= mReferencedStudySequence-> putAndInsertString(DCM_ReferencedSOPInstanceUID, strReferencedSOPInstanceUID.c_str());
                if(result.bad()){
                    LOGI("Bad DCM_ReferencedSOPInstanceUID ReferencedStudySequence");
                }

            }

        }else{
            LOGI("Cannot add DCM_ReferencedStudySequence");
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

    LOGI("Dicom multi file Step 1");

    FILE *fileOpen = fopen(fileInput, "rb");
    string resultData="Fail";
    if(fileOpen!= nullptr) {

        Uint16 samplesPerPixel = 3;
        EP_Interpretation interpr = EPI_RGB;
        LOGI("File Open pointer");
        /* determine filesize */
        const size_t fileSize = OFStandard::getFileSize(fileInput);
        //size_t buflen = fileSize;
        LOGI("Raw file size is %ld",fileSize);

        //OFCondition fileCreationResult;
        size_t mFrameNumber=fileSize/(mUintWidth*mUintHeight*samplesPerPixel);
        LOGI("Total Frame Count is %zu",mFrameNumber);
        float frameRate=std::stof(string(mFrameTime));
        float effectiveTime = mFrameNumber/frameRate;
        LOGI("Effective Time %s",to_string(effectiveTime).c_str());
        dataset->putAndInsertString(DCM_EffectiveDuration,to_string(effectiveTime).c_str());
        string mSizeFrameCount=to_string(mFrameNumber);

        dataset->putAndInsertString(DCM_NumberOfFrames, mSizeFrameCount.c_str());
        size_t mSizeFrame=mUintWidth*mUintHeight*samplesPerPixel;

        // create initial pixel sequence with empty offset table
        DcmPixelSequence *pixelSequence = nullptr;
        DcmPixelItem *offsetTable = nullptr;
        LOGI("JPEG step 1");
        if (result.good())
        {
            pixelSequence = new DcmPixelSequence(DcmTag(DCM_PixelData,EVR_OB));
            if (pixelSequence == nullptr)
            {
                result = EC_MemoryExhausted;
            }
            else
            {
                // create empty offset table
                offsetTable = new DcmPixelItem(DcmTag(DCM_Item,EVR_OB));
                if (offsetTable == nullptr) {
                    result = EC_MemoryExhausted;
                }
                else {
                    pixelSequence->insert(offsetTable);
                }
            }
        }



        DJ_RPLossy mLossyParams(JPEG_LOSSY_QUALITY);
        // create representation parameters for lossy and lossless
        DJ_RPLossless rp_lossless((int)opt_selection_value, (int)opt_point_transform);
        const DcmRepresentationParameter * mDcmRef=&mLossyParams;

        if(isJpegBaseline){
            mDcmRef = &mLossyParams;
        }else{
            mDcmRef = &rp_lossless;
        }

        DcmRLEEncoderRegistration::registerCodecs(); // Run length encoding encoder
        DJEncoderRegistration::registerCodecs();  // JPEG encoder
        DJLSEncoderRegistration::registerCodecs(); // JPEG-LS codec

        unsigned long compressedSize = 0;
        double compressionRatio = 0.0; // initialize if something goes wrong
        double uncompressedSize =mSizeFrame*mFrameNumber;
        DcmOffsetList offsetList;
        // assume we can cast the codecparameter to what we need
        auto *djcp =new DJCodecParameter(opt_compCSconversion,
                                         opt_decompCSconversion,
                                         opt_uidcreation,
                                         EPC_default,
                                         OFFalse,
                                         OFFalse,
                                         OFFalse,
                                         opt_huffmanOptimize,
                                         opt_smoothing,
                                         0,
                                         opt_fragmentSize,
                                         opt_createOffsetTable,
                                         opt_sampleFactors,
                                         opt_useYBR422,
                                         opt_secondarycapture,
                                         opt_windowType,
                                         opt_windowParameter,
                                         opt_windowCenter,
                                         opt_windowWidth,
                                         opt_roiLeft,
                                         opt_roiTop,
                                         opt_roiWidth,
                                         opt_roiHeight,
                                         opt_usePixelValues,
                                         opt_useModalityRescale,
                                         OFFalse,
                                         OFFalse,
                                         opt_trueLossless);
        // JPEG Lossy (8 bit)
        // main loop for compression: compress each frame
        for(int i=0;i<mFrameNumber;i++)
        {
            Uint8 *image_buffer;
            image_buffer = (Uint8 *) malloc (mSizeFrame);


            if(image_buffer!= nullptr)
            {
                bool isReadDone= true;

                /* read binary file into the buffer */
                if (fread(image_buffer, 1, OFstatic_cast(size_t, mSizeFrame), fileOpen) != mSizeFrame)
                {
                    LOGI("error reading binary data file:");
                    isReadDone=false;
                }

                if(isReadDone)
                {
                    //Main
                    //const Uint8 *framePointer = OFreinterpret_cast(const Uint8 *, image_buffer);

                    DJEncoder *jpeg = nullptr;
                    if(isJpegBaseline){
                        jpeg = new DJCompressIJG8Bit(*djcp, EJM_baseline, OFstatic_cast(Uint8, JPEG_LOSSY_QUALITY));
                    }else{
                        jpeg = new DJCompressIJG8Bit(*djcp, EJM_lossless, 1, rp_lossless.getPointTransformation());
                    }
                    // create encoder corresponding to bit depth (8 or 16 bit)

                    Uint8 *jpegData = nullptr;
                    Uint32 jpegLen  = 0;

                    if (jpeg)
                    {
                            //LOGI("JPEG step 2");
                            jpeg->encode(mUintWidth, mUintHeight, interpr, samplesPerPixel,image_buffer,jpegData, jpegLen);
                            //LOGI("JPEG step 2 DONE");
                            if (jpegLen == 0)
                            {
                                LOGE("True lossless encoder: Error encoding frame");
                                result = EC_CannotChangeRepresentation;
                            }
                            else
                            {
                                result = pixelSequence->storeCompressedFrame(offsetList, jpegData, jpegLen, djcp->getFragmentSize());
                                //LOGI("JPEG Data Size %d",jpegLen);
                            }
                            //LOGI("JPEG Encoder DONE");

                            delete[] jpegData;
                            compressedSize += jpegLen;
                    }
                    else
                    {
                        result = EC_MemoryExhausted;
                        LOGI("Unable to find DJCodecEncoder");
                    }


                    // store pixel sequence if everything was successful
                    if (result.good())
                    {
                       // LOGI("Good PixelSequence");
                    }
                    else
                    {
                        delete pixelSequence;
                        pixelSequence = nullptr;
                    }
                }

            }
            free(image_buffer);
            fseek(fileOpen,mSizeFrame * (i + 1),SEEK_SET);
        }

        if ((result.good()) && (djcp->getCreateOffsetTable()))
        {
            // create offset table
            result = offsetTable->createOffsetTable(offsetList);
        }

        if (compressedSize > 0) {
            compressionRatio = OFstatic_cast(double, uncompressedSize) / OFstatic_cast(double, compressedSize);

            std::string lossJpeg;
            if(isJpegBaseline){
                lossJpeg = "Lossy compression with JPEG baseline, IJG quality factor 95, compression ratio " + to_string(compressionRatio);
            }else{
                lossJpeg = "Lossless JPEG compression, selection value 6, point transform 0,compression ratio " + to_string(compressionRatio);
            }
            //Lossy Compression
            dataset->putAndInsertString(DCM_DerivationDescription,lossJpeg.c_str());
        }

        // pseudo-lossless mode may also result in lossy compression
        if (djcp->getTrueLosslessMode()){
            LOGI("JPEG:: Insert insertCodeSequence Lossless");
            result = DcmCodec::insertCodeSequence(dataset, DCM_DerivationCodeSequence, "DCM", "121327", "Full fidelity image");
        }else{
            LOGI("JPEG:: Insert insertCodeSequence Lossy");
            result = DcmCodec::insertCodeSequence(dataset, DCM_DerivationCodeSequence, "DCM", "113040", "Lossy Compression");
        }

        DcmRLEEncoderRegistration::cleanup(); // Run length encoding encoder
        DJEncoderRegistration::cleanup();  // deregister JPEG codecs
        DJLSEncoderRegistration::cleanup(); // JPEG-LS codec

        // insert pixel data attribute incorporating pixel sequence into dataset
        auto *pixelData = new DcmPixelData(DCM_PixelData);

        /* tell pixel data element that this is the original presentation of the pixel data
         * pixel data and how it compressed
        */
        pixelData->putOriginalRepresentation(opt_oxfer, mDcmRef, pixelSequence);
        result = dataset->insert(pixelData);
        if (result.bad())
        {
            LOGI("Unable to insert pixel data");
            delete pixelData;
            pixelData = nullptr; // also deletes contained pixel sequence
        }

        // update lossy compression ratio
        if (result.good() && isJpegBaseline){
            //updateLossyCompressionRatio(OFreinterpret_cast(DcmItem*, dataset), compressionRatio);
            if (dataset == nullptr) {
                result=EC_IllegalCall;
            }

            // set Lossy Image Compression to "01" (see DICOM part 3, C.7.6.1.1.5)
            result = dataset->putAndInsertString(DCM_LossyImageCompression, "01");
            if (result.bad()) {
                result=result;
            }

            // set Lossy Image Compression Ratio
            OFString s;
            const char *oldRatio = nullptr;
            if ((dataset->findAndGetString(DCM_LossyImageCompressionRatio, oldRatio)).good() && oldRatio)
            {
                s = oldRatio;
                s += "\\";
            }

            char buf[64];
            OFStandard::ftoa(buf, sizeof(buf), compressionRatio, OFStandard::ftoa_uppercase, 0, 5);
            s += buf;
            //appendCompressionRatio(s, ratio);

            result = dataset->putAndInsertString(DCM_LossyImageCompressionRatio, s.c_str());
            if (result.bad()){

            }

            // count VM of lossy image compression ratio
            size_t i;
            size_t s_vm = 0;
            size_t s_sz = s.size();
            for (i = 0; i < s_sz; ++i)
                if (s[i] == '\\') ++s_vm;

            // set Lossy Image Compression Method
            const char *oldMethod = nullptr;
            OFString m;
            if ((dataset->findAndGetString(DCM_LossyImageCompressionMethod, oldMethod)).good() && oldMethod)
            {
                m = oldMethod;
                m += "\\";
            }

            // count VM of lossy image compression method
            size_t m_vm = 0;
            size_t m_sz = m.size();
            for (i = 0; i < m_sz; ++i)
                if (m[i] == '\\') ++m_vm;

            // make sure that VM of Compression Method is not smaller than  VM of Compression Ratio
            while (m_vm++ < s_vm) m += "\\";

            m += "ISO_10918_1";
            dataset->putAndInsertString(DCM_LossyImageCompressionMethod, m.c_str());
        }

        fclose(fileOpen);

        OFCondition status;
        // check if everything went well
        if (dataset->canWriteXfer(opt_oxfer))
        {
            // force the meta-header UIDs to be re-generated when storing the file
            // since the UIDs in the data set may have changed
            // store in lossless JPEG format
            status=fileformat.saveFile(filePathC, opt_oxfer);
        }else{
            LOGI("Unable to encode dataset with desired transfer syntax");
        }

        if(status.bad()){
            LOGI("Unable to create DICOM file Error is--> %s",status.text());
            resultData.append("Fail");
            resultData.append(status.text());
        }else{
            LOGI("Multi created DICOM file created successfully-->");
            resultData="Success";
        }
    }
    LOGI("Dicom multi file Final");
    return env->NewStringUTF(resultData.c_str());
}

const char * getReferenceInstanceExamUID(const char * studyId) {
    const char * finalInstanceUId;

    if(sReferenceStudyId.length()==0){
        sReferenceStudyId=string(studyId);
        char uid[100];
        finalInstanceUId=dcmGenerateUniqueIdentifier(uid,  SITE_SERIES_UID_ROOT);
        sReferenceInstanceUid=string(finalInstanceUId);
        LOGI("Create new  Series UID for Reference %s",finalInstanceUId);
    }else{
        if(strcmp(sReferenceStudyId.c_str(),studyId)==0){
            const char * mPreviousId=sReferenceInstanceUid.c_str();
            LOGI("Use Existing Series UID for Reference");
            finalInstanceUId=mPreviousId;
        }else{
            sReferenceStudyId=string(studyId);
            char uid[100];
            finalInstanceUId=dcmGenerateUniqueIdentifier(uid,  SITE_SERIES_UID_ROOT);
            LOGI("Exam is different create new Reference UID for Study");
            sReferenceInstanceUid=string(finalInstanceUId);
        }
    }
    LOGI("Return new Series UID for Reference UID %s",finalInstanceUId);
    return sReferenceInstanceUid.c_str();
}