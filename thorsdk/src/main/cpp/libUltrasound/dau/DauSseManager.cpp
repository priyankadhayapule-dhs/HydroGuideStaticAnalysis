//
// Copyright 2017 EchoNous, Inc.
//
//
#define LOG_TAG "DauSseManager"

#include <DauSseManager.h>
#include <DauSequenceBuilder.h>
#include <DauRegisters.h>
#include <UpsReader.h>
#include <BitfieldMacros.h>

DauSseManager::DauSseManager(const std::shared_ptr<DauMemory>& daum,
                             const std::shared_ptr<DmaBuffer>& dmaBuffer) :
    mDaumSPtr(daum),
    mDmaBufferSPtr(dmaBuffer)
{
    LOGD("*** DauSseManager - constructor");
}

DauSseManager::~DauSseManager()
{
    LOGD("*** DauSseManager - destructor");
}

ThorStatus DauSseManager::init()
{
    uint32_t val;

    val = SEQ_ACTION_CONTINUE;
    mDaumSPtr->write( &val, SEQ_BB_ADDR, 1);
    mDaumSPtr->write( &val, SEQ_FB_ADDR, 1);
    mDaumSPtr->write( &val, SEQ_TB_ADDR, 1);
    

    LOGI("SSE initialized");
    return (THOR_OK);
}

ThorStatus DauSseManager::start()
{
    uint32_t val;
    
    // provide the starting LLE index to sequencer
    val = DauSequenceBuilder::START_NODE;
    mDaumSPtr->write( &val, SEQ_JMP_ADDR, 1);

    // Initialize Action Registers
    val = SEQ_ACTION_CONTINUE;
    mDaumSPtr->write(&val, SEQ_BB_ADDR, 1);
    mDaumSPtr->write(&val, SEQ_FB_ADDR, 1);
    mDaumSPtr->write(&val, SEQ_TB_ADDR, 1);
    mDaumSPtr->write(&val, SEQ_SB_ADDR, 1);
    
    // Disable 100MHz clock
    val = 0x302;
    mDaumSPtr->write(&val, SYS_TCTL_ADDR, 1);
    val = 0x400;
    mDaumSPtr->write(&val, SEQ_DEBUG_ADDR, 1);
    val = 0x0102;
    mDaumSPtr->write(&val, TBF_DP17 + TBF_BASE, 1);

    // Set the GO bit
    val = 1;
    mDaumSPtr->write(&val, SEQ_CMD_ADDR, 1);
    LOGI("SSE started");
    return (THOR_OK);
}


ThorStatus DauSseManager::stop(uint32_t waitTimeMs)
{
    uint32_t val;

    // Clear the GO bit
    val = 0;
    mDaumSPtr->write(&val, SEQ_CMD_ADDR, 1);

    // stop sequence starts from Power Down Node
    val = DauSequenceBuilder::PD_NODE;
    mDaumSPtr->write( &val, SEQ_JMP_ADDR, 1);

    val = SEQ_ACTION_JUMP;
    mDaumSPtr->write(&val, SEQ_SB_ADDR, 1);

    {
        uint32_t pollingIntervalMs = 5;
        uint32_t waitCount = (waitTimeMs/pollingIntervalMs) + 4; // ~ 20 ms grace period added
        uint32_t tryCount = 0;
        uint32_t status;
        uint32_t busyBit;

        for (uint32_t i = 0; i < waitCount; i++)
        {
            usleep(pollingIntervalMs * 1000);
            tryCount++;
            mDaumSPtr->read(SEQ_ST_ADDR, &status, 1);
            busyBit = BFN_GET(status, SEQ_ST_BUSY);
            if (busyBit == 0)
                break;
        }
        LOGI( "SSE status of sequencer on STOP: 0x%08X, tryCount = %d, (%d ms/count, maxWaitTime = %d ms)",
              status, tryCount, pollingIntervalMs, waitTimeMs );
        if ( (tryCount == waitCount) && (busyBit == 1) )
        {
            LOGE("SSE could not stop the sequencer");
            return( TER_IE_SEQ_STALLED );
        }
    }
    LOGI("SSE stopped");
    return(THOR_OK);
} 

