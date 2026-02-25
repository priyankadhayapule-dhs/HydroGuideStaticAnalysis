//
// Copyright 2018 EchoNous Inc.
//

#pragma once

#include <vector>
#include <ThorError.h>
#include <ThorUtils.h>
#include <Renderer.h>

class RenderSurface
{
public: // Methods
    RenderSurface();
    virtual ~RenderSurface();

    RenderSurface(const RenderSurface& that) = delete;
    RenderSurface& operator=(RenderSurface const&) = delete;

    virtual void    addRenderer(Renderer* rendererPtr,
                                float xPct,
                                float yPct,
                                float widthPct,
                                float heightPct) = 0;
    virtual void    clearRenderList() = 0;
    virtual void    draw() = 0;

protected: 
    struct RendererAttributes : public Renderer::RendererAttributes {};

protected: // Methods
    bool    checkGlError(const char* funcName);
    void    setRendererAttributes(Renderer* renderer, 
                                  RendererAttributes& attrib);
};
