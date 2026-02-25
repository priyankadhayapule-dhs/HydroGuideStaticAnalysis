//
//  BladderSegmentationTask.h
//  EchoNous
//
//  Created by Eung-Hun Kim on 2/22/24.
//
#pragma once

#include <mutex>
#include <condition_variable>
#include <thread>
#include <deque>

#include <CineBuffer.h>
#include <ImguiRenderer.h>
#include <UltrasoundManager.h>
#include <imgui.h>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/mat.hpp>
#include "BladderWorkflow.h"
//#import "BladderSegmentationModels.h"
#include <ScanConverterCl.h>
#include <stdint.h>
#define BLADDER_SEGMENTATION_ENSEMBLE_SIZE 4
#define BLADDER_SEG_IMG_WIDTH 128
#define BLADDER_SEG_IMG_HEIGHT 128
#define FLATTOP_ASPECT_ADJUST_FACTOR  1.21392194652f
#define BUILD_FOR_CLINICAL_STUDY 1

class AIManager;

/// Type of callback invoked on bladder location found/location updated
typedef void (*BladderSegmentationCallback)(float, float, float, float, int, int);
typedef void (*BladderContourCallback)(float**);
typedef bool (*BladderRoICallback)(int);
typedef bool (*BladderFrameInRoICallback)(int);
typedef void (*BladderVerificationLoadCallback)(int,int, int, uint32_t);// 0 = Sagittal 1 = Transverse // 0 = Use prevoid workflow, 1 = use postvoid workflow
typedef int (*MaxAreaFrameCallback)();
typedef float (*MaxAreaCallback)();

class BladderSegmentationTask: public CineBuffer::CineCallback, public ImGuiDrawable
{
    
public:
    BladderSegmentationTask(UltrasoundManager *usm);
    ~BladderSegmentationTask();
    
    void attachSegmentationCallback(BladderSegmentationCallback bladderSegmentationCallback);
    void attachContourCallback(BladderContourCallback bladderContourCallback);
    
    ThorStatus init(void* javaEnv, void* javaContext);
    ThorStatus init();
    void start();
    void start(void* javaEnv, void* javaContext);
    void stop();
    
    void setPause(bool pause);
    void setFlip(float flipX, float flipY);
    
    // ImGuiDrawable functions
    virtual void draw(CineViewer &cineviewer, uint32_t frameNum) override;
    virtual void close() override;
    virtual void openDrawable() override;
    // CineCallback functions
    virtual ThorStatus onInit() override;
    virtual void onTerminate() override;
    virtual void setParams(CineBuffer::CineParams& params) override;
    virtual void onData(uint32_t frameNum, uint32_t dauDataTypes) override;
    virtual void onSave(uint32_t startFrame, uint32_t endFrame,
                        echonous::capnp::ThorClip::Builder &builder) override;
    virtual void onLoad(echonous::capnp::ThorClip::Reader &reader) override;
 
    // Convert predictions from image space to phsyical space
    void convertToPhysicalSpace(float flipX);
    // convert to physical space from display scale.  (coordinate in display coordinate)
    void convertToPhysicalSpaceFromDisplayCoord(float flipX, cv::Point2f inPt, cv::Point2f& outPt);
    void convertToPhysicalSpaceFromDisplayCoord(float flipX, std::vector<cv::Point> inPtVec, std::vector<cv::Point2f>& outPtVec);
    
    // Workflow Interface
    void startCapture(uint32_t durationInSecound);
    void stopCapture();
    
    // Set segmentation color mask
    void setSegmentationColor(uint32_t color);
    // Get Segmentation results
    BVSegmentation getSegmentation(int frameNumber);
    BVSegmentation estimateCaliperLocations(uint32_t frameNum, BVType viewType);
    BVSegmentation estimateCaliperLocations(uint32_t frameNum, BVType viewType, Json::Value& verification, void* jni);
    
    /// Pipeline
    void setResultsFromSwift(int32_t frameNumber);

private:
    
    //BladderSegmentationModels* mBldSegModel;

    bool workerShouldAwaken();
    void setSegmentationParams(float centroidX, float centroidY, float uncertainty, float stability, int area, int frameNumber);

    void setContourParams(float** contourMap);

    void workerThreadFunc(void* jniInterface);
    ThorStatus processFrame(uint32_t frameNum, float flipX, void* jni);
    
    // caliper location estimation
    float estimateMaskAngleTransverse(uint32_t frameNum, cv::InputArray segMask);
    float estimateMaskAngleTransverse(uint32_t frameNum, cv::InputArray segMask, Json::Value& root);
    float estimateMaskAngleSagittal(std::vector<cv::Point> contour, cv::InputArray segMask, float refMaskLength);
    float estimateMaskAngleSagittalWithMask(uint32_t frameNum, cv::InputArray segMask, float refMaskLength);
    float estimateMaskAngleSagittalWithMask(uint32_t frameNum, cv::InputArray segMask, float refMaskLength, Json::Value& root);
    
    bool estimateIntersectionWithMask(cv::InputArray segMask, float angleInDegreee, float length, float centX, float centY, cv::Point& outPt1, cv::Point& outPt2);
    bool estimateIntersectionWithContour(std::vector<cv::Point> contour, float angleInDegree, float centX, float centY, cv::Point2f& outPt1, cv::Point2f& outPt2);
    bool findCrossSection(cv::Point2f centroid, float vectorX, float vectorY, cv::Point2f peakPt, cv::Point2f adjPt, cv::Point2f& outPt);
    void twoChannelSoftmax(cv::Mat& input, cv::Mat& output);
    bool mHasParams = false;
    CineBuffer::CineParams mParams;
    UltrasoundManager *mUSManager;
    CineBuffer *mCB;
    //running process pMap in base layer for now
    void processPMap(float* inputPatch, float* outputPMap, int imgWidth, int imgHeight);
    
    
    // Buffer of frame
    //std::vector<int>                             mFrameWasSkipped;
    
    bool mInit;
    bool mPaused;
    bool mCalculated;
    bool mStop;
    uint32_t mProcessedFrame;
    uint32_t mCineFrame;
    uint32_t mFrameCount;
    double mTimeMeasure;
    
    // input image / buffer
    float mScaledInputBuffer[BLADDER_SEG_IMG_WIDTH * BLADDER_SEG_IMG_HEIGHT * sizeof(float)];
    
    std::mutex mMutex;
    std::condition_variable mCV;
    std::thread mWorkerThread;
    
    float mFlipX;
    ImguiRenderer &mImguiRenderer;
    
    std::vector<cv::Mat> mProbMap;
    std::vector<cv::Mat> mProbMapFlt;
    cv::Mat              mSegMask;
    uint32_t             mMaskArea;
    float                mUncertainty;
    float                mCentroidX, mCentroidY;
    float                mScreenCentroidX, mScreenCentroidY;
    float                mStability;
    void*                mJNI;
    GLuint               mTexture;
#if BUILD_FOR_CLINICAL_STUDY
    std::vector<cv::Mat> mSegMaskSave;
    float*               mUncertaintyValue;
    cv::Mat              mSegMaskSelected;
#endif
    
    // texture for drawing
    //id<MTLTexture>      mTexture;
    uint32_t            mImageWidth;
    uint32_t            mImageHeight;
    
    // SegmentationMask Color
    uint8_t             mMaskColorR;
    uint8_t             mMaskColorG;
    uint8_t             mMaskColorB;
    uint8_t             mMaskColorA;
    
    void* mBladderSegmentationCallbackUser;
    BladderSegmentationCallback mBladderSegmentationCallback;
    BladderContourCallback     mBladderContourCallback;
    std::unique_ptr<ScanConverterCL> mScanConverter;
    std::vector<uint8_t>            mScanConvertedFrame;
    // verification
public:
    ThorStatus runAllVerification(const char* outputPath);
    ThorStatus runVerificationTest(void* jni, const char* inputPath, const char* outputPath);
    void runFrameVerification(Json::Value& root, uint32_t frameNum, const char* filepath, const char* clipname, void* jni);
    bool isCentroidValid(int frameNum);
    bool getFrameInRoIMap(int frameNum);
    BladderRoICallback mBladderRoICallback;
    BladderFrameInRoICallback mBladderFrameInRoICallback;
    void attachBladderFrameInRoICallBack(BladderFrameInRoICallback callback);
    struct VerificationFrame{
        float x;
        float y;
        float uncertainty;
        int32_t frame;
        uint32_t area;
    };
    VerificationFrame mSelectedVerificationFrame;
    void resetSelectedFrame();
    struct Coordinate {
        float x;
        float y;
    };
    struct LineSegment {
        Coordinate start;
        Coordinate end;
    };
    static constexpr float UNCERTAINTY_THRESHOLD = 0.7f;
    const char* mMapName;
    uint32_t mMapFrameNum;
    const char* mMapFilePath;
    const char* mClipname;
    int mInvalidFrameCount;
    static const int INVALID_FRAME_THRESHOLD = 50;
    uint32_t mSwiftSelectedFrame;
    bool mFrameSelected;
    void saveImgToPhysicalMatrix(Json::Value& verification);
    void savePixelSize(Json::Value& verification);
    void find_mask_largest_component(const cv::Mat& input, cv::Mat& output);
    Json::Value ToJson(const cv::Mat& mat);
    Json::Value ToJson(const BVSegmentation& calipers);
    Json::Value ToJson(const cv::Point2f& point);
    Json::Value ToJson(const BVPoint& point);
    Json::Value ToJson(const BVCaliper& caliper);
    BladderVerificationLoadCallback mLoadCallback;
    void attachVerificationLoadCallback(BladderVerificationLoadCallback depthCallback);
    void attachValidRoICallback(BladderRoICallback roICallback);
    void attachMaxAreaFrameCallback(MaxAreaFrameCallback callback);
    void attachMaxAreaCallback(MaxAreaCallback callback);
    int getDepthId();
    BVType determineClipViewType(const char* inputPath);
    int determineClipFlowType(const char* inputPath);
    int getMaxAreaFrame();
    int getMaxArea();
private:
    MaxAreaFrameCallback mMaxAreaFrameCallback;
    MaxAreaCallback mMaxAreaCallback;
};


