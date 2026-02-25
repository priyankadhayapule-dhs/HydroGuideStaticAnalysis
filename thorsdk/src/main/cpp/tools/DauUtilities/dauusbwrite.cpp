//
// Copyright (C) 2016 EchoNous, Inc.
//

#define LOG_TAG "dauusbwrite"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
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

//#define DEBUG_CMD_LINE 0
#define BUF_SZ 64

//-----------------------------------------------------------------------------
void usage(char utility[])
{
    printf("\nUsage:%s [-a <offset>] [-f <input_file>] <addr> <value>\n\n",utility);
    printf("addr\tHex value of the address to modify\n");
    printf("value\tHex value to write to address\n");
    printf("-f input_file\tFile containing raw data.  The number of words to write\n");
    printf("\tis determined from the file size.\n");
    printf("-a specify the offset to add in <addr>\n");
}

//-----------------------------------------------------------------------------
int readFromFile(char* filename, DauMemory& dauMem, uint32_t destAddress,bool service,int sock)
{
    int         retVal = -1;
    int         fd = -1;
    int         ret;
    struct stat statBuf;
    uint32_t*   srcAddressPtr = nullptr;
    char        filePath[BUF_SZ] = "/data/echonous/";
    DauRegRW    dauRegRw;
    uint32_t    bufLen;
    uint32_t*   dataBuf = nullptr;

    strcat(filePath,filename);

    fd = open(filePath, O_RDONLY);
    if (-1 == fd)
    {
        printf("Unable to open %s\n", filePath);
        goto err_ret;
    }

    ret = fstat(fd, &statBuf);
    if (ret)
    {
        printf("Unable to get size of %s\n", filePath);
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
        printf("Failed to map data file %s\n", filePath);
        goto err_ret;
    }

    if(!service)
    {
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
    }
    else
    {
        bufLen = statBuf.st_size; // File Data Size
        bufLen += sizeof(DauRegRW); // Total buffer size to send

        dataBuf = (uint32_t *) malloc (bufLen);
        if(dataBuf == NULL)
        {
            printf("Failed to allocate data buffer\n");
            goto err_ret;
        }

        dataBuf[0] = destAddress;
        dataBuf[1] = statBuf.st_size / sizeof(uint32_t);
        dataBuf[2] = WRITE;

        memcpy(&dataBuf[3],&srcAddressPtr[0],statBuf.st_size);
        send(sock , (char *)dataBuf , bufLen , 0 );
    }
    err_ret:
    if (nullptr != srcAddressPtr)
    {
        munmap(srcAddressPtr, statBuf.st_size);
        srcAddressPtr = nullptr;
    }

    if(!dataBuf)
        free(dataBuf);

    if (-1 != fd)
    {
        close(fd);
        fd = -1;
    }

    return(retVal);
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
        return ret;
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
int main(int argc, char** argv)
{
    int                     retVal = 0;
    char                    blPath[PATH_MAX] = "/dev/bus/usb/";
    char                    usbPath[BUF_SZ];
    UsbDauMemory            dauMem;
    uint32_t                addr;
    uint32_t                data = 0;
    DauContext              dauContext;
    uint32_t                addOffset = 0;
    bool                    aOs = false;
    int                     opt;
    static const char*      optString = "f:a:";
    char*                   inFile = nullptr;
    int                     index;
    uint32_t                dauAddr;

    bool                    service = false;
    int                     sock = 0;
    struct sockaddr_in      serv_addr;
    uint32_t                buffer[4];

#ifdef DEBUG_CMD_LINE
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
                inFile = optarg;
                break;
            case 'a':
                addOffset = strtoul(optarg,0,16);
                aOs = true;
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
        data = strtoul(argv[index++], 0, 16);
    }

    if (index != argc)
    {
        usage(argv[0]);
        goto err_ret;
    }

    retVal = getUsbPath(usbPath);
    if(retVal < 0)
    {
        printf("Can't get BL Path\n");
        goto err_ret;
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
        printf("Dau is off.Please turn on the dau using 'dauusbpower' utility\n");
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

        if(inFile)
        {
            retVal = readFromFile(inFile, dauMem, dauAddr,service,sock);
        }
        else
        {
            buffer[0] = dauAddr;
            buffer[1] = 1;
            buffer[2] = WRITE;
            buffer[3] = data;
            send(sock , (char *)buffer , sizeof(buffer) , 0 );
        }
    }
    else
    {
        if (inFile)
        {
            retVal = readFromFile(inFile, dauMem, dauAddr,service,0);
        }
        else
        {
            // Writing single value to register
            if (dauMem.write(&data, dauAddr, 1))
            {
                retVal = 0;
            }
            else
            {
                printf("Failed to write value to Dau\n");
            }
        }
    }

    err_ret:
    return(retVal);
}
