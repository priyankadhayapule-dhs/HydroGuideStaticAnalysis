//
// Copyright 2020 EchoNous, Inc.
//
#include <stdio.h>
#include <cstdint>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#
#include <DauArrayTest.h>
#include <DauMemory.h>
#include <PciDauMemory.h>
#include <UpsReader.h>

std::shared_ptr<UpsReader> gUpsReaderSPtr;
std::shared_ptr<DauMemory> gDauMemSPtr;
std::shared_ptr<DauHw> gDauHwSPtr;

int main()
{
    gUpsReaderSPtr = std::make_shared<UpsReader>();
    if (!gUpsReaderSPtr->open("thor.db"))
    {
        printf("Unable to open the UPS");
	    return (0);
    }
    
    //-------------------------------------------------
    gDauMemSPtr = std::make_shared<PciDauMemory>();
    if (!gDauMemSPtr->map())
    {
        printf("couldn't map DAU memory\n");
        return(-1);
    }
    
    gDauHwSPtr = std::make_shared<DauHw>(gDauMemSPtr, gDauMemSPtr);

    DauArrayTest dauArrayTest(gDauMemSPtr, gUpsReaderSPtr, gDauHwSPtr, PROBE_TYPE_TORSO3);

    uint32_t numErrors = dauArrayTest.run();
    printf("ThorArrayTest: number of errors = %d\n", numErrors);
    return(0);
}
