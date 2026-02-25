//
// Copyright (C) 2016 EchoNous, Inc.
//

#pragma once

#include <DauContext.h>

enum class DauPowerState
{
    On,
    Off,
    Reboot, // A reboot is necessary to restore link with Dau
};

class DauPower
{
public: // Methods
    DauPower();
    DauPower(DauContext* dauContextPtr);
    ~DauPower();

    bool powerOn();
    void powerOff();

    bool isPowered();
    DauPowerState getPowerState();

private: // Methods
#ifdef CHECK_DISABLE_DAU_POWER
    bool keepDauOn();
#endif

    bool configureDau();

private: // Properties
    DauContext*     mDauContextPtr;
};
