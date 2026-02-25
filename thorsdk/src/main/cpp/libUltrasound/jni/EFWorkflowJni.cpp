//
// Copyright 2019 EchoNous Inc.
//
//  It is assumed that all parameters passed to the JNI functions exist and
//  are valid.  The parameters should be verified by the upper layer
//  Ultrasound Manager (in Java space).
//

#define LOG_TAG "EFWorkflowJni"

//#define MOCK_SMARTCAPTURE 0

#include <algorithm>
#include <thread>
#include <chrono>
#include <future>
#include <ThorUtils.h>
#include <jni.h>
/* Implementation for class com_echonous_ai_EFWorkflow */

#include <unordered_map>
#include <EFWorkflow.h>
#include <UltrasoundManager.h>
#include <EFQualitySubviewTask.h>
#include <CardiacAnnotatorTask.h>

namespace echonous
{
    extern UltrasoundManager gUltrasoundManager;
}

// temporary global storage for user data
static jobject gUserFramesA4C = NULL, gUserFramesA2C = NULL;
static std::unordered_map<jint, jobject> gUserSegmentationsA4C, gUserSegmentationsA2C;

// Global Java VM
static JavaVM *gVM = nullptr;

// Cached jclass instances (for use from native thread)
static jclass gJCardiacView = nullptr;
static jclass gJCardiacSegmentation = nullptr;
static jclass gJCardiacFrames = nullptr;
static jclass gJEFStatistics = nullptr;
static jclass gJAutoDopplerTarget = nullptr;

// Utility methods to convert to and from java <-> c++
static CardiacView JtoCView(JNIEnv *jenv, jobject view)
{
    LOGD("JtoCView() ENTRY - jenv=%p, view=%p", jenv, view);
    jfieldID A4CField = jenv->GetStaticFieldID(gJCardiacView, "A4C", "Lcom/echonous/ai/CardiacView;");
    jfieldID A2CField = jenv->GetStaticFieldID(gJCardiacView, "A2C", "Lcom/echonous/ai/CardiacView;");
    jfieldID A5CField = jenv->GetStaticFieldID(gJCardiacView, "A5C", "Lcom/echonous/ai/CardiacView;");
    jfieldID PLAXField = jenv->GetStaticFieldID(gJCardiacView, "PLAX", "Lcom/echonous/ai/CardiacView;");
    jobject enumA4C = jenv->GetStaticObjectField(gJCardiacView, A4CField);
    jobject enumA2C = jenv->GetStaticObjectField(gJCardiacView, A2CField);
    jobject enumA5C = jenv->GetStaticObjectField(gJCardiacView, A5CField);
    jobject enumPLAX = jenv->GetStaticObjectField(gJCardiacView, PLAXField);

    if (jenv->IsSameObject(enumA4C, view)) {
        LOGD("JtoCView() EXIT - returning A4C");
        return CardiacView::A4C;
    }
    else if (jenv->IsSameObject(enumA2C, view)) {
        LOGD("JtoCView() EXIT - returning A2C");
        return CardiacView::A2C;
    }
    else if (jenv->IsSameObject(enumA5C, view)) {
        LOGD("JtoCView() EXIT - returning A5C");
        return CardiacView::A5C;
    }
    else if (jenv->IsSameObject(enumPLAX, view)) {
        LOGD("JtoCView() EXIT - returning PLAX");
        return CardiacView::PLAX;
    }
    else {
        LOGE("Unknown Java CardiacView type, expected A4C, A2C, A5C, or PLAX");
        LOGD("JtoCView() EXIT - returning A4C (default)");
        return CardiacView::A4C;
    }
}

static CardiacFrames JtoCFrames(JNIEnv *jenv, jobject frames)
{
    LOGD("JtoCFrames() ENTRY - jenv=%p, frames=%p", jenv, frames);
    CardiacFrames result;
    //jclass JCardiacFrames = jenv->FindClass("com/echonous/ai/CardiacFrames");
    jfieldID esField = jenv->GetFieldID(gJCardiacFrames, "esFrame", "I");
    result.esFrame = jenv->GetIntField(frames, esField);

    jfieldID edField = jenv->GetFieldID(gJCardiacFrames, "edFrame", "I");
    result.edFrame = jenv->GetIntField(frames, edField);
    LOGD("JtoCFrames() EXIT - esFrame=%d, edFrame=%d", result.esFrame, result.edFrame);
    return result;
}

static CardiacSegmentation JtoCSegmentation(JNIEnv *jenv, jobject segmentation)
{
    LOGD("JtoCSegmentation() ENTRY - jenv=%p, segmentation=%p", jenv, segmentation);
    CardiacSegmentation result;
    //jclass JCardiacSegmentation = jenv->FindClass("com/echonous/ai/CardiacSegmentation");
    jfieldID splinePointsField = jenv->GetFieldID(gJCardiacSegmentation, "splinePoints", "[F");

    jfloatArray pointsArray = static_cast<jfloatArray>(jenv->GetObjectField(segmentation, splinePointsField));
    jsize len = jenv->GetArrayLength(pointsArray);

    float *data = jenv->GetFloatArrayElements(pointsArray, NULL);
    // ignoring last point, which is a duplicate of the first
    for (jsize i=0; i < len-3; i += 2)
    {
        result.coords.emplace_back(data[i], data[i+1]);
    }
    // Set uncertainty to 0, since Java Segmentation object has no uncertainty
    result.uncertainty = 0.0f;
    jenv->ReleaseFloatArrayElements(pointsArray, data, JNI_ABORT); // abort means free the pointer but do not copy back any changes
    // Set uncertainty to 0, since Java Segmentation object has no uncertainty
    result.uncertainty = 0.0f;
    LOGD("JtoCSegmentation() EXIT - numPoints=%zu, uncertainty=%f", result.coords.size(), result.uncertainty);
    return result;
}
static AutoDopplerTarget JtoCAutoDopplerTarget(JNIEnv *jenv, jobject target)
{
    LOGD("JtoCAutoDopplerTarget() ENTRY - jenv=%p, target=%p", jenv, target);
#define COMPARE_ENUM(name) do { \
jfieldID enumField = jenv->GetStaticFieldID(gJAutoDopplerTarget, #name, "Lcom/echonous/ai/EFWorkflow$AutoDopplerTarget;"); \
jobject  enumObject = jenv->GetStaticObjectField(gJAutoDopplerTarget, enumField);                                        \
if (jenv->IsSameObject(enumObject, target)) { LOGD("JtoCAutoDopplerTarget() EXIT - returning %s", #name); return AutoDopplerTarget::name; }                                \
} while (0)

    COMPARE_ENUM(PLAX_RVOT_PV);
    COMPARE_ENUM(PLAX_AV_PV);
    COMPARE_ENUM(A4C_MV);
    COMPARE_ENUM(A4C_TV);
    COMPARE_ENUM(A5C_LVOT);
    COMPARE_ENUM(TDI_A4C_MVSA);
    COMPARE_ENUM(TDI_A4C_MVLA);
    COMPARE_ENUM(TDI_A4C_TVLA);
    LOGE("Bad enum value");
    LOGD("JtoCAutoDopplerTarget() EXIT - returning None");
    return AutoDopplerTarget::None;
}

static jobject CtoJView(JNIEnv *jenv, CardiacView view)
{
    LOGD("CtoJView() ENTRY - jenv=%p, view=%d", jenv, (int)view);
    //jclass JCardiacView = jenv->FindClass("com/echonous/ai/CardiacView");
    jfieldID field;
    switch (view) {
    case CardiacView::A4C:
        field = jenv->GetStaticFieldID(gJCardiacView, "A4C", "Lcom/echonous/ai/CardiacView;");
        break;
    case CardiacView::A2C:
        field = jenv->GetStaticFieldID(gJCardiacView, "A2C", "Lcom/echonous/ai/CardiacView;");
        break;
    case CardiacView::A5C:
        field = jenv->GetStaticFieldID(gJCardiacView, "A5C", "Lcom/echonous/ai/CardiacView;");
        break;
    case CardiacView::PLAX:
        field = jenv->GetStaticFieldID(gJCardiacView, "PLAX", "Lcom/echonous/ai/CardiacView;");
        break;
    default:
        LOGE("Unknown CardiacView value: %d", (int)view);
        field = jenv->GetStaticFieldID(gJCardiacView, "A4C", "Lcom/echonous/ai/CardiacView;");
    }
    jobject result = jenv->GetStaticObjectField(gJCardiacView, field);
    LOGD("CtoJView() EXIT - returning %p", result);
    return result;
}

static jobject CtoJFrames(JNIEnv *jenv, CardiacFrames frames)
{
    LOGD("CtoJFrames() ENTRY - jenv=%p, esFrame=%d, edFrame=%d", jenv, frames.esFrame, frames.edFrame);
    //jclass JCardiacFrames = jenv->FindClass("com/echonous/ai/CardiacFrames");
    jmethodID ctor = jenv->GetMethodID(gJCardiacFrames, "<init>", "(II)V");
    jobject jframes = jenv->NewObject(gJCardiacFrames, ctor, frames.edFrame, frames.esFrame);

    LOGD("CtoJFrames() EXIT - returning %p", jframes);
    return jframes;
}

static jobject CtoJSegmentation(JNIEnv *jenv, CardiacSegmentation segmentation)
{
    LOGD("CtoJSegmentation() ENTRY - jenv=%p, numCoords=%zu", jenv, segmentation.coords.size());
    //jclass JCardiacSegmentation = jenv->FindClass("com/echonous/ai/CardiacSegmentation");

    std::vector<float> points;
    for (const auto &p : segmentation.coords)
    {
        points.push_back(p.x);
        points.push_back(p.y);
    }
    // add base point to end
    points.push_back(segmentation.coords.front().x);
    points.push_back(segmentation.coords.front().y);
    jfloatArray jpoints = jenv->NewFloatArray(points.size());
    jenv->SetFloatArrayRegion(jpoints, 0, points.size(), &points.front());

    jmethodID ctor = jenv->GetMethodID(gJCardiacSegmentation, "<init>", "([F)V");
    jobject result = jenv->NewObject(gJCardiacSegmentation, ctor, jpoints);
    LOGD("CtoJSegmentation() EXIT - returning %p", result);
    return result;
}

static jobject CtoJStatistics(JNIEnv *jenv, EFStatistics stats)
{
    LOGD("CtoJStatistics() ENTRY - jenv=%p", jenv);
#define SET_JAVA_FIELD_FLOAT(env, cls, obj, name, value) do { jfieldID field = env->GetFieldID(cls, name, "F"); env->SetFloatField(obj, field, value); }while(0)

    //jclass JEFStatistics = jenv->FindClass("com/echonous/ai/EFStatistics");
    jmethodID ctor = jenv->GetMethodID(gJEFStatistics, "<init>", "()V");
    jobject jstats = jenv->NewObject(gJEFStatistics, ctor);

    SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "heartRate", stats.heartRate);

    SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "a4cUncertainty", stats.originalA4cStats.uncertainty);
    SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "a2cUncertainty", stats.originalA2cStats.uncertainty);
    SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "biplaneUncertainty", stats.originalBiplaneStats.uncertainty);

    SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "originalA4CEDVolume", stats.originalA4cStats.edVolume);
    SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "originalA4CESVolume", stats.originalA4cStats.esVolume);
    SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "originalA4CEF", stats.originalA4cStats.EF);
    SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "originalA4CStrokeVolume", stats.originalA4cStats.SV);
    SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "originalA4CCardiacOutput", stats.originalA4cStats.CO);

    SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "originalA2CEDVolume", stats.originalA2cStats.edVolume);
    SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "originalA2CESVolume", stats.originalA2cStats.esVolume);
    SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "originalA2CEF", stats.originalA2cStats.EF);
    SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "originalA2CStrokeVolume", stats.originalA2cStats.SV);
    SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "originalA2CCardiacOutput", stats.originalA2cStats.CO);

    SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "originalBiplaneEDVolume", stats.originalBiplaneStats.edVolume);
    SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "originalBiplaneESVolume", stats.originalBiplaneStats.esVolume);
    SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "originalBiplaneEF", stats.originalBiplaneStats.EF);
    SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "originalBiplaneStrokeVolume", stats.originalBiplaneStats.SV);
    SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "originalBiplaneCardiacOutput", stats.originalBiplaneStats.CO);

    // Legacy values
    if (stats.originalBiplaneStats.edVolume > 0)
    {
        SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "originalEDVolume", stats.originalBiplaneStats.edVolume);
        SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "originalESVolume", stats.originalBiplaneStats.esVolume);
        SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "originalEF", stats.originalBiplaneStats.EF);
        SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "originalStrokeVolume", stats.originalBiplaneStats.SV);
        SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "originalCardiacOutput", stats.originalBiplaneStats.CO);
    }
    else
    {
        SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "originalEDVolume", stats.originalA4cStats.edVolume);
        SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "originalESVolume", stats.originalA4cStats.esVolume);
        SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "originalEF", stats.originalA4cStats.EF);
        SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "originalStrokeVolume", stats.originalA4cStats.SV);
        SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "originalCardiacOutput", stats.originalA4cStats.CO);
    }

    do { jfieldID field = jenv->GetFieldID(gJEFStatistics, "hasEdited", "Z"); jenv->SetBooleanField(jstats, field, stats.hasEdited); }while(0);

    SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "editedA4CEDVolume", stats.editedA4cStats.edVolume);
    SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "editedA4CESVolume", stats.editedA4cStats.esVolume);
    SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "editedA4CEF", stats.editedA4cStats.EF);
    SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "editedA4CStrokeVolume", stats.editedA4cStats.SV);
    SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "editedA4CCardiacOutput", stats.editedA4cStats.CO);

    SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "editedA2CEDVolume", stats.editedA2cStats.edVolume);
    SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "editedA2CESVolume", stats.editedA2cStats.esVolume);
    SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "editedA2CEF", stats.editedA2cStats.EF);
    SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "editedA2CStrokeVolume", stats.editedA2cStats.SV);
    SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "editedA2CCardiacOutput", stats.editedA2cStats.CO);

    SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "editedBiplaneEDVolume", stats.editedBiplaneStats.edVolume);
    SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "editedBiplaneESVolume", stats.editedBiplaneStats.esVolume);
    SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "editedBiplaneEF", stats.editedBiplaneStats.EF);
    SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "editedBiplaneStrokeVolume", stats.editedBiplaneStats.SV);
    SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "editedBiplaneCardiacOutput", stats.editedBiplaneStats.CO);

    // Legacy values
    if (stats.editedBiplaneStats.edVolume > 0)
    {
        SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "editedEDVolume", stats.editedBiplaneStats.edVolume);
        SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "editedESVolume", stats.editedBiplaneStats.esVolume);
        SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "editedEF", stats.editedBiplaneStats.EF);
        SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "editedStrokeVolume", stats.editedBiplaneStats.SV);
        SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "editedCardiacOutput", stats.editedBiplaneStats.CO);
    }
    else
    {
        SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "editedEDVolume", stats.editedA4cStats.edVolume);
        SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "editedESVolume", stats.editedA4cStats.esVolume);
        SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "editedEF", stats.editedA4cStats.EF);
        SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "editedStrokeVolume", stats.editedA4cStats.SV);
        SET_JAVA_FIELD_FLOAT(jenv, gJEFStatistics, jstats, "editedCardiacOutput", stats.editedA4cStats.CO);
    }


    do { jfieldID field = jenv->GetFieldID(gJEFStatistics, "isA4CForeshortened", "Z"); jenv->SetBooleanField(jstats, field, stats.foreshortenedStatus == ForeshortenedStatus::A4C_Foreshortened); }while(0);
    do { jfieldID field = jenv->GetFieldID(gJEFStatistics, "isA2CForeshortened", "Z"); jenv->SetBooleanField(jstats, field, stats.foreshortenedStatus == ForeshortenedStatus::A2C_Foreshortened); }while(0);

    LOGD("CtoJStatistics() EXIT - returning %p, heartRate=%f, hasEdited=%d", jstats, stats.heartRate, stats.hasEdited);
    return jstats;
#undef SET_JAVA_FIELD_FLOAT
}

// Using a global EFWorkflow instance currently, which has global callbacks
static void CallbackFrameIDComplete(void *user, EFWorkflow *workflow, CardiacView view)
{
    LOGD("CallbackFrameIDComplete() ENTRY - user=%p, workflow=%p, view=%d", user, workflow, (int)view);
    JNIEnv *jenv = nullptr;
    if (!gVM)
    {
        LOGE("Global Java VM is null in AI callback, this should not happen.");
        return;
    }
    // Attach current thread OR get current threads JNIEnv if already attached    
    gVM->AttachCurrentThreadAsDaemon(&jenv, nullptr);

    // Since this function is not directly called from Java, we should allocate a local reference frame
    jenv->PushLocalFrame(16);
    jweak jworkflowWeak = static_cast<jweak>(user);
    jobject jworkflow = jenv->NewLocalRef(jworkflowWeak);
    if (!jworkflow)
    {
        // This isn't strictly an error, but it may be indicative on a problem
        LOGW("Callback triggered, but Java EFWorkflow object has been deleted.");
        return;
    }

    jclass JEFWorkflow = jenv->GetObjectClass(jworkflow);
    jmethodID postFrameIDMethod = jenv->GetMethodID(JEFWorkflow, "postFrameID", "(Lcom/echonous/ai/CardiacView;)V");
    jobject jview = CtoJView(jenv, view);
    LOGD("CallbackFrameIDComplete() calling postFrameID on Java side");
    jenv->CallVoidMethod(jworkflow, postFrameIDMethod, jview);

    // Delete global weak reference, and local reference frame
    jenv->PopLocalFrame(nullptr);
    jenv->DeleteWeakGlobalRef(jworkflowWeak);
    LOGD("CallbackFrameIDComplete() EXIT");
}
static void CallbackSegmentationComplete(void *user, EFWorkflow *workflow, CardiacView view, int frame)
{
    LOGD("CallbackSegmentationComplete() ENTRY - user=%p, workflow=%p, view=%d, frame=%d", user, workflow, (int)view, frame);
    JNIEnv *jenv = nullptr;
    if (!gVM)
    {
        LOGE("Global Java VM is null in AI callback, this should not happen.");
        return;
    }
    // Attach current thread OR get current threads JNIEnv if already attached    
    gVM->AttachCurrentThreadAsDaemon(&jenv, nullptr);

    // Since this function is not directly called from Java, we should allocate a local reference frame
    jenv->PushLocalFrame(16);
    jweak jworkflowWeak = static_cast<jweak>(user);
    jobject jworkflow = jenv->NewLocalRef(jworkflowWeak);
    if (!jworkflow)
    {
        // This isn't strictly an error, but it may be indicative on a problem
        LOGW("Callback triggered, but Java EFWorkflow object has been deleted.");
        return;
    }

    jclass JEFWorkflow = jenv->GetObjectClass(jworkflow);
    jmethodID postSegmentationMethod = jenv->GetMethodID(JEFWorkflow, "postSegmentation", "(Lcom/echonous/ai/CardiacView;I)V");
    jobject jview = CtoJView(jenv, view);
    LOGD("CallbackSegmentationComplete() calling postSegmentation on Java side");
    jenv->CallVoidMethod(jworkflow, postSegmentationMethod, jview, frame);

    // Delete global weak reference, and local reference frame
    jenv->PopLocalFrame(nullptr);
    jenv->DeleteWeakGlobalRef(jworkflowWeak);
    LOGD("CallbackSegmentationComplete() EXIT");
}
static void CallbackStatisticsComplete(void *user, EFWorkflow *workflow)
{
    LOGD("CallbackStatisticsComplete() ENTRY - user=%p, workflow=%p", user, workflow);
    JNIEnv *jenv = nullptr;
    if (!gVM)
    {
        LOGE("Global Java VM is null in AI callback, this should not happen.");
        return;
    }
    // Attach current thread OR get current threads JNIEnv if already attached    
    gVM->AttachCurrentThreadAsDaemon(&jenv, nullptr);

    // Since this function is not directly called from Java, we should allocate a local reference frame
    jenv->PushLocalFrame(16);
    jweak jworkflowWeak = static_cast<jweak>(user);
    jobject jworkflow = jenv->NewLocalRef(jworkflowWeak);
    if (!jworkflow)
    {
        // This isn't strictly an error, but it may be indicative on a problem
        LOGW("Callback triggered, but Java EFWorkflow object has been deleted.");
        return;
    }

    jclass JEFWorkflow = jenv->GetObjectClass(jworkflow);
    jmethodID postStatsMethod = jenv->GetMethodID(JEFWorkflow, "postStatistics", "()V");
    LOGD("CallbackStatisticsComplete() calling postStatistics on Java side");
    jenv->CallVoidMethod(jworkflow, postStatsMethod);

    // Delete global weak reference, and local reference frame
    jenv->PopLocalFrame(nullptr);
    jenv->DeleteWeakGlobalRef(jworkflowWeak);
    LOGD("CallbackStatisticsComplete() EXIT");
}

static void CallbackOnError(void *user, EFWorkflow *workflow, ThorStatus error)
{
    LOGD("CallbackOnError() ENTRY - user=%p, workflow=%p, error=%d", user, workflow, (int)error);
    JNIEnv *jenv = nullptr;
    if (!gVM)
    {
        LOGE("Global Java VM is null in AI callback, this should not happen.");
        return;
    }
    // Attach current thread OR get current threads JNIEnv if already attached    
    gVM->AttachCurrentThreadAsDaemon(&jenv, nullptr);

    // Since this function is not directly called from Java, we should allocate a local reference frame
    jenv->PushLocalFrame(16);
    jweak jworkflowWeak = static_cast<jweak>(user);
    jobject jworkflow = jenv->NewLocalRef(jworkflowWeak);
    if (!jworkflow)
    {
        // This isn't strictly an error, but it may be indicative on a problem
        LOGW("Callback triggered, but Java EFWorkflow object has been deleted.");
        return;
    }

    jclass JEFWorkflow = jenv->GetObjectClass(jworkflow);
    jmethodID postErrorMethod = jenv->GetMethodID(JEFWorkflow, "postError", "(I)V");
    LOGD("CallbackOnError() calling postError on Java side with error=%d", (int)error);
    jenv->CallVoidMethod(jworkflow, postErrorMethod, static_cast<int>(error));

    // Delete global weak reference, and local reference frame
    jenv->PopLocalFrame(nullptr);
    jenv->DeleteWeakGlobalRef(jworkflowWeak);
    LOGD("CallbackOnError() EXIT");
}

static void CallbackOnAutocaptureStart(void *user, EFWorkflow *workflow)
{
    LOGD("CallbackOnAutocaptureStart() ENTRY - user=%p, workflow=%p", user, workflow);
    JNIEnv *jenv = nullptr;
    if (!gVM)
    {
        LOGE("Global Java VM is null in AI callback, this should not happen.");
        return;
    }
    // Attach current thread OR get current threads JNIEnv if already attached
    gVM->AttachCurrentThreadAsDaemon(&jenv, nullptr);

    // Since this function is not directly called from Java, we should allocate a local reference frame
    jenv->PushLocalFrame(16);
    jweak jworkflowWeak = static_cast<jweak>(user);
    jobject jworkflow = jenv->NewLocalRef(jworkflowWeak);
    if (!jworkflow)
    {
        // This isn't strictly an error, but it may be indicative on a problem
        LOGW("Callback triggered, but Java EFWorkflow object has been deleted.");
        return;
    }

    jclass JEFWorkflow = jenv->GetObjectClass(jworkflow);
    jmethodID postAutocaptureMethod = jenv->GetMethodID(JEFWorkflow, "postAutocaptureStart", "()V");
    LOGD("CallbackOnAutocaptureStart() calling postAutocaptureStart on Java side");
    jenv->CallVoidMethod(jworkflow, postAutocaptureMethod);

    // Delete local reference frame (but not weak reference)
    jenv->PopLocalFrame(nullptr);
    LOGD("CallbackOnAutocaptureStart() EXIT");
}
static void CallbackOnAutocaptureFail(void *user, EFWorkflow *workflow)
{
    LOGD("CallbackOnAutocaptureFail() ENTRY - user=%p, workflow=%p", user, workflow);
    JNIEnv *jenv = nullptr;
    if (!gVM)
    {
        LOGE("Global Java VM is null in AI callback, this should not happen.");
        return;
    }
    // Attach current thread OR get current threads JNIEnv if already attached
    gVM->AttachCurrentThreadAsDaemon(&jenv, nullptr);

    // Since this function is not directly called from Java, we should allocate a local reference frame
    jenv->PushLocalFrame(16);
    jweak jworkflowWeak = static_cast<jweak>(user);
    jobject jworkflow = jenv->NewLocalRef(jworkflowWeak);
    if (!jworkflow)
    {
        // This isn't strictly an error, but it may be indicative on a problem
        LOGW("Callback triggered, but Java EFWorkflow object has been deleted.");
        return;
    }

    jclass JEFWorkflow = jenv->GetObjectClass(jworkflow);
    jmethodID postAutocaptureMethod = jenv->GetMethodID(JEFWorkflow, "postAutocaptureFail", "()V");
    LOGD("CallbackOnAutocaptureFail() calling postAutocaptureFail on Java side");
    jenv->CallVoidMethod(jworkflow, postAutocaptureMethod);

    // Delete local reference frame (but not weak reference)
    jenv->PopLocalFrame(nullptr);
    LOGD("CallbackOnAutocaptureFail() EXIT");
}
static void CallbackOnAutocaptureSucceed(void *user, EFWorkflow *workflow)
{
    LOGD("CallbackOnAutocaptureSucceed() ENTRY - user=%p, workflow=%p", user, workflow);
    JNIEnv *jenv = nullptr;
    if (!gVM)
    {
        LOGE("Global Java VM is null in AI callback, this should not happen.");
        return;
    }
    // Attach current thread OR get current threads JNIEnv if already attached
    gVM->AttachCurrentThreadAsDaemon(&jenv, nullptr);

    // Since this function is not directly called from Java, we should allocate a local reference frame
    jenv->PushLocalFrame(16);
    jweak jworkflowWeak = static_cast<jweak>(user);
    jobject jworkflow = jenv->NewLocalRef(jworkflowWeak);
    if (!jworkflow)
    {
        // This isn't strictly an error, but it may be indicative on a problem
        LOGW("Callback triggered, but Java EFWorkflow object has been deleted.");
        return;
    }

    jclass JEFWorkflow = jenv->GetObjectClass(jworkflow);
    jmethodID postAutocaptureMethod = jenv->GetMethodID(JEFWorkflow, "postAutocaptureSucceed", "()V");
    LOGD("CallbackOnAutocaptureSucceed() calling postAutocaptureSucceed on Java side");
    jenv->CallVoidMethod(jworkflow, postAutocaptureMethod);

    // Delete local reference frame (but not weak reference)
    jenv->PopLocalFrame(nullptr);
    LOGD("CallbackOnAutocaptureSucceed() EXIT");
}

static void CallbackOnAutocaptureProgress(void *user, EFWorkflow *workflow, float progress) {
    LOGD("CallbackOnAutocaptureProgress() ENTRY - user=%p, workflow=%p, progress=%f", user, workflow, progress);
    JNIEnv *jenv = nullptr;
    if (!gVM)
    {
        LOGE("Global Java VM is null in AI callback, this should not happen.");
        return;
    }
    // Attach current thread OR get current threads JNIEnv if already attached
    gVM->AttachCurrentThreadAsDaemon(&jenv, nullptr);

    // Since this function is not directly called from Java, we should allocate a local reference frame
    jenv->PushLocalFrame(16);
    jweak jworkflowWeak = static_cast<jweak>(user);
    jobject jworkflow = jenv->NewLocalRef(jworkflowWeak);
    if (!jworkflow)
    {
        // This isn't strictly an error, but it may be indicative on a problem
        LOGW("Callback triggered, but Java EFWorkflow object has been deleted.");
        return;
    }

    jclass JEFWorkflow = jenv->GetObjectClass(jworkflow);
    jmethodID postAutocaptureProgressMethod = jenv->GetMethodID(JEFWorkflow, "postAutocaptureProgress", "(F)V");
    LOGD("CallbackOnAutocaptureProgress() calling postAutocaptureProgress on Java side");
    jenv->CallVoidMethod(jworkflow, postAutocaptureProgressMethod, progress);

    // Delete local reference frame (but not weak reference)
    jenv->PopLocalFrame(nullptr);
    LOGD("CallbackOnAutocaptureProgress() EXIT");
}

static void CallbackOnAutoDopplerUpdate(void *user, float x, float y) {
    LOGD("CallbackOnAutoDopplerUpdate() ENTRY - user=%p, x=%f, y=%f", user, x, y);
    JNIEnv *jenv = nullptr;
    if (!gVM)
    {
        LOGE("Global Java VM is null in AI callback, this should not happen.");
        return;
    }
    // Attach current thread OR get current threads JNIEnv if already attached
    gVM->AttachCurrentThreadAsDaemon(&jenv, nullptr);

    // Since this function is not directly called from Java, we should allocate a local reference frame
    jenv->PushLocalFrame(16);
    jweak jworkflowWeak = static_cast<jweak>(user);
    jobject jworkflow = jenv->NewLocalRef(jworkflowWeak);
    if (!jworkflow)
    {
        // This isn't strictly an error, but it may be indicative on a problem
        LOGW("Callback triggered, but Java EFWorkflow object has been deleted.");
        return;
    }

    jclass JEFWorkflow = jenv->GetObjectClass(jworkflow);
    jmethodID postAutoDopplerUpdateGatePositionMethod = jenv->GetMethodID(JEFWorkflow, "postAutoDopplerUpdateGatePosition", "(FF)V");
    LOGD("CallbackOnAutoDopplerUpdate() calling postAutoDopplerUpdateGatePosition on Java side");
    jenv->CallVoidMethod(jworkflow, postAutoDopplerUpdateGatePositionMethod, x, y);

    // Delete local reference frame (but not weak reference)
    jenv->PopLocalFrame(nullptr);
    LOGD("CallbackOnAutoDopplerUpdate() EXIT");
}

static void CallbackOnSmartCaptureProgress(void *user, EFWorkflow *workflow, float progress) {
    LOGD("CallbackOnSmartCaptureProgress() ENTRY - user=%p, workflow=%p, progress=%f", user, workflow, progress);
    JNIEnv *jenv = nullptr;
    if (!gVM)
    {
        LOGE("Global Java VM is null in AI callback, this should not happen.");
        return;
    }
    // Attach current thread OR get current threads JNIEnv if already attached
    gVM->AttachCurrentThreadAsDaemon(&jenv, nullptr);
    // Since this function is not directly called from Java, we should allocate a local reference frame
    jenv->PushLocalFrame(16);
    jweak jworkflowWeak = static_cast<jweak>(user);
    jobject jworkflow = jenv->NewLocalRef(jworkflowWeak);
    if (!jworkflow)
    {
        // This isn't strictly an error, but it may be indicative on a problem
        LOGW("Callback triggered, but Java EFWorkflow object has been deleted.");
        return;
    }
    jclass JEFWorkflow = jenv->GetObjectClass(jworkflow);
    jmethodID postSmartCaptureProgressMethod = jenv->GetMethodID(JEFWorkflow, "postSmartCaptureProgress", "(F)V");
    LOGD("CallbackOnSmartCaptureProgress() calling postSmartCaptureProgress on Java side");
    jenv->CallVoidMethod(jworkflow, postSmartCaptureProgressMethod, progress);
    // Delete local reference frame (but not weak reference)
    jenv->PopLocalFrame(nullptr);
    LOGD("CallbackOnSmartCaptureProgress() EXIT");
}

// Get the EF Forced error option
int GetEFForcedError(void *user)
{
    LOGD("GetEFForcedError() ENTRY - user=%p", user);
    JNIEnv *jenv = nullptr;
    if (!gVM)
    {
        LOGE("Global Java VM is null in AI callback, this should not happen.");
        return 0;
    }
    // Attach current thread OR get current threads JNIEnv if already attached
    gVM->AttachCurrentThreadAsDaemon(&jenv, nullptr);

    // Since this function is not directly called from Java, we should allocate a local reference frame
    jenv->PushLocalFrame(16);
    jweak jworkflowWeak = static_cast<jweak>(user);
    jobject jworkflow = jenv->NewLocalRef(jworkflowWeak);
    if (!jworkflow)
    {
        // This isn't strictly an error, but it may be indicative on a problem
        LOGW("Trying to get EF Forced Error, but Java EFWorkflow object has been deleted.");
        return 0;
    }

    jclass JEFWorkflow = jenv->GetObjectClass(jworkflow);
    jmethodID getForcedErrorEnumMethod = jenv->GetMethodID(JEFWorkflow, "getForcedErrorEnum", "()I");
    jint result = jenv->CallIntMethod(jworkflow, getForcedErrorEnumMethod);

    // Delete local reference frame (but keep weak reference around!)
    jenv->PopLocalFrame(nullptr);
    LOGD("GetEFForcedError() EXIT - returning %d", result);
    return result;
}

// Clear the EF Forced error option
void ClearEFForcedError(void *user)
{
    LOGD("ClearEFForcedError() ENTRY - user=%p", user);
    JNIEnv *jenv = nullptr;
    if (!gVM)
    {
        LOGE("Global Java VM is null in AI callback, this should not happen.");
        return;
    }
    // Attach current thread OR get current threads JNIEnv if already attached
    gVM->AttachCurrentThreadAsDaemon(&jenv, nullptr);

    // Since this function is not directly called from Java, we should allocate a local reference frame
    jenv->PushLocalFrame(16);
    jweak jworkflowWeak = static_cast<jweak>(user);
    jobject jworkflow = jenv->NewLocalRef(jworkflowWeak);
    if (!jworkflow)
    {
        // This isn't strictly an error, but it may be indicative on a problem
        LOGW("Trying to clear EF Forced Error, but Java EFWorkflow object has been deleted.");
        return;
    }

    jclass JEFWorkflow = jenv->GetObjectClass(jworkflow);
    jmethodID clearForcedErrorMethod = jenv->GetMethodID(JEFWorkflow, "clearForcedError", "()V");
    jenv->CallVoidMethod(jworkflow, clearForcedErrorMethod);

    // Delete local reference frame (but keep weak reference around!)
    jenv->PopLocalFrame(nullptr);
    LOGD("ClearEFForcedError() EXIT");
}

/// Gets the EFWorkflow instance associated with the gUltrasoundManager
/// gUltrasoundManager MUST already be created
static EFWorkflow* GetJNIGlobalEFWorkflow() {
    LOGD("GetJNIGlobalEFWorkflow() ENTRY");
    EFWorkflow* result = echonous::gUltrasoundManager.getAIManager().getEFWorkflow();
    LOGD("GetJNIGlobalEFWorkflow() EXIT - returning %p", result);
    return result;
}

extern "C" {
JNIEXPORT void JNICALL Java_com_echonous_ai_EFWorkflow_init
    (JNIEnv *jenv, jobject thiz) {
    LOGD("Java_com_echonous_ai_EFWorkflow_init() ENTRY - jenv=%p, thiz=%p", jenv, thiz);
    EFWorkflow *workflow = GetJNIGlobalEFWorkflow();
    workflow->setFrameIDCallback(&CallbackFrameIDComplete);
    workflow->setSegmentationCallback(&CallbackSegmentationComplete);
    workflow->setStatisticsCallback(&CallbackStatisticsComplete);
    workflow->setErrorCallback(&CallbackOnError);
    workflow->setAutocaptureStartCallback(&CallbackOnAutocaptureStart);
    workflow->setAutocaptureFailCallback(&CallbackOnAutocaptureFail);
    workflow->setAutocaptureSucceedCallback(&CallbackOnAutocaptureSucceed);
    workflow->setAutocaptureProgressCallback(&CallbackOnAutocaptureProgress);
    workflow->setSmartCaptureProgressCallback(&CallbackOnSmartCaptureProgress);

    if (!gVM) {
        jenv->GetJavaVM(&gVM);
    }

    // Initialize global class references
    // Check for null, so that each class is only initialized once
    // These references are never deleted, but they are only initialized once, so there is no
    // memory leak.
    jclass cls;
    if (nullptr == gJCardiacView) {
        cls = jenv->FindClass("com/echonous/ai/CardiacView");
        gJCardiacView = static_cast<jclass>(jenv->NewGlobalRef(cls));
    }
    if (nullptr == gJCardiacFrames)
    {
        cls = jenv->FindClass("com/echonous/ai/CardiacFrames");
        gJCardiacFrames = static_cast<jclass>(jenv->NewGlobalRef(cls));
    }
    if (nullptr == gJCardiacSegmentation) {
        cls = jenv->FindClass("com/echonous/ai/CardiacSegmentation");
        gJCardiacSegmentation = static_cast<jclass>(jenv->NewGlobalRef(cls));
    }
    if (nullptr == gJEFStatistics) {
        cls = jenv->FindClass("com/echonous/ai/EFStatistics");
        gJEFStatistics = static_cast<jclass>(jenv->NewGlobalRef(cls));
    }
    if (nullptr == gJAutoDopplerTarget) {
        cls = jenv->FindClass("com/echonous/ai/EFWorkflow$AutoDopplerTarget");
        gJAutoDopplerTarget = static_cast<jclass>(jenv->NewGlobalRef(cls));
    }
    LOGD("Java_com_echonous_ai_EFWorkflow_init() EXIT");
}

JNIEXPORT void JNICALL Java_com_echonous_ai_EFWorkflow_reset
    (JNIEnv *jenv, jobject thiz)
{
    LOGD("Java_com_echonous_ai_EFWorkflow_reset() ENTRY - jenv=%p, thiz=%p", jenv, thiz);
    EFWorkflow *workflow = GetJNIGlobalEFWorkflow();
    workflow->reset();
    LOGD("Java_com_echonous_ai_EFWorkflow_reset() EXIT");
}

/*
 * Class:     com_echonous_ai_EFWorkflow
 * Method:    setHeartRate
 * Signature: (F)V
 */
JNIEXPORT void JNICALL Java_com_echonous_ai_EFWorkflow_setHeartRate
    (JNIEnv *jenv, jobject thiz, jfloat hr)
{
    LOGD("Java_com_echonous_ai_EFWorkflow_setHeartRate() ENTRY - jenv=%p, thiz=%p, hr=%f", jenv, thiz, hr);
    EFWorkflow *workflow = GetJNIGlobalEFWorkflow();
    workflow->setHeartRate(hr);
    LOGD("Java_com_echonous_ai_EFWorkflow_setHeartRate() EXIT");
}


/*
 * Class:     com_echonous_ai_EFWorkflow
 * Method:    beginQualityMeasure
 * Signature: (Lcom/echonous/ai/CardiacView;)V
 */
JNIEXPORT void JNICALL Java_com_echonous_ai_EFWorkflow_beginQualityMeasure
    (JNIEnv *jenv, jobject thiz, jobject view)
{
    // begin async quality measure
    LOGD("Java_com_echonous_ai_EFWorkflow_beginQualityMeasure() ENTRY - jenv=%p, thiz=%p, view=%p", jenv, thiz, view);
    auto *qualityTask = echonous::gUltrasoundManager.getAIManager().getEFQualityTask();
    auto cview = JtoCView(jenv, view);
    qualityTask->setView(cview);

    jweak globalThis = jenv->NewWeakGlobalRef(thiz);
    jweak previous = (jweak)qualityTask->setCallbackUserData(globalThis);
    if (previous)
        jenv->DeleteWeakGlobalRef(previous);

    qualityTask->setShowQuality(true);
    LOGD("Java_com_echonous_ai_EFWorkflow_beginQualityMeasure() EXIT");
}

/*
 * Class:     com_echonous_ai_EFWorkflow
 * Method:    endQualityMeasure
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_echonous_ai_EFWorkflow_endQualityMeasure
    (JNIEnv *jenv, jobject thiz)
{
    // end async quality measure
    LOGD("Java_com_echonous_ai_EFWorkflow_endQualityMeasure() ENTRY - jenv=%p, thiz=%p", jenv, thiz);
    auto *qualityTask = echonous::gUltrasoundManager.getAIManager().getEFQualityTask();
    qualityTask->setShowQuality(false);
    LOGD("Java_com_echonous_ai_EFWorkflow_endQualityMeasure() EXIT");
}


/*
 * Class:     com_echonous_ai_EFWorkflow
 * Method:    beginGuidance
 * Signature: (Lcom/echonous/ai/CardiacView;)V
 */
JNIEXPORT void JNICALL Java_com_echonous_ai_EFWorkflow_beginGuidance
    (JNIEnv *jenv, jobject thiz, jobject view)
{
    // begin async guidance
    LOGD("Java_com_echonous_ai_EFWorkflow_beginGuidance() ENTRY - jenv=%p, thiz=%p, view=%p", jenv, thiz, view);
    auto *qualityTask = echonous::gUltrasoundManager.getAIManager().getEFQualityTask();
    auto cview = JtoCView(jenv, view);
    qualityTask->setView(cview);

    jweak globalThis = jenv->NewWeakGlobalRef(thiz);
    jweak previous = (jweak)qualityTask->setCallbackUserData(globalThis);
    if (previous)
        jenv->DeleteWeakGlobalRef(previous);

    qualityTask->setShowGuidance(true);
    LOGD("Java_com_echonous_ai_EFWorkflow_beginGuidance() EXIT");
}

/*
 * Class:     com_echonous_ai_EFWorkflow
 * Method:    endGuidance
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_echonous_ai_EFWorkflow_endGuidance
    (JNIEnv *jenv, jobject thiz)
{
    // end async guidance
    LOGD("Java_com_echonous_ai_EFWorkflow_endGuidance() ENTRY - jenv=%p, thiz=%p", jenv, thiz);
    auto *qualityTask = echonous::gUltrasoundManager.getAIManager().getEFQualityTask();
    qualityTask->setShowGuidance(false);
    LOGD("Java_com_echonous_ai_EFWorkflow_endGuidance() EXIT");
}

/*
 * Class:     com_echonous_ai_EFWorkflow
 * Method:    beginAutoAnnotation
 * Signature: (Lcom/echonous/ai/CardiacView;)V
 */
JNIEXPORT void JNICALL Java_com_echonous_ai_EFWorkflow_beginAutoAnnotation
    (JNIEnv *jenv, jobject thiz, jobject view)
{
    // begin async auto annotation
    LOGD("Java_com_echonous_ai_EFWorkflow_beginAutoAnnotation() ENTRY - jenv=%p, thiz=%p, view=%p", jenv, thiz, view);
    auto *annotatorTask = echonous::gUltrasoundManager.getAIManager().getCardiacAnnotatorTask();
    auto cview = JtoCView(jenv, view);
    annotatorTask->setView(cview);
    annotatorTask->setPause(false, AutoDopplerTarget::None);
    LOGD("Java_com_echonous_ai_EFWorkflow_beginAutoAnnotation() EXIT");
}

/*
 * Class:     com_echonous_ai_EFWorkflow
 * Method:    endAutoAnnotation
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_echonous_ai_EFWorkflow_endAutoAnnotationDoppler
    (JNIEnv *jenv, jobject thiz)
{
    // end async auto annotation
    LOGD("Java_com_echonous_ai_EFWorkflow_endAutoAnnotationDoppler() ENTRY - jenv=%p, thiz=%p", jenv, thiz);
    auto *annotatorTask = echonous::gUltrasoundManager.getAIManager().getCardiacAnnotatorTask();
    annotatorTask->setPause(true, AutoDopplerTarget::None);
    LOGD("Java_com_echonous_ai_EFWorkflow_endAutoAnnotationDoppler() EXIT");
}

/*
 * Class:     com_echonous_ai_EFWorkflow
 * Method:    beginFrameID
 * Signature: (Lcom/echonous/ai/CardiacView;)V
 */
JNIEXPORT void JNICALL Java_com_echonous_ai_EFWorkflow_beginFrameID
    (JNIEnv *jenv, jobject thiz, jobject view)
{
    LOGD("Java_com_echonous_ai_EFWorkflow_beginFrameID() ENTRY - jenv=%p, thiz=%p, view=%p", jenv, thiz, view);
    EFWorkflow *workflow = GetJNIGlobalEFWorkflow();
    auto cview = JtoCView(jenv, view);
    // Using a weak global reference here to not prevent GC
    //  if the object is GC'd by the time the callback returns, then the callback simply is not fired
    jweak globalThis = jenv->NewWeakGlobalRef(thiz);
    auto task = workflow->createFrameIDTask(static_cast<void*>(globalThis), cview);
    echonous::gUltrasoundManager.getAIManager().push(std::move(task));
    LOGD("Java_com_echonous_ai_EFWorkflow_beginFrameID() EXIT");
}

/*
 * Class:     com_echonous_ai_EFWorkflow
 * Method:    beginSegmentation
 * Signature: (Lcom/echonous/ai/CardiacView;I)V
 */
JNIEXPORT void JNICALL Java_com_echonous_ai_EFWorkflow_beginSegmentation
    (JNIEnv *jenv, jobject thiz, jobject view, jint frame)
{
    LOGD("Java_com_echonous_ai_EFWorkflow_beginSegmentation() ENTRY - jenv=%p, thiz=%p, view=%p, frame=%d", jenv, thiz, view, frame);
    EFWorkflow *workflow = GetJNIGlobalEFWorkflow();
    auto cview = JtoCView(jenv, view);
    // Using a weak global reference here to not prevent GC
    //  if the object is GC'd by the time the callback returns, then the callback simply is not fired
    jweak globalThis = jenv->NewWeakGlobalRef(thiz);
    auto task = workflow->createSegmentationTask(static_cast<void*>(globalThis), cview, frame);
    echonous::gUltrasoundManager.getAIManager().push(std::move(task));
    LOGD("Java_com_echonous_ai_EFWorkflow_beginSegmentation() EXIT");
}

/*
 * Class:     com_echonous_ai_EFWorkflow
 * Method:    beginEFCalculation
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_echonous_ai_EFWorkflow_beginEFCalculation
  (JNIEnv *jenv, jobject thiz)
{
    LOGD("Java_com_echonous_ai_EFWorkflow_beginEFCalculation() ENTRY - jenv=%p, thiz=%p", jenv, thiz);
    EFWorkflow *workflow = GetJNIGlobalEFWorkflow();
    // Using a weak global reference here to not prevent GC
    //  if the object is GC'd by the time the callback returns, then the callback simply is not fired
    jweak globalThis = jenv->NewWeakGlobalRef(thiz);
    auto task = workflow->createEFCalculationTask(static_cast<void*>(globalThis));
    echonous::gUltrasoundManager.getAIManager().push(std::move(task));
    LOGD("Java_com_echonous_ai_EFWorkflow_beginEFCalculation() EXIT");
}

/*
 * Class:     com_echonous_ai_EFWorkflow
 * Method:    getFrameQuality
 * Signature: (I)Lcom/echonous/ai/QualityScore;
 */
JNIEXPORT jobject JNICALL Java_com_echonous_ai_EFWorkflow_getFrameQuality
  (JNIEnv *jenv, jobject thiz, jint frame)
{
    LOGD("Java_com_echonous_ai_EFWorkflow_getFrameQuality() ENTRY - jenv=%p, thiz=%p, frame=%d", jenv, thiz, frame);
    //int a4c, sub, a2c;
    //auto workflow = EFWorkflow::GetGlobalWorkflow();
    //workflow->getQualityScores(frame, &a4c, &sub, &a2c);

    LOGD("Java_com_echonous_ai_EFWorkflow_getFrameQuality() EXIT - returning NULL");
	return NULL;//CtoJQuality(a4c, sub, a2c);
}

/*
 * Class:     com_echonous_ai_EFWorkflow
 * Method:    setAIFrames
 * Signature: (Lcom/echonous/ai/CardiacView;Lcom/echonous/ai/CardiacFrames;)V
 */
JNIEXPORT void JNICALL Java_com_echonous_ai_EFWorkflow_setAIFrames
  (JNIEnv *jenv, jobject thiz, jobject view, jobject frames)
{
    LOGD("Java_com_echonous_ai_EFWorkflow_setAIFrames() ENTRY - jenv=%p, thiz=%p, view=%p, frames=%p", jenv, thiz, view, frames);
    EFWorkflow *workflow = GetJNIGlobalEFWorkflow();
    auto cview = JtoCView(jenv, view);
    auto cframes = JtoCFrames(jenv, frames);

    LOGW("Setting AI frames, multi cycle frame information is lost");
    MultiCycleFrames multiCycleFrames;
    multiCycleFrames.esFrames[0] = cframes.esFrame;
    multiCycleFrames.edFrames[0] = cframes.edFrame;
    multiCycleFrames.numCycles = 1;
    workflow->setAIFrames(cview, multiCycleFrames);
    LOGD("Java_com_echonous_ai_EFWorkflow_setAIFrames() EXIT");
}

/*
 * Class:     com_echonous_ai_EFWorkflow
 * Method:    getAIFrames
 * Signature: (Lcom/echonous/ai/CardiacView;)Lcom/echonous/ai/CardiacFrames;
 */
JNIEXPORT jobject JNICALL Java_com_echonous_ai_EFWorkflow_getAIFrames
  (JNIEnv *jenv, jobject thiz, jobject view)
{
    LOGD("Java_com_echonous_ai_EFWorkflow_getAIFrames() ENTRY - jenv=%p, thiz=%p, view=%p", jenv, thiz, view);
    EFWorkflow *workflow = GetJNIGlobalEFWorkflow();
    auto cview = JtoCView(jenv, view);
    auto multiCycleFrames = workflow->getAIFrames(cview);
    CardiacFrames cframes = {multiCycleFrames.esFrames[0], multiCycleFrames.edFrames[0]};
    jobject result = CtoJFrames(jenv, cframes);
    LOGD("Java_com_echonous_ai_EFWorkflow_getAIFrames() EXIT - returning %p", result);
    return result;
}

/*
 * Class:     com_echonous_ai_EFWorkflow
 * Method:    setAISegmentation
 * Signature: (Lcom/echonous/ai/CardiacView;ILcom/echonous/ai/CardiacSegmentation;)V
 */
JNIEXPORT void JNICALL Java_com_echonous_ai_EFWorkflow_setAISegmentation
  (JNIEnv *jenv, jobject thiz, jobject view, jint frame, jobject segmentation)
{
    LOGD("Java_com_echonous_ai_EFWorkflow_setAISegmentation() ENTRY - jenv=%p, thiz=%p, view=%p, frame=%d, segmentation=%p", jenv, thiz, view, frame, segmentation);
    EFWorkflow *workflow = GetJNIGlobalEFWorkflow();
    auto cview = JtoCView(jenv, view);
    auto csegmentation = JtoCSegmentation(jenv, segmentation);

    workflow->setAISegmentation(cview, frame, std::move(csegmentation));
    LOGD("Java_com_echonous_ai_EFWorkflow_setAISegmentation() EXIT");
}

/*
 * Class:     com_echonous_ai_EFWorkflow
 * Method:    getAISegmentation
 * Signature: (Lcom/echonous/ai/CardiacView;I)Lcom/echonous/ai/CardiacSegmentation;
 */
JNIEXPORT jobject JNICALL Java_com_echonous_ai_EFWorkflow_getAISegmentation
  (JNIEnv *jenv, jobject thiz, jobject view, jint frame)
{
    LOGD("Java_com_echonous_ai_EFWorkflow_getAISegmentation() ENTRY - jenv=%p, thiz=%p, view=%p, frame=%d", jenv, thiz, view, frame);
    EFWorkflow *workflow = GetJNIGlobalEFWorkflow();
    auto cview = JtoCView(jenv, view);
    auto cseg = workflow->getAISegmentation(cview, frame);
    jobject result = CtoJSegmentation(jenv, cseg);
    LOGD("Java_com_echonous_ai_EFWorkflow_getAISegmentation() EXIT - returning %p", result);
    return result;
}

/*
 * Class:     com_echonous_ai_EFWorkflow
 * Method:    setUserFrames
 * Signature: (Lcom/echonous/ai/CardiacView;Lcom/echonous/ai/CardiacFrames;)V
 */
JNIEXPORT void JNICALL Java_com_echonous_ai_EFWorkflow_setUserFrames
  (JNIEnv *jenv, jobject thiz, jobject view, jobject frames)
{
    LOGD("Java_com_echonous_ai_EFWorkflow_setUserFrames() ENTRY - jenv=%p, thiz=%p, view=%p, frames=%p", jenv, thiz, view, frames);
    EFWorkflow *workflow = GetJNIGlobalEFWorkflow();
    auto cview = JtoCView(jenv, view);
    auto cframes = JtoCFrames(jenv, frames);
    workflow->setUserFrames(cview, cframes);
    LOGD("Java_com_echonous_ai_EFWorkflow_setUserFrames() EXIT");
}

/*
 * Class:     com_echonous_ai_EFWorkflow
 * Method:    getUserFrames
 * Signature: (Lcom/echonous/ai/CardiacView;)Lcom/echonous/ai/CardiacFrames;
 */
JNIEXPORT jobject JNICALL Java_com_echonous_ai_EFWorkflow_getUserFrames
  (JNIEnv *jenv, jobject thiz, jobject view)
{
    LOGD("Java_com_echonous_ai_EFWorkflow_getUserFrames() ENTRY - jenv=%p, thiz=%p, view=%p", jenv, thiz, view);
    EFWorkflow *workflow = GetJNIGlobalEFWorkflow();
    auto cview = JtoCView(jenv, view);
    auto cframes = workflow->getUserFrames(cview);
    jobject result = CtoJFrames(jenv, cframes);
    LOGD("Java_com_echonous_ai_EFWorkflow_getUserFrames() EXIT - returning %p", result);
    return result;
}

/*
 * Class:     com_echonous_ai_EFWorkflow
 * Method:    setUserSegmentation
 * Signature: (Lcom/echonous/ai/CardiacView;ILcom/echonous/ai/CardiacSegmentation;)V
 */
JNIEXPORT void JNICALL Java_com_echonous_ai_EFWorkflow_setUserSegmentation
  (JNIEnv *jenv, jobject thiz, jobject view, jint frame, jobject segmentation)
{
    LOGD("Java_com_echonous_ai_EFWorkflow_setUserSegmentation() ENTRY - jenv=%p, thiz=%p, view=%p, frame=%d, segmentation=%p", jenv, thiz, view, frame, segmentation);
    EFWorkflow *workflow = GetJNIGlobalEFWorkflow();
    auto cview = JtoCView(jenv, view);
    auto csegmentation = JtoCSegmentation(jenv, segmentation);

    workflow->setUserSegmentation(cview, frame, std::move(csegmentation));
    LOGD("Java_com_echonous_ai_EFWorkflow_setUserSegmentation() EXIT");
}

/*
 * Class:     com_echonous_ai_EFWorkflow
 * Method:    getUserSegmentation
 * Signature: (Lcom/echonous/ai/CardiacView;I)Lcom/echonous/ai/CardiacSegmentation;
 */
JNIEXPORT jobject JNICALL Java_com_echonous_ai_EFWorkflow_getUserSegmentation
  (JNIEnv *jenv, jobject thiz, jobject view, jint frame)
{
    LOGD("Java_com_echonous_ai_EFWorkflow_getUserSegmentation() ENTRY - jenv=%p, thiz=%p, view=%p, frame=%d", jenv, thiz, view, frame);
    EFWorkflow *workflow = GetJNIGlobalEFWorkflow();
    auto cview = JtoCView(jenv, view);
    auto cseg = workflow->getUserSegmentation(cview, frame);
    jobject result = CtoJSegmentation(jenv, std::move(cseg));
    LOGD("Java_com_echonous_ai_EFWorkflow_getUserSegmentation() EXIT - returning %p", result);
    return result;
}

/*
 * Class:     com_echonous_ai_EFWorkflow
 * Method:    getStatistics
 * Signature: ()Lcom/echonous/ai/EFStatistics;
 */
JNIEXPORT jobject JNICALL Java_com_echonous_ai_EFWorkflow_getStatistics
  (JNIEnv *jenv, jobject thiz)
{
    LOGD("Java_com_echonous_ai_EFWorkflow_getStatistics() ENTRY - jenv=%p, thiz=%p", jenv, thiz);
    EFWorkflow *workflow = GetJNIGlobalEFWorkflow();
    auto cstats = workflow->getStatistics();

    jobject result = CtoJStatistics(jenv, cstats);
    LOGD("Java_com_echonous_ai_EFWorkflow_getStatistics() EXIT - returning %p", result);
    return result;
}

}
extern "C"
JNIEXPORT void JNICALL
Java_com_echonous_ai_EFWorkflow_setAutocaptureEnabled(JNIEnv *env, jobject thiz,
                                                      jboolean is_enabled) {
    LOGD("Java_com_echonous_ai_EFWorkflow_setAutocaptureEnabled() ENTRY - env=%p, thiz=%p, is_enabled=%s", env, thiz, is_enabled ? "true" : "false");
    echonous::gUltrasoundManager.getAIManager().getEFQualityTask()->setAutocaptureEnabled(is_enabled);
    LOGD("Java_com_echonous_ai_EFWorkflow_setAutocaptureEnabled() EXIT");
}

#include <FASTObjectDetectorTask.h>

extern "C"
JNIEXPORT void JNICALL
Java_com_echonous_ai_EFWorkflow_beginFastExam(JNIEnv *env, jobject thiz) {
    LOGD("Java_com_echonous_ai_EFWorkflow_beginFastExam() ENTRY - env=%p, thiz=%p", env, thiz);
    auto *fastModule = echonous::gUltrasoundManager.getAIManager().getFASTModule();
    fastModule->setPause(false);
    LOGD("Java_com_echonous_ai_EFWorkflow_beginFastExam() EXIT");
}
extern "C"
JNIEXPORT void JNICALL
Java_com_echonous_ai_EFWorkflow_endFastExam(JNIEnv *env, jobject thiz) {
    LOGD("Java_com_echonous_ai_EFWorkflow_endFastExam() ENTRY - env=%p, thiz=%p", env, thiz);
    auto *fastModule = echonous::gUltrasoundManager.getAIManager().getFASTModule();
    fastModule->setPause(true);
    LOGD("Java_com_echonous_ai_EFWorkflow_endFastExam() EXIT");
}
extern "C"
JNIEXPORT jintArray JNICALL
Java_com_echonous_ai_EFWorkflow_nativeSmartCapture(JNIEnv *env, jobject thiz, jint durationMS) {
    LOGD("Java_com_echonous_ai_EFWorkflow_nativeSmartCapture() ENTRY - env=%p, thiz=%p, durationMS=%d", env, thiz, durationMS);
#if MOCK_SMARTCAPTURE
    jintArray range = env->NewIntArray(3);
    jint buf[3] = {0,0,0};
    uint32_t minFrame = echonous::gUltrasoundManager.getCineBuffer().getMinFrameNum();
    uint32_t maxFrame = echonous::gUltrasoundManager.getCineBuffer().getMaxFrameNum();
    if (maxFrame - minFrame >= 40) {
        buf[0] = THOR_OK;
        buf[1] = minFrame+10;
        buf[2] = minFrame+40;
    } else {
        // Not enough data
        buf[0] = THOR_ERROR;
    }
    env->SetIntArrayRegion(range, 0, 3, buf);
    return range;
#else
    auto *guidanceModule = echonous::gUltrasoundManager.getAIManager().getEFQualityTask();
    uint32_t numFrames = durationMS / echonous::gUltrasoundManager.getCineBuffer().getParams().frameInterval;
    uint32_t begin, end;
    ThorStatus status = guidanceModule->findSmartCaptureRange(numFrames, &begin, &end);
    jintArray range = env->NewIntArray(3);
    jint buf[3] = {0,0,0};

    if (IS_THOR_ERROR(status)) {
        buf[0] = static_cast<jint>(status);
    } else {
        buf[0] = THOR_OK;
        buf[1] = static_cast<jint>(begin);
        buf[2] = static_cast<jint>(end);
        // Clear quality buffers so we cannot get the same range again
        guidanceModule->resetQualityBuffers();
    }
    env->SetIntArrayRegion(range, 0, 3, buf);
    LOGD("Java_com_echonous_ai_EFWorkflow_nativeSmartCapture() EXIT - returning range with status=%d, begin=%d, end=%d", buf[0], buf[1], buf[2]);
    return range;

#endif
}

extern "C"
JNIEXPORT void JNICALL
Java_com_echonous_ai_EFWorkflow_beginAutoDoppler(JNIEnv *jenv, jobject thiz, jobject target) {
    LOGD("Java_com_echonous_ai_EFWorkflow_beginAutoDoppler() ENTRY - jenv=%p, thiz=%p, target=%p", jenv, thiz, target);
    auto *annotator = echonous::gUltrasoundManager.getAIManager().getCardiacAnnotatorTask();
    annotator->setPause(false, JtoCAutoDopplerTarget(jenv, target));

    jweak globalThis = jenv->NewWeakGlobalRef(thiz);
    jweak previous = (jweak)annotator->setAutoDopplerCallback(globalThis, CallbackOnAutoDopplerUpdate);
    if (previous)
        jenv->DeleteWeakGlobalRef(previous);
    LOGD("Java_com_echonous_ai_EFWorkflow_beginAutoDoppler() EXIT");
}
extern "C"
JNIEXPORT void JNICALL
Java_com_echonous_ai_EFWorkflow_setMockAutoDoppler(JNIEnv *env, jobject thiz, jboolean mock) {
    LOGD("Java_com_echonous_ai_EFWorkflow_setMockAutoDoppler() ENTRY - env=%p, thiz=%p, mock=%d", env, thiz, mock);
    auto *annotator = echonous::gUltrasoundManager.getAIManager().getCardiacAnnotatorTask();
    // jboolean is uint8_t, convert to c++ bool
    bool cMock = (mock != 0);
    annotator->setMockAutoDoppler(cMock);
    LOGD("Java_com_echonous_ai_EFWorkflow_setMockAutoDoppler() EXIT");
}