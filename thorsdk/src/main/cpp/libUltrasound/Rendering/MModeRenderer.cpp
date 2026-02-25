//
// Copyright 2018 EchoNous Inc.
//

#define LOG_TAG "MModeRenderer"
#define M_TRACELINE_THICKNESS 2
#define MAX_TEXTURE_WIDTH 4800

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <GLES3/gl3.h>
#include <ThorUtils.h>
#include <MModeRenderer.h>

const char* const MModeRenderer::mVertexShaderSource =
#include "mmode_vertex.glsl"
;

const char* const MModeRenderer::mFragmentShaderSource =
#include "mmode_fragment.glsl"
;

const char *const MModeRenderer::mLineVertexShaderSource =
#include "og_vertex.glsl"
;

const char *const MModeRenderer::mLineFragmentShaderSource =
#include "og_fragment.glsl"
;

//-----------------------------------------------------------------------------
MModeRenderer::MModeRenderer() :
    Renderer(),
    mProgram(0),
    mTextureHandle(0),
    mTextureUniformHandle(-1),
    mColorHandle(-1),
    mTintHandle(-1),
    mNumLines(0),
    mDataIndex(0),
    mDrawIndex(0),
    mScrollSpeed(600),
    mScrollRate(0.0),
    mFeedRate(0.0),
    mNumFramesDraw(0.0),
    mNumFramesDrawPerBFrame(1.0),
    mDrawingWrapped(false),
    mIsLive(true),
    mIsSingleFrameTextureMode(false),
    mMinFrameNum(0),
    mMaxFrameNum(0),
    mPrevFrameNum(-1),
    mDataWidth(0),
    mMinDrawnIndex(0),
    mMaxDrawnIndex(0),
    mCurDrawnIndex(0),
    mNoScrollFreeze(0),
    mMinLoadedIndex(0),
    mMaxLoadedIndex(0),
    mTextureWidth(0),
    mStillMLineTime(0.0),
    mStillTimeShift(0.0),
    mCineBufferPtr(nullptr)
{
    // Tint adjustment
    mTintAdj[0] = 0.84f;
    mTintAdj[1] = 0.88f;
    mTintAdj[2] = 0.90f;

    memset(mVB, 0, sizeof(mVB));
}

//-----------------------------------------------------------------------------
MModeRenderer::~MModeRenderer()
{
    close();
}

//-----------------------------------------------------------------------------
ThorStatus MModeRenderer::open()
{
    ThorStatus      retVal = THOR_ERROR;

    float x_start = -1.0;
    float x_end   = 1.0;

    float y_start = -1.0;
    float y_end   = 1.0;

    float u_x = 0.5f / mScParams.numSamples;
    float u_y = 0.5f / getWidth();

    float p0_x = x_start;
    float p0_y = y_end;

    float p1_x = x_end;
    float p1_y = y_end;

    float p2_x = x_start;
    float p2_y = y_start;

    float p3_x = x_end;
    float p3_y = y_start;

    float depthScale;
    float scrollTime;
    float timeScale;
    float imagingDepth;

    // buffer location and binding
    float locData[16]; //4*4
    short idxData[4];

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

    mTextureUniformHandle = glGetUniformLocation(mProgram, "u_MTexture");
    mTintHandle = glGetUniformLocation(mProgram, "u_tintAdj");              // Tint Adj. Handle
    if (!checkGlError("glGetUniformLocation")) goto err_ret;

    // texture init
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mTextureHandle);
    if (!checkGlError("BindMModeTexture")) goto err_ret;

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_LUMINANCE,
                 mScParams.numSamples,
                 getWidth(),
                 0,
                 GL_LUMINANCE,
                 GL_UNSIGNED_BYTE,
                 0);

    if (!checkGlError("glTexImage2D-MMode-Initialize")) goto err_ret;

    glGenBuffers(2, mVB);

    locData[0] = p0_x;
    locData[1] = p0_y;
    locData[2] = u_x;
    locData[3] = u_y;

    locData[4] = p1_x;
    locData[5] = p1_y;
    locData[6] = u_x;
    locData[7] = 1.0f - u_y;

    locData[8]  = p2_x;
    locData[9]  = p2_y;
    locData[10] = 1.0f - u_x;
    locData[11] = u_y;

    locData[12] = p3_x;
    locData[13] = p3_y;
    locData[14] = 1.0f - u_x;
    locData[15] = 1.0f - u_y;

    glBindBuffer(GL_ARRAY_BUFFER, mVB[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 16, locData, GL_STATIC_DRAW);

    idxData[0] = 0;
    idxData[1] = 1;
    idxData[2] = 2;
    idxData[3] = 3;

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVB[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(short) * 4, idxData, GL_STATIC_DRAW);

    glVertexAttribPointer(3, 4, GL_FLOAT, false, sizeof(float) * 4, 0);
    glEnableVertexAttribArray(3);

    // init variables
    mNumFramesDraw = 0.0;
    mDataIndex = 0;
    mDrawIndex = 0;

    // setting up values for scHelper for the transfer matrix
    imagingDepth = (mScParams.sampleSpacingMm * (mScParams.numSamples - 1) +
                    mScParams.startSampleMm);
    depthScale = 2.0f / imagingDepth;
    scrollTime = getWidth()/mScrollSpeed;
    timeScale = 2.0f / scrollTime;

    mScParams.numRays = getWidth();
    mScParams.startRayRadian = 0.0f;
    mScParams.raySpacing = 1.0f / mScrollSpeed;

    // init ScHelper, set Scale and XY shift
    mScHelper.init(getWidth(), getHeight(), mScParams);
    mScHelper.setScaleXYShift(timeScale, depthScale, -1.0f, -1.0f);
    // set aspect ratio (this M-mode matrix is a special case)
    mScHelper.setAspect(1.0f);

    mIsLive = true;

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

    // clear the M- section screen
    glEnable(GL_SCISSOR_TEST);
    glScissor(getX(), getY(), getWidth(), getHeight());
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mDrawingWrapped = false;
    mIsSingleFrameTextureMode = false;

    retVal = THOR_OK;

err_ret:
    return(retVal);
}

//-----------------------------------------------------------------------------
void MModeRenderer::close()
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
ThorStatus MModeRenderer::prepareLiveMode(CineBuffer* cineBuffer)
{
    ThorStatus retVal = THOR_ERROR;
    float u_y;

    mCineBufferPtr = cineBuffer;
    mPrevFrameNum = -1;

    // if it's in live mode no change is needed.
    if (mIsLive)
    {
        retVal = THOR_OK;
        return retVal;
    }

    mIsLive = true;

    // init variables
    mNumFramesDraw = 0.0;
    mDataIndex = 0;
    mDrawIndex = 0;

    // update locData - x direction
    glBindBuffer(GL_ARRAY_BUFFER, mVB[0]);
    float *locPtr = (float *)glMapBufferRange(GL_ARRAY_BUFFER, 0, 16, GL_MAP_WRITE_BIT);

    u_y = 0.5f / getWidth();
    locPtr[3] = u_y;
    locPtr[7] = 1.0f - u_y;
    locPtr[11] = u_y;
    locPtr[15] = 1.0f - u_y;

    glUnmapBuffer(GL_ARRAY_BUFFER);

    // texture init
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mTextureHandle);
    if (!checkGlError("BindMModeTexture"))
        goto err_ret;

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_LUMINANCE,
                 mScParams.numSamples,
                 getWidth(),
                 0,
                 GL_LUMINANCE,
                 GL_UNSIGNED_BYTE,
                 0);

    if (!checkGlError("glTexImage2D-MMode-Initialize"))
        goto err_ret;

    retVal = THOR_OK;

err_ret:
    return (retVal);
}

//-----------------------------------------------------------------------------
ThorStatus MModeRenderer::prepareSingleFrameTextureMode()
{
    ThorStatus retVal = THOR_ERROR;
    uint8_t*   texPtr;

    mIsSingleFrameTextureMode = true;

    mTextureWidth = getWidth();
    texPtr = mCineBufferPtr->getFrame(TEX_FRAME_TYPE_M, DAU_DATA_TYPE_TEX);

    // prepare Texture unit for singleFrame Texture mode
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mTextureHandle);
    if (!checkGlError("BindMModeTexture"))
        goto err_ret;

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_LUMINANCE,
                 mScParams.numSamples,
                 mTextureWidth,
                 0,
                 GL_LUMINANCE,
                 GL_UNSIGNED_BYTE,
                 texPtr);

    if (!checkGlError("glTexImage2D-MMode-FreezeMode"))
        goto err_ret;

    // adjust time shift
    if (mStillTimeShift > 0.0f)
    {
        setPan(mStillTimeShift, 0.0f);
    }

    retVal = THOR_OK;

err_ret:
    return(retVal);
}

//-----------------------------------------------------------------------------
void MModeRenderer::setSingleFrameTexturePtr(uint8_t* texturePtr, int textureWidth) 
{
    // this function is designed mainly for the encoder.
    mIsLive = false;
    mNoScrollFreeze = true;
    mTextureWidth = textureWidth;

    // prepare Texture unit for singleFrame Texture mode
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mTextureHandle);
    checkGlError("BindMModeTexture");

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_LUMINANCE,
                 mScParams.numSamples,
                 mTextureWidth,
                 0,
                 GL_LUMINANCE,
                 GL_UNSIGNED_BYTE,
                 texturePtr);

    checkGlError("glTexImage2D-MMode-SingleFrameTexture");
}

//-----------------------------------------------------------------------------
ThorStatus MModeRenderer::prepareFreezeMode(CineBuffer* cineBuffer)
{
    ThorStatus retVal = THOR_ERROR;
    float u_y;

    mCineBufferPtr = cineBuffer;

    float lowerTexLimit = 0.0f;
    float upperTexLimit = 0.0f;

    // if it's in freeze mode no change is needed.
    if (!mIsLive)
    {
        retVal = THOR_OK;
        return retVal;
    }

    mIsLive = false;

    mMinFrameNum = mCineBufferPtr->getMinFrameNum();
    mMaxFrameNum = mCineBufferPtr->getMaxFrameNum();
    mDataWidth = (mMaxFrameNum - mMinFrameNum + 1) * mNumLines;

    if (mMaxFrameNum == mMinFrameNum)
    { 
        mNoScrollFreeze = true;
        retVal = prepareSingleFrameTextureMode();

        return retVal;
    }

    mMaxDrawnIndex = mDataWidth - 1;
    mCurDrawnIndex = mMaxDrawnIndex;

    if (mDataWidth <= getWidth())
    {
        // no scroll mode.
        mTextureWidth = getWidth();
        mNoScrollFreeze = true;

        mMinDrawnIndex = 0;
    }
    else
    {
        // scroll mode.
        if (mDataWidth > MAX_TEXTURE_WIDTH)
        {
            mTextureWidth = MAX_TEXTURE_WIDTH;
        }
        else
        {
            mTextureWidth = mDataWidth;
        }

        mNoScrollFreeze = false;
        mMinDrawnIndex = mMaxDrawnIndex - getWidth() + 1;
    }

    // update time scale shift
    setPan(2.0f * mMinDrawnIndex / getWidth(), 0.0f);

    // texture init
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mTextureHandle);
    if (!checkGlError("BindMModeTexture"))
        goto err_ret;

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_LUMINANCE,
                 mScParams.numSamples,
                 mTextureWidth,
                 0,
                 GL_LUMINANCE,
                 GL_UNSIGNED_BYTE,
                 0);

    if (!checkGlError("glTexImage2D-MMode-FreezeMode"))
        goto err_ret;

    // load texture
    loadFreezeModeTexture(false);

    if (!mNoScrollFreeze)
    {
        // update locData only for scroll mode
        glBindBuffer(GL_ARRAY_BUFFER, mVB[0]);
        float *locPtr = (float *)glMapBufferRange(GL_ARRAY_BUFFER, 0, 16, GL_MAP_WRITE_BIT);

        lowerTexLimit = (mMinDrawnIndex - mMinLoadedIndex + 0.5f) / mTextureWidth;
        upperTexLimit = (mMaxDrawnIndex - mMinLoadedIndex + 0.5f) / mTextureWidth;

        // update locData
        locPtr[3] = lowerTexLimit;
        locPtr[7] = upperTexLimit;
        locPtr[11] = lowerTexLimit;
        locPtr[15] = upperTexLimit;

        glUnmapBuffer(GL_ARRAY_BUFFER);
    }

    retVal = THOR_OK;

err_ret:
    return (retVal);
}

void MModeRenderer::loadFreezeModeTexture(bool upDirection)
{
    int maxIndex, minIndex;
    int minFrameNum, maxFrameNum;
    int startIndex;
    int frameWidth;
    int maxRelFrameNum; // relative max frame no. when minNum = 0;

    maxRelFrameNum = mMaxFrameNum - mMinFrameNum;

    if (mDataWidth <= mTextureWidth)
    {
        minFrameNum = 0;
        maxFrameNum = maxRelFrameNum;
    }
    else
    {
        frameWidth = mTextureWidth/mNumLines;

        if (upDirection)
        {
            // up direction
            minIndex = mMinDrawnIndex - mTextureWidth * 0.1f;
            minFrameNum = minIndex / mNumLines;

            if (minFrameNum < 0)
                minFrameNum = 0;
            else if (minFrameNum > (maxRelFrameNum - frameWidth + 1))
                minFrameNum = maxRelFrameNum - frameWidth + 1;

            maxFrameNum = minFrameNum + frameWidth - 1;

            if (maxFrameNum > maxRelFrameNum)
                maxFrameNum = maxRelFrameNum;
        }
        else
        {
            // down direction
            maxIndex = mMaxDrawnIndex + mTextureWidth * 0.1f;
            maxFrameNum = maxIndex / mNumLines;

            if (maxFrameNum > maxRelFrameNum)
                maxFrameNum = maxRelFrameNum;
            else if (maxFrameNum < frameWidth - 1)
                maxFrameNum = frameWidth - 1;

            minFrameNum = maxFrameNum - frameWidth + 1;

            if (minFrameNum < 0)
                minFrameNum = 0;
        }
    }

    if ((maxFrameNum - minFrameNum + 1) * mNumLines > mTextureWidth)
        ALOGE("FreezeModeTextureLoader tries to load more data than allocated.");

    startIndex = 0;

    // load texture
    for (int i = minFrameNum + mMinFrameNum; i < maxFrameNum + 1 + mMinFrameNum; i++)
    {
        // load up textures
        glTexSubImage2D(GL_TEXTURE_2D,
                        0,
                        0,
                        startIndex,
                        mScParams.numSamples,
                        mNumLines,
                        GL_LUMINANCE,
                        GL_UNSIGNED_BYTE,
                        mCineBufferPtr->getFrame(i, DAU_DATA_TYPE_M));

        startIndex += mNumLines;
    }

    checkGlError("loadData for FreezeMode");

    // update loaded indices
    mMinLoadedIndex = minFrameNum * mNumLines;
    mMaxLoadedIndex = maxFrameNum * mNumLines + mNumLines - 1;
}

//-----------------------------------------------------------------------------
bool MModeRenderer::prepare()
{
    // place holder for checking validity of indices (dataindex and drawindex)
    if ((mNumFramesDraw < 1.0f) && mIsLive)
    {
        ALOGD("Wating for the data.  No frame is ready to be drawn.");
        return(false);
    }
    else
        return(true);
}

//-----------------------------------------------------------------------------
bool MModeRenderer::updateIndices()
{
    bool        retVal = false;
    int         numBufLines;
    int         numLinesDraw;
    int         scrollRate;

    // index update
    numLinesDraw = 0;

    if (mDataIndex >= mDrawIndex) {
        numBufLines = mDataIndex - mDrawIndex;
    }
    else {
        numBufLines = mDataIndex + getWidth() - mDrawIndex;
    }

    if (mNumFramesDraw >= 1.0f)
    {
        numBufLines = numBufLines / (((int) mNumFramesDraw) + 1);
    }

    scrollRate = ((int) mScrollRate);

    if (numBufLines > 4 * scrollRate)
    {
        numLinesDraw = 2 * scrollRate;
    }
    else if (numBufLines > 2 * scrollRate)
    {
        numLinesDraw = scrollRate + 1;
    }
    else if (numBufLines > scrollRate)
    {
        numLinesDraw = scrollRate;
    }
    else
    {
        numLinesDraw = numBufLines;
    }

    mStartLineIdx = mDrawIndex;
    mEndLineIdx = mDrawIndex + numLinesDraw - 1;

    if (mEndLineIdx > getWidth() - 1) {
        mStartLineIdx = 0;
        mEndLineIdx -= getWidth();
        mDrawingWrapped = true;
    }

    if (numLinesDraw == 0) {
        ALOGD("Not Enough Data for Drawing Any M Lines");
        goto err_ret;
    }

    if (numBufLines > 10 * scrollRate)
    {
        ALOGW("Too much data in the Buffer");
    }

    retVal = true;

err_ret:
    return(retVal);
}

//-----------------------------------------------------------------------------
uint32_t MModeRenderer::draw()
{
    uint32_t    retNumDraw = 0;
    int         startPoint;
    int         endPoint;
    int         wrapStartPoint;
    float       lineX;

    // freeze mode
    if (!mIsLive)
    {
        drawFreezeMode();
        return 0;  // always set to redraw count to zero
    }

    // update num frames to draw
    mNumFramesDraw -= 1.0f;

    // ignore the negative values but needs to keep track of those.
    retNumDraw = (uint32_t) floor(mNumFramesDraw);

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
            wrapStartPoint = mDrawIndex;
        }
        else
        {
            wrapStartPoint = getWidth();
        }

        // when the drawing is wrapped.
        glScissor(getX() + wrapStartPoint - M_TRACELINE_THICKNESS, getY(),
                  getWidth() - wrapStartPoint + 2 * M_TRACELINE_THICKNESS, getHeight());
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindBuffer(GL_ARRAY_BUFFER, mVB[0]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVB[1]);
        glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0);
        checkGlError("glDrawArrays");

        mDrawingWrapped = false;
    }

    startPoint = mStartLineIdx;
    endPoint = mEndLineIdx + 1;

    lineX = 2.0f * ((float) (endPoint - 1)) / getWidth() - 1.0f;

    glScissor(getX() + startPoint - M_TRACELINE_THICKNESS, getY(),
              endPoint - startPoint + M_TRACELINE_THICKNESS, getHeight());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // update mEndLineIdx
    mDrawIndex = mEndLineIdx + 1;

    if (mDrawIndex > getWidth() - 1) {
        mDrawIndex -= getWidth();

        mDrawingWrapped = true;
    }

    glBindBuffer(GL_ARRAY_BUFFER, mVB[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVB[1]);
    glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0);
    checkGlError("glDrawArrays");

    // drawLine
    renderMTraceLine(lineX, -1.0f, 1.0f);

done:
    return(retNumDraw);
}

//-----------------------------------------------------------------------------
void MModeRenderer::drawFreezeMode()
{
    glUseProgram(mProgram);
    checkGlError("glUseProgram-FreezeMode");

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mTextureHandle);

    glUniform1i(mTextureUniformHandle, 3);

    // tint adjustment
    glUniform3fv(mTintHandle, 1, mTintAdj);
    checkGlError("glUniform3fv");

    glEnable(GL_SCISSOR_TEST);
    glScissor(getX(), getY(), getWidth(), getHeight());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindBuffer(GL_ARRAY_BUFFER, mVB[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVB[1]);
    glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0);
    checkGlError("glDrawArrays-FreezeMode");
}

//-----------------------------------------------------------------------------
void MModeRenderer::setFreezeModeFrameNum(int frameNum)
{
    // check the frameNum
    int tmpMaxDrawnIndex, tmpMinDrawnIndex;
    float lowerTexLimit, upperTexLimit;

    // TODO: finer granuality, if needed.
    mCurDrawnIndex = (frameNum - mMinFrameNum) * mNumLines;

    if (mNoScrollFreeze)
    {
        // no scroll mode
        if (mCurDrawnIndex > mMaxDrawnIndex)
        {
            mCurDrawnIndex = mMaxDrawnIndex;
        }
        if (mCurDrawnIndex < mMinDrawnIndex)
        {
            mCurDrawnIndex = mMinDrawnIndex;
        }
    }
    else
    {
        tmpMinDrawnIndex = mMinDrawnIndex;
        tmpMaxDrawnIndex = mMaxDrawnIndex;

        if (mCurDrawnIndex > mMaxDrawnIndex)
        {
            // check the index is in the range.
            if (mCurDrawnIndex > mDataWidth - 1)
            {
                mCurDrawnIndex = mDataWidth - 1;
            }

            tmpMaxDrawnIndex = mCurDrawnIndex;
            tmpMinDrawnIndex = tmpMaxDrawnIndex - getWidth() + 1;
        }
        else if (mCurDrawnIndex < mMinDrawnIndex)
        {
            if (mCurDrawnIndex < 0)
            {
                mCurDrawnIndex = 0;
            }

            tmpMinDrawnIndex = mCurDrawnIndex;
            tmpMaxDrawnIndex = tmpMinDrawnIndex + getWidth() - 1;
        }

        if ((tmpMinDrawnIndex != mMinDrawnIndex) || (tmpMaxDrawnIndex != mMaxDrawnIndex))
        {
            mMinDrawnIndex = tmpMinDrawnIndex;
            mMaxDrawnIndex = tmpMaxDrawnIndex;

            if (mMaxDrawnIndex > mMaxLoadedIndex)
            {
                loadFreezeModeTexture(true);
            }
            else if (mMinDrawnIndex < mMinLoadedIndex)
            {
                loadFreezeModeTexture(false);
            }

            // update locData
            glBindBuffer(GL_ARRAY_BUFFER, mVB[0]);
            float *locPtr = (float *)glMapBufferRange(GL_ARRAY_BUFFER, 0, 16, GL_MAP_WRITE_BIT);

            lowerTexLimit = (mMinDrawnIndex - mMinLoadedIndex + 0.5f) / mTextureWidth;
            upperTexLimit = (mMaxDrawnIndex - mMinLoadedIndex + 0.5f) / mTextureWidth;

            locPtr[3] = lowerTexLimit;
            locPtr[7] = upperTexLimit;
            locPtr[11] = lowerTexLimit;
            locPtr[15] = upperTexLimit;

            glUnmapBuffer(GL_ARRAY_BUFFER);

            // update time scale shift
            setPan(2.0f * mMinDrawnIndex / getWidth(), 0.0f);
        }
    }

    return;
}

//-----------------------------------------------------------------------------
void MModeRenderer::renderMTraceLine(float x1, float y1, float y2)
{
    float lineData[4];
    lineData[0] = x1;
    lineData[1] = y1;
    lineData[2] = x1;
    lineData[3] = y2;

    glUseProgram(mLineProgram);
    checkGlError("glUseLineProgram");

    glUniform3fv(mColorHandle, 1, mLineColor);

    glLineWidth(M_TRACELINE_THICKNESS/2.0f);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, lineData);
    glDrawArrays(GL_LINES, 0, 2);
    checkGlError("glDrawArrays - renderMTraceLine");
}

//-----------------------------------------------------------------------------
bool MModeRenderer::modeSpecificMethod()
{
    return skipFrameDraw();
}

//-----------------------------------------------------------------------------
bool MModeRenderer::skipFrameDraw()
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
int MModeRenderer::getDrawnIndex()
{
    int retVal = 0;

    if (!mIsLive)
    {
        retVal = mMinDrawnIndex;
    }

    return retVal;
}

//-----------------------------------------------------------------------------
int MModeRenderer::getCurDrawnIndex()
{
    int retVal = 0;

    if (!mIsLive)
    {
        retVal = mCurDrawnIndex;
    }

    return retVal;
}

//-----------------------------------------------------------------------------
void MModeRenderer::setParams(int numSamples, float sampleSpacingMm, float startSampleMm,
                              float scrollSpeed, uint32_t targetFrameRate,
                              uint32_t numLinesPerBFrame, uint32_t bFrameIntervalMs)
{
    mScParams.sampleSpacingMm = sampleSpacingMm;
    mScParams.startSampleMm = startSampleMm;
    mScParams.numSamples = numSamples;      // number of samples per display line

    mScrollSpeed = scrollSpeed;             // display lines per second

    float drawInterval = 1.0f/((float) targetFrameRate);
    float feedInterval = bFrameIntervalMs/1000.0f;

    mScrollRate = (mScrollSpeed) * drawInterval;    // lines per draw
    mFeedRate = mScrollSpeed * feedInterval;
    mNumLines = numLinesPerBFrame;          // number of display lines per B-frame

    mNumFramesDrawPerBFrame = mFeedRate/mScrollRate;

    // TODO: verify whether parameters make sense
    ALOGD("Scroll Rate (lines / draw) : %f", mScrollRate);
    ALOGD("Feed Rate (lines / bFrame): %f", mFeedRate);
    ALOGD("numLine per BFrame from dB: %d", mNumLines);
    ALOGD("MFrame / BFrame Draw Ratio: %f", mNumFramesDrawPerBFrame);
}

//-----------------------------------------------------------------------------
void MModeRenderer::getPhysicalToPixelMap(float *mapMat)
{
    mScHelper.getPhysicalToPixelMap(getWidth(), getHeight(), mapMat);
}

//-----------------------------------------------------------------------------
void MModeRenderer::setPan(float deltaX, float deltaY)
{
    mScHelper.setPan(deltaX, deltaY);
}

void MModeRenderer::setStillMLineTime(float lineTime)
{
    mStillMLineTime = lineTime;
}
float MModeRenderer::getStillMLineTime()
{
    return mStillMLineTime;
}
void MModeRenderer::setStillTimeShift(float timeShift)
{
    mStillTimeShift = timeShift;
}

//-----------------------------------------------------------------------------
void MModeRenderer::setTintAdjustment(float coeffR, float coeffG, float coeffB)
{
    mTintAdj[0] = coeffR;
    mTintAdj[1] = coeffG;
    mTintAdj[2] = coeffB;
}

//-----------------------------------------------------------------------------
void MModeRenderer::setFrameNum(int frameNum)
{
    if (((frameNum - mPrevFrameNum) > 5) || (frameNum <= mPrevFrameNum))
    {
        ALOGW("Current frameNum: %d, previous frameNum: %d", frameNum, mPrevFrameNum);
        mPrevFrameNum = frameNum - 1;
    }

    for (int i = mPrevFrameNum+1; i < frameNum + 1; i++)
    {
        setFrame(mCineBufferPtr->getFrame(i, DAU_DATA_TYPE_M));
    }

    mPrevFrameNum = frameNum;
}

//-----------------------------------------------------------------------------
void MModeRenderer::setFrame(uint8_t* framePtr)
{
    int startIndex = mDataIndex;
    int endIndex = mDataIndex + mNumLines - 1;

    bool wrapped = false;

    if (endIndex > getWidth() - 1) {
        // wrapped
        endIndex -= getWidth();
        wrapped = true;
    }
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mTextureHandle);

    if (!wrapped) {
        // straight case
        glTexSubImage2D(GL_TEXTURE_2D,
                        0,
                        0,
                        startIndex,
                        mScParams.numSamples,
                        mNumLines,
                        GL_LUMINANCE,
                        GL_UNSIGNED_BYTE,
                        framePtr);
    }
    else
    {
        ALOGD("feedData wrapped!!!!!!!");
        // wrapped case
        int num_trans = getWidth() - startIndex;
        glTexSubImage2D(GL_TEXTURE_2D,
                        0,
                        0,
                        startIndex,
                        mScParams.numSamples,
                        num_trans,
                        GL_LUMINANCE,
                        GL_UNSIGNED_BYTE,
                        framePtr);

        glTexSubImage2D(GL_TEXTURE_2D,
                        0,
                        0,
                        0,
                        mScParams.numSamples,
                        endIndex + 1,
                        GL_LUMINANCE,
                        GL_UNSIGNED_BYTE,
                        framePtr + (num_trans * mScParams.numSamples));
    }

    checkGlError("feedMData");

    mDataIndex = endIndex + 1;

    if (mDataIndex > getWidth() - 1) {
        //wrapped
        mDataIndex -= getWidth();
    }

    // update num of frames need to be drawn
    mNumFramesDraw += mNumFramesDrawPerBFrame;
}
