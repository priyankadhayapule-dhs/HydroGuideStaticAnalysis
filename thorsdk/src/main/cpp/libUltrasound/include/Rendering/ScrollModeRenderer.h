//
// Copyright 2020 EchoNous Inc.
//

#pragma once
#include <stdint.h>
#include <ScanConverterParams.h>
#include <ScanConverterHelper.h>
#include <Renderer.h>
#include <CineBuffer.h>
#include <ScrollModeRecordParams.h>

#define PEAK_BUFFER 2
#define MEAN_BUFFER 3

class ScrollModeRenderer : public Renderer
{
public: // Construction
    ScrollModeRenderer();
    ~ScrollModeRenderer();

    ScrollModeRenderer(const ScrollModeRenderer& that) = delete;
    ScrollModeRenderer& operator=(ScrollModeRenderer const&) = delete;

public: // Renderer overrides
    ThorStatus  open();
    void        close();
    bool        prepare();
    uint32_t    draw();
    bool        modeSpecificMethod();

public: // Mode specific methods
    void        setParams(int inputWidth, int numSamples, float scrollSpeed, float baselineShift,
                          int targetFrameRate, float numLinesPerFrame, float acqFrameRate,
                          int orgSamplesPerFrame, float desiredTimeSpan, float yPhysicalScale = 2.0f,
                          float yExpansionScale = 1.0f);
    void        setFrame(uint8_t* framePtr, int numLinesInFrame);
    void        setFrameNum(int frameNum);
    void        getPhysicalToPixelMap(float *mapMat);
    int         getNumLinesInFrameFromHeader(uint8_t* headerPtr);

    // Peak Mean Max frames
    void        setPeakMeanFrame(uint8_t* framePtr, int numLinesInFrame);
    void        setPeakMeanDrawing(bool peakDrawing, bool meanDrawing);
    void        setPeakMeanLineThickness(int lineThickness);
    ThorStatus  preparePeakMeanFreezeMode(int startFrameNum, int endFrameNum, int inStartDataIdx,
                            bool jitterAdjust);

    void        setPan(float deltaX, float deltaY);

    ThorStatus  prepareFreezeMode(int inFrameNum, bool updateIndices = true);
    ThorStatus  prepareRenderer(CineBuffer* cineBuffer, int dataType);
    ThorStatus  prepareRendererOffScreen(int minFrameNum, int maxFrameNum, int dataType);

    void        setSingleFrameTexturePtr(uint8_t* texturePtr, int textureWidth);
    void        setSingleFramePeakMeanTexturePtr(uint8_t* texturePtr, int textureWidth);
    void        getRecordParams(ScrollModeRecordParams& recParams);

    int         getInputDataWidth() { return (mInputWidth); };
    float       getLinesPerFrame() { return (mNumLinesPerFrame); };
    float       getLinesPerSecond() { return (mScrollSpeed); };
    int         getDrawIndex() { return (mDrawIndex); };
    bool        getIsSingleFrameTextureMode() { return mIsSingleFrameTextureMode; };
    int         getSingleFrameDrawIndex() { return mSingleFrameDrawIndex; };
    float       getYShiftFloat() { return mYShiftFloat; };
    bool        getLastDrawnWholeScreen() { return mLastDrawnWholeScreen; };

    void        setInvert(bool invert) { mInvert = invert; };
    void        setSingleFrameTextureMode(bool singleFrame) { mIsSingleFrameTextureMode = singleFrame; };
    void        setSingleFrameDrawIndex(int drawIndex) { mSingleFrameDrawIndex = drawIndex; };
    void        updateBaselineShiftInvert(float baselineShift, bool isInvert);
    void        updateCineBufferMinMaxFrameNum();

    void        setTintAdjustment(float coeffR, float coeffG, float coeffB);    // Tint adjustment

    void        setForcedFullScreenRender(bool fsr);
    bool        getForcedFullSreeenRender() { return (mForcedFSR); };

  private: // Methods
    void        calculateBaselineShiftCoord(float* locData, short* idxData);
    bool        updateIndices();
    bool        skipFrameDraw();
    void        drawFreezeMode();
    void        updateBaselineShiftInvertBuffer();
    void        setFrameN(int frameNum);

    void        renderTraceLine(float x1, float y1, float y2);
    void        renderPeakMeanLines(int Type);

private: // Properties
    CriticalSection         mLock;

    CineBuffer*             mCineBufferPtr;
    int                     mDataType;
    int                     mTexFrameType;

    GLuint                  mProgram;
    GLuint                  mTextureHandle;
    GLint                   mTextureUniformHandle;
    GLuint                  mVB[4];

    GLuint                  mLineProgram;
    GLuint                  mColorHandle;
    GLint                   mTintHandle;

    ScanConverterHelper     mScHelper;

    int                     mInputWidth;
    float                   mYPhysicalScale;
    float                   mJitterAmt;
    float                   mJitterSum;
    float                   mDrawingJitterSum;
    float                   mNumLinesPerFrame;          // num lines per frame
    int                     mDataIndex;
    int                     mDrawIndex;
    int                     mNumSamples;
    float                   mScrollSpeed;               // draw lines per second (pre-scaled)
    float                   mScrollRate;                // lines per draw
    int                     mStartLineIdx;
    int                     mEndLineIdx;
    float                   mNumFramesDraw;             // keeping track of how many frames need to be drawn
    float                   mNumFramesDrawPerFeed;      // num frames need to be drawn per a frame acquisition
    int                     mOrgNumLinesPerFrame;
    float                   mReSampleRatio;

    bool                    mDrawingWrapped;
    bool                    mIsSingleFrameTextureMode;
    bool                    mWholeScreenRedraw;
    bool                    mOffScreenDrawing;
    bool                    mLastDrawnWholeScreen;

    int                     mMinFrameNum;
    int                     mMaxFrameNum;
    int                     mPrevFrameNum;

    int                     mDataWidth;
    int                     mMaxDrawIndex;
    int                     mSingleFrameDrawIndex;
    int                     mSingleFrameTextureWidth;

    float                   mYExpScale;                 // Y expanstion scale: typical 1, CW 2 (2x zoom).
    float                   mBaselineShiftFloat;
    float                   mYShiftFloat;
    bool                    mInvert;
    float                   mDesiredTimeSpan;
    bool                    mUpdateBaselineShiftInvertBuffer;

    // Peak Mean/Max
    point*                  mPeakPointPtr;
    point*                  mMeanPointPtr;

    bool                    mDrawPeakLines;
    bool                    mDrawMeanLines;
    bool                    mCalcPeakLines;
    int                     mPeakMeanLineThickness;

    int                     mPeakStartIndex;
    int                     mPeakEndIndex;
    bool                    mPeakWrapped;
    bool                    mPeakJitterAdjusted;

    // for Forced Full Screen Render (for S8 series)
    bool                    mForcedFSR;

    // Trace-Line drawing params
    float                   mLineColor[3];
    float                   mPeakColor[3];
    float                   mMeanColor[3];
    float                   mTintAdj[3];           // tint RGB adjustment

    static const char* const    mVertexShaderSource;
    static const char* const    mFragmentShaderSource;

    static const char *const    mLineVertexShaderSource;
    static const char *const    mLineFragmentShaderSource;
};
