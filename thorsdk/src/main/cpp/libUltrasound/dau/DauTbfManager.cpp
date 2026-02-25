//
// Copyright (C) 2017 EchoNous, Inc.
//
//
#define LOG_TAG   "DauTbfManager"

#include <cstdint>
#include <DauTbfManager.h>
#include <TbfRegs.h>
#include <DauRegisters.h>
#include <BitfieldMacros.h>

//-----------------------------------------------------------------------------
DauTbfManager::DauTbfManager(const std::shared_ptr<DauMemory>& daum ) :
    mDaumSPtr(daum)
{
    AsicTestCount = 0;
    LOGD("*** DauTbfManager - constructor");
}

//-----------------------------------------------------------------------------
DauTbfManager::~DauTbfManager()
{
    LOGD("*** DauTbfManager - destructor");
}

//-----------------------------------------------------------------------------
bool DauTbfManager::ASICinit()
{
    uint32_t val;
    uint32_t count = 0;
    bool asicok = false;

    // This loop was intended to support multiple retry attempts but is currently not used that way
    // The flag bfok will be set to True no matter if the test beam is a pass or fail.
    bool bfok = false;
    // Read and save state of CK0
    uint32_t ck0val;
    mDaumSPtr->read(TBF_REG_CK0+TBF_BASE, &ck0val, 1);
    val = 0x00000C0C;
    mDaumSPtr->write(&val, TBF_DP17 + TBF_BASE, 1);

    while (bfok == false) {
        // The following writes to the top level and beam parameters
        // Configure the DAU for beamforming as a test of the power up condition of the ASIC
        //
        val = 0x00000000;
        mDaumSPtr->write(&val, TBF_REG_LS + TBF_BASE, 1);
        val = 0x00000100;
        mDaumSPtr->write(&val, TBF_REG_CK0 + TBF_BASE, 1);
        val = 0;
        mDaumSPtr->write(&val, TBF_REG_TR0 + TBF_BASE, 1);
        val = 1;
        mDaumSPtr->write(&val, TBF_REG_TR0 + TBF_BASE, 1);
        val = 0;
        mDaumSPtr->write(&val, TBF_REG_TR0 + TBF_BASE, 1);
        val = 1;
        mDaumSPtr->write(&val, TBF_REG_TR0 + TBF_BASE, 1);
        val = 0;
        mDaumSPtr->write(&val, TBF_REG_TR0 + TBF_BASE, 1);
        val = 0x000F0007;
        mDaumSPtr->write(&val, TBF_REG_EN0 + TBF_BASE, 1);
        val = 0x0000004B;
        mDaumSPtr->write(&val, TBF_REG_AFE0 + TBF_BASE, 1);
        val = 0x000F0007;
        mDaumSPtr->write(&val, TBF_REG_EN0 + TBF_BASE, 1);
        val = 0x00000100;
        mDaumSPtr->write(&val, TBF_REG_CK0 + TBF_BASE, 1);

        val = 0x0100165F;
        mDaumSPtr->write(&val, TBF_REG_BP0 + TBF_BASE, 1);
        val = 0x00000154;
        mDaumSPtr->write(&val, TBF_REG_FPS + TBF_BASE, 1);
        val = 0x01FF153F;
        mDaumSPtr->write(&val, TBF_REG_BPS + TBF_BASE, 1);
        val = 0x017D0033;
        mDaumSPtr->write(&val, TBF_REG_FPLUT + TBF_BASE, 1);
        // Note that only beamforming is enabled, not ADC input or transmit
        val = 0x01080004;
        mDaumSPtr->write(&val, TBF_REG_BENABLE + TBF_BASE, 1);

        val = 0xFFFFFFFF;
        mDaumSPtr->write(&val, TBF_REG_RXAP_LOW + TBF_BASE, 1);
        val = 0xFFFFFFFF;
        mDaumSPtr->write(&val, TBF_REG_RXAP_HIGH + TBF_BASE, 1);
        val = 0xFFFFFFFF;
        mDaumSPtr->write(&val, TBF_REG_TXAP_LOW + TBF_BASE, 1);
        val = 0xFFFFFFFF;
        mDaumSPtr->write(&val, TBF_REG_TXAP_HIGH + TBF_BASE, 1);
        val = 0x00006000;
        mDaumSPtr->write(&val, TBF_REG_BPRI + TBF_BASE, 1);
        val = 0x02800080;
        mDaumSPtr->write(&val, TBF_REG_TXRXT + TBF_BASE, 1);
        val = 0x00180500;
        mDaumSPtr->write(&val, TBF_REG_TXLEN + TBF_BASE, 1);
        val = 0x00000060;
        mDaumSPtr->write(&val, TBF_REG_BPWR + TBF_BASE, 1);
        val = 0x00001000;
        mDaumSPtr->write(&val, TBF_REG_TXLUT + TBF_BASE, 1);
        val = 0x301F4000;
        mDaumSPtr->write(&val, TBF_REG_VBPF0 + TBF_BASE, 1);
        val = 0x564E0B0A;
        mDaumSPtr->write(&val, TBF_REG_VBPF1 + TBF_BASE, 1);
        val = 0x01FF0054;
        mDaumSPtr->write(&val, TBF_REG_VBPF2 + TBF_BASE, 1);
        val = 0xAAAA153F;
        mDaumSPtr->write(&val, TBF_REG_VBPF3 + TBF_BASE, 1);

        val = 0x0600E2BF;
        mDaumSPtr->write(&val, TBF_REG_DTGC0 + TBF_BASE, 1);
        val = 0x3A00E576;
        mDaumSPtr->write(&val, TBF_REG_DTGC1 + TBF_BASE, 1);
        val = 0x0000E83C;
        mDaumSPtr->write(&val, TBF_REG_DTGC2 + TBF_BASE, 1);
        val = 0x0000EB13;
        mDaumSPtr->write(&val, TBF_REG_DTGC3 + TBF_BASE, 1);

        val = 0x00000000;
        mDaumSPtr->write(&val, TBF_REG_DTGC4 + TBF_BASE, 1);
        val = 0x03955500;
        mDaumSPtr->write(&val, TBF_REG_DTGC5 + TBF_BASE, 1);
        val = 0x00020000;
        mDaumSPtr->write(&val, TBF_REG_CCOMP0 + TBF_BASE, 1);
        val = 0x00000000;
        mDaumSPtr->write(&val, TBF_REG_IRPTR0 + TBF_BASE, 1);
        // In addition to the above registers the LUT MSDIS (interpolation rate)
        // must be filled with zero values for the test beam.
        {
            val = 0x00000000;
            for (int i = 0; i < 128; i++) {
                mDaumSPtr->write(&val, TBF_LUT_MSDIS + TBF_BASE + 4 * i, 1);
            }
        }

        // Trigger beamforming using the SW LS trigger
        val = 0x0;
        mDaumSPtr->write(&val, TBF_TRIG_LS + TBF_BASE, 1);
        // Waiting for 10ms
        usleep(10 * 1000);

        // We now read the register ROBT to determine it the test beam processing was correct
        // or not.
        uint32_t robt[1];
        mDaumSPtr->read(TBF_RO_ID + 0x1C + TBF_BASE, robt, 1);
        LOGI("TOP BF_CHECK ROBT [%2d] = 0x%08X", AsicTestCount, robt[0]);

        if (robt[0] != 0x55D5) {
            // The beam time to too long, indicating the BF is in an invalid state
            LOGI("TOP BF_CHECK FAIL, doing reset ");

            // Toggle the SW reset for the BF logic
            val = 1;
            mDaumSPtr->write(&val, TBF_REG_TR0 + TBF_BASE, 1);
            usleep(100);
            val = 0;
            mDaumSPtr->write(&val, TBF_REG_TR0 + TBF_BASE, 1);
            usleep(100);
            val = 1;
            mDaumSPtr->write(&val, TBF_REG_TR0 + TBF_BASE, 1);
            usleep(100);
            val = 0;
            mDaumSPtr->write(&val, TBF_REG_TR0 + TBF_BASE, 1);
            usleep(100);

            // TODO: A full power down/up will be done if this method returns false
            if (count < 8) {
                count += 1;
            } else {
                bfok = true;
            }
        } else {
            // The beam time is correct, no action needed.
            LOGI("TOP BF_CHECK Pass! ");
            bfok = true;
            asicok = true;
            AsicTestCount += 1;
        }
    }
    // Restore state of CK0
    mDaumSPtr->write( &ck0val, TBF_REG_CK0+TBF_BASE, 1 );
    return (asicok);
}

//-----------------------------------------------------------------------------
bool DauTbfManager::init(DauLutManager& lutm)
{
    bool        retVal = false;
    uint32_t    val = 0;

    // Enable test pattern
    val = 0x10300;
    //mDaumSPtr->write(&val, TBF_DP12+TBF_BASE, 1);
    val = 0x1000;
    // Enable page mode for beam parameter registers
    //mDaumSPtr->write(&val, TBF_DP23+TBF_BASE, 1);


    val = 0x0;
    mDaumSPtr->write(&val, TBF_DP9 + TBF_BASE, 1);

    mDaumSPtr->read(TBF_RO_ID + TBF_BASE, &val, 1);
    switch (val)
    {
    case TBF_VERSION_V1:
        LOGD( "TBF version (%08x) is Proto V1", val);
        break;

    case TBF_VERSION_V2:
        LOGD("TBF version (%08x) is Proto v1p or V2", val);
        break;

    case TBF_VERSION_ASIC:
        LOGD("TOP TBF version (%08x) is AX ASIC", val);
        val = 0xF40; // change needed for V3
        mDaumSPtr->write(&val, TBF_DP24+TBF_BASE, 1);
        break;

    default:
        LOGD("TBF version read from ROID (%08X) in invalid", val);
        return (false);
    }
    {
        uint32_t topRegs[TBF_NUM_TOP_REGS];

        mDaumSPtr->read( TBF_REG_TR0+TBF_BASE, topRegs, TBF_NUM_TOP_REGS);
        LOGI("============================================");
        LOGI("    TOP REGS before loading LUTS            ");
        LOGI("============================================");
        for (int i = 0; i < TBF_NUM_TOP_REGS; i++)
        {
            LOGI("TOP[%2d] = 0x%08X", i, topRegs[i]);
        }
        LOGI("============================================");
    }

    lutm.wakeupLuts();
    if (!lutm.loadGlobalLuts())
    {
        LOGE("Failed to load global LUTs");
        goto err_ret;
    }
    lutm.sleepLuts();

    val = 0x0C0C;
    mDaumSPtr->write(&val, TBF_DP17+TBF_BASE, 1);  // Sets the test points to show beamformer busy & data valid out

    val = 1;
    mDaumSPtr->write(&val, SYS_TCTL_ADDR, 1);  // Selects Beamformer LEDs

    retVal = true;

    LOGI("DauTbfMgr::init()");
    goto ok_ret;
    
err_ret:
ok_ret:

    LOGI("TBF initialized");
    return (retVal);
}


//-----------------------------------------------------------------------------
// Reinitialize beamformer registers after a reset.
void DauTbfManager::reinit()
{
    uint32_t val;

    val = 0;
    mDaumSPtr->write(&val, TBF_REG_TR0 + TBF_BASE, 1);
    val = 1;
    mDaumSPtr->write(&val, TBF_REG_TR0 + TBF_BASE, 1);
    val = 0;
    mDaumSPtr->write(&val, TBF_REG_TR0 + TBF_BASE, 1);
    mDaumSPtr->write(&val, TBF_DP9 + TBF_BASE, 1);
    val = 0x1000;
    mDaumSPtr->write(&val, TBF_DP23 + TBF_BASE, 1);
    mDaumSPtr->read(TBF_RO_ID + TBF_BASE, &val, 1);

    if (val == TBF_VERSION_ASIC)
    {
        val = 0xF40; // change needed for V3
        mDaumSPtr->write(&val, TBF_DP24 + TBF_BASE, 1);
    }
    val = 0x0C0C;
    mDaumSPtr->write(&val, TBF_DP17 + TBF_BASE,
                     1);  // Sets the test points to show beamformer busy & data valid out
    val = 1;
    mDaumSPtr->write(&val, SYS_TCTL_ADDR, 1);  // Selects Beamformer LEDs

    // This read and write back is needed as part of restoring the state of TBF after a reset
    // Otherwise TX clock gets turned off by default. CK0 has a shadow register and when the TBF
    // is reset the actual register gets cleared which turns on the Tx clock. The shadow register
    // is readble and its contents are not reset.
    mDaumSPtr->read(TBF_REG_CK0+TBF_BASE, &val, 1);
    mDaumSPtr->write( &val, TBF_REG_CK0+TBF_BASE, 1 );
}

//-----------------------------------------------------------------------------
void DauTbfManager::fillTestPattern()
{
    uint32_t val;
    uint32_t delayMemoryAddr = 0x600000;
    
    // brodcast to all 64 channels
    val = 0x1000000;
    mDaumSPtr->write( &val, 0x410010, 1);

    for (int i = 0; i < 128; i++)
    {
        val = 0x07D007D0;
        mDaumSPtr->write(&val, delayMemoryAddr+i*32,       1);
        mDaumSPtr->write(&val, delayMemoryAddr+i*32 + 4,   1);
        mDaumSPtr->write(&val, delayMemoryAddr+i*32 + 8,   1);
        mDaumSPtr->write(&val, delayMemoryAddr+i*32 + 12,  1);
        val = 0xF830F830;
        mDaumSPtr->write(&val, delayMemoryAddr+i*32 + 16,  1);
        mDaumSPtr->write(&val, delayMemoryAddr+i*32 + 20,  1);
        mDaumSPtr->write(&val, delayMemoryAddr+i*32 + 24,  1);
        mDaumSPtr->write(&val, delayMemoryAddr+i*32 + 28,  1);
    }
    
    val = 1;
    mDaumSPtr->write( &val, 0x410040, 1);
    val = 0xffffffff;
    mDaumSPtr->write( &val, 0x410014, 1);
    mDaumSPtr->write( &val, 0x410018, 1);
}

//-----------------------------------------------------------------------------
bool DauTbfManager::setImagingCase(DauLutManager& lutm,
                                   uint32_t imagingCaseId,
                                   int imagingMode)

{
    uint32_t val;

    lutm.wakeupLuts();
    if (!lutm.loadImagingCaseLuts(imagingCaseId, imagingMode))
    {
        LOGE("DauTbfManager::setImagingCase(%d) failed", imagingCaseId);
        return(false);
    }
    lutm.sleepLuts();

    LOGI("DauTbfMgr::setImagingCase(%d)", imagingCaseId);
    return (true);
}

//-----------------------------------------------------------------------------
bool DauTbfManager::setColorImagingCase(DauLutManager& lutm,
                                        uint32_t imagingCaseId,
                                        ColorAcq* colorAcqPtr,
                                        uint32_t wallFilterType)

{
    uint32_t val;

    lutm.wakeupLuts();
    if (!lutm.loadColorImagingCaseLuts(imagingCaseId, colorAcqPtr, wallFilterType))
    {
        LOGE("DauTbfManager::setImagingCase(%d) failed", imagingCaseId);
        return(false);
    }
    lutm.sleepLuts();

    LOGI("DauTbfMgr::setImagingCase(%d)", imagingCaseId);
    return (true);
}

//-----------------------------------------------------------------------------
bool DauTbfManager::setColorImagingCaseLinear(DauLutManager& lutm,
                                        uint32_t imagingCaseId,
                                        ColorAcqLinear* colorAcqPtr,
                                        uint32_t wallFilterType)

{
    uint32_t val;

    lutm.wakeupLuts();
    if (!lutm.loadColorImagingCaseLinearLuts(imagingCaseId, colorAcqPtr, wallFilterType))
    {
        LOGE("DauTbfManager::setImagingCase(%d) failed", imagingCaseId);
        return(false);
    }
    lutm.sleepLuts();

    LOGI("DauTbfMgr::setImagingCase(%d)", imagingCaseId);
    return (true);
}


//-----------------------------------------------------------------------------
bool DauTbfManager::setPwImagingCase(DauLutManager& lutm, uint32_t imagingCaseId, DopplerAcq *dopplerAcqPtr)
{
    uint32_t val;

    lutm.wakeupLuts();
    if (!lutm.loadDopplerImagingCaseLuts(imagingCaseId, dopplerAcqPtr))
    {
        LOGE("DauTbfManager::setPwImagingCase(%d) failed", imagingCaseId);
        return(false);
    }
    lutm.sleepLuts();

    LOGI("DauTbfMgr::setPwImagingCase(%d)", imagingCaseId);
    return (true);
}

//-----------------------------------------------------------------------------
bool DauTbfManager::setPwImagingCaseLinear(DauLutManager& lutm, uint32_t imagingCaseId, DopplerAcqLinear *dopplerAcqPtr)
{
    uint32_t val;

    lutm.wakeupLuts();
    if (!lutm.loadDopplerImagingCaseLinearLuts(imagingCaseId, dopplerAcqPtr))
    {
        LOGE("DauTbfManager::setPwImagingCase(%d) failed", imagingCaseId);
        return(false);
    }
    lutm.sleepLuts();

    LOGI("DauTbfMgr::setPwImagingCase(%d)", imagingCaseId);
    return (true);
}

//-----------------------------------------------------------------------------
bool DauTbfManager::setCwImagingCase(DauLutManager& lutm, uint32_t imagingCaseId, CWAcq *cwAcqPtr)
{
    uint32_t val;

    lutm.wakeupLuts();
    if (!lutm.loadCwImagingCaseLuts(imagingCaseId, cwAcqPtr))
    {
        LOGE("DauTbfManager::setCwImagingCase(%d) failed", imagingCaseId);
        return(false);
    }
    lutm.sleepLuts();

    LOGI("DauTbfMgr::setCwImagingCase(%d)", imagingCaseId);
    return (true);
}

//-----------------------------------------------------------------------------
bool DauTbfManager::start()
{
    bool retVal = false;
    uint32_t val;
    uint32_t topRegs[TBF_NUM_TOP_REGS];

    mDaumSPtr->read( TBF_REG_TR0+TBF_BASE, topRegs, TBF_NUM_TOP_REGS);
    LOGI("============================================");
    LOGI("    TOP REGS before starting signal path    ");
    LOGI("============================================");
    for (int i = 0; i < TBF_NUM_TOP_REGS; i++)
    {
        LOGI("TOP[%2d] = 0x%08X", i, topRegs[i]);
    }
    LOGI("============================================");
    return (true);
}

//-----------------------------------------------------------------------------
bool DauTbfManager::stop()
{
    LOGI("TBF stopped");
    return (true);
}
 
