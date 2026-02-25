//
// Copyright 2017 EchoNous, Inc.
//

#pragma once

#include <DauError.h>

class PciDauError : public DauError
{
public: // Methods
    PciDauError();
    ~PciDauError();

    ThorStatus convertDauError(uint32_t rawErrorCode);
};

