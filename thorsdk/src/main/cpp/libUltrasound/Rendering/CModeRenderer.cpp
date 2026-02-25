//
// Copyright 2018 EchoNous Inc.
//

#define LOG_TAG "CModeRenderer"

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <GLES3/gl3.h>
#include <ThorUtils.h>
#include <CModeRenderer.h>

const char* const CModeRenderer::mVertexShaderSource =
#include "cmode_vertex.glsl"
;

const char* const CModeRenderer::mFragmentShaderSource =
#include "cmode_fragment_cubic4x4_bilinear2x2.glsl"
;

//-----------------------------------------------------------------------------
CModeRenderer::CModeRenderer() :
    Renderer(),
    mProgram(0),
    mBTextureHandle(0),
    mCTextureHandle(0),
    mCMapTextureHandle(0),
    mMVPMatrixHandle(-1),
    mBTextureUniformHandle(-1),
    mBWidthHandle(-1),
    mBHeightHandle(-1),
    mBThresholdHandle(-1),
    mDopplerModeHandle(-1),
    mCTextureUniformHandle(-1),
    mCWidthHandle(-1),
    mCHeightHandle(-1),
    mCThresholdHandle(-1),
    mTintHandle(-1),
    mCMapTextureUniformHandle(-1),
    mBThreshold(0),
    mCThreshold(0),
    mDopplerMode(false),
    mBFramePtr(nullptr),
    mCFramePtr(nullptr)
{
    // Tint adjustment
    mTintAdj[0] = 0.84f;
    mTintAdj[1] = 0.88f;
    mTintAdj[2] = 0.90f;

    memset(mVB, 0, sizeof(mVB));
}

//-----------------------------------------------------------------------------
CModeRenderer::~CModeRenderer()
{
    close();
}

//-----------------------------------------------------------------------------
ThorStatus CModeRenderer::open()
{
    ThorStatus      retVal = THOR_ERROR;
    GLuint          textureHandles[3];

    mProgram = createProgram(mVertexShaderSource, mFragmentShaderSource);
    if (!checkGlError("createProgram")) goto err_ret;

    glGenTextures(3, textureHandles);
    if (!checkGlError("glGenTextures")) goto err_ret;

    if (!textureHandles[0] || !textureHandles[1] || !textureHandles[2]) {
        ALOGE("GenTextures failed: returned error %d", eglGetError());
        goto err_ret;
    }

    mBTextureHandle = textureHandles[0];
    mCTextureHandle = textureHandles[1];
    mCMapTextureHandle = textureHandles[2];

    // B-Mode texture setup
    glBindTexture(GL_TEXTURE_2D, mBTextureHandle);
    if (!checkGlError("glBindTexture")) goto err_ret;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (!checkGlError("glTexParameteri")) goto err_ret;

    // Color texture setup
    glBindTexture(GL_TEXTURE_2D, mCTextureHandle);
    if (!checkGlError("glBindTextureColor")) goto err_ret;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (!checkGlError("glTexParameteriColor")) goto err_ret;

    // Color Map texture setup
    glBindTexture(GL_TEXTURE_2D, mCMapTextureHandle);
    if (!checkGlError("glBindTextureCMap")) goto err_ret;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (!checkGlError("glTexParameteriCMap")) goto err_ret;

    mMVPMatrixHandle = glGetUniformLocation(mProgram, "u_MVPMatrix");
    mBTextureUniformHandle = glGetUniformLocation(mProgram, "u_Texture");
    mBWidthHandle = glGetUniformLocation(mProgram, "u_Width");
    mBHeightHandle = glGetUniformLocation(mProgram, "u_Height");
    mBThresholdHandle = glGetUniformLocation(mProgram, "u_BThreshold");
    mDopplerModeHandle = glGetUniformLocation(mProgram, "u_DopplerMode");

    mCTextureUniformHandle = glGetUniformLocation(mProgram, "u_CTexture");
    mCWidthHandle = glGetUniformLocation(mProgram, "u_CWidth");
    mCHeightHandle = glGetUniformLocation(mProgram, "u_CHeight");
    mCMapTextureUniformHandle = glGetUniformLocation(mProgram, "u_CMapTexture");
    mCThresholdHandle = glGetUniformLocation(mProgram, "u_CThreshold");
    mTintHandle = glGetUniformLocation(mProgram, "u_tintAdj");
    if (!checkGlError("glGetUniformLocation")) goto err_ret;

    // Update Scan Converter table
    glGenBuffers(2, mVB);

    bindScTable(mVB[0], mVB[1]);
    if (!checkGlError("bindBCSCTable")) goto err_ret;

    glBindBuffer(GL_ARRAY_BUFFER, mVB[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVB[1]);
    glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(float) * 6, 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(float) * 6,
                          (const GLvoid *) (sizeof(float) * 2));
    glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(float) * 6,
                          (const GLvoid *) (sizeof(float) * 4));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    if (!checkGlError("AttribPointer2")) goto err_ret;

    mScHelper.init(getWidth(), getHeight(), mBScParams);

    // bind ColorMap
    retVal = bindColorMap();

    if (IS_THOR_ERROR(retVal))
        goto err_ret;

    retVal = THOR_OK;

err_ret:
    return(retVal);
}

//-----------------------------------------------------------------------------
ThorStatus CModeRenderer::bindColorMap()
{
    ThorStatus retVal = THOR_ERROR;

    // Setup color map
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mCMapTextureHandle);
    if (!checkGlError("BindCMapTexture"))
        goto err_ret;

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 256,
                 1,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 mVelocityMap);
    if (!checkGlError("glTexImage2D-CMap"))
        goto err_ret;

    retVal = THOR_OK;

err_ret:
    return (retVal);
}

//-----------------------------------------------------------------------------
void CModeRenderer::setColorMap(uint8_t* colorMapPtr)
{
    memcpy(mVelocityMap, colorMapPtr, sizeof(mVelocityMap));
    bindColorMap();
}

//-----------------------------------------------------------------------------
void CModeRenderer::close()
{
    // unbind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (0 != mBTextureHandle)
    {
        glDeleteTextures(1, &mBTextureHandle);
        mBTextureHandle = 0;
    }
    if (0 != mCTextureHandle)
    {
        glDeleteTextures(1, &mCTextureHandle);
        mCTextureHandle = 0;
    }
    if (0 != mCMapTextureHandle)
    {
        glDeleteTextures(1, &mCMapTextureHandle);
        mCMapTextureHandle = 0;
    }
    if(0 != mVB[0])
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
bool CModeRenderer::prepare()
{
    if (nullptr != mBFramePtr || nullptr != mCFramePtr)
    {
        glScissor(getX(), getY(), getWidth(), getHeight());
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        return(true);
    }
    else
    {
        return(false);
    }
}

//-----------------------------------------------------------------------------
uint32_t CModeRenderer::draw()
{
    glUseProgram(mProgram);
    if (!checkGlError("glUseProgram")) goto err_ret;

    // Draw B-Mode
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mBTextureHandle);
    if (!checkGlError("BindTexture")) goto err_ret;

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_LUMINANCE,
                 mBScParams.numSamples,
                 mBScParams.numRays,
                 0,
                 GL_LUMINANCE,
                 GL_UNSIGNED_BYTE,
                 mBFramePtr);
    if (!checkGlError("glTexImage2D")) goto err_ret;

    // Tell the texture uniform sampler to use this texture in the shader by
    // binding to texture unit 0.
    glUniform1i(mBTextureUniformHandle, 0);
    if (!checkGlError("glUniform1i")) goto err_ret;

    // Draw C-Mode
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mCTextureHandle);
    if (!checkGlError("BindTexture-Color")) goto err_ret;

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_LUMINANCE,
                 mCScParams.numSamples,
                 mCScParams.numRays,
                 0,
                 GL_LUMINANCE,
                 GL_UNSIGNED_BYTE,
                 mCFramePtr);
    if (!checkGlError("glTexImage2D-Color")) goto err_ret;

    // Color Texture unit 1
    glUniform1i(mCTextureUniformHandle, 1);
    // Color Map Texture unit 2
    glUniform1i(mCMapTextureUniformHandle, 2);
    if (!checkGlError("glUniform1i-Color")) goto err_ret;

    glUniformMatrix3fv(mMVPMatrixHandle, 1, false, mScHelper.getMvpMatrixPtr());
    if (!checkGlError("MVMatrix3fv")) goto err_ret;

    // width height
    glUniform1f(mBWidthHandle, (float) mBScParams.numSamples);
    glUniform1f(mBHeightHandle, (float) mBScParams.numRays);
    glUniform1f(mCWidthHandle, (float) mCScParams.numSamples);
    glUniform1f(mCHeightHandle, (float) mCScParams.numRays);
    // threshold values
    glUniform1f(mBThresholdHandle, (float) mBThreshold);
    glUniform1f(mCThresholdHandle, (float) mCThreshold);
    // tint adjustment
    glUniform3fv(mTintHandle, 1, mTintAdj);
    // CVP or CVD
    glUniform1i(mDopplerModeHandle, mDopplerMode);

    if (!checkGlError("Uniform1f")) goto err_ret;

    glBindBuffer(GL_ARRAY_BUFFER, mVB[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVB[1]);

    glDrawElements(GL_TRIANGLE_STRIP,
                   (mBScParams.numSampleMesh * 2 + 2) * (mBScParams.numRayMesh - 1),
                   GL_UNSIGNED_SHORT,
                   0);

err_ret:
    mBFramePtr = nullptr;
    mCFramePtr = nullptr;

    return(0);
}

//-----------------------------------------------------------------------------
void CModeRenderer::setParams(ScanConverterParams& bScParams,
                              ScanConverterParams& cScParams,
                              int bThreshold,
                              int velocityThreshold,
                              uint8_t* velocityMapPtr,
                              uint32_t dopplerMode)
{
    mBScParams = bScParams;
    mCScParams = cScParams;
    mBThreshold = bThreshold;
    mCThreshold = velocityThreshold;
    mDopplerMode = dopplerMode;
    memcpy(mVelocityMap, velocityMapPtr, sizeof(mVelocityMap));
}

//-----------------------------------------------------------------------------
void CModeRenderer::setFrame(uint8_t* bFramePtr, uint8_t* cFramePtr)
{
    mBFramePtr = bFramePtr;
    mCFramePtr = cFramePtr;
}

//-----------------------------------------------------------------------------
void CModeRenderer::setPanDistance(float distX, float distY)
{
    mScHelper.panRelative(distX, distY);
}

//-----------------------------------------------------------------------------
void CModeRenderer::setScaleFactor(float scaleFactor)
{
    mScHelper.scaleRelative(scaleFactor);
}

//-----------------------------------------------------------------------------
void CModeRenderer::getDisplayDepth(float &startDepth, float &endDepth)
{
    mScHelper.getDisplayDepth(startDepth, endDepth);
}

//-----------------------------------------------------------------------------
void CModeRenderer::getPhysicalToPixelMap(float* mapMat)
{
    mScHelper.getPhysicalToPixelMap(getWidth(), getHeight(), mapMat);
}

//-----------------------------------------------------------------------------
void CModeRenderer::getPhysicalToPixelMap(float imgWidth, float imgHeight, float* mapMat)
{
    float aspectRatio_Renderer, aspectRatio_Queried;

    // Aspect Ratio on the Render Surface
    mScHelper.getAspect(aspectRatio_Renderer);

    // Aspect Ratio Queried Surface
    aspectRatio_Queried = (imgHeight/imgWidth);
    mScHelper.setAspect(aspectRatio_Queried);

    // get map matrix
    mScHelper.getPhysicalToPixelMap(imgWidth, imgHeight, mapMat);

    // restore original aspect ratio
    mScHelper.setAspect(aspectRatio_Renderer);
}

//-----------------------------------------------------------------------------
void CModeRenderer::getPan(float &deltaX, float &deltaY)
{
    mScHelper.getPan(deltaX, deltaY);
}

//-----------------------------------------------------------------------------
void CModeRenderer::getScale(float &scale)
{
    mScHelper.getScale(scale);
}

//-----------------------------------------------------------------------------
void CModeRenderer::getFlip(float &flipX, float &flipY)
{
    mScHelper.getFlip(flipX, flipY);
}

//-----------------------------------------------------------------------------
void CModeRenderer::setPan(float deltaX, float deltaY)
{
    mScHelper.setPan(deltaX, deltaY);
}

//-----------------------------------------------------------------------------
void CModeRenderer::setScale(float scale)
{
    mScHelper.setScale(scale);
}

//-----------------------------------------------------------------------------
void CModeRenderer::setFlip(float flipX, float flipY)
{
    mScHelper.setFlip(flipX, flipY);
}

//-----------------------------------------------------------------------------
void CModeRenderer::setTintAdjustment(float coeffR, float coeffG, float coeffB)
{
    mTintAdj[0] = coeffR;
    mTintAdj[1] = coeffG;
    mTintAdj[2] = coeffB;
}

//-----------------------------------------------------------------------------
void CModeRenderer::getParams(ScanConverterParams &bParams,
                              ScanConverterParams &cParams,
                              int& bThreshold,
                              int& velocityThreshold,
                              uint8_t* velocityMapPtr,
                              uint32_t& cDopplerMode)
{
    bParams = mBScParams;
    cParams = mCScParams;
    bThreshold = mBThreshold;
    velocityThreshold = mCThreshold;
    cDopplerMode = mDopplerMode;
    memcpy(velocityMapPtr, mVelocityMap, sizeof(mVelocityMap));
}

//-----------------------------------------------------------------------------
void CModeRenderer::bindScTable(GLuint locBuffer, GLuint indexBuffer)
{
    float* locData = new float[mBScParams.numSampleMesh * mBScParams.numRayMesh * 6];
    short* idxData = new short[(mBScParams.numSampleMesh * 2 + 2) * (mBScParams.numRayMesh - 1)];

    float totalSampleMm = (mBScParams.numSamples - 1) * mBScParams.sampleSpacingMm;
    // totalRayWidth in Radian (for Phased array) and in mm (for Linear)
    float totalRayWidth = (mBScParams.numRays - 1) * mBScParams.raySpacing;
    float imagingDepth = (mBScParams.sampleSpacingMm * (mBScParams.numSamples - 1) +
                          mBScParams.startSampleMm);

    float xShift = 0.0f;
    float yShift = -1.0f;
    float scale = 2.0f / (mBScParams.sampleSpacingMm * (mBScParams.numSamples - 1) +
                          mBScParams.startSampleMm);

    // update scale for linear case
    if (mBScParams.probeShape == PROBE_SHAPE_LINEAR)
    {
        // checking scale with screen and imaging asepct ratios
        float screenAspect = ((float) getWidth())/((float) getHeight());
        float imagingAsepct = totalRayWidth/imagingDepth;
        if (screenAspect < imagingAsepct)
        {
            // imaging width limits screen scale
            scale = scale / imagingAsepct * screenAspect;
        }

        yShift = yShift + (2.0f - scale * imagingDepth) / 2.0f;
    }

    // update ScanConverterHelper scale and xShift
    mScHelper.setScaleXYShift(scale, xShift, yShift);

    float x, y, u, v;
    float r, theta, s, thetac;
    float du, dv;
    float dum, dvm;
    float nx, ny;

    du = 1.0f / ((float) mBScParams.numSamples);
    dv = 1.0f / ((float) mBScParams.numRays);
    dum = 1.0f / ((float) (mBScParams.numSampleMesh - 1));
    dvm = 1.0f / ((float) (mBScParams.numRayMesh - 1));

    // Color SC LUTs
    float z, w;
    float dz, dw;
    float Dr, Dt;
    float r0, t0;

    dz = mCScParams.sampleSpacingMm;
    dw = mCScParams.raySpacing;
    r0 = mCScParams.startSampleMm - dz/2.0f;
    t0 = mCScParams.startRayRadian - dw/2.0f;
    Dr = 1.0f / (((float) mCScParams.numSamples) * mCScParams.sampleSpacingMm);
    Dt = 1.0f / (((float) mCScParams.numRays) * mCScParams.raySpacing);

    if (mBScParams.probeShape == PROBE_SHAPE_LINEAR)
    {
        // origin coordinate, vectors
        float org_x, org_y;
        float z_vec_x, z_vec_y, w_vec_x;
        float alpha, beta;

        // steering angles in rad.  CCW is +
        theta = mBScParams.steeringAngleRad;
        thetac = mCScParams.steeringAngleRad;

        // update params for linear
        Dr = 1.0f / (((float) (mCScParams.numSamples - 1)) * mCScParams.sampleSpacingMm);
        Dt = 1.0f / (((float) (mCScParams.numRays - 1)) * mCScParams.raySpacing);

        // org coordinate for color box
        org_x = mCScParams.startRayMm;
        org_y = mCScParams.startSampleMm;

        // color z-axis coordinate vector
        z_vec_x = sin(thetac);
        z_vec_y = cos(thetac);

        // color w-axis coordinate vector (y always 0)
        w_vec_x = 1.0f;

        // ray direction
        for (uint32_t j = 0; j < mBScParams.numRayMesh; j++) {
            s = mBScParams.startRayMm + ((float) j) * dvm * totalRayWidth;
            v = dv / 2.0f + (mBScParams.numRayMesh - 1 - j) * dvm * ((float) (mBScParams.numRays - 1)) * dv;

            for (uint32_t i = 0; i < mBScParams.numSampleMesh; i++) {
                r = mBScParams.startSampleMm + ((float) i) * dum * totalSampleMm;
                u = du / 2.0f + i * dum * ((float) (mBScParams.numSamples - 1)) * du;

                x = s + sin(theta) * r;
                y = cos(theta) * r;

                // find the cross section
                beta = ((y - org_y) / z_vec_y);
                alpha = (((x - org_x) - (beta * z_vec_x)) / w_vec_x);

                z = (beta + dz/2.0f) * Dr;
                w = (alpha + dw/2.0f) * Dt;

                // x - direction color texture reversed (flipped).
                w = 1.0f - w;

                // normalized x and y coordinate
                nx = x * scale + xShift;
                ny = -(y * scale + yShift);

                locData[(mBScParams.numSampleMesh * j + i) * 6 + 0] = nx;
                locData[(mBScParams.numSampleMesh * j + i) * 6 + 1] = ny;
                locData[(mBScParams.numSampleMesh * j + i) * 6 + 2] = u;
                locData[(mBScParams.numSampleMesh * j + i) * 6 + 3] = v;
                locData[(mBScParams.numSampleMesh * j + i) * 6 + 4] = z;
                locData[(mBScParams.numSampleMesh * j + i) * 6 + 5] = w;
            }
        }
    }
    else
    {
        // default: phased array
        // ray direction
        for (uint32_t j = 0; j < mBScParams.numRayMesh; j++) {
            theta = mBScParams.startRayRadian + ((float) j) * dvm * totalRayWidth;
            v = dv / 2.0f + j * dvm * ((float) (mBScParams.numRays - 1)) * dv;
            w = (theta - t0) * Dt;

            for (uint32_t i = 0; i < mBScParams.numSampleMesh; i++) {
                r = mBScParams.startSampleMm + ((float) i) * dum * totalSampleMm;
                u = du / 2.0f + i * dum * ((float) (mBScParams.numSamples - 1)) * du;
                z = (r - r0) * Dr;

                x = sin(theta) * r;
                y = cos(theta) * r;

                nx = x * scale + xShift;
                ny = -(y * scale + yShift);

                locData[(mBScParams.numSampleMesh * j + i) * 6 + 0] = nx;
                locData[(mBScParams.numSampleMesh * j + i) * 6 + 1] = ny;
                locData[(mBScParams.numSampleMesh * j + i) * 6 + 2] = u;
                locData[(mBScParams.numSampleMesh * j + i) * 6 + 3] = v;
                locData[(mBScParams.numSampleMesh * j + i) * 6 + 4] = z;
                locData[(mBScParams.numSampleMesh * j + i) * 6 + 5] = w;
            }
        }
    }

    short startShift = 0;
    short startShift2 = 0;

    for (uint32_t j = 0; j < mBScParams.numRayMesh - 1; j++) {
        startShift = (short) j * mBScParams.numSampleMesh;
        startShift2 = startShift + mBScParams.numSampleMesh;
        idxData[(mBScParams.numSampleMesh * 2 + 2) * j] = startShift;

        for (uint32_t i = 0; i < mBScParams.numSampleMesh; i++) {
            idxData[(mBScParams.numSampleMesh * 2 + 2) * j + 1 + 2 * i] = i + startShift;
            idxData[(mBScParams.numSampleMesh * 2 + 2) * j + 2 + 2 * i] = i + startShift2;
        }

        idxData[(mBScParams.numSampleMesh * 2 + 2) * j + 1 + 2 * mBScParams.numSampleMesh] =
                mBScParams.numSampleMesh -1 + startShift2;
    }

    glBindBuffer(GL_ARRAY_BUFFER, locBuffer);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(float) * mBScParams.numSampleMesh * mBScParams.numRayMesh * 6,
                 locData, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer); // index
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 sizeof(short) * (mBScParams.numSampleMesh * 2 + 2) * (mBScParams.numRayMesh - 1),
                 idxData,
                 GL_STATIC_DRAW);

    delete [] locData;
    delete [] idxData;
}
