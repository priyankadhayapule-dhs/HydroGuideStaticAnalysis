//
// Copyright 2017 EchoNous, Inc.
//

#define LOG_TAG "ThorTestSignalManager"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string>
#include <ThorUtils.h>
#include <ThorTestSignalManager.h>


//-----------------------------------------------------------------------------
ThorTestSignalManager& ThorTestSignalManager::getInstance()
{
    static ThorTestSignalManager signalMgr;

    return(signalMgr);
}

//-----------------------------------------------------------------------------
ThorTestSignalManager::ThorTestSignalManager() :
    mOpenCount(0)
{
    for (int index = 0; index < mSignalCount; index++)
    {
        mFds[index] = -1;
    }
}

//-----------------------------------------------------------------------------
ThorTestSignalManager::~ThorTestSignalManager()
{
    mLock.enter();
    if (mOpenCount)
    {
        mOpenCount = 1;
    }
    mLock.leave();

    close();
}


//-----------------------------------------------------------------------------
bool ThorTestSignalManager::open()
{
    bool        retVal = false;
    std::string path = "/sys/devices/dau/test_gpioX/value";
    int         XLoc = path.find('X');

    mLock.enter();
    if (mOpenCount)
    {
        mOpenCount++;
        retVal = true;
        goto ok_ret;
    }

    for (int index = 0; index < mSignalCount; index++)
    {
        path.replace(XLoc, 1, 1, index + '1');
        ALOGD("TestSignal path: %s", path.c_str());
        mFds[index] = ::open(path.c_str(), O_WRONLY);
        if (-1 == mFds[index])
        {
            ALOGE("Unable to open %s", path.c_str());
            ALOGE("errno = %d", errno);
            goto err_ret;
        }
        setSignal((ThorTestSignal)index, false);
    }

    mOpenCount = 1;
    retVal = true;

    ALOGD("Thor TestSignals opened");

err_ret:
ok_ret:
    mLock.leave();
    return(retVal);
}

//-----------------------------------------------------------------------------
// This method is currently same as non-context variant.  Currently GPIO access 
// for test signals are world read/writable.  If that changes then a DauContext
// will be necessary for access.
bool ThorTestSignalManager::open(DauContext* dauContextPtr)
{
    UNUSED(dauContextPtr);

    return(open());
}

//-----------------------------------------------------------------------------
void ThorTestSignalManager::close()
{
    mLock.enter();
    if (mOpenCount)
    {
        mOpenCount--;
    }
    else
    {
        goto ret;
    }

    if (!mOpenCount)
    {
        for (int index = 0; index < mSignalCount; index++)
        {
            if (-1 != mFds[index])
            {
                ::close(mFds[index]);
                mFds[index] = -1;
            }
        }

        ALOGD("Thor TestSignals closed");
    }

ret:
    mLock.leave();
}

//-----------------------------------------------------------------------------
void ThorTestSignalManager::setSignal(const ThorTestSignal& signal, bool isSet)
{
    int     index = (int) signal;
    int     fd = mFds[index];

    mLock.enter();
    if (-1 != fd)
    {
        ::write(fd, isSet ? "1" : "0", 1);
    }
    mLock.leave();
}

//-----------------------------------------------------------------------------
void ThorTestSignalManager::pulseSignal(const ThorTestSignal& signal)
{
    setSignal(signal, true);
    setSignal(signal, false);
}

