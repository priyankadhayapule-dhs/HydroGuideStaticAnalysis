//
// Copyright 2018 EchoNous Inc.
//

#define LOG_TAG "DauDopplerAcqLinear"

#include <cstring>
#include <algorithm>
#include <ThorUtils.h>
#include <math.h>
#include <DopplerAcqLinear.h>
#include <fstream>


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
        delayVals[eleIndex] = (int32_t)( round(rd * k) );

        /*
        float temp = (sqrt((xe-x)*(xe-x) + (ye-y)*(ye-y)) - dref)/1.54;
        //delayVals[eleIndex] = (int32_t)( round(temp * -1 * 125.0)  );
        delayVals[eleIndex] =  (int32_t)( (temp * 125.0)  );
         */
    }
}


DopplerAcqLinear::DopplerAcqLinear(const std::shared_ptr<UpsReader>& upsReader) :
    mUpsReaderSPtr(upsReader)
{
    LOGD("*** DopplerAcqLinear - constructor");
}

DopplerAcqLinear::~DopplerAcqLinear()
{
    LOGD("*** DopplerAcqLinear -destructor");
}

float DopplerAcqLinear::getSfdels( uint32_t imagingCaseId, float focalDepthMm, float focalWidthMm )
{
    const UpsReader::Globals *glbPtr = mUpsReaderSPtr->getGlobalsPtr();
    float sos = mUpsReaderSPtr->getSpeedOfSound(imagingCaseId);
    float Fs = glbPtr->samplingFrequency;
    const UpsReader::TransducerInfo *transducerInfoPtr = mUpsReaderSPtr->getTransducerInfoPtr();
    uint32_t els   = transducerInfoPtr->numElements/2;
    float    pitch = transducerInfoPtr->pitch;
    uint32_t ap = 64;

    float    elfp[64][2];
    {
        float initVal = -1.0f *(els - 1)*pitch/2;
        for (uint32_t eleIndex = 0; eleIndex < els; eleIndex++)
        {
            elfp[eleIndex][0] = 0.0f;
            elfp[eleIndex][1] = initVal + (eleIndex * pitch);
        }
    }
    float frange    = sqrt( focalDepthMm*focalDepthMm + focalWidthMm*focalWidthMm );

    uint32_t efirst = floor(32-ap/2);
    uint32_t elast  = ceil(31+ap/2);

    float r_efirst = sqrt( (elfp[efirst][0]-focalDepthMm)*(elfp[efirst][0]-focalDepthMm)+(elfp[efirst][1]-focalWidthMm)*(elfp[efirst][1]-focalWidthMm) );
    float r_elast  = sqrt( (elfp[elast][0]-focalDepthMm)*(elfp[elast][0]-focalDepthMm)+(elfp[elast][1]-focalWidthMm)*(elfp[elast][1]-focalWidthMm) );
    float rd_mm    = abs(frange-r_efirst) > abs(frange-r_elast) ? abs(frange-r_efirst) : abs(frange-r_elast);

    return (rd_mm);
}

void DopplerAcqLinear::calculateActualPrfIndex( uint32_t imagingCaseId, float gateStart, uint32_t gateIndex, uint32_t requestedPrfIndex, uint32_t& actualPrfIndex, bool isTDI )
{
    const UpsReader::Globals *glbPtr = mUpsReaderSPtr->getGlobalsPtr();
    const UpsReader::TransducerInfo *tinfoPtr = mUpsReaderSPtr->getTransducerInfoPtr();
    float Fs = glbPtr->samplingFrequency;
    float sos = mUpsReaderSPtr->getSpeedOfSound(imagingCaseId);
    mSos   = sos;
    mUpsReaderSPtr->getPwPrfList(mPrfList, mNumValidPrfs, isTDI);
    float gateRange = mUpsReaderSPtr->getGateSizeMm(gateIndex);
    float prtMinUsec = 2.0f * (gateStart + gateRange)/sos + 49.0f;
    float maxAchievablePrf = 1.0e6 / prtMinUsec;
    actualPrfIndex        = requestedPrfIndex;
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
        actualPrfIndex = prfIndex;
    }
    else
    {
        actualPrfIndex = requestedPrfIndex;
    }
}

void DopplerAcqLinear::calculateMaxPrfIndex ( uint32_t imagingCaseId, float gateStart, uint32_t gateIndex, uint32_t& maxPrfIndex, bool isTDI )
{
    const UpsReader::Globals *glbPtr = mUpsReaderSPtr->getGlobalsPtr();
    const UpsReader::TransducerInfo *tinfoPtr = mUpsReaderSPtr->getTransducerInfoPtr();
    float Fs = glbPtr->samplingFrequency;
    float sos = mUpsReaderSPtr->getSpeedOfSound(imagingCaseId);
    mUpsReaderSPtr->getPwPrfList(mPrfList, mNumValidPrfs, isTDI);
    float gateRange = mUpsReaderSPtr->getGateSizeMm(gateIndex);
    float prtMinUsec = 2.0f * (gateStart + gateRange)/sos + 49.0f;
    float maxAchievablePrf = 1.0e6 / prtMinUsec;
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

ThorStatus DopplerAcqLinear::calculateBeamParams(uint32_t imagingCaseId, float gateAxialStart, uint32_t gateIndex,
        float gateAzimuthLoc, float gateAngle, uint32_t actualPrfIndex, bool isTDI)
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
    float c = mUpsReaderSPtr->getSpeedOfSound(imagingCaseId);
    float sos=c;
    float mmps         = (c/2.0)/Fs;  // mm per ADC sample
    float roiatan;     // Tangent of ROI angle
    float roiacos;     // Cos of ROI angle
    float roiasin;     // Sin of ROI angle
    float tx_ox [64];  // TX beam origins
    float rx_ox [256]; // RX beam origins
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

    mUpsReaderSPtr->getImagingDepthMm( imagingCaseId, imagingDepth );
    mTxAngles = gateAngle;

    roiatan   = tan(gateAngle);
    roiacos   = cos(gateAngle);
    roiasin   = sin(gateAngle);

    float gateRange = mUpsReaderSPtr->getGateSizeMm(gateIndex);
    //gateRange  = gateRange < 2.0 ? 2.0 : gateRange;
    mTxPri = mUpsReaderSPtr->getAfeClksPerPwSample(actualPrfIndex, isTDI);

    mPrf = Fs * 1e6 / mTxPri;
    mGateAngle = gateAngle;
    mGateStart = gateAxialStart;
    mGateRange = gateRange;
    mGateAzimuth = gateAzimuthLoc;

    float gateStartX = gateAxialStart * cos(gateAngle);
    float gateStartY = gateAxialStart * sin(gateAngle);
    float rdMm = getSfdels(imagingCaseId, gateStartX, gateStartY);
    uint32_t rdS = ceil(Fs * rdMm / (sos / 2));
    float gateStartAcq = gateAxialStart - rdMm;
    float gateRangeAcq = gateRange + 2 * rdMm;
    uint32_t gateSamplesAcq = int(Fs * gateRangeAcq / (sos / 2));
    gateSamplesAcq = ceil(gateSamplesAcq / 16.0f) * 16 + 256;
    mTxrxOffset = int(Fs * gateAxialStart / (sos / 2));
    uint32_t gateSamples = int(Fs * gateRange / (sos / 2));
    gateSamples = ceil(gateSamples / 16.0f) * 16;
    mAdcSamples = gateSamplesAcq + 64;
    // Find TX beam set
    latStartTx = gateAzimuthLoc+rxtxo;
    latdelta   = gateAxialStart*roiasin;
    ls_beam    =  (int)(round((latStartTx-latdelta)/(tinfoPtr->pitch*2))+31); // First TX beam index
    latbeams   = 0;
    le_beam    = ls_beam; // Last TX beam index

    if (ls_beam<0){
        latover = -1*ls_beam;
        ls_beam = 0;
        le_beam += latover;
    }
    if (le_beam>63){
        latover = le_beam-63;
        ls_beam -= latover;
        le_beam -= latover;
    }
    ls_beam = ls_beam < 0 ? 0 : ls_beam;
    // Clamp lateral size based on axial position
    // First get TX and RX beam origin set in mm
    // TODO: this should be in the transducer info..
    // TX pitch is hard coded to 4x the element pitch
    for (int i = 0; i < 64; i++){
        tx_ox[i] = -18.9+i*tinfoPtr->pitch*2;
    }
    for (int i = 0; i < 256; i++){
        rx_ox[i] = -19.125+i*tinfoPtr->pitch*0.5;
    }

    uint32_t roi_tx_beams = le_beam-ls_beam+1;
    // Update actual lateral parameters
    //actualRoi.azimuthStart = tx_ox[ls_beam]+gateAxialStart*roiatan-rxtxo;
    //actualRoi.azimuthSpan  = roi_tx_beams*tinfoPtr->pitch*4;
    float dtxfd = gateAxialStart + 0.5*gateRange;

    // The ROI geometry should now be legal and quantized
    // Save lateral/axial bounds info for use by sequence builder
    ROIBounds.ls_beam = ls_beam;
    ROIBounds.le_beam = le_beam;
    ROIBounds.tx_beams  = (le_beam-ls_beam)+1;
    //

    mAdcSamples = gateSamplesAcq + 64;
    // compute dmStart
    {
        mBeamParams.dmStart = int((ceil(Fs * 2 * gateStartAcq / sos)));
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
        int32_t g = (int32_t)( (16.0f/64) * (1<<16) );
        mBeamParams.lateralGains[0] = g;
        mBeamParams.lateralGains[1] = g;
        mBeamParams.lateralGains[2] = g;
        mBeamParams.lateralGains[3] = g;
    }

    // bpfSelect
    {
        mBeamParams.bpfSelect = 0;
    }

    // compute decimationFactor
    {
        mBeamParams.decimationFactor = 10.0f;
    }

    // compute bfS (BF_S)
    {
        mBeamParams.bpfTaps = 64;
        mBeamParams.bfS = 624;//gateSamples + 128;
        computeGateSamples( imagingCaseId, gateIndex );
    }

    // compute focal point
    {
        uint32_t I_R = 16;
        mBeamParams.fpS = int((mBeamParams.bfS / I_R) + 1);
    }

    // compute output samples
    {
        mBeamParams.outputSamples = mOutputSamples;
    }

    // lateralGains[4]
    {
        int32_t g = (int32_t)( (16.0f/64) * (1<<16) );
        mBeamParams.lateralGains[0] = g;
        mBeamParams.lateralGains[1] = g;
        mBeamParams.lateralGains[2] = g;
        mBeamParams.lateralGains[3] = g;
    }

    //gain breakpoints
    {
        uint32_t G_I_R = 4;
        mBeamParams.numGainBreakpoints=int(mOutputSamples/G_I_R+1);
        mBeamParams.tgcirLow  = 0;
        mBeamParams.tgcirHigh = 0;
    }

    //beamscale
    {
        mBeamParams.beamScales[0] = (1 << 14) & ((1<<16)-1);
        mBeamParams.beamScales[1] = (1 << 14) & ((1<<16)-1);
        mBeamParams.beamScales[2] = (1 << 14) & ((1<<16)-1);
        mBeamParams.beamScales[3] = (1 << 14) & ((1<<16)-1);
    }
    // numTxSpiCommands
    {
        //mBeamParams.numTxSpiCommands = mTxdWc2+mTxdWc;
    }

    // bpfSelect TODO
    {
        mBeamParams.bpfSelect = 0;
    }

    {
        /*
        uint32_t txCenterOffset = (uint32_t)((glbPtr->txClamp) * Fs);
        uint32_t lensOffset     = (uint32_t)(tinfoPtr->lensDelay * Fs);
        uint32_t skinOffset     = tinfoPtr->skinAdjustSamples;
        uint32_t rxOffset       = txCenterOffset + lensOffset + skinOffset;
        mRxStart        = 470 + glbPtr->txRef + rxOffset + mBeamParams.dmStart;
        mBeamParams.hvClkEnOff = (uint32_t)(ceil(mRxStart/16.0f));
        mBeamParams.hvClkEnOn = (uint32_t)(ceil((mRxStart+mAdcSamples+glbPtr->fs_S+glbPtr->bpf_P)/16.0f));
         */
        float    dcenterfrequency;
        uint32_t dhalfcycles;
        uint32_t dtxele;
        uint32_t organId = mUpsReaderSPtr->getImagingOrgan(imagingCaseId);
        float pwFocus = 10.0f*floor((gateAxialStart + 0.5*gateRange)/10.0);
        mUpsReaderSPtr->getPwWaveformInfoLinear(organId, pwFocus, gateRange, dtxele, dcenterfrequency,dhalfcycles );

        mBeamParams.txWaveformLength = 0.5 * dhalfcycles / dcenterfrequency;
        mCenterFrequency = dcenterfrequency;
        float timeOffset = 0.0;
        //if (gateRange>c * mBeamParams.txWaveformLength)
        //    timeOffset = c * (gateRange-c * mBeamParams.txWaveformLength)/2;//0,1,2,3,4,5
        //float timeOffset = 0;//6,7,8,9,10,11
        timeOffset = c * (gateRange-c * mBeamParams.txWaveformLength)/2;

        uint32_t txCenterOffset = (uint32_t)((glbPtr->txClamp + (0.5 * (mBeamParams.txWaveformLength - timeOffset))) * Fs);
        uint32_t lensOffset     = (uint32_t)(tinfoPtr->lensDelay * Fs);
        uint32_t skinOffset     = tinfoPtr->skinAdjustSamples;
        uint32_t rxOffset       = txCenterOffset + lensOffset + skinOffset;
        uint32_t rxStart        = 470 + glbPtr->txRef + rxOffset + mBeamParams.dmStart;

        uint32_t extraRX=61;
        uint32_t hvOffset=5;
        uint32_t depthIndex;
        mUpsReaderSPtr->getDepthIndex(imagingCaseId, depthIndex);
        mUpsReaderSPtr->getExtraRX(depthIndex, extraRX);
        mUpsReaderSPtr->getHvOffset(depthIndex, hvOffset);
        mRxStart = rxStart-extraRX;//-int((ceil(Fs * rdMm / sos)));
        mBeamParams.hvClkEnOff = (uint32_t)(floor((mRxStart) /16.0))-hvOffset;
        mBeamParams.hvClkEnOn = (uint32_t)(ceil((mRxStart + mAdcSamples)/16.0))+hvOffset;
    }

    mBeamParams.hvWakeupEn = 0;
    return (retVal);
}

void DopplerAcqLinear::computeGateSamples( uint32_t imagingCaseId, uint32_t gateIndex ) {
    const UpsReader::Globals *glbPtr = mUpsReaderSPtr->getGlobalsPtr();

    float gateLengthMm = mUpsReaderSPtr->getGateSizeMm(gateIndex);
    float sos = mUpsReaderSPtr->getSpeedOfSound(imagingCaseId);
    float Fs = glbPtr->samplingFrequency;

    uint32_t gateSamples = (uint32_t) (Fs * gateLengthMm / (sos / 2));
    // make gate samples a multiple of 16
    gateSamples = ((gateSamples & 0xf) > 0) ? ((gateSamples & 0xfffffff0) + 0x10) : gateSamples;
    float decimationFactor = 10.0f;

    uint32_t taps = 64;
    mBeamParams.bfS  = 624;//gateSamples + 128;
    mOutputSamples = (int32_t) (mBeamParams.bfS - taps) / decimationFactor;
    LOGD("DopplerAcqLinear: imagingCaseId = %d, gateIndex = %d, mGateSamples = %d",
         imagingCaseId, gateIndex, mOutputSamples);
}

void DopplerAcqLinear::calculateTxConfig( uint32_t imagingCaseId, float gateAxialStart, float gateRange,
                                    float gateAzimuthLoc, float gateAngle, bool isTDI )
{
    const UpsReader::Globals *glbPtr = mUpsReaderSPtr->getGlobalsPtr();
    const UpsReader::TransducerInfo *transducerInfoPtr = mUpsReaderSPtr->getTransducerInfoPtr();

    ScanDescriptor fieldOfView;
    uint32_t els   = transducerInfoPtr->numElements;
    float    pitch = transducerInfoPtr->pitch;
    float    txfs  = glbPtr->txQuantizationFrequency;
    float sos = mUpsReaderSPtr->getSpeedOfSound(imagingCaseId);
    float c=sos;
    uint32_t numparallel = 4;
    uint32_t txbeams     = 1;
    uint32_t rxbeams     = txbeams * numparallel;

    uint32_t dtxele;
    float    dcenterfrequency;
    uint32_t dhalfcycles;
    uint32_t organId = mUpsReaderSPtr->getImagingOrgan(imagingCaseId);
    float pwFocus = 10.0f*floor((gateAxialStart + 0.5*gateRange)/10.0);
    mUpsReaderSPtr->getPwWaveformInfoLinear(organId, pwFocus, gateRange, dtxele, dcenterfrequency,dhalfcycles );
    //dhalfcycles  = int(2*round(2*(dcenterfrequency*(gateRange/c))/2));
    mGateHalfCycle = dhalfcycles;
    
    // e_first and e_last are beam dependent for linear

    uint32_t tx_apw     = dtxele;
    //uint32_t tx_e_first = (uint32_t)(floor(32 - 0.5*tx_apw));
    //uint32_t tx_e_last  = tx_e_first + tx_apw - 1;

    int32_t referenceTime104MHz = 512;
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


    // PW beam TX configuration
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

        uint32_t halfcycles9_6ns = (uint32_t)(txfs/(dcenterfrequency*2)) - 2;
        uint32_t baseAddr = 0x00004000 | (int(dhalfcycles/2-2) << 11);
        uint32_t hvpWord  = baseAddr   | hvp0  | (halfcycles9_6ns << 4);
        uint32_t hvmWord  = 0x00000000 | hvm0  | (halfcycles9_6ns << 4);
        for (uint32_t i = 0; i < 8; i++) // TX die
        {
            //mLutMSSTTXC[index++] = 0x00200007 + 0x10000*i;
            mLutMSSTTXC[index++] = 0x00200000 + 0x10000*i;
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
    mBaseConfigLength = index;
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


    {
        // Note: calculating parameters for only the TX beams in the ROI but allocating the full possible 32
        float    ctxfp[64][2];         // Focal points(x & y), move to class member
        int32_t ctxtxap[64][2];       // RX aperture first/last element
        uint32_t ctxrxen[64][8] = {0}; // TX chip RXEN words
        uint32_t ctxtxen[64][8] = {0}; // TX chip TXEN words
        int32_t  ctxfdel[128] = {0};     // Delay table for every Tx beam and every element
        int32_t  ctxfdelburst[8][16]={0};     // Delay table for every Tx beam and every element

        float    tx_ox2[64];
        for (int i = 0; i < 64; i++){
            tx_ox2[i] = -18.9+i*0.3*2;
        }
        float    elfp2[128];
        {
            float initVal = -1.0 *(128 - 1)*pitch/2;
            for (uint32_t eleIndex = 0; eleIndex < els; eleIndex++)
            {
                elfp2[eleIndex] = initVal + (eleIndex * pitch);
            }
        }
        bool  txap_en[64][128]={false};
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
        float dtxfd = gateAxialStart + 0.5*gateRange;

        float ctxfd =dtxfd;
        //ctxfp_xdelta = 0;
        //ctxfp_ydelta = ctxfd;

        ctxfp_xdelta = ctxfd*sin(gateAngle);
        ctxfp_ydelta = ctxfd*cos(gateAngle);
        float tx_beam_pitch = 2.0;
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
            //rxDelay( elfp, els, ctxfp[bi], ctxfd, k, offset, ctxfdel);

            for (uint32_t eleIndex = 0; eleIndex < els; eleIndex++) {
                float x = ctxfp[bi][0];
                float y = ctxfp[bi][1];
                float xe = elfp[eleIndex][0];
                float ye = elfp[eleIndex][1];

                float rd = sqrt((xe - x) * (xe - x) + (ye - y) * (ye - y)) - ctxfd;
                ctxfdel[eleIndex] = (int32_t) (round(rd * k));

            }
        }
        for (int bi = ROIBounds.ls_beam; bi<ROIBounds.le_beam+1; bi++) // C TX beams in ROI
        {
            // Set delay for element outside aperture to reference delay
            for (int ei = 0; ei < els; ei++) {

                if ((ei < ctxtxap[bi][0]) or (ei > ctxtxap[bi][1])) {
                    ctxfdel[ei] = offset;
                } else {
                    ctxfdel[ei] = ctxfdel[ei] + offset;
                }

            }
        }

        for (int bi = ROIBounds.ls_beam; bi<ROIBounds.le_beam+1; bi++)
        {
            for (int te = 0; te < els; te++)
            {
                uint32_t te_chip = chip_map[te];
                uint32_t te_ch = channel_map[te];
                ctxfdelburst[te_chip][te_ch] = ctxfdel[te];
            }
        }

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
            mTxdWc=0;

            // Delay burst
            for (int di = 0; di<8; di++){
                mLutMSSTTXC[index++] = 0x0020100A + (di<<16);
                mTxdWc++;
                for (int ci = 0; ci<15; ci++){
                    mLutMSSTTXC[index++] = ctxfdel[die_map[16*di+ci]];
                    mTxdWc++;
                }
                mLutMSSTTXC[index++] = 0x00400000 | ctxfdel[die_map[16*di+15]];
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

            if (bi == ROIBounds.ls_beam)
            {
                mTxdWc2=0;
                /*
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
                 */
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

void DopplerAcqLinear::calculateDgcInterpLut( uint32_t imagingCaseId, float gateStart, float gateRange, bool isTDI)
{
    const UpsReader::Globals *glbPtr = mUpsReaderSPtr->getGlobalsPtr();
    const UpsReader::TransducerInfo *transducerInfoPtr = mUpsReaderSPtr->getTransducerInfoPtr();
    uint32_t dtxele;
    float dcenterfrequency;
    float maxGain;
    mUpsReaderSPtr->getPwWaveformInfo( dtxele, dcenterfrequency, isTDI );
    mUpsReaderSPtr->getMaxGainDbToLinearLUT(maxGain);
    float depth = gateStart+gateRange;
    float StartGain = 0.1*gateStart * dcenterfrequency;
    float StopGain = 0.1*depth * dcenterfrequency;

    int32_t dgclut[32];

    for (uint32_t i = 0; i < mBeamParams.numGainBreakpoints; i++)
    {
        float totalGain = StartGain+i*(StopGain-StartGain)/(mBeamParams.numGainBreakpoints-1);
        totalGain = 20.0;
        //totalGain    = totalGain > 30.0f ? 30.0f : totalGain;
        int32_t beamgain = (totalGain/maxGain*511);
        dgclut[i] = beamgain + beamgain*(1<<9);

    }

    for (int i = 0; i < mBeamParams.numGainBreakpoints; i++)
    {
        mLutMSDBG01[i] = dgclut[i];
        mLutMSDBG23[i] = dgclut[i];
    }
}

void DopplerAcqLinear::calculateAtgc( uint32_t imagingCaseId )
{
    const UpsReader::Globals *glbPtr = mUpsReaderSPtr->getGlobalsPtr();
    const UpsReader::TransducerInfo *transducerInfoPtr = mUpsReaderSPtr->getTransducerInfoPtr();

    uint32_t bitset;
    mUpsReaderSPtr->getAfeInputResistance( glbPtr->afeResistSelectId, bitset );


    uint32_t gainmode = glbPtr->afeGainSelectIdD;
    float    gainval;
    mUpsReaderSPtr->getAfeGainModeParams( gainmode, gainval);


    uint32_t powerMode;
    float dummyMaxGainC, dummyMinGainC;
    uint32_t depthIndex;
    mUpsReaderSPtr->getDepthIndex(imagingCaseId, depthIndex);
    
    mUpsReaderSPtr->getGainAtgcParams( depthIndex, powerMode, dummyMaxGainC, dummyMinGainC );

    for (int i = 0; i < TBF_LUT_MSAFEINIT_SIZE; i++)
        mLutMSAFEINIT[i] = 0;

    uint32_t index = 0;

    mLutMSAFEINIT[index++] = formAfeWord(0, 0x10);  // Enables programming of the DTGC register map
    mLutMSAFEINIT[index++] = formAfeWord(181, 0x0);  // manual mode setup

    bitset |= 1 << 11;
    bitset |= 8 << 12;
    mLutMSAFEINIT[index++] = formAfeWord(181, formAfeGainCode(powerMode, gainval));
    mLutMSAFEINIT[index++] = formAfeWord(182, bitset);
    mLutMSAFEINIT[index++] = formAfeWord(229, powerMode);
    mLutMSAFEINIT[index++] = formAfeWord(199, 0x0500);  //0000 0001 1000 0000 HPF 75 KHz, LPF 10 MHz
    mLutMSAFEINIT[index++] = formAfeWord(0, 0x0);
}

void DopplerAcqLinear::calculateFocalInterpLut( uint32_t fpS, uint32_t interpRate, uint32_t startOffset )
{
    const UpsReader::Globals *glbPtr = mUpsReaderSPtr->getGlobalsPtr();

    // MSDIS LUT
    {
        for (uint32_t i = 0; i < TBF_LUT_MSDIS_SIZE; i++)
            mLutMSDIS[i] = 0;

    }

    // MSB0APS
    {
        for (uint32_t i = 0; i < fpS; i++)
        {
            mLutMSB0APS[i] = 1024;
        }
    }

    // B0FPX, B1FPX, B2FPX, B3FPX, B0FPY, B1FPY, B2FPY, B3FPY, B0FTXD, B1FTXD, B2FTXD, B3FTXD
    for (uint32_t i = 0; i < fpS; i++)
    {
        uint32_t val = ((i * interpRate) + startOffset) * (1 << 5);
        mLutB0FPX[i] = val;
        mLutB1FPX[i] = val;
        mLutB2FPX[i] = val;
        mLutB3FPX[i] = val;
        mLutB0FPY[i] = 0;
        mLutB1FPY[i] = 0;
        mLutB2FPY[i] = 0;
        mLutB3FPY[i] = 0;
        mLutB0FTXD[i] = val;
        mLutB1FTXD[i] = val;
        mLutB2FTXD[i] = val;
        mLutB3FTXD[i] = val;
    }
}

//------------------------------------------------------------------------------------------------------------------------
void DopplerAcqLinear::calculateBpfLut(uint32_t imagingCaseId, float gateRange, uint32_t& oscale)
{
    uint32_t organId = mUpsReaderSPtr->getImagingOrgan(imagingCaseId);
    mUpsReaderSPtr->getPwBpfLutLinear(organId, mGateRange, oscale, mLutMSBPFI, mLutMSBPFQ);
}


//--------------------------------------------------------------------------------
void DopplerAcqLinear::buildLuts( const uint32_t imagingCaseId, bool isTDI)
{
    calculateFocalInterpLut( mBeamParams.fpS, 16, mTxrxOffset );
    calculateDgcInterpLut  ( imagingCaseId, mGateStart, mGateRange, isTDI);
    calculateAtgc          ( imagingCaseId );
    calculateTxConfig      ( imagingCaseId, mGateStart, mGateRange, mGateAzimuth, mGateAngle, isTDI);
    calculateBpfLut        ( imagingCaseId, mGateRange, mBeamParams.bpfScale);
}

//--------------------------------------------------------------------------------
void DopplerAcqLinear::buildPwSequenceBlob()
{
    buildPwSequenceBlob(mSequenceBlob );
}

//--------------------------------------------------------------------------------
void DopplerAcqLinear::buildPwSequenceBlob(uint32_t dopplerBlob[62] )
{
    const uint32_t BPF_LS = 4;   // Bandpass filter latency per filter length (64 taps)
    const uint32_t DPL_S  = 128; // Data path latency

    const UpsReader::Globals* glbPtr = mUpsReaderSPtr->getGlobalsPtr();
    const UpsReader::TransducerInfo *tinfoPtr = mUpsReaderSPtr->getTransducerInfoPtr();
    float Fs = glbPtr->samplingFrequency;
    float fs  = glbPtr->samplingFrequency;
    float s   = mSos;
    uint32_t pi = ROIBounds.ls_beam;
    // Parameter #0
    {
        int32_t extraSamples = glbPtr->bpf_P + glbPtr->fs_S;
        int32_t preload      = glbPtr->dm_P;
        int32_t adcSamples = mBeamParams.bfS - 1 + extraSamples;
        dopplerBlob[0] = (preload << 16) | (mAdcSamples & 0xffff);
    }

    // Parameter #1
    {
        dopplerBlob[1] = ( ((mBeamParams.dmStart%1024) << 16) | ((mBeamParams.fpS - 1) & 0xffff) );
    }

    // Parameter #2
    {
        uint32_t val;
        val = ((mBeamParams.outputSamples * 4  - 1) << 16) | (mBeamParams.bfS-1);
        dopplerBlob[2] = val;
    }

    // Parameter #3
    {
        uint32_t val;
        val = (mBeamParams.hvClkEnOn << 16) | mBeamParams.hvClkEnOff;
        dopplerBlob[3] = val;
    }

    // Parameter #4
    {
        BpRegBENABLE reg;
        reg.val = 0;

        reg.bits.pmonEn         = 1;
        reg.bits.bpSlopeEn      = 1;
        reg.bits.ceof           = 0;
        reg.bits.eos            = 0;
        reg.bits.txEn = 1;
        reg.bits.rxEn = 1;
        reg.bits.bfEn = 1;
        reg.bits.outEn = 1;
        reg.bits.outType   = 2;
        reg.bits.outIqType = 0;
        dopplerBlob[4] = reg.val;
    }

    // Parameter #5 TODO
    {
        dopplerBlob[5] = 96;
    }

    // Parameter #6
    {
        dopplerBlob[6] = (mBaseConfigLength+16) | ((mTxdWc2+mTxdWc) << 16);
    }

    // Parameter #7
    {
        uint32_t txlen = 512;
        dopplerBlob[7] = txlen;
    }

    // Parameter #8, 9
    {
        dopplerBlob[8] = 0;
        dopplerBlob[9] = 0;
    }

    // Parameter #10, #11
    {
        dopplerBlob[10] = 0xFFFFFFFF;
        dopplerBlob[11] = 0xFFFFFFFF;
    }

    // Parameter #12
    {

        dopplerBlob[12] = mTxPri;
    }

    // Parameter #13
    {
        dopplerBlob[13] = 470 | (mRxStart << 16);
    }

    // Parameter #14, #15
    {
        dopplerBlob[14] = 0;
        dopplerBlob[15] = 0;
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

        bpf0.bits.vbpFs  = 0;
        bpf0.bits.vbpOscale = mBeamParams.bpfScale;
        dopplerBlob[16] = bpf0.val;
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
        dopplerBlob[17] = bpf1.val;
    }

    // Parameter #18
    {
        BpRegBPF2 bpf2;
        bpf2.val = 0;

        bpf2.bits.vbpPhaseInc = (uint32_t)(floor(mBeamParams.decimationFactor*8)) & 0xff;
        bpf2.bits.vbpWc       = (mBeamParams.outputSamples - 1) & 0x3fff;
        dopplerBlob[18] = bpf2.val;
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

        dopplerBlob[19] = bpf3.val;
    }

    // Parameter #20, 21, 22
    dopplerBlob[20] = (int32_t)(floor(-mApod_offset_seq[pi] * 256) ) & 0xffff;
    dopplerBlob[21] = 1;
    dopplerBlob[22] = 0;

    // Parameter #23
    {
        BpRegDTGC0 dtgc0;
        dtgc0.val = 0;

        dtgc0.bits.bgPtr   = 0;
        dtgc0.bits.tgc0Lgc = mBeamParams.lateralGains[0] & 0xffffff;
        dtgc0.bits.tgc0Flat = 0;
        dopplerBlob[23] = dtgc0.val;
    }

    // Parameter #24
    {
        BpRegDTGC1 dtgc1;
        dtgc1.val = 0;

        dtgc1.bits.tgc1Lgc = mBeamParams.lateralGains[1] & 0xffffff;
        dtgc1.bits.tgc1Flat = 0;
        dtgc1.bits.bgRc    = mBeamParams.numGainBreakpoints - 1;
        dtgc1.bits.tgcFlip  = 0;
        dopplerBlob[24] = dtgc1.val;
    }

    // Parameter #25
    {
        BpRegDTGC2 dtgc2;
        dtgc2.val = 0;

        dtgc2.bits.tgc2Lgc = mBeamParams.lateralGains[2] & 0xffffff;
        dtgc2.bits.tgc2Flat = 0;
        dopplerBlob[25] = dtgc2.val;
    }

    // Parameter #26
    {
        BpRegDTGC3 dtgc3;
        dtgc3.val = 0;

        dtgc3.bits.tgc3Lgc = mBeamParams.lateralGains[3] & 0xffffff;
        dtgc3.bits.tgc3Flat = 0;
        dopplerBlob[26] = dtgc3.val;
    }

    // Parameters #27, 28
    dopplerBlob[27] = 0;
    dopplerBlob[28] = 0;

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
        dopplerBlob[29] = ccomp.val;
    }
    float rx_ox [256]; // RX beam origins
    float rx_a_d [256];
    for (int i = 0; i < 256; i++)
    {
        rx_a_d[i] = mTxAngles;//actualRoi.roiA;
        rx_ox[i] = (int32_t)(round((-19.125+i*tinfoPtr->pitch*0.5)*(fs/s)*64));
    }

    dopplerBlob[30] = 0;
    dopplerBlob[31] = (int32_t)(floor(rx_ox[pi*4+0])) & 0xfffff;;
    dopplerBlob[32] = 0;
    dopplerBlob[33] = (int32_t)(floor(rx_ox[pi*4+1])) & 0xfffff;;
    dopplerBlob[34] = 0;
    dopplerBlob[35] = (int32_t)(floor(rx_ox[pi*4+2])) & 0xfffff;;
    dopplerBlob[36] = 0;
    dopplerBlob[37] = (int32_t)(floor(rx_ox[pi*4+3])) & 0xfffff;;

    dopplerBlob[38] = (int32_t)(floor(sin(rx_a_d[pi*4+0]) * (1<<16))) & 0x3ffff;
    dopplerBlob[39] = (int32_t)(floor(cos(rx_a_d[pi*4+0]) * (1<<16))) & 0x3ffff;
    dopplerBlob[40] = (int32_t)(floor(sin(rx_a_d[pi*4+1]) * (1<<16))) & 0x3ffff;
    dopplerBlob[41] = (int32_t)(floor(cos(rx_a_d[pi*4+1]) * (1<<16))) & 0x3ffff;
    dopplerBlob[42] = (int32_t)(floor(sin(rx_a_d[pi*4+2]) * (1<<16))) & 0x3ffff;
    dopplerBlob[43] = (int32_t)(floor(cos(rx_a_d[pi*4+2]) * (1<<16))) & 0x3ffff;;
    dopplerBlob[44] = (int32_t)(floor(sin(rx_a_d[pi*4+3]) * (1<<16))) & 0x3ffff;
    dopplerBlob[45] = (int32_t)(floor(cos(rx_a_d[pi*4+3]) * (1<<16))) & 0x3ffff;

    dopplerBlob[46] = mBeamParams.beamScales[0] | (mBeamParams.beamScales[1] << 16);
    dopplerBlob[47] = mBeamParams.beamScales[2] | (mBeamParams.beamScales[3] << 16);

    dopplerBlob[48] = 0; // FPPTR0
    dopplerBlob[49] = 0; // FPPTR1
    dopplerBlob[50] = 0; // TXPTR0
    dopplerBlob[51] = 0; // TXPTR1

    dopplerBlob[52] = 0;
    dopplerBlob[53] = 0;
    dopplerBlob[54] = el_ptr_seq[63-pi] | (ap_ptr_seq[63-pi] << 16);

    dopplerBlob[55] = 0;
    dopplerBlob[56] = 0;
    dopplerBlob[57] = 0;

    dopplerBlob[58] = 0;
    dopplerBlob[59] = 0;

    dopplerBlob[60] = 0;

    dopplerBlob[61] = 0;
}

uint32_t DopplerAcqLinear::formAfeWord( uint32_t addr, uint32_t data, uint32_t dieSelect)
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

uint32_t DopplerAcqLinear::formAfeGainCode(float powerMode, float g)
{
    float gc;
    if (powerMode == 0)
        gc = round((g -12.0f) * (319.0f / (51.5f - 12)));
    else
        gc = round((g - 6.0f) * (319.0f / (45.5f - 6)));

    gc = gc < 319.0f ? gc : 319.0f;
    gc = gc < 0 ? 0 : gc;

    return ((uint32_t)gc);
}
