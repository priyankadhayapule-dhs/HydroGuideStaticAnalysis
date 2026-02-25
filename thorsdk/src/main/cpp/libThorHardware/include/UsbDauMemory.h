//
// Copyright (C) 2017 EchoNous, Inc.
//

#pragma once

#include <DauMemory.h>
#include <UsbDataHandler.h>


class UsbDauMemory : public DauMemory {
public: // Methods

    UsbDauMemory();

    ~UsbDauMemory();

    bool map();

    bool map(DauContext* dauContextPtr);

    void unmap();

    size_t getMapSize();

    bool read(uint32_t dauAddress, uint32_t* destAddress, uint32_t numWords);

    bool write(uint32_t* srcAddress, uint32_t dauAddress, uint32_t numWords);

    bool initLibusb();

    libusb_device_handle* getDeviceHandle()
    {
        return mUsbHandlePtr;
    }

    void initializeErrorEvent();
    bool isInterfaceClaimed();

private:
    DauContext*                         mDauContextPtr;
    bool                                mUsbInitialized;
    bool                                mIsInterfaceClaimed;
    CriticalSection                     mLock;
    libusb_device_handle*               mUsbHandlePtr;
    std::shared_ptr<TriggeredValue>     mDauMemErrorEventSPtr;
};
