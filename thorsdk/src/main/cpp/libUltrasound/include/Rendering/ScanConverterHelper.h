//
// Copyright 2018 EchoNous Inc.
//

#pragma once

#include <ScanConverterParams.h>
#include <geometry.h>

/**
 * State held here:
 * output image width/height (does this ever change?? in what situation would this change?)
 * beam parameters (aka sScan converter parameters)
 *   shape, width, etc
 * mvp converts from normalized image (-1 to 1) to scaled screen (H, W/A)
 *
 */

class ScanConverterHelper
{
public: // Methods
    ScanConverterHelper();
    ~ScanConverterHelper();

    void        init(float width, float height, const ScanConverterParams& params);
    void        init();
    void        setScale(float scale);
    void        setPan(float deltaX, float deltaY);
    void        setFlip(float flipX, float flipY);
    void        setAspect(float aspectRatio);
    void        setScaleXYShift(float xyScale, float xShift, float yShift);
    void        setScaleXYShift(float xScale, float yScale, float xShift, float yShift);

    void        scaleRelative(float scaleFactor);
    void        panRelative(float distX, float distY); // deltaX/Y is calculated

    void        getDisplayDepth(float &startDepth, float &endDepth);
    float*      getMvpMatrixPtr() { return(mMvpMatrix); }

    void        getPan(float& deltaX, float& deltaY);
    void        getScale(float& scale);
    void        getFlip(float& flipX, float& flipY);
    void        getAspect(float& aspectRatio);

    void        getPhysicalToPixelMap(float width, float height, float* mapMat);     // 3x3 Matrix float mat[9]
    Matrix3     getPhysicalToPixelTransform(float width, float height);

private: // Properties
    float       mMvpMatrix[9];
    float       mDeltaX;
    float       mDeltaY;
    float       mScale;
    float       mXScale;
    float       mYScale;
    float       mXShift;
    float       mYShift;
    float       mLimitX1;
    float       mLimitX2;
    float       mFlipX;
    float       mFlipY;
    float       mAspect;
    float       mDensity;
};
