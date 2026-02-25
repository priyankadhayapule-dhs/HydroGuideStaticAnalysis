//
// Copyright (C) 2017 EchoNous, Inc.
//

#define LOG_TAG "UsbDauFwUpgrade"

#include <UsbDauFwUpgrade.h>
#include <libusb/libusb.h>
#include <UsbDauMemory.h>

const char* const UsbDauFwUpgrade::DEFAULT_BIN_FILE = "FwUpgrade.bin";
const char* const UsbDauFwUpgrade::DEFAULT_BL_BIN_FILE = "BLFwUpgrade.bin";

//-----------------------------------------------------------------------------
UsbDauFwUpgrade::UsbDauFwUpgrade():
        mDauContextPtr(nullptr),
        mUsbInitialized(false),
        mUsbHandlePtr(nullptr),
        mBISTLogBufLength(0),
        mCommonFwFlag(false)
{
}

//-----------------------------------------------------------------------------
UsbDauFwUpgrade::~UsbDauFwUpgrade()
{
    deInit();
}

//-----------------------------------------------------------------------------
bool UsbDauFwUpgrade::init(DauContext* dauContextPtr)
{
    int                             ret;
    const struct libusb_version*    version;
    int                             usbFd;
    int                             deviceDescClass;

    mDauContextPtr = dauContextPtr;

    version = libusb_get_version();
    ALOGD("Using libusb v%d.%d.%d.%d\n",
          version->major,
          version->minor,
          version->micro,
          version->nano);

    ret = libusb_init(nullptr);
    if (ret < 0)
    {
        ALOGE("libusb_init failed: %s", libusb_error_name(ret));
        goto err_ret;
    }
    mUsbInitialized = true;


    usbFd = libusb_wrap_fd(nullptr, mDauContextPtr->usbFd, &mUsbHandlePtr);
    if (0 != usbFd) {
        ALOGE("Unable to wrap libusb fd %s",libusb_error_name(usbFd));
        goto err_ret;
    }

    ret = libusb_claim_interface(mUsbHandlePtr, 0);
    if(ret < 0)
    {
        ALOGE("Cannot Claim Interface: ret = %d", ret);
        goto err_ret;
    }
    ALOGD("Claimed Interface");

    deviceDescClass = getDeviceDescClass();
    mCommonFwFlag = (deviceDescClass == LIBUSB_CLASS_MISC_DEVICE);

    return(true);
    err_ret:
    return(false);
}

//-----------------------------------------------------------------------------
void UsbDauFwUpgrade::deInit()
{
    mCommonFwFlag = false;
    if (nullptr != mUsbHandlePtr)
    {
        ALOGD("%s: Releasing usb interface", __func__);
        libusb_release_interface(mUsbHandlePtr, 0);
        libusb_close(mUsbHandlePtr);
        mUsbHandlePtr = nullptr;
    }

    if (mUsbInitialized)
    {
        libusb_exit(nullptr);
        mUsbInitialized = false;
    }
}

//-----------------------------------------------------------------------------
bool UsbDauFwUpgrade::getFwInfo(FwInfo *fwHeader)
{
    UsbTlv tlv;
    int actual_len, retVal;
    int device,inEp,outEp;

    if(NULL == fwHeader)
    {
        ALOGE("Firmware Header Not Found");
    }

    if(mCommonFwFlag)
    {
        inEp = EP2;
        outEp = EP3;
    }
    else
    {
        device = getDeviceClass();
        if (device == LIBUSB_CLASS_APPLICATION) /* Boot-Loader*/
        {
            inEp = EP1;
            outEp = EP2;
        } else        /* Fw App */
        {
            inEp = EP2;
            outEp = EP3;
        }
    }

    memset(&tlv, '\0', sizeof(UsbTlv));

    tlv.tag = TLV_TAG_GET_FIRMWARE_INFO;
    tlv.length = 0;
    tlv.values[0] = 0;

    retVal = libusb_control_transfer(mUsbHandlePtr,
                                     LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
                                     0x1,
                                     0,
                                     0,
                                     (unsigned char *)&tlv,
                                     sizeof(UsbTlv),
                                     UsbDauTimeout::SYNC);
    if (retVal < 0)
    {
        ALOGE("Error during control transfer. Response: %s", libusb_error_name(retVal));
        goto err_ret;
    }

    memset(&tlv, '\0', sizeof(UsbTlv));

    retVal = libusb_control_transfer(mUsbHandlePtr,
                                     LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR,
                                     0x1,
                                     0,
                                     0,
                                     (unsigned char *)&tlv,
                                     sizeof(UsbTlv),
                                     UsbDauTimeout::SYNC);
    if (retVal < 0)
    {
        ALOGE("Error during control transfer. Response: %s", libusb_error_name(retVal));
        goto err_ret;
    }

    switch(tlv.values[0])
    {
        case TLV_OK:
            ALOGI("TLV OK");
            break;
        case TLV_INVALID:
            ALOGE("TLV_INVALID");
            goto err_ret;
        case TLVERR_ERROR:
            ALOGE("TLVERR_ERROR");
            goto err_ret;
        case TLVERR_BADPARAMETER:
            ALOGE("TLVERR_BADPARAMETER");
            goto err_ret;
    }

    retVal = libusb_bulk_transfer(mUsbHandlePtr,
                                  inEp | LIBUSB_ENDPOINT_IN,
                                  (unsigned char *)fwHeader,
                                  sizeof(FwInfo),
                                  &actual_len,
                                  UsbDauTimeout::SYNC);
    if(retVal != 0)
    {
        ALOGE("Bulk Transfer failed:%d",retVal);
        goto err_ret;
    }

    if(fwHeader->bootUpdateFlag1 == fwHeader->bootUpdateFlag2)
    {
        ALOGE("Error in bootUpdateFlag\n");
        goto err_ret;
    }

    return true;

    err_ret:
    ALOGE("Fail to read FwInfo\n");
    return false;
}

//-----------------------------------------------------------------------------
bool UsbDauFwUpgrade::uploadImage(uint32_t *srcAddress, uint32_t dauAddress, uint32_t imgLen)
{
    int actual_len, rc;

    uint32_t *srcAddressPtr = srcAddress;
    UsbTlv tlv;
    uint32_t bulkin_rsp;
    int byte_recvd, device,inEp,outEp;

    if(mCommonFwFlag)
    {
        inEp = EP2;
        outEp = EP3;
    }
    else
    {
        device = getDeviceClass();
        if (device == LIBUSB_CLASS_APPLICATION) /* Boot-Loader*/
        {
            inEp = EP1;
            outEp = EP2;
        } else        /* Fw App */
        {
            inEp = EP2;
            outEp = EP3;
        }
    }

    memset(&tlv, 0, sizeof(UsbTlv));

    tlv.tag = TLV_TAG_UPLOAD_IMAGE;
    tlv.length = 2;
    tlv.values[0] = dauAddress;
    tlv.values[1] = imgLen;

    rc = libusb_control_transfer(mUsbHandlePtr,
                                 LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
                                 0x0,
                                 0,
                                 0,
                                 (unsigned char *) &tlv/*buffer*/,
                                 sizeof(UsbTlv),
                                 UsbDauTimeout::SYNC);

    if (rc < 0) /* Control Transfer returns error in minus */
    {
        ALOGE("Error during control transfer: %s Line:%d\n",libusb_error_name(rc),__LINE__);
        goto err_ret;
    }

    rc = libusb_bulk_transfer(mUsbHandlePtr,
                              outEp | LIBUSB_ENDPOINT_OUT,
                              (unsigned char *) srcAddressPtr,
                              imgLen,
                              &actual_len,
                              UsbDauTimeout::SYNC);

    if (rc != 0)    /* Bulk transfer LIBUSB_SUCCESS = 0 */
    {
        ALOGE("Error during Bulk transfer: %s Line:%d\n",libusb_error_name(rc),__LINE__);
        goto err_ret;
    }

    memset(&tlv, 0, sizeof(UsbTlv));

    libusb_control_transfer(mUsbHandlePtr,
                            LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR,
                            0x0,
                            0,
                            0,
                            (unsigned char *) &tlv/*buffer*/,
                            sizeof(UsbTlv),
                            UsbDauTimeout::SYNC);

    if (rc < 0) /* Control Transfer returns error in minus */
    {
        ALOGE("Error during control transfer: %s Line:%d\n",libusb_error_name(rc),__LINE__);
        goto err_ret;
    }

    switch (tlv.values[0])
    {
        case TLV_OK:
            ALOGI("TLV OK");
            break;
        case TLV_INVALID:
            ALOGE("TLV_INVALID\n");
            goto err_ret;
        case TLVERR_ERROR:
            ALOGE("TLVERR_ERROR");
            goto err_ret;
        case TLVERR_BADPARAMETER:
            ALOGE("TLVERR_BADPARAMETER");
            goto err_ret;
        default:
            ALOGE("TLVERR_UNKNOWN");
            goto err_ret;
    }

    rc = libusb_bulk_transfer(mUsbHandlePtr,
                              LIBUSB_ENDPOINT_IN | inEp,
                              (unsigned char *) &bulkin_rsp,
                              sizeof(uint32_t),
                              &byte_recvd,
                              UsbDauTimeout::FIRMWARE /*10 sec*/);

    if (rc != 0)    /* Bulk transfer LIBUSB_SUCCESS = 0 */
    {
        ALOGE("Error during Bulk transfer: %s Line:%d\n",libusb_error_name(rc),__LINE__);
        goto err_ret;
    }
    else
    {
        switch(bulkin_rsp)
        {
            case IMAGE_SUCCESS :
                ALOGI("FLASH_WRT_SUCCESS\n");
                break;
            case INVALID_IMAGE :
                ALOGE("Invalid Image \n");
                goto err_ret;
                break;
            case IMAGE_FLASH_ERROR :
                ALOGE("Flash Write Fail\n");
                goto err_ret;
                break;
            default:
                ALOGE("Invalid Response:0x%x\n",bulkin_rsp);
                goto err_ret;
        }
    }

    return(true);
    err_ret:
    return(false);
}

//-----------------------------------------------------------------------------
bool UsbDauFwUpgrade::rebootDevice()
{
    UsbTlv tlv;
    int rc;

    memset(&tlv, '\0', sizeof(UsbTlv));
    tlv.tag = TLV_TAG_REBOOT;
    tlv.length = 0;
    tlv.values[0] = 0;

    rc = libusb_control_transfer(mUsbHandlePtr,
                                 LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
                                 0x1,
                                 0,
                                 0,
                                 (unsigned char *)&tlv,
                                 sizeof(UsbTlv),
                                 UsbDauTimeout::SYNC);

    if (rc < 0) /* Control Transfer returns error in minus */
    {
        ALOGE("Error during control transfer: %s Line:%d\n",libusb_error_name(rc),__LINE__);
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
uint32_t UsbDauFwUpgrade::computeCRC (uint8_t *buf, uint32_t len)
{

    uint32_t crc = 0xffffffff;

    while (len--)
    {
        crc = (crc << 8) ^ crc32Table[((crc >> 24) ^ *buf) & 255];
        buf++;
    }
    return crc;
}

//-----------------------------------------------------------------------------
bool UsbDauFwUpgrade::writeFactData(uint32_t *srcAddress, uint32_t dauAddress, uint32_t factDataLen)
{
    int actual_len, rc;

    uint32_t *srcAddressPtr = srcAddress;
    UsbTlv tlv;
    uint32_t bulkin_rsp;
    int byte_recvd,device,inEp,outEp;;

    memset(&tlv, 0, sizeof(UsbTlv));

    tlv.tag = TLV_TAG_WRITE_FACTORY_DATA;
    tlv.length = 2;
    tlv.values[0] = dauAddress;
    tlv.values[1] = factDataLen;

    if(mCommonFwFlag)
    {
        inEp = EP2;
        outEp = EP3;
    }
    else
    {
        device = getDeviceClass();
        if (device == LIBUSB_CLASS_APPLICATION) /* Boot-Loader*/
        {
            inEp = EP1;
            outEp = EP2;
        } else        /* Fw App */
        {
            inEp = EP2;
            outEp = EP3;
        }
    }

    rc = libusb_control_transfer(mUsbHandlePtr,
                                 LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
                                 0x0,
                                 0,
                                 0,
                                 (unsigned char *) &tlv,
                                 sizeof(UsbTlv),
                                 UsbDauTimeout::SYNC);

    if (rc < 0) /* Control Transfer returns error in minus */
    {
        ALOGE("Error during control transfer: %s Line:%d\n",libusb_error_name(rc),__LINE__);
        goto err_ret;
    }

    rc = libusb_bulk_transfer(mUsbHandlePtr,
                              LIBUSB_ENDPOINT_OUT | outEp,
                              (unsigned char *) srcAddressPtr,
                              factDataLen,
                              &actual_len,
                              UsbDauTimeout::SYNC);

    if (rc != 0)    /* Bulk transfer LIBUSB_SUCCESS = 0 */
    {
        ALOGE("Error during Bulk transfer: %s Line:%d\n",libusb_error_name(rc),__LINE__);
        goto err_ret;
    }

    memset(&tlv, 0, sizeof(UsbTlv));

    rc = libusb_control_transfer(mUsbHandlePtr,
                                 LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR,
                                 0x0,
                                 0,
                                 0,
                                 (unsigned char *) &tlv/*buffer*/,
                                 sizeof(UsbTlv),
                                 UsbDauTimeout::SYNC);

    if (rc < 0) /* Control Transfer returns error in minus */
    {
        ALOGE("Error during control transfer: %s Line:%d\n",libusb_error_name(rc),__LINE__);
        goto err_ret;
    }

    switch (tlv.values[0])
    {
        case TLV_OK:
            ALOGI("TLV OK");
            break;
        case TLV_INVALID:
            ALOGE("TLV_INVALID\n");
            goto err_ret;
        case TLVERR_ERROR:
            ALOGE("TLVERR_ERROR");
            goto err_ret;
        case TLVERR_BADPARAMETER:
            ALOGE("TLVERR_BADPARAMETER");
            goto err_ret;
        default:
            ALOGE("TLVERR_UNKNOWN");
            goto err_ret;
    }

    rc = libusb_bulk_transfer(mUsbHandlePtr,
                              LIBUSB_ENDPOINT_IN | inEp,
                              (unsigned char *)&bulkin_rsp,
                              sizeof(uint32_t),
                              &byte_recvd,
                              UsbDauTimeout::FIRMWARE /*10 sec*/);

    if(rc == 0)
    {
        switch(bulkin_rsp)
        {
            case IMAGE_SUCCESS :
                ALOGI("Factory Data Wrote Successfully\n");
                break;
            case INVALID_KEY :
                ALOGE("Key Mismatched. Can't unlock Partition \n");
                goto err_ret;
                break;
            case IMAGE_FLASH_ERROR :
                ALOGE("Flash Write Fail\n");
                goto err_ret;
                break;
            default:
                ALOGE("Invalid Response:0x%x\n",bulkin_rsp);
                goto err_ret;
        }
    }
    else
    {
        ALOGE("Error during Bulk transfer: %s Line:%d\n",libusb_error_name(rc),__LINE__);
        goto err_ret;
    }

    return(true);
    err_ret:
    return(false);
}

//-----------------------------------------------------------------------------
bool UsbDauFwUpgrade::readFactData(uint32_t *rData, uint32_t dauAddress, uint32_t factDataLen)
{
    UsbTlv tlv;
    int actual_len, retVal ,rc;
    int device,inEp,outEp;

    memset(&tlv, '\0', sizeof(UsbTlv));

    tlv.tag = TLV_TAG_READ_FACTORY_DATA;
    tlv.length = 0;
    tlv.values[0] = dauAddress; //add
    tlv.values[1] = factDataLen;                  // length

    if(mCommonFwFlag)
    {
        inEp = EP2;
        outEp = EP3;
    }
    else
    {
        device = getDeviceClass();
        if (device == LIBUSB_CLASS_APPLICATION) /* Boot-Loader*/
        {
            inEp = EP1;
            outEp = EP2;
        } else        /* Fw App */
        {
            inEp = EP2;
            outEp = EP3;
        }
    }

    rc = libusb_control_transfer(mUsbHandlePtr,
                                 LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
                                 0x1,
                                 0,
                                 0,
                                 (unsigned char *)&tlv,
                                 sizeof(UsbTlv),
                                 UsbDauTimeout::SYNC);

    if (rc < 0) /* Control Transfer returns error in minus */
    {
        ALOGE("Error during control transfer: %s Line:%d\n",libusb_error_name(rc),__LINE__);
        goto err_ret;
    }

    memset(&tlv, '\0', sizeof(UsbTlv));

    libusb_control_transfer(mUsbHandlePtr,
                            LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR,
                            0x1,
                            0,
                            0,
                            (unsigned char *)&tlv,
                            sizeof(UsbTlv),
                            UsbDauTimeout::SYNC);

    if (rc < 0) /* Control Transfer returns error in minus */
    {
        ALOGE("Error during control transfer: %s Line:%d\n",libusb_error_name(rc),__LINE__);
        goto err_ret;
    }

    switch(tlv.values[0])
    {
        case TLV_OK:
            ALOGI("TLV OK");
            break;
        case TLV_INVALID:
            ALOGE("TLV_INVALID");
            goto err_ret;
        case TLVERR_ERROR:
            ALOGE("TLVERR_ERROR");
            goto err_ret;
        case TLVERR_BADPARAMETER:
            ALOGE("TLVERR_BADPARAMETER");
            goto err_ret;
    }

    rc = libusb_bulk_transfer(mUsbHandlePtr,
                              inEp | LIBUSB_ENDPOINT_IN,
                              (unsigned char *)rData,
                              factDataLen,
                              &actual_len,
                              UsbDauTimeout::SYNC);

    if(rc != 0)   /* Bulk transfer LIBUSB_SUCCESS = 0 */
    {
        ALOGE("Error during Bulk transfer: %s Line:%d\n",libusb_error_name(rc),__LINE__);
        goto err_ret;
    }

    return true;

    err_ret:
    ALOGE("Fail to read Factory Data\n");
    return false;
}

//-----------------------------------------------------------------------------
uint8_t UsbDauFwUpgrade::getDeviceClass()
{
    char tBuff[sizeof(libusb_config_descriptor)];
    libusb_get_descriptor(mUsbHandlePtr,LIBUSB_DT_CONFIG,0,(unsigned char *)&tBuff,sizeof(libusb_config_descriptor));
    return tBuff[DEVICE_CLASS_OFFSET];
}

//-----------------------------------------------------------------------------
uint8_t UsbDauFwUpgrade::getDeviceDescClass()
{
    char tBuff[sizeof(libusb_device_descriptor)];
    libusb_get_descriptor(mUsbHandlePtr,LIBUSB_DT_DEVICE,0,(unsigned char *)&tBuff,sizeof(libusb_config_descriptor));
    return tBuff[DEVICE_DESC_CLASS_OFFSET];
}