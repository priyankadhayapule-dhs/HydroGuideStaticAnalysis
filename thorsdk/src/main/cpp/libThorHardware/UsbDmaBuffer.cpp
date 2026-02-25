//
// Copyright 2017 EchoNous, Inc.
//

#define LOG_TAG "UsbDmaBuffer"

#include <ThorUtils.h>
#include <UsbDmaBuffer.h>

uint32_t    UsbDmaBuffer::mFifoLength = 0x1000000;

//-----------------------------------------------------------------------------
UsbDmaBuffer::UsbDmaBuffer() :
    mFifoBufferPtr(nullptr)
{
    ALOGD("*** UsbDmaBuffer - constructor");
}

//-----------------------------------------------------------------------------
UsbDmaBuffer::~UsbDmaBuffer()
{
    ALOGD("*** UsbDmaBuffer - destructor");
    close();
}

//-----------------------------------------------------------------------------
bool UsbDmaBuffer::open()
{
    bool    retVal = false;

    if (nullptr != mFifoBufferPtr)
    {
        retVal = true;
        goto ok_ret;
    }

    mFifoBufferPtr = new uint32_t[mFifoLength];
    retVal = true;

ok_ret:
    return(retVal);
}

//-----------------------------------------------------------------------------
bool UsbDmaBuffer::open(DauContext* dauContextPtr)
{
    UNUSED(dauContextPtr);

    return(open()); 
}

//-----------------------------------------------------------------------------
void UsbDmaBuffer::close()
{
    if (nullptr != mFifoBufferPtr)
    {
        delete [] mFifoBufferPtr;
        mFifoBufferPtr = nullptr;
    }
}

//-----------------------------------------------------------------------------
uint64_t UsbDmaBuffer::getPhysicalAddr(const DmaBufferType& bufferType)
{
    UNUSED(bufferType);

    return(0); 
}

//-----------------------------------------------------------------------------
uint32_t UsbDmaBuffer::getBufferLength(const DmaBufferType& bufferType)
{
    uint32_t  retVal = 0;

    switch (bufferType)
    {
        case DmaBufferType::Fifo:
            retVal = mFifoLength;
            break;

        case DmaBufferType::Sequence:
            retVal = mFifoLength;
            break;

        case DmaBufferType::Diagnostics:
            retVal = mFifoLength;
            break;
    }

    return(retVal);
}

//-----------------------------------------------------------------------------
uint32_t* UsbDmaBuffer::getBufferPtr(const DmaBufferType& bufferType)
{
    uint32_t*   retVal = nullptr;

    switch (bufferType)
    {
        case DmaBufferType::Fifo:
            retVal = mFifoBufferPtr;
            break;

        case DmaBufferType::Sequence:
            retVal = mFifoBufferPtr;
            break;

        case DmaBufferType::Diagnostics:
            retVal = mFifoBufferPtr;
            break;
    }

    return(retVal); 
}

