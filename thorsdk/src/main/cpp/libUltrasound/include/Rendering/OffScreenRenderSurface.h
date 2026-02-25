//
// Copyright 2018 EchoNous Inc.
//

#pragma once

#include <EGL/egl.h>
#include <GLES/gl.h>
#include <android/native_window.h>
#include <RenderSurface.h>
#include <time.h>

class OffScreenRenderSurface : public RenderSurface
{
public: // Methods
    OffScreenRenderSurface();
    virtual ~OffScreenRenderSurface();

    OffScreenRenderSurface(const OffScreenRenderSurface &that) = delete;
    OffScreenRenderSurface& operator=(OffScreenRenderSurface const&) = delete;

public: // RenderSurface overrides
    void            addRenderer(Renderer* rendererPtr,
                                float xPct,
                                float yPct,
                                float widthPct,
                                float heightPct);
    void            clearRenderList();
    void            draw();

public: // Methods
    ThorStatus      open(int width, int height);
    ThorStatus      open(ANativeWindow *inputSurface);
    void            close();
    void            takeScreenShotToRawFile(const char* filename);
    void            takeScreenShotToJpegFile(const char* filename);
    void            takeScreenShotToRGBAPtr(uint8_t *outImg);
    void            takeScreenShotToJpegFileUD(const char* filename);
    void            takeScreenShotToRGBAPtrUD(uint8_t *outImg);
    void            takeScreenShotToRGBPtrUD(uint8_t *outImg);
    void            takeScreenShotToGrayPtrUD(uint8_t *outImg);
    bool            isOpen() {return mOpen;}
private : // Methods
    void            encodeJpegImage(const char *filename, uint8_t *inputImg, int imgWidth, int imgHeight, int imgQuality);

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
    EGLDisplay                  mDisplay;
    EGLSurface                  mSurface;
    EGLContext                  mContext;

    GLint                       mWidth;
    GLint                       mHeight;
    bool                        mOpen;
};
