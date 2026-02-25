//
// Copyright (C) 2016 EchoNous, Inc.
//

#include <stdio.h>

#include <PciDmaBuffer.h>
#include <ThorUtils.h>

//-----------------------------------------------------------------------------
int main(int argc, char** argv)
{
    UNUSED(argc);
    UNUSED(argv);

    size_t          length;
    PciDmaBuffer    dmaBuffer;

    if (!dmaBuffer.open())
    {
        printf("Unable to access Dma Buffer\n");
        return(-1);
    }

    printf("Fifo Buffer:\n");
    printf("\tAddress: 0x%016lX\n", dmaBuffer.getPhysicalAddr(DmaBufferType::Fifo));
    length = dmaBuffer.getBufferLength(DmaBufferType::Fifo) / sizeof(uint32_t);
    printf("\tLength (words):  %lu (0x%08lX)\n\n", length, length);

    printf("Sequence Buffer:\n");
    printf("\tAddress: 0x%016lX\n", dmaBuffer.getPhysicalAddr(DmaBufferType::Sequence));
    length = dmaBuffer.getBufferLength(DmaBufferType::Sequence) / sizeof(uint32_t);
    printf("\tLength (words):  %lu (0x%08lX)\n\n", length, length);

    printf("Diagnostics Buffer:\n");
    printf("\tAddress: 0x%016lX\n", dmaBuffer.getPhysicalAddr(DmaBufferType::Diagnostics));
    length = dmaBuffer.getBufferLength(DmaBufferType::Diagnostics) / sizeof(uint32_t);
    printf("\tLength (words):  %lu (0x%08lX)\n\n", length, length);

    dmaBuffer.close();

    return(0);
}
