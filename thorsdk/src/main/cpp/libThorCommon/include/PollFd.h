//
// Copyright 2017 EchoNous, Inc.
//
// A specialized class for handling `struct pollfd`.  Will provide proper 
// initialization and reading of the underlying file based on the source.
// Sysfs files, timers, etc. have different ways of signaling poll() and 
// for accessing their values.
// 
// Currently intended for use by DauSignalHandler, but could maybe be made
// more generic later.
//

#pragma once

#ifndef LOG_TAG
#define LOG_TAG "PollFd"
#endif

#include <stdlib.h>
#include <string.h>
#include <memory>
#include <poll.h>
#include <ThorUtils.h>
#include <TriggeredValue.h>

enum class PollFdType
{
    PcieFrame,          // For accessing sysfs file for PCIe Dau irq for new frame
    PcieError,          // For accessing sysfs file for PCIe Dau irq for errors
    Usb,                // Initial Usb access but not currently used
    Timer,              // Keeps a running count for each timer tick
    TriggeredValue,     // For accessing a TriggeredValue object
    Invalid,
};

class PollFd
{
public: // Methods
    //-------------------------------------------------------------------------
    PollFd() :
        mType(PollFdType::Invalid),
        mTimerCount(0),
        mTriggeredValueSPtr(nullptr)
    {
        mPollFd.fd = -1;
        mPollFd.events = 0;
    }

    //-------------------------------------------------------------------------
    ~PollFd()
    {
        ALOGD("%s", __func__);
        clear();
    }

    //-------------------------------------------------------------------------
    void initialize(PollFdType type, int fd)
    {
        clear();
        mType = type;
        mPollFd.fd = fd;

        switch (mType)
        {
            case PollFdType::PcieFrame:
            case PollFdType::PcieError:
                mPollFd.events = POLLPRI;
                break;

            case PollFdType::Usb:
                mPollFd.events = POLLIN;
                mTimerCount = 0;
                break;

            case PollFdType::Timer:
                mPollFd.events = POLLIN;
                mTimerCount = 0;
                break;

            default:
                break;
        }
    }

    //-------------------------------------------------------------------------
    void initialize(const std::shared_ptr<TriggeredValue>& triggeredValue)
    {
        clear();
        mTriggeredValueSPtr = triggeredValue;
        mType = PollFdType::TriggeredValue;
        mPollFd.fd = mTriggeredValueSPtr->getFd();
        mPollFd.events = POLLIN;
    }

    //-------------------------------------------------------------------------
    void prepare()
    {
        if (-1 == mPollFd.fd)
        {
            return;
        }

        switch (mType)
        {
            case PollFdType::PcieFrame:
            case PollFdType::PcieError:
                // Dummy read to prevent initial poll hit
                memset(mReadBuf, 0, sizeof(mReadBuf));
                ::read(mPollFd.fd, &mReadBuf, sizeof(mReadBuf));
                ::lseek(mPollFd.fd, 0, SEEK_SET);
                break;

            case PollFdType::Timer:
                ::read(mPollFd.fd, &mReadBuf, sizeof(mReadBuf));
                mTimerCount = 0;
                break;

            case PollFdType::TriggeredValue:
                if (mTriggeredValueSPtr)
                {
                    mTriggeredValueSPtr->reset();
                }
                break;

            default:
                break;
        }
    }

    //-------------------------------------------------------------------------
    inline uint32_t read()
    {
        uint32_t    value = 0;

        if (-1 == mPollFd.fd)
        {
            goto err_ret; 
        }

        switch (mType)
        {
            case PollFdType::PcieFrame:
                memset(mReadBuf, 0, sizeof(mReadBuf));
                ::read(mPollFd.fd, &mReadBuf, sizeof(mReadBuf));
                lseek(mPollFd.fd, 0, SEEK_SET);
                value = strtol(mReadBuf, 0, 10);
                break;

            case PollFdType::PcieError:
                memset(mReadBuf, 0, sizeof(mReadBuf));
                ::read(mPollFd.fd, &mReadBuf, sizeof(mReadBuf));
                lseek(mPollFd.fd, 0, SEEK_SET);
                value = strtol(mReadBuf, 0, 16);
                ALOGD("Raw error code: 0x%X", value);
                break;

            case PollFdType::Usb:
            {
                uint64_t    readVal = 0;

                ::read(mPollFd.fd, &readVal, sizeof(readVal));
                value = (uint32_t) readVal;
                break;
            }

            case PollFdType::Timer:
                ::read(mPollFd.fd, &mReadBuf, sizeof(mReadBuf));
                value = ++mTimerCount;
                break;

            case PollFdType::TriggeredValue:
                if (mTriggeredValueSPtr)
                {
                    value = mTriggeredValueSPtr->read();
                }
                break;

            default:
                break;
        }

    err_ret:
        return(value);
    }

    //-------------------------------------------------------------------------
    void clear()
    {
        if (-1 != mPollFd.fd)
        {
            // Don't close fd for triggered value as mTriggeredValueSPtr will
            // be cleaned up (and fd closed) when set to null. Otherwise a
            // double-close will occur.
            if (PollFdType::TriggeredValue != mType)
            {
                ::close(mPollFd.fd);
            }
            mPollFd.fd = -1;
        }

        mPollFd.events = 0;
        mTimerCount = 0;
        mTriggeredValueSPtr = nullptr;
    }

    //-------------------------------------------------------------------------
    operator struct pollfd() const
    {
        return(mPollFd); 
    }

private: // Methods to prevent multiple copies of mPollFd when using the
         // operator override above.
    PollFd(const PollFd&);
    PollFd& operator=(const PollFd&);

private: // Properties
    PollFdType          mType;
    struct pollfd       mPollFd;
    size_t              mTimerCount;
    char                mReadBuf[20];   // Sized to hold 64 bit number in ascii

    std::shared_ptr<TriggeredValue>     mTriggeredValueSPtr;
};
