//
// Copyright (C) 2016 EchoNous, Inc.
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <PciDmaBuffer.h>
#include <ThorUtils.h>

#ifndef DEFFILEMODE
#define DEFFILEMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) /* 0666 */
#endif

const char* gFileParam = "-f";
const char* gFifoType = "fifo";
const char* gSequenceType = "seq";
const char* gDiagnosticsType = "diag";

//-----------------------------------------------------------------------------
void usage()
{
    printf("Usage: dmaread <type> <addr> [length] [-f <output_file>]\n");
    printf("\n  type\t\tBuffer type.  Select one of: fifo | seq | diag\n");
    printf("  addr\t\tHex value for the starting address to read\n");
    printf("  length\tThe number of 32-bit values to read\n");
    printf("       \t\tDefault is to read one value\n");
    printf("  output_file\tResulting data will be stored in this file. If the file\n");
    printf("   \t\talready exists, it will be truncated and overwritten.\n");
}

//-----------------------------------------------------------------------------
int writeToFile(char* path, uint32_t* bufPtr, uint32_t length)
{
    int         retVal = -1;
    int         fd = -1;
    uint32_t*   destAddressPtr = nullptr;
    uint32_t*   curDestPtr = nullptr;
    uint32_t*   curSrcPtr = bufPtr;
    uint32_t    fileLength = length * sizeof(uint32_t);
    uint32_t    numWords = length;

    fd = open(path, O_RDWR | O_CREAT | O_TRUNC, DEFFILEMODE);
    if (-1 == fd)
    {
        printf("Unable to create %s\n", path);
        goto err_ret;
    }

    if (ftruncate(fd, fileLength))
    {
        printf("Unable to grow file to needed length\n");
        goto err_ret;
    }
    lseek(fd, 0, SEEK_SET);

    destAddressPtr = (uint32_t*) mmap(0,
                                      fileLength,
                                      PROT_WRITE,
                                      MAP_SHARED,
                                      fd,
                                      0);
    if ((void*) -1 == destAddressPtr)
    {
        printf("Failed to map data file %s.  errno = %d\n", path, errno);
        goto err_ret;
    }

    curDestPtr = destAddressPtr;

    while (numWords--)
    {
        *curDestPtr++ = *curSrcPtr++;
    }

    retVal = 0;

err_ret:
    if (nullptr != destAddressPtr)
    {
        munmap(destAddressPtr, fileLength);
        destAddressPtr = nullptr;
    }

    if (-1 != fd)
    {
        close(fd);
        fd = -1;
    }

    return(retVal);
}

//-----------------------------------------------------------------------------
bool getBufferType(const char* typeName, DmaBufferType& bufferType)
{
    bool    retVal = false;

    if (0 == strncasecmp(gFifoType, typeName, strlen(gFifoType)))
    {
        bufferType = DmaBufferType::Fifo;
        retVal = true;
    }
    else if (0 == strncasecmp(gSequenceType, typeName, strlen(gSequenceType)))
    {
        bufferType = DmaBufferType::Sequence;
        retVal = true;
    }
    else if (0 == strncasecmp(gDiagnosticsType, typeName, strlen(gDiagnosticsType)))
    {
        bufferType = DmaBufferType::Diagnostics;
        retVal = true;
    }

    return(retVal);
}

//-----------------------------------------------------------------------------
int main(int argc, char** argv)
{
    int             retVal = -1;
    uint32_t        addr;
    uint32_t        length = 1;
    PciDmaBuffer    dmaBuffer;
    char*           outFile = nullptr;
    DmaBufferType   bufferType;
    size_t          physAddress;
    uint32_t        bufLength;
    uint32_t*       bufPtr = nullptr;

    // Check the mandatory parameters
    if (3 > argc)
    {
        usage();
        goto err_ret;
    }

    if (!dmaBuffer.open())
    {
        printf("Unable to access Dma Buffer\n");
        goto err_ret;
    }

    if (!getBufferType(argv[1], bufferType))
    {
        usage();
        goto err_ret;
    }
    addr = strtoul(argv[2], 0, 16);

    switch (argc)
    {
        // Type + Address only.  Already got them
        case 3:
            break;

        // Type + Address + length
        case 4:
            length = strtol(argv[3], 0, 0);
            break;

        // Type + Address + output file
        case 5:
            if (0 == strncasecmp(gFileParam, argv[3], strlen(gFileParam)))
            {
                outFile = argv[4];
            }
            else
            {
                usage();
                goto err_ret;
            }
            break;

        // Type Address + length + output file
        case 6:
            length = strtol(argv[3], 0, 0);

            if (0 == strncasecmp(gFileParam, argv[4], strlen(gFileParam)))
            {
                outFile = argv[5];
            }
            else
            {
                usage();
                goto err_ret;
            }
            break;

        // No bueno
        default:
            usage();
            goto err_ret;
            break;
    }

    physAddress = dmaBuffer.getPhysicalAddr(bufferType);
    bufLength = dmaBuffer.getBufferLength(bufferType);

    // Validate the address is in range of selected buffer
    if (physAddress > addr || addr >= (physAddress + bufLength))
    {
        printf("Specified address is out of range\n");
        goto err_ret;
    }

    if ((addr + (length * sizeof(uint32_t))) > (physAddress + bufLength))
    {
        printf("Specified length is too large\n");
        goto err_ret;
    }

    bufPtr = dmaBuffer.getBufferPtr(bufferType) + ((addr - physAddress) / sizeof(uint32_t));

    if (nullptr == outFile)
    {
        printBuffer32(bufPtr, length, addr);
        retVal = 0;
    }
    else
    {
        retVal = writeToFile(outFile, bufPtr, length);
    }

err_ret:
    dmaBuffer.close();

    return(retVal);
}
