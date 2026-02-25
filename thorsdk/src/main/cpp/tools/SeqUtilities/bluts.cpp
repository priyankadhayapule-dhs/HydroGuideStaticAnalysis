#include <cstdint>
#include <cstring>
#include <memory>

#include <PciDauMemory.h>
#include <DauLutManager.h>

#include <ThorUtils.h>

#include <TbfRegs.h>
#include <UpsReader.h>

//-----------------------------------------------------------------------------
void usage()
{
    printf("Usage: bluts [imagingCaseId]\n");
    printf("\t bseq extracts and loads all LUTs needed for the specified imaging case\n");
}

int main(int argc, char** argv)
{
    uint32_t                    imagingCaseId = 0;
    std::shared_ptr<DauMemory>  daumSPtr = std::make_shared<PciDauMemory>();
    std::shared_ptr<UpsReader>  upsReaderSPtr = std::make_shared<UpsReader>();
    DauLutManager               lutm(daumSPtr, upsReaderSPtr);

    if (!daumSPtr->map())
    {
        printf("DauMemory::map() failed\n");
        return(-1);
    }
    
    if (0 == upsReaderSPtr->open())
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

    lutm.wakeupLuts();

    lutm.loadGlobalLuts();
    lutm.loadImagingCaseLuts(imagingCaseId);

    lutm.sleepLuts();
    return(0);
}
