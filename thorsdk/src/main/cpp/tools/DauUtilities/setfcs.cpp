// Copyright (C) 2016 EchoNous, Inc.
//

#define LOG_TAG "setfcs"

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
//#include <DauRegister.h>

#define FCS_250_RESP_OK     0XFFFF0101 //
#define FCS_208_RESP_OK     0XFFFF0102 //
#define FCS_RESP_ERR    	0XFFFF010E
#define FCS_RESP_INVALID    0XFFFF010F //


/************************************************************/
/*!
* \brief Timer Control Registers
*/
/************************************************************/
#define FTTMR010_TM2_UP	        (1 << 10)
#define FTTMR010_TM2_OFENABLE	(1 << 5)
#define FTTMR010_TM2_CLOCK	    (1 << 4)
#define FTTMR010_TM2_ENABLE	    (1 << 3)
/************************************************************/
/*!
* \brief Timer Control Mask
*/
/************************************************************/
#define FTTMR010_TM2_CRMASK \
	(FTTMR010_TM2_UP | FTTMR010_TM2_OFENABLE \
	| FTTMR010_TM2_CLOCK | FTTMR010_TM2_ENABLE)



enum Type
{
    AX_FCS_250MHz = 1,
    AX_FCS_208MHz,
};

void usage()
{
    printf("********************************************\r\n");
    printf("Usage:\r\n");
    printf("To start FCS environment command  ./setfcs start \r\n");
    printf("Freq Change to 250 MHz command:  ./setfcs 250 \r\n");
    printf("Freq Change to 208 MHz command:  ./setfcs 208 \r\n");
    printf("********************************************\r\n");

}

//-----------------------------------------------------------------------------
int main(int argc, char** argv)
{
    int                 retVal = -1;
    PciDauMemory        dauMem;
    DauPower            dauPower;
    uint32_t            val = 0;
    uint32_t            fcsVal = 0;
    uint32_t            firstTime = 0;

    // Check the mandatory parameters
    if (2 > argc)
    {
        usage();
        exit(-1);
    }

    if(!(strcmp(argv[1],"250")))
    {
        fcsVal =  AX_FCS_250MHz;
        printf(" FCS 250MHz\n");
    }
    else if(!(strcmp(argv[1],"208")))
    {
        fcsVal = AX_FCS_208MHz;
        printf(" FCS 208MHz\n");
    }
    else if(!(strcmp(argv[1],"start")))
    {
        firstTime = 1;
    }
    else
    {
        usage();
        exit(-1);
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

    if(1 == firstTime) {
        printf("Send Boot Application Image Command\n");
        val = 0;
        dauMem.write(&val, PCI_BIST_RESP_ADDR, 1);

        val = TLV_TAG_BOOT_APPLICATION_IMAGE;
        dauMem.write(&val, PCI_BIST_CMD_ADDR, 1);
        printf("Send Boot Application Image Command Done\n");

        sleep(1);

        do {
            val = 0;
            dauMem.read(PCI_BIST_RESP_ADDR, &val, 1);
            switch (val) {
                case PCI_RESP_OK:
                    printf("Successfully Boot Application \n");
                    break;
                case PCI_RESP_ERR:
                    printf("Failed to Boot Application\n");
                    goto err_ret;
            }
            usleep(1000);
        } while ((val != PCI_RESP_OK));
    }
    else
    {
        val = 0;
        dauMem.write(&val, PCI_BIST_RESP_ADDR, 1);

        val = fcsVal;
        val = val << 12;  // masked value with 0xF000

        dauMem.write(&val, PCI_BIST_CMD_ADDR, 1);
        //Enable timer
        dauMem.read(FTTMR_TMCR_ADDR, &val, 1);
        val |= FTTMR010_TM2_CRMASK;
        dauMem.write(&val, FTTMR_TMCR_ADDR, 1);

        printf("Waiting..");
        do {
            val = 0;
            dauMem.read(PCI_BIST_RESP_ADDR, &val, 1);
            printf("..");
            switch (val) {
                case FCS_250_RESP_OK:
                    printf("\nSuccessfully Executed FCS 250\n");
                    break;

                case FCS_208_RESP_OK:
                    printf("\nSuccessfully Executed FCS 208\n");
                    break;

                case FCS_RESP_INVALID:
                    printf("\nInvalid FCS Value\n");
                    goto err_ret;

                case FCS_RESP_ERR:
                    printf("\n\n FCS Switching Error \n\n");
                    goto err_ret;

                case PCI_RESP_ERR:
                    printf("\nInvalid to Execute FCS Change\n");
                    goto err_ret;
            }
            usleep(1000);
        } while ((val != FCS_250_RESP_OK) && (val != FCS_208_RESP_OK));
    }

    retVal = 0;
    err_ret:
    dauMem.unmap();
    return(retVal);
}