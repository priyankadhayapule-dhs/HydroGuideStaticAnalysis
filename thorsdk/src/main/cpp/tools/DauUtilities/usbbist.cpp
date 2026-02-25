//
// Copyright (C) 2017 EchoNous, Inc.
//

#define LOG_TAG "usbbist"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string>
#include <libusb/libusb.h>

#include <DauContext.h>
#include <DauFwUpgrade.h>
#include <DauRegisters.h>
#include <DauPower.h>
#include <ThorUtils.h>
#include <ThorError.h>
#include <UsbDauFwUpgrade.h>
#include <UsbDauMemory.h>

#define BUF_SZ 64
DauContext                      dauContext;

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
int main()
{
    UsbTlv tlv;
    int actual_len, retVal, testVal;
    uint32_t logBufferLength,mBISTLogBufLength;
    unsigned char *logBuffer;
    char                            path[PATH_MAX] = "/dev/bus/usb/";
    char                            buffer[BUF_SZ];
    libusb_device_handle*           mUsbHandlePtr;
    UsbDauMemory                    usbDauMem;

    retVal = getUsbPath(buffer);
    if(retVal < 0)
    {
        printf("Can't get Usb Device Path\n");
        exit(-1);
    }
    strcat(path, buffer);

    dauContext.usbFd = open(path, O_RDWR);

    if (-1 == dauContext.usbFd)
    {
        printf("Unable to open Usb device %s\n", path);
        printf("errno = %d\n", errno);
        exit(-1);
    }
    printf("USB Path : %s\r\n", path);

    if(getDeviceClass(dauContext.usbFd) == LIBUSB_CLASS_APPLICATION)
    {
        printf("Dau is off.Please turn on the dau using 'dauusbpower' utility\n");
        exit(0);
    }
    else
    {
        usbDauMem.map(&dauContext);
        mUsbHandlePtr = usbDauMem.getDeviceHandle();

        printf("********************************************\r\n");
        printf("\t\t    USB BIST Test Menu\r\n");
        printf("********************************************\r\n");
        printf("%d  :  Test All\r\n", TEST_BIST);
        printf("%d  :  Test Registers\r\n", TEST_REG_TEST);
        printf("%d  :  Test Memory\r\n", TEST_MEM_TEST);
        printf("%d  :  Test Seq LLE\r\n", TEST_SEQ_LLE);
        printf("%d  :  Test Timer\r\n", TEST_FTTMR);
        printf("%d  :  Test WDT\r\n", TEST_FTWDT);
        printf("%d  :  Test SPIF\r\n", TEST_SPIF);
        printf("%d  :  Test I2C\r\n", TETS_I2C);
        printf("%d  :  Test SSPI\r\n", TEST_SSPI);
        printf("%d  :  Test SSI2C\r\n", TEST_SSI2C);
        printf("%d :  Test SSSPI\r\n", TEST_SSSPI);
        printf("%d :  Test TXSPI\r\n", TEST_TXSPI);
        printf("%d :  Test HVSUP\r\n", TEST_HVSUP);
        printf("%d :  Test FTDMA\r\n", TEST_FTDMA);
        //TODO:Currently this test is disabled
//    printf("%d :  Test INT\r\n",TEST_INT);
        printf("%d :  Test AFE\r\n", TEST_AFE);
        printf("%d :  Test FCS\r\n", TEST_FCS);
        printf("%d :  Test ASPM\r\n", TEST_ASPM);
        printf("%d :  Test PDMA\r\n", TEST_PDMA);
        printf("********************************************\r\n");
        scanf("%d", &testVal);
        printf("Entered Val : %d \n", testVal);

        memset(&tlv, '\0', sizeof(UsbTlv));

        tlv.tag = TLV_TAG_BIST;
        tlv.length = 0;
        tlv.values[0] = testVal;
        tlv.values[1] = 0;

        libusb_control_transfer(mUsbHandlePtr,
                                LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
                                0x1,
                                0,
                                0,
                                (unsigned char *) &tlv,
                                sizeof(UsbTlv),
                                0);

        memset(&tlv, '\0', sizeof(UsbTlv));

        libusb_control_transfer(mUsbHandlePtr,
                                LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR,
                                0x1,
                                0,
                                0,
                                (unsigned char *) &tlv,
                                sizeof(UsbTlv),
                                0);
        switch (tlv.values[0]) {
            case TLV_OK:
                ALOGI("TLV OK");
                printf("TLV OK\n");
                break;
            case TLV_INVALID:
                ALOGE("TLV_INVALID");
                printf("TLV_INVALID");
                goto err_ret;
            case TLVERR_ERROR:
                ALOGE("TLVERR_ERROR");
                printf("TLVERR_ERROR");
                goto err_ret;
            case TLVERR_BADPARAMETER:
                ALOGE("TLVERR_BADPARAMETER");
                printf("TLVERR_BADPARAMETER");
                goto err_ret;
        }

        retVal = libusb_bulk_transfer(mUsbHandlePtr,
                                      BIST_EP | LIBUSB_ENDPOINT_IN,
                                      (unsigned char *) &logBufferLength,
                                      sizeof(uint32_t),
                                      &actual_len,
                                      0);
        if (retVal != 0) {
            ALOGE("Bulk Transfer failed:%d", retVal);
            goto err_ret;
        }

        mBISTLogBufLength = (uint32_t) logBufferLength;

        logBuffer = (unsigned char *) malloc(mBISTLogBufLength);
        if (logBuffer == NULL) {
            printf("logBuffer:Memory Allocation Failed\n");
            goto err_ret;
        }

        memset(&tlv, '\0', sizeof(UsbTlv));
        memset(logBuffer, '\0', sizeof(UsbTlv));

        tlv.tag = TLV_TAG_BIST_LOG;
        tlv.length = 0;
        tlv.values[0] = 0;

        libusb_control_transfer(mUsbHandlePtr,
                                LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
                                0x1,
                                0,
                                0,
                                (unsigned char *) &tlv,
                                sizeof(UsbTlv),
                                0);

        memset(&tlv, '\0', sizeof(UsbTlv));

        libusb_control_transfer(mUsbHandlePtr,
                                LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR,
                                0x1,
                                0,
                                0,
                                (unsigned char *) &tlv,
                                sizeof(UsbTlv),
                                0);

        switch (tlv.values[0]) {
            case TLV_OK:
                ALOGI("TLV OK");
                break;
            case TLV_INVALID:
                ALOGE("TLV_INVALID");
                printf("TLV_INVALID");
                goto err_ret;
            case TLVERR_ERROR:
                ALOGE("TLVERR_ERROR");
                printf("TLVERR_ERROR");
                goto err_ret;
            case TLVERR_BADPARAMETER:
                ALOGE("TLVERR_BADPARAMETER");
                printf("TLVERR_BADPARAMETER");
                goto err_ret;
        }

        if (mBISTLogBufLength) {
            retVal = libusb_bulk_transfer(mUsbHandlePtr,
                                          BIST_EP | LIBUSB_ENDPOINT_IN,
                                          logBuffer,
                                          mBISTLogBufLength,
                                          &actual_len,
                                          0);
        } else {
            ALOGE("Please Execute BIST Test First\n");
            goto err_ret;
        }

        if (retVal != 0) {
            ALOGE("Bulk Transfer failed:%d", retVal);
            goto err_ret;
        }

        printf("BIST Logs\n");
        printf("================================================================================================================\n");
        printf("%s", logBuffer);
        printf("================================================================================================================\n");

        free(logBuffer);
        usbDauMem.unmap();
    }
    return 0;
    err_ret:
    return -1;
}
