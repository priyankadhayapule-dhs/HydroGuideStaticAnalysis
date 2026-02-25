//
// Copyright 2018 EchoNous Inc.
//

#pragma once

#include <EGL/egl.h>
#include <GLES/gl.h>
#include <android/native_window.h>
#include <RenderSurface.h>
#include <time.h>

#define FRAME_RATE_CHECK_INTERVAL 255   // unit: frames

class LiveRenderSurface : public RenderSurface
{
public: // Methods
    LiveRenderSurface();
    virtual ~LiveRenderSurface();

    LiveRenderSurface(const LiveRenderSurface& that) = delete;
    LiveRenderSurface& operator=(LiveRenderSurface const&) = delete;

public: // RenderSurface overrides
    void            addRenderer(Renderer* rendererPtr,
                                float xPct,
                                float yPct,
                                float widthPct,
                                float heightPct);
    void            clearRenderList();
    void            draw();

public: // Methods
    ThorStatus      open(ANativeWindow* windowPtr);
    void            close();
    void            setTargetDisplayFrameRate(uint8_t targetFrameRate);

private: // methods
    float           calculateReDrawFrameSkip();

private: // RendererNode struct
    struct RendererNode
    {
        Renderer*   rendererPtr;
        float       xPct;
        float       yPct;
        float       widthPct;
        float       heightPct;
    };

private: // Properties
    std::vector<RendererNode*>  mRenderList;
    ANativeWindow*              mWindowPtr;
    EGLDisplay                  mDisplay;
    EGLSurface                  mSurface;
    EGLContext                  mContext;

    GLint                       mWidth;
    GLint                       mHeight;

    timespec                    mPrevTime;
    uint32_t                    mFrameCount;
    uint32_t                    mReDrawFrameCount;
    uint32_t                    mNonZeroFrameSkipCount;
    uint8_t                     mTargetDisplayFrameRate;
    float                       mNumReDrawFrameSkip;
    float                       mTargetSleepTimeUs;
};
