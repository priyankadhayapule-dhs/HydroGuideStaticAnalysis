//
// Copyright (C) 2016 EchoNous, Inc.
//

#define LOG_TAG "dauusbpower"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string>
#include <libusb/libusb.h>

#include <ThorError.h>
#include <ThorUtils.h>
#include <DauFwUpgrade.h>
#include <DauContext.h>
#include <UsbDauMemory.h>
#include <UsbDauFwUpgrade.h>

#define BUF_SZ 64

const char* gParamOn = "on";
const char* gParamOff = "off";
const char* gParamQuery = "query";

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void usage(char utility[])
{
    printf("Usage: %s <command>\n",utility);
    printf("\n  command can be one of the following:\n");
    printf("    query\tDisplay current Dau FW in execution\n");
    printf("    on\t\tTurn the Dau on. Executes the Application FW.\n");
    printf("    off\t\tTurn the Dau off. Executes the Bootloader FW.\n");
}

//-----------------------------------------------------------------------------
void rebootDevice(int fd)
{
    libusb_device_handle*   usbHandlePtr = NULL;
    int                     rc = THOR_OK;
    UsbTlv                  tlv;

    rc = libusb_init(nullptr);
    if(rc < 0)
    {
        printf("libusb_init failed: %s", libusb_error_name(rc));
        exit(-1);
    }

    // Open USB (Dau) device
    if (0 != libusb_wrap_fd(nullptr, fd, &usbHandlePtr))
    {
        printf("Unable to wrap libusb fd");
        exit(-1);
    }

    libusb_claim_interface(usbHandlePtr, 0);


    memset(&tlv, '\0', sizeof(UsbTlv));
    tlv.tag = TLV_TAG_REBOOT;
    tlv.length = 0;
    tlv.values[0] = 0;

    libusb_control_transfer(usbHandlePtr,
                            LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
                            0x1,
                            0,
                            0,
                            (unsigned char *)&tlv,
                            sizeof(UsbTlv),
                            0);

    libusb_release_interface(usbHandlePtr, 0);
    libusb_close(usbHandlePtr);
    libusb_exit(nullptr);
}

//-----------------------------------------------------------------------------
int bootApplicationImage(int fd)
{
    libusb_device_handle*   usbHandlePtr = NULL;
    int                     rc = THOR_OK;
    UsbTlv                  tlv;

    rc = libusb_init(nullptr);
    if(rc < 0)
    {
        printf("libusb_init failed: %s", libusb_error_name(rc));
        rc = THOR_ERROR;
        goto err_ret;
    }

    // Open USB (Dau) device
    if (0 != libusb_wrap_fd(nullptr, fd, &usbHandlePtr))
    {
        printf("Unable to wrap libusb fd");
        rc = THOR_ERROR;
        goto err_ret;
    }

    libusb_claim_interface(usbHandlePtr, 0);

    tlv.tag = TLV_TAG_BOOT_APPLICATION_IMAGE;
    tlv.length = 0;
    tlv.values[0] = 0;

    libusb_control_transfer(usbHandlePtr,
                            LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
                            0x0,
                            0,
                            0,
                            (unsigned char *) &tlv,
                            sizeof(tlv),
                            0);

    libusb_release_interface(usbHandlePtr, 0);
    libusb_close(usbHandlePtr);
    libusb_exit(nullptr);
    return rc;
    err_ret:
    return(rc);
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
    unsigned char tmpBuf[64];
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
    int                     retVal = -1;
    char                    blPath[PATH_MAX]="/dev/bus/usb/";
    char                    buffer[BUF_SZ];
    DauContext              dauContext;

    if (2 != argc)
    {
        usage(argv[0]);
        goto err_ret;
    }

    retVal = getUsbPath(buffer);
    if(retVal < 0)
    {
        printf("Can't get Usb Device Path\n");
        goto err_ret;
    }

    strcat(blPath,buffer);
    dauContext.usbFd = open(blPath, O_RDWR);
    if (-1 == dauContext.usbFd)
    {
        printf("Unable to open Usb device %s\n", blPath);
        exit(-1);
    }

    if (0 == strncasecmp(gParamOn, argv[1], strlen(gParamOn)))
    {
        if(getDeviceClass(dauContext.usbFd) == LIBUSB_CLASS_VENDOR_SPEC)
        {
            printf("Dau is already On. Application FW is in execution.\n");
            exit(0);
        }
        bootApplicationImage(dauContext.usbFd);
        printf("Dau is On.Application FW executed.\n");
    }
    else if (0 == strncasecmp(gParamOff, argv[1], strlen(gParamOff)))
    {
        if(getDeviceClass(dauContext.usbFd) == LIBUSB_CLASS_APPLICATION)
        {
            printf("Dau is already off. Bootloader FW is in execution.\n");
            exit(0);
        }
        rebootDevice(dauContext.usbFd);
        usleep(1000000);
        printf("Dau is off.Bootloader FW executed.\n");
    }
    else if (0 == strncasecmp(gParamQuery, argv[1], strlen(gParamQuery)))
    {
        uint8_t deviceClass = getDeviceClass(dauContext.usbFd);
        if(deviceClass == LIBUSB_CLASS_APPLICATION)
        {
            printf("Dau is off.Bootloader FW in execution.\n");
        }
        else if(deviceClass == LIBUSB_CLASS_VENDOR_SPEC)
        {
            printf("Dau is on.Application FW in execution.\n");
        }
        else
        {
            printf("Usb Device is not connected. Device Class: %u\n", deviceClass);
        }
    }
    else
    {
        usage(argv[0]);
        goto err_ret;
    }
    retVal = 0;

    err_ret:
    return(retVal);
}

