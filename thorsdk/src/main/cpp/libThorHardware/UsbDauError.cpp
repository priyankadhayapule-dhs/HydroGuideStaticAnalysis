//
// Copyright 2017 EchoNous, Inc.
//

#include <UsbDauError.h>
#include <BitfieldMacros.h>
#include <DauRegisters.h>

UsbDauError::UsbDauError()
{
}

UsbDauError::~UsbDauError()
{
}

ThorStatus UsbDauError::convertDauError(uint32_t rawErrorCode)
{
    // This maps SYS_INT1_SERR_BIT from Interrupt Register 1 to unused bit in
    // Interrupt Register 0.
    const uint32_t  MAP_SYS_INT1_SERR_BIT = SYS_INT0_TXC_BIT + 1;
    const uint32_t  MAP_SYS_INT1_SERR_LEN = 1;

    // This conversion assumes that multiple sources will not be set.
    // If they are, then only a single one will be mapped based on priority.
    if (BFN_GET(rawErrorCode, SYS_INT0_PMON))
    {
        return(TER_DAU_PWR);
    }
    else if (BFN_GET(rawErrorCode, SYS_INT0_SEQ_ERR))
    {
        return(TER_DAU_SEQUENCE);
    }
    else if (BFN_GET(rawErrorCode, SYS_INT0_SE_ERR))
    {
        return(TER_DAU_STREAMING); 
    }
    else if (BFN_GET(rawErrorCode, SYS_INT0_TXC))
    {
        return(TER_DAU_TXC); 
    }
    else if (BFN_GET(rawErrorCode, MAP_SYS_INT1_SERR))
    {
        return(TER_DAU_MEM_ACCESS); 
    } 
    else
    {
        return(TER_DAU_UNKNOWN); 
    }
}

