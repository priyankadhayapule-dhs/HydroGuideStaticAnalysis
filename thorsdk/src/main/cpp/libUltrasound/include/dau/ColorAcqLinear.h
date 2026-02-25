//
// Copyright 2018 EchoNous Inc.
//

#pragma once

#include <stdint.h>
#include <memory>
#include <UpsReader.h>
#include <ScanDescriptor.h>
#include <DauRegisters.h>
#include <ThorError.h>

#define CBLOB_SIZE_LINEAR 560

class ColorAcqLinear
{
public:
    typedef struct
    {
        // Pointers to contents of MSSTTXC LUT
        uint32_t bconfig_ptr;       // Base config
        uint32_t pmline_ptr;        // PM line
        uint32_t bbeam_start_ptr;   // B transition config
        uint32_t cbeam_start_ptr;   // C transition config
        uint32_t btxconfig_ptr[32]; // B TX beam config set
        uint32_t ctxconfig_ptr[32]; // C TX beam config set
    } TXLUT_ptr;

    typedef struct {
        // ROI bounds after snap
        uint32_t ls_beam;      // Start TX beam
        uint32_t le_beam;      // End TX beam
        uint32_t tx_beams;     // Total TX beams
        uint32_t as_sample;    // Start ADC sample
        uint32_t ae_sample;    // End ADC sample
    } ROIBoundsStruct;


    typedef struct
    {
        uint32_t bfS;
        uint32_t adcS;
        uint32_t fpS; 
        int32_t  dmStart;
        uint32_t ccWe;
        uint32_t ccOne;

        // gain breakpoints
        uint32_t numGainBreakpoints;  // G_S
        uint32_t tgcirLow;
        uint32_t tgcirHigh;
        
        uint32_t outputSamples;       // O_S  -   #of samples/line coming out of DAU
        uint32_t cfpOutputSamples;    // CFP_O_S 
        
        float    rxAngles[4];
        uint32_t rxAperture;
        float    txWaveformLength;   // usec ?
        int32_t  beamScales[4];
        uint32_t tgcFlat;             // 1 => TGC is flattened
        uint32_t apodFlat;            // 1 => Apodization is flattened
         int32_t lateralGains[4];     // g0, g1, g2, g3
        uint32_t numTxSpiCommands;    // txd_w_c
        uint32_t bpfSelect;           // vbp_fs
        uint32_t tgcPtr;              // bg_ptr
        uint32_t hvClkEnOff;          // hv_clken_off
        uint32_t hvClkEnOn;           // hv_clken_on
        uint32_t hvWakeupEn;          // bp_hv_wakeup_en

        uint32_t fpPtr0;
        uint32_t fpPtr1;
        uint32_t txPtr0;
        uint32_t txPtr1;
        uint32_t irPtr0;
        uint32_t bApSptr;             // part of register IRPTR0
        uint32_t irPtr1;
        float    decimationFactor;    // BPF decimation factor aka D
        uint32_t bpfTaps;
        uint32_t bpfScale;
        uint32_t bpfCount;            // vbp_fc
        uint32_t agcProfile;
        uint32_t tgcFlip;
        uint32_t ensembleSize;
        uint32_t colorProcessingSamples;
    } BeamParams;

    typedef struct
    {
        //------------------------------
        uint32_t rxStart;
        uint32_t depthIndex;
        float    imagingDepth;
        uint32_t baseConfigLength;
        float    colorRxPitch;
        float    speedOfSound;
        float    bpfCenterFrequency;
        float    bpfBandwidth;
        uint32_t numTxElements;
        uint32_t numHalfCycles;
        uint32_t numTxBeamsBC;
        float    rxPitchBC;
        float    txCenterFrequency;
        uint32_t ensDiv;
        uint32_t ctbReadBlockDiv;
        uint32_t actualPrfIndex;
        uint32_t colorRxAperture;
        
    } BeamIndependentParams;

    
    typedef struct
    {
        bool txEn;
        bool rxEn;
        bool bfEn;
        bool ctbwEn;
        bool cfpEn;
        bool outEn;
        bool pmLine;
        bool pmAfePwrSb;
        bool bpSlopeEn;
        bool ceof;
        bool eos;
    } EnableFlags;
    

    typedef struct
    {
        uint32_t idxStart;
        uint32_t idxStop;
        uint32_t numBkptsB;
        uint32_t numBkptsC;
        float    focalDepthMm;
    } FocalPointInfo;

    typedef struct
    {
        uint32_t txB;
        uint32_t rxB;
        uint32_t txBC;
        uint32_t rxBC;
        uint32_t rxC;
    } BeamCount;

    union BpRegBENABLE
    {
        struct bits
        {
            uint32_t txEn           : 1;
            uint32_t rxEn           : 1;
            uint32_t bfEn           : 1;
            uint32_t outEn          : 1; // 3
            uint32_t outType        : 1; // 4

            uint32_t gap0           : 3; // 5, 6, 7

            uint32_t outIqType      : 1; // 8

            uint32_t gap1           : 1; // 9

            uint32_t ctbWrEn        : 1; // 10
            uint32_t cfpEn          : 1; // 11

            uint32_t gap2           : 4; // 12, 13, 14, 15

            uint32_t beof           : 1; // 16
            uint32_t ceof           : 1; // 17
            uint32_t eos            : 1; // 18
            uint32_t bpSlopeEn      : 1; // 19

            uint32_t gap3           : 2; // 20, 21

            uint32_t bpHvSpiEn      : 1; // 22
            uint32_t bpStHvTrigHold : 1; // 23
            uint32_t pmonEn         : 1; // 24

            uint32_t gap4           : 2; // 25,26

            uint32_t bpHvWakeupEn   : 1; // 27
            uint32_t bpAltHvConfig  : 1; // 28
            uint32_t bpHvPsBurstEn  : 1; // 29
            uint32_t pmonEnPwrMgt   : 1; // 30

            uint32_t gap5           : 1; // 31
        } bits;
        uint32_t val;
    };

    union BpRegBPWR
    {
        struct bits
        {
            uint32_t gap0           : 1; // 0
            uint32_t pmAfePwrOn     : 1; // 1
            uint32_t pmAfePwrOff    : 1; // 2
            uint32_t pmAfePwrSb     : 1; // 3
            uint32_t pmAfeGblTimer  : 1; // 4
            uint32_t pmAfeSbTimer   : 1; // 5
            uint32_t pmDigChPm      : 1; // 6
            uint32_t gap1           : 25; // 7 .. 31
        } bits;
        uint32_t val;
    };

    union BpRegBPF0
    {
        struct bits
        {
            uint32_t vbpFc          : 1;  // 0
            uint32_t vbpPr          : 1;  // 1

            uint32_t gap0           : 6;  // 2..7

            uint32_t vbpPlc         : 8;  // 8..15
            uint32_t vbpFl          : 8;  // 16..23
            uint32_t vbpFs          : 2;  // 24..25

            uint32_t gap1           : 2;  // 26..27

            uint32_t vbpOscale      : 3;  // 28..30
            uint32_t gap2           : 1;  // 31
    
        } bits;
        uint32_t val;
    };


    union BpRegBPF1
    {
        struct bits
        {
            uint32_t vbpPinc0       : 8;  // 0..7
            uint32_t vbpPinc1       : 8;  // 8..15
            uint32_t vbpWc0         : 8;  // 16..23
            uint32_t vbpWc1         : 8;  // 24..31
        } bits;
        uint32_t val;
    };

    union BpRegBPF2
    {
        struct bits
        {
            uint32_t vbpPhaseInc    : 8;  // 0..7

            uint32_t gap0           : 8;  // 8..15

            uint32_t vbpWc          : 14; // 16..29

            uint32_t gap1           : 2;  // 30..31
        } bits;
        uint32_t val;
    };

    union BpRegBPF3
    {
        struct bits
        {
            uint32_t vbpIc          : 14; // 0..13

            uint32_t gap0           : 2;  // 14..15

            uint32_t vbpDseq        : 16;  // 16..31
        } bits;
        uint32_t val;
    };

    union BpRegDTGC0
    {
        struct bits
        {
            uint32_t tgc0Lgc        : 24;  // 0..23
            uint32_t tgc0Flat       : 1;   // 24
            uint32_t bgPtr          : 5;   // 25..29
            uint32_t gap0           : 2;   // 30..31
        } bits;
        uint32_t val;
    };

    union BpRegDTGC1
    {
        struct bits
        {
            uint32_t tgc1Lgc        : 24;  // 0..23
            uint32_t tgc1Flat       : 1;   // 24
            uint32_t bgRc           : 5;   // 25..29
            uint32_t tgcFlip        : 1;   // 30
            uint32_t gap0           : 1;   // 31
        } bits;
        uint32_t val;
    };

    union BpRegDTGC2
    {
        struct bits
        {
            uint32_t tgc2Lgc        : 24;  // 0..23
            uint32_t tgc2Flat       : 1;   // 24
            uint32_t gap0           : 7;   // 25>>31
        } bits;
        uint32_t val;
    };

    union BpRegDTGC3
    {
        struct bits
        {
            uint32_t tgc3Lgc        : 24;  // 0..23
            uint32_t tgc3Flat       : 1;   // 24
            uint32_t gap0           : 7;   // 25>>31
        } bits;
        uint32_t val;
    };

    union BpRegCCOMP
    {
        struct bits
        {
            uint32_t ccPtr          : 10;  // 0..9
            uint32_t gap0           : 6;   // 10..15
            uint32_t ccOutEn        : 1;   // 16
            uint32_t ccWe           : 1;   // 17
            uint32_t ccSum          : 1;   // 18
            uint32_t ccOne          : 1;   // 19
            uint32_t ccInv          : 1;   // 20
            uint32_t ccOutMode      : 1;   // 21
            uint32_t gap1           : 10;  // 22..31
        } bits;
        uint32_t val;
    };


    typedef struct
    {
        uint32_t outerPtr;    // w_rptr, r_rptr
        uint32_t innerPtr;    // w_iptr, r_iptr
        uint32_t outerIncr;   // w_ri,   r_ri
        uint32_t innerIncr;   // w_ii,   r_ii
        uint32_t outerCount;  // w_r_s, r_r_s
        uint32_t innerCount;  // w_i_s, r_i_s
    } CtbParams;

public:
    
    ColorAcqLinear(const std::shared_ptr<UpsReader>& upsReader);
    ColorAcqLinear() {};
    ~ColorAcqLinear();

    void init( const std::shared_ptr<UpsReader>& upsReader) { mUpsReaderSPtr = upsReader; };
    ThorStatus calculateBeamParams(const uint32_t       imagingCaseId,
                             const uint32_t       requestedPrfIndex,
                             const ScanDescriptor requestedRoi,
                             uint32_t&            actualPrfIndex,
                             ScanDescriptor&      actualRoi);

    void calculateScanConverterParams( const ScanDescriptor& roi, ScanConverterParams& scp );
    
    void buildLuts( const uint32_t imagingCaseId, ScanDescriptor roi );
    
    void fetchGlobals();

    void getImagingCaseConstants(uint32_t imagingCaseId, uint32_t wallFilterType = 0 );

    void logImagingCaseConstants();
    void logBeamParams();
    void calculateMaxPrfIndex( ScanDescriptor&      actualRoi,
                               uint32_t&            maxPrfIndex );
    void calculateTxLinesSpan( const uint32_t       requestedPrfIndex,
                               const ScanDescriptor requestedRoi,
                               uint32_t&            actualPrfIndex,
                               ScanDescriptor&      actualRoi);

    void calculateGrid( uint32_t numTxBeams,
                        uint32_t numRxBeams,
                        const ScanDescriptor requestedRoi,
                        ScanDescriptor& actualRoi,
                        float pitch,
                        uint32_t NP,
                        uint32_t numTxBeamsC,
                        float txAngles[]);

    void calculateAxialRoi( const uint32_t imagingCaseId,
                            uint32_t       &bfS,
                            int32_t        &dmStart,
                            ScanDescriptor &roi );
    
    void buildSequenceBlob( uint32_t imagingCaseId, uint32_t colorBlob[512][62] );
    void buildSequenceBlob( uint32_t imagingCaseId );

    void calculateFocalInterpLut( const uint32_t imagingCaseId );

    void calculateDgcInterpLut( const uint32_t imagingCaseId, ScanDescriptor roi );

    void calculateAtgc( const uint32_t imagingCaseId,
                        const uint32_t bfS,
                        const uint32_t dmStart,
                        const uint32_t rxStart );

    void calculateTxConfig( const uint32_t imagingCaseId,
                            const ScanDescriptor &roi,
                            const uint32_t numTxBeamsC,
                            const float    t_a[] );

    void calculateBpfLut( const uint32_t imagingCaseId );

    void setupBpRegs( EnableFlags flags,
                      uint32_t    txdSpiPtr,
                      uint32_t    txdWordCount,
                      float       txAngle,
                      CtbParams   ctbInput,
                      CtbParams   ctbOutput,
                      uint32_t    txPingIndex,
                      uint32_t    pri,
                      uint32_t    bgPtr,
                      uint32_t    bpRegs[62],
                      uint32_t    pi);

    const BeamParams* getBeamParamsPtr() { return (&mBeamParams); };

    const BeamIndependentParams* getBeamIndependentParamsPtr() { return (&mBip); };

    void  getSequencerLoopCounts( uint32_t& totalBeams,
                                  uint32_t& interleaveFactor,
                                  uint32_t& numGroups,
                                  uint32_t& numCtbReadBlocks);
    void getBeamCounts( BeamCount &bc );

    void setupCtbParams(CtbParams &input, CtbParams &output, uint32_t groupIndex, uint32_t ensembleIndex, uint32_t interleaveIndex);
    void setupTxdSpiPtr( uint32_t & ptr, uint32_t groupIndex, uint32_t ensembleIndex, uint32_t interleaveIndex);
    void setupTxPingIndex( uint32_t & pindex, uint32_t groupIndex, uint32_t ensembleIndex, uint32_t interleaveIndex);
    void setupTxAngle( float & ta, uint32_t groupIndex, uint32_t ensembleIndex, uint32_t interleaveIndex);
    void initCtbParams( CtbParams &input, CtbParams &output);
    float* getTxAngles() { return (mTxAngles); };
    uint32_t* getSequenceBlobPtr() { return((uint32_t *)mSequenceBlob); };
    uint32_t getFiringCount() { return(mFiringCount); };
    
    int32_t*  getLutMSDIS()     { return (mLutMSDIS);     };
    uint32_t* getLutMSAPW()     { return (mLutMSAPW);     };
    int32_t*  getLutMSB0APS()   { return (mLutMSB0APS);   };
    uint32_t* getLutMSBPFI()    { return (mLutMSBPFI);    };
    uint32_t* getLutMSBPFQ()    { return (mLutMSBPFQ);    };
    int32_t*  getLutB0FPX()     { return (mLutB0FPX);     };
    int32_t*  getLutB0FPY()     { return (mLutB0FPY);     };
    int32_t*  getLutB1FPX()     { return (mLutB0FPX);     }; // Note: B0 is used for B1, B2 and B3 for now
    int32_t*  getLutB1FPY()     { return (mLutB1FPY);     };
    int32_t*  getLutB2FPX()     { return (mLutB0FPX);     }; // Note: B0 is used for B1, B2 and B3 for now
    int32_t*  getLutB2FPY()     { return (mLutB2FPY);     };
    int32_t*  getLutB3FPX()     { return (mLutB0FPX);     }; // Note: B0 is used for B1, B2 and B3 for now
    int32_t*  getLutB3FPY()     { return (mLutB3FPY);     };
    uint32_t* getLutMSDBG01()   { return (mLutMSDBG01);   };
    uint32_t* getLutMSDBG23()   { return (mLutMSDBG23);   };
    uint32_t* getLutB0FTXD()    { return ((uint32_t *)mLutB0FPX);     };
    uint32_t* getLutB1FTXD()    { return ((uint32_t *)mLutB0FPX);     };
    uint32_t* getLutB2FTXD()    { return ((uint32_t *)mLutB0FPX);     };
    uint32_t* getLutB3FTXD()    { return ((uint32_t *)mLutB0FPX);     };
    uint32_t* getLutMSSTTXC()   { return (mLutMSSTTXC);   };
    uint32_t* getLutMSAFEINIT() { return (mLutMSAFEINIT); };
    uint32_t* getLutMSRTAFE()   { return (mLutMSRTAFE);   };
    uint32_t  getMSRTAFElength(){ return (mMSRTAFElength);};
    uint32_t  getMSSTTXClength(){ return (mMSSTTXClength);};
    uint32_t* getLutMSWFC()     { return (mLutMSWFC);     };
    uint32_t  getBaseConfigLength() { return(mBip.baseConfigLength); };
    uint32_t  getEnsembleSize() { return(mBeamParams.ensembleSize);};
    float     getTxPrf()        { return(mTxPrf);};
    float     getFocalDepth()   { return(mFocalPointInfo.focalDepthMm);};

private:
    void findStartStopIndices( const float inputArray[],
                               uint32_t arrayLen,
                               uint32_t thresh,
                               uint32_t numSamples,
                               uint32_t &startIndex,
                               uint32_t &stopIndex);
    
    void findStartStopIndicesInt( const int32_t inputArray[],
                                  uint32_t arrayLen,
                                  int32_t threshStart,
                                  int32_t threshStop,
                                  uint32_t &startIndex,
                                  uint32_t &stopIndex);
    
    void     convertToFloat( const int32_t inArray[], uint32_t len, uint32_t intBits, uint32_t fracBits, float outArray[]);
    void     convertToInt( const float inArray[], uint32_t len, uint32_t intBits, uint32_t fracBits, int32_t outArray[]);
    uint32_t formAfeWord( uint32_t addr, uint32_t data, uint32_t dieSelect=0);
    uint32_t formAfeGainCode(float powerDb, float g);
    TXLUT_ptr  TXLUTinfo;
    ROIBoundsStruct ROIBounds;


    // Map elements to TX die numbers
    uint32_t chip_map[128]    = {0,0,1,1,0,1,1,0,0,1,1,0,1,1,0,0, 2,2,3,3,2,3,3,2,2,3,3,2,3,3,2,2,
                                 4,4,5,5,4,5,5,4,4,5,5,4,5,5,4,4, 6,6,7,7,6,7,7,6,6,7,7,6,7,7,6,6,
                                 0,0,1,1,0,1,1,0,0,1,1,0,1,1,0,0, 2,2,3,3,2,3,3,2,2,3,3,2,3,3,2,2,
                                 4,4,5,5,4,5,5,4,4,5,5,4,5,5,4,4, 6,6,7,7,6,7,7,6,6,7,7,6,7,7,6,6};

    // Map die channels to elements
    /*
    uint32_t die_map[128]    = {0,1,4,7,8,11,1,15, 64,65,68,71,72,75,78,79,
                             2,3,5,6,9,10,12,13, 66,67,69,70,73,74,76,77,
                             16,17,20,23,24,27,30,31, 80,81,84,87,88,91,94,95,
                             18,19,21,22,25,26,28,29, 82,83,85,86,89,90,92,93,
                             32,33,36,38,39,42,44,45, 96,97,100,103,104,107,110,111,
                             34,35,37,38,41,41,44,45, 98,99,101,102,105,106,108,109,
                             48,49,52,55,56,58,59, 112,113,116,119,120,123,126,127,
                             50,51,53,54,57,58,60,61, 114,115,117,118,121,122,124,125};
    */

    uint32_t die_map[128]    = {  0,   1,   4,   7,   8,  11,  14,  15,  64,  65,  68,  71,  72,
                                  75,  78,  79,   2,   3,   5,   6,   9,  10,  12,  13,  66,  67,
                                  69,  70,  73,  74,  76,  77,  16,  17,  20,  23,  24,  27,  30,
                                  31,  80,  81,  84,  87,  88,  91,  94,  95,  18,  19,  21,  22,
                                  25,  26,  28,  29,  82,  83,  85,  86,  89,  90,  92,  93,  32,
                                  33,  36,  39,  40,  43,  46,  47,  96,  97, 100, 103, 104, 107,
                                  110, 111,  34,  35,  37,  38,  41,  42,  44,  45,  98,  99, 101,
                                  102, 105, 106, 108, 109,  48,  49,  52,  55,  56,  59,  62,  63,
                                  112, 113, 116, 119, 120, 123, 126, 127,  50,  51,  53,  54,  57,
                                  58,  60,  61, 114, 115, 117, 118, 121, 122, 124, 125};

    // Map elements to TX die channel numbers
    uint32_t channel_map[128] = {0,1,0,1,2,2,3,3,4,4,5,5,6,7,6,7,0,1,0,1,2,2,3,3,4,4,5,5,6,7,6,7,
                              0,1,0,1,2,2,3,3,4,4,5,5,6,7,6,7,0,1,0,1,2,2,3,3,4,4,5,5,6,7,6,7,
                              8,9,8,9,10,10,11,11,12,12,13,13,14,15,14,15,8,9,8,9,10,10,11,11,12,12,13,13,14,15,14,15,
                              8,9,8,9,10,10,11,11,12,12,13,13,14,15,14,15,8,9,8,9,10,10,11,11,12,12,13,13,14,15,14,15};


    // Element pointer sequence (TX beam pitch of 4 elements)
    uint32_t el_ptr_seq[32]  = {0,0,0,0,0,0,0,0,
                              4,8,12,16,20,24,28,32,36,40,44,48,52,56,60,64,
                              64,64,64,64,64,64,64,64};
    // Apod element pointer sequence (TX beam pitch of 4 elements)
    uint32_t ap_ptr_seq[32]  = {0,  0,  0,  0,  0,  0,  0,  0,  2,  4,  6,  8, 10, 12, 14, 16, 18,
                                20, 22, 24, 26, 28, 30, 32, 32, 32, 32, 32, 32, 32, 32, 32};

    /*
    // Element pointer sequence (TX beam pitch of 4 elements)
    uint32_t el_ptr_seq[32]  = {64, 64, 64, 64, 64, 64, 64, 64, 64, 60, 56, 52, 48, 44, 40, 36, 32,
                                28, 24, 20, 16, 12,  8,  4,  0,  0,  0,  0,  0,  0,  0,  0};
    // Apod element pointer sequence (TX beam pitch of 4 elements)
    uint32_t ap_ptr_seq[32]  = {32, 32, 32, 32, 32, 32, 32, 32, 32, 30, 28, 26, 24, 22, 20, 18, 16,
                                14, 12, 10,  8,  6,  4,  2,  0,  0,  0,  0,  0,  0,  0,  0};
    */
private:
    std::shared_ptr<UpsReader>  mUpsReaderSPtr;
    float      mPrfList[UpsReader::MAX_PRF_SETTINGS];
    uint32_t   mNumValidPrfs;

    BeamParams            mBeamParams;
    BeamIndependentParams mBip;
    FocalPointInfo        mFocalPointInfo;   // populated in calcualAxialRoi()
    BeamCount             mBeamCount;

    uint32_t mFiringCount;
    uint32_t mInterleaveFactor;
    uint32_t mNumGroups;
    uint32_t mCtbReadBlocks;

    uint32_t mNumTxBeamsB;
    uint32_t mNumRxBeamsB;
    uint32_t mNumTxBeamsBC;
    uint32_t mNumRxBeamsBC;
    uint32_t mNumTxBeamsC;  // computed in calculateTxLinesSpan()
    uint32_t mTxdWc;
    uint32_t mTxdWc2;
    float mApod_offset_seq[32]= {31.0,  29.0,  27.0,  25.0,  23.0,  21.0,  19.0,  17.0,  15.0,  13.0,  11.0,
                                 9.0,   7.0,   5.0,   3.0,   1.0,  -1.0,  -3.0,  -5.0,  -7.0,  -9.0, -11.0,
                                 -13.0, -15.0, -17.0, -19.0, -21.0, -23.0, -25.0, -27.0, -29.0, -31.0};

    float    mTxPri;
    float    mTxPrf;
    float    mTxAngles[128];

    int32_t  mScaleCRoiMax[TBF_LUT_B0FPX_SIZE];
    int32_t  mBeam0XCRoiMax[TBF_LUT_B0FPX_SIZE];
    int32_t  mBeam0YCRoiMax[TBF_LUT_B0FPX_SIZE];
    int32_t  mBeam1YCRoiMax[TBF_LUT_B0FPX_SIZE];
    int32_t  mBeam2YCRoiMax[TBF_LUT_B0FPX_SIZE];
    int32_t  mBeam3YCRoiMax[TBF_LUT_B0FPX_SIZE];
    float    mBeam0XCRoiMaxFloat[TBF_LUT_B0FPX_SIZE];
    float    mBeam0YCRoiMaxFloat[TBF_LUT_B0FPX_SIZE];
    float    mBeam1YCRoiMaxFloat[TBF_LUT_B0FPX_SIZE];
    float    mBeam2YCRoiMaxFloat[TBF_LUT_B0FPX_SIZE];
    float    mBeam3YCRoiMaxFloat[TBF_LUT_B0FPX_SIZE];
    
    int32_t  mFirCodeCRoiMax[TBF_LUT_MSDIS_SIZE];

    uint32_t mGainValues01CRoiMax[32];
    uint32_t mGainValues23CRoiMax[32];


    // LUTS computed in this module
    int32_t  mLutMSDIS     [ TBF_LUT_MSDIS_SIZE     ];
    uint32_t mLutMSAPW     [ TBF_LUT_MSAPW_SIZE     ];
    int32_t  mLutMSB0APS   [ TBF_LUT_MSB0APS_SIZE   ];
    uint32_t mLutMSBPFI    [ TBF_LUT_MSBPFI_SIZE    ];
    uint32_t mLutMSBPFQ    [ TBF_LUT_MSBPFQ_SIZE    ];
    int32_t  mLutB0FPX     [ TBF_LUT_B0FPX_SIZE     ];
    int32_t  mLutB1FPX     [ TBF_LUT_B1FPX_SIZE     ]; // should be the same as B0FPX
    int32_t  mLutB2FPX     [ TBF_LUT_B2FPX_SIZE     ]; // should be the same as B0FPX
    int32_t  mLutB3FPX     [ TBF_LUT_B3FPX_SIZE     ]; // should be the same as B0FPX
    int32_t  mLutB0FPY     [ TBF_LUT_B0FPY_SIZE     ];
    int32_t  mLutB1FPY     [ TBF_LUT_B1FPY_SIZE     ];
    int32_t  mLutB2FPY     [ TBF_LUT_B2FPY_SIZE     ];
    int32_t  mLutB3FPY     [ TBF_LUT_B3FPY_SIZE     ];
    uint32_t mLutMSDBG01   [ TBF_LUT_MSDBG01_SIZE   ];
    uint32_t mLutMSDBG23   [ TBF_LUT_MSDBG23_SIZE   ];
    uint32_t mLutB0FTXD    [ TBF_LUT_B0FTXD_SIZE    ]; // should be the same as B0FPX
    uint32_t mLutB1FTXD    [ TBF_LUT_B1FTXD_SIZE    ]; // should be the same as B0FPX
    uint32_t mLutB2FTXD    [ TBF_LUT_B2FTXD_SIZE    ]; // should be the same as B0FPX
    uint32_t mLutB3FTXD    [ TBF_LUT_B3FTXD_SIZE    ]; // should be the same as B0FPX
    uint32_t mLutMSSTTXC   [ TBF_LUT_MSSTTXC_SIZE   ];
    uint32_t mLutMSAFEINIT [ TBF_LUT_MSAFEINIT_SIZE ];
    uint32_t mLutMSWFC     [ TBF_LUT_MSWFC_SIZE     ];
    uint32_t mLutMSRTAFE   [ TBF_LUT_MSRTAFE_SIZE   ];
    uint32_t mMSRTAFElength;
    uint32_t mMSSTTXClength;
    uint32_t mSequenceBlob [CBLOB_SIZE_LINEAR][62];
};
