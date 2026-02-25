//
// Copyright 2019 EchoNous Inc.
//
//
#define LOG_TAG "ApiManager"

#include "ApiManager.h"
#include "ThorUtils.h"
#include <json/json.h>
#include "ThorConstants.h"

/************************************************************************
* Function name:       Invoke
*
* Purpose:    Top level function of the AP & I control module.  Accepts
*             an input vector and an API data table, then calculates the
*             the appropriate drive voltage to be commanded.  Populates the
*             API output vector with the calculated parameters based on
*             the commanded voltage.
*
* Inputs:     API_INVECTOR_p_t pInvector   Pointer to an EchoNous input vector
*             API_TABLE_p_t pApiTable:  Pointer to the EchoNous API table
*             API_OUTVECTOR_p_t pApiOutVector:  Pointer to the EchoNous output
*                                               vector
*			  float vdrvCmd 
*
*************************************************************************/
  
#include "ApiManager.h"

#define FLATTOP_APEX 0.5445749f

ApiManager::ApiManager(const std::shared_ptr<UpsReader>& upsReader,
                       uint32_t probeType) :
    mNumUtps(0),
    mUpsReaderSPtr(upsReader),
    mProbeType(probeType),
    mStartUTPID(0)
{
    mNumUtps = mUpsReaderSPtr->getApiTableSize();
    LOGD("*** ApiManager - constructor");
}


int ApiManager::Invoke (API_INVECTOR_p_t pApiInvector,
                        API_TABLE_p_t pApiTable,
                        API_OUTVECTOR_p_t pApiOutvector)
{
    int status = 0;
    bool newCalc = false;
    m_calc.CalculateOutputVector (pApiInvector, pApiTable, pApiOutvector, true);
    logInputOutputVectors();
    if (pApiInvector->txVdrv != pApiOutvector->vdrvCmd)
    {
		newCalc = true;;
    }
    if (newCalc)
    {
		API_INVECTOR_t newInVector;   //  Create a new input vector

        newInVector = *pApiInvector;  //  Set values of the new input vector equal to the old one,
        newInVector.txVdrv = pApiOutvector->vdrvCmd;   //  except set the new vdrv.
        /*  Now calculate the new output vector  */
        m_calc.CalculateOutputVector(&newInVector, pApiTable, pApiOutvector, API_FALSE);
        logInputOutputVectors();
    }
    return status;
}

ThorStatus ApiManager::initApiTable()
{
    setupApiTableHeader();
    return (initUtpIdTable());
}

void ApiManager::setupApiTableHeader()
{
    const UpsReader::Globals* glbPtr = mUpsReaderSPtr->getGlobalsPtr();
    const UpsReader::TransducerInfo* tinfoPtr = mUpsReaderSPtr->getTransducerInfoPtr();

    mApiTable.api_table_header.SPTA_lim     = glbPtr->ispat3Limit;
    mApiTable.api_table_header.MI_lim       = glbPtr->miLimit;
    mApiTable.api_table_header.TI_lim       = glbPtr->tiLimit;
    mApiTable.api_table_header.ambient      = glbPtr->ambientTempC;
    mApiTable.api_table_header.Apex         = tinfoPtr->apex;  //
    mApiTable.api_table_header.ElePitch     = tinfoPtr->pitch;
    mApiTable.api_table_header.CrysToSkin   = tinfoPtr->lensDelay / 10.0f * 1.0f;
    mApiTable.api_table_header.EleLength    = tinfoPtr->arrayElevation;
    mApiTable.api_table_header.NumEles      = tinfoPtr->numElements;
    if ((mProbeType & DEV_PROBE_MASK) == PROBE_TYPE_LINEAR) {
        mApiTable.api_table_header.Class = API_LA; // BUGBUG: needs to come from UPS
    }
    else {
        mApiTable.api_table_header.Class = API_PHA; // BUGBUG: needs to come from UPS
    }

    // store default values
    mDefaultApex =  mApiTable.api_table_header.Apex;
}


ThorStatus ApiManager::initUtpIdTable()
{
    const UpsReader::Globals* glbPtr = mUpsReaderSPtr->getGlobalsPtr();
    const UpsReader::TransducerInfo* tinfoPtr = mUpsReaderSPtr->getTransducerInfoPtr();
    Json::Reader reader;
    Json::Value obj;

    if (mNumUtps > MAX_API_TABLE_SIZE)
    {
        LOGE("Number of UTPs (%d) in API table exceeds limit (%d), mNumUtps, MAX_API_TABLE_SIZE");
        return (TER_APIMGR_NUM_UTPS);
    }

    if ((mProbeType & DEV_PROBE_MASK) == PROBE_TYPE_LINEAR)
        mStartUTPID = 1;
    else
        mStartUTPID = 0;

    float imax_3_ref = 0.0f;
    for (uint32_t i = mStartUTPID; i < mNumUtps; i++)
    {
        // get UTP Data string for ID =
        if (!mUpsReaderSPtr->getUtpDataString(i, mUtpString))
        {
            return (TER_APIMGR_CORRUPTED_REF_DATA);
        }
        bool parsingSuccessful = reader.parse(mUtpString, obj);
        if ( !parsingSuccessful)
        {
            return (TER_APIMGR_UTP_STRING);
        }
        auto utpId = obj["UTP"].asInt();
        uint32_t txApertureElement = mUpsReaderSPtr->getTxApertureElement(utpId);
        float pd = obj["PD"].asFloat();
        float pii_3_ref = obj["PII3Max"].asFloat();
        float mi = obj["MI"].asFloat();
        if (mi > imax_3_ref)
            imax_3_ref = mi;
        float refVdrv = obj["Ref Vdrv"].asFloat();

        mUtpIdTable[i].utpID         = utpId;
        mUtpIdTable[i].apEles        = txApertureElement;
        float ld_ref = obj["ld_ref"].asFloat();
        if (ld_ref < 1.0)
            ld_ref = 1.0;
        mUtpIdTable[i].ld_ref        = ld_ref;
        mUtpIdTable[i].aaprt_ref     = txApertureElement * tinfoPtr->pitch * tinfoPtr->arrayElevation * 0.01f; // 0.01 is for mm^2 to cm^2 conversion
        mUtpIdTable[i].cFreq         = obj["FcAtZPii0Max"].asFloat();
        mUtpIdTable[i].w01sFrac      = obj["W01"].asFloat();
        mUtpIdTable[i].w0sFrac       = obj["W0"].asFloat();
        mUtpIdTable[i].pii_0_ref     = obj["PII0Max"].asFloat();
        mUtpIdTable[i].pii_3_ref     = pii_3_ref;
        mUtpIdTable[i].sii_3_ref     = obj["Sii3AtZsii3"].asFloat();
        mUtpIdTable[i].sppa_3_ref    = obj["Ipa3AtZPii3"].asFloat();
        mUtpIdTable[i].mi_3_ref      = mi;
        mUtpIdTable[i].vdrv_ref      = refVdrv;
        mUtpIdTable[i].w0u_ref       = obj["W0"].asFloat();
        mUtpIdTable[i].vdrv_corr     = 1.0;
        mUtpIdTable[i].TISas_ref     = obj["TISas"].asFloat();
        mUtpIdTable[i].TISbs_ref     = obj["TISbs"].asFloat();
        mUtpIdTable[i].TIBas_ref     = obj["TIBas"].asFloat();
        mUtpIdTable[i].TIBbs_ref     = obj["TIBbs"].asFloat();
        mUtpIdTable[i].min_ref_vdrv  = obj["Min Vdrv"].asFloat();
        mUtpIdTable[i].max_ref_vdrv  = obj["Max Vdrv"].asFloat();
        mUtpIdTable[i].pr_3          = obj["Pr3AtZPii3Max"].asFloat();

        auto quadPolyCoeffs = obj["Quad Poly Coeffs"];
        auto miCoeffs = quadPolyCoeffs["MI"];
        mUtpIdTable[i].mi_coeff_A = miCoeffs[0].asFloat();
        mUtpIdTable[i].mi_coeff_B = miCoeffs[1].asFloat();
        mUtpIdTable[i].mi_coeff_C = miCoeffs[2].asFloat();

        auto cubicPolyCoeffs = obj["Cubic Poly Coeffs"];
        auto pii3Coeffs = cubicPolyCoeffs["PII3Max"];
        mUtpIdTable[i].pii3_coeff_A = pii3Coeffs[0].asFloat();
        mUtpIdTable[i].pii3_coeff_B = pii3Coeffs[1].asFloat();
        mUtpIdTable[i].pii3_coeff_C = pii3Coeffs[2].asFloat();
        mUtpIdTable[i].pii3_coeff_D = pii3Coeffs[3].asFloat();
        auto sii3Coeffs = cubicPolyCoeffs["Sii3AtZsii3"];
        mUtpIdTable[i].sii3_coeff_A = sii3Coeffs[0].asFloat();
        mUtpIdTable[i].sii3_coeff_B = sii3Coeffs[1].asFloat();
        mUtpIdTable[i].sii3_coeff_C = sii3Coeffs[2].asFloat();
        mUtpIdTable[i].sii3_coeff_D = sii3Coeffs[3].asFloat();
    }

    for (uint32_t i = mStartUTPID; i < mNumUtps; i++)
    {
        mUtpIdTable[i].imax_3_ref = imax_3_ref;
    }
    return (THOR_OK);
}


ThorStatus ApiManager::findApiTableIndices(uint32_t numUtps, int32_t utpIds[3], int32_t apiTableIndices[3])
{
    bool utpFound = false;
    if (numUtps > 3)
    {
        LOGE("ApiManager does not support more than 3 UTPs for a given imaging case (%d), numUtps");
        return (TER_APIMGR_NUM_UTPS);
    }

    for (uint32_t i = 0; i < numUtps; i++)
    {
        utpFound = false;
        for (uint32_t j = mStartUTPID; j < mNumUtps; j++)
        {
            if (utpIds[i] == mUtpIdTable[j].utpID)
            {
                utpFound = true;
                apiTableIndices[i] = j;
                LOGD("UTP ID of index = %d is %d", i, j);
                break;
            }
        }
        if (!utpFound)
        {
            LOGD("%s UTP ID %d was not found in API Table", __func__, utpIds[i]);
            logApiTable();
            return (TER_APIMGR_UTPID);
        }
    }
    return (THOR_OK);
}

ThorStatus ApiManager::setupInputVector(uint32_t imagingCaseId, uint32_t imagingMode, bool isLinearProbe,
        DauSequenceBuilder* seqBuilderPtr, ColorAcq* colorAcqPtr, ColorAcqLinear* colorAcqLinearPtr,
        DopplerAcq* dopplerAcqPtr, DopplerAcqLinear* dopplerAcqLinearPtr, CWAcq* cwAcqPtr, bool isPWTDI, uint32_t organId, float gateRange)
{
    int32_t utpIds[3];
    int32_t apiTableIndices[3];
    float lineDensity;
    const UpsReader::Globals* gblPtr = mUpsReaderSPtr->getGlobalsPtr();

    API_INVECTOR_UTP_p_t  pInpUtpVector;

    float frameRate = seqBuilderPtr->getFrameRate();

    switch (imagingMode)
    {
        case IMAGING_MODE_COLOR:
        {
            mInputVector.numUtps = 2;
            float focalDepthMm;
            uint32_t ensembleSize;
            if (isLinearProbe) {
                focalDepthMm = colorAcqLinearPtr->getFocalDepth();
                ensembleSize = colorAcqLinearPtr->getEnsembleSize();
            }
            else {
                focalDepthMm = colorAcqPtr->getFocalDepth();
                ensembleSize = colorAcqPtr->getEnsembleSize();
            }

            bool utpsFound = mUpsReaderSPtr->getUtpIds(imagingCaseId, imagingMode, utpIds,
                                                       focalDepthMm);
            if (!utpsFound) {
                return (TER_APIMGR_UTPID);
            }
            if (THOR_OK == findApiTableIndices(mInputVector.numUtps, utpIds, apiTableIndices))
            {
                mApiTable.api_utpid_table[0] = &(mUtpIdTable[apiTableIndices[0]]);
                mApiTable.api_utpid_table[1] = &(mUtpIdTable[apiTableIndices[1]]);

                if ((mProbeType & DEV_PROBE_MASK) == PROBE_TYPE_LINEAR)
                {
                    mUpsReaderSPtr->getColorLineDensityLinear(imagingCaseId, lineDensity);
                }
                else {
                    mUpsReaderSPtr->getColorLineDensity(imagingCaseId, lineDensity);
                }

                /* Fill in InpUtpVector for color */
                pInpUtpVector = &mInputVector.api_invector_utp[1];
                pInpUtpVector->utp = 1;
                pInpUtpVector->fr = frameRate;
                pInpUtpVector->ppl = ensembleSize;
                pInpUtpVector->density = lineDensity;
                pInpUtpVector->lpFrame = (uint32_t) round(1.0*seqBuilderPtr->getNumCNodes()/pInpUtpVector->ppl);
                pInpUtpVector->prf = frameRate * seqBuilderPtr->getNumCNodes();
                pInpUtpVector->pulsMode = API_Ps; // Pulsed scanned

                /* Fill in InpUtpVector for B-Mode */
                pInpUtpVector = &mInputVector.api_invector_utp[0];
                pInpUtpVector->utp = 0;
                pInpUtpVector->fr = frameRate;
                pInpUtpVector->ppl = 1;
                pInpUtpVector->density = lineDensity;
                pInpUtpVector->lpFrame = seqBuilderPtr->getNumBNodes();
                pInpUtpVector->prf = frameRate * pInpUtpVector->lpFrame;
                pInpUtpVector->pulsMode = API_Es; // Echo scanned
                return (THOR_OK);
            }
            else
            {
                return (TER_APIMGR_UTPID);
            }
        }

        case IMAGING_MODE_M:
        {
            bool utpsFound = mUpsReaderSPtr->getUtpIds(imagingCaseId, imagingMode, utpIds);
            if (!utpsFound)
            {
                return (TER_APIMGR_UTPID);
            }
            mInputVector.numUtps = 2;
            if (THOR_OK == findApiTableIndices(mInputVector.numUtps, utpIds, apiTableIndices))
            {
                mApiTable.api_utpid_table[0] = &(mUtpIdTable[apiTableIndices[0]]);
                mApiTable.api_utpid_table[1] = &(mUtpIdTable[apiTableIndices[1]]);

                if ((mProbeType & DEV_PROBE_MASK) == PROBE_TYPE_LINEAR)
                {
                    mUpsReaderSPtr->getMLineDensityLinear(imagingCaseId, lineDensity);
                }
                else {
                    mUpsReaderSPtr->getMLineDensity(imagingCaseId, lineDensity);
                }
                /* Fill in InpUtpVector for M-Mode */
                pInpUtpVector = &mInputVector.api_invector_utp[1];
                pInpUtpVector->utp = 1;
                pInpUtpVector->prf = gblPtr->mPrf;
                pInpUtpVector->fr = frameRate;
                pInpUtpVector->ppl = 1;
                pInpUtpVector->density = lineDensity;
                pInpUtpVector->lpFrame = 1;
                pInpUtpVector->pulsMode = API_Eu; // Echo unscanned

                /* Fill in InpUtpVector for B-Mode */
                pInpUtpVector = &mInputVector.api_invector_utp[0];
                pInpUtpVector->utp = 0;
                pInpUtpVector->prf = gblPtr->mPrf;
                pInpUtpVector->fr = frameRate;
                pInpUtpVector->ppl = 1;
                pInpUtpVector->density = lineDensity;
                pInpUtpVector->lpFrame = seqBuilderPtr->getNumBNodes();
                pInpUtpVector->pulsMode = API_Es; // Echo scanned
                return (THOR_OK);
            }
            else
            {
                return (TER_APIMGR_UTPID);
            }
        }
        case IMAGING_MODE_B:
        {
            utpIds[0] = utpIds[1] = utpIds[2] = -1;
            bool utpsFound = mUpsReaderSPtr->getUtpIds(imagingCaseId, imagingMode, utpIds);
            if (!utpsFound)
            {
                return (TER_APIMGR_UTPID);
            }
            if (utpIds[1] == -1)
            {
                mInputVector.numUtps = 1;
            }
            else if (utpIds[2] == -1)
            {
                mInputVector.numUtps = 2;
            }
            else
            {
                mInputVector.numUtps = 3;
            }
            if (THOR_OK == findApiTableIndices(mInputVector.numUtps, utpIds, apiTableIndices))
            {
                mApiTable.api_utpid_table[0] = &(mUtpIdTable[apiTableIndices[0]]);
                if ((mProbeType & DEV_PROBE_MASK) == PROBE_TYPE_LINEAR)
                {
                    mUpsReaderSPtr->getBLineDensityLinear(imagingCaseId, lineDensity);
                }
                    else {
                    mUpsReaderSPtr->getBLineDensity(imagingCaseId, lineDensity);
                }
                pInpUtpVector = &mInputVector.api_invector_utp[0];
                pInpUtpVector->utp = 0;
                pInpUtpVector->fr = frameRate;
                pInpUtpVector->ppl = 1;
                pInpUtpVector->density = lineDensity;
                pInpUtpVector->lpFrame = seqBuilderPtr->getNumBNodes();
                pInpUtpVector->prf = frameRate * pInpUtpVector->lpFrame;
                pInpUtpVector->pulsMode = API_Es; // Echo scanned
                if (mInputVector.numUtps == 2)
                {
                    // Adjust the lines per frame and frame rate for the first UTP
                    pInpUtpVector->lpFrame = pInpUtpVector->lpFrame >> 1;
                    pInpUtpVector->prf = frameRate * pInpUtpVector->lpFrame;

                    mApiTable.api_utpid_table[1] = &(mUtpIdTable[apiTableIndices[1]]);
                    if ((mProbeType & DEV_PROBE_MASK) == PROBE_TYPE_LINEAR)
                    {
                        mUpsReaderSPtr->getBLineDensityLinear(imagingCaseId, lineDensity);
                    }
                    else {
                        mUpsReaderSPtr->getBLineDensity(imagingCaseId, lineDensity);
                    }
                    pInpUtpVector = &mInputVector.api_invector_utp[1];
                    pInpUtpVector->utp = 1;
                    pInpUtpVector->fr = frameRate;
                    pInpUtpVector->ppl = 1;
                    pInpUtpVector->density = lineDensity;
                    pInpUtpVector->lpFrame = seqBuilderPtr->getNumBNodes() >> 1;
                    pInpUtpVector->prf = frameRate * pInpUtpVector->lpFrame;
                    pInpUtpVector->pulsMode = API_Es; // Echo scanned
                }
                else if (mInputVector.numUtps == 3)
                {
                    mApiTable.api_utpid_table[0] = &(mUtpIdTable[apiTableIndices[0]]);
                    if ((mProbeType & DEV_PROBE_MASK) == PROBE_TYPE_LINEAR)
                    {
                        mUpsReaderSPtr->getBLineDensityLinear(imagingCaseId, lineDensity);
                    }
                    else {
                        mUpsReaderSPtr->getBLineDensity(imagingCaseId, lineDensity);
                    }
                    frameRate = frameRate/3;
                    pInpUtpVector = &mInputVector.api_invector_utp[0];
                    pInpUtpVector->utp = 0;
                    pInpUtpVector->fr = frameRate;
                    pInpUtpVector->ppl = 1;
                    pInpUtpVector->density = lineDensity;
                    pInpUtpVector->lpFrame = (uint32_t)(seqBuilderPtr->getNumBNodes()/3);
                    pInpUtpVector->prf = frameRate * pInpUtpVector->lpFrame;
                    pInpUtpVector->pulsMode = API_Es; // Echo scanned

                    mApiTable.api_utpid_table[1] = &(mUtpIdTable[apiTableIndices[1]]);
                    if ((mProbeType & DEV_PROBE_MASK) == PROBE_TYPE_LINEAR)
                    {
                        mUpsReaderSPtr->getBLineDensityLinear(imagingCaseId, lineDensity);
                    }
                    else {
                        mUpsReaderSPtr->getBLineDensity(imagingCaseId, lineDensity);
                    }
                    pInpUtpVector = &mInputVector.api_invector_utp[1];
                    pInpUtpVector->utp = 1;
                    pInpUtpVector->fr = frameRate;
                    pInpUtpVector->ppl = 1;
                    pInpUtpVector->density = lineDensity;
                    pInpUtpVector->lpFrame = (uint32_t)(seqBuilderPtr->getNumBNodes()/3);
                    pInpUtpVector->prf = frameRate * pInpUtpVector->lpFrame;
                    pInpUtpVector->pulsMode = API_Es; // Echo scanned

                    mApiTable.api_utpid_table[2] = &(mUtpIdTable[apiTableIndices[2]]);
                    if ((mProbeType & DEV_PROBE_MASK) == PROBE_TYPE_LINEAR)
                    {
                        mUpsReaderSPtr->getBLineDensityLinear(imagingCaseId, lineDensity);
                    }
                    else {
                        mUpsReaderSPtr->getBLineDensity(imagingCaseId, lineDensity);
                    }
                    pInpUtpVector = &mInputVector.api_invector_utp[2];
                    pInpUtpVector->utp = 2;
                    pInpUtpVector->fr = frameRate;
                    pInpUtpVector->ppl = 1;
                    pInpUtpVector->density = lineDensity;
                    pInpUtpVector->lpFrame = (uint32_t)(seqBuilderPtr->getNumBNodes()/3);
                    pInpUtpVector->prf = frameRate * pInpUtpVector->lpFrame;
                    pInpUtpVector->pulsMode = API_Es; // Echo scanned
                }
                return (THOR_OK);
            }
            else
            {
                return (TER_APIMGR_UTPID);
            }
        }

        case IMAGING_MODE_PW:
        {
            mInputVector.numUtps = 1;
            float focalDepthMm;
            uint32_t halfcycle;
            bool utpsFound;
            if (isLinearProbe) {
                focalDepthMm = dopplerAcqLinearPtr->getFocalDepth();
                halfcycle = dopplerAcqLinearPtr->getHalfCycle();
                if (focalDepthMm<10.0)
                    focalDepthMm=10.0;
                if (focalDepthMm>90.0)
                    focalDepthMm=90.0;
                utpsFound = mUpsReaderSPtr->getUtpIdsPW(imagingCaseId, imagingMode, utpIds,
                                                             focalDepthMm, halfcycle, isPWTDI, organId, gateRange);
            }
            else {
                focalDepthMm = dopplerAcqPtr->getFocalDepth();
                halfcycle = dopplerAcqPtr->getHalfCycle();
                utpsFound = mUpsReaderSPtr->getUtpIds(imagingCaseId, imagingMode, utpIds,
                                                           focalDepthMm, halfcycle, isPWTDI);
            }

            if (!utpsFound) {
                return (TER_APIMGR_UTPID);
            }

            if (THOR_OK == findApiTableIndices(mInputVector.numUtps, utpIds, apiTableIndices))
            {
                mApiTable.api_utpid_table[0] = &(mUtpIdTable[apiTableIndices[0]]);

                /* Fill in InpUtpVector for PW-Mode */
                pInpUtpVector = &mInputVector.api_invector_utp[0];
                pInpUtpVector->utp = 0;

                if (isLinearProbe)
                    pInpUtpVector->prf = dopplerAcqLinearPtr->getPrf();
                else
                    pInpUtpVector->prf = dopplerAcqPtr->getPrf();

                pInpUtpVector->fr = -1;
                pInpUtpVector->ppl = -1;
                pInpUtpVector->density = -1;
                pInpUtpVector->lpFrame = -1;
                pInpUtpVector->pulsMode = API_Pu;

                /*
                mInputVector.numUtps = 1;
                mInputVector.txVdrv = 10;

                mInputVector.api_invector_utp[0].utp = 0;
                mInputVector.api_invector_utp[0].prf = 17361.1;
                mInputVector.api_invector_utp[0].fr = -1;			 //  Unscanned mode so set to -1
                mInputVector.api_invector_utp[0].ppl = -1;			 //  Unscanned mode so set to -1
                mInputVector.api_invector_utp[0].density = -1;		 //  Unscanned mode so set to -1
                mInputVector.api_invector_utp[0].lpFrame = -1;		 //  Unscanned mode so set to -1
                mInputVector.api_invector_utp[0].pulsMode = API_Pu;	   //  Set pulsed doppler mode
                */
                return (THOR_OK);
            }
            else
            {
                return (TER_APIMGR_UTPID);
            }
        }

        case IMAGING_MODE_CW:
        {
            mInputVector.numUtps = 1;
            float focalDepthMm = cwAcqPtr->getFocalDepth();
            bool utpsFound = mUpsReaderSPtr->getUtpIds(imagingCaseId, imagingMode, utpIds,
                                                       focalDepthMm, 100);

            if (!utpsFound) {
                return (TER_APIMGR_UTPID);
            }

            if (THOR_OK == findApiTableIndices(mInputVector.numUtps, utpIds, apiTableIndices))
            {
                mApiTable.api_utpid_table[0] = &(mUtpIdTable[apiTableIndices[0]]);

                /* Fill in InpUtpVector for CW-Mode */
                pInpUtpVector = &mInputVector.api_invector_utp[0];
                pInpUtpVector->utp = 0;
                pInpUtpVector->prf = 1;
                pInpUtpVector->fr = -1;
                pInpUtpVector->ppl = -1;
                pInpUtpVector->density = -1;
                pInpUtpVector->lpFrame = -1;
                pInpUtpVector->pulsMode = API_Cu;
                return (THOR_OK);
            }
            else
            {
                return (TER_APIMGR_UTPID);
            }
        }

        default:
            LOGE("imaging mode %d is not supported", imagingMode);
            return (TER_APIMGR_UTPID);
    }
}

ThorStatus ApiManager::estimateIndices( uint32_t imagingCase,
        uint32_t imagingMode, bool isLinearProbe,
        DauSequenceBuilder* seqBuilderPtr,
        ColorAcq* colorAcqPtr, ColorAcqLinear* colorAcqLinearPtr,
        DopplerAcq* dopplerAcqPtr, DopplerAcqLinear* dopplerAcqLinearPtr, CWAcq* cwAcqPtr,
        float indices[],
        float &txVoltage, bool isPWTDI)
{
    float hvpb, hvnb;
    ThorStatus status;

    uint32_t organId = mUpsReaderSPtr->getImagingOrgan(imagingCase);
    uint32_t organGlobalId = mUpsReaderSPtr->getOrganGlobalId(organId);

    if (organGlobalId == BLADDER_GLOBAL_ID)
    {
        mApiTable.api_table_header.Apex = FLATTOP_APEX;
    }
    else
    {
        mApiTable.api_table_header.Apex = mDefaultApex;
    }

    if (IMAGING_MODE_PW == imagingMode)
    {
        mUpsReaderSPtr->getPwHvpsValues(imagingCase, hvpb, hvnb);
    }
    else if (IMAGING_MODE_CW == imagingMode) //3.3V for actual hardware, 3.15V for api
    {
        hvpb = 3.15;
        hvnb = 3.15;
    }
    else
    {
        mUpsReaderSPtr->getHvpsValues(imagingCase, hvpb, hvnb);
    }
    mInputVector.txVdrv = hvpb;
    status = setupInputVector(imagingCase, imagingMode, isLinearProbe, seqBuilderPtr, colorAcqPtr, colorAcqLinearPtr,
                              dopplerAcqPtr, dopplerAcqLinearPtr, cwAcqPtr, isPWTDI);
    if (THOR_OK == status)
    {
        Invoke(&mInputVector, &mApiTable, &mOutputVector);
        indices[0] = mOutputVector.mi;
        indices[1] = mOutputVector.tis_display;
        indices[2] = mOutputVector.tib_display;
        indices[3] = mOutputVector.tic_display;
        txVoltage = mOutputVector.vdrvCmd;
        if (abs(mOutputVector.vdrvCmd - mInputVector.txVdrv) > 1.0 )
        {
            LOGD("Requested Tx Voltage (%f) differs from API Manager output (%f",
                    hvpb, mOutputVector.vdrvCmd);  // BUGBUG: handle this condition
        }
    }
    else
    {
        txVoltage = MIN_HVPS_VOLTS;
    }
    return (status);
}

ThorStatus ApiManager::estimateIndicesPW( uint32_t imagingCase,
                                        uint32_t imagingMode, bool isLinearProbe,
                                        DauSequenceBuilder* seqBuilderPtr,
                                        ColorAcq* colorAcqPtr, ColorAcqLinear* colorAcqLinearPtr,
                                        DopplerAcq* dopplerAcqPtr, DopplerAcqLinear* dopplerAcqLinearPtr, CWAcq* cwAcqPtr,
                                        float indices[],
                                        float &txVoltage, bool isPWTDI, uint32_t organId, float gateRange)
{
    float hvpb, hvnb;
    ThorStatus status;
    float focalDepthMm;
    uint32_t halfcycle;
    bool utpsFound;
    int32_t utpIds[3];
    focalDepthMm = dopplerAcqLinearPtr->getFocalDepth();
    halfcycle = dopplerAcqLinearPtr->getHalfCycle();
    focalDepthMm = dopplerAcqLinearPtr->getFocalDepth();
    halfcycle = dopplerAcqLinearPtr->getHalfCycle();
    if (focalDepthMm<10.0)
        focalDepthMm=10.0;
    if (focalDepthMm>90.0)
        focalDepthMm=90.0;
    utpsFound = mUpsReaderSPtr->getUtpIdsPW(imagingCase, imagingMode, utpIds,
                                            focalDepthMm, halfcycle, isPWTDI, organId, gateRange);
    if (IMAGING_MODE_PW == imagingMode)
    {
        mUpsReaderSPtr->getPwHvpsValues(imagingCase, hvpb, hvnb);
    }
    else if (IMAGING_MODE_CW == imagingMode) //3.3V for actual hardware, 3.15V for api
    {
        hvpb = 3.15;
        hvnb = 3.15;
    }
    else
    {
        mUpsReaderSPtr->getHvpsValues(imagingCase, hvpb, hvnb);
    }
    if (gateRange > 2.0 or utpIds[0] == 405 or utpIds[0] == 410)
    {
        hvpb = 10.0;
        hvnb = 10.0;
    }
    if (utpIds[0] == 371)
    {
        hvpb = 11.0;
        hvnb = 11.0;
    }
    if (utpIds[0] == 389)
    {
        hvpb = 9.0;
        hvnb = 9.0;
    }
    if (utpIds[0] >=332 and utpIds[0]<=332)
    {
        hvpb = 12;
        hvnb = 12;
    }
    if (utpIds[0] >=411 and utpIds[0]<=411)
    {
        hvpb = 8.0;
        hvnb = 8.0;
    }
    if (utpIds[0] >=412 and utpIds[0]<=412)
    {
        hvpb = 10;
        hvnb = 10;
    }
    if (utpIds[0] >=426 and utpIds[0]<=426)
    {
        hvpb = 8;
        hvnb = 8;
    }
    mInputVector.txVdrv = hvpb;
    status = setupInputVector(imagingCase, imagingMode, isLinearProbe, seqBuilderPtr, colorAcqPtr, colorAcqLinearPtr,
                              dopplerAcqPtr, dopplerAcqLinearPtr, cwAcqPtr, isPWTDI, organId, gateRange);
    if (THOR_OK == status)
    {
        Invoke(&mInputVector, &mApiTable, &mOutputVector);
        indices[0] = mOutputVector.mi;
        indices[1] = mOutputVector.tis_display;
        indices[2] = mOutputVector.tib_display;
        indices[3] = mOutputVector.tic_display;
        txVoltage = mOutputVector.vdrvCmd;
        if (abs(mOutputVector.vdrvCmd - mInputVector.txVdrv) > 1.0 )
        {
            LOGD("Requested Tx Voltage (%f) differs from API Manager output (%f",
                 hvpb, mOutputVector.vdrvCmd);  // BUGBUG: handle this condition
        }
    }
    else
    {
        txVoltage = MIN_HVPS_VOLTS;
    }
    return (status);
}

void ApiManager::logApiTable()
{
    LOGI("API Table Header:");
    LOGI("\tSPTA_lim   = %f", mApiTable.api_table_header.SPTA_lim);
    LOGI("\tMI_lim     = %f", mApiTable.api_table_header.MI_lim);
    LOGI("\tambient    = %f", mApiTable.api_table_header.ambient);
    LOGI("\tApex       = %f", mApiTable.api_table_header.Apex);
    LOGI("\tElePitch   = %f", mApiTable.api_table_header.ElePitch);
    LOGI("\tCrysToSkin = %f", mApiTable.api_table_header.CrysToSkin);
    LOGI("\tEleLength  = %f", mApiTable.api_table_header.EleLength);
    LOGI("\tNumEles    = %d", mApiTable.api_table_header.NumEles);

    for (auto i = mStartUTPID; i < mNumUtps; i++)
    {
        LOGI("API Table UTP %d:", i);
        LOGI("\tutpID        = %d", mUtpIdTable[i].utpID);
        LOGI("\tswpEles      = %d", mUtpIdTable[i].apEles);
        LOGI("\tld_ref       = %f",mUtpIdTable[i].ld_ref);
        LOGI("\taaprt_ref    = %f",mUtpIdTable[i].aaprt_ref);
        LOGI("\tcFreq        = %e",mUtpIdTable[i].cFreq);
        LOGI("\tw01sFrac     = %e",mUtpIdTable[i].w01sFrac);
        LOGI("\tw0sFrac      = %e",mUtpIdTable[i].w0sFrac);
        LOGI("\tpii_0_ref    = %e",mUtpIdTable[i].pii_0_ref);
        LOGI("\tpii_3_ref    = %e",mUtpIdTable[i].pii_3_ref);
        LOGI("\tsii_3_ref    = %e",mUtpIdTable[i].sii_3_ref);
        LOGI("\timax_3_ref   = %e",mUtpIdTable[i].imax_3_ref);
        LOGI("\tvdrv_ref     = %e",mUtpIdTable[i].vdrv_ref);
        LOGI("\tw0u_ref      = %e",mUtpIdTable[i].w0u_ref);
        LOGI("\tvdrv_corr    = %f",mUtpIdTable[i].vdrv_corr);
        LOGI("\tTISas_ref    = %e",mUtpIdTable[i].TISas_ref);
        LOGI("\tTISbs_ref    = %e",mUtpIdTable[i].TISbs_ref);
        LOGI("\tTIBas_ref    = %e",mUtpIdTable[i].TIBas_ref);
        LOGI("\tmin_ref_vdrv = %e",mUtpIdTable[i].min_ref_vdrv);
        LOGI("\tmax_ref_vdrv = %e",mUtpIdTable[i].max_ref_vdrv);
        LOGI("\tpr_3         = %e",mUtpIdTable[i].pr_3);
        LOGI("\tmi_coeff_A   = %e",mUtpIdTable[i].mi_coeff_A);
        LOGI("\tmi_coeff_B   = %e",mUtpIdTable[i].mi_coeff_B);
        LOGI("\tmi_coeff_C   = %e",mUtpIdTable[i].mi_coeff_C);
        LOGI("\tpii3_coeff_A = %e",mUtpIdTable[i].pii3_coeff_A);
        LOGI("\tpii3_coeff_B = %e",mUtpIdTable[i].pii3_coeff_B);
        LOGI("\tpii3_coeff_C = %e",mUtpIdTable[i].pii3_coeff_C);
        LOGI("\tpii3_coeff_D = %e",mUtpIdTable[i].pii3_coeff_D);
        LOGI("\tsii3_coeff_A = %e",mUtpIdTable[i].sii3_coeff_A);
        LOGI("\tsii3_coeff_B = %e",mUtpIdTable[i].sii3_coeff_B);
        LOGI("\tsii3_coeff_C = %e",mUtpIdTable[i].sii3_coeff_C);
        LOGI("\tsii3_coeff_D = %e",mUtpIdTable[i].sii3_coeff_D);
    }
}

void ApiManager::logInputOutputVectors()
{
    LOGD("API Input Vector:");
    LOGD("\ttxVdrv     = %f", mInputVector.txVdrv);
    LOGD("\tnumUtps    = %d", mInputVector.numUtps);
    LOGD("\tUTP 0:");
    LOGD("\t\tutp      = %d", mInputVector.api_invector_utp[0].utp);
    LOGD("\t\tprf      = %f", mInputVector.api_invector_utp[0].prf);
    LOGD("\t\tfr       = %f", mInputVector.api_invector_utp[0].fr);
    LOGD("\t\tppl      = %d", mInputVector.api_invector_utp[0].ppl);
    LOGD("\t\tpulsMode = %d", mInputVector.api_invector_utp[0].pulsMode);
    LOGD("\t\tlpFrame  = %d", mInputVector.api_invector_utp[0].lpFrame);
    LOGD("\t\tdenisty  = %f", mInputVector.api_invector_utp[0].density);
    if (mInputVector.numUtps == 2)
    {
        LOGD("\tUTP 1:");
        LOGD("\t\tutp      = %d", mInputVector.api_invector_utp[1].utp);
        LOGD("\t\tprf      = %f", mInputVector.api_invector_utp[1].prf);
        LOGD("\t\tfr       = %f", mInputVector.api_invector_utp[1].fr);
        LOGD("\t\tppl      = %d", mInputVector.api_invector_utp[1].ppl);
        LOGD("\t\tpulsMode = %d", mInputVector.api_invector_utp[1].pulsMode);
        LOGD("\t\tlpFrame  = %d", mInputVector.api_invector_utp[1].lpFrame);
        LOGD("\t\tdenisty  = %f", mInputVector.api_invector_utp[1].density);
    }
    if (mInputVector.numUtps == 3)
    {
        LOGD("\tUTP 1:");
        LOGD("\t\tutp      = %d", mInputVector.api_invector_utp[1].utp);
        LOGD("\t\tprf      = %f", mInputVector.api_invector_utp[1].prf);
        LOGD("\t\tfr       = %f", mInputVector.api_invector_utp[1].fr);
        LOGD("\t\tppl      = %d", mInputVector.api_invector_utp[1].ppl);
        LOGD("\t\tpulsMode = %d", mInputVector.api_invector_utp[1].pulsMode);
        LOGD("\t\tlpFrame  = %d", mInputVector.api_invector_utp[1].lpFrame);
        LOGD("\t\tdenisty  = %f", mInputVector.api_invector_utp[1].density);

        LOGD("\tUTP 2:");
        LOGD("\t\tutp      = %d", mInputVector.api_invector_utp[2].utp);
        LOGD("\t\tprf      = %f", mInputVector.api_invector_utp[2].prf);
        LOGD("\t\tfr       = %f", mInputVector.api_invector_utp[2].fr);
        LOGD("\t\tppl      = %d", mInputVector.api_invector_utp[2].ppl);
        LOGD("\t\tpulsMode = %d", mInputVector.api_invector_utp[2].pulsMode);
        LOGD("\t\tlpFrame  = %d", mInputVector.api_invector_utp[2].lpFrame);
        LOGD("\t\tdenisty  = %f", mInputVector.api_invector_utp[2].density);
    }
    LOGD("API Output Vector:");
    LOGD("\tpii_3       = %f", mOutputVector.pii_3);
    LOGD("\tsppa_3      = %f", mOutputVector.sppa_3);
    LOGD("\tspta_3      = %f", mOutputVector.spta_3);
    LOGD("\tMI          = %f", mOutputVector.mi);
    LOGD("\timax_3      = %f", mOutputVector.imax_3);
    LOGD("\tw0          = %f", mOutputVector.w0);
    LOGD("\tw01         = %f", mOutputVector.w01);
    LOGD("\ttisas       = %f", mOutputVector.tisas);
    LOGD("\ttisbs       = %f", mOutputVector.tisbs);
    LOGD("\ttibbs       = %f", mOutputVector.tibbs);
    LOGD("\ttic         = %f", mOutputVector.tic);
    LOGD("\ttis_display = %f", mOutputVector.tis_display);
    LOGD("\ttib_display = %f", mOutputVector.tib_display);
    LOGD("\ttic_display = %f", mOutputVector.tic_display);
    LOGD("\tmi_vlim     = %f", mOutputVector.mi_vlim);
    LOGD("\tti_vlim     = %f", mOutputVector.ti_vlim);
    LOGD("\tspta_vlim   = %f", mOutputVector.spta_vlim);
    LOGD("\tvdrvCmd     = %f", mOutputVector.vdrvCmd);
}

void ApiManager::test()
{
    //  Populate the API Table Header
    mApiTable.api_table_header.SPTA_lim = 500;
    mApiTable.api_table_header.MI_lim = 1.5;
    mApiTable.api_table_header.ambient = 23.0;
    mApiTable.api_table_header.Apex = 0.0;    		/*  Apex in centimeters per spec  */
    mApiTable.api_table_header.ElePitch = 0.281;       /*  Pitch in millimeters per spec  */
    mApiTable.api_table_header.CrysToSkin = 0.13;
    mApiTable.api_table_header.EleLength = 1.2;    	   // element length in centimeters
    mApiTable.api_table_header.NumEles = 64;
    if ((mProbeType & DEV_PROBE_MASK) == PROBE_TYPE_LINEAR) {
        mApiTable.api_table_header.Class = API_LA; // BUGBUG: needs to come from UPS
    }
    else {
        mApiTable.api_table_header.Class = API_PHA; // BUGBUG: needs to come from UPS
    }

    for (int i = 0; i < 5; i++)
    {
        mApiTable.api_utpid_table[i] = &(mUtpIdTable[i]);
    }

    //  Populate the API UTP ID Table for 1 UTP
    mApiTable.api_utpid_table[0]->utpID = 0;
    mApiTable.api_utpid_table[0]->apEles = 40;
    mApiTable.api_utpid_table[0]->ld_ref = 1.0;
    mApiTable.api_utpid_table[0]->aaprt_ref = 1.3488;
    mApiTable.api_utpid_table[0]->cFreq = 2.684;
    mApiTable.api_utpid_table[0]->w01sFrac = -1.0;
    mApiTable.api_utpid_table[0]->w0sFrac = -1.0;
    mApiTable.api_utpid_table[0]->pii_0_ref = 0.0284;
    mApiTable.api_utpid_table[0]->pii_3_ref = 0.014635;
    mApiTable.api_utpid_table[0]->sii_3_ref = 0.014686;
    mApiTable.api_utpid_table[0]->sppa_3_ref = 0.02275;
    mApiTable.api_utpid_table[0]->mi_3_ref = 0.6353;
    mApiTable.api_utpid_table[0]->imax_3_ref = 1.0;
    mApiTable.api_utpid_table[0]->w0u_ref = 0.00219;
    mApiTable.api_utpid_table[0]->vdrv_corr = 1.0;
    mApiTable.api_utpid_table[0]->TISas_ref = 1.2415;
    mApiTable.api_utpid_table[0]->TISbs_ref = 2.837E-05;
    mApiTable.api_utpid_table[0]->TIBas_ref = 2.837E-05;
    mApiTable.api_utpid_table[0]->TIBbs_ref = 3.629E-05;
    mApiTable.api_utpid_table[0]->pr_3 = 0.88065;
    mApiTable.api_utpid_table[0]->vdrv_ref = 15.0;
    mApiTable.api_utpid_table[0]->min_ref_vdrv = 5.0;
    mApiTable.api_utpid_table[0]->max_ref_vdrv = 15.0;
    mApiTable.api_utpid_table[0]->mi_coeff_A = 0.0002912;
    mApiTable.api_utpid_table[0]->mi_coeff_B = 0.03307;
    mApiTable.api_utpid_table[0]->mi_coeff_C = 0.004033;
    mApiTable.api_utpid_table[0]->pii3_coeff_A = 2.02E-06;
    mApiTable.api_utpid_table[0]->pii3_coeff_B = 2.4828E-05;
    mApiTable.api_utpid_table[0]->pii3_coeff_C = 0.0002011;
    mApiTable.api_utpid_table[0]->pii3_coeff_D = -0.00053324;
    mApiTable.api_utpid_table[0]->sii3_coeff_A = 1.7688E-06;
    mApiTable.api_utpid_table[0]->sii3_coeff_B = 2.5816E-05;
    mApiTable.api_utpid_table[0]->sii3_coeff_C = 0.0002463;
    mApiTable.api_utpid_table[0]->sii3_coeff_D = -0.0007942;

    //  Populate the API UTP ID Table for 1 UTP
    mApiTable.api_utpid_table[1]->utpID = 1;
    mApiTable.api_utpid_table[1]->apEles = 40;
    mApiTable.api_utpid_table[1]->ld_ref = 1.0;
    mApiTable.api_utpid_table[1]->aaprt_ref = 1.3488;
    mApiTable.api_utpid_table[1]->cFreq = 2.439;
    mApiTable.api_utpid_table[1]->w01sFrac = -1.0;
    mApiTable.api_utpid_table[1]->w0sFrac = -1.0;
    mApiTable.api_utpid_table[1]->pii_0_ref = 0.02735;
    mApiTable.api_utpid_table[1]->pii_3_ref = 0.014888;
    mApiTable.api_utpid_table[1]->sii_3_ref = 0.014678;
    mApiTable.api_utpid_table[1]->sppa_3_ref = 0.02275;
    mApiTable.api_utpid_table[1]->mi_3_ref = 0.56566;
    mApiTable.api_utpid_table[1]->imax_3_ref = 1.0;
    mApiTable.api_utpid_table[1]->w0u_ref = 0.00289;
    mApiTable.api_utpid_table[1]->vdrv_corr = 1.0;
    mApiTable.api_utpid_table[1]->TISas_ref = 1.0239;
    mApiTable.api_utpid_table[1]->TISbs_ref = 3.357E-05;
    mApiTable.api_utpid_table[1]->TIBas_ref = 3.357E-05;
    mApiTable.api_utpid_table[1]->TIBbs_ref = 3.629E-05;
    mApiTable.api_utpid_table[1]->pr_3 = 0.88065;
    mApiTable.api_utpid_table[1]->vdrv_ref = 15.0;
    mApiTable.api_utpid_table[1]->min_ref_vdrv = 5.0;
    mApiTable.api_utpid_table[1]->max_ref_vdrv = 15.0;
    mApiTable.api_utpid_table[1]->mi_coeff_A = 0.0002913;
    mApiTable.api_utpid_table[1]->mi_coeff_B = 0.03307;
    mApiTable.api_utpid_table[1]->mi_coeff_C = 0.004033;
    mApiTable.api_utpid_table[1]->pii3_coeff_A = 2.02E-06;
    mApiTable.api_utpid_table[1]->pii3_coeff_B = 2.4828E-05;
    mApiTable.api_utpid_table[1]->pii3_coeff_C = 0.0002011;
    mApiTable.api_utpid_table[1]->pii3_coeff_D = -0.00053324;
    mApiTable.api_utpid_table[1]->sii3_coeff_A = 1.7688E-06;
    mApiTable.api_utpid_table[1]->sii3_coeff_B = 2.5816E-05;
    mApiTable.api_utpid_table[1]->sii3_coeff_C = 0.0002463;
    mApiTable.api_utpid_table[1]->sii3_coeff_D = -0.0007942;

    //  Populate the API UTP ID Table for 1 UTP
    mApiTable.api_utpid_table[2]->utpID = 2;
    mApiTable.api_utpid_table[2]->apEles = 40;
    mApiTable.api_utpid_table[2]->ld_ref = 1.0;
    mApiTable.api_utpid_table[2]->aaprt_ref = 1.3488;
    mApiTable.api_utpid_table[2]->cFreq = 2.537;
    mApiTable.api_utpid_table[2]->w01sFrac = -1.0;
    mApiTable.api_utpid_table[2]->w0sFrac = -1.0;
    mApiTable.api_utpid_table[2]->pii_0_ref = 0.0307;
    mApiTable.api_utpid_table[2]->pii_3_ref = 0.01467;
    mApiTable.api_utpid_table[2]->sii_3_ref = 0.01522;
    mApiTable.api_utpid_table[2]->sppa_3_ref = 0.02275;
    mApiTable.api_utpid_table[2]->mi_3_ref = 0.4908;
    mApiTable.api_utpid_table[2]->imax_3_ref = 1.0;
    mApiTable.api_utpid_table[2]->w0u_ref = 0.003171;
    mApiTable.api_utpid_table[2]->vdrv_corr = 1.0;
    mApiTable.api_utpid_table[2]->TISas_ref = 1.05788;
    mApiTable.api_utpid_table[2]->TISbs_ref = 3.8314E-05;
    mApiTable.api_utpid_table[2]->TIBas_ref = 3.8314E-05;
    mApiTable.api_utpid_table[2]->TIBbs_ref = 5.185E-05;
    mApiTable.api_utpid_table[2]->pr_3 = 0.7499;
    mApiTable.api_utpid_table[2]->vdrv_ref = 15.0;
    mApiTable.api_utpid_table[2]->min_ref_vdrv = 5.0;
    mApiTable.api_utpid_table[2]->max_ref_vdrv = 15.0;
    mApiTable.api_utpid_table[2]->mi_coeff_A = 0.0001587;
    mApiTable.api_utpid_table[2]->mi_coeff_B = 0.030436;
    mApiTable.api_utpid_table[2]->mi_coeff_C = -0.001453;
    mApiTable.api_utpid_table[2]->pii3_coeff_A = 1.833E-06;
    mApiTable.api_utpid_table[2]->pii3_coeff_B = 2.529E-05;
    mApiTable.api_utpid_table[2]->pii3_coeff_C = 0.0002309;
    mApiTable.api_utpid_table[2]->pii3_coeff_D = -0.0006712;
    mApiTable.api_utpid_table[2]->sii3_coeff_A = 2.01578E-06;
    mApiTable.api_utpid_table[2]->sii3_coeff_B = 2.5795E-05;
    mApiTable.api_utpid_table[2]->sii3_coeff_C = 0.000217399;
    mApiTable.api_utpid_table[2]->sii3_coeff_D = -0.00065137;

    //  Populate the API UTP ID Table for 1 UTP
    mApiTable.api_utpid_table[3]->utpID = 3;
    mApiTable.api_utpid_table[3]->apEles = 40;
    mApiTable.api_utpid_table[3]->ld_ref = 1.0;
    mApiTable.api_utpid_table[3]->aaprt_ref = 1.3488;
    mApiTable.api_utpid_table[3]->cFreq = 2.5716;
    mApiTable.api_utpid_table[3]->w01sFrac = -1.0;
    mApiTable.api_utpid_table[3]->w0sFrac = -1.0;
    mApiTable.api_utpid_table[3]->pii_0_ref = 0.02832;
    mApiTable.api_utpid_table[3]->pii_3_ref = 0.011187;
    mApiTable.api_utpid_table[3]->sii_3_ref = 0.01273;
    mApiTable.api_utpid_table[3]->sppa_3_ref = 0.02275;
    mApiTable.api_utpid_table[3]->mi_3_ref = 0.4376;
    mApiTable.api_utpid_table[3]->imax_3_ref = 1.0;
    mApiTable.api_utpid_table[3]->w0u_ref = 0.002772;
    mApiTable.api_utpid_table[3]->vdrv_corr = 1.0;
    mApiTable.api_utpid_table[3]->TISas_ref = 1.0015;
    mApiTable.api_utpid_table[3]->TISbs_ref = 3.3945E-05;
    mApiTable.api_utpid_table[3]->TIBas_ref = 3.3945E-05;
    mApiTable.api_utpid_table[3]->TIBbs_ref = 4.5324E-05;
    mApiTable.api_utpid_table[3]->pr_3 = 0.69308;
    mApiTable.api_utpid_table[3]->vdrv_ref = 15.0;
    mApiTable.api_utpid_table[3]->min_ref_vdrv = 5.0;
    mApiTable.api_utpid_table[3]->max_ref_vdrv = 15.0;
    mApiTable.api_utpid_table[3]->mi_coeff_A = -0.00019312;
    mApiTable.api_utpid_table[3]->mi_coeff_B = 0.0342218;
    mApiTable.api_utpid_table[3]->mi_coeff_C = -0.03412;
    mApiTable.api_utpid_table[3]->pii3_coeff_A = 9.2063E-07;
    mApiTable.api_utpid_table[3]->pii3_coeff_B = 2.3699E-05;
    mApiTable.api_utpid_table[3]->pii3_coeff_C = 0.00030645;
    mApiTable.api_utpid_table[3]->pii3_coeff_D = -0.001169;
    mApiTable.api_utpid_table[3]->sii3_coeff_A = 1.7373E-06;
    mApiTable.api_utpid_table[3]->sii3_coeff_B = 2.10706E-05;
    mApiTable.api_utpid_table[3]->sii3_coeff_C = 0.00016877;
    mApiTable.api_utpid_table[3]->sii3_coeff_D = -0.00040898;

    //  Populate the API UTP ID Table for 1 UTP
    mApiTable.api_utpid_table[4]->utpID = 4;
    mApiTable.api_utpid_table[4]->apEles = 40;
    mApiTable.api_utpid_table[4]->ld_ref = 1.0;
    mApiTable.api_utpid_table[4]->aaprt_ref = 1.3488;
    mApiTable.api_utpid_table[4]->cFreq = 2.5716;
    mApiTable.api_utpid_table[4]->w01sFrac = -1.0;
    mApiTable.api_utpid_table[4]->w0sFrac = -1.0;
    mApiTable.api_utpid_table[4]->pii_0_ref = 0.02505;
    mApiTable.api_utpid_table[4]->pii_3_ref = 0.0099519;
    mApiTable.api_utpid_table[4]->sii_3_ref = 0.01273;
    mApiTable.api_utpid_table[4]->sppa_3_ref = 0.02275;
    mApiTable.api_utpid_table[4]->mi_3_ref = 0.395;
    mApiTable.api_utpid_table[4]->imax_3_ref = 1.0;
    mApiTable.api_utpid_table[4]->w0u_ref = 0.002987;
    mApiTable.api_utpid_table[4]->vdrv_corr = 1.0;
    mApiTable.api_utpid_table[4]->TISas_ref = 0.94509;
    mApiTable.api_utpid_table[4]->TISbs_ref = 3.65735E-05;
    mApiTable.api_utpid_table[4]->TIBas_ref = 3.65735E-05;
    mApiTable.api_utpid_table[4]->TIBbs_ref = 4.8446E-05;
    mApiTable.api_utpid_table[4]->pr_3 = 0.60638;
    mApiTable.api_utpid_table[4]->vdrv_ref = 15.0;
    mApiTable.api_utpid_table[4]->min_ref_vdrv = 5.0;
    mApiTable.api_utpid_table[4]->max_ref_vdrv = 15.0;
    mApiTable.api_utpid_table[4]->mi_coeff_A = -2.11179E-05;
    mApiTable.api_utpid_table[4]->mi_coeff_B = 0.027865;
    mApiTable.api_utpid_table[4]->mi_coeff_C = -0.018185;
    mApiTable.api_utpid_table[4]->pii3_coeff_A = 9.6098E-07;
    mApiTable.api_utpid_table[4]->pii3_coeff_B = 1.8849E-05;
    mApiTable.api_utpid_table[4]->pii3_coeff_C = 0.0002174;
    mApiTable.api_utpid_table[4]->pii3_coeff_D = -0.00079367;
    mApiTable.api_utpid_table[4]->sii3_coeff_A = -2.3514E-05;
    mApiTable.api_utpid_table[4]->sii3_coeff_B = 0.0007054;
    mApiTable.api_utpid_table[4]->sii3_coeff_C = -0.0064664;
    mApiTable.api_utpid_table[4]->sii3_coeff_D = -0.98236;

    //  Populate the input vector structure
    mInputVector.numUtps = 1;
    mInputVector.txVdrv = 15;

    mInputVector.api_invector_utp[0].utp = 0;
    mInputVector.api_invector_utp[0].prf = 11574.0;
    mInputVector.api_invector_utp[0].fr = 30.0;
    mInputVector.api_invector_utp[0].ppl = 1;
    mInputVector.api_invector_utp[0].density = 0.625;
    mInputVector.api_invector_utp[0].lpFrame = 57;
    mInputVector.api_invector_utp[0].pulsMode = API_Es;   // B mode

    Invoke (&mInputVector, &mApiTable, &mOutputVector);  //  THIS IS WHERE THE WORK GETS DONE BY CALLING THE API BLACK BOX !!!!

    //   Write the output vector to a file
    LOGI("\n\n UTP 0, Class = Phased array, tx vdrv = 15, mode = B mode\n\n");	  // //  Write some comments
    LOGI("UTP ID = %d\n", mInputVector.api_invector_utp[0].utp);
    LOGI("pii3 = %f", mOutputVector.pii_3);
    LOGI("\n spta_3 = %f", mOutputVector.spta_3);
    LOGI("\n MI = %f", mOutputVector.mi);
    LOGI("\n Pii.3 = %f", mOutputVector.pii_3);
    LOGI("\n SPPA.3 t = %f", mOutputVector.sppa_3);
    LOGI("\n Imax.3 = %f", mOutputVector.imax_3);
    LOGI("\n W0 = %f", mOutputVector.w0);
    LOGI("\n W01 = %f", mOutputVector.w01);
    LOGI("\n TISas = %f", mOutputVector.tisas);
    LOGI("\n TISbs = %f", mOutputVector.tisbs);
    LOGI("\n TIBbs = %f", mOutputVector.tibbs);
    LOGI("\n TIC = %f", mOutputVector.tic);
    LOGI("\n TIS Display = %f", mOutputVector.tis_display);
    LOGI("\n TIB Display = %f", mOutputVector.tib_display);
    LOGI("\n TIC Display = %f", mOutputVector.tic_display);
    LOGI("\n spta_vlim = %f", mOutputVector.spta_vlim);
    LOGI("\n mi_vlim = %f", mOutputVector.mi_vlim);
    LOGI("\n Commanded vDrv = %f\n\n\n", mOutputVector.vdrvCmd);

    //  Populate the input vector structure
    mInputVector.numUtps = 1;
    mInputVector.txVdrv = 15;

    mInputVector.api_invector_utp[0].utp = 1;
    mInputVector.api_invector_utp[0].prf = 8979.9;
    mInputVector.api_invector_utp[0].fr = 30.0;
    mInputVector.api_invector_utp[0].ppl = 1;
    mInputVector.api_invector_utp[0].density = 0.625;
    mInputVector.api_invector_utp[0].lpFrame = 57;
    mInputVector.api_invector_utp[0].pulsMode = API_Es;   // B mode

    Invoke (&mInputVector, &mApiTable, &mOutputVector);   //  THIS IS WHERE THE WORK GETS DONE BY CALLING THE API BLACK BOX !!!!

    //   Write the output vector to a file
    LOGI("\n\n UTP 1, Class = Phased array, tx vdrv = 15, mode = B mode\n\n");	  // //  Write some comments
    LOGI("UTP ID = %d\n", mInputVector.api_invector_utp[0].utp);
    LOGI("pii3 = %f", mOutputVector.pii_3);
    LOGI("\n spta_3 = %f", mOutputVector.spta_3);
    LOGI("\n MI = %f", mOutputVector.mi);
    LOGI("\n Pii.3 = %f", mOutputVector.pii_3);
    LOGI("\n SPPA.3 t = %f", mOutputVector.sppa_3);
    LOGI("\n Imax.3 = %f", mOutputVector.imax_3);
    LOGI("\n W0 = %f", mOutputVector.w0);
    LOGI("\n W01 = %f", mOutputVector.w01);
    LOGI("\n TISas = %f", mOutputVector.tisas);
    LOGI("\n TISbs = %f", mOutputVector.tisbs);
    LOGI("\n TIBbs = %f", mOutputVector.tibbs);
    LOGI("\n TIC = %f", mOutputVector.tic);
    LOGI("\n TIS Display = %f", mOutputVector.tis_display);
    LOGI("\n TIB Display = %f", mOutputVector.tib_display);
    LOGI("\n TIC Display = %f", mOutputVector.tic_display);
    LOGI("\n spta_vlim = %f", mOutputVector.spta_vlim);
    LOGI("\n mi_vlim = %f", mOutputVector.mi_vlim);
    LOGI("\n Commanded vDrv = %f\n\n\n", mOutputVector.vdrvCmd);

    //  Populate the input vector structure
    mInputVector.numUtps = 1;
    mInputVector.txVdrv = 15;

    mInputVector.api_invector_utp[0].utp = 2;
    mInputVector.api_invector_utp[0].prf = 7183.9;
    mInputVector.api_invector_utp[0].fr = 30.0;
    mInputVector.api_invector_utp[0].ppl = 1;
    mInputVector.api_invector_utp[0].density = 0.625;
    mInputVector.api_invector_utp[0].lpFrame = 57;
    mInputVector.api_invector_utp[0].pulsMode = API_Es;   // B mode

    Invoke (&mInputVector, &mApiTable, &mOutputVector);  //  THIS IS WHERE THE WORK GETS DONE BY CALLING THE API BLACK BOX !!!!

    //   Write the output vector to a file
    LOGI("\n\n UTP 2, Class = Phased array, tx vdrv = 15, mode = B mode\n\n");	  // //  Write some comments
    LOGI("UTP ID = %d\n", mInputVector.api_invector_utp[0].utp);
    LOGI("pii3 = %f", mOutputVector.pii_3);
    LOGI("\n spta_3 = %f", mOutputVector.spta_3);
    LOGI("\n MI = %f", mOutputVector.mi);
    LOGI("\n Pii.3 = %f", mOutputVector.pii_3);
    LOGI("\n SPPA.3 t = %f", mOutputVector.sppa_3);
    LOGI("\n Imax.3 = %f", mOutputVector.imax_3);
    LOGI("\n W0 = %f", mOutputVector.w0);
    LOGI("\n W01 = %f", mOutputVector.w01);
    LOGI("\n TISas = %f", mOutputVector.tisas);
    LOGI("\n TISbs = %f", mOutputVector.tisbs);
    LOGI("\n TIBbs = %f", mOutputVector.tibbs);
    LOGI("\n TIC = %f", mOutputVector.tic);
    LOGI("\n TIS Display = %f", mOutputVector.tis_display);
    LOGI("\n TIB Display = %f", mOutputVector.tib_display);
    LOGI("\n TIC Display = %f", mOutputVector.tic_display);
    LOGI("\n spta_vlim = %f", mOutputVector.spta_vlim);
    LOGI("\n mi_vlim = %f", mOutputVector.mi_vlim);
    LOGI("\n Commanded vDrv = %f\n\n\n", mOutputVector.vdrvCmd);

    //  Populate the input vector structure
    mInputVector.numUtps = 1;
    mInputVector.txVdrv = 15;

    mInputVector.api_invector_utp[0].utp = 3;
    mInputVector.api_invector_utp[0].prf = 6127.45;
    mInputVector.api_invector_utp[0].fr = 30.0;
    mInputVector.api_invector_utp[0].ppl = 1;
    mInputVector.api_invector_utp[0].density = 0.625;
    mInputVector.api_invector_utp[0].lpFrame = 57;
    mInputVector.api_invector_utp[0].pulsMode = API_Es;   // B mode

    Invoke (&mInputVector, &mApiTable, &mOutputVector);  //  THIS IS WHERE THE WORK GETS DONE BY CALLING THE API BLACK BOX !!!!

    //   Write the output vector to a file
    LOGI("\n\n UTP 3, Class = Phased array, tx vdrv = 15, mode = B mode\n\n");	  // //  Write some comments
    LOGI("UTP ID = %d\n", mInputVector.api_invector_utp[0].utp);
    LOGI("pii3 = %f", mOutputVector.pii_3);
    LOGI("\n spta_3 = %f", mOutputVector.spta_3);
    LOGI("\n MI = %f", mOutputVector.mi);
    LOGI("\n Pii.3 = %f", mOutputVector.pii_3);
    LOGI("\n SPPA.3 t = %f", mOutputVector.sppa_3);
    LOGI("\n Imax.3 = %f", mOutputVector.imax_3);
    LOGI("\n W0 = %f", mOutputVector.w0);
    LOGI("\n W01 = %f", mOutputVector.w01);
    LOGI("\n TISas = %f", mOutputVector.tisas);
    LOGI("\n TISbs = %f", mOutputVector.tisbs);
    LOGI("\n TIBbs = %f", mOutputVector.tibbs);
    LOGI("\n TIC = %f", mOutputVector.tic);
    LOGI("\n TIS Display = %f", mOutputVector.tis_display);
    LOGI("\n TIB Display = %f", mOutputVector.tib_display);
    LOGI("\n TIC Display = %f", mOutputVector.tic_display);
    LOGI("\n spta_vlim = %f", mOutputVector.spta_vlim);
    LOGI("\n mi_vlim = %f", mOutputVector.mi_vlim);
    LOGI("\n Commanded vDrv = %f\n\n\n", mOutputVector.vdrvCmd);

    //  Populate the input vector structure
    mInputVector.numUtps = 1;
    mInputVector.txVdrv = 15;

    mInputVector.api_invector_utp[0].utp = 4;
    mInputVector.api_invector_utp[0].prf = 800.0;
    mInputVector.api_invector_utp[0].fr = -1;
    mInputVector.api_invector_utp[0].ppl = -1;
    mInputVector.api_invector_utp[0].density = -1;
    mInputVector.api_invector_utp[0].lpFrame = -1;
    mInputVector.api_invector_utp[0].pulsMode = API_Eu;   // M mode

    Invoke (&mInputVector, &mApiTable, &mOutputVector);  //  THIS IS WHERE THE WORK GETS DONE BY CALLING THE API BLACK BOX !!!!

    //   Write the output vector to a file
    LOGI("\n\n UTP 4, Class = Phased array, tx vdrv = 15, mode = M mode\n\n");	  // //  Write some comments
    LOGI("UTP ID = %d\n", mInputVector.api_invector_utp[0].utp);
    LOGI("pii3 = %f", mOutputVector.pii_3);
    LOGI("\n spta_3 = %f", mOutputVector.spta_3);
    LOGI("\n MI = %f", mOutputVector.mi);
    LOGI("\n Pii.3 = %f", mOutputVector.pii_3);
    LOGI("\n SPPA.3 = %f", mOutputVector.sppa_3);
    LOGI("\n Imax.3 = %f", mOutputVector.imax_3);
    LOGI("\n W0 = %f", mOutputVector.w0);
    LOGI("\n W01 = %f", mOutputVector.w01);
    LOGI("\n TISas = %f", mOutputVector.tisas);
    LOGI("\n TISbs = %f", mOutputVector.tisbs);
    LOGI("\n TIBbs = %f", mOutputVector.tibbs);
    LOGI("\n TIC = %f", mOutputVector.tic);
    LOGI("\n TIS Display = %f", mOutputVector.tis_display);
    LOGI("\n TIB Display = %f", mOutputVector.tib_display);
    LOGI("\n TIC Display = %f", mOutputVector.tic_display);
    LOGI("\n spta_vlim = %f", mOutputVector.spta_vlim);
    LOGI("\n mi_vlim = %f", mOutputVector.mi_vlim);
    LOGI("\n Commanded vDrv = %f\n\n\n", mOutputVector.vdrvCmd);


    mApiTable.api_table_header.Class = API_CLA;    //  Change to a curved array
    mInputVector.api_invector_utp[0].utp = 0;		  //  Set input vector back to UTP 0


    Invoke (&mInputVector, &mApiTable, &mOutputVector);  //  THIS IS WHERE THE WORK GETS DONE BY CALLING THE API BLACK BOX !!!!

    LOGI("\n\n Class = Curved array, tx vdrv = 15, mode = B mode\n\n"); //  Write some comments
    LOGI("UTP ID = %d\n", mInputVector.api_invector_utp[0].utp);
    LOGI("pii3 = %f", mOutputVector.pii_3);
    LOGI("\n spta_3 = %f", mOutputVector.spta_3);
    LOGI("\n MI = %f", mOutputVector.mi);
    LOGI("\n Pii.3 = %f", mOutputVector.pii_3);
    LOGI("\n SPPA.3 = %f", mOutputVector.sppa_3);
    LOGI("\n Imax.3 = %f", mOutputVector.imax_3);
    LOGI("\n W0 = %f", mOutputVector.w0);
    LOGI("\n W01 = %f", mOutputVector.w01);
    LOGI("\n TISas = %f", mOutputVector.tisas);
    LOGI("\n TISbs = %f", mOutputVector.tisbs);
    LOGI("\n TIBbs = %f", mOutputVector.tibbs);
    LOGI("\n TIC = %f", mOutputVector.tic);
    LOGI("\n TIS Display = %f", mOutputVector.tis_display);
    LOGI("\n TIB Display = %f", mOutputVector.tib_display);
    LOGI("\n TIC Display = %f", mOutputVector.tic_display);
    LOGI("\n spta_vlim = %f", mOutputVector.spta_vlim);
    LOGI("\n mi_vlim = %f", mOutputVector.mi_vlim);
    LOGI("\n Commanded vDrv = %f\n\n\n", mOutputVector.vdrvCmd);

    if ((mProbeType & DEV_PROBE_MASK) == PROBE_TYPE_LINEAR) {
        mApiTable.api_table_header.Class = API_LA; // BUGBUG: needs to come from UPS
    }
    else {
        mApiTable.api_table_header.Class = API_PHA; // BUGBUG: needs to come from UPS
    }


    mInputVector.numUtps = 1;
    mInputVector.txVdrv = 30;		//  Set txVdrv to 30

    mInputVector.api_invector_utp[0].utp = 0;
    mInputVector.api_invector_utp[0].prf = 6666.666504;
    mInputVector.api_invector_utp[0].fr = 26.041666;
    mInputVector.api_invector_utp[0].ppl = 1;
    mInputVector.api_invector_utp[0].density = 1.00;
    mInputVector.api_invector_utp[0].lpFrame = 128;
    mInputVector.api_invector_utp[0].pulsMode = API_Es;

    Invoke (&mInputVector, &mApiTable, &mOutputVector);  //  THIS IS WHERE THE WORK GETS DONE BY CALLING THE API BLACK BOX !!!!


    LOGI("\n\n Class = Phased array, tx vdrv = 30, mode = B mode\n\n");	   //  Write some comments
    LOGI("UTP ID = %d\n", mInputVector.api_invector_utp[0].utp);
    LOGI("pii3 = %f", mOutputVector.pii_3);
    LOGI("\n spta_3 = %f", mOutputVector.spta_3);
    LOGI("\n MI = %f", mOutputVector.mi);
    LOGI("\n Pii.3 = %f", mOutputVector.pii_3);
    LOGI("\n SPPA.3 = %f", mOutputVector.sppa_3);
    LOGI("\n Imax.3 = %f", mOutputVector.imax_3);
    LOGI("\n W0 = %f", mOutputVector.w0);
    LOGI("\n W01 = %f", mOutputVector.w01);
    LOGI("\n TISas = %f", mOutputVector.tisas);
    LOGI("\n TISbs = %f", mOutputVector.tisbs);
    LOGI("\n TIBbs = %f", mOutputVector.tibbs);
    LOGI("\n TIC = %f", mOutputVector.tic);
    LOGI("\n TIS Display = %f", mOutputVector.tis_display);
    LOGI("\n TIB Display = %f", mOutputVector.tib_display);
    LOGI("\n TIC Display = %f", mOutputVector.tic_display);
    LOGI("\n spta_vlim = %f", mOutputVector.spta_vlim);
    LOGI("\n mi_vlim = %f", mOutputVector.mi_vlim);
    LOGI("\n Commanded vDrv = %f\n\n\n", mOutputVector.vdrvCmd);

    mInputVector.numUtps = 1;
    mInputVector.txVdrv = 20;		 //  Set txVdrv to 20

    mInputVector.api_invector_utp[0].utp = 0;
    mInputVector.api_invector_utp[0].prf = 4196;
    mInputVector.api_invector_utp[0].fr = -1;			 //  Unscanned mode so set to -1
    mInputVector.api_invector_utp[0].ppl = -1;			 //  Unscanned mode so set to -1
    mInputVector.api_invector_utp[0].density = -1;		 //  Unscanned mode so set to -1
    mInputVector.api_invector_utp[0].lpFrame = -1;		 //  Unscanned mode so set to -1
    mInputVector.api_invector_utp[0].pulsMode = API_Pu;	   //  Set pulsed doppler mode

    Invoke (&mInputVector, &mApiTable, &mOutputVector);  //  THIS IS WHERE THE WORK GETS DONE !!!!


    LOGI("\n\n Class = Phased array, tx vdrv = 20, mode = PW Doppler\n\n");	   //  Write some comments
    LOGI("UTP ID = %d\n", mInputVector.api_invector_utp[0].utp);
    LOGI("pii3 = %f", mOutputVector.pii_3);
    LOGI("\n spta_3 = %f", mOutputVector.spta_3);
    LOGI("\n MI = %f", mOutputVector.mi);
    LOGI("\n Pii.3 = %f", mOutputVector.pii_3);
    LOGI("\n SPPA.3 = %f", mOutputVector.sppa_3);
    LOGI("\n Imax.3 = %f", mOutputVector.imax_3);
    LOGI("\n W0 = %f", mOutputVector.w0);
    LOGI("\n W01 = %f", mOutputVector.w01);
    LOGI("\n TISas = %f", mOutputVector.tisas);
    LOGI("\n TISbs = %f", mOutputVector.tisbs);
    LOGI("\n TIBbs = %f", mOutputVector.tibbs);
    LOGI("\n TIC = %f", mOutputVector.tic);
    LOGI("\n TIS Display = %f", mOutputVector.tis_display);
    LOGI("\n TIB Display = %f", mOutputVector.tib_display);
    LOGI("\n TIC Display = %f", mOutputVector.tic_display);
    LOGI("\n spta_vlim = %f", mOutputVector.spta_vlim);
    LOGI("\n mi_vlim = %f", mOutputVector.mi_vlim);
    LOGI("\n Commanded vDrv = %f\n\n\n", mOutputVector.vdrvCmd);

    mInputVector.numUtps = 1;
    mInputVector.txVdrv = 50;

    mInputVector.api_invector_utp[0].utp = 0;
    mInputVector.api_invector_utp[0].prf = 800.0;
    mInputVector.api_invector_utp[0].fr = -1;		 //  Unscanned mode so set to -1
    mInputVector.api_invector_utp[0].ppl = -1;		 //  Unscanned mode so set to -1
    mInputVector.api_invector_utp[0].density = -1;	   //  Unscanned mode so set to -1
    mInputVector.api_invector_utp[0].lpFrame = -1;	   //  Unscanned mode so set to -1
    mInputVector.api_invector_utp[0].pulsMode = API_Eu;	   // Set M mode

    Invoke (&mInputVector, &mApiTable, &mOutputVector);  //  THIS IS WHERE THE WORK GETS DONE BY CALLING THE API BLACK BOX !!!!


    LOGI("\n\n Class = Phased array, tx vdrv = 50, mode = M mode\n\n");	   //  Write some comments
    LOGI("UTP ID = %d\n", mInputVector.api_invector_utp[0].utp);
    LOGI("pii3 = %f", mOutputVector.pii_3);
    LOGI("\n spta_3 = %f", mOutputVector.spta_3);
    LOGI("\n MI = %f", mOutputVector.mi);
    LOGI("\n Pii.3 = %f", mOutputVector.pii_3);
    LOGI("\n SPPA.3 = %f", mOutputVector.sppa_3);
    LOGI("\n Imax.3 = %f", mOutputVector.imax_3);
    LOGI("\n W0 = %f", mOutputVector.w0);
    LOGI("\n W01 = %f", mOutputVector.w01);
    LOGI("\n TISas = %f", mOutputVector.tisas);
    LOGI("\n TISbs = %f", mOutputVector.tisbs);
    LOGI("\n TIBbs = %f", mOutputVector.tibbs);
    LOGI("\n TIC = %f", mOutputVector.tic);
    LOGI("\n TIS Display = %f", mOutputVector.tis_display);
    LOGI("\n TIB Display = %f", mOutputVector.tib_display);
    LOGI("\n TIC Display = %f", mOutputVector.tic_display);
    LOGI("\n spta_vlim = %f", mOutputVector.spta_vlim);
    LOGI("\n mi_vlim = %f", mOutputVector.mi_vlim);
    LOGI("\n Commanded vDrv = %f\n\n\n", mOutputVector.vdrvCmd);


    mApiTable.api_table_header.Class = API_LA;    //  Change to linear array

    mInputVector.numUtps = 1;
    mInputVector.txVdrv = 50;

    mInputVector.api_invector_utp[0].utp = 0;
    mInputVector.api_invector_utp[0].prf = 4989.086426;
    mInputVector.api_invector_utp[0].fr = 26.041666;
    mInputVector.api_invector_utp[0].ppl = 1;
    mInputVector.api_invector_utp[0].density = 2.976744;
    mInputVector.api_invector_utp[0].lpFrame = 128;
    mInputVector.api_invector_utp[0].pulsMode = API_Es;

    Invoke (&mInputVector, &mApiTable, &mOutputVector);  //  THIS IS WHERE THE WORK GETS DONE BY CALLING THE API BLACK BOX !!!!

    LOGI("\n\n Class = Linear array, tx vdrv = 50, mode = B mode\n\n");	   //  Write some comments
    LOGI("UTP ID = %d\n", mInputVector.api_invector_utp[0].utp);
    LOGI("pii3 = %f", mOutputVector.pii_3);
    LOGI("\n spta_3 = %f", mOutputVector.spta_3);
    LOGI("\n MI = %f", mOutputVector.mi);
    LOGI("\n Pii.3 = %f", mOutputVector.pii_3);
    LOGI("\n SPPA.3 = %f", mOutputVector.sppa_3);
    LOGI("\n Imax.3 = %f", mOutputVector.imax_3);
    LOGI("\n W0 = %f", mOutputVector.w0);
    LOGI("\n W01 = %f", mOutputVector.w01);
    LOGI("\n TISas = %f", mOutputVector.tisas);
    LOGI("\n TISbs = %f", mOutputVector.tisbs);
    LOGI("\n TIBbs = %f", mOutputVector.tibbs);
    LOGI("\n TIC = %f", mOutputVector.tic);
    LOGI("\n TIS Display = %f", mOutputVector.tis_display);
    LOGI("\n TIB Display = %f", mOutputVector.tib_display);
    LOGI("\n TIC Display = %f", mOutputVector.tic_display);
    LOGI("\n spta_vlim = %f", mOutputVector.spta_vlim);
    LOGI("\n mi_vlim = %f", mOutputVector.mi_vlim);
    LOGI("\n Commanded vDrv = %f\n\n\n", mOutputVector.vdrvCmd);

}
