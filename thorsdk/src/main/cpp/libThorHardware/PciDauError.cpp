//
// Copyright 2017 EchoNous, Inc.
//

#include <PciDauError.h>
#include <BitfieldMacros.h>
#include <DauRegisters.h>

PciDauError::PciDauError()
{
}

PciDauError::~PciDauError()
{
}

ThorStatus PciDauError::convertDauError(uint32_t rawErrorCode)
{
    // This maps SYS_INT1_SERR_BIT from Interrupt Register 1 to unused bit in
    // Interrupt Register 0.
    const uint32_t  MAP_SYS_INT1_SERR_BIT = SYS_INT0_BERR_BIT + SYS_INT0_BERR_LEN;
    const uint32_t  MAP_SYS_INT1_SERR_LEN = SYS_INT1_SERR_LEN;

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
    else if (BFN_GET(rawErrorCode, SYS_INT0_MSI_ERR))
    {
        return(TER_DAU_MSI); 
    }
    else if (BFN_GET(rawErrorCode, SYS_INT0_TXC))
    {
        return(TER_DAU_TXC); 
    }
    else if (BFN_GET(rawErrorCode, SYS_INT0_BERR) ||
             BFN_GET(rawErrorCode, MAP_SYS_INT1_SERR))
    {
        return(TER_DAU_MEM_ACCESS); 
    } 
    else if (BFN_GET(rawErrorCode, SYS_INT0_PCIE_ERR0) ||
             BFN_GET(rawErrorCode, SYS_INT0_PCIE_ERR1) ||
             BFN_GET(rawErrorCode, SYS_INT0_PCIE_ERR2) ||
             BFN_GET(rawErrorCode, SYS_INT0_PCIE_ERR3) ||
             BFN_GET(rawErrorCode, SYS_INT0_PCIE_ERR4) ||
             BFN_GET(rawErrorCode, SYS_INT0_PCIE_ERR5) ||
             BFN_GET(rawErrorCode, SYS_INT0_PCIE_ERR6))
    {
        return(TER_DAU_PCIE); 
    }
    else
    {
        return(TER_DAU_UNKNOWN); 
    }
}

