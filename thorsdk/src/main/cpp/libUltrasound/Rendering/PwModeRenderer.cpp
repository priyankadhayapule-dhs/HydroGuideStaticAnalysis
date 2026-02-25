//
// Copyright 2020 EchoNous Inc.
//

#define LOG_TAG "PwModeRenderer"
#define PW_TRACELINE_THICKNESS 2
#define MAX_TEXTURE_WIDTH 7000

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <GLES3/gl3.h>
#include <ThorUtils.h>
#include <PwModeRenderer.h>
#include "../include/Rendering/PwModeRenderer.h"

const char* const PwModeRenderer::mVertexShaderSource =
#include "mmode_vertex.glsl"
;

const char* const PwModeRenderer::mFragmentShaderSource =
#include "mmode_fragment.glsl"
;

const char *const PwModeRenderer::mLineVertexShaderSource =
#include "og_vertex.glsl"
;

const char *const PwModeRenderer::mLineFragmentShaderSource =
#include "og_fragment.glsl"
;

//-----------------------------------------------------------------------------
PwModeRenderer::PwModeRenderer() :
    Renderer(),
    mProgram(0),
    mTextureHandle(0),
    mTextureUniformHandle(-1),
    mInputWidth(1000),
    mJitterAmt(0.0),
    mJitterSum(0.0),
    mDrawingJitterSum(0.0),
    mNumLines(0.0),
    mDataIndex(0),
    mDrawIndex(0),
    mFFTSize(256),
    mScrollSpeed(1000.0),
    mScrollRate(0.0),
    mNumFramesDraw(0.0),
    mNumFramesDrawPerFeed(1.0),
    mDrawingWrapped(false),
    mIsSingleFrameTextureMode(false),
    mMinFrameNum(0),
    mMaxFrameNum(0),
    mPrevFrameNum(-1),
    mBaselineShiftFloat(0.0f),
    mInvert(false),
    mCineBufferPtr(nullptr)
{
    memset(mVB, 0, sizeof(mVB));
}

//-----------------------------------------------------------------------------
PwModeRenderer::~PwModeRenderer()
{
    close();
}

//-----------------------------------------------------------------------------
ThorStatus PwModeRenderer::open()
{
    ThorStatus      retVal = THOR_ERROR;

    float depthScale;
    float scrollTime;
    float timeScale;
    float imagingDepth;

    // buffer location and binding
    float locData[32];          // 4*4*2
    short idxData[10];          // (4+1)*2

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
    if (!checkGlError("glGetUniformLocation")) goto err_ret;

    // texture init
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mTextureHandle);
    if (!checkGlError("BindPwModeTexture")) goto err_ret;

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_LUMINANCE,
                 mFFTSize,
                 mInputWidth,
                 0,
                 GL_LUMINANCE,
                 GL_UNSIGNED_BYTE,
                 0);

    if (!checkGlError("glTexImage2D-MMode-Initialize")) goto err_ret;

    glGenBuffers(2, mVB);

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
        mScHelper.setScaleXYShift(timeScale, -2.0f/mYPhysicalScale, -1.0f, mBaselineShiftFloat*2.0f);
        // set aspect ratio (this matrix is a special case)
        mScHelper.setAspect(1.0f);

        // if invert, flipY
        if (mInvert)
            mScHelper.setFlip(1.0f, -1.0f);
    }

    // M trace line drawing related setups
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

    mLineColor[0] = 1.0f;
    mLineColor[1] = 0.427f;
    mLineColor[2] = 0.0f;

    // clear the PW-section screen
    glEnable(GL_SCISSOR_TEST);
    glScissor(getX(), getY(), getWidth(), getHeight());
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mDrawingWrapped = false;
    mWholeScreenRedraw = false;

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
 *  int   y_shift_from_end = mFFTSize/2 - mBaselineShiftPixels;       // y_shift from top
 *  float u_x_shift = ((float) mFFTSize - 1 - y_shift_from_end)/mFFTSize;
 *  float y_shift = y_end - ((float) y_shift_from_end)/((float)(mFFTSize - 1)) * (y_end - y_start);
 *
 */
void PwModeRenderer::calculateBaselineShiftCoord(float* locData, short* idxData)
{
    // calculate array and element buffer for texture mapping with baseline shift
    float x_start = -1.0;
    float x_end   = 1.0;

    float y_start = -1.0;
    float y_end   = 1.0;

    float u_x = 0.5f / mFFTSize;
    float u_y = 0.5f / mInputWidth;

    // shifted coordinate value
    float u_x_shift = 0.5f - mBaselineShiftFloat * (1.0f - u_x * 2.0f) ;
    float y_shift = y_end - (0.5f + mBaselineShiftFloat) * 2.0f;

    // invert the y-axis: FFT output order is downward, so should be inverted by default.
    if (!mInvert)
    {
        y_start = -y_start;
        y_end = -y_end;
        y_shift = -y_shift;

        //u_x_shift = 1.0f - u_x_shift;
    }

    ALOGD("BaselineShift - u_x_shift: %f, y_shift: %f", u_x_shift, y_shift);

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
void PwModeRenderer::updateBaselineShift(float baselineShift)
{
    mBaselineShiftFloat = baselineShift;

    // update locData - x direction
    glBindBuffer(GL_ARRAY_BUFFER, mVB[0]);
    float *locPtr = (float *)glMapBufferRange(GL_ARRAY_BUFFER, 0, 32, GL_MAP_WRITE_BIT);

    // dummy idxData
    short idxData[10];
    calculateBaselineShiftCoord(locPtr, idxData);

    glUnmapBuffer(GL_ARRAY_BUFFER);
}

//-----------------------------------------------------------------------------
void PwModeRenderer::close()
{
    // unbind texture
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (0 != mTextureHandle)
    {
        glDeleteTextures(1, &mTextureHandle);
        mTextureHandle = 0;
    }
    if (0 != mVB[0])
    {
        glDeleteBuffers(1, &mVB[0]);
        mVB[0] = 0;
    }
    if (0 != mVB[1])
    {
        glDeleteBuffers(1, &mVB[1]);
        mVB[1] = 0;
    }
}

//-----------------------------------------------------------------------------
ThorStatus PwModeRenderer::prepareRenderer(CineBuffer *cineBuffer)
{
    ThorStatus retVal = THOR_ERROR;
    float u_y;

    mCineBufferPtr = cineBuffer;
    mPrevFrameNum = -1;
    mWholeScreenRedraw = false;

    // init variables
    mNumFramesDraw = 0.0f;
    mDataIndex = 0;
    mDrawIndex = 0;

    // get min/max frame numbers
    mMinFrameNum = mCineBufferPtr->getMinFrameNum();
    mMaxFrameNum = mCineBufferPtr->getMaxFrameNum();

    // texture init
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mTextureHandle);
    if (!checkGlError("BindPwModeTexture"))
        goto err_ret;

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_LUMINANCE,
                 mFFTSize,
                 mInputWidth,
                 0,
                 GL_LUMINANCE,
                 GL_UNSIGNED_BYTE,
                 0);

    if (!checkGlError("glTexImage2D-PwMode-Initialize"))
        goto err_ret;

    retVal = THOR_OK;

err_ret:
    return (retVal);
}

//-----------------------------------------------------------------------------
void PwModeRenderer::setSingleFrameTexturePtr(uint8_t* texturePtr, int textureWidth)
{
    if (textureWidth < 1)
        textureWidth = mInputWidth;

    // prepare Texture unit for singleFrame Texture mode
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mTextureHandle);
    checkGlError("BindPwModeTexture-SingleFrameTexture");

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_LUMINANCE,
                 mFFTSize,
                 textureWidth,
                 0,
                 GL_LUMINANCE,
                 GL_UNSIGNED_BYTE,
                 texturePtr);

    checkGlError("glTexImage2D-PwMode-SingleFrameTexture");
}

//-----------------------------------------------------------------------------
ThorStatus PwModeRenderer::prepareFreezeMode(int inFrameNum)
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

    // check frame Nnumber range
    if (inFrameNum < 0)
        inFrameNum = mPrevFrameNum;

    // for singleFrameTextureMode
    if (mIsSingleFrameTextureMode)
    {
        // single frame texture mode.
        texPtr = mCineBufferPtr->getFrame(TEX_FRAME_TYPE_PW, DAU_DATA_TYPE_TEX,
                CineBuffer::FrameContents::IncludeHeader);

        cineHeaderPtr = reinterpret_cast<CineBuffer::CineFrameHeader*>(texPtr);
        mMaxDrawIndex = cineHeaderPtr->numSamples - 1;
        mDrawIndex = mSingleFrameDrawIndex;

        retVal = THOR_OK;
        goto err_ret;
    }

    // texRefPtr;
    texRefPtr = mCineBufferPtr->getFrame(TEX_FRAME_TYPE_PW, DAU_DATA_TYPE_TEX);

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
    frameWidth = (int) ceil(inputWidthWithJitter/mNumLines);
    startFrameNum = endFrameNum - frameWidth + 1;

    if (startFrameNum < realMinFrameNum)
        startFrameNum = realMinFrameNum;

    totalInSampleIndex = startFrameNum * mPwSamplesPerFrame - 1;
    estimatedNumLines = floor(totalInSampleIndex / mPwSampleStep) + 1;
    if (round((floor(totalInSampleIndex / mPwSampleStep) + 1) * mPwSampleStep) <= totalInSampleIndex)
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

    // check PW DATA TYPE is available.
    framePtr = mCineBufferPtr->getFrame(curFrameNum, DAU_DATA_TYPE_PW,
                                        CineBuffer::FrameContents::IncludeHeader);
    if (framePtr == nullptr)
        goto err_ret;

    // concatenate frames to build a texture
    while(curFrameNum <= (endFrameNum + mMinFrameNum))
    {
        // read from header
        framePtr = mCineBufferPtr->getFrame(curFrameNum, DAU_DATA_TYPE_PW,
                                            CineBuffer::FrameContents::IncludeHeader);
        cineHeaderPtr = reinterpret_cast<CineBuffer::CineFrameHeader*>(framePtr);

        // framePtr for data location
        framePtr += sizeof(CineBuffer::CineFrameHeader);

        curNumLines = cineHeaderPtr->numSamples;
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
            memcpy(texRefPtr + mFFTSize*startDataIndex, framePtr, mFFTSize*curNumLines);
        }
        else
        {
            numLinesTransfer1 = mInputWidth - startDataIndex;
            // Transfer
            memcpy(texRefPtr + mFFTSize*startDataIndex, framePtr, mFFTSize*numLinesTransfer1);

            // wrapped - Jitter adjustment
            mJitterSum += mJitterAmt;

            if (mJitterSum >= 1.0f) {
                // Jitter Adjusted
                numLinesTransfer1 += 1;
            }

            numLinesTransfer2 = curNumLines - numLinesTransfer1;
            if (numLinesTransfer2 > 0)
            {
                //Transfer2 - wrapped so line no. 0
                memcpy(texRefPtr, framePtr + mFFTSize * numLinesTransfer1, mFFTSize*numLinesTransfer2);
            }
        }

        // update startDataIndex
        startDataIndex += curNumLines;

        if (startDataIndex > mInputWidth - 1)
        {
            startDataIndex -= mInputWidth;
        }
    }

    mDrawingJitterSum = mJitterSum;
    endDataIndex = startDataIndex;

    mDataIndex = endDataIndex;
    mDrawIndex = endDataIndex;

    // Min Max Draw Index
    if (mDataWidth < mInputWidth)
        mMaxDrawIndex = endDataIndex - 1;
    else
        mMaxDrawIndex = mInputWidth - 1;

    // update Tex Buffer header
    texPtr = mCineBufferPtr->getFrame(TEX_FRAME_TYPE_PW, DAU_DATA_TYPE_TEX,
                                      CineBuffer::FrameContents::IncludeHeader);
    cineHeaderPtr = reinterpret_cast<CineBuffer::CineFrameHeader*>(texPtr);
    cineHeaderPtr->numSamples = mDataWidth;

    retVal = THOR_OK;

err_ret:
    return (retVal);
}

//-----------------------------------------------------------------------------
bool PwModeRenderer::prepare()
{
    // place holder for checking validity of indices (dataindex and drawindex)
    if ((mNumFramesDraw < 1.0f) && !mWholeScreenRedraw)
    {
        return(false);
    }
    else
        return(true);
}

//-----------------------------------------------------------------------------
bool PwModeRenderer::updateIndices()
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
        ALOGD("Not Enough Data for Drawing Any PW Lines");
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
uint32_t PwModeRenderer::draw()
{
    uint32_t    retNumDraw = 0;
    int         startPoint;
    int         endPoint;
    int         wrapStartPoint;
    float       lineX;

    if (mWholeScreenRedraw)
    {
        // TODO: Offscreen Rendering
        prepareFreezeMode(-1);
        mNumFramesDraw = 0.0f;

        setSingleFrameTexturePtr(mCineBufferPtr->getFrame(TEX_FRAME_TYPE_PW, DAU_DATA_TYPE_TEX), mInputWidth);

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

    glEnable(GL_SCISSOR_TEST);
    // additional drawing when the drawing is wrapped around.
    if (mDrawingWrapped)
    {
        if (mDrawIndex > 0)
        {
            wrapStartPoint = (int) floor(((float) mDrawIndex)/((float) mInputWidth) * getWidth()) - 2;
        }
        else
        {
            wrapStartPoint = getWidth();
        }

        // when the drawing is wrapped.
        glScissor(getX() + wrapStartPoint - PW_TRACELINE_THICKNESS, getY(),
                  getWidth() - wrapStartPoint + 2 * PW_TRACELINE_THICKNESS + 1, getHeight());
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindBuffer(GL_ARRAY_BUFFER, mVB[0]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVB[1]);
        glDrawElements(GL_TRIANGLE_STRIP, 10, GL_UNSIGNED_SHORT, 0);
        checkGlError("glDrawArrays");

        mDrawingWrapped = false;
    }

    startPoint = ((int) floor(((float) mStartLineIdx)/((float) mInputWidth) * getWidth())) - PW_TRACELINE_THICKNESS - 2;
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

    // adjust scissor for tracline
    endPoint += PW_TRACELINE_THICKNESS;
    glScissor(getX() + startPoint, getY(),
              endPoint - startPoint, getHeight());

    // drawLine
    renderTraceLine(lineX, -1.0f, 1.0f);

    // update num frames to draw
    mNumFramesDraw -= 1.0f;

    // ignore the negative values but needs to keep track of those.
    retNumDraw = (uint32_t) floor(mNumFramesDraw);

done:
    return(retNumDraw);
}

//-----------------------------------------------------------------------------
void PwModeRenderer::drawFreezeMode()
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

    glEnable(GL_SCISSOR_TEST);

    startPoint = (int) floor((0.0f)/((float) mInputWidth) * getWidth()) - PW_TRACELINE_THICKNESS - 2;
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

    // adjust scissor for traceline
    endPoint += PW_TRACELINE_THICKNESS;
    glScissor(getX() + startPoint, getY(),
              endPoint - startPoint, getHeight());

    // draw traceLine
    renderTraceLine(lineX, -1.0f, 1.0f);

    return;
}

//-----------------------------------------------------------------------------
void PwModeRenderer::renderTraceLine(float x1, float y1, float y2)
{
    float lineData[4];
    lineData[0] = x1;
    lineData[1] = y1;
    lineData[2] = x1;
    lineData[3] = y2;

    glUseProgram(mLineProgram);
    checkGlError("glUseLineProgram");

    glUniform3fv(mColorHandle, 1, mLineColor);
    glLineWidth(PW_TRACELINE_THICKNESS);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, lineData);
    glDrawArrays(GL_LINES, 0, 2);
    checkGlError("glDrawArrays - renderMTraceLine");
}

//-----------------------------------------------------------------------------
bool PwModeRenderer::modeSpecificMethod()
{
    return skipFrameDraw();
}

//-----------------------------------------------------------------------------
bool PwModeRenderer::skipFrameDraw()
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
void PwModeRenderer:: setParams(int inputWidth, int fftSize, float scrollSpeed, float baselineShift,
                                int targetFrameRate, float numLinesPerFrame, float frameRate,
                                int pwSamplesPerFrame, float desiredTimeSpan, float yPhysicalScale)
{
    // inputWidth = samplesPerScreen (before scaling for display)
    if (inputWidth > 0)                 // if non-positive => input width = display width
        mInputWidth = inputWidth;
    else
        mInputWidth = getWidth();

    // Y Physical to Pixel Scale (Imaging Depth for M, Velocity Scale (full: 2x) for PW/CW)
    mYPhysicalScale = yPhysicalScale;

    mFFTSize = fftSize;
    mScrollSpeed = scrollSpeed;             // input lines per second

    // Baseline shift parameter should be -0.5 ~ 0.5
    mBaselineShiftFloat = baselineShift;

    float drawInterval = 1.0f/((float) targetFrameRate);
    float feedInterval = 1.0f/frameRate;
    float feedRate = mScrollSpeed * feedInterval;

    mScrollRate = (mScrollSpeed) * drawInterval;    // lines per draw
    mNumLines = numLinesPerFrame;
    mNumFramesDrawPerFeed = mNumLines / mScrollRate;

    mPwSamplesPerFrame = pwSamplesPerFrame;
    mPwSampleStep = mPwSamplesPerFrame/mNumLines;

    // jitter amount
    mDesiredTimeSpan = desiredTimeSpan;
    mJitterAmt = desiredTimeSpan * mScrollSpeed - ((float) mInputWidth);
    if ((mJitterAmt < 0.0f) || (mJitterAmt >= 1.0f))
        mJitterAmt = 0.0f;

    ALOGD("Scroll Rate (lines / draw) : %f", mScrollRate);
    ALOGD("Feed Rate (lines / frame): %f",feedRate);
    ALOGD("numLine per frame from dB: %f", mNumLines);
    ALOGD("Frames Draw / frame feed: %f", mNumFramesDrawPerFeed);
    ALOGD("Pw Samples Per Frame: %d", mPwSamplesPerFrame);
    ALOGD("FFT Resample Ratio: %f", mPwSampleStep);
    ALOGD("BaselineShift: %f", baselineShift);
    ALOGD("Pixel Jitter: %f", mJitterAmt);
}

//-----------------------------------------------------------------------------
void PwModeRenderer::getPhysicalToPixelMap(float *mapMat)
{
    mScHelper.getPhysicalToPixelMap(getWidth(), getHeight(), mapMat);
}

//-----------------------------------------------------------------------------
void PwModeRenderer::setPan(float deltaX, float deltaY)
{
    mScHelper.setPan(deltaX, deltaY);
}

//-----------------------------------------------------------------------------
void PwModeRenderer::getPwRecordParams(ScrollModeRecordParams &recParams)
{
    recParams.traceIndex = mDrawIndex;
    recParams.frameWidth = mDataWidth;
    recParams.scrDataWidth = mInputWidth;
    recParams.samplesPerLine = mFFTSize;
    recParams.orgNumLinesPerFrame = mPwSamplesPerFrame;
    recParams.numLinesPerFrame = mNumLines;
    recParams.baselineShift = mBaselineShiftFloat;
    recParams.scrollSpeed = mScrollSpeed;
    recParams.desiredTimeSpan = mDesiredTimeSpan;
    recParams.yPhysicalScale = mYPhysicalScale;
    recParams.isInvert = mInvert;
}

//-----------------------------------------------------------------------------
void PwModeRenderer::setFrameNum(int frameNum)
{
    if ((((frameNum - mPrevFrameNum) == 1) || ((frameNum - mPrevFrameNum) == 2)) && !mIsSingleFrameTextureMode)
    {
        // sequential playback cases - partial update
        mWholeScreenRedraw = false;

        for (int i = mPrevFrameNum + 1; i < frameNum + 1; i++)
        {
            setFrame(i);
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
void PwModeRenderer::setFrame(int frameNum)
{
    // framePtr including Header
    uint8_t* framePtr = mCineBufferPtr->getFrame(frameNum, DAU_DATA_TYPE_PW, CineBuffer::FrameContents::IncludeHeader);
    CineBuffer::CineFrameHeader* cineHeaderPtr = reinterpret_cast<CineBuffer::CineFrameHeader*>(framePtr);
    int numLinesInFrame = (int) cineHeaderPtr->numSamples;

    // framePtr - DataOnly
    framePtr = mCineBufferPtr->getFrame(frameNum, DAU_DATA_TYPE_PW, CineBuffer::FrameContents::DataOnly);

    if (abs(numLinesInFrame - mNumLines) > 1.0f)
    {
        ALOGW("Expected numLinesPerFrame: %f, numLines in current frame: %d", mNumLines, numLinesInFrame);
    }

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

    // load data to texture memory
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mTextureHandle);

    if (!wrapped) {
        // straight case
        glTexSubImage2D(GL_TEXTURE_2D,
                        0,
                        0,
                        startIndex,
                        mFFTSize,
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
                        mFFTSize,
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
                            mFFTSize,
                            endIndex + 1,
                            GL_LUMINANCE,
                            GL_UNSIGNED_BYTE,
                            framePtr + (num_trans * mFFTSize));
        }
    }

    checkGlError("feedPwData-setFrame");

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
