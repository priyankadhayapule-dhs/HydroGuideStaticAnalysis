//
// Copyright 2022 EchoNous Inc.
//
#pragma once
#include <EGL/egl.h>
#include <GLES/gl.h>
#include <vector>

// Forward declarations
class Video;

class VideoPlayerGL {
public:
    VideoPlayerGL();
    ~VideoPlayerGL();

    bool init();
    void uninit();

    void setScreenSize(int width, int height);
    void setLocation(int x, int y, int w, int h);

    void play(Video* video);
    void close();

    double elapsedTime() const;

    void draw();

private:
    void setTransform();

    GLuint mProgram;
    GLuint mYTexture;
    GLuint mUVTexture;
    GLuint mVertexArray;
    GLuint mVertexBuffer;
    GLuint mIndexBuffer;
    Video* mVideo;

    GLint mTransformLocation;
    GLint mYTextureLocation;
    GLint mUVTextureLocation;

    int mScreenWidth;
    int mScreenHeight;
    int mLocX;
    int mLocY;
    int mLocW;
    int mLocH;
    float mTransform[16];

    double mElapsedTime;
};

