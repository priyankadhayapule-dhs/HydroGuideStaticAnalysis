//
// Copyright 2017 EchoNous Inc.
//
#pragma once

#include <sys/types.h>
#include <string>
#include <ViewMatrix3x3.h>

// ScanConverter Reference Model

class ScanConverterReference
{
public: // Methods
    ScanConverterReference();
    virtual ~ScanConverterReference();

    // setting ScanConversion Parameters
    void setConversionParams(int numSamples, int numRays, int screenWidth, int screenHeight,
                                  float startImgDepth, float endImgDepth, float startSampleMm, float sampleSpacing,
                                  float startRayRadian, float raySpacing);

    // run ScanConversion Models
    void runConversionBiCubic4x4(u_char* inPtr, u_char* outPtr);
    void runConversionBiCubic4x4(u_char* inPtr, unsigned char *outPtr, int screenWidth,
                                 int screenHeight, float *matPixel2Phys,
                                 float *apexLocMm, float startDepth, float endDepth,
                                 float startSampleMm, float sampleSpacing, int numSample,
                                 float startRayRadian, float raySpacing, int numRay);

    void setZoomPanParams(float scale, float deltaX, float deltaY);
    void setZoomPanParams(float scale, float deltaX, float deltaY, float aspect);
    void setMappingTableScaleParams(float xyScale, float xShift, float yShift);
    void setFlipXFlipY(float flipX, float flipY);


private: // Methods
    void calculateCurrentPixel2PhysMatrix(float* mat);
    float cubicInt(float y0, float y1, float y2, float y3, float mu);

private: // Properties
    float       mMatPixel2Phys[9];      // model matrix transforming from pixel coordinate to physical coordinate
    float       mVecApex[3];            // vector representing apex
    int         mScreenWidth;
    int         mScreenHeight;

    int         mNumSamples;
    int         mNumRays;
    float       mStartSampleMm;
    float       mSampleSpacingMm;
    float       mRaySpacing;
    float       mStartRayRadian;

    float       mStartImgDepth;
    float       mEndImgDepth;

    // ScanConversion Table Parameters
    float       mXYScale;
    float       mXShift;
    float       mYShift;

    // zoom pan parameters
    float       mAspect;
    float       mScale;
    float       mDeltaX;
    float       mDeltaY;

    // flip / mirroring
    float       mFlipX;
    float       mFlipY;

};

