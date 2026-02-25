//
// Copyright 2016 EchoNous, Inc.
//
#include <stdio.h>
#include <stdlib.h>

#ifdef DAU_EMULATED
    #include <EmuDmaBuffer.h>
#else
    #include <PciDmaBuffer.h>
#endif

#include <ThorUtils.h>
#include <utils/Log.h>
#include <sys/param.h>

int main(int argc, char *argv[])
{
    int         ret = 0;
#ifdef DAU_EMULATED
    DmaBuffer&  dmaBuffer = EmuDmaBuffer::getInstance();
#else
    PciDmaBuffer    dmaBuffer;
#endif
    uint32_t*   bufPtr;

    if (!dmaBuffer.open())
    {
        printf("Unable to map dma buffer\n");
        goto err_ret;
    }

    // Open a second time to test refcounting
    if (!dmaBuffer.open())
    {
        printf("Unable to map dma buffer\n");
        goto err_ret;
    }

    bufPtr = dmaBuffer.getBufferPtr(DmaBufferType::Fifo);

    printf("Fifo:\n");
    printf("\tPhysical addr: 0x%016lX\n", dmaBuffer.getPhysicalAddr(DmaBufferType::Fifo));
    printf("\tBuffer Len:    0x%016X\n", dmaBuffer.getBufferLength(DmaBufferType::Fifo));
    printf("\tBuffer addr:   0x%016zX\n", (size_t) bufPtr);

    printf("Sequence:\n");
    printf("\tPhysical addr: 0x%016lX\n", dmaBuffer.getPhysicalAddr(DmaBufferType::Sequence));
    printf("\tBuffer Len:    0x%016X\n", dmaBuffer.getBufferLength(DmaBufferType::Sequence));
    printf("\tBuffer addr:   0x%016zX\n", (size_t) dmaBuffer.getBufferPtr(DmaBufferType::Sequence));

    printf("Diagnostics:\n");
    printf("\tPhysical addr: 0x%016lX\n", dmaBuffer.getPhysicalAddr(DmaBufferType::Diagnostics));
    printf("\tBuffer Len:    0x%016X\n", dmaBuffer.getBufferLength(DmaBufferType::Diagnostics));
    printf("\tBuffer addr:   0x%016zX\n", (size_t) dmaBuffer.getBufferPtr(DmaBufferType::Diagnostics));

    printf("\nRead results (physical addr):\n");
    ALOGD("Read results (physical addr):");
    printBuffer32(dmaBuffer.getBufferPtr(DmaBufferType::Fifo), 
                  MIN(dmaBuffer.getBufferLength(DmaBufferType::Fifo), 50),
                  dmaBuffer.getPhysicalAddr(DmaBufferType::Fifo));

    logBuffer32(dmaBuffer.getBufferPtr(DmaBufferType::Fifo), 
                MIN(dmaBuffer.getBufferLength(DmaBufferType::Fifo), 50),
                dmaBuffer.getPhysicalAddr(DmaBufferType::Fifo));

    printf("\nWriting passed data\n");
    ALOGD("\nWriting passed data");
    if (2 == argc)
    {
        bufPtr[1] = strtol(argv[1], 0, 16);
    }
    else
    {
        bufPtr[1] = 0xBADDCAFE;

        for (int pos = 2; pos < 32; pos++)
        {
            bufPtr[pos] = pos;
        }
    }

    printf("\nRead results (virtual addr):\n");
    ALOGD("Read results (virtual addr):");
    printBuffer32(dmaBuffer.getBufferPtr(DmaBufferType::Fifo), 
                  MIN(dmaBuffer.getBufferLength(DmaBufferType::Fifo), 
                      50));

    logBuffer32(dmaBuffer.getBufferPtr(DmaBufferType::Fifo), 
                MIN(dmaBuffer.getBufferLength(DmaBufferType::Fifo), 
                    50));

    // Close twice since we opened twice
    dmaBuffer.close();
    dmaBuffer.close();

    ALOGD("Exiting");

err_ret:
    return(ret);
}
