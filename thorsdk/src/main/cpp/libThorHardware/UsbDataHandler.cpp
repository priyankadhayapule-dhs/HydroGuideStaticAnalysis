//
// Copyright 2017 EchoNous, Inc.
//
// Creates a dedicated thread for the sole purpose of getting incoming
// image data into a memory buffer (FIFO).
//

#define LOG_TAG "UsbDataHandler"

#include <stdlib.h>
#include <errno.h>
#include <poll.h>
#include <ThorUtils.h>
#include <ThorConstants.h>
#include <UsbDataHandler.h>
#include <libusb.h>
#include <ThorError.h>
#include <DauFwUpgrade.h>
#include <UsbDauError.h>

//#define DEBUG
#define ENABLE_EXTERNAL_ECG 1

#define INTERRUPT_FRAME_SIZE -1

typedef struct {
    uint32_t index          : 24; // frame index
    uint32_t rsrv1          :  8; // reserved

    uint32_t dataType       :  3; // data type
    uint32_t rsrv0          : 29; // reserved

    uint64_t timeStamp;           // time stamp
} FrameHeader;

//-----------------------------------------------------------------------------
UsbDataHandler::UsbDataHandler(DauContext &dauContext,
                               const std::shared_ptr<TriggeredValue> &dataEvent,
                               const std::shared_ptr<TriggeredValue> &hidEvent,
                               const std::shared_ptr<TriggeredValue> &errorEvent,
                               const std::shared_ptr<TriggeredValue> &extEcgEvent) :
        mDauContext(dauContext),
        mDataEventSPtr(dataEvent),
        mHidEventSPtr(hidEvent),
        mErrorEventSPtr(errorEvent),
        mExternalEcgEventSPtr(extEcgEvent),
        mLibusbInitialized(false),
        mIsInitialized(false),
        mIsDaSignal(false),
        mIsEcgSignal(false),
        mProcessingCompleted(0),
        mUsbHandlePtr(nullptr),
        mImageBModeEp(0),
        mImageMModeEp(0),
        mImageColorEp(0),
        mImagePwEp(0),
        mImageCwEp(0),
        mConfigOutEp(0),
        mHidEp(0),
        mTransferPtr(nullptr),
        mTransferPtrMMode(nullptr),
        mTransferPtrColor(nullptr),
        mTransferPtrPw(nullptr),
        mTransferPtrCw(nullptr),
        mBModeOffset(0),
        mCModeOffset(0),
        mMModeOffset(0),
        mPwOffset(0),
        mCwOffset(0),
        mTransfersCompleted(0),
        mTransferCompletedColor(0),
        mTransferCompletedMMode(0),
        mTransferCompletedDA(0),
        mTransferCompletedECG(0),
        mTransferCompletedPw(0),
        mTransferCompletedCw(0) {
}

//-----------------------------------------------------------------------------
UsbDataHandler::~UsbDataHandler() {
    terminate();
}

//-----------------------------------------------------------------------------
bool UsbDataHandler::init(uint8_t *frameBufferPtr, int frameCount) {
    bool retVal = false;
    const struct libusb_version *version;
    int ret;

    ALOGD("%s", __func__);

    mFrameBufferPtr = frameBufferPtr;
    mFrameCount = frameCount;
    mTransfersCompleted = 0;
    mProcessingCompleted = 0;

    version = libusb_get_version();
    ALOGD("Using libusb v%d.%d.%d.%d\n",
          version->major,
          version->minor,
          version->micro,
          version->nano);

    ret = libusb_init(nullptr);
    if (ret < 0) {
        ALOGE("libusb_init failed: %s", libusb_error_name(ret));
        goto err_ret;
    }

    mLibusbInitialized = true;
    mIsUsbError = false;

    if (0 != libusb_wrap_fd(nullptr, mDauContext.usbFd, &mUsbHandlePtr)) {
        ALOGE("Unable to wrap libusb fd");
        goto err_ret;
    }

    ret = libusb_claim_interface(mUsbHandlePtr, 0);

    mAckEvent.init(EventMode::AutoReset, false);
    mStartEvent.init(EventMode::ManualReset, false);
    mExitEvent.init(EventMode::ManualReset, false);

    mTransferPtr = libusb_alloc_transfer(0);
    if (nullptr == mTransferPtr) {
        ALOGE("libusb_alloc_transfer failed for mTransferPtr");
        goto err_ret;
    }

    mTransferPtrColor = libusb_alloc_transfer(0);
    if (nullptr == mTransferPtrColor) {
        ALOGE("libusb_alloc_transfer failed for mTransferPtrColor");
        goto err_ret;
    }

    mTransferPtrMMode = libusb_alloc_transfer(0);
    if (nullptr == mTransferPtrMMode) {
        ALOGE("libusb_alloc_transfer failed for mTransferPtrMMode");
        goto err_ret;
    }

    mTransferPtrDA = libusb_alloc_transfer(0);
    if (nullptr == mTransferPtrDA) {
        ALOGE("libusb_alloc_transfer failed for mTransferPtrDA");
        goto err_ret;
    }

    mTransferPtrEcg = libusb_alloc_transfer(0);
    if (nullptr == mTransferPtrEcg) {
        ALOGE("libusb_alloc_transfer failed for mTransferPtrEcg");
        goto err_ret;
    }

    mInterruptTransferPtr = libusb_alloc_transfer(0);
    if (nullptr == mInterruptTransferPtr) {
        ALOGE("libusb_alloc_transfer failed for mInterruptTransferPtr");
        goto err_ret;
    }

    mTransferPtrPw = libusb_alloc_transfer(0);
    if (nullptr == mTransferPtrPw) {
        ALOGE("libusb_alloc_transfer failed for mTransferPtrPw");
        goto err_ret;
    }

    mTransferPtrCw = libusb_alloc_transfer(0);
    if (nullptr == mTransferPtrCw) {
        ALOGE("libusb_alloc_transfer failed for mTransferPtrCw");
        goto err_ret;
    }

    ret = pthread_create(&mThreadId, NULL, workerThreadEntry, this);
    if (0 != ret) {
        ALOGE("Failed to create worker thread: ret = %d", ret);
        goto err_ret;
    }

    mAckEvent.wait(2000);

    mIsInitialized = true;
    retVal = true;

err_ret:
    return (retVal);
}

//-----------------------------------------------------------------------------
void UsbDataHandler::setDataTypeOffset(uint8_t imagingMode, uint32_t offset) {
    switch (imagingMode) {
        case IMAGING_MODE_B:
            mBModeOffset = offset;
            break;
        case IMAGING_MODE_COLOR:
            mCModeOffset = offset;
            break;
        case IMAGING_MODE_M:
            mMModeOffset = offset;
            break;
        case DAU_DATA_TYPE_DA:
            mDAOffset = offset;
            break;
        case DAU_DATA_TYPE_ECG:
            mEcgOffset = offset;
            break;
        case DAU_DATA_TYPE_PW:
            mPwOffset = offset;
            break;
        case DAU_DATA_TYPE_CW:
            mCwOffset = offset;
            break;
        default:
            ALOGE("Invalid Imaging Mode");
    }
}

//-----------------------------------------------------------------------------
void UsbDataHandler::enableDa(bool daSignal) {
    mIsDaSignal = daSignal;
}

//-----------------------------------------------------------------------------
void UsbDataHandler::enableECG(bool ecgSignal) {
    mIsEcgSignal = ecgSignal;
}

//-----------------------------------------------------------------------------
void UsbDataHandler::enableUS(bool usSignal) {
    mIsUsSignal = usSignal;
}

//-----------------------------------------------------------------------------
bool UsbDataHandler::configureFCSSwitch(FCSType fcsVal) {

    UsbTlv  tlv;
    int rc = -1;

    if((fcsVal != AX_FCS_250MHz) && ((fcsVal != AX_FCS_208MHz)))
    {
        ALOGE("Invalid FCS Value");
        goto err_ret;
    }

    tlv.tag = TLV_TAG_SET_FCS;
    tlv.length = 1;
    tlv.values[0] = fcsVal;

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
        ALOGE("Error during Write control transfer: %s\n",libusb_error_name(rc));
        mErrorEventSPtr->set((uint64_t) TER_IE_USB_ERROR);
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
        ALOGE("Error during Write Response control transfer: %s\n",libusb_error_name(rc));
        mErrorEventSPtr->set((uint64_t) TER_IE_USB_ERROR);
        goto err_ret;
    }

    switch(tlv.values[0])
    {
        case TLV_OK:
            ALOGD("FCS Switch TLV OK");
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

    return true;
err_ret:
    return false;
}

//-----------------------------------------------------------------------------
bool UsbDataHandler::start(uint8_t imagingMode) {
    bool retVal = false;
    int ret;

#ifdef ENABLE_EXTERNAL_ECG
    InterruptStatus intrStatus = {0};
    mIntrEp = EP1 | LIBUSB_ENDPOINT_IN; //0x81
#endif

    ALOGD("%s", __func__);

    if (nullptr == mTransferPtr) {
        goto err_ret;
    }

    if (nullptr == mTransferPtrColor) {
        goto err_ret;
    }

    if (nullptr == mTransferPtrMMode) {
        goto err_ret;
    }

    if (nullptr == mTransferPtrDA) {
        goto err_ret;
    }

    if (nullptr == mTransferPtrEcg) {
        goto err_ret;
    }

    if (nullptr == mInterruptTransferPtr) {
        goto err_ret;
    }

    if (nullptr == mTransferPtrPw) {
        goto err_ret;
    }

    if (nullptr == mTransferPtrCw) {
        goto err_ret;
    }

    mProcessingCompleted = 0;
    mTransfersCompleted = 0;
    mTransferCompletedColor = 0;
    mTransferCompletedMMode = 0;
    mTransferCompletedECG = 0;
    mTransferCompletedDA = 0;
    mTransferCompletedPw = 0;
    mTransferCompletedCw = 0;

    mStartEvent.set();

    mImageBModeEp = LIBUSB_ENDPOINT_IN | EP4;   //0x84;
    mImageColorEp = LIBUSB_ENDPOINT_IN | EP5;   //0x85;
    mImageMModeEp = LIBUSB_ENDPOINT_IN | EP7;   //0x87;
    mEcgEp        = LIBUSB_ENDPOINT_IN | EP10;  //0x8A;
    mDAEp         = LIBUSB_ENDPOINT_IN | EP11;  //0x8B;
    mImagePwEp    = LIBUSB_ENDPOINT_IN | EP6;
    mImageCwEp    = LIBUSB_ENDPOINT_IN | EP8;

#ifdef ENABLE_EXTERNAL_ECG
    libusb_fill_interrupt_transfer(mInterruptTransferPtr,
                                   mUsbHandlePtr,
                                   mIntrEp,
                                   (unsigned char *)&intrStatus,
                                   sizeof(InterruptStatus),
                                   libusbCallbackInterruptEP,
                                   this,
                                   UsbDauTimeout::INFINITE);

    ret = libusb_submit_transfer(mInterruptTransferPtr);
    if (0 != ret) {
        ALOGE("libusb_submit_transfer failed for Interrupt EP: %s",
              libusb_error_name(ret));
        goto err_ret;
    }
#endif

    ALOGI("US:%d DA:%d ECG: %d\n", mIsUsSignal, mIsDaSignal, mIsEcgSignal);
    switch (imagingMode) {
        case IMAGING_MODE_B:

            if(mIsUsSignal) {
                ALOGD("Asked to transfer %d bytes for BMode", mFrameSize);

                libusb_fill_bulk_transfer(mTransferPtr,
                                          mUsbHandlePtr,
                                          mImageBModeEp,
                                          (mFrameBufferPtr + mBModeOffset),
                                          mFrameSize,
                                          libusbCallbackBMode,
                                          this,
                                          UsbDauTimeout::ASYNC);

                ret = libusb_submit_transfer(mTransferPtr);
                if (0 != ret) {
                    ALOGE("libusb_submit_transfer failed for Data EP: %s",
                          libusb_error_name(ret));
                    goto err_ret;
                }
            }

            if(mIsEcgSignal) {
                ALOGD("Asked to transfer %d bytes for ECG", mEcgFrameSize);
                libusb_fill_bulk_transfer(mTransferPtrEcg,
                                          mUsbHandlePtr,
                                          mEcgEp,
                                          (mFrameBufferPtr + mEcgOffset),
                                          mEcgFrameSize,
                                          libusbCallbackEcg,
                                          this,
                                          UsbDauTimeout::ASYNC);

                ret = libusb_submit_transfer(mTransferPtrEcg);
                if (0 != ret) {
                    ALOGE("libusb_submit_transfer failed for Data EP: %s",
                          libusb_error_name(ret));
                    goto err_ret;
                }
            }

            if(mIsDaSignal) {
                ALOGD("Asked to transfer %d bytes for DA", mDAFrameSize);
                libusb_fill_bulk_transfer(mTransferPtrDA,
                                          mUsbHandlePtr,
                                          mDAEp,
                                          (mFrameBufferPtr + mDAOffset),
                                          mDAFrameSize,
                                          libusbCallbackDA,
                                          this,
                                          UsbDauTimeout::ASYNC);

                ret = libusb_submit_transfer(mTransferPtrDA);
                if (0 != ret) {
                    ALOGE("libusb_submit_transfer failed for Data EP: %s",
                          libusb_error_name(ret));
                    goto err_ret;
                }
            }
            break;

        case IMAGING_MODE_COLOR:
            if(mIsUsSignal) {
                ALOGD("Asked to transfer %d bytes for BMode", mFrameSize);

                libusb_fill_bulk_transfer(mTransferPtr,
                                          mUsbHandlePtr,
                                          mImageBModeEp,
                                          (mFrameBufferPtr + mBModeOffset),
                                          mFrameSize,
                                          libusbCallbackBMode,
                                          this,
                                          UsbDauTimeout::ASYNC);

                ret = libusb_submit_transfer(mTransferPtr);
                if (0 != ret) {
                    ALOGE("libusb_submit_transfer failed for Data EP: %s",
                          libusb_error_name(ret));
                    goto err_ret;
                }

                ALOGD("Asked to transfer %d bytes for Color Mode", mColorFrameSize);

                libusb_fill_bulk_transfer(mTransferPtrColor,
                                          mUsbHandlePtr,
                                          mImageColorEp,
                                          (mFrameBufferPtr + mCModeOffset),
                                          mColorFrameSize,
                                          libusbCallbackColor,
                                          this,
                                          UsbDauTimeout::ASYNC);

                ret = libusb_submit_transfer(mTransferPtrColor);
                if (0 != ret) {
                    ALOGE("libusb_submit_transfer failed for Data EP: %s",
                          libusb_error_name(ret));
                    goto err_ret;
                }
            }

            if (mIsEcgSignal) {
                libusb_fill_bulk_transfer(mTransferPtrEcg,
                                          mUsbHandlePtr,
                                          mEcgEp,
                                          (mFrameBufferPtr + mEcgOffset),
                                          mEcgFrameSize,
                                          libusbCallbackEcg,
                                          this,
                                          UsbDauTimeout::ASYNC);

                ret = libusb_submit_transfer(mTransferPtrEcg);
                if (0 != ret) {
                    ALOGE("libusb_submit_transfer failed for Data EP: %s",
                          libusb_error_name(ret));
                    goto err_ret;
                }
            }

            if (mIsDaSignal) {
                libusb_fill_bulk_transfer(mTransferPtrDA,
                                          mUsbHandlePtr,
                                          mDAEp,
                                          (mFrameBufferPtr + mDAOffset),
                                          mDAFrameSize,
                                          libusbCallbackDA,
                                          this,
                                          UsbDauTimeout::ASYNC);

                ret = libusb_submit_transfer(mTransferPtrDA);
                if (0 != ret) {
                    ALOGE("libusb_submit_transfer failed for Data EP: %s",
                          libusb_error_name(ret));
                    goto err_ret;
                }
            }
            break;

        case IMAGING_MODE_M:
            ALOGD("Asked to transfer %d bytes for BMode", mFrameSize);

            libusb_fill_bulk_transfer(mTransferPtr,
                                      mUsbHandlePtr,
                                      mImageBModeEp,
                                      (mFrameBufferPtr + mBModeOffset),
                                      mFrameSize,
                                      libusbCallbackBMode,
                                      this,
                                      UsbDauTimeout::ASYNC);

            ret = libusb_submit_transfer(mTransferPtr);
            if (0 != ret) {
                ALOGE("libusb_submit_transfer failed for Data EP: %s",
                      libusb_error_name(ret));
                goto err_ret;
            }
            ALOGD("Asked to transfer %d bytes for M-Mode", mMFrameSize);

            libusb_fill_bulk_transfer(mTransferPtrMMode,
                                      mUsbHandlePtr,
                                      mImageMModeEp,
                                      (mFrameBufferPtr + mMModeOffset),
                                      mMFrameSize,
                                      libusbCallbackMMode,
                                      this,
                                      UsbDauTimeout::ASYNC);

            ret = libusb_submit_transfer(mTransferPtrMMode);
            if (0 != ret) {
                ALOGE("libusb_submit_transfer failed for Data EP: %s",
                      libusb_error_name(ret));
                goto err_ret;
            }
            break;

        case IMAGING_MODE_PW:
            ALOGD("Asked to transfer %d bytes for PW-Mode", mPwFrameSize);

            libusb_fill_bulk_transfer(mTransferPtrPw,
                                      mUsbHandlePtr,
                                      mImagePwEp,
                                      (mFrameBufferPtr + mPwOffset),
                                      mPwFrameSize,
                                      libusbCallbackPw,
                                      this,
                                      UsbDauTimeout::ASYNC);

            ret = libusb_submit_transfer(mTransferPtrPw);
            if (0 != ret) {
                ALOGE("libusb_submit_transfer failed for Data EP: %s",
                      libusb_error_name(ret));
                goto err_ret;
            }
            break;

        case IMAGING_MODE_CW:
            ALOGD("Asked to transfer %d bytes for CW-Mode", mCwFrameSize);

            libusb_fill_bulk_transfer(mTransferPtrCw,
                                      mUsbHandlePtr,
                                      mImageCwEp,
                                      (mFrameBufferPtr + mCwOffset),
                                      mCwFrameSize,
                                      libusbCallbackCw,
                                      this,
                                      UsbDauTimeout::ASYNC);

            ret = libusb_submit_transfer(mTransferPtrCw);
            if (0 != ret) {
                ALOGE("libusb_submit_transfer failed for Data EP: %s",
                      libusb_error_name(ret));
                goto err_ret;
            }
            break;

        default:
            ALOGE("Invalid Imaging Case");
            goto err_ret;
    }

    retVal = true;

    err_ret:
    return (retVal);
}

//-----------------------------------------------------------------------------
void UsbDataHandler::setFrameSize(int frameSize, uint8_t imagingMode) {
    switch (imagingMode) {
        case IMAGING_MODE_B:
            mFrameSize = frameSize;
            break;
        case IMAGING_MODE_COLOR:
            mColorFrameSize = frameSize;
            break;
        case IMAGING_MODE_M:
            mMFrameSize = frameSize;
            break;
        case DAU_DATA_TYPE_DA:
            mDAFrameSize = frameSize;
            break;
        case DAU_DATA_TYPE_ECG:
            mEcgFrameSize = frameSize;
            break;
        case DAU_DATA_TYPE_PW:
            mPwFrameSize = frameSize;
            break;
        case DAU_DATA_TYPE_CW:
            mCwFrameSize = frameSize;
            break;
        default:
            ALOGE("Invalid Imaging Mode");
    }
}

//-----------------------------------------------------------------------------
void UsbDataHandler::stop() {
    UsbTlv tlv;

    ALOGD("%s", __func__);

    memset(&tlv, 0, sizeof(UsbTlv));

    tlv.tag = TLV_TAG_SE_STOP;
    tlv.length = 0;
    tlv.values[0] = 0;
    tlv.values[1] = 0;

    if (nullptr != mTransferPtr) {
        libusb_control_transfer(mUsbHandlePtr,
                                LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
                                0x0,
                                0,
                                0,
                                (unsigned char *) &tlv,
                                sizeof(UsbTlv),
                                UsbDauTimeout::SYNC);

        memset(&tlv, 0, sizeof(UsbTlv));

        libusb_control_transfer(mUsbHandlePtr,
                                LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR,
                                0x0,
                                0,
                                0,
                                (unsigned char *) &tlv,
                                sizeof(UsbTlv),
                                UsbDauTimeout::SYNC);

        switch (tlv.values[0]) {
            case TLV_OK:
                break;
            case TLV_INVALID:
                ALOGE("TLV_INVALID");
                break;
            case TLVERR_ERROR:
                ALOGE("TLVERR_ERROR");
                break;
            case TLVERR_BADPARAMETER:
                ALOGE("TLVERR_BADPARAMETER");
                break;
        }

        libusb_cancel_transfer(mInterruptTransferPtr);
        libusb_cancel_transfer(mTransferPtr);
        libusb_cancel_transfer(mTransferPtrColor);
        libusb_cancel_transfer(mTransferPtrMMode);
        libusb_cancel_transfer(mTransferPtrDA);
        libusb_cancel_transfer(mTransferPtrEcg);
        libusb_cancel_transfer(mTransferPtrPw);
        libusb_cancel_transfer(mTransferPtrCw);
    }
}

//-----------------------------------------------------------------------------
void UsbDataHandler::terminate() {
    int ret;

    if (!mIsInitialized) {
        return;
    }

    ALOGD("%s", __func__);

    if (nullptr != mInterruptTransferPtr) {
        libusb_cancel_transfer(mInterruptTransferPtr);
    }

    if (nullptr != mTransferPtr) {
        libusb_cancel_transfer(mTransferPtr);
    }

    if (nullptr != mTransferPtrColor) {
        libusb_cancel_transfer(mTransferPtrColor);
    }

    if (nullptr != mTransferPtrMMode) {
        libusb_cancel_transfer(mTransferPtrMMode);
    }

    if (nullptr != mTransferPtrDA) {
        libusb_cancel_transfer(mTransferPtrDA);
    }

    if (nullptr != mTransferPtrEcg) {
        libusb_cancel_transfer(mTransferPtrEcg);
    }

    if (nullptr != mTransferPtrPw) {
        libusb_cancel_transfer(mTransferPtrPw);
    }

    if (nullptr != mTransferPtrCw) {
        libusb_cancel_transfer(mTransferPtrCw);
    }

    mExitEvent.set();
    pthread_join(mThreadId, NULL);

    if (nullptr != mInterruptTransferPtr) {
        libusb_free_transfer(mInterruptTransferPtr);
        mInterruptTransferPtr = nullptr;
    }

    if (nullptr != mTransferPtr) {
        libusb_free_transfer(mTransferPtr);
        mTransferPtr = nullptr;
    }

    if (nullptr != mTransferPtrColor) {
        libusb_free_transfer(mTransferPtrColor);
        mTransferPtrColor = nullptr;
    }

    if (nullptr != mTransferPtrMMode) {
        libusb_free_transfer(mTransferPtrMMode);
        mTransferPtrMMode = nullptr;
    }

    if (nullptr != mTransferPtrDA) {
        libusb_free_transfer(mTransferPtrDA);
        mTransferPtrDA = nullptr;
    }

    if (nullptr != mTransferPtrEcg) {
        libusb_free_transfer(mTransferPtrEcg);
        mTransferPtrEcg = nullptr;
    }

    if (nullptr != mTransferPtrPw) {
        libusb_free_transfer(mTransferPtrPw);
        mTransferPtrPw = nullptr;
    }

    if (nullptr != mTransferPtrCw) {
        libusb_free_transfer(mTransferPtrCw);
        mTransferPtrCw = nullptr;
    }

    if (nullptr != mUsbHandlePtr) {
        libusb_release_interface(mUsbHandlePtr, 0);

        if (!mIsUsbError)
            libusb_close(mUsbHandlePtr);

        mUsbHandlePtr = nullptr;
    }

    if (mLibusbInitialized) {
        libusb_exit(nullptr);
        mLibusbInitialized = false;
    }

    mIsInitialized = false;
    mIsUsbError = false;
}

//-----------------------------------------------------------------------------
void *UsbDataHandler::workerThreadEntry(void *thisPtr) {
    ((UsbDataHandler *) thisPtr)->threadLoop();

    ALOGD("Usb processing thread - leave");
    return (nullptr);
}

//-----------------------------------------------------------------------------
void UsbDataHandler::threadLoop() {
    int ret;
    const int numFd = 2;
    const int exitEvent = 0;
    const int startEvent = 1;
    struct pollfd pollFd[numFd];
    bool keepLooping = true;

    ALOGD("Usb processing thread - enter");

    pollFd[exitEvent].fd = mExitEvent.getFd();
    pollFd[exitEvent].events = POLLIN;
    pollFd[exitEvent].revents = 0;

    pollFd[startEvent].fd = mStartEvent.getFd();
    pollFd[startEvent].events = POLLIN;
    pollFd[startEvent].revents = 0;

    mAckEvent.set();

    while (keepLooping) {
        ret = poll(pollFd, numFd, -1);
        if (-1 == ret) {
            ALOGE("threadLoop: Error occurred during poll(). errno = %d", errno);
            keepLooping = false;
        } else if (pollFd[exitEvent].revents & POLLIN) {
            keepLooping = false;
            mExitEvent.reset();
        } else if (pollFd[startEvent].revents & POLLIN) {
            mStartEvent.reset();
            while (!mProcessingCompleted) {
                ret = libusb_handle_events_completed(nullptr,
                                                     &mProcessingCompleted);
                if (LIBUSB_SUCCESS != ret) {
                    ALOGE("libusb_handle_events_completed returned: %d", ret);
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
void UsbDataHandler::libusbCallbackBMode(struct libusb_transfer *transferPtr) {
    UsbDataHandler *thisPtr = (UsbDataHandler *) transferPtr->user_data;
    static int receivedBytes = 0;

    thisPtr->processCallback(transferPtr,
                             receivedBytes,
                             thisPtr->mBModeOffset,
                             thisPtr->mTransfersCompleted,
                             thisPtr->mFrameSize,
                             thisPtr->mIsUsSignal,
                             "B");
}

//-----------------------------------------------------------------------------
void UsbDataHandler::libusbCallbackColor(struct libusb_transfer* transferPtrColor) {
    UsbDataHandler *thisPtr = (UsbDataHandler *) transferPtrColor->user_data;
    static int receivedBytes = 0;

    thisPtr->processCallback(transferPtrColor,
                             receivedBytes,
                             thisPtr->mCModeOffset,
                             thisPtr->mTransferCompletedColor,
                             thisPtr->mColorFrameSize,
                             false,
                             "C");
}

//-----------------------------------------------------------------------------
void UsbDataHandler::libusbCallbackMMode(struct libusb_transfer* transferPtrMMode) {
    UsbDataHandler *thisPtr = (UsbDataHandler *) transferPtrMMode->user_data;
    static int receivedBytes = 0;

    thisPtr->processCallback(transferPtrMMode,
                             receivedBytes,
                             thisPtr->mMModeOffset,
                             thisPtr->mTransferCompletedMMode,
                             thisPtr->mMFrameSize,
                             false,
                             "M");
}

//-----------------------------------------------------------------------------
void UsbDataHandler::libusbCallbackEcg(struct libusb_transfer *transferPtrECG) {
    UsbDataHandler *thisPtr = (UsbDataHandler *) transferPtrECG->user_data;
    static int receivedBytes = 0;

    bool    fireEvent = false;

    if ((!thisPtr->mIsUsSignal) &&
        (!thisPtr->mIsDaSignal) &&
        (thisPtr->mIsEcgSignal))
    {
        fireEvent = true;
    }

    thisPtr->processCallback(transferPtrECG,
                             receivedBytes,
                             thisPtr->mEcgOffset,
                             thisPtr->mTransferCompletedECG,
                             thisPtr->mEcgFrameSize,
                             fireEvent,
                             "ECG");
}

//-----------------------------------------------------------------------------
void UsbDataHandler::libusbCallbackDA(struct libusb_transfer *transferPtrDA) {
    UsbDataHandler *thisPtr = (UsbDataHandler *) transferPtrDA->user_data;
    static int receivedBytes = 0;

    bool    fireEvent = false;

    if (!thisPtr->mIsUsSignal && thisPtr->mIsDaSignal)
    {
        fireEvent = true;
    }

    thisPtr->processCallback(transferPtrDA,
                             receivedBytes,
                             thisPtr->mDAOffset,
                             thisPtr->mTransferCompletedDA,
                             thisPtr->mDAFrameSize,
                             fireEvent,
                             "DA");
}

//-----------------------------------------------------------------------------
void UsbDataHandler::libusbCallbackInterruptEP(struct libusb_transfer *transferPtrIEP) {
    UsbDataHandler *thisPtr = (UsbDataHandler *) transferPtrIEP->user_data;

    static int      receivedBytes = 0;
    static uint64_t frameCount = 0;

    thisPtr->processCallback(transferPtrIEP,
                             receivedBytes,
                             0,
                             frameCount,
                             INTERRUPT_FRAME_SIZE,
                             false,
                             "INT");
}

//-----------------------------------------------------------------------------
void UsbDataHandler::libusbCallbackPw(struct libusb_transfer *transferPtrPw) {
    UsbDataHandler *thisPtr = (UsbDataHandler *) transferPtrPw->user_data;
    static int receivedBytes = 0;

    thisPtr->processCallback(transferPtrPw,
                             receivedBytes,
                             thisPtr->mPwOffset,
                             thisPtr->mTransferCompletedPw,
                             thisPtr->mPwFrameSize,
                             true,
                             "PW");
}

//-----------------------------------------------------------------------------
void UsbDataHandler::libusbCallbackCw(struct libusb_transfer *transferPtrCw) {
    UsbDataHandler *thisPtr = (UsbDataHandler *) transferPtrCw->user_data;
    static int receivedBytes = 0;

    thisPtr->processCallback(transferPtrCw,
                             receivedBytes,
                             thisPtr->mCwOffset,
                             thisPtr->mTransferCompletedCw,
                             thisPtr->mCwFrameSize,
                             true,
                             "CW");
}

//-----------------------------------------------------------------------------
void UsbDataHandler::

processCallback(struct libusb_transfer* transferPtr,
                int&                    receivedBytes,
                uint32_t                bufferOffset,
                uint64_t&               frameCount,
                int                     frameSize,
                bool                    fireEvent,
                const char* const       modeName)
{
    switch (transferPtr->status) {
        case LIBUSB_TRANSFER_COMPLETED:
            if (LIBUSB_TRANSFER_TYPE_BULK == transferPtr->type) {
#ifdef DEBUG
                uint8_t *inputPtr;
                uint32_t frameIndex, dt;
                uint64_t ts;

                inputPtr = transferPtr->buffer;
                frameIndex = ((FrameHeader *) inputPtr)->index;
                dt = ((FrameHeader *) inputPtr)->dataType;
                ts = ((FrameHeader *) inputPtr)->timeStamp;
                ALOGD("CB-%s  [%d]> %d (T:%u)", modeName, dt, frameIndex, ts);
#endif
                receivedBytes = transferPtr->actual_length;
                if (receivedBytes == frameSize) {
                    if (fireEvent) {
                        mDataEventSPtr->set(frameCount);
                    }
                    frameCount++;
                    receivedBytes = 0;
                    int index = frameCount % mFrameCount;
                    transferPtr->buffer = (mFrameBufferPtr + bufferOffset) +
                                          (index * frameSize);
                }
                libusb_submit_transfer(transferPtr);

            } else if (LIBUSB_TRANSFER_TYPE_INTERRUPT == transferPtr->type) {
                if (INTERRUPT_FRAME_SIZE == frameSize)
                {
                    InterruptStatus intrStatus;

                    memcpy(&intrStatus, transferPtr->buffer, sizeof(InterruptStatus));
                    ALOGD("Interrupt ID:%d Value:0x%x", intrStatus.interruptID, intrStatus.value);

                    if (DAU_INTERNAL_ERROR == intrStatus.interruptID) {
                        mErrorEventSPtr->set((uint64_t) intrStatus.value);
                    }
                    else if (DAU_EXTERNAL_ECG == intrStatus.interruptID) {
                        mExternalEcgEventSPtr->set((uint64_t) intrStatus.value);
                    }
                    else {
                        ALOGD("Invalid Data-->ID:%d Value:0x%x", intrStatus.interruptID,
                              intrStatus.value);
                    }
                }
                else
                {
                    ALOGD("\aLIBUSB_TRANSFER_COMPLETED for Interrupt Transfer\n");

                    uint16_t hidId = *transferPtr->buffer;

                    mHidEventSPtr->set((uint64_t) hidId);
                }

                libusb_submit_transfer(transferPtr);
            }
            break;

        case LIBUSB_TRANSFER_ERROR:
            ALOGE("LIBUSB_TRANSFER_ERROR %s-Mode", modeName);
            mErrorEventSPtr->set((uint64_t) TER_IE_USB_ERROR);
            mProcessingCompleted = 1;
            mIsUsbError = true;
            break;

        case LIBUSB_TRANSFER_TIMED_OUT:
            ALOGE("LIBUSB_TRANSFER_TIMED_OUT %s-Mode", modeName);
            mErrorEventSPtr->set((uint64_t) TER_IE_USB_TIMED_OUT);
            mProcessingCompleted = 1;
            break;

        case LIBUSB_TRANSFER_CANCELLED:
            ALOGD("LIBUSB_TRANSFER_CANCELLED %s-Mode", modeName);
            mProcessingCompleted = 1;
            break;

        case LIBUSB_TRANSFER_STALL:
            ALOGE("LIBUSB_TRANSFER_STALL %s-Mode", modeName);
            mErrorEventSPtr->set((uint64_t) TER_IE_USB_STALL);
            mProcessingCompleted = 1;
            break;

        case LIBUSB_TRANSFER_NO_DEVICE:
            ALOGE("LIBUSB_TRANSFER_NO_DEVICE %s-Mode", modeName);
            mErrorEventSPtr->set((uint64_t) TER_IE_USB_NO_DEVICE);
            mProcessingCompleted = 1;
            mIsUsbError = true;
            break;

        case LIBUSB_TRANSFER_OVERFLOW:
            ALOGE("LIBUSB_TRANSFER_OVERFLOW %s-Mode", modeName);
            mErrorEventSPtr->set((uint64_t) TER_IE_USB_OVERFLOW);
            mProcessingCompleted = 1;
            break;

        default:
            ALOGE("Unknown response %s-Mode", modeName);
            mErrorEventSPtr->set((uint64_t) TER_DAU_UNKNOWN);
            mProcessingCompleted = 1;
            break;
    }
}

//-----------------------------------------------------------------------------
bool UsbDataHandler::mapDtEp(bfDtEnable dtVal)
{
    UsbTlv tlv;
    int    rc = 0;

    memset(&tlv, 0, sizeof(UsbTlv));

    tlv.tag = TLV_TAG_SE_START;
    tlv.length = 1;
    memcpy(&tlv.values[0], &dtVal, sizeof(uint32_t));

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
        ALOGE("Error during Write control transfer Request: %s\n",libusb_error_name(rc));
        mErrorEventSPtr->set((uint64_t) TER_IE_USB_ERROR);
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

    if (rc < 0) /* Control Transfer returns error in minus */
    {
        ALOGE("Error during control transfer Response: %s\n",libusb_error_name(rc));
        mErrorEventSPtr->set((uint64_t) TER_IE_USB_ERROR);
        goto err_ret;
    }

    switch (tlv.values[0])
    {
        case TLV_OK:
//            ALOGD("TLV OK");
            break;
        case TLV_INVALID:
            ALOGE("TLV_INVALID");
            mErrorEventSPtr->set((uint64_t) TER_IE_USB_TLV_INVALID);
            goto err_ret;
        case TLVERR_ERROR:
            ALOGE("TLVERR_ERROR");
            mErrorEventSPtr->set((uint64_t) TER_IE_USB_TLV_ERROR);
            goto err_ret;
        case TLVERR_BADPARAMETER:
            ALOGE("TLVERR_BADPARAMETER");
            mErrorEventSPtr->set((uint64_t) TER_IE_USB_TLV_BADPARAMETER);
            goto err_ret;
    }
    return true;
    err_ret:
    return false;
}
