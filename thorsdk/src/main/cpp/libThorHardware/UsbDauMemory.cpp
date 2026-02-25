//
// Copyright (C) 2017 EchoNous, Inc.
//

#define LOG_TAG "UsbDauMemory"

#include <UsbDauMemory.h>
#include <UsbDataHandler.h>
#include <libusb.h>
#include <DauFwUpgrade.h>
#include <ThorError.h>

//-----------------------------------------------------------------------------
UsbDauMemory::UsbDauMemory():
        mDauContextPtr(nullptr),
        mUsbInitialized(false),
        mUsbHandlePtr(nullptr),
        mIsInterfaceClaimed(false)
{
}

//-----------------------------------------------------------------------------
UsbDauMemory::~UsbDauMemory()
{
    unmap();
}

//-----------------------------------------------------------------------------
bool UsbDauMemory::map()
{
    return(true);
}

//-----------------------------------------------------------------------------
bool UsbDauMemory::map(DauContext* dauContextPtr)
{
    mDauContextPtr = dauContextPtr;
    initializeErrorEvent();
    return(initLibusb());
}

//-----------------------------------------------------------------------------
void UsbDauMemory::unmap()
{
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
bool UsbDauMemory::initLibusb()
{
    int                             ret;
    const struct libusb_version*    version;
    int                             usbFd;

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

    /*  find out if kernel driver is attached   */
    if(libusb_kernel_driver_active(mUsbHandlePtr, 0) == 1)
    {
        ALOGI("Kernel Driver Active");
        if(libusb_detach_kernel_driver(mUsbHandlePtr, 0) == 0)
            ALOGI("Kernel Driver Detached!");
    }

    ret = libusb_claim_interface(mUsbHandlePtr, 0);
    if(ret < 0)
    {
        ALOGE("Cannot Claim Interface: ret = %d", ret);
        goto err_ret;
    }
    ALOGD("Claimed Interface");
    mIsInterfaceClaimed = true;
    return(true);

    err_ret:
    return(false);
}

//-----------------------------------------------------------------------------
void UsbDauMemory::initializeErrorEvent()
{
    mDauMemErrorEventSPtr = std::make_shared<TriggeredValue>();
    mDauMemErrorEventSPtr->init();
    mDauContextPtr->usbDauMemPollFd.initialize(mDauMemErrorEventSPtr);
}

//-----------------------------------------------------------------------------
size_t UsbDauMemory::getMapSize()
{
    return(UINT32_MAX);
}

//-----------------------------------------------------------------------------
bool UsbDauMemory::isInterfaceClaimed()
{
    return mIsInterfaceClaimed;
}

//-----------------------------------------------------------------------------
bool UsbDauMemory::read(uint32_t dauAddress, uint32_t *destAddress, uint32_t numWords)
{
    bool retVal = false;
    int actual_len ,rc;
    uint32_t addr = dauAddress;
    uint32_t words = numWords;
    UsbTlv tlv;

    mLock.enter();

    if (nullptr == mUsbHandlePtr)
    {
        goto err_ret;
    }

    if (1 == words)
    {
        memset(&tlv, 0, sizeof(UsbTlv));

        tlv.tag = TLV_TAG_REG_READ;
        tlv.length = 1;
        tlv.values[0] = addr;

        rc = libusb_control_transfer(mUsbHandlePtr,
                                     LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
                                     0x0,
                                     0,
                                     0,
                                     (unsigned char *) &tlv,
                                     sizeof(UsbTlv),
                                     UsbDauTimeout::SYNC);
        if (rc < 0) /* Control Transfer return in error in minus */
        {
            if(rc == LIBUSB_ERROR_NO_DEVICE)
            {
                ALOGE("Error during Read control transfer: %s\n",libusb_error_name(rc));
                mDauMemErrorEventSPtr->set((uint64_t) TER_IE_USB_NO_DEVICE);
            }
            else {
                ALOGE("Error during Read control transfer: %s\n", libusb_error_name(rc));
                mDauMemErrorEventSPtr->set((uint64_t) TER_IE_USB_ERROR);
            }
            goto err_ret;
        }

        memset(&tlv, 0, sizeof(UsbTlv));

        rc = libusb_control_transfer(mUsbHandlePtr,
                                     LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR,
                                     0x0,
                                     0,
                                     0,
                                     (unsigned char *) &tlv,
                                     sizeof(UsbTlv),
                                     UsbDauTimeout::SYNC);
        if (rc < 0) /* Control Transfer return in error in minus */
        {
            if(rc == LIBUSB_ERROR_NO_DEVICE)
            {
                ALOGE("Error during Read Response control transfer: %s\n",libusb_error_name(rc));
                mDauMemErrorEventSPtr->set((uint64_t) TER_IE_USB_NO_DEVICE);
            }
            else {
                ALOGE("Error during Read Response control transfer: %s\n",libusb_error_name(rc));
                mDauMemErrorEventSPtr->set((uint64_t) TER_IE_USB_ERROR);
            }
            goto err_ret;
        }

        switch (tlv.values[0])
        {
            case TLV_OK:
//                ALOGI("TLV OK");
                break;
            case TLV_INVALID:
                ALOGE("TLV_INVALID");
                mDauMemErrorEventSPtr->set((uint64_t) TER_IE_USB_TLV_INVALID);
                goto err_ret;
            case TLVERR_ERROR:
                ALOGE("TLVERR_ERROR: SRead Ad:%x Val:0x%x\n",dauAddress,numWords);
                mDauMemErrorEventSPtr->set((uint64_t) TER_IE_USB_TLV_ERROR);
                goto err_ret;
            case TLVERR_BADPARAMETER:
                ALOGE("TLVERR_BADPARAMETER ::SRead Ad:%x Val:0x%x\n",dauAddress,numWords);
                mDauMemErrorEventSPtr->set((uint64_t) TER_IE_USB_TLV_BADPARAMETER);
                goto err_ret;
        }
        *destAddress = tlv.values[1];
    }
    else if (words > 1)
    {
        memset(&tlv, 0, sizeof(UsbTlv));

        tlv.tag = TLV_TAG_BLK_READ;
        tlv.length = 2;
        tlv.values[0] = addr;
        tlv.values[1] = words * 4;

        rc =  libusb_control_transfer(mUsbHandlePtr,
                                      LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
                                      0x0,
                                      0,
                                      0,
                                      (unsigned char *) &tlv,
                                      sizeof(UsbTlv),
                                      UsbDauTimeout::SYNC);

        if (rc < 0) /* Control Transfer return in error in minus */
        {
            if(rc == LIBUSB_ERROR_NO_DEVICE)
            {
                ALOGE("Error during BRead control transfer: %s\n",libusb_error_name(rc));
                mDauMemErrorEventSPtr->set((uint64_t) TER_IE_USB_NO_DEVICE);
            }
            else {
                ALOGE("Error during BRead control transfer: %s\n",libusb_error_name(rc));
                mDauMemErrorEventSPtr->set((uint64_t) TER_IE_USB_ERROR);
            }
            goto err_ret;
        }

        rc = libusb_bulk_transfer(mUsbHandlePtr,
                                  EP2 | LIBUSB_ENDPOINT_IN,
                                  (unsigned char *) destAddress,
                                  words * sizeof(uint32_t),
                                  &actual_len,
                                  UsbDauTimeout::SYNC);

        if(rc != 0)   /* Bulk transfer LIBUSB_SUCCESS = 0 */
        {
            if(rc == LIBUSB_ERROR_NO_DEVICE)
            {
                ALOGE("Error during read Bulk transfer : %s\n",libusb_error_name(rc));
                mDauMemErrorEventSPtr->set((uint64_t) TER_IE_USB_NO_DEVICE);
            }
            else {
                ALOGE("Error during read Bulk transfer : %s\n",libusb_error_name(rc));
                mDauMemErrorEventSPtr->set((uint64_t) TER_IE_USB_ERROR);
            }
            goto err_ret;
        }

        memset(&tlv, 0, sizeof(UsbTlv));

        rc = libusb_control_transfer(mUsbHandlePtr,
                                     LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR,
                                     0x0,
                                     0,
                                     0,
                                     (unsigned char *) &tlv,
                                     sizeof(UsbTlv),
                                     UsbDauTimeout::SYNC);

        if (rc < 0) /* Control Transfer return in error in minus */
        {
            if(rc == LIBUSB_ERROR_NO_DEVICE)
            {
                ALOGE("Error during BRead Response control transfer: %s\n",libusb_error_name(rc));
                mDauMemErrorEventSPtr->set((uint64_t) TER_IE_USB_NO_DEVICE);
            }
            else {
                ALOGE("Error during BRead Response control transfer: %s\n",libusb_error_name(rc));
                mDauMemErrorEventSPtr->set((uint64_t) TER_IE_USB_ERROR);
            }

            goto err_ret;
        }

        switch (tlv.values[0]) {
            case TLV_OK:
                //ALOGI("TLV OK");
                break;
            case TLV_INVALID:
                ALOGE("TLV_INVALID");
                mDauMemErrorEventSPtr->set((uint64_t) TER_IE_USB_TLV_INVALID);
                goto err_ret;
            case TLVERR_ERROR:
                ALOGD("TLVERR_ERROR: BRead Ad:%x Val:0x%x\n",dauAddress,numWords);
                mDauMemErrorEventSPtr->set((uint64_t) TER_IE_USB_TLV_ERROR);
                goto err_ret;
            case TLVERR_BADPARAMETER:
                ALOGE("TLVERR_BADPARAMETER :BRead Ad:%x Val:0x%x\n",dauAddress,numWords);
                mDauMemErrorEventSPtr->set((uint64_t) TER_IE_USB_TLV_BADPARAMETER);
                goto err_ret;
        }
    }
    else
    {
        goto err_ret;
    }

    retVal = true;

err_ret:
    mLock.leave();
    return (retVal);
}

//-----------------------------------------------------------------------------
bool UsbDauMemory::write(uint32_t* srcAddress, uint32_t dauAddress, uint32_t numWords)
{
    bool retVal = false;
    int actualLen, rc;
    uint32_t addr = dauAddress;
    uint32_t words = numWords;
    uint32_t* srcAddressPtr = srcAddress;
    UsbTlv tlv;

    mLock.enter();

    if (nullptr == mUsbHandlePtr)
    {
        goto err_ret;
    }

    if(1 == words)
    {
        memset(&tlv,0,sizeof(UsbTlv));

        tlv.tag = TLV_TAG_REG_WRITE;
        tlv.length = 2;
        tlv.values[0] = addr;
        tlv.values[1] = *srcAddressPtr;

        rc = libusb_control_transfer(mUsbHandlePtr,
                                     LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
                                     0x0,
                                     0,
                                     0,
                                     (unsigned char *)&tlv,
                                     sizeof(UsbTlv),
                                     UsbDauTimeout::SYNC);
        if (rc < 0) /* Control Transfer returns error in minus */
        {
            if(rc == LIBUSB_ERROR_NO_DEVICE)
            {
                ALOGE("Error during BRead Response control transfer: %s\n",libusb_error_name(rc));
                mDauMemErrorEventSPtr->set((uint64_t) TER_IE_USB_NO_DEVICE);
            }
            else {
                ALOGE("Error during BRead Response control transfer: %s\n",libusb_error_name(rc));
                mDauMemErrorEventSPtr->set((uint64_t) TER_IE_USB_ERROR);
            }
            goto err_ret;
        }

        memset(&tlv, 0, sizeof(UsbTlv));

        rc = libusb_control_transfer(mUsbHandlePtr,
                                     LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR,
                                     0x0,
                                     0,
                                     0,
                                     (unsigned char *)&tlv,
                                     sizeof(UsbTlv),
                                     UsbDauTimeout::SYNC);

        if (rc < 0) /* Control Transfer return in error in minus */
        {
            if(rc == LIBUSB_ERROR_NO_DEVICE)
            {
                ALOGE("Error during BRead Response control transfer: %s\n",libusb_error_name(rc));
                mDauMemErrorEventSPtr->set((uint64_t) TER_IE_USB_NO_DEVICE);
            }
            else {
                ALOGE("Error during BRead Response control transfer: %s\n",libusb_error_name(rc));
                mDauMemErrorEventSPtr->set((uint64_t) TER_IE_USB_ERROR);
            }
            goto err_ret;
        }


        switch(tlv.values[0])
        {
            case TLV_OK:
//            ALOGI("TLV OK");
                break;

            case TLV_INVALID:
                ALOGE("TLV_INVALID");
                mDauMemErrorEventSPtr->set((uint64_t) TER_IE_USB_TLV_INVALID);
                goto err_ret;

            case TLVERR_ERROR:
                ALOGE("TLVERR_ERROR: SWrite Ad:%x Val:0x%x\n",dauAddress,numWords);
                mDauMemErrorEventSPtr->set((uint64_t) TER_IE_USB_TLV_ERROR);
                goto err_ret;

            case TLVERR_BADPARAMETER:
                ALOGE("TLVERR_BADPARAMETER: SWrite Ad:%x Val:0x%x\n",dauAddress,numWords);
                mDauMemErrorEventSPtr->set((uint64_t) TER_IE_USB_TLV_BADPARAMETER);
                goto err_ret;
        }

    }

    else if(words > 1)
    {
        ALOGD("BWrite Ad:%x=Val:0x%x\n",dauAddress,numWords);
        memset(&tlv,0,sizeof(UsbTlv));

        tlv.tag = TLV_TAG_BLK_WRITE;
        tlv.length = 2;
        tlv.values[0] = addr;
        tlv.values[1] = words*4;

        rc = libusb_control_transfer(mUsbHandlePtr,
                                     LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
                                     0x0,
                                     0,
                                     0,
                                     (unsigned char *)&tlv,
                                     sizeof(UsbTlv),
                                     UsbDauTimeout::SYNC);
        if (rc < 0) /* Control Transfer return in error in minus */
        {
            if(rc == LIBUSB_ERROR_NO_DEVICE)
            {
                ALOGE("Error during BRead Response control transfer: %s\n",libusb_error_name(rc));
                mDauMemErrorEventSPtr->set((uint64_t) TER_IE_USB_NO_DEVICE);
            }
            else {
                ALOGE("Error during BRead Response control transfer: %s\n",libusb_error_name(rc));
                mDauMemErrorEventSPtr->set((uint64_t) TER_IE_USB_ERROR);
            }
            goto err_ret;
        }

        rc = libusb_bulk_transfer(mUsbHandlePtr,
                                  EP3 | LIBUSB_ENDPOINT_OUT,
                                  (unsigned char *)srcAddress,
                                  words * sizeof(uint32_t),
                                  &actualLen,
                                  UsbDauTimeout::SYNC);
        if(rc < 0)   /* Bulk transfer LIBUSB_SUCCESS = 0 */
        {
            if(rc == LIBUSB_ERROR_NO_DEVICE)
            {
                ALOGE("Error during BRead Response control transfer: %s\n",libusb_error_name(rc));
                mDauMemErrorEventSPtr->set((uint64_t) TER_IE_USB_NO_DEVICE);
            }
            else {
                ALOGE("Error during BRead Response control transfer: %s\n",libusb_error_name(rc));
                mDauMemErrorEventSPtr->set((uint64_t) TER_IE_USB_ERROR);
            }
            goto err_ret;
        }

        memset(&tlv, 0, sizeof(UsbTlv));

        rc = libusb_control_transfer(mUsbHandlePtr,
                                     LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR,
                                     0x0,
                                     0,
                                     0,
                                     (unsigned char *)&tlv,
                                     sizeof(UsbTlv),
                                     UsbDauTimeout::SYNC);

        if (rc < 0) /* Control Transfer return in error in minus */
        {
            if(rc == LIBUSB_ERROR_NO_DEVICE)
            {
                ALOGE("Error during BRead Response control transfer: %s\n",libusb_error_name(rc));
                mDauMemErrorEventSPtr->set((uint64_t) TER_IE_USB_NO_DEVICE);
            }
            else {
                ALOGE("Error during BRead Response control transfer: %s\n",libusb_error_name(rc));
                mDauMemErrorEventSPtr->set((uint64_t) TER_IE_USB_ERROR);
            }
            goto err_ret;
        }

        switch(tlv.values[0])
        {
            case TLV_OK:
                //ALOGI("TLV OK");
                break;

            case TLV_INVALID:
                ALOGE("TLV_INVALID");
                mDauMemErrorEventSPtr->set((uint64_t) TER_IE_USB_TLV_INVALID);
                goto err_ret;

            case TLVERR_ERROR:
                ALOGD("TLVERR_ERROR: BWrite Ad:%x Val:0x%x\n",dauAddress,numWords);
                mDauMemErrorEventSPtr->set((uint64_t) TER_IE_USB_TLV_ERROR);
                goto err_ret;

            case TLVERR_BADPARAMETER:
                ALOGE("TLVERR_BADPARAMETER: BWrite Ad:%x Val:0x%x\n",dauAddress,numWords);
                mDauMemErrorEventSPtr->set((uint64_t) TER_IE_USB_TLV_BADPARAMETER);
                goto err_ret;
        }
    }
    else
    {
        goto err_ret;
    }

    retVal = true;

err_ret:
    mLock.leave();
    return(retVal);
}
