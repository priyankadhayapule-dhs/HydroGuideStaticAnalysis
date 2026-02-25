//
// Copyright 2018 EchoNous Inc.
//

#pragma once

#include <ScanConverterParams.h>
#include <ScanConverterHelper.h>
#include <Renderer.h>
#include <ThorConstants.h>

class BModeRenderer : public Renderer
{
public: // Construction
    BModeRenderer();
    ~BModeRenderer();

    BModeRenderer(const BModeRenderer& that) = delete;
    BModeRenderer& operator=(BModeRenderer const&) = delete;

public: // Renderer overrides
    ThorStatus  open();
    void        close();
    bool        prepare();
    uint32_t    draw();

public: // B-Mode specific methods
    void        setParams(ScanConverterParams& params);
    void        setFrame(uint8_t* framePtr);

    void        setPanDistance(float distX, float distY);
    void        setScaleFactor(float scaleFactor);
    void        setPan(float deltaX, float deltaY);
    void        setScale(float scale);
    void        setFlip(float flipX, float flipY);
    void        setTintAdjustment(float coeffR, float coeffG, float coeffB);

    void        getPan(float &deltaX, float &deltaY);
    void        getScale(float &scale);
    void        getFlip(float &flipX, float &flipY);
    void        getDisplayDepth(float &startDepth, float &endDepth);
    void        getPhysicalToPixelMap(float *mapMat);
    void        getPhysicalToPixelMap(float imgWidth, float imgHeight, float *mapMat);
    void        getParams(ScanConverterParams &params);

private : // Methods
    void        bindScTable(GLuint locBuffer, GLuint indexBuffer);

  private: // Properties
    ScanConverterHelper     mScHelper;
    ScanConverterParams     mScParams;
    GLuint                  mProgram;
    GLuint                  mTextureHandle;
    GLint                   mMVPMatrixHandle;
    GLint                   mTextureUniformHandle;
    GLint                   mWidthHandle;
    GLint                   mHeightHandle;
    GLint                   mTintHandle;
    GLuint                  mVB[2];
    float                   mTintAdj[3];           // tint RGB adjustment

    uint8_t*                mFramePtr;

    static const char* const    mVertexShaderSource;
    static const char* const    mFragmentShaderSource;
};
