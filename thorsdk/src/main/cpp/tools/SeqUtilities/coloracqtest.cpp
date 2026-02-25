//
// Copyright 2018 EchoNous, Inc.
//
#include <stdio.h>
#include <cstdint>
#include <algorithm>
#include <memory>

#include <TbfRegs.h>
#include <UpsReader.h>
#include <ColorAcq.h>
#include <DauMemory.h>
#include <ScanDescriptor.h>
#include <fstream>
#include <iostream>
#include <json/json.h>

static char     coloracqtestversion[]   = "2.0";
static char     defaultUpsFileName[]    = "thor.db";
static char     defaultOutputFileName[] = "coloracqout.json";
static float    defaultAxialStart       = 40.0;
static float    defaultAxialSpan        = 40.0;
static float    defaultAzimuthStart     = -0.78539816/2;
static float    defaultAzimuthSpan      = 1.570796326/2;
static uint32_t defaultPrfIndex         = 0;
static uint32_t defaultImagingCaseId    = 86;
static uint32_t defaultWallFilterType   = 0;

char*                      gUpsFileName;
char*                      gOutputFileName;
ColorAcq*                  gColorAcqPtr;
std::shared_ptr<UpsReader> gUpsReaderSPtr;
uint32_t                   gImagingCaseId;
uint32_t                   gRequestedPrfIndex;
uint32_t                   gActualPrfIndex;
uint32_t                   gBeamIndex;
uint32_t                   gRegisterIndex;
ScanDescriptor             gRequestedRoi;
ScanDescriptor             gActualRoi;
uint32_t                   gWallFilterType;
uint32_t                   gFiringCount;
uint32_t                   gColorBlob[512][62];
ScanConverterParams        gScanConverterParams;

bool gBeamIndexIsSpecified = false;
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

void getRequestedRoi( int argc, char *argv[])
{
    char * str;
    
    // Azimuth
    if (cmdOptionExists(argv, argv+argc, "-azstart"))
    {
        str = getCmdOption(argv, argv + argc, "-azstart");
        sscanf(str, "%f", &gRequestedRoi.azimuthStart );
    }
    else
    {
        gRequestedRoi.azimuthStart = defaultAzimuthStart;
    }

    if (cmdOptionExists(argv, argv+argc, "-azspan"))
    {
        str = getCmdOption(argv, argv + argc, "-azspan");
        sscanf(str, "%f", &gRequestedRoi.azimuthSpan );
    }
    else
    {
        gRequestedRoi.azimuthSpan = defaultAzimuthSpan;
    }

    // Axial
    if (cmdOptionExists(argv, argv+argc, "-axstart"))
    {
        str = getCmdOption(argv, argv + argc, "-axstart");
        sscanf(str, "%f", &gRequestedRoi.axialStart );
    }
    else
    {
        gRequestedRoi.axialStart = defaultAxialStart;
    }
    if (cmdOptionExists(argv, argv+argc, "-axspan"))
    {
        str = getCmdOption(argv, argv + argc, "-axspan");
        sscanf(str, "%f", &gRequestedRoi.axialSpan );
    }
    else
    {
        gRequestedRoi.axialSpan = defaultAxialSpan;  //
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

void getBeamIndex( int argc, char *argv[])
{
    char *str;

    if (cmdOptionExists(argv, argv+argc, "-bi"))
    {
        str = getCmdOption(argv, argv + argc, "-bi");
        sscanf(str, "%d", &gBeamIndex );
        gBeamIndexIsSpecified = true;
    }
    else
    {
        gBeamIndex = 0;
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

void getWallFilterType( int argc, char *argv[] )
{
    char *str;
    if (cmdOptionExists(argv, argv+argc, "-wfType"))
    {
        str = getCmdOption(argv, argv + argc, "-wfType");
        sscanf(str, "%d", &gWallFilterType );
    }
    else
    {
        gWallFilterType = defaultWallFilterType;
    }
}

void buildImagingCaseJsonObject(Json::Value& ic)
{
    ic["ID"] = gImagingCaseId;

    ic["RequestedRoi"] = Json::Value();
    ic["RequestedRoi"]["axialStart"]   = gRequestedRoi.axialStart;
    ic["RequestedRoi"]["axialSpan"]    = gRequestedRoi.axialSpan;
    ic["RequestedRoi"]["azimuthStart"] = gRequestedRoi.azimuthStart;
    ic["RequestedRoi"]["azimuthSpan"]  = gRequestedRoi.azimuthSpan;

    ic["ActualRoi"] = Json::Value();
    ic["ActualRoi"]["axialStart"]   = gActualRoi.axialStart;
    ic["ActualRoi"]["axialSpan"]    = gActualRoi.axialSpan;
    ic["ActualRoi"]["azimuthStart"] = gActualRoi.azimuthStart;
    ic["ActualRoi"]["azimuthSpan"]  = gActualRoi.azimuthSpan;
}

void buildBeamParamsJsonObject(Json::Value& p)
{
    const ColorAcq::BeamParams* bpPtr = gColorAcqPtr->getBeamParamsPtr();

    p["bfS"]                    = bpPtr->bfS;
    p["fpS"]                    = bpPtr->fpS;
    p["dmStart"]                = bpPtr->dmStart;
    p["ccWe"]                   = bpPtr->ccWe;
    p["ccOne"]                  = bpPtr->ccOne;
    p["numGainBreakpoints"]     = bpPtr->numGainBreakpoints;
    p["tgcirLow"]               = bpPtr->tgcirLow;
    p["tgcirHigh"]              = bpPtr->tgcirLow;
    p["outputSamples"]          = bpPtr->outputSamples;
    p["cfpOutputSamples"]       = bpPtr->cfpOutputSamples;
    p["rxAngles"]               = Json::Value(Json::arrayValue);
    for (int i = 0; i < 4; i++)
        p["rxAngles"][i]        = bpPtr->rxAngles[i];
    p["rxAperture"]             = bpPtr->rxAperture;
    p["txWaveformLength"]       = bpPtr->txWaveformLength;
    p["beamScales"]             = Json::Value(Json::arrayValue);
    for (int i = 0; i < 4; i++)
        p["beamScales"][i]      = bpPtr->beamScales[i];
    p["tgcFlat"]                = bpPtr->tgcFlat;
    p["apodFlat"]               = bpPtr->apodFlat;
    p["lateralGains"]           = Json::Value(Json::arrayValue);
    for (int i = 0; i < 4; i++)
        p["lateralGains"][i]    = bpPtr->lateralGains[i];
    p["numTxSpiCommands"]       = bpPtr->numTxSpiCommands;
    p["bpfSelect"]              = bpPtr->bpfSelect;
    p["tgcPtr"]                 = bpPtr->tgcPtr;
    p["hvClkEnOff"]             = bpPtr->hvClkEnOff;
    p["hvClkEnOn"]              = bpPtr->hvClkEnOn;
    p["hvWakeupEn"]             = bpPtr->hvWakeupEn;
    p["fpPtr0"]                 = bpPtr->fpPtr0;
    p["fpPtr1"]                 = bpPtr->fpPtr1;
    p["txPtr0"]                 = bpPtr->txPtr0;
    p["txPtr1"]                 = bpPtr->txPtr1;
    p["irPtr0"]                 = bpPtr->irPtr0;
    p["irPtr1"]                 = bpPtr->irPtr1;
    p["bApSptr"]                = bpPtr->bApSptr;
    p["decimationFctor"]        = bpPtr->decimationFactor;
    p["bpfTaps"]                = bpPtr->bpfTaps;
    p["bpfScale"]               = bpPtr->bpfScale;
    p["bpfCount"]               = bpPtr->bpfCount;
    p["agcProfile"]             = bpPtr->agcProfile;
    p["tgcFlip"]                = bpPtr->tgcFlip;
    p["ensembleSize"]           = bpPtr->ensembleSize;
    p["colorProcessingSamples"] = bpPtr->colorProcessingSamples;
}

void buildBeamIndependentParamsJsonObject( Json::Value &p )
{
    const ColorAcq::BeamIndependentParams* ptr = gColorAcqPtr->getBeamIndependentParamsPtr();
    
    p["rxStart"]            = ptr->rxStart;
    p["depthIndex"]         = ptr->depthIndex;
    p["imagingDepth"]       = ptr->imagingDepth;
    p["baseConfigLength"]   = ptr->baseConfigLength;
    p["colorRxPitch"]       = ptr->colorRxPitch;
    p["speedOfSound"]       = ptr->speedOfSound;
    p["bpfCenterFrequency"] = ptr->bpfCenterFrequency;
    p["bpfBandwidth"]       = ptr->bpfBandwidth;
    p["numTxElements"]      = ptr->numTxElements;
    p["numHalgCycles"]      = ptr->numHalfCycles;
    p["numTxBeamsBC"]       = ptr->numTxBeamsBC;
    p["rxPitchBC"]          = ptr->rxPitchBC;
    p["txCenterFrequency"]  = ptr->txCenterFrequency;
    p["ensDiv"]             = ptr->ensDiv;
    p["ctbReadBlockDiv"]    = ptr->ctbReadBlockDiv;
}

void buildTxAngles( Json::Value &r )
{
    const float * t_a = gColorAcqPtr->getTxAngles();

    r["txAngle"] = Json::Value(Json::arrayValue);
    for (int i = 0; i < 64; i++)
    {
        r["txAngle"][i] = t_a[i]; // * 45.0/atan(1.0);
    }
}

void buildSequencerCountsJsonObject( Json::Value &c )
{
    uint32_t interleaveFactor;
    uint32_t numGroups;
    uint32_t numCtbReadBlocks;

    gColorAcqPtr->getSequencerLoopCounts( gFiringCount,
                                          interleaveFactor,
                                          numGroups,
                                          numCtbReadBlocks );
    c["firingCount"]      = gFiringCount;
    c["interleaveFactor"] = interleaveFactor;
    c["numGroups"]        = numGroups;
    c["numCtbReadBlocks"] = numCtbReadBlocks;
}

void saveResults()
{
    Json::Value root;

    root["upsVersion"] = gUpsReaderSPtr->getVersion();

    root["imagingCase"] = Json::Value();
    buildImagingCaseJsonObject(root["imagingCase"]);
    
    root["BeamParams"] = Json::Value();
    buildBeamParamsJsonObject(root["BeamParams"]);

    root["BeamIndependentParams"] = Json::Value();
    buildBeamIndependentParamsJsonObject(root["BeamIndependentParams"]);

    root["SequencerCounts"] = Json::Value();
    buildSequencerCountsJsonObject(root["SequencerCounts"]);

    root["PRF"] = Json::Value();
    root["PRF"]["requestedIndex"] = gRequestedPrfIndex;
    root["PRF"]["actualIndex"]    = gActualPrfIndex;

    buildTxAngles(root);
    
    ColorAcq::BeamCount bc;
    gColorAcqPtr->getBeamCounts(bc);
    root["BeamCounts"] = Json::Value();
    root["BeamCounts"]["NumTxBeamsB"]  = bc.txB;
    root["BeamCounts"]["NumRxBeamsB"]  = bc.rxB;
    root["BeamCounts"]["NumTxBeamsBC"] = bc.txBC;
    root["BeamCounts"]["NumRxBeamsBC"] = bc.rxBC;
    
    root["ScanConverterParams"] = Json::Value();
    root["ScanConverterParams"]["startSampleMm"]    = gScanConverterParams.startSampleMm;
    root["ScanConverterParams"]["samplesSpacingMm"] = gScanConverterParams.sampleSpacingMm;
    root["ScanConverterParams"]["numSamples"]       = gScanConverterParams.numSamples;
    root["ScanConverterParams"]["startRayRadian"]   = gScanConverterParams.startRayRadian;
    root["ScanConverterParams"]["raySpacing"]       = gScanConverterParams.raySpacing;
    root["ScanConverterParams"]["numRays"]          = gScanConverterParams.numRays;
    
    root["testVersion"] = coloracqtestversion;
        
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "    ";

    std::ofstream file_id;
    file_id.open(gOutputFileName);

    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    writer->write(root, &file_id);
    file_id.close();
    if (gRegisterIndexIsSpecified && gBeamIndexIsSpecified)
    {
        printf ("Beam %02d, Register %02d - %08X\n", gBeamIndex, gRegisterIndex, gColorBlob[gBeamIndex][gRegisterIndex]);
    }
    else if (gRegisterIndexIsSpecified)
    {
        for (uint32_t i = 0; i < gFiringCount; i++)
        {
            printf("Regster %3d %3d = %08x\n", gRegisterIndex, i, gColorBlob[i][gRegisterIndex]);
        }
    }
    else if (gBeamIndexIsSpecified)
    {
        for ( uint32_t i = 0; i < 62; i++)
        {
            printf("Beam %02d %2d %08x\n", gBeamIndex, i, gColorBlob[gBeamIndex][i]);
        }
    }
    std::ofstream blobFile;
    blobFile.open("colorBlob.bin", std::ios::out | std::ios::binary);
    blobFile.write( (char *)gColorBlob, gFiringCount*62*sizeof(uint32_t) );
    blobFile.close();

    // Save LUTS
    std::ofstream lutFile;
    lutFile.open("LutMSDIS.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gColorAcqPtr->getLutMSDIS(), TBF_LUT_MSDIS_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutMSAPW.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gColorAcqPtr->getLutMSAPW(), TBF_LUT_MSAPW_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutMSB0APS.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gColorAcqPtr->getLutMSB0APS(), TBF_LUT_MSB0APS_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutMSBPFI.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gColorAcqPtr->getLutMSBPFI(), TBF_LUT_MSBPFI_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutMSBPFQ.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gColorAcqPtr->getLutMSBPFQ(), TBF_LUT_MSBPFQ_SIZE *sizeof(uint32_t));
    lutFile.close();

#if 0
    lutFile.open("LutMSCBC.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gColorAcqPtr->getLutMSCBC(), TBF_LUT_MSCBC_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutMSCOMP.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gColorAcqPtr->getLutMSCOMP(), TBF_LUT_MSCOMP_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutMSFC.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gColorAcqPtr->getLutMSFC(), TBF_LUT_MSFC_SIZE *sizeof(uint32_t));
    lutFile.close();
#endif    
    lutFile.open("LutB0FPX.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gColorAcqPtr->getLutB0FPX(), TBF_LUT_B0FPX_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutB1FPX.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gColorAcqPtr->getLutB0FPX(), TBF_LUT_B0FPX_SIZE *sizeof(uint32_t));
    lutFile.close();
    
    lutFile.open("LutB2FPX.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gColorAcqPtr->getLutB0FPX(), TBF_LUT_B0FPX_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutB3FPX.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gColorAcqPtr->getLutB0FPX(), TBF_LUT_B0FPX_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutB0FTXD.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gColorAcqPtr->getLutB0FPX(), TBF_LUT_B0FPX_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutB1FTXD.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gColorAcqPtr->getLutB0FPX(), TBF_LUT_B0FPX_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutB2FTXD.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gColorAcqPtr->getLutB0FPX(), TBF_LUT_B0FPX_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutB3FTXD.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gColorAcqPtr->getLutB0FPX(), TBF_LUT_B0FPX_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutB0FPY.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gColorAcqPtr->getLutB0FPY(), TBF_LUT_B0FPY_SIZE *sizeof(uint32_t));
    lutFile.close();
    
    lutFile.open("LutB1FPY.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gColorAcqPtr->getLutB1FPY(), TBF_LUT_B1FPY_SIZE *sizeof(uint32_t));
    lutFile.close();
    
    lutFile.open("LutB2FPY.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gColorAcqPtr->getLutB2FPY(), TBF_LUT_B2FPY_SIZE *sizeof(uint32_t));
    lutFile.close();
    
    lutFile.open("LutB3FPY.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gColorAcqPtr->getLutB3FPY(), TBF_LUT_B3FPY_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutMSDBG01.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gColorAcqPtr->getLutMSDBG01(), TBF_LUT_MSDBG01_SIZE *sizeof(uint32_t));
    lutFile.close();
    
    lutFile.open("LutMSDBG23.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gColorAcqPtr->getLutMSDBG23(), TBF_LUT_MSDBG23_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutMSSTTXC.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gColorAcqPtr->getLutMSSTTXC(), TBF_LUT_MSSTTXC_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutMSAFEINIT.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gColorAcqPtr->getLutMSAFEINIT(), TBF_LUT_MSAFEINIT_SIZE *sizeof(uint32_t));
    lutFile.close();

    lutFile.open("LutMSRTAFE.bin", std::ios::out | std::ios::binary);
    lutFile.write( (char *)gColorAcqPtr->getLutMSRTAFE(), TBF_LUT_MSRTAFE_SIZE *sizeof(uint32_t));
    lutFile.close();
}

void printUsage()
{
    printf("Usage: coloracqtest <one or more args>\n");
    printf("\t-u        ups file name\n");
    printf("\t-i        imaging case Id\n");
    printf("\t-o        output json file name\n");
    printf("\t-axstart  axial start in mm\n");
    printf("\t-axspan   axial span in mm\n");
    printf("\t-azstart  azimuth start in rad or mm\n");
    printf("\t-azspan   azimith span in rad or mm\n");
    printf("\t-bi       beam index for dumping register set\n");
    printf("\t-ri       register index for dumping register values for all beams\n");
    printf("\t-prf      requested PRF index\n");
    printf("\t-wfType   requested Wall Filter Type (0 is default)\n");
    printf("\nNote: if both -ri and -bi are specified only one register value will be printed\n");
    printf("Version %s\n", coloracqtestversion);
    printf("\nDefault values:\n");
    printf("\tUPS file name:          %s\n",        defaultUpsFileName);
    printf("\timaging case id         %d\n",        defaultImagingCaseId);
    printf("\tPRF index               %d\n",        defaultPrfIndex);
    printf("\toutput json file name:  %s\n",        defaultOutputFileName);
    printf("\taxstart:                %3.4f mm\n",  defaultAxialStart);
    printf("\taxspan:                 %3.4f mm\n",  defaultAxialSpan);
    printf("\tazstart:                %3.4f rad\n", defaultAzimuthStart);
    printf("\tazspan:                 %3.4f rad\n", defaultAzimuthSpan);
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
    getRequestedRoi     ( argc, argv );
    getRequestedPrfIndex( argc, argv );
    getBeamIndex        ( argc, argv );
    getRegisterIndex    ( argc, argv );
    getWallFilterType   ( argc, argv );
    
    gUpsReaderSPtr = std::make_shared<UpsReader>();
    
    if (!gUpsReaderSPtr->open(gUpsFileName))
    {
        printf("ERROR: couldn't open %s", gUpsFileName);
        return (-1);
    }

    gUpsReaderSPtr->loadGlobals();
    gUpsReaderSPtr->loadTransducerInfo();
    

#if 0
    gColorAcqPtr = new ColorAcq( gUpsReaderSPtr );
#else
    gColorAcqPtr = new ColorAcq();
    gColorAcqPtr->init( gUpsReaderSPtr );
#endif
    gColorAcqPtr->fetchGlobals();
    gColorAcqPtr->getImagingCaseConstants( gImagingCaseId, gWallFilterType );


    gColorAcqPtr->calculateBeamParams( gImagingCaseId,
                                       gRequestedPrfIndex,
                                       gRequestedRoi,
                                       gActualPrfIndex,
                                       gActualRoi );
    
    gColorAcqPtr->buildLuts( gImagingCaseId, gActualRoi );
    gColorAcqPtr->buildSequenceBlob( gImagingCaseId, gColorBlob );
    gColorAcqPtr->calculateScanConverterParams( gActualRoi, gScanConverterParams );
    saveResults();

    // UPS Reader should be closed at the very end
    gUpsReaderSPtr->close();
    return (0);
}
