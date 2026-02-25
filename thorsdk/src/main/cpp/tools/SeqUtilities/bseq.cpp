#include <cstdint>
#include <cstring>
#include <memory>

#include <PciDmaBuffer.h>
#include <ThorUtils.h>

#include <TbfRegs.h>
#include <UpsReader.h>

#include <DauSequenceBuilder.h>

//-----------------------------------------------------------------------------
void usage()
{
    printf("Usage: bseq [imagingCaseId]\n");
    printf("\t bseq builds and places sequence for the specified imaging case at the base of sequence buffer.\n");
    printf("\t using sequence blobs contained in the UPS file thor.db located \n");
    printf("\t in the folder /data/echonous\n");
}


int main(int argc, char** argv)
{
    uint32_t                    imagingCaseId = 0;
    std::shared_ptr<UpsReader>  upsReaderSPtr = std::make_shared<UpsReader>();

    if (0 == upsReaderSPtr->open("/data/echonous/thor.db"))
    {
        printf("couldn't open thor.db\n");
        return(-1);
    }

    uint32_t numImagingCases = upsReaderSPtr->getNumImagingCases();
    printf("UPS version - %s\n", upsReaderSPtr->getVersion() );
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
    printf("requested imaging case = %d\n", imagingCaseId);
    
    PciDmaBuffer  dmaBuffer;
    if (!dmaBuffer.open())
    {
        printf("Unable to access Dma Buffer\n");
        return(-1);
    }

    uint64_t seqPhyAddr = dmaBuffer.getPhysicalAddr(DmaBufferType::Sequence);
    printf( "Physical address of sequence is 0x%016llX\n", seqPhyAddr );
    uint32_t* bufPtr = dmaBuffer.getBufferPtr(DmaBufferType::Sequence);
    
    DauSequenceBuilder          sb(upsReaderSPtr);

    int imagingMode = upsReaderSPtr->getImagingMode(imagingCaseId);
    printf("imaging mode for imaging case %d is %d\n", imagingCaseId, imagingMode);
    bool buildSucceeded = false;
    if (IMAGING_MODE_B == imagingMode)
    {
        buildSucceeded = sb.buildBSequence(imagingCaseId, seqPhyAddr, bufPtr);
    }
    else
    {
        printf("Building BC sequence!\n");
        buildSucceeded = sb.buildBCSequence(imagingCaseId, seqPhyAddr, bufPtr);
    }
    if ( buildSucceeded )
    {
      printf("sequence has been placed at 0x%016llx", seqPhyAddr);
      printf("\n you can use dmaread utility to examine the contents of sequence buffer\n");
#ifdef DEBUG
        for (int i = 0; i < (23*72); i++)
            printf("%08X\n", bufPtr[i]);
#endif
    }
    else
    {
      printf("bseq failed :( \n");
    }

    return(0);
}
