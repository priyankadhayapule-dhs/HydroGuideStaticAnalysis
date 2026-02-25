//
// Copyright 2018 EchoNous Inc.
//

#define LOG_TAG "OffScreenRenderSurface"

#include <string.h>
#include <unistd.h>
#include <math.h>
#include <ThorUtils.h>
#include <OffScreenRenderSurface.h>

#include <fcntl.h>
#include <sys/stat.h>

#include <stdio.h>
#include <opencv2/imgcodecs.hpp>

#ifndef EGL_OPENGL_ES3_BIT_KHR
#define EGL_OPENGL_ES3_BIT_KHR                  0x00000040
#endif

#ifndef EGL_RECORDABLE_ANDROID
#define EGL_RECORDABLE_ANDROID                  0x00003142
#endif

//-----------------------------------------------------------------------------
OffScreenRenderSurface::OffScreenRenderSurface() :
    mDisplay(EGL_NO_DISPLAY),
    mSurface(EGL_NO_SURFACE),
    mContext(EGL_NO_CONTEXT),
    mWidth(0),
    mHeight(0)
{
}

//-----------------------------------------------------------------------------
OffScreenRenderSurface::~OffScreenRenderSurface()
{
    close();
    clearRenderList();
}

//-----------------------------------------------------------------------------
ThorStatus OffScreenRenderSurface::open(int width, int height)
{
    ThorStatus  retVal = THOR_ERROR;
    EGLConfig   config;
    EGLint      numConfigs;
    EGLint      format;

    const EGLint configAttribs[] =
    {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
        EGL_NONE
    };

    const EGLint contextAttribs[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };

    // off screen surface attributes - sizes
    const EGLint surfaceAttribs[] = {
            EGL_WIDTH, width,
            EGL_HEIGHT, height,
            EGL_NONE
    };

    mWidth = width;
    mHeight = height;
    if (0 == mRenderList.size())
    {
        ALOGE("setRenderList() must be called first with valid Renderers");
        goto err_ret;
    }

    if ((mDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY)) == EGL_NO_DISPLAY)
    {
        ALOGE("eglGetDisplay() returned error 0x%X", eglGetError());
        return(false);
    }

    if (!eglInitialize(mDisplay, 0, 0))
    {
        ALOGE("eglInitialize() returned error 0x%X", eglGetError());
        goto err_ret;
    }

    if (!eglChooseConfig(mDisplay, configAttribs, &config, 1, &numConfigs))
    {
        ALOGE("eglChooseConfig() returned error 0x%X", eglGetError());
        goto err_ret;
    }

    if (!(mSurface = eglCreatePbufferSurface(mDisplay, config, surfaceAttribs))) {
        ALOGE("eglCreateWindowSurface() returned error %d", eglGetError());
        goto err_ret;
    }

    if (!(mContext = eglCreateContext(mDisplay, config, 0, contextAttribs)))
    {
        ALOGE("eglCreateContext() returned error 0x%X", eglGetError());
        goto err_ret;
    }

    if (!eglMakeCurrent(mDisplay, mSurface, mSurface, mContext))
    {
        ALOGE("eglMakeCurrent() returned error 0x%X", eglGetError());
        goto err_ret;
    }

    ALOGD("Surface Width: %u, Height: %u", mWidth, mHeight);

    glViewport(0, 0, mWidth, mHeight);
    glClearColor(0, 0, 0, 0);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_SCISSOR_TEST);

    // Initialize all Renderers
    for (auto it = mRenderList.begin();
         it != mRenderList.end();
         it++)
    {
        RendererAttributes    attrib;

        attrib.xPct = (*it)->xPct;
        attrib.yPct = (*it)->yPct;
        attrib.widthPct = (*it)->widthPct;
        attrib.heightPct = (*it)->heightPct;

        attrib.x = round((float) attrib.xPct / 100.0f * mWidth);
        attrib.y = round((float) attrib.yPct / 100.0f * mHeight);
        attrib.width = round((float) attrib.widthPct / 100.0f * mWidth);
        attrib.height = round((float) attrib.heightPct / 100.0f * mHeight);

        RenderSurface::setRendererAttributes((*it)->rendererPtr, attrib);

        retVal = (*it)->rendererPtr->open();
        if (IS_THOR_ERROR(retVal))
        {
            goto err_ret;
        }
    }

    retVal = THOR_OK;
    mOpen = true;
    ALOGD("OffScreenRenderSurface created");

err_ret:
    if (IS_THOR_ERROR(retVal))
    {
        close();
    }
    return(retVal);
}

// -----------------------------------------------------------------------------
ThorStatus OffScreenRenderSurface::open(ANativeWindow *inputSurface)
{
    ThorStatus retVal = THOR_ERROR;
    EGLConfig config;
    EGLint numConfigs;
    EGLint format;

    const EGLint configAttribs[] =
    {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
        EGL_RECORDABLE_ANDROID, 1,
        EGL_NONE
    };

    const EGLint contextAttribs[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };

    if (0 == mRenderList.size())
    {
        ALOGE("setRenderList() must be called first with valid Renderers");
        goto err_ret;
    }

    if ((mDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY)) == EGL_NO_DISPLAY)
    {
        ALOGE("eglGetDisplay() returned error 0x%X", eglGetError());
        return (false);
    }

    if (!eglInitialize(mDisplay, 0, 0))
    {
        ALOGE("eglInitialize() returned error 0x%X", eglGetError());
        goto err_ret;
    }

    if (!eglChooseConfig(mDisplay, configAttribs, &config, 1, &numConfigs))
    {
        ALOGE("eglChooseConfig() returned error 0x%X", eglGetError());
        goto err_ret;
    }

    if (!(mSurface = eglCreateWindowSurface(mDisplay, config, inputSurface, 0)))
    {
        ALOGE("eglCreateWindowSurface() returned error %d", eglGetError());
        goto err_ret;
    }

    if (!eglSurfaceAttrib(mDisplay, mSurface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_PRESERVED))
    {
        ALOGE("eglSurfaceAttrib() returned error 0x%X", eglGetError());
        goto err_ret;
    }

    if (!(mContext = eglCreateContext(mDisplay, config, 0, contextAttribs)))
    {
        ALOGE("eglCreateContext() returned error 0x%X", eglGetError());
        goto err_ret;
    }

    if (!eglMakeCurrent(mDisplay, mSurface, mSurface, mContext))
    {
        ALOGE("eglMakeCurrent() returned error 0x%X", eglGetError());
        goto err_ret;
    }

    if (!eglQuerySurface(mDisplay, mSurface, EGL_WIDTH, &mWidth) ||
        !eglQuerySurface(mDisplay, mSurface, EGL_HEIGHT, &mHeight))
    {
        ALOGE("eglQuerySurface() returned error %d", eglGetError());
        goto err_ret;
    }

    ALOGD("Surface Width: %u, Height: %u", mWidth, mHeight);

    glViewport(0, 0, mWidth, mHeight);
    glClearColor(0, 0, 0, 0);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Initialize all Renderers
    for (auto it = mRenderList.begin();
         it != mRenderList.end();
         it++)
    {
        RendererAttributes attrib;

        attrib.xPct = (*it)->xPct;
        attrib.yPct = (*it)->yPct;
        attrib.widthPct = (*it)->widthPct;
        attrib.heightPct = (*it)->heightPct;

        attrib.x = round((float)attrib.xPct / 100.0f * mWidth);
        attrib.y = round((float)attrib.yPct / 100.0f * mHeight);
        attrib.width = round((float)attrib.widthPct / 100.0f * mWidth);
        attrib.height = round((float)attrib.heightPct / 100.0f * mHeight);

        RenderSurface::setRendererAttributes((*it)->rendererPtr, attrib);

        retVal = (*it)->rendererPtr->open();
        if (IS_THOR_ERROR(retVal))
        {
            goto err_ret;
        }
    }

    retVal = THOR_OK;
    mOpen = true;
    ALOGD("OffScreenRenderSurface created");

err_ret:
    if (IS_THOR_ERROR(retVal))
    {
        close();
    }
    return (retVal);
}

//-----------------------------------------------------------------------------
void OffScreenRenderSurface::close()
{
    // finish the queued up operations
    glFinish();

    for (auto it = mRenderList.begin();
         it != mRenderList.end();
         it++)
    {
        (*it)->rendererPtr->close();
    }

    if (EGL_NO_DISPLAY != mDisplay)
    {
        ALOGD("Destroying context");

        eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        if (EGL_NO_CONTEXT != mContext)
        {
            eglDestroyContext(mDisplay, mContext);
        }
        if (EGL_NO_SURFACE != mSurface)
        {
            eglDestroySurface(mDisplay, mSurface);
        }
        eglTerminate(mDisplay);

        mDisplay = EGL_NO_DISPLAY;
        mSurface = EGL_NO_SURFACE;
        mContext = EGL_NO_CONTEXT;
    }

    clearRenderList();
    mOpen = false;
}

//-----------------------------------------------------------------------------
void OffScreenRenderSurface::addRenderer(Renderer *rendererPtr,
                                         float xPct,
                                         float yPct,
                                         float widthPct,
                                         float heightPct)
{
    RendererNode*   newNodePtr = new RendererNode();

    newNodePtr->rendererPtr = rendererPtr;
    newNodePtr->xPct = xPct;
    newNodePtr->yPct = yPct;
    newNodePtr->widthPct = widthPct;
    newNodePtr->heightPct = heightPct;

    mRenderList.push_back(newNodePtr);
}

//-----------------------------------------------------------------------------
void OffScreenRenderSurface::clearRenderList()
{
    for (auto it = mRenderList.begin();
         it != mRenderList.end();
         it++)
    {
        delete *it;
    }

    mRenderList.clear();
}

//-----------------------------------------------------------------------------
void OffScreenRenderSurface::draw()
{
    for (auto it = mRenderList.begin();
         it != mRenderList.end();
         it++)
    {
        Renderer*   rendererPtr = (*it)->rendererPtr;

        glViewport(rendererPtr->getX(),
                   rendererPtr->getY(),
                   rendererPtr->getWidth(),
                   rendererPtr->getHeight());

        if (rendererPtr->prepare())
        {
            // ignore redraw for the still screen image capture
            rendererPtr->draw();
        }
    }

    eglSwapBuffers(mDisplay, mSurface);
}

//-----------------------------------------------------------------------------
void OffScreenRenderSurface::takeScreenShotToRGBAPtrUD(uint8_t *outImg)
{
    // Img before flipUD - RGBA format
    u_char* tmpImg = new u_char[mWidth * mHeight * 4];
    int idx, nidx;

    // draw
    draw();
    glFinish();
    // readPixels
    glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, tmpImg);

    for (int j = 0; j < mHeight; j++)
    {
        for (int i = 0; i < mWidth; i++)
        {
            idx = j * mWidth + i;
            nidx = (mHeight - 1 - j) * mWidth + i;

            outImg[4*nidx] = tmpImg[4*idx];
            outImg[4*nidx + 1] = tmpImg[4*idx + 1];
            outImg[4*nidx + 2] = tmpImg[4*idx + 2];
            outImg[4*nidx + 3] = tmpImg[4*idx + 3];
        }
    }

    delete [] tmpImg;
}

//-----------------------------------------------------------------------------
void OffScreenRenderSurface::takeScreenShotToRGBPtrUD(uint8_t *outImg)
{
    // Img before flipUD - RGB format
    u_char* tmpImg = new u_char[mWidth * mHeight * 4];
    int idx, nidx;

    // draw
    draw();
    glFinish();
    // readPixels
    glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, tmpImg);

    for (int j = 0; j < mHeight; j++)
    {
        for (int i = 0; i < mWidth; i++)
        {
            idx = j * mWidth + i;
            nidx = (mHeight - 1 - j) * mWidth + i;

            outImg[3*nidx] = tmpImg[4*idx];
            outImg[3*nidx + 1] = tmpImg[4*idx + 1];
            outImg[3*nidx + 2] = tmpImg[4*idx + 2];
        }
    }

    delete [] tmpImg;
}

//-----------------------------------------------------------------------------
void OffScreenRenderSurface::takeScreenShotToJpegFileUD(const char *filename)
{
    // RGBA format
    u_char* outImg = new u_char[mWidth * mHeight * 4];
    // RGB channel for JPEG fileout
    u_char* outImgBGR = new u_char[mWidth * mHeight * 3];
    int idx, nidx;

    //glReadPixels
    draw();
    glFinish();
    glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, outImg);

    // copying only RGB channels to output
    for (int j = 0; j < mHeight; j++)
    {
        for (int i = 0; i < mWidth; i++)
        {
            idx = j * mWidth + i;
            nidx = (mHeight - 1 - j) * mWidth + i;

            outImgBGR[3*nidx + 0] = outImg[4*idx + 2];
            outImgBGR[3*nidx + 1] = outImg[4*idx + 1];
            outImgBGR[3*nidx + 2] = outImg[4*idx + 0];
        }
    }

    // encode RGB image to a Jpeg file.
    encodeJpegImage(filename, outImgBGR, mWidth, mHeight, 100);

    delete [] outImg;
    delete [] outImgBGR;
}

//-----------------------------------------------------------------------------
void OffScreenRenderSurface::takeScreenShotToRGBAPtr(uint8_t *outImg)
{
    // draw
    draw();
    glFinish();
    // readPixels
    glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, outImg);
}

//-----------------------------------------------------------------------------
void OffScreenRenderSurface::takeScreenShotToJpegFile(const char *filename)
{
    // RGBA format
    u_char* outImg = new u_char[mWidth * mHeight * 4];
    // BGR channel for JPEG fileout
    u_char* outImgBGR = new u_char[mWidth * mHeight * 3];

    //glReadPixels
    draw();
    glFinish();
    glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, outImg);

    // copying only RGB channels to output
    // OpenCV using BGR image planes. Convert directly to that format instead of RGB then BGR
    for (int i = 0; i < mWidth * mHeight; i++) {
        outImgBGR[3*i + 0] = outImg[4*i + 2];
        outImgBGR[3*i + 1] = outImg[4*i + 1];
        outImgBGR[3*i + 2] = outImg[4*i + 0];
    }

    // encode RGB image to a Jpeg file.
    encodeJpegImage(filename, outImgBGR, mWidth, mHeight, 100);

    delete [] outImg;
    delete [] outImgBGR;
}

//-----------------------------------------------------------------------------
void OffScreenRenderSurface::encodeJpegImage(const char *filename, uint8_t *inputImg,
                                             int imgWidth, int imgHeight, int imgQuality)
{
    ALOGD("start encodeJpegImage");
    std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, imgQuality};
    cv::Mat image(imgHeight, imgWidth, CV_8UC3, inputImg);
    cv::imwrite(filename, image, params);
}


//-----------------------------------------------------------------------------
void OffScreenRenderSurface::takeScreenShotToRawFile(const char *filename)
{
    // RGBA format
    u_char* outImg = new u_char[mWidth * mHeight * 4];
    // R channel only for Raw fileout
    u_char* outImgR = new u_char[mWidth * mHeight];
    // file
    int     fd = -1;

    //glReadPixels
    draw();
    glFinish();
    glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, outImg);

    // copying only R channel to output
    for (int i = 0; i < mWidth * mHeight; i++) {
        outImgR[i] = outImg[4*i];
    }

    fd = ::open(filename,
                O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRWXG);
    if (-1 == fd)
    {
        ALOGE("Unable to create data file: %s", filename);
        goto err_ret;
    }

    ::write(fd, outImgR, sizeof(u_char) * mWidth * mHeight);

    delete [] outImg;
    delete [] outImgR;

    ALOGD("takeScreenShotToRawFile has been completed.");

err_ret:
    if (-1 != fd)
    {
        ::close(fd);
    }
}

//-----------------------------------------------------------------------------
void OffScreenRenderSurface::takeScreenShotToGrayPtrUD(uint8_t *outImg)
{
    uint8_t* tmpImg = new uint8_t[mWidth * mHeight * 4];
    // draw
    draw();
    glFinish();
    // readPixels
    glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, tmpImg);
    int idx, nidx = 0;
    for (int j = 0; j < mHeight; j++)
    {
        for (int i = 0; i < mWidth; i++)
        {
            idx = j * mWidth + i;
            nidx = (mHeight - 1 - j) * mWidth + i;

            outImg[nidx] = tmpImg[4*idx + 2]*.11f + tmpImg[4*idx + 1]*.59f + tmpImg[4*idx + 0]*.3f;
        }
    }
    delete[] tmpImg;
}