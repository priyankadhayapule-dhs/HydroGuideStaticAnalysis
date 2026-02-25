//
// Copyright (C) 2016 EchoNous, Inc.
//

#define LOG_TAG "fwupgrade"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <string>

#include <ThorError.h>
#include <ThorUtils.h>
#include <DauContext.h>
#include <DauRegisters.h>
#include <DauPower.h>
#include <PciDauFwUpgrade.h>
#include <UsbDauFwUpgrade.h>

#define BUF_SZ 64
DauContext                      dauContext;

enum
{
    GETFWINFO=1,
    APPFWUPGRADE,
    BOOTFWUPGRADE,
    DEVREBOOT,
    DEVEXIT
};

//-----------------------------------------------------------------------------
void usage(char utility[])
{
    printf("Usage:In USB Case : %s [Interface]\n",utility);
    printf("Example: %s USB\n",utility);
    printf("\nUsage:In PCIe Case : %s [Interface]\n",utility);
    printf("Example: %s PCIe\n",utility);
}

//-----------------------------------------------------------------------------
void menu()
{
    printf("********************************************\r\n");
    printf("%d : Get Firmware Information\r\n", GETFWINFO);
    printf("%d : Application Firmware Upgrade\r\n", APPFWUPGRADE);
    printf("%d : Boot-loader Firmware Upgrade\r\n", BOOTFWUPGRADE);
    printf("%d : Reboot Device\r\n", DEVREBOOT);
    printf("%d : EXIT\r\n", DEVEXIT);
    printf("********************************************\r\n");
}

//-----------------------------------------------------------------------------
void pritnFwInfo(FwInfo *fwInfo)
{
    printf("%s\n",__func__);

    printf("BootLoader v%d.%d.%d\n\r", fwInfo->bootLoaderVersion.majorV,fwInfo->bootLoaderVersion.minorV,fwInfo->bootLoaderVersion.patchV);

    printf("Application Count: %d\n\r", fwInfo->appImgCntr);

    printf("Application1 v%d.%d.%d\n\r", fwInfo->app1Version.majorV,fwInfo->app1Version.minorV,fwInfo->app1Version.patchV);
    printf("Boot Flag 1 : %d\n\r", fwInfo->bootUpdateFlag1);

    printf("Application2 v%d.%d.%d\n\r", fwInfo->app2Version.majorV,fwInfo->app2Version.minorV,fwInfo->app2Version.patchV);
    printf("Boot Flag 2 : %d\n\r", fwInfo->bootUpdateFlag2);
}

//-----------------------------------------------------------------------------
int appFirmWareUpgrade(FwInfo *fwInfo, const char *filePath, std::shared_ptr<DauFwUpgrade>  dauMem)
{
    FILE *fp;
    ImageHeader *ih;
    uint32_t flashAddress;
    int fileSize = 0;
    unsigned char *fileBuf;
    char localPath[BUF_SZ]= {};
    bool ret = false;

    strcat(localPath,filePath);
    if (fwInfo->bootUpdateFlag1 != fwInfo->bootUpdateFlag2)
    {
        strcat(localPath, "FwUpgrade.bin");
        fp = fopen(localPath,"rb");

        if (fp != NULL)
        {
            printf("File opened successfully\n");
        }
        else
        {
            printf("Failed to open File\n");
            return false;
        }

        fseek(fp, APPLICATION_2_IMAGE_SIZE + sizeof(ImageHeader), SEEK_SET);
        fileSize = ftell(fp);
        fileBuf = (unsigned char *) malloc(fileSize);

        printf("File Size: %d\n",fileSize);

        fseek(fp, 0L, SEEK_SET);
        fread(fileBuf, fileSize, 1, (FILE *) fp);

        if(fwInfo->bootUpdateFlag1 == INEXECUTION)
        {
            flashAddress = APPLICATION_2_IMAGE_START;
            printf("Upgrade Application 2\n");
        }
        else if(fwInfo->bootUpdateFlag2 == INEXECUTION)
        {
            flashAddress = APPLICATION_1_IMAGE_START;
            printf("Upgrade Application 1\n");
        }
    }
    else
    {
        return false;
    }
    fclose(fp);

    ih = (ImageHeader *) fileBuf;

    switch (ih->imgTyp)
    {
        case BL_IMAGE:
            printf("BL_IMAGE\n");
            break;
        case APP_IMAGE:
            printf("APP_IMAGE\n");
            break;
    }

    printf("MajV:%d\nMinV:%d\nPatchV:%d\n", ih->appVersion.majorV, ih->appVersion.minorV, ih->appVersion.patchV);
    printf("Img Len:%d\n", ih->imageLength);

    uint32_t crcLength = ih->imageLength + (sizeof(ImageHeader)- sizeof(uint32_t));
    uint32_t crcCheck = dauMem->computeCRC(fileBuf + sizeof(uint32_t), crcLength);
    printf("Received CRC:0x%x\n", ih->CRC);
    printf("Calculated CRC:0x%x\n", crcCheck);

    if (ih->CRC == crcCheck)
    {
        printf("CRC OK\n");
    }
    else
    {
        printf("CRC Fail\n");
        goto err_ret;
    }

    if(dauContext.isUsbContext)
    {
        if( (fwInfo->bootLoaderVersion.majorV <= 1) && (fwInfo->bootLoaderVersion.minorV == 0) && (fwInfo->bootLoaderVersion.patchV <= 5) )
        {
            printf("Old Firmware\n");
            crcLength = ih->imageLength + (sizeof(ImageHeader)- (sizeof(uint32_t)*2) );  // Remove ih.imgTyp for older version
            crcCheck = dauMem->computeCRC(fileBuf + (sizeof(uint32_t)*2), crcLength);
            printf("Calculated CRC:0x%x\n", crcCheck);
            ih->CRC = crcCheck;
            ih->imgTyp = crcCheck;
            fileBuf += sizeof(ih->CRC);
        }

        flashAddress = SE_FB_MEM_START_ADDR;
    }

    if(!dauMem->uploadImage((uint32_t *)fileBuf, flashAddress, fileSize))
    {
        printf("Error in Upload image\n");
        goto err_ret;
    }
    ret = true;
    err_ret:
    return (ret);
}

//-----------------------------------------------------------------------------
int bootLoaderFirmWareUpgrade(FwInfo *fwInfo, const char *filePath, std::shared_ptr<DauFwUpgrade>  dauMem)
{
    FILE *fp;
    ImageHeader *ih;
    uint32_t flashAddress;
    int fileSize = 0;
    unsigned char *fileBuf;
    char localPath[BUF_SZ]= {};
    bool ret = false;

    strcat(localPath,filePath);

    strcat(localPath, "BLFwUpgrade.bin");
    fp = fopen(localPath,"rb");

    if (fp != NULL)
    {
        printf("File opened successfully\n");
    }
    else
    {
        printf("Failed to open File\n");
        return false;
    }

    fseek(fp, EBL_IMAGE_SIZE + sizeof(ImageHeader), SEEK_SET);
    fileSize = ftell(fp);
    fileBuf = (unsigned char *) malloc(fileSize);

    printf("File Size: %d\n",fileSize);

    fseek(fp, 0L, SEEK_SET);
    fread(fileBuf, fileSize, 1, (FILE *) fp);

    printf("Upgrade BootLoader\n");


    fclose(fp);

    ih = (ImageHeader *) fileBuf;

    switch (ih->imgTyp)
    {
        case BL_IMAGE:
            printf("BL_IMAGE\n");
            break;
        case APP_IMAGE:
            printf("APP_IMAGE\n");
            break;
    }

    printf("MajV:%d\nMinV:%d\nPatchV:%d\n",ih->appVersion.majorV, ih->appVersion.minorV, ih->appVersion.patchV);
    printf("Img Len:%d\n", ih->imageLength);

    uint32_t crcLength = ih->imageLength + (sizeof(ImageHeader)- sizeof(uint32_t));
    uint32_t crcCheck = dauMem->computeCRC(fileBuf + sizeof(uint32_t), crcLength);
    printf("Received CRC:0x%x\n", ih->CRC);
    printf("Calculated CRC:0x%x\n", crcCheck);

    if (ih->CRC == crcCheck)
    {
        printf("CRC OK\n");
    }
    else
    {
        printf("CRC Fail\n");
        goto err_ret;
    }

    if(dauContext.isUsbContext)
    {
        printf("bootUpdateFlag1:%d\nbootUpdateFlag2:%d\n",fwInfo->bootUpdateFlag1,fwInfo->bootUpdateFlag2);

        if(fwInfo->bootUpdateFlag1 == 1)
        {
            printf("Application1 v%d.%d.%d\n",fwInfo->app1Version.majorV,fwInfo->app1Version.minorV,fwInfo->app1Version.patchV);
            if( (fwInfo->app1Version.majorV <= 1) && (fwInfo->app1Version.minorV == 0) && (fwInfo->app1Version.patchV <= 5) )
            {
                printf("Old Firmware 1\n");
                crcLength = ih->imageLength + (sizeof(ImageHeader)- (sizeof(uint32_t)*2) );  // Remove ih.imgTyp for older version
                crcCheck = dauMem->computeCRC(fileBuf + (sizeof(uint32_t)*2), crcLength);
                printf("Calculated CRC:0x%x\n", crcCheck);
                ih->CRC = crcCheck;
                ih->imgTyp = crcCheck;
                fileBuf += sizeof(ih->CRC);
            }
        }
        else if(fwInfo->bootUpdateFlag2 == 1)
        {
            printf("Application2 v%d.%d.%d\n",fwInfo->app2Version.majorV,fwInfo->app2Version.minorV,fwInfo->app2Version.patchV);

            if( (fwInfo->app2Version.majorV <= 1) && (fwInfo->app2Version.minorV == 0) && (fwInfo->app2Version.patchV <= 5) )
            {
                printf("Old Firmware 2\n");
                crcLength = ih->imageLength + (sizeof(ImageHeader)- (sizeof(uint32_t)*2) );  // Remove ih.imgTyp for older version
                crcCheck = dauMem->computeCRC(fileBuf + (sizeof(uint32_t)*2), crcLength);
                printf("Calculated CRC:0x%x\n", crcCheck);
                ih->CRC = crcCheck;
                ih->imgTyp = crcCheck;
                fileBuf += sizeof(ih->CRC);
            }
        }

        flashAddress = SE_FB_MEM_START_ADDR;
    }
    else
    {
        flashAddress = EBL_IMAGE_START;
    }

    if(!dauMem->uploadImage((uint32_t *)fileBuf, flashAddress, fileSize))
    {
        printf("Error in Upload image\n");
        goto err_ret;
    }
    ret = true;
    free(fileBuf);
    err_ret:
    return (ret);
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
int main(int argc, char **argv)
{
    char                            path[PATH_MAX] = "/dev/bus/usb/";
    const char                      filePath[BUF_SZ] = "/sdcard/";
    char                            buffer[BUF_SZ];
    DauPower                        dauPower;
    std::shared_ptr<DauFwUpgrade>   dauFwUpMem;
    FwInfo*                         fwInfo;

    int tag, supLoop = 1,retVal;
    bool x = false;

    // Check the mandatory parameters
    if (2 > argc)
    {
        usage(argv[0]);
        exit(-1);
    }

    if(!(strcmp(argv[1],"PCIe")))
    {
        dauContext.isUsbContext = false;
    }
    else if(!(strcmp(argv[1],"USB")))
    {
        retVal = getUsbPath(buffer);
        if(retVal < 0)
        {
            printf("Can't get Usb Device Path\n");
            exit(-1);
        }
        strcat(path, buffer);
        dauContext.isUsbContext = true;
    }
    else
    {
        usage(argv[0]);
        exit(-1);
    }

    if (dauContext.isUsbContext)
    {
        dauFwUpMem = std::make_shared<UsbDauFwUpgrade>();
    }
    else
    {
        dauFwUpMem = std::make_shared<PciDauFwUpgrade>();
    }

//     Open USB (Dau) device
    if (dauContext.isUsbContext)
    {
        dauContext.usbFd = open(path, O_RDWR);
        if (-1 == dauContext.usbFd)
        {
            printf("Unable to open Usb device %s\n", path);
            printf("errno = %d\n", errno);
            exit(-1);
        }
        printf("USB Path : %s\r\n", path);
    }
    else
    {
        if (!dauPower.isPowered())
        {
            printf("The DAU is off\n");
            return -1;
        }
        else
        {
            printf("The DAU is ON\n");
        }
    }

    if(!dauFwUpMem->init(&dauContext))
    {
        printf("Fail To Init\n");
        return -1;
    }

    fwInfo = (FwInfo *) malloc(sizeof(FwInfo));
    if(nullptr == fwInfo)
    {
        printf("FwInfo Malloc Failed\n");
        return -1;
    }

    while (supLoop) {
        menu();
        printf("Enter Tag:\r\n");
        if(scanf("%d", &tag) == 0)
        {
            printf("Invalid Value. Please input correct value\n");
            exit(-1);
        }
        else {
            switch (tag) {
                case GETFWINFO:
                    printf("--> Get Firmware Information\r\n");
                    dauFwUpMem->getFwInfo(fwInfo);
                    printf("Print Firmware Information:%d\n", x);
                    pritnFwInfo(fwInfo);
                    break;

                case APPFWUPGRADE:
                    if (dauContext.isUsbContext &&
                        (getDeviceClass(dauContext.usbFd) == LIBUSB_CLASS_VENDOR_SPEC)) {
                        printf("Application Firmware Can't Upgrade from an Application.\n");
                        dauFwUpMem->rebootDevice();
                        supLoop = 0;
                        printf("Device Rebooted to Boot-loader\n");
                        exit(0);
                    }
                    printf("--> Firmware Upgrade\r\n");
                    if (dauFwUpMem->getFwInfo(fwInfo)) {
                        if (appFirmWareUpgrade(fwInfo, filePath, dauFwUpMem)) {
                            printf("appFirmWareUpgrade Success\r\n");
                        } else {
                            printf("appFirmWareUpgrade Fail\n");
                        }
                    } else {
                        printf("getFwInfo Fail \r\n");
                    }
                    break;

                case BOOTFWUPGRADE:
                    if (dauContext.isUsbContext &&
                        (getDeviceClass(dauContext.usbFd) == LIBUSB_CLASS_APPLICATION)) {
                        printf("Boot-Loader Firmware Can't Upgrade from Boot-loader.\n");
                        supLoop = 0;
                        printf("Please Goto Application using 'dauusbpower on' command.\n");
                        exit(0);
                    }
                    printf("--> BL Upgrade\r\n");
                    if (dauFwUpMem->getFwInfo(fwInfo)) {
                        if (bootLoaderFirmWareUpgrade(fwInfo, filePath, dauFwUpMem)) {
                            printf("bootLoaderFirmWareUpgrade Success\r\n");
                        } else {
                            printf("bootLoaderFirmWareUpgrade Fail\n");
                        }
                    } else {
                        printf("getFwInfo Fail \r\n");
                    }
                    break;

                case DEVREBOOT:
                    printf("--> Reboot Device\r\n");
                    dauFwUpMem->rebootDevice();
                    supLoop = 0;
                    break;

                case DEVEXIT:
                    printf("--> EXIT\r\n");
                    supLoop = 0;
                    break;

                default:
                    printf("--> Enter Valid Tag\r\n");
                    exit(-1);
            }
        }
    }

    printf("--> Exit Loop\n");
    free(fwInfo);
    return 0;
}