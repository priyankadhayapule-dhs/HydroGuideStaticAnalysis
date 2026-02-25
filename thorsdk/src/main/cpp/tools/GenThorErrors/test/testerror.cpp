//
// Copyright 2017 EchoNous Inc.
//
//  This is for manual testing of the generated ThorError.h.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ThorError.h>

ThorStatus doSomeThing()
{
    ThorStatus  retVal = THOR_ERROR;

    return(retVal); 
}

int main(int argc, char** argv)
{
    ThorStatus retVal = THOR_OK;

    printf("THOR_OK is %s\n", IS_THOR_ERROR(retVal) ? "bad" : "ok");

    retVal = doSomeThing();

    printf("doSomeThing() returned %s\n", IS_THOR_ERROR(retVal) ? "bad" : "ok");

    retVal = TER_THERMAL_TABLET;
    printf("TER_THERMAL_TABLET is %s\n", IS_THOR_ERROR(retVal) ? "bad" : "ok");

    retVal = TES_TABLET_HW;
    printf("TES_TABLET_HW is %s\n", IS_THOR_ERROR(retVal) ? "bad" : "ok");

    retVal = DEFINE_THOR_ERROR(TES_THERMAL, 2);
    printf("Custom error 0x%X is %s\n", 
           retVal,
           IS_THOR_ERROR(retVal) ? "bad" : "ok");


    return(0); 
}
