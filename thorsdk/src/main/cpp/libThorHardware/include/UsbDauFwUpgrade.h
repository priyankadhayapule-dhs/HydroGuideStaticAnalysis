//
// Copyright (C) 2017 EchoNous, Inc.
//

#pragma once

#include <libusb/libusb.h>
#include <DauFwUpgrade.h>
#include <UsbDauMemory.h>

class UsbDauFwUpgrade : public DauFwUpgrade {
public: // Methods

    UsbDauFwUpgrade();
    ~UsbDauFwUpgrade();

    bool init(DauContext* dauContextPtr);
    bool getFwInfo(FwInfo* fwInfo);
    bool uploadImage(uint32_t* srcAddress, uint32_t dauAddress, uint32_t imgLen);
    bool rebootDevice();
    bool readFactData(uint32_t* rData,uint32_t dauAddress, uint32_t factDataLen);
    bool writeFactData(uint32_t* srcAddress, uint32_t dauAddress, uint32_t factDataLen);
    uint8_t getDeviceClass();
    void    deInit();
    uint8_t getDeviceDescClass();

    libusb_device_handle* getDeviceHandle()
    {
        return mUsbHandlePtr;
    }


    uint32_t computeCRC(uint8_t* buf, uint32_t len);

    static const char* const DEFAULT_BIN_FILE;
    static const char* const DEFAULT_BL_BIN_FILE;

private:
    DauContext*                 mDauContextPtr;
    bool                        mUsbInitialized;
    bool                        mCommonFwFlag;
    CriticalSection             mLock;
    libusb_device_handle*       mUsbHandlePtr;
    uint32_t                    mBISTLogBufLength;
};
