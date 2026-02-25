//
// Copyright 2018 EchoNous Inc.
//

#define LOG_TAG "WaveformTexRenderer"

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <GLES3/gl3.h>
#include <ThorUtils.h>
#include <WaveformTexRenderer.h>

#define PIX_VAL 250

const char* const WaveformTexRenderer::mVertexShaderSource =
#include "wf_vertex.glsl"
;

const char* const WaveformTexRenderer::mFragmentShaderSource =
#include "wf_fragment.glsl"
;

//-----------------------------------------------------------------------------
WaveformTexRenderer::WaveformTexRenderer() :
    Renderer(),
    mProgram(0),
    mTextureHandle(0),
    mTextureUniformHandle(-1),
    mNumLines(0),
    mDataIndex(0),
    mDrawIndex(0),
    mScrollSpeed(600),
    mNumSamples(512),
    mScrollRate(0.0),
    mFeedRate(0.0),
    mNumFramesDraw(0.0),
    mNumFramesDrawPerBFrame(1.0),
    mLineThickness(5)
{
    memset(mVB, 0, sizeof(mVB));
    // Text Color - default Yellow
    mLineColor[0] = 1.0f;
    mLineColor[1] = 1.0f;
    mLineColor[2] = 0.0f;
 }

//-----------------------------------------------------------------------------
WaveformTexRenderer::~WaveformTexRenderer()
{
    close();
}

//-----------------------------------------------------------------------------
ThorStatus WaveformTexRenderer::open()
{
    ThorStatus      retVal = THOR_ERROR;

    float x_start = -1.0;
    float x_end   = 1.0;

    float y_start = -1.0;
    float y_end   = 1.0;

    float u_x = 0.5f / mNumSamples;
    float u_y = 0.5f / getWidth();

    float p0_x = x_start;
    float p0_y = y_end;

    float p1_x = x_end;
    float p1_y = y_end;

    float p2_x = x_start;
    float p2_y = y_start;

    float p3_x = x_end;
    float p3_y = y_start;

    // buffer location and binding
    float locData[16];     //4*4
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
    if (!checkGlError("glGetUniformLocation")) goto err_ret;

    mTextColorUniformHandle = glGetUniformLocation(mProgram, "u_TextColor");

    // texture init
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, mTextureHandle);
    if (!checkGlError("BindWaveformTexture")) goto err_ret;

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_LUMINANCE,
                 mNumSamples,
                 getWidth(),
                 0,
                 GL_LUMINANCE,
                 GL_UNSIGNED_BYTE,
                 0);

    if (!checkGlError("glTexImage2D-Waveform-Initialize")) goto err_ret;

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

    retVal = THOR_OK;

err_ret:
    return(retVal);
}

//-----------------------------------------------------------------------------
void WaveformTexRenderer::close()
{
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
bool WaveformTexRenderer::prepare()
{
    // place holder for checking validity of indices (dataindex and drawindex)
    if (mNumFramesDraw < 1.0f)
    {
        ALOGD("Wating for the data.  No frame is ready to be drawn.");
        return(false);
    }
    else
        return(true);
}

//-----------------------------------------------------------------------------
void WaveformTexRenderer::setLineColor(float *lineColor)
{
    mLineColor[0] = lineColor[0];
    mLineColor[1] = lineColor[1];
    mLineColor[2] = lineColor[2];
}

//-----------------------------------------------------------------------------
void WaveformTexRenderer::setLineThickness(int lineThickness) {
    mLineThickness = lineThickness;
}

//-----------------------------------------------------------------------------
bool WaveformTexRenderer::updateIndices()
{
    bool        retVal = false;
    int         numBufLines;
    int         numLinesDraw;
    int         scrollRate;

    // index update
    numBufLines = 0;
    numLinesDraw = 0;

    if (mDataIndex >= mDrawIndex) {
        numBufLines = mDataIndex - mDrawIndex;
    }
    else {
        numBufLines = mDataIndex + getWidth() - mDrawIndex;
    }

    scrollRate = ((int) mScrollRate);

    if (numBufLines > 6 * scrollRate) {
        numLinesDraw = 2 * scrollRate;
    }
    else if (numBufLines > 4 * scrollRate)
    {
        numLinesDraw = scrollRate + 1;
    }
    else if (numBufLines >= 2* scrollRate)
    {
        numLinesDraw = scrollRate;
    }

    mStartLineIdx = mDrawIndex;
    mEndLineIdx = mDrawIndex + numLinesDraw - 1;

    if (mEndLineIdx > getWidth() - 1) {
        mStartLineIdx = 0;
        mEndLineIdx -= getWidth();
    }

    // cleaning is not needed as the live renderer clears the entire screen

    if (numLinesDraw == 0) {
        ALOGD("Not Enough Data for Drawing Waveform");
        goto err_ret;
    }

    retVal = true;

err_ret:
    return(retVal);

}

//-----------------------------------------------------------------------------
uint32_t WaveformTexRenderer::draw()
{
    uint32_t    retNumDraw = 0;
    int         startPoint;
    int         endPoint;

    // update num frames to draw
    mNumFramesDraw -= 1.0f;

    // ignore the negative values but needs to keep track of those.
    retNumDraw = (uint32_t) floor(mNumFramesDraw);

    if (!updateIndices())
        goto done;

    glUseProgram(mProgram);
    checkGlError("glUseProgram");

    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, mTextureHandle);

    glUniform1i(mTextureUniformHandle, 5);
    glUniform3fv(mTextColorUniformHandle, 1, mLineColor);

    // setting up to redraw the whole waveform area.
    startPoint = 0;
    endPoint = (int)(((float) mEndLineIdx) * getWidth()/ ((float) getWidth()));

    glEnable(GL_SCISSOR_TEST);
    glScissor(getX() + startPoint, getY(), endPoint - startPoint + 1, getHeight());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // update mEndLineIdx
    mDrawIndex = mEndLineIdx + 1;

    if (mDrawIndex > getWidth() - 1) {
        mDrawIndex -= getWidth();
    }

    glBindBuffer(GL_ARRAY_BUFFER, mVB[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVB[1]);
    glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0);
    checkGlError("glDrawArrays");

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, 0);

done:
    return(retNumDraw);
}

//-----------------------------------------------------------------------------
bool WaveformTexRenderer::modeSpecificMethod()
{
    return skipFrameDraw();
}

//-----------------------------------------------------------------------------
bool WaveformTexRenderer::skipFrameDraw()
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
void WaveformTexRenderer::setParams(int numSamples, int scrollSpeed, int targetFrameRate,
                              int numLinesPerBFrame, float bFrameIntervalMs)
{
    mNumSamples = numSamples;           // number of samples per display line
    mScrollSpeed = scrollSpeed;         // display lines per second

    float drawInterval = 1.0f/((float) targetFrameRate);
    float feedInterval = bFrameIntervalMs/1000.0f;

    mScrollRate = ((float) mScrollSpeed) * drawInterval;    // lines per draw
    mFeedRate = ((float) mScrollSpeed) * feedInterval;
    mNumLines = numLinesPerBFrame;      // number of display lines per B-frame

    mNumFramesDrawPerBFrame = mFeedRate/mScrollRate;

    // TODO: verify whether parameters make sense
    ALOGD("Scroll Rate (lines / draw) : %f", mScrollRate);
    ALOGD("Feed Rate (lines / bFrame): %f", mFeedRate);
    ALOGD("numLine per BFrame from dB: %d", mNumLines);
    ALOGD("WFFrame / BFrame Draw Ratio: %f", mNumFramesDrawPerBFrame);
}

//-----------------------------------------------------------------------------
void WaveformTexRenderer::setFrame(float* dataPtr)
{
    // assuming the input data are in the range of -1 and 1.
    int startIndex = mDataIndex;
    int endIndex = mDataIndex + mNumLines - 1;

    bool wrapped = false;

    int i, j, pLocR;
    int drawThickness = mLineThickness;
    float halfDrawThickness = ((float)drawThickness)/2.0f;
    float pLoc;
    float halfNumSample = ((float)mNumSamples)/2.0f;
    float halfNumSampleX = halfNumSample - halfDrawThickness - 1.0f;
    float pLocAdjustment = (halfNumSample - 0.5f) - halfDrawThickness;

    // data conversion to a texture format
    uint8_t* framePtr = new uint8_t[mNumLines * mNumSamples];

    // memset to zero
    memset(framePtr, 0, mNumLines * mNumSamples);

    for (i = 0; i < mNumLines; i++) {
        pLoc = -dataPtr[i] * (halfNumSampleX) + pLocAdjustment;

        pLocR = (int) round(pLoc);

        for (j = 0; j < drawThickness; j++)
            framePtr[i*mNumSamples + pLocR + j] = PIX_VAL;

    }

    if (endIndex > getWidth() - 1) {
        // wrapped
        endIndex -= getWidth();
        wrapped = true;
    }
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, mTextureHandle);

    if (!wrapped) {
        // straight case
        glTexSubImage2D(GL_TEXTURE_2D,
                        0,
                        0,
                        startIndex,
                        mNumSamples,
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
                        mNumSamples,
                        num_trans,
                        GL_LUMINANCE,
                        GL_UNSIGNED_BYTE,
                        framePtr);

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

    checkGlError("feedWaveFormData");

    mDataIndex = endIndex + 1;

    if (mDataIndex > getWidth() - 1) {
        //wrapped
        mDataIndex -= getWidth();
    }

    // update num of frames need to be drawn
    mNumFramesDraw += mNumFramesDrawPerBFrame;
}
