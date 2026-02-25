//
// Copyright 2018 EchoNous Inc.
//

#pragma once

#include <Renderer.h>

class WaveformTexRenderer : public Renderer
{
public: // Construction
    WaveformTexRenderer();
    ~WaveformTexRenderer();

    WaveformTexRenderer(const WaveformTexRenderer& that) = delete;
    WaveformTexRenderer& operator=(WaveformTexRenderer const&) = delete;

public: // Renderer overrides
    ThorStatus  open();
    void        close();
    bool        prepare();
    uint32_t    draw();
    bool        modeSpecificMethod();

public: // Mode specific methods
    void        setParams(int numSamples, int scrollSpeed, int targetFrameRate, int numLinesPerBFrame, float bFrameIntervalMs);
    void        setFrame(float* framePtr);
    void        setLineColor(float* txtColor);
    void        setLineThickness(int lineThickness);

private: // Methods
    bool        updateIndices();
    bool        skipFrameDraw();

private: // Properties
    GLuint                  mProgram;
    GLuint                  mTextureHandle;
    GLint                   mTextureUniformHandle;
    GLint                   mTextColorUniformHandle;
    GLuint                  mVB[2];
    float                   mLineColor[3];
    int                     mLineThickness;

    int                     mNumLines;
    int                     mDataIndex;
    int                     mDrawIndex;
    int                     mScrollSpeed;
    int                     mNumSamples;
    float                   mScrollRate;
    float                   mFeedRate;
    int                     mStartLineIdx;
    int                     mEndLineIdx;
    float                   mNumFramesDraw;               // keeping track of how many frames need to be drawn
    float                   mNumFramesDrawPerBFrame;      // calculated secondary parameter, indicating num frames need to be drawn per bframe acqusition


    static const char* const    mVertexShaderSource;
    static const char* const    mFragmentShaderSource;
};
