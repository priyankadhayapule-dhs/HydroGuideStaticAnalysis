//
// Copyright 2018 EchoNous Inc.
//

#include <string.h>
#include <math.h>
#include <ScanConverterHelper.h>
#include <ViewMatrix3x3.h>
#include <ThorUtils.h>
#include <ThorConstants.h>
#include <ViewMatrix3x3.h>

//-----------------------------------------------------------------------------
ScanConverterHelper::ScanConverterHelper() :
    mDeltaX(0.0f),
    mDeltaY(0.0f),
    mScale(1.0f),
    mXScale(1.0f),
    mYScale(1.0f),
    mXShift(0.0f),
    mYShift(-1.0f),
    mLimitX1(0.0f),
    mLimitX2(0.0f),
    mFlipX(1.0f),
    mFlipY(1.0f),
    mAspect(1.0f),
    mDensity(0.0f)
{
    memset(mMvpMatrix, 0, sizeof(mMvpMatrix));
}

//-----------------------------------------------------------------------------
ScanConverterHelper::~ScanConverterHelper()
{
}

//-----------------------------------------------------------------------------
void ScanConverterHelper::init(float width, float height, const ScanConverterParams& params)
{
    float           arc_len;
    float           arc_rad1;
    float           arc_rad2;

    mAspect = (height / width);

    if (width > height)
    {
        mDensity = width;
    }
    else
    {
        mDensity = height;
    }
    mDensity = mDensity / 1.5f;

    mScale = 1.0f;
    mDeltaX = 0.0f;
    mDeltaY = 0.0f;
    mFlipX = 1.0f;
    mFlipY = 1.0f;

    mMvpMatrix[0] = mFlipX * mScale * mAspect;
    mMvpMatrix[1] = 0;
    mMvpMatrix[2] = 0;
    mMvpMatrix[3] = 0;
    mMvpMatrix[4] = mFlipY * mScale;
    mMvpMatrix[5] = 0;
    mMvpMatrix[6] = -mFlipX * mDeltaX * mAspect;
    mMvpMatrix[7] = mFlipY * mDeltaY;
    mMvpMatrix[8] = 1;

    if (params.probeShape == PROBE_SHAPE_LINEAR)
    {
        mLimitX1 = params.startRayMm * mXScale;
        mLimitX2 = (params.startRayMm + (params.numRays - 1) * params.raySpacing) * mXScale;
    }
    else
    {
        // default type: Phased Array
        // calculate X limits - for phased or curved arrays
        arc_len = params.sampleSpacingMm * (params.numSamples - 1) + params.startSampleMm;
        arc_rad1 = params.startRayRadian;
        arc_rad2 = params.startRayRadian + (params.numRays - 1) * params.raySpacing;

        mLimitX1 = arc_len * sin(arc_rad1) * mYScale;
        mLimitX2 = arc_len * sin(arc_rad2) * mYScale;
    }
}

//-----------------------------------------------------------------------------
void ScanConverterHelper::init()
{
    mScale = 1.0f;
    mDeltaX = 0.0f;
    mDeltaY = 0.0f;
    mFlipX = 1.0f;
    mFlipY = 1.0f;

    mMvpMatrix[0] = mFlipX * mScale * mAspect;
    mMvpMatrix[1] = 0;
    mMvpMatrix[2] = 0;
    mMvpMatrix[3] = 0;
    mMvpMatrix[4] = mFlipY * mScale;
    mMvpMatrix[5] = 0;
    mMvpMatrix[6] = -mFlipX * mDeltaX * mAspect;
    mMvpMatrix[7] = mFlipY * mDeltaY;
    mMvpMatrix[8] = 1;
}

//-----------------------------------------------------------------------------
void ScanConverterHelper::setAspect(float aspectRatio)
{
    mAspect = aspectRatio;
    // update MvpMatrix
    mMvpMatrix[0] = mFlipX * mScale * mAspect;
    mMvpMatrix[6] = -mFlipX * mDeltaX * mAspect;
}

//-----------------------------------------------------------------------------
void ScanConverterHelper::setScaleXYShift(float xyScale, float xShift, float yShift)
{
    mXScale = xyScale;
    mYScale = xyScale;
    mXShift = xShift;
    mYShift = yShift;
}

//-----------------------------------------------------------------------------
void ScanConverterHelper::setScaleXYShift(float xScale, float yScale, float xShift, float yShift)
{
    mXScale = xScale;
    mYScale = yScale;
    mXShift = xShift;
    mYShift = yShift;
}

//-----------------------------------------------------------------------------
void ScanConverterHelper::setScale(float scale)
{
    mScale = scale;

    mMvpMatrix[0] = mFlipX * mScale * mAspect;
    mMvpMatrix[4] = mFlipY * mScale;
}

//-----------------------------------------------------------------------------
void ScanConverterHelper::setPan(float deltaX, float deltaY)
{
    mDeltaX = deltaX;
    mDeltaY = deltaY;

    mMvpMatrix[6] = -mFlipX * mDeltaX * mAspect;
    mMvpMatrix[7] = mFlipY * mDeltaY;
}

//-----------------------------------------------------------------------------
void ScanConverterHelper::setFlip(float flipX, float flipY)
{
    // setting flipX and flipY (mirroring) parameters
    // should be either 1.0f or -1.0f
    mFlipX = flipX;
    mFlipY = flipY;

    mMvpMatrix[0] = mFlipX * mScale * mAspect;
    mMvpMatrix[4] = mFlipY * mScale;
    mMvpMatrix[6] = -mFlipX * mDeltaX * mAspect;
    mMvpMatrix[7] = mFlipY * mDeltaY;
}

//-----------------------------------------------------------------------------
void ScanConverterHelper::scaleRelative(float scaleFactor)
{
    mScale *= scaleFactor;

    // min/max scale - currently 1 ~ 4
    if (mScale < 1.0f)
    {
        mScale = 1.0f;
    }
    if (mScale > 4.0f)
    {
        mScale = 4.0f;
    }

    mMvpMatrix[0] = mFlipX * mScale * mAspect;
    mMvpMatrix[4] = mFlipY * mScale;
}

//-----------------------------------------------------------------------------
void ScanConverterHelper::panRelative(float distX, float distY)
{
    float deltaX = distX / mDensity * mScale;
    float deltaY = distY / mDensity * mScale;

    mDeltaX += mFlipX * deltaX;
    mDeltaY += mFlipY * deltaY;

    // calculate X limit with respect to the aspect ratio (e.g., LandScape or Portrait)
    if (mScale * abs(mYShift) <= 1.0f)
        mDeltaY = 0.0f;
    else
    {
        if (mDeltaY > (abs(mYShift) * mScale - 1.0f ))
            mDeltaY = (abs(mYShift) * mScale - 1.0f );
        else if (mDeltaY < -(abs(mYShift) * mScale - 1.0f))
            mDeltaY = -(abs(mYShift) * mScale - 1.0f);
    }

    if ((mScale < 2.0f / (mAspect * (mLimitX2 - mLimitX1))))
        mDeltaX = 0.0f;
    else
    {
        if (mDeltaX > (mLimitX2 * mScale - 1.0f / mAspect))
            mDeltaX = (mLimitX2 * mScale - 1.0f / mAspect);
        else if (mDeltaX < (mLimitX1 * mScale + 1.0f / mAspect))
            mDeltaX = (mLimitX1 * mScale + 1.0f / mAspect);
    }

    mMvpMatrix[6] = -mFlipX * mDeltaX * mAspect;
    mMvpMatrix[7] = mFlipY * mDeltaY;
}

//-----------------------------------------------------------------------------
void ScanConverterHelper::getDisplayDepth(float &startDepth, float &endDepth)
{
    // yshift is assumed to be -1.0f (ff = -mFlipY * mScale * mYShift + mFlipY * mDeltay)
    float ee = - mFlipY * mScale * mYScale;
    float ff = - mFlipY * ((mScale * mYShift) - mDeltaY);

    startDepth = (1.0f - ff) / ee;
    endDepth = -(1.0f + ff) / ee;
}

//-----------------------------------------------------------------------------
void ScanConverterHelper::getPan(float& deltaX, float& deltaY)
{
    deltaX = mDeltaX;
    deltaY = mDeltaY;
}

//-----------------------------------------------------------------------------
void ScanConverterHelper::getScale(float& scale)
{
    scale = mScale;
}

//-----------------------------------------------------------------------------
void ScanConverterHelper::getFlip(float& flipX, float& flipY)
{
    flipX = mFlipX;
    flipY = mFlipY;
}

//-----------------------------------------------------------------------------
void ScanConverterHelper::getAspect(float &aspectRatio)
{
    aspectRatio = mAspect;
}

//-----------------------------------------------------------------------------
void ScanConverterHelper::getPhysicalToPixelMap(float screenWidth, float screenHeight, float* mapMat)
{
    // matrix allocation
    float mat_p2n[9];   // phyiscal to normalized screen coordinate
    float mat_s2pp[9];  // scaled screen to pixel coordinate
    float mat_tmp[9];   // temp Matrix


    // phys to norm screen
    ViewMatrix3x3::setIdentityM(mat_p2n);
    mat_p2n[I(0, 0)] = mXScale;
    mat_p2n[I(1,1)] = -mYScale;
    mat_p2n[I(0, 2)] = mXShift;
    mat_p2n[I(1,2)] = -mYShift;


    // scaled screen to pixel coordinate
    ViewMatrix3x3::setIdentityM(mat_s2pp);
    mat_s2pp[I(0,0)] = screenWidth/2.0f;
    mat_s2pp[I(0,2)] = screenWidth/2.0f - 0.5f;
    mat_s2pp[I(1,1)] = -screenHeight/2.0f; // put/remove minus sign for polarity change
    mat_s2pp[I(1,2)] = screenHeight/2.0f - 0.5f;


    // physical to scaled screen
    ViewMatrix3x3::multiplyMM(mat_tmp, mMvpMatrix, mat_p2n);

    // physical to pixel coordinate
    ViewMatrix3x3::multiplyMM(mapMat, mat_s2pp, mat_tmp);
}

Matrix3 ScanConverterHelper::getPhysicalToPixelTransform(float width, float height) {
    Matrix3 result;
    getPhysicalToPixelMap(width, height, result.m);
    return result;
}

