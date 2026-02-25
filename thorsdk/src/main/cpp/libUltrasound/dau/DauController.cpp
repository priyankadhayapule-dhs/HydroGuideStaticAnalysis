//
// Copyright 2016 EchoNous Inc.
// This class for reading PCIe probe factory data independent of the Dau class.
//

#define LOG_TAG "DauController"

#include <cstdint>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <DauController.h>
#include <DauPower.h>
#include <PciDauError.h>
#include <PciDauFwUpgrade.h>

//-----------------------------------------------------------------------------
DauController::DauController() {
}

//-----------------------------------------------------------------------------
DauController::~DauController() {
    ALOGD("%s", __func__);

}
//-----------------------------------------------------------------------------
uint32_t DauController::getProbeType(){
    DauPower    dauPower;
    ThorStatus                  retVal = THOR_ERROR;
    if (!dauPower.powerOn())
    {
        if (dauPower.getPowerState() == DauPowerState::Reboot)
        {
            ALOGE("Unable to power the Dau on, a reboot is required.");
            retVal = TER_IE_DAU_POWER_REBOOT;
        }
        else
        {
            ALOGE("Unable to power the Dau on");
            retVal = TER_IE_DAU_POWER;
        }

        goto err_ret;
    }
    usleep(1000*1000);

    readProbeFactData();

    dauPower.powerOff();

    return mProbeInfo.probeType;

    err_ret:
    return  retVal;
}

//-----------------------------------------------------------------------------
void DauController::readProbeFactData() {

    FactoryData fData;

    memset(&mProbeInfo, '\0', sizeof(ProbeInfo));


    PciDauFwUpgrade pciFwUpgrade;

    if (!pciFwUpgrade.init(nullptr)) {
        ALOGE("Unable to intialize PciDauFwUpgrade");
        goto err_ret;
    }

    if (!pciFwUpgrade.readFactData((uint32_t * ) & fData, FACTORY_DATA_START,
                                   sizeof(FactoryData))) {
        ALOGE("Unable to read factory data");
        goto err_ret;
    }


    if (!pciFwUpgrade.deinit())
        ALOGE("Failed to deinit PciDauFwUpgrade");

    mProbeInfo.probeType = fData.probeType;
    if (fData.probeType != -1) {
        mProbeInfo.serialNo = fData.serialNum;
        mProbeInfo.part = fData.partNum;
        ALOGD("probeType : 0x%x\n\r", fData.probeType);
        ALOGD("serialNum : %s\n\r", fData.serialNum);
        ALOGD("partNum : %s\n\r", fData.partNum);
    } else {
        ALOGE("probeType : 0x%x\n\r", fData.probeType);
        ALOGE("serialNum : %s\n\r", fData.serialNum);
        ALOGE("partNum : %s\n\r", fData.partNum);
        mProbeInfo.serialNo = "00000000";
        mProbeInfo.part = "00000000";
    }

    ALOGD("Probe Type : 0x%X\n\r", mProbeInfo.probeType);
    ALOGD("Serial Number : %s\n\r", mProbeInfo.serialNo.c_str());
    ALOGD("part : %s\n\r", mProbeInfo.part.c_str());

    err_ret:;
}
