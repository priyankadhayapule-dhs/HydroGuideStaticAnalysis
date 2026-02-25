//
// Copyright 2018 EchoNous Inc.
//

#define LOG_TAG "LiveRenderSurface"

#include <string.h>
#include <unistd.h>
#include <math.h>
#include <ThorUtils.h>
#include <LiveRenderSurface.h>

#ifndef EGL_OPENGL_ES3_BIT_KHR
#define EGL_OPENGL_ES3_BIT_KHR				    0x00000040
#endif

//-----------------------------------------------------------------------------
LiveRenderSurface::LiveRenderSurface() :
    mWindowPtr(nullptr),
    mDisplay(EGL_NO_DISPLAY),
    mSurface(EGL_NO_SURFACE),
    mContext(EGL_NO_CONTEXT),
    mWidth(0),
    mHeight(0),
    mNonZeroFrameSkipCount(0),
    mTargetDisplayFrameRate(30),
    mTargetSleepTimeUs(33333.33)
{
}

//-----------------------------------------------------------------------------
LiveRenderSurface::~LiveRenderSurface()
{
    close();
    clearRenderList();
}

//-----------------------------------------------------------------------------
ThorStatus LiveRenderSurface::open(ANativeWindow* windowPtr)
{
    ThorStatus  retVal = THOR_ERROR;
    EGLConfig   config;
    EGLint      numConfigs;
    EGLint      format;

    const EGLint configAttribs[] =
    {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
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

    if (nullptr == windowPtr)
    {
        ALOGE("windowPtr cannot be NULL");
        goto err_ret;
    }

    if (nullptr != mWindowPtr)
    {
        ALOGD("%s: Release window", __func__);
        ANativeWindow_release(mWindowPtr);
    }
    mWindowPtr = windowPtr;
    ALOGD("%s Acquire window", __func__);
    ANativeWindow_acquire(mWindowPtr);

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

    if (!eglGetConfigAttrib(mDisplay, config, EGL_NATIVE_VISUAL_ID, &format))
    {
        ALOGE("eglGetConfigAttrib() returned error 0x%X", eglGetError());
        goto err_ret;
    }

    ANativeWindow_setBuffersGeometry(mWindowPtr, 0, 0, format);

    if (!(mSurface = eglCreateWindowSurface(mDisplay, config, mWindowPtr, 0)))
    {
        ALOGE("eglCreateWindowSurface() returned error 0x%X", eglGetError());
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
        ALOGE("eglQuerySurface() returned error 0x%X", eglGetError());
        goto err_ret;
    }

    ALOGD("Surface Width: %u, Height: %u", mWidth, mHeight);

    glViewport(0, 0, mWidth, mHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    eglSwapBuffers(mDisplay, mSurface);

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

    mFrameCount = 5000;
    mReDrawFrameCount = 0;
    mNumReDrawFrameSkip = 0;

    retVal = THOR_OK;

err_ret:
    if (IS_THOR_ERROR(retVal))
    {
        close();
    }
    return(retVal);
}

//-----------------------------------------------------------------------------
void LiveRenderSurface::close()
{
    // clear the entire render surface
    glViewport(0, 0, mWidth, mHeight);
    glScissor(0, 0, mWidth, mHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    eglSwapBuffers(mDisplay, mSurface);

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

    if (nullptr != mWindowPtr)
    {
        ALOGD("%s: Release window", __func__);
        ANativeWindow_release(mWindowPtr);
        mWindowPtr = nullptr;
    }

    clearRenderList();
}

//-----------------------------------------------------------------------------
void LiveRenderSurface::addRenderer(Renderer* rendererPtr,
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
void LiveRenderSurface::clearRenderList()
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
void LiveRenderSurface::setTargetDisplayFrameRate(uint8_t targetFrameRate)
{
    mTargetDisplayFrameRate = targetFrameRate;
    // 0.8: empirical value -> good for 30 frames/sec
    mTargetSleepTimeUs = 0.8f * 1000000.0f/((float) mTargetDisplayFrameRate);

    ALOGD("Target FrameRate: %d, Target SleepTimeUs: %f", mTargetDisplayFrameRate, mTargetSleepTimeUs);
}

//-----------------------------------------------------------------------------
float LiveRenderSurface::calculateReDrawFrameSkip()
{
    timespec now;

    clock_gettime(CLOCK_REALTIME, &now);

    int64_t timespan = (now.tv_sec - mPrevTime.tv_sec)*1000000000LL + (now.tv_nsec - mPrevTime.tv_nsec);

#if 1
    // for debugging
    ALOGD("Time span (ms): %f; total Frames: %d; redraw Frames: %d", ((float) (timespan / 1000000.0L)), mFrameCount, mReDrawFrameCount);
    float timePerFrame = ((float) (timespan / 1000000.0L))/((float) mFrameCount);
    ALOGD("Feed frameRate: %f", ((float)(mFrameCount - mReDrawFrameCount))/((float) (timespan / 1000000000.0L)));
    ALOGD("Time per frame (ms): %f", timePerFrame);
#endif

    float expectedFrames = ((float) (timespan / 1000000.0L))/(1000.0f/((float) mTargetDisplayFrameRate));

    float numFramesSkip = 0.0f;

    if (mReDrawFrameCount)      // this method is meaningful when mReDrawFrameCount >= 0
    {
        numFramesSkip = ((float) mFrameCount) - expectedFrames;

        if ((mReDrawFrameCount > FRAME_RATE_CHECK_INTERVAL/8) && (numFramesSkip < -3.0f))
        {
            mNonZeroFrameSkipCount++;

            if (mNonZeroFrameSkipCount > 1)
            {
                mNonZeroFrameSkipCount = 0;
                mTargetSleepTimeUs = 0.9f * mTargetSleepTimeUs;
                ALOGD("Target SleepTimeUs has been adjusted to %f", mTargetSleepTimeUs);
            }
        }
        else
        {
            mNonZeroFrameSkipCount = 0;
        }

        if (numFramesSkip < 0.0f)
            numFramesSkip = 0.0f;

        ALOGD("Expected number of frames: %f; measured frames: %d; frames to be skipped: %f", expectedFrames, mFrameCount, numFramesSkip);
    }

    mPrevTime = now;
    mFrameCount = 0;
    mReDrawFrameCount = 0;

    // returns number of frames to be skipped among ReDrawFrame.
    return numFramesSkip;
}

//-----------------------------------------------------------------------------
void LiveRenderSurface::draw()
{
    uint32_t    redrawCnt = 0;
    bool frameSkipped = false;
    bool reDrawFrameSkipped;

    if (mFrameCount > FRAME_RATE_CHECK_INTERVAL) {
        float numFrameSkip = calculateReDrawFrameSkip();
        mNumReDrawFrameSkip += numFrameSkip;
    }

    do
    {
        redrawCnt = 0;
        reDrawFrameSkipped = false;

        for (auto it = mRenderList.begin();
                 it != mRenderList.end();
                 it++)
        {
            uint32_t    cntRet;
            Renderer*   rendererPtr = (*it)->rendererPtr;

            glViewport(rendererPtr->getX(),
                       rendererPtr->getY(),
                       rendererPtr->getWidth(),
                       rendererPtr->getHeight());

            if (rendererPtr->prepare())
            {
                cntRet = rendererPtr->draw();

                if (reDrawFrameSkipped)
                {
                    if (rendererPtr->modeSpecificMethod())
                    {
                        ALOGD("Skipping a frame following skipped reDrawFrame.");
                    }
                }
                else if (cntRet > redrawCnt)
                {
                    redrawCnt = cntRet;
                    // adjust redraw frame numbers - this only happens on scrolling modes
                    if (!frameSkipped && (mNumReDrawFrameSkip >= 1.0f))
                    {
                        if (rendererPtr->modeSpecificMethod())
                        {
                            redrawCnt -= 1;
                            mNumReDrawFrameSkip-=1.0f;
                            frameSkipped = true;
                            reDrawFrameSkipped = true;
                            ALOGD("Skipping one reDrawFrame.");
                        }
                    }
                }
            }
        }
        if (redrawCnt)
        {
            eglSwapBuffers(mDisplay, mSurface);
            mFrameCount++;
            mReDrawFrameCount++;
            // TODO: may need to replace it with a more sophisticated timer mechanism
            usleep(mTargetSleepTimeUs);
        }
    } while (redrawCnt);

    eglSwapBuffers(mDisplay, mSurface);
    mFrameCount++;
}
