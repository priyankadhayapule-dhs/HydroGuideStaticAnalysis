//
// Copyright 2018 EchoNous Inc.
//

#define LOG_TAG "DauColorAcq"

#include <cstring>
#include <algorithm>
#include <ThorUtils.h>
#include <math.h>
#include <ColorAcq.h>
#include <fstream>

typedef struct
{
    uint32_t outerBasePtr; // CF0-L or CF3-L
    uint32_t innerBasePtr; // CF0-H or CF3-H
    uint32_t outerInc;
    uint32_t innerInc;
    uint32_t outerCount;
    uint32_t innerCount;
} CtbLoopParam;

    
typedef struct
{
    float      txAngle;
    uint32_t   txSpiPtr;
    uint32_t   txPingIndex;
} BeamDependentParams;

typedef struct
{
    float prf;
} FirstDummyParams;

inline void rxDelay( const float eleLoc[64][2], const uint32_t els, const float txfp[64][2],
                     const float k, const float offset,
                     int32_t  delayVals[64])
{
    for (uint32_t eleIndex = 0; eleIndex < els; eleIndex++)
    {
        float x = txfp[eleIndex][0];
        float y = txfp[eleIndex][1];
        float dref = sqrt( x*x + y*y );

        float xe = eleLoc[eleIndex][0];
        float ye = eleLoc[eleIndex][1];
        float rd = sqrt((xe-x)*(xe-x) + (ye-y)*(ye-y)) - dref;
        delayVals[eleIndex] = (int32_t)( round(rd * k) + offset );
    }
}

ColorAcq::ColorAcq(const std::shared_ptr<UpsReader>& upsReader) :
    mUpsReaderSPtr(upsReader)
{
    LOGD("*** ColorAcq - constructor");
    fetchGlobals();
}

ColorAcq::~ColorAcq()
{
    LOGD("*** ColorAcq -destructor");
}

//-----------------------------------------------------------------------------------------------
void ColorAcq::calculateScanConverterParams( const ScanDescriptor& roi, ScanConverterParams& scp)
{
    uint32_t numSamples = mBeamParams.outputSamples; 
    scp.numSampleMesh = 50;
    scp.numRayMesh = 100;
    scp.numSamples = numSamples;
    scp.startSampleMm = roi.axialStart;
    scp.sampleSpacingMm = roi.axialSpan/(numSamples - 1);
    scp.startRayRadian = roi.azimuthStart;
    scp.raySpacing = mBip.colorRxPitch;
    scp.numRays = mNumTxBeamsC * 4; // TODO: remove hardcoding of multiline factor of 4
}


//--------------------------------------------------------------------------------
ThorStatus ColorAcq::calculateBeamParams(const uint32_t       imagingCaseId,
                                   const uint32_t       requestedPrfIndex,
                                   const ScanDescriptor requestedRoi,
                                   uint32_t&            actualPrfIndex,
                                   ScanDescriptor&      actualRoi)
{
    ThorStatus retVal = THOR_OK;
    const UpsReader::Globals* glbPtr = mUpsReaderSPtr->getGlobalsPtr();
    const UpsReader::TransducerInfo* tinfoPtr = mUpsReaderSPtr->getTransducerInfoPtr();

    float    adcSamples;
    uint32_t numTxBeamsBC;
    float    Fs = glbPtr->samplingFrequency;
    float    imagingDepth;

    if (requestedPrfIndex >= mNumValidPrfs)
    {
        LOGE("requested PRF index (%d) exceeds max (%d)",
             requestedPrfIndex, mNumValidPrfs);
        return (TER_IE_COLORFLOW_PARAM);
    }

    mUpsReaderSPtr->getImagingDepthMm( imagingCaseId, imagingDepth );
    mUpsReaderSPtr->getDepthIndex( imagingCaseId, mBip.depthIndex );
    mBip.imagingDepth = imagingDepth;

    actualRoi = requestedRoi;

    // adjust requestedRoi axialStart, when axialStart is near the limit
    {
        float minRoiASos = glbPtr->maxCSamplesPerLine * glbPtr->minRoiAxialSpan;
        uint32_t minRoioutputSamples = (uint32_t)(ceil(minRoiASos/16.0)*16);
        float minRoiAxialSpanMm = imagingDepth*minRoioutputSamples/glbPtr->maxCSamplesPerLine;
        float extraSamplesMm = 4.0f * mBip.speedOfSound / glbPtr->samplingFrequency;        // 8 samples
        if ((actualRoi.axialStart + minRoiAxialSpanMm + extraSamplesMm) > imagingDepth)
            actualRoi.axialStart = imagingDepth - minRoiAxialSpanMm - extraSamplesMm;
    }

    // initial estimation of mBeamParams.dmStart
    {
        float fs  = glbPtr->samplingFrequency;
        float axs = actualRoi.axialStart;
        float s   = mBip.speedOfSound;

        mBeamParams.dmStart = (uint32_t)(ceil(fs * 2 * axs/s));
        mBeamParams.dmStart = mBeamParams.dmStart < 0 ? 0 : mBeamParams.dmStart;
    }
    
    // compute mBeamParams.outputSamples
    {
        float os = glbPtr->maxCSamplesPerLine * requestedRoi.axialSpan / imagingDepth;
        if (os > MAX_COLOR_SAMPLES_PER_LINE)
            os = MAX_COLOR_SAMPLES_PER_LINE;
        mBeamParams.outputSamples = (uint32_t)(ceil(os/16.0)*16);   // make it a multiple of 16
        actualRoi.axialSpan = imagingDepth*mBeamParams.outputSamples/glbPtr->maxCSamplesPerLine;
        /*
         * Patch taken from THSW-2628, Roi axial span adjust
         * */
        while ((actualRoi.axialSpan+actualRoi.axialStart) > (imagingDepth)) {
            mBeamParams.outputSamples = mBeamParams.outputSamples - 16;
            actualRoi.axialSpan = imagingDepth * mBeamParams.outputSamples / glbPtr->maxCSamplesPerLine;
        }
        uint32_t globalId = mUpsReaderSPtr->getImagingGlobalID(imagingCaseId);
        float maxRoiAxialSpan = glbPtr->maxRoiAxialSpan;
        if (globalId == 10)
        {
            maxRoiAxialSpan = 0.4;
        }
        while ((actualRoi.axialSpan) > (imagingDepth * (maxRoiAxialSpan))) {
            mBeamParams.outputSamples = mBeamParams.outputSamples - 16;
            actualRoi.axialSpan = imagingDepth * mBeamParams.outputSamples / glbPtr->maxCSamplesPerLine;
        }
     }

    // compute decimation factor
    {
        adcSamples = actualRoi.axialSpan * glbPtr->samplingFrequency * 2/mBip.speedOfSound;
        mBeamParams.decimationFactor = (floor(8 * adcSamples/512)/8)*(512.0/mBeamParams.outputSamples);
        mBeamParams.decimationFactor = floor(mBeamParams.decimationFactor*8)/8;
        if (mBeamParams.decimationFactor > 32)
        {
            float os;
            mBeamParams.decimationFactor = 32;
            os = (floor(8.0*adcSamples/512)/8)*512/32;
            if (os > MAX_COLOR_SAMPLES_PER_LINE)
                os = MAX_COLOR_SAMPLES_PER_LINE;
            mBeamParams.outputSamples = (uint32_t)(ceil(os/16)*16);
        }
    }

    // compute bfS (BF_S)
    {
        mBeamParams.bfS = mBeamParams.outputSamples * mBeamParams.decimationFactor + mBeamParams.bpfTaps;

        uint32_t bkpt = 5488;
        uint32_t bfs = mBeamParams.bfS;

        if (bfs < bkpt)
        {
            if ( (bfs & 0xf) != 0 )
            {
                bfs = (bfs & 0xfffffff0) + 16;
            }
        }
        else
        {
            uint32_t val  = bkpt + (((bfs - bkpt) >> 8) << 8);
            uint32_t diff = bfs - val;

            if      (diff > 64) bfs = val + 256;
            else if (diff > 32) bfs = val + 64;
            else if (diff > 16) bfs = val + 32;
            else                bfs = val + 16;
        }
        mBeamParams.bfS = bfs;
    }

    // calc_Axial_Roi to update dmStart and actualRoi.axialStart, and bfS 
    {
        calculateAxialRoi( imagingCaseId, mBeamParams.bfS, mBeamParams.dmStart, actualRoi );
        mBeamParams.fpS = mFocalPointInfo.numBkptsC;
    }

    calculateTxLinesSpan( imagingCaseId, requestedPrfIndex, requestedRoi, actualPrfIndex, actualRoi ); // actual azimuthSpan gets updated here

    // Calculate numGainBreakpoint aka G_S, tgcirLow, tgcirHigh
    {
        uint32_t  breakPoints[2];
        mBeamParams.numGainBreakpoints = (uint32_t)(mBeamParams.outputSamples/16) + 1;

        if (mBeamParams.numGainBreakpoints < 5)
        {
            mBeamParams.numGainBreakpoints = (uint32_t)(mBeamParams.outputSamples/4) + 1;
            mBeamParams.tgcirLow  = 0;
            mBeamParams.tgcirHigh = 0;
        }
        else
        {
            mUpsReaderSPtr->getColorGainBreakpoints( breakPoints );
            mBeamParams.tgcirLow  = breakPoints[0];
            mBeamParams.tgcirHigh = breakPoints[1];
        }
    }

    // Calculate cfpOutputSamples
    {
        mBeamParams.cfpOutputSamples = (uint32_t)(1.0 * mBeamParams.outputSamples/(2*(uint32_t)(mBeamParams.ensembleSize/8)));
    }

    // Calculate rxAngles[4] {t0, t1, t2, t3}
    {
        float    rxPitchBC;
        float    fovAzimuthSpan;
        
        mUpsReaderSPtr->getFovInfoForColorAcq( imagingCaseId,
                                               numTxBeamsBC,
                                               rxPitchBC,
                                               fovAzimuthSpan );
        
        mBip.numTxBeamsBC = numTxBeamsBC;
        mBip.rxPitchBC    = rxPitchBC;
        // TODO: conversion const below needs a #define
        // Cleanup: remove fovAzimuthSpan

        float pitchRad = rxPitchBC;
        mBeamParams.rxAngles[0] = -1.5*pitchRad;
        mBeamParams.rxAngles[1] = -0.5*pitchRad;
        mBeamParams.rxAngles[2] =  0.5*pitchRad;
        mBeamParams.rxAngles[3] =  1.5*pitchRad;

        // The following call must happen after the calulateTxLineSpan
        calculateGrid( numTxBeamsBC, numTxBeamsBC*4, requestedRoi, actualRoi, rxPitchBC, 4, mNumTxBeamsC, mTxAngles ); 
    }

    mBeamParams.rxAperture = mBip.colorRxAperture;

    // TX_WFL
    {
        mBeamParams.txWaveformLength = 0.5 * mBip.numHalfCycles / mBip.txCenterFrequency;
    }

    // beamScales[4]
    {
        mBeamParams.beamScales[0] = (1 << 14) & ((1<<16)-1);
        mBeamParams.beamScales[1] = (1 << 14) & ((1<<16)-1);
        mBeamParams.beamScales[2] = (1 << 14) & ((1<<16)-1);
        mBeamParams.beamScales[3] = (1 << 14) & ((1<<16)-1);
    }

    {
        mBeamParams.tgcFlat  = 0.0;
        mBeamParams.apodFlat = 1.0;
    }
    // lateralGains[4]
    {
        int32_t g = (int32_t)( (16.0/mBip.colorRxAperture) * (1<<16) );
        mBeamParams.lateralGains[0] = g;
        mBeamParams.lateralGains[1] = g;
        mBeamParams.lateralGains[2] = g;
        mBeamParams.lateralGains[3] = g;
    }

    // numTxSpiCommands
    {
        mBeamParams.numTxSpiCommands = 86;
    }

    // bpfSelect
    {
        mBeamParams.bpfSelect = 2;
    }

    // tgcPtr aka bg_ptr
    mBeamParams.tgcPtr = 28;

    {
        uint32_t txCenterOffset = (uint32_t)((glbPtr->txClamp + (0.5 * mBeamParams.txWaveformLength)) * Fs);
        uint32_t lensOffset     = (uint32_t)(tinfoPtr->lensDelay * Fs);
        uint32_t skinOffset     = tinfoPtr->skinAdjustSamples;
        uint32_t rxOffset       = txCenterOffset + lensOffset + skinOffset;
        uint32_t rxStart        = glbPtr->txStart + glbPtr->txRef + rxOffset + mBeamParams.dmStart;
        mBip.rxStart = rxStart;

        mBeamParams.hvClkEnOff = (uint32_t)(ceil(rxStart/16.0));
        mBeamParams.hvClkEnOn = (uint32_t)(ceil((rxStart+adcSamples+glbPtr->fs_S+glbPtr->bpf_P)/16.0));
    }

    mBeamParams.hvWakeupEn = 0;

    mBeamParams.fpPtr0     = 512;
    mBeamParams.fpPtr1     = 512;
    mBeamParams.txPtr0     = 512;
    mBeamParams.txPtr1     = 512;
    mBeamParams.irPtr1     = 0;
    mBeamParams.bApSptr    = 512;
    mBeamParams.bpfCount   = 1;
    mBeamParams.agcProfile = 0;
    mBeamParams.tgcFlip    = 0;
    
    {
        uint32_t bkpts = mFocalPointInfo.numBkptsB;
        mBeamParams.irPtr0 = (uint32_t)(ceil(bkpts/8.0)*8);
    }

    {
        mCtbReadBlocks              = (uint32_t)(mBeamParams.ensembleSize/8.0) * 8;
        mBip.ensDiv                 = (uint32_t)(mBeamParams.ensembleSize/8.0) * 2;
        mBip.ctbReadBlockDiv        = (uint32_t)(mCtbReadBlocks/8.0) * 2;
        mBeamParams.colorProcessingSamples = mBeamParams.outputSamples/mBip.ctbReadBlockDiv;
    }

    // compute Tx PRI (not color PRI)
    {
        float actualPrf = mPrfList[ actualPrfIndex ];
        mTxPri = Fs*1e6/(actualPrf*mInterleaveFactor);
    }

    mBeamParams.ccWe  = 1;
    mBeamParams.ccOne = 1;
    return (retVal);
}

void ColorAcq::logBeamParams()
{
    printf("Beam Params:\n");
    printf("------------\n");
    printf("\tbfS                = %d\n", mBeamParams.bfS);
    printf("\tfpS                = %d\n", mBeamParams.fpS);
    printf("\tdmStart            = %d\n", mBeamParams.dmStart);
    printf("\tccWe               = %d\n", mBeamParams.ccWe);
    printf("\tccOne              = %d\n", mBeamParams.ccOne);
    printf("\tnumGainBreakpoints = %d\n", mBeamParams.numGainBreakpoints);
    printf("\ttgcirLow           = %d\n", mBeamParams.tgcirLow);
    printf("\ttgirHigh           = %d\n", mBeamParams.tgcirHigh);
    printf("\toutputSamples      = %d\n", mBeamParams.outputSamples);
    printf("\tcfputputSamples    = %d\n", mBeamParams.cfpOutputSamples);
    printf("\trxAngles[]         = %f %f %f %f\n", mBeamParams.rxAngles[0], mBeamParams.rxAngles[1], mBeamParams.rxAngles[2], mBeamParams.rxAngles[3]);
    printf("\trxAperture         = %d\n", mBeamParams.rxAperture);
    printf("\ttxWaveformLength   = %f\n", mBeamParams.txWaveformLength);
    printf("\tbeamScales[]       = %08X %08X %08X %08X\n",
           mBeamParams.beamScales[0], mBeamParams.beamScales[1], mBeamParams.beamScales[2], mBeamParams.beamScales[3]);
    printf("\ttgcFlat            = %d\n", mBeamParams.tgcFlat);
    printf("\tapodFlat           = %d\n", mBeamParams.apodFlat);
    printf("\tlateralGains[]     = %d %d %d %d\n",
           mBeamParams.lateralGains[0], mBeamParams.lateralGains[1], mBeamParams.lateralGains[2], mBeamParams.lateralGains[3] );
    printf("\tnumTxSpiCommands   = %d\n", mBeamParams.numTxSpiCommands);
    printf("\tbpfSelect          = %d\n", mBeamParams.bpfSelect);
    printf("\ttgcPtr             = %d\n", mBeamParams.tgcPtr );
    printf("\thvClkEnOff         = %d\n", mBeamParams.hvClkEnOff );
    printf("\thvClkEnOn          = %d\n", mBeamParams.hvClkEnOn );
    printf("\thvWakeupEn         = %d\n", mBeamParams.hvWakeupEn );
    printf("\tfpPtr0, fpPtr1     = %d %d\n", mBeamParams.fpPtr0, mBeamParams.fpPtr1 );
    printf("\ttxPtr0, txPtr1     = %d %d\n", mBeamParams.txPtr0, mBeamParams.txPtr1 );
    printf("\tirPtr0, irPtr1     = %d %d\n", mBeamParams.irPtr0, mBeamParams.irPtr1 );
    printf("\tbApSptr            = %d\n", mBeamParams.bApSptr );
    printf("\tdecimationFactor   = %f\n", mBeamParams.decimationFactor);
    printf("\tbpfTaps            = %d\n", mBeamParams.bpfTaps );
    printf("\tbpfScale           = %d\n", mBeamParams.bpfScale );
    printf("\tbpfCount           = %d\n", mBeamParams.bpfCount );
    printf("\tagcProfile         = %d\n", mBeamParams.agcProfile );
    printf("\ttgcFlip            = %d\n", mBeamParams.tgcFlip );
    printf("\tensembleSize       = %d\n", mBeamParams.ensembleSize );
    printf("\tcolorProcessingSamples = %d\n", mBeamParams.colorProcessingSamples );
}


//--------------------------------------------------------------------------------
void ColorAcq::buildLuts( const uint32_t imagingCaseId, const ScanDescriptor roi )
{
    uint32_t numFocalBkptsB, numFocalBkptsC;
    
    calculateFocalInterpLut( imagingCaseId );
    calculateDgcInterpLut  ( imagingCaseId, mBeamParams.bfS, mBeamParams.dmStart );
    calculateAtgc          ( imagingCaseId, mBeamParams.bfS, mBeamParams.dmStart, mBip.rxStart );
    calculateTxConfig      ( imagingCaseId, roi, mNumTxBeamsC, mTxAngles );
    calculateBpfLut        ( imagingCaseId );
}


//--------------------------------------------------------------------------------
void ColorAcq::fetchGlobals()
{
    mUpsReaderSPtr->getPrfList(mPrfList, mNumValidPrfs);
}

//--------------------------------------------------------------------------------
void ColorAcq::getImagingCaseConstants(uint32_t imagingCaseId, uint32_t wallFilterType )
{
    mBip.speedOfSound   = mUpsReaderSPtr->getSpeedOfSound(imagingCaseId);
    mBeamParams.ensembleSize = mUpsReaderSPtr->getEnsembleSize(imagingCaseId, wallFilterType);

    mBip.colorRxPitch     = mUpsReaderSPtr->getColorRxPitch    (imagingCaseId);
    mBip.colorRxAperture  = mUpsReaderSPtr->getColorRxAperture (imagingCaseId);

    mUpsReaderSPtr->getColorBpfParams( imagingCaseId,
                                       mBip.bpfCenterFrequency,
                                       mBip.bpfBandwidth,
                                       mBeamParams.bpfTaps,
                                       mBeamParams.bpfScale );
    mUpsReaderSPtr->getColorWaveformInfo( imagingCaseId,
                                          mBip.numTxElements,
                                          mBip.txCenterFrequency,
                                          mBip.numHalfCycles );
    mUpsReaderSPtr->getNumTxRxBeams( imagingCaseId,
                                     mNumTxBeamsB,
                                     mNumRxBeamsB,
                                     mNumTxBeamsBC,
                                     mNumRxBeamsBC );
}

void ColorAcq::logImagingCaseConstants()
{
    printf("mBip.speedOfSound          = %f\n",   mBip.speedOfSound);

    printf("mBip.baseConfigLength      = %d\n",   mBip.baseConfigLength);
    printf("mBip.colorRxPitch          = %f\n\n", mBip.colorRxPitch);

    printf("mBip.bpfCenterFrequency    = %f\n",   mBip.bpfCenterFrequency);
    printf("mBip.bpfBandwidth          = %f\n",   mBip.bpfBandwidth);

    printf("mBip.numTxElements         = %d\n",   mBip.numTxElements);
    printf("mBip.txCenterFrequency     = %f\n",   mBip.txCenterFrequency);
    printf("mBip.numHalfCycles         = %d\n\n", mBip.numHalfCycles);
    printf("mBip.rxStart               = %d\n",   mBip.rxStart);

    printf("mNumTxBeamsB               = %d\n",   mNumTxBeamsB );
    printf("mNumRxBeamsB               = %d\n",   mNumRxBeamsB );
    printf("mNumTxBeamsBC              = %d\n",   mNumTxBeamsBC );
    printf("mNumRxBeamsBC              = %d\n\n", mNumRxBeamsBC);

    printf("\nSequencer loop controls\n");
    printf("\tmInterleaveFactor        = %d\n", mInterleaveFactor);
    printf("\tmNumGroups               = %d\n", mNumGroups);
    printf("\tmCtbReadBlocks           = %d\n", mCtbReadBlocks);
           
}

void ColorAcq::calculateMaxPrfIndex( ScanDescriptor&      actualRoi,
                                     uint32_t&            maxPrfIndex )
{
    const UpsReader::Globals *glbPtr = mUpsReaderSPtr->getGlobalsPtr();
    const UpsReader::TransducerInfo *transducerInfoPtr = mUpsReaderSPtr->getTransducerInfoPtr();

    float Fs = glbPtr->samplingFrequency;
    float roundTripDistance = 2.0 * (actualRoi.axialStart + actualRoi.axialSpan);

    float maxAchievablePrf = 1.0e6 * (1.0/((roundTripDistance/mBip.speedOfSound) + 50.0));
    uint32_t prfIndex = mNumValidPrfs - 1;
    float requestedPrf = mPrfList[prfIndex];

    if (requestedPrf > maxAchievablePrf)
    {
        while (prfIndex > 0)
        {
            if (maxAchievablePrf < mPrfList[prfIndex])
            {
                prfIndex--;
            }
            else
            {
                break;
            }
        }
    }

    maxPrfIndex = prfIndex;
}

//--------------------------------------------------------------------------------
// ColorAcq::calculateTxLinesSpan()
//
// Inputs:
//    requestedPrfIndex - user selected value
//    requestedRoi      - structure describing the ROI specified by the user
//
// Outputs:
//    actualPrfIndex    - index in the PRF table corresponding to achievable
//                        PRF value
//    actualRoi         - ROI that is closest to user requested ROI.
//
// Private members needed:
//     mBip.colorRxPitch
// Private members updated:
//     mInterleaveFactor
//     mNumGroups
//     mNumTxBeamsC
// 
void ColorAcq::calculateTxLinesSpan( const uint32_t       imagingCaseId,
                                     const uint32_t       requestedPrfIndex,
                                     const ScanDescriptor requestedRoi,
                                     uint32_t&            actualPrfIndex,
                                     ScanDescriptor&      actualRoi)
{
    const UpsReader::Globals *glbPtr = mUpsReaderSPtr->getGlobalsPtr();
    const UpsReader::TransducerInfo *transducerInfoPtr = mUpsReaderSPtr->getTransducerInfoPtr();
    
    float Fs = glbPtr->samplingFrequency;
    //
    // IMPORTANT NOTE:
    // roundTripDistance calculation below assumes that actualRoi.axialStart and
    // actualRoi.axialSpan have been calculated elsewhere before this function gets
    // called.
    // 
    float roundTripDistance = 2.0 * (actualRoi.axialStart + actualRoi.axialSpan);

    float maxAchievablePrf = 1.0e6 * (1.0/((roundTripDistance/mBip.speedOfSound) + 50.0));
    float actualPrf;

    // Initialize returned values
    {
        actualPrfIndex        = requestedPrfIndex;
        actualRoi.azimuthSpan = requestedRoi.azimuthSpan;
        mInterleaveFactor     = 1;
        mNumTxBeamsC          = (uint32_t)(requestedRoi.azimuthSpan/(mBip.colorRxPitch*4));
    }
    
    float requestedPrf = mPrfList[requestedPrfIndex];
    float prf = (requestedPrf <= maxAchievablePrf) ? requestedPrf : maxAchievablePrf;

    // Pick a PRF that is the highest available less than or equal to actualPrf
    if (requestedPrf > maxAchievablePrf)
    {
        uint32_t prfIndex = mNumValidPrfs - 1;
        while (prfIndex > 0)
        {
            if (prf < mPrfList[prfIndex])
            {
                prfIndex--;
            }
            else
            {
                break;
            }
            
        }
        actualPrf = mPrfList[prfIndex];
        actualPrfIndex = prfIndex;
    }
    else
    {
        actualPrfIndex = requestedPrfIndex;
        actualPrf = mPrfList[requestedPrfIndex];
    }

    // Calculate interleave factor
    {
        float f_if = maxAchievablePrf/actualPrf;
        uint32_t fpgaLimit = (uint32_t)( 0.25 * glbPtr->ctbLimit / (mBeamParams.outputSamples * mBeamParams.ensembleSize));
        uint32_t maxInterleaveFactor = (glbPtr->maxInterleaveFactor > fpgaLimit) ? fpgaLimit : glbPtr->maxInterleaveFactor;
        if (f_if < (float)maxInterleaveFactor)
        {
            mInterleaveFactor = (uint32_t)f_if;
        }
        else
        {
            mInterleaveFactor = maxInterleaveFactor;
        }
    }
    //float pricomp = Fs*1.0e6/(actualPrf*mInterleaveFactor)+453;
    //uint32_t tempInterleaveFactor = ((uint32_t)(Fs*1.0e6/pricomp)/actualPrf) < 1 ? 1: (uint32_t)(Fs*1.0e6/pricomp)/actualPrf;
    //pricomp = ceil(Fs*1.0e6/(actualPrf*tempInterleaveFactor));
    //mInterleaveFactor = ((uint32_t)(Fs*1.0e6/pricomp)/actualPrf) < 1 ? 1: (uint32_t)(Fs*1.0e6/pricomp)/actualPrf;
    if (mInterleaveFactor == 3)
        mInterleaveFactor = 2;
    // Calculate numTxLines, number of groups and actual azimuth span
    {
        mNumTxBeamsC = (uint32_t)(mInterleaveFactor * (uint32_t)ceil((float)mNumTxBeamsC/mInterleaveFactor));
        actualRoi.azimuthSpan = mBip.colorRxPitch*4*mNumTxBeamsC;
        mNumGroups = (uint32_t)(1.0*mNumTxBeamsC/mInterleaveFactor);
    }
    ScanDescriptor fieldOfView;
    mUpsReaderSPtr->getFov(mBip.depthIndex, fieldOfView, IMAGING_MODE_COLOR);
    float fov = fieldOfView.azimuthSpan;
    uint32_t globalId = mUpsReaderSPtr->getImagingGlobalID(imagingCaseId);
    float maxRoiAzimConstraint = glbPtr->maxRoiAzimuthSpan;
    if (globalId == 10)
    {
        maxRoiAzimConstraint = 0.3;
    }
    if (actualRoi.azimuthSpan < fov*glbPtr->minRoiAzimuthSpan)
    {
        mNumTxBeamsC          = (uint32_t)(requestedRoi.azimuthSpan/(mBip.colorRxPitch*4)) + mInterleaveFactor;
        mNumTxBeamsC = (uint32_t)(mInterleaveFactor * (uint32_t)ceil((float)mNumTxBeamsC/mInterleaveFactor));
        actualRoi.azimuthSpan = mBip.colorRxPitch*4*mNumTxBeamsC;
        mNumGroups = (uint32_t)(1.0*mNumTxBeamsC/mInterleaveFactor);
        /*
        mNumTxBeamsC = mNumTxBeamsC + 1;
        actualRoi.azimuthSpan = mBip.colorRxPitch*4*mNumTxBeamsC;
        mNumGroups = (uint32_t)(1.0*mNumTxBeamsC/mInterleaveFactor);
         */
    }

    if (actualRoi.azimuthSpan > fov*maxRoiAzimConstraint)
    {
        mNumTxBeamsC          = (uint32_t)(requestedRoi.azimuthSpan/(mBip.colorRxPitch*4)) - mInterleaveFactor;
        mNumTxBeamsC = (uint32_t)(mInterleaveFactor * (uint32_t)ceil((float)mNumTxBeamsC/mInterleaveFactor));
        actualRoi.azimuthSpan = mBip.colorRxPitch*4*mNumTxBeamsC;
        mNumGroups = (uint32_t)(1.0*mNumTxBeamsC/mInterleaveFactor);
        /*
        mNumTxBeamsC = mNumTxBeamsC - 1;
        actualRoi.azimuthSpan = mBip.colorRxPitch*4*mNumTxBeamsC;
        mNumGroups = (uint32_t)(1.0*mNumTxBeamsC/mInterleaveFactor);
         */
    }
    mBip.actualPrfIndex = actualPrfIndex;
}

//--------------------------------------------------------------------------------
void ColorAcq::calculateGrid( uint32_t numTxBeams,
                              uint32_t numRxBeams,
                              const ScanDescriptor requestedRoi,
                              ScanDescriptor& actualRoi,
                              float pitch,
                              uint32_t NP,
                              uint32_t numTxBeamsC,
                              float txAngles[])
{
    float rxStep = pitch;
    float txStep = rxStep * NP;

    float txa[256];  // worst case for linear
    float rxa[1024];

    float startTxAngle = -(float)(numTxBeams - 1)*txStep*0.5;
    float startRxAngle = -(float)(numRxBeams - 1)*rxStep*0.5;

    if (numTxBeamsC >= numTxBeams)
    {
        for (uint32_t i = 0; i < numTxBeams; i++)
        {
            txAngles[i] = startTxAngle + i*txStep;
        }
        actualRoi.azimuthStart = startRxAngle;
    }
    else
    {
        float minDiff = 1.0e9;
        uint32_t minIdx = 0;

        for (uint32_t i = 0; i < numTxBeams; i++)
        {
            txa[i] = startTxAngle + i*txStep;
        }
        
        for (uint32_t i = 0; i < numRxBeams; i++)
        {
            rxa[i] = startRxAngle + i*rxStep;
            float absDiff = abs(rxa[i] -requestedRoi.azimuthStart);

            minIdx  = absDiff < minDiff ? i : minIdx;
            minDiff = absDiff < minDiff ? absDiff : minDiff;
        }
        minIdx >>= 2;
        for (uint32_t i = 0; i < numTxBeamsC; i++)
        {
            txAngles[i] = txa[i+minIdx];
        }
        minIdx <<= 2;
        actualRoi.azimuthStart = rxa[minIdx];
        ScanDescriptor fieldOfView;
        mUpsReaderSPtr->getFov(mBip.depthIndex, fieldOfView, IMAGING_MODE_COLOR);
        float fov = fieldOfView.azimuthSpan;
        while (actualRoi.azimuthStart+actualRoi.azimuthSpan > 0.95*abs(fov/2))
        {
            minIdx = minIdx-1;
            actualRoi.azimuthStart = rxa[minIdx];
        }
        minIdx >>= 2;
        for (uint32_t i = 0; i < numTxBeamsC; i++)
        {
            txAngles[i] = txa[i+minIdx];
        }
    }
}


//--------------------------------------------------------------------------------
void ColorAcq::buildSequenceBlob( uint32_t imagingCaseId )
{
    buildSequenceBlob( imagingCaseId, mSequenceBlob );
}

//--------------------------------------------------------------------------------
void ColorAcq::buildSequenceBlob( uint32_t imagingCaseId, uint32_t colorBlob[512][62] )
{
    UNUSED(imagingCaseId);
    float txAngle;
    
    mFiringCount = 0;
    const UpsReader::Globals* glbPtr = mUpsReaderSPtr->getGlobalsPtr();
    EnableFlags flags;
    
    // Initialize enable flags
    flags.txEn   = false;
    flags.rxEn   = false;
    flags.bfEn   = false;
    flags.ctbwEn = false;
    flags.cfpEn  = false;
    flags.outEn  = false;
    flags.pmLine = false;
    flags.pmAfePwrSb = false;
    flags.bpSlopeEn  = false;
    flags.ceof       = false;
    flags.eos        = false;

    
    uint32_t pri;   // TODO: consider making this a beam independent parameter
    {
        float prf = mPrfList[mBip.actualPrfIndex]; 
        float fs = glbPtr->samplingFrequency;
        pri = (uint32_t)(fs*1.0e6/(prf * mInterleaveFactor));  // TODO: log this value
    }

    uint32_t txPingIndex;
    uint32_t txdSpiPtr;
    uint32_t txdWordCount;
    uint32_t bgPtr;
    CtbParams ctbInput;
    CtbParams ctbOutput;
    
    initCtbParams( ctbInput, ctbOutput );
    txdWordCount = 86;
    for (uint32_t gi = 0; gi < mNumGroups; gi++)
    {
        if (gi == 0)
        {
            // Dummies at the beginning of color sequence
            flags.txEn = true; flags.pmAfePwrSb = true;
            flags.rxEn = false; flags.bfEn = false; flags.cfpEn = false; flags.outEn = false; flags.bpSlopeEn = false;
            txPingIndex = 0;
            bgPtr = 0;
            // First DUMMY After B BEGIN
            for (uint32_t di = 0; di < glbPtr->firstDummyVector; di++)  // loop A
            {
                // 
                setupTxAngle( txAngle, gi, 0, 0 );
                setupTxdSpiPtr( txdSpiPtr, gi, 0, 0 );

                flags.pmAfePwrSb = true;

                setupCtbParams( ctbInput, ctbOutput, gi, 0, 0);
                setupBpRegs( flags, txdSpiPtr, txdWordCount, txAngle, ctbInput, ctbOutput, txPingIndex, pri, bgPtr, colorBlob[mFiringCount] );
                mFiringCount++;

            }
        
            // add a set of dummies per group
            // Other Dummy BEGIN
            for (uint32_t ii=1; ii < mInterleaveFactor; ii++)  // loop B
            {
                //
                setupTxAngle( txAngle, gi, 0, ii );
                setupTxdSpiPtr( txdSpiPtr, gi, 0, ii );

                setupCtbParams( ctbInput, ctbOutput, gi, 0, 0);
                setupBpRegs( flags, txdSpiPtr, txdWordCount, txAngle, ctbInput, ctbOutput, txPingIndex, pri, bgPtr, colorBlob[mFiringCount] );
                mFiringCount++;
            }

            //  Actual ENS BEGIN, First group, no color processing/output
            bgPtr = ((mNumTxBeamsBC >> 1) > 28) ? (mNumTxBeamsBC >> 1) : 28;
            flags.bpSlopeEn = true;
            flags.txEn = true; flags.rxEn = true; flags.bfEn = true; flags.ctbwEn = true;
            flags.pmAfePwrSb = false;
            for (uint32_t ei = 0; ei < mBeamParams.ensembleSize; ei++)  // loop C
            {
                //
                for (uint32_t ii = 0; ii < mInterleaveFactor; ii++)
                {
                    //
                    setupTxPingIndex( txPingIndex, gi, ei, ii );
                    setupTxAngle( txAngle, gi, ei, ii );
                    setupTxdSpiPtr( txdSpiPtr, gi, ei, ii );
                    setupCtbParams( ctbInput, ctbOutput, gi, ei, ii);
                    ctbOutput.innerPtr = 0;
                    setupBpRegs( flags, txdSpiPtr, txdWordCount, txAngle, ctbInput, ctbOutput, txPingIndex, pri, bgPtr, colorBlob[mFiringCount] );
                    mFiringCount++;
                }
            }
        }
        else
        {
            // add a set of dummies per group
            flags.txEn = true; flags.rxEn = false; flags.bfEn = false; flags.ctbwEn = false; flags.outEn  = false; flags.bpSlopeEn = false;
            flags.pmAfePwrSb = true;
            bgPtr = 0;
            txPingIndex = 0;
            for (uint32_t ii=0; ii < mInterleaveFactor; ii++)  // loop D
            {
                //
                setupTxAngle( txAngle, gi, 0, ii );
                setupTxdSpiPtr( txdSpiPtr, gi, 0, ii );
                setupCtbParams( ctbInput, ctbOutput, 0, 0, 0);
                setupBpRegs( flags, txdSpiPtr, txdWordCount, txAngle, ctbInput, ctbOutput, txPingIndex, pri, bgPtr, colorBlob[mFiringCount] );
                mFiringCount++;

            }

            flags.rxEn = true; flags.bfEn = true; flags.ctbwEn = true; flags.outEn = true; flags.cfpEn = true; flags.bpSlopeEn = true;
            flags.pmAfePwrSb = false;
            bgPtr = ((mNumTxBeamsBC >> 1) > 28) ? (mNumTxBeamsBC >> 1) : 28;
            for (uint32_t ei = 0; ei < mCtbReadBlocks; ei++)  // loop E
            {
                //
                for (uint32_t ii = 0; ii < mInterleaveFactor; ii++)
                {
                    //
                    setupTxPingIndex( txPingIndex, gi, ei, ii );
                    setupTxAngle( txAngle, gi, ei, ii );
                    setupTxdSpiPtr( txdSpiPtr, gi, ei, ii );
                    setupCtbParams( ctbInput, ctbOutput, gi, ei, ii);

                    setupBpRegs( flags, txdSpiPtr, txdWordCount, txAngle, ctbInput, ctbOutput, txPingIndex, pri, bgPtr, colorBlob[mFiringCount] );
                    mFiringCount++;
                }
            }
            flags.cfpEn = false; flags.outEn = false;
            for (uint32_t ei = mCtbReadBlocks; ei < mBeamParams.ensembleSize; ei++)  // loop F
            {
                //
                for (uint32_t ii=0; ii < mInterleaveFactor; ii++)
                {
                    //
                    setupTxPingIndex( txPingIndex, gi, ei, ii );
                    setupTxAngle( txAngle, gi, ei, ii );
                    setupTxdSpiPtr( txdSpiPtr, gi, ei, ii );
                    setupCtbParams( ctbInput, ctbOutput, gi, ei, ii);

                    setupBpRegs( flags, txdSpiPtr, txdWordCount, txAngle, ctbInput, ctbOutput, txPingIndex, pri, bgPtr, colorBlob[mFiringCount] );
                    mFiringCount++;

                }
            }
        }
    }

    // Flush out CTB pipeline
    flags.txEn = false; flags.rxEn = false; flags.bfEn = false; flags.ctbwEn = false; flags.cfpEn = true; flags.outEn = true;
    flags.pmAfePwrSb = true;

    for (uint32_t ei = 0; ei < mCtbReadBlocks; ei++)  // loop FLUSH
    {
        //
        for (uint32_t ii=0; ii < mInterleaveFactor; ii++)
        {
            //
            if ((ei*mInterleaveFactor + ii) == (mCtbReadBlocks*mInterleaveFactor - 2))
            {
                flags.eos = true;
                flags.ceof = false;
            }
            else if ((ei*mInterleaveFactor + ii) == (mCtbReadBlocks*mInterleaveFactor - 1))
            {
                flags.eos = false;
                flags.ceof = true;
            }
            setupTxPingIndex( txPingIndex, mNumGroups-1, ei, ii );
            setupTxAngle( txAngle, mNumGroups-1, ei, ii );
            setupTxdSpiPtr( txdSpiPtr, mNumGroups-1, ei, ii );
            setupCtbParams( ctbInput, ctbOutput, mNumGroups, ei, ii);
            
            setupBpRegs( flags, txdSpiPtr, txdWordCount, txAngle, ctbInput, ctbOutput, txPingIndex, pri, bgPtr, colorBlob[mFiringCount] );
            mFiringCount++;
        }
    }
}

void ColorAcq::calculateFocalInterpLut( const uint32_t imagingCaseId )
{
    const UpsReader::Globals *glbPtr = mUpsReaderSPtr->getGlobalsPtr();
    uint32_t bfS = mBeamParams.bfS;
    float decimationRate;
    const uint32_t depthIndex = mBip.depthIndex;
    mUpsReaderSPtr->getDecimationRate( depthIndex, decimationRate );

    //------------------------------------------------------------------------------------------
    mUpsReaderSPtr->getFocalCompensationParams(imagingCaseId,
                                               mLutB0FPX, mLutB0FPY, mLutB1FPY, mLutB2FPY, mLutB3FPY,
                                               mBeam0XCRoiMax, mBeam0YCRoiMax, mBeam1YCRoiMax, mBeam2YCRoiMax, mBeam3YCRoiMax);

    uint32_t codeCRoiMaxLength;
    mUpsReaderSPtr->getFocalInterpolationRateParams(imagingCaseId, mLutMSDIS, mFirCodeCRoiMax, codeCRoiMaxLength);
    
    uint32_t numBkptsB = mFocalPointInfo.numBkptsB;
    uint32_t numBkptsC = mFocalPointInfo.numBkptsC;
    
    //------------------------------------------------------------------------------------------
    
    uint32_t idxStart = mFocalPointInfo.idxStart;
    uint32_t idxStop  = mFocalPointInfo.idxStop;

    {
        memcpy( &(mLutB0FPX[TBF_LUT_B0FPX_SIZE/2]), &(mBeam0XCRoiMax[idxStart]), sizeof(int32_t)*numBkptsC );
        memcpy( &(mLutB0FPY[TBF_LUT_B0FPY_SIZE/2]), &(mBeam0YCRoiMax[idxStart]), sizeof(int32_t)*numBkptsC );
        memcpy( &(mLutB1FPY[TBF_LUT_B1FPY_SIZE/2]), &(mBeam1YCRoiMax[idxStart]), sizeof(int32_t)*numBkptsC );
        memcpy( &(mLutB2FPY[TBF_LUT_B2FPY_SIZE/2]), &(mBeam2YCRoiMax[idxStart]), sizeof(int32_t)*numBkptsC );
        memcpy( &(mLutB3FPY[TBF_LUT_B3FPY_SIZE/2]), &(mBeam3YCRoiMax[idxStart]), sizeof(int32_t)*numBkptsC );
    }

    //------------------------------------------------------------------------------------------
    // Concatanate B-Mode and Color focal interpolation rates
    { 
        uint32_t BfocalLen = mFocalPointInfo.numBkptsB + 1;
        uint32_t bBkptsWords = uint32_t(ceil((float)(mFocalPointInfo.numBkptsB)/8.0));

        int32_t tmpColorCodes[1024];
        uint32_t ind = 0;

        // Unwrap color codes for easy access
        for (uint32_t i = 0; i < 128; i++)
        {
            uint32_t codeWord = mFirCodeCRoiMax[i];

            for (uint32_t j = 0; j < 32; j=j+4)
            {
                tmpColorCodes[ind] = (codeWord >> j) & 0xf;
                ind++;
            }
        }

        ind = bBkptsWords;
        for (uint32_t i = idxStart; i < idxStart+(codeCRoiMaxLength-bBkptsWords)*8; i=i+8)
        {
            int32_t w = 0;

            w = ( tmpColorCodes[i]   & 0xff)        |
                ((tmpColorCodes[i+1] & 0xff) << 4)  |
                ((tmpColorCodes[i+2] & 0xff) << 8)  |
                ((tmpColorCodes[i+3] & 0xff) << 12) |
                ((tmpColorCodes[i+4] & 0xff) << 16) |
                ((tmpColorCodes[i+5] & 0xff) << 20) |
                ((tmpColorCodes[i+6] & 0xff) << 24) |
                ((tmpColorCodes[i+7] & 0xff) << 28);
            mLutMSDIS[ind] = w;
            ind++;
                   
        }
    }

    
    {
        // Get B Apodization scale from UPS and combine with C Apodization scale
        mUpsReaderSPtr->getApodizationScaleParams(imagingCaseId,
                                                  mLutMSB0APS,
                                                  mScaleCRoiMax);

        const uint32_t halfLutSize = TBF_LUT_MSB0APS_SIZE/2;
        for (uint32_t i = 0; i < halfLutSize; i++)
        {
            mLutMSB0APS[halfLutSize + i] = mScaleCRoiMax[i];
        }
    }
}



void ColorAcq::calculateDgcInterpLut( const uint32_t imagingCaseId,
                                     const uint32_t bfS,
                                     const uint32_t dmStart)
{
    UNUSED(bfS);
    UNUSED(dmStart);
    
    const UpsReader::Globals *glbPtr = mUpsReaderSPtr->getGlobalsPtr();

    uint32_t numSamplesB = glbPtr->maxBSamplesPerLine;
    uint32_t numSamplesC = glbPtr->maxCSamplesPerLine;
    uint32_t intBits;
    uint32_t fracBits;

    mUpsReaderSPtr->getGainDgcParams( imagingCaseId,
                                      intBits,
                                      fracBits,
                                      mLutMSDBG01,
                                      mLutMSDBG23,
                                      mGainValues01CRoiMax,
                                      mGainValues23CRoiMax );
    
    uint32_t txBeams = mNumTxBeamsBC;
    uint32_t out32allC[4][32];

    for (int i = 0; i < 32; i++)
    {
        out32allC[0][i] = (mGainValues01CRoiMax[i]     ) & 0x1ff;  // bits 0..8
        out32allC[1][i] = (mGainValues01CRoiMax[i] >> 9) & 0x1ff;; // bits 9..17
        out32allC[2][i] = (mGainValues23CRoiMax[i]     ) & 0x1ff;
        out32allC[3][i] = (mGainValues23CRoiMax[i] >> 9) & 0x1ff;
    }
    
	// gainBkptsFull=np.arange(0,NumSamplesC+1,16)
    int32_t gainBkptsFull[512];
    for (int i = 0; i < 32; i++)
    {
        int32_t val = i*16;
        val = val > 256 ? 256 : val;
        gainBkptsFull[i] = val;
    }
    
    float Fs = glbPtr->samplingFrequency;
	int32_t s0   = (int32_t)(Fs*mBip.imagingDepth*2.0/mBip.speedOfSound);

    uint32_t startInd;
    uint32_t stopInd;
        
    findStartStopIndicesInt( gainBkptsFull, 32,
                             (int32_t)(mBeamParams.dmStart*256.0/s0),
                             (int32_t)((mBeamParams.bfS+mBeamParams.dmStart)*256/s0),
                             startInd, stopInd );

    uint32_t offset = 28*32;
    uint32_t word01;
    uint32_t word23;

    for (uint32_t i = 0; i < stopInd-startInd; i++)
    {
        word01  = out32allC[0][i + startInd];
        word01 |= out32allC[1][i + startInd] << 9;
        word23  = out32allC[2][i + startInd];
        word23 |= out32allC[3][i + startInd] << 9;

        mLutMSDBG01[offset + i] = word01;
        mLutMSDBG23[offset + i] = word23;
    }


    word01 = mLutMSDBG01[offset + (stopInd - startInd)];
    word23 = mLutMSDBG23[offset + (stopInd - startInd)];

    for (int i = stopInd-startInd; i < 32; i++)
    {
        mLutMSDBG01[offset + i] = word01;
        mLutMSDBG23[offset + i] = word23;
    }
}


//-----------------------------------------------------------------------------------------------------------------------
// This method computes values of the LUT MSRTAFE
#define MAX_AFE_GAIN_STEPS 320

void ColorAcq::calculateAtgc( const uint32_t imagingCaseId,
                              const uint32_t bfS,
                              const uint32_t dmStart,
                              const uint32_t rxStart ) {
    uint32_t index = 0;


    const UpsReader::Globals *glbPtr = mUpsReaderSPtr->getGlobalsPtr();
    const UpsReader::TransducerInfo *transducerInfoPtr = mUpsReaderSPtr->getTransducerInfoPtr();

    uint32_t c_rx_ap = mBip.colorRxAperture;

    uint32_t txApertureElement;
    float centerfreq;
    uint32_t numhalfcycle;
    mUpsReaderSPtr->getColorWaveformInfo(imagingCaseId,
                                         txApertureElement,
                                         centerfreq,
                                         numhalfcycle);

    float fs = glbPtr->samplingFrequency;
    uint32_t txStart = glbPtr->txStart;

    uint32_t powerMode;
    float maxGainC;
    float minGainC;
    mUpsReaderSPtr->getGainAtgcParams(mBip.depthIndex,
                                      powerMode,
                                      maxGainC,
                                      minGainC);
    uint32_t bitset;
    mUpsReaderSPtr->getAfeInputResistance(glbPtr->afeResistSelectId, bitset);

    uint32_t gainmode = glbPtr->afeGainSelectIdC;
    float gainval;
    mUpsReaderSPtr->getAfeGainModeParams(gainmode, gainval);

    float freqScale = centerfreq / 10.0;
    float max_atgc_depth = maxGainC / freqScale;
    uint32_t max_adc_s = (uint32_t) (2 * fs * max_atgc_depth / mBip.speedOfSound) - rxStart;

    uint32_t adc_samples = bfS + 224 + 64 - 1;  // 224 is Todd's variable, 64 is BPF preload
    uint32_t dtgc_adc_samples = max_adc_s > adc_samples ? adc_samples : max_adc_s;

    float gstart = minGainC + (dmStart * (0.5 * mBip.speedOfSound) * freqScale / fs);
    mLutMSRTAFE[index++] = formAfeWord(0, 0x10);  // Enables programming of the DTGC register map
    mLutMSRTAFE[index++] = formAfeWord(181, 0x0);  // manual mode setup
    uint32_t globalId = mUpsReaderSPtr->getImagingGlobalID(imagingCaseId);
    if (globalId == 10)
    {
        gainmode = 2;
        maxGainC = 30;
    }
    //if (((gainmode << 2) !=8) && (gstart <= (maxGainC - 0.5))) //if the gainmode is not fixed and the roi is not position below a threshold
    if ((gainmode << 2) !=8)
    {
        float gend = minGainC +
                     ((adc_samples + dmStart) * (0.5 * mBip.speedOfSound) * freqScale / fs);
        gend = gend > maxGainC ? maxGainC : gend;
        gstart = gstart > (maxGainC-0.5) ? (maxGainC-0.5) : gstart;
        uint32_t dtgc_hold_samples = adc_samples - dtgc_adc_samples;
        float grange_db = gend - (gstart > minGainC ? gstart : minGainC);

        uint32_t max_steps = MAX_AFE_GAIN_STEPS;   // maximum gain steps
        float slp_qnt = 0.125;   // gain slope quantization

        float gain_pos_slope = (grange_db / max_steps);
        float gain_pos_qnt = ceil(gain_pos_slope / slp_qnt) * slp_qnt;
        uint32_t dtgc_steps = (uint32_t) (ceil(grange_db / gain_pos_qnt));
        dtgc_steps = dtgc_steps & 0xFFFFFFFE; //clear LSB
        dtgc_steps = dtgc_steps < max_steps ? dtgc_steps : max_steps;
        uint32_t dtgc_lut_words = (uint32_t) (ceil(dtgc_steps * 0.5));

        float g_last = gstart + (dtgc_steps * gain_pos_qnt);
        int32_t start_gain = (int32_t) (4.0 * (gstart - minGainC));
        int32_t stop_gain = (int32_t) (4.0 * (g_last  - minGainC));
        int32_t pos_step = (int32_t) (round(gain_pos_qnt * 8) - 1);
        uint32_t neg_step = 255;
        uint32_t start_index = 0;
        uint32_t stop_index = dtgc_lut_words - 1;
        uint32_t start_gain_time = rxStart * 2;
        uint32_t hold_gain_time = dtgc_hold_samples * 2;

        int32_t dtgc_codes[MAX_AFE_GAIN_STEPS];
        for (int i = 0; i < MAX_AFE_GAIN_STEPS; i++)
            dtgc_codes[i] = 0;


        if (start_gain < 0)
        {
            start_gain = 0;
        }

        {
            int32_t prevVal = 0.0;
            float k = (2.0 * dtgc_adc_samples - 1) / (dtgc_steps - 1);
            for (uint32_t i = 1; i < dtgc_steps; i++) {
                int32_t curVal = (int32_t) round(i * k);
                dtgc_codes[i - 1] = (curVal - prevVal);
                prevVal = curVal;
            }
        }
        dtgc_codes[dtgc_steps-1] = 128 + dtgc_codes[dtgc_steps - 2];

        for (uint32_t i = 0; i < dtgc_lut_words; i++)
        {
            int32_t dtgc_val = dtgc_codes[i * 2] + 256 * dtgc_codes[i * 2 + 1];
            mLutMSRTAFE[index++] = formAfeWord(i + 1, dtgc_val);
        }

        mLutMSRTAFE[index++] = formAfeWord(161, stop_gain + 256 * start_gain);
        mLutMSRTAFE[index++] = formAfeWord(162, neg_step + 256 * pos_step);
        mLutMSRTAFE[index++] = formAfeWord(163, stop_index + 256 * start_index);
        mLutMSRTAFE[index++] = formAfeWord(164, start_gain_time);
        mLutMSRTAFE[index++] = formAfeWord(165, hold_gain_time);
        mLutMSRTAFE[index++] = formAfeWord(182, 0x0000);
        gainmode <<= 2;
        bitset |= 1 << 11;
        bitset |= gainmode << 12;
    }
    else
    {
        bitset |= 1 << 11;
        bitset |= 8 << 12;
        mLutMSRTAFE[index++] = formAfeWord(181, formAfeGainCode(powerMode, maxGainC));
    }
    mLutMSRTAFE[index++] = formAfeWord(182, bitset);

    c_rx_ap = (uint32_t)(ceil(c_rx_ap*0.25)*4);
    c_rx_ap = (c_rx_ap > 64) ? 64 : c_rx_ap;
    
    if (c_rx_ap == 32)
    {
        mLutMSRTAFE[index++] = formAfeWord(0, 0x0);
        mLutMSRTAFE[index++] = formAfeWord(24,0x0f0f,1);  //off 1,2,3,4 on afe0
        mLutMSRTAFE[index++] = formAfeWord(36,0x0f0f,1);  //off 5,6,7,8 on afe0
        mLutMSRTAFE[index++] = formAfeWord(48,0x0f0f,2);  //off 9,10,11,12 on afe1
        mLutMSRTAFE[index++] = formAfeWord(60,0x0f0f,2);  //off 13,14,15,16 on afe1
        mLutMSRTAFE[index++] = formAfeWord(197,0x00ff,1);  //off 1,2,3,4,5,6,7,8 on afe0
        mLutMSRTAFE[index++] = formAfeWord(197,0xff00,2);  //off 9,10,11,12,13,14,15,16 on afe1
    }
    else if (c_rx_ap == 36)
    {
        mLutMSRTAFE[index++] = formAfeWord(0, 0x0);
		mLutMSRTAFE[index++] = formAfeWord(24,0x0f0f,1);  //off 1,2,3,4 on afe0
		mLutMSRTAFE[index++] = formAfeWord(36,0x0707,1);  //off 5,6,7,8 on afe0
		mLutMSRTAFE[index++] = formAfeWord(48,0x0e0e,2);  //off 9,10,11,12 on afe1
		mLutMSRTAFE[index++] = formAfeWord(60,0x0f0f,2);  //off 13,14,15,16 on afe1
		mLutMSRTAFE[index++] = formAfeWord(197,0x007f,1);  //off 1,2,3,4,5,6,7,8 on afe0
		mLutMSRTAFE[index++] = formAfeWord(197,0xfe00,2);  //off 9,10,11,12,13,14,15,16 on afe1
    }
    else if (c_rx_ap == 40) 
    {
		mLutMSRTAFE[index++] = formAfeWord(0, 0x0);
		mLutMSRTAFE[index++] = formAfeWord(24,0x0f0f,1);  //off 1,2,3,4 on afe0
		mLutMSRTAFE[index++] = formAfeWord(36,0x0303,1);  //off 5,6,7,8 on afe0
		mLutMSRTAFE[index++] = formAfeWord(48,0x0c0c,2);  //off 9,10,11,12 on afe1
		mLutMSRTAFE[index++] = formAfeWord(60,0x0f0f,2);  //off 13,14,15,16 on afe1
		mLutMSRTAFE[index++] = formAfeWord(197,0x003f,1);  //off 1,2,3,4,5,6,7,8 on afe0
		mLutMSRTAFE[index++] = formAfeWord(197,0xfc00,2);  //off 9,10,11,12,13,14,15,16 on afe1
    }
    else if (c_rx_ap == 44) 
    {
		mLutMSRTAFE[index++] = formAfeWord(0, 0x0);
		mLutMSRTAFE[index++] = formAfeWord(24,0x0f0f,1);  //off 1,2,3,4 on afe0
		mLutMSRTAFE[index++] = formAfeWord(36,0x0101,1);  //off 5,6,7,8 on afe0
		mLutMSRTAFE[index++] = formAfeWord(48,0x0808,2);  //off 9,10,11,12 on afe1
		mLutMSRTAFE[index++] = formAfeWord(60,0x0f0f,2);  //off 13,14,15,16 on afe1
		mLutMSRTAFE[index++] = formAfeWord(197,0x001f,1);  //off 1,2,3,4,5,6,7,8 on afe0
		mLutMSRTAFE[index++] = formAfeWord(197,0xf800,2);  //off 9,10,11,12,13,14,15,16 on afe1
    }
    else if (c_rx_ap == 48) 
    {
		mLutMSRTAFE[index++] = formAfeWord(0, 0x0);
		mLutMSRTAFE[index++] = formAfeWord(24,0x0f0f,1);  //off 1,2,3,4 on afe0
		mLutMSRTAFE[index++] = formAfeWord(36,0x0000,1);  //off 5,6,7,8 on afe0
		mLutMSRTAFE[index++] = formAfeWord(48,0x0000,2);  //off 9,10,11,12 on afe1
		mLutMSRTAFE[index++] = formAfeWord(60,0x0f0f,2);  //off 13,14,15,16 on afe1
		mLutMSRTAFE[index++] = formAfeWord(197,0x000f,1);  //off 1,2,3,4,5,6,7,8 on afe0
		mLutMSRTAFE[index++] = formAfeWord(197,0xf000,2);  //off 9,10,11,12,13,14,15,16 on afe1
    }
    else if (c_rx_ap == 52) 
    {
		mLutMSRTAFE[index++] = formAfeWord(0, 0x0);
		mLutMSRTAFE[index++] = formAfeWord(24,0x0707,1);  //off 1,2,3,4 on afe0
		mLutMSRTAFE[index++] = formAfeWord(36,0x0000,1);  //off 5,6,7,8 on afe0
		mLutMSRTAFE[index++] = formAfeWord(48,0x0000,2);  //off 9,10,11,12 on afe1
		mLutMSRTAFE[index++] = formAfeWord(60,0x0e0e,2);  //off 13,14,15,16 on afe1
		mLutMSRTAFE[index++] = formAfeWord(197,0x0007,1);  //off 1,2,3,4,5,6,7,8 on afe0
		mLutMSRTAFE[index++] = formAfeWord(197,0xe000,2);  //off 9,10,11,12,13,14,15,16 on afe1
    }
    else if (c_rx_ap == 56) 
    {
		mLutMSRTAFE[index++] = formAfeWord(0, 0x0);
		mLutMSRTAFE[index++] = formAfeWord(24,0x0303,1);  //off 1,2,3,4 on afe0
		mLutMSRTAFE[index++] = formAfeWord(36,0x0000,1);  //off 5,6,7,8 on afe0
		mLutMSRTAFE[index++] = formAfeWord(48,0x0000,2);  //off 9,10,11,12 on afe1
		mLutMSRTAFE[index++] = formAfeWord(60,0x0c0c,2);  //off 13,14,15,16 on afe1
		mLutMSRTAFE[index++] = formAfeWord(197,0x0003,1);  //off 1,2,3,4,5,6,7,8 on afe0
		mLutMSRTAFE[index++] = formAfeWord(197,0xc000,2);  //off 9,10,11,12,13,14,15,16 on afe1
    }
    else if (c_rx_ap == 60) 
    {
		mLutMSRTAFE[index++] = formAfeWord(0, 0x0);
		mLutMSRTAFE[index++] = formAfeWord(24,0x0101,1);  //off 1,2,3,4 on afe0
		mLutMSRTAFE[index++] = formAfeWord(36,0x0000,1);  //off 5,6,7,8 on afe0
		mLutMSRTAFE[index++] = formAfeWord(48,0x0000,2);  //off 9,10,11,12 on afe1
		mLutMSRTAFE[index++] = formAfeWord(60,0x0808,2);  //off 13,14,15,16 on afe1
		mLutMSRTAFE[index++] = formAfeWord(197,0x0001,1);  //off 1,2,3,4,5,6,7,8 on afe0
		mLutMSRTAFE[index++] = formAfeWord(197,0x8000,2);  //off 9,10,11,12,13,14,15,16 on afe1
    }

    if (powerMode == 0)
    {
        
        mLutMSRTAFE[index++] = formAfeWord(229, powerMode);
        mLutMSRTAFE[index++] = formAfeWord(199, 0x0180);  //0000 0001 1000 0000 HPF 75 KHz, LPF 10 MHz
    }
    else if (powerMode == 1)
    {
        mLutMSRTAFE[index++] = formAfeWord(229, powerMode);
        mLutMSRTAFE[index++] = formAfeWord(199, 0x0080);  //0000 0000 1000 0000 HPF 75 KHz, LPF 10 MHz
    }
    else if (powerMode == 2)
    {
        mLutMSRTAFE[index++] = formAfeWord(0xC8,0x050);
        mLutMSRTAFE[index++] = formAfeWord(0xCC,0x080);
        mLutMSRTAFE[index++] = formAfeWord(0xCD,0x880);
        mLutMSRTAFE[index++] = formAfeWord(0xD0,0x1FE);
        mLutMSRTAFE[index++] = formAfeWord(0xD8,0x400);
        mLutMSRTAFE[index++] = formAfeWord(0xDB,0x010);
        mLutMSRTAFE[index++] = formAfeWord(0xE5,0x201);
        mLutMSRTAFE[index++] = formAfeWord(199, 0x0080);  //0000 0000 1000 0000 HPF 75 KHz, LPF 10 MHz
    }
    mLutMSRTAFE[index++] = formAfeWord(0, 0x0);
    mMSRTAFElength = index;
#if 0
    for (uint32_t i = 0; i < mMSRTAFElength; i++)
    {
        LOGD("MSRTAFE[%3d] = %08X", i, mLutMSRTAFE[i]);
    }
#endif
}

//------------------------------------------------------------------------------------------------------------------------
// This method calculates the values for the LUT MSSTTXC
//
void ColorAcq::calculateTxConfig( const uint32_t imagingCaseId,
                                  const ScanDescriptor &roi,
                                  const uint32_t numTxBeamsC,
                                  const float    t_a[] )
{
    const UpsReader::Globals *glbPtr = mUpsReaderSPtr->getGlobalsPtr();
    const UpsReader::TransducerInfo *transducerInfoPtr = mUpsReaderSPtr->getTransducerInfoPtr();


    float btxfd;
    mUpsReaderSPtr->getFocus( mBip.depthIndex, btxfd );

    ScanDescriptor fieldOfView;
    mUpsReaderSPtr->getFov(mBip.depthIndex, fieldOfView, IMAGING_MODE_COLOR);
    float fov = fieldOfView.azimuthSpan;  // Weird naming is to mimic python script calc_txconfig_writeDirect.py

    uint32_t els   = transducerInfoPtr->numElements;
    float    pitch = transducerInfoPtr->pitch;
    float    txfs  = glbPtr->txQuantizationFrequency;
    float    c     = mBip.speedOfSound;
    
    float    rxpitch     = mBip.rxPitchBC;
    uint32_t numparallel = mUpsReaderSPtr->getNumParallel(imagingCaseId);
    uint32_t txbeams     = mNumTxBeamsBC;
    uint32_t rxbeams     = mNumRxBeamsBC;
    
    uint32_t btxele;
    float    bcenterfrequency;
    uint32_t bhalfcycles;
    mUpsReaderSPtr->getBWaveformInfo( imagingCaseId,
                                      btxele,
                                      bcenterfrequency,
                                      bhalfcycles );

    uint32_t ctxele;
    float    ccenterfrequency;
    uint32_t chalfcycles;
    mUpsReaderSPtr->getColorWaveformInfo( imagingCaseId,
                                          ctxele,
                                          ccenterfrequency,
                                          chalfcycles );

    uint32_t channel_map[] = {15, 14,  7, 6,  5, 13,  4, 12,  3, 11,  2, 10,  1,  0,  9,  8};
    uint32_t index_map[]   = {13, 12, 10, 8,  6,  4,  3,  2, 15, 14, 11,  9,  7,  5,  1,  0}; 
    
    uint32_t tx_apw     = btxele;
    uint32_t tx_e_first = (uint32_t)(floor(32 - 0.5*tx_apw));
    uint32_t tx_e_last  = tx_e_first + tx_apw - 1;

    uint32_t dev0[4];
    uint32_t dev3[4];

    int32_t referenceTime104MHz = 512;
	float    btxp  =   rxpitch * numparallel;
    uint32_t btxbc =  (uint32_t)(fov / btxp);
    
    float    elfp[64][2];
    {
        float initVal = -1.0 *(els - 1)*pitch/2;
        for (uint32_t eleIndex = 0; eleIndex < els; eleIndex++)
        {
            elfp[eleIndex][0] = initVal + (eleIndex * pitch);
            elfp[eleIndex][1] = 0.0;
        }
    }
    

    {
        int devIndex = 0;
        for (uint32_t i = tx_e_first; i < tx_e_first+4; i++)
        {
            dev0[devIndex] = channel_map[i];
            devIndex++;
        }
        devIndex = 0;
        for (uint32_t i = (tx_e_last-3)%16; i < (tx_e_last+1)%16; i++)
        {
            dev3[devIndex] = channel_map[i];
            devIndex++;
        }
    }

    uint32_t halfcycles9_6ns = (uint32_t)(txfs/(bcenterfrequency*2)) - 2;
    
    // Note hardcoded values in this method are obtained from python script calc_txconfig_writeDirect.py
    // This python script is mainatined by ultrasound engineering.
    
    uint32_t index = 0;
    //-----------  Base config begin -------------------------------------
    mLutMSSTTXC[index++] = 0x00201000;
	mLutMSSTTXC[index++] = 0x00000000;
	mLutMSSTTXC[index++] = 0x00000000;
	mLutMSSTTXC[index++] = 0x0000030C;
	mLutMSSTTXC[index++] = 0x000000F3;
	mLutMSSTTXC[index++] = 0x00008500;
	mLutMSSTTXC[index++] = 0x0000000A;
    mLutMSSTTXC[index++] = (1 << dev0[0]) | (1 << dev0[1]) | (1 << dev0[2]) | (1 << dev0[3]);
	//mLutMSSTTXC[index++] = 0x00000603; #enable 12,13,14,15 maps to 10,9,0,1
	mLutMSSTTXC[index++] = 0x0000FFFF; 
	mLutMSSTTXC[index++] = 0x00000002;
	mLutMSSTTXC[index++] = 0x00401e00;

    
	mLutMSSTTXC[index++] = 0x00211000;
	mLutMSSTTXC[index++] = 0x00000000;
	mLutMSSTTXC[index++] = 0x00000000;
	mLutMSSTTXC[index++] = 0x0000030C;
	mLutMSSTTXC[index++] = 0x000000F3;
	mLutMSSTTXC[index++] = 0x00008500;
	mLutMSSTTXC[index++] = 0x0000000A;
	mLutMSSTTXC[index++] = 0x0000FFFF;
	mLutMSSTTXC[index++] = 0x0000FFFF;
	mLutMSSTTXC[index++] = 0x00000002;
	mLutMSSTTXC[index++] = 0x00401e00;

	mLutMSSTTXC[index++] = 0x00221000;
	mLutMSSTTXC[index++] = 0x00000000;
	mLutMSSTTXC[index++] = 0x00000000;
	mLutMSSTTXC[index++] = 0x0000030C;
	mLutMSSTTXC[index++] = 0x000000F3;
	mLutMSSTTXC[index++] = 0x00008500;
	mLutMSSTTXC[index++] = 0x0000000A;
	mLutMSSTTXC[index++] = 0x0000FFFF;
	mLutMSSTTXC[index++] = 0x0000FFFF;
	mLutMSSTTXC[index++] = 0x00000002;
	mLutMSSTTXC[index++] = 0x00401e00;

	mLutMSSTTXC[index++] = 0x00231000;
	mLutMSSTTXC[index++] = 0x00000000;
	mLutMSSTTXC[index++] = 0x00000000;
	mLutMSSTTXC[index++] = 0x0000030C;
	mLutMSSTTXC[index++] = 0x000000F3;
	mLutMSSTTXC[index++] = 0x00008500;
	mLutMSSTTXC[index++] = 0x0000000A;
	mLutMSSTTXC[index++] = (1 << dev3[0]) | (1 << dev3[1]) | (1 << dev3[2]) | (1 << dev3[3]);
	//mLutMSSTTXC[index++] = 0x0000C0C0; #enable 48,49,50,51 maps to 15,14,7,6
	mLutMSSTTXC[index++] = 0x0000FFFF;
	mLutMSSTTXC[index++] = 0x00000002;
	mLutMSSTTXC[index++] = 0x00401e00;
    

    {
        uint32_t Tclamp_preRX  = 207; // 207+2 cycles at 9.6 ns = 2006 ns, 300 ns according to Fulvio, 300/9.6=31.3, 32-2=30
        uint32_t Tclamp_postRX =  9;  // 9 +2 cycles at 9.6 ns = 105.6 ns, 100 ns accodring to Fulvio, 100/9.6=10.4, 11-2=9
        uint32_t hvp0          = 5;
        uint32_t hvm0          = 10;
        uint32_t cl_wt_rx      = 14;
        uint32_t clamp         = 15;

        uint32_t preWord  = 0x00000000 | clamp | (Tclamp_preRX << 4);   // 0x0040009F        
        uint32_t postWord = 0x00000000 | clamp | (Tclamp_postRX << 4);   // 0x0040009F        
        uint32_t hvpWord  = 0x00000000 | hvp0  | (halfcycles9_6ns << 4);
        uint32_t hvmWord  = 0x00000000 | hvm0  | (halfcycles9_6ns << 4);

        //------------- B waveform begin ----------------------------------
        for (int i = 0; i < 4; i++)
        {
            mLutMSSTTXC[index++] = 0x00200000 + 0x10000*i;
            mLutMSSTTXC[index++] = postWord;
            for (uint32_t j = 0; j < bhalfcycles/2; j++)
            {
                mLutMSSTTXC[index++] = hvpWord;
                mLutMSSTTXC[index++] = hvmWord;
            }
            mLutMSSTTXC[index++] = preWord;
            mLutMSSTTXC[index++] = 0x00400000 | cl_wt_rx; // 0x0040000E

        }

        //--------- Color waveform begin ---------------------------------
        halfcycles9_6ns = (uint32_t)(txfs/(ccenterfrequency*2)) - 2;
        hvpWord  = 0x00000000 | hvp0  | (halfcycles9_6ns << 4);
        hvmWord  = 0x00000000 | hvm0  | (halfcycles9_6ns << 4);
        for (uint32_t i = 0; i < 4; i++)
        {
            mLutMSSTTXC[index++] = 0x00200005 + 0x10000*i;
            mLutMSSTTXC[index++] = postWord;
            for (uint32_t j = 0; j < chalfcycles/2; j++)
            {
                mLutMSSTTXC[index++] = hvpWord;
                mLutMSSTTXC[index++] = hvmWord;
            }
            mLutMSSTTXC[index++] = preWord;
            mLutMSSTTXC[index++] = 0x00400000 | cl_wt_rx; // 0x0040000E
        }
    }

    //------------- ser_trigger_tx
    {
        mLutMSSTTXC[index++] = 0x00201008;
        mLutMSSTTXC[index++] = 0x00400003;
        mLutMSSTTXC[index++] = 0x00211008;
        mLutMSSTTXC[index++] = 0x00400003;
        mLutMSSTTXC[index++] = 0x00221008;
        mLutMSSTTXC[index++] = 0x00400003;
        mLutMSSTTXC[index++] = 0x00231008;
        mLutMSSTTXC[index++] = 0x00400003;
        mLutMSSTTXC[index++] = 0x00201008;
        mLutMSSTTXC[index++] = 0x00400002;
        mLutMSSTTXC[index++] = 0x00211008;
        mLutMSSTTXC[index++] = 0x00400002;
        mLutMSSTTXC[index++] = 0x00221008;
        mLutMSSTTXC[index++] = 0x00400002;
        mLutMSSTTXC[index++] = 0x00231008;
        mLutMSSTTXC[index++] = 0x00400002;
    }
    mBip.baseConfigLength = index;
    
    //------------- PM line, clear all interrupts, low power mode
    {
        mLutMSSTTXC[index++] = 0x00201003;
        mLutMSSTTXC[index++] = 0x004000F3;
        mLutMSSTTXC[index++] = 0x00211003;
        mLutMSSTTXC[index++] = 0x004000F3;
        mLutMSSTTXC[index++] = 0x00221003;
        mLutMSSTTXC[index++] = 0x004000F3;
        mLutMSSTTXC[index++] = 0x00231003;
        mLutMSSTTXC[index++] = 0x004000F3;

        mLutMSSTTXC[index++] = 0x00201008;
        mLutMSSTTXC[index++] = 0x00404502;
        mLutMSSTTXC[index++] = 0x00211008;
        mLutMSSTTXC[index++] = 0x00404502;
        mLutMSSTTXC[index++] = 0x00221008;
        mLutMSSTTXC[index++] = 0x00404502;
        mLutMSSTTXC[index++] = 0x00231008;
        mLutMSSTTXC[index++] = 0x00404502;
    }

    //------------ B delay
    {
        float txfp[64][64][2];  // 64 beams, 64 elements, 2 (x & y), move to class member
        float txa[64];          // beam position array

        float initBeamPos = -1.0*(btxbc - 1) * btxp * 0.5;

        for (uint32_t i = 0; i < btxbc; i++)
        {
            txa[i] = initBeamPos + (i * btxp);
        }

        for (uint32_t beamIndex = 0; beamIndex < btxbc; beamIndex++)
        {
            for (uint32_t eleIndex = 0; eleIndex < els; eleIndex++)
            {
                txfp[beamIndex][eleIndex][0] = btxfd * sin(txa[beamIndex]);
                txfp[beamIndex][eleIndex][1] = btxfd * cos(txa[beamIndex]);
            }
        }
        int32_t  txfdel[64][64]; // delay table for every Tx beam and every element
        float k = -txfs/c;
        int32_t offset = referenceTime104MHz;

        for (uint32_t beamIndex = 0; beamIndex < btxbc; beamIndex++)
        {
            rxDelay( elfp, els, txfp[beamIndex], k, offset, txfdel[beamIndex]);
        }
#ifdef DUMP_DELAY_VALUES
        // Dump txfdel
        {
            std::ofstream delayfile;
            delayfile.open("btxfdelay.bin", std::ios::out | std::ios::binary);
            delayfile.write( (char *)txfdel, btxbc*64*sizeof(uint32_t) );
            delayfile.close();
        }
#endif
        for (uint32_t ti = 0; ti < btxbc; ti++)
        {
            // B LINE, ENABLE BACK INTERRUPTS, NORMAL POWER MODE
            mLutMSSTTXC[index++] = 0x00201003;
            mLutMSSTTXC[index++] = 0x004000F7;
            mLutMSSTTXC[index++] = 0x00211003;
            mLutMSSTTXC[index++] = 0x004000F7;
            mLutMSSTTXC[index++] = 0x00221003;
            mLutMSSTTXC[index++] = 0x004000F7;
            mLutMSSTTXC[index++] = 0x00231003;
            mLutMSSTTXC[index++] = 0x004000F7;
            mLutMSSTTXC[index++] = 0x00201008;
            mLutMSSTTXC[index++] = 0x00404102;
            mLutMSSTTXC[index++] = 0x00211008;
            mLutMSSTTXC[index++] = 0x00404102;
            mLutMSSTTXC[index++] = 0x00221008;
            mLutMSSTTXC[index++] = 0x00404102;
            mLutMSSTTXC[index++] = 0x00231008;
            mLutMSSTTXC[index++] = 0x00404102;      

            int deviceId = 0;
            for (int ci = 12; ci < 16; ci++)
            {
                mLutMSSTTXC[index++] = 0x0020100A + channel_map[ci];
                mLutMSSTTXC[index++] = 0x00400000 | txfdel[ti][ci + deviceId*16];
            }
            for (deviceId = 1; deviceId < 3; deviceId++)
            {
                mLutMSSTTXC[index++] = 0x0020100A | (deviceId << 16);   
                for (int ci = 0; ci < 15; ci++)
                {
                    mLutMSSTTXC[index++] = txfdel[ti][index_map[ci] + deviceId*16];
                }
                mLutMSSTTXC[index++] = 0x00400000 | txfdel[ti][index_map[15] + deviceId*16];
            }
            deviceId = 3;
            for (int ci = 0; ci < 4; ci++)
            {
                mLutMSSTTXC[index++] = (0x0020100A | (deviceId << 16)) + channel_map[ci];
                mLutMSSTTXC[index++] = 0x00400000 | txfdel[ti][ci + deviceId*16];
            }
            
            // Waveform select (pointer at 6e, len 24)
            mLutMSSTTXC[index++] = 0x0020101A;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00400000;
            mLutMSSTTXC[index++] = 0x0021101A;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00400000;
            mLutMSSTTXC[index++] = 0x0022101A;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00400000;
            mLutMSSTTXC[index++] = 0x0023101A;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00400000;
        }
    }

    // Color delay
    {
        float txfp[64][64][2];
        float btxfd = roi.axialStart + 0.6*roi.axialSpan;

        for (uint32_t beamIndex = 0; beamIndex < numTxBeamsC; beamIndex++)
        {
            for (uint32_t eleIndex = 0; eleIndex < els; eleIndex++)
            {
                txfp[beamIndex][eleIndex][0] = btxfd * sin(t_a[beamIndex]);
                txfp[beamIndex][eleIndex][1] = btxfd * cos(t_a[beamIndex]);
            }
        }

        int32_t  txfdel[64][64]; // delay table for every Tx beam and every element
        float k = -txfs/c;
        int32_t offset = referenceTime104MHz;
        
        for (uint32_t beamIndex = 0; beamIndex < numTxBeamsC; beamIndex++)
        {
            rxDelay( elfp, els, txfp[beamIndex], k, offset, txfdel[beamIndex]);
        }

#ifdef DUMP_DELAY_VALUES
        // Dump txfdel
        {
            std::ofstream delayfile;
            delayfile.open("ctxfdelay.bin", std::ios::out | std::ios::binary);
            delayfile.write( (char *)txfdel, btxbc*64*sizeof(uint32_t) );
            delayfile.close();
        }
#endif
        for (uint32_t ti = 0; ti < numTxBeamsC; ti++)
        {
            int deviceId = 0;
            for (int ci = 12; ci < 16; ci++)
            {
                mLutMSSTTXC[index++] = 0x0020100A + channel_map[ci];
                mLutMSSTTXC[index++] = 0x00400000 | txfdel[ti][ci + deviceId*16];
            }

            for (deviceId = 1; deviceId < 3; deviceId++)
            {
                mLutMSSTTXC[index++] = 0x0020100A | (deviceId << 16);   

                for (int ci = 0; ci < 15; ci++)
                {
                    mLutMSSTTXC[index++] = txfdel[ti][index_map[ci] + deviceId*16];
                }
                mLutMSSTTXC[index++] = 0x00400000 | txfdel[ti][index_map[15] + deviceId*16];
            }
            deviceId = 3;
            for (int ci = 0; ci < 4; ci++)
            {
                mLutMSSTTXC[index++] = (0x0020100A | (deviceId << 16)) + channel_map[ci];
                mLutMSSTTXC[index++] = 0x00400000 | txfdel[ti][ci + deviceId*16];
            }

            // waveform select (pointer at 6e, len 24)
            mLutMSSTTXC[index++] = 0x0020101A;
            mLutMSSTTXC[index++] = 0x00000505;
            mLutMSSTTXC[index++] = 0x00000505;
            mLutMSSTTXC[index++] = 0x00000505;
            mLutMSSTTXC[index++] = 0x00000505;
            mLutMSSTTXC[index++] = 0x00000505;
            mLutMSSTTXC[index++] = 0x00000505;
            mLutMSSTTXC[index++] = 0x00000505;
            mLutMSSTTXC[index++] = 0x00400505;
            mLutMSSTTXC[index++] = 0x0021101A;
            mLutMSSTTXC[index++] = 0x00000505;
            mLutMSSTTXC[index++] = 0x00000505;
            mLutMSSTTXC[index++] = 0x00000505;
            mLutMSSTTXC[index++] = 0x00000505;
            mLutMSSTTXC[index++] = 0x00000505;
            mLutMSSTTXC[index++] = 0x00000505;
            mLutMSSTTXC[index++] = 0x00000505;
            mLutMSSTTXC[index++] = 0x00400505;
            mLutMSSTTXC[index++] = 0x0022101A;
            mLutMSSTTXC[index++] = 0x00000505;
            mLutMSSTTXC[index++] = 0x00000505;
            mLutMSSTTXC[index++] = 0x00000505;
            mLutMSSTTXC[index++] = 0x00000505;
            mLutMSSTTXC[index++] = 0x00000505;
            mLutMSSTTXC[index++] = 0x00000505;
            mLutMSSTTXC[index++] = 0x00000505;
            mLutMSSTTXC[index++] = 0x00400505;
            mLutMSSTTXC[index++] = 0x0023101A;
            mLutMSSTTXC[index++] = 0x00000505;
            mLutMSSTTXC[index++] = 0x00000505;
            mLutMSSTTXC[index++] = 0x00000505;
            mLutMSSTTXC[index++] = 0x00000505;
            mLutMSSTTXC[index++] = 0x00000505;
            mLutMSSTTXC[index++] = 0x00000505;
            mLutMSSTTXC[index++] = 0x00000505;
            mLutMSSTTXC[index++] = 0x00400505;
        }
    }
    mMSSTTXClength = index;
#ifdef DUMP_DELAY_VALUES
    printf("Size of MSSTTXC is %d words\n", index);
    for (int i = 164; i < 256; i++)
        printf("%3d %08X\n", i, mLutMSSTTXC[i]);
#endif
}

//------------------------------------------------------------------------------------------------------------------------
void ColorAcq::calculateBpfLut( const uint32_t imagingCaseId )
{
    mUpsReaderSPtr->getColorBpfLut( imagingCaseId, mLutMSBPFI, mLutMSBPFQ );
}


//------------------------------------------------------------------------------------------------------------------------
// Performs calculations needed to determine roi.axialStart, delay memory start (dmStart), and beamformer samples (bfS)
// see calc_Axia_ROi.py
//
void ColorAcq::calculateAxialRoi( const uint32_t imagingCaseId,
                                  uint32_t       &bfS,
                                  int32_t        &dmStart,
                                  ScanDescriptor &roi )
{
    const UpsReader::Globals* glbPtr = mUpsReaderSPtr->getGlobalsPtr();
    const UpsReader::TransducerInfo* tinfoPtr = mUpsReaderSPtr->getTransducerInfoPtr();

    float    soundspeed = mBip.speedOfSound;
    uint32_t depthIndex = mBip.depthIndex;

    float    fs         = glbPtr->samplingFrequency;
    uint32_t intBits;
    uint32_t fracBits;

    mUpsReaderSPtr->getColorFocalCompensationParams( imagingCaseId, mBeam0XCRoiMax, intBits, fracBits );
    convertToFloat( mBeam0XCRoiMax, TBF_LUT_B0FPX_SIZE/2, intBits, fracBits-1, mBeam0XCRoiMaxFloat);

    uint32_t numBkptsC;
    uint32_t numBkptsB;
    
    mUpsReaderSPtr->getNumFocalBkpts( imagingCaseId, numBkptsB, numBkptsC );
    
    uint32_t idxStart, idxStop;
    findStartStopIndices( mBeam0XCRoiMaxFloat, numBkptsC, mBeamParams.dmStart, bfS, idxStart, idxStop );

    while ( (bfS > (mBeam0XCRoiMaxFloat[idxStop] - mBeam0XCRoiMaxFloat[idxStart])) &&
            (idxStop < numBkptsC))
    {
        idxStop++;
    }
    bfS = (uint32_t)(mBeam0XCRoiMaxFloat[idxStop] - mBeam0XCRoiMaxFloat[idxStart]);
    roi.axialStart = mBeam0XCRoiMaxFloat[idxStart] * (0.5 * soundspeed)/fs;
    /*
     * Patch taken from THSW-2628, Roi axial span adjust
     * */
    while ((roi.axialSpan+roi.axialStart) > (mBip.imagingDepth)) {
        mBeamParams.outputSamples = mBeamParams.outputSamples - 16;
        roi.axialSpan = mBip.imagingDepth * mBeamParams.outputSamples / glbPtr->maxCSamplesPerLine;
    }
    float ctxfd =roi.axialStart+0.6*roi.axialSpan;
    float maxRoiAzim = abs(roi.azimuthStart) > abs(roi.azimuthStart+roi.azimuthSpan) ? abs(roi.azimuthStart) : abs(roi.azimuthStart+roi.azimuthSpan);
    float center = roi.azimuthStart+roi.azimuthSpan/2;
    float xx=abs(ctxfd*sin(maxRoiAzim));
    float yy=abs(ctxfd*cos(maxRoiAzim));
    float HalfAp = 8.992;
    float zz=sqrt((HalfAp+xx)*(HalfAp+xx)+yy*yy);
    uint32_t dm_start_offset_ref = 40;
    uint32_t dm_start_offset=zz*fs/soundspeed-ctxfd*fs/soundspeed-dm_start_offset_ref;
    dmStart = (uint32_t)(mBeam0XCRoiMaxFloat[idxStart] - dm_start_offset);
    // The following info is needed to build Focal Interpolation Rate LUT
    mFocalPointInfo.idxStart  = idxStart;
    mFocalPointInfo.idxStop   = idxStop;
    mFocalPointInfo.numBkptsC = idxStop - idxStart + 1;
    mFocalPointInfo.numBkptsB = numBkptsB;
    mFocalPointInfo.focalDepthMm = roi.axialStart + roi.axialSpan * glbPtr->cTxFocusFraction;
}

//-----------------------------------------------------------------------------------------------------------------------
void ColorAcq::setupCtbParams(CtbParams &input, CtbParams &output, uint32_t groupIndex, uint32_t ensembleIndex, uint32_t interleaveIndex)
{
    const UpsReader::Globals *glbPtr = mUpsReaderSPtr->getGlobalsPtr();
    uint32_t gi = groupIndex;
    uint32_t ii = interleaveIndex;
    uint32_t ei = ensembleIndex;
    uint32_t csamples = mBeamParams.outputSamples;
    uint32_t cens = mBeamParams.ensembleSize;
    uint32_t ctbLimit = glbPtr->ctbLimit;

    input.innerPtr  = csamples*ei + cens*csamples*ii*4;  // TODO: 4 should be ML factor
    output.innerPtr = (((ei*mInterleaveFactor + ii) % mBip.ctbReadBlockDiv) * mBeamParams.colorProcessingSamples) + \
                      (uint32_t)((ei*mInterleaveFactor + ii)/mBip.ensDiv)*csamples*cens;

    if ( gi == 0 )
    {
        input.outerPtr  = 0;
        output.outerPtr = 0;
    }
    #if 0
    else if (gi == (mNumGroups - 0))
    {
        input.outerPtr  = ((gi & 1) != 0) ? ctbLimit : 0;
        output.outerPtr = ((gi & 1) == 0) ? ctbLimit : 0;
    }
    #endif
    else
    {
        input.outerPtr  = ((gi & 1) != 0) ? ctbLimit : 0;
        output.outerPtr = ((gi & 1) == 0) ? ctbLimit : 0;
    }
}

//-----------------------------------------------------------------------------------------------------------------------
void ColorAcq::setupTxdSpiPtr( uint32_t & ptr, uint32_t groupIndex, uint32_t ensembleIndex, uint32_t interleaveIndex)
{
    UNUSED(ensembleIndex);
    uint32_t gi = groupIndex;
    uint32_t ii = interleaveIndex;

    ptr = (gi*mInterleaveFactor + ii)*86 + mNumTxBeamsBC*102 + mBip.baseConfigLength + 16;
}

//-----------------------------------------------------------------------------------------------------------------------
void ColorAcq::setupTxPingIndex( uint32_t & pindex, uint32_t groupIndex, uint32_t ensembleIndex, uint32_t interleaveIndex)
{
    UNUSED(ensembleIndex);
    uint32_t gi = groupIndex;
    uint32_t ii = interleaveIndex;

    pindex = gi*mInterleaveFactor + ii;
}

//-----------------------------------------------------------------------------------------------------------------------
void ColorAcq::setupTxAngle( float & ta, uint32_t groupIndex, uint32_t ensembleIndex, uint32_t interleaveIndex)
{
    UNUSED(ensembleIndex);
    uint32_t gi = groupIndex;
    uint32_t ii = interleaveIndex;

    ta = mTxAngles[gi*mInterleaveFactor + ii];
}

//-----------------------------------------------------------------------------------------------------------------------
void ColorAcq::initCtbParams( CtbParams &input, CtbParams &output)
{
    input.innerCount  = 4; // TODO: multiline factor
    output.innerCount = mBeamParams.ensembleSize;
    input.outerCount  = mBeamParams.outputSamples;
    output.outerCount = mBeamParams.outputSamples;
    input.innerIncr   = mBeamParams.outputSamples * mBeamParams.ensembleSize;
    output.innerIncr  = mBeamParams.outputSamples;
    input.outerIncr   = 1;
    output.outerIncr  = 1;

}

//-----------------------------------------------------------------------------------------------------------------------

void ColorAcq::setupBpRegs( EnableFlags flags,
                            uint32_t    txdSpiPtr,
                            uint32_t    txdWordCount,
                            float       txAngle,
                            CtbParams   ctbInput,
                            CtbParams   ctbOutput,
                            uint32_t    txPingIndex,
                            uint32_t    pri,
                            uint32_t    bgPtr,
                            uint32_t    bpRegs[62])
{
    const uint32_t BPF_LS = 4;   // Bandpass filter latency per filter length (64 taps)
    const uint32_t DPL_S  = 128; // Data path latency

    const UpsReader::Globals* glbPtr = mUpsReaderSPtr->getGlobalsPtr();

    // Parameter #0
    {
        int32_t extraSamples = glbPtr->bpf_P + glbPtr->fs_S;
        int32_t preload      = glbPtr->dm_P;
        int32_t adcSamples = mBeamParams.bfS - 1 + extraSamples;
        bpRegs[0] = (preload << 16) | (adcSamples & 0xffff);
    }

    // Parameter #1
    {
        bpRegs[1] = ( ((mBeamParams.dmStart%1024) << 16) | ((mBeamParams.fpS - 1) & 0xffff) );
    }

    // Parameter #2
    {
        uint32_t val;
        if ( flags.ctbwEn || flags.cfpEn )
        {
            val = (((mBeamParams.cfpOutputSamples * 4) - 1) << 16) | (mBeamParams.bfS-1);
        }
        else
        {
            val = ((mBeamParams.outputSamples  - 1) << 16) | (mBeamParams.bfS-1);
        }
        bpRegs[2] = val;
    }

    // Parameter #3
    {
        uint32_t val;
        if (flags.pmLine)
        {
            val = 0x00010000;
        }
        else
        {
            val = (mBeamParams.hvClkEnOn << 16) | mBeamParams.hvClkEnOff;
        }
        bpRegs[3] = val;
    }

    // Parameter #4
    {
        BpRegBENABLE reg;
        reg.val = 0;

        reg.bits.pmonEn         = 1;
        reg.bits.bpSlopeEn      = flags.bpSlopeEn;
        reg.bits.ceof           = flags.ceof;
        reg.bits.eos            = flags.eos;
        if (flags.pmLine)
        { 
            reg.bits.bpHvSpiEn      = 1;
            reg.bits.bpStHvTrigHold = 1;
            reg.bits.pmonEnPwrMgt   = 1;
            reg.bits.bpHvPsBurstEn  = 1;
        }
        else
        {
            reg.bits.txEn = flags.txEn;
            reg.bits.rxEn = flags.rxEn;
            reg.bits.bfEn = flags.bfEn;
            reg.bits.outEn = flags.outEn;
            reg.bits.outIqType = flags.cfpEn;
            reg.bits.ctbWrEn   = flags.ctbwEn;
            reg.bits.cfpEn     = flags.cfpEn;
        }
        if (flags.ctbwEn | flags.cfpEn)
        {
            reg.bits.outType   = 1;
            reg.bits.outIqType = 1;
        }
        bpRegs[4] = reg.val;
    }

    // Parameter #5
    {
        BpRegBPWR reg;
        reg.val = 0;

        reg.bits.pmDigChPm = 1;
        if (flags.pmAfePwrSb)
        {
            reg.bits.pmAfePwrSb = 1;
        }
        else
        {
            reg.bits.pmAfeSbTimer    = 1;
            if (flags.pmLine)
            {
                reg.bits.pmAfeGblTimer = 1;
            }
        }
        bpRegs[5] = reg.val;
    }

    // Parameter #6
    {
        bpRegs[6] = txdSpiPtr | (txdWordCount << 16);
    }

    // Parameter #7
    {
        uint32_t txlen = 1280;
        bpRegs[7] = (txPingIndex << 16) | txlen;
    }

    // Parameter #8, 9
    {
        bpRegs[8] = 0;
        bpRegs[9] = 0;
    }

    // Parameter #10, #11
    {
        uint32_t rx_e_first  = (uint32_t)floor(32-0.5*mBeamParams.rxAperture);
        uint32_t rx_e_last   = rx_e_first + (mBeamParams.rxAperture - 1);

        uint32_t rxap_low  = 0;
        uint32_t rxap_high = 0;

        for (uint32_t ee = 0; ee < 32; ee++)
        {
            if ( (ee >= rx_e_first) && (ee <= rx_e_last) )
            {
                rxap_low = rxap_low + (1 << ee);
            }
        }

        for (uint32_t ee = 32; ee < 64; ee++)
        {
            if ( (ee >= rx_e_first) && (ee <= rx_e_last) )
            {
                rxap_high = rxap_high + (1 << (ee-32));

            }
        }
        bpRegs[10] = rxap_low;
        bpRegs[11] = rxap_high;
    }

    // Parameter #12
    {
        uint32_t pri_min = mBip.rxStart + glbPtr->dm_P + mBeamParams.bfS + glbPtr->bpf_P +  BPF_LS + DPL_S;

        uint32_t bpri = pri > pri_min ? pri : pri_min;
        bpri = (uint32_t)ceil(bpri/25.0)*25;
        bpRegs[12] = bpri;
        mTxPrf = glbPtr->samplingFrequency / bpri;
    }

    // Parameter #13
    {
        bpRegs[13] = glbPtr->txStart | (mBip.rxStart << 16);
    }

    // Parameter #14, #15
    {
        bpRegs[14] = 0;
        bpRegs[15] = 0;
    }

    // Parameter #16
    {
        uint32_t taps = mBeamParams.bpfTaps;
        BpRegBPF0 bpf0;
        bpf0.val = 0;
        
        bpf0.bits.vbpFc  = 0;
        bpf0.bits.vbpPr  = 0;
        bpf0.bits.vbpPlc = 64;

        uint32_t max_taps = (uint32_t)floor(mBeamParams.decimationFactor)*16;
        max_taps = max_taps > 64 ? 64 : max_taps;

        bpf0.bits.vbpFl = ((max_taps >> 1) - 1) & 0xff;  
        
        bpf0.bits.vbpFs  = 2;
        bpf0.bits.vbpOscale = mBeamParams.bpfScale;

        bpRegs[16] = bpf0.val;
    }

    // Parameter 17
    {

        BpRegBPF1 bpf1;
        bpf1.val = 0;

        bpf1.bits.vbpPinc0 = (uint32_t)(floor(mBeamParams.decimationFactor)) & 0xff; 
        bpf1.bits.vbpPinc1 = (uint32_t)( ceil(mBeamParams.decimationFactor)) & 0xff; 
        
        bpf1.bits.vbpWc0   = (uint32_t)(floor(mBeamParams.decimationFactor)*8 -2) & 0xff;
        bpf1.bits.vbpWc1   = (uint32_t)( ceil(mBeamParams.decimationFactor)*8 -2) & 0xff;
        
        bpRegs[17] = bpf1.val;
    }

    // Parameter #18
    {
        BpRegBPF2 bpf2;
        bpf2.val = 0;

        bpf2.bits.vbpPhaseInc = (uint32_t)(floor(mBeamParams.decimationFactor*8)) & 0xff;
        bpf2.bits.vbpWc       = (mBeamParams.outputSamples - 1) & 0x3fff;
        bpRegs[18] = bpf2.val;
    }

    // Parameters #19
    {
        float D = mBeamParams.decimationFactor;
        float vbp_df = (D-floor(D));
        BpRegBPF3 bpf3;
        bpf3.val = 0;
        
        if (vbp_df == 0.0)
            bpf3.bits.vbpDseq = 0x0000;
        if (vbp_df == 0.0625)
            bpf3.bits.vbpDseq = 0x8000;
        if (vbp_df == 0.125)
            bpf3.bits.vbpDseq = 0x8080;
        if (vbp_df == 0.1875)
            bpf3.bits.vbpDseq = 0x8420;
        if (vbp_df == 0.25)
            bpf3.bits.vbpDseq = 0x8888;
        if (vbp_df == 0.3125)
            bpf3.bits.vbpDseq = 0x9248;
        if (vbp_df == 0.375)
            bpf3.bits.vbpDseq = 0xA4A4;
        if (vbp_df == 0.4375)
            bpf3.bits.vbpDseq = 0xAA54;
        if (vbp_df == 0.5)
            bpf3.bits.vbpDseq = 0xAAAA;
        if (vbp_df == 0.5625)
            bpf3.bits.vbpDseq = 0xD5AA;
        if (vbp_df == 0.625)
            bpf3.bits.vbpDseq = 0xDADA;
        if (vbp_df == 0.6875)
            bpf3.bits.vbpDseq = 0xEDB6;
        if (vbp_df == 0.75)
            bpf3.bits.vbpDseq = 0xEEEE;
        if (vbp_df == 0.8125)
            bpf3.bits.vbpDseq = 0xFBDE;
        if (vbp_df == 0.875)
            bpf3.bits.vbpDseq = 0xFEFE;
        if (vbp_df == 0.9375)
            bpf3.bits.vbpDseq = 0xFFFE;

        bpf3.bits.vbpIc =  (mBeamParams.bfS - 1) & 0x3FFF;

        bpRegs[19] = bpf3.val;
    }

    // Parameter #20, 21, 22
    bpRegs[20] = 0x01000000;
    bpRegs[21] = 1;
    bpRegs[22] = 0;

    // Parameter #23
    {
        BpRegDTGC0 dtgc0;
        dtgc0.val = 0;
        
        dtgc0.bits.bgPtr   = bgPtr;
        dtgc0.bits.tgc0Lgc = mBeamParams.lateralGains[0] & 0xffffff;
        dtgc0.bits.tgc0Flat = 0;
        bpRegs[23] = dtgc0.val;
    }

    // Parameter #24
    {
        BpRegDTGC1 dtgc1;
        dtgc1.val = 0;
        
        dtgc1.bits.tgc1Lgc = mBeamParams.lateralGains[1] & 0xffffff;
        dtgc1.bits.tgc1Flat = 0;
        dtgc1.bits.bgRc    = mBeamParams.numGainBreakpoints - 1;
        dtgc1.bits.tgcFlip  = 0;
        bpRegs[24] = dtgc1.val;
    }

    // Parameter #25
    {
        BpRegDTGC2 dtgc2;
        dtgc2.val = 0;

        dtgc2.bits.tgc2Lgc = mBeamParams.lateralGains[2] & 0xffffff;
        dtgc2.bits.tgc2Flat = 0;
        bpRegs[25] = dtgc2.val;
    }

    // Parameter #26
    {
        BpRegDTGC3 dtgc3;
        dtgc3.val = 0;

        dtgc3.bits.tgc3Lgc = mBeamParams.lateralGains[3] & 0xffffff;
        dtgc3.bits.tgc3Flat = 0;
        bpRegs[26] = dtgc3.val;
    }

    // Parameters #27, 28
    bpRegs[27] = mBeamParams.tgcirLow;
    bpRegs[28] = mBeamParams.tgcirHigh;

    // Parameter #29
    {
        BpRegCCOMP ccomp;
        ccomp.val = 0;
        
        ccomp.bits.ccPtr     = 0;
        ccomp.bits.ccOutEn   = 1;
        ccomp.bits.ccWe      = 1;
        ccomp.bits.ccSum     = 0;
        ccomp.bits.ccOne     = 1;
        ccomp.bits.ccInv     = 0;
        ccomp.bits.ccOutMode = 0;
        bpRegs[29] = ccomp.val;
    }

    bpRegs[30] = 0;
    bpRegs[31] = 0;
    bpRegs[32] = 0;
    bpRegs[33] = 0;
    bpRegs[34] = 0;
    bpRegs[35] = 0;
    bpRegs[36] = 0;
    bpRegs[37] = 0;

    bpRegs[38] = (int32_t)(floor(sin(txAngle + mBeamParams.rxAngles[0]) * (1<<16))) & 0x3ffff;
    bpRegs[39] = (int32_t)(floor(cos(txAngle + mBeamParams.rxAngles[0]) * (1<<16))) & 0x3ffff;
    bpRegs[40] = (int32_t)(floor(sin(txAngle + mBeamParams.rxAngles[1]) * (1<<16))) & 0x3ffff;
    bpRegs[41] = (int32_t)(floor(cos(txAngle + mBeamParams.rxAngles[1]) * (1<<16))) & 0x3ffff;
    bpRegs[42] = (int32_t)(floor(sin(txAngle + mBeamParams.rxAngles[2]) * (1<<16))) & 0x3ffff;
    bpRegs[43] = (int32_t)(floor(cos(txAngle + mBeamParams.rxAngles[2]) * (1<<16))) & 0x3ffff;
    bpRegs[44] = (int32_t)(floor(sin(txAngle + mBeamParams.rxAngles[3]) * (1<<16))) & 0x3ffff;
    bpRegs[45] = (int32_t)(floor(cos(txAngle + mBeamParams.rxAngles[3]) * (1<<16))) & 0x3ffff;

    bpRegs[46] = mBeamParams.beamScales[0] | (mBeamParams.beamScales[1] << 16);
    bpRegs[47] = mBeamParams.beamScales[2] | (mBeamParams.beamScales[3] << 16);
    
    bpRegs[48] = 0x2000200; // FPPTR0    
    bpRegs[49] = 0x2000200; // FPPTR1
    bpRegs[50] = 0x2000200; // TXPTR0    
    bpRegs[51] = 0x2000200; // TXPTR1

    {
        uint32_t bkpts = mFocalPointInfo.numBkptsB;

        bkpts = (bkpts & 7) == 0 ? bkpts : ((bkpts & 0xfffffff8) + 8);
        bpRegs[52] = 0x02000000 | (bkpts & 0xffff);
    }
    bpRegs[53] = 0;
    bpRegs[54] = 0;

    bpRegs[55] = (ctbInput.outerPtr >> 1)   | ((ctbInput.innerPtr >> 1)   << 16); 
    bpRegs[56] =  ctbInput.outerIncr        |  (ctbInput.innerIncr        << 16);
    bpRegs[57] = (ctbInput.outerCount - 1)  | ((ctbInput.innerCount - 1)  << 16);

    bpRegs[58] = (ctbOutput.outerPtr >> 1)  | ((ctbOutput.innerPtr >> 1)  << 16); 
    bpRegs[59] =  ctbOutput.outerIncr       |  (ctbOutput.innerIncr       << 16);

    {
        uint32_t ens = mBeamParams.ensembleSize;
        uint32_t val = (ctbOutput.outerCount/((ens >> 3) << 1)) - 1;
        bpRegs[60] = val | ((ctbOutput.innerCount - 1) << 16);
    }

    {
        uint32_t os       = mBeamParams.outputSamples;
        uint32_t blockDiv = mBip.ctbReadBlockDiv;
        uint32_t ens      = mBeamParams.ensembleSize;

        bpRegs[61] = (((os/blockDiv) - 1) << 16) | (ens - 1);
    }
}    

void  ColorAcq::getSequencerLoopCounts( uint32_t& totalBeams,
                                        uint32_t& interleaveFactor,
                                        uint32_t& numGroups,
                                        uint32_t& numCtbReadBlocks)
{
    totalBeams       = mFiringCount;
    interleaveFactor = mInterleaveFactor;
    numGroups        = mNumGroups;
    numCtbReadBlocks = mCtbReadBlocks;
}

void ColorAcq::getBeamCounts( BeamCount &bc )
{
    bc.txB = mNumTxBeamsB;
    bc.rxB = mNumRxBeamsB;
    bc.txBC = mNumTxBeamsBC;
    bc.rxBC = mNumRxBeamsBC;
}

//-----------------------   private methods ------------------------------------------------------------------------------
void ColorAcq::findStartStopIndices( const float inputArray[],
                           uint32_t arrayLen,
                           uint32_t thresh,
                           uint32_t numSamples,
                           uint32_t &startIndex,
                           uint32_t &stopIndex)
{
    float    minDiff = 1.0e9;
    uint32_t minIndex = 0;
    float val = thresh;

    
    for (uint32_t i = 0; i < arrayLen; i++)
    {
        float absDiff = abs(inputArray[i] - val);
        if (absDiff < minDiff)
        {
            minDiff = absDiff;
            minIndex = i;
        }
    }
    startIndex = minIndex;

    minDiff = 1.0e9;
    val = inputArray[startIndex] + numSamples;

    for (uint32_t i = minIndex; i < arrayLen; i++)
    {
        float absDiff = abs(inputArray[i] - val);
        if (absDiff < minDiff)
        {
            minDiff = absDiff;
            minIndex = i;
        }
    }
    stopIndex = minIndex;
}


void ColorAcq::findStartStopIndicesInt( const int32_t inputArray[],
                                     uint32_t arrayLen,
                                     int32_t threshStart,
                                     int32_t threshStop,
                                     uint32_t &startIndex,
                                     uint32_t &stopIndex)
{
    int32_t  minDiff = 0x7fffffff;
    uint32_t minIndex = 0;
    
    for (uint32_t i = 0; i < arrayLen; i++)
    {
        int32_t absDiff = (inputArray[i] - threshStart);
        if (absDiff < 0)
            absDiff = -absDiff;
        
        if (absDiff < minDiff)
        {
            minDiff = absDiff;
            minIndex = i;
        }
    }
    startIndex = minIndex;

    minDiff = 0x7fffffff;

    for (uint32_t i = minIndex; i < arrayLen; i++)
    {
        int32_t absDiff = (inputArray[i] - threshStop);
        if (absDiff < 0) absDiff = -absDiff;
        if (absDiff < minDiff)
        {
            minDiff = absDiff;
            minIndex = i;
        }
    }
    stopIndex = minIndex;
}

void ColorAcq::convertToFloat( const int32_t inArray[], uint32_t len, uint32_t intBits, uint32_t fracBits, float outArray[])
{
    UNUSED(intBits);

    float scaleFactor = 1.0/(1<<fracBits);
    
    for (uint32_t i = 0; i < len; i++)
    {
        outArray[i] = inArray[i] * scaleFactor;
    }
}

void ColorAcq::convertToInt( const float inArray[], uint32_t len, uint32_t intBits, uint32_t fracBits, int32_t outArray[])
{
    UNUSED(intBits);
    float scaleFactor = (float)(1<<fracBits);
    
    for (uint32_t i = 0; i < len; i++)
    {
          outArray[i] = (int32_t)floor(inArray[i] * scaleFactor + 0.5);
    }
}

uint32_t ColorAcq::formAfeWord( uint32_t addr, uint32_t data, uint32_t dieSelect)
{
    uint32_t afeWord = data & 0xffff;
    switch (dieSelect)
    {
    case 0:
        afeWord |= 0x0f000000;
        break;

    case 1:
        afeWord |= 0x03000000;
        break;

    case 2:
        afeWord |= 0x0c000000;
        break;

    default:
        LOGE("Invalid dieSelect: %d", dieSelect);
        break;
    }
    afeWord |= (addr & 0xff) << 16;
    return (afeWord);         
}

uint32_t ColorAcq::formAfeGainCode(float powerMode, float g)
{
    float gc;
    if (powerMode == 0)
        gc = round((g -12.0) * (319.0 / (51.5 - 12)));
    else
        gc = round((g - 6.0) * (319.0 / (45.5 - 6)));

    gc = gc < 319.0 ? gc : 319.0;
    gc = gc < 0 ? 0 : gc;

    return ((uint32_t)gc);
}
