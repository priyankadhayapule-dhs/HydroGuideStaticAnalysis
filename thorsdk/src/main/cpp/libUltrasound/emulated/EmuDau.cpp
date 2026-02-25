//
// Copyright 2016 EchoNous Inc.
//
//

#define LOG_TAG "DauEmulatedNative"

#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <ThorUtils.h>
#include <EmuDau.h>
#include <UltrasoundManager.h>


//-----------------------------------------------------------------------------
EmuDau::EmuDau(DauContext& dauContext,
               CineBuffer& cineBuffer) :
    mDauContext(dauContext),
    mDauSignalHandler(mDauContext),
    mCallback((void*) this),
    mTimerFd(-1),
    mCineBuffer(cineBuffer)
{
}

//-----------------------------------------------------------------------------
EmuDau::~EmuDau()
{
    close();
}

//-----------------------------------------------------------------------------
ThorStatus EmuDau::open()
{
    ThorStatus                  retVal = THOR_ERROR;
    struct ScanConverterParams  converterParams;
    std::string                 upsPath = mDauContext.appPath + "/" + UpsReader::DEFAULT_UPS_FILE;

    ALOGD("%s", __func__);

    mUpsReaderSPtr = std::make_shared<UpsReader>();
    if (!mUpsReaderSPtr->open(upsPath.c_str()))
    {
        ALOGE("Unable to open the UPS");
        retVal = TER_IE_UPS_OPEN;
        goto err_ret;
    }
    ALOGD("UPS version : %s", mUpsReaderSPtr->getVersion());

    converterParams.numSampleMesh = 50;
    converterParams.numRayMesh = 100;
    converterParams.startSampleMm = 0.0f;
    converterParams.numSamples = 512;
    converterParams.sampleSpacingMm = 0.015625f;
    converterParams.startRayRadian = -0.7819f;
    converterParams.raySpacing = 0.013962f;
    converterParams.numRays = 112;

    mCineBuffer.setParams(converterParams,
                          1008,
                          IMAGING_MODE_B,
                          mUpsReaderSPtr);

    mTimerFd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
    if (-1 == mTimerFd)
    {
        ALOGE("Unable to create timer");
        goto err_ret;
    }

    mDauContext.dataPollFd.initialize(PollFdType::Timer, mTimerFd);

    if (!mDauSignalHandler.init(&mCallback))
    {
        ALOGE("Unable to initialize signal handler");
        ::close(mTimerFd);
        mTimerFd = -1;
        retVal = TER_IE_SIGHANDLE_INIT;
        goto err_ret;
    }

    retVal = THOR_OK;

err_ret:
    return(retVal); 
}

//-----------------------------------------------------------------------------
void EmuDau::close()
{
    ALOGD("%s", __func__);

    mDauSignalHandler.terminate();
    mDauContext.clear();

    if (-1 != mTimerFd)
    {
        ::close(mTimerFd);
        mTimerFd = -1;
    }

    if (nullptr != mUpsReaderSPtr)
    {
        mUpsReaderSPtr->close();
        mUpsReaderSPtr = nullptr;
    }
}

//-----------------------------------------------------------------------------
bool EmuDau::start()
{
    bool                retVal = false;
    struct timespec     now;

    ALOGD("%s", __func__);

    populateDmaFifo();

    if (!mDauSignalHandler.start())
    {
        ALOGE("Unable to start signal handler");
        goto err_ret;
    }

    if (-1 == clock_gettime(CLOCK_REALTIME, &now))
    {
        ALOGE("Unable to get time");
        goto err_ret;
    }

    mTimerSpec.it_value.tv_sec = now.tv_sec;
    mTimerSpec.it_value.tv_nsec = 0;
    mTimerSpec.it_interval.tv_sec = 0;
    mTimerSpec.it_interval.tv_nsec = 30000000;

    if (-1 == timerfd_settime(mTimerFd, TFD_TIMER_ABSTIME, &mTimerSpec, NULL))
    {
        ALOGE("Unable to set timer");
        goto err_ret;
    }

    retVal = true;

err_ret:
    return(retVal); 
}

//-----------------------------------------------------------------------------
void EmuDau::stop()
{
    ALOGD("%s", __func__);

    mTimerSpec.it_value.tv_sec = 0;
    mTimerSpec.it_value.tv_nsec = 0;
    mTimerSpec.it_interval.tv_sec = 0;
    mTimerSpec.it_interval.tv_nsec = 0;

    if (-1 == timerfd_settime(mTimerFd, TFD_TIMER_ABSTIME, &mTimerSpec, NULL))
    {
        ALOGE("Unable to clear timer");
    }

    mDauSignalHandler.stop();
}

//-----------------------------------------------------------------------------
void EmuDau::populateDmaFifo()
{
    float       ux = 255.0;
    uint8_t*    cineBufferPtr = mFifoBuffer[0];
    int         numSamples = 512;
    int         numRays = 112;

    // Build a ramp pattern
    for (int index = 0; index < FIFO_FRAME_COUNT; index++)
    {
        for (int j = 0; j < numRays; j++)
        {
            for (int i = 0; i < numSamples; i++)
            {
                ux = (i + j * (index + 1)) % 256;
                *(cineBufferPtr + (index * MAX_B_FRAME_SIZE) + (j * numSamples + i)) = ux;
            }
        }
    }
}

//-----------------------------------------------------------------------------
ThorStatus EmuDau::SignalCallback::onInit()
{
    ThorStatus  openStatus = THOR_OK;
    EmuDau*     thisPtr = (EmuDau*) getDataPtr();

    ALOGD("onInit");

    thisPtr->mCineBuffer.reset();
    thisPtr->mCineBuffer.prepareClients();

    return(openStatus);
}

//-----------------------------------------------------------------------------
ThorStatus EmuDau::SignalCallback::onData(int count)
{
    EmuDau*     thisPtr = (EmuDau*) getDataPtr();
    uint8_t*    inputPtr = thisPtr->mFifoBuffer[count % FIFO_FRAME_COUNT];
    uint8_t*    outputPtr = thisPtr->mCineBuffer.getFrame(count, IMAGING_MODE_B);

    memcpy(outputPtr, inputPtr, MAX_B_FRAME_SIZE);
    thisPtr->mCineBuffer.setFrameComplete(count, IMAGING_MODE_B);

    return(THOR_OK); 
}

//-----------------------------------------------------------------------------
void EmuDau::SignalCallback::onTerminate()
{
    EmuDau*     thisPtr = (EmuDau*) getDataPtr();

    thisPtr->mCineBuffer.freeClients();
}


