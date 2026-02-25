//
// Copyright 2018 EchoNous Inc.
//

#define LOG_TAG "RenderSurface"

#include <ThorUtils.h>
#include <RenderSurface.h>

//-----------------------------------------------------------------------------
RenderSurface::RenderSurface()
{
}

//-----------------------------------------------------------------------------
RenderSurface::~RenderSurface()
{
}

//-----------------------------------------------------------------------------
bool RenderSurface::checkGlError(const char* funcName)
{
    GLint glError = glGetError();

    if (GL_NO_ERROR != glError)
    {
        ALOGE("GL Error after %s(): 0x%08x", funcName, glError);
        return(false);
    }

    return(true);
}

//-----------------------------------------------------------------------------
void RenderSurface::setRendererAttributes(Renderer* renderer,
                                          RendererAttributes& attrib)
{
    renderer->setAttributes(attrib);
}

