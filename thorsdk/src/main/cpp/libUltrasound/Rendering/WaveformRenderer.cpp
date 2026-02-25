//
// Copyright 2018 EchoNous Inc.
//

#define LOG_TAG "WaveformRenderer"

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <GLES3/gl3.h>
#include <ThorUtils.h>
#include <WaveformRenderer.h>

#define WF_TRACELINE_THICKNESS 2

const char* const WaveformRenderer::mVertexShaderSource =
#include "og_vertex.glsl"
;

const char* const WaveformRenderer::mFragmentShaderSource =
#include "og_fragment.glsl"
;

//-----------------------------------------------------------------------------
WaveformRenderer::WaveformRenderer() :
    Renderer(),
    mCineBufferPtr(nullptr),
    mDataType(0),
    mProgram(0),
    mLineThickness(5),
    mNumLines(0),
    mDataIndex(0),
    mDrawIndex(0),
    mInputWidth(1000),
    mScrollSpeed(600.0f),
    mScrollRate(0.0),
    mFeedRate(0.0),
    mNumFramesDraw(0.0),
    mNumFramesDrawPerBFrame(1.0),
    mPrevFrameNum(-1),
    mMinFrameNum(0),
    mMaxFrameNum(0),
    mDrawingWrapped(false),
    mWholeScreenRedraw(false),
    mSingleFrameFreezeMode(false),
    mOffScreenDrawing(false),
    mSingleFrameFrameWidth(-1),
    mSingleFrameTraceIndex(-1),
    mDecimRatio(1),
    mDecimInputWidth(1),
    mMaxDrawIndex(0),
    mJitterAmt(0.0f),
    mJitterSum(0.0f),
    mDrawingJitterSum(0.0f)
{
    memset(mVB, 0, sizeof(mVB));
    // Text Color - default Yellow
    mLineColor[0] = 1.0f;
    mLineColor[1] = 1.0f;
    mLineColor[2] = 0.0f;

    // TraceLine Color
    mTraceLineColor[0] = 1.0f;
    mTraceLineColor[1] = 0.427f;
    mTraceLineColor[2] = 0.0f;
 }

//-----------------------------------------------------------------------------
WaveformRenderer::~WaveformRenderer()
{
    close();
}

//-----------------------------------------------------------------------------
ThorStatus WaveformRenderer::open()
{
    ThorStatus      retVal = THOR_ERROR;
    float           scrollTime;
    float           timeScale;

    // Initial Buffer data - zeros
    point           waveform[mDecimInputWidth];
    float           halfWidth;
    int             waveformIdxCnt;

    mProgram = createProgram(mVertexShaderSource, mFragmentShaderSource);
    if (!checkGlError("createProgram")) goto err_ret;

    mDrawColorUniformHandle = glGetUniformLocation(mProgram, "u_drawColor");
    if (!checkGlError("glGetUniformLocation")) goto err_ret;

    glGenBuffers(2, mVB);
    glBindBuffer(GL_ARRAY_BUFFER, mVB[0]);
    halfWidth = ((float) mInputWidth)/2.0f;

    waveformIdxCnt = 0;

    for (int i = 0; i < mInputWidth; i += mDecimRatio)
    {
        float x = (i - halfWidth - 0.5f) / halfWidth;

        waveform[waveformIdxCnt].x = x;
        waveform[waveformIdxCnt].y = 0.0f;

        waveformIdxCnt++;
    }

    // Copy an array to the buffer object
    glBufferData(GL_ARRAY_BUFFER, sizeof(point) * waveformIdxCnt, waveform, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(4, 2, GL_FLOAT, false, sizeof(float) * 2, 0);
    glEnableVertexAttribArray(4);
    if (!checkGlError("glVertexAttribPointer")) goto err_ret;

    // init variables
    mNumFramesDraw = 0.0;
    mDataIndex = 0;
    mDrawIndex = 0;
    mWholeScreenRedraw = false;
    mOffScreenDrawing = false;

    // preparing transformation matrix
    scrollTime = ((float) mInputWidth) / mScrollSpeed;
    timeScale = 2.0f / scrollTime;
    // init ScHelper, set Scale and XY shift
    mScHelper.init();

    if (scrollTime > 0.0f)
    {
        mScHelper.setScaleXYShift(timeScale, 1.0f, -1.0f, 0.0f);
        // set aspect ratio (this matrix is a special case)
        mScHelper.setAspect(1.0f);
    }

    mJitterSum = 0.0f;
    mDrawingJitterSum = 0.0f;

    retVal = THOR_OK;

err_ret:
    return(retVal);
}

//-----------------------------------------------------------------------------
void WaveformRenderer::close()
{
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
ThorStatus WaveformRenderer::prepareRenderer(CineBuffer* cineBuffer, uint32_t dataType)
{
    ThorStatus retVal = THOR_ERROR;

    mCineBufferPtr = cineBuffer;
    mDataType = dataType;

    // get min/max frame numbers
    mMinFrameNum = mCineBufferPtr->getMinFrameNum();
    mMaxFrameNum = mCineBufferPtr->getMaxFrameNum();

    // clean up the screen - whole waveform renderer section
    glScissor(getX(), getY(), getWidth(), getHeight());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mWholeScreenRedraw = false;
    mOffScreenDrawing = false;
    mPrevFrameNum = -1;

    retVal = THOR_OK;

    return (retVal);
}

//-----------------------------------------------------------------------------
ThorStatus WaveformRenderer::prepareRendererOffScreen(int minFrameNum, int maxFrameNum, uint32_t dataType)
{
    ThorStatus retVal = THOR_ERROR;

    mCineBufferPtr = nullptr;
    mDataType = dataType;

    // get min/max frame numbers
    mMinFrameNum = minFrameNum;
    mMaxFrameNum = maxFrameNum;

    // clean up the screen - whole waveform renderer section
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

    retVal = THOR_OK;

    return (retVal);
}


//-----------------------------------------------------------------------------
void WaveformRenderer::setFrameNum(int frameNum)
{
    uint8_t*                        framePtr;

    if ((((frameNum - mPrevFrameNum) == 1) || ((frameNum - mPrevFrameNum) == 2)) && !mSingleFrameFreezeMode)
    {
        // sequential playback - partial update (upto 2 frames)
        mWholeScreenRedraw = false;

        for (int i = mPrevFrameNum+1; i < frameNum + 1; i++)
        {
            framePtr = mCineBufferPtr->getFrame(i,
                                                mDataType,
                                                CineBuffer::FrameContents::DataOnly);
            setFrame((float*) framePtr);
        }
    }
    else
    {
        // whole screen redraw
        mWholeScreenRedraw = true;
    }

    mPrevFrameNum = frameNum;

    return;
}

//-----------------------------------------------------------------------------
void WaveformRenderer::prepareFreezeMode(int reqFrameNum, bool updateIndices)
{
    int     frameWidth;
    int     endFrameNum;
    int     startFrameNum;
    int     realMinFrameNum;
    int     curDrawIndex;
    int     endDrawIndex;
    int     numLinesPerFrame;
    int     dataType;
    int     inFrameNum;
    int     frameDecimRatio;
    float   inputWidthWithJitter;
    float   dataSpan;
    float   divVal;
    float   curDrawIndexFlt;
    float   curJitterSum;
    bool    skipPoint = false;
    bool    partialWidthData = false;

    CineBuffer::CineFrameHeader* frameHeaderPtr = nullptr;

    // buffer data
    point   waveform[mDecimInputWidth];
    float   halfWidth;
    float*  dataPtr;
    float*  dstRefPtr;
    float   x;
    int     dataCnt, modVal, j, decimDrawIndex;
    int     initOffset = 0;

    inFrameNum = reqFrameNum;
    if (inFrameNum < 0)
    {
        inFrameNum = mPrevFrameNum;
    }

    // mMinFrameNum is not updated when the system gets into CineReview.
    // However, it is updated (typically to 0) when it gets into ExamReview.
    // real MinFrameNum: calculated based on inFrameNum and CINE_FRAME_COUNT, as we cannot use
    // mMinFrameNum for CineReview and LiveMode.
    realMinFrameNum = inFrameNum - CINE_FRAME_COUNT + 1 - mMinFrameNum;
    if (realMinFrameNum < 0)
        realMinFrameNum = 0;

    // start and end frame numbers
    endFrameNum = inFrameNum - mMinFrameNum;    // in live mode mMinFrameNum: 0
    frameWidth = (mInputWidth + mNumLines - 1)/mNumLines;
    inputWidthWithJitter = (((float) mInputWidth) + mJitterAmt);
    startFrameNum = endFrameNum - frameWidth + 1;

    if (startFrameNum < realMinFrameNum)
    {
        startFrameNum = realMinFrameNum;
        partialWidthData = true;                // partial screen drawing (as dataWidth < drawingWidth)
    }

    // drawing start location, so (FrameNum - 1) -> last drawing location.
    dataSpan = ((float) (startFrameNum * mNumLines));
    divVal = dataSpan / inputWidthWithJitter;
    curDrawIndexFlt = dataSpan - floor(divVal) * inputWidthWithJitter;
    curDrawIndex = (int) ceil(curDrawIndexFlt);
    curJitterSum = ((float) curDrawIndex) - curDrawIndexFlt;

    // set numLinesPerFrame to read and dataType
    numLinesPerFrame = mNumLines;
    dataType = mDataType;
    frameDecimRatio = mDecimRatio;

    // singleFrameFreezeMode - update dataType and numLinesPerFrame
    if (mSingleFrameFreezeMode && updateIndices)
    {
        if (mDataType == DAU_DATA_TYPE_DA)
            dataType = DAU_DATA_TYPE_DA_SCR;

        if (mDataType == DAU_DATA_TYPE_ECG)
            dataType = DAU_DATA_TYPE_ECG_SCR;

        startFrameNum = 0;
        endFrameNum = 0;

        if (mSingleFrameFrameWidth < 0)
        {
            frameHeaderPtr = (CineBuffer::CineFrameHeader*) mCineBufferPtr->getFrame(0,
                                                                                     dataType, CineBuffer::FrameContents::IncludeHeader);
            mSingleFrameFrameWidth = frameHeaderPtr->numSamples;
        }

        // To Prevent drawing erroneous lines due to the time-span mismatch
        if (mSingleFrameFrameWidth > mDecimInputWidth)
            mSingleFrameFrameWidth = mDecimInputWidth;

        numLinesPerFrame = mSingleFrameFrameWidth;

        // should be always 0
        curDrawIndex = 0;
        frameDecimRatio = 1;
    }

    dataCnt = 0;

    // current index adjustment for decimation offset
    modVal = curDrawIndex%mDecimRatio;
    decimDrawIndex = curDrawIndex/mDecimRatio;
    halfWidth = ((float) mInputWidth)/2.0f;

    if (modVal != 0)
    {
        decimDrawIndex += 1;
        initOffset = mDecimRatio - modVal;
        curDrawIndex += initOffset;
    }

    for (int i = startFrameNum + mMinFrameNum; i <= endFrameNum + mMinFrameNum; i++)
    {
        // frameData point;
        dataPtr = (float*) mCineBufferPtr->getFrame(i, dataType);

        for (j = initOffset; j < numLinesPerFrame; j += frameDecimRatio)
        {
            x = (curDrawIndex - halfWidth - 0.5f) / halfWidth;

            waveform[decimDrawIndex].x = x;
            waveform[decimDrawIndex].y = dataPtr[j];

            curDrawIndex += mDecimRatio;
            decimDrawIndex++;

            if (curDrawIndex > mInputWidth - 1)
            {
                curDrawIndex -= mInputWidth;

                // Index adjustment
                j -= curDrawIndex;
                curDrawIndex = 0;
                decimDrawIndex = 0;
            }

            dataCnt++;
        }

        // update initOffset
        initOffset = j - numLinesPerFrame;
    }

    endDrawIndex = curDrawIndex;

    // put remaining area to zeros (if the whole InputWidth is not filled)
    for (int i = dataCnt; i < mDecimInputWidth; i++)
    {
        x = (curDrawIndex - halfWidth - 0.5f) / halfWidth;

        waveform[decimDrawIndex].x = x;
        waveform[decimDrawIndex].y = 0.0f;

        curDrawIndex += mDecimRatio;
        decimDrawIndex++;

        if (curDrawIndex > mInputWidth - 1)
        {
            curDrawIndex -= mInputWidth;

            // Index adjustment
            i -= curDrawIndex;
            curDrawIndex = 0;
            decimDrawIndex = 0;
        }
    }

    if (updateIndices)
    {
        // LiveMode or SingleFrameFreezeMode
        mDataIndex = endDrawIndex;      // update with endDrawIndex
        mDrawingJitterSum = 0.0f;
        mDrawIndex = mDataIndex;
        mJitterSum = curJitterSum;

        if (partialWidthData)
            mMaxDrawIndex = mDataIndex - 1;
        else
            mMaxDrawIndex = mInputWidth - 1;

        // SingleFrameFreezeMode
        if (mSingleFrameFreezeMode)
        {
            mMaxDrawIndex = mSingleFrameFrameWidth * mDecimRatio - 1;
            mDrawIndex = mSingleFrameTraceIndex * mDecimRatio;
        }

        // load waveform buffer to ARRAY_BUFFER
        glBindBuffer(GL_ARRAY_BUFFER, mVB[0]);
        glBufferData(GL_ARRAY_BUFFER, mDecimInputWidth*sizeof(point), waveform, GL_DYNAMIC_DRAW);
    }
    else
    {
        // For Recording - concatenating frames and store it into texture memory
        mSingleFrameTraceIndex = endDrawIndex/mDecimRatio;

        if (partialWidthData)
            mSingleFrameFrameWidth = mSingleFrameTraceIndex - 1;
        else
            mSingleFrameFrameWidth = mDecimInputWidth;

        frameHeaderPtr = nullptr;

        // copy the concatenated data
        if (dataType == DAU_DATA_TYPE_DA)
        {
            dstRefPtr = (float*) mCineBufferPtr->getFrame(0, DAU_DATA_TYPE_DA_SCR);
            memset(dstRefPtr, 0, MAX_DA_SCR_FRAME_SIZE);

            frameHeaderPtr = (CineBuffer::CineFrameHeader*) mCineBufferPtr->getFrame(0, DAU_DATA_TYPE_DA_SCR,
                                                                                     CineBuffer::FrameContents::IncludeHeader);
        }
        else if (dataType == DAU_DATA_TYPE_ECG)
        {
            dstRefPtr = (float*) mCineBufferPtr->getFrame(0, DAU_DATA_TYPE_ECG_SCR);
            memset(dstRefPtr, 0, MAX_ECG_SCR_FRAME_SIZE);

            frameHeaderPtr = (CineBuffer::CineFrameHeader*) mCineBufferPtr->getFrame(0, DAU_DATA_TYPE_ECG_SCR,
                                                                                     CineBuffer::FrameContents::IncludeHeader);
        }

        if (frameHeaderPtr != nullptr)
        {
            frameHeaderPtr->numSamples = (uint32_t) mSingleFrameFrameWidth;
            frameHeaderPtr->frameNum = endFrameNum + mMinFrameNum;

            for (int i = 0; i < mSingleFrameFrameWidth; i++)
            {
                dstRefPtr[i] = waveform[i].y;
            }
        }
    }

    return;
}

//-----------------------------------------------------------------------------
void WaveformRenderer::prepareFreezeModeOffScreen(float* framePtr)
{
    // this only occurs for the whole screen rendering
    int     numLinesPerFrame;

    // buffer data
    point   waveform[mDecimInputWidth];
    float   halfWidth;
    float   x;
    int     dataCnt, j, decimDrawIndex;
    int     curDrawIndex = 0;

    numLinesPerFrame = mSingleFrameFrameWidth;

    mMaxDrawIndex = mSingleFrameFrameWidth * mDecimRatio- 1;
    mDrawIndex = mSingleFrameTraceIndex * mDecimRatio;

    halfWidth = ((float) mInputWidth)/2.0f;

    // current index adjustment for decimation offset
    decimDrawIndex = 0;

    for (j = 0; j < numLinesPerFrame; j += 1)
    {
        x = (curDrawIndex - halfWidth - 0.5f) / halfWidth;

        waveform[decimDrawIndex].x = x;
        waveform[decimDrawIndex].y = framePtr[j];

        curDrawIndex += mDecimRatio;
        decimDrawIndex++;

        if (curDrawIndex > mInputWidth - 1)
        {
            curDrawIndex -= mInputWidth;

            // Index adjustment
            j -= curDrawIndex;
            curDrawIndex = 0;
            decimDrawIndex = 0;
        }

        dataCnt++;
    }

    // put remaining area to zeros (if the whole InputWidth is not filled)
    for (int i = dataCnt; i < mDecimInputWidth; i++)
    {
        x = (curDrawIndex - halfWidth - 0.5f) / halfWidth;

        waveform[decimDrawIndex].x = x;
        waveform[decimDrawIndex].y = 0.0f;

        curDrawIndex += mDecimRatio;
        decimDrawIndex++;

        if (curDrawIndex > mInputWidth - 1)
        {
            curDrawIndex -= mInputWidth;

            // Index adjustment
            i -= curDrawIndex;
            curDrawIndex = 0;
            decimDrawIndex = 0;
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, mVB[0]);
    glBufferData(GL_ARRAY_BUFFER, mDecimInputWidth*sizeof(point), waveform, GL_DYNAMIC_DRAW);
}

//-----------------------------------------------------------------------------
void WaveformRenderer::drawFreezeMode()
{
    int     startPoint;
    int     endPoint;
    float   lineX;

    // draw
    glUseProgram(mProgram);
    checkGlError("glUseProgram");

    glUniform3fv(mDrawColorUniformHandle, 1, mLineColor);
    glLineWidth(mLineThickness);

    glEnable(GL_SCISSOR_TEST);

    startPoint = (int) floor((0.0f)/((float) mInputWidth) * getWidth()) - WF_TRACELINE_THICKNESS - 2;
    endPoint = (int) floor(((float) mMaxDrawIndex)/((float) mInputWidth) * getWidth());

    lineX = 2.0f * ((float) (mDrawIndex - 1))/((float) mInputWidth) - 1.0f;

    // clear whole screen
    glScissor(getX(), getY(), getWidth(), getHeight());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glScissor(getX() + startPoint, getY(),
              endPoint - startPoint, getHeight());

    glBindBuffer(GL_ARRAY_BUFFER, mVB[0]);
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_LINE_STRIP, 0, mMaxDrawIndex/mDecimRatio+1);
    checkGlError("glDrawArrays");

    // adjust scissor for tracline
    endPoint += WF_TRACELINE_THICKNESS;
    glScissor(getX() + startPoint, getY(),
              endPoint - startPoint, getHeight());

    // draw traceLine
    renderTraceLine(lineX, -1.0f, 1.0f);

    return;
}

//-----------------------------------------------------------------------------
bool WaveformRenderer::prepare()
{
    // place holder for checking validity of indices (dataindex and drawindex)
    if (mNumFramesDraw < 1.0f && !mWholeScreenRedraw)
    {
        ALOGD("Wating for the data.  No frame is ready to be drawn.");
        return(false);
    }
    else
        return(true);
}

//-----------------------------------------------------------------------------
void WaveformRenderer::setLineColor(float *lineColor)
{
    mLineColor[0] = lineColor[0];
    mLineColor[1] = lineColor[1];
    mLineColor[2] = lineColor[2];
}

//-----------------------------------------------------------------------------
void WaveformRenderer::setLineThickness(int lineThickness) {
    mLineThickness = lineThickness;
}

//-----------------------------------------------------------------------------
bool WaveformRenderer::updateIndices()
{
    bool    retVal = false;
    int     numBufLines;
    int     numLinesDraw;
    int     scrollRateInt;
    float   jitter;

    scrollRateInt = (int) mScrollRate;
    jitter = mScrollRate - ((float) scrollRateInt);
    mDrawingJitterSum += jitter;

    if (mDataIndex >= mDrawIndex)
    {
        numBufLines = mDataIndex - mDrawIndex;
    }
    else
    {
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
        ALOGD("Not Enough Data for Drawing Waveform");
        goto err_ret;
    }

    if (numBufLines > 10 * scrollRateInt)
    {
        ALOGW("Too much data in the Buffer");
    }

    retVal = true;

err_ret:
    return(retVal);

}

//-----------------------------------------------------------------------------
uint32_t WaveformRenderer::draw()
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
            prepareFreezeMode(-1, true);
            mNumFramesDraw = 0.0f;
        }

        drawFreezeMode();
        goto done;
    }

    if (!updateIndices())
        goto done;

    glUseProgram(mProgram);
    checkGlError("glUseProgram");

    glUniform3fv(mDrawColorUniformHandle, 1, mLineColor);
    glLineWidth(mLineThickness);

    glEnable(GL_SCISSOR_TEST);
    // additional drawing is needed
    //      when the drawing is wrapped around.
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

        // when drawing is wrapped.
        glScissor(getX() + wrapStartPoint - WF_TRACELINE_THICKNESS, getY(),
                  getWidth() - wrapStartPoint + 2 * WF_TRACELINE_THICKNESS + 1, getHeight());
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindBuffer(GL_ARRAY_BUFFER, mVB[0]);
        glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glDrawArrays(GL_LINE_STRIP, 0, mDecimInputWidth);
        checkGlError("glDrawArrays");

        mDrawingWrapped = false;
    }

    startPoint = ((int) floor(((float) mStartLineIdx)/((float) mInputWidth) * getWidth())) - WF_TRACELINE_THICKNESS - 2;
    endPoint = ((int) floor(((float) mEndLineIdx)/((float) mInputWidth) * getWidth()));

    lineX = 2.0f * ((float) mEndLineIdx)/((float) mInputWidth) - 1.0f;

    //
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
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_LINE_STRIP, 0, mDecimInputWidth);
    checkGlError("glDrawArrays");

    // adjust scissor for tracline
    endPoint += WF_TRACELINE_THICKNESS;
    glScissor(getX() + startPoint, getY(),
              endPoint - startPoint, getHeight());

    // draw traceLine
    renderTraceLine(lineX, -1.0f, 1.0f);

    // update num frames to draw
    mNumFramesDraw -= 1.0f;

    // ignore the negative values but needs to keep track of those.
    retNumDraw = (uint32_t) floor(mNumFramesDraw);

done:
    return(retNumDraw);
}

//-----------------------------------------------------------------------------
void WaveformRenderer::renderTraceLine(float x1, float y1, float y2) {
    point TLine[2];
    // assign points
    TLine[0].x = x1;
    TLine[0].y = y1;
    TLine[1].x = x1;
    TLine[1].y = y2;

    glUseProgram(mProgram);
    checkGlError("glUseProgram");

    glUniform3fv(mDrawColorUniformHandle, 1, mTraceLineColor);
    glLineWidth(WF_TRACELINE_THICKNESS);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(point), TLine);
    glDrawArrays(GL_LINES, 0, 2);
    checkGlError("glDrawArrays - renderTraceLine");
}

//-----------------------------------------------------------------------------
bool WaveformRenderer::modeSpecificMethod()
{
    return skipFrameDraw();
}

//-----------------------------------------------------------------------------
bool WaveformRenderer::skipFrameDraw()
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
void WaveformRenderer::setSamplesPerScreen(int inputWidth, int decimationRatio)
{
    // inputWidth = samplesPerScreen (before scaling for display)
    if (inputWidth > 0)                 // if non-positive => input width = display width
        mInputWidth = inputWidth;
    else
        mInputWidth = getWidth();

    if (decimationRatio > 0)
    {
        // decimated input width, pre-scaled screen width
        mDecimRatio = decimationRatio;
        mDecimInputWidth = (int) ceil(mInputWidth/((float) mDecimRatio));
    }
    else if (decimationRatio < 0)
    {
        // keep the same decimation ratio
        mDecimInputWidth = (int) ceil(mInputWidth/((float) mDecimRatio));
    }
    else
    {
        // decimationRatio = 0, find suitable parameters - placeholder
        mDecimInputWidth = (int) ceil(mInputWidth/((float) mDecimRatio));
    }

    ALOGD("Updated SamplesPerScreen: %d", mInputWidth);
    ALOGD("Sweeping Speed: %f", ((float) mInputWidth)/mScrollSpeed);
    ALOGD("Decimated pre-scaled Screen Width: %d", mDecimInputWidth);
}

//-----------------------------------------------------------------------------
void WaveformRenderer::setParams(int inputWidth, float scrollSpeed, int targetFrameRate,
                              int numLinesPerBFrame, float bFrameRate, float desiredTimeSpan,
                              int decimationRatio)
{
    // inputWidth = samplesPerScreen (before scaling for display)
    if (inputWidth > 0)                 // if non-positive => input width = display width
        mInputWidth = inputWidth;
    else
        mInputWidth = getWidth();

    mScrollSpeed = scrollSpeed;         // input lines per second (before scaling)

    float drawInterval = 1.0f/((float) targetFrameRate);
    float feedInterval = 1.0f/bFrameRate;
    float sweepingSpeed = ((float) mInputWidth)/mScrollSpeed;

    mScrollRate = mScrollSpeed * drawInterval;    // lines per draw
    mFeedRate = mScrollSpeed * feedInterval;
    mNumLines = numLinesPerBFrame;      // number of display lines per B-frame

    mNumFramesDrawPerBFrame = mFeedRate/mScrollRate;

    // decimated input width, pre-scaled screen width
    mDecimRatio = decimationRatio;
    mDecimInputWidth = (int) ceil(mInputWidth/((float) decimationRatio));

    // jitter amount
    mJitterAmt = desiredTimeSpan * mScrollSpeed - ((float) inputWidth);
    if ((mJitterAmt < 0.0f) || (mJitterAmt >= 1.0f))
    {
        ALOGW("Jitter Amount is out of range: %f", mJitterAmt);
        mJitterAmt = 0.0f;
    }

    // TODO: verify whether parameters make sense
    ALOGD("Scroll Rate (lines / draw) : %f", mScrollRate);
    ALOGD("Feed Rate (lines / bFrame): %f", mFeedRate);
    ALOGD("numLine per BFrame from dB: %d", mNumLines);
    ALOGD("WFFrame / BFrame Draw Ratio: %f", mNumFramesDrawPerBFrame);
    ALOGD("Sweeping Speed: %f", sweepingSpeed);
    ALOGD("Pixel Jitter: %f", mJitterAmt);
    ALOGD("Decimated pre-scaled Screen Width: %d", mDecimInputWidth);
}

//-----------------------------------------------------------------------------
ThorStatus WaveformRenderer::checkScrollParams()
{
    ThorStatus retVal = THOR_ERROR;
    float sweepingSpeed = ((float) mInputWidth)/mScrollSpeed;

    // parameters to check
    // 1. inputWidth must be >= 1
    // 2. scrollspeed (line / sec) should be > 1.0f
    // 3. sweeping speed must be in a reasonable range
    //           betweeen 0.5 ~ 30 sec
    // 4. feedRate should be >= 1.0f
    // 5. feedRate and mNumLines should be close

    if (mInputWidth < 1.0f)
    {
        ALOGE("Out of Range: samples per screen: %d", mInputWidth);
        goto err_ret;
    }

    if (mScrollSpeed < 1.0f)
    {
        ALOGD("Out of Range: scroll speed: %f", mScrollSpeed);
        goto err_ret;
    }

    if (sweepingSpeed < 0.5f || sweepingSpeed > 30.0f)
    {
        ALOGD("Out of Range: sweeping speed: %f", sweepingSpeed);
        goto err_ret;
    }

    if (mFeedRate < 1.0f)
    {
        ALOGD("Out of Range: feed rate: %f", mFeedRate);
        goto err_ret;
    }

    if (abs(mFeedRate - mNumLines) > 1.0f)
    {
        ALOGD("Mismatch!!! FeedRate: %f, numLinesPerBframe: %d", mFeedRate, mNumLines);
        goto err_ret;
    }

    retVal = THOR_OK;

err_ret:
    return (retVal);
}

//-----------------------------------------------------------------------------
void WaveformRenderer::getPhysicalToPixelMap(float *mapMat)
{
    mScHelper.getPhysicalToPixelMap(getWidth(), getHeight(), mapMat);
}

//-----------------------------------------------------------------------------
void WaveformRenderer::setFrame(float* dataPtr)
{
    // assuming the input data are in the range of -1 and 1.
    int     startIndex, curIndex;
    int     endIndex    = mDataIndex + mNumLines - 1;
    int     numTrans    = 0;
    float   halfWidth   = ((float) mInputWidth)/2.0f;
    int     dataCnt     = 0;
    int     initOffset  = 0;
    int     modVal      = 0;
    bool    adjustJitter = false;
    bool    wrapped = false;

    point tmpData[mNumLines];

    // start and end indices
    startIndex = mDataIndex/mDecimRatio;

    if (endIndex > mInputWidth - 1) {
        // wrapped
        endIndex -= mInputWidth;
        wrapped = true;

        mJitterSum += mJitterAmt;

        if (mJitterSum >= 1.0f)
        {
            mJitterSum -= 1.0f;
            adjustJitter = true;

            if (endIndex == 0)
                endIndex = mInputWidth - 1;
            else
                endIndex -= 1;
        }
    }

    // current index adjustment for decimation offset
    curIndex = mDataIndex;
    modVal = mDataIndex%mDecimRatio;

    if (modVal != 0)
    {
        startIndex += 1;
        initOffset = mDecimRatio - modVal;
        curIndex += initOffset;

        // when startIndex wraps around
        if (startIndex > mDecimInputWidth - 1)
        {
            startIndex = 0;
            initOffset = mInputWidth - mDataIndex;
            curIndex = 0;
        }
    }

    for (int i = initOffset; i < mNumLines; i += mDecimRatio)
    {
        tmpData[dataCnt].x = (curIndex - halfWidth - 0.5f) / halfWidth;
        tmpData[dataCnt].y = dataPtr[i];

        dataCnt++;
        curIndex += mDecimRatio;

        if (curIndex > mInputWidth - 1)
        {
            curIndex -= mInputWidth;

            // Index adjustment
            i -= curIndex;
            curIndex = 0;

            if (adjustJitter)
            {
                // skip one sample for adjustment
                i++;
            }

            numTrans = dataCnt;
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, mVB[0]);

    if (!wrapped || (dataCnt == numTrans))
    {
        glBufferSubData(GL_ARRAY_BUFFER,
                        startIndex * sizeof(point),
                        sizeof(point) * dataCnt,
                        tmpData);
    }
    else
    {
        glBufferSubData(GL_ARRAY_BUFFER,
                        startIndex * sizeof(point),
                        sizeof(point) * numTrans,
                        tmpData);

        glBufferSubData(GL_ARRAY_BUFFER,
                        0,
                        sizeof(point)*(dataCnt - numTrans),
                        &tmpData[numTrans]);
    }

    checkGlError("feedWaveFormData");

    mDataIndex = endIndex + 1;

    if (mDataIndex > mInputWidth - 1)
    {
        //wrapped
        mDataIndex -= mInputWidth;

        // if jitterSum has not been updated.
        if (!wrapped)
        {
            mJitterSum += mJitterAmt;
        }
    }

    // update num of frames need to be drawn
    mNumFramesDraw += mNumFramesDrawPerBFrame;
}
