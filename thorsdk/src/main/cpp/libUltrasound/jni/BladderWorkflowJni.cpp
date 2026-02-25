//
// Created by Michael May on 4/22/25.
//
#include <jni.h>
#include <Bladder/BladderSegmentationTask.h>
#include <UltrasoundManager.h>
// Global Java VM
static JavaVM *gVM = nullptr;
namespace echonous {
    extern UltrasoundManager gUltrasoundManager;
}
    struct BladderWorkflowContextIds {
        jmethodID onBladderSegmentation;
        jmethodID isFrameInRoI;
        jmethodID clearRoIData;
        jmethodID getRoISize;
        jmethodID getFrameInRoIMap;
        jmethodID setVerificationDepth;
        jmethodID setVerificationLoadInfo;
        jmethodID getMaxAreaFrame;
        jmethodID getMaxArea;
    } gBladderWorkflowContextIds;
    struct BladderSegmentationContextIds {
        jmethodID processOrdered;
        jmethodID process;
    } gBladderSegmentationContextIds;
    static jobject gJavaBladderWorkflow = nullptr;    // java object / listener
    static jclass gJavaBladderSegmentation = nullptr; // BVSeg
    static jclass gJavaBladderCaliper = nullptr; // BVCal
    static jclass gJavaBladderPoint = nullptr; // BVPoint

    static JavaVM *gJavaVmPtr = nullptr;
    static CriticalSection gBvwControlLock;

    static void setSegmentationParams(jfloat centroidX, jfloat centroidY, jfloat uncertainty, jfloat
    stability, jint area, jint frameNum) {
        JNIEnv *jenv = nullptr;
        gBvwControlLock.enter();
        if (nullptr != gJavaVmPtr && nullptr != gJavaBladderWorkflow) {
            gJavaVmPtr->AttachCurrentThread(&jenv, NULL);
            if (nullptr == jenv) {
                ALOGE("[ BLADDER ] Unable to get Android Runtime");
                goto err_ret;
            }
            jenv->CallVoidMethod(gJavaBladderWorkflow,
                                 gBladderWorkflowContextIds.onBladderSegmentation,
                                 centroidX, centroidY, uncertainty, stability, area,
                                 frameNum);
        }

        err_ret:
        gBvwControlLock.leave();
    }

    static bool isCentroidInRoIForFrame(int frameNum){
        JNIEnv *jenv = nullptr;
        gBvwControlLock.enter();
        if (nullptr != gJavaVmPtr && nullptr != gJavaBladderWorkflow) {
            gJavaVmPtr->AttachCurrentThread(&jenv, NULL);
            if (nullptr == jenv) {
                ALOGE("[ BLADDER ] Unable to get Android Runtime");
                goto err_ret;
            }
            return jenv->CallBooleanMethod(gJavaBladderWorkflow,
                                 gBladderWorkflowContextIds.isFrameInRoI,
                                 frameNum);
        }
        if(gJavaVmPtr == nullptr){
            LOGD("[ BLADDER ] Nullptr for global java vm");
        }
        if(gJavaBladderWorkflow == nullptr){
            LOGD("[ BLADDER ] Nullptr for gJavaBladderWorkflow");
        }

        err_ret:
        LOGD("[ BLADDER ] ERROR CALLING ROI METHOD");
        gBvwControlLock.leave();
        return false;
    }

static bool isFrameInRoIMap(int frameNum){
    JNIEnv *jenv = nullptr;
    gBvwControlLock.enter();
    if (nullptr != gJavaVmPtr && nullptr != gJavaBladderWorkflow) {
        gJavaVmPtr->AttachCurrentThread(&jenv, NULL);
        if (nullptr == jenv) {
            ALOGE("[ BLADDER ] Unable to get Android Runtime");
            goto err_ret;
        }
        return jenv->CallBooleanMethod(gJavaBladderWorkflow,
                                       gBladderWorkflowContextIds.getFrameInRoIMap,
                                       frameNum);
    }

    err_ret:
    LOGD("[ BLADDER ] ERROR CALLING ROI METHOD");
    gBvwControlLock.leave();
    return false;
}
static void setVerificationLoadInfo(int flowtype, int viewtype, int verificationDepth,
                                    uint32_t probetype){
    JNIEnv *jenv = nullptr;
    gBvwControlLock.enter();
    if (nullptr != gJavaVmPtr && nullptr != gJavaBladderWorkflow) {
        gJavaVmPtr->AttachCurrentThread(&jenv, NULL);
        if (nullptr == jenv) {
            ALOGE("[ BLADDER ] Unable to get Android Runtime");
            goto err_ret;
        }
        jenv->CallVoidMethod(gJavaBladderWorkflow,
                                gBladderWorkflowContextIds.setVerificationLoadInfo,
                                flowtype,
                                viewtype,
                                verificationDepth,
                                probetype);
        return;
    }

    err_ret:
    LOGD("[ BLADDER ] ERROR CALLING ROI METHOD");
    gBvwControlLock.leave();
    return;
}
static void setVerificationDepth(int verificationDepth){
    JNIEnv *jenv = nullptr;
    gBvwControlLock.enter();
    if (nullptr != gJavaVmPtr && nullptr != gJavaBladderWorkflow) {
        gJavaVmPtr->AttachCurrentThread(&jenv, NULL);
        if (nullptr == jenv) {
            ALOGE("[ BLADDER ] Unable to get Android Runtime");
            goto err_ret;
        }
        jenv->CallBooleanMethod(gJavaBladderWorkflow,
                                       gBladderWorkflowContextIds.setVerificationDepth,
                                       verificationDepth);
        return;
    }

    err_ret:
    LOGD("[ BLADDER ] ERROR CALLING ROI METHOD");
    gBvwControlLock.leave();
    return;
}

static int getRoiFrameCount(){
    JNIEnv *jenv = nullptr;
    gBvwControlLock.enter();
    if (nullptr != gJavaVmPtr && nullptr != gJavaBladderWorkflow) {
        gJavaVmPtr->AttachCurrentThread(&jenv, NULL);
        if (nullptr == jenv) {
            ALOGE("[ BLADDER ] Unable to get Android Runtime");
            goto err_ret;
        }
        return jenv->CallIntMethod(gJavaBladderWorkflow,
                                       gBladderWorkflowContextIds.isFrameInRoI);
    }
    if(gJavaVmPtr == nullptr){
        LOGD("[ BLADDER ] Nullptr for global java vm");
    }
    if(gJavaBladderWorkflow == nullptr){
        LOGD("[ BLADDER ] Nullptr for gJavaBladderWorkflow");
    }

    err_ret:
    LOGD("[ BLADDER ] ERROR CALLING ROI METHOD");
    gBvwControlLock.leave();
    return false;
}

static int getMaxAreaFrame(){
    JNIEnv *jenv = nullptr;
    gBvwControlLock.enter();
    if (nullptr != gJavaVmPtr && nullptr != gJavaBladderWorkflow) {
        gJavaVmPtr->AttachCurrentThread(&jenv, NULL);
        if (nullptr == jenv) {
            ALOGE("[ BLADDER ] Unable to get Android Runtime for getMaxAreaFrame");
            goto err_ret;
        }
        int result = jenv->CallIntMethod(gJavaBladderWorkflow,
                                       gBladderWorkflowContextIds.getMaxAreaFrame);
        gBvwControlLock.leave();
        return result;
    }
    
    err_ret:
    ALOGE("[ BLADDER ] ERROR CALLING getMaxAreaFrame - returning -1");
    gBvwControlLock.leave();
    return -1;
}

static float getMaxArea(){
    JNIEnv *jenv = nullptr;
    gBvwControlLock.enter();
    if (nullptr != gJavaVmPtr && nullptr != gJavaBladderWorkflow) {
        gJavaVmPtr->AttachCurrentThread(&jenv, NULL);
        if (nullptr == jenv) {
            ALOGE("[ BLADDER ] Unable to get Android Runtime for getMaxArea");
            goto err_ret;
        }
        float result = jenv->CallFloatMethod(gJavaBladderWorkflow,
                                           gBladderWorkflowContextIds.getMaxArea);
        gBvwControlLock.leave();
        return result;
    }
    
    err_ret:
    ALOGE("[ BLADDER ] ERROR CALLING getMaxArea - returning -1.0f");
    gBvwControlLock.leave();
    return -1.0f;
}

    static jobject CtoJBladderContour(JNIEnv *jenv, BVSegmentation contour) {
        std::vector<float> points;
        if(contour.contour.size() > 0) {
            for (const auto &p: contour.contour) {
                points.push_back(p.x);
                points.push_back(p.y);
            }
            // add base point to end
            points.push_back(contour.contour.front().x);
            points.push_back(contour.contour.front().y);
        }
        jfloatArray jpoints = jenv->NewFloatArray(points.size());
        jenv->SetFloatArrayRegion(jpoints, 0, points.size(), &points.front());

        jmethodID ctor = jenv->GetMethodID(gJavaBladderSegmentation, "<init>",
                                           "([FLcom/echonous/ai/BVCaliper;Lcom/echonous/ai/BVCaliper;)V");
        jmethodID ctorCaliper = jenv->GetMethodID(gJavaBladderCaliper, "<init>",
                                                  "(Lcom/echonous/ai/BVPoint;Lcom/echonous/ai/BVPoint;)V");
        jmethodID ctorPoint = jenv->GetMethodID(gJavaBladderPoint, "<init>", "(FF)V");
        jobject majCalStart = jenv->NewObject(gJavaBladderPoint, ctorPoint,
                                              contour.majorCaliper.startPoint.x,
                                              contour.majorCaliper.startPoint.y);
        jobject majCalEnd = jenv->NewObject(gJavaBladderPoint, ctorPoint,
                                            contour.majorCaliper.endPoint.x,
                                            contour.majorCaliper.endPoint.y);
        jobject majPoint = jenv->NewObject(gJavaBladderCaliper, ctorCaliper, majCalStart,
                                           majCalEnd);
        jobject minCalStart = jenv->NewObject(gJavaBladderPoint, ctorPoint,
                                              contour.minorCaliper.startPoint.x,
                                              contour.minorCaliper.startPoint.y);
        jobject minCalEnd = jenv->NewObject(gJavaBladderPoint, ctorPoint,
                                            contour.minorCaliper.endPoint.x,
                                            contour.minorCaliper.endPoint.y);
        jobject minPoint = jenv->NewObject(gJavaBladderCaliper, ctorCaliper, minCalStart,
                                           minCalEnd);
        return jenv->NewObject(gJavaBladderSegmentation, ctor, jpoints, majPoint, minPoint);
    }

//-----------------------------------------------------------------------------
    static void initBladderSegmentation(JNIEnv *jenv, jobject obj, jobject thisObj) {
        UNUSED(obj);
        LOGD("[ BLADDER ] initBladderSegmentation");
        jenv->GetJavaVM(&gJavaVmPtr);

        gBvwControlLock.enter();
        gJavaBladderWorkflow = jenv->NewGlobalRef(thisObj);
        if (nullptr == gJavaBladderSegmentation) {
            jclass cls = jenv->FindClass("com/echonous/ai/BVSegmentation");
            gJavaBladderSegmentation = static_cast<jclass>(jenv->NewGlobalRef(cls));
        }
        if (nullptr == gJavaBladderCaliper) {
            jclass cls = jenv->FindClass("com/echonous/ai/BVCaliper");
            gJavaBladderCaliper = static_cast<jclass>(jenv->NewGlobalRef(cls));
        }
        if (nullptr == gJavaBladderPoint) {
            jclass cls = jenv->FindClass("com/echonous/ai/BVPoint");
            gJavaBladderPoint = static_cast<jclass>(jenv->NewGlobalRef(cls));
        }
        gBvwControlLock.leave();

    echonous::gUltrasoundManager.getAIManager().getBladderSegmentationTask()->attachSegmentationCallback(
                setSegmentationParams);

    echonous::gUltrasoundManager.getAIManager().getBladderSegmentationTask()->attachValidRoICallback(
            isCentroidInRoIForFrame);

    echonous::gUltrasoundManager.getAIManager().getBladderSegmentationTask()->attachBladderFrameInRoICallBack(
            isFrameInRoIMap);
    echonous::gUltrasoundManager.getAIManager().getBladderSegmentationTask()->attachVerificationLoadCallback(
            setVerificationLoadInfo);
    echonous::gUltrasoundManager.getAIManager().getBladderSegmentationTask()->attachMaxAreaFrameCallback(
            getMaxAreaFrame);
    echonous::gUltrasoundManager.getAIManager().getBladderSegmentationTask()->attachMaxAreaCallback(
            getMaxArea);
    }

    static void terminateBladderSegmentation(JNIEnv *jenv, jobject obj) {
        UNUSED(obj);
        LOGD("[ BLADDER ] terminateBladderSegmentation");
        // TODO:: write detach onSegmentationParams
        //gUltrasoundManager.getAIManager().getBladderSegmentationTask()->detachSegmentationCallback(setSegmentationParams);
        gBvwControlLock.enter();
        if (nullptr != gJavaBladderWorkflow) {
            jenv->DeleteGlobalRef(gJavaBladderWorkflow);
            gJavaBladderWorkflow = nullptr;
        }
        if (nullptr != gJavaBladderSegmentation) {
            // delete segmentation
        }
        gBvwControlLock.leave();
    }

    static jobject getContour(JNIEnv* env, jobject thiz, jint frameNum, jint viewType){
        UNUSED(thiz);
        LOGD("[ BLADDER ] BladderVolumeWorkflow::getContour(%d,%d)", frameNum, viewType);
        auto *bvw = echonous::gUltrasoundManager.getAIManager().getBladderSegmentationTask();
        auto calipers = bvw->estimateCaliperLocations(frameNum, BVType(viewType));
        //process calipers, return object
        return CtoJBladderContour(env, calipers);
    }

    extern "C"
    JNIEXPORT void JNICALL
    Java_com_echonous_ai_BladderWorkflow_beginBladderExam(JNIEnv *env, jobject thiz, jobject context) {
        //LOGD("[ BLADDER ] BladderWorkflow::beginBladderSegmentationTask (GetJavaVM Result: %d)", result);

        auto *bvw = echonous::gUltrasoundManager.getAIManager().getBladderSegmentationTask();
        //bvw->start(env, context);
        bvw->setPause(true);
    }
    extern "C"
    JNIEXPORT void JNICALL
    Java_com_echonous_ai_BladderWorkflow_endBladderExam(JNIEnv *env, jobject thiz) {
        LOGD("[ BLADDER ] BladderWorkflow::endBladderSegmentationTask");
        auto *bvw = echonous::gUltrasoundManager.getAIManager().getBladderSegmentationTask();
        bvw->setPause(false);
    }
    //-----------------------------------------------------------------------------
    static JNINativeMethod bladderSeg_method_table[] =
            {
                    {"nativeInit",
                            "(Lcom/echonous/ai/BladderWorkflow;)V",
                            (void *) initBladderSegmentation
                    },
                    {"nativeTerminate",
                            "()V",
                            (void *) terminateBladderSegmentation
                    },
                    {
                     "getContour",
                            "(II)Lcom/echonous/ai/BVSegmentation;",
                            (void *) getContour
                    },
            };
namespace echonous {
    //-----------------------------------------------------------------------------
    int register_bladderSeg_methods(JNIEnv *env) {
        jclass bladderSegClass = env->FindClass("com/echonous/ai/BladderWorkflow");
        LOG_FATAL_IF(bladderSegClass == NULL,
                     "Unable to find class com.echonous.ai.BladderWorkflow");

        gBladderWorkflowContextIds.onBladderSegmentation = env->GetMethodID(bladderSegClass,
                                                                            "setSegmentationParams",
                                                                            "(FFFFII)V");
        gBladderWorkflowContextIds.isFrameInRoI = env->GetMethodID(bladderSegClass,
                                                                            "getCentroidRegionOfInterestStatus",
                                                                            "(I)Z");
        gBladderWorkflowContextIds.clearRoIData = env->GetMethodID(bladderSegClass,
                                                                   "clearCentroidRegionOfInterestStatus",
                                                                   "()V");
        gBladderWorkflowContextIds.getFrameInRoIMap = env->GetMethodID(bladderSegClass,
                                                                   "getFrameInRoIMap",
                                                                   "(I)Z");
        gBladderWorkflowContextIds.setVerificationDepth = env->GetMethodID(bladderSegClass,
                                                                       "setVerificationDepth",
                                                                       "(I)V");
        gBladderWorkflowContextIds.setVerificationLoadInfo = env->GetMethodID(bladderSegClass,
                                                                           "setVerificationLoadInfo",
                                                                           "(IIII)V"); // silent conversion from int to unsigned int for fouth argument
        gBladderWorkflowContextIds.getRoISize = env->GetMethodID(bladderSegClass,
                                                                       "getTotalFramesProcessed",
                                                                       "()I");
        gBladderWorkflowContextIds.getMaxAreaFrame = env->GetMethodID(bladderSegClass,
                                                                     "getMaxAreaFrame",
                                                                     "()I");
        gBladderWorkflowContextIds.getMaxArea = env->GetMethodID(bladderSegClass,
                                                                "getMaxArea",
                                                                "()F");
        LOG_FATAL_IF(gBladderSegmentationContextIds.onBladderSegmentation == NULL,
                     "Unable to find method onCompletion");
        return env->RegisterNatives(bladderSegClass,
                                    bladderSeg_method_table,
                                    NELEM(bladderSeg_method_table));
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_echonous_ai_BladderWorkflow_setSegmentationColor(JNIEnv *env, jobject thiz, long color) {
    UNUSED(thiz);
    auto *bvw = echonous::gUltrasoundManager.getAIManager().getBladderSegmentationTask();
    bvw->setSegmentationColor(color);
}