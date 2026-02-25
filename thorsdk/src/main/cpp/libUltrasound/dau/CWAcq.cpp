//
// Copyright 2018 EchoNous Inc.
//

#define LOG_TAG "DauCWAcq"

#include <cstring>
#include <algorithm>
#include <ThorUtils.h>
#include <math.h>
#include <CWAcq.h>
#include <fstream>


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


CWAcq::CWAcq(const std::shared_ptr<UpsReader>& upsReader) :
    mUpsReaderSPtr(upsReader)
{
    LOGD("*** CWAcq - constructor");
}

CWAcq::~CWAcq()
{
    LOGD("*** CWAcq -destructor");
}

float CWAcq::getSfdels( uint32_t imagingCaseId, float focalDepthMm, float focalWidthMm )
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

void CWAcq::calculateTxConfig( uint32_t imagingCaseId, float gateStart, float gateAngle)
{
    const UpsReader::Globals *glbPtr = mUpsReaderSPtr->getGlobalsPtr();
    const UpsReader::TransducerInfo *transducerInfoPtr = mUpsReaderSPtr->getTransducerInfoPtr();

    uint32_t els   = transducerInfoPtr->numElements;
    float    pitch = transducerInfoPtr->pitch;
    float    txfs  = 125.0;
    float    c     = mUpsReaderSPtr->getSpeedOfSound(imagingCaseId);
    float focusoffset = mUpsReaderSPtr->getCwFocusOffset();
    float cwtxfd = gateStart+focusoffset;
    mGateRange = cwtxfd;

    uint32_t utpId = mUpsReaderSPtr->getCwUtpIndex(imagingCaseId, cwtxfd);
    uint32_t txap = mUpsReaderSPtr->getCwTransmitAperture(utpId);
    uint32_t cwtxele = (txap > 60 ? 60 : txap) < 8 ? 8 : (txap > 60 ? 60 : txap);
    uint32_t cwrxele = (62-cwtxele) > 30 ? 30 : (62-cwtxele);
    float    cwcenterfrequency=txfs/4.0/16.0;

    uint32_t ReferenceTime104MHz = 768;

    uint32_t channel_map[] = {15, 14,  7, 6,  5, 13,  4, 12,  3, 11,  2, 10,  1,  0,  9,  8};
    uint32_t index_map[]   = {13, 12, 10, 8,  6,  4,  3,  2, 15, 14, 11,  9,  7,  5,  1,  0};

    uint32_t tx_apw     = cwtxele;
    uint32_t tx_e_first = 0;
    uint32_t tx_e_last  = tx_e_first + tx_apw - 1;

    uint32_t rx_apw     = cwrxele;
    uint32_t rx_e_first = 63 - rx_apw + 1;;
    uint32_t rx_e_last  = 63;

    uint32_t dev0txlen = 0;
    uint32_t dev1txlen = 0;
    uint32_t dev2txlen = 0;
    uint32_t dev3txlen = 0;
    uint32_t dev2rxlen = 0;
    uint32_t dev3rxlen = 0;
    if (tx_apw >=8 and tx_apw <=16)
    {
        dev0txlen = tx_apw;
        dev2rxlen = cwrxele-16;
        dev3rxlen = 16;
    }
    else if (tx_apw >=17 and tx_apw <=32)
    {
        dev0txlen = 16;
        dev1txlen = tx_apw - 16;
        dev2rxlen = cwrxele - 16;
        dev3rxlen = 16;
    }
    else if (tx_apw >=33 and tx_apw <=46)
    {
        dev0txlen = 16;
        dev1txlen = 16;
        dev2txlen = tx_apw - 32;
        dev2rxlen = cwrxele - 16;
        dev3rxlen = 16;
    }
    else if (tx_apw >=47 and tx_apw <=60)
    {
        dev0txlen = 16;
        dev1txlen = 16;
        dev2txlen = 16;
        dev3txlen = tx_apw - 48;
        dev3rxlen = cwrxele;
    }

    uint32_t dev0tx[dev0txlen];
    uint32_t dev1tx[dev1txlen];
    uint32_t dev2tx[dev2txlen];
    uint32_t dev2rx[dev2rxlen];
    uint32_t dev3tx[dev3txlen];
    uint32_t dev3rx[dev3rxlen];

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

    uint32_t index = 0;
    //-----------  Base config begin -------------------------------------
    mLutMSSTTXC[index++] = 0x00201000;
	mLutMSSTTXC[index++] = 0x00000000;
	mLutMSSTTXC[index++] = 0x00000000;
	mLutMSSTTXC[index++] = 0x0000030C;
	mLutMSSTTXC[index++] = 0x000000F7;
	mLutMSSTTXC[index++] = 0x00000000;
	mLutMSSTTXC[index++] = 0x0000000A;
    uint32_t enb0tx=0;
    for (uint32_t i = 0; i < dev0txlen; i++)
    {
        dev0tx[i] = channel_map[i];
    }
    for (uint32_t enbi = 0; enbi<dev0txlen; enbi++)
        enb0tx = enb0tx | (1<<dev0tx[enbi]);
    mLutMSSTTXC[index++] = enb0tx;
    mLutMSSTTXC[index++] = 0x00000000;
	mLutMSSTTXC[index++] = 0x0000000A;
	mLutMSSTTXC[index++] = 0x00401E10;

	mLutMSSTTXC[index++] = 0x00211000;
    mLutMSSTTXC[index++] = 0x00000000;
    mLutMSSTTXC[index++] = 0x00000000;
    mLutMSSTTXC[index++] = 0x0000030C;
    mLutMSSTTXC[index++] = 0x000000F7;
    mLutMSSTTXC[index++] = 0x00000000;
    mLutMSSTTXC[index++] = 0x0000000A;
    uint32_t enb1tx=0;
    for (uint32_t i = 0; i < dev1txlen; i++)
    {
        dev1tx[i] = channel_map[i];
    }
    for (uint32_t enbi = 0; enbi<dev1txlen; enbi++)
        enb1tx = enb1tx | (1<<dev1tx[enbi]);
    mLutMSSTTXC[index++] = enb1tx;
    mLutMSSTTXC[index++] = 0x00000000;
    if (dev1txlen > 0)
        mLutMSSTTXC[index++] = 0x0000000A;
    else
        mLutMSSTTXC[index++] = 0x00000002;
    mLutMSSTTXC[index++] = 0x00401E10;

	mLutMSSTTXC[index++] = 0x00221000;
	mLutMSSTTXC[index++] = 0x00000000;
	mLutMSSTTXC[index++] = 0x00000000;
	mLutMSSTTXC[index++] = 0x0000030C;
	mLutMSSTTXC[index++] = 0x000000F7;
	mLutMSSTTXC[index++] = 0x00000000;
	mLutMSSTTXC[index++] = 0x0000000A;
    uint32_t enb2tx=0;
    for (uint32_t i = 0; i < dev2txlen; i++)
    {
        dev2tx[i] = channel_map[i];
    }
    for (uint32_t enbi = 0; enbi<dev2txlen; enbi++)
        enb2tx = enb2tx | (1 << dev2tx[enbi]);
	mLutMSSTTXC[index++] = enb2tx;
    uint32_t enb2rx=0;
    for (uint32_t i = (16-dev2rxlen); i < 16; i++)
    {
        dev2rx[i] = channel_map[i];
    }
    for (uint32_t enbi = (16-dev2rxlen); enbi < 16; enbi++)
        enb2rx = enb2rx | (1 << dev2rx[enbi]);
    mLutMSSTTXC[index++] = enb2rx;
    if (dev2txlen > 0 and dev2rxlen > 0)
        mLutMSSTTXC[index++] = 0x0000200A;
    else if (dev2rxlen > 0)
        mLutMSSTTXC[index++] = 0x00002002;
    else if (dev2txlen > 0)
        mLutMSSTTXC[index++] = 0x0000000A;
	mLutMSSTTXC[index++] = 0x00401E10;

	mLutMSSTTXC[index++] = 0x00231000;
    mLutMSSTTXC[index++] = 0x00000000;
    mLutMSSTTXC[index++] = 0x00000000;
    mLutMSSTTXC[index++] = 0x0000030C;
    mLutMSSTTXC[index++] = 0x000000F7;
    mLutMSSTTXC[index++] = 0x00000000;
    mLutMSSTTXC[index++] = 0x0000000A;
    uint32_t enb3tx=0;
    for (uint32_t i = 0; i < dev3txlen; i++)
    {
        dev3tx[i] = channel_map[i];
    }
    for (uint32_t enbi = 0; enbi<dev3txlen; enbi++)
        enb3tx = enb3tx | (1<<dev3tx[enbi]);
    mLutMSSTTXC[index++] = enb3tx;
    uint32_t enb3rx=0;
    for (uint32_t i = (16-dev3rxlen); i < 16; i++) {
        dev3rx[i] = channel_map[i];
    }
    for (uint32_t enbi = (16-dev3rxlen); enbi < 16; enbi++)
        enb3rx = enb3rx | (1<<dev3rx[enbi]);
    mLutMSSTTXC[index++] = enb3rx;
    if (dev3txlen > 0 and dev3rxlen > 0)
        mLutMSSTTXC[index++] = 0x0000200A;
    else if (dev3rxlen > 0)
        mLutMSSTTXC[index++] = 0x00002002;
    else if (dev3txlen > 0)
        mLutMSSTTXC[index++] = 0x0000000A;
    mLutMSSTTXC[index++] = 0x00401E10;

    //---------- CW waveform begin ---------------------------------------------------------
    {
        mLutMSSTTXC[index++] = 0x00200000;
        mLutMSSTTXC[index++] = 0x0000001F;
        mLutMSSTTXC[index++] = 0x000041E6;
        mLutMSSTTXC[index++] = 0x000001E9;
        mLutMSSTTXC[index++] = 0x0000001F;
        mLutMSSTTXC[index++] = 0x0000000E;
        mLutMSSTTXC[index++] = 0x00000000;
        mLutMSSTTXC[index++] = 0x00000000;
        mLutMSSTTXC[index++] = 0x00400000;
        if (dev1txlen > 0)
        {
            mLutMSSTTXC[index++] = 0x00210000;
            mLutMSSTTXC[index++] = 0x0000001F;
            mLutMSSTTXC[index++] = 0x000041E6;
            mLutMSSTTXC[index++] = 0x000001E9;
            mLutMSSTTXC[index++] = 0x0000001F;
            mLutMSSTTXC[index++] = 0x0000000E;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00400000;
        }
        else
        {
            mLutMSSTTXC[index++] = 0x00210000;
            mLutMSSTTXC[index++] = 0x0000000E;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00400000;
        }
        if (dev2txlen > 0)
        {
            mLutMSSTTXC[index++] = 0x00220000;
            mLutMSSTTXC[index++] = 0x0000001F;
            mLutMSSTTXC[index++] = 0x000041E6;
            mLutMSSTTXC[index++] = 0x000001E9;
            mLutMSSTTXC[index++] = 0x0000001F;
            mLutMSSTTXC[index++] = 0x0000000E;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00400000;
        }
        else
        {
            mLutMSSTTXC[index++] = 0x00220000;
            mLutMSSTTXC[index++] = 0x0000000E;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00400000;
        }
        if (dev3txlen > 0)
        {
            mLutMSSTTXC[index++] = 0x00230000;
            mLutMSSTTXC[index++] = 0x0000001F;
            mLutMSSTTXC[index++] = 0x000041E6;
            mLutMSSTTXC[index++] = 0x000001E9;
            mLutMSSTTXC[index++] = 0x0000001F;
            mLutMSSTTXC[index++] = 0x0000000E;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00400000;
        }
        else
        {
            mLutMSSTTXC[index++] = 0x00230000;
            mLutMSSTTXC[index++] = 0x0000000E;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00000000;
            mLutMSSTTXC[index++] = 0x00400000;
        }
    }
    mBaseConfigLength = index;

    //----------- CW Delay
    {
        float txfp[64][2];
        for (uint32_t eleIndex = 0; eleIndex < els; eleIndex++)
        {
            txfp[eleIndex][0] = cwtxfd * sin(gateAngle);
            txfp[eleIndex][1] = cwtxfd * cos(gateAngle);
        }

        int32_t  txfdel[64]; // delay table for every element
        float k = -txfs/c;
        int32_t offset = referenceTime104MHz;

        rxDelay( elfp, els, txfp, k, 0, txfdel);
        int32_t temp = txfdel[0];
        for (uint32_t minind=1; minind < 64; minind++)
        {
            if (temp>txfdel[minind])
            {
                temp = txfdel[minind];
            }
        }

        for (uint32_t ind=0; ind < 64; ind++)
        {
            txfdel[ind] = txfdel[ind]-temp;
        }

        int deviceId = 0;
        for (deviceId = 0; deviceId < 4; deviceId++)
        {
            mLutMSSTTXC[index++] = 0x0020100A | (deviceId << 16);
            for (int ci = 0; ci < 15; ci++)
            {
                mLutMSSTTXC[index++] = txfdel[index_map[ci] + deviceId*16];
            }
            mLutMSSTTXC[index++] = 0x00400000 | txfdel[index_map[15] + deviceId*16];
        }

        // waveform select (pointer at 6e, len 24)
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
        mLutMSSTTXC[index++] = 0x00271000;
        mLutMSSTTXC[index++] = 0x00400000;
    }
    mMSSTTXClength = index;
}

void CWAcq::calculateAtgc( uint32_t imagingCaseId, float gateStart, float gateAngle )
{
    const UpsReader::Globals *glbPtr = mUpsReaderSPtr->getGlobalsPtr();
    const UpsReader::TransducerInfo *transducerInfoPtr = mUpsReaderSPtr->getTransducerInfoPtr();

    uint32_t bitset;
    mUpsReaderSPtr->getAfeInputResistance( glbPtr->afeResistSelectId, bitset );

    uint32_t gainmode = glbPtr->afeGainSelectIdC;
    float    gainval;
    mUpsReaderSPtr->getAfeGainModeParams( gainmode, gainval);


    uint32_t powerMode;

    uint32_t els   = transducerInfoPtr->numElements;
    float    pitch = transducerInfoPtr->pitch;
    float    txfs  = 125.0;
    float    c     = mUpsReaderSPtr->getSpeedOfSound(imagingCaseId);
    float cwtxfd = gateStart;
    uint32_t utpId = mUpsReaderSPtr->getCwUtpIndex(imagingCaseId, cwtxfd);
    uint32_t txap = mUpsReaderSPtr->getCwTransmitAperture(utpId);
    uint32_t cwtxele = (txap > 60 ? 60 : txap) < 8 ? 8 : (txap > 60 ? 60 : txap);
    uint32_t cwrxele = (62-cwtxele) > 30 ? 30 : (62-cwtxele);
    float    cwcenterfrequency=txfs/4.0/16.0;

    uint32_t CWGAP = 2;
    uint32_t ReferenceTime104MHz = 768;

    float txfp[64][2];
    for (uint32_t eleIndex = 0; eleIndex < els; eleIndex++)
    {
        txfp[eleIndex][0] = cwtxfd * sin(gateAngle);
        txfp[eleIndex][1] = cwtxfd * cos(gateAngle);
    }
    int32_t  txfdel[64]; // delay table for every element
    int32_t  cwp_el[64]; // delay table for every element
    int32_t v;
    float k = 16*cwcenterfrequency/c;

    float    elfp[64][2];
    {
        float initVal = -1.0f *(els - 1)*pitch/2;
        for (uint32_t eleIndex = 0; eleIndex < els; eleIndex++)
        {
            elfp[eleIndex][0] = initVal + (eleIndex * pitch);
            elfp[eleIndex][1] = 0.0f;
        }
    }
    rxDelay( elfp, els, txfp, k, 0, txfdel);

    int32_t temp = txfdel[0];
    for (uint32_t minind=1; minind < 64; minind++)
    {
        if (temp>txfdel[minind])
        {
            temp = txfdel[minind];
        }
    }

    for (uint32_t ind=0; ind < 64; ind++)
    {
        txfdel[ind] = txfdel[ind]-temp;
    }

    for (int32_t ind=63; ind >=0; ind--)
    {
        cwp_el[ind] = ((txfdel[63-ind]) % 16 + 16 ) % 16;
    }

    for (int i = 0; i < TBF_LUT_MSAFEINIT_SIZE; i++)
        mLutMSAFEINIT[i] = 0;

    uint32_t index = 0;

    mLutMSAFEINIT[index++] = afe_write_die(0, 0x10);  // Enables programming of the DTGC register map
    mLutMSAFEINIT[index++] = afe_write_die(0x1,1,true,true,false,false);

    mLutMSAFEINIT[index++] = afe_write_die(0, 0x0);

    for (uint32_t cc = 0; cc < 4; cc++)
    {
        // Bottom die
        v=cwp_el[cc*8+0]+cwp_el[cc*8+2]*(2<<3)+cwp_el[cc*8+4]*(2<<7)+cwp_el[cc*8+6]*(2<<11);
        mLutMSAFEINIT[index++] = afe_write_die(0xC1+cc,v,false,true,false,false);
        // Top die
        v=cwp_el[cc*8+1]+cwp_el[cc*8+3]*(2<<3)+cwp_el[cc*8+5]*(2<<7)+cwp_el[cc*8+7]*(2<<11);
        mLutMSAFEINIT[index++] = afe_write_die(0xC1+cc,v,true,false,false,false);
    }
    mLutMSAFEINIT[index++] = afe_write_die(0xc7, 0x200, true,true,false,false); // disable hpf

    //Get LNA power down bits from RX aperture
    //Note the RX channels are reversed from TX
    uint32_t e_max = cwrxele-1;         // Last RX element
    uint32_t e_min = 0 ;     // First RX element
    e_min = e_min > 0 ? e_min : 0;
    bool lna_rx_en[64];
    uint32_t ee = 0;
    for (ee = 0; ee < 64; ee++)
        lna_rx_en[ee] = false;

    for (ee = 0; ee < 64; ee++) {
        if ((ee >= e_min) && (ee <= e_max))
            lna_rx_en[ee] = true;
    }
    uint32_t pd_bot = 0;
    uint32_t pd_top = 0;
    for (uint32_t ee = 0; ee < 16; ee++)  // Die channels
    {
        if (lna_rx_en[ee*2]==false)
            pd_bot = pd_bot+(2<<(ee-1));
        if (lna_rx_en[ee*2+1]==false)
            pd_top = pd_top+(2<<(ee-1));
    }

    mLutMSAFEINIT[index++] = afe_write_die(0xC5,pd_bot,false,true,false,false);
    mLutMSAFEINIT[index++] = afe_write_die(0xC5,pd_top,true,false,false,false);
    mLutMSAFEINIT[index++] = afe_write_die(0xc0, 0x11, true,true,false,false); // CW AFE Elable
}

//--------------------------------------------------------------------------------
void CWAcq::buildLuts( const uint32_t imagingCaseId, float gateStart, float gateAngle)
{
    calculateAtgc          ( imagingCaseId, gateStart, gateAngle );
    calculateTxConfig      ( imagingCaseId, gateStart, gateAngle );
}

//--------------------------------------------------------------------------------
void CWAcq::buildCwSequenceBlob()
{
    buildCwSequenceBlob(mSequenceBlob );
}

//--------------------------------------------------------------------------------
void CWAcq::buildCwSequenceBlob(uint32_t cwBlob[62] )
{
    const uint32_t BPF_LS = 4;   // Bandpass filter latency per filter length (64 taps)
    const uint32_t DPL_S  = 128; // Data path latency

    const UpsReader::Globals* glbPtr = mUpsReaderSPtr->getGlobalsPtr();
    const UpsReader::TransducerInfo *tinfoPtr = mUpsReaderSPtr->getTransducerInfoPtr();
    float Fs = glbPtr->samplingFrequency;

     // Parameter #4
    {
        cwBlob[4] = 0x00180041;
    }

    // Parameter #6
    {
        cwBlob[6] = mBaseConfigLength | (104 << 16); //both fixed in dopppler
    }

    // Parameter #13
    {
        cwBlob[13] = 0x80;
    }
}

uint32_t CWAcq::afe_write_die( uint32_t adr, uint32_t d=0, bool afe0top, bool afe0bot, bool afe1top, bool afe1bot)
{
    uint32_t afeWord = d & 0xffff;

    if (afe0bot)
        afeWord |= 0x01000000;

    if (afe0top)
        afeWord |= 0x02000000;

    if (afe1bot)
        afeWord |= 0x04000000;

    if (afe1top)
        afeWord |= 0x08000000;

    afeWord |= (adr & 0xff) << 16;
    return (afeWord);
}

uint32_t CWAcq::formAfeGainCode(float powerMode, float g)
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
