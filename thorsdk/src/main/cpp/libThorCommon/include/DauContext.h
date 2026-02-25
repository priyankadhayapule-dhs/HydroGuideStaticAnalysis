//
// Copyright 2016 EchoNous, Inc.
//
#pragma once

#include <unistd.h>
#include <string>
#include <PollFd.h>

// Native variant of the Java based DauContext class.  Allows native use
// of the data from the master Java instance.  This is a glorified struct.
class DauContext
{
public:
    DauContext() :
        isUsbContext(false),
        eibIoctlFd(-1),
        ionFd(-1),
        dauPowerFd(-1),
        dauDataLinkFd(-1),
        dauResourceFd(-1),
        dauResource2Fd(-1),
        dauResource4Fd(-1),
        dauMsiAddressFd(-1),
        usbFd(-1),
        enableWriteLogging(false)
    {
    }

    ~DauContext()
    {
        clear();
    }

    inline void closeFd(int& fd)
    {
        if (-1 != fd)
        {
            ::close(fd);
            fd = -1;
        }
    }

    void clear()
    {
        closeFd(eibIoctlFd);
        closeFd(ionFd);
        closeFd(dauPowerFd);
        closeFd(dauDataLinkFd);
        closeFd(dauResourceFd);
        closeFd(dauResource2Fd);
        closeFd(dauResource4Fd);
        closeFd(dauMsiAddressFd);
        closeFd(usbFd);
        dataPollFd.clear();
        errorPollFd.clear();
        usbDauMemPollFd.clear();
        hidPollFd.clear();
        externalEcgPollFd.clear();
        temperaturePollFd.clear();
    }

    // Set to true if using a Usb interface to Dau
    bool    isUsbContext;

    // Full pathname for local storage area for the current application
    std::string appPath;

    // IOCTL interface to the EIB driver
    int     eibIoctlFd;

    // IOCTL interface to standard ION driver
    int     ionFd;

    // Control power to the Dau
    int     dauPowerFd;

    // Used to query and control the data link between the DAU and the HIU
    int     dauDataLinkFd;

    // For memory mapping the Dau
    int     dauResourceFd;
    int     dauResource2Fd;
    int     dauResource4Fd;

    // Get MSI Address for configuring the Dau
    int     dauMsiAddressFd;

    // For accessing Usb interface
    int     usbFd;

    // Used for notification of new ultrasound data.  Examples sources are
    // interrupt notification and timer (for emulation).
    PollFd  dataPollFd;

    // Used for notification of errors in ultrasound data source (i.e. DAU).
    PollFd  errorPollFd;

    // Used for notification of errors in UsbDauMemory class
    PollFd  usbDauMemPollFd;

    // Used for user input from Dau
    PollFd  hidPollFd;

    // Used for external ECG connection notifications
    PollFd  externalEcgPollFd;

    // Used for temperature monitoring
    PollFd  temperaturePollFd;

    // Used for generating data for test bench simulation 
    bool    enableWriteLogging;

private: // Constructors
    DauContext(DauContext& source);
};
