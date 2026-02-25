// Started
// Created by Echonous on 6/20/2018.
//
#include <jni.h>
#include "dcmtk/dcmdata/dctk.h"
#include "include/dcmtkUtilJni.h"
#include "include/dcmtkClientJni.h"
#include "dcmtk/dcmjpeg/djrplol.h"
#include "dcmtk/dcmjpeg/djencode.h"
#include "dcmtk/dcmdata/dcpixel.h"
#include "dcmtk/dcmdata/libi2d/i2d.h"
#include "dcmtk/dcmdata/libi2d/i2djpgs.h"
#include "dcmtk/dcmdata/libi2d/i2dbmps.h"
#include "dcmtk/dcmdata/libi2d/i2dplsc.h"
#include "dcmtk/dcmdata/libi2d/i2dplvlp.h"
#include "dcmtk/dcmdata/libi2d/i2dplnsc.h"
#include "dcmtk/dcmdata/dcuid.h"
#include <android/log.h>
#include <dcmtk/ofstd/ofcmdln.h>
#include "jsoncpp/json/json.h"

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
#include "dcmtk/dcmjpls/djencode.h"
#include "dcmtk/dcmjpeg/djrploss.h"

#define ERROR 0
#define SUCCESS 1

using namespace std;

#define LOGIDCM(...) \
((void)__android_log_print(ANDROID_LOG_INFO, "dcmtk::", __VA_ARGS__))


JNIEXPORT jobject
DCMTKUTIL_METHOD(createDcm)(JNIEnv* env,jobject thiz,jobjectArray dcmInfo,jstring filePath,jstring fileName,jbyteArray imgArray,
        jint width,jint height,jobjectArray ultrasoundRegionsInfo,jint jpegCompressionType){


     jboolean isCopy;
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
    LOGIDCM("DCM MWL JSON DATA Is %s", mJsonString.c_str());

    std::stringstream jsonData{mJsonString};
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    bool ret = Json::parseFromStream(builder, jsonData, &root, &errs);
    if (!ret || !root.isObject()) {
        std::cout << "cannot convert string to json, err: " << errs << std::endl;
        LOGIDCM("JSON Read Error for the MWL Data");
        mIsMwlRead= false;
    }else{
        LOGIDCM("JSON Read Success for the MWL Data");
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

    LOGIDCM("Size of US Region is %d Pixel X0 %s , Pixel Y0 %s", mSize,mReferencePixelX0,mReferencePixelY0);

    bool mIsSecondRegion = false;

    if(mSize == UsRegionSize){
        mIsSecondRegion = true;
    }

     const char* inputFilePath=env->GetStringUTFChars(fileName,&isCopy);
     const char* outputFile=env->GetStringUTFChars(filePath,&isCopy);

    jboolean inputCopy=JNI_TRUE;
    jbyte* const i = env->GetByteArrayElements(imgArray, &inputCopy);
    const long size=env->GetArrayLength(imgArray);
    const char* modality="US";
    LOGIDCM("Image byte size is %ld",size);
    /*Removed as PART of THSW:272 comments
     * if ((patientName != nullptr) && ((patientName[0] == '\0') || (patientName[0] == ' '))){
        patientName = "Not Applicable";
    }*/

    const auto mWidth=(Uint16)width;
    const auto mHeight=(Uint16)height;
    LOGIDCM("Image mWidth size is %d",mWidth);
    LOGIDCM("Image mHeight size is %d",mHeight);
    //DcmPixelData dcmPixelData(DCM_PixelData);

    //Uint16 *PixelData=;


    // Main class for controlling conversion
    /*Image2Dcm i2d;
    // Output plugin to use (i.e. SOP class to write)
    I2DOutputPlug *outPlug = new I2DOutputPlugSC();
    // Input plugin to use (i.e. file format to read)
    //I2DImgSource *inputPlug = NULL;
    I2DImgSource *inputPlug = new I2DJpegSource();
    // Group length encoding mode for output DICOM file
    E_GrpLenEncoding grpLengthEnc = EGL_recalcGL;
    // Item and Sequence encoding mode for output DICOM file
    // Item and Sequence encoding mode for output DICOM file
    E_EncodingType lengthEnc = EET_ExplicitLength;
    // Padding mode for output DICOM file
    E_PaddingEncoding padEnc = EPD_noChange;
    // File pad length for output DICOM file
    OFCmdUnsignedInt filepad = 0;
    // Item pad length for output DICOM file
    OFCmdUnsignedInt itempad = 0;
    // Write only pure dataset, i.e. without meta header
    E_FileWriteMode writeMode = EWM_fileformat;
    // Override keys are applied at the very end of the conversion "pipeline"
    OFList<OFString> overrideKeys;
    // The transfer syntax proposed to be written by output plugin
    E_TransferSyntax writeXfer;

    DcmDataset *dataset = nullptr;

    inputPlug->setImageFile(inputFilePath);

    OFCondition result = i2d.convertFirstFrame(inputPlug, outPlug, 1, dataset, writeXfer); // convert(inputPlug, outPlug, dataset, writeXfer);
*/
    // Create a DICOM dataset
    DcmFileFormat fileformat;
    DcmDataset* dataset = fileformat.getDataset();

    string resultData;
    jobject resultObject;

    OFCondition result = *new OFCondition(EC_Normal);
    // Saving output DICOM image
    if(result.good()){

        char uid[100];
        LOGIDCM("Image converted successfully now try to write DICOM file");
        dataset->putAndInsertString(DCM_SOPClassUID,(char*)UID_UltrasoundImageStorage, OFTrue);

        //Study
        OFCondition resultTag = EC_InvalidValue;
        LOGIDCM("StudyInstance is %s",studyInstance);
        resultTag=dataset->putAndInsertString(DCM_StudyInstanceUID, studyInstance,OFTrue);
        if(resultTag.good()){
            LOGIDCM("StudyInstance inserted successfully JPEG");
        }else{
            LOGIDCM("StudyInstance inserted Failed JPEG  %s",resultTag.text());
        }
        //Series
        LOGIDCM("SeriesInstance is %s",seriesInstance);
        resultTag=dataset->putAndInsertString(DCM_SeriesInstanceUID, seriesInstance,OFTrue);
        if(resultTag.good()){
            LOGIDCM("SeriesInstance inserted successfully JPEG");
        }else{
            LOGIDCM("SeriesInstance inserted Failed JPEG %s",resultTag.text());
        }

        dataset->putAndInsertString(DCM_SeriesNumber, seriesNumber);
        dataset->putAndInsertString(DCM_StudyTime, studyTime);
        dataset->putAndInsertString(DCM_AcquisitionDate,acquisitionDate);
        string aquisitionDateTime=string();
        aquisitionDateTime.append(acquisitionTime,6);
        dataset->putAndInsertString(DCM_AcquisitionTime,aquisitionDateTime.c_str());
        //dataset->putAndInsertString(DCM_TransferSyntaxUID,UID_JPEGProcess1TransferSyntax);
        dataset->putAndInsertString(DCM_PatientName,patientName,OFTrue);
        dataset->putAndInsertString(DCM_PatientID,patientId,OFTrue);
        dataset->putAndInsertString(DCM_StudyID,studyId,OFTrue);
        dataset->putAndInsertString(DCM_PatientSex,patientSex,OFTrue);
        dataset->putAndInsertString(DCM_PatientSize,patientHeight,OFTrue);
        dataset->putAndInsertString(DCM_PatientWeight,patientWeight,OFTrue);
        LOGIDCM("Patient Wight JPEG %s Height %s",patientWeight,patientHeight);
        dataset->putAndInsertString(DCM_StudyDate,studyDate,OFTrue);
        dataset->putAndInsertString(DCM_Modality,modality,OFTrue);
        dataset->putAndInsertString(DCM_AccessionNumber,studyAccessNumber,OFTrue);
        dataset->putAndInsertString(DCM_Manufacturer,MANUFACTURE);
        dataset->putAndInsertString(DCM_TimezoneOffsetFromUTC,timeZoneoffSetFromUTC,OFTrue);
        LOGIDCM("InstanceNumber is JPEG %s",instanceNumber);
        dataset->putAndInsertString(DCM_InstanceNumber, instanceNumber);
        dataset->putAndInsertString(DCM_PatientOrientation, patientOrientation);  //anterior  https://dicom.innolitics.com/ciods/us-image/general-image/00200020
        dataset->putAndInsertString(DCM_ImageType, imageType);
        dataset->putAndInsertString(DCM_PatientBirthDate, patientDoB);
        dataset->putAndInsertString(DCM_ReferringPhysicianName, referPhysician);
        dataset->putAndInsertString(DCM_ManufacturerModelName,"KOSMOS");
        dataset->putAndInsertString(DCM_InstitutionName,facilityName);
        dataset->putAndInsertString(DCM_StudyDescription,examPurpose);
        dataset->putAndInsertString(DCM_PatientComments,patientComments);
        dataset->putAndInsertString(DCM_DeviceSerialNumber,deviceSerialNo);
        dataset->putAndInsertString(DCM_SoftwareVersions,softVerNo);

        dataset->putAndInsertString(DCM_ProtocolName,protocolName);
        dataset->putAndInsertString(DCM_EthnicGroup,ethnicGroup);
        dataset->putAndInsertString(DCM_TransducerData,TransducerData);

        dataset->putAndInsertString(DCM_PerformedProcedureStepStartDate,studyDate,OFTrue);
        dataset->putAndInsertString(DCM_PerformedProcedureStepStartTime,studyTime,OFTrue);
        //PerformedProcedureStepDescription
        dataset->putAndInsertString(DCM_PerformedProcedureStepDescription,examPurpose,OFTrue);

        dataset->putAndInsertString(DCM_PhysiciansOfRecord,physicianofRecord,OFTrue);
        dataset->putAndInsertString(DCM_PerformingPhysicianName,performPhysician,OFTrue);
        dataset->putAndInsertString(DCM_OperatorsName,performPhysician,OFTrue);
        dataset->putAndInsertString(DCM_StationName,kosmosStationName,OFTrue);
        dataset->putAndInsertString(DCM_SOPInstanceUID, sopInstantUid, OFTrue);
        if(strlen(institutionDepartmentName) != 0){
            LOGIDCM("institutionDepartmentName is %s",institutionDepartmentName);
            dataset->putAndInsertString(DCM_InstitutionalDepartmentName,institutionDepartmentName,OFTrue);
        }
        if(string(calReportComment).compare("NC") != 0){
            dataset->putAndInsertString(DCM_PatientComments,calReportComment,OFTrue);
        }

        dataset->putAndInsertString(DCM_PhotometricInterpretation, "RGB");
        dataset->putAndInsertString(DCM_PlanarConfiguration, "0");
        // Set the necessary DICOM attributes
        dataset->putAndInsertUint16(DCM_SamplesPerPixel, 3);
        //dataset->putAndInsertString(DCM_PixelSpacing, "1.0\\1.0");
        dataset->putAndInsertString(DCM_BitsAllocated, "8");
        dataset->putAndInsertString(DCM_BitsStored, "8");
        dataset->putAndInsertString(DCM_HighBit, "7");
        dataset->putAndInsertString(DCM_PixelRepresentation, "0"); // Unsigned integer
        dataset->putAndInsertUint16(DCM_Columns,mWidth);
        dataset->putAndInsertUint16(DCM_Rows, mHeight);

        dataset->putAndInsertString(DCM_SeriesDate,studyDate,OFTrue);
        dataset->putAndInsertString(DCM_SeriesTime,studyTime,OFTrue);

        dataset->putAndInsertString(DCM_ContentDate,acquisitionDate,OFTrue);
        dataset->putAndInsertString(DCM_ContentTime,acquisitionTime,OFTrue);

        DcmItem *mSequenceofUltrasoundRegions;

        //Insert DcmItem tags
        if(dataset->findOrCreateSequenceItem(DCM_SequenceOfUltrasoundRegions,mSequenceofUltrasoundRegions).good()){

            LOGIDCM("Creating First US Region");

            auto regionSpatial=(Uint16)std::atoi(mRegionSpatialFormat);
            result=mSequenceofUltrasoundRegions->putAndInsertUint16(DCM_RegionSpatialFormat,regionSpatial);
            if (result.bad()){
                LOGIDCM("Bad DCM_RegionSpatialFormat");
            }

            auto RegionDataType=(Uint16)std::atoi(mRegionDataType);
            result=mSequenceofUltrasoundRegions->putAndInsertUint16(DCM_RegionDataType,RegionDataType);
            if (result.bad()){
                LOGIDCM("Bad DCM_RegionDataType");
            }

            auto RegionFlags=(Uint32)std::atoi(mRegionFlags);
            result=mSequenceofUltrasoundRegions->putAndInsertUint32(DCM_RegionFlags,RegionFlags);
            if (result.bad()){
                LOGIDCM("Bad DCM_RegionFlags");
            }


            result=mSequenceofUltrasoundRegions->putAndInsertUint32(DCM_RegionLocationMinX0,(Uint32)std::atoi(mRegionLocX0));
            if (result.bad()){
                LOGIDCM("Bad DCM_RegionLocationMinX0");
            }

            result=mSequenceofUltrasoundRegions->putAndInsertUint32(DCM_RegionLocationMinY0,(Uint32)std::atoi(mRegionLocY0));
            if (result.bad()){
                LOGIDCM("Bad DCM_RegionLocationMinY0");
            }

            result=mSequenceofUltrasoundRegions->putAndInsertUint32(DCM_RegionLocationMaxX1,(Uint32)std::atoi(mRegionLocX1));
            if (result.bad()){
                LOGIDCM("Bad DCM_RegionLocationMaxX1");
            }

            result=mSequenceofUltrasoundRegions->putAndInsertUint32(DCM_RegionLocationMaxY1,(Uint32)std::atoi(mRegionLocY1));
            if (result.bad()){
                LOGIDCM("Bad DCM_RegionLocationMaxY1");
            }

            result=mSequenceofUltrasoundRegions->putAndInsertUint16(DCM_PhysicalUnitsXDirection,(Uint16)std::strtol(mPhysicalUnitsXDirection,
                                                                                                                    nullptr,10));
            if (result.bad()){
                LOGIDCM("Bad DCM_PhysicalUnitsXDirection");
            }

            result=mSequenceofUltrasoundRegions->putAndInsertUint16(DCM_PhysicalUnitsYDirection,(Uint16)std::strtol(mPhysicalUnitsYDirection,
                                                                                                                    nullptr,10));
            if (result.bad()){
                LOGIDCM("Bad DCM_PhysicalUnitsYDirection");
            }

            result=mSequenceofUltrasoundRegions->putAndInsertFloat64(DCM_PhysicalDeltaX,(Float64)std::strtod(mPhysicalDeltaX,nullptr));
            if (result.bad()){
                LOGIDCM("Bad DCM_PhysicalDeltaX");
            }

            result=mSequenceofUltrasoundRegions->putAndInsertFloat64(DCM_PhysicalDeltaY,(Float64) std::strtod(mPhysicalDeltaY,nullptr));
            if (result.bad()){
                LOGIDCM("Bad DCM_PhysicalDeltaY");
            }

            result=mSequenceofUltrasoundRegions->putAndInsertSint32(DCM_ReferencePixelX0,(Sint32)std::atoi(mReferencePixelX0));
            if(result.bad()){
                LOGIDCM("Bad DCM_ReferencePixelX0");
            }

            result=mSequenceofUltrasoundRegions->putAndInsertSint32(DCM_ReferencePixelY0,(Sint32)std::atoi(mReferencePixelY0));
            if(result.bad()){
                LOGIDCM("Bad DCM_ReferencePixelY0");
            }


            result=mSequenceofUltrasoundRegions->putAndInsertFloat64(DCM_ReferencePixelPhysicalValueX,(Float64)std::strtod(mReferencePixelPhysicalValueX,nullptr));
            if(result.bad()){
                LOGIDCM("Bad DCM_ReferencePixelPhysicalValueX");
            }

            result=mSequenceofUltrasoundRegions->putAndInsertFloat64(DCM_ReferencePixelPhysicalValueY,(Float64)std::strtod(mReferencePixelPhysicalValueY,nullptr));
            if(result.bad()){
                LOGIDCM("Bad DCM_ReferencePixelPhysicalValueY");
            }

        }else{
            LOGIDCM("Cannot add SequenceOfUltrasoundRegions");
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

                LOGIDCM("Creating Second US Region");
                auto regionSpatial=(Uint16)std::atoi(mRegionSpatialFormatSecond);
                result=mSequenceofUltrasoundRegionsSecond->putAndInsertUint16(DCM_RegionSpatialFormat,regionSpatial);
                if (result.bad()){
                    LOGIDCM("Bad DCM_RegionSpatialFormat %s",result.text());
                }

                auto RegionDataType=(Uint16)std::atoi(mRegionDataTypeSecond);
                result=mSequenceofUltrasoundRegionsSecond->putAndInsertUint16(DCM_RegionDataType,RegionDataType);
                if (result.bad()){
                    LOGIDCM("Bad DCM_RegionDataType %s",result.text());
                }

                auto RegionFlags=(Uint32)std::atoi(mRegionFlagsSecond);
                result=mSequenceofUltrasoundRegionsSecond->putAndInsertUint32(DCM_RegionFlags,RegionFlags);
                if (result.bad()){
                    LOGIDCM("Bad DCM_RegionFlags %s ",result.text());
                }

                result=mSequenceofUltrasoundRegionsSecond->putAndInsertUint32(DCM_RegionLocationMinX0,(Uint32)std::atoi(mRegionLocX0Second));
                if (result.bad()){
                    LOGIDCM("Bad DCM_RegionLocationMinX0");
                }

                result=mSequenceofUltrasoundRegionsSecond->putAndInsertUint32(DCM_RegionLocationMinY0,(Uint32)std::atoi(mRegionLocY0Second));
                if (result.bad()){
                    LOGIDCM("Bad DCM_RegionLocationMinY0");
                }

                result=mSequenceofUltrasoundRegionsSecond->putAndInsertUint32(DCM_RegionLocationMaxX1,(Uint32)std::atoi(mRegionLocX1Second));
                if (result.bad()){
                    LOGIDCM("Bad DCM_RegionLocationMaxX1");
                }

                result=mSequenceofUltrasoundRegionsSecond->putAndInsertUint32(DCM_RegionLocationMaxY1,(Uint32)std::atoi(mRegionLocY1Second));
                if (result.bad()){
                    LOGIDCM("Bad DCM_RegionLocationMaxY1");
                }

                result=mSequenceofUltrasoundRegionsSecond->putAndInsertUint16(DCM_PhysicalUnitsXDirection,(Uint16)std::strtol(mPhysicalUnitsXDirectionSecond,
                                                                                                                              nullptr,10));
                if (result.bad()){
                    LOGIDCM("Bad DCM_PhysicalUnitsXDirection");
                }

                result=mSequenceofUltrasoundRegionsSecond->putAndInsertUint16(DCM_PhysicalUnitsYDirection,(Uint16)std::strtol(mPhysicalUnitsYDirectionSecond,
                                                                                                                              nullptr,10));
                if (result.bad()){
                    LOGIDCM("Bad DCM_PhysicalUnitsYDirection");
                }
                LOGIDCM("DopplerArchive Delta X %s, Delta Y %s 1",mPhysicalDeltaXSecond,mPhysicalDeltaYSecond);
                LOGIDCM("DopplerArchive Delta X %f, Delta Y %f 2",(Float64)std::strtod(mPhysicalDeltaXSecond,nullptr),(Float64) std::strtod(mPhysicalDeltaYSecond,nullptr));
                result=mSequenceofUltrasoundRegionsSecond->putAndInsertFloat64(DCM_PhysicalDeltaX,(Float64)std::strtod(mPhysicalDeltaXSecond,nullptr));
                if (result.bad()){
                    LOGIDCM("Bad DCM_PhysicalDeltaX");
                }

                result=mSequenceofUltrasoundRegionsSecond->putAndInsertFloat64(DCM_PhysicalDeltaY,(Float64) std::strtod(mPhysicalDeltaYSecond,nullptr));
                if (result.bad()){
                    LOGIDCM("Bad DCM_PhysicalDeltaY");
                }

                result=mSequenceofUltrasoundRegionsSecond->putAndInsertSint32(DCM_ReferencePixelX0,(Sint32)std::atoi(mReferencePixelX0Second));
                if(result.bad()){
                    LOGIDCM("Bad DCM_ReferencePixelX0");
                }

                result=mSequenceofUltrasoundRegionsSecond->putAndInsertSint32(DCM_ReferencePixelY0,(Sint32)std::atoi(mReferencePixelY0Second));
                if(result.bad()){
                    LOGIDCM("Bad DCM_ReferencePixelY0");
                }


                result=mSequenceofUltrasoundRegionsSecond->putAndInsertFloat64(DCM_ReferencePixelPhysicalValueX,(Float64)std::strtod(mReferencePixelPhysicalValueXSecond,nullptr));
                if(result.bad()){
                    LOGIDCM("Bad DCM_ReferencePixelPhysicalValueX");
                }

                result=mSequenceofUltrasoundRegionsSecond->putAndInsertFloat64(DCM_ReferencePixelPhysicalValueY,(Float64)std::strtod(mReferencePixelPhysicalValueYSecond,nullptr));
                if(result.bad()){
                    LOGIDCM("Bad DCM_ReferencePixelPhysicalValueY");
                }

            }else{
                LOGIDCM("Cannot add SequenceOfUltrasoundRegions");
            }


            //DcmItem *mSequenceofUltrasoundRegions;


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
                LOGIDCM("MWL Encoding DCM_RequestAttributesSequence");
                result=mRequestAttributesSequence->putAndInsertString(DCM_RequestedProcedureDescription, rpDesc.c_str());
                if(result.bad()){
                    LOGIDCM("Bad DCM_RequestedProcedureDescription RequestAttributesSequence");
                }

                result=mRequestAttributesSequence->putAndInsertString(DCM_ScheduledProcedureStepDescription, spStepDesc.c_str());
                if(result.bad()){
                    LOGIDCM("Bad DCM_ScheduledProcedureStepDescription RequestAttributesSequence");
                }

                result=mRequestAttributesSequence->putAndInsertString(DCM_RequestedProcedureID, rpId.c_str());
                if(result.bad()){
                    LOGIDCM("Bad RequestedProcedureID RequestAttributesSequence");
                }

                result=mRequestAttributesSequence->putAndInsertString(DCM_ScheduledProcedureStepID, spStepId.c_str());
                if(result.bad()){
                    LOGIDCM("Bad DCM_ScheduledProcedureStepID RequestAttributesSequence");
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
                            LOGIDCM("Bad DCM_CodeValue ScheduledProtocolCodeSequence");
                        }

                        result=mScheduledProtocolCodeSequence->putAndInsertString(DCM_CodingSchemeDesignator, codingSchemeDesignator.c_str());
                        if(result.bad()){
                            LOGIDCM("Bad DCM_CodingSchemeDesignator ScheduledProtocolCodeSequence");
                        }
                        result=mScheduledProtocolCodeSequence->putAndInsertString(DCM_CodeMeaning, codeMeaning.c_str());
                        if(result.bad()){
                            LOGIDCM("Bad DCM_CodeMeaning ScheduledProtocolCodeSequence");
                        }

                        result = mScheduledProtocolCodeSequence->putAndInsertString(DCM_CodingSchemeVersion,codingSchemeVersion.c_str());
                        if(result.bad()){
                            LOGIDCM("Bad DCM_CodingSchemeVersion ScheduledProtocolCodeSequence");
                        }

                    }else{
                        LOGIDCM("Bad DCM_ScheduledProtocolCodeSequence RequestAttributesSequence DICOM Sub tag failed");
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
                        LOGIDCM("Bad DCM_ReferencedSOPClassUID ReferencedStudySequence");
                    }

                    auto strReferencedSOPInstanceUID = referencedStudySequenceJson[ReferencedSOPInstanceUID].asString();
                    result= mReferencedStudySequence-> putAndInsertString(DCM_ReferencedSOPInstanceUID, strReferencedSOPInstanceUID.c_str());
                    if(result.bad()){
                        LOGIDCM("Bad DCM_ReferencedSOPInstanceUID ReferencedStudySequence");
                    }

                }

            }else{
                LOGIDCM("Cannot add DCM_ReferencedStudySequence");
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

        // JPEG options
        E_TransferSyntax opt_oxfer = EXS_JPEGProcess14SV1;
        OFCmdUnsignedInt opt_selection_value = 6;
        OFCmdUnsignedInt opt_point_transform = 0;
        OFCmdUnsignedInt opt_quality = 90;
        OFBool           opt_huffmanOptimize = OFTrue;
        OFCmdUnsignedInt opt_smoothing = 0;
        int              opt_compressedBits = 0; // 0=auto, 8/12/16=force
        E_CompressionColorSpaceConversion opt_compCSconversion = ECC_lossyYCbCr;
        E_DecompressionColorSpaceConversion opt_decompCSconversion = EDC_photometricInterpretation;
        OFBool           opt_predictor6WorkaroundEnable = OFFalse;
        OFBool           opt_cornellWorkaroundEnable = OFFalse;
        OFBool           opt_forceSingleFragmentPerFrame = OFFalse;
        E_SubSampling    opt_sampleFactors = ESS_422;
        OFBool           opt_useYBR422 = OFTrue;
        OFCmdUnsignedInt opt_fragmentSize = 0; // 0=unlimited
        OFBool           opt_createOffsetTable = OFTrue;
        int              opt_windowType = 0;  /* default: no windowing; 1=Wi, 2=Wl, 3=Wm, 4=Wh, 5=Ww, 6=Wn, 7=Wr */
        OFCmdUnsignedInt opt_windowParameter = 0;
        OFCmdFloat       opt_windowCenter=0.0, opt_windowWidth=0.0;
        E_UIDCreation    opt_uidcreation = EUC_default;
        OFBool           opt_secondarycapture = OFFalse;
        OFCmdUnsignedInt opt_roiLeft = 0, opt_roiTop = 0, opt_roiWidth = 0, opt_roiHeight = 0;
        OFBool           opt_usePixelValues = OFTrue;
        OFBool           opt_useModalityRescale = OFFalse;
        OFBool           opt_trueLossless = OFTrue;
        OFBool           opt_lossless = OFTrue;
        OFBool           lossless = OFTrue;  /* see opt_oxfer */
        OFBool           configOff = OFFalse;
        if(isJpegBaseline){
            opt_oxfer = EXS_JPEGProcess1;
            opt_trueLossless = OFFalse;
        }else{
            opt_oxfer = EXS_JPEGProcess14SV1;
            opt_trueLossless = OFTrue;
        }
        // register global compression codecs
        DJEncoderRegistration::registerCodecs(
                opt_compCSconversion,
                opt_uidcreation,
                opt_huffmanOptimize,
                OFstatic_cast(int, opt_smoothing),
                opt_compressedBits,
                OFstatic_cast(Uint32, opt_fragmentSize),
                opt_createOffsetTable,
                opt_sampleFactors,
                opt_useYBR422,
                opt_secondarycapture,
                OFstatic_cast(Uint32, opt_windowType),
                OFstatic_cast(Uint32, opt_windowParameter),
                opt_windowCenter,
                opt_windowWidth,
                OFstatic_cast(Uint32, opt_roiLeft),
                OFstatic_cast(Uint32, opt_roiTop),
                OFstatic_cast(Uint32, opt_roiWidth),
                OFstatic_cast(Uint32, opt_roiHeight),
                opt_usePixelValues,
                opt_useModalityRescale,
                configOff,
                configOff,
                opt_trueLossless);
        auto bufferSize = mWidth * mHeight*3;
        // Create a buffer for one frame of pixel data
        auto* frameBuffer = new Uint8[bufferSize];
        FILE *FileP = fopen(inputFilePath,"rb");
        LOGIDCM("File Open .rgb");
        fread(frameBuffer,bufferSize,1,FileP);
        LOGIDCM("File Read .rgb");
        fclose(FileP);
        LOGIDCM("File close .rgb");
        OFCondition fileResult;
        fileResult = dataset->putAndInsertString(DCM_NumberOfFrames, "1");
        if(fileResult.good()){
            LOGIDCM("DCM_NumberOfFrames inserted successfully --> %s",fileResult.text());
        }else{
            LOGIDCM("Unable to inserted DCM_NumberOfFrames  Error --> %s",fileResult.text());
        }
        fileResult = dataset->putAndInsertString(DCM_FrameIncrementPointer, "0028,0009"); // Point to the frame increment attribute
        if(fileResult.good()){
            LOGIDCM("DCM_FrameIncrementPointer inserted successfully --> %s",fileResult.text());
        }else{
            LOGIDCM("Unable to inserted DCM_FrameIncrementPointer  Error --> %s",fileResult.text());
        }

        DJ_RPLossy mLossyParams(JPEG_LOSSY_QUALITY_STILL);
        // create representation parameters for lossy and lossless
        DJ_RPLossless rp_lossless((int)opt_selection_value, (int)opt_point_transform);

        const DcmRepresentationParameter * mDcmRef=&mLossyParams;

        if (isJpegBaseline) {
            mDcmRef = &mLossyParams;
        } else {
            mDcmRef = &rp_lossless;
        }

        fileResult = dataset->putAndInsertUint8Array(DCM_PixelData, frameBuffer, bufferSize);
        if(fileResult.good()){
            LOGIDCM("Pixel Data inserted successfully --> %s",fileResult.text());
        }else{
            LOGIDCM("Unable to inserted Pixel Data Error --> %s",fileResult.text());
        }
        // this causes the lossless JPEG version of the dataset to be created
        fileResult = dataset->chooseRepresentation(opt_oxfer, mDcmRef);
        if (fileResult.good() &&
            dataset->canWriteXfer(opt_oxfer))
        {
            dataset->putAndInsertString(DCM_SOPInstanceUID, sopInstantUid, OFTrue);
            // store in lossless JPEG format
            fileResult = fileformat.saveFile(outputFile, opt_oxfer);
        }else{
            LOGIDCM("Unable to create DICOM file Error during Representation --> %s",fileResult.text());
            string errorMessage="Unable to create DICOM file Error is"+string(fileResult.text());
        }

        //It's must be after all DICOM tag encoding complete
        //DcmFileFormat dicomFileFormat(dataset);
        //OFCondition fileResult=dicomFileFormat.saveFile(outputFile,writeXfer,lengthEnc,  grpLengthEnc, padEnc, OFstatic_cast(Uint32, filepad), OFstatic_cast(Uint32, itempad), writeMode);
        DJEncoderRegistration::cleanup();  // deregister JPEG codecs

        if(fileResult.bad()){
            LOGIDCM("Unable to create DICOM file Error is--> %s",fileResult.text());
            string errorMessage="Unable to create DICOM file Error is"+string(fileResult.text());
                resultData.append("Fail");
                resultData.append(fileResult.text());
            //"Fail"+fileResult.text();
            resultObject=getResultObject(env,errorMessage,ERROR);
        }else{
            LOGIDCM("DICOM file created successfully-->");
                resultData="Success";
            resultObject=getResultObject(env,"Success",SUCCESS);
        }
    }else{

        LOGIDCM("Unable to Convert JPEG to  DICOM file Error is--> %s",result.text());
        string errorMessage="Unable to create DICOM file Error is"+string(result.text());
        resultObject=getResultObject(env,errorMessage,ERROR);
    }

    return resultObject;
}
