//
// Copyright (C) 2019 EchoNous, Inc.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>

#include <DauPower.h>
#include <DauHw.h>
#include <PciDauMemory.h>
#include <PciDmaBuffer.h>
#include <BitfieldMacros.h>
#include <DauRegisters.h>


//-----------------------------------------------------------------------------
int main(int argc, char** argv)
{
    int                         retVal = -1;
    DauPower                    dauPower;
    std::shared_ptr<DauMemory>  mDaumSPtr;
    std::shared_ptr<DauHw>      mDauHwSPtr;
    float                       temp;
    ThorStatus                  thorStatus;

    if (!dauPower.isPowered())
    {
        printf("The Dau is off\n");
        goto err_ret;
    }

    mDaumSPtr = std::make_shared<PciDauMemory>();
    mDauHwSPtr = std::make_shared<DauHw>(mDaumSPtr, mDaumSPtr);

    if (!mDaumSPtr->map())
    {
        printf("Failed to access the Dau\n");
        goto err_ret;
    }

    if (argc > 1)
    {
        printf("Initializing temp sensor\n");

        thorStatus = mDauHwSPtr->initTmpSensor(DauHw::I2C_ADDR_TMP0);

        if (IS_THOR_ERROR(thorStatus))
        {
            printf("Unable to initialize temp sensor!\n");
            goto err_ret;
        }
    }

    thorStatus = mDauHwSPtr->readTmpSensor(DauHw::I2C_ADDR_TMP0, temp);

    if (IS_THOR_ERROR(thorStatus))
    {
        printf("Failed to get probe temperature");
    }
    else
    {
        printf("%f\n", temp);
    }

    retVal = 0;

err_ret:

    return(retVal);
}
