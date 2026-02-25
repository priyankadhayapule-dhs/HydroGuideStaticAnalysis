//
// Copyright (C) 2016 EchoNous, Inc.
//

#define LOG_TAG "dauread"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <string>

#include <PciDauMemory.h>
#include <DauPower.h>
#include <ThorUtils.h>

#ifndef DEFFILEMODE
#define DEFFILEMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) /* 0666 */
#endif

#define DEBUG_CMD_LINE 1

//-----------------------------------------------------------------------------
void usage()
{
    printf("Usage: dauread [-b bar] [-a] [-f <output_file>] <addr> [length]\n");
    printf("\n  addr\t\t\tHex value for the starting address to read\n");
    printf("  length\t\tThe number of 32-bit values to read.   If not\n");
    printf("       \t\t\tspecified, the default is to read one value.\n");
    printf("  -b bar\t\tSpecify the BAR to read from.  If not specified, the default\n");
    printf("       \t\t\tis from BAR0.\n");
    printf("  -a\t\t\tUse BAR address instead of internal address.  Default is\n");
    printf("  \t\t\tto use internal address.\n");
    printf("  -f output_file\tResulting data will be stored in this file.  If the file\n");
    printf("\t\t\talready exists, it will be truncated and overwritten.\n");
}

//-----------------------------------------------------------------------------
int writeToFile(char* path, DauMemory& dauMem, uint32_t srcAddress, uint32_t length)
{
    int         retVal = -1;
    int         fd = -1;
    uint32_t*   destAddressPtr = nullptr;
    uint32_t    fileLength = length * sizeof(uint32_t);

    fd = open(path, O_RDWR | O_CREAT | O_TRUNC, DEFFILEMODE);
    if (-1 == fd)
    {
        printf("Unable to create %s\n", path);
        goto err_ret;
    }

    if (ftruncate(fd, length * sizeof(uint32_t)))
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

    if (!dauMem.read(srcAddress, destAddressPtr, length))
    {
        printf("Failed to read Dau data into file\n");
    }
    else
    {
        retVal = 0;
    }

err_ret:
    if (nullptr != destAddressPtr)
    {
        munmap(destAddressPtr, length);
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
int main(int argc, char** argv)
{
    int                 retVal = -1;
    uint32_t*           buffer = nullptr;
    uint32_t            addr;
    uint32_t            length = 1;
    uint8_t             bar = 0;
    PciDauMemory        dauMem;
    DauPower            dauPower;
    char*               outFile = nullptr;
    int                 opt;
    static const char*  optString = "b:f:a";
    int                 index;
    bool                useInternalAddress = true;

#ifdef DEBUG_CMD_LINE
    std::string         commandLine;

    for (int index = 0; index < argc; index++)
    {
        commandLine += argv[index];
        commandLine += " ";
    }
    ALOGD("%s", commandLine.c_str());
#endif

    while ((opt = getopt(argc, argv, optString)) != -1)
    {
        switch (opt)
        {
            case 'b':
                bar = strtoul(optarg, 0, 0);
                if (5 < bar)
                {
                    printf("BAR value must be between 0 and 5\n");
                    goto err_ret;
                }
                break;

            case 'f':
                outFile = optarg;
                break;

            case 'a':
                useInternalAddress = false;
                break;

            case '?':
            default:
                usage();
                goto err_ret;
                break;
        }
    }

    if (optind == argc)
    {
        usage();
        goto err_ret;
    }

    index = optind;

    addr = strtoul(argv[index++], 0, 16);
    if (index < argc)
    {
        length = strtoul(argv[index++], 0, 0);
    }

    if (index != argc)
    {
        usage();
        goto err_ret;
    }

    if (!dauPower.isPowered())
    {
        printf("The DAU is off\n");
        goto err_ret;
    }

    dauMem.selectBar(bar);
    dauMem.useInternalAddress(useInternalAddress);

    if (!dauMem.map())
    {
        printf("Failed to access Dau\n");
        goto err_ret;
    }

    if (nullptr == outFile)
    {
        buffer = new uint32_t[length];
        memset(buffer, 0, sizeof(uint32_t) * length);

        if (dauMem.read(addr, buffer, length))
        {
            printBuffer32(buffer, length, addr);
            retVal = 0;
        }
        else
        {
            printf("Failed to read Dau memory\n");
        }
    }
    else
    {
        retVal = writeToFile(outFile, dauMem, addr, length);
    }

err_ret:
    if (nullptr != buffer)
    {
        delete [] buffer;
        buffer = nullptr;
    }

    dauMem.unmap();

    return(retVal);
}
