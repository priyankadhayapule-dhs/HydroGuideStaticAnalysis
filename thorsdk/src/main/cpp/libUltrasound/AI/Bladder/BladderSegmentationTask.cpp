//
//  BladderSegmentationTask.cpp
//  EchoNous
//
//  Created by Eung-Hun Kim on 2/22/24.
//
#define LOG_TAG "BladderSegmentationTask"

#include <opencv2/imgproc.hpp>
#include <stdio.h>
#include <BladderSegmentationTask.h>
#include <AIManager.h>
#include <MLOperations.h>
#include "BladderWorkflow.h"
#include <Bladder/BladderSegmentationJNI.h>
#include <math.h>

// verification includes
#include <json/json.h>
#include <fstream>
#include <sys/stat.h>
#include <cstdio>
#include <GLES3/gl3.h>
#define NO_IMAGE_OUTPUTS
using lock = std::unique_lock<std::mutex>;
void SaveMap(cv::Mat image, const char* name, unsigned int frameNum, const char* filepath, const char* clipname)
{
#ifdef NO_IMAGE_OUTPUTS
    return;
#endif
    std::stringstream path;
    path << filepath << "/";
    path << clipname << "/"; // store raw files by clip
    path << "frame_" << frameNum << "_";
    path << name << ".raw";
    LOGD("Saving probmap (%d, %d) in path %s", image.rows, image.cols, path.str().c_str());
    std::ofstream output(path.str(), std::ios::binary);
    // Warning: only works if the image is contiguous
    output.write((const char*)image.data, image.total() * image.elemSize());
}
void WriteImg(const char *filepath, const char *name, cv::InputArray _input, bool normalize, uint32_t frameNum)
{
    std::stringstream path;
    path << filepath;
    path << "_frame" << frameNum << "_";
    path << name << ".png";
    FILE *stream = fopen(path.str().c_str(), "wb");
    if (!stream)
    {
        //LOGD("[ BLADDER ] Failed to open path %s", path.str().c_str());
        return;
    }

    cv::Mat input = _input.getMat();
    CV_Assert(input.depth() == CV_8U || input.depth() == CV_32F);
    CV_Assert(input.channels() == 1 || input.channels() == 3);

    cv::Mat img;
    if (normalize)
    {
        // normalize and possibly convert type
        cv::Mat normalized;
        cv::normalize(input, normalized, 0.0, 255.0, cv::NORM_MINMAX, CV_8U);
        img = normalized;
    }
    else if (input.depth() == CV_32F)
    {
        // convert type
        input.convertTo(img, CV_8U, 255.0);
    }
    else
    {
        // no change, img can reference input (avoids copy)
        img = input;
    }

    fprintf(stream, "P%c\n%d %d\n255\n", img.channels() == 1 ? '5' : '6', img.size().width, img.size().height);
    fwrite(img.ptr<uint8_t>(), img.channels(), img.total(), stream);
    fclose(stream);
}

/**x
 * Flatten a probability map into a binary mask where channel 1 is the highest.
 */
static void flatten(cv::InputArray input, cv::OutputArray output, uint8_t outVal = 255u)
{
    cv::Mat probmap = input.getMat();
    cv::Size size = probmap.size();

    //output.create(size, CV_8UC1);
    cv::Mat seg = output.getMat();

    for (int y=0; y < size.height; ++y)
    for (int x=0; x < size.width; ++x)
    {
        float ch0 = probmap.at<float[2]>(y,x)[0];
        float ch1 = probmap.at<float[2]>(y,x)[1];

        if (ch1 > ch0)
            seg.at<uint8_t>(y,x) = outVal;
        else
            seg.at<uint8_t>(y,x) = 0u;
    }
}

static void translate_image(cv::InputArray input, cv::OutputArray output, float shiftX, float shiftY)
{
    float mtx[2][3] = {{1, 0, shiftX},{0, 1, shiftY}};
    cv::Mat mat = cv::Mat(2, 3, CV_32F, mtx);
        
    cv::Mat inputMap = input.getMat();
    cv::Size inSize = inputMap.size();
    
    cv::warpAffine(input, output, mat, inSize);
}

static void rorate_image(cv::InputArray input, cv::OutputArray output, float angle, float centerX, float centerY)
{
    cv::Point2f ctrPt(centerX, centerY);
    
    cv::Mat inputMap = input.getMat();
    cv::Size inSize = inputMap.size();
    
    cv::Mat rotMtx = cv::getRotationMatrix2D(ctrPt, angle, 1.0);
    cv::warpAffine(input, output, rotMtx, inSize);
}

BladderSegmentationTask::BladderSegmentationTask(UltrasoundManager *usm):
        mUSManager(usm),
        mCB(&usm->getCineBuffer()),
        mPaused(true),
        mCalculated(false),
        mStop(false),
        mInit(false),
        mMutex(),
        mCV(),
        mWorkerThread(),
        mFlipX(1.0f),
        mScanConverter(nullptr),
        mImageWidth(BLADDER_SEG_IMG_WIDTH),
        mImageHeight(BLADDER_SEG_IMG_HEIGHT),
        mImguiRenderer(usm->getAIManager().getImguiRenderer()),
        mBladderSegmentationCallbackUser(nullptr),
        mBladderSegmentationCallback(nullptr),
        mJNI(nullptr),
        mTexture(0),
        mMaxAreaFrameCallback(nullptr),
        mMaxAreaCallback(nullptr)
{
    // SegmentationMask Color: default
    mMaskColorR = 100;
    mMaskColorG = 0;
    mMaskColorB = 0;
    mMaskColorA = 255;
    mScanConvertedFrame.resize(BLADDER_SEG_IMG_HEIGHT*BLADDER_SEG_IMG_WIDTH);
    // TODO: remove if not needed
    // mFrameWasSkipped.resize(CINE_FRAME_COUNT);
}

BladderSegmentationTask::~BladderSegmentationTask()
{
    stop();
}

ThorStatus BladderSegmentationTask::init(void* javaEnv, void* javaContext)
{
    mJNI = static_cast<void*>(new BladderSegmentationJNI((JNIEnv*)javaEnv, (jobject)javaContext));
    // Create scan converter (destroys old scan converter, if any)
    mScanConverter = std::unique_ptr<ScanConverterCL>(new ScanConverterCL);
    // Init scan converter with dummy values. setParams will be called before imaging
    // actually starts, and that will overwrite any values set here.
    // Might as well use reasonable default though
    ThorStatus status = mScanConverter->setConversionParameters(
            512, 112,
            BLADDER_SEG_IMG_WIDTH, BLADDER_SEG_IMG_HEIGHT,
            0.0f, 120.0f, 0.0f, 512.0f/120.0f, -0.7819f, 0.013962f,
            -1.0f, 1.0f);
    if (IS_THOR_ERROR(status)) { return status; }
    status = mScanConverter->init();
    if (IS_THOR_ERROR(status)) { return status; }
    return THOR_OK;
}

ThorStatus BladderSegmentationTask::init()
{
    mCB->registerCallback(this);
    // Create scan converter (destroys old scan converter, if any)
    mScanConverter = std::unique_ptr<ScanConverterCL>(new ScanConverterCL);
    // Init scan converter with dummy values. setParams will be called before imaging
    // actually starts, and that will overwrite any values set here.
    // Might as well use reasonable default though
    ThorStatus status = mScanConverter->setConversionParameters(
            512, 112,
            BLADDER_SEG_IMG_WIDTH, BLADDER_SEG_IMG_HEIGHT,
            0.0f, 120.0f, 0.0f, 512.0f/120.0f, -0.7819f, 0.013962f,
            -1.0f, 1.0f);
    if (IS_THOR_ERROR(status)) { return status; }
    status = mScanConverter->init();
    if (IS_THOR_ERROR(status)) { return status; }
    return THOR_OK;
}

void BladderSegmentationTask::start()
{
    stop();
    mStop = false;

    mInit = true;
    // TEMP/TODO: Set to always on by default
    mPaused = true;
    mCalculated = false;
    
    // set up prob Map
    mProbMap.resize(BLADDER_SEGMENTATION_ENSEMBLE_SIZE);
    cv::Size outSize = {BLADDER_SEG_IMG_WIDTH, BLADDER_SEG_IMG_HEIGHT};
    // flattened
    mProbMapFlt.resize(BLADDER_SEGMENTATION_ENSEMBLE_SIZE);
    
    for (auto i = 0u; i != mProbMap.size(); i++)
        mProbMap[i] =  cv::Mat(outSize, CV_32FC2);
    
    for (auto i = 0u; i != mProbMapFlt.size(); i++)
        mProbMapFlt[i] = cv::Mat(outSize, CV_8UC1);

    // segMask
    mSegMask = cv::Mat(outSize, CV_8UC1);

    // reset FrameCount
    mFrameCount = 0;
    
    // for clinical study - save segmentation mask
#if BUILD_FOR_CLINICAL_STUDY
    mSegMaskSave.resize(CINE_FRAME_COUNT);
    
    for (auto i = 0u; i != CINE_FRAME_COUNT; i++)
        mSegMaskSave[i] = cv::Mat(outSize, CV_8UC1);
    
    mUncertaintyValue = new float[CINE_FRAME_COUNT];
#endif
    
    auto l = lock(mMutex);
    mCB->registerCallback(this);
    mWorkerThread = std::thread(&BladderSegmentationTask::workerThreadFunc, this, mJNI);
}

void BladderSegmentationTask::start(void* javaEnv, void* javaContext)
{
    stop();
    mStop = false;

    mInit = true;
    // TEMP/TODO: Set to always on by default
    mPaused = true;
    mCalculated = false;

    // set up prob Map
    mProbMap.resize(BLADDER_SEGMENTATION_ENSEMBLE_SIZE);
    cv::Size outSize = {BLADDER_SEG_IMG_WIDTH, BLADDER_SEG_IMG_HEIGHT};
    // flattened
    mProbMapFlt.resize(BLADDER_SEGMENTATION_ENSEMBLE_SIZE);

    for (auto i = 0u; i != mProbMap.size(); i++)
        mProbMap[i] =  cv::Mat(outSize, CV_32FC2);

    for (auto i = 0u; i != mProbMapFlt.size(); i++)
        mProbMapFlt[i] = cv::Mat(outSize, CV_8UC1);

    // segMask
    mSegMask = cv::Mat(outSize, CV_8UC1);

    // reset FrameCount
    mFrameCount = 0;

    // for clinical study - save segmentation mask
#if BUILD_FOR_CLINICAL_STUDY
    mSegMaskSave.resize(CINE_FRAME_COUNT);

    for (auto i = 0u; i != CINE_FRAME_COUNT; i++)
        mSegMaskSave[i] = cv::Mat(outSize, CV_8UC1);

    mUncertaintyValue = new float[CINE_FRAME_COUNT];
#endif
    BladderSegmentationJNI* jniInterface = new BladderSegmentationJNI((JNIEnv*)javaEnv, (jobject)javaContext);
    mJNI = static_cast<void*>(jniInterface);
    auto l = lock(mMutex);

    mWorkerThread = std::thread(&BladderSegmentationTask::workerThreadFunc, this, mJNI);
}

void BladderSegmentationTask::stop()
{
    LOGD("stop()");
    {
        auto l = lock(mMutex);
        if (!mWorkerThread.joinable())
        {
            LOGD("Worker thread not joinable, stop complete.");
            return;
        }
        mStop = true;
    }
    LOGD("Worker thread joinable, awaiting done");
    mCV.notify_one();
    mWorkerThread.join();

    LOGD("Joined worker thread, stop complete.");
    
    // clean up prob maps and seg mask
    mProbMap.clear();
    mProbMapFlt.clear();
    mSegMask.release();
    mFrameCount = 0;

#if BUILD_FOR_CLINICAL_STUDY
    mSegMaskSave.clear();
    delete[] mUncertaintyValue;
    
    mSegMaskSelected.release();
#endif
    // mTexture = nil;
}

void BladderSegmentationTask::setPause(bool running)
{
    LOGD("[ BLADDER ] %s(%s)", __func__, running ? "true":"false");
    // TEMP/TODO: Disable pausing
    //return;

    auto l = lock(mMutex);
    
    if (!running)
    {
        // Clear predictions from the display
        // mImguiRenderer.setPredictions({});
    }
    
    // reset counters
    mFrameCount = 0;
    mMaskArea = 0;
    mCalculated = false;
    
    // re-init prob maps
    for (auto i = 0u; i != mProbMap.size(); i++)
        mProbMap[i].setTo(cv::Scalar::all(0.0f));
    
    // re-init seg mask
    mSegMask.setTo(cv::Scalar::all(0));

    mPaused = !running;
    mCV.notify_one();
}

void BladderSegmentationTask::setFlip(float flipX, float flipY)
{
    auto l = lock(mMutex);
    mFlipX = flipX;
    //flipY unused
}



ThorStatus BladderSegmentationTask::onInit()
{
    LOGD("[ BLADDER ] %s", __func__);
    auto l = lock(mMutex);
    mProcessedFrame = 0xFFFFFFFFu;
    mCineFrame = 0xFFFFFFFFu;
    mMaskArea = 0;
    mCalculated = false;
    // DO NOT reset buffer data, since we can have an onTerminate/onInit pair without clearing the cinebuffer data!
    return THOR_OK;
}
void BladderSegmentationTask::onTerminate()
{
    LOGD("%s", __func__);
    auto l = lock(mMutex);
    mProcessedFrame = 0xFFFFFFFFu;
    mCineFrame = 0xFFFFFFFFu;
    mMaskArea = 0;
    mCalculated = false;
    mPaused = true;
}

void BladderSegmentationTask::setParams(CineBuffer::CineParams& params)
{
    // Quick exit in Color or M mode
    LOGD("[ BLADDER ] setParams, dauDataTypes = 0x%08x", params.dauDataTypes);

    auto l = lock(mMutex);
    mScanConverter->setConversionParameters(
            BLADDER_SEG_IMG_WIDTH,
            BLADDER_SEG_IMG_HEIGHT,
            mFlipX,
            1.0f,
            ScanConverterCL::ScaleMode::NON_UNIFORM_SCALE_TO_FIT,
            params.converterParams);
    mHasParams = true;
    mParams = params;
}

void BladderSegmentationTask::onData(uint32_t frameNum, uint32_t dauDataTypes)
{
    //if (frameNum > 30 && mPaused == true)
        //setPause(true);
        
    if (BF_GET(dauDataTypes, DAU_DATA_TYPE_B, 1))
    {
        {
            auto l = lock(mMutex);
            mCineFrame = frameNum;
            if (frameNum == 0) {
                // On first frame, reset the processed frame count to -1 (no processed frames)
                mProcessedFrame = 0xFFFFFFFFu;
            }
            if (mPaused)
            {
              
            }

        }
        mCV.notify_one();
    }
}

bool BladderSegmentationTask::workerShouldAwaken()
{
    if (mStop) {
        return true;
    }
    if (!mPaused && mCineFrame != mProcessedFrame) {
        return true;
    }
    return false;
}


void BladderSegmentationTask::workerThreadFunc(void* jniInterface)
{
    uint32_t prevFrameNum = 0xFFFFFFFFu;
    uint32_t frameNum = 0xFFFFFFFFu;
    float flipX;
    BladderSegmentationJNI* jni = static_cast<BladderSegmentationJNI*>(jniInterface);
    jni->attachToThread();
    while (true)
    {
        // Get next frame number to process
        {
            auto l = lock(mMutex);

            // Update the processed frame number
            mProcessedFrame = frameNum;
            // Wait while paused, or until either mStop or we have a frame to process
            while (!workerShouldAwaken())
                mCV.wait(l);
            if (mStop)
                return;
            if (mProcessedFrame == 0xFFFFFFFFu)
                prevFrameNum = 0xFFFFFFFFu;
            else
                prevFrameNum = frameNum;
            frameNum = mCineFrame;
            flipX = mFlipX;
        }

        // Record frames that were skipped
        if (prevFrameNum != 0xFFFFFFFFu) {
            for (uint32_t frame = prevFrameNum+1; frame != frameNum; ++frame) {
                //mFrameWasSkipped[frame % CINE_FRAME_COUNT] = 1;
            }
        }
        BladderSegmentationJNI* bladderSegmentationJni = static_cast<BladderSegmentationJNI*>(mJNI);
        // Process the frame (without holding the lock)
        if(mHasParams && bladderSegmentationJni->isAttached)
        {
            ThorStatus status = processFrame(frameNum, flipX, jniInterface);

            if (IS_THOR_ERROR(status))
            {
                LOGE("[ BLADDER ] Error processing frame %u: %08x", frameNum, status);
            }
        }
    }
}

void BladderSegmentationTask::convertToPhysicalSpace(float flipX)
{
    // Taken from LVSegmentation.cpp - ConvertToPhysical
    float endImgDepth = mParams.converterParams.startSampleMm +
                        (mParams.converterParams.sampleSpacingMm * (mParams.converterParams.numSamples - 1));

    // img coordinate (BLADDER IMG - 128x128) to Physical
    float imgToPhys[9];
    ViewMatrix3x3::setIdentityM(imgToPhys);
    
    float scaleX = (BLADDER_SEG_IMG_WIDTH*0.5f) / (BLADDER_SEG_IMG_HEIGHT *
                                    sin(-mParams.converterParams.startRayRadian)) * FLATTOP_ASPECT_ADJUST_FACTOR;

    flipX /= scaleX;

    imgToPhys[I(0,0)] = flipX * 1.0f/BLADDER_SEG_IMG_WIDTH * endImgDepth; // scale to normalized, then to physical
    imgToPhys[I(0,2)] = flipX * -0.5f * endImgDepth; // translate so that middle of the image is at 0
    imgToPhys[I(1,1)] = 1.0f/BLADDER_SEG_IMG_HEIGHT * endImgDepth;
    imgToPhys[I(1,2)] = 0.0f; // no translation on y
        
    float posImg[3] = {mCentroidX, mCentroidY, 1.0f};
    float posPhys[3];
    
    // in physical coordinate
    ViewMatrix3x3::multiplyMV(posPhys, imgToPhys, posImg);
    
    mCentroidX = posPhys[0];
    mCentroidY = posPhys[1];
    
}

void BladderSegmentationTask::convertToPhysicalSpaceFromDisplayCoord(float flipX, cv::Point2f inPt, cv::Point2f& outPt)
{
    // Taken from LVSegmentation.cpp - ConvertToPhysical
    float endImgDepth = mParams.converterParams.startSampleMm +
                        (mParams.converterParams.sampleSpacingMm * (mParams.converterParams.numSamples - 1));

    // img coordinate (BLADDER IMG - 128x128) to Physical
    float imgToPhys[9];
    ViewMatrix3x3::setIdentityM(imgToPhys);
    
    float scrWidth = (float) mImguiRenderer.getWidth();
    float scrHeight = (float) mImguiRenderer.getHeight();
    
    imgToPhys[I(0,0)] = flipX * 1.0f/scrHeight * endImgDepth;                   // scale to normalized, then to physical
    imgToPhys[I(0,2)] = flipX * -0.5f * endImgDepth * scrWidth / scrHeight;     // translate so that middle of the image is at 0
    imgToPhys[I(1,1)] = 1.0f/scrHeight * endImgDepth;
    imgToPhys[I(1,2)] = 0.0f;                                                   // no translation on y
        
    float posImg[3] = {inPt.x, inPt.y, 1.0f};
    float posPhys[3];
    
    // in physical coordinate
    ViewMatrix3x3::multiplyMV(posPhys, imgToPhys, posImg);
    
    outPt.x = posPhys[0];
    outPt.y = posPhys[1];
}

void BladderSegmentationTask::convertToPhysicalSpaceFromDisplayCoord(float flipX, std::vector<cv::Point> inPtVec,
                                                                     std::vector<cv::Point2f>& outPtVec)
{
    float endImgDepth = mParams.converterParams.startSampleMm +
                        (mParams.converterParams.sampleSpacingMm * (mParams.converterParams.numSamples - 1));

    // img coordinate (BLADDER IMG - 128x128) to Physical
    float imgToPhys[9];
    ViewMatrix3x3::setIdentityM(imgToPhys);
    
    float scrWidth = (float) mImguiRenderer.getWidth();
    float scrHeight = (float) mImguiRenderer.getHeight();
    
    imgToPhys[I(0,0)] = flipX * 1.0f/scrHeight * endImgDepth;                   // scale to normalized, then to physical
    imgToPhys[I(0,2)] = flipX * -0.5f * endImgDepth * scrWidth / scrHeight;     // translate so that middle of the image is at 0
    imgToPhys[I(1,1)] = 1.0f/scrHeight * endImgDepth;
    imgToPhys[I(1,2)] = 0.0f;                                                   // no translation on y
        
    float posImg[3];
    float posPhys[3];
    
    posImg[2] = 1.0f;
    
    for (auto it = inPtVec.begin(); it != inPtVec.end(); it++) {
        cv::Point   inPt = *it;
        cv::Point2f outPt;
        
        posImg[0] = (float)inPt.x;
        posImg[1] = (float)inPt.y;
        
        // in physical coordinate
        ViewMatrix3x3::multiplyMV(posPhys, imgToPhys, posImg);
        
        outPt.x = posPhys[0];
        outPt.y = posPhys[1];
        
        outPtVec.push_back(outPt);
    }
}

ThorStatus BladderSegmentationTask::processFrame(uint32_t frameNum, float flipX, void* jni)
{
    // This is safe because we will only ever have the one worker thread
    if (!mInit)
    {
        mInit = true;
    }

    ThorStatus status = THOR_OK;
    const static int FRAME_DIVIDER = 2;
    // keep runnung
    if (frameNum % FRAME_DIVIDER == 0)
    {
        uint32_t modelIdx = mFrameCount%BLADDER_SEGMENTATION_ENSEMBLE_SIZE;

        BladderSegmentationJNI* bJni = static_cast<BladderSegmentationJNI*>(mJNI);
        // scan converted image
        mUSManager->getAIManager().getScanConvertedFrame(frameNum, BLADDER_SEG_IMG_WIDTH, BLADDER_SEG_IMG_HEIGHT, (mScanConvertedFrame.data()));

        if (IS_THOR_ERROR(status)) {
            return status;
        }

        cv::Size outSize = {BLADDER_SEG_IMG_WIDTH, BLADDER_SEG_IMG_HEIGHT};
        cv::Mat inputMat = {outSize, CV_32FC1, bJni->imageBufferNative.data()};
        cv::Mat inputCharMat = {outSize, CV_8UC1, mScanConvertedFrame.data()};
        inputCharMat.convertTo(inputMat, CV_32FC1, 1.0/255.0);
      //  WriteImg("/data/data/com.echonous.kosmosapp/debug/", "input", inputMat, true, frameNum);
        cv::Mat probMapMean(outSize, CV_32FC2);
        cv::Mat probMapFltSum(outSize, CV_8UC1);
        cv::Mat probMapFltOne(outSize, CV_8UC1);
        cv::Mat segMap(outSize, CV_8UC1);
        cv::Mat probMapUnfmt(outSize, CV_32FC2);
        // processFrame no longer includes softmax operations
        bJni->jenv->CallVoidMethod(bJni->instance, bJni->process, bJni->imageBuffer, bJni->viewBuffer, bJni->pMapBuffer, modelIdx);
        memcpy(mProbMap[modelIdx].ptr<float>(), bJni->pMapBufferNative.data(), sizeof(float) * BLADDER_SEG_IMG_WIDTH * BLADDER_SEG_IMG_HEIGHT * 2);
        //processPMap(bJni->pMapBufferNative.data(), mProbMap[modelIdx].ptr<float>(), BLADDER_SEG_IMG_WIDTH, BLADDER_SEG_IMG_HEIGHT);
        twoChannelSoftmax(mProbMap[modelIdx], mProbMap[modelIdx]);
        // Flatten ProbMap.  Setting 1 (instead of 255)
        flatten(mProbMap[modelIdx], mProbMapFlt[modelIdx], 1u);

        // TODO: improve computation efficiency
        // Mean - probMap
        probMapMean = mProbMap[0] + mProbMap[1] + mProbMap[2] + mProbMap[3];
        probMapMean = probMapMean * 0.25;
        probMapFltSum = mProbMapFlt[0] + mProbMapFlt[1] + mProbMapFlt[2] + mProbMapFlt[3];
        probMapFltOne = probMapFltSum == 4;

        int pMapFltNonZero = cv::countNonZero(probMapFltSum);
        int pMapFltOneNonZero = cv::countNonZero(probMapFltOne);
        
        // flatten
        flatten(probMapMean, segMap);

        // set to zeros to segmentation mask
        mSegMask.setTo(cv::Scalar::all(0));
        mMaskArea = 0;
        
        cv::Mat labels(outSize, CV_8UC1);    //output
        cv::Mat stats;
        cv::Mat centroids;

        int nb_components = cv::connectedComponentsWithStats(segMap, labels, stats, centroids, 4, CV_16U);
        // only 1 component
        if (nb_components < 2)
        {
            // TODO: 1 component case
            // ignore
            mCentroidX = 0.0f;
            mCentroidY = 0.0f;
        }
        else
        {
            // nb compts >= 2
            int curVal;
            int max1, max2;
            int max1Idx, max2Idx;
            max1 = -10;
            max2 = -20;
            max1Idx = 0;
            max2Idx = 0;
            
            for (int i = 0; i < nb_components; i++)
            {
                curVal =  stats.at<int>(i, cv::CC_STAT_AREA);
                
                if (curVal > max1)
                {
                    // update 2nd max
                    max2 = max1;
                    max2Idx = max1Idx;
                    
                    // update the max
                    max1 = curVal;
                    max1Idx = i;
                }
                else if (curVal > max2)
                {
                    // update 2nd max
                    max2 = curVal;
                    max2Idx = i;
                }
            }

            // IOS-2776.
            // check whether max2 can be background. > 95% of width and height
            if (stats.at<int>(max2Idx, cv::CC_STAT_WIDTH) > BLADDER_SEG_IMG_WIDTH * 0.95f
                && stats.at<int>(max2Idx, cv::CC_STAT_HEIGHT) > BLADDER_SEG_IMG_HEIGHT * 0.95f)
            {
                max2 = max1;
                max2Idx = max1Idx;
            }
                        
            mMaskArea = max2;
            
            // apply min filter - min area should be 100 - this set values to 0 or 255
            cv::inRange(labels, cv::Scalar(max2Idx), cv::Scalar(max2Idx), mSegMask);

            // centroid of identified index - defined on 128 x 128 (BLADDER_SEG_IMG_WIDTH & HEIGHT)
            mCentroidX = centroids.at<double>(max2Idx, 0);
            mCentroidY = centroids.at<double>(max2Idx, 1);

            float scrWidth = (float) mImguiRenderer.getWidth();
            float scrHeight = (float) mImguiRenderer.getHeight();
        }
                
        // Uncertainty
        // iou = (map_count_ratio == 1).sum() / (map_count_ratio > 0).sum()
        // when map_count_ratio > 0 is not zero
        if ( (mMaskArea < 100) || (pMapFltNonZero == 0) )
            mUncertainty = 1.0f;
        else
            mUncertainty = 1.0f - ((float) pMapFltOneNonZero) / ((float) pMapFltNonZero);
        
        mStability = 0.0f;
        // convert to Physical coordinate
        convertToPhysicalSpace(flipX);
        
        // TODO: update
        //mFrameWasSkipped[frameNum % CINE_FRAME_COUNT] = 0;
                

        if (nullptr != mBladderSegmentationCallback) {
            // Send callback here
            setSegmentationParams(mCentroidX, mCentroidY, mUncertainty, mStability, mMaskArea, frameNum);
        }

#if BUILD_FOR_CLINICAL_STUDY
        // save segmentation mask image for clinical study
        std::copy(mSegMask.ptr<uint8_t>(), mSegMask.ptr<uint8_t>() + BLADDER_SEG_IMG_WIDTH * BLADDER_SEG_IMG_HEIGHT, mSegMaskSave[frameNum%CINE_FRAME_COUNT].ptr<uint8_t>());
        mUncertaintyValue[frameNum%CINE_FRAME_COUNT] = mUncertainty;
#endif
        
        mFrameCount++;
        
        // clean up
        probMapMean.release();
        segMap.release();
        labels.release();
        stats.release();
        centroids.release();
    }
    // removed comment block
    return THOR_OK;
}

void BladderSegmentationTask::processPMap(float* inputPatch, float* pMapOut, int imgWidth, int imgHeight) {

    float *patch1_ch0, *patch1_ch1;
    float *patch_out;

    // input channel
    patch1_ch0 = inputPatch;
    patch1_ch1 = inputPatch + imgWidth * imgHeight;
    for ( int row = 0; row < imgHeight; row++ )
    {
        patch1_ch0 = inputPatch + row * imgWidth;
        patch1_ch1 = inputPatch + row * imgWidth + imgWidth * imgHeight;

        patch_out = pMapOut + ((row * 2) * imgWidth);    // 2 ch

        for ( int column = 0; column < imgWidth; column++ )
        {
            *patch_out++ = *patch1_ch0++;
            *patch_out++ = *patch1_ch1++;
        }
    }
}

float BladderSegmentationTask::estimateMaskAngleTransverse(uint32_t frameNum, cv::InputArray segMask) {
    
    // screen display size
    int scrWidth = mImguiRenderer.getWidth();
    int scrHeight = mImguiRenderer.getHeight();
    
    cv::Size scrSize = {scrWidth, scrHeight};
    cv::Mat screenImageRGBA(scrSize, CV_8UC4);
    cv::Mat screenImageUD(scrSize, CV_8UC1);
    cv::Mat screenImage(scrSize, CV_8UC1);
      
    // get Ultrasound Image (Screen Image)
    mUSManager->getCineViewer().getUltrasoundScreenImage(screenImageRGBA.ptr<uint8_t>(), scrWidth, scrHeight, false, frameNum);
    // convert to gray scale
    cv::cvtColor(screenImageRGBA, screenImage, cv::COLOR_RGBA2GRAY);
    // flip image as ultrasound image is vertically flipped
    //cv::flip(screenImageUD, screenImage, 0);
    
    // centered image
    cv::Mat mask_centered(scrSize, CV_8UC1);
    cv::Mat image_centered(scrSize, CV_8UC1);
    cv::Mat combo_centered(scrSize, CV_8UC1);
    cv::Mat mask_rotate(scrSize, CV_8UC1);
    
    float imgCenterX = scrWidth/2.0f;
    float imgCenterY = scrHeight/2.0f;
    int   imgCenterXInt = int(imgCenterX);
    
    float shiftX = imgCenterX - mCentroidX;
    float shiftY = imgCenterY - mCentroidY;
    
    // Centered images
    translate_image(segMask, mask_centered, shiftX, shiftY);
    translate_image(screenImage, image_centered, shiftX, shiftY);
    
    combo_centered = mask_centered * 0.5 + image_centered * 0.5;
    float currentAngle = -10.0f;
    float angleStep = 1.f;
    float maxAngle = 10.01f;
    
    float angleSelect = 0.0f;
    float maxSimCos = -1000000.0f;
    // more sophisticated angle search - check 20 points, then 20 more near the highest point
    // start with angle step of 1
    while (currentAngle <= maxAngle)
    {
                
        rorate_image(combo_centered, mask_rotate, currentAngle, imgCenterX, imgCenterY);
                
        cv::Mat leftHalf = mask_rotate(cv::Rect(0, 0, imgCenterXInt, scrHeight));
        cv::Mat rightHalf = mask_rotate(cv::Rect(imgCenterXInt, 0, scrWidth - imgCenterXInt, scrHeight));
                       
        cv::Mat leftLine, leftLine2;
        cv::Mat rightLine, rightLine2;
        cv::Mat lrMul;
            
        cv::reduce(leftHalf, leftLine, 1, cv::REDUCE_SUM, CV_32F);
        cv::reduce(rightHalf, rightLine, 1, cv::REDUCE_SUM, CV_32F);
                
        cv::multiply(leftLine, rightLine, lrMul);
        cv::multiply(leftLine, leftLine, leftLine2);
        cv::multiply(rightLine, rightLine, rightLine2);
                
        double lrMulSum = cv::sum(lrMul)[0];
        double leftLineSum = cv::sum(leftLine2)[0];
        double rightLineSum = cv::sum(rightLine2)[0];
        
        float sim_cos = (float) (lrMulSum / ( sqrt(leftLineSum) * sqrt(rightLineSum) ) );
        
        if (sim_cos > maxSimCos)
        {
            angleSelect = currentAngle;
            maxSimCos = sim_cos;
        }
        
        currentAngle += angleStep;
    }
    // check for end points
    // now adjust min and max angles
    float firstSearchSelectedAngle = angleSelect;
    float minAngle = max(-10.f, firstSearchSelectedAngle - 0.9f); // keep search inside bounds, have already searched the whole number angles
    maxAngle = min(10.1f, firstSearchSelectedAngle + 0.94f); // slightly over .9 so we search angle +.9 but don't search angle +1 (already checked)
    angleStep = 0.1f; // increase search granularity
    currentAngle = minAngle; // for now, don't worry about checking
    // run the search again
    const static float EPSILON = 0.00001f;
    while (currentAngle <= maxAngle)
    {
        if((abs(currentAngle - firstSearchSelectedAngle) < EPSILON) ) {
            currentAngle += angleStep;
            continue;
        }
        rorate_image(combo_centered, mask_rotate, currentAngle, imgCenterX, imgCenterY);

        cv::Mat leftHalf = mask_rotate(cv::Rect(0, 0, imgCenterXInt, scrHeight));
        cv::Mat rightHalf = mask_rotate(cv::Rect(imgCenterXInt, 0, scrWidth - imgCenterXInt, scrHeight));

        cv::Mat leftLine, leftLine2;
        cv::Mat rightLine, rightLine2;
        cv::Mat lrMul;

        cv::reduce(leftHalf, leftLine, 1, cv::REDUCE_SUM, CV_32F);
        cv::reduce(rightHalf, rightLine, 1, cv::REDUCE_SUM, CV_32F);

        cv::multiply(leftLine, rightLine, lrMul);
        cv::multiply(leftLine, leftLine, leftLine2);
        cv::multiply(rightLine, rightLine, rightLine2);

        double lrMulSum = cv::sum(lrMul)[0];
        double leftLineSum = cv::sum(leftLine2)[0];
        double rightLineSum = cv::sum(rightLine2)[0];

        float sim_cos = (float) (lrMulSum / ( sqrt(leftLineSum) * sqrt(rightLineSum) ) );

        if (sim_cos > maxSimCos)
        {
            angleSelect = currentAngle;
            maxSimCos = sim_cos;
        }

        currentAngle += angleStep;
    }
    return angleSelect;
}

/// For verification we're going to attempt to remove all physical and display space transformations
BVSegmentation BladderSegmentationTask::estimateCaliperLocations(uint32_t frameNum, BVType viewType, Json::Value& verification, void* jni) {
    // estimate caliper location
    BVSegmentation caliperLocs;

    // need to have segmentation mask. - re-calculate
    // scan converted image
    BladderSegmentationJNI* bJni = static_cast<BladderSegmentationJNI*>(mJNI);
    cv::Size outSize = {BLADDER_SEG_IMG_WIDTH, BLADDER_SEG_IMG_HEIGHT};
    mUSManager->getAIManager().getScanConvertedFrame(frameNum, BLADDER_SEG_IMG_WIDTH, BLADDER_SEG_IMG_HEIGHT, (mScanConvertedFrame.data()));
    cv::Mat inputMat = {outSize, CV_32FC1, bJni->imageBufferNative.data()};
    cv::Mat inputCharMat = {outSize, CV_8UC1, mScanConvertedFrame.data()};
    inputCharMat.convertTo(inputMat, CV_32FC1, 1.0/255.0);
    cv::Mat probMapMean(outSize, CV_32FC2);
    cv::Mat segMap(outSize, CV_8UC1);

    JNIEnv* jniEnv = static_cast<JNIEnv*>(jni);
    // processFrame no longer includes softmax operations
    for (uint32_t modelIdx = 0; modelIdx < BLADDER_SEGMENTATION_ENSEMBLE_SIZE; modelIdx++) {
        jniEnv->CallVoidMethod(bJni->instance, bJni->process, bJni->imageBuffer, bJni->viewBuffer, bJni->pMapBuffer, modelIdx);
        memcpy(mProbMap[modelIdx].ptr<float>(), bJni->pMapBufferNative.data(), sizeof(float) * BLADDER_SEG_IMG_WIDTH * BLADDER_SEG_IMG_HEIGHT * 2);
        //processPMap(bJni->pMapBufferNative.data(), mProbMap[modelIdx].ptr<float>(), BLADDER_SEG_IMG_WIDTH, BLADDER_SEG_IMG_HEIGHT);
        twoChannelSoftmax(mProbMap[modelIdx], mProbMap[modelIdx]);
    }
    
    // Mean - probMap
    probMapMean = mProbMap[0] + mProbMap[1] + mProbMap[2] + mProbMap[3];
    probMapMean = probMapMean * 0.25;
    
    // flatten
    flatten(probMapMean, segMap);
         
    // set to zeros to segmentation mask
    mSegMask.setTo(cv::Scalar::all(0));
    mMaskArea = 0;
    
    cv::Mat labels(outSize, CV_8UC1);    //output
    cv::Mat stats;
    cv::Mat centroids;
    
    float segMaskWidth = 500.0f;
    float segMaskHeight = 500.0f;
                
    int nb_components = cv::connectedComponentsWithStats(segMap, labels, stats, centroids, 4, CV_16U);
    
    // only 1 component
    if (nb_components < 2)
    {
        // TODO: 1 component case
        // ignore
    }
    else
    {
        // nb compts >= 2
        int curVal;
        int max1, max2;
        int max1Idx, max2Idx;
        max1 = -10;
        max2 = -20;
        max1Idx = 0;
        max2Idx = 0;
        
        for (int i = 0; i < nb_components; i++)
        {
            curVal =  stats.at<int>(i, cv::CC_STAT_AREA);
            
            if (curVal > max1)
            {
                // update 2nd max
                max2 = max1;
                max2Idx = max1Idx;
                
                // update the max
                max1 = curVal;
                max1Idx = i;
            }
            else if (curVal > max2)
            {
                // update 2nd max
                max2 = curVal;
                max2Idx = i;
            }
        }

        // IOS-2776.
        // check whether max2 can be background. > 95% of width and height
        if (stats.at<int>(max2Idx, cv::CC_STAT_WIDTH) > BLADDER_SEG_IMG_WIDTH * 0.95f
            && stats.at<int>(max2Idx, cv::CC_STAT_HEIGHT) > BLADDER_SEG_IMG_HEIGHT * 0.95f)
        {
            max2 = max1;
            max2Idx = max1Idx;
        }
                    
        mMaskArea = max2;
        
        // apply min filter - min area should be 100 - values set to 0 or 255
        cv::inRange(labels, cv::Scalar(max2Idx), cv::Scalar(max2Idx), mSegMask);
        
        // centroid of identified index - defined on 128 x 128 (BLADDER_SEG_IMG_WIDTH & HEIGHT)
        mCentroidX = centroids.at<double>(max2Idx, 0);
        mCentroidY = centroids.at<double>(max2Idx, 1);
        
        // Segmentation Mask Width & Height
        segMaskWidth = stats.at<int>(max2Idx, cv::CC_STAT_WIDTH);
        segMaskHeight = stats.at<int>(max2Idx, cv::CC_STAT_HEIGHT);
    }
    
    // convert centroidX and Y to Physical coordinate
    // convertToPhysicalSpace(mFlipX);      -- ignore as we don't need a centroid
    
    // apply hole-fill on mSegMask
    // Contours
    std::vector<std::vector<cv::Point>> preContours;
    //mSegMask = mSegMask / 255;
    cv::findContours(mSegMask, preContours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);
    
    int preContours_MaxIdx = 0;
    float maxArea = -1000000.0f;
    
    // find max area
    if (preContours.size() > 1) {
        for (int i = 0; i < preContours.size(); i++)
        {
            float curArea = abs(cv::contourArea(preContours[i]));
            
            if (curArea > maxArea) {
                maxArea = curArea;
                preContours_MaxIdx = i;
            }
        }
    }
    cv::Mat newMask(outSize, CV_8UC1);
    newMask.setTo(cv::Scalar::all(0));
    cv::drawContours(newMask, preContours, preContours_MaxIdx,  cv::Scalar(255,255,255), cv::FILLED);
    // updated segmentation mask with 0/255
    cv::threshold(newMask, mSegMask, 127, 255, 0);
    
    // screen display size
    int scrWidth = mImguiRenderer.getWidth();
    int scrHeight = mImguiRenderer.getHeight();
    // scaling to screen size
    segMaskWidth *= scrWidth/BLADDER_SEG_IMG_WIDTH;
    segMaskHeight *= scrHeight/BLADDER_SEG_IMG_HEIGHT;
    
    cv::Size scrSize = {scrWidth, scrHeight};
            
    // scaled Image to U8
    cv::Mat segMaskU8(scrSize, CV_8UC1);
    cv::Mat segMaskU8T(scrSize, CV_8UC1);
    
    // resize
    cv::resize(mSegMask, segMaskU8, scrSize);
    
    // resized/scaled Image Threshold
    cv::threshold(segMaskU8, segMaskU8T, 127, 255, 0);
    
    // segmentation Mask filtered
    cv::Mat segMaskSM(scrSize, CV_8UC1);
    cv::Mat segMaskSMT(scrSize, CV_8UC1);
    
    // Gaussian Filter
    cv::GaussianBlur(segMaskU8T, segMaskSM, cv::Size(41, 41), 5);
    
    // Threshold
    cv::threshold(segMaskSM, segMaskSMT, 127, 255, 0);
    
    // store segmentation mask (for verification, regardless of define)
    segMaskU8T.copyTo(mSegMaskSelected);
    
    
    // Contours
    std::vector<std::vector<cv::Point>> contours;
    //segMaskSMT = (segMaskSMT / 255);
    cv::findContours(segMaskSMT, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);
    
    // Contour - when we have multiple contours
    int contours_MaxIdx = 0;
    maxArea = -1000000.0f;
    
    // find max area
    if (contours.size() > 1) {
        for (int i = 0; i < contours.size(); i++)
        {
            float curArea = abs(cv::contourArea(contours[i]));
            
            if (curArea > maxArea) {
                maxArea = curArea;
                contours_MaxIdx = i;
            }
        }
    }

    // TODO: update center of mass equation
    // calculating moments -> center of mass based on scaled up imaged
    //auto mom = cv::moments(contours[contours_MaxIdx]);
    auto mom = cv::moments(segMaskSMT);
    
    float cx = mom.m10/mom.m00;
    float cy = mom.m01/mom.m00;
        
    mCentroidX = cx;
    mCentroidY = cy;
    
    // Major Axis
    float length;
    float majorAngle, minorAngle;
    
    // length
    length = sqrt(segMaskWidth * segMaskWidth + segMaskHeight * segMaskHeight);       // 2.0 for extra length
         
    if (viewType == BVType::SAGITTAL)
    {
        // Sagittal - after Threshold
        majorAngle = estimateMaskAngleSagittalWithMask(frameNum, segMaskSMT, length, verification);
    }
    else
    {
        // Transverse - testing with post-threshold value
        majorAngle = estimateMaskAngleTransverse(frameNum, segMaskSMT, verification) + 90.0f;
    }

    // minor angle = + 90
    minorAngle = majorAngle + 90.0f;
        
    // caliper points
    cv::Point2f majorStartPt, majorEndPt, minorStartPt, minorEndPt;
        
    {
        // using mask and line draw method
        cv::Point majorStartPti, majorEndPti;
        estimateIntersectionWithMask(segMaskSMT, majorAngle, length, mCentroidX, mCentroidY, majorStartPti, majorEndPti);
        
        majorStartPt = (cv::Point2f) majorStartPti;
        majorEndPt = (cv::Point2f) majorEndPti;
    }
        
    {
        // using mask and line draw method
        cv::Point minorStartPti, minorEndPti;
        estimateIntersectionWithMask(segMaskSMT, minorAngle, length, mCentroidX, mCentroidY, minorStartPti, minorEndPti);
        
        minorStartPt = (cv::Point2f) minorStartPti;
        minorEndPt = (cv::Point2f) minorEndPti;
    }
            
    // converting to Physical Coordinate
    cv::Point2f majorStartPtPhy, majorEndPtPhy, minorStartPtPhy, minorEndPtPhy;
    
    //convertToPhysicalSpaceFromDisplayCoord(mFlipX, majorStartPt, majorStartPtPhy);
    //convertToPhysicalSpaceFromDisplayCoord(mFlipX, majorEndPt, majorEndPtPhy);
    //convertToPhysicalSpaceFromDisplayCoord(mFlipX, minorStartPt, minorStartPtPhy);
    //convertToPhysicalSpaceFromDisplayCoord(mFlipX, minorEndPt, minorEndPtPhy);
        
    // Major and Minor points in phyiscal coordinate
    caliperLocs.majorCaliper.startPoint.x = majorStartPt.x;
    caliperLocs.majorCaliper.startPoint.y = majorStartPt.y;
    
    caliperLocs.majorCaliper.endPoint.x = majorEndPt.x;
    caliperLocs.majorCaliper.endPoint.y = majorEndPt.y;
    
    caliperLocs.minorCaliper.startPoint.x = minorStartPt.x;
    caliperLocs.minorCaliper.startPoint.y = minorStartPt.y;
    
    caliperLocs.minorCaliper.endPoint.x = minorEndPt.x;
    caliperLocs.minorCaliper.endPoint.y = minorEndPt.y;
    
    // contour
    // clear the vector
    caliperLocs.contour.clear();
    
    // not converting to physical space for verification
    
    // TODO - comment this line to not draw segmentation mask in caliper screen
    mCalculated = true;
    
    return caliperLocs;
}

BVSegmentation BladderSegmentationTask::estimateCaliperLocations(uint32_t frameNum, BVType viewType) {
    // estimate caliper location
    BVSegmentation caliperLocs;
    // need to have segmentation mask. - re-calculate
    // scan converted image
    BladderSegmentationJNI* bJni = static_cast<BladderSegmentationJNI*>(mJNI);
    cv::Size outSize = {BLADDER_SEG_IMG_WIDTH, BLADDER_SEG_IMG_HEIGHT};
    mUSManager->getAIManager().getScanConvertedFrame(frameNum, BLADDER_SEG_IMG_WIDTH, BLADDER_SEG_IMG_HEIGHT, (mScanConvertedFrame.data()));
    cv::Mat inputMat = {outSize, CV_32FC1, bJni->imageBufferNative.data()};
    cv::Mat inputCharMat = {outSize, CV_8UC1, mScanConvertedFrame.data()};
    inputCharMat.convertTo(inputMat, CV_32FC1, 1.0/255.0);
    cv::Mat probMapMean(outSize, CV_32FC2);
    cv::Mat segMap(outSize, CV_8UC1);
    // processFrame no longer includes SoftMax operations as well
    for (uint32_t modelIdx = 0; modelIdx < BLADDER_SEGMENTATION_ENSEMBLE_SIZE; modelIdx++) {
        bJni->jenv->CallVoidMethod(bJni->instance, bJni->process, bJni->imageBuffer, bJni->viewBuffer, bJni->pMapBuffer, modelIdx);
        memcpy(mProbMap[modelIdx].ptr<float>(), bJni->pMapBufferNative.data(), sizeof(float) * BLADDER_SEG_IMG_WIDTH * BLADDER_SEG_IMG_HEIGHT * 2);
        //processPMap(bJni->pMapBufferNative.data(), mProbMap[modelIdx].ptr<float>(), BLADDER_SEG_IMG_WIDTH, BLADDER_SEG_IMG_HEIGHT);
        twoChannelSoftmax(mProbMap[modelIdx], mProbMap[modelIdx]);
    }
    
    // Mean - probMap
    probMapMean = mProbMap[0] + mProbMap[1] + mProbMap[2] + mProbMap[3];
    probMapMean = probMapMean * 0.25;
    
    // flatten
    flatten(probMapMean, segMap);
         
    // set to zeros to segmentation mask
    mSegMask.setTo(cv::Scalar::all(0));
    mMaskArea = 0;
    
    cv::Mat labels(outSize, CV_8UC1);    //output
    cv::Mat stats;
    cv::Mat centroids;
    
    float segMaskWidth = 500.0f;
    float segMaskHeight = 500.0f;
                
    int nb_components = cv::connectedComponentsWithStats(segMap, labels, stats, centroids, 4, CV_16U);
    
    // only 1 component
    if (nb_components < 2)
    {
        // TODO: 1 component case
        // ignore

    }
    else
    {
        // nb compts >= 2
        int curVal;
        int max1, max2;
        int max1Idx, max2Idx;
        max1 = -10;
        max2 = -20;
        max1Idx = 0;
        max2Idx = 0;
        
        for (int i = 0; i < nb_components; i++)
        {
            curVal =  stats.at<int>(i, cv::CC_STAT_AREA);
            
            if (curVal > max1)
            {
                // update 2nd max
                max2 = max1;
                max2Idx = max1Idx;
                
                // update the max
                max1 = curVal;
                max1Idx = i;
            }
            else if (curVal > max2)
            {
                // update 2nd max
                max2 = curVal;
                max2Idx = i;
            }
        }

        // IOS-2776.
        // check whether max2 can be background. > 95% of width and height
        if (stats.at<int>(max2Idx, cv::CC_STAT_WIDTH) > BLADDER_SEG_IMG_WIDTH * 0.95f
            && stats.at<int>(max2Idx, cv::CC_STAT_HEIGHT) > BLADDER_SEG_IMG_HEIGHT * 0.95f)
        {
            max2 = max1;
            max2Idx = max1Idx;
        }
                    
        mMaskArea = max2;

        // apply min filter - min area should be 100 - values set to 0 or 255
        cv::inRange(labels, cv::Scalar(max2Idx), cv::Scalar(max2Idx), mSegMask);
        
        // centroid of identified index - defined on 128 x 128 (BLADDER_SEG_IMG_WIDTH & HEIGHT)
        mCentroidX = centroids.at<double>(max2Idx, 0);
        mCentroidY = centroids.at<double>(max2Idx, 1);
        
        // Segmentation Mask Width & Height
        segMaskWidth = stats.at<int>(max2Idx, cv::CC_STAT_WIDTH);
        segMaskHeight = stats.at<int>(max2Idx, cv::CC_STAT_HEIGHT);
    }
    
    // convert centroidX and Y to Physical coordinate
    // convertToPhysicalSpace(mFlipX);      -- ignore as we don't need a centroid
    
    // apply hole-fill on mSegMask
    // Contours

    std::vector<std::vector<cv::Point>> preContours;
    //mSegMask = mSegMask / 255;
    cv::findContours(mSegMask, preContours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

    int preContours_MaxIdx = 0;
    float maxArea = -1000000.0f;
    
    // find max area
    if (preContours.size() > 1) {
        for (int i = 0; i < preContours.size(); i++)
        {
            float curArea = abs(cv::contourArea(preContours[i]));
            
            if (curArea > maxArea) {
                maxArea = curArea;
                preContours_MaxIdx = i;
            }
        }
    }

    cv::Mat newMask(outSize, CV_8UC1);
    newMask.setTo(cv::Scalar::all(0));
    cv::drawContours(newMask, preContours, preContours_MaxIdx,  cv::Scalar(255,255,255), cv::FILLED);
    // updated segmentation mask with 0/255
    cv::threshold(newMask, mSegMask, 127, 255, 0);

    // screen display size
    int scrWidth = mImguiRenderer.getWidth();
    int scrHeight = mImguiRenderer.getHeight();
    
    // scaling to screen size
    segMaskWidth *= scrWidth/BLADDER_SEG_IMG_WIDTH;
    segMaskHeight *= scrHeight/BLADDER_SEG_IMG_HEIGHT;
    
    cv::Size scrSize = {scrWidth, scrHeight};
            
    // scaled Image to U8
    cv::Mat segMaskU8(scrSize, CV_8UC1);
    cv::Mat segMaskU8T(scrSize, CV_8UC1);
    
    // resize
    cv::resize(mSegMask, segMaskU8, scrSize);
    
    // resized/scaled Image Threshold
    cv::threshold(segMaskU8, segMaskU8T, 127, 255, 0);
    // segmentation Mask filtered
    cv::Mat segMaskSM(scrSize, CV_8UC1);
    cv::Mat segMaskSMT(scrSize, CV_8UC1);

    // Gaussian Filter
    cv::GaussianBlur(segMaskU8T, segMaskSM, cv::Size(41, 41), 5);

    // Threshold
    cv::threshold(segMaskSM, segMaskSMT, 127, 255, 0);

    
#if BUILD_FOR_CLINICAL_STUDY
    // store segmentation mask
    segMaskU8T.copyTo(mSegMaskSelected);
#endif
    
    
    // Contours
    std::vector<std::vector<cv::Point>> contours;
    // segMaskSMT = segMaskSMT / 255;
    cv::findContours(segMaskSMT, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

    // Contour - when we have multiple contours
    int contours_MaxIdx = 0;
    maxArea = -1000000.0f;
    
    // find max area
    if (contours.size() > 1) {
        for (int i = 0; i < contours.size(); i++)
        {
            float curArea = abs(cv::contourArea(contours[i]));
            
            if (curArea > maxArea) {
                maxArea = curArea;
                contours_MaxIdx = i;
            }
        }
    }

    // TODO: update center of mass equation
    // calculating moments -> center of mass based on scaled up imaged
    //auto mom = cv::moments(contours[contours_MaxIdx]);
    auto mom = cv::moments(segMaskSMT);
    
    float cx = mom.m10/mom.m00;
    float cy = mom.m01/mom.m00;
        
    mCentroidX = cx;
    mCentroidY = cy;
    
    // Major Axis
    float length;
    float majorAngle, minorAngle;
    
    // length
    length = sqrt(segMaskWidth * segMaskWidth + segMaskHeight * segMaskHeight);       // 2.0 for extra length

    if (viewType == BVType::SAGITTAL)
    {
        // Sagittal - after Threshold
        majorAngle = estimateMaskAngleSagittalWithMask(frameNum, segMaskSMT, length);
    }
    else
    {
        // Transverse - before Treshold
        majorAngle = estimateMaskAngleTransverse(frameNum, segMaskSM) + 90.0f;
    }

    // minor angle = + 90
    minorAngle = majorAngle + 90.0f;
        
    // caliper points
    cv::Point2f majorStartPt, majorEndPt, minorStartPt, minorEndPt;

    {
        // using mask and line draw method
        cv::Point majorStartPti, majorEndPti;
        estimateIntersectionWithMask(segMaskSMT, majorAngle, length, mCentroidX, mCentroidY, majorStartPti, majorEndPti);
        
        majorStartPt = (cv::Point2f) majorStartPti;
        majorEndPt = (cv::Point2f) majorEndPti;
    }

    {
        // using mask and line draw method
        cv::Point minorStartPti, minorEndPti;
        estimateIntersectionWithMask(segMaskSMT, minorAngle, length, mCentroidX, mCentroidY, minorStartPti, minorEndPti);
        
        minorStartPt = (cv::Point2f) minorStartPti;
        minorEndPt = (cv::Point2f) minorEndPti;
    }

    // converting to Physical Coordinate
    cv::Point2f majorStartPtPhy, majorEndPtPhy, minorStartPtPhy, minorEndPtPhy;
    
    convertToPhysicalSpaceFromDisplayCoord(mFlipX, majorStartPt, majorStartPtPhy);
    convertToPhysicalSpaceFromDisplayCoord(mFlipX, majorEndPt, majorEndPtPhy);
    convertToPhysicalSpaceFromDisplayCoord(mFlipX, minorStartPt, minorStartPtPhy);
    convertToPhysicalSpaceFromDisplayCoord(mFlipX, minorEndPt, minorEndPtPhy);
        
    // Major and Minor points in phyiscal coordinate
    caliperLocs.majorCaliper.startPoint.x = majorStartPtPhy.x;
    caliperLocs.majorCaliper.startPoint.y = majorStartPtPhy.y;
    
    caliperLocs.majorCaliper.endPoint.x = majorEndPtPhy.x;
    caliperLocs.majorCaliper.endPoint.y = majorEndPtPhy.y;
    
    caliperLocs.minorCaliper.startPoint.x = minorStartPtPhy.x;
    caliperLocs.minorCaliper.startPoint.y = minorStartPtPhy.y;
    
    caliperLocs.minorCaliper.endPoint.x = minorEndPtPhy.x;
    caliperLocs.minorCaliper.endPoint.y = minorEndPtPhy.y;
    
    // contour
    // clear the vector
    caliperLocs.contour.clear();

    if (contours.size() > 0 && contours[contours_MaxIdx].size() > 0) {
        convertToPhysicalSpaceFromDisplayCoord(mFlipX, contours[contours_MaxIdx], caliperLocs.contour);
    }
    
    // TODO - comment this line to not draw segmentation mask in caliper screen
    mCalculated = true;
    return caliperLocs;
}

float BladderSegmentationTask::estimateMaskAngleSagittal(std::vector<cv::Point> contour, cv::InputArray segMask, float refMaskLength)
{
    float angleSelect = 0.0f;
    std::vector<float> angle_list;
    float currentAngle;
    float maxDistance = -100000.0f;
    
    // angle list
    for (int i = -80; i <= -30; i++)
        angle_list.push_back((float) i);
    
    for (int i = 30; i <= 80; i++)
        angle_list.push_back((float) i);
    
    
    for (auto it = angle_list.begin(); it != angle_list.end(); it++)
    {
        currentAngle = *it;
        
        cv::Point2f startPt, endPt;
        
        if (!estimateIntersectionWithContour(contour, currentAngle, mCentroidX, mCentroidY, startPt, endPt))
        {
            // When contour method does not work.
            cv::Point startPti, endPti;
            estimateIntersectionWithMask(segMask, currentAngle, refMaskLength, mCentroidX, mCentroidY, startPti, endPti);
            
            startPt = (cv::Point2f) startPti;
            endPt = (cv::Point2f) endPti;
        }
          
        float deltaX = startPt.x - endPt.x;
        float deltaY = startPt.y - endPt.y;
        
        float curDistance = sqrt(deltaX * deltaX + deltaY * deltaY);
        
        if (curDistance > maxDistance)
        {
            angleSelect = currentAngle;
            maxDistance = curDistance;
        }
    }
    
    return angleSelect;
}

float BladderSegmentationTask::estimateMaskAngleSagittalWithMask(uint32_t frameNum, cv::InputArray segMask, float refMaskLength) {
    float angleSelect = 0.0f;
    float currentAngle;
    float maxDistance = -100000.0f;
    float length = refMaskLength;
    float currentDistance;
    
    std::vector<float> angle_list;
    
    // angle list
    for (int i = -80; i <= -30; i++)
        angle_list.push_back((float) i);
    
    for (int i = 30; i <= 80; i++)
        angle_list.push_back((float) i);
    
    // screen display size
    int scrWidth = mImguiRenderer.getWidth();
    int scrHeight = mImguiRenderer.getHeight();
    
    cv::Size scrSize = {scrWidth, scrHeight};
    cv::Mat segMaskSMT2 = segMask.getMat();
    
    for (auto it = angle_list.begin(); it != angle_list.end(); it++)
    {
        currentAngle = *it;
        
        // Line mask
        cv::Mat lineMask(scrSize, CV_8UC1, cv::Scalar(0));
            
        cv::Point2f startPt, endPt;
        
        // major line
        startPt.x = mCentroidX - sin(currentAngle / 180.0 * PI) * length;
        startPt.y = mCentroidY + cos(currentAngle / 180.0 * PI) * length;
        
        endPt.x = mCentroidX - sin( (currentAngle + 180) / 180.0 * PI) * length;
        endPt.y = mCentroidY + cos( (currentAngle + 180) / 180.0 * PI) * length;
        
        // draw lines on mask
        // Major
        cv::line(lineMask, startPt, endPt, cv::Scalar(250,250,250), 1);
        cv::threshold(lineMask, lineMask, 127, 255, 0);
            
        // bitwise_and
        cv::bitwise_and(segMaskSMT2, lineMask, lineMask);
        // find largest component of intersection 11/12/24
        cv::Mat lineMaskLca(scrSize, CV_8UC1, cv::Scalar(0));
        find_mask_largest_component(lineMask, lineMaskLca);
        // Find non-zero points
        cv::Mat nonZero;
        cv::findNonZero(lineMaskLca, nonZero);
        {
            // find min and max points
            float refVecX, refVecY;
            float minVal = 100.0;
            float maxVal = -100.0;
            float curVal;
            float curVecX, curVecY;
            
            // major axis
            refVecX = (startPt.x - endPt.x) / length / 2.0f;
            refVecY = (startPt.y - endPt.y) / length / 2.0f;
            
            for (int i = 0; i < nonZero.total(); i++) {
                cv::Point curPt = nonZero.at<cv::Point>(i);
                
                curVecX = curPt.x - mCentroidX;
                curVecY = curPt.y - mCentroidY;
                
                curVal = curVecX * refVecX + curVecY * refVecY;
                
                if (curVal > maxVal) {
                    maxVal = curVal;
                }
                if (curVal < minVal) {
                    minVal = curVal;
                }
            }
            
            currentDistance = maxVal - minVal;
        }
        
        if (currentDistance > maxDistance)
        {
            maxDistance = currentDistance;
            angleSelect = currentAngle;
        }
    }
    
    return angleSelect;
}

bool BladderSegmentationTask::estimateIntersectionWithContour(std::vector<cv::Point> contour, float angleInDegree, float centX, float centY, cv::Point2f &outPt1, cv::Point2f &outPt2)
{
    bool retVal = false;
    bool foundPt1 = false;      // max Value
    bool foundPt2 = false;
    
    auto contourSize = contour.size();
    
    float angleVecX = -sin(angleInDegree / 180.0 * PI);
    float angleVecY = cos(angleInDegree / 180.0 * PI);
    
    float currentVecX, currentVecY;
    float minVal = 10000000.0f;
    float maxVal = -10000000.0f;
    int minIdx = 0;
    int maxIdx = 0;
    float innerProd;
    float normTerm;
       
    for (int i = 0; i < contourSize; i++)
    {
        cv::Point2f currentPt;
        currentPt = contour.at(i);
        
        currentVecX = currentPt.x - centX;
        currentVecY = currentPt.y - centY;
        
        normTerm = sqrt(currentVecX * currentVecX + currentVecY * currentVecY);
        
        if (normTerm <= 0.0f)
            continue;
        
        // normalize
        currentVecX /= normTerm;
        currentVecY /= normTerm;
        
        // inner product
        innerProd = currentVecX * angleVecX + currentVecY * angleVecY;
        
        if (innerProd > maxVal)
        {
            maxVal = innerProd;
            maxIdx = i;
        }
        
        if (innerProd < minVal)
        {
            minVal = innerProd;
            minIdx = i;
        }
    }
    
    // Centroid Point
    cv::Point2f centroid(centX, centY);
    cv::Point2f peakPoint, adjPoint;
    cv::Point2f currentPoint;
    cv::Point2f outPoint;
    int currentIdx;
    
    // max value - aligned with angleVec
    peakPoint = contour.at(maxIdx);
    currentIdx = (maxIdx-1) % contourSize;
    adjPoint = contour.at(currentIdx);
    
    if ( findCrossSection(centroid, angleVecX, angleVecY, peakPoint, adjPoint, outPoint) )
    {
        outPt1 = outPoint;
        foundPt1 = true;
    }
    else
    {
        currentIdx = (maxIdx+1) % contourSize;
        adjPoint = contour.at(currentIdx);
        
        if ( findCrossSection(centroid, angleVecX, angleVecY, peakPoint, adjPoint, outPoint) )
        {
            outPt1 = outPoint;
            foundPt1 = true;
        }
    }
    
    
    ////////////////
    // min value - alinged with -angleVec
    peakPoint = contour.at(minIdx);
    currentIdx = (minIdx-1) % contourSize;
    adjPoint = contour.at(currentIdx);
    
    if ( findCrossSection(centroid, angleVecX, angleVecY, peakPoint, adjPoint, outPoint) )
    {
        outPt2 = outPoint;
        foundPt2 = true;
    }
    else
    {
        currentIdx = (minIdx+1) % contourSize;
        adjPoint = contour.at(currentIdx);
        
        if ( findCrossSection(centroid, angleVecX, angleVecY, peakPoint, adjPoint, outPoint) )
        {
            outPt2 = outPoint;
            foundPt2 = true;
        }
    }
    
    if (foundPt1 & foundPt2)
        retVal = true;
        
    
    return retVal;
}

bool BladderSegmentationTask::findCrossSection(cv::Point2f centroid, float vectorX, float vectorY, cv::Point2f peakPt, cv::Point2f adjPt, cv::Point2f& outPt)
{
    bool retVal = false;
    // Angle Vector
    float Ax, Ay;
    // Centroid
    float Cx, Cy;
    // Peak Point
    float Px, Py;
    // Vector from Peak
    float Vx, Vy;
    
    Ax = vectorX;
    Ay = vectorY;
    Cx = centroid.x;
    Cy = centroid.y;
    
    Px = peakPt.x;
    Py = peakPt.y;
    
    Vx = adjPt.x - Px;
    Vy = adjPt.y - Py;
    
    float beta, bottom;
    
    bottom = Ay*Vx - Ax*Vy;
    
    if (bottom == 0.0f) {
        ALOGD("FindCrossSection - Bottom is Zero");
        return retVal;
    }
    
    beta = ( Ay * (Cx - Px) + Ax * (Py - Cy) ) / bottom;
        
    if (beta < 0.0f || beta > 1.0f)
    {
        // out of range
        ALOGD("FindCrossSection - out of range");
        return retVal;
    }
    
    outPt.x = Px + Vx * beta;
    outPt.y = Py + Vy * beta;
    
    retVal = true;
    
    return retVal;
}
void BladderSegmentationTask::find_mask_largest_component(const cv::Mat& input, cv::Mat& output)
{
    if(cv::sum(input)[0] == 0)
    {
        output = input.clone();
        return;
    }
    output = cv::Mat::zeros(input.size(), input.type());

    cv::Mat labels(input.size(), CV_8UC1);    //output
    cv::Mat stats;
    cv::Mat centroids;

    int nb_components = cv::connectedComponentsWithStats(input, labels, stats, centroids, 8, CV_16U);

    // nb compts >= 2
    int curVal;
    int max1, max2;
    int max1Idx, max2Idx;
    max1 = -10;
    max2 = -20;
    max1Idx = 0;
    max2Idx = 0;
    
    for (int i = 0; i < nb_components; i++)
    {
        curVal =  stats.at<int>(i, cv::CC_STAT_AREA);
        
        if (curVal > max1)
        {
            // update 2nd max
            max2 = max1;
            max2Idx = max1Idx;
            
            // update the max
            max1 = curVal;
            max1Idx = i;
        }
        else if (curVal > max2)
        {
            // update 2nd max
            max2 = curVal;
            max2Idx = i;
        }
    }
    // apply min filter - min area should be 100 - this set values to 0 or 255
    cv::inRange(labels, cv::Scalar(max2Idx), cv::Scalar(max2Idx), output);
    if(false)//nb_components > 2)
    {
        SaveMap(input, "lca_processing_input", mMapFrameNum, mMapFilePath, mClipname);
        SaveMap(output, "lca_processing_output", mMapFrameNum, mMapFilePath, mClipname);
    }
    
    ///SaveMap(screenImageRGBA, mMapName, mMapFrameNum, mMapFilePath, mClipname);
    /*
     int largestLabel = 0;
     int largestArea = 0;
     for (int i = 1; i < numLabels; i++) { // Start from 1 to skip background (label 0)
         int area = stats.at<int>(i, CC_STAT_AREA);
         if (area > largestArea) {
             largestArea = area;
             largestLabel = i;
         }
     }

         // Create an image with only the largest component
         Mat largestComponent = Mat::zeros(binaryImage.size(), CV_8UC1);
         for (int y = 0; y < labels.rows; y++) {
             for (int x = 0; x < labels.cols; x++) {
                 if (labels.at<int>(y, x) == largestLabel) {
                     largestComponent.at<uchar>(y, x) = 255;
                 }
             }
         }
     */
}
bool BladderSegmentationTask::estimateIntersectionWithMask(cv::InputArray segMaskArray, float angleInDegreee, float length, float centX, float centY, cv::Point& outPt1, cv::Point& outPt2)
{
    bool retVal = false;
   
    // Segmentation Mask
    cv::Mat segMask = segMaskArray.getMat();
    cv::Size size = segMask.size();
    
    cv::Mat lineMask(size, CV_8UC1, cv::Scalar(0));
    cv::Point startPt, endPt;
        
    // major line
    startPt.x = centX - sin(angleInDegreee / 180.0 * PI) * length;
    startPt.y = centY + cos(angleInDegreee / 180.0 * PI) * length;
    
    endPt.x = centX - sin( (angleInDegreee + 180) / 180.0 * PI) * length;
    endPt.y = centY + cos( (angleInDegreee + 180) / 180.0 * PI) * length;
    
    // draw lines on mask
    cv::line(lineMask, startPt, endPt, cv::Scalar(250,250,250), 1);
    cv::threshold(lineMask, lineMask, 127, 255, 0);
            
    // bitwise_and
    cv::bitwise_and(segMask, lineMask, lineMask);
    // find largest component of intersection 11/12/24
    cv::Mat lineMaskLca(size, CV_8UC1, cv::Scalar(0));
    find_mask_largest_component(lineMask, lineMaskLca);
    // Find non-zero points
    cv::Mat nonZeroMap;
    cv::findNonZero(lineMaskLca, nonZeroMap);
        
    // find min and max points
    float refVecX, refVecY;
    float minVal = 100.0;
    float maxVal = -100.0;
    float curVal;
    float curVecX, curVecY;
    
    
    // major axis
    refVecX = (startPt.x - endPt.x) / length / 2.0f;
    refVecY = (startPt.y - endPt.y) / length / 2.0f;
    // check only against the points on the actual line
    startPt.x = centX;
    startPt.y = centY;
    endPt.x = centX;
    endPt.y = centY;
    for (int i = 0; i < nonZeroMap.total(); i++) {
        cv::Point curPt = nonZeroMap.at<cv::Point>(i);
        curVecX = curPt.x - mCentroidX;
        curVecY = curPt.y - mCentroidY;
        
        curVal = curVecX * refVecX + curVecY * refVecY;
        
        if (curVal > maxVal) {
            startPt.x = curPt.x;
            startPt.y = curPt.y;
            
            maxVal = curVal;
        }
        if (curVal < minVal) {
            endPt.x = curPt.x;
            endPt.y = curPt.y;
            minVal = curVal;
        }
    }
    
    outPt1 = startPt;
    outPt2 = endPt;
    
    return retVal;
}

void BladderSegmentationTask::draw(CineViewer &cineviewer, uint32_t frameNum) {
        //LOGD("[ BLADDER ] Drawing Segmentation");
    // disable - drawing bladder segmentation mask if paused
    if (mMaskArea < 100 || (mPaused && !mCalculated)) {
        // ignore - area too small or Paused
        return;
    }
    // background draw list
    ImDrawList *background = ImGui::GetBackgroundDrawList();
    // BGRA8 texture
    uint8_t* texPtr = new uint8_t[BLADDER_SEG_IMG_WIDTH * BLADDER_SEG_IMG_HEIGHT * 4];

    uint8_t curVal;
    // TODO: update color
    uint8_t R = mMaskColorR;
    uint8_t G = mMaskColorG;
    uint8_t B = mMaskColorB;
    uint8_t A = mMaskColorA;
    //set the openGL texture
    for (int h = 0; h < BLADDER_SEG_IMG_HEIGHT; h++) {
        for (int w = 0; w < BLADDER_SEG_IMG_WIDTH; w++) {
            curVal = mSegMask.at<uint8_t>(h, w);

            if (curVal > 1)
                A = mMaskColorA;

            else
                A = 0;

            // BGRA format
            texPtr[h * BLADDER_SEG_IMG_WIDTH * 4 + 4 * w] = R;
            texPtr[h * BLADDER_SEG_IMG_WIDTH * 4 + 4 * w + 1] = G;
            texPtr[h * BLADDER_SEG_IMG_WIDTH * 4 + 4 * w + 2] = B;
            texPtr[h * BLADDER_SEG_IMG_WIDTH * 4 + 4 * w + 3] = A;
        }
    }
    glBindTexture(GL_TEXTURE_2D, mTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mSegMask.cols, mSegMask.rows, GL_RGBA, GL_UNSIGNED_BYTE, texPtr);
    glBindTexture(GL_TEXTURE_2D, 0);
    {
        // drawing set up
        mImageWidth = cineviewer.getImguiRenderer().getWidth();
        mImageHeight = cineviewer.getImguiRenderer().getHeight();
        ImVec2 minXY   = ImVec2(0.0, 0.0);
        ImVec2 maxXY   = ImVec2(mImageWidth, mImageHeight);

        background->AddImage((void*)(intptr_t)mTexture, minXY, maxXY);
    }

    // clean up texPtr;
    delete[] texPtr;
}

void BladderSegmentationTask::startCapture(uint32_t durationInSecond) {
    // TODO: placeholder for starting the capture
}

void BladderSegmentationTask::stopCapture() {
    // TODO: placehomder for stopping the capture
}

void BladderSegmentationTask::attachSegmentationCallback(BladderSegmentationCallback bladderSegmentationCallback) {
    mBladderSegmentationCallback = bladderSegmentationCallback;
}

void BladderSegmentationTask::attachValidRoICallback(BladderRoICallback bladderRoICallback){
    mBladderRoICallback = bladderRoICallback;
}
void BladderSegmentationTask::attachBladderFrameInRoICallBack(BladderFrameInRoICallback bladderRoICallback){
    mBladderFrameInRoICallback = bladderRoICallback;
}
void BladderSegmentationTask::attachContourCallback(BladderContourCallback bladderContourCallback) {
    mBladderContourCallback = bladderContourCallback;
}

void BladderSegmentationTask::setSegmentationParams(float centroidX, float centroidY, float uncertainty, float stability, int area, int frameNumber) {
    mBladderSegmentationCallback(centroidX, centroidY, uncertainty, stability, area, frameNumber);
}

void BladderSegmentationTask::setContourParams(float** contourMap) {
    mBladderContourCallback(contourMap);
}

void BladderSegmentationTask::setSegmentationColor(uint32_t color) {
    uint8_t colorR, colorG, colorB, colorA;
    
    colorR = (0xFF000000 & color) >> 24;
    colorG = (0x00FF0000 & color) >> 16;
    colorB = (0x0000FF00 & color) >> 8;
    colorA = (0x000000FF & color);
        
    mMaskColorR = colorR;
    mMaskColorG = colorG;
    mMaskColorB = colorB;
    mMaskColorA = colorA;
    
}

// TODO: placeholder - remove if not needed.
BVSegmentation BladderSegmentationTask::getSegmentation(int frameNumber) {
    LOGD("getSegmentation, frameNumber: %d", frameNumber);
    // TODO: Logic to get calipers and contour
    BVSegmentation demo;
    // Minor caliper
    demo.minorCaliper.startPoint.x = mCentroidX;
    demo.minorCaliper.startPoint.y = mCentroidY + 20;
    demo.minorCaliper.endPoint.x = mCentroidX;
    demo.minorCaliper.endPoint.y = mCentroidY - 20;
    // Major caliper
    demo.majorCaliper.startPoint.x = mCentroidX + 20;
    demo.majorCaliper.startPoint.y = mCentroidY;
    demo.majorCaliper.endPoint.x = mCentroidX - 20;
    demo.majorCaliper.endPoint.y = mCentroidY;
    
    cv::Point point;
    point.x = mCentroidX + 20;
    point.y = mCentroidY + 20;
    cv::Point point2;
    point2.x = point.x;
    point2.y = point.y - 40;
    cv::Point point3;
    point3.x = point2.x - 40;
    point3.y = point2.y;
    cv::Point point4;
    point4.x = point3.x;
    point4.y = point3.y + 40;
    demo.contour = {point, point2, point3, point4};
    return demo;
}

void BladderSegmentationTask::close() {
    glDeleteTextures(1, &mTexture);
}

void BladderSegmentationTask::openDrawable() {
    glGenTextures(1, &mTexture);
    glBindTexture(GL_TEXTURE_2D, mTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mSegMask.cols, mSegMask.rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
}


void BladderSegmentationTask::onSave(uint32_t startFrame, uint32_t endFrame, echonous::capnp::ThorClip::Builder &builder) {
    /*
#if BUILD_FOR_CLINICAL_STUDY
    
    // TODO: update OrgainID
    // TODO:: Update Capnp schema
    if (mCB->getParams().imagingMode == IMAGING_MODE_B && mCB->getParams().organId == 5) {
        
        auto bladderexam = builder.initBladderexam();
        bladderexam.setModelVersion("TBD");
        
        // segmentation frames
        auto segFramesBuilder = bladderexam.initSegframes();
        
        // CineFrame
        uint32_t numFrames = endFrame-startFrame+1;
        auto framesBuilder = segFramesBuilder.initRawFrames(numFrames);
        auto dataSize = BLADDER_SEG_IMG_WIDTH * BLADDER_SEG_IMG_HEIGHT;
        
        // saving Uncertainty
        auto labeledFrames = bladderexam.initFrames(numFrames);

        for (uint32_t i=0; i != numFrames; i++)
        {
            uint32_t frameNum = i+startFrame;
            
            // Capnp builders
            auto frameBuilder = framesBuilder[i];
            auto frameHeaderBuilder = frameBuilder.getHeader();
            auto frameDataBuilder = frameBuilder.initFrame(dataSize);
            
            // Frame header from the CineBuffer
            CineBuffer::CineFrameHeader *frameHeader = reinterpret_cast<CineBuffer::CineFrameHeader*>(
                                                                                                      mCB->getFrame(frameNum, DAU_DATA_TYPE_B, CineBuffer::FrameContents::IncludeHeader));
            
            frameHeaderBuilder.setFrameNum(frameHeader->frameNum);
            frameHeaderBuilder.setTimeStamp(frameHeader->timeStamp);
            frameHeaderBuilder.setNumSamples(frameHeader->numSamples);
            frameHeaderBuilder.setHeartRate(frameHeader->heartRate);
            frameHeaderBuilder.setStatusCode(frameHeader->statusCode);
            
            // frame data from mSeqMaskSave
            uint8_t *frameData = mSegMaskSave[frameNum % CINE_FRAME_COUNT].ptr<uint8_t>();
            
            // copy data
            std::copy(frameData, frameData+dataSize, frameDataBuilder.begin());
            
            // saving uncertainty
            labeledFrames[i].setFrameNum(frameNum);
            labeledFrames[i].setUncertainty(mUncertaintyValue[frameNum % CINE_FRAME_COUNT]);
        }
        
        // save selected frame Mask - scaled up
        if (!mSegMaskSelected.empty())
        {
            auto selectedMaskSize = mSegMaskSelected.rows * mSegMaskSelected.cols;
            auto selectedMaskDataBuilder = bladderexam.initSelectedFrameMask(selectedMaskSize);
            
            uint8_t *maskData = mSegMaskSelected.ptr<uint8_t>();
            
            // copy data
            std::copy(maskData, maskData + selectedMaskSize, selectedMaskDataBuilder.begin());
        }
    }
    
#endif
     */
}


void BladderSegmentationTask::onLoad(echonous::capnp::ThorClip::Reader &reader) {
    // TODO:
}

void BladderSegmentationTask::twoChannelSoftmax(cv::Mat& input, cv::Mat& output)
{
    const int CHANNEL_COUNT = 2;
    cv::Mat channel_split[CHANNEL_COUNT];
    split(input, channel_split);
    cv::Mat maxElems;
    // should probably generalize parts of this, but two channels for now
    cv::max(channel_split[0], channel_split[1], maxElems);
    cv::Mat expCh[CHANNEL_COUNT];
    cv::Mat expSum;
    channel_split[0] = channel_split[0] - maxElems;
    channel_split[1] = channel_split[1] - maxElems;
    cv::exp(channel_split[0], expCh[0]);
    cv::exp(channel_split[1], expCh[1]);
    // exponentiated by channels
    expSum = expCh[0] + expCh[1];
    expCh[0] = expCh[0] / expSum;
    expCh[1] = expCh[1] / expSum;
    cv::merge(expCh, CHANNEL_COUNT, output);
}
// verification
ThorStatus BladderSegmentationTask::runAllVerification(const char *outputPath)
{
    std::string appPath = std::string(outputPath);
    std::string outPath =  appPath + "/bladder";
    ::mkdir(outPath.c_str(), S_IRWXU | S_IRGRP | S_IROTH);
    // TODO:: Switch to OTS paradigm for file walking verification directories
    /*@autoreleasepool {
        NSStringEncoding encoding = [NSString defaultCStringEncoding];
            NSFileManager* defMan = [NSFileManager defaultManager];
            NSString* ns_appPath = [defMan URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask].lastObject.path;
            NSString* verificationPath = [ns_appPath stringByAppendingString:@"/AI/verification/bladder/"];
            __autoreleasing NSArray<NSString*> *rawFiles = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:verificationPath error:nil];
        
        for (NSString *raw : rawFiles) {
            NSLog(@"[ VERIFY ] Bladder Verify: Found resource file %@", raw);
            NSString* inPath = [verificationPath stringByAppendingString:raw];
            const char *inputPath = [inPath cStringUsingEncoding:encoding];
            // clean up before starting verification test
            // clean up prob maps and seg mask
            mProbMap.clear();
            mProbMapFlt.clear();
            mSegMask.release();
            mFrameCount = 0;
        */
        #if BUILD_FOR_CLINICAL_STUDY
            mSegMaskSave.clear();
            delete[] mUncertaintyValue;
            
            mSegMaskSelected.release();
        #endif
            // mTexture = nil;
            mProcessedFrame = 0xFFFFFFFFu;
            mCineFrame = 0xFFFFFFFFu;
            // run verification
            
            //stop();
            mStop = false;

            mInit = true;
            // TEMP/TODO: Set to always on by default
            mPaused = true;
            mCalculated = false;
            
            // set up prob Map
            mProbMap.resize(BLADDER_SEGMENTATION_ENSEMBLE_SIZE);
            cv::Size outSize = {BLADDER_SEG_IMG_WIDTH, BLADDER_SEG_IMG_HEIGHT};
            // flattened
            mProbMapFlt.resize(BLADDER_SEGMENTATION_ENSEMBLE_SIZE);
            
            for (auto i = 0u; i != mProbMap.size(); i++)
                mProbMap[i] =  cv::Mat(outSize, CV_32FC2);
            
            for (auto i = 0u; i != mProbMapFlt.size(); i++)
                mProbMapFlt[i] = cv::Mat(outSize, CV_8UC1);
            
            // segMask
            mSegMask = cv::Mat(outSize, CV_8UC1);

            // reset FrameCount
            mFrameCount = 0;
            
            // for clinical study - save segmentation mask
        #if BUILD_FOR_CLINICAL_STUDY
            mSegMaskSave.resize(CINE_FRAME_COUNT);
            
            for (auto i = 0u; i != CINE_FRAME_COUNT; i++)
                mSegMaskSave[i] = cv::Mat(outSize, CV_8UC1);
            
            mUncertaintyValue = new float[CINE_FRAME_COUNT];
        #endif
            
            //auto l = lock(mMutex);
          //  mCB->registerCallback(this);
          //  l.release();
            //runVerificationTest(inputPath, outPath.c_str());
        //}
    //}
    return THOR_OK;
}

BVType BladderSegmentationTask::determineClipViewType(const char* inputPath)
{
    if(strstr(inputPath, "SAGITTAL") != NULL) {
        return BVType::SAGITTAL;
    }
    else {
        return BVType::TRANSVERSE;
    }
}

ThorStatus BladderSegmentationTask::runVerificationTest(void* jni, const char* inputPath, const char* outputPath)
{
    LOGD("[ VERIFY ] Running verification");
    (mUSManager->getCinePlayer().openRawFile(inputPath));
    mFrameCount = 0;
    // Iterate through frames, storing intermediate data
    if(mLoadCallback != nullptr) {
        mLoadCallback(determineClipFlowType(inputPath), determineClipViewType(inputPath), getDepthId(), mParams.probeType);
    }
    // Output JSON file
    Json::Value root(Json::ValueType::objectValue);
    mInvalidFrameCount = 0;
    uint32_t maxFrame = mUSManager->getCinePlayer().getEndFrame() + 1;
    resetSelectedFrame();
    root["file"] = inputPath;
    const char *id_begin = strrchr(inputPath, '/')+1;
    const char *id_end = strrchr(inputPath, '.');
    char outpath[512];
    snprintf(outpath, sizeof(outpath), "%s/%.*s.json",
             outputPath,
             static_cast<int>(id_end - id_begin), id_begin);
    char filename[512];
    snprintf(filename, sizeof(filename), "%.*s",
             static_cast<int>(id_end - id_begin), id_begin);
    char rawout[512];
    snprintf(rawout, sizeof(rawout), "%s/%s", outputPath, filename);
    ::mkdir(rawout, S_IRWXU | S_IRGRP | S_IROTH);
    for(uint32_t frameNum = 0; frameNum < maxFrame; ++frameNum)
    {
        runFrameVerification(root, frameNum, outputPath, filename, jni);
    }

    // testing caliper estimation after running all frames
    while(!getFrameInRoIMap(maxFrame-1)){//while(!mFrameSelected) {
            static int waitCount = 0;
            ++waitCount;
            if(waitCount % 2) {
                LOGD("[ BVW ] waiting for GODOT");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            if(waitCount > 300)
            {
                waitCount = 0;
                break;
            }
        }
    mSwiftSelectedFrame = getMaxAreaFrame();
    if(mSwiftSelectedFrame >= 0){
        mMapFrameNum = mSwiftSelectedFrame;
        auto estimatedCalipers = estimateCaliperLocations(mSwiftSelectedFrame, determineClipViewType(mClipname), root, jni);
        // Expect update to centroid after estimating caliper location
        root["selected_frame"]["centroid"]["screenX"] = mCentroidX;
        root["selected_frame"]["centroid"]["screenY"] = mCentroidY;
        // get screen image for selected frame
        int scrWidth = mImguiRenderer.getWidth();
        int scrHeight = mImguiRenderer.getHeight();
        
        cv::Size scrSize = {scrWidth, scrHeight};
        cv::Mat screenImageRGBA(scrSize, CV_8UC4);
        cv::Mat screenImageUD(scrSize, CV_8UC1);
        cv::Mat screenImage(scrSize, CV_8UC1);
        // get Ultrasound Image (Screen Image)
        mUSManager->getCineViewer().getUltrasoundScreenImage(screenImageRGBA.ptr<uint8_t>(), scrWidth, scrHeight, false, mSwiftSelectedFrame);
        cv::cvtColor(screenImageRGBA, screenImage, cv::COLOR_RGBA2GRAY);
        // flip image as ultrasound image is vertically flipped
        //cv::flip(screenImageUD, screenImage, 0);
        
        SaveMap(screenImage, "screen_image", mSwiftSelectedFrame, mMapFilePath, mClipname);
        SaveMap(mSegMaskSelected, "selected_mask", mSwiftSelectedFrame, mMapFilePath, mClipname);
        root["selected_frame"]["frame"] = mSwiftSelectedFrame;
        root["selected_frame"]["segmentation"] = ToJson(estimatedCalipers);
        root["selected_frame"]["width"] = scrWidth;
        root["selected_frame"]["height"] = scrHeight;
    }
    else {
        root["selected_frame"] = "None";
    }
    //end test
    savePixelSize(root);
    saveImgToPhysicalMatrix(root);
    // follow up tasks
    

    std::ofstream os(outpath);
    os << root << "\n";
    return THOR_OK;
}

Json::Value BladderSegmentationTask::ToJson(const cv::Mat& mat)
{
    Json::Value root;
    const uint8_t CHANNEL_COUNT = 2;
    // the vector is 2 matrices imgsize x imgsize with the two channels
    const char* frames[2] = {"raw_output_ch1\0", "raw_output_ch2\0"};
    for(int i = 0; i < CHANNEL_COUNT; ++i)
    {
        for(int x = 0; x < 128; ++x)
        {
            for(int y = 0; y < 128; ++y)
            {
                root[frames[i]].append((float)((mat.ptr(x,y)[i])));
            }
        }
    }
    return root;
}

Json::Value BladderSegmentationTask::ToJson(const cv::Point2f& point)
{
    Json::Value root;
    root["x"] = point.x;
    root["y"] = point.y;
    return root;
}

Json::Value BladderSegmentationTask::ToJson(const BVPoint& point)
{
    Json::Value root;
    root["x"] = point.x;
    root["y"] = point.y;
    return root;
}

Json::Value BladderSegmentationTask::ToJson(const BVCaliper& caliper)
{
    Json::Value root;
    root["start_point"] = ToJson(caliper.startPoint);
    root["end_point"] = ToJson(caliper.endPoint);
    return root;
}

Json::Value BladderSegmentationTask::ToJson(const BVSegmentation& calipers)
{
    Json::Value root;
    root["major_caliper"] = ToJson(calipers.majorCaliper);
    root["minor_caliper"] = ToJson(calipers.minorCaliper);
    for(int i = 0; i < calipers.contour.size(); ++i)
    {
        root["contour"].append(ToJson(calipers.contour[i]));
    }
    return root;
}

void BladderSegmentationTask::resetSelectedFrame()
{
    mSwiftSelectedFrame = 0;
    mFrameSelected = false;
    mSelectedVerificationFrame.area = 0;
    mSelectedVerificationFrame.uncertainty = 1.0f;
    mSelectedVerificationFrame.x = 0.0f;
    mSelectedVerificationFrame.y = 0.0f;
    mSelectedVerificationFrame.frame = -1;
    
}

void BladderSegmentationTask::runFrameVerification(Json::Value& root, uint32_t frameNum, const char* filepath, const char* clipname, void* jni)
{
    mMapFilePath = filepath;
    mClipname = clipname;
    mMapFrameNum = frameNum;
    mMapName = "screen";

    {
        uint32_t modelIdx = mFrameCount%BLADDER_SEGMENTATION_ENSEMBLE_SIZE;
     
        // scan converted image
        // TODO:: confirm scaled input buffer gets used for prediction, update prescan
        mUSManager->getAIManager().getScanConvertedFrame(frameNum, BLADDER_SEG_IMG_WIDTH, BLADDER_SEG_IMG_HEIGHT, (mScanConvertedFrame.data()));
        //uint8_t *prescan = mCB->getFrame(frameNum, DAU_DATA_TYPE_B);
        //mScanConverter->runCLTex(prescan, mScaledInputBuffer);
        cv::Size outSize = {BLADDER_SEG_IMG_WIDTH, BLADDER_SEG_IMG_HEIGHT};

        cv::Mat probMapMean(outSize, CV_32FC2);
        cv::Mat probMapFltSum(outSize, CV_8UC1);
        cv::Mat probMapFltOne(outSize, CV_8UC1);
        cv::Mat segMap(outSize, CV_8UC1);
        cv::Mat rawOutMat(outSize, CV_32FC2);
        cv::Mat inputBuffer = cv::Mat(outSize, CV_8UC1, mScaledInputBuffer);
        SaveMap(inputBuffer, "scInput", frameNum, filepath, clipname);
        
        // processFrame rawOuts does not include SoftMax operations as well
        float viewscore[5] = {0.0f};
        // process into raw outputs
        BladderSegmentationJNI* bJni = static_cast<BladderSegmentationJNI*>(mJNI);
        cv::Mat inputMat = {outSize, CV_32FC1, bJni->imageBufferNative.data()};
        cv::Mat inputCharMat = {outSize, CV_8UC1, mScanConvertedFrame.data()};
        inputCharMat.convertTo(inputMat, CV_32FC1, 1.0/255.0);
        JNIEnv* jEnv = static_cast<JNIEnv*>(jni);
        jEnv->CallVoidMethod(bJni->instance, bJni->process, bJni->imageBuffer, bJni->viewBuffer, bJni->pMapBuffer, modelIdx);
        memcpy(mProbMap[modelIdx].ptr<float>(), bJni->pMapBufferNative.data(), sizeof(float) * BLADDER_SEG_IMG_WIDTH * BLADDER_SEG_IMG_HEIGHT * 2);
        memcpy(viewscore, bJni->viewBufferNative.data(), sizeof(float) * 5);
        //processPMap(bJni->pMapBufferNative.data(), mProbMap[modelIdx].ptr<float>(), BLADDER_SEG_IMG_WIDTH, BLADDER_SEG_IMG_HEIGHT);
        //softmax by channel
        cv::Mat channel_split[2];
        split(mProbMap[modelIdx], channel_split);
        //SaveMap(channel_split[0], "ch0", frameNum, filepath, clipname);
        //SaveMap(channel_split[1], "ch1", frameNum, filepath, clipname);
        twoChannelSoftmax(mProbMap[modelIdx], mProbMap[modelIdx]);
        split(mProbMap[modelIdx], channel_split);
        //SaveMap(channel_split[0], "ch0", frameNum, filepath, clipname);
        //SaveMap(channel_split[1], "ch1", frameNum, filepath, clipname);
        Json::Value frameData;
        frameData["frameNum"] = (frameNum);
        frameData["viewscore"];
        for(int i = 0; i < 5; ++i)
        {
            frameData["viewscore"].append(viewscore[i]);
        }
        // debug
        double minMatVal = 0.0;
        double maxMatVal = 0.0;
        cv::minMaxLoc(mProbMap[modelIdx], &minMatVal, &maxMatVal);
        auto scalarTotal = cv::sum(mProbMap[modelIdx]);
        double total = scalarTotal[0] + scalarTotal[1] + scalarTotal[2] + scalarTotal[3];
        //root["frame"].append(ToJson(mProbMap[modelIdx]));
        // should be the equivalent of processFrame after softmax
        // Flatten ProbMap.  Setting 1 (instead of 255)
        flatten(mProbMap[modelIdx], mProbMapFlt[modelIdx], 1u);
        // TODO: improve computation efficiency
        // Mean - probMap
        probMapMean = mProbMap[0] + mProbMap[1] + mProbMap[2] + mProbMap[3];
        probMapMean = probMapMean * 0.25;
        //split(probMapMean, channel_split);
        //SaveMap(channel_split[0], "mean_ch0", frameNum, filepath, clipname);
        //SaveMap(channel_split[1], "mean_ch1", frameNum, filepath, clipname);
        
        //SaveMap(probMapMean, "mean", frameNum, filepath, clipname);
        //WriteImg(probMapMean, "mean", frameNum, filepath, clipname);
        probMapFltSum = mProbMapFlt[0] + mProbMapFlt[1] + mProbMapFlt[2] + mProbMapFlt[3];
        probMapFltOne = probMapFltSum == 4;
        
        int pMapFltNonZero = cv::countNonZero(probMapFltSum);
        int pMapFltOneNonZero = cv::countNonZero(probMapFltOne);
        
        // flatten
        flatten(probMapMean, segMap);
             
        // set to zeros to segmentation mask
        mSegMask.setTo(cv::Scalar::all(0));
        mMaskArea = 0;
        
        cv::Mat labels(outSize, CV_8UC1);    //output
        cv::Mat stats;
        cv::Mat centroids;
        
        int nb_components = cv::connectedComponentsWithStats(segMap, labels, stats, centroids, 4, CV_16U);
        bool centroidFound = false;
        // only 1 component
        if (nb_components < 2)
        {
            // TODO: 1 component case
            // ignoref
            LOGD("[ BLADDER VERIFICATON ] Only one component found");
        }
        else
        {
            // nb compts >= 2
            int curVal;
            int max1, max2;
            int max1Idx, max2Idx;
            max1 = -10;
            max2 = -20;
            max1Idx = 0;
            max2Idx = 0;
            
            for (int i = 0; i < nb_components; i++)
            {
                curVal =  stats.at<int>(i, cv::CC_STAT_AREA);
                
                if (curVal > max1)
                {
                    // update 2nd max
                    max2 = max1;
                    max2Idx = max1Idx;
                    
                    // update the max
                    max1 = curVal;
                    max1Idx = i;
                }
                else if (curVal > max2)
                {
                    // update 2nd max
                    max2 = curVal;
                    max2Idx = i;
                }
            }

            // IOS-2776.
            // check whether max2 can be background. > 95% of width and height
            if (stats.at<int>(max2Idx, cv::CC_STAT_WIDTH) > BLADDER_SEG_IMG_WIDTH * 0.95f
                && stats.at<int>(max2Idx, cv::CC_STAT_HEIGHT) > BLADDER_SEG_IMG_HEIGHT * 0.95f)
            {
                max2 = max1;
                max2Idx = max1Idx;
            }
                        
            mMaskArea = max2;
            
            // apply min filter - min area should be 100 - this set values to 0 or 255
            cv::inRange(labels, cv::Scalar(max2Idx), cv::Scalar(max2Idx), mSegMask);
            
            // centroid of identified index - defined on 128 x 128 (BLADDER_SEG_IMG_WIDTH & HEIGHT)
            mCentroidX = centroids.at<double>(max2Idx, 0);
            mCentroidY = centroids.at<double>(max2Idx, 1);
            centroidFound = true;
        }
        if(centroidFound){
            frameData["centroid"]["x"] = (mCentroidX);
            frameData["centroid"]["y"] = (mCentroidY);
        }
        // Uncertainty
        // iou = (map_count_ratio == 1).sum() / (map_count_ratio > 0).sum()
        // when map_count_ratio > 0 is not zero
        if ( (mMaskArea < 100) || (pMapFltNonZero == 0) )
            mUncertainty = 1.0f;
        else
            mUncertainty = 1.0f - ((float) pMapFltOneNonZero) / ((float) pMapFltNonZero);
        float centX128 = mCentroidX;
        float centY128 = mCentroidY;
        // convert to Physical coordinate
        convertToPhysicalSpace(mFlipX);
        
        // TODO: update

        int frameSelectionRange = 200;
        int minFrame = mUSManager->getCinePlayer().getFrameCount() - frameSelectionRange;
        int maxFrame = minFrame + frameSelectionRange;
        if (maxFrame > mUSManager->getCinePlayer().getFrameCount()) {
            maxFrame = mUSManager->getCinePlayer().getFrameCount();
        }
        if (minFrame < 0)
        {
            minFrame = 49;
        }
        if (nullptr != mBladderSegmentationCallback) {
            // Send callback here
            setSegmentationParams(mCentroidX, mCentroidY, mUncertainty, mStability, mMaskArea, frameNum);
        }
        
        // save segmentation mask image for clinical study
        std::copy(mSegMask.ptr<uint8_t>(), mSegMask.ptr<uint8_t>() + BLADDER_SEG_IMG_WIDTH * BLADDER_SEG_IMG_HEIGHT, mSegMaskSave[frameNum%CINE_FRAME_COUNT].ptr<uint8_t>());
        mUncertaintyValue[frameNum%CINE_FRAME_COUNT] = mUncertainty;
        SaveMap(mSegMask, "segMask", frameNum, filepath, clipname);
        frameData["uncertainty"] = (mUncertainty);
        while(!getFrameInRoIMap(frameNum)){
            LOGD("[ BLADDER ] Waiting on frame calculation for frame %d", frameNum);
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        }
        LOGD("[ BLADDER ] Frame found");
        bool centroidValid = isCentroidValid(frameNum);
        frameData["centroid"]["valid"] = centroidValid;
        if(centroidValid){
            LOGD("[ BLADDER SEGMENTATION VALID ] Valid Centroid in Frame %d (%s)", frameNum, clipname);
        }
        if(!centroidValid)
        {
            LOGD("[ BLADDER SEGMENTATION ] Invalid Centroid in Frame %d (%s)", frameNum, clipname);
            //frameData["centroid"] = false;
        }

        frameData["stability"] = (mStability);
        frameData["model_index"] = (modelIdx);
        frameData["mask_area"] = (mMaskArea);
        root["frames"].append(frameData);

        mFrameCount++;
        // selected frame view
        // clean up
        probMapMean.release();
        segMap.release();
        labels.release();
        stats.release();
        centroids.release();
    }
}
bool BladderSegmentationTask::isCentroidValid(int frameNum)
{
    return mBladderRoICallback(frameNum);
}
bool BladderSegmentationTask::getFrameInRoIMap(int frameNum)
{
    return mBladderFrameInRoICallback(frameNum);
}

void BladderSegmentationTask::saveImgToPhysicalMatrix(Json::Value& verification)
{
    // Taken from LVSegmentation.cpp - ConvertToPhysical
    float endImgDepth = mParams.converterParams.startSampleMm +
                        (mParams.converterParams.sampleSpacingMm * (mParams.converterParams.numSamples - 1));

    // img coordinate (BLADDER IMG - 128x128) to Physical
    float imgToPhys[9];
    ViewMatrix3x3::setIdentityM(imgToPhys);
    
    float scrWidth = (float) mImguiRenderer.getWidth();
    float scrHeight = (float) mImguiRenderer.getHeight();
    
    imgToPhys[I(0,0)] = mFlipX * 1.0f/scrHeight * endImgDepth;                   // scale to normalized, then to physical
    imgToPhys[I(0,2)] = mFlipX * -0.5f * endImgDepth * scrWidth / scrHeight;     // translate so that middle of the image is at 0
    imgToPhys[I(1,1)] = 1.0f/scrHeight * endImgDepth;
    imgToPhys[I(1,2)] = 0.0f;                                                   // no translation on y

    // in physical coordinate
    for(int i = 0; i < 9; ++i)
    {
        verification["img_to_physical"].append(imgToPhys[i]);
    }
}

float BladderSegmentationTask::estimateMaskAngleSagittalWithMask(uint32_t frameNum, cv::InputArray segMask, float refMaskLength, Json::Value& verification) {
    float angleSelect = 0.0f;
    float currentAngle;
    float maxDistance = -100000.0f;
    float length = refMaskLength;
    float currentDistance;
    
    std::vector<float> angle_list;
    
    // angle list
    for (int i = -80; i <= -30; i++)
        angle_list.push_back((float) i);
    
    for (int i = 30; i <= 80; i++)
        angle_list.push_back((float) i);
    
    // screen display size
    int scrWidth = mImguiRenderer.getWidth();
    int scrHeight = mImguiRenderer.getHeight();
    
    cv::Size scrSize = {scrWidth, scrHeight};
    cv::Mat segMaskSMT2 = segMask.getMat();
    
    for (auto it = angle_list.begin(); it != angle_list.end(); it++)
    {
        currentAngle = *it;
        
        // Line mask
        cv::Mat lineMask(scrSize, CV_8UC1, cv::Scalar(0));
            
        cv::Point2f startPt, endPt;
        
        // major line
        startPt.x = mCentroidX - sin(currentAngle / 180.0 * PI) * length;
        startPt.y = mCentroidY + cos(currentAngle / 180.0 * PI) * length;
        
        endPt.x = mCentroidX - sin( (currentAngle + 180) / 180.0 * PI) * length;
        endPt.y = mCentroidY + cos( (currentAngle + 180) / 180.0 * PI) * length;
        
        // draw lines on mask
        // Major
        cv::line(lineMask, startPt, endPt, cv::Scalar(250,250,250), 1);
        cv::threshold(lineMask, lineMask, 127, 255, 0);
            
        // bitwise_and
        cv::bitwise_and(segMaskSMT2, lineMask, lineMask);
        // find largest component of intersection 11/12/24
        cv::Mat lineMaskLca(scrSize, CV_8UC1, cv::Scalar(0));
        find_mask_largest_component(lineMask, lineMaskLca);
        // Find non-zero points
        cv::Mat nonZero;
        cv::findNonZero(lineMaskLca, nonZero);
        {
            // find min and max points
            float refVecX, refVecY;
            float minVal = 100.0;
            float maxVal = -100.0;
            float curVal;
            float curVecX, curVecY;
            
            // major axis
            refVecX = (startPt.x - endPt.x) / length / 2.0f;
            refVecY = (startPt.y - endPt.y) / length / 2.0f;
            for (int i = 0; i < nonZero.total(); i++) {
                cv::Point curPt = nonZero.at<cv::Point>(i);
                
                curVecX = curPt.x - mCentroidX;
                curVecY = curPt.y - mCentroidY;
                
                curVal = curVecX * refVecX + curVecY * refVecY;
                
                if (curVal > maxVal) {
                    maxVal = curVal;
                }
                if (curVal < minVal) {
                    minVal = curVal;
                }
            }
            
            currentDistance = maxVal - minVal;
            Json::Value caliperCandidate;
            caliperCandidate["length"] = currentDistance;
            caliperCandidate["angle"] = currentAngle;
            verification["selected_frame"]["caliper_candidates"].append(caliperCandidate);
        }
        
        if (currentDistance > maxDistance)
        {
            maxDistance = currentDistance;
            angleSelect = currentAngle;
        }
    }
    
    return angleSelect;
}

float BladderSegmentationTask::estimateMaskAngleTransverse(uint32_t frameNum, cv::InputArray segMask, Json::Value& verification) {
    
    // screen display size
    int scrWidth = mImguiRenderer.getWidth();
    int scrHeight = mImguiRenderer.getHeight();
    
    cv::Size scrSize = {scrWidth, scrHeight};
    cv::Mat screenImageRGBA(scrSize, CV_8UC4);
    cv::Mat screenImageUD(scrSize, CV_8UC1);
    cv::Mat screenImage(scrSize, CV_8UC1);
      
    // get Ultrasound Image (Screen Image)
    mUSManager->getCineViewer().getUltrasoundScreenImage(screenImageRGBA.ptr<uint8_t>(), scrWidth, scrHeight, false, frameNum);
    //void SaveMap(cv::Mat image, const char* name, unsigned int frameNum, const char* filepath, const char* clipname)
    //SaveMap(screenImageRGBA, mMapName, mMapFrameNum, mMapFilePath, mClipname);
    // convert to gray scale
    cv::cvtColor(screenImageRGBA, screenImage, cv::COLOR_RGBA2GRAY);
    // flip image as ultrasound image is vertically flipped
    
    // centered image
    cv::Mat mask_centered(scrSize, CV_8UC1);
    cv::Mat image_centered(scrSize, CV_8UC1);
    cv::Mat combo_centered(scrSize, CV_8UC1);
    cv::Mat combo_centeredUD(scrSize, CV_8UC1);
    cv::Mat mask_rotate(scrSize, CV_8UC1);
    
    float imgCenterX = ((float)scrWidth-1.f)/2.0f;
    float imgCenterY = ((float)scrHeight)/2.0f;
    int   imgCenterXInt = int(imgCenterX);
    
    float shiftX = imgCenterX - mCentroidX;
    float shiftY = imgCenterY - mCentroidY;
    
    // Centered images
    translate_image(segMask, mask_centered, shiftX, shiftY);
    translate_image(screenImage, image_centered, shiftX, shiftY);
    combo_centered = mask_centered * 0.5 + image_centered * 0.5;

    SaveMap(combo_centered, "combo_centered", mMapFrameNum, mMapFilePath, mClipname);
    float currentAngle = -10.0f;
    float angleStep = 1.0f;
    float maxAngle = 10.01f;
    
    float angleSelect = 0.0f;
    float maxSimCos = -1000000.0f;
    while (currentAngle <= maxAngle)
    {
        rorate_image(combo_centered, mask_rotate, currentAngle, imgCenterX, imgCenterY);
                
        cv::Mat leftHalf = mask_rotate(cv::Rect(0, 0, imgCenterXInt, scrHeight));
        cv::Mat rightHalf = mask_rotate(cv::Rect(imgCenterXInt, 0, scrWidth - imgCenterXInt, scrHeight));
                       
        cv::Mat leftLine, leftLine2;
        cv::Mat rightLine, rightLine2;
        cv::Mat lrMul;
            
        cv::reduce(leftHalf, leftLine, 1, cv::REDUCE_SUM, CV_32F);
        cv::reduce(rightHalf, rightLine, 1, cv::REDUCE_SUM, CV_32F);
                
        cv::multiply(leftLine, rightLine, lrMul);
        cv::multiply(leftLine, leftLine, leftLine2);
        cv::multiply(rightLine, rightLine, rightLine2);
                
        double lrMulSum = cv::sum(lrMul)[0];
        double leftLineSum = cv::sum(leftLine2)[0];
        double rightLineSum = cv::sum(rightLine2)[0];
        
        float sim_cos = (float) (lrMulSum / ( sqrt(leftLineSum) * sqrt(rightLineSum) ) );
        if (sim_cos > maxSimCos)
        {
            angleSelect = currentAngle;
            maxSimCos = sim_cos;
        }
        Json::Value caliperCandidate;
        caliperCandidate["sim_cos"] = sim_cos;
        caliperCandidate["angle"] = currentAngle;
        verification["selected_frame"]["caliper_candidates"].append(caliperCandidate);
        currentAngle += angleStep;
    }
    // check for end points
    // now adjust min and max angles
    float firstSearchSelectedAngle = angleSelect;
    float minAngle = max(-10.f, firstSearchSelectedAngle - 0.9f); // keep search inside bounds, but we've already searched the whole numbers
    maxAngle = min(10.1f, firstSearchSelectedAngle + 0.94f); //slightly over .9 for floating point imprecision
    angleStep = 0.1f; // increase search granularity
    currentAngle = minAngle;
    // run the search again
    const static float EPSILON = 0.00001f;
    while (currentAngle <= maxAngle)
    {
        if((abs(currentAngle - firstSearchSelectedAngle) < EPSILON) ) {
            currentAngle += angleStep;
            continue;
        }
        rorate_image(combo_centered, mask_rotate, currentAngle, imgCenterX, imgCenterY);

        cv::Mat leftHalf = mask_rotate(cv::Rect(0, 0, imgCenterXInt, scrHeight));
        cv::Mat rightHalf = mask_rotate(cv::Rect(imgCenterXInt, 0, scrWidth - imgCenterXInt, scrHeight));

        cv::Mat leftLine, leftLine2;
        cv::Mat rightLine, rightLine2;
        cv::Mat lrMul;

        cv::reduce(leftHalf, leftLine, 1, cv::REDUCE_SUM, CV_32F);
        cv::reduce(rightHalf, rightLine, 1, cv::REDUCE_SUM, CV_32F);

        cv::multiply(leftLine, rightLine, lrMul);
        cv::multiply(leftLine, leftLine, leftLine2);
        cv::multiply(rightLine, rightLine, rightLine2);

        double lrMulSum = cv::sum(lrMul)[0];
        double leftLineSum = cv::sum(leftLine2)[0];
        double rightLineSum = cv::sum(rightLine2)[0];

        float sim_cos = (float) (lrMulSum / ( sqrt(leftLineSum) * sqrt(rightLineSum) ) );

        if (sim_cos > maxSimCos)
        {
            angleSelect = currentAngle;
            maxSimCos = sim_cos;
        }
        Json::Value caliperCandidate;
        caliperCandidate["sim_cos"] = sim_cos;
        caliperCandidate["angle"] = currentAngle;
        verification["selected_frame"]["caliper_candidates"].append(caliperCandidate);
        currentAngle += angleStep;
    }
    return angleSelect;
}

int BladderSegmentationTask::determineClipFlowType(const char* inputPath)
{
    if(strstr(inputPath, "POSTVOID") != NULL) {
        return 1;
    }
    else {
        return 0;
    }
}

/// Pipeline
void BladderSegmentationTask::setResultsFromSwift(int frameNumber) {
    // TODO: Complete logic for pipeline
    LOGD("[ BVW ] SetResultsFromSwift");
    mSwiftSelectedFrame = frameNumber;
    mFrameSelected = true;
}

void BladderSegmentationTask::attachVerificationLoadCallback(BladderVerificationLoadCallback loadCallback)
{
    mLoadCallback = loadCallback;
}

void BladderSegmentationTask::attachMaxAreaFrameCallback(MaxAreaFrameCallback callback)
{
    mMaxAreaFrameCallback = callback;
}

void BladderSegmentationTask::attachMaxAreaCallback(MaxAreaCallback callback)
{
    mMaxAreaCallback = callback;
}

int BladderSegmentationTask::getDepthId()
{
    uint32_t retval = 0;
    mParams.upsReader->getDepthIndex(mParams.imagingCaseId, retval);
    return retval;
}

void BladderSegmentationTask::savePixelSize(Json::Value& verification)
{
    // Taken from LVSegmentation.cpp - ConvertToPhysical
    float endImgDepth = mParams.converterParams.startSampleMm +
                        (mParams.converterParams.sampleSpacingMm * (mParams.converterParams.numSamples - 1));
    float scrHeight = (float) mImguiRenderer.getHeight();
    float pixelSize = 1.0f/scrHeight * endImgDepth;
    verification["pixel_size"] = pixelSize;
}

int BladderSegmentationTask::getMaxAreaFrame(){
    return mMaxAreaFrameCallback();
}

int BladderSegmentationTask::getMaxArea(){
    return mMaxAreaCallback();
}