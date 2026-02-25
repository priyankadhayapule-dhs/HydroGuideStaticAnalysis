//
// Copyright (C) 2017 EchoNous, Inc.
//
#pragma once

#include <cstdint>
#include <memory>
#include <UpsReader.h>
#include <ThorConstants.h>
#include <DauMemory.h>
#include <TbfRegs.h>
#include <DauHw.h>
#include <DauRegisters.h>
#include <ThorError.h>


class DauSequenceBuilder
{
public:
    typedef struct
    {
        // Word 0
        uint32_t next      : 10;  // 16-byte-word address (= byte address >> 4)
        uint32_t rsrv0a    :  6;  // blank
        uint32_t bb        :  1;  // Beam boundary flag
        uint32_t fb        :  1;  // Frame boundary flag
        uint32_t tb        :  1;  // Timer boundary flag
        uint32_t sb        :  1;  // Sequence boundary flag
        uint32_t rsrv0b    : 12;  // blank

        // Word 1
        uint32_t src       : 17;  // 4-byte-word address  (= byte address >> 2)
        uint32_t rsrv1a    :  3;  // blank
        uint32_t index     : 10;  // SW Index, Usable any way software desires
        uint32_t rsrv1b    :  2;  // blank

        // Word 2
        uint32_t tcount    : 16;  // Timer count in 1 us ticks.
        uint32_t check     : 16;  // Magic number (or TBD CRC-16)

        // Word 3
        uint32_t rsrv3;
    } LleHeader;

    typedef struct
    {
        uint32_t dest      : 24; // byte address
        uint32_t wcount    :  7; // payload size in words
        uint32_t last      :  1; // last flag
    } PayloadHeader;

    typedef struct
    {
        PayloadHeader	topHeader;
        uint32_t		topPayload[ TBF_NUM_TOP_REGS ];
    } StartPayload;

    typedef struct
    {
        PayloadHeader  header;
        uint32_t       payload[ TBF_NUM_BP_REGS ];
    } StartPmPayload;

    typedef struct
    {
        PayloadHeader   header;
        uint32_t        payload;
    } StartLsPayload;

    typedef struct
    {
        PayloadHeader  header;
        uint32_t       payload[ TBF_NUM_BP_REGS ];
    } PowerMgmtPayload;

    typedef struct
    {
        PayloadHeader  header;
        uint32_t       payload[ TBF_NUM_BP_REGS ];
    } BeamPayload;

    typedef struct
    {
        PayloadHeader   pdHeader;
        uint32_t        pdPayload[ TBF_NUM_BP_REGS ];
        PayloadHeader	topStopHeader;
        uint32_t		topStopPayload[ TBF_NUM_TOP_REGS ];
    } PowerDownPayload;

    typedef struct
    {
        PayloadHeader   header;
        uint32_t        payload;
    } StopPayload;

    // regular register payload
    typedef struct
    {
        PayloadHeader   header;
        uint32_t        payload;
    } RegPayload;

public: // methods
    DauSequenceBuilder(const std::shared_ptr<DauMemory>& daum,
                       const std::shared_ptr<UpsReader>& upsReader,
                       const std::shared_ptr<DauHw>& dauHw);
    ~DauSequenceBuilder();

    ThorStatus buildBSequence(const uint32_t imagingCaseId, uint32_t imagingMode = IMAGING_MODE_B);
    ThorStatus buildBCompoundSequence(const uint32_t imagingCaseId, const uint32_t numAngles,
                                      uint32_t imagingMode = IMAGING_MODE_B,
                                      bool attachTXConfig = false);
    ThorStatus buildCSequence(const uint32_t imagingCaseId);
    ThorStatus buildCSequence(const uint32_t imagingCaseId,
                              uint32_t sequenceBlob[],
                              uint32_t numCNodes ,
                              uint32_t msrtAfe[],
                              uint32_t msrtAfeLength);
    ThorStatus buildBCSequence(const uint32_t imagingCaseId,
                               uint32_t cSequenceBlob[],
                               uint32_t numCNodes,
                               uint32_t msrtAfe[],
                               uint32_t msrtAfeLength);
    ThorStatus buildCSequenceLinear(const uint32_t imagingCaseId,
                              uint32_t sequenceBlob[],
                              uint32_t numCNodes ,
                              uint32_t msrtAfe[],
                              uint32_t msrtAfeLength,
                              uint32_t lutMSSTTXC[],
                              uint32_t lutMSSTTXCLength);
    ThorStatus buildBCSequenceLinear(const uint32_t imagingCaseId,
                               uint32_t cSequenceBlob[],
                               uint32_t numCNodes,
                               uint32_t msrtAfe[],
                               uint32_t msrtAfeLength,
                               uint32_t lutMSSTTXC[],
                               uint32_t lutMSSTTXCLength);

    ThorStatus buildPwSequence(uint32_t imagingCaseId, uint32_t prfIndex, uint32_t sequenceBlob[], bool isTDI);
    ThorStatus loadSequence( uint32_t imagingMode );
    ThorStatus buildBMSequence(const uint32_t imagingCaseId, uint32_t beamNumber);
    ThorStatus updateMLine(uint32_t beamNumber);
    void       updateHvPsSetting( float txVoltage );

    float getFrameRate();
    uint32_t getFrameIntervalMs();
    uint32_t getActiveImagingInterval() { return(mActiveImagingInterval);}
    uint32_t getNumEcgSamplesPerFrame() { return(mNumEcgSamplesPerFrame);}
    uint32_t getNumDaSamplesPerFrame() { return(mNumDaSamplesPerFrame);}
    float    getDutyCycle() { return(mDutyCycle);}
    uint32_t getNumCNodes() { return(mNumCNodes);}
    uint32_t getNumBNodes() { return(mNumBNodes);}
    uint32_t getNumPwNodes() { return(mNumPwNodes);}
    uint32_t getPayloadSize() { return(mPayloadOffset);}
    uint32_t getPmNodeDuration() { return(mPmNodeDuration);}
    const LleHeader* getLleHeaderPtr() { return(&(mLle[0]));}
    
    //    uint64_t getPowerDownAddr() { return (mPowerDownAddr); }

    static const uint32_t MAX_PAYLOAD_WORDS = 64;
    static const uint32_t MAX_LLE_HEADERS   = SEQ_MEM_LLE_WORDS*sizeof(uint32_t)/sizeof(LleHeader);

    // Following constants establish key nodes of any sequence
    static const uint32_t START_NODE        = 0;                 // Sequence is always started from here
    static const uint32_t START_PM_NODE     = 1;
    static const uint32_t START_LS_NODE     = 2;
    static const uint32_t PM_NODE           = 3;                 //
    static const uint32_t PD_NODE           = 4;
    static const uint32_t STOP_NODE         = 5;
    static const uint32_t AFE_NODE          = 6;
    static const uint32_t FIRST_PRI_NODE    = 7;


private:
    void setLleHeader(LleHeader &hdr,
                      uint32_t nextIndex,
                      uint32_t bbFlag,
                      uint32_t indexVal,
                      uint32_t payloadOffset);

    void setPayloadHeader(PayloadHeader &hdr,
                          uint32_t destAddr,
                          uint32_t size,
                          uint32_t lastBit);

    uint32_t buildStartPayload     ( uint32_t imagingCaseId, uint32_t imagingMode, uint32_t offset );
    uint32_t buildStartPmPayload   ( uint32_t imagingCaseId, uint32_t offset );
    uint32_t buildStartLsPayload   ( uint32_t imagingCaseId, uint32_t offset );
    uint32_t buildPowerDownPayload ( uint32_t imagingCaseId, uint32_t offset );
    uint32_t buildStopPayload      ( uint32_t offset );
    uint32_t buildPowerMgmtPayload ( uint32_t imagingCaseId, uint32_t offset );
    uint32_t buildTXCfgLUTPayload  ( uint32_t imagingCaseId, uint32_t offset );
    uint32_t buildTXCfgLUTPayloadColor  ( uint32_t lutMSSTTXC[], uint32_t lutMSSTTXCLength, uint32_t offset );

    void     adjustPmNodeForDaEcgSynchronization();
    void     adjustDaEcgSamplesPerFrame();

    void setHvpsRegVals( float hvpb, float hvnb );

    bool loadLleBuffer(uint32_t numLles);
    bool loadPayloadBuffer(uint32_t numPayloadWords );
    bool loadMPayload(uint32_t offset);
    void updateLsNodeNext(uint32_t nodeNum);

private:
    // members used for book keeping purposes in the construction of sequencer
    uint32_t                    mNumSeqRegs;
    uint32_t                    mPayloadOffset;
    uint32_t                    mNumBNodes;
    uint32_t                    mNumCNodes;
    uint32_t                    mNumPwNodes;
    uint32_t                    mNumCwNodes;
private: // members
    uint32_t                    mPayload[SEQ_MEM_PLD_WORDS];
    LleHeader                   mLle[MAX_LLE_HEADERS];

    uint32_t                    mSequenceBlob[UpsReader::MAX_SEQUENCE_BLOB_SIZE];
    uint32_t                    mFsClocksPerFrame;
    uint32_t                    mActiveImagingInterval;
    uint32_t                    mPmNodePayloadOffset;
    uint32_t                    mStartPmNodePayloadOffset;
    uint32_t                    mPmNodeDuration;
//    uint64_t                    mPowerDownAddr;

    // Node indices for the ping & pong sequences used in conjunction with
    // M-Mode imaging
    uint32_t                    mPingSeqStartNode;
    uint32_t                    mPongSeqStartNode;
    uint32_t                    mMmodePingOrPong;  // 0 => Ping 1 => Pong

    // Payload memory offset for the M-Mode payloads
    // There are two M-Mode payload nodes each for ping and pong sequences.
    // The only difference between the two is the way b_eof flag is set.
    uint32_t                    mMmodePingPayloadOffset;
    uint32_t                    mMmodePongPayloadOffset;
    uint32_t                    mNumEcgSamplesPerFrame;
    uint32_t                    mNumDaSamplesPerFrame;
    float                       mDutyCycle;
    uint32_t*                   mHvPs1PayloadPtr;
    uint32_t*                   mHvPs2PayloadPtr;
    std::shared_ptr<DauMemory>  mDaumSPtr;
    std::shared_ptr<UpsReader>  mUpsReaderSPtr;
    std::shared_ptr<DauHw>      mDauHwSPtr;
};
