//
// Copyright 2016 EchoNous, Inc.
//

#define LOG_TAG "PciDmaBuffer"

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/ion.h>
#include <ion/ion.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <ThorUtils.h>
#include <PciDmaBuffer.h>

#define EIB_IOCTL 0x2
#define EIB_IOCTL_IMPORT _IOWR(EIB_IOCTL, 0, struct eib_data)


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
uint32_t    PciDmaBuffer::mFifoLength = 0x1000000;
uint32_t    PciDmaBuffer::mSequenceLength = 0x400000;
uint32_t    PciDmaBuffer::mDiagnosticsLength = 0x400000;


//-----------------------------------------------------------------------------
PciDmaBuffer::PciDmaBuffer() :
    mBufPtr(nullptr),
    mMapFd(-1)
{
    ALOGD("*** PciDmaBuffer - constructor");
}

//-----------------------------------------------------------------------------
PciDmaBuffer::~PciDmaBuffer()
{
    ALOGD("*** PciDmaBuffer - destructor");
    close();
}

//-----------------------------------------------------------------------------
bool PciDmaBuffer::open()
{
    bool                ret = false;
    int                 rc;
    int                 eibFd = -1;
    int                 ionFd = -1;
    ion_user_handle_t   ionHandle;

    mLock.enter();
    if (mBufPtr)
    {
        // Buffer is already open
        ret = true;
        goto ok_ret;
    }

    // Get ION buffer information from EIB driver
    eibFd = ::open("/dev/eib_ioctl", O_RDWR | O_NONBLOCK);
    if (eibFd < 0)
    {
        ALOGE("unable to open eib_ioctl.  errno = %d\n", errno);
        goto err_ret;
    }

    rc = ioctl(eibFd, EIB_IOCTL_IMPORT, &mEibData);
    if (rc < 0)
    {
        ALOGE("ioctl failed. errno = %d\n", errno);
        goto err_ret;
    }

    // Validate the length of the buffer
    if (mEibData.buf_len < (mFifoLength + mSequenceLength + mDiagnosticsLength))
    {
        ALOGE("Dma Buffer is too small");
        goto err_ret;
    }

    // Use standard ION library calls to map the ION buffer into our
    // process space.
    ionFd = ion_open();
    if (ionFd < 0)
    {
        ALOGE("unable to open ion\n");
        goto err_ret;
    }

    rc = ion_import(ionFd, mEibData.buf_fd, &ionHandle);
    if (rc)
    {
        ALOGE("ion_import failed. rc = %d\n", rc);
        goto err_ret;
    }

    rc = ion_map(ionFd, ionHandle, mEibData.buf_len, PROT_READ | PROT_WRITE,
                 MAP_SHARED, 0, (unsigned char **)&mBufPtr, &mMapFd);
    if (rc)
    {
        ALOGE("ion_map failed. rc = %d\n", rc);
        goto err_ret;
    }

    ion_free(ionFd, ionHandle);

    ret = true;

    ALOGD("Buffer mapped\n");

err_ret:
    if (-1 != ionFd)
    {
        ion_close(ionFd);
    }

    if (-1 != mEibData.buf_fd)
    {
        ::close(mEibData.buf_fd);
        mEibData.buf_fd = -1;
    }

    if (-1 != eibFd)
    {
        ::close(eibFd);
    }

ok_ret:
    mLock.leave();

    return(ret);
}

//-----------------------------------------------------------------------------
bool PciDmaBuffer::open(DauContext *dauContextPtr)
{
    bool                ret = false;
    int                 rc;
    ion_user_handle_t   ionHandle;

    mLock.enter();
    if (mBufPtr)
    {
        // Buffer is already open
        ret = true;
        goto ok_ret;
    }

    if (nullptr == dauContextPtr ||
        -1 == dauContextPtr->eibIoctlFd ||
        -1 == dauContextPtr->ionFd)
    {
        ALOGE("DauContext is invalid");
        goto err_ret;
    }

    // Get ION buffer information from EIB driver

    rc = ioctl(dauContextPtr->eibIoctlFd, EIB_IOCTL_IMPORT, &mEibData);
    if (rc < 0)
    {
        ALOGE("ioctl failed. errno = %d", errno);
        goto err_ret;
    }

    // Validate the length of the buffer
    if (mEibData.buf_len < (mFifoLength + mSequenceLength + mDiagnosticsLength))
    {
        ALOGE("Dma Buffer is too small");
        goto err_ret;
    }

    // Use standard ION library calls to map the ION buffer into our
    // process space.
    rc = ion_import(dauContextPtr->ionFd, mEibData.buf_fd, &ionHandle);
    if (rc)
    {
        ALOGE("ion_import failed. rc = %d\n", rc);
        goto err_ret;
    }

    rc = ion_map(dauContextPtr->ionFd, ionHandle, mEibData.buf_len, PROT_READ | PROT_WRITE,
                 MAP_SHARED, 0, (unsigned char **)&mBufPtr, &mMapFd);
    if (rc)
    {
        ALOGE("ion_map failed. rc = %d\n", rc);
        goto err_ret;
    }

    ion_free(dauContextPtr->ionFd, ionHandle);

    ret = true;

    ALOGD("Buffer mapped\n");

err_ret:
    if (-1 != mEibData.buf_fd)
    {
        ::close(mEibData.buf_fd);
        mEibData.buf_fd = -1;
    }

ok_ret:
    mLock.leave();

    return(ret);
}

//-----------------------------------------------------------------------------
void PciDmaBuffer::close()
{
    mLock.enter();
    if (mBufPtr)
    {
        munmap(mBufPtr, mEibData.buf_len);
        mBufPtr = nullptr;
        ALOGD("Buffer unmapped");
    }
    if (-1 != mMapFd)
    {
        ::close(mMapFd);
        mMapFd = -1;
    }

    mEibData.phys_addr = 0;
    mEibData.buf_len = 0;
    mEibData.buf_fd = -1;

ret:
    mLock.leave();
}

//-----------------------------------------------------------------------------
uint64_t PciDmaBuffer::getPhysicalAddr(const DmaBufferType &bufferType)
{
    uint64_t  retVal = 0;

    if (mEibData.phys_addr)
    {
        switch (bufferType)
        {
            case DmaBufferType::Fifo:
                retVal = mEibData.phys_addr;
                break;

            case DmaBufferType::Sequence:
                retVal = mEibData.phys_addr + mFifoLength;
                break;

            case DmaBufferType::Diagnostics:
                retVal = mEibData.phys_addr + mFifoLength + mSequenceLength;
                break;
        }
    }

    return(retVal);
}

//-----------------------------------------------------------------------------
uint32_t PciDmaBuffer::getBufferLength(const DmaBufferType &bufferType)
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
uint32_t* PciDmaBuffer::getBufferPtr(const DmaBufferType &bufferType)
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
                retVal = mBufPtr + (mFifoLength / sizeof(uint32_t));
                break;

            case DmaBufferType::Diagnostics:
                retVal = mBufPtr + ((mFifoLength + mSequenceLength) / sizeof(uint32_t));
                break;
        }
    }

    return(retVal);
}

