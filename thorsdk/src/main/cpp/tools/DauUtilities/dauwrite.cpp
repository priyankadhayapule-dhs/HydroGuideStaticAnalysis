//
// Copyright (C) 2016 EchoNous, Inc.
//

#define LOG_TAG "dauwrite"

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string>

#include <PciDauMemory.h>
#include <DauPower.h>
#include <ThorUtils.h>

#define DEBUG_CMD_LINE 1

//-----------------------------------------------------------------------------
void usage()
{
    printf("Usage:\n");
    printf("  dauwrite [-b <bar>] [-a] [-f <input_file>] <addr> <value>\n");
    printf("\n    addr\t\tHex value of the address to modify\n");
    printf("    value\t\tHex value to write to address\n");
    printf("    -b bar\t\tSpecify the BAR to read from.  If not specified, the default\n");
    printf("       \t\t\tis from BAR0.\n");
    printf("    -a\t\t\tUse BAR address instead of internal address.  Default is\n");
    printf("  \t\t\tto use internal address.\n");
    printf("    -f input_file\tFile containing raw data.  The number of words to write\n");
    printf("    \t\t\tis determined from the file size.\n");
}

//-----------------------------------------------------------------------------
int readFromFile(char* path, DauMemory& dauMem, uint32_t destAddress)
{
    int         retVal = -1;
    int         fd = -1;
    int         ret;
    struct stat statBuf;
    uint32_t*   srcAddressPtr = nullptr;

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

    if (!dauMem.write(srcAddressPtr,
                      destAddress,
                      statBuf.st_size / sizeof(uint32_t)))
    {
        printf("Failed to write data to Dau\n");
    }
    else
    {
        retVal = 0;
    }

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
int main(int argc, char** argv)
{
    int                 retVal = -1;
    uint32_t            data = 0;
    uint32_t            addr;
    uint8_t             bar = 0;
    PciDauMemory        dauMem;
    DauPower            dauPower;
    char*               inFile = nullptr;
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
                inFile = optarg;
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
        data = strtoul(argv[index++], 0, 16);
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

    if (inFile)
    {
        retVal = readFromFile(inFile, dauMem, addr);
    }
    else
    {
        // Writing single value to register
        if (dauMem.write(&data, addr, 1) )
        {
            retVal = 0;
        }
        else
        {
            printf("Failed to write value to Dau\n");
        }
    }

err_ret:
    dauMem.unmap();

    return(retVal);
}
