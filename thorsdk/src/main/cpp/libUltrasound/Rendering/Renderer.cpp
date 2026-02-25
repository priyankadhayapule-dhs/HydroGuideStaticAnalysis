//
// Copyright 2018 EchoNous Inc.
//

#define LOG_TAG "Renderer"

#include <stdlib.h>
#include <string.h>
#include <GLES3/gl3.h>
#include <ThorUtils.h>
#include <Renderer.h>

Renderer::Renderer()
{
    memset(&mAttrib, 0, sizeof(mAttrib));
}

//-----------------------------------------------------------------------------
Renderer::~Renderer()
{
}

//-----------------------------------------------------------------------------
GLuint Renderer::createProgram(const char* vertexSource,
                               const char* fragmentSource)
{
    GLuint  program = 0;
    GLuint  vtxShader = 0;
    GLuint  fragShader = 0;
    GLint   linked = GL_FALSE;

    vtxShader = loadShader(GL_VERTEX_SHADER, vertexSource);
    if (!vtxShader)
    {
        ALOGE("Unable to load vertex shader");
        goto err_ret;
    }

    fragShader = loadShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (!fragShader)
    {
        ALOGE("Unable to load fragment shader");
        goto err_ret;
    }

    program = glCreateProgram();
    if (!program)
    {
        if (!checkGlError("glCreateProgram")) goto err_ret;
    }

    glAttachShader(program, vtxShader);
    glAttachShader(program, fragShader);

    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        ALOGD("Could not link program");
        GLint infoLogLen = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLen);
        if (infoLogLen)
        {
            GLchar* infoLogPtr = (GLchar*) malloc(infoLogLen);
            if (infoLogPtr)
            {
                glGetProgramInfoLog(program, infoLogLen, NULL, infoLogPtr);
                ALOGD("Could not link program:\n%s\n", infoLogPtr);
                free(infoLogPtr);
            }
        }
        glDeleteProgram(program);
        program = 0;
    }

err_ret:
    glDeleteShader(vtxShader);
    glDeleteShader(fragShader);

    return(program);
}

//-----------------------------------------------------------------------------
GLuint Renderer::loadShader(GLenum shaderType, const char* shaderSource)
{
    GLuint      shader = glCreateShader(shaderType);
    GLint       shaderCompiled = 0;

    if (!shader)
    {
        if (!checkGlError("glCreateShader")) goto err_ret;
    }

    glShaderSource(shader, 1, &shaderSource, NULL);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &shaderCompiled);
    if (!shaderCompiled)
    {
        GLint infoLogLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);
        if (infoLogLen > 0)
        {
            GLchar* infoLogPtr = (GLchar*) malloc(infoLogLen);
            if (infoLogPtr)
            {
                glGetShaderInfoLog(shader, infoLogLen, NULL, infoLogPtr);
                ALOGE("Could not compile %s shader:\n%s\n",
                      shaderType == GL_VERTEX_SHADER ? "vertex" : "fragment",
                      infoLogPtr);
                free(infoLogPtr);
            }
        }
        glDeleteShader(shader);
        shader = 0;
    }

err_ret:
    return(shader);
}

//-----------------------------------------------------------------------------
bool Renderer::checkGlError(const char* funcName)
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
void Renderer::setAttributes(RendererAttributes& attrib)
{
    mAttrib = attrib;
}

