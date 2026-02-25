//
// Copyright (C) 2016 EchoNous, Inc.
//

#define LOG_TAG "PciDauMemory"

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <CriticalSection.h>
#include <PciDauMemory.h>
#include <ThorUtils.h>

//#define DAU_DEVICE_PATH "/sys/bus/pci/drivers/dau_pci/0001:01:00.0"
#define DAU_DEVICE_PATH "/sys/bus/pci/drivers/dau_pci/0000:01:00.0"
#define RESOURCE_PATH DAU_DEVICE_PATH"/resource"

#ifdef TBENCH_WRITE_LOGGING
#define WRITE_LOG_PATH "/data/data/com.echonous.liveimagedemo/DauWriteLog.txt"
#endif

const char* const PciDauMemory::mBarResources[6] =
{
    RESOURCE_PATH"0",
    RESOURCE_PATH"1",
    RESOURCE_PATH"2",
    RESOURCE_PATH"3",
    RESOURCE_PATH"4",
    RESOURCE_PATH"5"
};

// BUGBUG: This should ideally come from DauRegisters.h
const uint32_t PciDauMemory::mBar0Offset = 0x10800000;


//-----------------------------------------------------------------------------
PciDauMemory::PciDauMemory() : 
    mDauContextPtr(nullptr),
    mMemoryPtr(nullptr),
    mMemLength(0),
    mBar(0),
    mFd(-1),
    mUseInternalAddress(true),
    mLogStream(NULL)
{
    ALOGD("*** PciDauMemory - constructor");
}

//-----------------------------------------------------------------------------
PciDauMemory::~PciDauMemory()
{
    unmap();
    ALOGD("*** PciDauMemory - destructor");
}


//-----------------------------------------------------------------------------
void PciDauMemory::selectBar(uint8_t bar)
{
    if (bar < 7 && nullptr == mMemoryPtr)
    {
        mBar = bar;
    }
}

//-----------------------------------------------------------------------------
void PciDauMemory::useInternalAddress(bool useInternal)
{
    mLock.enter();
    mUseInternalAddress = useInternal;
    mLock.leave();
}


//-----------------------------------------------------------------------------
bool PciDauMemory::map()
{
    bool    retVal = false;
    int     fd = -1;

    ALOGD("Mapping BAR%u", mBar);

    mLock.enter();
    if (nullptr != mMemoryPtr)
    {
        // Already mapped
        retVal = true;
        goto err_ret;
    }

    // Map memory from selected BAR
    if (nullptr == mDauContextPtr)
    {
        mFd = ::open(mBarResources[mBar], O_RDWR | O_SYNC);
        if (-1 == mFd)
        {
            ALOGE("Unable to open resource path: errno = %d", errno);
            goto err_ret;
        }
        fd = mFd;
    }
    else
    {
        switch (mBar)
        {
            case 0:
                fd = mDauContextPtr->dauResourceFd;
                break;

            case 2:
                fd = mDauContextPtr->dauResource2Fd;
                break;

            case 4:
                fd = mDauContextPtr->dauResource4Fd;
                break;

            default:
                ALOGE("Unsupported BAR%d", mBar);
                break;
        }
        if (-1 == fd)
        {
            ALOGE("DauContext has invalid fd");
            goto err_ret;
        }

#ifdef TBENCH_WRITE_LOGGING
        if (mDauContextPtr->enableWriteLogging)
        {
            // Ignore any errors that occur here
            mLogStream = ::fopen(WRITE_LOG_PATH, "w+");
            if (NULL == mLogStream)
            {
                ALOGE("Unable to open write log: %s, errno = %d", WRITE_LOG_PATH, errno);
            }
        }
#endif
    }

    mMemLength = getMemLength(fd);
    if (!mMemLength)
    {
        goto err_ret;
    }
    ALOGD("getMemLength() returns %zu", mMemLength);

    mMemoryPtr = (uint32_t*) mmap(0,
                                  mMemLength,
                                  PROT_READ | PROT_WRITE,
                                  MAP_SHARED,
                                  fd,
                                  0);
    if ((void*) -1 == mMemoryPtr)
    {
        ALOGE("mmap failed. errno = %d", errno);
        goto err_ret;
    }

    retVal = true;

    ALOGD("Dau Memory mapped");

err_ret:
    if (-1 != mFd)
    {
        ::close(mFd);
        mFd = -1;
    }
    mLock.leave();

    return (retVal);
}

//-----------------------------------------------------------------------------
bool PciDauMemory::map(DauContext* dauContextPtr)
{
    mDauContextPtr = dauContextPtr;

    return(map()); 
}

//-----------------------------------------------------------------------------
void PciDauMemory::unmap()
{
    mLock.enter();
    // Cleanup mapped memory with BAR
    if (mMemoryPtr)
    {
        munmap(mMemoryPtr, mMemLength);
        mMemoryPtr = nullptr;
        mMemLength = 0;
        ALOGD("BAR%u unmapped", mBar);
    }

    if (-1 != mFd)
    {
        ::close(mFd);
        mFd = -1;
    }

#ifdef TBENCH_WRITE_LOGGING
    if (NULL != mLogStream)
    {
        ::fclose(mLogStream);
        mLogStream = NULL;
    }
#endif

ret:
    mLock.leave();
}

//-----------------------------------------------------------------------------
size_t PciDauMemory::getMapSize()
{
    return(mMemLength); 
}

//-----------------------------------------------------------------------------
bool PciDauMemory::read(uint32_t dauAddress, uint32_t* destAddress, uint32_t numWords)
{
    bool        retVal = false;
    uint32_t*   srcAddressPtr = nullptr;
    uint32_t*   destAddressPtr = destAddress;
    uint32_t    offset = dauAddress;
    uint32_t    words = numWords;

    mLock.enter();
    if (nullptr == mMemoryPtr)
    {
        ALOGE("Memory is not mapped");
        goto err_ret;
    }

    if (2 == mBar)
    {
        offset -= 0x10000000;
        if ((offset / sizeof(uint32_t)) + numWords > (mMemLength / sizeof(uint32_t)))
        {
            ALOGE("Specified length is too large for BAR2");
            goto err_ret;
        }
    }
    else
    {
        if (0 == mBar && mUseInternalAddress) {
            offset -= mBar0Offset;
        }

        if ((offset / sizeof(uint32_t)) + numWords > (mMemLength / sizeof(uint32_t)))
        {
            ALOGE("Specified length is too large");
            goto err_ret;
        }
    }

    srcAddressPtr = mMemoryPtr + (offset / sizeof(uint32_t));

    while (words--)
    {
        // Without this small sleep, accessing certain addresses causes a 
        // SIGBUS fault.  Suspect this happens when crossing a page boundary.
        usleep(10);

        *destAddressPtr++ = *srcAddressPtr++;
    }

    retVal = true;

err_ret:
    mLock.leave();
    return(retVal);
}

//-----------------------------------------------------------------------------
bool PciDauMemory::write(uint32_t* srcAddress, uint32_t dauAddress, uint32_t numWords)
{
    bool        retVal = false;
    uint32_t*   destAddressPtr = nullptr;
    uint32_t*   srcAddressPtr = srcAddress;
    uint32_t    offset = dauAddress;
    uint32_t    words = numWords;

    mLock.enter();
    if (nullptr == mMemoryPtr)
    {
        ALOGE("Memory is not mapped");
        goto err_ret;
    }


    if (2 == mBar)
    {
        offset -= 0x10000000;
        if ((offset / sizeof(uint32_t)) + numWords > (mMemLength / sizeof(uint32_t)))
        {
            ALOGE("Specified length is too large for BAR2");
            goto err_ret;
        }
    }
    else
    {
        if (0 == mBar && mUseInternalAddress)
        {
            offset -= mBar0Offset;
        }

        if ((offset / sizeof(uint32_t)) + numWords > (mMemLength / sizeof(uint32_t)))
        {
            ALOGE("Specified length is too large");
            goto err_ret;
        }

    }

    destAddressPtr = mMemoryPtr + (offset / sizeof(uint32_t));

    while (words--)
    {
#ifdef TBENCH_WRITE_LOGGING
        // Write Log for test bench
        if (NULL != mLogStream)
        {
            ::fprintf(mLogStream, "0 %08X %08X\n",
                      (dauAddress + ((numWords - (words + 1)) * sizeof(uint32_t))),
                      *srcAddressPtr);
        }
#endif

        *destAddressPtr++ = *srcAddressPtr++;
    }

    retVal = true;

err_ret:
    mLock.leave();
    return(retVal);
}

//-----------------------------------------------------------------------------
size_t PciDauMemory::getMemLength(int fd)
{
    size_t          memLength = 0;
    int             ret;
    struct stat     statBuf;

    ret = fstat(fd, &statBuf);

    memLength = statBuf.st_size;

    if (0 != (memLength % getpagesize()))
    {
        ALOGE("Specified memory length doesn't end on page boundary.  %zu",
              memLength);
        memLength = 0;
    }

    return(memLength); 
}

