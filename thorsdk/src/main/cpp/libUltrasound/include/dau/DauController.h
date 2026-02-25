//
// Copyright 2016 EchoNous Inc.
//
//

#pragma once

#include <DauError.h>
#include <Dau.h>


class DauController
{
public: // Methods
    DauController();
    ~DauController();

    uint32_t            getProbeType();


private: // Methods
    void                readProbeFactData();

private: // Properties
    ProbeInfo           mProbeInfo;

};
