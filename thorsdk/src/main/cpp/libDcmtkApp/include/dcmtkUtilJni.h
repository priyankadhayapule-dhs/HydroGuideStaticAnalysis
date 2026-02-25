//
// Created by himanshu on 14/6/18.
//

#ifndef DCMTKCLIENT_DCMTK_UTIL_JNI_H
#define DCMTKCLIENT_DCMTK_UTIL_JNI_H


#include <jni.h>
#include <jsoncpp/json/value.h>
#include "dcmtk/dcmdata/dcuid.h"
#include "dcmtk/ofstd/ofstd.h"
#include "dcmtk/ofstd/ofstdinc.h"
#include "dcmtk/dcmdata/dcitem.h"

#define MANUFACTURE  "EchoNous"
#define DCMCodeValue "{SUVlbm}g/ml"
#define DCMCodingSchemeDesignator "UCUM"
#define DCMCodingSchemeVersion "DCM"
#define DCMCodeMeaning "Standardized Uptake Value lean body mass"
#define DCMValueType "TEXT"
#define DCMImageType "DERIVED\\PRIMARY"
#define patientNameIndex 0
#define patientIdIndex 1
#define patientSexIndex 2
#define studyIdIndex 3
#define studyDateIndex 4
#define studyAccessNumberIndex 5
#define timezoneIndex 6
#define patientDoBIndex 7
#define referPhysicianIndex 8
#define patientOrientationIndex 9
#define instanceNumberIndex 10
#define seriesNumberIndex 11
#define studyTimeIndex 12
#define institutionNameIndex 13
#define examPurposeIndex 14
#define healthConditionIndex 15
#define deviceSerialIndex 16
#define softVerIndex 17
#define imageTypeIndex 18
#define acquisitionDateIndex 19
#define acquisitionTimeIndex 20
#define protocolNameIndex 21
#define ethnicGroupIndex 22
#define patientWeightIndex 23
#define patientHeightIndex 24
#define studyInstantUID 25
#define seriesInstantUID 26
#define performPhysicianIndex 27
#define physicianOfRecordIndex 28
#define kosmosStationNameIndex 29
#define mwlJsonDataIndex 30
#define sopInstantUID 31
#define calReportCommentIndex 32
#define patientAlternativeIdIndex 33
#define imageGradeIndex 34
#define institutionDepartmentNameIndex 35
#define DefaultDate "20080303"
#define DefaultTime "083045"
#define TransducerData "TORSO\\\\"

#define RegionSpatialFormatIndex 0
#define RegionDataTypeIndex 1
#define RegionFlagsIndex 2
#define RegionLocationMinX0Index 3
#define RegionLocationMinY0Index 4
#define RegionLocationMaxX1Index 5
#define RegionLocationMaxY1Index 6
#define PhysicalUnitsXDirectionIndex 7
#define PhysicalUnitsYDirectionIndex 8
#define PhysicalDeltaXIndex 9
#define PhysicalDeltaYIndex 10
#define FrameTimeIndex 11
#define ReferencePixelX0Index 12
#define ReferencePixelY0Index 13
#define ReferencePixelPhysicalValueXIndex 14
#define ReferencePixelPhysicalValueYIndex 15
#define UsRegionSize  32
#define IndexPlus 16

#define TypeStudyInstanceUID 0
#define TypeSeriesInstanceUID 1
#define TypeSopInstanceUID 2

#define JPEG_BASELINE 0
#define JPEG_LOSSLESS 1

#define ScheduledProcedureStepSequence "ScheduledProcedureStepSequence"
#define ScheduledProtocolCodeSequence "ScheduledProtocolCodeSequence"
#define CodeValueJson "CodeValue"
#define CodingSchemeDesignatorJson "CodingSchemeDesignator"
#define CodingSchemeVersionJson "CodingSchemeVersion"
#define CodeMeaningJson "CodeMeaning"

#define RequestedProcedureCodeSequence "RequestedProcedureCodeSequence"
#define ScheduledProcedureStepStartTime "ScheduledProcedureStepStartTime"
#define ScheduledProcedureStepStartDate "ScheduledProcedureStepStartDate"
#define ScheduledStationAETitle "ScheduledStationAETitle"
#define ScheduledProcedureStepID "ScheduledProcedureStepID"
#define ScheduledProcedureStepDescription "ScheduledProcedureStepDescription"
#define RequestedProcedureID "RequestedProcedureID"
#define RequestedProcedureDescription "RequestedProcedureDescription"
#define ReferencedStudySequence "ReferencedStudySequence"
#define ReferencedSOPClassUID "ReferencedSOPClassUID"
#define ReferencedSOPInstanceUID "ReferencedSOPInstanceUID"

#define Allergies "Allergies"
#define AdmittingDiagnosesDescription "AdmittingDiagnosesDescription"
#define MedicalAlerts "MedicalAlerts"
#define AdditionalPatientHistory "AdditionalPatientHistory"
#define PregnancyStatus "PregnancyStatus"
#define PatientComments "PatientComments"
#define RequestingPhysician "RequestingPhysician"
#define ReasonForTheRequestedProcedure "ReasonForTheRequestedProcedure"
#define NamesOfIntendedRecipientsOfResults "NamesOfIntendedRecipientsOfResults"
#define InstitutionName "InstitutionName"

#define PRIVATE_CREATOR_NAME "EchoNous, Inc."
#define PRIVATE_ALTERNATIVE_ID_TAG "Alternative Id"
#define PRIVATE_GROUP 0x2021
#define PRIVATE_CREATOR_ELE 0x0010
#define PRIVATE_CREATOR_TAG  PRIVATE_GROUP, PRIVATE_CREATOR_ELE
#define PRIVATE_ELEMENT1_TAG PRIVATE_GROUP, 0x1000
#define PRIVATE_ELEMENT1_TAG_1 PRIVATE_GROUP, 0x1001
#define PRV_PrivateCreator   DcmTag(PRIVATE_CREATOR_TAG)
#define PRV_PrivateElement1  DcmTag(PRIVATE_ELEMENT1_TAG, PRIVATE_CREATOR_NAME)
#define PRV_PrivateElement2  DcmTag(PRIVATE_ELEMENT1_TAG_1, PRIVATE_CREATOR_NAME)

//! Quality settings for lossy JPEGS.
const int JPEG_LOSSY_QUALITY = 95;
//! Quality settings for lossy JPEGS.
const int JPEG_LOSSY_QUALITY_STILL = 100;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


#define DCMTKUTIL_METHOD(METHOD_NAME)\
    Java_com_echonous_kosmosapp_framework_dcmtkclient_DcmtkUtil_##METHOD_NAME // NOLINT

JNIEXPORT jobject JNICALL
DCMTKUTIL_METHOD(createDcm)(JNIEnv* env,jobject thiz,jobjectArray dcmInfo,jstring filePath,jstring fileName,jbyteArray imgArray,
                            jint width,jint height,jobjectArray ultrasoundRegionsInfo, jint jpegCompressionType);


JNIEXPORT jstring JNICALL
DCMTKUTIL_METHOD(createMutiDcm)(JNIEnv* env,jobject thiz,jobjectArray dcmInfo,jstring filePath,
                                jstring fileName,jobjectArray ultrasoundRegionsInfo,jint width,jint height, jint jpegCompressionType);

JNIEXPORT jstring JNICALL
DCMTKUTIL_METHOD(createVideoDcm)(JNIEnv* env,jobject thiz,jobjectArray dcmInfo,jstring filePath,
                                 jstring fileName,jbyteArray imgArray,
                                 jstring fileInput,jint width,jint height,jobjectArray ultrasoundRegionsInfo);

JNIEXPORT jobject JNICALL
DCMTKUTIL_METHOD(createPdfDcm)(JNIEnv* env,jobject thiz,jobjectArray dcmInfo,jstring inputFilePath,jstring outPutFilePath);

JNIEXPORT jstring JNICALL
DCMTKUTIL_METHOD(generateInstanceUID)(JNIEnv* env,jobject thiz,jint instantType);

JNIEXPORT jobject JNICALL
DCMTKUTIL_METHOD(createDicomSr)(JNIEnv *env, jobject thiz, jstring jsonReport,
                               jstring outputFilePath);

JNIEXPORT jobject JNICALL
DCMTKUTIL_METHOD(createDicomSrOb)(JNIEnv *env, jobject thiz, jstring jsonReport,
                               jstring outputFilePath);

JNIEXPORT jobject JNICALL
DCMTKUTIL_METHOD(createDicomSrGyn)(JNIEnv *env, jobject thiz, jstring jsonReport,
                               jstring outputFilePath);

JNIEXPORT jobject JNICALL
DCMTKUTIL_METHOD(createVascularSr)(JNIEnv *env, jobject thiz, jstring jsonReport,
                                   jstring outputFilePath);

JNIEXPORT jobject JNICALL
DCMTKUTIL_METHOD(createAbdomenSr)(JNIEnv *env, jobject thiz, jstring jsonReport,
                                   jstring outputFilePath);

static void createIdentifiers(
        OFBool opt_readSeriesInfo,
        const char *opt_seriesFile,
        OFString& studyUID,
        OFString& seriesUID,
        OFString& patientName,
        OFString& patientID,
        OFString& patientBirthDate,
        OFString& patientSex,
        Sint32& incrementedInstance);

static OFCondition insertPDFFile(DcmItem *dataset,const char *filename);
static OFCondition createHeader(
        DcmItem *dataset,
        const char *opt_patientName,
        const char *opt_patientID,
        const char *opt_patientBirthdate,
        const char *opt_patientSex,
        OFBool opt_burnedInAnnotation,
        const char *opt_studyUID,
        const char *opt_seriesUID,
        const char *opt_documentTitle,
        const char *opt_conceptCSD,
        const char *opt_conceptCV,
        const char *opt_conceptCM,
        Sint32 opt_instanceNumber,
        const char *timeOffset,
        const char *accessionNumber,
        const char *studyDate,
        const char *physicianName,
        const char *studyinstantUID,
        const char *seriesinstantUID,
        const char *sopInstanceUID);

static const char * getReferenceInstanceExamUID(const char * studyId);
static const char *createSR(JNIEnv *env,jstring outfilePath);
static jobject createUS2Sr(JNIEnv *env,jstring outfilePath,jstring jsonReport);
static jobject createObSr(JNIEnv *env,jstring outfilePath,jstring jsonReport);
static jobject createVascularSrDocument(JNIEnv *env,jstring outfilePath,jstring jsonReport);

static jobject createAbdomenSrDocument(JNIEnv *env,jstring outfilePath,jstring jsonReport);

static void fetchCoordinate(Json::Value coordinates);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif //
