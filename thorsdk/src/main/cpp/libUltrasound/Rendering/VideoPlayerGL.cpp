//
// Copyright 2022 EchoNous Inc.
//
#define LOG_TAG "VideoPlayerGL"
#include <VideoPlayerGL.h>
#include <Media/Video.h>
#include <ThorUtils.h>
#include <GLES2/gl2.h>
#include <GLES3/gl3.h>
#include <vector>

static const char *VIDEO_PLAYER_VERT_SRC = R"(#version 300 es
precision highp float;
layout(location=0) in vec2 aPos;
layout(location=1) in vec2 aTexCoord;
out vec2 TexCoord;
uniform mat4 uTransform;
void main() {
    gl_Position = uTransform * vec4(aPos, 0.0, 1.0);
    TexCoord = vec2(aTexCoord.x, 1.0-aTexCoord.y);
}
)";
static const char *VIDEO_PLAYER_FRAG_SRC = R"(#version 300 es
precision highp float;
in vec2 TexCoord;
layout(location=0) out vec4 FragColor;
uniform sampler2D YChannel;
uniform sampler2D UVChannel;
vec3 yuv2rgb(float y, float u, float v) {
    float r = y + 1.370705 * (u-0.5);
    float g = y - 0.698001 * (u-0.5) - 0.337633 * (v-0.5);
    float b = y + 1.732446 * (v-0.5);
    return vec3(r,g,b);
}
void main() {
    float y = texture(YChannel, TexCoord).x;
    vec2 vu = texture(UVChannel, TexCoord).xy;
    FragColor = vec4(yuv2rgb(y, vu.y, vu.x), 1.0);
}
)";

static bool CompileShader(GLenum type, const char* src, GLuint* pshader) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        GLint logLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<GLchar> log(logLength+1);
        glGetShaderInfoLog(shader, log.size(), nullptr, log.data());
        LOGE("Failed to compile shader, error: %s", log.data());
        return false;
    }
    if (pshader)
        *pshader = shader;
    return true;
}
static void glerror(int line) {
    GLenum error = glGetError();
    while (error != GL_NO_ERROR) {
        LOGE("GL Error 0x%x on line %d", error, line);
        error = glGetError();
    }
}
#define GLERROR() glerror(__LINE__)

VideoPlayerGL::VideoPlayerGL() :
        mProgram(0),
        mYTexture(0),
        mUVTexture(0),
        mVertexArray(0),
        mVertexBuffer(0),
        mIndexBuffer(0),
        mVideo(nullptr),
        mTransformLocation(-1),
        mYTextureLocation(-1),
        mUVTextureLocation(-1),
        mScreenWidth(1),
        mScreenHeight(1),
        mLocX(0),
        mLocY(0),
        mLocW(1),
        mLocH(1) {}
VideoPlayerGL::~VideoPlayerGL() {
    uninit();
}

bool VideoPlayerGL::init() {
    GLERROR();

    GLuint vshader, fshader;
    if (!CompileShader(GL_VERTEX_SHADER, VIDEO_PLAYER_VERT_SRC, &vshader))
        return false;
    if (!CompileShader(GL_FRAGMENT_SHADER, VIDEO_PLAYER_FRAG_SRC, &fshader))
        return false;
    GLERROR();

    mProgram = glCreateProgram();
    glAttachShader(mProgram, vshader);
    glAttachShader(mProgram, fshader);
    glLinkProgram(mProgram);
    glDeleteShader(vshader);
    glDeleteShader(fshader);
    GLERROR();

    GLint status;
    glGetProgramiv(mProgram, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        GLint logLength;
        glGetProgramiv(mProgram, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<GLchar> log(logLength+1);
        glGetProgramInfoLog(mProgram, log.size(), nullptr, log.data());
        LOGE("Failed to link program, error: %s", log.data());
        glDeleteProgram(mProgram);
        mProgram = 0;
        return false;
    }
    GLERROR();

    // Get uniform locations, and set texture units
    mTransformLocation = glGetUniformLocation(mProgram, "uTransform");
    mYTextureLocation = glGetUniformLocation(mProgram, "YChannel");
    mUVTextureLocation = glGetUniformLocation(mProgram, "UVChannel");
    glUseProgram(mProgram);
    glUniform1i(mYTextureLocation, 0);
    glUniform1i(mUVTextureLocation, 1);
    setTransform();
    LOGD("Transform location = %d, YChannel location = %d, UVChannel location = %d",
         mTransformLocation, mYTextureLocation, mUVTextureLocation);
    GLERROR();

    // Create texture names
    GLuint texs[2];
    glGenTextures(2, texs);
    mYTexture = texs[0];
    mUVTexture = texs[1];
    for (GLuint tex : texs) {
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,  GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    GLERROR();

    // Create buffers and vertex array
    GLuint bufs[2];
    glGenBuffers(2, bufs);
    mVertexBuffer = bufs[0];
    mIndexBuffer = bufs[1];
    glGenVertexArrays(1, &mVertexArray);
    GLERROR();

    glBindVertexArray(mVertexArray); GLERROR();
    const GLfloat verts[] = {
            -1.0f, -1.0f, 0.0f, 0.0f,
            -1.0f,  1.0f, 0.0f, 1.0f,
            1.0f, -1.0f, 1.0f, 0.0f,
            1.0f,  1.0f, 1.0f, 1.0f
    };
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer); GLERROR();
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); GLERROR();
    glVertexAttribPointer(0, 2, GL_FLOAT, false, 4*sizeof(GLfloat), nullptr); GLERROR();
    glVertexAttribPointer(1, 2, GL_FLOAT, false, 4*sizeof(GLfloat), reinterpret_cast<const void*>(2*sizeof(float))); GLERROR();
    glEnableVertexAttribArray(0); GLERROR();
    glEnableVertexAttribArray(1); GLERROR();

    const GLubyte indices[] = {
            0, 2, 1,
            1, 2, 3
    };
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer); GLERROR();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); GLERROR();
    glBindVertexArray(0); GLERROR();

    LOGD("Compiled video vert and frag shaders");
    return true;
}
void VideoPlayerGL::uninit() {
    close();
    if (mProgram) {
        glDeleteProgram(mProgram);
        mProgram = 0;
    }
    if (mYTexture) {
        glDeleteTextures(1, &mYTexture);
        mYTexture = 0;
    }
    if (mUVTexture) {
        glDeleteTextures(1, &mUVTexture);
        mUVTexture = 0;
    }
    if (mVertexArray) {
        glDeleteVertexArrays(1, &mVertexArray);
        mVertexArray = 0;
    }
    if (mVertexBuffer) {
        glDeleteBuffers(1, &mVertexBuffer);
        mVertexBuffer = 0;
    }
    if (mIndexBuffer) {
        glDeleteBuffers(1, &mIndexBuffer);
        mIndexBuffer = 0;
    }
}
void VideoPlayerGL::setScreenSize(int width, int height) {
    LOGD("Setting screen size to %d, %d", width, height);
    mScreenWidth = width;
    mScreenHeight = height;

    setTransform();
}
void VideoPlayerGL::setLocation(int x, int y, int w, int h) {
    mLocX = x;
    mLocY = y;
    mLocW = w;
    mLocH = h;

    setTransform();
}
void VideoPlayerGL::play(Video* video) {
    mVideo = video;
    mVideo->resetTime();
}
void VideoPlayerGL::close() {
    mVideo = nullptr;
}
double VideoPlayerGL::elapsedTime() const {
    return mElapsedTime;
}
void VideoPlayerGL::draw() {
    if (!mVideo) {
        LOGD("%s: No video selected.", __FUNCTION__);
        return;
    }
    if (mProgram == 0) {
        LOGE("%s: Program is NULL, has the video player been initialized?", __FUNCTION__);
        return;
    }

    const YUVFrame& frame = mVideo->currentFrame(&mElapsedTime);

    GLERROR();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mYTexture);
    glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_R8,
            frame.ywidth(),
            frame.yheight(),
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            static_cast<const GLvoid*>(frame.ychannel()));

    GLERROR();

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mUVTexture);
    glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RG8,
            frame.uvwidth(),
            frame.uvheight(),
            0,
            GL_RG,
            GL_UNSIGNED_BYTE,
            static_cast<const GLvoid*>(frame.uvchannel()));

    GLERROR();

    GLenum buffers[] = {GL_BACK};
    glDrawBuffers(1, buffers);
    glUseProgram(mProgram);
    glBindVertexArray(mVertexArray);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);
    glBindVertexArray(0);
    glUseProgram(0);

    GLERROR();
}
void VideoPlayerGL::setTransform() {
    // Only set transform uniform if we have a program object
    if (mProgram == 0)
        return;

    // -1 maps to x/screenwidth*2-1
    // 1 maps to (x+w)/screenwidth*2-1

    // f(a) = (x+w*(a+1)/2) / screenwidth * 2 - 1
    //      = (2x+wa+w) / screenwidth - 1
    //      = wa/screenwidth + 2x/screenwidth + w/screenwidth - 1
    LOGD("Setting transform, screen size = %d, %d", mScreenWidth, mScreenHeight);
    LOGD("  Location = %d, %d, %d, %d", mLocX, mLocY, mLocW, mLocH);

    mTransform[0] = static_cast<float>(mLocW)/mScreenWidth;
    mTransform[1] = 0.0f;
    mTransform[2] = 0.0f;
    mTransform[3] = 0.0f;

    mTransform[4] = 0.0f;
    mTransform[5] = static_cast<float>(mLocH)/mScreenHeight;
    mTransform[6] = 0.0f;
    mTransform[7] = 0.0f;

    mTransform[8]  = 0.0f;
    mTransform[9]  = 0.0f;
    mTransform[10] = 0.0f;
    mTransform[11] = 0.0f;

    mTransform[12] = 2.0f*mLocX/mScreenWidth + static_cast<float>(mLocW)/mScreenWidth - 1.0f;
    mTransform[13] = 2.0f*mLocY/mScreenHeight + static_cast<float>(mLocH)/mScreenHeight - 1.0f;
    mTransform[14] = 0.0f;
    mTransform[15] = 1.0f;

    LOGD("Setting transform to:\n"
         "%f, %f, %f, %f,\n"
         "%f, %f, %f, %f,\n"
         "%f, %f, %f, %f,\n"
         "%f, %f, %f, %f\n",
         mTransform[0], mTransform[4], mTransform[8], mTransform[12],
         mTransform[1], mTransform[5], mTransform[9], mTransform[13],
         mTransform[2], mTransform[6], mTransform[10], mTransform[14],
         mTransform[3], mTransform[7], mTransform[11], mTransform[15]);

    GLERROR();
    glUseProgram(mProgram);
    glUniformMatrix4fv(
            mTransformLocation,
            1,
            false,
            mTransform);
    glUseProgram(0);
    GLERROR();
}