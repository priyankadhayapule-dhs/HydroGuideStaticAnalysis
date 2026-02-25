//
// Copyright (C) 2016 EchoNous, Inc.
//

#define LOG_TAG "pcibist"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <string>

#include <PciDauMemory.h>
#include <DauPower.h>
#include <ThorUtils.h>
#include <DauFwUpgrade.h>

//-----------------------------------------------------------------------------
int main(int argc, char** argv)
{
    int                 retVal = -1;
    PciDauMemory        dauMem;
    DauPower            dauPower;
    uint32_t            val = 0;
    char                *readBuffer;
    uint32_t            logLength = 0;
    uint32_t            logLengthWords = 0;

    if(!dauPower.powerOn())
    {
        printf("Unable to power on Dau\n");
        goto err_ret;
    }

    if (!dauPower.isPowered())
    {
        printf("The DAU is off\n");
        goto err_ret;
    }

    dauMem.selectBar(2);
    dauMem.useInternalAddress(true);

    if (!dauMem.map())
    {
        printf("Failed to access Dau\n");
        goto err_ret;
    }

    printf("Send Boot Application Image Command\n");
    val = TLV_TAG_BOOT_APPLICATION_IMAGE;
    dauMem.write(&val,PCI_BIST_CMD_ADDR,1);
    printf("Send Boot Application Image Command Done\n");

    dauMem.unmap();

    sleep(4);

    dauMem.selectBar(2);
    dauMem.useInternalAddress(true);

    if (!dauMem.map())
    {
        printf("Failed to access Dau\n");
        goto err_ret;
    }

    do
    {
        val = 0;
        dauMem.read(PCI_BIST_RESP_ADDR,&val,1);
        switch(val)
        {
            case PCI_RESP_OK:
                printf("Successfully Boot Application \n");
                break;
            case PCI_RESP_ERR:
                printf("Failed to Boot Application\n");
                goto err_ret;
        }
        sleep(1);
    } while((val != PCI_RESP_OK));

    val = 0;

    printf("********************************************\r\n");
    printf("\t\tPCIe BIST Test Menu\r\n");
    printf("********************************************\r\n");
    printf("%d  :  Test All\r\n",TEST_BIST);
    printf("%d  :  Test Registers\r\n",TEST_REG_TEST);
    printf("%d  :  Test Memory\r\n",TEST_MEM_TEST);
    printf("%d  :  Test Seq LLE\r\n",TEST_SEQ_LLE);
    printf("%d  :  Test Timer\r\n",TEST_FTTMR);
    printf("%d  :  Test WDT\r\n",TEST_FTWDT);
    printf("%d  :  Test SPIF\r\n",TEST_SPIF);
    printf("%d  :  Test I2C\r\n",TETS_I2C);
    printf("%d  :  Test SSPI\r\n",TEST_SSPI);
    printf("%d  :  Test SSI2C\r\n",TEST_SSI2C);
    printf("%d :  Test SSSPI\r\n",TEST_SSSPI);
    printf("%d :  Test TXSPI\r\n",TEST_TXSPI);
    printf("%d :  Test HVSUP\r\n",TEST_HVSUP);
    printf("%d :  Test FTDMA\r\n",TEST_FTDMA);
    //TODO:Currently this test is disabled
//    printf("%d :  Test INT\r\n",TEST_INT);
    printf("%d :  Test AFE\r\n",TEST_AFE);
    printf("%d :  Test FCS\r\n",TEST_FCS);
    printf("%d :  Test ASPM\r\n",TEST_ASPM);
    printf("%d :  Test PDMA\r\n",TEST_PDMA);
    printf("********************************************\r\n");
    scanf("%d",&val);
    printf("Entered Val : 0x%x \n",val);
    val |= PCIE_BIST_TEST_MASK;
    printf("Masked Val : 0x%x \n",val);

    dauMem.write(&val,PCI_BIST_CMD_ADDR,1);

    do
    {
        val = 0;
        dauMem.read(PCI_BIST_RESP_ADDR,&val,1);
        switch(val)
        {
            case PCI_RESP_OK:
                printf("Successfully Executed BIST Code \n");
                break;
            case PCI_RESP_ERR:
                printf("Failed to Execute BIST Code\n");
                goto err_ret;
        }
        sleep(1);
    } while((val != PCI_RESP_OK));


    do
    {
        sleep(1);
        dauMem.read(PCI_BIST_LOG_LEN_ADDR,&logLength,1);
    } while(logLength == 0);

    printf("BIST Log Length:%d\n",logLength);

    if(logLength > 0)
    {
        readBuffer = (char *) malloc(logLength);

        if((logLength % 4) == 0 )
        {
            logLengthWords = logLength / 4;
        }
        else
        {
            logLengthWords = logLength / 4;
            logLengthWords += 1;
        }

        dauMem.read(PCI_BIST_LOG_ADDR, (uint32_t *) readBuffer, logLengthWords);

        for (int i = 0; i < logLength; i++) {
            printf("%c", readBuffer[i]);
        }

        free(readBuffer);
    }
    else
    {
        printf("Invalid Log Length:%d\n",logLength);
        goto err_ret;
    }

    dauMem.unmap();
    dauPower.powerOff();

    retVal = 0;

err_ret:
    return(retVal);
}
