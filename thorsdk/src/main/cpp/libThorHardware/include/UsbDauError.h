//
// Copyright 2017 EchoNous, Inc.
//

#pragma once

#include <DauError.h>

class UsbDauError : public DauError
{
public: // Methods
    UsbDauError();
    ~UsbDauError();

    ThorStatus convertDauError(uint32_t rawErrorCode);
};
