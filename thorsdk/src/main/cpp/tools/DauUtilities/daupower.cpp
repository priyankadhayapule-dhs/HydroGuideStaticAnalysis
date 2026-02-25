//
// Copyright (C) 2016 EchoNous, Inc.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>

#include <DauPower.h>
#include <PciDauMemory.h>
#include <PciDmaBuffer.h>
#include <BitfieldMacros.h>
#include <DauAtuRegisters.h>
#include <DauRegisters.h>

const char* gParamOn = "on";
const char* gParamOff = "off";
const char* gParamQuery = "query";

//-----------------------------------------------------------------------------
void usage()
{
    printf("Usage: daupower <command>\n");
    printf("\n  command can be one of the following:\n");
    printf("    query\tDisplay current Dau power state\n");
    printf("    on\t\tTurn the Dau on\n");
    printf("    off\t\tTurn the Dau off\n");
}

//-----------------------------------------------------------------------------
int main(int argc, char** argv)
{
    int         retVal = -1;
    DauPower    dauPower;

    if (2 != argc)
    {
        usage();
        goto err_ret;
    }

    if (0 == strncasecmp(gParamOn, argv[1], strlen(gParamOn)))
    {
        if (!dauPower.powerOn())
        {
            printf("Failed to power on and configure the Dau\n");
            goto err_ret;
        }
        else
        {
            printf("Dau is on\n");
        }
    }
    else if (0 == strncasecmp(gParamOff, argv[1], strlen(gParamOff)))
    {
        dauPower.powerOff();
        printf("Dau is off\n");
    }
    else if (0 == strncasecmp(gParamQuery, argv[1], strlen(gParamQuery)))
    {
        switch (dauPower.getPowerState())
        {
            case DauPowerState::On:
                printf("Dau is on\n");
                break;

            case DauPowerState::Off:
                printf("Dau is off\n");
                break;

            case DauPowerState::Reboot:
                printf("A reboot is needed to restore link to Dau\n");
                break;
        }
    }
    else
    {
        usage();
        goto err_ret;
    }

    retVal = 0;

err_ret:

    return(retVal);
}
