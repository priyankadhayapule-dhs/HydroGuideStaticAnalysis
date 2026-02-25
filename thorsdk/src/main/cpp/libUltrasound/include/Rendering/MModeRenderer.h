//
// Copyright 2018 EchoNous Inc.
//

#pragma once
#include <stdint.h>
#include <ScanConverterParams.h>
#include <ScanConverterHelper.h>
#include <Renderer.h>
#include <CineBuffer.h>

class MModeRenderer : public Renderer
{
public: // Construction
    MModeRenderer();
    ~MModeRenderer();

    MModeRenderer(const MModeRenderer& that) = delete;
    MModeRenderer& operator=(MModeRenderer const&) = delete;

public: // Renderer overrides
    ThorStatus  open();
    void        close();
    bool        prepare();
    uint32_t    draw();
    bool        modeSpecificMethod();

public: // M-Mode specific methods
    void        setParams(int numSamples, float sampleSpacingMm, float startSampleMm,
                          float scrollSpeed, uint32_t targetFrameRate,
                          uint32_t numLinesPerBFrame, uint32_t bFrameIntervalMs);
    void        setFrame(uint8_t* framePtr);
    void        setFrameNum(int frameNum);
    void        getPhysicalToPixelMap(float *mapMat);

    void        setPan(float deltaX, float deltaY);

    ThorStatus  prepareLiveMode(CineBuffer* cineBuffer);
    ThorStatus  prepareFreezeMode(CineBuffer* cineBuffer);

    void        setFreezeModeFrameNum(int frameNum);
    void        setSingleFrameTexturePtr(uint8_t* texturePtr, int textureWidth);

    int         getDrawnIndex();
    int         getCurDrawnIndex();

    void        setStillMLineTime(float mLineTime);
    float       getStillMLineTime();
    void        setStillTimeShift(float timeShift);

    bool        getIsSingleFrameTextureMode() { return mIsSingleFrameTextureMode; };

    void        setTintAdjustment(float coeffR, float coeffG, float coeffB);    // Tint adjustment

  private: // Methods
    bool        updateIndices();
    bool        skipFrameDraw();
    void        drawFreezeMode();

    void        loadFreezeModeTexture(bool upDirection);
    ThorStatus  prepareSingleFrameTextureMode();

    void        renderMTraceLine(float x1, float y1, float y2);

private: // Properties
    CineBuffer*             mCineBufferPtr;

    GLuint                  mProgram;
    GLuint                  mTextureHandle;
    GLint                   mTextureUniformHandle;
    GLuint                  mVB[2];

    GLuint                  mLineProgram;
    GLuint                  mColorHandle;
    GLint                   mTintHandle;

    ScanConverterHelper     mScHelper;
    ScanConverterParams     mScParams;

    int                     mNumLines;
    int                     mDataIndex;
    int                     mDrawIndex;
    float                   mScrollSpeed;
    float                   mScrollRate;
    float                   mFeedRate;
    int                     mStartLineIdx;
    int                     mEndLineIdx;
    float                   mNumFramesDraw;               // keeping track of how many frames need to be drawn
    float                   mNumFramesDrawPerBFrame;      // calculated secondary parameter, indicating num frames need to be drawn per bframe acqusition

    bool                    mDrawingWrapped;

    int                     mMinFrameNum;
    int                     mMaxFrameNum;
    int                     mPrevFrameNum;
    int                     mDataWidth;

    int                     mMinDrawnIndex;
    int                     mMaxDrawnIndex;
    int                     mCurDrawnIndex;
    bool                    mNoScrollFreeze;

    int                     mTextureWidth;
    int                     mMinLoadedIndex;
    int                     mMaxLoadedIndex;

    bool                    mIsLive;
    bool                    mIsSingleFrameTextureMode;

    float                   mStillMLineTime;
    float                   mStillTimeShift;

    // M-Line drawing params
    float                   mLineColor[3];
    float                   mTintAdj[3];           // tint RGB adjustment

    static const char* const    mVertexShaderSource;
    static const char* const    mFragmentShaderSource;

    static const char *const    mLineVertexShaderSource;
    static const char *const    mLineFragmentShaderSource;
};
