#define LOG_TAG "LVSegmentation"

#include <opencv2/imgproc.hpp>
#include <LVSegmentation.h>
#include <CineBuffer.h>
#include <ScanConverterCl.h>
#include <ThorError.h>
#include <EFWorkflow.h>
#include <ImageUtils.h>
#include <EFNativeJNI.h>


#include <ostream>
#include <fstream>
#include <string>
#include <sstream>

void SaveProbMap(cv::Mat image, const char* name, CardiacView view, unsigned int frameNum, const char* outpath)
{
    std::stringstream path;
    path << outpath;
    path << "/";
    path << ((view == CardiacView::A4C) ? "a4c_" : "a2c_");
    path << name;

    path << "_frame" << frameNum << "_";
    path << name << ".raw";
    LOGD("Saving probmap in path %s - NAME: %s", path.str().c_str(), name);
    std::ofstream output(path.str(), std::ios::binary);

    // Warning: only works if the image is contiguous
    if(output.is_open())
    {
        LOGD("[ SAVE RAW ] Opened Correctly: %s", path.str().c_str());
    }
    else
    {
        LOGD("[ SAVE RAW ] Failed to Open: %s", path.str().c_str());
    }
    output.write((const char*)image.data, image.total() * image.elemSize());
    output.close();
    if(!output)
    {
        LOGD("[ SAVE RAW ] Could not save in path %s", path.str().c_str());
    }
}

namespace echonous
{
namespace lv
{

static ScanConverterCL *gScanConverter = nullptr;

/**
Scan convert a window of 5 frames from the CineBuffer.
*/
ThorStatus ScanConvertWindow(CineBuffer *cine,
                             cv::Size size,
                             int frameNum,
                             float flipX,
                             std::vector<float> &frameWindow)
{
    ThorStatus err;
    // Check that all 5 frames are in cinebuffer
    // Use (int) instead of default uint32_t to avoid overflow
    if (frameNum - 4 < (int)cine->getMinFrameNum() ||
        frameNum > cine->getMaxFrameNum())
    {
        LOGE("Cannot run LV segmentation on frame %d, need 5 frame window in cinebuffer. Cinebuffer bounds [%u,%u]",
                frameNum, cine->getMinFrameNum(), cine->getMaxFrameNum());
        return TER_MISC_PARAM;
    }

    if (nullptr == gScanConverter) {
        gScanConverter = new ScanConverterCL(ScanConverterCL::OutputType::FLOAT_UNSIGNED_SCALE_255);
    }
    gScanConverter->setConversionParameters(
            size.width,
            size.height,
            flipX,
            1.0f,
            ScanConverterCL::ScaleMode::NON_UNIFORM_SCALE_TO_FIT,
            cine->getParams().converterParams);

    // Scan convert directly into frameWindow
    if (frameWindow.size() != size.width * size.height * 5) {
        LOGE("Framewindow is incorrect size (expected %zu, found %zu)", size.width * size.height * 5, frameWindow.size());
        return TER_MISC_PARAM;
    }

    for (int i=0; i < 5; ++i)
    {
        int f = frameNum-4+i;
        uint8_t *raw = cine->getFrame(f, DAU_DATA_TYPE_B);
        err = gScanConverter->runCLTex(raw, &frameWindow[size.width*size.height*i]);
        if (IS_THOR_ERROR(err))
            return err;
    }

    return THOR_OK;
}
/**
 * Flatten a probability map into a binary mask where channel 1 is the highest.
 */
static void flatten(cv::InputArray input, cv::OutputArray output)
{
    cv::Mat probmap = input.getMat();
    cv::Size size = probmap.size();

    output.create(size, CV_8UC1);
    cv::Mat seg = output.getMat();

    const float* probmapRaw = probmap.ptr<const float>();
    size_t yStride = size.width;
    size_t chStride = size.height * size.width;

    for (int y=0; y < size.height; ++y)
    for (int x=0; x < size.width; ++x)
    {
        float ch0 = probmapRaw[0 * chStride + y * yStride + x];
        float ch1 = probmapRaw[1 * chStride + y * yStride + x];
        float ch2 = probmapRaw[2 * chStride + y * yStride + x];

        if (ch1 > ch0 && ch1 > ch2)
            seg.at<uint8_t>(y,x) = 255u;
        else
            seg.at<uint8_t>(y,x) = 0u;
    }
}

static void format(cv::InputArray input, cv::OutputArray output) {
    cv::Mat probmap = input.getMat();
    cv::Size size = probmap.size();

    output.create(size, CV_32FC3);
    cv::Mat seg = output.getMat();

    const float *probmapRaw = probmap.ptr<const float>();
    size_t yStride = size.width;
    size_t chStride = size.height * size.width;
    std::vector<cv::Mat> channels(3);
    cv::split(input, channels);
    for (int y = 0; y < size.height; ++y){
        for (int x = 0; x < size.width; ++x) {
            float ch0 = probmapRaw[0 * chStride + y * yStride + x];
            float ch1 = probmapRaw[1 * chStride + y * yStride + x];
            float ch2 = probmapRaw[2 * chStride + y * yStride + x];

            channels[0].at<float>(y, x) = ch0;
            channels[1].at<float>(y, x) = ch1;
            channels[2].at<float>(y, x) = ch2;
        }
    }
    cv::merge(channels, output);
}

ThorStatus RunEnsemble(
        EFNativeJNI *jni,
        cv::OutputArray seg,
        float *uncertainty,
        bool ED,
        unsigned int frameNum,
        CardiacView view)
{
    ThorStatus status;
    cv::Size size(128, 128);
    cv::Mat probMap1(size, CV_32FC3, jni->segmentationConfMapBufferNative.data() + size.width*size.height*3*0);
    cv::Mat probMap2(size, CV_32FC3, jni->segmentationConfMapBufferNative.data() + size.width*size.height*3*1);
    cv::Mat probMap3(size, CV_32FC3, jni->segmentationConfMapBufferNative.data() + size.width*size.height*3*2);
    cv::Mat avgProbMap;
    cv::Mat segSmall;
    cv::Mat seg1;
    cv::Mat seg2;
    cv::Mat seg3;
    cv::Mat lvMean, lv1, lv2, lv3;

    jmethodID process = nullptr;
    if (ED) {
        process = jni->runSegmentationED;
    } else {
        process = jni->runSegmentationES;
    }


    jni->jenv->CallVoidMethod(jni->instance, process, jni->segmentationImageBuffer, jni->segmentationConfMapBuffer);

#if THOR_SAVE_EF_PROB_MAPS
    SaveProbMap(probMap1, "model1", view, frameNum, "/data/local/thortests/EF_Verification_RAW");
    SaveProbMap(probMap2, "model2", view, frameNum, "/data/local/thortests/EF_Verification_RAW");
    SaveProbMap(probMap3, "model3", view, frameNum, "/data/local/thortests/EF_Verification_RAW");
#endif

    // Compute map of mean confidences and flatten into a segmentation mask (selecting channel 1)
    avgProbMap = (probMap1 + probMap2 + probMap3)/3.0;
    flatten(avgProbMap, segSmall);
    status = LargestComponent(segSmall, lvMean, nullptr);
    if (IS_THOR_ERROR(status)) {
        return status;
    }
    // Resize to 1024 x 1024
    cv::resize(segSmall, seg, cv::Size(1024, 1024));
    cv::threshold(seg, seg, 127.0, 255.0, cv::THRESH_BINARY);

    // Compute uncertainty map
    // Flatten each individual probability map, then select the largest component
    flatten(probMap1, seg1);
    flatten(probMap2, seg2);
    flatten(probMap3, seg3);
    status = LargestComponent(seg1, lv1, nullptr);
    if (IS_THOR_ERROR(status)) {
        return status;
    }
    status = LargestComponent(seg2, lv2, nullptr);
    if (IS_THOR_ERROR(status)) {
        return status;
    }
    status = LargestComponent(seg3, lv3, nullptr);
    if (IS_THOR_ERROR(status)) {
        return status;
    }

#if THOR_SAVE_UNCERTAINTY_IMAGES
    char path[256];
    snprintf(path, sizeof(path), "/sdcard/uncert/%s_%s_%u_model1.pgm",
            view == CardiacView::A4C ? "a4c" : "a2c", ED ? "ed" : "es", frameNum);
    WriteImg(path, lv1);
    snprintf(path, sizeof(path), "/sdcard/uncert/%s_%s_%u_model2.pgm",
             view == CardiacView::A4C ? "a4c" : "a2c", ED ? "ed" : "es", frameNum);
    WriteImg(path, lv2);
    snprintf(path, sizeof(path), "/sdcard/uncert/%s_%s_%u_model3.pgm",
             view == CardiacView::A4C ? "a4c" : "a2c", ED ? "ed" : "es", frameNum);
    WriteImg(path, lv3);
#endif

    cv::Mat uncertaintyImg = cv::Mat(lv1.size(), CV_8UC1);

    for (int y=0; y != lv1.size().height; ++y)
    for (int x=0; x != lv1.size().width; ++x)
    {
        uint8_t value1 = lv1.at<uint8_t>(y,x);
        uint8_t value2 = lv2.at<uint8_t>(y,x);
        uint8_t value3 = lv3.at<uint8_t>(y,x);
        if (!(value1 == value2 && value2 == value3))
            uncertaintyImg.at<uint8_t>(y,x) = 255u;
        else
            uncertaintyImg.at<uint8_t>(y,x) = 0;
    }

#if THOR_SAVE_UNCERTAINTY_IMAGES
    snprintf(path, sizeof(path), "/sdcard/uncert/%s_%s_%u_uncertainty.pgm",
             view == CardiacView::A4C ? "a4c" : "a2c", ED ? "ed" : "es", frameNum);
    WriteImg(path, uncertaintyImg);
#endif
    float uncertainArea = cv::countNonZero(uncertaintyImg);
    float totalArea = cv::countNonZero(lvMean);
    *uncertainty = uncertainArea / totalArea;
    LOGD("uncertainty = %f (%f / %f)", *uncertainty, uncertainArea, totalArea);

    return THOR_OK;
}

    ThorStatus RunEnsemble(
            EFNativeJNI *jni,
            cv::OutputArray seg,
            float *uncertainty,
            bool ED,
            unsigned int frameNum,
            CardiacView view,
            void *jenv,
            const char* outpath) {
        ThorStatus status;
        cv::Size size(128, 128);
        cv::Mat probMap1(size, CV_32FC3, jni->segmentationConfMapBufferNative.data() +
                                         size.width * size.height * 3 * 0);
        cv::Mat probMap2(size, CV_32FC3, jni->segmentationConfMapBufferNative.data() +
                                         size.width * size.height * 3 * 1);
        cv::Mat probMap3(size, CV_32FC3, jni->segmentationConfMapBufferNative.data() +
                                         size.width * size.height * 3 * 2);
        cv::Mat formatProbMap1(size, CV_32FC3, jni->segmentationConfMapBufferNative.data() +
                                         size.width * size.height * 3 * 0);
        cv::Mat formatProbMap2(size, CV_32FC3, jni->segmentationConfMapBufferNative.data() +
                                         size.width * size.height * 3 * 1);
        cv::Mat formatProbMap3(size, CV_32FC3, jni->segmentationConfMapBufferNative.data() +
                                         size.width * size.height * 3 * 2);
        cv::Mat avgProbMap;
        cv::Mat segSmall;
        cv::Mat seg1;
        cv::Mat seg2;
        cv::Mat seg3;
        cv::Mat lvMean, lv1, lv2, lv3;

        jmethodID process = nullptr;
        if (ED) {
            process = jni->runSegmentationED;
        } else {
            process = jni->runSegmentationES;
        }
        auto jnienv = static_cast<JNIEnv *>(jenv);

        jnienv->CallVoidMethod(jni->instance, process, jni->segmentationImageBuffer,
                               jni->segmentationConfMapBuffer);
        format(probMap1, formatProbMap1);
        format(probMap2, formatProbMap2);
        format(probMap3, formatProbMap3);
        if(outpath != nullptr) {
            SaveProbMap(formatProbMap1, "model1", view, frameNum, outpath);
            SaveProbMap(formatProbMap2, "model2", view, frameNum, outpath);
            SaveProbMap(formatProbMap3, "model3", view, frameNum, outpath);
        }
        // Compute map of mean confidences and flatten into a segmentation mask (selecting channel 1)
        avgProbMap = (probMap1 + probMap2 + probMap3) / 3.0;
        flatten(avgProbMap, segSmall);
        status = LargestComponent(segSmall, lvMean, nullptr);
        if (IS_THOR_ERROR(status)) {
            return status;
        }
        // Resize to 1024 x 1024
        cv::resize(segSmall, seg, cv::Size(1024, 1024));
        cv::threshold(seg, seg, 127.0, 255.0, cv::THRESH_BINARY);

        // Compute uncertainty map
        // Flatten each individual probability map, then select the largest component
        flatten(probMap1, seg1);
        flatten(probMap2, seg2);
        flatten(probMap3, seg3);
        status = LargestComponent(seg1, lv1, nullptr);
        if (IS_THOR_ERROR(status)) {
            return status;
        }
        status = LargestComponent(seg2, lv2, nullptr);
        if (IS_THOR_ERROR(status)) {
            return status;
        }
        status = LargestComponent(seg3, lv3, nullptr);
        if (IS_THOR_ERROR(status)) {
            return status;
        }
        if(outpath != nullptr) {
            char path[256];
            snprintf(path, sizeof(path), "%s/%s_%s_%u_model1.pgm", outpath,
                     view == CardiacView::A4C ? "a4c" : "a2c", ED ? "ed" : "es", frameNum);
            WriteImg(path, lv1);
            snprintf(path, sizeof(path), "%s/%s_%s_%u_model2.pgm", outpath,
                     view == CardiacView::A4C ? "a4c" : "a2c", ED ? "ed" : "es", frameNum);
            WriteImg(path, lv2);
            snprintf(path, sizeof(path), "%s/%s_%s_%u_model3.pgm", outpath,
                     view == CardiacView::A4C ? "a4c" : "a2c", ED ? "ed" : "es", frameNum);
            WriteImg(path, lv3);
        }
        cv::Mat uncertaintyImg = cv::Mat(lv1.size(), CV_8UC1);

        for (int y = 0; y != lv1.size().height; ++y) {
            for (int x = 0; x != lv1.size().width; ++x) {
                uint8_t value1 = lv1.at<uint8_t>(y, x);
                uint8_t value2 = lv2.at<uint8_t>(y, x);
                uint8_t value3 = lv3.at<uint8_t>(y, x);
                if (!(value1 == value2 && value2 == value3))
                    uncertaintyImg.at<uint8_t>(y, x) = 255u;
                else
                    uncertaintyImg.at<uint8_t>(y, x) = 0;
            }
        }
        if (outpath != nullptr) {
            char path[256];
            snprintf(path, sizeof(path), "%s/%s_%s_%u_uncertainty.pgm", outpath,
                     view == CardiacView::A4C ? "a4c" : "a2c", ED ? "ed" : "es", frameNum);
            WriteImg(path, uncertaintyImg);
        }
        float uncertainArea = cv::countNonZero(uncertaintyImg);
        float totalArea = cv::countNonZero(lvMean);
        *uncertainty = uncertainArea / totalArea;
        LOGD("uncertainty = %f (%f / %f)", *uncertainty, uncertainArea, totalArea);

        return THOR_OK;
    }

ThorStatus LargestComponent(cv::InputArray _mask, cv::OutputArray _result, float *area)
{
    cv::Size size;
    cv::Mat mask;
    cv::Mat result;
    cv::Mat labels;
    cv::Mat areas;
    cv::Mat stats;
    cv::Mat centroids;

    size = _mask.size();
    mask = _mask.getMat();
    _result.create(size, CV_8UC1);
    result = _result.getMat();

    int numLabels = cv::connectedComponentsWithStats(mask, labels, stats, centroids, 8, CV_16U)-1;
    areas = stats.col(cv::CC_STAT_AREA);
    LOGD("areas.size = %d x %d", areas.size().height, areas.size().width);
    if (numLabels == 0)
    {
        LOGE("Did not find any components?");
        return TER_AI_LV_NOT_FOUND;
    }
    auto maxArea = std::max_element(++areas.begin<int>(), areas.end<int>()); // Start with offset 1 because offset 0 is background
    uint16_t maxIndex = static_cast<uint16_t>(std::distance(areas.begin<int>(), maxArea));

    cv::inRange(labels, cv::Scalar(maxIndex), cv::Scalar(maxIndex), result);

    // remove holes
    cv::Mat inverted;
    cv::bitwise_not(result, inverted);
    cv::floodFill(inverted, cv::Point(0,0), cv::Scalar(0));
    result = result | inverted;

    if (area)
        *area = *maxArea + cv::countNonZero(inverted);

    return THOR_OK;
}


/**
Select the largest connected component of a given label type. Returns an error if two or more
components are nearly tied for largest area (the definition of "nearly tied" is configurable, see
implementation).
*/
ThorStatus LargestComponent(cv::Mat labelMap, int label, cv::OutputArray lv)
{
    // Selected only the desired label and threshold it to all ones
    cv::Mat selectedLabel(labelMap.size(), CV_8UC1);
    cv::inRange(labelMap, cv::Scalar(label), cv::Scalar(label), selectedLabel);

    cv::Mat labels;
    cv::Mat stats;
    cv::Mat centroids;

    int numLabels = cv::connectedComponentsWithStats(selectedLabel, labels, stats, centroids, 8, CV_16U)-1;
    if (numLabels > 1) {
        // Model should ideally only produce one connected area, warn if more than one
        LOGW("LVSegmentation::LargestComponent(): Found %d connected components", numLabels);
    }

    // Because we are selecting only 1 component, we know it'll fit in an 8U type
    lv.create(labels.size(), CV_8UC1);

    if (numLabels < 2)
    {
        // if only 1 label, don't bother with the stats, just convert (and max out the value)
        labels.convertTo(lv, CV_8UC1, 255.0);
        return THOR_OK;
    }

    // find largest
    int largest = 0;
    int maxArea = 0;
    for (int label=1; label <= numLabels; ++label)
    {
        int area = stats.at<int>(label, cv::CC_STAT_AREA);
        LOGD("  Label %d has area %d", label, area);
        if (area > maxArea)
        {
            largest = label;
            maxArea = area;
        }
    }

    // Check for nearly tied (default to within 10%)
    float varianceAllowed = 0.1f;
    int areaAllowed = static_cast<int>(maxArea * (1.0f-varianceAllowed));
    for (int label=1; label < numLabels; ++label)
    {
        if (label == largest)
            continue;
        if (stats.at<int>(label, cv::CC_STAT_AREA) > areaAllowed)
        {
            LOGE("Found component with similar area to selected LV (multiple LV's found?)");
            return TER_AI_MULTIPLE_LV;
        }
    }

    // mask out everything but the largest label (this also maxes out the dst)
    cv::inRange(labels, cv::Scalar(largest), cv::Scalar(largest), lv);
    return THOR_OK;
}

/**
 Extract the dense contour from the thresholded lv segmentation. The dense contour is always closed,
 and winds counter-clockwise. Returns an error if the number of located contours is not 1.
 */
ThorStatus DenseContour(cv::Mat lv, std::vector<cv::Point> &denseContour)
{
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(lv, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);
    if (contours.size() < 1)
    {
        LOGE("No contour found during segmentation");
        return TER_AI_LV_TOO_SMALL;
    }
    if (contours.size() > 1)
    {
        LOGE("Found more than one contour during image segmentation");
        return TER_AI_MULTIPLE_LV;
    }

    denseContour = std::move(contours.front());
    return THOR_OK;
}

// returns the point furthest in the given direction, from a contour
static int FurthestInDirection(const std::vector<cv::Point> &points, cv::Point dir)
{
    int maxDist = dir.dot(points.front());
    int furthest = 0;

    for (int i=1; i != points.size(); ++i)
    {
        int dist = dir.dot(points[i]);
        if (dist > maxDist)
        {
            maxDist = dist;
            furthest = i;
        }
    }
    return furthest;
}

// Square norm of a point
static int sqnorm(cv::Point p)
{
    return p.x*p.x + p.y*p.y;
}

// Returns the index of the point with the minimum euclidean distance from p
static int MinDistance(const std::vector<cv::Point> &points, cv::Point p)
{
    int minDist = sqnorm(p-points.front());
    int minIdx = 0;

    for (int i=1; i != points.size(); ++i)
    {
        int dist = sqnorm(p-points[i]);
        if (dist < minDist)
        {
            minDist = dist;
            minIdx = i;
        }
    }
    return minIdx;
}

// Returns the index of the point with the maximum euclidean distance from p
static int MaxDistance(const std::vector<cv::Point> &points, cv::Point p)
{
    int maxDist = sqnorm(p-points.front());
    int maxIdx = 0;

    for (int i=1; i != points.size(); ++i)
    {
        int dist = sqnorm(p-points[i]);
        if (dist > maxDist)
        {
            maxDist = dist;
            maxIdx = i;
        }
    }
    return maxIdx;
}

/**
 Performs shape detection on the dense contour, locating the apex and base points. To make later
 processing simpler, the denseContour vector is rotated so that the bottom right base point is at
 index 0 (keeping the winding CCW). SplineIndices is a vector of indices into denseContour, noting
 which points make up the spline control points.

 Preconditions: denseContour contains a closed, CCW list of points which make up the contour
    of the LV. The starting point of this list is arbitrary (may be anywhere in the contour)
 Postconditions:
    splineIndices[BASE_RIGHT] = Index of right base point in denseContour
    splineIndices[APEX] = Index of apex point in denseContour
    splineIndices[BASE_LEFT] = Index of left base point in dense contour
    denseContour is rotated so that splineIndices[BASE_RIGHT] = 0
 */
ThorStatus ShapeDetect(std::vector<cv::Point> &denseContour, std::vector<int> &splineIndices)
{
    // Check to ensure we have a large enough contour to even fit an ellipse
    if (denseContour.size() < 20)
    {
        LOGE("Not enough points in dense contour to fit ellipse (%lu).", denseContour.size());
        return TER_AI_LV_TOO_SMALL;
    }

    splineIndices.resize(BASE_LEFT+1);
    auto rect = cv::fitEllipse(denseContour);

    cv::Point2f pts[4];
    rect.points(pts);

    // ATTENTION: All directional references below (left/right) are relative to the IMAGE space,
    // not physical space! That is, left is defined as closer to 0 on the x axis than right
    //
    // This is OPPOSITE of physical space: the left atrium is on the RIGHT side of the image.

    // Ensure we use the correct points for each of these locations (not upside-down or otherwise
    // rotated)

    // index of top left ellipse point
    int idxTopLeft = 0;
    {
        float minnorm = cv::norm(pts[0]);
        for (int i=1; i < 4; ++i)
        {
            float n = cv::norm(pts[i]);
            if (n < minnorm)
            {
                minnorm = n;
                idxTopLeft = i;
            }
        }
    }

    cv::Point ellipseTop = 0.5f*(pts[(idxTopLeft+1) % 4]+pts[(idxTopLeft+0) % 4]);
    int apex = MinDistance(denseContour, ellipseTop);

    cv::Point ellipseBL = pts[(idxTopLeft+3)%4];
    cv::Point ellipseBR = pts[(idxTopLeft+2)%4];
    int BL = MinDistance(denseContour, ellipseBL);
    int BR = MinDistance(denseContour, ellipseBR);
    cv::Point baseMid = (ellipseBL + ellipseBR)/2;

    apex = MaxDistance(denseContour, baseMid);

    // rotate so that BR is index 0
    std::rotate(denseContour.begin(), denseContour.begin()+BR, denseContour.end());
    apex -= BR;
    if (apex < 0)
        apex += denseContour.size();
    BL -= BR;
    if (BL < 0)
        BL += denseContour.size();
    BR = 0;
    splineIndices[BASE_RIGHT] = BR;
    splineIndices[APEX] = apex;
    splineIndices[BASE_LEFT] = BL;

    if (BR >= apex || apex >= BL)
    {
        // This would be a logic bug, never seen this error occur...
        LOGE("Error detecting base or apex points (not in expected order?)");
        return TER_AI_SEGMENTATION;
    }

    return THOR_OK;
}

/**
 Computes the partial arc length of denseContour, writing the result into arcLength. That is,
 arcLength[i] is the arc length of denseContour from index 0 to index i.

 Never returns an error.
 */
ThorStatus PartialArcLength(const std::vector<cv::Point> &denseContour, std::vector<float> &arcLength)
{
    arcLength.resize(denseContour.size());
    double perimeter = 0.0f;
    cv::Point2f prev = denseContour.front();
    for (unsigned int i=0; i < denseContour.size(); ++i)
    {
        cv::Point2f pt = denseContour[i];
        float dx = pt.x-prev.x;
        float dy = pt.y-prev.y;
        perimeter += std::sqrt(dx*dx + dy*dy);
        arcLength[i] = perimeter;

        prev = pt;
    }
    return THOR_OK;
}


ThorStatus PlaceSidePoints(const std::vector<cv::Point> &denseContour,
                           const std::vector<float> &arcLength,
                           std::vector<int> &splineIndices)
{
    int BR = splineIndices[BASE_RIGHT];
    int apex = splineIndices[APEX];
    int BL = splineIndices[BASE_LEFT];

    // length of the right side wall
    float rightLength = arcLength[apex]-arcLength[BR];
    float rightSpacing = rightLength / (SIDE_POINTS + 1.0f);
    float leftLength = arcLength[BL]-arcLength[apex];
    float leftSpacing = leftLength / (SIDE_POINTS + 1.0f);

    for (int i=1; i <= SIDE_POINTS; ++i)
    {
        auto right = std::lower_bound(arcLength.begin()+BR, arcLength.begin()+apex, i*rightSpacing);
        splineIndices[BASE_RIGHT+i] = std::distance(arcLength.begin(), right);


        auto left = std::lower_bound(arcLength.begin()+apex, arcLength.begin()+BL, i*leftSpacing + rightLength);
        splineIndices[APEX+i] = std::distance(arcLength.begin(), left);

    }
    return THOR_OK;
}

ThorStatus ConvertToPhysical(const std::vector<cv::Point> &denseContour,
                             const std::vector<int> &splineIndices,
                             float theta,
                             float scanDepth,
                             float flipX,
                             cv::Size imgSize,
                             CardiacSegmentation &seg)
{
    seg.coords.clear();
    seg.coords.resize(splineIndices.size());

    // Because we rescaled the image from original image size to 1024x1024 in RunEnsemble,
    // now we need to keep that rescaling in mind
    auto nonScaledToPhysical = gScanConverter->getPixelToPhysicalTransform();
    auto scalingFactor = Matrix3::Scale(static_cast<float>(imgSize.width)/1024.0f, static_cast<float>(imgSize.height)/1024.0f);
    auto transform = nonScaledToPhysical * scalingFactor;

    for (int i=0; i < splineIndices.size(); ++i)
    {
        const auto &pt = denseContour[splineIndices[i]];

        auto point = vec2(static_cast<float>(pt.x), static_cast<float>(pt.y));
        seg.coords[splineIndices.size()-i-1] = transform * point;
    }

    if (flipX > 0.0f) {
        // UI expects contour to be wound CCW, but a flipped image is wound CW
        // reverse the ordering
        std::reverse(seg.coords.begin(), seg.coords.end());
    }
    return THOR_OK;
}

}
}