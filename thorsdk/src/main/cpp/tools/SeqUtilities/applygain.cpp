#include <cstdint>
#include <cstring>
#include <stdio.h>
#include <memory>

#include <PciDauMemory.h>
#include <DauGainManager.h>
#include <DauLutManager.h>
#include <ThorUtils.h>
#include <UpsReader.h>

int main(int argc, char** argv)
{
    uint32_t                    imagingCaseId = 0;
    std::shared_ptr<DauMemory>  daumSPtr;
    std::shared_ptr<UpsReader>  upsReaderSPtr;

    daumSPtr = std::make_shared<PciDauMemory>();
    upsReaderSPtr = std::make_shared<UpsReader>();

    if (!daumSPtr->map())
    {
        printf("DauMemory::map() failed\n");
        return(-1);
    }

    if (0 == upsReaderSPtr->open())
    {
        printf("couldn't open thor.db\n");
        daumSPtr->unmap();
        return(-1);
    }

    // TODO: error check
    float nearGain, farGain;
    if (argc != 3)
    {
        printf("Usage: applygain neargain fargain\n");
        printf("\t gain values must be in [-15.0 15.0]\n");
        daumSPtr->unmap();
        upsReaderSPtr->close();
        return(-1);
    }
    sscanf(argv[1], "%f", &nearGain);
    sscanf(argv[2], "%f", &farGain);

    DauLutManager lutm(daumSPtr, upsReaderSPtr);
    DauGainManager gainm(daumSPtr, upsReaderSPtr);

    if ( (nearGain >  15.0)  ||
         (nearGain < -15.0)  ||
         (farGain  >  15.0)  ||
         (farGain  < -15.0) )
    {
       printf("ERROR: specified gains are out of range\n");
    }
    else
    {
        printf("applying gain %f %f\n", nearGain, farGain);
        if (!gainm.applyBModeGain( lutm, nearGain, farGain))
        {
            printf("Failed to apply gain\n");
        }
    }
    return(0);
}
