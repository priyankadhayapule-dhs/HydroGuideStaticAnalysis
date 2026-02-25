//
// Copyright 2017 EchoNous, Inc.
//

#pragma once

#include <pthread.h>
#include <memory>
#include <DauContext.h>
#include <Event.h>
#include <TriggeredValue.h>
#include <libusb/libusb.h>
#include <ThorConstants.h>

enum DauInterruptStatus
{
    DAU_INTERNAL_ERROR=1, // If Error bit is set in Dau this bit will be set.
    DAU_EXTERNAL_ECG      // If External ECG is connected with Dau this Interrupt status will be returned.
};

typedef struct
{
    uint32_t interruptID; // Interrupt status event (Error OR ExternalEcg)
    uint32_t value;       // Either Register value OR External Ecg connection status
} InterruptStatus;

/* Structure to associate Endpoint with Data type
 *
 * Every 4-bit contains value of EP
 * EP value is in between (1~0xF)
*/
typedef struct
{
    uint8_t dt0:4;
    uint8_t dt1:4;
    uint8_t dt2:4;
    uint8_t dt3:4;
    uint8_t dt4:4;
    uint8_t dt5:4;
    uint8_t dt6:4;
    uint8_t dt7:4;
} bfDtEnable;

namespace UsbDauTimeout
{
    // Timeout values in milliseconds for libusb transfers
    enum
    {
        INFINITE = 0,
        SYNC = 100,
        ASYNC = 2000,
        FIRMWARE = 10000
    };
}

struct libusb_device_handle;
struct libusb_transfer;

class UsbDataHandler
{
public: // Methods
    UsbDataHandler(DauContext& dauContext,
                   const std::shared_ptr<TriggeredValue>& dataEvent,
                   const std::shared_ptr<TriggeredValue>& hidEvent,
                   const std::shared_ptr<TriggeredValue>& errorEvent,
                   const std::shared_ptr<TriggeredValue>& extEcgEvent);

    ~UsbDataHandler();

    bool init(uint8_t* frameBufferPtr, int frameCount);
    bool start(uint8_t imagingMode);
    void stop();
    void terminate();
    bool mapDtEp(bfDtEnable mapVal);
    void enableECG(bool ecgSignal);
    void enableDa(bool daSignal);
    void enableUS(bool daSignal);
    bool configureFCSSwitch(FCSType fcsVal);

    // This is called everytime when the imaging mode changes (image case ID).
    // Needs revisited for handling modes such as C-Mode, M-Mode, etc.
    void setFrameSize(int frameSize,uint8_t imagingMode);
    void setDataTypeOffset(uint8_t imagingMode,uint32_t imagingModeOffset);

private:
// Methods
    UsbDataHandler();

    static void*    workerThreadEntry(void* thisPtr);
    static void     libusbCallbackBMode(struct libusb_transfer* transferPtr);
    static void     libusbCallbackColor(struct libusb_transfer* transferPtrColor);
    static void     libusbCallbackMMode(struct libusb_transfer* transferPtrMMode);
    static void     libusbCallbackEcg(struct libusb_transfer* transferPtrECG);
    static void     libusbCallbackDA(struct libusb_transfer* transferPtrDA);
    static void     libusbCallbackInterruptEP(struct libusb_transfer *transferPtrIEP);
    static void     libusbCallbackPw(struct libusb_transfer* transferPtrPw);
    static void     libusbCallbackCw(struct libusb_transfer* transferPtrCw);
    void            threadLoop();

    void            processCallback(struct libusb_transfer* transferPtr,
                                    int&                    receivedBytes,
                                    uint32_t                bufferOffset,
                                    uint64_t&               frameCount,
                                    int                     frameSize,
                                    bool                    fireEvent,
                                    const char* const       modeName);

private:
    // Properties
    DauContext&                     mDauContext;
    std::shared_ptr<TriggeredValue> mDataEventSPtr;
    std::shared_ptr<TriggeredValue> mHidEventSPtr;
    std::shared_ptr<TriggeredValue> mErrorEventSPtr;
    std::shared_ptr<TriggeredValue> mExternalEcgEventSPtr;

    Event                           mAckEvent;
    Event                           mStartEvent;
    Event                           mExitEvent;
    bool                            mLibusbInitialized;
    bool                            mIsInitialized;
    bool                            mIsEcgSignal;
    bool                            mIsDaSignal;
    bool                            mIsUsSignal;
    bool                            mIsUsbError;
    pthread_t                       mThreadId;

    // Transfer Data
    uint64_t                        mTransfersCompleted;
    uint64_t                        mTransferCompletedColor;
    uint64_t                        mTransferCompletedMMode;
    uint64_t                        mTransferCompletedECG;
    uint64_t                        mTransferCompletedDA;
    uint64_t                        mTransferCompletedPw;
    uint64_t                        mTransferCompletedCw;
    uint32_t                        mBModeOffset;
    uint32_t                        mCModeOffset;
    uint32_t                        mMModeOffset;
    uint32_t                        mDAOffset;
    uint32_t                        mEcgOffset;
    uint32_t                        mPwOffset;
    uint32_t                        mCwOffset;
    uint8_t*                        mFrameBufferPtr;

    int                             mFrameSize;
    int                             mColorFrameSize;
    int                             mMFrameSize;
    int                             mDAFrameSize;
    int                             mEcgFrameSize;
    int                             mPwFrameSize;
    int                             mCwFrameSize;
    int                             mFrameCount;

    int                             mProcessingCompleted;

    libusb_device_handle*           mUsbHandlePtr;

    // Usb Endpoints
    unsigned char                   mIntrEp;
    unsigned char                   mImageBModeEp;  // Input Image Data
    unsigned char                   mImageColorEp;  // Color Data
    unsigned char                   mImageMModeEp;  // MMode Data
    unsigned char                   mDAEp;          // DA Data
    unsigned char                   mEcgEp;         // Ecg Data
    unsigned char                   mImagePwEp;     // PW Data
    unsigned char                   mImageCwEp;     // CW Data
    unsigned char                   mConfigOutEp;   // Output Config Data (Dau mem)
    unsigned char                   mHidEp;         // Hid input (buttons)

    struct libusb_transfer*         mInterruptTransferPtr;
    struct libusb_transfer*         mTransferPtr;
    struct libusb_transfer*         mTransferPtrColor;
    struct libusb_transfer*         mTransferPtrMMode;
    struct libusb_transfer*         mTransferPtrDA;
    struct libusb_transfer*         mTransferPtrEcg;
    struct libusb_transfer*         mTransferPtrPw;
    struct libusb_transfer*         mTransferPtrCw;
};
