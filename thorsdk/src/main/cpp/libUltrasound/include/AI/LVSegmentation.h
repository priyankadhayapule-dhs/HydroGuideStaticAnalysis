//
// Copyright 2019 EchoNous, Inc.
//
#include <ThorError.h>
#include <ThorUtils.h>
#include <opencv2/core.hpp>
#include <vector>
#include <EFWorkflow.h>

// Forward declarations
class CineBuffer;
class AIModel;
class CardiacSegmentation;
struct EFNativeJNI;

namespace echonous
{
namespace lv
{

// Number of intermediate points per side wall
const static unsigned int SIDE_POINTS = 5;

// Index of right base point, apex, and left base point
const static unsigned int BASE_RIGHT = 0;
const static unsigned int APEX = BASE_RIGHT+SIDE_POINTS+1;
const static unsigned int BASE_LEFT = APEX+SIDE_POINTS+1;


/**
 Scan convert a window of 5 frames from the CineBuffer.
 */
ThorStatus ScanConvertWindow(CineBuffer *cine,
                       cv::Size size,
                       int frameNum,
                       float flipX,
                       std::vector<float> &frameWindow);

ThorStatus RunEnsemble(EFNativeJNI *jni, cv::OutputArray seg,
        float* uncertainty, bool ED, unsigned int frameNum, CardiacView view);

ThorStatus RunEnsemble(EFNativeJNI *jni, cv::OutputArray seg,
                       float* uncertainty, bool ED, unsigned int frameNum, CardiacView view, void *jenv, const char* outpath);

/**
 Select the largest connected component of a given label type. Returns an error if two or more
 components are nearly tied for largest area (the definition of "nearly tied" is configurable, see
 implementation).
 */
ThorStatus LargestComponent(cv::InputArray mask, cv::OutputArray result, float *area);

/**
 Extract the dense contour from the thresholded lv segmentation. The dense contour is always closed,
 and winds counter-clockwise. Returns an error if the number of located contours is not 1.
 */
ThorStatus DenseContour(cv::Mat lv, std::vector<cv::Point> &denseContour);

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
ThorStatus ShapeDetect(std::vector<cv::Point> &denseContour, std::vector<int> &splineIndices);

/**
 Computes the partial arc length of denseContour, writing the result into arcLength. That is,
 arcLength[i] is the arc length of denseContour from index 0 to index i.

 Never returns an error.
 */
ThorStatus PartialArcLength(const std::vector<cv::Point> &denseContour, std::vector<float> &arcLength);

ThorStatus PlaceSidePoints(const std::vector<cv::Point> &denseContour,
                           const std::vector<float> &arcLength,
                           std::vector<int> &splineIndices);

ThorStatus ConvertToPhysical(const std::vector<cv::Point> &denseContour,
                             const std::vector<int> &splineIndices,
                             float theta,
                             float scanDepth,
                             float flipX,
                             cv::Size imgSize,
                             CardiacSegmentation &seg);
}
}
