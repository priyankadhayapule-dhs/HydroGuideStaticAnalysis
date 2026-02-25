//
// Copyright (C) 2016 EchoNous, Inc.
//

#define LOG_TAG "factdatarw"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <libusb/libusb.h>

#include <ThorError.h>
#include <ThorUtils.h>
#include <DauFwUpgrade.h>
#include <DauPower.h>
#include <DauContext.h>
#include <UsbDauMemory.h>
#include <UsbDauFwUpgrade.h>
#include <PciDauFwUpgrade.h>

#define BUF_SZ 64

DauContext dauContext;

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

    ret = system("rm -f -- /data/local/thortools/getFD.txt");
    if(ret < 0)
    {
        printf("File not Avail to remove\n");
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
void usage(char utility[])
{
    printf("\nUsage:%s [-i <Interface>] [-w -p <probeType> -s <Serial No> -n <Part No>] \n\n",utility);
    printf("  -i Interface Specify the USB or PCIe\n");
    printf("  -r To Read Factory Data\n");
    printf("  -w To Write Factory Data\n");
    printf("  -p <ProbeType> To write ProbeType only\n");
    printf("  -s <Serial No> To write Serial No only\n");
    printf("  -n <Part No>   To write Part No only\n");
    printf("E.g. %s -i USB -w -p 0x80000003 -s SN1234657 -n PN753858\n",utility);
    printf("     %s -i USB -r \n\n",utility);
}

//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    int             retVal = false;
    char            path[PATH_MAX] = "/dev/bus/usb/";
    char            usbPath[BUF_SZ];
    char            wrFactData[ sizeof(FactoryDataKey) + sizeof(FactoryData) ];
    FactoryData     fData, fDataRd;

    FactoryDataKey  fDataKey;
    DauPower        dauPower;
    std::shared_ptr<DauFwUpgrade> dauFwUpMem;
    uint32_t wrfactDatlen;

    bool readData = false;
    bool writeData = false;
    bool snUpdate = false;
    bool pnUpdate = false;
    bool probeTypUpdate = false;

    int probeType;
    char sNo[ 1 + sizeof(fData.serialNum)];
    char pNo[ 1 + sizeof(fData.partNum)];
    char interFace[BUF_SZ];
    int opt;
    static const char *optString = "p:s:n:i:wr";

    while ((opt = getopt(argc, argv, optString)) != -1) {
        switch (opt) {
            case 'i':
                strcpy(interFace, optarg);
                printf("interFace:%s\n", interFace);
                break;

            case 'w':
                printf("Write\n");
                writeData = true;
                readData = false;
                break;

            case 'r':
                printf("Read\n");
                readData = true;
                writeData = false;
                break;

            case 'p':
                probeType = strtoul(optarg, 0, 0);
                probeTypUpdate = true;
                printf("Probe Type: %x\n", probeType);
                break;

            case 's':
                strcpy(sNo, optarg);
                snUpdate = true;
                printf("SN: %s\n", sNo);
                break;

            case 'n':
                strcpy(pNo, optarg);
                pnUpdate = true;
                printf("PN: %s\n", pNo);
                break;

            case '?':
            default:
                usage(argv[0]);
                goto err_ret;
        }
    }

    if (!(strcmp(interFace, "PCIe")))
    {
        printf("PCIe Interface\n");
        dauContext.isUsbContext = false;
        dauFwUpMem = std::make_shared<PciDauFwUpgrade>();
    }
    else if (!(strcmp(interFace, "USB")))
    {
        printf("USB interface\n");
        retVal = getUsbPath(usbPath);
        if(retVal < 0)
        {
            printf("Can't get Usb Device Path\n");
            exit(-1);
        }
        strcat(path, usbPath);
        dauContext.isUsbContext = true;
        dauFwUpMem = std::make_shared<UsbDauFwUpgrade>();
    }
    else
    {
        printf("Interface Not Valid\n\n");
        usage(argv[0]);
        exit(-1);
    }

    /* PCIe Interface */
    if(!dauContext.isUsbContext)
    {
        if (!dauPower.isPowered()) {
            printf("The DAU is off. Use daupower utility to turn on Dau.\n");
            exit(-1);
        } else {
            printf("The DAU is ON\n");
        }
    }
    else
    {
        //Open USB (Dau) device
        dauContext.usbFd = open(path, O_RDWR);
        if (-1 == dauContext.usbFd)
        {
            printf("Unable to open Usb device %s\n", path);
            printf("errno = %d\n", errno);
            exit(-1);
        }
        printf("USB Path : %s\r\n", path);
    }

    if (!dauFwUpMem->init(&dauContext)) {
        printf("Fail To Init\n");
        exit(-1);
    }

    if(dauContext.isUsbContext && (getDeviceClass(dauContext.usbFd) == LIBUSB_CLASS_VENDOR_SPEC) && writeData ==
                                                                                                    true)
    {
        printf("Application Firmware Can't Write Factory Data.\n");
        printf("Usb 'dauusbpower off' utility to switch to Bootloader Firmware\n");
        exit(0);
    }

    memset(&fDataRd,0, sizeof(FactoryData));
    if (!dauFwUpMem->readFactData((uint32_t*) &fDataRd, FACTORY_DATA_START, sizeof(FactoryData) ) )
    {
        printf("Fail to Read Factory Data\r\n");
        exit(0);
    }


    if(readData == false && writeData == false)
    {
        usage(argv[0]);
        exit(0);
    }

    if (readData == true)
    {
        printf("--> Read Factory Data\r\n");
        printf("\n");
        printf("Factory Data\n");
        printf("Probe Type:0x%x\n", fDataRd.probeType);
        printf("Serial No:%s\n", fDataRd.serialNum);
        printf("Part No:%s\n", fDataRd.partNum);
    }

    if (writeData == true)
    {
        if(probeTypUpdate || snUpdate || pnUpdate)
        {
            printf("--> Write Factory Data\r\n");
            memset(&fData,0, sizeof(FactoryData));
            memcpy(&fData,&fDataRd, sizeof(FactoryData));

            if (probeTypUpdate == true)
            {
                fData.probeType = probeType;
            }

            if (strlen(sNo) > 32)
            {
                printf("\n SN Characters exceed more than 32. String Truncated");
                sNo[sizeof(fData.serialNum)] = '\0';
            }

            if (snUpdate == true)
            {
                strcpy(fData.serialNum, sNo);
            }

            if (strlen(pNo) > 32)
            {
                printf("\n Part No Characters exceed more than 32. String Truncated");
                pNo[sizeof(fData.serialNum)] = '\0';
            }

            if (pnUpdate == true)
            {
                memcpy(fData.partNum, pNo, sizeof(fData.partNum));
                strcpy(fData.partNum, pNo);
            }

            if (dauContext.isUsbContext)
            {
                wrfactDatlen = sizeof(FactoryData) + sizeof(FactoryDataKey);
                memset(wrFactData, 0, wrfactDatlen);   // Set  with 0

                fDataKey.key0 = FACTORY_DATA_KEY0;
                fDataKey.key1 = FACTORY_DATA_KEY1;

                memcpy(wrFactData, &fDataKey, sizeof(FactoryDataKey));
                memcpy(wrFactData + sizeof(FactoryDataKey), &fData, sizeof(FactoryData));


                if (!dauFwUpMem->writeFactData((uint32_t *) wrFactData, SE_FB_MEM_START_ADDR,
                                               wrfactDatlen))
                {
                    printf("Fail to write Factory Data\r\n");
                }
                else
                {
                    printf("Write Factory data successfully\n");
                }
            }
            else    /* PCIe no need to add  FactoryDataKey */
            {
                if (!dauFwUpMem->writeFactData((uint32_t*)&fData, FACTORY_DATA_START,
                                               sizeof(FactoryData)))
                {
                    printf("Fail to write Factory Data\r\n");
                }
                else
                {
                    printf("Write Factory data successfully\n");
                }
            }
        }
        else
        {
            usage(argv[0]);
        }
    }

    retVal = true;
    err_ret:
    return retVal;
}
