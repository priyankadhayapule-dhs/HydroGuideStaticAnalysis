//
// Copyright 2016 EchoNous, Inc.
//

#define LOG_TAG "Event"

#include <sys/eventfd.h>
#include <stdint.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <ThorUtils.h>
#include <Event.h>

Event::Event() :
    mFd(-1),
    mEventMode(EventMode::ManualReset)
{
}

Event::~Event()
{
    if (-1 != mFd)
    {
        close(mFd);
        mFd = -1;
    }
}

bool Event::init(const EventMode& eventMode, bool isSet)
{
    if (-1 != mFd)
    {
        // Event already initialized
        return (true); 
    }

    mFd = eventfd(0, EFD_NONBLOCK);
    if (-1 == mFd)
    {
        ALOGE("Failed to initialize event: errno = %d\n", errno);
        return (false);
    }
    else
    {
        mEventMode = eventMode;
        if (isSet)
        {
            set();
        }

        return (true);
    }
}

void Event::set()
{
    if (-1 != mFd)
    {
        uint64_t    value = 1;
        write(mFd, &value, sizeof(value));
    }
    else
    {
        ALOGE("event not initialized");
    }
}

void Event::reset()
{
    if (-1 != mFd)
    {
        uint64_t    value;
        read(mFd, &value, sizeof(value));
    }
    else
    {
        ALOGE("event not initialized");
    }
}

bool Event::wait(int msecTimeout)
{
    bool    retVal = false;

    if (-1 != mFd)
    {
        struct pollfd   pollFd;
        int             ret;

        pollFd.fd = mFd;
        pollFd.events = POLLIN;
        pollFd.revents = 0;

        ret = poll(&pollFd, 1, msecTimeout);
        if (-1 == ret)
        {
            ALOGE("poll failed errno = %d\n", errno);
        }
        else if (ret > 0)
        {
            if (EventMode::AutoReset == mEventMode)
            {
                reset();
            }
            retVal = true;
        }
    }
    else
    {
        ALOGE("event not initialized");
    }

    return (retVal); 
}
