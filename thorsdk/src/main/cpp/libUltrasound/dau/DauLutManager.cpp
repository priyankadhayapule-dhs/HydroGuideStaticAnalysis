//
// Copyright 2017 EchoNous Inc.
//
//
#define LOG_TAG "DauLutManager"

#include <cstdint>
#include <DauLutManager.h>
#include <BitfieldMacros.h>
#include <TbfRegs.h>


DauLutManager::DauLutManager(const std::shared_ptr<DauMemory>& daum,
                             const std::shared_ptr<UpsReader>& upsReader) :
    mDaumSPtr(daum),
    mUpsReaderSPtr(upsReader)
{
    uint32_t val = 0x2D2D;

    mDaumSPtr->write( &val, TBF_DP17+TBF_BASE, 1 );  // set test points
    LOGD("*** DauLutManager - constructor");
}

DauLutManager::~DauLutManager()
{
    LOGD("*** DauLutManager - destructor");
}


bool DauLutManager::loadGlobalLuts()
{
    bool ret = false;
    uint32_t bufSize = 0;
    
    LOGI("Loading Global LUTs:");

    //-------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getElementLocXLut( mTransferBuffer );
    if (0 == bufSize)
    {
        LOGE("LoadGlobalBLuts: failed to get ElementLocXLut from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_ELOC_X+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_ELOC_X\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : ElementLocXLut", bufSize, TBF_LUT_ELOC_X);
    
    //-------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getElementLocYLut( mTransferBuffer );
    if (0 == bufSize)
    {
        LOGE("LoadGlobalBLuts: failed to get ElementLocYLut from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_ELOC_Y+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_ELOC_Y\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : ElementLocYLut", bufSize, TBF_LUT_ELOC_Y);
    
    //-------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getApodPosLut( mTransferBuffer );
    if (0 == bufSize)
    {
        LOGE("LoadGlobalBLuts: failed to get ApodPosLut from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_APODEL+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_APODEL\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : ApodizationPositionLut", bufSize, TBF_LUT_APODEL);

    //-------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getGainDbToLinearLut( mTransferBuffer );
    if (0 == bufSize)
    {
        LOGE("LoadGlobalBLuts: failed to get GainDbToLinearLut from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_MSDBL+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSDBL\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : GainDbToLinearLut", bufSize, TBF_LUT_MSDBL);

    //-------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getFractionalDelayLut( mTransferBuffer );
    if (0 == bufSize)
    {
        LOGE("LoadGlobalBLuts: failed to get FractionalDelayLut from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_MSRF+TBF_BASE, TBF_LUT_MSRF_SIZE))
    {
        LOGE("dau write failed for TBF_LUT_MSRF\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FractionalDelayLut", bufSize, TBF_LUT_MSRF);


    ret = true;
    goto ok_ret;
    
err_ret:
ok_ret:
    return (ret);
}

bool DauLutManager::loadImagingCaseLuts(const uint32_t imagingCaseId, int imagingMode)
{
    bool ret = false;
    uint32_t bufSize = 0;
    uint32_t val = 0;
    uint32_t baseConfigLength = 0;

    // Enable analog power. Required prior to TX chip config
    val = 0xF;
    mDaumSPtr->write( &val, TBF_REG_PS0 + TBF_BASE, 1);
    usleep(1000*10);

    LOGI("Loading LUTs for Imaging Case %d", imagingCaseId);
    
    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getApodInterpRateLut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get ApodizationInterpolationRateLUT from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_MSDIS+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSDIS\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : ApodizationInterpolationRateLut", bufSize, TBF_LUT_MSDIS);

    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getApodizationLut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get ApodizationLUT from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_MSAPW+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSAPW\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : ApodizationLut", bufSize, TBF_LUT_MSAPW);

    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getApodizationScaleLut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get ApodizationScaleLUT from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_MSB0APS+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSB0APS\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : ApodizationScaleLut", bufSize, TBF_LUT_MSB0APS);
    
    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getBandPassFilter1I(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get BandPassFilter1I from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_MSBPFI+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSBPFI\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : BandPassFilter1I", bufSize, TBF_LUT_MSBPFI);

    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getBandPassFilter1Q(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get BandPassFilter1Q from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_MSBPFQ+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSBPFQ\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : BandPassFilter1Q", bufSize, TBF_LUT_MSBPFQ);

    //--------------------------------------------------------------------
    // CompoundGainLut is not written to hardware unlike other LUTs.
    // It is saved here for use in DauGainManager.
    bufSize = mUpsReaderSPtr->getCoherentCompoundGainLut(imagingCaseId, (uint32_t *)mUserGainLut);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get CoherentCompoundGainLut from UPS\n");
        goto err_ret;
    }


    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getCompressionLut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get CompressionLut from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_MSCOMP+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSCOMP\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : CompressionLut", bufSize, TBF_LUT_MSCOMP);

    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getFocalCompensationB0XLut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get FocalCompensationB0XLut from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_B0FPX+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B0FPX\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB0XLut", bufSize, TBF_LUT_B0FPX);

    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getFilterBlendingLut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get FilterBlendingLut from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_MSFC+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSFC\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FilterBlendingLut", bufSize, TBF_LUT_MSFC);

    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getFocalCompensationB0YLut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get FocalCompensationB0YLut from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_B0FPY+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B0FPY\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB0YLut", bufSize, TBF_LUT_B0FPY);

    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getFocalCompensationB1XLut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get FocalCompensationB1XLut from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_B1FPX+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B1FPX\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB1XLut", bufSize, TBF_LUT_B1FPX);

    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getFocalCompensationB1YLut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get FocalCompensationB1YLut from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_B1FPY+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B1FPY\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB1YLut", bufSize, TBF_LUT_B1FPY);

    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getFocalCompensationB2XLut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get FocalCompensationB2XLut from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_B2FPX+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B2FPX\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB2XLut", bufSize, TBF_LUT_B2FPX);

    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getFocalCompensationB2YLut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get FocalCompensationB2YLut from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_B2FPY+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B2FPY\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB2YLut", bufSize, TBF_LUT_B2FPY);

    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getFocalCompensationB3XLut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get FocalCompensationB3XLut from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_B3FPX+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B3FPX\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB3XLut", bufSize, TBF_LUT_B3FPX);

    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getFocalCompensationB3YLut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get FocalCompensationB3YLut from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_B3FPY+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B3FPY\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB3YLut", bufSize, TBF_LUT_B3FPY);
    
    //--------------------------------------------------------------------
#if 0   // not sure if this needs to be loaded
    bufSize = mUpsReaderSPtr->getFocalInterpolationRateLut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get FocalInterpolationRateLut from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_MSDIS+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSDIS\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalInterpolationRateLut", bufSize, TBF_LUT_MSDIS);
#endif
    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getGainDgc01Lut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get GainDgc01Lut from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_MSDBG01+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSDBG01\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : GainDgc01Lut", bufSize, TBF_LUT_MSDBG01);

    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getGainDgc23Lut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get GainDgc01Lut from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_MSDBG23+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSDBG23\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : GainDgc23Lut", bufSize, TBF_LUT_MSDBG23);

    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getTxPropagationDelayB0Lut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get TxPropagationDelayB0Lut from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_B0FTXD+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B0FTXD\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : TxPropagationDelayB0Lut", bufSize, TBF_LUT_B0FTXD);

    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getTxPropagationDelayB1Lut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get TxPropagationDelayB1Lut from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_B1FTXD+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B1FTXD\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : TxPropagationDelayB1Lut", bufSize, TBF_LUT_B1FTXD);

    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getTxPropagationDelayB2Lut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get TxPropagationDelayB2Lut from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_B2FTXD+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B2FTXD\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : TxPropagationDelayB2Lut", bufSize, TBF_LUT_B2FTXD);

    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getTxPropagationDelayB3Lut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get TxPropagationDelayB3Lut from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_B3FTXD+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B3FTXD\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : TxPropagationDelayB3Lut", bufSize, TBF_LUT_B3FTXD);

    {
        //--------------------------------------------------------------------
        bufSize = mUpsReaderSPtr->getTxConfigLut(imagingCaseId, mTransferBuffer);
        if (0 == bufSize)
        {
            LOGE("LoadBLuts: failed to get TxConfigLut from UPS\n");
            goto err_ret;
        }
        wakeupLuts();
        // Reset TX chips by pulsing the TR_TX_SWRST_BIT in TBF_REG_TR0
        val = 0;
        BFN_SET(val, 1, TR_TX_SWRST);
        LOGI("writing 0x%08X to 0x%08X", val, TBF_REG_TR0);
        mDaumSPtr->write(&val, TBF_REG_TR0+TBF_BASE, 1);
        usleep(10);
        val = 0;
        mDaumSPtr->write(&val, TBF_REG_TR0+TBF_BASE, 1);

        val = 0x20000000;
        mDaumSPtr->write(&val, TBF_REG_EN0+TBF_BASE, 1);
        val = 0;
        mDaumSPtr->write(&val, TBF_REG_EN0+TBF_BASE, 1);

        mDaumSPtr->read(TBF_RO_TX+TBF_BASE, &val, 1);
        LOGI("TBF_R0_TX value after reset, before loading MSSTTXC is 0x%08X", val);

        if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_MSSTTXC+TBF_BASE, bufSize))
        {
            LOGE("dau write failed for TBF_LUT_MSSTTXC\n");
            goto err_ret;
        }
        LOGI("\t%5d words to 0x%08X : TxConfig", bufSize, TBF_LUT_MSSTTXC);
    }

    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getAfeInitLut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get AfeInit from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_MSAFEINIT+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSAFEINIT\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : AfeInit", bufSize, TBF_LUT_MSAFEINIT);

    startSpiWrite( mUpsReaderSPtr->getBaseConfigLength(imagingCaseId) );
    //--------------------------------------------------------------------
    //    COLOR LUTS
    //--------------------------------------------------------------------

    if (IMAGING_MODE_COLOR == imagingMode)
    {
        ret = loadColorLuts(imagingCaseId);
    }
    else
    {
        ret = true;
    }
    goto ok_ret;
    
err_ret:
ok_ret:
    return (ret);
}

bool DauLutManager::loadColorImagingCaseLuts(uint32_t imagingCaseId, ColorAcq* colorAcqPtr, uint32_t wallFilterType )
{
    bool ret = false;
    uint32_t bufSize = 0;
    uint32_t val = 0;
    uint32_t baseConfigLength = 0;

    uint32_t* lutPtr;

    // Enable analog power. Required prior to TX chip config.
    val = 0xF;
    mDaumSPtr->write( &val, TBF_REG_PS0 + TBF_BASE, 1);
    usleep(1000*10);

    LOGI("Loading LUTs for Color Imaging Case %d", imagingCaseId);

    //--------------------------------------------------------------------
    bufSize = TBF_LUT_MSDIS_SIZE;
    lutPtr = (uint32_t *)(colorAcqPtr->getLutMSDIS());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_MSDIS+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSDIS\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : ApodizationInterpolationRateLut", bufSize, TBF_LUT_MSDIS);
    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getApodizationLut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get ApodizationLUT from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_MSAPW+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSAPW\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : ApodizationLut", bufSize, TBF_LUT_MSAPW);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_MSB0APS_SIZE;
    lutPtr = (uint32_t *)(colorAcqPtr->getLutMSB0APS());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_MSB0APS+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSB0APS\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : ApodizationScaleLut", bufSize, TBF_LUT_MSB0APS);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_MSBPFI_SIZE;
    lutPtr = colorAcqPtr->getLutMSBPFI();
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_MSBPFI+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSBPFI\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : BandPassFilter1I", bufSize, TBF_LUT_MSBPFI);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_MSBPFQ_SIZE;
    lutPtr = colorAcqPtr->getLutMSBPFQ();
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_MSBPFQ+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSBPFQ\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : BandPassFilter1Q", bufSize, TBF_LUT_MSBPFQ);
    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getCoherentCompoundGainLut(imagingCaseId, (uint32_t *)mUserGainLut);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get CoherentCompoundGainLut from UPS\n");
        goto err_ret;
    }

    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getCompressionLut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get CompressionLut from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_MSCOMP+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSCOMP\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : CompressionLut", bufSize, TBF_LUT_MSCOMP);

    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getFilterBlendingLut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get FilterBlendingLut from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_MSFC+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSFC\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FilterBlendingLut", bufSize, TBF_LUT_MSFC);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B0FPX_SIZE;
    lutPtr = (uint32_t *)(colorAcqPtr->getLutB0FPX());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B0FPX+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B0FPX\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB0XLut", bufSize, TBF_LUT_B0FPX);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B0FPY_SIZE;
    lutPtr = (uint32_t *)(colorAcqPtr->getLutB0FPY());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B0FPY+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B0FPY\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB0YLut", bufSize, TBF_LUT_B0FPY);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B1FPX_SIZE;
    lutPtr = (uint32_t *)(colorAcqPtr->getLutB1FPX());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B1FPX+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B1FPX\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB1XLut", bufSize, TBF_LUT_B1FPX);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B1FPY_SIZE;
    lutPtr = (uint32_t *)(colorAcqPtr->getLutB1FPY());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B1FPY+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B1FPY\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB1YLut", bufSize, TBF_LUT_B1FPY);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B2FPX_SIZE;
    lutPtr = (uint32_t *)(colorAcqPtr->getLutB2FPX());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B2FPX+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B2FPX\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB2XLut", bufSize, TBF_LUT_B2FPX);

    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B2FPY_SIZE;
    lutPtr = (uint32_t *)(colorAcqPtr->getLutB2FPY());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B2FPY+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B2FPY\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB2YLut", bufSize, TBF_LUT_B2FPY);

    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B3FPX_SIZE;
    lutPtr = (uint32_t *)(colorAcqPtr->getLutB3FPX());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B3FPX+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B3FPX\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB3XLut", bufSize, TBF_LUT_B3FPX);

    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B3FPY_SIZE;
    lutPtr = (uint32_t *)(colorAcqPtr->getLutB3FPY());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B3FPY+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B3FPY\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB3YLut", bufSize, TBF_LUT_B3FPY);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_MSDBG01_SIZE;
    lutPtr = colorAcqPtr->getLutMSDBG01();
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_MSDBG01+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSDBG01\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : GainDgc01Lut", bufSize, TBF_LUT_MSDBG01);

    //--------------------------------------------------------------------
    bufSize = TBF_LUT_MSDBG23_SIZE;
    lutPtr = colorAcqPtr->getLutMSDBG23();
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_MSDBG23+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSDBG23\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : GainDgc23Lut", bufSize, TBF_LUT_MSDBG23);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B0FTXD_SIZE;
    lutPtr = colorAcqPtr->getLutB0FTXD();
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B0FTXD+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B0FTXD\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : TxPropagationDelayB0Lut", bufSize, TBF_LUT_B0FTXD);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B1FTXD_SIZE;
    lutPtr = colorAcqPtr->getLutB1FTXD();
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B1FTXD+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B1FTXD\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : TxPropagationDelayB1Lut", bufSize, TBF_LUT_B1FTXD);

    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B2FTXD_SIZE;
    lutPtr = colorAcqPtr->getLutB2FTXD();
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B2FTXD+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B2FTXD\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : TxPropagationDelayB2Lut", bufSize, TBF_LUT_B2FTXD);

    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B3FTXD_SIZE;
    lutPtr = colorAcqPtr->getLutB3FTXD();
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B3FTXD+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B3FTXD\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : TxPropagationDelayB3Lut", bufSize, TBF_LUT_B3FTXD);

    //--------------------------------------------------------------------
    {
        wakeupLuts();
        lutPtr = colorAcqPtr->getLutMSSTTXC();
        bufSize = colorAcqPtr->getMSSTTXClength();

        // Reset TX chips by pulsing the TR_TX_SWRST_BIT in TBF_REG_TR0
        val = 0;
        BFN_SET(val, 1, TR_TX_SWRST);
        LOGI("writing 0x%08X to 0x%08X", val, TBF_REG_TR0);
        mDaumSPtr->write(&val, TBF_REG_TR0+TBF_BASE, 1);
        usleep(10);
        val = 0;
        mDaumSPtr->write(&val, TBF_REG_TR0+TBF_BASE, 1);

        val = 0x20000000;
        mDaumSPtr->write(&val, TBF_REG_EN0+TBF_BASE, 1);
        val = 0;
        mDaumSPtr->write(&val, TBF_REG_EN0+TBF_BASE, 1);

        mDaumSPtr->read(TBF_RO_TX+TBF_BASE, &val, 1);
        LOGI("TBF_R0_TX value after reset, before loading MSSTTXC is 0x%08X", val);

        if (!mDaumSPtr->write(lutPtr, TBF_LUT_MSSTTXC+TBF_BASE, bufSize))
        {
            LOGE("dau write failed for TBF_LUT_MSSTTXC\n");
            goto err_ret;
        }
        LOGI("\t%5d words to 0x%08X : TxConfig", bufSize, TBF_LUT_MSSTTXC);
        LOGI("\tBase Config Length used for color is %d", colorAcqPtr->getBaseConfigLength());
            
        startSpiWrite( colorAcqPtr->getBaseConfigLength() );
    }

    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getAfeInitLut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get AfeInit from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_MSAFEINIT+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSAFEINIT\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : AfeInit", bufSize, TBF_LUT_MSAFEINIT);

    ret = loadColorLuts(imagingCaseId, wallFilterType);
    goto ok_ret;
    
err_ret:
ok_ret:
    return (ret);
}

bool DauLutManager::loadColorImagingCaseLinearLuts(uint32_t imagingCaseId, ColorAcqLinear* colorAcqPtr, uint32_t wallFilterType )
{
    bool ret = false;
    uint32_t bufSize = 0;
    uint32_t val = 0;
    uint32_t baseConfigLength = 0;

    uint32_t* lutPtr;

    // Enable analog power. Required prior to TX chip config.
    val = 0xF;
    mDaumSPtr->write( &val, TBF_REG_PS0 + TBF_BASE, 1);
    usleep(1000*10);

    LOGI("Loading LUTs for Color Imaging Case %d", imagingCaseId);

    //--------------------------------------------------------------------
    bufSize = TBF_LUT_MSDIS_SIZE;
    lutPtr = (uint32_t *)(colorAcqPtr->getLutMSDIS());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_MSDIS+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSDIS\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : ApodizationInterpolationRateLut", bufSize, TBF_LUT_MSDIS);
    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getApodizationLut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get ApodizationLUT from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_MSAPW+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSAPW\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : ApodizationLut", bufSize, TBF_LUT_MSAPW);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_MSB0APS_SIZE;
    lutPtr = (uint32_t *)(colorAcqPtr->getLutMSB0APS());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_MSB0APS+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSB0APS\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : ApodizationScaleLut", bufSize, TBF_LUT_MSB0APS);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_MSBPFI_SIZE;
    lutPtr = colorAcqPtr->getLutMSBPFI();
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_MSBPFI+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSBPFI\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : BandPassFilter1I", bufSize, TBF_LUT_MSBPFI);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_MSBPFQ_SIZE;
    lutPtr = colorAcqPtr->getLutMSBPFQ();
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_MSBPFQ+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSBPFQ\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : BandPassFilter1Q", bufSize, TBF_LUT_MSBPFQ);
    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getCoherentCompoundGainLut(imagingCaseId, (uint32_t *)mUserGainLut);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get CoherentCompoundGainLut from UPS\n");
        goto err_ret;
    }

    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getCompressionLut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get CompressionLut from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_MSCOMP+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSCOMP\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : CompressionLut", bufSize, TBF_LUT_MSCOMP);

    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getFilterBlendingLut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get FilterBlendingLut from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_MSFC+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSFC\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FilterBlendingLut", bufSize, TBF_LUT_MSFC);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B0FPX_SIZE;
    lutPtr = (uint32_t *)(colorAcqPtr->getLutB0FPX());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B0FPX+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B0FPX\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB0XLut", bufSize, TBF_LUT_B0FPX);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B0FPY_SIZE;
    lutPtr = (uint32_t *)(colorAcqPtr->getLutB0FPY());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B0FPY+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B0FPY\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB0YLut", bufSize, TBF_LUT_B0FPY);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B1FPX_SIZE;
    lutPtr = (uint32_t *)(colorAcqPtr->getLutB1FPX());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B1FPX+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B1FPX\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB1XLut", bufSize, TBF_LUT_B1FPX);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B1FPY_SIZE;
    lutPtr = (uint32_t *)(colorAcqPtr->getLutB1FPY());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B1FPY+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B1FPY\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB1YLut", bufSize, TBF_LUT_B1FPY);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B2FPX_SIZE;
    lutPtr = (uint32_t *)(colorAcqPtr->getLutB2FPX());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B2FPX+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B2FPX\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB2XLut", bufSize, TBF_LUT_B2FPX);

    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B2FPY_SIZE;
    lutPtr = (uint32_t *)(colorAcqPtr->getLutB2FPY());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B2FPY+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B2FPY\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB2YLut", bufSize, TBF_LUT_B2FPY);

    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B3FPX_SIZE;
    lutPtr = (uint32_t *)(colorAcqPtr->getLutB3FPX());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B3FPX+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B3FPX\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB3XLut", bufSize, TBF_LUT_B3FPX);

    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B3FPY_SIZE;
    lutPtr = (uint32_t *)(colorAcqPtr->getLutB3FPY());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B3FPY+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B3FPY\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB3YLut", bufSize, TBF_LUT_B3FPY);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_MSDBG01_SIZE;
    lutPtr = colorAcqPtr->getLutMSDBG01();
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_MSDBG01+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSDBG01\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : GainDgc01Lut", bufSize, TBF_LUT_MSDBG01);

    //--------------------------------------------------------------------
    bufSize = TBF_LUT_MSDBG23_SIZE;
    lutPtr = colorAcqPtr->getLutMSDBG23();
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_MSDBG23+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSDBG23\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : GainDgc23Lut", bufSize, TBF_LUT_MSDBG23);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B0FTXD_SIZE;
    lutPtr = colorAcqPtr->getLutB0FTXD();
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B0FTXD+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B0FTXD\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : TxPropagationDelayB0Lut", bufSize, TBF_LUT_B0FTXD);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B1FTXD_SIZE;
    lutPtr = colorAcqPtr->getLutB1FTXD();
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B1FTXD+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B1FTXD\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : TxPropagationDelayB1Lut", bufSize, TBF_LUT_B1FTXD);

    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B2FTXD_SIZE;
    lutPtr = colorAcqPtr->getLutB2FTXD();
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B2FTXD+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B2FTXD\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : TxPropagationDelayB2Lut", bufSize, TBF_LUT_B2FTXD);

    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B3FTXD_SIZE;
    lutPtr = colorAcqPtr->getLutB3FTXD();
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B3FTXD+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B3FTXD\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : TxPropagationDelayB3Lut", bufSize, TBF_LUT_B3FTXD);

    //--------------------------------------------------------------------
    {
        wakeupLuts();
        lutPtr = colorAcqPtr->getLutMSSTTXC();
        bufSize = colorAcqPtr->getMSSTTXClength();

        // Reset TX chips by pulsing the TR_TX_SWRST_BIT in TBF_REG_TR0
        val = 0;
        BFN_SET(val, 1, TR_TX_SWRST);
        LOGI("writing 0x%08X to 0x%08X", val, TBF_REG_TR0);
        mDaumSPtr->write(&val, TBF_REG_TR0+TBF_BASE, 1);
        usleep(10);
        val = 0;
        mDaumSPtr->write(&val, TBF_REG_TR0+TBF_BASE, 1);

        val = 0x20000000;
        mDaumSPtr->write(&val, TBF_REG_EN0+TBF_BASE, 1);
        val = 0;
        mDaumSPtr->write(&val, TBF_REG_EN0+TBF_BASE, 1);

        mDaumSPtr->read(TBF_RO_TX+TBF_BASE, &val, 1);
        LOGI("TBF_R0_TX value after reset, before loading MSSTTXC is 0x%08X", val);

        if (!mDaumSPtr->write(lutPtr, TBF_LUT_MSSTTXC+TBF_BASE, bufSize))
        {
            LOGE("dau write failed for TBF_LUT_MSSTTXC\n");
            goto err_ret;
        }
        LOGI("\t%5d words to 0x%08X : TxConfig", bufSize, TBF_LUT_MSSTTXC);
        LOGI("\tBase Config Length used for color is %d", colorAcqPtr->getBaseConfigLength());

        startSpiWrite( colorAcqPtr->getBaseConfigLength() );
    }

    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getAfeInitLut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get AfeInit from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_MSAFEINIT+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSAFEINIT\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : AfeInit", bufSize, TBF_LUT_MSAFEINIT);

    ret = loadColorLuts(imagingCaseId, wallFilterType);
    goto ok_ret;

    err_ret:
    ok_ret:
    return (ret);
}

bool DauLutManager::loadDopplerImagingCaseLuts(uint32_t imagingCaseId, DopplerAcq* dopplerAcqPtr )
{
    bool ret = false;
    uint32_t bufSize = 0;
    uint32_t val = 0;
    uint32_t baseConfigLength = 0;

    uint32_t* lutPtr;
    // Enable analog power. Required prior to TX chip config.
    val = 0xF;
    mDaumSPtr->write( &val, TBF_REG_PS0 + TBF_BASE, 1);
    usleep(1000*10);

    LOGI("Loading LUTs for PW Doppler Imaging Case %d", imagingCaseId);

    //--------------------------------------------------------------------
    bufSize = TBF_LUT_MSDIS_SIZE;
    lutPtr = (uint32_t *)(dopplerAcqPtr->getLutMSDIS());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_MSDIS+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSDIS\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : ApodizationInterpolationRateLut", bufSize, TBF_LUT_MSDIS);
    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getApodizationLut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get ApodizationLUT from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_MSAPW+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSAPW\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : ApodizationLut", bufSize, TBF_LUT_MSAPW);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_MSB0APS_SIZE;
    lutPtr = (uint32_t *)(dopplerAcqPtr->getLutMSB0APS());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_MSB0APS+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSB0APS\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : ApodizationScaleLut", bufSize, TBF_LUT_MSB0APS);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_MSBPFI_SIZE;
    lutPtr = dopplerAcqPtr->getLutMSBPFI();
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_MSBPFI+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSBPFI\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : BandPassFilter1I", bufSize, TBF_LUT_MSBPFI);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_MSBPFQ_SIZE;
    lutPtr = dopplerAcqPtr->getLutMSBPFQ();
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_MSBPFQ+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSBPFQ\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : BandPassFilter1Q", bufSize, TBF_LUT_MSBPFQ);
    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getCoherentCompoundGainLut(imagingCaseId, (uint32_t *)mUserGainLut);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get CoherentCompoundGainLut from UPS\n");
        goto err_ret;
    }

    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getCompressionLut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get CompressionLut from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_MSCOMP+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSCOMP\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : CompressionLut", bufSize, TBF_LUT_MSCOMP);

    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getFilterBlendingLut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get FilterBlendingLut from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_MSFC+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSFC\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FilterBlendingLut", bufSize, TBF_LUT_MSFC);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B0FPX_SIZE;
    lutPtr = (uint32_t *)(dopplerAcqPtr->getLutB0FPX());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B0FPX+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B0FPX\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB0XLut", bufSize, TBF_LUT_B0FPX);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B0FPY_SIZE;
    lutPtr = (uint32_t *)(dopplerAcqPtr->getLutB0FPY());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B0FPY+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B0FPY\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB0YLut", bufSize, TBF_LUT_B0FPY);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B1FPX_SIZE;
    lutPtr = (uint32_t *)(dopplerAcqPtr->getLutB1FPX());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B1FPX+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B1FPX\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB1XLut", bufSize, TBF_LUT_B1FPX);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B1FPY_SIZE;
    lutPtr = (uint32_t *)(dopplerAcqPtr->getLutB1FPY());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B1FPY+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B1FPY\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB1YLut", bufSize, TBF_LUT_B1FPY);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B2FPX_SIZE;
    lutPtr = (uint32_t *)(dopplerAcqPtr->getLutB2FPX());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B2FPX+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B2FPX\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB2XLut", bufSize, TBF_LUT_B2FPX);

    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B2FPY_SIZE;
    lutPtr = (uint32_t *)(dopplerAcqPtr->getLutB2FPY());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B2FPY+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B2FPY\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB2YLut", bufSize, TBF_LUT_B2FPY);

    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B3FPX_SIZE;
    lutPtr = (uint32_t *)(dopplerAcqPtr->getLutB3FPX());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B3FPX+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B3FPX\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB3XLut", bufSize, TBF_LUT_B3FPX);

    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B3FPY_SIZE;
    lutPtr = (uint32_t *)(dopplerAcqPtr->getLutB3FPY());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B3FPY+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B3FPY\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB3YLut", bufSize, TBF_LUT_B3FPY);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_MSDBG01_SIZE;
    lutPtr = dopplerAcqPtr->getLutMSDBG01();
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_MSDBG01+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSDBG01\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : GainDgc01Lut", bufSize, TBF_LUT_MSDBG01);

    //--------------------------------------------------------------------
    bufSize = TBF_LUT_MSDBG23_SIZE;
    lutPtr = dopplerAcqPtr->getLutMSDBG23();
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_MSDBG23+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSDBG23\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : GainDgc23Lut", bufSize, TBF_LUT_MSDBG23);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B0FTXD_SIZE;
    lutPtr = dopplerAcqPtr->getLutB0FTXD();
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B0FTXD+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B0FTXD\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : TxPropagationDelayB0Lut", bufSize, TBF_LUT_B0FTXD);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B1FTXD_SIZE;
    lutPtr = dopplerAcqPtr->getLutB1FTXD();
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B1FTXD+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B1FTXD\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : TxPropagationDelayB1Lut", bufSize, TBF_LUT_B1FTXD);

    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B2FTXD_SIZE;
    lutPtr = dopplerAcqPtr->getLutB2FTXD();
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B2FTXD+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B2FTXD\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : TxPropagationDelayB2Lut", bufSize, TBF_LUT_B2FTXD);

    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B3FTXD_SIZE;
    lutPtr = dopplerAcqPtr->getLutB3FTXD();
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B3FTXD+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B3FTXD\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : TxPropagationDelayB3Lut", bufSize, TBF_LUT_B3FTXD);

    //--------------------------------------------------------------------
    {
        wakeupLuts();
        lutPtr = dopplerAcqPtr->getLutMSSTTXC();
        bufSize = dopplerAcqPtr->getMSSTTXClength();

        // Reset TX chips by pulsing the TR_TX_SWRST_BIT in TBF_REG_TR0
        val = 0;
        BFN_SET(val, 1, TR_TX_SWRST);
        LOGI("writing 0x%08X to 0x%08X", val, TBF_REG_TR0);
        mDaumSPtr->write(&val, TBF_REG_TR0+TBF_BASE, 1);
        usleep(10);
        val = 0;
        mDaumSPtr->write(&val, TBF_REG_TR0+TBF_BASE, 1);

        val = 0x20000000;
        mDaumSPtr->write(&val, TBF_REG_EN0+TBF_BASE, 1);
        val = 0;
        mDaumSPtr->write(&val, TBF_REG_EN0+TBF_BASE, 1);

        mDaumSPtr->read(TBF_RO_TX+TBF_BASE, &val, 1);
        LOGI("TBF_R0_TX value after reset, before loading MSSTTXC is 0x%08X", val);

        if (!mDaumSPtr->write(lutPtr, TBF_LUT_MSSTTXC+TBF_BASE, bufSize))
        {
            LOGE("dau write failed for TBF_LUT_MSSTTXC\n");
            goto err_ret;
        }
        LOGI("\t%5d words to 0x%08X : TxConfig", bufSize, TBF_LUT_MSSTTXC);
        LOGI("\tBase Config Length used for color is %d", dopplerAcqPtr->getBaseConfigLength());

        startSpiWrite( dopplerAcqPtr->getBaseConfigLength() );
    }

    //--------------------------------------------------------------------
    lutPtr = dopplerAcqPtr->getLutMSAFEINIT();
    bufSize = TBF_LUT_MSAFEINIT_SIZE;

    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get AfeInit from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_MSAFEINIT+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSAFEINIT\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : AfeInit", bufSize, TBF_LUT_MSAFEINIT);

    ret = true;

err_ret:
ok_ret:
    return (ret);
}

bool DauLutManager::loadDopplerImagingCaseLinearLuts(uint32_t imagingCaseId, DopplerAcqLinear* dopplerAcqPtr )
{
    bool ret = false;
    uint32_t bufSize = 0;
    uint32_t val = 0;
    uint32_t baseConfigLength = 0;

    uint32_t* lutPtr;
    // Enable analog power. Required prior to TX chip config.
    val = 0xF;
    mDaumSPtr->write( &val, TBF_REG_PS0 + TBF_BASE, 1);
    usleep(1000*10);

    LOGI("Loading LUTs for PW Doppler Imaging Case %d", imagingCaseId);

    //--------------------------------------------------------------------
    bufSize = TBF_LUT_MSDIS_SIZE;
    lutPtr = (uint32_t *)(dopplerAcqPtr->getLutMSDIS());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_MSDIS+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSDIS\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : ApodizationInterpolationRateLut", bufSize, TBF_LUT_MSDIS);
    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getApodizationLut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get ApodizationLUT from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_MSAPW+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSAPW\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : ApodizationLut", bufSize, TBF_LUT_MSAPW);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_MSB0APS_SIZE;
    lutPtr = (uint32_t *)(dopplerAcqPtr->getLutMSB0APS());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_MSB0APS+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSB0APS\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : ApodizationScaleLut", bufSize, TBF_LUT_MSB0APS);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_MSBPFI_SIZE;
    lutPtr = dopplerAcqPtr->getLutMSBPFI();
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_MSBPFI+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSBPFI\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : BandPassFilter1I", bufSize, TBF_LUT_MSBPFI);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_MSBPFQ_SIZE;
    lutPtr = dopplerAcqPtr->getLutMSBPFQ();
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_MSBPFQ+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSBPFQ\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : BandPassFilter1Q", bufSize, TBF_LUT_MSBPFQ);
    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getCoherentCompoundGainLut(imagingCaseId, (uint32_t *)mUserGainLut);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get CoherentCompoundGainLut from UPS\n");
        goto err_ret;
    }

    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getCompressionLut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get CompressionLut from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_MSCOMP+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSCOMP\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : CompressionLut", bufSize, TBF_LUT_MSCOMP);

    //--------------------------------------------------------------------
    bufSize = mUpsReaderSPtr->getFilterBlendingLut(imagingCaseId, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get FilterBlendingLut from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_MSFC+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSFC\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FilterBlendingLut", bufSize, TBF_LUT_MSFC);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B0FPX_SIZE;
    lutPtr = (uint32_t *)(dopplerAcqPtr->getLutB0FPX());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B0FPX+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B0FPX\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB0XLut", bufSize, TBF_LUT_B0FPX);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B0FPY_SIZE;
    lutPtr = (uint32_t *)(dopplerAcqPtr->getLutB0FPY());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B0FPY+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B0FPY\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB0YLut", bufSize, TBF_LUT_B0FPY);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B1FPX_SIZE;
    lutPtr = (uint32_t *)(dopplerAcqPtr->getLutB1FPX());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B1FPX+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B1FPX\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB1XLut", bufSize, TBF_LUT_B1FPX);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B1FPY_SIZE;
    lutPtr = (uint32_t *)(dopplerAcqPtr->getLutB1FPY());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B1FPY+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B1FPY\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB1YLut", bufSize, TBF_LUT_B1FPY);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B2FPX_SIZE;
    lutPtr = (uint32_t *)(dopplerAcqPtr->getLutB2FPX());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B2FPX+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B2FPX\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB2XLut", bufSize, TBF_LUT_B2FPX);

    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B2FPY_SIZE;
    lutPtr = (uint32_t *)(dopplerAcqPtr->getLutB2FPY());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B2FPY+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B2FPY\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB2YLut", bufSize, TBF_LUT_B2FPY);

    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B3FPX_SIZE;
    lutPtr = (uint32_t *)(dopplerAcqPtr->getLutB3FPX());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B3FPX+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B3FPX\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB3XLut", bufSize, TBF_LUT_B3FPX);

    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B3FPY_SIZE;
    lutPtr = (uint32_t *)(dopplerAcqPtr->getLutB3FPY());
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B3FPY+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B3FPY\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : FocalCompensationB3YLut", bufSize, TBF_LUT_B3FPY);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_MSDBG01_SIZE;
    lutPtr = dopplerAcqPtr->getLutMSDBG01();
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_MSDBG01+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSDBG01\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : GainDgc01Lut", bufSize, TBF_LUT_MSDBG01);

    //--------------------------------------------------------------------
    bufSize = TBF_LUT_MSDBG23_SIZE;
    lutPtr = dopplerAcqPtr->getLutMSDBG23();
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_MSDBG23+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSDBG23\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : GainDgc23Lut", bufSize, TBF_LUT_MSDBG23);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B0FTXD_SIZE;
    lutPtr = dopplerAcqPtr->getLutB0FTXD();
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B0FTXD+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B0FTXD\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : TxPropagationDelayB0Lut", bufSize, TBF_LUT_B0FTXD);
    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B1FTXD_SIZE;
    lutPtr = dopplerAcqPtr->getLutB1FTXD();
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B1FTXD+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B1FTXD\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : TxPropagationDelayB1Lut", bufSize, TBF_LUT_B1FTXD);

    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B2FTXD_SIZE;
    lutPtr = dopplerAcqPtr->getLutB2FTXD();
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B2FTXD+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B2FTXD\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : TxPropagationDelayB2Lut", bufSize, TBF_LUT_B2FTXD);

    //--------------------------------------------------------------------
    bufSize = TBF_LUT_B3FTXD_SIZE;
    lutPtr = dopplerAcqPtr->getLutB3FTXD();
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_B3FTXD+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_B3FTXD\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : TxPropagationDelayB3Lut", bufSize, TBF_LUT_B3FTXD);

    //--------------------------------------------------------------------
    {
        wakeupLuts();
        lutPtr = dopplerAcqPtr->getLutMSSTTXC();
        bufSize = dopplerAcqPtr->getMSSTTXClength();

        // Reset TX chips by pulsing the TR_TX_SWRST_BIT in TBF_REG_TR0
        val = 0;
        BFN_SET(val, 1, TR_TX_SWRST);
        LOGI("writing 0x%08X to 0x%08X", val, TBF_REG_TR0);
        mDaumSPtr->write(&val, TBF_REG_TR0+TBF_BASE, 1);
        usleep(10);
        val = 0;
        mDaumSPtr->write(&val, TBF_REG_TR0+TBF_BASE, 1);

        val = 0x20000000;
        mDaumSPtr->write(&val, TBF_REG_EN0+TBF_BASE, 1);
        val = 0;
        mDaumSPtr->write(&val, TBF_REG_EN0+TBF_BASE, 1);

        mDaumSPtr->read(TBF_RO_TX+TBF_BASE, &val, 1);
        LOGI("TBF_R0_TX value after reset, before loading MSSTTXC is 0x%08X", val);

        if (!mDaumSPtr->write(lutPtr, TBF_LUT_MSSTTXC+TBF_BASE, bufSize))
        {
            LOGE("dau write failed for TBF_LUT_MSSTTXC\n");
            goto err_ret;
        }
        LOGI("\t%5d words to 0x%08X : TxConfig", bufSize, TBF_LUT_MSSTTXC);
        LOGI("\tBase Config Length used for color is %d", dopplerAcqPtr->getBaseConfigLength());

        startSpiWrite( dopplerAcqPtr->getBaseConfigLength() );
    }

    //--------------------------------------------------------------------
    lutPtr = dopplerAcqPtr->getLutMSAFEINIT();
    bufSize = TBF_LUT_MSAFEINIT_SIZE;

    if (0 == bufSize)
    {
        LOGE("LoadBLuts: failed to get AfeInit from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(lutPtr, TBF_LUT_MSAFEINIT+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSAFEINIT\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : AfeInit", bufSize, TBF_LUT_MSAFEINIT);

    ret = true;

    err_ret:
    ok_ret:
    return (ret);
}

bool DauLutManager::loadCwImagingCaseLuts(uint32_t imagingCaseId, CWAcq* cwAcqPtr )
{
    bool ret = false;
    uint32_t bufSize = 0;
    uint32_t val = 0;
    uint32_t baseConfigLength = 0;

    uint32_t* lutPtr;

    // Enable analog power. Required prior to TX chip config.
    val = 0xF;
    mDaumSPtr->write( &val, TBF_REG_PS0 + TBF_BASE, 1);
    usleep(1000*10);

    LOGI("Loading LUTs for CW Doppler Imaging Case %d", imagingCaseId);


    //--------------------------------------------------------------------
    {
        wakeupLuts();
        lutPtr = cwAcqPtr->getLutMSSTTXC();
        bufSize = cwAcqPtr->getMSSTTXClength();

        // Reset TX chips by pulsing the TR_TX_SWRST_BIT in TBF_REG_TR0
        val = 0;
        BFN_SET(val, 1, TR_TX_SWRST);
        LOGI("writing 0x%08X to 0x%08X", val, TBF_REG_TR0);
        mDaumSPtr->write(&val, TBF_REG_TR0+TBF_BASE, 1);
        usleep(10);
        val = 0;
        mDaumSPtr->write(&val, TBF_REG_TR0+TBF_BASE, 1);

        val = 0x20000000;
        mDaumSPtr->write(&val, TBF_REG_EN0+TBF_BASE, 1);
        val = 0;
        mDaumSPtr->write(&val, TBF_REG_EN0+TBF_BASE, 1);

        mDaumSPtr->read(TBF_RO_TX+TBF_BASE, &val, 1);
        LOGI("TBF_R0_TX value after reset, before loading MSSTTXC is 0x%08X", val);

        if (!mDaumSPtr->write(lutPtr, TBF_LUT_MSSTTXC+TBF_BASE, bufSize))
        {
            LOGE("dau write failed for TBF_LUT_MSSTTXC\n");
            goto err_ret;
        }
        LOGI("\t%5d words to 0x%08X : TxConfig", bufSize, TBF_LUT_MSSTTXC);
        LOGI("\tBase Config Length used for color is %d", cwAcqPtr->getBaseConfigLength());

        startSpiWrite( cwAcqPtr->getBaseConfigLength() );
    }

    ret = true;

err_ret:
ok_ret:
    return (ret);
}

bool DauLutManager::loadColorLuts(const uint32_t imagingCaseId, uint32_t wallFilterType)
{
    bool        ret = false;
    uint32_t    bufSize = 0;

    LOGI("Loading LUTs for Imaging Case %d", imagingCaseId);

    bufSize = mUpsReaderSPtr->getCFWallFilter(imagingCaseId, wallFilterType, mTransferBuffer);
    if (0 == bufSize)
    {
        LOGE("LoadCFLuts: failed to get WallFilter from UPS\n");
        goto err_ret;
    }
    if (!mDaumSPtr->write(mTransferBuffer, TBF_LUT_MSWFC+TBF_BASE, bufSize))
    {
        LOGE("dau write failed for TBF_LUT_MSWFC\n");
        goto err_ret;
    }
    LOGI("\t%5d words to 0x%08X : WallFilterCoefficientsLut", bufSize, TBF_LUT_MSWFC);
    
    ret = true;
   
err_ret:
ok_ret:
    return (ret);

}


void DauLutManager::wakeupLuts()
{
    uint32_t val;

    mDaumSPtr->read(TBF_REG_CK0+TBF_BASE, &val, 1);
    LOGI("CK0 read from TBF is 0x%08X", val);

    // Set wakeup enable bit
    val = 0x100;
    LOGI("Value written to CK0 (0x%08X) in wakeupLuts is 0x%08X",
         TBF_REG_CK0, val);
    if (!mDaumSPtr->write( &val, TBF_REG_CK0+TBF_BASE, 1 ))
    {
        LOGE("dau.write failed for wakeupLuts()\n");
        return;
    }
}

void DauLutManager::sleepLuts()
{
#if 0  // temporarily commented out during V2 design integration
    uint32_t val = mCK0;

    LOGI("Value written to CK0 (0x%08X) in sleepLuts is 0x%08X",
         TBF_REG_CK0, val);
    if (!mDaumSPtr->write( &val, TBF_REG_CK0+TBF_BASE, 1 ))
    {
        LOGE("dau.write failed for wakeupLuts()\n");
        return;
    }
#endif
}


void DauLutManager::TXchipLP()
{
    uint32_t val;
    // BF out of reset
    val = 0x0;
    mDaumSPtr->write(&val, TBF_REG_TR0 + TBF_BASE,1);
    // Enable TX clock
    val = 0x100;
    mDaumSPtr->write(&val, TBF_REG_CK0 + TBF_BASE,1);
    // Configure 2 word burst write
    val = 0x00020000;
    mDaumSPtr->write(&val, TBF_REG_HVSPI0 + TBF_BASE,1);
    // TX register 0x1008 value
    uint32_t lpval = 0x00400402;
    mDaumSPtr->write(&lpval, TBF_LUT_MSSTTXC + 0x4 + TBF_BASE,1);
    // Note that we write to 8 parts for compatibility with linear
    uint32_t spiheader = 0x00201008;
    for (int tc = 0; tc<8; tc++) {
        val = spiheader + (tc << 16);
        // Complete header word with die select
        mDaumSPtr->write(&val, TBF_LUT_MSSTTXC + TBF_BASE,1);
        val = 0x00000000;
        mDaumSPtr->write(&val, TBF_TRIG_HVSPI + TBF_BASE,1);
        usleep(100); // Wait for completion of SPI writes
    }
    // Enable TX clock
    val = 0xF;
    mDaumSPtr->write(&val, TBF_REG_CK0 + TBF_BASE,1);
    LOGD("TXCHIP low power state activated");
}



void DauLutManager::TXchipON()
{
    uint32_t val;
    // BF out of reset
    val = 0x0;
    mDaumSPtr->write(&val, TBF_REG_TR0 + TBF_BASE,1);
    // Enable TX clock
    val = 0x100;
    mDaumSPtr->write(&val, TBF_REG_CK0 + TBF_BASE,1);
    // TX trigger pulse to bring chips out of low power state
    // Toggle bit 29 "hv_tx_trig_pulse"
    val = 0x20000000;
    mDaumSPtr->write(&val, TBF_REG_EN0 + TBF_BASE,1);
    val = 0x00000000;
    mDaumSPtr->write(&val, TBF_REG_EN0 + TBF_BASE,1);
    LOGD("TXCHIP out of low power state");
}


void DauLutManager::startSpiWrite(uint32_t wordCount)
{
    uint32_t val;

    val = 0x3c3c;
    mDaumSPtr->write( &val, TBF_DP17+TBF_BASE, 1);
    usleep(1000);
    val = 0x2d2d;
    mDaumSPtr->write( &val, TBF_DP17+TBF_BASE, 1);
    if (wordCount > 0)
    {
        val = 0;
        BFN_SET(val, wordCount, HVSPI_W_C);
        LOGI("setting BaseConfigLength of %d (0x%08x)", wordCount, val);
        mDaumSPtr->write(&val, TBF_REG_HVSPI0+TBF_BASE, 1);
        LOGI("Triggering HV SPI write");
        mDaumSPtr->write(&val, TBF_TRIG_HVSPI+TBF_BASE, 1);
        usleep(1000);

        // Read TBF_RO_TX and log
        mDaumSPtr->read(TBF_RO_TX+TBF_BASE, &val, 1);
        LOGI("TBF_R0_TX value after reset, before loading MSSTTXC is 0x%08X", val);
    }
    else
    {
        LOGE("%s : wordCount is 0");
    }
}
