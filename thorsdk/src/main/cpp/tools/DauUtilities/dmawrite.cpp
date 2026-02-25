//
// Copyright (C) 2016 EchoNous, Inc.
//

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <PciDmaBuffer.h>
#include <ThorUtils.h>

const char* gFileParam = "-f";
const char* gSetParam = "-s";
const char* gFifoType = "fifo";
const char* gSequenceType = "seq";
const char* gDiagnosticsType = "diag";

//-----------------------------------------------------------------------------
void usage()
{
    printf("Usage:\n");
    printf("  dmawrite <type> <addr> <value>\n");
    printf("  dmawrite <type> <addr> -f <input_file>\n");
    printf("  dmawrite <type> <addr> -s <value> <length>\n");
    printf("\n    type\tBuffer type.  Select one of: fifo | seq | diag\n");
    printf("    addr\tHex value of the address to modify\n");
    printf("    value\tHex value to write to address\n");
    printf("    input_file\tFile containing raw data. The number of words to write\n");
    printf("    \t\tis determined from the file size.\n");
    printf("    length\tThe number of 32-bit values to write\n");
}

//-----------------------------------------------------------------------------
int readFromFile(char* path, uint32_t* bufPtr, uint32_t maxLength)
{
    int         retVal = -1;
    int         fd = -1;
    int         ret;
    struct stat statBuf;
    uint32_t*   srcAddressPtr = nullptr;
    uint32_t*   curDestPtr = bufPtr;
    uint32_t*   curSrcPtr;
    uint32_t    numWords;

    fd = open(path, O_RDONLY);
    if (-1 == fd)
    {
        printf("Unable to open %s\n", path);
        goto err_ret;
    }

    ret = fstat(fd, &statBuf);
    if (ret)
    {
        printf("Unable to get size of %s\n", path);
        goto err_ret;
    }

    // Verify the data will fit
    if (statBuf.st_size > maxLength)
    {
        printf("Too much data to fit\n");
        goto err_ret;
    }

    srcAddressPtr = (uint32_t*) mmap(0,
                                     statBuf.st_size,
                                     PROT_READ,
                                     MAP_PRIVATE,
                                     fd,
                                     0);
    if ((void*) -1 == srcAddressPtr)
    {
        printf("Failed to map data file %s\n", path);
        goto err_ret;
    }

    numWords = statBuf.st_size / sizeof(uint32_t);
    curSrcPtr = srcAddressPtr;

    while (numWords--)
    {
        *curDestPtr++ = *curSrcPtr++;
    }

    retVal = 0;

err_ret:
    if (nullptr != srcAddressPtr)
    {
        munmap(srcAddressPtr, statBuf.st_size);
        srcAddressPtr = nullptr;
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
    PciDmaBuffer    dmaBuffer;
    uint32_t        data = 0;
    uint32_t        addr;
    DmaBufferType   bufferType;
    char*           outFile = nullptr;
    size_t          physAddress;
    uint32_t        bufLength;
    uint32_t*       bufPtr = nullptr;
    uint32_t        length = 0;

    // Check the mandatory parameters
    if (4 > argc)
    {
        printf("not enough parameters\n");
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
        printf("Unknown buffer type\n");
        usage();
        goto err_ret;
    }
    addr = strtoul(argv[2], 0, 16);

    switch (argc)
    {
        // Type + Address + Value
        case 4:
            data = strtol(argv[3], 0, 16);
            break;

        // Type + Address + input file
        case 5:
            if (0 == strncasecmp(gFileParam, argv[3], strlen(gFileParam)))
            {
                outFile = argv[4];
            }
            else
            {
                printf("Unknown file parameter\n");
                usage();
                goto err_ret;
            }
            break;

        // Type + Address + Value + Length
        case 6:
            if (0 == strncasecmp(gSetParam, argv[3], strlen(gSetParam)))
            {
                data = strtol(argv[4], 0, 16);
                length = strtol(argv[5], 0, 0);
            }
            else
            {
                printf("Unknown file parameter\n");
                usage();
                goto err_ret;
            }
            break;

        // No bueno
        default:
            printf("Wrong parameter count: %d\n", argc);
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

    bufPtr = dmaBuffer.getBufferPtr(bufferType) + ((addr - physAddress) / sizeof(uint32_t));

    if (length > 0)
    {
        if ((length * sizeof(uint32_t) ) > (physAddress + bufLength - addr))
        {
            printf("'length' is too large for the specified address\n");
            goto err_ret;
        }

        while (length--)
        {
            *bufPtr++ = data;
        }
    }
    else if (nullptr == outFile)
    {
        *bufPtr = data;
        retVal = 0;
    }
    else
    {
        retVal = readFromFile(outFile,
                              bufPtr,
                              (physAddress + bufLength - addr));
    }

err_ret:
    dmaBuffer.close();

    return retVal;
}
