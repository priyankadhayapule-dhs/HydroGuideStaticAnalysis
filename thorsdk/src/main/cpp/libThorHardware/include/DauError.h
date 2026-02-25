//
// Copyright 2017 EchoNous, Inc.
//

#pragma once

#include <stdlib.h>
#include <ThorError.h>

class DauError
{
public: // Methods
    virtual ~DauError() {}

    virtual ThorStatus convertDauError(uint32_t rawErrorCode) = 0;

protected: // Methods
    DauError() {}
};
