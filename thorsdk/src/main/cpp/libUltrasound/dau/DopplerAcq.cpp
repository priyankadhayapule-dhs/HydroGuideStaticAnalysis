//
// Copyright 2018 EchoNous Inc.
//

#define LOG_TAG "DauDopplerAcq"

#include <cstring>
#include <algorithm>
#include <ThorUtils.h>
#include <math.h>
#include <DopplerAcq.h>
#include <fstream>
#include <algorithm>


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


DopplerAcq::DopplerAcq(const std::shared_ptr<UpsReader>& upsReader) :
    mUpsReaderSPtr(upsReader)
{
    LOGD("*** DopplerAcq - constructor");
}

DopplerAcq::~DopplerAcq()
{
    LOGD("*** DopplerAcq -destructor");
}

float DopplerAcq::getSfdels( uint32_t imagingCaseId, float focalDepthMm, float focalWidthMm )
{
    const UpsReader::Globals *glbPtr = mUpsReaderSPtr->getGlobalsPtr();
    float sos = mUpsReaderSPtr->getSpeedOfSound(imagingCaseId);
    float Fs = glbPtr->samplingFrequency;
    const UpsReader::TransducerInfo *transducerInfoPtr = mUpsReaderSPtr->getTransducerInfoPtr();
    uint32_t els   = transducerInfoPtr->numElements;
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

void DopplerAcq::calculateActualPrfIndex( uint32_t imagingCaseId, float gateStart, uint32_t gateIndex, uint32_t requestedPrfIndex, uint32_t& actualPrfIndex, bool isTDI )
{
    const UpsReader::Globals *glbPtr = mUpsReaderSPtr->getGlobalsPtr();
    const UpsReader::TransducerInfo *tinfoPtr = mUpsReaderSPtr->getTransducerInfoPtr();
    float Fs = glbPtr->samplingFrequency;
    float sos = mUpsReaderSPtr->getSpeedOfSound(imagingCaseId);
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

void DopplerAcq::calculateMaxPrfIndex ( uint32_t imagingCaseId, float gateStart, uint32_t gateIndex, uint32_t& maxPrfIndex, bool isTDI )
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
    if (mUpsReaderSPtr->getImagingGlobalID(imagingCaseId) == TCD_GLOBAL_ID) {
        // OTS-3101: Limit valid prfs in TCD, ignore highest 5.
        prfIndex -= std::min(prfIndex, TCD_LIMIT_PRFS);
    }
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

ThorStatus DopplerAcq::calculateBeamParams(uint32_t imagingCaseId, float gateStart, uint32_t gateIndex,
        float gateAngle, uint32_t actualPrfIndex, bool isTDI)
{
    ThorStatus retVal = THOR_OK;
    const UpsReader::Globals *glbPtr = mUpsReaderSPtr->getGlobalsPtr();
    const UpsReader::TransducerInfo *tinfoPtr = mUpsReaderSPtr->getTransducerInfoPtr();
    float Fs = glbPtr->samplingFrequency;
    float sos = mUpsReaderSPtr->getSpeedOfSound(imagingCaseId);
    float gateRange = mUpsReaderSPtr->getGateSizeMm(gateIndex);

    mTxPri = mUpsReaderSPtr->getAfeClksPerPwSample(actualPrfIndex, isTDI);
    mPrf = Fs * 1e6 / mTxPri;
    mGateAngle = gateAngle;
    mGateStart = gateStart;
    mGateRange = gateRange;
    float gateStartX = gateStart * cos(gateAngle);
    float gateStartY = gateStart * sin(gateAngle);
    float rdMm = getSfdels(imagingCaseId, gateStartX, gateStartY);
    uint32_t rdS = ceil(Fs * rdMm / (sos / 2));
    float gateStartAcq = gateStart - rdMm;
    float gateRangeAcq = gateRange + 2 * rdMm;
    uint32_t gateSamplesAcq = int(Fs * gateRangeAcq / (sos / 2));
    gateSamplesAcq = ceil(gateSamplesAcq / 16.0f) * 16 + 256;
    mTxrxOffset = int(Fs * gateStart / (sos / 2));
    uint32_t gateSamples = int(Fs * gateRange / (sos / 2));
    gateSamples = ceil(gateSamples / 16.0f) * 16;

    mAdcSamples = gateSamplesAcq + 64;
    // compute dmStart
    {
        mBeamParams.dmStart = int((ceil(Fs * 2 * gateStartAcq / sos)));
    }

    // compute decimationFactor
    {
        mBeamParams.decimationFactor = 12.0f;
    }

    // compute bfS (BF_S)
    {
        mBeamParams.bpfTaps = 64;
        mBeamParams.bfS = 544;//gateSamples + 128;
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
        mBeamParams.numTxSpiCommands = 86;
    }

    // bpfSelect TODO
    {
        mBeamParams.bpfSelect = 0;
    }

    {
        uint32_t txCenterOffset = (uint32_t)((glbPtr->txClamp) * Fs);
        uint32_t lensOffset     = (uint32_t)(tinfoPtr->lensDelay * Fs);
        uint32_t skinOffset     = tinfoPtr->skinAdjustSamples;
        uint32_t rxOffset       = txCenterOffset + lensOffset + skinOffset;
        mRxStart        = glbPtr->txStart + glbPtr->txRef + rxOffset + mBeamParams.dmStart;
        mBeamParams.hvClkEnOff = (uint32_t)(ceil(mRxStart/16.0f));
        mBeamParams.hvClkEnOn = (uint32_t)(ceil((mRxStart+mAdcSamples+glbPtr->fs_S+glbPtr->bpf_P)/16.0f));
    }


    mBeamParams.hvWakeupEn = 0;

    return (retVal);
}

void DopplerAcq::computeGateSamples( uint32_t imagingCaseId, uint32_t gateIndex ) {
    const UpsReader::Globals *glbPtr = mUpsReaderSPtr->getGlobalsPtr();

    float gateLengthMm = mUpsReaderSPtr->getGateSizeMm(gateIndex);

    float sos = mUpsReaderSPtr->getSpeedOfSound(imagingCaseId);
    float Fs = glbPtr->samplingFrequency;

    uint32_t gateSamples = (uint32_t) (Fs * gateLengthMm / (sos / 2));

    // make gate samples a multiple of 16
    gateSamples = ((gateSamples & 0xf) > 0) ? ((gateSamples & 0xfffffff0) + 0x10) : gateSamples;
    float decimationFactor = 12.0f;

    uint32_t taps = 64;
    mBeamParams.bfS  = 544;//gateSamples + 128;
    mOutputSamples = (int32_t) (mBeamParams.bfS - taps) / decimationFactor;
    LOGD("DopplerAcq: imagingCaseId = %d, gateIndex = %d, mGateSamples = %d",
         imagingCaseId, gateIndex, mOutputSamples);
}

void DopplerAcq::calculateTxConfig( uint32_t imagingCaseId, float gateStart, float gateRange,
                                    float gateAngle, bool isTDI )
{
    const UpsReader::Globals *glbPtr = mUpsReaderSPtr->getGlobalsPtr();
    const UpsReader::TransducerInfo *transducerInfoPtr = mUpsReaderSPtr->getTransducerInfoPtr();

    uint32_t els   = transducerInfoPtr->numElements;
    float    pitch = transducerInfoPtr->pitch;
    float    txfs  = glbPtr->txQuantizationFrequency;
    float    c     = mUpsReaderSPtr->getSpeedOfSound(imagingCaseId);

    uint32_t dtxele;
    float    dcenterfrequency;

    mUpsReaderSPtr->getPwWaveformInfo( dtxele, dcenterfrequency, isTDI );
    uint32_t dhalfcycles  = int(2*round(2*(dcenterfrequency*(gateRange/c))/2));
    mGateHalfCycle = dhalfcycles;

    uint32_t channel_map[] = {15, 14,  7, 6,  5, 13,  4, 12,  3, 11,  2, 10,  1,  0,  9,  8};
    uint32_t index_map[]   = {13, 12, 10, 8,  6,  4,  3,  2, 15, 14, 11,  9,  7,  5,  1,  0}; 
    
    uint32_t tx_apw     = dtxele;
    uint32_t tx_e_first = (uint32_t)(floor(32 - 0.5*tx_apw));
    uint32_t tx_e_last  = tx_e_first + tx_apw - 1;

    uint32_t dev0[4];
    uint32_t dev3[4];

    int32_t referenceTime104MHz = 768;

    float    elfp[64][2];
    {
        float initVal = -1.0f *(els - 1)*pitch/2;
        for (uint32_t eleIndex = 0; eleIndex < els; eleIndex++)
        {
            elfp[eleIndex][0] = initVal + (eleIndex * pitch);
            elfp[eleIndex][1] = 0.0f;
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

    uint32_t halfcycles9_6ns = (uint32_t)(txfs/(dcenterfrequency*2)) - 2;

    uint32_t index = 0;

    //-----------  Base config begin -------------------------------------
    mLutMSSTTXC[index++] = 0x00201000;
	mLutMSSTTXC[index++] = 0x00000000;
	mLutMSSTTXC[index++] = 0x00000000;
	mLutMSSTTXC[index++] = 0x0000030C;
	mLutMSSTTXC[index++] = 0x000000F3;
	mLutMSSTTXC[index++] = 0x00000000;
	mLutMSSTTXC[index++] = 0x0000000A;
    mLutMSSTTXC[index++] = (1 << dev0[0]) | (1 << dev0[1]) | (1 << dev0[2]) | (1 << dev0[3]);
    mLutMSSTTXC[index++] = 0x0000FFFF; 
	mLutMSSTTXC[index++] = 0x00000002;
	mLutMSSTTXC[index++] = 0x00401e00;

    
	mLutMSSTTXC[index++] = 0x00211000;
	mLutMSSTTXC[index++] = 0x00000000;
	mLutMSSTTXC[index++] = 0x00000000;
	mLutMSSTTXC[index++] = 0x0000030C;
	mLutMSSTTXC[index++] = 0x000000F3;
	mLutMSSTTXC[index++] = 0x00000000;
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
	mLutMSSTTXC[index++] = 0x00000000;
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
	mLutMSSTTXC[index++] = 0x00000000;
	mLutMSSTTXC[index++] = 0x0000000A;
	mLutMSSTTXC[index++] = (1 << dev3[0]) | (1 << dev3[1]) | (1 << dev3[2]) | (1 << dev3[3]);
	//mLutMSSTTXC[index++] = 0x0000C0C0; #enable 48,49,50,51 maps to 15,14,7,6
	mLutMSSTTXC[index++] = 0x0000FFFF;
	mLutMSSTTXC[index++] = 0x00000002;
	mLutMSSTTXC[index++] = 0x00401e00;

    //---------- PW waveform begin ---------------------------------------------------------
    {
        uint32_t halfcycles9_6ns = (uint32_t)(txfs/(dcenterfrequency*2)) - 2;
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

        hvpWord  = 0x00000000 | hvp0  | (halfcycles9_6ns << 4);
        hvmWord  = 0x00000000 | hvm0  | (halfcycles9_6ns << 4);
        for (uint32_t i = 0; i < 4; i++)
        {
            mLutMSSTTXC[index++] = 0x00200005 + 0x10000*i;
            mLutMSSTTXC[index++] = postWord;
            for (uint32_t j = 0; j < dhalfcycles/2; j++)
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
        mLutMSSTTXC[index++] = 0x00404502;
        mLutMSSTTXC[index++] = 0x00211008;
        mLutMSSTTXC[index++] = 0x00404502;
        mLutMSSTTXC[index++] = 0x00221008;
        mLutMSSTTXC[index++] = 0x00404502;
        mLutMSSTTXC[index++] = 0x00231008;
        mLutMSSTTXC[index++] = 0x00404502;
    }

    mBaseConfigLength = index;

    //----------- PW Delay
    {
        float txfp[64][2];
        float dtxfd = gateStart + 0.5*gateRange;
        
        for (uint32_t eleIndex = 0; eleIndex < els; eleIndex++)
        {
            txfp[eleIndex][0] = dtxfd * sin(gateAngle);
            txfp[eleIndex][1] = dtxfd * cos(gateAngle);
        }

        int32_t  txfdel[64]; // delay table for every element
        float k = -txfs/c;
        int32_t offset = referenceTime104MHz;

        rxDelay( elfp, els, txfp, k, offset, txfdel);

        int deviceId = 0;
        for (int ci = 12; ci < 16; ci++)
        {
            mLutMSSTTXC[index++] = 0x0020100A + channel_map[ci];
            mLutMSSTTXC[index++] = 0x00400000 | txfdel[ci + deviceId*16];
        }
        for (deviceId = 1; deviceId < 3; deviceId++)
        {
            mLutMSSTTXC[index++] = 0x0020100A | (deviceId << 16);   
            for (int ci = 0; ci < 15; ci++)
            {
                mLutMSSTTXC[index++] = txfdel[index_map[ci] + deviceId*16];
            }
            mLutMSSTTXC[index++] = 0x00400000 | txfdel[index_map[15] + deviceId*16];
        }
        deviceId = 3;
        for (int ci = 0; ci < 4; ci++)
        {
            mLutMSSTTXC[index++] = (0x0020100A | (deviceId << 16)) + channel_map[ci];
            mLutMSSTTXC[index++] = 0x00400000 | txfdel[ci + deviceId*16];
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
    mMSSTTXClength = index;
}

void DopplerAcq::calculateDgcInterpLut( uint32_t imagingCaseId, float gateStart, float gateRange, bool isTDI)
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

void DopplerAcq::calculateAtgc( uint32_t imagingCaseId )
{
    const UpsReader::Globals *glbPtr = mUpsReaderSPtr->getGlobalsPtr();
    const UpsReader::TransducerInfo *transducerInfoPtr = mUpsReaderSPtr->getTransducerInfoPtr();

    uint32_t bitset;
    mUpsReaderSPtr->getAfeInputResistance( glbPtr->afeResistSelectId, bitset );


    uint32_t gainmode = glbPtr->afeGainSelectIdD;
    float    gainval;
    mUpsReaderSPtr->getAfeGainModeParams( gainmode, gainval);

    uint32_t globalId = mUpsReaderSPtr->getImagingGlobalID(imagingCaseId);
    if (globalId == 10)
    {
        gainval = 35;
    }
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
    mLutMSAFEINIT[index++] = formAfeWord(199, 0x0180);  //0000 0001 1000 0000 HPF 75 KHz, LPF 10 MHz
    mLutMSAFEINIT[index++] = formAfeWord(0, 0x0);
}

void DopplerAcq::calculateFocalInterpLut( uint32_t fpS, uint32_t interpRate, uint32_t startOffset )
{
    const UpsReader::Globals *glbPtr = mUpsReaderSPtr->getGlobalsPtr();

    // MSDIS LUT
    {
        for (uint32_t i = 0; i < TBF_LUT_MSDIS_SIZE; i++)
            mLutMSDIS[ i ] = 0;

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
void DopplerAcq::calculateBpfLut(const uint32_t imagingCaseId, uint32_t& oscale, bool isTDI)
{
    mUpsReaderSPtr->getPwBpfLut( imagingCaseId, oscale, mLutMSBPFI, mLutMSBPFQ, isTDI);
}


//--------------------------------------------------------------------------------
void DopplerAcq::buildLuts( const uint32_t imagingCaseId, bool isTDI)
{
    calculateFocalInterpLut( mBeamParams.fpS, 16, mTxrxOffset );
    calculateDgcInterpLut  ( imagingCaseId, mGateStart, mGateRange, isTDI);
    calculateAtgc          ( imagingCaseId );
    calculateTxConfig      ( imagingCaseId, mGateStart, mGateRange, mGateAngle, isTDI);
    calculateBpfLut        ( imagingCaseId, mBeamParams.bpfScale, isTDI);
}

//--------------------------------------------------------------------------------
void DopplerAcq::buildPwSequenceBlob()
{
    buildPwSequenceBlob(mSequenceBlob );
}

//--------------------------------------------------------------------------------
void DopplerAcq::buildPwSequenceBlob(uint32_t dopplerBlob[62] )
{
    const uint32_t BPF_LS = 4;   // Bandpass filter latency per filter length (64 taps)
    const uint32_t DPL_S  = 128; // Data path latency

    const UpsReader::Globals* glbPtr = mUpsReaderSPtr->getGlobalsPtr();
    const UpsReader::TransducerInfo *tinfoPtr = mUpsReaderSPtr->getTransducerInfoPtr();
    float Fs = glbPtr->samplingFrequency;

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
        dopplerBlob[6] = mBaseConfigLength | (mBeamParams.numTxSpiCommands << 16);
    }

    // Parameter #7
    {
        uint32_t txlen = 1280;
        dopplerBlob[7] = txlen;
    }

    // Parameter #8, 9
    {
        dopplerBlob[8] = 0;
        dopplerBlob[9] = 0;
    }

    // Parameter #10, #11
    {
        uint32_t rx_e_first  = 0;
        uint32_t rx_e_last   = 63;

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
        dopplerBlob[10] = rxap_low;
        dopplerBlob[11] = rxap_high;
    }

    // Parameter #12
    {

        dopplerBlob[12] = mTxPri;
    }

    // Parameter #13
    {
        dopplerBlob[13] = glbPtr->txStart | (mRxStart << 16);
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

        bpf1.bits.vbpWc0   = (uint32_t)(floor(mBeamParams.decimationFactor)*8 -2) & 0xff;
        bpf1.bits.vbpWc1   = (uint32_t)( ceil(mBeamParams.decimationFactor)*8 -2) & 0xff;

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
    dopplerBlob[20] = 0x01000000;
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

    dopplerBlob[30] = 0;
    dopplerBlob[31] = 0;
    dopplerBlob[32] = 0;
    dopplerBlob[33] = 0;
    dopplerBlob[34] = 0;
    dopplerBlob[35] = 0;
    dopplerBlob[36] = 0;
    dopplerBlob[37] = 0;

    dopplerBlob[38] = (int32_t)(floor(sin(mGateAngle) * (1<<16))) & 0x3ffff;
    dopplerBlob[39] = (int32_t)(floor(cos(mGateAngle) * (1<<16))) & 0x3ffff;
    dopplerBlob[40] = (int32_t)(floor(sin(mGateAngle) * (1<<16))) & 0x3ffff;
    dopplerBlob[41] = (int32_t)(floor(cos(mGateAngle) * (1<<16))) & 0x3ffff;
    dopplerBlob[42] = (int32_t)(floor(sin(mGateAngle) * (1<<16))) & 0x3ffff;
    dopplerBlob[43] = (int32_t)(floor(cos(mGateAngle) * (1<<16))) & 0x3ffff;
    dopplerBlob[44] = (int32_t)(floor(sin(mGateAngle) * (1<<16))) & 0x3ffff;
    dopplerBlob[45] = (int32_t)(floor(cos(mGateAngle) * (1<<16))) & 0x3ffff;

    dopplerBlob[46] = mBeamParams.beamScales[0] | (mBeamParams.beamScales[1] << 16);
    dopplerBlob[47] = mBeamParams.beamScales[2] | (mBeamParams.beamScales[3] << 16);

    dopplerBlob[48] = 0; // FPPTR0
    dopplerBlob[49] = 0; // FPPTR1
    dopplerBlob[50] = 0; // TXPTR0
    dopplerBlob[51] = 0; // TXPTR1

    dopplerBlob[52] = 0;
    dopplerBlob[53] = 0;
    dopplerBlob[54] = 0;

    dopplerBlob[55] = 0;
    dopplerBlob[56] = 0;
    dopplerBlob[57] = 0;

    dopplerBlob[58] = 0;
    dopplerBlob[59] = 0;

    dopplerBlob[60] = 0;

    dopplerBlob[61] = 0;
}

uint32_t DopplerAcq::formAfeWord( uint32_t addr, uint32_t data, uint32_t dieSelect)
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

uint32_t DopplerAcq::formAfeGainCode(float powerMode, float g)
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
