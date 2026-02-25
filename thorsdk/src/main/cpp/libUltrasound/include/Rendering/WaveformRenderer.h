//
// Copyright 2018 EchoNous Inc.
//

#pragma once

#include <Renderer.h>
#include <CineBuffer.h>
#include <ScanConverterHelper.h>

class WaveformRenderer : public Renderer
{
public: // Construction
    WaveformRenderer();
    ~WaveformRenderer();

    WaveformRenderer(const WaveformRenderer& that) = delete;
    WaveformRenderer& operator=(WaveformRenderer const&) = delete;

public: // Renderer overrides
    ThorStatus  open();
    void        close();
    bool        prepare();
    uint32_t    draw();
    bool        modeSpecificMethod();

public: // Mode specific methods
    void        setParams(int inputWidth, float scrollSpeed, int targetFrameRate,
                          int numLinesPerBFrame, float bFrameRate, float desiredTimeSpan, int decimationRatio = 1);
    void        setFrame(float* framePtr);
    void        setLineColor(float* lineColor);
    void        setLineThickness(int lineThickness);
    ThorStatus  checkScrollParams();
    void        setSamplesPerScreen(int inputWidth, int decimationRatio = -1);   // -1 to keep the decimationRatio

    ThorStatus  prepareRenderer(CineBuffer* cineBuffer, uint32_t dataType);
    ThorStatus  prepareRendererOffScreen(int minFrameNum, int maxFrameNum, uint32_t dataType);

    void        setFrameNum(int frameNum);
    void        prepareFreezeMode(int reqFrameNum, bool updateIndices = true);
    void        prepareFreezeModeOffScreen(float* framePtr);
    void        drawFreezeMode();

    void        getPhysicalToPixelMap(float *mapMat);

    int         getInputDataWidth() { return (mInputWidth); };
    int         getSamplesPerFrame() { return (mNumLines); };
    float       getSamplesPerSecond() { return (mScrollSpeed); };
    int         getDecimRatio() { return (mDecimRatio); };

    void        setSingleFrameFreezeMode(bool sffMode) { mSingleFrameFreezeMode = sffMode; };
    void        setSingleFrameFrameWidth(int frameWidth) { mSingleFrameFrameWidth = frameWidth; };
    void        setSingleFrameTraceIndex(int traceIndex) { mSingleFrameTraceIndex = traceIndex; };

    int         getSingleFrameFrameWidth() { return ( mSingleFrameFrameWidth ); };
    int         getSingleFrameTraceIndex() { return ( mSingleFrameTraceIndex ); };

private: // Methods
    bool        updateIndices();
    bool        skipFrameDraw();

    void        renderTraceLine(float x1, float y1, float y2);

private: // Properties
    CineBuffer*             mCineBufferPtr;
    uint32_t                mDataType;

    GLuint                  mProgram;
    GLint                   mDrawColorUniformHandle;
    GLuint                  mVB[2];
    float                   mLineColor[3];
    float                   mTraceLineColor[3];
    int                     mLineThickness;

    ScanConverterHelper     mScHelper;

    int                     mNumLines;
    int                     mDataIndex;
    int                     mDrawIndex;
    int                     mInputWidth;
    float                   mScrollSpeed;
    float                   mScrollRate;
    float                   mFeedRate;
    int                     mStartLineIdx;
    int                     mEndLineIdx;
    float                   mNumFramesDraw;               // keeping track of how many frames need to be drawn
    float                   mNumFramesDrawPerBFrame;      // calculated secondary parameter, indicating num frames need to be drawn per bframe acquisition

    int                     mPrevFrameNum;

    int                     mMinFrameNum;
    int                     mMaxFrameNum;

    bool                    mDrawingWrapped;
    bool                    mWholeScreenRedraw;
    bool                    mSingleFrameFreezeMode;
    bool                    mOffScreenDrawing;

    int                     mSingleFrameFrameWidth;
    int                     mSingleFrameTraceIndex;

    int                     mDecimRatio;
    int                     mDecimInputWidth;
    int                     mMaxDrawIndex;

    float                   mJitterAmt;
    float                   mJitterSum;
    float                   mDrawingJitterSum;

    static const char* const    mVertexShaderSource;
    static const char* const    mFragmentShaderSource;
};
