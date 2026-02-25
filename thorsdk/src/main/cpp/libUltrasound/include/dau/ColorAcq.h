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

class ColorAcq
{
public:
    typedef struct
    {
        uint32_t bfS;
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
    
    ColorAcq(const std::shared_ptr<UpsReader>& upsReader);
    ColorAcq() {};
    ~ColorAcq();

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
    void calculateTxLinesSpan( const uint32_t       imagingCaseId,
                               const uint32_t       requestedPrfIndex,
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

    void calculateDgcInterpLut( const uint32_t imagingCaseId,
                                const uint32_t bfS,
                                const uint32_t dmStart);
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
                      uint32_t    bpRegs[62]);

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
    uint32_t mSequenceBlob [512][62];  // TODO: define constants for the sizes
};
