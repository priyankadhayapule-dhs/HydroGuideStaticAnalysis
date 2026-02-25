//
// Copyright 2016 EchoNous, Inc.
//

#define LOG_TAG "DauSignalHandler"

#include <stdlib.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <DauSignalHandler.h>
#include <CriticalSection.h>
#include <ThorUtils.h>
#include <ThorError.h>
#include <PollFd.h>

const char* const DauSignalHandler::mStateNames[StateCount] = 
{
    "Unknown",
    "Idle",
    "Running",
};

const char* const DauSignalHandler::mPollFdNames[FdCount] = 
{
    "Start Event",
    "Stop Event",
    "Exit Event",
    "Data FD",
    "Hid FD",
    "Error FD",
    "USBMem FD",
    "External ECG FD",
};

//-----------------------------------------------------------------------------
DauSignalHandler::DauSignalHandler(DauContext& dauContext) :
    mDauContext(dauContext),
    mCallbackPtr(nullptr),
    mCurState(State::Unknown)
{
}

//-----------------------------------------------------------------------------
DauSignalHandler::~DauSignalHandler()
{
    terminate();
}

//-----------------------------------------------------------------------------
bool DauSignalHandler::init(SignalCallback* callbackPtr)
{
    bool    retVal = false;
    int     ret;

    mLock.enter();
    if (State::Unknown != mCurState)
    {
        ALOGI("Already initialized");
        retVal = true;
        goto err_ret;
    }

    if (!mStartEvent.init(EventMode::ManualReset, false))
    {
        ALOGE("Unable to initialize StartEvent");
        goto err_ret;
    }
    if (!mStopEvent.init(EventMode::ManualReset, false))
    {
        ALOGE("Unable to initialize StopEvent");
        goto err_ret;
    }
    if (!mExitEvent.init(EventMode::ManualReset, false))
    {
        ALOGE("Unable to initialize ExitEvent");
        goto err_ret;
    }
    if (!mAckEvent.init(EventMode::AutoReset, false))
    {
        ALOGE("Unable to initialize AckEvent");
        goto err_ret;
    }

    ret = pthread_create(&mThreadId, NULL, workerThreadEntry, this);
    if (0 != ret)
    {
        ALOGE("Failed to create worker thread: ret = %d", ret);
        goto err_ret;
    }

    mCallbackPtr = callbackPtr;

    ALOGI("Initialized");

    mCurState = State::Idle;
    retVal = true;

err_ret:
    mLock.leave();
    return(retVal);
}

//-----------------------------------------------------------------------------
bool DauSignalHandler::start()
{
    bool retVal = false;

    mLock.enter();
    if (State::Running == mCurState)
    {
        ALOGI("Already running");
        retVal = true;
        goto err_ret;
    }
    else if (State::Idle != mCurState)
    {
        ALOGE("Cannot start unless in Idle state.  Current state: (%s)",
              mStateNames[mCurState]);
        goto err_ret;
    }

    mStartEvent.set();
    mAckEvent.wait();

    ALOGI("Started");
    mCurState = State::Running;
    retVal = true;

err_ret:
    mLock.leave();
    return(retVal);
}

//-----------------------------------------------------------------------------
void DauSignalHandler::stop()
{
    mLock.enter();
    if (State::Idle == mCurState)
    {
        ALOGI("Already stopped");
        goto err_ret;
    }
    else if (State::Running != mCurState)
    {
        ALOGE("Cannot stop unless in Running state.  Current state: (%s)",
              mStateNames[mCurState]);
        goto err_ret;
    }

    mStopEvent.set();
    mAckEvent.wait();

    ALOGI("Stopped");
    mCurState = State::Idle;

err_ret:
    mLock.leave();
}

//-----------------------------------------------------------------------------
void DauSignalHandler::terminate()
{
    bool    waitForThread = false;
    State   curState;

    mLock.enter();
    curState = mCurState;
    mLock.leave();

    switch (curState)
    {
        case State::Running:
            stop();

        case State::Idle:
        {
            mExitEvent.set();
            if (mAckEvent.wait(2000))
            {
                waitForThread = true;
            }
            else
            {
                ALOGE("Worker thread failed to respond");
            }
            break;
        }

        default:
            return; 
    }

    // Wait outside of critical section to prevent deadlock
    if (waitForThread)
    {
        ALOGI("Waiting for thread to exit");
        pthread_join(mThreadId, NULL);
    }

    ALOGI("Terminated");

    mLock.enter();
    mCurState = State::Unknown;
    mLock.leave();
}


//-----------------------------------------------------------------------------
void* DauSignalHandler::workerThreadEntry(void *thisPtr)
{
    ((DauSignalHandler *)thisPtr)->threadLoop();

    return(nullptr);
}

//-----------------------------------------------------------------------------
void DauSignalHandler::threadLoop()
{
    struct pollfd   pollFd[FdCount];
    int             numFd = FdCount;
    int             ret;
    bool            keepLooping = true;
    int             timeout = -1; // infinite
    const int       BufSize = 128;
    char            readBuf[BufSize];
    uint32_t        procCount = 0;      // Processed frame count
    uint32_t        recCount = 0;       // Received frame count
    uint32_t        countOffset = 0;    // Received count may not start from 0
    ThorStatus      dauErrorCode = THOR_OK;
    uint16_t        hidId;

    ALOGD("threadLoop: entered");

    initPollFd(pollFd);

    while (keepLooping)
    {
        ret = poll(pollFd, numFd, timeout);
        if (-1 == ret)
        {
            ALOGE("threadLoop: Error occurred during poll(). errno = %d", errno);
            ALOGD("Processed %d frames", procCount);
            dauErrorCode = THOR_ERROR;
            keepLooping = false;
        }
        else if (0 == ret)
        {
            ALOGE("threadLoop: poll() timed out, possible data underflow.");
            ALOGD("Processed %d frames", procCount);
            dauErrorCode = TER_IE_UNDERFLOW;
            keepLooping = false;
        }
        else
        {
            //
            // Exit
            //
            if (pollFd[ExitFd].revents & POLLIN)
            {
                keepLooping = false;
                mExitEvent.reset();
                mAckEvent.set();
            }
            //
            // Stop
            //
            else if (pollFd[StopFd].revents & POLLIN)
            {
                if (nullptr != mCallbackPtr)
                {
                    mCallbackPtr->onTerminate();
                }

                pollFd[DataFd].fd = -1;
                pollFd[ErrorFd].fd = -1;

                ALOGD("Processed %d frames", procCount);

                ALOGD("threadLoop: Monitoring stopped");
                mStopEvent.reset();
                mAckEvent.set();

                timeout = -1;
            }
            //
            // Start
            //
            else if (pollFd[StartFd].revents & POLLIN)
            {
                procCount = 0;

                mDauContext.dataPollFd.prepare();
                mDauContext.errorPollFd.prepare();

                pollFd[DataFd].fd = ((struct pollfd) mDauContext.dataPollFd).fd;
                pollFd[ErrorFd].fd = ((struct pollfd) mDauContext.errorPollFd).fd;

                countOffset = mDauContext.dataPollFd.read();

                if (nullptr != mCallbackPtr)
                {
                    dauErrorCode = mCallbackPtr->onInit();
                    if (IS_THOR_ERROR(dauErrorCode))
                    {
                        ALOGE("threadLoop: Failed to initialize callback");
                        keepLooping = false;
                    }
                    else
                    {
                        ALOGD("threadLoop: Monitoring started.  procCount = %u",
                              procCount);
                    }
                }

                mStartEvent.reset();
                mAckEvent.set();

                timeout = 1000;
            }
            //
            // Error
            //
            else if (pollFd[ErrorFd].revents & ((struct pollfd) mDauContext.errorPollFd).events)
            {
                dauErrorCode = mDauContext.errorPollFd.read();
                if ((0 == dauErrorCode) || (0xFFFFFFFF == dauErrorCode))
                {
                    //dauErrorCode = TER_DAU_UNKNOWN;
                    ALOGE("Received an interrupt from the Dau for no known reason!  Ignoring...");
                    dauErrorCode = THOR_OK;
                    continue;
                }
                keepLooping = false;
            }
            //
            // USB DAU Memory TLV Protocol Error
            //
            else if (pollFd[DauMemFd].revents & ((struct pollfd) mDauContext.usbDauMemPollFd).events)
            {
                dauErrorCode = mDauContext.usbDauMemPollFd.read();
                if ((0 == dauErrorCode) || (0xFFFFFFFF == dauErrorCode))
                {
                    dauErrorCode = TER_DAU_UNKNOWN;
                }
                keepLooping = false;
            }
            //
            // User Input (USB HID / buttons)
            //
            else if (pollFd[HidFd].revents & ((struct pollfd) mDauContext.hidPollFd).events)
            {
                hidId = mDauContext.hidPollFd.read();

                if (nullptr != mCallbackPtr)
                {
                    mCallbackPtr->onHid(hidId);
                }
            }
            //
            // Data
            //
            else if (pollFd[DataFd].revents & ((struct pollfd) mDauContext.dataPollFd).events)
            {
                recCount = mDauContext.dataPollFd.read();

                if ((procCount + 1) < (recCount -  countOffset))
                {
                    ALOGD("Behind, catching up...  procCount = %u,  recCount (calculated) = %u",
                          procCount, recCount -  countOffset);
                    ALOGD("recCount (actual) = %d,  countOffset = %d",
                          recCount, countOffset);
                }

                while (procCount < (recCount -  countOffset))
                {
                    if (nullptr != mCallbackPtr)
                    {
                        dauErrorCode = mCallbackPtr->onData(procCount);
                        if (IS_THOR_ERROR(dauErrorCode))
                        {
                            keepLooping = false;
                            break;
                        }
                    }

                    if (0 == (procCount % 120))
                    {
                        ALOGD("recCount = %u", recCount -  countOffset);
                    }
                    procCount++;
                }
            }
            //
            // External ECG Lead Connection
            // 
            else if (pollFd[ExtEcgFd].revents & ((struct pollfd) mDauContext.externalEcgPollFd).events)
            {
                ALOGD("ECG Lead Connect Event");
                int ecgConnect = mDauContext.externalEcgPollFd.read();

                if (nullptr != mCallbackPtr)
                {
                    mCallbackPtr->onExternalEcg(1 == ecgConnect);
                }
            }
            //
            // Check Temperature
            // 
            else if (pollFd[ProbeTempFd].revents & ((struct pollfd) mDauContext.temperaturePollFd).events)
            {
                mDauContext.temperaturePollFd.read();

                if (nullptr != mCallbackPtr)
                {
                    dauErrorCode = mCallbackPtr->onCheckTemperature();
                    if (IS_THOR_ERROR(dauErrorCode))
                    {
                        keepLooping = false;
                        break;
                    }
                }
            }
            //
            // Unknown
            //
            else
            {
                // Unexpected results.  Might be an error condition.
                logPollResults(pollFd);
            }
        }
    }

    if (nullptr != mCallbackPtr && THOR_OK != dauErrorCode)
    {
        mCallbackPtr->onError(dauErrorCode);
        mCallbackPtr->onTerminate();
    }

    // Just-in-case signal to prevent infinite locking in start/stop methods
    mAckEvent.set();

    mLock.enter();
    mCurState = State::Unknown;
    mLock.leave();

    ALOGD("threadLoop: exited");
}

//-----------------------------------------------------------------------------
void DauSignalHandler::initPollFd(struct pollfd (&pollArray)[FdCount])
{
    const int       BufSize = 128;
    char            readBuf[BufSize];

    pollArray[StartFd].fd = mStartEvent.getFd();
    pollArray[StartFd].events = POLLIN;
    pollArray[StartFd].revents = 0;

    pollArray[StopFd].fd = mStopEvent.getFd();
    pollArray[StopFd].events = POLLIN;
    pollArray[StopFd].revents = 0;

    pollArray[ExitFd].fd = mExitEvent.getFd();
    pollArray[ExitFd].events = POLLIN;
    pollArray[ExitFd].revents = 0;

    mDauContext.dataPollFd.prepare();
    mDauContext.hidPollFd.prepare();
    mDauContext.errorPollFd.prepare();
    // USB error related fd
    mDauContext.usbDauMemPollFd.prepare();

    mDauContext.externalEcgPollFd.prepare();
    mDauContext.temperaturePollFd.prepare();

    pollArray[DataFd].fd = ((struct pollfd) mDauContext.dataPollFd).fd;
    pollArray[DataFd].events = ((struct pollfd) mDauContext.dataPollFd).events;
    pollArray[DataFd].revents = 0;

    pollArray[HidFd].fd = ((struct pollfd) mDauContext.hidPollFd).fd;
    pollArray[HidFd].events = ((struct pollfd) mDauContext.hidPollFd).events;
    pollArray[HidFd].revents = 0;

    pollArray[ErrorFd].fd = ((struct pollfd) mDauContext.errorPollFd).fd;
    pollArray[ErrorFd].events = ((struct pollfd) mDauContext.errorPollFd).events;
    pollArray[ErrorFd].revents = 0;

    // USB error related fd
    pollArray[DauMemFd].fd = ((struct pollfd) mDauContext.usbDauMemPollFd).fd;
    pollArray[DauMemFd].events = ((struct pollfd) mDauContext.usbDauMemPollFd).events;
    pollArray[DauMemFd].revents = 0;

    pollArray[ExtEcgFd].fd = ((struct pollfd) mDauContext.externalEcgPollFd).fd;
    pollArray[ExtEcgFd].events = ((struct pollfd) mDauContext.externalEcgPollFd).events;
    pollArray[ExtEcgFd].revents = 0;

    pollArray[ProbeTempFd].fd = ((struct pollfd) mDauContext.temperaturePollFd).fd;
    pollArray[ProbeTempFd].events = ((struct pollfd) mDauContext.temperaturePollFd).events;
    pollArray[ProbeTempFd].revents = 0;
}

//-----------------------------------------------------------------------------
void DauSignalHandler::logPollResults(struct pollfd (&pollArray)[FdCount])
{
    int     index;

    // Log the error codes for each poll fd that failed
    for (index = 0; index < FdCount; index++)
    {
        if (pollArray[index].revents)
        {
            ALOGE("File Descriptor: %s, revents: 0x%X",
                  mPollFdNames[index],
                  pollArray[index].revents);
        }
    }
}

