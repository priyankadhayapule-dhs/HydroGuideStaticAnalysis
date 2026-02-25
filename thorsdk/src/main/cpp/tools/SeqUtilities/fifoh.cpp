#include <cstdint>
#include <cstring>
#include <inttypes.h>
#include <time.h>
#include <unistd.h>

#include <DauMemory.h>
#include <PciDmaBuffer.h>
#include <DauInputManager.h>
#include <ThorUtils.h>
#include <DauPower.h>
#include <PciDauMemory.h>
#include <TbfRegs.h>
#include <UpsReader.h>
#include <DauRegisters.h>
#include <BitfieldMacros.h>

typedef struct
{
    uint32_t ctl;
    uint32_t base;
    uint32_t fstride;
    uint32_t lstride;
} SE_Regs;

SE_Regs gSE_Regs[] =
{
    {
        SE_DT0_CTL_ADDR,
        SE_DT0_FADDR_ADDR,
        SE_DT0_FSTR_ADDR,
        SE_DT0_LSTR_ADDR
    },
    {
        SE_DT1_CTL_ADDR,
        SE_DT1_FADDR_ADDR,
        SE_DT1_FSTR_ADDR,
        SE_DT1_LSTR_ADDR
    },
    {
        SE_DT2_CTL_ADDR,
        SE_DT2_FADDR_ADDR,
        SE_DT2_FSTR_ADDR,
        SE_DT2_LSTR_ADDR
    },
    {
        SE_DT3_CTL_ADDR,
        SE_DT3_FADDR_ADDR,
        SE_DT3_FSTR_ADDR,
        SE_DT3_LSTR_ADDR
    },
    {
        SE_DT4_CTL_ADDR,
        SE_DT4_FADDR_ADDR,
        SE_DT4_FSTR_ADDR,
        SE_DT4_LSTR_ADDR
    },
    {
        SE_DT5_CTL_ADDR,
        SE_DT5_FADDR_ADDR,
        SE_DT5_FSTR_ADDR,
        SE_DT5_LSTR_ADDR
    },
    {
        SE_DT6_CTL_ADDR,
        SE_DT6_FADDR_ADDR,
        SE_DT6_FSTR_ADDR,
        SE_DT6_LSTR_ADDR
    },
    {
        SE_DT7_CTL_ADDR,
        SE_DT7_FADDR_ADDR,
        SE_DT7_FSTR_ADDR,
        SE_DT7_LSTR_ADDR
    }
};

int main()
{
    uint32_t* bufPtr;
    uint32_t  frameStride;
    uint32_t  lineStride;
    uint32_t  fifoLength;
    uint64_t  physAddr;
    
    //-------------------------------------------------
    PciDauMemory  dau_mem;
    if (!dau_mem.map())
    {
        printf("couldn't map DAU memory\n");
        return(-1);
    }

    //-------------------------------------------------
    PciDmaBuffer dau_dma;
    if (!dau_dma.open())
    {
        printf("Unable to access DMA buffer\n");
        return(-1);
    }

    bufPtr = dau_dma.getBufferPtr(DmaBufferType::Fifo);
    if ( NULL == bufPtr )
    {
        printf("FIFO buffer pointer is NULL\n");
        return(-1);
    }

    bool dataEnabled = false;
    uint32_t val;

    for (uint32_t i = 0; i < 7; i++)
    {
        dau_mem.read( gSE_Regs[i].ctl, &val, 1 );
        dataEnabled = (val == 2);
        if (dataEnabled)
        {
            printf("\n\n------ SSR settings for data type %d ------------\n", i);
            dau_mem.read( gSE_Regs[i].base, (uint32_t*)(&physAddr), 1);
            dau_mem.read( gSE_Regs[i].fstride, &frameStride, 1);
            dau_mem.read( gSE_Regs[i].lstride, &lineStride,  1);

            fifoLength = 1 << BFN_GET(frameStride, SE_DT0_FSTR_SIZE);
            if ((fifoLength == 0) || (fifoLength > 16))
            {
                printf( "FIFO length (%d) is not in [1, 16]\n", fifoLength);
                return(-1);
            }

            frameStride = (frameStride & 0xfffff) >> 2;
            printf ("frame stride words= %d words\n", frameStride);
            printf ("line strde        = %d\n\n", lineStride);
    
            for (uint32_t i = 0; i < fifoLength; i++)
            {
                printf("[%2d] : 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X \n",
                       i,
                       *(bufPtr + i*frameStride),
                       *(bufPtr + i*frameStride + 1),
                       *(bufPtr + i*frameStride + 2),
                       *(bufPtr + i*frameStride + 3),
                       *(bufPtr + i*frameStride + 4),
                       *(bufPtr + i*frameStride + 5));
            }
        }

    }
    
    return(0);
}
