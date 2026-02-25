#include <cstdint>
#include <cstring>
#include <inttypes.h>
#include <time.h>
#include <unistd.h>

#include <PciDmaBuffer.h>
#include <ThorUtils.h>
#include <DauPower.h>
#include <PciDauMemory.h>
#include <TbfRegs.h>
#include <UpsReader.h>

#include <DauSequenceBuilder.h>
#include <DauLutManager.h>
#include <DauTbfManager.h>
#include <DauSseManager.h>

//-----------------------------------------------------------------------------
void usage()
{
    printf("Usage: runic [imagingCaseId]\n");
    printf("\t runic constructs and places b mode sequence at the base of sequence buffer.\n");
    printf("\t using sequence blobs contained in the UPS file thor.db located \n");
    printf("\t in the folder /data/echonous\n");
}

int main(int argc, char** argv)
{
    uint32_t                    imagingCaseId = 0;
    std::shared_ptr<DauMemory>  daumSPtr = std::make_shared<PciDauMemory>();
    std::shared_ptr<UpsReader>  upsReaderSPtr = std::make_shared<UpsReader>();
    std::shared_ptr<DmaBuffer>  dmaBufferSPtr = std::make_shared<PciDmaBuffer>();
    DauPower                    dau_pwr;
    DauSequenceBuilder          dau_seq(upsReaderSPtr);
    DauSseManager               dau_sse(daumSPtr, dmaBufferSPtr);
    DauLutManager               dau_lut(daumSPtr, upsReaderSPtr);
    DauTbfManager               dau_tbf(daumSPtr, upsReaderSPtr);

    //-------------------------------------------------
    if (!dau_pwr.powerOn())
    {
        printf("Couldn't turn on DAU\n");
        return(-1);
    }

    //-------------------------------------------------
    if (0 == upsReaderSPtr->open())
    {
        printf("couldn't open thor.db\n");
        return(-1);
    }

    //-------------------------------------------------
    if (!daumSPtr->map())
    {
        printf("couldn't map DAU memory\n");
        return(-1);
    }

    //-------------------------------------------------
    if (!dmaBufferSPtr->open())
    {
        printf("Unable to access DMA buffer\n");
        return(-1);
    }

    uint32_t numImagingCases = upsReaderSPtr->getNumImagingCases();
    printf("Opened Ups version is %s\n", upsReaderSPtr->getVersion() );
    printf("Number of imaging cases available in UPS is %d\n", numImagingCases);

    if (argc == 2)
    {
        if (1 != sscanf(argv[1], "%d", &imagingCaseId))
        {
            usage();
            return(-1);
        }
        if (imagingCaseId >= numImagingCases)
        {
            printf( "Specified imaging case (%d) is greater than available cases (%d)\n",
                    imagingCaseId, numImagingCases);
            return(-1);
        }
    }
    

    uint64_t seqPhyAddr = dmaBufferSPtr->getPhysicalAddr(DmaBufferType::Sequence);
    printf( "Physical address of sequence is %" PRIx64 "\n", seqPhyAddr );
    uint32_t* bufPtr = dmaBufferSPtr->getBufferPtr(DmaBufferType::Sequence);
    
    if (!dau_seq.buildBSequence(imagingCaseId, seqPhyAddr, bufPtr))
    {
        printf("Unable to build the sequence\n");
        return(-1);
    }

    dau_sse.init();
    dau_tbf.init(dau_lut);

    dau_tbf.setImagingCase(dau_lut, imagingCaseId);

    dau_sse.start();
    usleep(10000);
    dau_tbf.start();
    
    return(0);
}
