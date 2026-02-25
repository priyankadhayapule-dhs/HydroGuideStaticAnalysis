//
// Copyright 2018 EchoNous Inc.
//

#pragma once

#include <ScanConverterParams.h>
#include <ScanConverterHelper.h>
#include <Renderer.h>

class CModeRenderer : public Renderer
{
public: // Construction
    CModeRenderer();
    ~CModeRenderer();

    CModeRenderer(const CModeRenderer& that) = delete;
    CModeRenderer& operator=(CModeRenderer const&) = delete;

public: // Renderer overrides
    ThorStatus  open();
    void        close();
    bool        prepare();
    uint32_t    draw();

public: // C-Mode specific methods
    void        setParams(ScanConverterParams& bScParams,
                          ScanConverterParams& cScParams,
                          int bThreshold,
                          int velocityThreshold,
                          uint8_t* velocityMapPtr,
                          uint32_t dopplerMode);

    void        setFrame(uint8_t* bFramePtr, uint8_t* cFramePtr);

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
    void        getPhysicalToPixelMap(float* mapMat);
    void        getPhysicalToPixelMap(float imgWidth, float imgHeight, float *mapMat);
    void        getParams(ScanConverterParams& bScParams,
                          ScanConverterParams& cScParams,
                          int& bThreshold,
                          int& velocityThreshold,
                          uint8_t* velocityMapPtr,
                          uint32_t& dopplerMode);

    int         getBThreshold() { return( mBThreshold ); };
    int         getVelThreshold() { return( mCThreshold ); };

    ThorStatus  bindColorMap();
    void        setColorMap(uint8_t* colorMapPtr);

private: // Methods
    void        bindScTable(GLuint locBuffer, GLuint indexBuffer);

private: // Properties
    ScanConverterHelper     mScHelper;
    ScanConverterParams     mBScParams;
    ScanConverterParams     mCScParams;
    GLuint                  mProgram;
    GLuint                  mBTextureHandle;
    GLuint                  mCTextureHandle;
    GLuint                  mCMapTextureHandle;
    GLint                   mMVPMatrixHandle;
    GLuint                  mDopplerModeHandle;

    GLint                   mBTextureUniformHandle;
    GLint                   mBWidthHandle;
    GLint                   mBHeightHandle;
    GLint                   mBThresholdHandle;

    GLint                   mCTextureUniformHandle;
    GLint                   mCWidthHandle;
    GLint                   mCHeightHandle;
    GLint                   mCThresholdHandle;
    GLint                   mTintHandle;           // tint handle

    GLint                   mCMapTextureUniformHandle;

    float                   mTintAdj[3];           // tint RGB adjustment

    GLuint                  mVB[2];
    int                     mBThreshold;
    int                     mCThreshold;
    uint8_t                 mVelocityMap[1024];
    uint32_t                mDopplerMode;           // CVD / CPD

    uint8_t*                mBFramePtr;
    uint8_t*                mCFramePtr;

    static const char* const    mVertexShaderSource;
    static const char* const    mFragmentShaderSource;
};
