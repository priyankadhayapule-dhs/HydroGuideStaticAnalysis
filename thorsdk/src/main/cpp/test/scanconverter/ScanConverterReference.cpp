//
// Copyright 2017 EchoNous Inc.
//

#define LOG_TAG "ScanConverterReference"

#include "ScanConverterReference.h"
#include <math.h>


//-----------------------------------------------------------------------------
ScanConverterReference::ScanConverterReference() :
    mScreenWidth(-1),
    mScreenHeight(-1),
    mNumSamples(-1),
    mNumRays(-1),
    mStartSampleMm(-1),
    mSampleSpacingMm(-1),
    mRaySpacing(-1),
    mStartRayRadian(-1),
    mStartImgDepth(-1),
    mEndImgDepth(-1),
    mXYScale(-1),
    mXShift(0),
    mYShift(-1),
    mAspect(-1),
    mScale(1),
    mDeltaX(0),
    mDeltaY(0),
    mFlipX(1),
    mFlipY(1)
{
    memset(mMatPixel2Phys, 0, sizeof(float)*9);
    memset(mVecApex, 0, sizeof(float)*3);

    printf("ScanConverterReference instance created\n");
}

//-----------------------------------------------------------------------------
ScanConverterReference::~ScanConverterReference() {
    printf("ScanConverterReference instance destroyed\n");
}

//-----------------------------------------------------------------------------
void ScanConverterReference::setZoomPanParams(float scale, float deltaX,
                                              float deltaY) {
    mScale = scale;
    mDeltaX = deltaX;
    mDeltaY = deltaY;
}

//-----------------------------------------------------------------------------
void ScanConverterReference::setZoomPanParams(float scale, float deltaX, float deltaY,
                                              float aspect) {
    mScale = scale;
    mDeltaX = deltaX;
    mDeltaY = deltaY;

    if (aspect < 0)
        mAspect = ((float) mScreenHeight)/((float) mScreenWidth);
    else
        mAspect = aspect;
}

//-----------------------------------------------------------------------------
void ScanConverterReference::setMappingTableScaleParams(float xyScale, float xShift,
                                                        float yShift) {
    mXYScale = xyScale;
    mXShift = xShift;
    mYShift = yShift;
}

//-----------------------------------------------------------------------------
void ScanConverterReference::setFlipXFlipY(float flipX, float flipY) {
    mFlipX = flipX;
    mFlipY = flipY;
}

//-----------------------------------------------------------------------------
void ScanConverterReference::setConversionParams(int numSamples, int numRays, int screenWidth,
                                                 int screenHeight, float startImgDepth,
                                                 float endImgDepth, float startSampleMm,
                                                 float sampleSpacing, float startRayRadian,
                                                 float raySpacing) {
    mNumSamples = numSamples;
    mNumRays = numRays;
    mScreenWidth = screenWidth;
    mScreenHeight = screenHeight;
    mStartImgDepth = startImgDepth;
    mEndImgDepth = endImgDepth;
    mStartSampleMm = startSampleMm;
    mSampleSpacingMm = sampleSpacing;
    mStartRayRadian = startRayRadian;
    mRaySpacing = raySpacing;
}

//-----------------------------------------------------------------------------
void ScanConverterReference::calculateCurrentPixel2PhysMatrix(float *mat) {
    // checking Mapping Table Scale, it should be positive
    if (mXYScale < 0.0f)
        mXYScale = 2.0f / (mEndImgDepth - mStartImgDepth);

    // checking Aspect. It should be positive
    if (mAspect < 0.0f)
        mAspect = ((float) mScreenHeight)/((float) mScreenWidth);

    // matrix
    float mat_p2n[9];
    float mat_n2s[9];
    float mat_s2pp[9];
    float mat_tmp[9];
    float mat_p2pp[9];
    float mat_pp2p[9];

    // phys to norm screen
    ViewMatrix3x3::setIdentityM(mat_p2n);
    mat_p2n[I(0,0)] = mXYScale;
    mat_p2n[I(1,1)] = -mXYScale;
    mat_p2n[I(0,2)] = mXShift;
    mat_p2n[I(1,2)] = -mYShift;

    // zoom pan parameters
    float mS = mScale;
    float mA = mAspect;
    float mDx = mDeltaX;
    float mDy = mDeltaY;

    // norm screen to ss screen
    ViewMatrix3x3::setIdentityM(mat_n2s);
    mat_n2s[I(0,0)] = mFlipX*mS*mA;
    mat_n2s[I(0,2)] = -mFlipX*mDx*mA;
    mat_n2s[I(1,1)] = mFlipY*mS;
    mat_n2s[I(1,2)] = mFlipY*mDy;   //mDy is actually negative in scaled screen coordinate

    // ss screen to pix
    ViewMatrix3x3::setIdentityM(mat_s2pp);
    mat_s2pp[I(0,0)] = ((float) mScreenWidth)/2.0f;
    mat_s2pp[I(0,2)] = ((float) mScreenWidth)/2.0f - 0.5f;
    mat_s2pp[I(1,1)] = ((float) mScreenHeight)/2.0f;
    mat_s2pp[I(1,2)] = ((float) mScreenHeight)/2.0f - 0.5f;

    // combined matrix then calculate inverse
    ViewMatrix3x3::multiplyMM(mat_tmp, mat_n2s, mat_p2n);
    ViewMatrix3x3::multiplyMM(mat_p2pp, mat_s2pp, mat_tmp);
    ViewMatrix3x3::inverseM(mat_pp2p, mat_p2pp);

    memcpy(mat, mat_pp2p, sizeof(mat_pp2p));
}

//-----------------------------------------------------------------------------
float ScanConverterReference::cubicInt(float y0, float y1, float y2, float y3, float mu) {
    float x = mu;
    float x2 = mu*mu;
    float x3 = mu*mu*mu;
    float wx =   -x3   + 3.0f*x2 - 3.0f*x + 1.0f;
    float wy =  3.0f*x3 - 6.0f*x2         + 4.0f;
    float wz = -3.0f*x3 + 3.0f*x2 + 3.0f*x + 1.0f;
    float ww =  x3;
    float rst = (y0*wx + y1*wy + y2*wz + y3*ww)/6.0f;

    return rst;
}

//-----------------------------------------------------------------------------
void ScanConverterReference::runConversionBiCubic4x4(u_char* inPtr, u_char* outPtr) {
    calculateCurrentPixel2PhysMatrix(mMatPixel2Phys);

    mVecApex[0] = 0.0f;
    mVecApex[1] = 0.0f;
    mVecApex[2] = 1.0f;

    runConversionBiCubic4x4(inPtr, outPtr,
                            mScreenWidth, mScreenHeight,
                            mMatPixel2Phys, mVecApex,
                            mStartImgDepth, mEndImgDepth,
                            mStartSampleMm, mSampleSpacingMm,
                            mNumSamples, mStartRayRadian,
                            mRaySpacing, mNumRays);
}

//-----------------------------------------------------------------------------
void ScanConverterReference::runConversionBiCubic4x4(u_char *inPtr, unsigned char *outPtr,
                                                     int screenWidth, int screenHeight,
                                                     float *matPixel2Phys, float *apexLocMm,
                                                     float startDepth, float endDepth,
                                                     float startSampleMm, float sampleSpacing,
                                                     int numSample, float startRayRadian,
                                                     float raySpacing, int numRay) {
    // pixel output
    float out_pix;
    float pix_loc[3] = {0.0f, 0.0f, 1.0f};

    float endSampleMm = startSampleMm + (numSample-1) * sampleSpacing;
    float endRayRadian = startRayRadian + (numRay - 1) * raySpacing;

    int i,j;
    float mm_loc[3];
    float x_dis, y_dis, xy_dis, radd;
    float x_loc, r_loc, x_frac, r_frac;

    int x_loc_int, r_loc_int;

    int y0_loc, y1_loc, y2_loc, y3_loc;
    int x0_loc, x1_loc, x2_loc, x3_loc;
    float y0q0, y1q0, y2q0, y3q0;
    float y0q1, y1q1, y2q1, y3q1;
    float y0q2, y1q2, y2q2, y3q2;
    float y0q3, y1q3, y2q3, y3q3;

    float yyq0, yyq1, yyq2, yyq3;

    for (j = 0; j < screenHeight; j++)
    {
        pix_loc[1] = ((float) j);

        for (i = 0; i < screenWidth; i++) {
            pix_loc[0] = ((float) i);
            // mm loc (phy)
            ViewMatrix3x3::multiplyMV(mm_loc, matPixel2Phys, pix_loc);
            x_dis = mm_loc[0] - apexLocMm[0];
            y_dis = mm_loc[1] - apexLocMm[1];

            xy_dis = hypot(x_dis, y_dis);
            radd = PI/2 - atan2(y_dis, x_dis);
            //radd = radd - PI/2;     // adjustment of angles (starting point)

            // out_pix calculation
            if (xy_dis < startSampleMm || xy_dis > endSampleMm || y_dis < startDepth || y_dis > endDepth ) {
                // out of range in screen or sample space
                out_pix = 0.0f;
            }
            else {
                if (radd < startRayRadian || radd > endRayRadian) {
                    // out of range in ray space
                    out_pix = 0.0f;
                }
                else {
                    // interpolation
                    x_loc = (xy_dis - startSampleMm)/sampleSpacing;
                    r_loc = (radd - startRayRadian)/raySpacing;
                    x_loc_int = (int) floor(x_loc);
                    x_frac = x_loc - x_loc_int;
                    r_loc_int = (int) floor(r_loc);
                    r_frac = r_loc - r_loc_int;

                    //ALOGD("i: %f, j: %f, x_dis: %f, y_dis: %f, x_loc: %d, r_loc: %d", pix_loc[0], pix_loc[1], x_dis, y_dis, x_loc_int, r_loc_int);

                    if (x_loc >= 0.0f && x_loc < (float)(numSample - 1) && r_loc >= 0.0f && r_loc < (float)(numRay - 1)) {
                        y0_loc = r_loc_int - 1;
                        y1_loc = r_loc_int;
                        y2_loc = r_loc_int + 1;
                        y3_loc = r_loc_int + 2;

                        if (r_loc_int == 0)
                            y0_loc = 0;
                        else if (r_loc_int ==  numRay - 2)
                            y3_loc = r_loc_int + 1;

                        x0_loc = x_loc_int - 1;
                        x1_loc = x_loc_int;
                        x2_loc = x_loc_int + 1;
                        x3_loc = x_loc_int + 2;

                        if (x_loc_int == 0)
                            x0_loc = 0;
                        else if (x_loc_int == numSample - 2)
                            x3_loc = x_loc_int + 1;

                        y0q0 = (float)inPtr[y0_loc * numSample + x0_loc];
                        y1q0 = (float)inPtr[y1_loc * numSample + x0_loc];
                        y2q0 = (float)inPtr[y2_loc * numSample + x0_loc];
                        y3q0 = (float)inPtr[y3_loc * numSample + x0_loc];
                        yyq0 = cubicInt(y0q0, y1q0, y2q0, y3q0, r_frac);

                        y0q1 = (float)inPtr[y0_loc * numSample + x1_loc];
                        y1q1 = (float)inPtr[y1_loc * numSample + x1_loc];
                        y2q1 = (float)inPtr[y2_loc * numSample + x1_loc];
                        y3q1 = (float)inPtr[y3_loc * numSample + x1_loc];
                        yyq1 = cubicInt(y0q1, y1q1, y2q1, y3q1, r_frac);

                        y0q2 = (float)inPtr[y0_loc * numSample + x2_loc];
                        y1q2 = (float)inPtr[y1_loc * numSample + x2_loc];
                        y2q2 = (float)inPtr[y2_loc * numSample + x2_loc];
                        y3q2 = (float)inPtr[y3_loc * numSample + x2_loc];
                        yyq2 = cubicInt(y0q2, y1q2, y2q2, y3q2, r_frac);

                        y0q3 = (float)inPtr[y0_loc * numSample + x3_loc];
                        y1q3 = (float)inPtr[y1_loc * numSample + x3_loc];
                        y2q3 = (float)inPtr[y2_loc * numSample + x3_loc];
                        y3q3 = (float)inPtr[y3_loc * numSample + x3_loc];
                        yyq3 = cubicInt(y0q3, y1q3, y2q3, y3q3, r_frac);

                        out_pix = round(cubicInt(yyq0, yyq1, yyq2, yyq3, x_frac));

                        if (out_pix > 255.0f)
                            out_pix = 255.0f;
                        else if (out_pix < 0.0f)
                            out_pix = 0.0f;
                    }
                    else {
                        out_pix = 0.0f;
                    }
                }
            }  // end of if else

            outPtr[j*screenWidth + i] = (u_char) out_pix;

        }   // end i

    }   // end j

}
