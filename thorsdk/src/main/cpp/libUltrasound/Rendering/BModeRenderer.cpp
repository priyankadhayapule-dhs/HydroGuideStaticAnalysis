//
// Copyright 2018 EchoNous Inc.
//

#define LOG_TAG "BModeRenderer"

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <GLES3/gl3.h>
#include <ThorUtils.h>
#include <BModeRenderer.h>

const char* const BModeRenderer::mVertexShaderSource =
#include "bmode_vertex.glsl"
;

const char* const BModeRenderer::mFragmentShaderSource =
#include "bmode_fragment_cubic4x4.glsl"
;

//-----------------------------------------------------------------------------
BModeRenderer::BModeRenderer() :
    Renderer(),
    mProgram(0),
    mTextureHandle(0),
    mMVPMatrixHandle(-1),
    mTextureUniformHandle(-1),
    mWidthHandle(-1),
    mHeightHandle(-1),
    mTintHandle(-1),
    mFramePtr(nullptr)
{
    // Tint adjustment
    mTintAdj[0] = 0.84f;
    mTintAdj[1] = 0.88f;
    mTintAdj[2] = 0.90f;

    memset(mVB, 0, sizeof(mVB));
}

//-----------------------------------------------------------------------------
BModeRenderer::~BModeRenderer()
{
    close();
}

//-----------------------------------------------------------------------------
ThorStatus BModeRenderer::open()
{
    ThorStatus      retVal = THOR_ERROR;

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

    mMVPMatrixHandle = glGetUniformLocation(mProgram, "u_MVPMatrix");
    mTextureUniformHandle = glGetUniformLocation(mProgram, "u_Texture");
    mWidthHandle = glGetUniformLocation(mProgram, "u_Width");
    mHeightHandle = glGetUniformLocation(mProgram, "u_Height");
    mTintHandle = glGetUniformLocation(mProgram, "u_tintAdj");
    if (!checkGlError("glGetUniformLocation")) goto err_ret;

    // Update Scan Converter table
    glGenBuffers(2, mVB);

    bindScTable(mVB[0], mVB[1]);
    if (!checkGlError("bindSCTable")) goto err_ret;

    glBindBuffer(GL_ARRAY_BUFFER, mVB[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVB[1]);
    glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(float) * 4, 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(float) * 4,
                          (const GLvoid *) (sizeof(float) * 2));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    if (!checkGlError("AttribPointer2")) goto err_ret;

    mScHelper.init(getWidth(), getHeight(), mScParams);

    retVal = THOR_OK;

err_ret:
    return(retVal);
}

//-----------------------------------------------------------------------------
void BModeRenderer::close()
{
    // unbind texture
    glActiveTexture(GL_TEXTURE0);
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
bool BModeRenderer::prepare()
{
    if (nullptr != mFramePtr)
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
uint32_t BModeRenderer::draw()
{
    glUseProgram(mProgram);
    if (!checkGlError("glUseProgram")) goto err_ret;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTextureHandle);
    if (!checkGlError("BindTexture")) goto err_ret;

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_LUMINANCE,
                 mScParams.numSamples,
                 mScParams.numRays,
                 0,
                 GL_LUMINANCE,
                 GL_UNSIGNED_BYTE,
                 mFramePtr);
    if (!checkGlError("glTexImage2D")) goto err_ret;

    // Tell the texture uniform sampler to use this texture in the shader by
    // binding to texture unit 0.
    glUniform1i(mTextureUniformHandle, 0);
    if (!checkGlError("glUniform1i")) goto err_ret;

    glUniformMatrix3fv(mMVPMatrixHandle, 1, false, mScHelper.getMvpMatrixPtr());
    if (!checkGlError("MVMatrix3fv")) goto err_ret;

    // width height
    glUniform1f(mWidthHandle, (float) mScParams.numSamples);
    glUniform1f(mHeightHandle, (float) mScParams.numRays);
    // tint adjustment
    glUniform3fv(mTintHandle, 1, mTintAdj);
    if (!checkGlError("glUniform1f/3fv")) goto err_ret;

    glBindBuffer(GL_ARRAY_BUFFER, mVB[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVB[1]);

    glDrawElements(GL_TRIANGLE_STRIP,
                   (mScParams.numSampleMesh * 2 + 2) * (mScParams.numRayMesh - 1),
                   GL_UNSIGNED_SHORT,
                   0);
    if (!checkGlError("DrawArrays")) goto err_ret;

err_ret:
    mFramePtr = nullptr;

    return(0);
}

//-----------------------------------------------------------------------------
void BModeRenderer::setParams(ScanConverterParams& params)
{
    mScParams = params;
}

//-----------------------------------------------------------------------------
void BModeRenderer::setFrame(uint8_t* framePtr)
{
    mFramePtr = framePtr;
}

//-----------------------------------------------------------------------------
void BModeRenderer::setPanDistance(float distX, float distY)
{
    mScHelper.panRelative(distX, distY);
}

//-----------------------------------------------------------------------------
void BModeRenderer::setScaleFactor(float scaleFactor)
{
    mScHelper.scaleRelative(scaleFactor);
}

//-----------------------------------------------------------------------------
void BModeRenderer::getDisplayDepth(float &startDepth, float &endDepth)
{
    mScHelper.getDisplayDepth(startDepth, endDepth);
}

//-----------------------------------------------------------------------------
void BModeRenderer::getPhysicalToPixelMap(float* mapMat)
{
    mScHelper.getPhysicalToPixelMap(getWidth(), getHeight(), mapMat);
}

//-----------------------------------------------------------------------------
void BModeRenderer::getPhysicalToPixelMap(float imgWidth, float imgHeight, float* mapMat)
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
void BModeRenderer::getPan(float& deltaX, float& deltaY)
{
    mScHelper.getPan(deltaX, deltaY);
}

//-----------------------------------------------------------------------------
void BModeRenderer::getScale(float& scale)
{
    mScHelper.getScale(scale);
}

//-----------------------------------------------------------------------------
void BModeRenderer::getFlip(float& flipX, float& flipY)
{
    mScHelper.getFlip(flipX, flipY);
}

//-----------------------------------------------------------------------------
void BModeRenderer::setPan(float deltaX, float deltaY)
{
    mScHelper.setPan(deltaX, deltaY);
}

//-----------------------------------------------------------------------------
void BModeRenderer::setScale(float scale)
{
    mScHelper.setScale(scale);
}

//-----------------------------------------------------------------------------
void BModeRenderer::setFlip(float flipX, float flipY)
{
    mScHelper.setFlip(flipX, flipY);
}

//-----------------------------------------------------------------------------
void BModeRenderer::setTintAdjustment(float coeffR, float coeffG, float coeffB)
{
    mTintAdj[0] = coeffR;
    mTintAdj[1] = coeffG;
    mTintAdj[2] = coeffB;
}

//-----------------------------------------------------------------------------
void BModeRenderer::getParams(ScanConverterParams &params)
{
    params = mScParams;
}

//-----------------------------------------------------------------------------
void BModeRenderer::bindScTable(GLuint locBuffer, GLuint indexBuffer)
{
    float* locData = new float[mScParams.numSampleMesh * mScParams.numRayMesh * 4];
    short* idxData = new short[(mScParams.numSampleMesh * 2 + 2) * (mScParams.numRayMesh - 1)];

    float totalSampleMm = (mScParams.numSamples - 1) * mScParams.sampleSpacingMm;
    // totalRayWidth in Radian (for Phased array) and in mm (for Linear)
    float totalRayWidth = (mScParams.numRays - 1) * mScParams.raySpacing;
    float imagingDepth = (mScParams.sampleSpacingMm * (mScParams.numSamples - 1) +
                          mScParams.startSampleMm);

    float xShift = 0.0f;
    float yShift = -1.0f;
    float scale = 2.0f / imagingDepth;

    // update scale for linear case
    if (mScParams.probeShape == PROBE_SHAPE_LINEAR)
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
    float r, theta, s;
    float du, dv, dum, dvm;
    float nx, ny;

    du = 1.0f / ((float) mScParams.numSamples);
    dv = 1.0f / ((float) mScParams.numRays);
    dum = 1.0f / ((float) (mScParams.numSampleMesh - 1));
    dvm = 1.0f / ((float) (mScParams.numRayMesh - 1));

    if (mScParams.probeShape == PROBE_SHAPE_LINEAR)
    {
        // Linear
        // steering angle radian (change polarity as cw is +)
        theta = -mScParams.steeringAngleRad;
        // ray direction
        for (uint32_t j = 0; j < mScParams.numRayMesh; j++) {
            // x - direction shift
            s = mScParams.startRayMm + ((float) j) * dvm * totalRayWidth;
            // mapping of the elements is flipped for Linear.
            v = dv/2.0f + (mScParams.numRayMesh - 1 - j) * dvm * ((float) (mScParams.numRays - 1)) * dv;

            for (uint32_t i = 0; i < mScParams.numSampleMesh; i++) {
                r = mScParams.startSampleMm + ((float) i) * dum * totalSampleMm;
                u = du/2.0f + i * dum * ((float) (mScParams.numSamples - 1)) * du;

                x = s + sin(theta) * r;
                y = cos(theta) * r;

                nx = x * scale + xShift;
                ny = -(y * scale + yShift);

                locData[(mScParams.numSampleMesh * j + i) * 4 + 0] = nx;
                locData[(mScParams.numSampleMesh * j + i) * 4 + 1] = ny;
                locData[(mScParams.numSampleMesh * j + i) * 4 + 2] = u;
                locData[(mScParams.numSampleMesh * j + i) * 4 + 3] = v;
            }
        }
    }
    else if (mScParams.probeShape == PROBE_SHAPE_PHASED_ARRAY_FLATTOP)
    {
        // phased array - flattop
        // offset -> from flattop to virtual apex
        float offset = (17.984f / 2.0f) / ( tan(PI / 180.0f * 117.6f / 2.0f) );
        float rlong;
        float r1;
        float totalSampleMmCur;
        float maxR;

        // rlong
        rlong = totalSampleMm + offset;

        // ray direction
        for (uint32_t j = 0; j < mScParams.numRayMesh; j++) {
            theta = mScParams.startRayRadian + ((float) j) * dvm * totalRayWidth;
            v = dv/2.0f + j * dvm * ((float) (mScParams.numRays - 1)) * dv;

            // params for axial/sample direction
            r1 = offset / cos(theta);
            totalSampleMmCur = rlong - r1;
            maxR = mScParams.startSampleMm + totalSampleMmCur;

            // sample direction
            for (uint32_t i = 0; i < mScParams.numSampleMesh; i++) {
                r = mScParams.startSampleMm + ((float) i) * dum * totalSampleMm;
                if (r > maxR)
                    r = maxR;

                u = du/2.0f + i * dum * ((float) (mScParams.numSamples - 1)) * du;

                x = sin(theta) * r + offset * tan(theta);
                y = cos(theta) * r;

                nx = x * scale + xShift;
                ny = -(y * scale + yShift);

                locData[(mScParams.numSampleMesh * j + i) * 4 + 0] = nx;
                locData[(mScParams.numSampleMesh * j + i) * 4 + 1] = ny;
                locData[(mScParams.numSampleMesh * j + i) * 4 + 2] = u;
                locData[(mScParams.numSampleMesh * j + i) * 4 + 3] = v;
            }
        }
    }
    else
    {
        // default: phased array
        // ray direction
        for (uint32_t j = 0; j < mScParams.numRayMesh; j++) {
            theta = mScParams.startRayRadian + ((float) j) * dvm * totalRayWidth;
            v = dv/2.0f + j * dvm * ((float) (mScParams.numRays - 1)) * dv;

            for (uint32_t i = 0; i < mScParams.numSampleMesh; i++) {
                r = mScParams.startSampleMm + ((float) i) * dum * totalSampleMm;
                u = du/2.0f + i * dum * ((float) (mScParams.numSamples - 1)) * du;

                x = sin(theta) * r;
                y = cos(theta) * r;

                nx = x * scale + xShift;
                ny = -(y * scale + yShift);

                locData[(mScParams.numSampleMesh * j + i) * 4 + 0] = nx;
                locData[(mScParams.numSampleMesh * j + i) * 4 + 1] = ny;
                locData[(mScParams.numSampleMesh * j + i) * 4 + 2] = u;
                locData[(mScParams.numSampleMesh * j + i) * 4 + 3] = v;
            }
        }
    }

    short startShift = 0;
    short startShift2 = 0;

    for (uint32_t j = 0; j < mScParams.numRayMesh - 1; j++) {
        startShift = (short) j * mScParams.numSampleMesh;
        startShift2 = startShift + mScParams.numSampleMesh;
        idxData[(mScParams.numSampleMesh * 2 + 2) * j] = startShift;

        for (uint32_t i = 0; i < mScParams.numSampleMesh; i++) {
            idxData[(mScParams.numSampleMesh * 2 + 2) * j + 1 + 2 * i] = i + startShift;
            idxData[(mScParams.numSampleMesh * 2 + 2) * j + 2 + 2 * i] = i + startShift2;
        }

        idxData[(mScParams.numSampleMesh * 2 + 2) * j + 1 + 2 * mScParams.numSampleMesh] =
            mScParams.numSampleMesh -1 + startShift2;
    }

    glBindBuffer(GL_ARRAY_BUFFER, locBuffer);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(float) * mScParams.numSampleMesh * mScParams.numRayMesh * 4,
                 locData, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer); // index
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 sizeof(short) * (mScParams.numSampleMesh * 2 + 2) * (mScParams.numRayMesh - 1),
                 idxData,
                 GL_STATIC_DRAW);

    delete [] locData;
    delete [] idxData;
}
