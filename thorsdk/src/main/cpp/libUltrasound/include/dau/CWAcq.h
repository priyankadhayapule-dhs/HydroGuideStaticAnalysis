//
// Copyright 2020 EchoNous Inc.
//

#pragma once

#include <stdint.h>
#include <memory>
#include <UpsReader.h>

#include <ScanDescriptor.h>
#include <DauRegisters.h>
#include <ThorError.h>

class CWAcq
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

    union BpRegBENABLE
    {
        struct bits
        {
            uint32_t txEn           : 1;
            uint32_t rxEn           : 1;
            uint32_t bfEn           : 1;
            uint32_t outEn          : 1; // 3
            uint32_t outType        : 3; // 4, 5, 6

            uint32_t gap0           : 1; // 7

            uint32_t outIqType      : 2; // 8, 9

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

public:
    CWAcq	(const std::shared_ptr<UpsReader>& upsReader);
    CWAcq	() {};
    ~CWAcq	();

    void init( const std::shared_ptr<UpsReader>& upsReader) { mUpsReaderSPtr = upsReader; };

    int32_t getGateSamples() { return (mOutputSamples); };
    uint32_t getBaseConfigLength() { return(mBaseConfigLength); };
    void buildCwSequenceBlob();
    void buildCwSequenceBlob(uint32_t cwBlob[62] );
    ThorStatus calculateBeamParams(uint32_t imagingCaseId, float gateStart, float gateAngle);
    uint32_t* getSequenceBlobPtr() { return((uint32_t *)mSequenceBlob); };
    void buildLuts(const uint32_t imagingCaseId, float gateStart, float gateAngle);

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
    float     getFocalDepth()   { return(mGateRange);};

private:
    void calculateTxConfig(uint32_t imagingCaseId, float gateStart, float gateAngle);
    void calculateAtgc(uint32_t imagingCaseId, float gateStart, float gateAngle );

    // primitives needed for computing beam parameters
    float getSfdels( uint32_t imagingCaseId, float focalDepthMm, float focalWidthMm = 0.0f);

    uint32_t formAfeWord( uint32_t addr, uint32_t data, uint32_t dieSelect=0);
    uint32_t afe_write_die( uint32_t adr, uint32_t d, bool afe0top=true, bool afe0bot=true, bool afe1top=true, bool afe1bot=true);
    uint32_t formAfeGainCode(float powerMode, float g);

private:
    std::shared_ptr<UpsReader>  mUpsReaderSPtr;
    float    mPrfList[UpsReader::MAX_PRF_SETTINGS];
    uint32_t mNumValidPrfs;
    uint32_t mOutputSamples;  // number of samples in the range gate provided by DAU
    uint32_t mTxPri;
    uint32_t mRxStart;
    uint32_t mTxrxOffset;
    float    mGateRange;
    uint32_t mBaseConfigLength;
    uint32_t mAdcSamples;
    BeamParams            mBeamParams;

    // LUTs computed in this module
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
    uint32_t mLutMSRTAFE   [ TBF_LUT_MSRTAFE_SIZE   ];
    uint32_t mMSRTAFElength;
    uint32_t mMSSTTXClength;

    uint32_t mSequenceBlob[62];
};
