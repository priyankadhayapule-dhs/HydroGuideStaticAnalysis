//
// Copyright 2018 EchoNous, Inc.
//
#include <stdio.h>
#include <cstdint>
#include <algorithm>
#include <memory>

#include <TbfRegs.h>
#include <UpsReader.h>
#include <DopplerAcq.h>
#include <DauMemory.h>
#include <DauSequenceBuilder.h>
#include <ScanDescriptor.h>
#include <fstream>
#include <iostream>
#include <json/json.h>

static char     doppleracqtestversion[]   = "1.0";
static char     defaultUpsFileName[]    = "thor.db";
static char     defaultOutputFileName[] = "doppleracqout.json";
static float    defaultGateStartMm      = 40.0f;
static uint32_t defaultGateSizeIndex    = 0;
static float    defaultBeamAngleRad     = 0.0f;
static uint32_t defaultPrfIndex         = 0;
static uint32_t defaultImagingCaseId    = 86;

char*                      gUpsFileName;
char*                      gOutputFileName;
DopplerAcq*                gDopplerAcqPtr;
std::shared_ptr<UpsReader> gUpsReaderSPtr;
DauSequenceBuilder*        gSequenceBuilderPtr;
uint32_t                   gImagingCaseId;
uint32_t                   gRequestedPrfIndex;
uint32_t                   gActualPrfIndex;
float                      gGateStartMm;
uint32_t                   gGateSizeIndex;
float                      gBeamAngleRad;
uint32_t                   gRegisterIndex;
uint32_t                   gPwSequenceBlob[62];

bool gBeamAngleIsSpecified = false;
bool gRegisterIndexIsSpecified = false;

std::string removeExtension(const std::string& filename)
{
    size_t lastdot = filename.find_last_of(".");
    if (lastdot == std::string::npos)
        return (filename);
    return (filename.substr(0, lastdot));
}

char* getCmdOption(char ** begin, char ** end, const std::string & option)
{
    char ** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return 0;
}

bool cmdOptionExists(char** begin, char** end, const std::string& option)
{
    return std::find(begin, end, option) != end;
}

void getUpsFileName(int argc, char* argv[])
{
    if (cmdOptionExists(argv, argv+argc, "-u"))
    {
        gUpsFileName = getCmdOption(argv, argv + argc, "-u");
    }
    else
    {
        gUpsFileName = defaultUpsFileName;
    }
}

void getOutputFileName(int argc, char* argv[])
{
    if (cmdOptionExists(argv, argv+argc, "-o"))
    {
        gOutputFileName = getCmdOption(argv, argv + argc, "-o");
    }
    else
    {
        gOutputFileName = defaultOutputFileName;
    }
}

void getImagingCaseId( int argc, char *argv[])
{
    if (cmdOptionExists(argv, argv+argc, "-i"))
    {
        char *str = getCmdOption(argv, argv + argc, "-i");
        sscanf(str, "%d", &gImagingCaseId );
    }
    else
    {
        gImagingCaseId = defaultImagingCaseId;
    }
}

void getRequestedGate( int argc, char *argv[])
{
    char * str;

    if (cmdOptionExists(argv, argv+argc, "-gstart"))
    {
        str = getCmdOption(argv, argv + argc, "-gstart");
        sscanf(str, "%f", &gGateStartMm );
    }
    else
    {
        gGateStartMm = defaultGateStartMm;
    }

    if (cmdOptionExists(argv, argv+argc, "-gsize"))
    {
        str = getCmdOption(argv, argv + argc, "-gsize");
        sscanf(str, "%d", &gGateSizeIndex );
    }
    else
    {
        gGateSizeIndex = defaultGateSizeIndex;
    }

    if (cmdOptionExists(argv, argv+argc, "-bangle"))
    {
        str = getCmdOption(argv, argv + argc, "-bangle");
        sscanf(str, "%f", &gBeamAngleRad );
    }
    else
    {
        gBeamAngleRad = defaultGateSizeIndex;
    }
}

void getRequestedPrfIndex( int argc, char *argv[])
{
    char *str;

    if (cmdOptionExists(argv, argv+argc, "-prf"))
    {
        str = getCmdOption(argv, argv + argc, "-prf");
        sscanf(str, "%d", &gRequestedPrfIndex );
    }
    else
    {
        gRequestedPrfIndex = defaultPrfIndex;
    }
}

void getRegisterIndex( int argc, char *argv[])
{
    char *str;

    if (cmdOptionExists(argv, argv+argc, "-ri"))
    {
        str = getCmdOption(argv, argv + argc, "-ri");
        sscanf(str, "%d", &gRegisterIndex );
        gRegisterIndexIsSpecified = true;
    }
    else
    {
        gRegisterIndex = 0;
    }
}


void saveResults()
{
    Json::Value root;

    root["upsVersion"] = gUpsReaderSPtr->getVersion();

    root["PRF"] = Json::Value();
    root["PRF"]["requestedIndex"] = gRequestedPrfIndex;
    root["PRF"]["actualIndex"]    = gActualPrfIndex;

    root["testVersion"] = doppleracqtestversion;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "    ";

    std::ofstream file_id;
    file_id.open(gOutputFileName);

    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    writer->write(root, &file_id);
    file_id.close();

    std::ofstream blobFile;
    blobFile.open("dopplerBlob.bin", std::ios::out | std::ios::binary);
    blobFile.write( (char *)gPwSequenceBlob, 62*sizeof(uint32_t) );
    blobFile.close();

    // Save LUTS
    std::ofstream lutFile;
    lutFile.open("LutMSDIS.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gDopplerAcqPtr->getLutMSDIS(), TBF_LUT_MSDIS_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutMSAPW.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gDopplerAcqPtr->getLutMSAPW(), TBF_LUT_MSAPW_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutMSB0APS.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gDopplerAcqPtr->getLutMSB0APS(), TBF_LUT_MSB0APS_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutMSBPFI.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gDopplerAcqPtr->getLutMSBPFI(), TBF_LUT_MSBPFI_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutMSBPFQ.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gDopplerAcqPtr->getLutMSBPFQ(), TBF_LUT_MSBPFQ_SIZE *sizeof(uint32_t));
    lutFile.close();

#if 0
    lutFile.open("LutMSCBC.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gDopplerAcqPtr->getLutMSCBC(), TBF_LUT_MSCBC_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutMSCOMP.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gDopplerAcqPtr->getLutMSCOMP(), TBF_LUT_MSCOMP_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutMSFC.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gDopplerAcqPtr->getLutMSFC(), TBF_LUT_MSFC_SIZE *sizeof(uint32_t));
    lutFile.close();
#endif
    lutFile.open("LutB0FPX.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gDopplerAcqPtr->getLutB0FPX(), TBF_LUT_B0FPX_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutB1FPX.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gDopplerAcqPtr->getLutB0FPX(), TBF_LUT_B0FPX_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutB2FPX.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gDopplerAcqPtr->getLutB0FPX(), TBF_LUT_B0FPX_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutB3FPX.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gDopplerAcqPtr->getLutB0FPX(), TBF_LUT_B0FPX_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutB0FTXD.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gDopplerAcqPtr->getLutB0FPX(), TBF_LUT_B0FPX_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutB1FTXD.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gDopplerAcqPtr->getLutB0FPX(), TBF_LUT_B0FPX_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutB2FTXD.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gDopplerAcqPtr->getLutB0FPX(), TBF_LUT_B0FPX_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutB3FTXD.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gDopplerAcqPtr->getLutB0FPX(), TBF_LUT_B0FPX_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutB0FPY.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gDopplerAcqPtr->getLutB0FPY(), TBF_LUT_B0FPY_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutB1FPY.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gDopplerAcqPtr->getLutB1FPY(), TBF_LUT_B1FPY_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutB2FPY.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gDopplerAcqPtr->getLutB2FPY(), TBF_LUT_B2FPY_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutB3FPY.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gDopplerAcqPtr->getLutB3FPY(), TBF_LUT_B3FPY_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutMSDBG01.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gDopplerAcqPtr->getLutMSDBG01(), TBF_LUT_MSDBG01_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutMSDBG23.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gDopplerAcqPtr->getLutMSDBG23(), TBF_LUT_MSDBG23_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutMSSTTXC.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gDopplerAcqPtr->getLutMSSTTXC(), TBF_LUT_MSSTTXC_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutMSAFEINIT.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gDopplerAcqPtr->getLutMSAFEINIT(), TBF_LUT_MSAFEINIT_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutMSRTAFE.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gDopplerAcqPtr->getLutMSRTAFE(), TBF_LUT_MSRTAFE_SIZE *sizeof(uint32_t));
    lutFile.close();
}

void checkAndSaveSequence()
{
    uint32_t numPwNodes = gSequenceBuilderPtr->getNumPwNodes();
    uint32_t payloadSize = gSequenceBuilderPtr->getPayloadSize();

    uint32_t expectedNumPwNodes = gUpsReaderSPtr->getNumPwSamplesPerFrame(gActualPrfIndex);

    printf("expected nodes = %d,nodes in the sequence = %d\n",
           expectedNumPwNodes, numPwNodes);

    if (expectedNumPwNodes != (numPwNodes-1))
    {
        printf("ERROR: number of PW nodes in the sequence did not match expected value\n");
        return;
    }
    const DauSequenceBuilder::LleHeader* llePtr;
    llePtr = gSequenceBuilderPtr->getLleHeaderPtr();
    const DauSequenceBuilder::LleHeader* curNode = llePtr;

    // Traverse through the linked list elements and verify that it loops back
    // as expected
    uint32_t nextNodeNum;
    for (uint32_t i = 0; i < numPwNodes + 3; i++)
    {
        nextNodeNum = curNode->next;
        if (nextNodeNum > (DauSequenceBuilder::FIRST_PRI_NODE + numPwNodes))
        {
            printf("ERROR: next ptr in node %d is %d\n", i, nextNodeNum);
            return;
        }
        curNode = llePtr + nextNodeNum;
    }
    printf("next node of last node is %d\n", nextNodeNum);
    if (nextNodeNum == DauSequenceBuilder::FIRST_PRI_NODE + 1)
    {
        printf("\nLLE are linked correctly\n");
    }

    // Check payload

    // Save the LLEs
}

void printUsage()
{
    printf("Usage: doppleracqtest <one or more args>\n");
    printf("\t-u        ups file name\n");
    printf("\t-i        imaging case Id\n");
    printf("\t-o        output json file name\n");
    printf("\t-gstart   range gate start in mm\n");
    printf("\t-gsize    index of range gate size\n");
    printf("\t-bangle   beam angle in radians\n");
    printf("\t-prf      requested PRF index\n");
    printf("\nNote: if both -ri and -bi are specified only one register value will be printed\n");
    printf("Version %s\n", doppleracqtestversion);
    printf("\nDefault values:\n");
    printf("\tUPS file name:          %s\n",        defaultUpsFileName);
    printf("\timaging case id         %d\n",        defaultImagingCaseId);
    printf("\tPRF index               %d\n",        defaultPrfIndex);
    printf("\toutput json file name:  %s\n",        defaultOutputFileName);
}

int main(int argc, char * argv[])
{
    if (argc == 1)
    {
        printUsage();
        return (0);
    }
    getUpsFileName      ( argc, argv );
    getOutputFileName   ( argc, argv );
    getImagingCaseId    ( argc, argv );
    getRequestedGate    ( argc, argv );
    getRequestedPrfIndex( argc, argv );
    getRegisterIndex    ( argc, argv );

    gUpsReaderSPtr = std::make_shared<UpsReader>();
    gSequenceBuilderPtr = new DauSequenceBuilder(nullptr, gUpsReaderSPtr);

    if (!gUpsReaderSPtr->open(gUpsFileName))
    {
        printf("ERROR: couldn't open %s", gUpsFileName);
        return (-1);
    }

    gUpsReaderSPtr->loadGlobals();
    gUpsReaderSPtr->loadTransducerInfo();


#if 0
    gDopplerAcqPtr = new DopplerAcq( gUpsReaderSPtr );
#else
    gDopplerAcqPtr = new DopplerAcq();
    gDopplerAcqPtr->init( gUpsReaderSPtr );
#endif

    gDopplerAcqPtr->calculateActualPrfIndex( gImagingCaseId, gGateStartMm, gGateSizeIndex, gRequestedPrfIndex, gActualPrfIndex );

    gDopplerAcqPtr->calculateBeamParams( gImagingCaseId, gGateStartMm, gGateSizeIndex, gBeamAngleRad, gActualPrfIndex);

    gDopplerAcqPtr->buildLuts( gImagingCaseId );
    gDopplerAcqPtr->buildPwSequenceBlob( gPwSequenceBlob );
    gSequenceBuilderPtr->buildPwSequence( gImagingCaseId, gActualPrfIndex, gPwSequenceBlob);

    saveResults();
    checkAndSaveSequence();

    // UPS Reader should be closed at the very end
    gUpsReaderSPtr->close();
    return (0);
}
