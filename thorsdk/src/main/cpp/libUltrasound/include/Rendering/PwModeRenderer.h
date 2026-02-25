//
// Copyright 2018 EchoNous Inc.
//

#pragma once
#include <stdint.h>
#include <ScanConverterParams.h>
#include <ScanConverterHelper.h>
#include <Renderer.h>
#include <CineBuffer.h>
#include <ScrollModeRecordParams.h>

class PwModeRenderer : public Renderer
{
public: // Construction
    PwModeRenderer();
    ~PwModeRenderer();

    PwModeRenderer(const PwModeRenderer& that) = delete;
    PwModeRenderer& operator=(PwModeRenderer const&) = delete;

public: // Renderer overrides
    ThorStatus  open();
    void        close();
    bool        prepare();
    uint32_t    draw();
    bool        modeSpecificMethod();

public: // PW-Mode specific methods
    void        setParams(int inputWidth, int fftSize, float scrollSpeed, float baselineShift,
                          int targetFrameRate, float numLinesPerFrame, float acqFrameRate,
                          int pwSamplesPerFrame, float desiredTimeSpan, float physicalScale);
    void        setFrame(int frameNum);
    void        setFrameNum(int frameNum);
    void        getPhysicalToPixelMap(float *mapMat);

    void        setPan(float deltaX, float deltaY);

    ThorStatus  prepareFreezeMode(int inFrameNum);
    ThorStatus  prepareRenderer(CineBuffer* cineBuffer);

    void        setSingleFrameTexturePtr(uint8_t* texturePtr, int textureWidth);
    void        getPwRecordParams(ScrollModeRecordParams& recParams);

    int         getInputDataWidth() { return (mInputWidth); };
    float       getLinesPerFrame() { return (mNumLines); };
    float       getLinesPerSecond() { return (mScrollSpeed); };
    bool        getIsSingleFrameTextureMode() { return mIsSingleFrameTextureMode; };

    void        setStillPwLineTime(float mLineTime);
    float       getStillPwLineTime();
    void        setStillTimeShift(float timeShift);
    void        setInvert(bool invert) { mInvert = invert; };
    void        setSingleFrameTextureMode(bool singleFrame) { mIsSingleFrameTextureMode = singleFrame; };
    void        setSingleFrameDrawIndex(int drawIndex) { mSingleFrameDrawIndex = drawIndex; };
    void        updateBaselineShift(float baselineShift);


  private: // Methods
    void        calculateBaselineShiftCoord(float* locData, short* idxData);
    bool        updateIndices();
    bool        skipFrameDraw();
    void        drawFreezeMode();

    void        renderTraceLine(float x1, float y1, float y2);

private: // Properties
    CineBuffer*             mCineBufferPtr;

    GLuint                  mProgram;
    GLuint                  mTextureHandle;
    GLint                   mTextureUniformHandle;
    GLuint                  mVB[2];

    GLuint                  mLineProgram;
    GLuint                  mColorHandle;

    ScanConverterHelper     mScHelper;

    int                     mInputWidth;
    float                   mJitterAmt;
    float                   mJitterSum;
    float                   mDrawingJitterSum;
    float                   mNumLines;                  // num lines per frame
    int                     mDataIndex;
    int                     mDrawIndex;
    int                     mFFTSize;
    float                   mScrollSpeed;               // draw lines per second (pre-scaled)
    float                   mScrollRate;                // lines per draw
    int                     mStartLineIdx;
    int                     mEndLineIdx;
    float                   mNumFramesDraw;             // keeping track of how many frames need to be drawn
    float                   mNumFramesDrawPerFeed;      // num frames need to be drawn per a pw frame acqusition
    int                     mPwSamplesPerFrame;
    float                   mPwSampleStep;

    bool                    mDrawingWrapped;
    bool                    mIsSingleFrameTextureMode;
    bool                    mWholeScreenRedraw;

    int                     mMinFrameNum;
    int                     mMaxFrameNum;
    int                     mPrevFrameNum;

    int                     mDataWidth;
    int                     mMaxDrawIndex;
    int                     mSingleFrameDrawIndex;

    float                   mBaselineShiftFloat;
    bool                    mInvert;
    float                   mDesiredTimeSpan;

    float                   mYPhysicalScale;

    // Trace-Line drawing params
    float                   mLineColor[3];

    static const char* const    mVertexShaderSource;
    static const char* const    mFragmentShaderSource;

    static const char *const    mLineVertexShaderSource;
    static const char *const    mLineFragmentShaderSource;
};
