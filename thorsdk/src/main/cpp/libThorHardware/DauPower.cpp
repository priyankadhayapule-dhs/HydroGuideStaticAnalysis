//
// Copyright (C) 2016 EchoNous, Inc.
//
//

#define LOG_TAG "DauPower"

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <DauPower.h>
#include <ThorUtils.h>
#include <ThorError.h>
#include <PciDauMemory.h>
#include <PciDmaBuffer.h>
#include <BitfieldMacros.h>
#include <DauAtuRegisters.h>
#include <DauRegisters.h>

#ifdef CHECK_DISABLE_DAU_POWER
#include <cutils/properties.h>
#endif

#define DAU_POWER_PATH "/sys/devices/dau/dau_pwr/value"
#define DAU_ENABLE_PATH "/sys/devices/dau/pci_link"
#define DAU_MSI_ADDRESS_PATH "/sys/bus/pci/drivers/dau_pci/0000:01:00.0/msi_addr"

//-----------------------------------------------------------------------------
DauPower::DauPower() :
    mDauContextPtr(nullptr)
{
}

//-----------------------------------------------------------------------------
DauPower::DauPower(DauContext* dauContextPtr) :
    mDauContextPtr(dauContextPtr)
{
}

//-----------------------------------------------------------------------------
DauPower::~DauPower()
{
}

//-----------------------------------------------------------------------------
bool DauPower::powerOn()
{
    bool    retVal = false;
    int     localPowerFd = -1;
    int     powerFd = -1;
    int     localEnableFd = -1;
    int     enableFd = -1;

    // Collect the needed file descriptors
    if (nullptr == mDauContextPtr)
    {
        localPowerFd = ::open((const char *)DAU_POWER_PATH, O_RDWR);
        if (-1 == localPowerFd)
        {
            ALOGE("Unable to access dau_pwr: errno = %d", errno);
            goto err_ret;
        }
        else
        {
            powerFd = localPowerFd;
        }

        localEnableFd = ::open((const char *)DAU_ENABLE_PATH, O_RDWR);
        if (-1 == localEnableFd)
        {
            ALOGE("Unable to access pci_link: errno = %d", errno);
            goto err_ret;
        }
        enableFd = localEnableFd;
    }
    else
    {
        powerFd = mDauContextPtr->dauPowerFd;
        if (-1 == powerFd)
        {
            ALOGE("DauContext has invalid power fd");
            goto err_ret;
        }

        enableFd = mDauContextPtr->dauDataLinkFd;
        if (-1 == enableFd)
        {
            ALOGE("DauContext has invalid datalink fd");
            goto err_ret;
        }
    }

    // Power up the Dau
    if (-1 != powerFd)
    {
        if (1 != write(powerFd, "1", 1))
        {
            ALOGE("Unable to write to dau_pwr.  errno = %d", errno);
            goto err_ret;
        }
        lseek(powerFd, 0, SEEK_SET);

        // In theory the FGPA/ASIC should be ready for PCIe communication in 300ms.
        // Will be conservative here.
        usleep(500000);
    }

    // Enable PCIe link with Dau
    if (1 != write(enableFd, "1", 1))
    {
        ALOGE("Failed to initialize PCIe link.  errno = %d", errno);
        goto err_ret;
    }
    lseek(enableFd, 0, SEEK_SET);

    if (!isPowered())
    {
        goto err_ret;
    }

// Will call configureDau() once we have migrated to V3 Dau.
#if 0
    retVal = configureDau();
#else
    retVal = true;
#endif

err_ret:
    if (-1 != localEnableFd)
    {
        ::close(localEnableFd);
    }
    if (-1 != localPowerFd)
    {
        ::close(localPowerFd);
    }

    if (!retVal)
    {
#ifdef CHECK_DISABLE_DAU_POWER
        if (!keepDauOn())
#endif
        {
            powerOff();
        }
    }
    return(retVal);
}

//-----------------------------------------------------------------------------
void DauPower::powerOff()
{
    int     localPowerFd = -1;
    int     powerFd = -1;
    int     localEnableFd = -1;
    int     enableFd = -1;

    // Collect the needed file descriptors
    if (nullptr == mDauContextPtr)
    {
        localPowerFd = ::open((const char *)DAU_POWER_PATH, O_RDWR);
        if (-1 == localPowerFd)
        {
            ALOGE("Unable to access dau_pwr: errno = %d", errno);
            goto err_ret;
        }
        else
        {
            powerFd = localPowerFd;
        }

        localEnableFd = ::open(DAU_ENABLE_PATH, O_RDWR);
        if (-1 == localEnableFd)
        {
            ALOGE("Unable to access pci_link: errno = %d", errno);
            goto err_ret;
        }
        enableFd = localEnableFd;
    }
    else
    {
        powerFd = mDauContextPtr->dauPowerFd;
        enableFd = mDauContextPtr->dauDataLinkFd;
    }

    // Disable PCIe link with Dau
    if (-1 == enableFd)
    {
        ALOGE("DauContext has invalid datalink fd");
    }
    else
    {
        if (1 != write(enableFd, "0", 1))
        {
            ALOGE("Unable to terminate PCIe link.  errno = %d", errno);
        }
        lseek(enableFd, 0, SEEK_SET);
    }

    // Power down the Dau
    if (-1 == powerFd)
    {
        ALOGE("DauContext has invalid power fd");
    }
    else
    {
        if (1 != write(powerFd, "0", 1))
        {
            ALOGE("Unable to write to dau_pwr.  errno = %d", errno);
            goto err_ret;
        }
        lseek(powerFd, 0, SEEK_SET);
    }

err_ret:
    if (-1 != localEnableFd)
    {
        ::close(localEnableFd);
    }
    if (-1 != localPowerFd)
    {
        ::close(localPowerFd);
    }
}

//-----------------------------------------------------------------------------
bool DauPower::isPowered()
{
    return(DauPowerState::On == getPowerState()); 
}

//-----------------------------------------------------------------------------
DauPowerState DauPower::getPowerState()
{
    int             enableFd = -1;
    int             fd = -1;
    DauPowerState   retVal = DauPowerState::Off;
    char            buf;

    if (nullptr == mDauContextPtr)
    {
        enableFd = ::open(DAU_ENABLE_PATH, O_RDWR);
        if (-1 == enableFd)
        {
            ALOGE("Unable to access pci_link: errno = %d", errno);
            goto err_ret;
        }
        fd = enableFd;
    }
    else
    {
        fd = mDauContextPtr->dauDataLinkFd;
        if (-1 == fd)
        {
            ALOGE("DauContext has invalid fd");
            goto err_ret;
        }
    }

    if (read(fd, &buf, 1) <= 0)
    {
        ALOGE("Cannot read pci_link");
        goto err_ret;
    }
    else
    {
        lseek(fd, 0, SEEK_SET);
        switch (buf)
        {
            case '0':
                retVal = DauPowerState::Off;
                break;

            case '1':
                retVal = DauPowerState::On;
                break;

            case '2':
                retVal = DauPowerState::Reboot;
                break;
        }
    }

err_ret:
    if (-1 != enableFd)
    {
        ::close(enableFd);
    }

    return(retVal);
}

//-----------------------------------------------------------------------------
#ifdef CHECK_DISABLE_DAU_POWER
bool DauPower::keepDauOn()
{
    const char*     key = "persist.thor.disable-dau-power-cycle";
    bool            disablePowerCycle = property_get_bool(key, false);

    return(disablePowerCycle);
}
#endif

//-----------------------------------------------------------------------------
bool DauPower::configureDau()
{
    bool            retVal = false;
    PciDauMemory    dauMem;
    uint32_t        val;
    const uint32_t  region1Offset = 0x200;
    PciDmaBuffer    dmaBuffer;
    uint64_t        dmaAddr;
    int             localMsiAddrFd = -1;
    int             msiAddrFd = -1;
    uint64_t        msiAddr;

    dauMem.selectBar(4);

    // Collect needed file descriptors and map to Dau & DMA memory.
    if (nullptr == mDauContextPtr)
    {
        if (!dauMem.map())
        {
            ALOGE("Cannot access Dau memory at BAR4 to configure ATU\n");
            goto err_ret;
        }
        if (!dmaBuffer.open())
        {
            ALOGE("Cannot get Dma Buffer address to configure ATU\n");
            goto err_ret;
        }

        localMsiAddrFd = ::open(DAU_MSI_ADDRESS_PATH, O_RDONLY);
        if (-1 == localMsiAddrFd)
        {
            ALOGE("Unable to open msi_addr\n");
            goto err_ret;
        }
        msiAddrFd = localMsiAddrFd;
    }
    else
    {
        if (!dauMem.map(mDauContextPtr))
        {
            ALOGE("Cannot access Dau memory at BAR4 to configure ATU\n");
            goto err_ret;
        }
        if (!dmaBuffer.open(mDauContextPtr))
        {
            ALOGE("Cannot get Dma Buffer address to configure ATU\n");
            goto err_ret;
        }

        msiAddrFd = mDauContextPtr->dauMsiAddressFd;
    }

    //
    // Get the DMA & MSI Addresses for later use
    //
    dmaAddr = dmaBuffer.getPhysicalAddr(DmaBufferType::Fifo);

    if (::read(msiAddrFd, &msiAddr, sizeof(msiAddr)) != sizeof(msiAddr))
    {
        ALOGE("Could not get correct MSI Address\n");
        goto err_ret;
    }

    //
    // Setup BAR0 to point to the EchoNous logic (Address 0x10800000 inside ASIC).
    //
    val = 0x10800000;
    dauMem.write(&val, ATU_IB_TRGT_ADDR, 1);

    val = 0;
    BFN_SET(val, 1, ATU_IB_CTL2_MODE);
    BFN_SET(val, 1, ATU_IB_CTL2_ENABLE);
    BFN_SET(val, 1, ATU_IB_CTL2_FUZZY);
    dauMem.write(&val, ATU_IB_CTL2_ADDR, 1);

    //
    // Setup BAR2 to point to Faraday logic (Address 0x10000000 inside ASIC).
    //
    val = 0x10000000;
    dauMem.write(&val, ATU_IB_TRGT_ADDR + region1Offset, 1);

    val = 0;
    BFN_SET(val, 1, ATU_IB_CTL2_MODE);
    BFN_SET(val, 1, ATU_IB_CTL2_ENABLE);
    BFN_SET(val, 2, ATU_IB_CTL2_BAR);
    BFN_SET(val, 1, ATU_IB_CTL2_FUZZY);
    dauMem.write(&val, ATU_IB_CTL2_ADDR + region1Offset, 1);

    //
    // Setup Outbound Region 0 for Streaming Engine Data.
    //
    val = 0x20000000;
    dauMem.write(&val, ATU_OB_BASE_ADDR, 1);

    val = 0x3ffe0000;
    dauMem.write(&val, ATU_OB_LIMIT_ADDR, 1);

    val = (uint32_t) dmaAddr;
    dauMem.write(&val, ATU_OB_TRGT_LO_ADDR, 1);

    val = (uint32_t) (dmaAddr >> 32);
    dauMem.write(&val, ATU_OB_TRGT_HI_ADDR, 1);

    val = 0;
    BFN_SET(val, 1, ATU_OB_CTL2_ENABLE);
    dauMem.write(&val, ATU_OB_CTL2_ADDR, 1);

    //
    // Setup Outbound Region 1 for MSI Transactions.
    //
    val = 0x3FFF0000;
    dauMem.write(&val, ATU_OB_BASE_ADDR + region1Offset, 1);

    val = 0x10000;
    dauMem.write(&val, ATU_OB_LIMIT_ADDR + region1Offset, 1);

    val = (uint32_t) msiAddr;
    dauMem.write(&val, ATU_OB_TRGT_LO_ADDR + region1Offset, 1);

    val = (uint32_t) (msiAddr >> 32);
    dauMem.write(&val, ATU_OB_TRGT_HI_ADDR + region1Offset, 1);

    val = 0;
    BFN_SET(val, 1, ATU_OB_CTL2_ENABLE);
    BFN_SET(val, 1, ATU_OB_CTL2_HSE);
    dauMem.write(&val, ATU_OB_CTL2_ADDR + region1Offset, 1);

    //
    // Point MSI transactions to MSI AXI region
    //
    dauMem.unmap();
    dauMem.selectBar(0);

    if (nullptr == mDauContextPtr)
    {
        if (!dauMem.map())
        {
            ALOGE("Cannot access Dau memory at BAR0 to configure ATU\n");
            goto err_ret;
        }
    }
    else
    {
        if (!dauMem.map(mDauContextPtr))
        {
            ALOGE("Cannot access Dau memory at BAR0 to configure ATU\n");
            goto err_ret;
        }
    }

    val = 0x3FFF0000;
    dauMem.write(&val, SYS_MSIADR_ADDR, 1);

    retVal = true;

err_ret:
    dmaBuffer.close();
    dauMem.unmap();

    if (-1 != localMsiAddrFd)
    {
        ::close(localMsiAddrFd);
    }

    return(retVal);
}

