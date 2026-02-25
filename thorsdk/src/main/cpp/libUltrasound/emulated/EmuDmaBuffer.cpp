//
// Copyright 2016 EchoNous, Inc.
//
// ----------------------------------------------------------------------------
//
// This is intended for use on commercial tablets for developemnt / demo uses,
// that do not have the Thor created ION buffer.
// 
// This singleton class uses a refcounting mechanism for accessing (mapping) the
// emualted DMA buffer.  Only the first call to open() will map the buffer.
// Subsequent calls to open() will only increment the refcount.
//
// Each call to close() will decrement the refcount and when reaches 0, the
// buffer will be unmapped.
//
// ----------------------------------------------------------------------------

#define LOG_TAG "EmuDmaBuffer"

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <ion/ion.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ThorUtils.h>
#include <EmuDmaBuffer.h>


#ifndef ION_SYSTEM_HEAP_ID
#define ION_SYSTEM_HEAP_ID 25
#endif

#ifndef ION_HEAP
#define ION_HEAP(bit) (1 << (bit))
#endif

//
// The DmaBuffer is divided into three sections as follows:
//
//  ---------------------------
//  |          Fifo           |
//  ---------------------------
//  |        Sequence         |
//  ---------------------------
//  |       Diagnostics       |
//  ---------------------------
//
// Note: Lengths are in bytes
uint32_t    EmuDmaBuffer::mFifoLength = 0x7D0000;
uint32_t    EmuDmaBuffer::mSequenceLength = 0x0;
uint32_t    EmuDmaBuffer::mDiagnosticsLength = 0x0;

uint32_t    EmuDmaBuffer::mBufferLength = mFifoLength + mSequenceLength + mDiagnosticsLength;


//-----------------------------------------------------------------------------
DmaBuffer& EmuDmaBuffer::getInstance()
{
    static EmuDmaBuffer dmaBuffer;

    return(dmaBuffer);
}

//-----------------------------------------------------------------------------
EmuDmaBuffer::EmuDmaBuffer() :
    mOpenCount(0),
    mBufPtr(nullptr),
    mIonFd(-1),
    mMapFd(-1)
{
}

//-----------------------------------------------------------------------------
EmuDmaBuffer::~EmuDmaBuffer()
{
    mLock.enter();
    if (mOpenCount)
    {
        // Ensure resources are released
        mOpenCount = 1;
    }
    mLock.leave();

    close();
}


//-----------------------------------------------------------------------------
bool EmuDmaBuffer::open()
{
    bool                ret = false;
    int                 rc;
    int                 ionFd = -1;

    mLock.enter();
    if (mOpenCount)
    {
        // Buffer is already open, just increment our refcount
        mOpenCount++;
        ret = true;
        goto ok_ret;
    }

    // Use standard ION library calls to map the ION buffer into our
    // process space.
    ionFd = ion_open();
    if (ionFd < 0)
    {
        ALOGE("unable to open ion\n");
        goto err_ret;
    }

    rc = ion_alloc(ionFd, mBufferLength, 4096, ION_HEAP(ION_SYSTEM_HEAP_ID), 0,
                   &mIonHandle);
    if (rc)
    {
        ALOGE("ion_alloc failed. rc = %d\n", rc);
        goto err_ret;
    }

    rc = ion_map(ionFd, mIonHandle, mBufferLength, PROT_READ | PROT_WRITE,
                 MAP_SHARED, 0, (unsigned char **)&mBufPtr, &mMapFd);
    if (rc)
    {
        ALOGE("ion_map failed. rc = %d\n", rc);
        goto err_ret;
    }

    // Put magic value into first location
    mBufPtr[0] = 0xDEADBEEF;

    mOpenCount = 1;
    ret = true;

    ALOGD("Buffer mapped\n");

err_ret:

ok_ret:
    mLock.leave();

    return(ret);
}

//-----------------------------------------------------------------------------
bool EmuDmaBuffer::open(DauContext *dauContextPtr)
{
    bool                ret = false;

    return(ret);
}

//-----------------------------------------------------------------------------
void EmuDmaBuffer::close()
{
    mLock.enter();
    if (mOpenCount)
    {
        mOpenCount--;
    }
    else
    {
        // refcount is already 0, so nothing to do
        goto ret;
    }

    if (!mOpenCount)
    {
        // Last remaining reference, clean up
        if (mBufPtr)
        {
            munmap(mBufPtr, mBufferLength);
            mBufPtr = nullptr;
        }
        if (-1 != mMapFd)
        {
            ::close(mMapFd);
            mMapFd = -1;
        }

        if (-1 != mIonFd)
        {
            ion_free(mIonFd, mIonHandle);
            ::close(mIonFd);
            mIonFd = -1;
        }

        ALOGD("Buffer unmapped");
    }

ret:
    mLock.leave();
}

//-----------------------------------------------------------------------------
uint64_t EmuDmaBuffer::getPhysicalAddr(const DmaBufferType &bufferType)
{
    return((uint64_t)getBufferPtr(bufferType)); 
}

//-----------------------------------------------------------------------------
uint32_t EmuDmaBuffer::getBufferLength(const DmaBufferType &bufferType)
{
    uint32_t  retVal = 0;

    switch (bufferType)
    {
        case DmaBufferType::Fifo:
            retVal = mFifoLength;
            break;

        case DmaBufferType::Sequence:
            retVal = mSequenceLength;
            break;

        case DmaBufferType::Diagnostics:
            retVal = mDiagnosticsLength;
            break;
    }

    return(retVal);
}

//-----------------------------------------------------------------------------
uint32_t* EmuDmaBuffer::getBufferPtr(const DmaBufferType &bufferType)
{
    uint32_t*   retVal = nullptr;

    if (mBufPtr)
    {
        switch (bufferType)
        {
            case DmaBufferType::Fifo:
                retVal = mBufPtr;
                break;

            case DmaBufferType::Sequence:
                if (mSequenceLength)
                {
                    retVal = mBufPtr + (mFifoLength / sizeof(uint32_t));
                }
                break;

            case DmaBufferType::Diagnostics:
                if (mDiagnosticsLength)
                {
                    retVal = mBufPtr + ((mFifoLength + mSequenceLength) / sizeof(uint32_t));
                }
                break;
        }
    }

    return(retVal);
}

