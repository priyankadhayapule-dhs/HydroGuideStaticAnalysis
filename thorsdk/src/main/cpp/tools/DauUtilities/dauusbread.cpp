//
// Copyright (C) 2016 EchoNous, Inc.
//

#define LOG_TAG "dauusbread"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string>
#include <libusb/libusb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <ThorError.h>
#include <ThorUtils.h>
#include <DauFwUpgrade.h>
#include <DauContext.h>
#include <DauRegRWService.h>
#include <UsbDauMemory.h>
#include <UsbDauFwUpgrade.h>

#define DEBUG_CMD_LINE 0
#define BUF_SZ 64

//-----------------------------------------------------------------------------
void usage(char utility[])
{
    printf("\nUsage:%s [-r] [-a <offset>] [-f <output_file>] <addr> [length]\n\n",utility);
    printf("addr \tHex value of the Address to read\n");
    printf("length\tThe number of 32-bit values to read.If not\n");
    printf("\tspecified, the default is to read one value.\n\n");
    printf("-f output_file\tResulting data will be stored in this file.  If the file\n");
    printf("\talready exists, it will be truncated and overwritten.\n");
    printf("-r To get Raw data only of register read operation. It gives directly value of register.\n");
    printf("-a <offset> specify the offset to add in <addr>\n");
}

//-----------------------------------------------------------------------------
static void printBuffer32(uint32_t* bufPtr, uint32_t bufSize, size_t refAddr, bool rawData)
{
    const uint32_t  numElements = 8;  // Elements per row
    uint32_t        index;
    uint32_t        pos;

    for (index = 0; index < bufSize; index += numElements)
    {
        if(!rawData)
            printf("0x%016X: ", (uint32_t)(refAddr + index*sizeof(uint32_t)));

        for (pos = 0; pos < MIN(numElements, (bufSize - index)); pos++)
        {
            printf("0x%08X  ", *(bufPtr + index + pos));
        }
        printf("\n");
    }
}

//-----------------------------------------------------------------------------
uint8_t getDeviceClass(int fd)
{
    libusb_device_handle*       usbHandlePtr = NULL;
    int rc = 0;
    unsigned char tmpBuf[BUF_SZ];
    uint8_t deviceClass;

    rc = libusb_init(nullptr);
    if(rc < 0)
    {
        printf("libusb_init failed: %s", libusb_error_name(rc));
        return 0;
    }

    if (0 != libusb_wrap_fd(nullptr, fd, &usbHandlePtr))
    {
        printf("Unable to wrap libusb fd");
        return 0;
    }

    libusb_claim_interface(usbHandlePtr, 0);

    libusb_get_descriptor(usbHandlePtr,LIBUSB_DT_CONFIG,0,tmpBuf,sizeof(tmpBuf));

    deviceClass = tmpBuf[DEVICE_CLASS_OFFSET];

    libusb_release_interface(usbHandlePtr, 0);
    libusb_close(usbHandlePtr);
    libusb_exit(nullptr);

    return deviceClass;
}

//-----------------------------------------------------------------------------
int getUsbPath(char *buf)
{
    char bufferPath[BUF_SZ],busNo[BUF_SZ],devNo[BUF_SZ];
    int ret = 0;
    char* token;
    FILE *fp;

    memset(bufferPath,'\0', sizeof(bufferPath));

    ret = system("rm -f /data/local/thortools/getFD.txt");
    if(ret < 0)
    {
        printf("File not Avail to remove\n");
        //return ret;
    }

    ret = system("lsusb | grep 1dbf > /data/local/thortools/getFD.txt");
    if(ret < 0)
    {
        return ret;
    }

    fp = fopen("/data/local/thortools/getFD.txt","r");
    if(!fp)
    {
        printf("Can't Open File--Usb Disconnected\n");
        exit(1);
    }

    while (fgets(bufferPath, sizeof(bufferPath), fp) != NULL);
    fclose(fp);

    if(bufferPath[0] == '\0')
    {
        ret = -1;
        return ret;
    }

    token = strtok(bufferPath, " ");
    token = strtok(NULL, " ");
    strcpy(busNo,token);

    token = strtok(NULL, " ");
    token = strtok(NULL, ":");
    strcpy(devNo,token);

    token = strtok(NULL, " ");

    strcpy(buf,busNo);
    strcat(buf,"/");
    strcat(buf,devNo);

    return ret;
}


//-----------------------------------------------------------------------------
int writeFile(char* filename, uint32_t* buffer, uint32_t length)
{
    int         retVal = -1;
    int         fd = -1;
    uint32_t*   destAddressPtr = nullptr;
    uint32_t    fileLength = length * sizeof(uint32_t);
    char        filePath[BUF_SZ] = "/data/echonous/";

    strcat(filePath,filename);

    fd = open(filePath, O_RDWR | O_CREAT | O_TRUNC, DEFFILEMODE);
    if (-1 == fd)
    {
        printf("Unable to create %s\n", filePath);
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
        printf("Failed to map data file %s.  errno = %d\n", filePath, errno);
        goto err_ret;
    }

    memcpy(&destAddressPtr[0],&buffer[0],length * sizeof(uint32_t));

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
int writeToFile(char* filename, DauMemory& dauMem, uint32_t srcAddress, uint32_t length)
{
    int         retVal = -1;
    int         fd = -1;
    uint32_t*   destAddressPtr = nullptr;
    uint32_t    fileLength = length * sizeof(uint32_t);
    char        filePath[BUF_SZ] = "/data/echonous/";

    strcat(filePath,filename);

    fd = open(filePath, O_RDWR | O_CREAT | O_TRUNC, DEFFILEMODE);
    if (-1 == fd)
    {
        printf("Unable to create %s\n", filePath);
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
        printf("Failed to map data file %s.  errno = %d\n", filePath, errno);
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
    char                    blPath[PATH_MAX] = "/dev/bus/usb/";
    uint32_t                retVal = 0;
    char                    usbPath[BUF_SZ];
    UsbDauMemory            dauMem;
    uint32_t                addr;
    DauContext              dauContext;
    uint32_t                dauAddr;
    static const char*      optString = "f:a:r";
    int                     opt;
    char*                   outFile = nullptr;
    bool                    rawData = false, aOs = false;
    uint32_t                addOffset = 0;
    uint32_t                subOffset = 0;
    int                     index;
    uint32_t                length = 1;
    uint32_t*               buffer = nullptr;

    bool                    service = false;
    int                     sock = 0, valread;
    struct sockaddr_in      serv_addr;
    DauRegRW                dauRegRw;

#if DEBUG_CMD_LINE
    std::string         commandLine;

    for (int index = 0; index < argc; index++)
    {
        commandLine += argv[index];
        commandLine += " ";
    }
    printf("%s\n", commandLine.c_str());
#endif

    while ((opt = getopt(argc, argv, optString)) != -1)
    {
        switch (opt)
        {
            case 'f':
                outFile = optarg;
                break;
            case 'a':
                addOffset = strtoul(optarg,0,16);
                aOs = true;
                break;
            case 'r':
                rawData = true;
                break;

            case '?':
            default:
                usage(argv[0]);
                goto err_ret;
        }
    }

    if (optind == argc)
    {
        usage(argv[0]);
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
        usage(argv[0]);
        goto err_ret;
    }

    retVal = getUsbPath(usbPath);
    if(retVal < 0)
    {
        printf("Can't get Usb Device Path\n");
        exit(-1);
    }

    strcat(blPath,usbPath);

    dauContext.usbFd = open(blPath, O_RDWR);
    if (-1 == dauContext.usbFd)
    {
        printf("Unable to open Usb device %s\n", blPath);
        exit(-1);
    }

    if(getDeviceClass(dauContext.usbFd) == LIBUSB_CLASS_APPLICATION)
    {
        printf("Dau is in Bootloader Firmware\n");
        printf("Please Execute Application Firmware using 'dauusbpower on' command\n");
        exit(0);
    }

    if(aOs)
    {
        dauAddr = addr + addOffset;
    }
    else
    {
        dauAddr = addr;
    }

    if (!dauMem.map(&dauContext))
    {
        if (!dauMem.isInterfaceClaimed())
        {
            service = true;
        }
    }

    buffer = new uint32_t[length];
    memset(buffer, 0, sizeof(uint32_t) * length);

    if(service)
    {
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            printf("\n Socket creation error \n");
            return -1;
        }

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORT);

        // Convert IPv4 and IPv6 addresses from text to binary form
        if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)
        {
            printf("\nInvalid address/ Address not supported \n");
            return -1;
        }

        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
            printf("\nConnection Failed \n");
            return -1;
        }

        dauRegRw.regAddr = dauAddr;
        dauRegRw.numWords = length;
        dauRegRw.flag = READ;

        send(sock , &dauRegRw , sizeof(dauRegRw) , 0 );
        valread = read( sock , buffer, length * sizeof(uint32_t));

        if(nullptr == outFile)
        {
            printBuffer32(buffer, length, dauAddr, rawData);
            retVal = 0;
        }
        else
        {
            retVal = writeFile(outFile, buffer, length);
        }
    }

    else
    {
        if(nullptr == outFile) {
            if (dauMem.read(dauAddr, &buffer[0], length)) {
                retVal = 0;
            } else {
                printf("Failed to read Dau memory\n");
                goto err_ret;
            }
            printBuffer32(buffer, length, dauAddr,rawData);
        }
        else
        {
            retVal = writeToFile(outFile, dauMem, dauAddr , length);
        }
    }

    err_ret:
    if (nullptr != buffer)
    {
        delete [] buffer;
        buffer = nullptr;
    }
    return(retVal);
}
