//
// Copyright 2018 EchoNous Inc.
//

#define LOG_TAG "DauColorAcq"

#include <cstring>
#include <algorithm>
#include <ThorUtils.h>
#include <math.h>
#include <ColorAcqLinear.h>
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

// RX delay calc, hard coded for linear 128 elements and 64 beams
inline void rxDelay( const float eleLoc[128][2], const uint32_t els, const float txfp[2],
                     const float dref, const float k, const float offset,
                     int32_t  delayVals[128])
{
    for (uint32_t eleIndex = 0; eleIndex < els; eleIndex++)
    {
        float x = txfp[0];
        float y = txfp[1];
        float xe = eleLoc[eleIndex][0];
        float ye = eleLoc[eleIndex][1];
        float rd = sqrt((xe-x)*(xe-x) + (ye-y)*(ye-y)) - dref;
        delayVals[eleIndex] = (int32_t)( round(rd * k) + offset );
    }
}

ColorAcqLinear::ColorAcqLinear(const std::shared_ptr<UpsReader>& upsReader) :
    mUpsReaderSPtr(upsReader)
{
    LOGD("*** ColorAcqLinear - constructor");
    fetchGlobals();
}

ColorAcqLinear::~ColorAcqLinear()
{
    LOGD("*** ColorAcqLinear -destructor");
}

//-----------------------------------------------------------------------------------------------
void ColorAcqLinear::calculateScanConverterParams( const ScanDescriptor& roi, ScanConverterParams& scp)
{
    uint32_t numSamples = mBeamParams.outputSamples;
    scp.numSampleMesh = 50;
    scp.numRayMesh = 100;
    scp.numSamples = numSamples;
    scp.startSampleMm = roi.axialStart;
    scp.sampleSpacingMm = roi.axialSpan/(numSamples - 1);
    scp.startRayRadian = -(roi.azimuthStart + roi.azimuthSpan);
    scp.raySpacing = mBip.colorRxPitch;
    scp.numRays = mNumTxBeamsC * 4; // TODO: remove hardcoding of multiline factor of 4
}


//--------------------------------------------------------------------------------
ThorStatus ColorAcqLinear::calculateBeamParams(const uint32_t       imagingCaseId,
                                   const uint32_t       requestedPrfIndex,
                                   const ScanDescriptor requestedRoi,
                                   uint32_t&            actualPrfIndex,
                                   ScanDescriptor&      actualRoi)
{
    ThorStatus retVal = THOR_OK;
    const UpsReader::Globals* glbPtr = mUpsReaderSPtr->getGlobalsPtr();
    const UpsReader::TransducerInfo* tinfoPtr = mUpsReaderSPtr->getTransducerInfoPtr();

    uint32_t    adcSamples;
    uint32_t numTxBeamsBC;
    float    Fs = glbPtr->samplingFrequency;
    float    imagingDepth;
    // ROI correction
    float rxtxo        = (3.0/1.0)*(tinfoPtr->pitch);  // Offset in mm from RX beam 0 or 3 to TX beam
    float    c         = mBip.speedOfSound;
    float mmps         = (c/2.0)/Fs;  // mm per ADC sample
    float roiatan;     // Tangent of ROI angle
    float roiacos;     // Cos of ROI angle
    float tx_ox [32];  // TX beam origins
    float rx_ox [128]; // RX beam origins
    float latStartTx;
    float latdelta;
    uint32_t range_s;
    uint32_t span_s;
    float range_mm;
    float span_mm;
    float tv;
    int ls_beam;
    int le_beam;
    int latbeams;
    int latover;
    float effective_depthSpan = 0.0f;
    float amount_adjust = 0.0f;

    if (requestedPrfIndex >= mNumValidPrfs)
    {
        LOGE("requested PRF index (%d) exceeds max (%d)",
             requestedPrfIndex, mNumValidPrfs);
        return (TER_IE_COLORFLOW_PARAM);
    }

    mUpsReaderSPtr->getImagingDepthMm( imagingCaseId, imagingDepth );
    mUpsReaderSPtr->getDepthIndex( imagingCaseId, mBip.depthIndex );
    mBip.imagingDepth = imagingDepth;

    // Get actual ROI from requested ROI
    actualRoi = requestedRoi;
    roiatan   = tan(actualRoi.roiA);
    roiacos   = cos(actualRoi.roiA);

    // To handle a corner case in Linear steered case where axial span can go beyond the imaging depth
    {
        effective_depthSpan = actualRoi.axialSpan * roiacos;

        if ( (actualRoi.axialStart + effective_depthSpan) > imagingDepth )
        {
            amount_adjust = actualRoi.axialStart + effective_depthSpan - imagingDepth;

            if (actualRoi.axialStart > amount_adjust)
            {
                // move axialStart if possible
                actualRoi.axialStart -= amount_adjust;
            }
            else if (actualRoi.axialSpan > amount_adjust / roiacos)
            {
                // reduce axialSpan
                actualRoi.axialSpan -= amount_adjust / roiacos;
            }
        }
    }

    // Quantize ROI start ADC sample
    range_s = (uint32_t)(floor((actualRoi.axialStart/roiacos)/mmps)); // First ADC sample
    span_s  = (uint32_t)(floor((actualRoi.axialSpan/roiacos)/mmps));  // ROI ADC samples
    range_mm = (float)(range_s*mmps); // Quantized range to ROI
    span_mm  = (float)(span_s*mmps);  // Quantized ROI span
    // Quantized axial start and span
    actualRoi.axialStart = range_mm*roiacos;
    actualRoi.axialSpan  = span_mm*roiacos;

    // Find TX beam set
    latStartTx = actualRoi.azimuthStart+rxtxo;
    latdelta   = actualRoi.axialStart*roiatan;
    ls_beam    = (int)(round((latStartTx-latdelta)/(tinfoPtr->pitch*4))+15); // First TX beam index
    latbeams   = (int)(actualRoi.azimuthSpan/(tinfoPtr->pitch*4));
    le_beam    = ls_beam+latbeams-1; // Last TX beam index

    if (ls_beam<0){
        latover = -1*ls_beam;
        ls_beam = 0;
        le_beam += latover;
    }
    if (le_beam>31){
        latover = le_beam-31;
        ls_beam -= latover;
        le_beam -= latover;
    }
    ls_beam = ls_beam < 0 ? 0 : ls_beam;

    /*no need to do the following
    // Clamp lateral position based on possible TX beam range
    // The maximum C tx beams is 16 for now.
    if (actualRoi.roiA>=0.0)
    {
        if (ls_beam<0){
            latover = -1*ls_beam;
            ls_beam = 0;
            le_beam += latover;
        }
        if (le_beam>31){
            latover = le_beam-31;
            ls_beam -= latover;
            le_beam -= latover;
        }
        ls_beam = ls_beam < 0 ? 0 : ls_beam;
        // Set maximum of 16 TX beams No need to limit since we are loading tx config for color
        //if ((le_beam-ls_beam+1)>16) {
        //    le_beam = ls_beam+15;
        //}
    }
    else {
        if (le_beam>31){
            latover = le_beam-31;
            ls_beam -= latover;
            le_beam -= latover;
        }
        if (ls_beam<0){
            latover = -1*ls_beam;
            ls_beam = 0;
            le_beam += latover;
        }
        // Set maximum of 16 TX beams?
        if ((le_beam-ls_beam+1)>16) {
            ls_beam = le_beam-15;
        }
    }
    */

    // Clamp lateral size based on axial position
    // First get TX and RX beam origin set in mm
    // TODO: this should be in the transducer info..
    // TX pitch is hard coded to 4x the element pitch
    for (int i = 0; i < 32; i++){
        tx_ox[i] = -18.6+i*tinfoPtr->pitch*4;
    }
    for (int i = 0; i < 128; i++){
        rx_ox[i] = -19.05+i*tinfoPtr->pitch;
    }
    tv = (actualRoi.axialStart+actualRoi.axialSpan)*roiatan+rxtxo;
    if (actualRoi.roiA>=0){
        while ((tx_ox[le_beam]+tv>rx_ox[127]) && (le_beam > 0)){
            le_beam -= 1; // Reduce ROI width by 1 TX beam (4 RX beams)
        }

        if (ls_beam > le_beam)
            ls_beam = le_beam;
    }
    else {
        while (tx_ox[ls_beam]+tv<rx_ox[0]){
            ls_beam += 1; // Shift ROI position by 1 TX beam (4 RX beams)
        }

        if (ls_beam > le_beam)
            le_beam = ls_beam;
    }

    uint32_t roi_tx_beams = le_beam-ls_beam+1;
    float roi_as_dx    = actualRoi.axialStart*roiatan;
    // Update actual lateral parameters
    actualRoi.azimuthStart = tx_ox[ls_beam]+actualRoi.axialStart*roiatan-rxtxo;
    actualRoi.azimuthSpan  = roi_tx_beams*tinfoPtr->pitch*4;
    /*
    uint32_t rxIndex=0;
    while (rxIndex < 128)
    {
        if (actualRoi.azimuthStart > rx_ox[rxIndex])
        {
            rxIndex++;
        }
        else
        {
            break;
        }
    }
    actualRoi.azimuthStart = rx_ox[rxIndex];
    ls_beam = rxIndex>>2;
    roi_tx_beams = le_beam-ls_beam+1;
    actualRoi.azimuthSpan  = roi_tx_beams*tinfoPtr->pitch*4;
    */
    // The ROI geometry should now be legal and quantized
    // Save lateral/axial bounds info for use by sequence builder
    ROIBounds.ls_beam = ls_beam;
    ROIBounds.le_beam = le_beam;
    ROIBounds.as_sample = range_s;
    ROIBounds.ae_sample = range_s+span_s-1;
    ROIBounds.tx_beams  = (le_beam-ls_beam)+1;

    mFocalPointInfo.focalDepthMm = actualRoi.axialStart + actualRoi.axialSpan * glbPtr->cTxFocusFraction;
    //

    //APOD POINTER
    {
        mNumTxBeamsC=ROIBounds.tx_beams;
        /*
        for (int i = 0; i < mNumTxBeamsC; i++){
            mApod_offset_seq[i] = 2*(i-((tinfoPtr->numElements/4)-1)/2);
        }
         */
    }

    // initial estimation of mBeamParams.dmStart
    {
        float fs  = glbPtr->samplingFrequency;
        float axs = requestedRoi.axialStart;
        float s   = mBip.speedOfSound;

        mBeamParams.dmStart = (uint32_t)(ceil(fs * 2 * axs/s));
        mBeamParams.dmStart = mBeamParams.dmStart < 0 ? 0 : mBeamParams.dmStart;
    }
    
    // compute mBeamParams.outputSamples
    {
        float os = glbPtr->maxCSamplesPerLine * actualRoi.axialSpan / imagingDepth;

        if (os > MAX_COLOR_SAMPLES_PER_LINE)
            os = MAX_COLOR_SAMPLES_PER_LINE;
        mBeamParams.outputSamples = (uint32_t)(ceil(os/16.0)*16);   // make it a multiple of 16
        actualRoi.axialSpan = imagingDepth*mBeamParams.outputSamples/glbPtr->maxCSamplesPerLine;

        while ((actualRoi.axialSpan+actualRoi.axialStart) > (imagingDepth)) {
            mBeamParams.outputSamples = mBeamParams.outputSamples - 16;
            actualRoi.axialSpan = imagingDepth * mBeamParams.outputSamples / glbPtr->maxCSamplesPerLine;
        }

        while ((actualRoi.axialSpan) > (imagingDepth * (glbPtr->maxRoiAxialSpan))) {
            mBeamParams.outputSamples = mBeamParams.outputSamples - 16;
            actualRoi.axialSpan = imagingDepth * mBeamParams.outputSamples / glbPtr->maxCSamplesPerLine;
        }
    }

    // compute decimation factor
    {
        float mmrs = 1/glbPtr->samplingFrequency * mBip.speedOfSound/2;
        float rx_f_min  = 1.0;
        float RX_AP     = tinfoPtr->pitch * tinfoPtr->numElements;
        float ap_r   = RX_AP * rx_f_min;
        float ap_fdm  = pow(pow((RX_AP/2),2)+pow(ap_r,2),0.5)-ap_r;
        float ap_sdm  = abs((RX_AP/2)*sin(actualRoi.roiA));
        float adc_ed  = ap_fdm > ap_sdm ? ap_fdm : ap_sdm;
        uint32_t adc_es = (uint32_t)(adc_ed/mmrs);
        mBeamParams.adcS = actualRoi.axialSpan / mmrs ;
        mBeamParams.adcS = (uint32_t)(ceil(mBeamParams.adcS/16)*16);
        mBeamParams.bfS = mBeamParams.adcS + 64;
        if  (mBip.imagingDepth <20)
            adc_es = adc_es + 75;
        mBeamParams.adcS = mBeamParams.adcS + adc_es;


        mBeamParams.decimationFactor = (floor(8*(mBeamParams.bfS-64)/mBeamParams.outputSamples)/8);
        if (mBeamParams.decimationFactor > 32)
        {
            float os;
            mBeamParams.decimationFactor = 32;
            os = (floor(8.0*mBeamParams.adcS/512)/8)*512/32;
            if (os > MAX_COLOR_SAMPLES_PER_LINE)
                os = MAX_COLOR_SAMPLES_PER_LINE;
            mBeamParams.outputSamples = (uint32_t)(ceil(os/16)*16);
        }
    }
    int32_t I_R = 16;
    mBeamParams.fpS  = (uint32_t)((mBeamParams.bfS/I_R) + 1);
    calculateTxLinesSpan( requestedPrfIndex, actualRoi, actualPrfIndex, actualRoi ); // actual azimuthSpan gets updated here
    uint32_t numBkptsB;
    uint32_t numBkptsC;
    mUpsReaderSPtr->getNumFocalBkpts( imagingCaseId, numBkptsB, numBkptsC );

    mFocalPointInfo.numBkptsC = mBeamParams.fpS;
    mFocalPointInfo.numBkptsB = numBkptsB;

    // Calculate numGainBreakpoint aka G_S, tgcirLow, tgcirHigh
    {
        uint32_t  breakPoints[2];
        mBeamParams.numGainBreakpoints = (uint32_t)(mBeamParams.outputSamples/16) + 1;

        if (mBeamParams.numGainBreakpoints < 9)
        {
            mBeamParams.numGainBreakpoints = (uint32_t)(mBeamParams.outputSamples/4) + 1;
            mBeamParams.tgcirLow  = 0;
            mBeamParams.tgcirHigh = 0;
        }
        else
        {
            //mUpsReaderSPtr->getColorGainBreakpoints( breakPoints );
            mBeamParams.tgcirLow  = 0x55555555;//breakPoints[0];
            mBeamParams.tgcirHigh = 0x55555555;//breakPoints[1];
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

        // The following call must happen after the calulateTxLineSpan
        calculateGrid( numTxBeamsBC, numTxBeamsBC*4, requestedRoi, actualRoi, rxPitchBC, 4, mNumTxBeamsC, mTxAngles );
        float pitchRad = rxPitchBC;
        mBeamParams.rxAngles[0] = 0;//mTxAngles[0];//-1.5;
        mBeamParams.rxAngles[1] = 0;//mTxAngles[0];//-0.5;
        mBeamParams.rxAngles[2] = 0;//mTxAngles[0];//0.5;
        mBeamParams.rxAngles[3] = 0;//mTxAngles[0];// 1.5;

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

    // bpfSelect
    {
        mBeamParams.bpfSelect = 2;
    }

    {
        uint32_t txCenterOffset = (uint32_t)((glbPtr->txClamp + (0.5 * mBeamParams.txWaveformLength)) * Fs);
        uint32_t lensOffset     = (uint32_t)(tinfoPtr->lensDelay * Fs);
        uint32_t skinOffset     = tinfoPtr->skinAdjustSamples;
        uint32_t rxOffset       = txCenterOffset + lensOffset + skinOffset;
        uint32_t rxStart        = 470 + glbPtr->txRef + rxOffset + mBeamParams.dmStart;

        uint32_t extraRX=61;
        uint32_t hvOffset=5;
        mUpsReaderSPtr->getExtraRX(mBip.depthIndex, extraRX);
        mUpsReaderSPtr->getHvOffset(mBip.depthIndex, hvOffset);
        mBip.rxStart = rxStart-extraRX;
        mBeamParams.hvClkEnOff = (uint32_t)(floor((mBip.rxStart) /16.0))-hvOffset;
        mBeamParams.hvClkEnOn = (uint32_t)(ceil((mBip.rxStart +mBeamParams.adcS)/16.0))+hvOffset;
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
        mTxPri=(uint32_t)(ceil((actualRoi.axialSpan*2*glbPtr->samplingFrequency/mBip.speedOfSound/+50)*glbPtr->samplingFrequency/glbPtr->samplingFrequency)*glbPtr->samplingFrequency);
        //mTxPri = Fs*1e6/(actualPrf*mInterleaveFactor);
    }

    mBeamParams.ccWe  = 1;
    mBeamParams.ccOne = 1;
    return (retVal);
}

void ColorAcqLinear::logBeamParams()
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
void ColorAcqLinear::buildLuts( const uint32_t imagingCaseId, const ScanDescriptor roi )
{
    uint32_t numFocalBkptsB, numFocalBkptsC;
    
    calculateFocalInterpLut( imagingCaseId );
    calculateDgcInterpLut  ( imagingCaseId, roi );
    calculateAtgc          ( imagingCaseId, mBeamParams.bfS, mBeamParams.dmStart, mBip.rxStart );
    calculateTxConfig      ( imagingCaseId, roi, mNumTxBeamsC, mTxAngles );
    calculateBpfLut        ( imagingCaseId );
}


//--------------------------------------------------------------------------------
void ColorAcqLinear::fetchGlobals()
{
    mUpsReaderSPtr->getPrfList(mPrfList, mNumValidPrfs);
}

//--------------------------------------------------------------------------------
void ColorAcqLinear::getImagingCaseConstants(uint32_t imagingCaseId, uint32_t wallFilterType )
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

void ColorAcqLinear::logImagingCaseConstants()
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

void ColorAcqLinear::calculateMaxPrfIndex( ScanDescriptor&      actualRoi,
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
// ColorAcqLinear::calculateTxLinesSpan()
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
void ColorAcqLinear::calculateTxLinesSpan( const uint32_t       requestedPrfIndex,
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
        //mNumTxBeamsC          = (uint32_t)(requestedRoi.azimuthSpan/(mBip.colorRxPitch*4));
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
    /*
    float pricomp = Fs*1.0e6/(actualPrf*mInterleaveFactor)+200;
    uint32_t tempInterleaveFactor = ((uint32_t)(Fs*1.0e6/pricomp)/actualPrf) < 1 ? 1: (uint32_t)(Fs*1.0e6/pricomp)/actualPrf;
    pricomp = ceil(Fs*1.0e6/(actualPrf*tempInterleaveFactor));
    mInterleaveFactor = ((uint32_t)(Fs*1.0e6/pricomp)/actualPrf) < 1 ? 1: (uint32_t)(Fs*1.0e6/pricomp)/actualPrf;
    */
    float tx_ox [32];  // TX beam origins
    for (int i = 0; i < 32; i++){
        tx_ox[i] = -18.6+i*mBip.colorRxPitch*4;
    }
    float rx_ox [128];
    for (int i = 0; i < 128; i++){
        rx_ox[i] = -19.05+i*mBip.colorRxPitch;
    }
    float roiatan   = tan(actualRoi.roiA);
    float rxtxo        = (3.0/1.0)*(mBip.colorRxPitch);
    // Calculate numTxLines, number of groups and actual azimuth span
    {
        if ((actualRoi.roiA<0) and ((uint32_t)(mInterleaveFactor * (uint32_t)ceil((float)mNumTxBeamsC/mInterleaveFactor))!=mNumTxBeamsC)) {
            mNumTxBeamsC = (uint32_t) (mInterleaveFactor *
                                       (uint32_t) ceil((float) mNumTxBeamsC / mInterleaveFactor));
            ROIBounds.ls_beam = ROIBounds.ls_beam-1;
            actualRoi.azimuthStart =
                    tx_ox[ROIBounds.ls_beam] + actualRoi.axialStart * roiatan - rxtxo;
            actualRoi.azimuthSpan = mNumTxBeamsC * mBip.colorRxPitch * 4;
            mNumGroups = (uint32_t) (1.0 * mNumTxBeamsC / mInterleaveFactor);
        }
        else if ((actualRoi.roiA>=0) or ((uint32_t)(mInterleaveFactor * (uint32_t)ceil((float)mNumTxBeamsC/mInterleaveFactor))==mNumTxBeamsC))
            {
            mNumTxBeamsC = (uint32_t) (mInterleaveFactor *
                                       (uint32_t) ceil((float) mNumTxBeamsC / mInterleaveFactor));
            actualRoi.azimuthStart =
                    tx_ox[ROIBounds.ls_beam] + actualRoi.axialStart * roiatan - rxtxo;
            actualRoi.azimuthSpan = mNumTxBeamsC * mBip.colorRxPitch * 4;
            mNumGroups = (uint32_t) (1.0 * mNumTxBeamsC / mInterleaveFactor);
        }
    }
    ScanDescriptor fieldOfView;
    mUpsReaderSPtr->getFov(mBip.depthIndex, fieldOfView, IMAGING_MODE_COLOR);
    float fov = fieldOfView.azimuthSpan;

    if (actualRoi.azimuthSpan < fov*glbPtr->minRoiAzimuthSpan)
    {
        mNumTxBeamsC          = (uint32_t)ceil((fov*glbPtr->minRoiAzimuthSpan)/(mBip.colorRxPitch*4));
        mNumTxBeamsC = (uint32_t)(mInterleaveFactor * (uint32_t)ceil((float)mNumTxBeamsC/mInterleaveFactor));
        //actualRoi.azimuthSpan = mBip.colorRxPitch*4*mNumTxBeamsC;
        actualRoi.azimuthStart = tx_ox[ROIBounds.ls_beam]+actualRoi.axialStart*roiatan-rxtxo;
        actualRoi.azimuthSpan  = mNumTxBeamsC*mBip.colorRxPitch*4;
        mNumGroups = (uint32_t)(1.0*mNumTxBeamsC/mInterleaveFactor);
        /*
        mNumTxBeamsC = mNumTxBeamsC + 1;
        actualRoi.azimuthSpan = mBip.colorRxPitch*4*mNumTxBeamsC;
        mNumGroups = (uint32_t)(1.0*mNumTxBeamsC/mInterleaveFactor);
         */
    }
    if ( (actualRoi.azimuthSpan > fov*glbPtr->maxRoiAzimuthSpan) || ( (actualRoi.azimuthSpan + actualRoi.azimuthStart) > fov/2.0f) )
    {
        if ((uint32_t)(requestedRoi.azimuthSpan/(mBip.colorRxPitch*4)) > mInterleaveFactor)
            mNumTxBeamsC          = (uint32_t)(requestedRoi.azimuthSpan/(mBip.colorRxPitch*4)) - mInterleaveFactor;

        mNumTxBeamsC = (uint32_t)(mInterleaveFactor * (uint32_t)ceil((float)mNumTxBeamsC/mInterleaveFactor));
        //actualRoi.azimuthSpan = mBip.colorRxPitch*4*mNumTxBeamsC;
        actualRoi.azimuthStart = tx_ox[ROIBounds.ls_beam]+actualRoi.axialStart*roiatan-rxtxo;
        actualRoi.azimuthSpan  = mNumTxBeamsC*mBip.colorRxPitch*4;
        mNumGroups = (uint32_t)(1.0*mNumTxBeamsC/mInterleaveFactor);
        /*
        mNumTxBeamsC = mNumTxBeamsC - 1;
        actualRoi.azimuthSpan = mBip.colorRxPitch*4*mNumTxBeamsC;
        mNumGroups = (uint32_t)(1.0*mNumTxBeamsC/mInterleaveFactor);
         */
    }
    mBip.actualPrfIndex = actualPrfIndex;
    ROIBounds.tx_beams = mNumTxBeamsC ;
    ROIBounds.le_beam = mNumTxBeamsC + ROIBounds.ls_beam -1;
    if (ROIBounds.le_beam > 31)
    {
        ROIBounds.le_beam = 31;
        mNumTxBeamsC = ROIBounds.le_beam - ROIBounds.ls_beam + 1;
        if ((uint32_t)(mInterleaveFactor * (uint32_t)ceil((float)mNumTxBeamsC/mInterleaveFactor))!=mNumTxBeamsC)
        {
            mNumTxBeamsC = (uint32_t) (mInterleaveFactor *
                                       (uint32_t) ceil((float) mNumTxBeamsC / mInterleaveFactor));
            ROIBounds.ls_beam = ROIBounds.le_beam - mNumTxBeamsC + 1;
            actualRoi.azimuthStart =
                    tx_ox[ROIBounds.ls_beam] + actualRoi.axialStart * roiatan - rxtxo;
            actualRoi.azimuthSpan = mNumTxBeamsC * mBip.colorRxPitch * 4;
            mNumGroups = (uint32_t) (1.0 * mNumTxBeamsC / mInterleaveFactor);
        }
        ROIBounds.tx_beams = mNumTxBeamsC;
        actualRoi.azimuthSpan = mNumTxBeamsC * mBip.colorRxPitch * 4;
        mNumGroups = (uint32_t) (1.0 * mNumTxBeamsC / mInterleaveFactor);
    }
    if (actualRoi.azimuthStart<rx_ox[0])
        actualRoi.azimuthStart = rx_ox[0];
}

//--------------------------------------------------------------------------------
void ColorAcqLinear::calculateGrid( uint32_t numTxBeams,
                              uint32_t numRxBeams,
                              const ScanDescriptor requestedRoi,
                              ScanDescriptor& actualRoi,
                              float pitch,
                              uint32_t NP,
                              uint32_t numTxBeamsC,
                              float txAngles[])
{
    // TX beam angles for linear are all the same
    for (uint32_t i = 0; i < numTxBeams; i++){
        txAngles[i] = actualRoi.roiA;
    }

}


//--------------------------------------------------------------------------------
void ColorAcqLinear::buildSequenceBlob( uint32_t imagingCaseId )
{
    buildSequenceBlob( imagingCaseId, mSequenceBlob );
}

//--------------------------------------------------------------------------------
void ColorAcqLinear::buildSequenceBlob( uint32_t imagingCaseId, uint32_t colorBlob[CBLOB_SIZE_LINEAR][62] )
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
    uint32_t pi;
    initCtbParams( ctbInput, ctbOutput );

    for (uint32_t gi = 0; gi < mNumGroups; gi++)
    {
        if (gi == 0)
        {
            //txdWordCount=mTxdWc2+mTxdWc;
            // Dummies at the beginning of color sequence
            flags.txEn = true; flags.pmAfePwrSb = true;
            flags.rxEn = false; flags.bfEn = false; flags.cfpEn = false; flags.outEn = false; flags.bpSlopeEn = false;
            txPingIndex = 0;
            bgPtr = 0;
            // First DUMMY After B BEGIN
            for (uint32_t di = 0; di < glbPtr->firstDummyVector; di++)  // loop A
            {
                //
                txdWordCount=mTxdWc;
                if (di==0)
                    txdWordCount=mTxdWc2+mTxdWc;
                setupTxAngle( txAngle, gi, 0, 0 );
                setupTxdSpiPtr( txdSpiPtr, gi, 0, 0 );

                flags.pmAfePwrSb = true;
                pi = ROIBounds.ls_beam;
                setupCtbParams( ctbInput, ctbOutput, gi, 0, 0);
                setupBpRegs( flags, txdSpiPtr, txdWordCount, txAngle, ctbInput, ctbOutput, txPingIndex, pri, bgPtr, colorBlob[mFiringCount], pi );
                mFiringCount++;
            }

            // add a set of dummies per group
            // Other Dummy BEGIN
            txdWordCount = mTxdWc;
            for (uint32_t ii=1; ii < mInterleaveFactor; ii++)  // loop B
            {
                for (uint32_t di = 0; di < glbPtr->dummyVector; di++)  // loop A
                {
                    setupTxAngle(txAngle, gi, 0, ii);
                    setupTxdSpiPtr(txdSpiPtr, gi, 0, ii);
                    pi = ROIBounds.ls_beam + (gi * mInterleaveFactor) + ii;
                    setupCtbParams(ctbInput, ctbOutput, gi, 0, 0);
                    setupBpRegs(flags, txdSpiPtr, txdWordCount, txAngle, ctbInput, ctbOutput, txPingIndex, pri, bgPtr, colorBlob[mFiringCount], pi);
                    mFiringCount++;
                }
            }

            //  Actual ENS BEGIN, First group, no color processing/output
            bgPtr = 30;
            flags.bpSlopeEn = true;
            flags.txEn = true; flags.rxEn = true; flags.bfEn = true; flags.ctbwEn = true;
            flags.pmAfePwrSb = false;
            for (uint32_t ei = 0; ei < mBeamParams.ensembleSize; ei++)  // loop C
            {
                //
                for (uint32_t ii = 0; ii < mInterleaveFactor; ii++)
                {
                    //////////////////////////////////
                    /*
                    if (ii==0)
                        txdWordCount = mTxdWc2+mTxdWc;
                    else
                        txdWordCount = mTxdWc;


                    if (ei==0 and ii==0)
                        txdWordCount = mTxdWc2+mTxdWc;
                    else
                        txdWordCount = mTxdWc;
                    */
                    txdWordCount = mTxdWc;
                    ////////////////////////////////
                    setupTxPingIndex( txPingIndex, gi, ei, ii );
                    setupTxAngle( txAngle, gi, ei, ii );
                    setupTxdSpiPtr( txdSpiPtr, gi, ei, ii );
                    setupCtbParams( ctbInput, ctbOutput, gi, ei, ii);
                    ctbOutput.innerPtr = 0;
                    pi = ROIBounds.ls_beam+(gi*mInterleaveFactor) + ii;
                    setupBpRegs( flags, txdSpiPtr, txdWordCount, txAngle, ctbInput, ctbOutput, txPingIndex, pri, bgPtr, colorBlob[mFiringCount], pi );
                    mFiringCount++;
                }
            }
        }
        else
        {
            txdWordCount = mTxdWc;
            // add a set of dummies per group
            flags.txEn = true; flags.rxEn = false; flags.bfEn = false; flags.ctbwEn = false; flags.outEn  = false; flags.bpSlopeEn = false;
            flags.pmAfePwrSb = true;
            bgPtr = 0;
            txPingIndex = 0;
            for (uint32_t ii=0; ii < mInterleaveFactor; ii++)  // loop D
            {
                for (uint32_t di = 0; di < glbPtr->dummyVector; di++)  // loop A
                {
                    setupTxAngle(txAngle, gi, 0, ii);
                    setupTxdSpiPtr(txdSpiPtr, gi, 0, ii);
                    setupCtbParams(ctbInput, ctbOutput, 0, 0, 0);
                    pi = ROIBounds.ls_beam + (gi * mInterleaveFactor) + ii;
                    setupBpRegs(flags, txdSpiPtr, txdWordCount, txAngle, ctbInput, ctbOutput, txPingIndex, pri, bgPtr, colorBlob[mFiringCount], pi);
                    mFiringCount++;
                }
            }

            flags.rxEn = true; flags.bfEn = true; flags.ctbwEn = true; flags.outEn = true; flags.cfpEn = true; flags.bpSlopeEn = true;
            flags.pmAfePwrSb = false;
            bgPtr = 30;
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
                    pi = ROIBounds.ls_beam+(gi*mInterleaveFactor) + ii;
                    setupBpRegs( flags, txdSpiPtr, txdWordCount, txAngle, ctbInput, ctbOutput, txPingIndex, pri, bgPtr, colorBlob[mFiringCount], pi );
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
                    pi = ROIBounds.ls_beam+(gi*mInterleaveFactor) + ii;
                    setupBpRegs( flags, txdSpiPtr, txdWordCount, txAngle, ctbInput, ctbOutput, txPingIndex, pri, bgPtr, colorBlob[mFiringCount], pi );
                    mFiringCount++;
                }
            }
        }
    }

    // Flush out CTB pipeline
    flags.txEn = false; flags.rxEn = false; flags.bfEn = false; flags.ctbwEn = false; flags.cfpEn = true; flags.outEn = true;
    flags.pmAfePwrSb = true;
    txdWordCount = mTxdWc;
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
            pi = ROIBounds.ls_beam+((mNumGroups-1)*mInterleaveFactor) + ii;//(mNumGroups-1)*mInterleaveFactor + ii;
            setupBpRegs( flags, txdSpiPtr, txdWordCount, txAngle, ctbInput, ctbOutput, txPingIndex, pri, bgPtr, colorBlob[mFiringCount], pi );
            mFiringCount++;
        }
    }
}

void ColorAcqLinear::calculateFocalInterpLut( const uint32_t imagingCaseId )
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

    uint32_t I_R=16;

    for (uint32_t  bb = 0; bb<mBeamParams.fpS; bb++)
    {
        //mBeam0XCRoiMax[bb] = floor((16*floor(mBeamParams.dmStart/16)+(bb * (I_R))) * (1<<5));
        mBeam0XCRoiMax[bb] = floor((mBeamParams.dmStart+(bb * (I_R))) * (1<<5));
        mBeam0YCRoiMax[bb] = 0;
        mBeam1YCRoiMax[bb] = 0;
        mBeam2YCRoiMax[bb] = 0;
        mBeam3YCRoiMax[bb] = 0;
    }

    {
        memcpy( &(mLutB0FPX[TBF_LUT_B0FPX_SIZE/2]), &(mBeam0XCRoiMax), sizeof(int32_t)*numBkptsC );
        memcpy( &(mLutB0FPY[TBF_LUT_B0FPY_SIZE/2]), &(mBeam0YCRoiMax), sizeof(int32_t)*numBkptsC );
        memcpy( &(mLutB1FPY[TBF_LUT_B1FPY_SIZE/2]), &(mBeam1YCRoiMax), sizeof(int32_t)*numBkptsC );
        memcpy( &(mLutB2FPY[TBF_LUT_B2FPY_SIZE/2]), &(mBeam2YCRoiMax), sizeof(int32_t)*numBkptsC );
        memcpy( &(mLutB3FPY[TBF_LUT_B3FPY_SIZE/2]), &(mBeam3YCRoiMax), sizeof(int32_t)*numBkptsC );
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
            uint32_t codeWord = 0x00000000;

            for (uint32_t j = 0; j < 32; j=j+4)
            {
                tmpColorCodes[ind] = (codeWord >> j) & 0xf;
                ind++;
            }
        }

        ind = TBF_LUT_MSDIS_SIZE/2;//bBkptsWords;
        for (uint32_t i = 0; i < (numBkptsC)*8; i=i+8)
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
        for (uint32_t i = 0; i < mBeamParams.fpS; i++)
        {
            mLutMSB0APS[halfLutSize + i] = 1024;
        }
    }
}



void ColorAcqLinear::calculateDgcInterpLut( const uint32_t imagingCaseId, ScanDescriptor roi )
{
    uint32_t intBits;
    uint32_t fracBits;
    mUpsReaderSPtr->getGainDgcParams( imagingCaseId,
                                      intBits,
                                      fracBits,
                                      mLutMSDBG01,
                                      mLutMSDBG23,
                                      mGainValues01CRoiMax,
                                      mGainValues23CRoiMax );

    uint32_t offset = 30*32;

    float depth = roi.axialStart + roi.axialSpan;
    float StartGain = 0.1*roi.axialStart * mBip.txCenterFrequency;
    float StopGain = 0.1*depth * mBip.txCenterFrequency;
    float maxGain = 40.0;
    float zeroDGC = 1.7;

    int32_t dgclut[32];
    float totalGain = 0;
    mUpsReaderSPtr->getZeroDGC(mBip.depthIndex, zeroDGC);
    for (uint32_t i = 0; i < mBeamParams.numGainBreakpoints; i++)
    {
        if (imagingCaseId >= 2042)
            totalGain = 14+StartGain+i*(StopGain-StartGain)/(mBeamParams.numGainBreakpoints-1);
        else
            totalGain = 14+StartGain+i*(StopGain-StartGain)/(mBeamParams.numGainBreakpoints-1);
        //totalGain = 20.0;
        totalGain = totalGain > 20 ? 20 : totalGain;
        if ((roi.axialStart+i*(depth-roi.axialStart)/(mBeamParams.numGainBreakpoints-1))<=zeroDGC)
            totalGain = 0;
        int32_t beamgain = (totalGain/maxGain*511);
        dgclut[i] = beamgain + beamgain*(1<<9);
    }

    for (int i = 0; i < mBeamParams.numGainBreakpoints; i++)
    {
        mLutMSDBG01[offset + i] = dgclut[i];
        mLutMSDBG23[offset + i] = dgclut[i];
    }
}

//-----------------------------------------------------------------------------------------------------------------------
// This method computes values of the LUT MSRTAFE
#define MAX_AFE_GAIN_STEPS 320

void ColorAcqLinear::calculateAtgc( const uint32_t imagingCaseId,
                              const uint32_t bfS,
                              const uint32_t dmStart,
                              const uint32_t rxStart )
{
    uint32_t index = 0;


    const UpsReader::Globals *glbPtr = mUpsReaderSPtr->getGlobalsPtr();
    const UpsReader::TransducerInfo *transducerInfoPtr = mUpsReaderSPtr->getTransducerInfoPtr();

    uint32_t c_rx_ap = mBip.colorRxAperture;

    uint32_t txApertureElement;
    float    centerfreq;
    uint32_t numhalfcycle;
    mUpsReaderSPtr->getColorWaveformInfo( imagingCaseId,
                                          txApertureElement,
                                          centerfreq,
                                          numhalfcycle);

    float fs = glbPtr->samplingFrequency;
    uint32_t txStart = glbPtr->txStart;

    uint32_t powerMode;
    float    maxGainC;
    float    minGainC;
    mUpsReaderSPtr->getGainAtgcParams( mBip.depthIndex,
                                       powerMode,
                                       maxGainC,
                                       minGainC );
    //if (imagingCaseId >= 2042 and imagingCaseId<=2047)
    //    maxGainC = 16.0;
    uint32_t bitset;
    mUpsReaderSPtr->getAfeInputResistance( glbPtr->afeResistSelectId, bitset );

    uint32_t gainmode = glbPtr->afeGainSelectIdC;
    float    gainval;
    mUpsReaderSPtr->getAfeGainModeParams( gainmode, gainval);

    float freqScale = centerfreq / 10.0;
    float max_atgc_depth = maxGainC/freqScale;
    uint32_t max_adc_s = (uint32_t)( 2 * fs * max_atgc_depth/mBip.speedOfSound ) - rxStart;

    uint32_t adc_samples = bfS + 224 + 64 - 1;  // 224 is Todd's variable, 64 is BPF preload
    uint32_t dtgc_adc_samples = max_adc_s > adc_samples ? adc_samples : max_adc_s;
    float gstart = minGainC + (dmStart*(0.5 * mBip.speedOfSound)*freqScale/fs);
    mLutMSRTAFE[index++] = formAfeWord(0, 0x10);  // Enables programming of the DTGC register map
    mLutMSRTAFE[index++] = formAfeWord(181, 0x0);  // manual mode setup
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
      //  int32_t start_gain = (int32_t) (4.0 * (gstart - minGainC));
        //int32_t stop_gain = (int32_t) (4.0 * (g_last  - minGainC));
        int32_t start_gain = (int32_t) (4.0 * (gstart));
        int32_t stop_gain = (int32_t) (4.0 * (g_last));
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
        mLutMSRTAFE[index++] = formAfeWord(199, 0x0500);  //0000 0001 1000 0000 HPF 75 KHz, LPF 10 MHz
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
void ColorAcqLinear::calculateTxConfig( const uint32_t imagingCaseId,
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
    uint32_t numparallel = 4;
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

    // e_first and e_last are beam dependent for linear

    uint32_t tx_apw     = ctxele;
    //uint32_t tx_e_first = (uint32_t)(floor(32 - 0.5*tx_apw));
    //uint32_t tx_e_last  = tx_e_first + tx_apw - 1;

    int32_t referenceTime104MHz = 512;
	float    btxp  =   rxpitch * numparallel;
    uint32_t btxbc =  (uint32_t)(fov / btxp); // Maximum possible B TX beams
    uint32_t ctxbc =  (uint32_t)(fov / btxp); // Maximum possible C TX beams

    // TODO element positions should be from transducer info
    float    elfp[128][2];
    {
        float initVal = -1.0 *(128 - 1)*pitch/2;
        for (uint32_t eleIndex = 0; eleIndex < els; eleIndex++)
        {
            elfp[eleIndex][0] = initVal + (eleIndex * pitch);
            elfp[eleIndex][1] = 0.0;
        }
    }


    uint32_t halfcycles9_6ns = (uint32_t)(txfs/(bcenterfrequency*2)) - 2;
    //
    // C beam TX configuration
    //
    uint32_t index = 0;

    //-----------  Base config begin ------(88 LUT words)
    for (int di=0; di<8; di++){
        mLutMSSTTXC[index++] = 0x00201000 + (di<<16);
        mLutMSSTTXC[index++] = 0x00000000;
        mLutMSSTTXC[index++] = 0x00000000;
        mLutMSSTTXC[index++] = 0x0000030C;
        mLutMSSTTXC[index++] = 0x000000F7;
        mLutMSSTTXC[index++] = 0x00000000;
        mLutMSSTTXC[index++] = 0x0000000A;
        mLutMSSTTXC[index++] = 0x0000FFFF;
        mLutMSSTTXC[index++] = 0x0000FFFF;
        mLutMSSTTXC[index++] = 0x00002002;
        mLutMSSTTXC[index++] = 0x00401e00;
    }

    // ser_trigger_tx
    {
        mLutMSSTTXC[index++] = 0x00201008;
        mLutMSSTTXC[index++] = 0x00400003;
        mLutMSSTTXC[index++] = 0x00201008;
        mLutMSSTTXC[index++] = 0x00400002;
        mLutMSSTTXC[index++] = 0x00211008;
        mLutMSSTTXC[index++] = 0x00400003;
        mLutMSSTTXC[index++] = 0x00211008;
        mLutMSSTTXC[index++] = 0x00400002;

        mLutMSSTTXC[index++] = 0x00221008;
        mLutMSSTTXC[index++] = 0x00400003;
        mLutMSSTTXC[index++] = 0x00221008;
        mLutMSSTTXC[index++] = 0x00400002;

        mLutMSSTTXC[index++] = 0x00231008;
        mLutMSSTTXC[index++] = 0x00400003;
        mLutMSSTTXC[index++] = 0x00231008;
        mLutMSSTTXC[index++] = 0x00400002;

        mLutMSSTTXC[index++] = 0x00241008;
        mLutMSSTTXC[index++] = 0x00400003;
        mLutMSSTTXC[index++] = 0x00241008;
        mLutMSSTTXC[index++] = 0x00400002;

        mLutMSSTTXC[index++] = 0x00251008;
        mLutMSSTTXC[index++] = 0x00400003;
        mLutMSSTTXC[index++] = 0x00251008;
        mLutMSSTTXC[index++] = 0x00400002;
        mLutMSSTTXC[index++] = 0x00261008;
        mLutMSSTTXC[index++] = 0x00400003;
        mLutMSSTTXC[index++] = 0x00261008;
        mLutMSSTTXC[index++] = 0x00400002;

        mLutMSSTTXC[index++] = 0x00271008;
        mLutMSSTTXC[index++] = 0x00400003;
        mLutMSSTTXC[index++] = 0x00271008;
        mLutMSSTTXC[index++] = 0x00400002;
    }

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

        //--------- B waveform begin -------- (128 LUT words)

        halfcycles9_6ns = (uint32_t)(txfs/(bcenterfrequency*2)) - 2;
        hvpWord  = 0x00000000 | hvp0  | (halfcycles9_6ns << 4);
        hvmWord  = 0x00000000 | hvm0  | (halfcycles9_6ns << 4);
        for (uint32_t i = 0; i < 8; i++) // TX die
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

        //--------- Color waveform begin -------- (128 LUT words)
        /*
        halfcycles9_6ns = (uint32_t)(txfs/(ccenterfrequency*2)) - 2;
        hvpWord  = 0x00000000 | hvp0  | (halfcycles9_6ns << 4);
        hvmWord  = 0x00000000 | hvm0  | (halfcycles9_6ns << 4);
        for (uint32_t i = 0; i < 8; i++) // TX die
        {
            mLutMSSTTXC[index++] = 0x00200007 + 0x10000*i;
            mLutMSSTTXC[index++] = postWord;
            for (uint32_t j = 0; j < chalfcycles/2; j++)
            {
                mLutMSSTTXC[index++] = hvpWord;
                mLutMSSTTXC[index++] = hvmWord;
            }
            mLutMSSTTXC[index++] = preWord;
            mLutMSSTTXC[index++] = 0x00400000 | cl_wt_rx; // 0x0040000E
        }
        */

        halfcycles9_6ns = (uint32_t)(txfs/(ccenterfrequency*2)) - 2;
        uint32_t baseAddr = 0x00004000 | (int(chalfcycles/2-2) << 11);
        hvpWord  = baseAddr   | hvp0  | (halfcycles9_6ns << 4);
        hvmWord  = 0x00000000 | hvm0  | (halfcycles9_6ns << 4);
        for (uint32_t i = 0; i < 8; i++) // TX die
        {
            mLutMSSTTXC[index++] = 0x00200007 + 0x10000*i;
            mLutMSSTTXC[index++] = postWord;
            mLutMSSTTXC[index++] = hvpWord;
            mLutMSSTTXC[index++] = hvmWord;
            mLutMSSTTXC[index++] = preWord;
            mLutMSSTTXC[index++] = 0x00400000 | cl_wt_rx; // 0x0040000E
        }
    }

    for (uint32_t i = 0; i < 8; i++) // TX die
    {
        mLutMSSTTXC[index++] = 0x0020100A + (i<<16);
        for (uint32_t j = 0; j < 15; j++)
        {
            mLutMSSTTXC[index++] = 0x200;
        }
        mLutMSSTTXC[index++] = 0x400200;
    }
    mBip.baseConfigLength = index;
    mLutMSSTTXC[index++] = 0x00201003;
    mLutMSSTTXC[index++] = 0x004000F3;
    mLutMSSTTXC[index++] = 0x00211003;
    mLutMSSTTXC[index++] = 0x004000F3;
    mLutMSSTTXC[index++] = 0x00221003;
    mLutMSSTTXC[index++] = 0x004000F3;
    mLutMSSTTXC[index++] = 0x00231003;
    mLutMSSTTXC[index++] = 0x004000F3;
    mLutMSSTTXC[index++] = 0x00241003;
    mLutMSSTTXC[index++] = 0x004000F3;
    mLutMSSTTXC[index++] = 0x00251003;
    mLutMSSTTXC[index++] = 0x004000F3;
    mLutMSSTTXC[index++] = 0x00261003;
    mLutMSSTTXC[index++] = 0x004000F3;
    mLutMSSTTXC[index++] = 0x00271003;
    mLutMSSTTXC[index++] = 0x004000F3;
    //------------- PM line, clear all interrupts, low power mode
    /*
    {
        mLutMSSTTXC[index++] = 0x00201003;
        mLutMSSTTXC[index++] = 0x004000F3;
        mLutMSSTTXC[index++] = 0x00211003;
        mLutMSSTTXC[index++] = 0x004000F3;
        mLutMSSTTXC[index++] = 0x00221003;
        mLutMSSTTXC[index++] = 0x004000F3;
        mLutMSSTTXC[index++] = 0x00231003;
        mLutMSSTTXC[index++] = 0x004000F3;
        mLutMSSTTXC[index++] = 0x00241003;
        mLutMSSTTXC[index++] = 0x004000F3;
        mLutMSSTTXC[index++] = 0x00251003;
        mLutMSSTTXC[index++] = 0x004000F3;
        mLutMSSTTXC[index++] = 0x00261003;
        mLutMSSTTXC[index++] = 0x004000F3;
        mLutMSSTTXC[index++] = 0x00271003;
        mLutMSSTTXC[index++] = 0x004000F3;

        mLutMSSTTXC[index++] = 0x00201008;
        mLutMSSTTXC[index++] = 0x00404102;
        mLutMSSTTXC[index++] = 0x00211008;
        mLutMSSTTXC[index++] = 0x00404102;
        mLutMSSTTXC[index++] = 0x00221008;
        mLutMSSTTXC[index++] = 0x00404102;
        mLutMSSTTXC[index++] = 0x00231008;
        mLutMSSTTXC[index++] = 0x00404102;
        mLutMSSTTXC[index++] = 0x00241008;
        mLutMSSTTXC[index++] = 0x00404102;
        mLutMSSTTXC[index++] = 0x00251008;
        mLutMSSTTXC[index++] = 0x00404102;
        mLutMSSTTXC[index++] = 0x00261008;
        mLutMSSTTXC[index++] = 0x00404102;
        mLutMSSTTXC[index++] = 0x00271008;
        mLutMSSTTXC[index++] = 0x00404102;
    }
     */

    {
        // Note: calculating parameters for only the TX beams in the ROI but allocating the full possible 32
        float    ctxfp[32][2];         // Focal points(x & y), move to class member
        int32_t ctxtxap[32][2];       // RX aperture first/last element
        uint32_t ctxrxen[32][8] = {0}; // TX chip RXEN words
        uint32_t ctxtxen[32][8] = {0}; // TX chip TXEN words
        float    ctxa[32] = {0};       // All B beams in BC sequence are angle 0
        int32_t  ctxfdel[32][128]={0};     // Delay table for every Tx beam and every element
        int32_t  ctxfdelburst[32][8][16]={0};     // Delay table for every Tx beam and every element
        //
        // C transmit aperture
        //
        // Note: using the B TX aperture for C beams also
        /*
        for (int bi = ROIBounds.ls_beam; bi<ROIBounds.le_beam+1; bi++)
        { // C TX beams in ROI
            ctxtxap[bi][0] = bi*2-(floor(tx_apw/2)); // TX aperture first element
            ctxtxap[bi][1] = bi*2+(floor(tx_apw/2)); // TX aperture last element
            // Shift to valid range
            if (ctxtxap[bi][0]<0){
                ctxtxap[bi][1] += -1*ctxtxap[bi][0];
                ctxtxap[bi][0] = 0;
            }
            if (ctxtxap[bi][0]>els-1){
                ctxtxap[bi][0] -= (ctxtxap[bi][1]-(els-1));
                ctxtxap[bi][1] = els-1;
            }
            //
            // C TXEN words
            //
            for (uint32_t ai = ctxtxap[bi][0]; ai < ctxtxap[bi][1]+1; ai++){
                // Set TX enable bit for chip/channel
                ctxtxen[bi][chip_map[ai]] += 1<<channel_map[ai];
            }
        }
         */
        //if (roi.axialStart+glbPtr->cTxFocusFraction*roi.axialSpan<=10)
        //    tx_apw = 16;
        //else if (roi.axialStart+glbPtr->cTxFocusFraction*roi.axialSpan<=20)
        //    tx_apw = 32;

        float    tx_ox2[32];
        for (int i = 0; i < 32; i++){
            tx_ox2[i] = -18.6+i*0.3*4;
        }
        float    elfp2[128];
        {
            float initVal = -1.0 *(128 - 1)*pitch/2;
            for (uint32_t eleIndex = 0; eleIndex < els; eleIndex++)
            {
                elfp2[eleIndex] = initVal + (eleIndex * pitch);
            }
        }
        bool  txap_en[32][128]={false};
        for (int tt = ROIBounds.ls_beam; tt<ROIBounds.le_beam+1; tt++)
        {
            float tx_ap_mm = tx_apw * 0.3;
            float txap_min = -(tx_ap_mm / 2);
            int txap_max = -txap_min;
            float txapb_low = tx_ox2[tt] + txap_min;
            float txapb_high;
            if (txapb_low <= elfp2[0])
            {
                //txapb_low = elfp2[0];
                txapb_high = txapb_low + tx_ap_mm;
            }
            else
            {
                txapb_high = txapb_low + tx_ap_mm;
            }
            if (txapb_high >= elfp2[127])
            {
                //txapb_high = elfp2[127];
                txapb_low = txapb_high - tx_ap_mm;
            }
            float txap_low_bound = txapb_low;
            float txap_high_bound = txapb_high;
            bool tx_el_min_found = false;
            for (int ee = 0; ee < 128; ee++)
            {
                if ((elfp2[ee] >= txap_low_bound) and (elfp2[ee] <= txap_high_bound))
                {
                    txap_en[tt][ee] = true;
                    if (tx_el_min_found == false)
                        tx_el_min_found = true;

                }
            }
        }

        for (int ii = ROIBounds.ls_beam; ii<ROIBounds.le_beam+1; ii++)
        {
            int ee=0;
            int startind=0;
            while (txap_en[ii][ee]==false)
            {
                ee = ee + 1;
                if (txap_en[ii][ee] == true or ee == 128)
                {
                    startind = ee;
                    break;
                }
            }
            ee=127;
            int stopind=127;
            while (txap_en[ii][ee]==false)
            {
                ee = ee - 1;
                if (txap_en[ii][ee] == true or ee == 0)
                {
                    stopind = ee;
                    break;
                }
            }
            ctxtxap[ii][0]=startind;
            ctxtxap[ii][1]=stopind;
            //
            // C TXEN words
            //
            for (uint32_t ai = ctxtxap[ii][0]; ai < ctxtxap[ii][1]+1; ai++)
            {
                // Set TX enable bit for chip/channel
                ctxtxen[ii][chip_map[ai]] += 1<<channel_map[ai];
            }
        }
        //
        // C transmit focal points
        //
        float ctxfp_xdelta; // Lateral offset from TX origin due to steering
        float ctxfp_ydelta; // Axial offset from TX origin due to steering
        float ctx_xorigin;  // C TX origin X
        float ctxfd =roi.axialStart+glbPtr->cTxFocusFraction*roi.axialSpan;
        ctxfp_xdelta = ctxfd*sin(roi.roiA);
        ctxfp_ydelta = ctxfd*cos(roi.roiA);
        float tx_beam_pitch = 4.0;
        for (int bi = ROIBounds.ls_beam; bi<ROIBounds.le_beam+1; bi++) // C TX beams in ROI
        {
            ctx_xorigin = (float) (bi - ((els / tx_beam_pitch) - 1) / 2.0) * pitch * tx_beam_pitch;
            // Focal point X
            ctxfp[bi][0] = ctx_xorigin + ctxfp_xdelta;
            // Focal point Y
            ctxfp[bi][1] = ctxfp_ydelta;
        }
        //
        // C Transmit delays
        //
        float k = -txfs/c;
        uint32_t offset = referenceTime104MHz;

        for (int bi = ROIBounds.ls_beam; bi<ROIBounds.le_beam+1; bi++) // C TX beams in ROI
        {
            rxDelay( elfp, els, ctxfp[bi], ctxfd, k, offset, ctxfdel[bi]);
            // Set delay for element outside aperture to reference delay
            for (int ei = 0; ei<els; ei++)
            {
                if ((ei<ctxtxap[bi][0]) or (ei>ctxtxap[bi][1]))
                {
                    ctxfdel[bi][ei] = offset;
                }
            }
        }
        for (int bi = ROIBounds.ls_beam; bi<ROIBounds.le_beam+1; bi++)
        {
            for (int te = 0; te < els; te++)
            {
                uint32_t te_chip = chip_map[te];
                uint32_t te_ch = channel_map[te];
                ctxfdelburst[bi][te_chip][te_ch] = ctxfdel[bi][te];
            }
        }


        ctxbc=ROIBounds.le_beam-ROIBounds.ls_beam+1;
        //
        // C RXEN words
        //
        for (int bi = ROIBounds.ls_beam; bi<ROIBounds.le_beam+1; bi++){ // C TX beams in ROI
            for (int ro=0; ro<64; ro++) { // AFE RX channels
                 //if (ro<el_ptr_seq[31-bi])
                if ((ro<el_ptr_seq[bi]) and ((el_ptr_seq[bi]+64-1)>64))
                {
                    ctxrxen[bi][chip_map[ro+64]] += 1<<(channel_map[ro+64]);
                }
                else {
                    ctxrxen[bi][chip_map[ro]] += 1<<(channel_map[ro]);
                }
            }
        }

        //
        // C TX configuration per beam (160 LUT words per beam)
        //

        for (int bi = ROIBounds.ls_beam; bi<ROIBounds.le_beam+1; bi++) // C TX beams in ROI
        {
            TXLUTinfo.ctxconfig_ptr[bi-ROIBounds.ls_beam] = index;
            mTxdWc=0;
            /*
            for (int di = 0; di<8; di++)
            {
                for (int ci = 0; ci < 16; ci++)
                {
                    //if ((bi==ROIBounds.ls_beam) or (ctxfdelburst[bi-1][di][ci]!=ctxfdelburst[bi][di][ci]))
                    //{
                        mLutMSSTTXC[index++] = 0x0020100A + ci + (di << 16);
                        mTxdWc++;
                        mLutMSSTTXC[index++] = 0x00400000 | ctxfdelburst[bi][di][ci];
                        mTxdWc++;
                }
            }


            // RXEN and TXEN, two word burst
            for (int di = 0; di<8; di++){
                mLutMSSTTXC[index++] = 0x00201006 + (di<<16);
                mTxdWc++;
                mLutMSSTTXC[index++] = ctxtxen[bi][di] + 0x00400000;
                mTxdWc++;
                mLutMSSTTXC[index++] = 0x00201007 + (di<<16);
                mTxdWc++;
                mLutMSSTTXC[index++] = ctxrxen[bi][di] + 0x00400000;
                mTxdWc++;
            }
            */


            // Delay burst
            for (int di = 0; di<8; di++){
                mLutMSSTTXC[index++] = 0x0020100A + (di<<16);
                mTxdWc++;
                for (int ci = 0; ci<15; ci++){
                    mLutMSSTTXC[index++] = ctxfdel[bi][die_map[16*di+ci]];
                    mTxdWc++;
                }
                mLutMSSTTXC[index++] = 0x00400000 | ctxfdel[bi][die_map[16*di+15]];
                mTxdWc++;
            }
            // RXEN and TXEN, two word burst
            for (int di = 0; di<8; di++) {
                mLutMSSTTXC[index++] = 0x00201006 + (di << 16);
                mTxdWc++;
                mLutMSSTTXC[index++] = ctxtxen[bi][di];
                mTxdWc++;
                mLutMSSTTXC[index++] = ctxrxen[bi][di] + 0x00400000;
                mTxdWc++;
            }

            /*
            for (int di = 0; di<8; di++)
            {
                for (int ci = 0; ci < 16; ci++)
                {
                    mLutMSSTTXC[index++] = 0x0020100A + ci + (di << 16);
                    mTxdWc++;
                    mLutMSSTTXC[index++] = 0x00400000 | ctxfdelburst[bi][di][ci];
                    mTxdWc++;
                }
            }

            // RXEN and TXEN, two word burst
            for (int di = 0; di<8; di++) {
                mLutMSSTTXC[index++] = 0x00201006 + (di << 16);
                mTxdWc++;
                mLutMSSTTXC[index++] = ctxtxen[bi][di];
                mTxdWc++;
                mLutMSSTTXC[index++] = ctxrxen[bi][di] + 0x00400000;
                mTxdWc++;
            }
             */
            if (bi == ROIBounds.ls_beam)
            {
                mTxdWc2=0;
                mLutMSSTTXC[index++] = 0x0020101A;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00400707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x0021101A;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00400707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x0022101A;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00400707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x0023101A;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00400707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x0024101A;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00400707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x0025101A;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00400707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x0026101A;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00400707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x0027101A;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00000707;
                mTxdWc2++;
                mLutMSSTTXC[index++] = 0x00400707;
                mTxdWc2++;
            }
        }

#ifdef DUMP_DELAY_VALUES
        // Dump txfdel
        {
            std::ofstream delayfile;
            delayfile.open("ctxfdelay.bin", std::ios::out | std::ios::binary);
            delayfile.write( (char *)ctxfdel, ctxbc*128*sizeof(uint32_t) );
            delayfile.close();
        }
#endif
        //
        // C beam waveform select (72 LUT words)
        //
        /*
        TXLUTinfo.cbeam_start_ptr = index;
        for (int di=0; di<8; di++)
        { // TX die
            mLutMSSTTXC[index++] = 0x0020101A + (di << 16);
            for (int ci = 0; ci < 7; ci++) {
                mLutMSSTTXC[index++] = 0x00000505;
            }
            mLutMSSTTXC[index++] = 0x00400505;
        }
         */
    }
    mMSSTTXClength = index;
#ifdef DUMP_DELAY_VALUES
    printf("Size of MSSTTXC is %d words\n", index);
    for (int i = 0; i < 400; i++)
        printf("%3d %08X\n", i, mLutMSSTTXC[i]);
#endif
}

//------------------------------------------------------------------------------------------------------------------------
void ColorAcqLinear::calculateBpfLut( const uint32_t imagingCaseId )
{
    mUpsReaderSPtr->getColorBpfLut( imagingCaseId, mLutMSBPFI, mLutMSBPFQ );
}


//------------------------------------------------------------------------------------------------------------------------
// Performs calculations needed to determine roi.axialStart, delay memory start (dmStart), and beamformer samples (bfS)
// see calc_Axia_ROi.py
//
void ColorAcqLinear::calculateAxialRoi( const uint32_t imagingCaseId,
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
    float ctxfd =roi.axialStart+glbPtr->cTxFocusFraction*roi.axialSpan;
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
    mFocalPointInfo.numBkptsC = mBeamParams.fpS ;//idxStop - idxStart + 1;
    mFocalPointInfo.numBkptsB = numBkptsB;
}

//-----------------------------------------------------------------------------------------------------------------------
void ColorAcqLinear::setupCtbParams(CtbParams &input, CtbParams &output, uint32_t groupIndex, uint32_t ensembleIndex, uint32_t interleaveIndex)
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
void ColorAcqLinear::setupTxdSpiPtr( uint32_t & ptr, uint32_t groupIndex, uint32_t ensembleIndex, uint32_t interleaveIndex)
{
    UNUSED(ensembleIndex);
    uint32_t gi = groupIndex;
    uint32_t ii = interleaveIndex;

    //ptr = (gi*mInterleaveFactor + ii)*86 + mNumTxBeamsBC*102 + mBip.baseConfigLength + 16;
    uint32_t pi = gi*mInterleaveFactor + ii;
    ptr = TXLUTinfo.ctxconfig_ptr[pi];
}

//-----------------------------------------------------------------------------------------------------------------------
void ColorAcqLinear::setupTxPingIndex( uint32_t & pindex, uint32_t groupIndex, uint32_t ensembleIndex, uint32_t interleaveIndex)
{
    UNUSED(ensembleIndex);
    uint32_t gi = groupIndex;
    uint32_t ii = interleaveIndex;

    pindex = gi*mInterleaveFactor + ii;
}

//-----------------------------------------------------------------------------------------------------------------------
void ColorAcqLinear::setupTxAngle( float & ta, uint32_t groupIndex, uint32_t ensembleIndex, uint32_t interleaveIndex)
{
    UNUSED(ensembleIndex);
    uint32_t gi = groupIndex;
    uint32_t ii = interleaveIndex;

    ta = mTxAngles[gi*mInterleaveFactor + ii];
}

//-----------------------------------------------------------------------------------------------------------------------
void ColorAcqLinear::initCtbParams( CtbParams &input, CtbParams &output)
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

void ColorAcqLinear::setupBpRegs( EnableFlags flags,
                            uint32_t    txdSpiPtr,
                            uint32_t    txdWordCount,
                            float       txAngle,
                            CtbParams   ctbInput,
                            CtbParams   ctbOutput,
                            uint32_t    txPingIndex,
                            uint32_t    pri,
                            uint32_t    bgPtr,
                            uint32_t    bpRegs[62],
                            uint32_t    pi)
{
    const uint32_t BPF_LS = 4;   // Bandpass filter latency per filter length (64 taps)
    const uint32_t DPL_S  = 128; // Data path latency
    const UpsReader::TransducerInfo* tinfoPtr = mUpsReaderSPtr->getTransducerInfoPtr();
    const UpsReader::Globals* glbPtr = mUpsReaderSPtr->getGlobalsPtr();
    float fs  = glbPtr->samplingFrequency;
    float s   = mBip.speedOfSound;
    // Parameter #0
    {
        int32_t preload      = glbPtr->dm_P;
        bpRegs[0] = (preload << 16) | (mBeamParams.adcS & 0xffff);
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
        uint32_t txlen = 512;
        bpRegs[7] = (txPingIndex << 16) | txlen;
    }

    // Parameter #8, 9
    {
        bpRegs[8] = 0;
        bpRegs[9] = 0;
    }

    // Parameter #10, #11
    {
        bpRegs[10] = 0xFFFFFFFF;
        bpRegs[11] = 0xFFFFFFFF;
    }

    // Parameter #12
    {
        //uint32_t bpri=mTxPri;//(uint32_t)(ceil((actualRoi.axialSpan*2*glbPtr->samplingFrequency/mBip.speedOfSound/+50)*glbPtr->samplingFrequency/glbPtr->samplingFrequency)*glbPtr->samplingFrequency);
        bpRegs[12] =(uint32_t)(ceil(pri/100)*100);
        mTxPrf = glbPtr->samplingFrequency / (uint32_t)(ceil(pri/100)*100);
    }

    // Parameter #13
    {
        bpRegs[13] = 470 | (mBip.rxStart << 16);
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
        bpf0.bits.vbpFl = ((taps >> 1) - 1) & 0xff;

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

        uint32_t temp1 = 16*floor(mBeamParams.decimationFactor) > 64 ? 64 :  16*floor(mBeamParams.decimationFactor);
        uint32_t temp2 = temp1 > 16 ? temp1 : 16;
        if (temp2 == mBeamParams.bpfTaps)
        {
            bpf1.bits.vbpWc0 = (uint32_t) (floor(mBeamParams.decimationFactor) * 8 - 2) & 0xff;
            bpf1.bits.vbpWc1 = (uint32_t) (ceil(mBeamParams.decimationFactor) * 8 - 2) & 0xff;
        }
        else
        {
            uint32_t v = (uint32_t)(mBeamParams.bpfTaps/2)-2;
            bpf1.bits.vbpWc0 = (uint32_t) (v) & 0xff;
            bpf1.bits.vbpWc1 = (uint32_t) (v) & 0xff;
        }
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
    bpRegs[20] = (int32_t)(floor(-mApod_offset_seq[pi] * 256) ) & 0xffff;
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
    float rx_ox [128]; // RX beam origins
    float rx_a_d [128];
    for (int i = 0; i < 128; i++)
    {
        rx_a_d[i] = mTxAngles[0];//actualRoi.roiA;
        rx_ox[i] = (int32_t)(round((-19.05+i*tinfoPtr->pitch)*(fs/s)*64));
    }

    bpRegs[30] = 0;
    bpRegs[31] = (int32_t)(floor(rx_ox[pi*4+0])) & 0xfffff;
    bpRegs[32] = 0;
    bpRegs[33] = (int32_t)(floor(rx_ox[pi*4+1])) & 0xfffff;
    bpRegs[34] = 0;
    bpRegs[35] = (int32_t)(floor(rx_ox[pi*4+2])) & 0xfffff;
    bpRegs[36] = 0;
    bpRegs[37] = (int32_t)(floor(rx_ox[pi*4+3])) & 0xfffff;

    bpRegs[38] = (int32_t)(floor(sin(mBeamParams.rxAngles[0]+rx_a_d[pi*4+0]) * (1<<16))) & 0x3ffff;
    bpRegs[39] = (int32_t)(floor(cos(mBeamParams.rxAngles[0]+rx_a_d[pi*4+0]) * (1<<16))) & 0x3ffff;
    bpRegs[40] = (int32_t)(floor(sin(mBeamParams.rxAngles[1]+rx_a_d[pi*4+1]) * (1<<16))) & 0x3ffff;
    bpRegs[41] = (int32_t)(floor(cos(mBeamParams.rxAngles[1]+rx_a_d[pi*4+1]) * (1<<16))) & 0x3ffff;
    bpRegs[42] = (int32_t)(floor(sin(mBeamParams.rxAngles[2]+rx_a_d[pi*4+2]) * (1<<16))) & 0x3ffff;
    bpRegs[43] = (int32_t)(floor(cos(mBeamParams.rxAngles[2]+rx_a_d[pi*4+2]) * (1<<16))) & 0x3ffff;;
    bpRegs[44] = (int32_t)(floor(sin(mBeamParams.rxAngles[3]+rx_a_d[pi*4+3]) * (1<<16))) & 0x3ffff;
    bpRegs[45] = (int32_t)(floor(cos(mBeamParams.rxAngles[3]+rx_a_d[pi*4+3]) * (1<<16))) & 0x3ffff;
    bpRegs[46] = mBeamParams.beamScales[0] | (mBeamParams.beamScales[1] << 16);
    bpRegs[47] = mBeamParams.beamScales[2] | (mBeamParams.beamScales[3] << 16);

    bpRegs[48] = 0x2000200; // FPPTR0
    bpRegs[49] = 0x2000200; // FPPTR1
    bpRegs[50] = 0x2000200; // TXPTR0
    bpRegs[51] = 0x2000200; // TXPTR1

    {
        uint32_t bkpts = mFocalPointInfo.numBkptsB;

        bkpts = (bkpts & 7) == 0 ? bkpts : ((bkpts & 0xfffffff8) + 8);
        //bpRegs[52] = 0x02000000 | (bkpts & 0xffff);
        bpRegs[52] = 0x2000040;
    }
    /*
    uint32_t ap_ptr_seq[32];
    uint32_t el_ptr_seq[32];
    uint32_t rx_apw=64;
    uint32_t indexap=0;
    uint32_t indexel=0;
    for (int i = 0; i < int(rx_apw/8); i++)
    {
        ap_ptr_seq[indexap] = 0;
        el_ptr_seq[indexel] = 0;
        indexap++;
        indexel++;
    }
    for (int i = 0; i < int(rx_apw/4); i++)
    {
        ap_ptr_seq[indexap] = 2*i+2;
        indexap++;
    }
    for (int i = 0; i < (int(rx_apw/4)-1); i++)
    {
        el_ptr_seq[indexel] = 4*i+4;
        indexel++;
    }
    for (int i = 0; i < int(rx_apw/8); i++)
    {
        ap_ptr_seq[indexap] = (tinfoPtr->numElements-rx_apw)/2;
        indexap++;
    }
    for (int i = 0; i < (int(rx_apw/8)+1); i++)
    {
        el_ptr_seq[indexel] = (tinfoPtr->numElements-rx_apw);
        indexel++;
    }
     */
    bpRegs[53] = 0;
    bpRegs[54] =  el_ptr_seq[31-pi] | (ap_ptr_seq[31-pi] << 16);

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

void  ColorAcqLinear::getSequencerLoopCounts( uint32_t& totalBeams,
                                        uint32_t& interleaveFactor,
                                        uint32_t& numGroups,
                                        uint32_t& numCtbReadBlocks)
{
    totalBeams       = mFiringCount;
    interleaveFactor = mInterleaveFactor;
    numGroups        = mNumGroups;
    numCtbReadBlocks = mCtbReadBlocks;
}

void ColorAcqLinear::getBeamCounts( BeamCount &bc )
{
    bc.txB = mNumTxBeamsB;
    bc.rxB = mNumRxBeamsB;
    bc.txBC = mNumTxBeamsBC;
    bc.rxBC = mNumRxBeamsBC;
}

//-----------------------   private methods ------------------------------------------------------------------------------
void ColorAcqLinear::findStartStopIndices( const float inputArray[],
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


void ColorAcqLinear::findStartStopIndicesInt( const int32_t inputArray[],
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

void ColorAcqLinear::convertToFloat( const int32_t inArray[], uint32_t len, uint32_t intBits, uint32_t fracBits, float outArray[])
{
    UNUSED(intBits);

    float scaleFactor = 1.0/(1<<fracBits);
    
    for (uint32_t i = 0; i < len; i++)
    {
        outArray[i] = inArray[i] * scaleFactor;
    }
}

void ColorAcqLinear::convertToInt( const float inArray[], uint32_t len, uint32_t intBits, uint32_t fracBits, int32_t outArray[])
{
    UNUSED(intBits);
    float scaleFactor = (float)(1<<fracBits);
    
    for (uint32_t i = 0; i < len; i++)
    {
          outArray[i] = (int32_t)floor(inArray[i] * scaleFactor + 0.5);
    }
}

uint32_t ColorAcqLinear::formAfeWord( uint32_t addr, uint32_t data, uint32_t dieSelect)
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

uint32_t ColorAcqLinear::formAfeGainCode(float powerMode, float g)
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
