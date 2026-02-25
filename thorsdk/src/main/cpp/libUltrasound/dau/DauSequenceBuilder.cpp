//`
// Copyright (C) 2017 EchoNous, Inc.
//
//
#define LOG_TAG "DauSequenceBuilder"


#include <cstring>

#include <ThorUtils.h>
#include <TbfRegs.h>
#include <BitfieldMacros.h>
#include <DauSequenceBuilder.h>
#include <UpsReader.h>
#include <BitfieldMacros.h>
#include <stdio.h>

// Beam parameters modified/read in the sequence builder
#define BP_BP0			      ((TBF_REG_BP0     - TBF_REG_BP0)/sizeof(uint32_t))
#define BP_BENABLE            ((TBF_REG_BENABLE - TBF_REG_BP0) >> 2)
#define BENABLE_OUTTYPE_BIT   4
#define BENABLE_OUTTYPE_MASK  0xFFFFFF8F
#define BENABLE_B_EOF         16
#define BENABLE_C_EOF         17
#define BENABLE_EOS           18
#define BP_BPRI               ((TBF_REG_BPRI    - TBF_REG_BP0)/sizeof(uint32_t))


DauSequenceBuilder::DauSequenceBuilder(const std::shared_ptr<DauMemory>& daum,
                                       const std::shared_ptr<UpsReader>& upsReader,
                                       const std::shared_ptr<DauHw>& dauHw) :
                                       mNumSeqRegs(0),
                                       mPayloadOffset(0),
                                       mNumBNodes(0),
                                       mNumCNodes(0),
                                       mNumPwNodes(0),
                                       mNumCwNodes(0),
                                       mFsClocksPerFrame(0),
                                       mActiveImagingInterval(0),
                                       mPmNodePayloadOffset(0),
                                       mPmNodeDuration(0),
                                       mPingSeqStartNode(FIRST_PRI_NODE),
                                       mPongSeqStartNode(FIRST_PRI_NODE),
                                       mMmodePingPayloadOffset(1),
                                       mMmodePongPayloadOffset(1),
                                       mNumEcgSamplesPerFrame(0),
                                       mNumDaSamplesPerFrame(0),
                                       mDutyCycle(0.0f),
                                       mHvPs1PayloadPtr(nullptr),
                                       mHvPs2PayloadPtr(nullptr),
                                       mDaumSPtr(daum),
                                       mUpsReaderSPtr(upsReader),
                                       mDauHwSPtr(dauHw)
{
    LOGD("*** DauSequenceBuilder - constructor");
}

DauSequenceBuilder::~DauSequenceBuilder()
{
    LOGD("*** DauSequenceBuilder - destructor");
}

/**
 * @brief Builds the entire sequence for colorflow imaging including the B-mode portion.
 */
ThorStatus DauSequenceBuilder::buildBCSequence(uint32_t imagingCaseId,
                                               uint32_t cSequenceBlob[],
                                               uint32_t numCNodes,
                                               uint32_t msrtAfe[],
                                               uint32_t msrtAfeLength)
{
    ThorStatus status;
    status = buildBSequence( imagingCaseId, IMAGING_MODE_COLOR );
    if (THOR_OK == status)
    {
        status = buildCSequence( imagingCaseId, cSequenceBlob, numCNodes, msrtAfe, msrtAfeLength );
    }
    return (status);
}

/**
 * @brief Builds the entire sequence for colorflow imaging including the B-mode portion for Linear probe.
 */
ThorStatus DauSequenceBuilder::buildBCSequenceLinear(uint32_t imagingCaseId,
                                               uint32_t cSequenceBlob[],
                                               uint32_t numCNodes,
                                               uint32_t msrtAfe[],
                                               uint32_t msrtAfeLength,
                                               uint32_t lutMSSTTXC[],
                                               uint32_t lutMSSTTXCLength)
{
    ThorStatus status;
    // attach TX config at the first LLE node
    status = buildBCompoundSequence(imagingCaseId, 1, IMAGING_MODE_COLOR, true);
    if (THOR_OK == status)
    {
        status = buildCSequenceLinear( imagingCaseId, cSequenceBlob, numCNodes, msrtAfe, msrtAfeLength,
                                        lutMSSTTXC, lutMSSTTXCLength);
    }
    return (status);
}


/**
 * @brief Builds a B-Mode sequence for the desired imaging case.
 *
 * The sequence is placed in a memory block at the specified memory location
 * and uses the corresponding physical address for the sequencer to use.
 *
 * The structure of the sequence built by this function is as follows:
 *
 * Number of links in the linked list = n+2, where n = # of Tx lines (nTxLines)
 * Size of sequence blob obtained from UPS is assumed to contain payloads
 * for the power management line followed by payloads for the n Tx lines.
 *
 * First link (#0) contains payload for power management with BB = 0.
 *
 *                +-------------------------+
 *                |                         |
 * +-----------+  |  +-----------+     +----V---+     +--------+
 * | PM (BB=0) |--+  | PM (BB=1) |---->|  L[0]  |---->| L[n-1] |---+
 * +-----------+     +-----------+     +--------+     +--------+   |
 *                         ^                                       |
 *                         |                                       |
 *                         +---------------------------------------+
 *
 * @param imagingCaseID    - the numebr of the imaging case.
 * @param sequencePhysAddr - The physical starting address of the sequence
 * @param pSequenceBuffer  - Virtual address of sequencePhysAddr
 *
 * @return returns true if there were no errors.
 */

ThorStatus DauSequenceBuilder::buildBSequence( const uint32_t imagingCaseId, uint32_t imagingMode )
{
    mNumSeqRegs    = 0;
    mPayloadOffset = 1;

    // Fetch number of Tx lines from UPS
    mNumBNodes = mUpsReaderSPtr->getNumTxBeamsB( imagingCaseId, imagingMode );
    uint32_t imagingType = mUpsReaderSPtr->getImagingType( imagingCaseId );
    if (imagingType == UpsReader::IMAGING_TYPE_ITHI)
        mNumBNodes = mNumBNodes * 2;
    uint32_t blobSize  = mUpsReaderSPtr->getSequenceBlob(imagingCaseId, mSequenceBlob, &mNumSeqRegs);

    if (blobSize == 0)
    {
        LOGE("ups sequence blob read failed\n");
        return(TER_IE_UPS_NULL_BLOB);
    }

    if (mNumSeqRegs > DauSequenceBuilder::MAX_PAYLOAD_WORDS)
    {
        LOGE("DauSequenceBuilder::buildSequence - mNumSeqRegs (%d) > MAX_PAYLOAD_WORDS (%d)\n",
             mNumSeqRegs, DauSequenceBuilder::MAX_PAYLOAD_WORDS);
        return(TER_IE_UPS_INVALID_BLOB);
    }

    // Set the START_NODE
    {
        setLleHeader (mLle[START_NODE], START_PM_NODE, 0, 0, mPayloadOffset);
        mPayloadOffset += buildStartPayload( imagingCaseId, IMAGING_MODE_B, mPayloadOffset );
    }

    // Set START_PM_NODE
    {
        setLleHeader (mLle[START_PM_NODE], START_LS_NODE, 0, 0, mPayloadOffset);
        mPayloadOffset += buildStartPmPayload( imagingCaseId, mPayloadOffset );
    }

    // Set START_LS_NODE
    {
        setLleHeader (mLle[START_LS_NODE], FIRST_PRI_NODE, 0, 0, mPayloadOffset);
        mPayloadOffset += buildStartLsPayload( imagingCaseId, mPayloadOffset );
    }

    mFsClocksPerFrame = 0;
    mActiveImagingInterval = 0;
    mPmNodePayloadOffset = mPayloadOffset;  // This is needed in color flow imaging to clamp frame rate
    // Set the PM_NODE
    {
        uint32_t tmpOffset = mPayloadOffset;
        setLleHeader (mLle[PM_NODE], FIRST_PRI_NODE, 1, 0, mPayloadOffset);
        mPayloadOffset += buildPowerMgmtPayload( imagingCaseId, mPayloadOffset );
        LOGI("PM line BENABLE register contains 0x%08X", mPayload[tmpOffset+1+4]);
    }

    // Set up rest of the linked list
    uint32_t curNode = FIRST_PRI_NODE;
    uint32_t numDummyPings = mUpsReaderSPtr->getBDummyVector(); 
    mNumBNodes += numDummyPings;
    for (uint32_t i = 0; i < mNumBNodes; i++)
    {
        BeamPayload *bpPayload = (BeamPayload *)(&(mPayload[mPayloadOffset]));

        setLleHeader (mLle[FIRST_PRI_NODE+i],
                      curNode+1,
                      1,
                      i,
                      mPayloadOffset);
        setPayloadHeader(bpPayload->header,
                         TBF_REG_BP0,
                         mNumSeqRegs,
                         1);
        memcpy(&(mPayload[mPayloadOffset+1]),
               &(mSequenceBlob[mNumSeqRegs * i]),
               mNumSeqRegs*sizeof(uint32_t));
        uint32_t pingPeriod = mPayload[mPayloadOffset+1+BP_BPRI];  // BPRI

        // Set the bit that start HV voltage and current monitoring on the first ping
        // of every frame. For all other pings this bit is assumed to be cleared.
        if (i == 0)
        {
            uint32_t benable = mPayload[mPayloadOffset+1+BP_BENABLE];
            BIT_SET(benable, BIT(BF_BENABLE_PMON_START_BIT));
            mPayload[mPayloadOffset+1+BP_BENABLE] = benable;
        }
        mFsClocksPerFrame += pingPeriod;
        mActiveImagingInterval += pingPeriod;

        mPayloadOffset += (mNumSeqRegs + 1);
        curNode++;
    }

    // Modify next address of last link to point to PM_NODE
    mLle[FIRST_PRI_NODE + mNumBNodes - 1].next = PM_NODE;

    mLle[FIRST_PRI_NODE + mNumBNodes - 1].sb   = 1;
    mLle[PM_NODE].fb   = 1;

#if 0
    {
        ALOGD( "B-BENABLE[N-2] = 0x%08X",
               mSequenceBlob[mNumSeqRegs*(mNumBNodes-2)+4]);
        ALOGD( "B-BENABLE[N-1] = 0x%08X",
               mSequenceBlob[mNumSeqRegs*(mNumBNodes-1)+4]);
    }
#endif
    // Set power down node
    setLleHeader(mLle[PD_NODE],
                 STOP_NODE,
                 1,
                 0,
                 mPayloadOffset);
    mPayloadOffset += buildPowerDownPayload( imagingCaseId, mPayloadOffset );

    // Set stop node
    setLleHeader(mLle[STOP_NODE],
                 0,  // next element address == 0 => stop
                 1,
                 0,
                 mPayloadOffset);
    mPayloadOffset += buildStopPayload( mPayloadOffset ) - 1;

    if (IMAGING_MODE_COLOR == imagingMode)
    {
        return (THOR_OK);
    }
    adjustPmNodeForDaEcgSynchronization();
    return(THOR_OK);
}

/*
 * @brief Builds a B Sequence for compounding
 *
 * This is similar to BuildBSequence except its loop generates 3 frames.
 *
 * TX LUTs (as those needs to be changed for sterring) are switched on the fly.
 * All TX LUT tables are loaded via payload in this sequencer.
 *
 */

ThorStatus DauSequenceBuilder::buildBCompoundSequence( const uint32_t imagingCaseId, uint32_t numViews,
                                                        uint32_t imagingMode, bool attachTXConfig )
{
    uint32_t    numBNodesInLoop = 0;
    uint32_t    tmpPayloadOffset = 0;

    mNumSeqRegs    = 0;
    mPayloadOffset = 1;

    // Fetch number of Tx lines from UPS
    mNumBNodes = mUpsReaderSPtr->getNumTxBeamsB( imagingCaseId, imagingMode );
    uint32_t imagingType = mUpsReaderSPtr->getImagingType( imagingCaseId );
    if (imagingType == UpsReader::IMAGING_TYPE_ITHI)
        mNumBNodes = mNumBNodes * 2;
    uint32_t blobSize  = mUpsReaderSPtr->getSequenceBlob(imagingCaseId, mSequenceBlob, &mNumSeqRegs);

    if (blobSize == 0)
    {
        LOGE("ups sequence blob read failed\n");
        return(TER_IE_UPS_NULL_BLOB);
    }

    if (mNumSeqRegs > DauSequenceBuilder::MAX_PAYLOAD_WORDS)
    {
        LOGE("DauSequenceBuilder::buildSequence - mNumSeqRegs (%d) > MAX_PAYLOAD_WORDS (%d)\n",
             mNumSeqRegs, DauSequenceBuilder::MAX_PAYLOAD_WORDS);
        return(TER_IE_UPS_INVALID_BLOB);
    }

    // Set the START_NODE
    {
        setLleHeader (mLle[START_NODE], START_PM_NODE, 0, 0, mPayloadOffset);
        mPayloadOffset += buildStartPayload( imagingCaseId, IMAGING_MODE_B, mPayloadOffset );
    }

    // Set START_PM_NODE
    {
        setLleHeader (mLle[START_PM_NODE], START_LS_NODE, 0, 0, mPayloadOffset);
        mPayloadOffset += buildStartPmPayload( imagingCaseId, mPayloadOffset );
    }

    // Set START_LS_NODE
    {
        setLleHeader (mLle[START_LS_NODE], FIRST_PRI_NODE, 0, 0, mPayloadOffset);
        mPayloadOffset += buildStartLsPayload( imagingCaseId, mPayloadOffset );
    }

    mFsClocksPerFrame = 0;
    mActiveImagingInterval = 0;
    mPmNodePayloadOffset = mPayloadOffset;  // This is needed in color flow imaging to clamp frame rate
    // Set the PM_NODE
    {
        tmpPayloadOffset = mPayloadOffset;
        setLleHeader (mLle[PM_NODE], FIRST_PRI_NODE, 1, 0, mPayloadOffset);
        mPayloadOffset += buildPowerMgmtPayload( imagingCaseId, mPayloadOffset );
        LOGI("PM line BENABLE register contains 0x%08X", mPayload[tmpPayloadOffset+1+4]);
        LOGI("PM line BPRI register contains 0x%08X", mPayload[tmpPayloadOffset+1+BP_BPRI]);
    }

    // First Node: TX Table load
    uint32_t curNode = FIRST_PRI_NODE;
    uint32_t numDummyPings = mUpsReaderSPtr->getBDummyVector();
    mNumBNodes += numDummyPings;

    // numBNodes in a Loop
    numBNodesInLoop = mNumBNodes;

    // steering angle loop: conventional = 1
    for (uint32_t j = 0; j < numViews; j++)
    {
        // BNode Loop
        for (uint32_t i = 0; i < numBNodesInLoop; i++)
        {
            BeamPayload *bpPayload = (BeamPayload * )(&(mPayload[mPayloadOffset]));

            setLleHeader(mLle[curNode],
                         curNode + 1,
                         1,
                         i,
                         mPayloadOffset);
            setPayloadHeader(bpPayload->header,
                             TBF_REG_BP0,
                             mNumSeqRegs,
                             1);
            memcpy(&(mPayload[mPayloadOffset + 1]),
                   &(mSequenceBlob[mNumSeqRegs * i]),
                   mNumSeqRegs * sizeof(uint32_t));
            uint32_t pingPeriod = mPayload[mPayloadOffset + 1 + BP_BPRI];  // BPRI

            // Set the bit that start HV voltage and current monitoring on the first ping
            // of every frame. For all other pings this bit is assumed to be cleared.
            if (i == 0)
            {
                uint32_t benable = mPayload[mPayloadOffset+1+BP_BENABLE];
                BIT_SET(benable, BIT(BF_BENABLE_PMON_START_BIT));
                mPayload[mPayloadOffset+1+BP_BENABLE] = benable;
            }

            if (j == 0)
            {
                mFsClocksPerFrame += pingPeriod;
                mActiveImagingInterval += pingPeriod;
            }

            mPayloadOffset += (mNumSeqRegs + 1);

            // attach TX Config Payload
            if (((numViews > 1) || attachTXConfig ) && (i == 0))
            {
                // clear the payload last bit
                bpPayload->header.last = 0;

                // Build TX Config Payload
                mPayloadOffset += buildTXCfgLUTPayload( imagingCaseId + j, mPayloadOffset );
            }

            curNode++;
        }

        // For the next steered angle
        if (numViews > 1 && j < (numViews - 1) )
        {
            uint32_t imgCaseIdShift = j + 1;

            // set flags
            mLle[curNode - 1].sb = 1;

            // PM Node
            setLleHeader (mLle[curNode], curNode + 1, 1, 0, mPmNodePayloadOffset);

            // set frame bounday
            mLle[curNode].fb = 1;

            // PM or dummy Node
            mNumBNodes += 1;
            // update index
            curNode++;

            // Load next imaging case Blob
            uint32_t numBNodesSteer = mUpsReaderSPtr->getNumTxBeamsB( imagingCaseId + imgCaseIdShift, imagingMode );
            uint32_t blobSizeSteer  = mUpsReaderSPtr->getSequenceBlob(imagingCaseId + imgCaseIdShift, mSequenceBlob, &mNumSeqRegs);

            if (blobSizeSteer == 0)
            {
                LOGE("ups sequence blob read failed\n");
                return(TER_IE_UPS_NULL_BLOB);
            }

            if (mNumSeqRegs > DauSequenceBuilder::MAX_PAYLOAD_WORDS)
            {
                LOGE("DauSequenceBuilder::buildSequence - mNumSeqRegs (%d) > MAX_PAYLOAD_WORDS (%d)\n",
                     mNumSeqRegs, DauSequenceBuilder::MAX_PAYLOAD_WORDS);
                return(TER_IE_UPS_INVALID_BLOB);
            }

            mNumBNodes += numBNodesSteer;
            mNumBNodes += numDummyPings;
        }
    }

    // Modify next address of last link to point to PM_NODE
    mLle[FIRST_PRI_NODE + mNumBNodes - 1].next = PM_NODE;

    mLle[FIRST_PRI_NODE + mNumBNodes - 1].sb   = 1;
    mLle[PM_NODE].fb   = 1;

#if 0
    {
        ALOGD( "B-BENABLE[N-2] = 0x%08X",
               mSequenceBlob[mNumSeqRegs*(mNumBNodes-2)+4]);
        ALOGD( "B-BENABLE[N-1] = 0x%08X",
               mSequenceBlob[mNumSeqRegs*(mNumBNodes-1)+4]);
    }
#endif
    // Set power down node
    setLleHeader(mLle[PD_NODE],
                 STOP_NODE,
                 1,
                 0,
                 mPayloadOffset);
    mPayloadOffset += buildPowerDownPayload( imagingCaseId, mPayloadOffset );

    // Set stop node
    setLleHeader(mLle[STOP_NODE],
                 0,  // next element address == 0 => stop
                 1,
                 0,
                 mPayloadOffset);
    mPayloadOffset += buildStopPayload( mPayloadOffset ) - 1;

    if (IMAGING_MODE_COLOR == imagingMode)
    {
        return (THOR_OK);
    }

    // ignore DA/ECG when SC is enabled
    // adjustPmNodeForDaEcgSynchronization();
    return(THOR_OK);
}


ThorStatus DauSequenceBuilder::buildCSequence( const uint32_t imagingCaseId )
{
    uint32_t* blobPtr = &(mSequenceBlob[mNumSeqRegs * mNumBNodes]);
    uint32_t numNodes = mUpsReaderSPtr->getNumTxBeamsC(imagingCaseId);
    uint32_t msrtAfe[TBF_LUT_MSRTAFE_SIZE];
    
    mUpsReaderSPtr->getCFAfeInitLut(imagingCaseId, msrtAfe);
    return (buildCSequence(imagingCaseId, blobPtr, numNodes, msrtAfe, TBF_LUT_MSRTAFE_SIZE));
}

/*
 * @brief Builds a B+C sequence for the desired imaging case.
 *
 * The sequence is placed in a memory block at the specified memory location
 * and uses the corresponding physical address for the sequencer to use.
 *
 * The structure of the sequence built by this function is as follows:
 *
 * Number of links in the linked list = n+m+2, where n = # of Tx PRIs
 * and m = # of colorflow PRIs
 *
 * Size of sequence blob obtained from UPS is assumed to contain payloads
 * for the power management line followed by payloads for the n Tx lines.
 *
 * First link (#0) contains payload for power management with BB = 0.
 *
 *  ## IMPORTANT NOTE ##
 *  ATGC node for colorflow is inserted after the second (index 1) node
 *  to account for the pipeline delay.
 *
 *                +-------------------------+
 *                |                         |
 * +-----------+  |  +-----------+     +----V---+     +---------+
 * | PM (BB=0) |--+  | PM (BB=1) |---->| BL[0]  |---->| BL[n-1] |---+
 * +-----------+     +-----------+     +--------+     +---------+   |
 *                         ^           +--------+     +---------+   |
 *                         |      +--->| CL[1]  |<----|  CL[0]  |---+
 *                         |      |    +--------+     +---------+
 *                         |      |    +--------+
 *                         |      +--->| CFATGC |--+
 *                         |           +--------+  |
 *                         |      +----------------+
 *                         |      |    +--------+     +---------+
 *                         |      +--->| CL[2]  |---->| CL[m-1] |---+
 *                         |           +--------+     +---------+   |
 *                         |                                        |
 *                         +----------------------------------------+
 *
 * This call assumes the following:
 * mSequenceBlob has been loaded with sequence data from the UPS.
 */
ThorStatus DauSequenceBuilder::buildCSequence( const uint32_t imagingCaseId,
                                               uint32_t sequenceBlob[],
                                               uint32_t numCNodes,
                                               uint32_t msrtAfe[],
                                               uint32_t msrtAfeLength)
{
    uint32_t lleStart = FIRST_PRI_NODE + mNumBNodes;

    mNumCNodes = numCNodes;
    
    // Make the last B node to point to first C node
    mLle[FIRST_PRI_NODE + mNumBNodes - 1].next = lleStart;
    mLle[FIRST_PRI_NODE + mNumBNodes - 1].sb   = 0;

    // Save copy of BPRI from previous line for AFE node.
    uint32_t curBPRI = 0;

    // build color links
    uint32_t curNode = lleStart;
    for (int i = 0; i < 2; i++)
    {
        BeamPayload *bpPayload = (BeamPayload *)(&(mPayload[mPayloadOffset]));

        setLleHeader(mLle[lleStart+i],
                     curNode+1,
                     1,
                     i,
                     mPayloadOffset);
        setPayloadHeader(bpPayload->header,
                         TBF_REG_BP0,
                         mNumSeqRegs,
                         1);
        memcpy(&(mPayload[mPayloadOffset+1]),
               &(sequenceBlob[mNumSeqRegs * i]),
               mNumSeqRegs*sizeof(uint32_t));

        uint32_t pingPeriod = mPayload[mPayloadOffset+1+BP_BPRI];
        mFsClocksPerFrame += pingPeriod;
        mActiveImagingInterval += pingPeriod;
        curBPRI = pingPeriod;
        mPayloadOffset += (mNumSeqRegs + 1);
        curNode++;
    }

    // build and insert insert AFE link in between 2nd (index=1) & 3rd (index=2) color beams
    {
        BeamPayload *bpPayload = (BeamPayload *)(&(mPayload[mPayloadOffset]));

        mLle[lleStart+1].next = AFE_NODE;  // inserting AFE node after 2nd color node

        setLleHeader(mLle[AFE_NODE],
                     lleStart+2,            // pointing to 3rd color node after AFE node
                     1,
                     0,
                     mPayloadOffset);
        setPayloadHeader(bpPayload->header,
                         TBF_LUT_MSRTAFE,
                         msrtAfeLength,
                         1);

        UNUSED(imagingCaseId);
        memcpy( &(mPayload[mPayloadOffset+1]), msrtAfe, msrtAfeLength*sizeof(uint32_t));
        mPayloadOffset += (1 + msrtAfeLength);

        mFsClocksPerFrame += curBPRI;
        mActiveImagingInterval += curBPRI;
    }

    // build remainder of color nodes
    for (unsigned i = 2; i < mNumCNodes; i++)
    {
        BeamPayload *bpPayload = (BeamPayload *)(&(mPayload[mPayloadOffset]));

        setLleHeader(mLle[lleStart+i],
                     curNode+1,
                     1,
                     i,
                     mPayloadOffset);
        setPayloadHeader(bpPayload->header,
                         TBF_REG_BP0,
                         mNumSeqRegs,
                         1);
        memcpy(&(mPayload[mPayloadOffset+1]),
               &(sequenceBlob[mNumSeqRegs * i]),
               mNumSeqRegs*sizeof(uint32_t));

        uint32_t pingPeriod = mPayload[mPayloadOffset+1+BP_BPRI];
        mFsClocksPerFrame += pingPeriod;
        mActiveImagingInterval += pingPeriod;
        mPayloadOffset += (mNumSeqRegs + 1);
        curNode++;
    }
    mLle[FIRST_PRI_NODE + mNumBNodes + mNumCNodes - 1].next = PM_NODE;
    mLle[FIRST_PRI_NODE + mNumBNodes + mNumCNodes - 1].sb   = 1;

    uint32_t globalId = mUpsReaderSPtr->getImagingGlobalID(imagingCaseId);

    // Make sure frame doesn't exceed target rate specified in the UPS
    // Increase the BPRI value in the payload associated with PM node.
    {
        const UpsReader::Globals* glbPtr = mUpsReaderSPtr->getGlobalsPtr();
        float fs = glbPtr->samplingFrequency;
        float targetFrameRate = glbPtr->targetFrameRate;
        if ( globalId == 10 )
        {
            targetFrameRate = 18;
        }
        float fr = fs * 1.0e6 / mFsClocksPerFrame;
        LOGD("BuildCSequence: color frame rate without compensation is %f fps", fr);
        if (fr > targetFrameRate)
        {
            float extraTime = (1.0/targetFrameRate) - (1.0/fr);
            uint32_t extraCycles = extraTime * fs * 1.0e6;
            LOGD("BuildCSequence: extra BPRI cycles needed to bring down the frame rate to %f Hz is %d",
                 targetFrameRate, extraCycles);
            LOGD("BuildCSequence: default PM node BPRI setting is %d", mPayload[mPmNodePayloadOffset + 1 + BP_BPRI]);
            mPayload[mPmNodePayloadOffset+1+BP_BPRI] += extraCycles;
            mFsClocksPerFrame += extraCycles;
        }
    }
    adjustPmNodeForDaEcgSynchronization();
    return (THOR_OK);
}

/*
 * @brief Builds a B+C sequence for the Linear imaging case.
 *
 * Can be combined with BuildCSequence
 */
ThorStatus DauSequenceBuilder::buildCSequenceLinear( const uint32_t imagingCaseId,
                                               uint32_t sequenceBlob[],
                                               uint32_t numCNodes,
                                               uint32_t msrtAfe[],
                                               uint32_t msrtAfeLength,
                                               uint32_t lutMSSTTXC[],
                                               uint32_t lutMSSTTXCLength)
{
    // dummy node Reg
    uint32_t    dummyNodeRegs[TBF_NUM_BP_REGS];
    uint32_t    dummyNode = FIRST_PRI_NODE + mNumBNodes;
    uint32_t    lleStart = dummyNode + 1;

    // numCNodes = numCNodes + dummyNode
    mNumCNodes = numCNodes + 1;

    // Make the last B node to point to first C node
    mLle[FIRST_PRI_NODE + mNumBNodes - 1].next = dummyNode;
    mLle[FIRST_PRI_NODE + mNumBNodes - 1].sb   = 0;

    // Save copy of BPRI from previous line for AFE node.
    uint32_t curBPRI = 0;

    // current node
    uint32_t curNode = dummyNode;

    {
        // dummy node - Tx Config Luts Prep Node
        for (uint32_t i = 0; i < TBF_NUM_BP_REGS; i++) {
            dummyNodeRegs[i] = mSequenceBlob[i];
        }
        // Update Regs
        dummyNodeRegs[BP_BENABLE] = 0x01000000;
        dummyNodeRegs[5] = 0x00000060;
        dummyNodeRegs[BP_BPRI] = 0x00008000;

        // Dummy Node
        BeamPayload *bpPayload = (BeamPayload *)(&(mPayload[mPayloadOffset]));

        setLleHeader(mLle[curNode],
                     lleStart,
                     1,
                     0,
                     mPayloadOffset);
        setPayloadHeader(bpPayload->header,
                         TBF_REG_BP0,
                         TBF_NUM_BP_REGS,
                         1);

        memcpy( &(mPayload[mPayloadOffset+1]), dummyNodeRegs,
                TBF_NUM_BP_REGS*sizeof(uint32_t));
        mPayloadOffset += (1 + TBF_NUM_BP_REGS);

        // update FsClocksPerFrame
        mFsClocksPerFrame += dummyNodeRegs[BP_BPRI];

        // active imaging interval
        mActiveImagingInterval += dummyNodeRegs[BP_BPRI];
    }

    //curNode
    curNode = lleStart;

    // build color links
    for (int i = 0; i < 2; i++)
    {
        BeamPayload *bpPayload = (BeamPayload *)(&(mPayload[mPayloadOffset]));

        setLleHeader(mLle[lleStart+i],
                     curNode+1,
                     1,
                     i,
                     mPayloadOffset);
        setPayloadHeader(bpPayload->header,
                         TBF_REG_BP0,
                         mNumSeqRegs,
                         1);
        memcpy(&(mPayload[mPayloadOffset+1]),
               &(sequenceBlob[mNumSeqRegs * i]),
               mNumSeqRegs*sizeof(uint32_t));

        uint32_t pingPeriod = mPayload[mPayloadOffset+1+BP_BPRI];
        mFsClocksPerFrame += pingPeriod;
        mActiveImagingInterval += pingPeriod;
        curBPRI = pingPeriod;
        mPayloadOffset += (mNumSeqRegs + 1);

        // put TX cfg
        if ( i == 0 )
        {
            // clear the payload last bit
            bpPayload->header.last = 0;

            // Build TX Config Payload
            mPayloadOffset += buildTXCfgLUTPayloadColor(lutMSSTTXC, lutMSSTTXCLength, mPayloadOffset);
        }

        curNode++;
    }

    // build and insert insert AFE link in between 2nd (index=1) & 3rd (index=2) color beams
    {
        BeamPayload *bpPayload = (BeamPayload *)(&(mPayload[mPayloadOffset]));

        mLle[lleStart+1].next = AFE_NODE;  // inserting AFE node after 2nd color node

        setLleHeader(mLle[AFE_NODE],
                     lleStart+2,            // pointing to 3rd color node after AFE node
                     1,
                     0,
                     mPayloadOffset);
        setPayloadHeader(bpPayload->header,
                         TBF_LUT_MSRTAFE,
                         msrtAfeLength,
                         1);

        UNUSED(imagingCaseId);
        memcpy( &(mPayload[mPayloadOffset+1]), msrtAfe, msrtAfeLength*sizeof(uint32_t));
        mPayloadOffset += (1 + msrtAfeLength);

        mFsClocksPerFrame += curBPRI;
        mActiveImagingInterval += curBPRI;
    }

    // build remainder of color nodes
    for (unsigned i = 2; i < mNumCNodes - 1; i++)       // NumCNode = real numCNodes + dummy
    {
        BeamPayload *bpPayload = (BeamPayload *)(&(mPayload[mPayloadOffset]));

        setLleHeader(mLle[lleStart+i],
                     curNode+1,
                     1,
                     i,
                     mPayloadOffset);
        setPayloadHeader(bpPayload->header,
                         TBF_REG_BP0,
                         mNumSeqRegs,
                         1);
        memcpy(&(mPayload[mPayloadOffset+1]),
               &(sequenceBlob[mNumSeqRegs * i]),
               mNumSeqRegs*sizeof(uint32_t));

        uint32_t pingPeriod = mPayload[mPayloadOffset+1+BP_BPRI];
        mFsClocksPerFrame += pingPeriod;
        mActiveImagingInterval += pingPeriod;
        mPayloadOffset += (mNumSeqRegs + 1);
        curNode++;
    }

    mLle[FIRST_PRI_NODE + mNumBNodes + mNumCNodes - 1].next = PM_NODE;
    mLle[FIRST_PRI_NODE + mNumBNodes + mNumCNodes - 1].sb   = 1;

    // Make sure frame doesn't exceed target rate specified in the UPS
    // Increase the BPRI value in the payload associated with PM node.
    {
        const UpsReader::Globals* glbPtr = mUpsReaderSPtr->getGlobalsPtr();
        float fs = glbPtr->samplingFrequency;
        float targetFrameRate = glbPtr->targetFrameRate;

        float fr = fs * 1.0e6 / mFsClocksPerFrame;
        LOGD("BuildCSequence: color frame rate without compensation is %f fps", fr);
        if (fr > targetFrameRate)
        {
            float extraTime = (1.0/targetFrameRate) - (1.0/fr);
            uint32_t extraCycles = extraTime * fs * 1.0e6;
            LOGD("BuildCSequence: extra BPRI cycles needed to bring down the frame rate to %f Hz is %d",
                 targetFrameRate, extraCycles);
            LOGD("BuildCSequence: default PM node BPRI setting is %d", mPayload[mPmNodePayloadOffset + 1 + BP_BPRI]);
            mPayload[mPmNodePayloadOffset+1+BP_BPRI] += extraCycles;
            mFsClocksPerFrame += extraCycles;
        }
    }
    adjustPmNodeForDaEcgSynchronization();
    return (THOR_OK);
}

/**
 * @brief Builds M-Mode sequence
 *
 *
 * The general structure of the sequence setup for M-Mode imaging is as shown below:
 *
 *                                  +---------------------------------------------+
 *                                  |              PING SEQUENCE                  |
 *                                  V                                             |
 * +-------+  +----+  +----+    +------+  +------+         +--------+  +--------+ |
 * | START |->| PM |->| LS |--->| L[0] |->| M[0] |-  ... ->| B[N-1] |->| M[N-1] |-+
 * +-------+  +----+  +----+    +------+  +------+         +--------+  +--------+
 *
 *
 *                              +------+  +------+         +--------+  +--------+     
 *                              | L[0] |->| M[0] |-  ... ->| B[N-1] |->| M[N-1] |-+
 *                              +------+  +------+         +--------+  +--------+ |
 *                                  ^                                             |
 *                                  |              PONG SEQUENCE                  |
 *                                  +---------------------------------------------+
 *
 * B-Mode payloads are shared between ping and pong sequences.
 * All M-nodes of ping sequence share the same payload and all M-nodes of pong sequence
 * share the same payload.
 *
 * mPingSeqStartNode points to L[0] of ping sequence
 * mPongSeqStartNode points to L[0] of pong sequence
 *
 * mMModePingPayloadOffset points to the M payloads for the ping sequence
 * mMModePongPayloadOffset points to the M payloads for the pong sequence
 *
 */
ThorStatus DauSequenceBuilder::buildBMSequence(const uint32_t imagingCaseId, uint32_t beamNumber)
{
    UNUSED(beamNumber);
    
    mNumSeqRegs = 0;
    mMmodePingOrPong = 0;  // Make PING sequence active by default
    
    mNumBNodes = mUpsReaderSPtr->getNumTxBeamsB(imagingCaseId, IMAGING_MODE_M);

    mPingSeqStartNode = FIRST_PRI_NODE;
    mPongSeqStartNode = mPingSeqStartNode + (mNumBNodes*2);
    
    uint32_t blobSize  = mUpsReaderSPtr->getSequenceBlob(imagingCaseId, mSequenceBlob, &mNumSeqRegs);

    if (blobSize == 0)
    {
        LOGE("ups sequence blob read failed\n");
        return(TER_IE_UPS_NULL_BLOB);
    }

    if (mNumSeqRegs > DauSequenceBuilder::MAX_PAYLOAD_WORDS)
    {
        LOGE("DauSequenceBuilder::buildSequence - mNumSeqRegs (%d) > MAX_PAYLOAD_WORDS (%d)\n",
             mNumSeqRegs, DauSequenceBuilder::MAX_PAYLOAD_WORDS);
        return(TER_IE_UPS_INVALID_BLOB);
    }

    mPayloadOffset = 1;  // It is illegal to set payload at 0
    
    // Set the START_NODE
    {
        setLleHeader (mLle[START_NODE], START_PM_NODE, 0, 0, mPayloadOffset);
        mPayloadOffset += buildStartPayload( imagingCaseId, IMAGING_MODE_M, mPayloadOffset );
    }

    // Set START_PM_NODE
    {
        setLleHeader (mLle[START_PM_NODE], START_LS_NODE, 0, 0, mPayloadOffset);
        mPayloadOffset += buildStartPmPayload( imagingCaseId, mPayloadOffset );
    }

    // Set START_LS_NODE
    {
        setLleHeader (mLle[START_LS_NODE], FIRST_PRI_NODE, 0, 0, mPayloadOffset);
        mPayloadOffset += buildStartLsPayload( imagingCaseId, mPayloadOffset );
    }

    // Set power down node
    setLleHeader(mLle[PD_NODE],
                 STOP_NODE,
                 1,
                 0,
                 mPayloadOffset);
    mPayloadOffset += buildPowerDownPayload( imagingCaseId, mPayloadOffset );

    // Set stop node
    setLleHeader(mLle[STOP_NODE],
                 0,  // next element address == 0 => stop
                 1,
                 0,
                 mPayloadOffset);
    mPayloadOffset += buildStopPayload( mPayloadOffset ) - 1;

    mMmodePingPayloadOffset = mPayloadOffset + mNumBNodes*(mNumSeqRegs + 1);
    mMmodePongPayloadOffset = mMmodePingPayloadOffset + 2*(mNumSeqRegs + 1);

    uint32_t firstNodePayloadOffset = mPayloadOffset;

    uint32_t curPingNode = mPingSeqStartNode;
    uint32_t curPongNode = mPongSeqStartNode;

    mFsClocksPerFrame = 0;
    mActiveImagingInterval = 0;
    for (uint32_t i = 0; i < mNumBNodes; i++)
    {
        BeamPayload *bpPayload = (BeamPayload *)(&(mPayload[mPayloadOffset]));

        // B-Mode Ping Header
        setLleHeader(mLle[mPingSeqStartNode+2*i],
                     curPingNode+1,
                     1,
                     i,
                     mPayloadOffset);

        // B-Mode Pong Header
        setLleHeader(mLle[mPongSeqStartNode+2*i],
                     curPongNode+1,
                     1,
                     i,
                     mPayloadOffset);

        // B payload shared between ping and pong
        setPayloadHeader(bpPayload->header,
                         TBF_REG_BP0,
                         mNumSeqRegs,
                         1);
        memcpy(&(mPayload[mPayloadOffset+1]),
               &(mSequenceBlob[mNumSeqRegs * i]),
               mNumSeqRegs*sizeof(uint32_t));
        // Set the bit that start HV voltage and current monitoring on the first ping
        // of every frame. For all other pings this bit is assumed to be cleared.
        if (i == 0)
        {
            uint32_t benable = mPayload[mPayloadOffset+1+BP_BENABLE];
            BIT_SET(benable, BIT(BF_BENABLE_PMON_START_BIT));
            mPayload[mPayloadOffset+1+BP_BENABLE] = benable;
        }
        uint32_t pingPeriod = mPayload[mPayloadOffset+1+BP_BPRI];
        mFsClocksPerFrame += 2*pingPeriod;
        mActiveImagingInterval += 2*pingPeriod;
        mPayloadOffset += (mNumSeqRegs + 1);

        curPingNode++;
        curPongNode++;

        // M-Mode Ping Header
        setLleHeader(mLle[mPingSeqStartNode+2*i+1],
                     curPingNode+1,
                     1,
                     i,
                     mMmodePingPayloadOffset);

        // M-Mode Pong Header
        setLleHeader(mLle[mPongSeqStartNode+2*i+1],
                     curPongNode+1,
                     1,
                     i,
                     mMmodePongPayloadOffset);
        
        curPingNode++;
        curPongNode++;
    }

    // Loop back last nodes to their respective beginnings
    mLle[mPingSeqStartNode + 2*mNumBNodes - 1].next = mPingSeqStartNode;
    mLle[mPongSeqStartNode + 2*mNumBNodes - 1].next = mPongSeqStartNode;
    mLle[mPingSeqStartNode + 2*mNumBNodes - 1].sb   = 1;
    mLle[mPongSeqStartNode + 2*mNumBNodes - 1].sb   = 1;
    mLle[mPingSeqStartNode + 2*mNumBNodes - 1].src  = mMmodePingPayloadOffset + (mNumSeqRegs + 1); // need a separate payload to set b_eof = 1
    mLle[mPongSeqStartNode + 2*mNumBNodes - 1].src  = mMmodePongPayloadOffset + (mNumSeqRegs + 1); // need a separate payload to set b_eof = 1

    // set eos and b_eof in the payload for the last B node
    {
        uint32_t benableOffset;  // for the last B node
        benableOffset = firstNodePayloadOffset + (mNumBNodes-1)*(1+mNumSeqRegs) + 1 + BP_BENABLE;
        LOGD("offset for the last B node is %d", benableOffset);
        uint32_t val = mPayload[benableOffset];
        val |= BIT(BENABLE_EOS);
        val |= BIT(BENABLE_B_EOF);
        LOGD("BENABLE for the last B Node is %08X", val);
        mPayload[benableOffset] = val;

        // clear eos set by UPS builder
        benableOffset = firstNodePayloadOffset + (mNumBNodes-2)*(1+mNumSeqRegs) + 1 + BP_BENABLE;
        LOGD("offset for the last but one B node is %d", benableOffset);
        val = mPayload[benableOffset];
        val &= ~(BIT(BENABLE_EOS));
        LOGD("BENABLE for the last but one B Node is %08X", val);
        mPayload[benableOffset] = val;
    }
    
    // Build M payloads
    {
        BeamPayload *bpPayload;
        uint32_t val;

        // -------- Two M payloads for the ping sequence
        bpPayload = (BeamPayload *)(&(mPayload[mMmodePingPayloadOffset]));
        setPayloadHeader(bpPayload->header, TBF_REG_BP0, mNumSeqRegs, 1);
        memcpy(&(mPayload[mMmodePingPayloadOffset+1]),
               &(mSequenceBlob[beamNumber*mNumSeqRegs]),
               mNumSeqRegs*sizeof(uint32_t));
        // clear b_eof
        val = mPayload[mMmodePingPayloadOffset+1+BP_BENABLE];
        LOGD("BENABLE register %08x", val);
        val &= ~(BIT(BENABLE_B_EOF));
        val &= BENABLE_OUTTYPE_MASK;
        val |= 3 << BENABLE_OUTTYPE_BIT;
        mPayload[mMmodePingPayloadOffset+1+BP_BENABLE] = val;
        LOGD("BENABLE register %08x", val);
        
        bpPayload = (BeamPayload *)(&(mPayload[mMmodePingPayloadOffset + mNumSeqRegs + 1]));
        setPayloadHeader(bpPayload->header, TBF_REG_BP0, mNumSeqRegs, 1);
        memcpy(&(mPayload[mMmodePingPayloadOffset+mNumSeqRegs+2]),
               &(mSequenceBlob[beamNumber*mNumSeqRegs]),
               mNumSeqRegs*sizeof(uint32_t));
        // set b_eof
        val = mPayload[mMmodePingPayloadOffset+mNumSeqRegs+2+BP_BENABLE];
        LOGD("BENABLE register %08x", val);
        val |= BIT(BENABLE_B_EOF);
        val &= BENABLE_OUTTYPE_MASK;
        val |= 3 << BENABLE_OUTTYPE_BIT;
        mPayload[mMmodePingPayloadOffset+mNumSeqRegs+2+BP_BENABLE] = val;
        LOGD("BENABLE register %08x", val);
        
        // -------- Two M payloads for the pong sequence
        bpPayload = (BeamPayload *)(&(mPayload[mMmodePongPayloadOffset]));
        setPayloadHeader(bpPayload->header, TBF_REG_BP0, mNumSeqRegs, 1);
        memcpy(&(mPayload[mMmodePongPayloadOffset+1]),
               &(mSequenceBlob[beamNumber*mNumSeqRegs]),
               mNumSeqRegs*sizeof(uint32_t));
        // clear b_eof
        val = mPayload[mMmodePongPayloadOffset+1+BP_BENABLE];
        LOGD("BENABLE register %08x", val);
        val &= ~(BIT(BENABLE_B_EOF));
        val &= BENABLE_OUTTYPE_MASK;
        val |= 3 << BENABLE_OUTTYPE_BIT;
        mPayload[mMmodePongPayloadOffset+1+BP_BENABLE] = val;
        LOGD("BENABLE register %08x", val);
            
        bpPayload = (BeamPayload *)(&(mPayload[mMmodePongPayloadOffset + mNumSeqRegs + 1]));
        setPayloadHeader(bpPayload->header, TBF_REG_BP0, mNumSeqRegs, 1);
        memcpy(&(mPayload[mMmodePongPayloadOffset+mNumSeqRegs+2]),
               &(mSequenceBlob[beamNumber*mNumSeqRegs]),
               mNumSeqRegs*sizeof(uint32_t));
        // set b_eof
        val = mPayload[mMmodePongPayloadOffset+mNumSeqRegs+2+BP_BENABLE];
        LOGD("BENABLE register %08x", val);
        val |= BIT(BENABLE_B_EOF);
        val &= BENABLE_OUTTYPE_MASK;
        val |= 3 << BENABLE_OUTTYPE_BIT;
        mPayload[mMmodePongPayloadOffset+mNumSeqRegs+2+BP_BENABLE] = val;
        LOGD("BENABLE register %08x", val);
    }

    return (THOR_OK);
}

ThorStatus DauSequenceBuilder::updateMLine(uint32_t beamNumber)
{
    if (mMmodePingOrPong == 3)  // BUGBUG: switching from PONG to PING sequence results in PciDauMemroy::write failure.
                                //         There are no visible artifcats updating the pong sequence with M-line movement
    {
        BeamPayload *bpPayload;
        uint32_t val;

        LOGD("switching to PING sequence");
        // Two M payloads for the ping sequence
        bpPayload = (BeamPayload *)(&(mPayload[mMmodePingPayloadOffset]));
        setPayloadHeader(bpPayload->header, TBF_REG_BP0, mNumSeqRegs, 1);
        memcpy(&(mPayload[mMmodePingPayloadOffset+1]),
               &(mSequenceBlob[beamNumber*mNumSeqRegs]),
               mNumSeqRegs*sizeof(uint32_t));
        // clear b_eof
        val = mPayload[mMmodePingPayloadOffset+1+BP_BENABLE];
        val &= ~(BIT(BENABLE_B_EOF));
        val &= BENABLE_OUTTYPE_MASK;
        val |= 3 << BENABLE_OUTTYPE_BIT;
        mPayload[mMmodePingPayloadOffset+1+BP_BENABLE] = val;
            
        bpPayload = (BeamPayload *)(&(mPayload[mMmodePingPayloadOffset + mNumSeqRegs + 1]));
        setPayloadHeader(bpPayload->header, TBF_REG_BP0, mNumSeqRegs, 1);
        memcpy(&(mPayload[mMmodePingPayloadOffset+mNumSeqRegs+2]),
               &(mSequenceBlob[beamNumber*mNumSeqRegs]),
               mNumSeqRegs*sizeof(uint32_t));
        // set b_eof
        val = mPayload[mMmodePingPayloadOffset+mNumSeqRegs+2+BP_BENABLE];
        val |= BIT(BENABLE_B_EOF);
        val &= BENABLE_OUTTYPE_MASK;
        val |= 3 << BENABLE_OUTTYPE_BIT;
        mPayload[mMmodePingPayloadOffset+mNumSeqRegs+2+BP_BENABLE] = val;
        if (!loadMPayload(mMmodePingPayloadOffset))
        {
            LOGE("couldn't load M-mode payload");
            return(TER_IE_SEQ_PLD_LOAD_FAILURE);
        }
        updateLsNodeNext(mPingSeqStartNode);
        mMmodePingOrPong = 0;

        val = mPingSeqStartNode;
        mDaumSPtr->write( &val, SEQ_JMP_ADDR, 1);
        val = SEQ_ACTION_JUMP;
        mDaumSPtr->write( &val, SEQ_SB_ADDR, 1);
    }
    else
    {
        BeamPayload *bpPayload;
        uint32_t val;
        
        LOGD("switching to PONG sequence");
        // Two M payloads for the pong sequence
        bpPayload = (BeamPayload *)(&(mPayload[mMmodePongPayloadOffset]));
        setPayloadHeader(bpPayload->header, TBF_REG_BP0, mNumSeqRegs, 1);
        memcpy(&(mPayload[mMmodePongPayloadOffset+1]),
               &(mSequenceBlob[beamNumber*mNumSeqRegs]),
               mNumSeqRegs*sizeof(uint32_t));
        // clear b_eof
        val = mPayload[mMmodePongPayloadOffset+1+BP_BENABLE];
        val &= ~(BIT(BENABLE_B_EOF));
        val &= BENABLE_OUTTYPE_MASK;
        val |= 3 << BENABLE_OUTTYPE_BIT;
        mPayload[mMmodePongPayloadOffset+1+BP_BENABLE] = val;
        
        bpPayload = (BeamPayload *)(&(mPayload[mMmodePongPayloadOffset + mNumSeqRegs + 1]));
        setPayloadHeader(bpPayload->header, TBF_REG_BP0, mNumSeqRegs, 1);
        memcpy(&(mPayload[mMmodePongPayloadOffset+mNumSeqRegs+2]),
               &(mSequenceBlob[beamNumber*mNumSeqRegs]),
               mNumSeqRegs*sizeof(uint32_t));
        // set b_eof
        val = mPayload[mMmodePongPayloadOffset+mNumSeqRegs+2+BP_BENABLE];
        val |= BIT(BENABLE_B_EOF);
        val &= BENABLE_OUTTYPE_MASK;
        val |= 3 << BENABLE_OUTTYPE_BIT;
        mPayload[mMmodePongPayloadOffset+mNumSeqRegs+2+BP_BENABLE] = val;

        if (!loadMPayload(mMmodePongPayloadOffset))
        {
            LOGE("couldn't load M-mode payload");
            return(TER_IE_SEQ_PLD_LOAD_FAILURE);
        }
        updateLsNodeNext(mPongSeqStartNode);
        mMmodePingOrPong = 1;

        val = mPongSeqStartNode;
        mDaumSPtr->write( &val, SEQ_JMP_ADDR, 1);
        val = SEQ_ACTION_JUMP;
        mDaumSPtr->write( &val, SEQ_SB_ADDR, 1);
    }
    
    return (THOR_OK);
}

/**
 * @brief Builds PW-Mode sequence
 *
 *
 * The general structure of the sequence setup for PW imaging is as shown below:
 *
 *                                           +----------------------------------+
 *                                           |                                  |
 *                                           V                                  |
 * +-------+  +----+  +----+    +------+  +------+         +--------+  +------+ |
 * | START |->| PM |->| LS |--->| P[0] |->| P[1] |- ... -> | P[N-1] |->| P[N] |-+
 * +-------+  +----+  +----+    +------+  +------+         +--------+  +------+
 *                                fb=0                         sb=1      fb=1
 * 
 *
 * Please note two instance of P[0]. The second one at the end is needed to set the 
 * Frame Boundary (fb) flag.
 * */
ThorStatus DauSequenceBuilder::buildPwSequence(uint32_t imagingCaseId, uint32_t prfIndex, uint32_t sequenceBlob[], bool isTDI)
{
    mNumSeqRegs    = 62;
    mPayloadOffset = 1;

    mNumPwNodes = mUpsReaderSPtr->getNumPwSamplesPerFrame( prfIndex, isTDI );
    mNumPwNodes += 1; // adding 1 to account for the extra node needed on start
    
    // check if the PW frame size exceeds sequence LLE memory
    if (mNumPwNodes > (MAX_LLE_HEADERS - FIRST_PRI_NODE))
    {
        LOGE("PW frame size (%d) can not be accomodated in sequence memory");
        return (THOR_ERROR);
    }
    
    // Set the START_NODE
    {
        setLleHeader (mLle[START_NODE], START_PM_NODE, 0, 0, mPayloadOffset);
        mPayloadOffset += buildStartPayload( imagingCaseId, IMAGING_MODE_PW, mPayloadOffset );
    }

    // Set START_PM_NODE
    {
        setLleHeader (mLle[START_PM_NODE], START_LS_NODE, 0, 0, mPayloadOffset);
        mPayloadOffset += buildStartPmPayload( imagingCaseId, mPayloadOffset );
    }

    // Set START_LS_NODE
    {
        setLleHeader (mLle[START_LS_NODE], FIRST_PRI_NODE, 0, 0, mPayloadOffset);
        mPayloadOffset += buildStartLsPayload( imagingCaseId, mPayloadOffset );
    }

    // check if the PW frame size exceeds sequence LLE memory
    if (mNumPwNodes > (MAX_LLE_HEADERS - FIRST_PRI_NODE))
    {
        LOGE("PW frame size (%d) can not be accomodated in sequence memory");
        return (THOR_ERROR);
    }

    mFsClocksPerFrame = 0;
    // Set up the only payload needed for PW sequence
    BeamPayload *bpPayload = (BeamPayload *)(&(mPayload[mPayloadOffset]));

    setPayloadHeader( bpPayload->header, TBF_REG_BP0, mNumSeqRegs, 1);
    memcpy( &(mPayload[mPayloadOffset+1]), sequenceBlob, mNumSeqRegs*sizeof(uint32_t));
    uint32_t pingPeriod = mPayload[mPayloadOffset + 1 + BP_BPRI];
    uint32_t curNode = FIRST_PRI_NODE;
    // An extra node is added to set the frame boundary flag and it points to the
    // second node.
    for (uint32_t i= 0; i < mNumPwNodes; i++)
    {
        setLleHeader (mLle[FIRST_PRI_NODE+i],
                      curNode+1,
                      1,
                      i,
                      mPayloadOffset);
        mFsClocksPerFrame += pingPeriod;
        curNode++;
    }
    mPayloadOffset += (mNumSeqRegs + 1);

    // need to subtract 1 pingPeriod as it repeats from 1 ~ N nodes
    mFsClocksPerFrame -= pingPeriod;
    mActiveImagingInterval = mFsClocksPerFrame;

    // Set up the 2nd payload with eos set.
    // and change LLE_P[N-3] to point to the 2nd payload
    {
        bpPayload = (BeamPayload *) (&(mPayload[mPayloadOffset]));
        setPayloadHeader(bpPayload->header, TBF_REG_BP0, mNumSeqRegs, 1);
        memcpy(&(mPayload[mPayloadOffset + 1]),
               sequenceBlob,
               mNumSeqRegs * sizeof(uint32_t));
        uint32_t benable = mPayload[mPayloadOffset+1+BP_BENABLE];
        BIT_SET(benable, BIT(BF_BENABLE_EOS_BIT));
        mPayload[mPayloadOffset+1+BP_BENABLE] = benable;

        setLleHeader(mLle[FIRST_PRI_NODE + mNumPwNodes -3],
                     FIRST_PRI_NODE + mNumPwNodes - 2,
                     1,
                     mNumPwNodes -2,
                     mPayloadOffset);
        mPayloadOffset += (mNumSeqRegs + 1);
    }

    // Set up the 3rd payload with b_eof set
    // and change LLE_P[N-2] to point to the 3rd payload
    {
        bpPayload = (BeamPayload *) (&(mPayload[mPayloadOffset]));
        setPayloadHeader(bpPayload->header, TBF_REG_BP0, mNumSeqRegs, 1);
        memcpy(&(mPayload[mPayloadOffset + 1]),
               sequenceBlob,
               mNumSeqRegs * sizeof(uint32_t));
        uint32_t benable = mPayload[mPayloadOffset+1+BP_BENABLE];
        BIT_SET(benable, BIT(BF_BENABLE_B_EOF_BIT));
        BIT_SET(benable, BIT(BF_BENABLE_PMON_START_BIT));
        mPayload[mPayloadOffset+1+BP_BENABLE] = benable;
        setLleHeader(mLle[FIRST_PRI_NODE + mNumPwNodes - 2],
                     FIRST_PRI_NODE + mNumPwNodes - 1,
                     1,
                     mNumPwNodes -1,
                     mPayloadOffset);
        mPayloadOffset += (mNumSeqRegs + 1);
    }

    mLle[FIRST_PRI_NODE + mNumPwNodes - 2].sb = 1;
    mLle[FIRST_PRI_NODE + mNumPwNodes - 1].fb = 1;
    mLle[FIRST_PRI_NODE + mNumPwNodes - 1].next = FIRST_PRI_NODE + 1;

    // Set power down node
    setLleHeader(mLle[PD_NODE],
                 STOP_NODE,
                 1,
                 0,
                 mPayloadOffset);
    mPayloadOffset += buildPowerDownPayload( imagingCaseId, mPayloadOffset );

    // Set stop node
    setLleHeader(mLle[STOP_NODE],
                 0,  // next element address == 0 => stop
                 1,
                 0,
                 mPayloadOffset);
    mPayloadOffset += buildStopPayload( mPayloadOffset ) - 1;

    adjustDaEcgSamplesPerFrame();
    return (THOR_OK);
    
}

inline void DauSequenceBuilder::setLleHeader( LleHeader &hdr,
                                              uint32_t nextIndex,
                                              uint32_t bbFlag,
                                              uint32_t indexVal,
                                              uint32_t payloadOffset )
{
    hdr.next  = nextIndex;
    hdr.bb    = bbFlag;
    hdr.fb    = 0;
    hdr.tb    = 0;
    hdr.sb    = 0;
    hdr.check = 0xCADA;
    hdr.index = indexVal;
    hdr.src   = payloadOffset;

    hdr.rsrv0a = 0;
    hdr.rsrv0b = 0;
    hdr.rsrv1a = 0;
    hdr.rsrv1b = 0;
    hdr.rsrv0a = 0;
    hdr.rsrv3  = 0;
}


inline void DauSequenceBuilder::setPayloadHeader( PayloadHeader& hdr,
                                                  uint32_t destAddr,
                                                  uint32_t size,
                                                  uint32_t lastBit )
{
    hdr.dest   = destAddr;
    hdr.wcount = size;
    hdr.last   = lastBit;
}

uint32_t DauSequenceBuilder::buildStartPayload( uint32_t imagingCaseId, uint32_t imagingMode, uint32_t offset )
{
    StartPayload* payloadPtr;
    uint32_t payloadSize = 0;

    payloadPtr = (StartPayload *)(&(mPayload[offset]));

    // construct payload segment for TOP start
    setPayloadHeader(payloadPtr->topHeader,
                     TBF_REG_TR0,
                     TBF_NUM_TOP_REGS,
                     1); // last segment flag is CLEAR
    mUpsReaderSPtr->getTopParamsBlob( payloadPtr->topPayload );

    // for ECG, TBF_REG_PS1
    payloadPtr->topPayload[5] = 0x317f3131;

    {
        // Modify top parameters obtained from UPS to include computed HVPS1/2 reg values
        const uint32_t hvps1Offset = (TBF_REG_HVPS1 - TBF_REG_TR0)/sizeof(uint32_t);
        const uint32_t hvps2Offset = (TBF_REG_HVPS2 - TBF_REG_TR0)/sizeof(uint32_t);
#if 0
        LOGI("============================================");
        LOGI("    TOP REGS in the start payload           ");
        LOGI("============================================");
        for (int i = 0; i < TBF_NUM_TOP_REGS; i++)
        {
            LOGI("TOP[%2d] = 0x%08X", i, payloadPtr->topPayload[i]);
        }
        LOGI("============================================");
        LOGI("hvps1Offset = %d, hvps2offset = %d", hvps1Offset, hvps2Offset);
#endif
        float hvpb, hvnb;

        if ( (imagingMode == IMAGING_MODE_PW) || (imagingMode == IMAGING_MODE_CW) )
        {
            // TODO: check whether we need CW version or change function name?
            mUpsReaderSPtr->getPwHvpsValues(imagingCaseId, hvpb, hvnb);
        }
        else
        {
            mUpsReaderSPtr->getHvpsValues(imagingCaseId, hvpb, hvnb);
        }
        mHvPs1PayloadPtr = &(payloadPtr->topPayload[hvps1Offset]);
        mHvPs2PayloadPtr = &(payloadPtr->topPayload[hvps2Offset]);
        setHvpsRegVals( hvpb, hvnb );
#if 0
        LOGI("============================================");
        LOGI("    TOP REGS in the start payload afer hv setting          ");
        LOGI("============================================");
        for (int i = 0; i < TBF_NUM_TOP_REGS; i++)
        {
            LOGI("TOP[%2d] = 0x%08X", i, payloadPtr->topPayload[i]);
        }
        LOGI("============================================");
#endif
    }

    payloadSize += TBF_NUM_TOP_REGS + 1;

    return (payloadSize);
}

uint32_t DauSequenceBuilder::buildStartPmPayload( uint32_t imagingCaseId, uint32_t offset )
{
    StartPmPayload* payloadPtr;
    uint32_t payloadSize = 0;

    mStartPmNodePayloadOffset = offset;
    payloadPtr = (StartPmPayload *)(&(mPayload[offset]));

    // construct payload segment for power management
    setPayloadHeader(payloadPtr->header,
                     TBF_REG_BP0,
                     TBF_NUM_BP_REGS,
                     1);  // last segment flag is CLEAR
    mUpsReaderSPtr->getPowerMgmtBlob( imagingCaseId, payloadPtr->payload ); // TODO: handle error
    payloadSize += TBF_NUM_BP_REGS + 1;

    return (payloadSize);
}

uint32_t DauSequenceBuilder::buildStartLsPayload( uint32_t imagingCaseId, uint32_t offset )
{
    UNUSED(imagingCaseId);

    StartLsPayload* payloadPtr;
    uint32_t payloadSize = 0;

    payloadPtr = (StartLsPayload *)(&(mPayload[offset]));

    // construct payload segment for LS on
    setPayloadHeader(payloadPtr->header,
                     TBF_REG_LS,
                     1,
                     1);  // last segment flag is SET
    payloadPtr->payload = 1; // turn on LS
    payloadSize += 2;

    return (payloadSize);
}

uint32_t DauSequenceBuilder::buildPowerMgmtPayload( uint32_t imagingCaseId, uint32_t offset )
{

    PowerMgmtPayload* payloadPtr;
    uint32_t payloadSize = 0;

    payloadPtr = (PowerMgmtPayload *)(&(mPayload[offset]));

    // construct payload segment for power management
    setPayloadHeader(payloadPtr->header,
                     TBF_REG_BP0,
                     TBF_NUM_BP_REGS,
                     1); // last segment flag is SET
    mUpsReaderSPtr->getPowerMgmtBlob( imagingCaseId, payloadPtr->payload ); // TODO: handle error
    mFsClocksPerFrame += payloadPtr->payload[BP_BPRI];
    payloadSize += TBF_NUM_BP_REGS + 1;

    return (payloadSize);
}

uint32_t DauSequenceBuilder::buildPowerDownPayload( uint32_t imagingCaseId, uint32_t offset )
{

    PowerDownPayload* payloadPtr;
    uint32_t payloadSize = 0;

    payloadPtr = (PowerDownPayload *)(&(mPayload[offset]));

    // construct payload segment for power management
    setPayloadHeader(payloadPtr->pdHeader,
                     TBF_REG_BP0,
                     TBF_NUM_BP_REGS,
                     0); // last segment flag is CLEAR
    mUpsReaderSPtr->getPowerDownBlob( imagingCaseId, payloadPtr->pdPayload ); // TODO: handle error

    payloadSize += TBF_NUM_BP_REGS + 1;

    // construct payload segment for TOP start
    setPayloadHeader(payloadPtr->topStopHeader,
                     TBF_REG_TR0,
                     TBF_NUM_TOP_REGS,
                     1); // last segment flag is SET.
    mUpsReaderSPtr->getTopStopParamsBlob( payloadPtr->topStopPayload ); // TODO: handle error
    payloadPtr->topStopPayload[1] = 1;  // LS should ON
    payloadSize += TBF_NUM_TOP_REGS + 1;

    return (payloadSize);
}

uint32_t DauSequenceBuilder::buildStopPayload( uint32_t offset )
{

    StopPayload* payloadPtr;
    uint32_t payloadSize = 0;

    payloadPtr = (StopPayload *)(&(mPayload[offset]));

    // construct payload segment for LS on
    setPayloadHeader(payloadPtr->header,
                     TBF_REG_LS,
                     1,
                     1); // last segment flag is SET
    payloadPtr->payload = 0; // turn off LS
    payloadSize += 2;

    return (payloadSize);
}

uint32_t DauSequenceBuilder::buildTXCfgLUTPayload( uint32_t imagingCaseId, uint32_t offset )
{
    uint32_t txConfigLut[TBF_LUT_MSSTTXC_SIZE];
    uint32_t curLoc = 0;            // current offset location in Table
    uint32_t remBufSize = 0;        // remaining buffer size
    uint32_t payloadSize = 0;

    // MAX PAYLOAD HEADER WORD COUNT
    uint32_t maxPldHdrWordCnt = 127;

    remBufSize = mUpsReaderSPtr->getTxConfigLut(imagingCaseId,
            &txConfigLut[0]);  // TODO: handle error

    ALOGD("buildTXCfgLUTPayload - TX Config LUT buffer size: %d", remBufSize);

    // wakeup LUT
    {
        RegPayload* regPayloadPtr = (RegPayload*) (&mPayload[offset + payloadSize]);

        setPayloadHeader(regPayloadPtr->header,
                         TBF_REG_CK0,
                         1,
                         0);         // clear last segment flag

        regPayloadPtr->payload = 0x100;     // wake up for LUT update
        payloadSize += 2;
    }

    while (remBufSize > maxPldHdrWordCnt)
    {
        BeamPayload *bpPayload = (BeamPayload *)(&(mPayload[offset + payloadSize]));

        setPayloadHeader(bpPayload->header,
                         TBF_LUT_MSSTTXC + curLoc * sizeof(uint32_t),
                         maxPldHdrWordCnt,
                         0);        // clear the last segment flag

        memcpy( &(mPayload[offset + payloadSize + 1]), &txConfigLut[curLoc], maxPldHdrWordCnt*sizeof(uint32_t));
        payloadSize += (1 +  maxPldHdrWordCnt);
        curLoc += maxPldHdrWordCnt;

        remBufSize -= maxPldHdrWordCnt;
    }

    // last payload header
    {
        BeamPayload *bpPayload = (BeamPayload *)(&(mPayload[offset + payloadSize]));

        setPayloadHeader(bpPayload->header,
                         TBF_LUT_MSSTTXC + curLoc * sizeof(uint32_t),
                         remBufSize,
                         0);        // clear the last segment flag

        memcpy( &(mPayload[offset + payloadSize + 1]), &txConfigLut[curLoc], remBufSize*sizeof(uint32_t));
        payloadSize += (1 +  remBufSize);
    }

    // sleep LUT
    {
        RegPayload* regPayloadPtr = (RegPayload*) (&mPayload[offset + payloadSize]);

        setPayloadHeader(regPayloadPtr->header,
                         TBF_REG_CK0,
                         1,
                         1);         // set the last segment flag

        regPayloadPtr->payload =0x426F;
        payloadSize += 2;
    }

    return (payloadSize);
}


uint32_t DauSequenceBuilder::buildTXCfgLUTPayloadColor( uint32_t lutMSSTTXC[], uint32_t lutMSSTTXCLength, uint32_t offset )
{
    uint32_t curLoc = 0;            // current offset location in Table
    uint32_t remBufSize = 0;        // remaining buffer size
    uint32_t payloadSize = 0;

    // MAX PAYLOAD HEADER WORD COUNT
    uint32_t maxPldHdrWordCnt = 127;

    remBufSize = lutMSSTTXCLength;

    ALOGD("buildTXCfgLUTPayloadColor - TX Config LUT buffer size: %d", remBufSize);

    // wakeup LUT
    {
        RegPayload* regPayloadPtr = (RegPayload*) (&mPayload[offset + payloadSize]);

        setPayloadHeader(regPayloadPtr->header,
                         TBF_REG_CK0,
                         1,
                         0);         // clear last segment flag

        regPayloadPtr->payload = 0x100;     // wake up for LUT update
        payloadSize += 2;
    }

    while (remBufSize > maxPldHdrWordCnt)
    {
        BeamPayload *bpPayload = (BeamPayload *)(&(mPayload[offset + payloadSize]));

        setPayloadHeader(bpPayload->header,
                         TBF_LUT_MSSTTXC + curLoc * sizeof(uint32_t),
                         maxPldHdrWordCnt,
                         0);        // clear the last segment flag

        memcpy( &(mPayload[offset + payloadSize + 1]), &lutMSSTTXC[curLoc], maxPldHdrWordCnt*sizeof(uint32_t));
        payloadSize += (1 +  maxPldHdrWordCnt);
        curLoc += maxPldHdrWordCnt;

        remBufSize -= maxPldHdrWordCnt;
    }

    // last payload header
    {
        BeamPayload *bpPayload = (BeamPayload *)(&(mPayload[offset + payloadSize]));

        setPayloadHeader(bpPayload->header,
                         TBF_LUT_MSSTTXC + curLoc * sizeof(uint32_t),
                         remBufSize,
                         0);        // set the last segment flag

        memcpy( &(mPayload[offset + payloadSize + 1]), &lutMSSTTXC[curLoc], remBufSize*sizeof(uint32_t));
        payloadSize += (1 +  remBufSize);
    }

    // sleep LUT
    {
        RegPayload* regPayloadPtr = (RegPayload*) (&mPayload[offset + payloadSize]);

        setPayloadHeader(regPayloadPtr->header,
                         TBF_REG_CK0,
                         1,
                         1);         // set the last segment flag

        regPayloadPtr->payload =0x426F;
        payloadSize += 2;
    }

    return (payloadSize);
}

bool DauSequenceBuilder::loadLleBuffer(uint32_t numLles)
{
    bool ret = true;
    if (numLles > MAX_LLE_HEADERS)
    {
        ALOGE("number of LLEs (%d) exceeds the limit (%d)", numLles, MAX_LLE_HEADERS);
        return(false);
    }
#if 0
    ALOGD("loadLleBuffer: numlle: %d", numLles);
    uint32_t* ptr = (uint32_t *)mLle;
    for (uint32_t i = 0; i < numLles*4; i++)
    {
        ALOGD("%4d %08X", i, *ptr++);
    }
#endif

    ret = mDaumSPtr->write( (uint32_t *)(mLle), SEQ_MEM_LLE_START_ADDR, numLles * 4);
    return(ret);
}

bool DauSequenceBuilder::loadPayloadBuffer(uint32_t numWords)
{
    bool ret = true;
    if (numWords > SEQ_MEM_PLD_WORDS)
    {
        ALOGE("Sequence payload size (%d) exceeds the limit (%d)", numWords, SEQ_MEM_PLD_WORDS);
        return(false);
    }
    ALOGD("loadPayloadBuffer: numWords: %d", numWords);
    ret = mDaumSPtr->write( mPayload, SEQ_MEM_PLD_START_ADDR, numWords+1); // word 0 of payload buffer is reserved
    return(ret);
}

bool DauSequenceBuilder::loadMPayload(uint32_t offset)
{
    LOGD("mPaylod+offset = %08X, destination = %08X, size = %d",
         mPayload+offset,
         SEQ_MEM_PLD_START_ADDR + offset*sizeof(uint32_t),
         2*(mNumSeqRegs+1));
    mDaumSPtr->write( mPayload+offset, SEQ_MEM_PLD_START_ADDR + offset*sizeof(uint32_t), 2*(mNumSeqRegs+1));
    return (true);
}

void DauSequenceBuilder::updateLsNodeNext(uint32_t nodeNum)
{
    mLle[START_LS_NODE].next = nodeNum;
    mDaumSPtr->write((uint32_t *)(&(mLle[START_LS_NODE])), SEQ_MEM_LLE_START_ADDR+START_LS_NODE*sizeof(LleHeader), 4);
}

void DauSequenceBuilder::setHvpsRegVals( float hvpb, float hvnb)
{
    uint32_t ps1;
    uint32_t ps2;
    mDauHwSPtr->convertHvToRegVals(hvpb, hvnb, ps1, ps2);

    if ( (mHvPs1PayloadPtr != nullptr) && (mHvPs2PayloadPtr != nullptr) )
    {
        *mHvPs1PayloadPtr = ps1;
        *mHvPs2PayloadPtr = ps2;
    }
}

void DauSequenceBuilder::updateHvPsSetting(float txVoltage)
{
    setHvpsRegVals( txVoltage, txVoltage);
}

float DauSequenceBuilder::getFrameRate()
{
    const UpsReader::Globals* glbPtr = mUpsReaderSPtr->getGlobalsPtr();

    return ( 1.0e6 * glbPtr->samplingFrequency/mFsClocksPerFrame);
}

uint32_t DauSequenceBuilder::getFrameIntervalMs()
{
    const UpsReader::Globals* glbPtr = mUpsReaderSPtr->getGlobalsPtr();

    return ( 1.0e-3 * mFsClocksPerFrame /glbPtr->samplingFrequency );
}

void DauSequenceBuilder::adjustPmNodeForDaEcgSynchronization()
{
    uint32_t afeClksPerEcgSample, afeClksPerDaSample;
    uint32_t pmNodeDuration;

    mUpsReaderSPtr->getAfeClksPerDaEcgSample(0, afeClksPerDaSample, afeClksPerEcgSample);

    mNumEcgSamplesPerFrame = (uint32_t) ceil((double) mFsClocksPerFrame / afeClksPerEcgSample);
    mNumDaSamplesPerFrame = (uint32_t) ceil((double) mFsClocksPerFrame / afeClksPerDaSample);
    uint32_t requiredAdjustment =
            (mNumEcgSamplesPerFrame * afeClksPerEcgSample) - mFsClocksPerFrame;

    LOGD("AfeClocksPerFrame before adjustment = %d (afeClksPerDaSample = %d, afeClksPerEcgSample = %d)",
         mFsClocksPerFrame, afeClksPerDaSample, afeClksPerEcgSample);
    LOGD("---- additional clocks for US, DA, ECG Frame Syncronization = %d",
         requiredAdjustment);
    pmNodeDuration = mPayload[mPmNodePayloadOffset + 1 + BP_BPRI] + requiredAdjustment;
    mPayload[mPmNodePayloadOffset + 1 + BP_BPRI] = pmNodeDuration;
    mFsClocksPerFrame += requiredAdjustment;
    mNumEcgSamplesPerFrame = (uint32_t) ceil((double) mFsClocksPerFrame / afeClksPerEcgSample);
    mNumDaSamplesPerFrame = (uint32_t) ceil((double) mFsClocksPerFrame / afeClksPerDaSample);
    LOGD("After adjustment: ecg samples per frame = %d, da samples per frame = %d",
         mNumEcgSamplesPerFrame, mNumDaSamplesPerFrame);

    mDutyCycle = 100.0f * (mFsClocksPerFrame - mPayload[mPmNodePayloadOffset + 1 + BP_BPRI])/mFsClocksPerFrame;

    // Increase the duration of the START PM node such that it matches the duration of the PM node used
    // in the sequence loop.
    mPayload[mStartPmNodePayloadOffset + 1 + BP_BPRI] = pmNodeDuration;
    mPmNodeDuration = pmNodeDuration;
}

void DauSequenceBuilder::adjustDaEcgSamplesPerFrame()
{
    // this is for M, PW, or CW, which do not have PM node
    uint32_t afeClksPerEcgSample, afeClksPerDaSample;
    uint32_t pmNodeDuration;

    mUpsReaderSPtr->getAfeClksPerDaEcgSample(0, afeClksPerDaSample, afeClksPerEcgSample);

    mNumEcgSamplesPerFrame = (uint32_t) ceil((double) mFsClocksPerFrame / afeClksPerEcgSample);
    mNumDaSamplesPerFrame = (uint32_t) ceil((double) mFsClocksPerFrame / afeClksPerDaSample);
    uint32_t requiredAdjustment =
            (mNumEcgSamplesPerFrame * afeClksPerEcgSample) - mFsClocksPerFrame;

    LOGD("AfeClocksPerFrame before adjustment = %d (afeClksPerDaSample = %d, afeClksPerEcgSample = %d)",
         mFsClocksPerFrame, afeClksPerDaSample, afeClksPerEcgSample);
    LOGD("---- additional clocks for US, DA, ECG Frame Syncronization = %d",
         requiredAdjustment);

    if (requiredAdjustment != 0)
        ALOGW("----  REQUIRED ADJUSTMENT for SCROLL MODE IS NOT ZERO!!!!");
}

ThorStatus DauSequenceBuilder::loadSequence( uint32_t imagingMode )
{
    uint32_t numNodes;
    uint32_t payloadOffset;

    switch (imagingMode)
    {
        case IMAGING_MODE_B:
            numNodes = mNumBNodes;
            payloadOffset = mPayloadOffset;
            break;

        case IMAGING_MODE_COLOR:
            numNodes = mNumBNodes + mNumCNodes;
            payloadOffset = mPayloadOffset;
            break;

        case IMAGING_MODE_M:
            numNodes = 4 * mNumBNodes;
            payloadOffset = mMmodePongPayloadOffset + 1 + mNumSeqRegs; // TODO: needs some explanation here
            break;

        case IMAGING_MODE_PW:
	        numNodes = mNumPwNodes;
	        payloadOffset = mPayloadOffset;
            break;

        case IMAGING_MODE_CW:
            numNodes = mNumCwNodes;
            payloadOffset = mPayloadOffset;
            break;

        default:
            LOGE("%s imaging mode %d not supported", __func__, imagingMode);
            return (TER_IE_IMAGING_MODE);
    }
    if (!loadLleBuffer(FIRST_PRI_NODE + numNodes))
    {
        return(TER_IE_SEQ_LLE_LOAD_FAILURE);
    }
    LOGD("%s loaded %d LLE headers for imaging mode %d", __func__, numNodes, imagingMode);
    if (!loadPayloadBuffer(payloadOffset))
    {
        return(TER_IE_SEQ_PLD_LOAD_FAILURE);
    }
    LOGD("%s loaded %d words of payload for imaging mode %d", __func__, payloadOffset, imagingMode);
    return (THOR_OK);
}
