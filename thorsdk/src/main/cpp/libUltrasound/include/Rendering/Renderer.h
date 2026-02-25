//
// Copyright 2018 EchoNous Inc.
//

#pragma once

#include <EGL/egl.h>
#include <GLES/gl.h>
#include <ThorError.h>

struct point {
    GLfloat x;
    GLfloat y;
};

class Renderer
{
public: // Construction
    Renderer();
    virtual ~Renderer();

    Renderer(const Renderer& that) = delete;
    Renderer& operator=(Renderer const&) = delete;

public: // Virtual methods that all derivations must implement
    virtual ThorStatus  open() = 0;
    virtual void        close() = 0;
    virtual bool        prepare() = 0;  // If false then draw() will not be called
    virtual uint32_t    draw() = 0;     // Return 0 if drawing complete,
                                        // otherwise draw() will be called again
                                        // in number of redraws.
    // mode specific special methods
    virtual bool        modeSpecificMethod() { return true;}

public: // Property accessors
    float       getXPct() { return(mAttrib.xPct); }
    float       getYPct() { return(mAttrib.yPct); }
    float       getWidthPct() { return(mAttrib.widthPct); }
    float       getHeightPct() { return(mAttrib.heightPct); }

    GLint       getX() { return(mAttrib.x); }
    GLint       getY() { return(mAttrib.y); }
    GLsizei     getWidth() { return(mAttrib.width); }
    GLsizei     getHeight() { return(mAttrib.height); }

protected: // Methods
    GLuint      createProgram(const char* vertexSource, const char* fragmentSource);
    GLuint      loadShader(GLenum shaderType, const char* shaderSource);
    bool        checkGlError(const char* funcName);

private: // Attributes struct
    struct RendererAttributes
    {
        float       xPct;
        float       yPct;
        float       widthPct;
        float       heightPct;
        GLint       x;
        GLint       y;
        GLsizei     width;
        GLsizei     height;
    };

private: // Methods
    void        setAttributes(RendererAttributes& attrib);

private: // Properties
    RendererAttributes      mAttrib;

    friend class RenderSurface;
};
