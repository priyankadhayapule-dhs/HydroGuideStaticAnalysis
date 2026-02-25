//
// Copyright 2020 EchoNous Inc.
//

#define LOG_TAG "ScrollModeRenderer"
#define SM_TRACELINE_THICKNESS 2
#define MAX_TEXTURE_WIDTH 7000

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <GLES3/gl3.h>
#include <ThorUtils.h>
#include <ScrollModeRenderer.h>

const char* const ScrollModeRenderer::mVertexShaderSource =
#include "mmode_vertex.glsl"
;

const char* const ScrollModeRenderer::mFragmentShaderSource =
#include "mmode_fragment.glsl"
;

const char *const ScrollModeRenderer::mLineVertexShaderSource =
#include "og_vertex.glsl"
;

const char *const ScrollModeRenderer::mLineFragmentShaderSource =
#include "og_fragment.glsl"
;

//-----------------------------------------------------------------------------
ScrollModeRenderer::ScrollModeRenderer() :
    Renderer(),
    mCineBufferPtr(nullptr),
    mDataType(DAU_DATA_TYPE_PW),
    mTexFrameType(TEX_FRAME_TYPE_PW),
    mProgram(0),
    mTextureHandle(0),
    mTextureUniformHandle(-1),
    mColorHandle(-1),
    mTintHandle(-1),
    mInputWidth(1000),
    mYPhysicalScale(2.0f),
    mJitterAmt(0.0),
    mJitterSum(0.0),
    mDrawingJitterSum(0.0),
    mNumLinesPerFrame(0.0),
    mDataIndex(0),
    mDrawIndex(0),
    mNumSamples(256),
    mScrollSpeed(1000.0),
    mScrollRate(0.0),
    mNumFramesDraw(0.0),
    mNumFramesDrawPerFeed(1.0),
    mDrawingWrapped(false),
    mIsSingleFrameTextureMode(false),
    mWholeScreenRedraw(false),
    mOffScreenDrawing(false),
    mLastDrawnWholeScreen(false),
    mMinFrameNum(0),
    mMaxFrameNum(0),
    mPrevFrameNum(-1),
    mSingleFrameDrawIndex(0),
    mYExpScale(1.0f),
    mBaselineShiftFloat(0.0f),
    mInvert(false),
    mUpdateBaselineShiftInvertBuffer(false),
    mPeakPointPtr(nullptr),
    mMeanPointPtr(nullptr),
    mDrawPeakLines(false),
    mDrawMeanLines(false),
    mCalcPeakLines(false),
    mPeakMeanLineThickness(3),
    mForcedFSR(false)
{
    // Tint adjustment
    mTintAdj[0] = 0.84f;
    mTintAdj[1] = 0.88f;
    mTintAdj[2] = 0.90f;

    memset(mVB, 0, sizeof(mVB));
}

//-----------------------------------------------------------------------------
ScrollModeRenderer::~ScrollModeRenderer()
{
    close();
}

//-----------------------------------------------------------------------------
ThorStatus ScrollModeRenderer::open()
{
    ThorStatus      retVal = THOR_ERROR;

    float depthScale;
    float scrollTime;
    float timeScale;
    float imagingDepth;

    // buffer location and binding
    float locData[32];          // 4*4*2
    short idxData[10];          // (4+1)*2

    // Peak Mean drawing related params
    float           halfWidth;

    // Check the input width - should be smaller than MAX_TEXTURE_WIDTH
    if (mInputWidth > MAX_TEXTURE_WIDTH)
    {
        ALOGE("Input Width (pre-scale) is too big: %d", mInputWidth);
        goto err_ret;
    }

    mProgram = createProgram(mVertexShaderSource, mFragmentShaderSource);
    if (!checkGlError("createProgram")) goto err_ret;

    glGenTextures(1, &mTextureHandle);
    if (!checkGlError("glGenTextures")) goto err_ret;

    glBindTexture(GL_TEXTURE_2D, mTextureHandle);
    if (!checkGlError("glBindTexture")) goto err_ret;

    // texture parameter
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (!checkGlError("glTexParameteri")) goto err_ret;

    mTextureUniformHandle = glGetUniformLocation(mProgram, "u_MTexture");   // re-use M mode Shader
    mTintHandle = glGetUniformLocation(mProgram, "u_tintAdj");              // Tint Adj. Handle
    if (!checkGlError("glGetUniformLocation")) goto err_ret;

    // texture init
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mTextureHandle);
    if (!checkGlError("BindScrollModeTexture")) goto err_ret;

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_LUMINANCE,
                 mNumSamples,
                 mInputWidth,
                 0,
                 GL_LUMINANCE,
                 GL_UNSIGNED_BYTE,
                 0);

    if (!checkGlError("glTexImage2D-ScrollMode-Initialize")) goto err_ret;

    glGenBuffers(4, mVB);   //  2 for Scroll & 2 for Mean/Max.

    // before calling this calculation function, all the params need to be set
    calculateBaselineShiftCoord(locData, idxData);

    glBindBuffer(GL_ARRAY_BUFFER, mVB[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 32, locData, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVB[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(short) * 10, idxData, GL_STATIC_DRAW);

    glVertexAttribPointer(3, 4, GL_FLOAT, false, sizeof(float) * 4, 0);
    glEnableVertexAttribArray(3);

    // init variables
    mNumFramesDraw = 0.0;
    mDataIndex = 0;
    mDrawIndex = 0;

    // preparing transformation matrix
    scrollTime = ((float) mInputWidth) / mScrollSpeed;
    timeScale = 2.0f / scrollTime;
    // init ScHelper, set Scale and XY shift
    mScHelper.init();

    if (scrollTime > 0.0f)
    {
        // y-direction adjustment so -ve in Y scale
        mScHelper.setScaleXYShift(timeScale, -2.0f/mYPhysicalScale, -1.0f, mBaselineShiftFloat*2.0f);
        // set aspect ratio (this matrix is a special case)
        mScHelper.setAspect(1.0f);

        // if invert, flipY
        if (mInvert)
            mScHelper.setFlip(1.0f, -1.0f);
    }

    // Trace line + Peak Mean/Max drawing related setups
    mLineProgram = createProgram(mLineVertexShaderSource, mLineFragmentShaderSource);
    if (!checkGlError("createLineProgram"))
        goto err_ret;

    mColorHandle = glGetUniformLocation(mLineProgram, "u_drawColor");
    if (!checkGlError("glGetUniformLocation"))
        goto err_ret;

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glVertexAttribPointer(4, 2, GL_FLOAT, false, sizeof(float) * 2, 0);
    glEnableVertexAttribArray(4);
    if (!checkGlError("glVertexAttribPointer"))
        goto err_ret;

    // -----  point arrays for Peak Mean/Max -----
    mPeakPointPtr = new point[mInputWidth];
    mMeanPointPtr = new point[mInputWidth];

    halfWidth = ((float) mInputWidth)/2.0f;
    for (int i = 0; i < mInputWidth; i++)
    {
        float x = (i - halfWidth - 0.5f) / halfWidth;

        mPeakPointPtr[i].x = x;
        mPeakPointPtr[i].y = 0.0f;

        mMeanPointPtr[i].x = x;
        mMeanPointPtr[i].y = 0.0f;
    }

    // Peak Max
    glBindBuffer(GL_ARRAY_BUFFER, mVB[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(point) * mInputWidth, mPeakPointPtr, GL_DYNAMIC_DRAW);

    // Mean
    glBindBuffer(GL_ARRAY_BUFFER, mVB[3]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(point) * mInputWidth, mMeanPointPtr, GL_DYNAMIC_DRAW);

    // Peak color
    mPeakColor[0] = 1.0f;
    mPeakColor[1] = 0.9333f;
    mPeakColor[2] = 0.0f;

    // Mean Color
    mMeanColor[0] = 0.3059f;
    mMeanColor[1] = 0.1686f;
    mMeanColor[2] = 0.5608f;


    mLineColor[0] = 1.0f;
    mLineColor[1] = 0.427f;
    mLineColor[2] = 0.0f;

    // clear the section screen
    glEnable(GL_SCISSOR_TEST);
    glScissor(getX(), getY(), getWidth(), getHeight());
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mDrawingWrapped = false;
    mWholeScreenRedraw = false;
    mOffScreenDrawing = false;

    // initialize JitterSums
    mJitterSum = 0.0f;
    mDrawingJitterSum = 0.0f;

    retVal = THOR_OK;

err_ret:
    return(retVal);
}

//-----------------------------------------------------------------------------
/*
 * point location in normlaized coordinate
 *
 *                  ^ y
 *                  |
 *  p0 ------------------------------- p1
 *  |               |                  |
 *  |        y_shift_from_end          |
 *  p4 ------------------------------- p5
 *  |                                  |   -> x
 *  |                                  |
 *  |                                  |
 *  p2 ------------------------------- p3
 *
 *  mBaselineShift -  0 value => center
 *      +ve shift => move toward +y direction
 *
 *  // shifted coordinate value (in terms of Pixels)
 *  int   y_shift_from_end = mNumSamples/2 - mBaselineShiftPixels;       // y_shift from top
 *  float u_x_shift = ((float) mNumSamples - 1 - y_shift_from_end)/mNumSamples;
 *  float y_shift = y_end - ((float) y_shift_from_end)/((float)(mNumSamples - 1)) * (y_end - y_start);
 *
 */
void ScrollModeRenderer::calculateBaselineShiftCoord(float* locData, short* idxData)
{
    // expansion scale value (zoom in y-direction) = mYExpScale

    // calculate array and element buffer for texture mapping with baseline shift
    float x_start = -1.0;
    float x_end   = 1.0;

    float y_start = -1.0 * mYExpScale;
    float y_end   = 1.0 * mYExpScale;

    float u_x = 0.5f / mNumSamples;
    float u_y = 0.5f / mInputWidth;

    // shifted coordinate value
    float u_x_shift = 0.5f - mBaselineShiftFloat * (1.0f - u_x * 2.0f) ;
    float y_shift = y_end - (0.5f + mBaselineShiftFloat) * 2.0f * mYExpScale;

    // invert the y-axis: FFT output order is downward, so should be inverted by default.
    if (!mInvert)
    {
        y_start = -y_start;
        y_end = -y_end;
        y_shift = -y_shift;

        //u_x_shift = 1.0f - u_x_shift;
    }

    // mYShift
    mYShiftFloat = y_shift;

    float p0_x = x_start;
    float p0_y = y_end;

    float p1_x = x_end;
    float p1_y = y_end;

    float p2_x = x_start;
    float p2_y = y_start;

    float p3_x = x_end;
    float p3_y = y_start;

    float p4_x = x_start;
    float p4_y = y_shift;

    float p5_x = x_end;
    float p5_y = y_shift;

    // top rectangle (+ve y side)
    locData[0] = p0_x;
    locData[1] = p0_y;
    locData[2] = u_x_shift;
    locData[3] = u_y;

    locData[4] = p1_x;
    locData[5] = p1_y;
    locData[6] = u_x_shift;
    locData[7] = 1.0f - u_y;

    locData[8]  = p4_x;
    locData[9]  = p4_y;
    locData[10] = 1.0f - u_x;
    locData[11] = u_y;

    locData[12] = p5_x;
    locData[13] = p5_y;
    locData[14] = 1.0f - u_x;
    locData[15] = 1.0f - u_y;

    // bottom rectangle (-ve y side)
    locData[16] = p4_x;
    locData[17] = p4_y;
    locData[18] = u_x;
    locData[19] = u_y;

    locData[20] = p5_x;
    locData[21] = p5_y;
    locData[22] = u_x;
    locData[23] = 1.0f - u_y;

    locData[24] = p2_x;
    locData[25] = p2_y;
    locData[26] = u_x_shift;
    locData[27] = u_y;

    locData[28] = p3_x;
    locData[29] = p3_y;
    locData[30] = u_x_shift;
    locData[31] = 1.0f - u_y;

    // top side
    idxData[0] = 0;
    idxData[1] = 1;
    idxData[2] = 2;
    idxData[3] = 3;
    idxData[4] = 3;

    // bottom side
    idxData[5] = 4;
    idxData[6] = 4;
    idxData[7] = 5;
    idxData[8] = 6;
    idxData[9] = 7;
}

//-----------------------------------------------------------------------------
void ScrollModeRenderer::updateBaselineShiftInvert(float baselineShift, bool isInvert)
{
    // update baslineshift and invert params
    mBaselineShiftFloat = baselineShift;
    mInvert = isInvert;

    // update transformation matrix
    float scrollTime = ((float) mInputWidth) / mScrollSpeed;
    float timeScale = 2.0f / scrollTime;

    if (scrollTime > 0.0f)
    {
        // y-direction adjustment so -ve in Y scale
        mScHelper.setScaleXYShift(timeScale, -2.0f/mYPhysicalScale, -1.0f, mBaselineShiftFloat*2.0f);
        // set aspect ratio (this matrix is a special case)
        mScHelper.setAspect(1.0f);

        // if invert, flipY
        if (mInvert)
            mScHelper.setFlip(1.0f, -1.0f);
    }

    // set flag
    mUpdateBaselineShiftInvertBuffer = true;
}

//-----------------------------------------------------------------------------
void ScrollModeRenderer::updateBaselineShiftInvertBuffer()
{
    float locData[32];
    short idxData[10];      // dummy
    calculateBaselineShiftCoord(locData, idxData);

    glBindBuffer(GL_ARRAY_BUFFER, mVB[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 32, locData, GL_STATIC_DRAW);

    checkGlError("updateBaselineShiftInvertBuffer - Array Buffer");

    // set flags - whole screen redraw & shiftbufferupdate
    mWholeScreenRedraw = true;
    mUpdateBaselineShiftInvertBuffer = false;
}

//-----------------------------------------------------------------------------
void ScrollModeRenderer::close()
{
    // unbind texture
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (0 != mTextureHandle)
    {
        glDeleteTextures(1, &mTextureHandle);
        mTextureHandle = 0;
    }

    // Array/Index Buffers
    for (int i = 0; i < 4; i++)
    {
        if (0 != mVB[i])
        {
            glDeleteBuffers(1, &mVB[i]);
            mVB[i] = 0;
        }
    }

    // clean up Peak Mean ptrs
    if (mPeakPointPtr != nullptr)
    {
        delete[] mPeakPointPtr;
        mPeakPointPtr = nullptr;
    }
    if (mMeanPointPtr != nullptr)
    {
        delete[] mMeanPointPtr;
        mMeanPointPtr = nullptr;
    }
}

//-----------------------------------------------------------------------------
ThorStatus ScrollModeRenderer::prepareRenderer(CineBuffer *cineBuffer, int dataType)
{
    ThorStatus retVal = THOR_ERROR;

    mCineBufferPtr = cineBuffer;
    mDataType = dataType;

    switch (mDataType)
    {
        case DAU_DATA_TYPE_M:
            mTexFrameType = TEX_FRAME_TYPE_M;
            break;

        case DAU_DATA_TYPE_PW:
            mTexFrameType = TEX_FRAME_TYPE_PW;
            break;

        case DAU_DATA_TYPE_CW:
            mTexFrameType = TEX_FRAME_TYPE_CW;
            break;

        default:
            mTexFrameType = TEX_FRAME_TYPE_PW;
            break;
    }

    // if M-mode disable Peak Mean drawing
    if (mDataType == DAU_DATA_TYPE_M)
    {
        mDrawPeakLines = false;
        mDrawMeanLines = false;
        mCalcPeakLines = false;
    }
    else
        mCalcPeakLines = true;

    // init variables
    mPrevFrameNum = -1;
    mWholeScreenRedraw = false;
    mOffScreenDrawing = false;
    mNumFramesDraw = 0.0f;
    mDataIndex = 0;
    mDrawIndex = 0;

    // get min/max frame numbers
    mMinFrameNum = mCineBufferPtr->getMinFrameNum();
    mMaxFrameNum = mCineBufferPtr->getMaxFrameNum();

    // texture init
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mTextureHandle);
    if (!checkGlError("BindScrollModeTexture"))
        goto err_ret;

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_LUMINANCE,
                 mNumSamples,
                 mInputWidth,
                 0,
                 GL_LUMINANCE,
                 GL_UNSIGNED_BYTE,
                 0);

    if (!checkGlError("glTexImage2D-ScrollMode-Initialize"))
        goto err_ret;

    retVal = THOR_OK;

err_ret:
    return (retVal);
}

//-----------------------------------------------------------------------------
ThorStatus ScrollModeRenderer::prepareRendererOffScreen(int minFrameNum, int maxFrameNum, int dataType)
{
    ThorStatus retVal = THOR_ERROR;

    mCineBufferPtr = nullptr;
    mDataType = dataType;

    // get min/max frame numbers
    mMinFrameNum = minFrameNum;
    mMaxFrameNum = maxFrameNum;

    switch (mDataType)
    {
        case DAU_DATA_TYPE_M:
            mTexFrameType = TEX_FRAME_TYPE_M;
            break;

        case DAU_DATA_TYPE_PW:
            mTexFrameType = TEX_FRAME_TYPE_PW;
            break;

        case DAU_DATA_TYPE_CW:
            mTexFrameType = TEX_FRAME_TYPE_CW;
            break;

        default:
            mTexFrameType = TEX_FRAME_TYPE_PW;
            break;
    }

    // clean up the screen - whole renderer section
    glScissor(getX(), getY(), getWidth(), getHeight());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (minFrameNum == maxFrameNum)
    {
        mWholeScreenRedraw = true;
    }
    else
    {
        mWholeScreenRedraw = false;
    }

    mOffScreenDrawing = true;
    mPrevFrameNum = -1;

    // texture init
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mTextureHandle);
    if (!checkGlError("BindScrollModeTexture"))
        goto err_ret;

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_LUMINANCE,
                 mNumSamples,
                 mInputWidth,
                 0,
                 GL_LUMINANCE,
                 GL_UNSIGNED_BYTE,
                 0);

    if (!checkGlError("glTexImage2D-ScrollMode-Initialize"))
        goto err_ret;

    retVal = THOR_OK;

err_ret:
    return (retVal);
}

//-----------------------------------------------------------------------------
void ScrollModeRenderer::updateCineBufferMinMaxFrameNum()
{
    if (mCineBufferPtr == nullptr)
        return;

    // get min/max frame numbers
    mMinFrameNum = mCineBufferPtr->getMinFrameNum();
    mMaxFrameNum = mCineBufferPtr->getMaxFrameNum();
}

//-----------------------------------------------------------------------------
void ScrollModeRenderer::setSingleFrameTexturePtr(uint8_t* texturePtr, int textureWidth)
{
    mLock.enter();

    if ((textureWidth < 1)||(textureWidth > mInputWidth))
        textureWidth = mInputWidth;

    mSingleFrameTextureWidth = textureWidth;

    // prepare Texture unit for singleFrame Texture mode
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mTextureHandle);
    checkGlError("BindScrollModeTexture-SingleFrameTexture");

    glTexSubImage2D(GL_TEXTURE_2D,
                    0,
                    0,
                    0,
                    mNumSamples,
                    textureWidth,
                    GL_LUMINANCE,
                    GL_UNSIGNED_BYTE,
                    texturePtr);

    checkGlError("glTexImage2D-ScrollMode-SingleFrameTexture");

    mLock.leave();
}

//-----------------------------------------------------------------------------
void ScrollModeRenderer::setSingleFramePeakMeanTexturePtr(uint8_t* texturePtr, int textureWidth)
{
    mLock.enter();

    if ((textureWidth < 1)||(textureWidth > mInputWidth))
        textureWidth = mInputWidth;

    float       signAdj = 1.0f;
    float*      framePtrFloat;
    framePtrFloat = (float*) texturePtr;

    if (mInvert)
        signAdj = -1.0f;

    for (int i = 0; i < textureWidth; i++)
    {
        mPeakPointPtr[i].y = (signAdj*(*framePtrFloat++) * mYExpScale) + mYShiftFloat;
        mMeanPointPtr[i].y = (signAdj*(*framePtrFloat++) * mYExpScale) + mYShiftFloat;
    }

    // Peak Buffer
    glBindBuffer(GL_ARRAY_BUFFER, mVB[PEAK_BUFFER]);
    glBufferSubData(GL_ARRAY_BUFFER,
                    0,
                    sizeof(point) * textureWidth,
                    &mPeakPointPtr[0]);

    // Mean Buffer
    glBindBuffer(GL_ARRAY_BUFFER, mVB[MEAN_BUFFER]);
    glBufferSubData(GL_ARRAY_BUFFER,
                    0,
                    sizeof(point) * textureWidth,
                    &mMeanPointPtr[0]);

    mLock.leave();
}

//-----------------------------------------------------------------------------
ThorStatus ScrollModeRenderer::prepareFreezeMode(int inFrameNum, bool updateIndices)
{
    ThorStatus retVal = THOR_ERROR;

    uint8_t *framePtr, *texRefPtr, *texPtr;
    CineBuffer::CineFrameHeader* cineHeaderPtr;
    int     curFrameNum, endFrameNum, frameWidth;
    float   inputWidthWithJitter;
    float   dataSpan, divVal, dataIndexFloat;
    int     totalInSampleIndex, estimatedNumLines, curNumLines;
    int     startFrameNum, startDataIndex, endDataIndex;
    int     realMinFrameNum;
    int     numLinesTransfer1, numLinesTransfer2;
    bool    wrapped;
    bool    jitterAdjusted = false;
    int     startDataIndex_Pre;
    uint64_t    headerTimeStamp;

    mLock.enter();

    // check frame Nnumber range
    if (inFrameNum < 0)
        inFrameNum = mPrevFrameNum;

    // for singleFrameTextureMode
    if (mIsSingleFrameTextureMode)
    {
        // single frame texture mode.
        texPtr = mCineBufferPtr->getFrame(mTexFrameType, DAU_DATA_TYPE_TEX,
                CineBuffer::FrameContents::IncludeHeader);

        cineHeaderPtr = reinterpret_cast<CineBuffer::CineFrameHeader*>(texPtr);
        mMaxDrawIndex = cineHeaderPtr->numSamples - 1;
        mDrawIndex = mSingleFrameDrawIndex;

        retVal = THOR_OK;
        goto ok_ret;
    }

    // texRefPtr;
    texRefPtr = mCineBufferPtr->getFrame(mTexFrameType, DAU_DATA_TYPE_TEX);

    // mMinFrameNum is not updated when the system gets into CineReview.
    // However, it is updated (typically to 0) when it gets into ExamReview.
    // real MinFrameNum: calculated based on inFrameNum and CINE_FRAME_COUNT, as we cannot use
    // mMinFrameNum for CineReview and LiveMode.
    realMinFrameNum = inFrameNum - CINE_FRAME_COUNT + 1 - mMinFrameNum;
    if (realMinFrameNum < 0)
        realMinFrameNum = 0;

    // relative current frame number
    endFrameNum = inFrameNum - mMinFrameNum;        // in live mode mMinFrameNum is always 0
    inputWidthWithJitter = (((float) mInputWidth) + mJitterAmt);
    frameWidth = (int) ceil(inputWidthWithJitter/mNumLinesPerFrame);
    startFrameNum = endFrameNum - frameWidth + 1;

    if (startFrameNum < realMinFrameNum)
        startFrameNum = realMinFrameNum;

    totalInSampleIndex = startFrameNum * mOrgNumLinesPerFrame - 1;
    estimatedNumLines = floor(totalInSampleIndex / mReSampleRatio) + 1;
    if (round((floor(totalInSampleIndex / mReSampleRatio) + 1) * mReSampleRatio) <= totalInSampleIndex)
        estimatedNumLines += 1;

    dataSpan = (float) estimatedNumLines;
    divVal = dataSpan / inputWidthWithJitter;
    dataIndexFloat = dataSpan - floor(divVal) * inputWidthWithJitter;

    startDataIndex = (int) ceil(dataIndexFloat);
    if (startDataIndex < 0)
        startDataIndex = 0;

    mJitterSum = ((float) startDataIndex) - dataIndexFloat;
    if (mJitterSum < 0.0f)
        mJitterSum = 0.0f;

    mDataWidth = 0;

    // curFrameNum
    curFrameNum  = startFrameNum + mMinFrameNum;

    // check DATA TYPE is available.
    framePtr = mCineBufferPtr->getFrame(curFrameNum, mDataType,
                                        CineBuffer::FrameContents::IncludeHeader);
    if (framePtr == nullptr)
        goto err_ret;

    // timestamp
    cineHeaderPtr = reinterpret_cast<CineBuffer::CineFrameHeader*>(framePtr);
    headerTimeStamp = cineHeaderPtr->timeStamp;

    // store index for Peak Mean Frames
    startDataIndex_Pre = startDataIndex;

    // concatenate frames to build a texture
    while(curFrameNum <= (endFrameNum + mMinFrameNum))
    {
        // read from header
        framePtr = mCineBufferPtr->getFrame(curFrameNum, mDataType,
                                            CineBuffer::FrameContents::IncludeHeader);
        cineHeaderPtr = reinterpret_cast<CineBuffer::CineFrameHeader*>(framePtr);

        // framePtr for data location
        framePtr += sizeof(CineBuffer::CineFrameHeader);
        // current number of lines in the frameheader
        curNumLines = cineHeaderPtr->numSamples;

        // when num lines are not stored or different too much.
        if ((mDataType == DAU_DATA_TYPE_M) || (abs(((float) curNumLines) - mNumLinesPerFrame) > 1.0f))
            curNumLines = (int) mNumLinesPerFrame;

        mDataWidth += curNumLines;

        curFrameNum++;

        endDataIndex = startDataIndex + curNumLines - 1;
        wrapped = false;

        // endDataIndex is wrapped
        if (endDataIndex > mInputWidth - 1)
        {
            wrapped = true;
        }

        if (!wrapped)
        {
            memcpy(texRefPtr + mNumSamples*startDataIndex, framePtr, mNumSamples*curNumLines);
        }
        else
        {
            numLinesTransfer1 = mInputWidth - startDataIndex;
            // Transfer 1
            if (numLinesTransfer1 > 0) {
                memcpy(texRefPtr + mNumSamples * startDataIndex, framePtr,
                       mNumSamples * numLinesTransfer1);
            }
            else
            {
                numLinesTransfer1 = 0;
            }

            // wrapped - Jitter adjustment
            mJitterSum += mJitterAmt;

            if (mJitterSum >= 1.0f) {
                // Jitter Adjusted
                numLinesTransfer1 += 1;
                jitterAdjusted = true;
            }

            numLinesTransfer2 = curNumLines - numLinesTransfer1;
            if (numLinesTransfer2 > 0)
            {
                //Transfer2 - wrapped so line no. 0
                memcpy(texRefPtr, framePtr + mNumSamples * numLinesTransfer1, mNumSamples*numLinesTransfer2);
            }
        }

        // update startDataIndex
        startDataIndex += curNumLines;

        if (startDataIndex > mInputWidth - 1)
        {
            startDataIndex -= mInputWidth;
        }
    }

    // preparePeakMeanFrames for FreezeMode
    if (mCalcPeakLines)
        preparePeakMeanFreezeMode(startFrameNum, endFrameNum, startDataIndex_Pre, jitterAdjusted);

    mSingleFrameDrawIndex = endDataIndex;

    if (updateIndices) {
        mDrawingJitterSum = mJitterSum;
        endDataIndex = startDataIndex;

        mDataIndex = endDataIndex;
        mDrawIndex = endDataIndex;
    }

    // Min Max Draw Index
    if (mDataWidth < mInputWidth)
        mMaxDrawIndex = endDataIndex - 1;
    else
        mMaxDrawIndex = mInputWidth - 1;

    if (mDataWidth > mInputWidth)
        mDataWidth = mInputWidth;

    // update Tex Buffer header
    texPtr = mCineBufferPtr->getFrame(mTexFrameType, DAU_DATA_TYPE_TEX,
                                      CineBuffer::FrameContents::IncludeHeader);
    cineHeaderPtr = reinterpret_cast<CineBuffer::CineFrameHeader*>(texPtr);
    cineHeaderPtr->numSamples = mDataWidth;
    cineHeaderPtr->frameNum = inFrameNum;
    cineHeaderPtr->timeStamp = headerTimeStamp;

    retVal = THOR_OK;

ok_ret:
err_ret:
    mLock.leave();
    return (retVal);
}

//-----------------------------------------------------------------------------
bool ScrollModeRenderer::prepare()
{
    // update baselineShift and Invert Array Buffer if set
    if (mUpdateBaselineShiftInvertBuffer)
        updateBaselineShiftInvertBuffer();

    // place holder for checking validity of indices (dataindex and drawindex)
    if ((mNumFramesDraw < 1.0f) && !mWholeScreenRedraw)
    {
        return(false);
    }
    else
        return(true);
}

//-----------------------------------------------------------------------------
bool ScrollModeRenderer::updateIndices()
{
    bool        retVal = false;
    int         numBufLines;
    int         numLinesDraw;
    int         scrollRateInt;
    float       jitter;

    // index update
    scrollRateInt = ((int) mScrollRate);
    jitter = mScrollRate - ((float) scrollRateInt);
    mDrawingJitterSum += jitter;

    if (mDataIndex >= mDrawIndex) {
        numBufLines = mDataIndex - mDrawIndex;
    }
    else {
        numBufLines = mDataIndex + mInputWidth - mDrawIndex;
    }

    if (mNumFramesDraw >= 2.0f)
    {
        numBufLines = numBufLines / ((int) mNumFramesDraw);
    }

    // estimate number of lines to draw
    if (numBufLines > scrollRateInt * 2)
    {
        numLinesDraw = scrollRateInt * 2;
    }
    else if (numBufLines > scrollRateInt)
    {
        numLinesDraw = scrollRateInt;

        if (mDrawingJitterSum >= 1.0f)
        {
            mDrawingJitterSum -= 1.0f;
            numLinesDraw++;
        }
    }
    else
    {
        numLinesDraw = numBufLines;
    }

    mStartLineIdx = mDrawIndex;
    mEndLineIdx = mDrawIndex + numLinesDraw - 1;

    if (mEndLineIdx > mInputWidth - 1) {
        mStartLineIdx = 0;
        mEndLineIdx -= mInputWidth;
        mDrawingWrapped = true;
    }

    if (numLinesDraw == 0) {
        ALOGD("Not Enough Data for Drawing Any Lines");
        goto err_ret;
    }

    if (numBufLines > 10 * scrollRateInt)
    {
        ALOGW("Too much data in the Buffer - Please check feed rate and consume rate!");
    }

    retVal = true;

err_ret:
    return(retVal);
}

//-----------------------------------------------------------------------------
uint32_t ScrollModeRenderer::draw()
{
    uint32_t    retNumDraw = 0;
    int         startPoint;
    int         endPoint;
    int         wrapStartPoint;
    float       lineX;

    if (mWholeScreenRedraw)
    {
        if (!mOffScreenDrawing)
        {
            prepareFreezeMode(-1);
            mNumFramesDraw = 0.0f;

            setSingleFrameTexturePtr(mCineBufferPtr->getFrame(mTexFrameType, DAU_DATA_TYPE_TEX), mInputWidth);

            // only when the render is in singleFrameTextureMode
            if ((mDrawPeakLines || mDrawMeanLines))
                setSingleFramePeakMeanTexturePtr(mCineBufferPtr->getFrame(0, DAU_DATA_TYPE_DPMAX_SCR), mInputWidth);
        }
        else if (mIsSingleFrameTextureMode)
        {
            // offScreen SingleFrameTextureMode
            mMaxDrawIndex = mSingleFrameTextureWidth - 1;
            mDrawIndex = mSingleFrameDrawIndex;
        }

        drawFreezeMode();

        mWholeScreenRedraw = false;
        goto done;
    }

    if (!updateIndices())
        goto done;

    glUseProgram(mProgram);
    checkGlError("glUseProgram");

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mTextureHandle);

    glUniform1i(mTextureUniformHandle, 3);

    // tint adjustment
    glUniform3fv(mTintHandle, 1, mTintAdj);
    checkGlError("glUniform3fv");

    glEnable(GL_SCISSOR_TEST);
    // additional drawing when the drawing is wrapped around.
    if (mDrawingWrapped)
    {
        if (mDrawIndex > 0)
        {
            wrapStartPoint = (int) floor(((float) mDrawIndex)/((float) mInputWidth) * getWidth()) - 5;
        }
        else
        {
            wrapStartPoint = getWidth() - 4;
        }

        // when the drawing is wrapped.
        glScissor(getX() + wrapStartPoint - SM_TRACELINE_THICKNESS - 6, getY(),
                  getWidth() - wrapStartPoint + 2 * SM_TRACELINE_THICKNESS + 3, getHeight());
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindBuffer(GL_ARRAY_BUFFER, mVB[0]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVB[1]);
        glDrawElements(GL_TRIANGLE_STRIP, 10, GL_UNSIGNED_SHORT, 0);
        checkGlError("glDrawArrays");

        if (mDrawPeakLines)
            renderPeakMeanLines(PEAK_BUFFER);
        if (mDrawMeanLines)
            renderPeakMeanLines(MEAN_BUFFER);

        mDrawingWrapped = false;
    }

    glUseProgram(mProgram);
    checkGlError("glUseProgram");

    // for forced Full screen render -> for S9
    if (mForcedFSR)
    {
        if (mStartLineIdx < mNumLinesPerFrame * 5.0f)
        {
            // re-draw right-side strip
            glScissor(getX() + getWidth() - mNumLinesPerFrame * 3.0f, getY(),
                      mNumLinesPerFrame * 3.0f + 5, getHeight());
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glBindBuffer(GL_ARRAY_BUFFER, mVB[0]);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVB[1]);
            glDrawElements(GL_TRIANGLE_STRIP, 10, GL_UNSIGNED_SHORT, 0);
            checkGlError("glDrawArrays - right-end");
        }

        // set startline index to 0
        mStartLineIdx = 0;
    }

    startPoint = ((int) floor(((float) mStartLineIdx)/((float) mInputWidth) * getWidth())) - SM_TRACELINE_THICKNESS - 8;
    endPoint = ((int) floor(((float) mEndLineIdx)/((float) mInputWidth) * getWidth()));

    lineX = 2.0f * ((float) mEndLineIdx)/((float) mInputWidth) - 1.0f;

    glScissor(getX() + startPoint, getY(),
              endPoint - startPoint, getHeight());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // update mEndLineIdx
    mDrawIndex = mEndLineIdx + 1;

    if (mDrawIndex > mInputWidth - 1) {
        mDrawIndex -= mInputWidth;

        mDrawingWrapped = true;
    }

    glBindBuffer(GL_ARRAY_BUFFER, mVB[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVB[1]);
    glDrawElements(GL_TRIANGLE_STRIP, 10, GL_UNSIGNED_SHORT, 0);
    checkGlError("glDrawArrays");

    if (mDrawPeakLines)
        renderPeakMeanLines(PEAK_BUFFER);
    if (mDrawMeanLines)
        renderPeakMeanLines(MEAN_BUFFER);

    // adjust scissor for tracline
    endPoint += SM_TRACELINE_THICKNESS;
    glScissor(getX() + startPoint, getY(),
              endPoint - startPoint, getHeight());

    // drawLine
    renderTraceLine(lineX, -1.0f, 1.0f);

    // update num frames to draw
    mNumFramesDraw -= 1.0f;

    // ignore the negative values but needs to keep track of those.
    retNumDraw = (uint32_t) floor(mNumFramesDraw);

    mLastDrawnWholeScreen = false;

done:
    return(retNumDraw);
}

//-----------------------------------------------------------------------------
void ScrollModeRenderer::drawFreezeMode()
{
    int startPoint;
    int endPoint;
    float lineX;

    // draw frames in texture
    glUseProgram(mProgram);
    checkGlError("glUseProgram-FreezeMode");

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mTextureHandle);

    glUniform1i(mTextureUniformHandle, 3);

    // tint adjustment
    glUniform3fv(mTintHandle, 1, mTintAdj);
    checkGlError("glUniform3fv");

    glEnable(GL_SCISSOR_TEST);

    startPoint = (int) floor((0.0f)/((float) mInputWidth) * getWidth()) - SM_TRACELINE_THICKNESS - 2;
    endPoint = (int) floor(((float) mMaxDrawIndex)/((float) mInputWidth) * getWidth());

    lineX = 2.0f * ((float) (mDrawIndex - 1))/((float) mInputWidth) - 1.0f;

    // clear the whole screen
    glScissor(getX(), getY(), getWidth(), getHeight());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glScissor(getX() + startPoint, getY(),
              endPoint - startPoint, getHeight());

    glBindBuffer(GL_ARRAY_BUFFER, mVB[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVB[1]);
    glDrawElements(GL_TRIANGLE_STRIP, 10, GL_UNSIGNED_SHORT, 0);
    checkGlError("glDrawArrays-FreezeMode");

    if (mDrawPeakLines)
        renderPeakMeanLines(PEAK_BUFFER);
    if (mDrawMeanLines)
        renderPeakMeanLines(MEAN_BUFFER);

    // adjust scissor for traceline
    endPoint += SM_TRACELINE_THICKNESS;
    glScissor(getX() + startPoint, getY(),
              endPoint - startPoint, getHeight());

    // draw traceLine
    renderTraceLine(lineX, -1.0f, 1.0f);

    mLastDrawnWholeScreen = true;

    return;
}

//-----------------------------------------------------------------------------
void ScrollModeRenderer::renderPeakMeanLines(int type)
{
    float* lineColor;
    if (type == MEAN_BUFFER)
        lineColor = mMeanColor;
    else
        lineColor = mPeakColor;

    glUseProgram(mLineProgram);
    checkGlError("glUseLineProgram");

    glUniform3fv(mColorHandle, 1, lineColor);
    glLineWidth(mPeakMeanLineThickness);

    glBindBuffer(GL_ARRAY_BUFFER, mVB[type]);
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_LINE_STRIP, 0, mInputWidth);
    checkGlError("glDrawArrays - renderPeakMeanLines");
}

//-----------------------------------------------------------------------------
void ScrollModeRenderer::renderTraceLine(float x1, float y1, float y2)
{
    float lineData[4];
    lineData[0] = x1;
    lineData[1] = y1;
    lineData[2] = x1;
    lineData[3] = y2;

    glUseProgram(mLineProgram);
    checkGlError("glUseLineProgram");

    glUniform3fv(mColorHandle, 1, mLineColor);
    glLineWidth(SM_TRACELINE_THICKNESS);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, lineData);
    glDrawArrays(GL_LINES, 0, 2);
    checkGlError("glDrawArrays - renderMTraceLine");
}

//-----------------------------------------------------------------------------
bool ScrollModeRenderer::modeSpecificMethod()
{
    return skipFrameDraw();
}

//-----------------------------------------------------------------------------
bool ScrollModeRenderer::skipFrameDraw()
{
    bool retVal = false;

    if (mNumFramesDraw >= 2.0f)
    {
        mNumFramesDraw -= 1.0f;
        retVal = true;
    }

    return retVal;
}

//-----------------------------------------------------------------------------
void ScrollModeRenderer:: setParams(int inputWidth, int numSamples, float scrollSpeed, float baselineShift,
                                int targetFrameRate, float numLinesPerFrame, float frameRate,
                                int orgLinesPerFrame, float desiredTimeSpan, float yPhysicalScale, float yExpScale)
{
    // inputWidth = samplesPerScreen (before scaling for display)
    if (inputWidth > 0)                 // if non-positive => input width = display width
        mInputWidth = inputWidth;
    else
        mInputWidth = getWidth();

    // Y Physical to Pixel Scale (Imaging Depth for M-mode)
    mYPhysicalScale = yPhysicalScale;

    // Y Expansion Scale - PW/M: 1.0f, CW: 2.0f
    mYExpScale = yExpScale;

    // Number of samples per line: mFFTSize for PW/CW
    mNumSamples = numSamples;
    mScrollSpeed = scrollSpeed;             // input lines per second

    // Baseline shift parameter should be -0.5 ~ 0.5
    mBaselineShiftFloat = baselineShift;

    float drawInterval = 1.0f/((float) targetFrameRate);
    float feedInterval = 1.0f/frameRate;
    float feedRate = mScrollSpeed * feedInterval;

    mScrollRate = (mScrollSpeed) * drawInterval;    // lines per draw
    mNumLinesPerFrame = numLinesPerFrame;
    mNumFramesDrawPerFeed = mNumLinesPerFrame / mScrollRate;

    // mOrgNumLinesPerFrame -> input SamplesPerFrame for PW/CW
    // numLinesPerFrame -> output (drawing)
    mOrgNumLinesPerFrame = orgLinesPerFrame;
    mReSampleRatio = mOrgNumLinesPerFrame/mNumLinesPerFrame;

    // jitter amount
    mDesiredTimeSpan = desiredTimeSpan;
    mJitterAmt = desiredTimeSpan * mScrollSpeed - ((float) mInputWidth);
    if ((mJitterAmt < 0.0f) || (mJitterAmt >= 1.0f))
        mJitterAmt = 0.0f;

    ALOGD("InputWidth: %d", mInputWidth);
    ALOGD("Y-PhysicalScale: %f", mYPhysicalScale);
    ALOGD("NumSamples: %d", mNumSamples);
    ALOGD("ScrollSpeed: %f", mScrollSpeed);
    ALOGD("Scroll Rate (lines / draw) : %f", mScrollRate);
    ALOGD("Feed Rate (lines / frame): %f",feedRate);
    ALOGD("numLine per frame from dB: %f", mNumLinesPerFrame);
    ALOGD("Frames Draw / frame feed: %f", mNumFramesDrawPerFeed);
    ALOGD("Original NumLines Per Frame: %d", mOrgNumLinesPerFrame);
    ALOGD("Resample Ratio: %f", mReSampleRatio);
    ALOGD("BaselineShift: %f", baselineShift);
    ALOGD("Pixel Jitter: %f", mJitterAmt);
}

//-----------------------------------------------------------------------------
void ScrollModeRenderer::getPhysicalToPixelMap(float *mapMat)
{
    mScHelper.getPhysicalToPixelMap(getWidth(), getHeight(), mapMat);
}

//-----------------------------------------------------------------------------
void ScrollModeRenderer::setPan(float deltaX, float deltaY)
{
    mScHelper.setPan(deltaX, deltaY);
}

//-----------------------------------------------------------------------------
void ScrollModeRenderer::setTintAdjustment(float coeffR, float coeffG, float coeffB)
{
    mTintAdj[0] = coeffR;
    mTintAdj[1] = coeffG;
    mTintAdj[2] = coeffB;
}

//-----------------------------------------------------------------------------
void ScrollModeRenderer::setPeakMeanDrawing(bool drawPeakLines, bool drawMeanLines)
{
    mDrawPeakLines = drawPeakLines;
    mDrawMeanLines = drawMeanLines;

    if ((mDrawPeakLines || mDrawMeanLines) && (!mCalcPeakLines))
        mCalcPeakLines = true;
}

//-----------------------------------------------------------------------------
void ScrollModeRenderer::setPeakMeanLineThickness(int lineThickness)
{
    mPeakMeanLineThickness = lineThickness;
}

//-----------------------------------------------------------------------------
void ScrollModeRenderer::getRecordParams(ScrollModeRecordParams &recParams)
{
    // traceIndex usually used for capturing still images
    recParams.traceIndex = mSingleFrameDrawIndex;
    recParams.frameWidth = mDataWidth;
    recParams.scrDataWidth = mInputWidth;
    recParams.samplesPerLine = mNumSamples;
    recParams.orgNumLinesPerFrame = mOrgNumLinesPerFrame;
    recParams.numLinesPerFrame = mNumLinesPerFrame;
    recParams.baselineShift = mBaselineShiftFloat;
    recParams.scrollSpeed = mScrollSpeed;
    recParams.desiredTimeSpan = mDesiredTimeSpan;
    recParams.yPhysicalScale = mYPhysicalScale;
    recParams.isInvert = mInvert;
    recParams.isScrFrame = mIsSingleFrameTextureMode;
}

//-----------------------------------------------------------------------------
void ScrollModeRenderer::setFrameNum(int frameNum)
{
    if ((((frameNum - mPrevFrameNum) == 1) || ((frameNum - mPrevFrameNum) == 2)) && !mIsSingleFrameTextureMode)
    {
        // sequential playback cases - partial update
        mWholeScreenRedraw = false;

        for (int i = mPrevFrameNum + 1; i < frameNum + 1; i++)
        {
            setFrameN(i);
        }
    }
    else
    {
        mWholeScreenRedraw = true;
    }

    mPrevFrameNum = frameNum;

    return;
}

//-----------------------------------------------------------------------------
int ScrollModeRenderer::getNumLinesInFrameFromHeader(uint8_t *headerPtr)
{
    CineBuffer::CineFrameHeader *cineHeaderPtr = reinterpret_cast<CineBuffer::CineFrameHeader *>(headerPtr);
    int numLinesInFrame = (int) cineHeaderPtr->numSamples;

    // for those modes that do not keep numSamples in the frame header
    if ((mDataType == DAU_DATA_TYPE_M) || (numLinesInFrame == 0))
        numLinesInFrame = mNumLinesPerFrame;

    if (abs(numLinesInFrame - mNumLinesPerFrame) > 1.0f) {
        ALOGW("Expected numLinesPerFrame: %f, numLines in current frame: %d", mNumLinesPerFrame,
              numLinesInFrame);

        // change numLinesInFrame to (int) mNumLinesPerFrame
        numLinesInFrame = (int) mNumLinesPerFrame;
    }

    return numLinesInFrame;
}

//-----------------------------------------------------------------------------
void ScrollModeRenderer::setFrameN(int frameNum)
{
    // framePtr including Header
    uint8_t *frameHPtr = mCineBufferPtr->getFrame(frameNum, mDataType,
                                                 CineBuffer::FrameContents::IncludeHeader);
    int numLinesInFrame = getNumLinesInFrameFromHeader(frameHPtr);

    // framePtr - DataOnly
    uint8_t* framePtr = mCineBufferPtr->getFrame(frameNum, mDataType, CineBuffer::FrameContents::DataOnly);

    setFrame(framePtr, numLinesInFrame);

    if (mCalcPeakLines)
    {
        framePtr = mCineBufferPtr->getFrame(frameNum, DAU_DATA_TYPE_DOP_PM, CineBuffer::FrameContents::DataOnly);
        setPeakMeanFrame(framePtr, numLinesInFrame);
    }
}

//-----------------------------------------------------------------------------
void ScrollModeRenderer::setFrame(uint8_t *framePtr, int numLinesInFrame)
{
    int startIndex = mDataIndex;
    int endIndex = mDataIndex + numLinesInFrame - 1;
    bool wrapped = false;
    bool adjustJitter = false;

    if (endIndex > mInputWidth - 1) {
        // wrapped
        endIndex -= mInputWidth;
        wrapped = true;

        // Jitter
        mJitterSum += mJitterAmt;

        // Jitter adjustment
        if (mJitterSum >= 1.0f)
        {
            mJitterSum -= 1.0f;
            adjustJitter = true;
            numLinesInFrame -= 1;

            if (endIndex == 0)
                endIndex = mInputWidth - 1;
            else
                endIndex -= 1;
        }
    }

    // store key indices for Peak/Mean drawing
    mPeakStartIndex = startIndex;
    mPeakEndIndex = endIndex;
    mPeakWrapped = wrapped;
    mPeakJitterAdjusted = adjustJitter;

    // load data to texture memory
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mTextureHandle);

    if (!wrapped) {
        // straight case
        glTexSubImage2D(GL_TEXTURE_2D,
                        0,
                        0,
                        startIndex,
                        mNumSamples,
                        numLinesInFrame,
                        GL_LUMINANCE,
                        GL_UNSIGNED_BYTE,
                        framePtr);
    }
    else
    {
        ALOGD("feedData wrapped!!!!!!!");
        // wrapped case
        int num_trans = mInputWidth - startIndex;
        glTexSubImage2D(GL_TEXTURE_2D,
                        0,
                        0,
                        startIndex,
                        mNumSamples,
                        num_trans,
                        GL_LUMINANCE,
                        GL_UNSIGNED_BYTE,
                        framePtr);

        // if jitter adjust, then skip 1 sample
        if (adjustJitter)
            num_trans += 1;

        // check whether there are data to transfer
        if ((numLinesInFrame - num_trans) > 0) {
            glTexSubImage2D(GL_TEXTURE_2D,
                            0,
                            0,
                            0,
                            mNumSamples,
                            endIndex + 1,
                            GL_LUMINANCE,
                            GL_UNSIGNED_BYTE,
                            framePtr + (num_trans * mNumSamples));
        }
    }

    checkGlError("ScrollModeRendering-feedData-setFrame");

    // update indices
    mDataIndex = endIndex + 1;

    if (mDataIndex > mInputWidth - 1) {
        //wrapped
        mDataIndex -= mInputWidth;

        // if jitterSum has not been updated.
        if (!wrapped)
        {
            mJitterSum += mJitterAmt;
        }

    }

    // update num of frames need to be drawn
    mNumFramesDraw += mNumFramesDrawPerFeed;
}

//-----------------------------------------------------------------------------
void ScrollModeRenderer::setPeakMeanFrame(uint8_t *framePtr, int numLinesInFrame)
{
    int numTrans1, numTrans2;
    float* framePtrFloat = (float*) framePtr;
    float  signAdj = 1.0f;

    if (mInvert)
        signAdj = -1.0f;


    if (!mPeakWrapped)
    {
        numTrans1 = numLinesInFrame;
        numTrans2 = 0;
    }
    else
    {
        // if wrapped
        numTrans1 = mInputWidth - mPeakStartIndex;
        numTrans2 = mPeakEndIndex + 1;
    }

    for (int i = mPeakStartIndex; i < (mPeakStartIndex + numTrans1); i++)
    {
        mPeakPointPtr[i].y = (signAdj*(*framePtrFloat++) * mYExpScale) + mYShiftFloat;
        mMeanPointPtr[i].y = (signAdj*(*framePtrFloat++) * mYExpScale) + mYShiftFloat;
    }

    if (mPeakWrapped)
    {
        if (mPeakJitterAdjusted)
            framePtrFloat++;

        for (int i = 0; i < numTrans2; i++)
        {
            mPeakPointPtr[i].y = (signAdj*(*framePtrFloat++) * mYExpScale) + mYShiftFloat;
            mMeanPointPtr[i].y = (signAdj*(*framePtrFloat++) * mYExpScale) + mYShiftFloat;
        }
    }

    // update Array Buffer
    // Peak Buffer
    glBindBuffer(GL_ARRAY_BUFFER, mVB[PEAK_BUFFER]);
    glBufferSubData(GL_ARRAY_BUFFER,
                    mPeakStartIndex * sizeof(point),
                    sizeof(point) * numTrans1,
                    &mPeakPointPtr[mPeakStartIndex]);

    if (mPeakWrapped)
    {
        glBufferSubData(GL_ARRAY_BUFFER,
                        0,
                        sizeof(point) * numTrans2,
                        &mPeakPointPtr[0]);
    }

    // Mean Buffer
    glBindBuffer(GL_ARRAY_BUFFER, mVB[MEAN_BUFFER]);
    glBufferSubData(GL_ARRAY_BUFFER,
                    mPeakStartIndex * sizeof(point),
                    sizeof(point) * numTrans1,
                    &mMeanPointPtr[mPeakStartIndex]);

    if (mPeakWrapped)
    {
        glBufferSubData(GL_ARRAY_BUFFER,
                        0,
                        sizeof(point) * numTrans2,
                        &mMeanPointPtr[0]);
    }

    checkGlError("setPeakMeanFramData");
}

//-----------------------------------------------------------------------------
ThorStatus ScrollModeRenderer::preparePeakMeanFreezeMode(int startFrameNum, int endFrameNum, int inStartDataIdx,
        bool jitterAdjust)
{
    ThorStatus retVal = THOR_ERROR;
    CineBuffer::CineFrameHeader* cineHeaderPtr;

    int         curFrameNum, curNumLines;
    int         curDataIndex, numTrans1, numTrans2;
    uint8_t*    framePtr;
    float*      framePtrFloat;
    float*      outPtrFloat;
    bool        wrapped = false;

    curFrameNum = startFrameNum + mMinFrameNum;
    curDataIndex = inStartDataIdx;

    outPtrFloat = (float*) mCineBufferPtr->getFrame(0, DAU_DATA_TYPE_DPMAX_SCR);

    // clear texture
    memset(outPtrFloat, 0, mInputWidth * sizeof(float) * 2);

    // concatenate frames to build a texture
    while(curFrameNum <= (endFrameNum + mMinFrameNum))
    {
        // read from header
        framePtr = mCineBufferPtr->getFrame(curFrameNum, DAU_DATA_TYPE_DOP_PM,
                                            CineBuffer::FrameContents::IncludeHeader);
        cineHeaderPtr = reinterpret_cast<CineBuffer::CineFrameHeader*>(framePtr);

        // framePtr for data location
        framePtr += sizeof(CineBuffer::CineFrameHeader);
        framePtrFloat = (float*) framePtr;
        // current number of lines in the frameheader
        curNumLines = cineHeaderPtr->numSamples;

        // if different too much.
        if (abs(((float) curNumLines) - mNumLinesPerFrame) > 1.0f)
            curNumLines = (int) mNumLinesPerFrame;

        // update PeakMean buffer
        for (int i = 0; i < curNumLines; i++)
        {
            outPtrFloat[curDataIndex*2] = *framePtrFloat++;
            outPtrFloat[curDataIndex*2 + 1] = *framePtrFloat++;

            curDataIndex += 1;

            if (curDataIndex > mInputWidth - 1)
            {
                if (jitterAdjust)
                {
                    i++;
                    framePtrFloat++;
                    framePtrFloat++;
                }

                curDataIndex -= mInputWidth;
                wrapped = true;
            }
        }

        // update frameNum
        curFrameNum++;
    }

    // fill in zeros if not fill in the whole screen
    if (!wrapped)
    {
        for (int ix = curDataIndex; ix < mInputWidth; ix++)
        {
            outPtrFloat[ix*2] = 0.0f;
            outPtrFloat[ix*2 + 1] = 0.0f;
        }
    }

    CineBuffer::CineFrameHeader* dpmOutHeader =
            ( CineBuffer::CineFrameHeader*) mCineBufferPtr->getFrame(0, DAU_DATA_TYPE_DPMAX_SCR, CineBuffer::FrameContents::IncludeHeader);

    dpmOutHeader->frameNum =0;

    retVal = THOR_OK;

ok_ret:
err_ret:
    return retVal;

}

//-----------------------------------------------------------------------------
void ScrollModeRenderer::setForcedFullScreenRender(bool fsr)
{
    mForcedFSR = fsr;

    // set texture buffer to zeros
    if (mForcedFSR && mTextureHandle)
    {
        ALOGD("Setting Forced Full Screen Render - clearing up the texture memory");
        // set texture images to zero
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, mTextureHandle);

        // zeros ptr
        uint8_t* zerosPtr = new uint8_t[mNumSamples * mInputWidth];
        memset(zerosPtr, 0, mNumSamples * mInputWidth * sizeof(uint8_t));

        glTexSubImage2D(GL_TEXTURE_2D,
                        0,
                        0,
                        0,
                        mNumSamples,
                        mInputWidth,
                        GL_LUMINANCE,
                        GL_UNSIGNED_BYTE,
                        zerosPtr);

        checkGlError("ScrollModeRendering-setForcedFSR - clearing Texture");

        // cleanup the memory
        delete[] zerosPtr;
    }
}