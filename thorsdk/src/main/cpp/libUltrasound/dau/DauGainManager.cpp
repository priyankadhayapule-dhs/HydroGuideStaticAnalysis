//
// Copyright 2017 EchoNous Inc.
//
//
#define LOG_TAG "DauGainManager"

#include <DauLutManager.h>
#include <DauGainManager.h>
#include <TbfRegs.h>
#include <BitfieldMacros.h>
#include <math.h>

DauGainManager::DauGainManager(const std::shared_ptr<DauMemory>& daum,
                               const std::shared_ptr<UpsReader>& upsReader) :
    mDaumSPtr(daum),
    mUpsReaderSptr(upsReader)
{
    LOGD("*** DauGainManager - constructor");
}

DauGainManager::~DauGainManager()
{
    LOGD("*** DauGainManager - destructor");
}

uint32_t DauGainManager::quantizeGain( float gain )
{
    // convert to S3.12 format (sign bit + 3 int bits + 12 fractional bits == 16)
    int32_t qg = (int32_t)floor(gain * (1 << 12));
    return(qg);
}

bool DauGainManager::applyBModeGain(DauLutManager& lutm, float nearGainDb, float farGainDb, uint32_t imagingCase)
{

    bool ret = false;
    sqlite3_stmt* pStmt;
    const float* userGainLut = lutm.getUserGainTable();

    if ( (nearGainDb < -15.0) ||
         (nearGainDb >  15.0) ||
         (farGainDb  < -15.0) ||
         (farGainDb  >  15.0) )
    {
        LOGE("specified gains (%f %f) are out of range", nearGainDb, farGainDb);
        return (false);
    }

    // Get near Gain fraction, far Gain fraction
    float nearbp = 0.0;
    float farbp = 1.0;
    mUpsReaderSptr->getGainBreakpoints( nearbp, farbp );
    if ( (nearbp < 0.0)    ||
         (nearbp > 1.0)    ||
         (farbp  < 0.0)    ||
         (farbp  > 1.0)    ||
         (nearbp >= farbp) )
    {
        LOGE("gain breakpoints [%f, %f] are out of range", nearbp, farbp);
        return (false);
    }

    uint32_t numSamples = 0;
    if ( !mUpsReaderSptr->getNumSamplesB(imagingCase, numSamples) )
    {
        LOGE("applyBModeGain(): failed to get numSamples");
        return (false);
    }

    if ( (numSamples == 0) || (numSamples > 512) )
    {
        LOGE("applyBModeGain(): numSamples (%d) > 512", numSamples);
        return( false);
    }

    uint32_t nearRange = (uint32_t) floor(nearbp * numSamples);
    uint32_t farRange  = (uint32_t) floor(farbp  * numSamples);

    if ( nearRange == farRange )
    {
        LOGE("applyBModeGain() nearRange == farRange (%u)", nearRange);
        return (false);
    }

    float nearGain;
    float farGain;

    //----------------------------------------------------------------
    // Fill nearRange with nearGain

    uint32_t quantizedGain;
    for (uint32_t index = 0; index < nearRange; index++)
    {
        if ((nearGainDb + userGainLut[index]) < -15)
        {
            nearGain = (float)pow(10.0, (-15)/20.0f);
        }
        else if ((nearGainDb + userGainLut[index]) > 15)
        {
            nearGain = (float)pow(10.0, (15)/20.0f);
        }
        else {
            nearGain = (float)pow(10.0, (nearGainDb + userGainLut[index])/20.0f);
        }

        quantizedGain = quantizeGain( nearGain );
        mGainTable[index] = quantizedGain;

        if ((nearGainDb + userGainLut[index+512]) < -15)
        {
            nearGain = (float)pow(10.0, (-15)/20.0f);
        }
        else if ((nearGainDb + userGainLut[index+512]) > 15)
        {
            nearGain = (float)pow(10.0, (15)/20.0f);
        }
        else {
            nearGain = (float)pow(10.0, (nearGainDb + userGainLut[index+512])/20.0f);
        }

        quantizedGain = quantizeGain( nearGain );
        mGainTable[index+512] = quantizedGain;
    }

    //----------------------------------------------------------------
    // Use linearly interpolated gain values in db from nearRange to
    // farRange

    float gainDeltaDb = (farGainDb - nearGainDb)/(farRange - nearRange);
    float gainDb;
    for (uint32_t index = nearRange; index < farRange; index++)
    {
        gainDb = userGainLut[index] + nearGainDb + gainDeltaDb*(index-nearRange);
        if (gainDb < -15)
        {
            gainDb = -15;
        }
        else if (gainDb > 15)
        {
            gainDb = 15;
        }

        mGainTable[index] = quantizeGain((float)pow(10.0, gainDb/20.0));

        gainDb = userGainLut[index+512] + nearGainDb + gainDeltaDb*(index-nearRange);
        if (gainDb < -15)
        {
            gainDb = -15;
        }
        else if (gainDb > 15)
        {
            gainDb = 15;
        }
        mGainTable[index+512] = quantizeGain((float)pow(10.0, gainDb/20.0));
    }

    //----------------------------------------------------------------
    // Fill rest of the table with farGain value

    for (uint32_t index = farRange; index < numSamples; index++)
    {
        if ((farGainDb + userGainLut[index]) < -15)
        {
            farGain = (float)pow(10.0, (-15)/20.0f);
        }
        else if ((farGainDb + userGainLut[index]) > 15)
        {
            farGain = (float)pow(10.0, (15)/20.0f);
        }
        else {
            farGain = (float)pow(10.0, (farGainDb + userGainLut[index])/20.0f);
        }

        quantizedGain = quantizeGain( farGain );
        mGainTable[index] = quantizedGain;

        if ((farGainDb + userGainLut[index+512]) < -15)
        {
            farGain = (float)pow(10.0, (-15)/20.0f);
        }
        else if ((farGainDb + userGainLut[index+512]) > 15)
        {
            farGain = (float)pow(10.0, (15)/20.0f);
        }
        else {
            farGain = (float)pow(10.0, (farGainDb + userGainLut[index+512])/20.0f);
        }

        quantizedGain = quantizeGain( farGain );
        mGainTable[index+512] = quantizedGain;
    }

    //-------- Load the newly computed values into Gain LUT ---------
    {
        uint32_t ckRegVal;

        mDaumSPtr->read(TBF_REG_CK0+TBF_BASE, &ckRegVal, 1);
        BFN_SET(ckRegVal, 1, DTGC_WAKEUP);
        mDaumSPtr->write(&ckRegVal, TBF_REG_CK0+TBF_BASE, 1);

        bool retVal = mDaumSPtr->write(mGainTable, TBF_LUT_MSCBC+TBF_BASE, numSamples);

        BFN_SET(ckRegVal, 0, DTGC_WAKEUP);
        mDaumSPtr->write(&ckRegVal, TBF_REG_CK0+TBF_BASE, 1);

        if (!retVal)
        {
            LOGE("applyBGain(): failed to write gain table");
            return(false);
        }
    }

    return (true);
}
