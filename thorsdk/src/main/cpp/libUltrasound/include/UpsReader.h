//
// Copyright 2017 EchoNous, Inc.
//
//
#pragma once
#include <stdint.h>
#include <sqlite3.h>
#include <mutex>

#include <SpeckleReduction.h>
#include <ScanConverterParams.h>
#include <BSmoothParams.h>
#include <BSpatialFilterParams.h>
#include <BEdgeFilterParams.h>
#include <Colorflow.h>
#include <Autoecho.h>
#include <ScanDescriptor.h>
#include <TbfRegs.h>
#include <ThorConstants.h>
#include "DBWrappers.h"


class UpsReader {
public:
    enum
    {
        RELEASE_TYPE_NO_RX = -1,
        RELEASE_TYPE_ENGINEERING = 0,
        RELEASE_TYPE_CLINICAL = 1,
        RELEASE_TYPE_PRODUCTION = 2
    };
    enum
    {
        IMAGING_TYPE_NORMAL = 0,
        IMAGING_TYPE_FC = 1,
        IMAGING_TYPE_FTHI = 2,
        IMAGING_TYPE_ITHI = 3,
    };
    typedef struct
    {
        uint32_t numElements;
        float    radiusOfCurvature;
        float    apex;
        float    pitch;
        float    lensDelay;
        uint32_t skinAdjustSamples;
        float    arrayElevation;
        float    arrayAperture;
        float    elevationFocalDepth;
        float    maxVoltage;
        float    centerFrequency;
        float    bandwidthPercent;
        uint32_t positionIntBits;
        uint32_t positionFracBits;
        uint32_t elementPositionX[256];
        uint32_t elementPositionY[256];
        uint32_t fovShapeIndex;
    } TransducerInfo;

    typedef struct
    {
        float    samplingFrequency;        // 0
        float    quantizationFrequency;    // 1
        float    minRoiAxialSpan;          // 2
        float    minRoiAzimuthSpan;        // 3
        float    maxRoiAxialSpan;          // 4
        float    maxRoiAzimuthSpan;        // 5
        float    minRoiAxialStart;         // 6
        uint32_t defaultOrganIndex;        // 7
        uint32_t defaultWorkflowIndex;     // 8
        float    txQuantizationFrequency;  // 9
        uint32_t txStart;                  // 10
        uint32_t txRef;                    // 11
        float    txTimingAdjustment;       // 12
        float    rxTimingAdjustment;       // 13
        float    txClamp;                  // 14
        uint32_t maxBSamplesPerLine;       // 15
        uint32_t maxCSamplesPerLine;       // 16
        uint32_t maxInterleaveFactor;      // 17
        float    cTxFocusFraction;         // 18
        float    bNearGainFraction;        // 19
        float    bFarGainFraction;         // 20
        float    afePgblTime;              // 21
        float    afePsbTime;               // 22
        uint32_t afeInitCommands;          // 23
        uint32_t txApMode;                 // 24
        uint32_t topParams    [TBF_NUM_TOP_REGS]; // 25
        uint32_t topStopParams[TBF_NUM_TOP_REGS]; // 26
        uint32_t afeResistSelectId;        // 27
        uint32_t dummyVector;              // 28
        uint32_t firstDummyVector;         // 29
        uint32_t afeGainSelectIdB;         // 30
        uint32_t afeGainSelectIdC;         // 31
        float    afeVersion;               // 32
        float    targetFrameRate;          // 33
        float    spiOverheadUs;            // 34
        float    minPmTimeUs;              // 35
        float    pmHvClkOffsetUs;          // 36
        float    bHvClkOffsetUs;           // 37
        float    cHvClkOffsetUs;           // 38
        uint32_t protoVersion;             // 39
        uint32_t bDummyVector;             // 40
        uint32_t colorRxAperture;          // 41
        uint32_t quantFR;                  // 42
        uint32_t fs_S;                     // 43
        uint32_t dm_P;                     // 44
        uint32_t bpf_P;                    // 45
        uint32_t ctbLimit;                 // 46
        float    mPrf;                     // 47
        float    ispat3Limit;              // 48
        float    miLimit;                  // 49
        float    targetSurfaceTempC;       // 50
        float    ambientTempC;             // 51
        float    azimuthMarginFraction;    // 52
        float    axialMarginFraction;      // 53
        float    maxProbeTemperature;      // 54
        float    pwTargetFrameRate;        // 55
        float    tiLimit;                  // 56
        uint32_t afeGainSelectIdD;         // 57
    } Globals;

    typedef struct
    {
        int32_t releaseNumMajor;
        int32_t releaseNumMinor;
        int32_t releaseType;
    } VersionInfo;
    
    typedef struct
    {
        uint32_t            probeType;
        const char* const   dbName;
    } ProbeDbInfo;

public:  // Methods
    UpsReader();
    ~UpsReader();

    bool        open(const char *dbFilename);
    ThorStatus  open(const char *dbPath, uint32_t probeType);

    const char* getDbName(uint32_t probeType);

    bool     checkIntegrity();

    bool     getTransducerInfo(TransducerInfo& transducerInfo);
    void     loadTransducerInfo();
    void     logTransducerInfo();
    const TransducerInfo* getTransducerInfoPtr() { return(&mTransducerInfo); };
    void     loadGlobals();
    const Globals* getGlobalsPtr() { return(&mGlobals); };
    void     logGlobals();
    // LUTs needed to be loaded at power up:
    uint32_t getElementLocXLut            (uint32_t* lutPtr);
    uint32_t getElementLocYLut            (uint32_t* lutPtr);
    uint32_t getApodPosLut                (uint32_t* lutPtr);
    uint32_t getGainDbToLinearLut         (uint32_t* lutPtr);
    uint32_t getFractionalDelayLut        (uint32_t* lutPtr);
    uint32_t getTopParamsBlob             (uint32_t* lutPtr);
    uint32_t getTopStopParamsBlob         (uint32_t* lutPtr);

    // Methods needed to get stuff needed to build the sequence
    uint32_t getSequenceBlob              (uint32_t imagingCaseId, uint32_t* pBuf, uint32_t* numRegsPtr);
    uint32_t getPowerMgmtBlob             (uint32_t imagingCaseId, uint32_t* pBuf);
    uint32_t getPowerDownBlob             (uint32_t imagingCaseId, uint32_t* pBuf);

    uint32_t getNumImagingCases();
    uint32_t getImagingType               (uint32_t imagingCaseId);
    uint32_t getImagingOrgan              (uint32_t imagingCaseId);
    uint32_t getImagingGlobalID           (uint32_t imagingCaseId);
    uint32_t getNumTxBeamsB               (uint32_t imagingCaseId, uint32_t imagingMode = IMAGING_MODE_B);
    uint32_t getNumTxBeamsC               (uint32_t imagingCaseId);
    uint32_t getEnsembleSize              (uint32_t imagingCaseId);
    uint32_t getEnsembleSize              (uint32_t imagingCaseId, uint32_t wallFilterType);
    uint32_t getNumParallel               (uint32_t imagingCaseId);
    uint32_t getOrganGlobalId             (uint32_t organId);
    uint32_t getBDummyVector              ();

    // Methods needed to get LUTs needed whenever the image case is changed
    uint32_t getApodInterpRateLut         (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getApodizationLut            (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getApodizationScaleLut       (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getApodizationScaleCRoiMaxLut(uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getBandPassFilter1I          (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getBandPassFilter1Q          (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getCoherentCompoundGainLut   (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getCompressionLut            (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getFilterBlendingLut         (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getFocalCompensationB0XLut   (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getFocalCompensationB0YLut   (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getFocalCompensationB1XLut   (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getFocalCompensationB1YLut   (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getFocalCompensationB2XLut   (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getFocalCompensationB2YLut   (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getFocalCompensationB3XLut   (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getFocalCompensationB3YLut   (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getFocalInterpolationRateLut (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getGainDgc01Lut              (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getGainDgc23Lut              (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getTxDelayLut                (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getTxPropagationDelayB0Lut   (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getTxPropagationDelayB1Lut   (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getTxPropagationDelayB2Lut   (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getTxPropagationDelayB3Lut   (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getTxWaveformBLut            (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getAfeInitLut                (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getTxConfigLut               (uint32_t imagingCaseId, uint32_t* lutPtr);

    // Mehods needed to get LUTs assoociated with colorflow processing
    uint32_t getCFApodInterpRateLut       (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getCFApodWeightLut           (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getCFApodScaleLut            (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getCFFocalCompensationB0XLut (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getCFFocalCompensationB0YLut (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getCFFocalCompensationB1XLut (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getCFFocalCompensationB1YLut (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getCFFocalCompensationB2XLut (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getCFFocalCompensationB2YLut (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getCFFocalCompensationB3XLut (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getCFFocalCompensationB3YLut (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getCFFocalInterpRateLut      (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getCFGainDgc01Lut            (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getCFGainDgc23Lut            (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getCFTxDelayLut              (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getCFTxPropagationDelayB0Lut (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getCFTxPropagationDelayB1Lut (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getCFTxPropagationDelayB2Lut (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getCFTxPropagationDelayB3Lut (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getCFAfeInitLut              (uint32_t imagingCaseId, uint32_t* lutPtr);

    uint32_t getCFWallFilter              (uint32_t imagingCaseId, uint32_t* lutPtr);
    uint32_t getCFWallFilter              (uint32_t imagingCaseId, uint32_t wallFilterType, uint32_t* lutPtr);

    uint32_t getMLinesToAverage           (uint32_t imagingCaseId, uint32_t scrollSpeedIndex);

    // Methods needed by signal path components implemented in sw
    bool     getImagingDepthMm            (uint32_t imagingCaseId, float&    depth);
    bool     getNumSamples                (uint32_t& samples);
    bool     getNumSamplesB               (uint32_t imagingCaseId, uint32_t& samples);
    bool     getGainBreakpoints           (float&   nearGainFrac,  float&    farGainFrac);
    bool     getAutoGainParams            (uint32_t imagingCaseId,
                                           struct Autoecho::Params& params,
                                           uint8_t* noiseFramePtr);
    void     getFov                       ( uint32_t depthId, ScanDescriptor& sc, uint32_t imagingMode = IMAGING_MODE_B, uint32_t organId = 0 );
    void     getDefaultRoi                ( uint32_t organId, uint32_t depthId, ScanDescriptor& sc);
    bool     getScanConverterParams       (uint32_t imagingCaseId,
                                           struct ScanConverterParams& params,
                                           uint32_t imagingMode = IMAGING_MODE_B);
    bool     getCScanConverterParams      (uint32_t imagingCaseId,
                                           struct ScanConverterParams& params);
    bool     getBPersistenceLut           (uint32_t imagingCaseId, float (&lut)[256]);
    bool     getBSmoothParams             (uint32_t imagingCaseId, struct BSmoothParams& params);
    bool     getBSpatialFilterParams      (uint32_t imagingCaseId, struct BSpatialFilterParams& params);
    bool     getBEdgeFilterParams         (uint32_t imagingCaseId, struct BEdgeFilterParams& params);
    bool     getSCParams                  (uint32_t imagingCaseId, uint32_t &numViews, float &steeringAngle, uint32_t &crossWidth);
    bool     getSpeckleReductionParams    (uint32_t imagingCaseId,
                                           float (&paramArray)[SpeckleReduction::PARAM_ARRAY_SIZE]);
    int      getImagingMode               (uint32_t imagingCaseId);
    bool     getColorflowParams           (uint32_t imagingCaseId, Colorflow::Params &params);
    int      getVelocityMap               (uint32_t imagingCaseId, uint32_t* pColormap);
    int      getDefaultColorMapIndex      (uint32_t imagingCaseId, uint32_t& colorMapId);
    int      getColorMap                  (uint32_t colorMapId, bool isInverted, uint32_t* pColormap);
    bool     getBEThresholds              (uint32_t imagingCaseId, int& velThreshold, int& bThreshold);
    bool     getCPDParams                 (float& cpdCompressDynamicRangeDb, float& cpdCompressPivotPtIn, float& cpdCompressPivotPtOut);
    void     getPrfList                   (float prfs[], uint32_t& numValidPrfs);
    uint32_t getBaseConfigLength          (uint32_t imagingCaseId);
    bool     getHvpsValues                (uint32_t imagingCaseId, float &hvp, float& hvn);
    bool     getPwHvpsValues              (uint32_t imagingCaseId, float &hvp, float& hvn);
    bool     getCwHvpsValues              (uint32_t imagingCaseId, float &hvp, float& hvn);
    float    getSpeedOfSound              (uint32_t imagingCaseId);
    float    getColorRxPitch              (uint32_t imagingCaseId);
    uint32_t getColorRxAperture           (uint32_t imagingCaseId);
    void     getColorBpfParams            (uint32_t  imagingCaseId,
                                           float&    centerFrequency,
                                           float&    bandwidth,
                                           uint32_t& numTaps,
                                           uint32_t& bpfScale);
    void     getColorBpfLut               (uint32_t imagingCaseId, uint32_t *coeffsI, uint32_t *coeffsQ );
    bool     getExtraRX                   (uint32_t depthId, uint32_t& extraRX);
    bool     getHvOffset                  (uint32_t depthId, uint32_t& hvOffset);
    bool     getZeroDGC                   (uint32_t depthId, float& zeroDGC);
    void     getColorFocalBreakpoints     (uint32_t imagingCaseId, uint32_t& numBreakpointsC, uint32_t& numBreakpointsB);
    void     getColorGainBreakpoints      (uint32_t breakpoints[] );
    void     getFovInfoForColorAcq        (uint32_t  imagingCaseId,
                                           uint32_t& numTxBeamsBC,
                                           float&    rxPitchBC,
                                           float&    fovAzimuthSpan);
    void     getFocalInterpolationRateParams( uint32_t  imagingCaseId,
                                              int32_t   code0[],
                                              int32_t   codeCRoiMax[],
                                              uint32_t& codeCRoiMaxLength);


    void     getNumFocalBkpts(uint32_t imagingCaseId, uint32_t &numBkptsB, uint32_t &numBkptsCRoiMax);
    
    void     getFocalCompensationParams( uint32_t imagingCaseId,
                                         int32_t beam0X[],
                                         int32_t beam0Y[],
                                         int32_t beam1Y[],
                                         int32_t beam2Y[],
                                         int32_t beam3Y[],
                                         int32_t beam0XCRoiMax[],
                                         int32_t beam0YCRoiMax[],
                                         int32_t beam1YCRoiMax[],
                                         int32_t beam2YCRoiMax[],
                                         int32_t beam3YCRoiMax[] );

    void     getColorFocalCompensationParams( uint32_t imagingCaseId,
                                              int32_t beam0XCRoiMax[],
                                              uint32_t &intBits,
                                              uint32_t &fracBits );

    void     getApodizationScaleParams( uint32_t imagingCaseId,
                                        int32_t scaleBeam0[],
                                        int32_t scaleCRoiMax[]);

    void     getGainDgcParams( const uint32_t imagingCaseId,
                               uint32_t &intBits,
                               uint32_t &fracBits,
                               uint32_t gainValues01[],
                               uint32_t gainValues23[],
                               uint32_t gainValues01CRoiMax[],
                               uint32_t gainValues23CRoiMax[] );
    void     getAtgcParams( uint32_t depthIndex,
                            uint32_t &afePowerSelectAdc,
                            float    &minGainC,
                            float    &maxGainC);
                            
                            
    bool     getDepthIndex( uint32_t imagingCaseId, uint32_t& depthIndex);
    void     getDecimationRate(uint32_t depthIndex, float& decimationRate);

    void     getNumTxRxBeams( uint32_t imagingCaseId,
                              uint32_t &numTxBeamsB,
                              uint32_t &numRxBeamsB,
                              uint32_t &numTxBeamsBC,
                              uint32_t &numRxBeamsBC );

    void     getBWaveformInfo( uint32_t imagingCaseId,
                               uint32_t &txApertureElement,
                               float    &centerFrequency,
                               uint32_t &numHalfCycles );

    void     getColorWaveformInfo( uint32_t imagingCaseId,
                                   uint32_t &txApertureElement,
                                   float    &centerFrequency,
                                   uint32_t &numHalfCycles );

    void     getGainAtgcParams( uint32_t depthIndex,
                                uint32_t &afePowerSelectIDC,
                                float    &maxGainC,
                                float    &minGainC );

    void     getAfeInputResistance(uint32_t afeResistSelectId, uint32_t &bitset);
    
    void     getAfeGainModeParams(uint32_t gainSelectId, float &gainVal);

    uint32_t getEcgBlob(bool isInternal, uint32_t leadNum, uint32_t daecgId, uint32_t ecgBlob[]);
    bool     getDaGainConstants( float &gainOffset, float &gainRange );
    bool     getEcgGainConstants( bool isInternal, float &gainOffset, float &gainRange );
    uint32_t getGyroBlob(uint32_t daEcgId, uint32_t gyroBlob[]);
    bool     getAfeClksPerDaEcgSample(uint32_t daEcgId,
                                      uint32_t &afeClksPerDaSample,
                                      uint32_t &afeClksPerEcgSample);
    bool     getPsClkDivForEcg(uint32_t daEcgId, uint32_t &psClkDiv);
    bool     getEcgADCMax(uint32_t daEcgId, uint32_t &adcMax);
    uint32_t getDaCoeff(uint32_t  daId, float daCoeff[]);
    
    void     getFocus( uint32_t depthIndex, float &focus);

    // PW/CW related methods
    float    getGateSizeMm(uint32_t gateIndex);
    uint32_t getGateSizeSamples(uint32_t gateIndex);
    void     getPwWaveformInfo(uint32_t &txApertureElement,
                               float    &centerFrequency, bool isTDI);
    void     getPwWaveformInfoLinear(uint32_t organId, float pwFocus, float gateRange, uint32_t &txApertureElement, float    &centerFrequency, uint32_t &halfCycles);
    void     getPwCenterFrequencyLinear(uint32_t organId, float pwFocus, float gateRange, float &centerFrequency);
    void     getPwPrfList(float prfs[], uint32_t& numValidPrfs, bool isTDI);
    void     getCwPrfList(float prfs[], uint32_t& numValidPrfs);
    uint32_t getNumPwSamplesPerFrame(uint32_t prfIndex, bool isTDI);
    uint32_t getNumCwSamplesPerFrame(uint32_t prfIndex);
    uint32_t getAfeClksPerPwSample(uint32_t prfIndex, bool isTDI);
    uint32_t getCwClksPerCwSample(uint32_t prfIndex);
    bool     getMaxGainDbToLinearLUT(float &maxGain);
    uint32_t getAfeClksPerPwFft(uint32_t prfIndex, bool isTDI);
    uint32_t getNumSamplesFft(uint32_t prfIndex, bool isTDI);
    bool     getFftWindow(uint32_t prfIndex, uint32_t &fftWindowSize, float fftWindowCoeff[], bool isTDI);
    uint32_t getPwWallFilterCoefficients(uint32_t wallFilterId, float wallFilterCoeff[], bool isTDI);//wallFilterId should come from UI
    bool     getPwFftParams(uint32_t prfIndex, uint32_t &fftSize, uint32_t &fftOutputRateAfeClks, uint32_t &fftAverageNum, uint32_t &fftWindowType, uint32_t &fftWindowSize, float fftWindowCoeff[], bool isTDI); //FftAverageNum would be calculated from sweep speed
    bool     getCwFftParams(uint32_t prfIndex, uint32_t &fftSize, uint32_t &fftOutputRateCwClks, uint32_t &fftAverageNum, uint32_t &fftWindowType, float fftWindowCoeff[]); //FftAverageNum would be calculated from sweep speed
    bool     getPwFftSizeRate(uint32_t &fftSize, uint32_t &fftOutputRateAfeClks);
    bool     getCwFftSizeRate(uint32_t &fftSize, uint32_t &fftOutputRateCwClks);
    bool     getPwFftAverageNum(uint32_t index, uint32_t &fftAverageNum, uint32_t &fftSmoothingNum);
    bool     getCwFftAverageNum(uint32_t index, uint32_t &fftAverageNum, uint32_t &fftSmoothingNum);
    void     getPwBpfLut(uint32_t imagingCaseId, uint32_t &oscale, uint32_t *coeffsI, uint32_t *coeffsQ, bool isTDI);
    void     getPwBpfLutLinear(uint32_t organId, float GateRange, uint32_t &oscale, uint32_t *coeffsI, uint32_t *coeffsQ);
    bool     getCwFftWindow(uint32_t prfIndex, uint32_t &fftWindowSize, float fftWindowCoeff[]);
    uint32_t getCwTransmitAperture(uint32_t utpId);
    float    getCwFocusOffset();
    uint32_t getCwDownsampleCoeffs(uint32_t prfIndex, float downsampleCoeff[]);
    bool     getCwDecFactorSamples(uint32_t prfIndex, uint32_t &decimationFactor,  uint32_t &samples);
    bool     getPwGateSamples(uint32_t gateIndex, uint32_t &gateSamples);
    bool     getPwGateRange(uint32_t gateIndex, float &gateRange);
    bool     getPWSteeringAngle(uint32_t steeringAngleIndex, float& steeringAngleDegrees);
    bool     getPwBaselineShiftFraction(uint32_t baselineShiftIndex, float &baselineShiftFraction);
    bool     getCwBaselineShiftFraction(uint32_t baselineShiftIndex, float &baselineShiftFraction);
    uint32_t getPwHilbertCoefficients(uint32_t filterIndex, float hilbertFilterCoeffs[]);
    bool     getPwAudioHilbertLpfParams(uint32_t filterIndex, uint32_t &audioLpfTap, float &audioLpfCutoff);
    uint32_t getPwAudioHilbertLpfCoeffs(uint32_t filterIndex, float filterCoeffs[]);
    bool     getCWPeakThresholdParams(uint32_t organId, float &refGainDbCW, float &peakThresholdCW);
    bool     getPWPeakThresholdParams(uint32_t organId, float &refGainDbPW, float &peakThresholdPW);
    bool     getCWGainParams(uint32_t organId, float &defaultGainCW, float &defaultGainRangeCW);
    bool     getCWGainAudioParams(uint32_t organId, float &defaultGainAudioCW, float &defaultGainAudioRangeCW);
    bool     getPWGainParams(uint32_t organId, float &defaultGainPW, float &defaultGainRangePW, bool isTDI);
    bool     getPWGainAudioParams(uint32_t organId, float &defaultGainAudioPW, float &defaultGainAudioRangePW);
    uint32_t getDopplerCompressionCoeff(uint32_t compressionId, float compressionCoeff[]);

    bool     getTint(float &r, float &g, float &b);

    // AP&I related methods
    uint32_t getApiTableSize();
    bool     getUtpDataString(uint32_t id, char str[]);
    uint32_t getTxApertureElement(uint32_t utpId );
    bool     getUtpIds(uint32_t imagingCaseId, uint32_t imagingMode, int32_t utpIds[3], float colorFocalDepth = 0.0f, uint32_t halfcycle=0, bool isTDI=false);
    bool     getUtpIdsPW(uint32_t imagingCaseId, uint32_t imagingMode, int32_t utpIds[3], float colorFocalDepth = 0.0f, uint32_t halfcycle=0, bool isTDI=false, uint32_t organId=0, float gateRange=0.0);
    bool     getBLineDensity( uint32_t imagingCaseId, float &ld );
    bool     getBLineDensityLinear( uint32_t imagingCaseId, float &ld );
    bool     getColorLineDensity( uint32_t imagingCaseId, float &ld );
    bool     getColorLineDensityLinear( uint32_t imagingCaseId, float &ld );
    bool     getMLineDensity( uint32_t imagingCaseId, float &ld );
    bool     getMLineDensityLinear( uint32_t imagingCaseId, float &ld );
    int32_t  getColorFocalIndex( uint32_t imagingCaseId, float focalDepthMm);
    int32_t  getPWCycleFocalIndex(uint32_t imagingCaseId, float focalDepthMm, uint32_t halfcycle, bool isTDI);
    int32_t  getPWCycleFocalIndexLinear(uint32_t imagingCaseId, int32_t pwUtpIdList[100], float focalDepthMm, uint32_t halfcycle, bool isTDI);
    int32_t  getCwUtpIndex(uint32_t imagingCaseId, float focalDepthMm);

    void     close();

    void     clearErrorCount()  { mErrorCount = 0; }
    uint32_t getErrorCount()    { return(mErrorCount); }
    
    // Test parameters
    bool getReverbTestParams( float& lowLimit,
			      float& highLimit,
			      uint32_t txWaveform[16],
			      uint32_t& hvps1,
			      uint32_t& hvps2,
			      uint32_t& txrxt,
			      uint32_t& afetr,
			      uint32_t& samples );
    
    // Debug methods
    char*    getVersion();
    bool     getVersionInfo( VersionInfo& versionInfo );

public: // members
    static const uint32_t MAX_SEQUENCE_BLOB_SIZE = 64*1024;
    static const uint32_t MAX_PRF_SETTINGS = 32;
    static const uint32_t MAX_ECG_BLOB_SIZE = 64;  // # of words
    static const uint32_t MAX_GYRO_BLOB_SIZE = 64;   // # of words
    static const uint32_t MAX_UTPS_IN_UPS = 500;

    static const char* const DEFAULT_UPS_FILE;

    static const ProbeDbInfo   ProbeInfoArray[];

private: // Methods
    bool     openInternal(const char *dbFilename);
    uint32_t getGlobalLut (const char* queryString, uint32_t maxSize, uint32_t* lutPtr);
    uint32_t getImagingCaseLut(const char* idQuesryString,
                               const char* lutQueryString,
                               uint32_t imagingCaseId,
                               uint32_t maxSize,
                               uint32_t* lutPtr);

    size_t getImagingCaseLut_v2(const char *fkey, const char *ftable, const char *colName, uint32_t imagingCaseId, void *lut, size_t size);

private: // Properties
    uint32_t             mErrorCount;
    TransducerInfo       mTransducerInfo;
    Globals              mGlobals;
    Sqlite3              mDb_ptr;
    std::string          mDbFilename;
    std::recursive_mutex mLock;
    int                  mReleaseType;
    char                 mVersionString[80];  // TODO: cleanup
};
