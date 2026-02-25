//
// Copyright 2017 EchoNous Inc.
//
//
#define LOG_TAG "DauInputManager"

//#define V3_DAECG 1
#define ENABLE_EXTERNAL_ECG 1

#include <DauInputManager.h>
#include <ThorConstants.h>
#include <DauRegisters.h>
#include <DauFwUpgrade.h>

DauInputManager::DauInputManager(const std::shared_ptr<DauMemory>& daum,
                                 const std::shared_ptr<DauMemory>& daumBar2,
                                 const std::shared_ptr<UpsReader>& upsReader,
                                 const std::shared_ptr<DmaBuffer>& dmaBuffer,
                                 const std::shared_ptr<DauHw>& dauHw,
                                 const std::shared_ptr<UsbDataHandler>& usbDataHandler,
                                 bool detectEcg):
    mNumTxLinesB(23),
    mMultiLineFactor(4),
    mImagingMode(0),
    mBFrameStride(0),
    mCFrameStride(0),
    mMFrameStride(0),
    mPwFrameStride(0),
    mCwFrameStride(0),
    mNumBSamples(0),
    mNumBLines(0),
    mNumCSamples(0),
    mNumCLines(0),
    mEcgFrameSize(0),
    mDaFrameSize(0),
    mDaumSPtr(daum),
    mDaumBar2SPtr(daumBar2),
    mUpsReaderSPtr(upsReader),
    mDmaBufferSPtr(dmaBuffer),
    mDauHwSPtr(dauHw),
    mIsExternalECG(false),
    mLeadNumber(2),
    mUsbDataHandlerSPtr(usbDataHandler),
    mDetectEcg(detectEcg)
{
    LOGD("*** DauInputManager - constructor");
}

DauInputManager::~DauInputManager()
{
    LOGD("*** DauInputManager - destructor");
}

void DauInputManager::start()
{
    uint32_t val;

    // Reset Streaming Engine State Machine
    if (IMAGING_MODE_CW != mImagingMode)
    {
        if (mUsbDataHandlerSPtr)
        {
            val = 0xF3;
            mDaumSPtr->write( &val, SE_CTL_ADDR, 1 );
            val = 0xF2;
            mDaumSPtr->write( &val, SE_CTL_ADDR, 1 );
        }
        else
        {
            // Reset Streaming Engine State Machine
            val = 1;
            mDaumSPtr->write(&val, SE_CTL_ADDR, 1);
            val = 0;
            mDaumSPtr->write(&val, SE_CTL_ADDR, 1);
        }
    }

    // Enable the streaming engine for desired data types
    switch (mImagingMode)
    {
    case IMAGING_MODE_B:
        startDataType( DAU_DATA_TYPE_B );
        break;

    case IMAGING_MODE_COLOR:
        startDataType( DAU_DATA_TYPE_B );
        startDataType( DAU_DATA_TYPE_COLOR );
        break;

    case IMAGING_MODE_M:
        startDataType( DAU_DATA_TYPE_B );
        startDataType( DAU_DATA_TYPE_M );
        break;

    case IMAGING_MODE_PW:
        startDataType( DAU_DATA_TYPE_PW );
        break;

    case IMAGING_MODE_CW:
        // CW Mode RESET
        val = 0x80000000;
        mDaumSPtr->write( &val, I2S_I2SCONFIG_ADDR, 1);

        // reset SE
        val = 1;
        if (mUsbDataHandlerSPtr)
        {
            val |= BIT(SE_CTL_SLAVE_BIT);
        }
        mDaumSPtr->write(&val, SE_CTL_ADDR, 1);

        // SE DT4 Enable
        startDataType( DAU_DATA_TYPE_CW );

        val = 0x0000000e;
        mDaumSPtr->write( &val, I2S_I2SCONFIG_ADDR, 1);

        // SE no sleep | Slave
        val = 0;
        if (mUsbDataHandlerSPtr)
        {
            val |= BIT(SE_CTL_SLAVE_BIT);
        }
        mDaumSPtr->write( &val, SE_CTL_ADDR, 1 );

        // START I2S
        val = 0x0001;
        mDaumSPtr->write( &val, I2S_START_ADDR, 1);
        break;

    default:
        // TODO: return error code
        LOGE( "DauInputManager::start() imaging Mode %d is not supported", mImagingMode );
        break;
    }

}

void DauInputManager::initMotionSensors()
{
//    DauHw::initMotionSensors(mDaumSPtr, 128, 30);
}

void DauInputManager::startMotionSensors()
{
//    startDataType(DAU_DATA_TYPE_SPI);
}

void DauInputManager::stopMotionSensors()
{
}

#ifdef V3_DAEKG
void DauInputManager::initDaEkg(bool daIsOn, bool EcgIsOn)
{
    uint32_t val;

    // Reset SPI
    {
        val = SENSOR_SPIMCFG_SPIRESET_MASK;
        mDaumSPtr->write( &val, SENSOR_SPIMCFG_ADDR, 1 );
        val = 0;
        mDaumSPtr->write( &val, SENSOR_SPIMCFG_ADDR, 1 );
    }
        
    {
        // p.writeTOP(reg.SENSOR_ADDR_SAMPLESIZE, (nSampleWords-1) << reg.SENSOR_SAMPLESIZE_SPIDSIZE_SH)
        // nSampleWords = 8
        val = 0;
        mDaumSPtr->read(SENSOR_SAMPLESIZE_ADDR, &val, 1 );
        BFN_SET( val, 4-1, SENSOR_SAMPLESIZE_SPIDSIZE );
        mDaumSPtr->write( &val, SENSOR_SAMPLESIZE_ADDR, 1 );
        
        // p.writeTOP(reg.SENSOR_ADDR_SPIFRAMESIZE, nSamples-1)
        // nSamples = mEcgFrameSize
        val = 0;
        mDaumSPtr->read(SENSOR_SPIFRAMESIZE_ADDR, &val, 1 );
        BFN_SET( val, mEcgFrameSize-1, SENSOR_SPIFRAMESIZE_SPISAMPLES );
        mDaumSPtr->write( &val, SENSOR_SPIFRAMESIZE_ADDR, 1);
    }

    // ti.evm_file()
    {  
        uint32_t val;

        val = 0;

        BFN_SET( val, 16-1, SENSOR_SPI3CFG_LENGTH );
        BFN_SET( val, 1,    SENSOR_SPI3CFG_MSB );
        BFN_SET( val, 3,    SENSOR_SPI3CFG_MODE );
        BFN_SET( val, 20,   SENSOR_SPI3CFG_CLKDIV );

        mDaumSPtr->write( &val, SENSOR_SPI3CFG_ADDR, 1 );

        uint32_t ecgBlob[ UpsReader::MAX_ECG_BLOB_SIZE ];
        //uint32_t ecgBlobSize = mUpsReaderSPtr->getEcgBlob(0, ecgBlob);
        uint32_t ecgBlobSize = mUpsReaderSPtr->getEcgBlob(!mIsExternalECG, mLeadNumber, ecgBlob);

        val = 0;
        BFN_SET( val, 1, SENSOR_SPIMCFG_SPIGO );
        BFN_SET( val, 3, SENSOR_SPIMCFG_SPIDEV );
    
        if (ecgBlobSize > 0)
        {
            for (uint32_t i = 0; i < ecgBlobSize; i++)
            {
                mDaumSPtr->write( &(ecgBlob[i]), SENSOR_SPI3WRDATA0_ADDR, 1);
                mDaumSPtr->write( &val, SENSOR_SPIMCFG_ADDR, 1 );
                LOGD("ECGBLOB[%d] = %08x", i, ecgBlob[i]);
                usleep(50);
            }
        }
        else
        {
            // TODO: handle error
        }
    }

    LOGD("DauInputManager::initDaEkg(), mEcgFrameSize used is %d", mEcgFrameSize );
}

void DauInputManager::openDaEkg()
{
    // Turn on EKG power
    uint32_t val = 0x40;
    mDaumSPtr->write( &val, SYS_GPIO_SET_ADDR, 1);
}

void DauInputManager::startDaEkg()
{ 
    uint32_t val;
    uint32_t afeClksPerEcgSample, afeClksPerDaSample;

    // ti.start()
    {
        val = 0;

        BFN_SET( val, 16-1, SENSOR_SPI3CFG_LENGTH );
        BFN_SET( val, 1,    SENSOR_SPI3CFG_MSB );
        BFN_SET( val, 3,    SENSOR_SPI3CFG_MODE );
        BFN_SET( val, 20,   SENSOR_SPI3CFG_CLKDIV );

        mDaumSPtr->write( &val, SENSOR_SPI3CFG_ADDR, 1 );

        val = 1; // write 1 to TI ADS1293 address 0
        mDaumSPtr->write( &val, SENSOR_SPI3WRDATA0_ADDR, 1);

        val = 0;
        BFN_SET( val, 1, SENSOR_SPIMCFG_SPIGO );
        BFN_SET( val, 3, SENSOR_SPIMCFG_SPIDEV );
        mDaumSPtr->write( &val, SENSOR_SPIMCFG_ADDR, 1);
        usleep(5);
    }

    // ti.dataloop(4)
    {
        val = 0;
        mDaumSPtr->write( &val, SENSOR_SPI3WRDATA0_ADDR, 1 );
        val = 0xD0; // 0x80 | ads1293.kDATA_LOOP (0x50)
        mDaumSPtr->write( &val, SENSOR_SPI3WRDATA1_ADDR, 1 );

        // prep_streaming
        val = 0;
        BFN_SET( val, 40-1,  SENSOR_SPI3CFG_LENGTH );
        BFN_SET( val, 1,     SENSOR_SPI3CFG_MSB );
        BFN_SET( val, 3,     SENSOR_SPI3CFG_MODE );
        BFN_SET( val, 20,    SENSOR_SPI3CFG_CLKDIV );
        mDaumSPtr->write( &val, SENSOR_SPI3CFG_ADDR, 1);
    }

    mUpsReaderSPtr->getAfeClksPerDaEcgSample( 0,
                                              afeClksPerDaSample,
                                              afeClksPerEcgSample );
    // Start ECG
    {
        configureEcgSERegs();
        startDataType(DAU_DATA_TYPE_ECG);
        val = afeClksPerEcgSample*4 - 1;
        mDaumSPtr->write( &val, SENSOR_SPIACQRATE_ADDR, 1 );

        val = 0;
        BFN_SET( val, 1, SENSOR_SPIMCFG_SPIRESET);
        mDaumSPtr->write( &val, SENSOR_SPIMCFG_ADDR, 1);

        val = 0;
        mDaumSPtr->write( &val, SENSOR_SPIMCFG_ADDR, 1);

        val = 0;
        mDaumSPtr->read(SENSOR_GBLCFG_ADDR, &val, 1 );
        BFN_SET( val, 1, SENSOR_GBLCFG_SPIAUTOMODE );
        mDaumSPtr->write( &val, SENSOR_GBLCFG_ADDR, 1 );
        
        val = 0;
        mDaumSPtr->read(SENSOR_AUTOCFG_ADDR, &val, 1 );
        // use 8 for EKG only.
        BFN_SET( val, 8, SENSOR_AUTOCFG_SPIENABLE ); // enable ch 3 only
        BFN_SET( val, 1, SENSOR_AUTOCFG_SPIAUTOGO );
        mDaumSPtr->write( &val, SENSOR_AUTOCFG_ADDR, 1 );
    }

    // Start DA
    {
        configureDaSERegs();
        startDataType(DAU_DATA_TYPE_DA);

        val = 1;
        mDaumSPtr->write( &val, SYS_TEST_ADDR, 1);

        // Turn on CW power supply by writing 0x100 to SYS_ADDR_GPIO_SET
        val = 0x100;
        mDaumSPtr->write( &val, SYS_GPIO_SET_ADDR, 1);

        // Turn on DA Amps
        val = 0x20;
        mDaumSPtr->write( &val, SYS_GPIO_SET_ADDR, 1 );


        uint32_t nSampleWords = 4;
        val = 0;
        mDaumSPtr->read(SENSOR_SAMPLESIZE_ADDR, &val, 1);
        BFN_SET( val, nSampleWords-1, SENSOR_SAMPLESIZE_DAEKGDSIZE );
        mDaumSPtr->write( &val, SENSOR_SAMPLESIZE_ADDR, 1 );
        
        val = mDaFrameSize - 1;
        mDaumSPtr->write( &val, SENSOR_DAEKGFRAMESIZE_ADDR, 1);

        // Set acquisition rate
        uint32_t nXmClkDivider = 25;
        val = 0;

        BFN_SET( val, (afeClksPerDaSample*4 - 1), SENSOR_ADC0CLKDIVS_ACQRATE);
        BFN_SET( val, (nXmClkDivider - 1), SENSOR_ADC0CLKDIVS_XMCLK_DIV);
        mDaumSPtr->write( &val, SENSOR_ADC0CLKDIVS_ADDR, 1);


        // Put ADC in reset
        // Select bias control, external VRef
        // Enable external sample clock
        val = 0;
        BFN_SET( val, 1, SENSOR_ADC0CFG_XRST);
        BFN_SET( val, 1, SENSOR_ADC0CFG_XPWDB);
        BFN_SET( val, 1, SENSOR_ADC0CFG_XVREFEN);
        BFN_SET( val, 1, SENSOR_ADC0CFG_XSAMPSEL);
        BFN_SET( val, 1, SENSOR_ADC0CFG_XIBC );
        mDaumSPtr->write( &val, SENSOR_ADC0CFG_ADDR, 1 );

        // Release reset
        BFN_SET( val, 0, SENSOR_ADC0CFG_XRST );
        mDaumSPtr->write( &val, SENSOR_ADC0CFG_ADDR, 1 );

        
        // Enable ADC auto mode
        // Only clock ADC when accessing
        // Select channels 1, 2 & 3 (DA only)
        BFN_SET( val, 1, SENSOR_ADC0CFG_XTCP1 );
        BFN_SET( val, 1, SENSOR_ADC0CFG_ADCAUTOGO );
        BFN_SET( val, 1, SENSOR_ADC0CFG_ADCMODE );
        BFN_SET( val, 0xF, SENSOR_ADC0CFG_XAINSEL );
        mDaumSPtr->write( &val, SENSOR_ADC0CFG_ADDR, 1 );
    }
}

void DauInputManager::stopDaEkg()
{
    uint32_t val = 0;

    val = 0;
    mDaumSPtr->write( &val, SENSOR_GBLCFG_ADDR, 1 );
    usleep(50);
    mDaumSPtr->write( &val, SENSOR_AUTOCFG_ADDR, 1 );


    do {
        val = 0;
        mDaumSPtr->read(SENSOR_SPISTAT_ADDR, &val, 1);
    } while (val != 0);

    {
        val = 0;
        BFN_SET( val, 16-1,  SENSOR_SPI3CFG_LENGTH );
        BFN_SET( val, 1,     SENSOR_SPI3CFG_MSB );
        BFN_SET( val, 3,     SENSOR_SPI3CFG_MODE );
        BFN_SET( val, 20,    SENSOR_SPI3CFG_CLKDIV );
        mDaumSPtr->write( &val, SENSOR_SPI3CFG_ADDR, 1);
    }

    val = 0;
    mDaumSPtr->write( &val, SENSOR_SPI3WRDATA0_ADDR, 1 );
    BFN_SET( val, 1, SENSOR_SPIMCFG_SPIGO );
    BFN_SET( val, 3, SENSOR_SPIMCFG_SPIDEV );
    mDaumSPtr->write( &val, SENSOR_SPIMCFG_ADDR, 1 );

    val = 0;
    mDaumSPtr->write( &val, SENSOR_ADC0CFG_ADDR, 1 );

    stopDataType( DAU_DATA_TYPE_ECG );
    stopDataType( DAU_DATA_TYPE_DA );
}

void DauInputManager::setExternalECG(bool isExternal)
{
    mIsExternalECG = isExternal;
}

void DauInputManager::setLeadNumber(uint32_t leadNumber)
{
    mLeadNumber=leadNumber;
}

#else
void DauInputManager::initDaEkg(bool daIsOn, bool EcgIsOn)
{
    uint32_t val;

    // Reset SPI
    {
        val = SENSOR_SPIMCFG_SPIRESET_MASK;
        mDaumSPtr->write( &val, SENSOR_SPIMCFG_ADDR, 1 );
        val = 0;
        mDaumSPtr->write( &val, SENSOR_SPIMCFG_ADDR, 1 );
    }

    {
        // p.writeTOP(reg.SENSOR_ADDR_SAMPLESIZE, (nSampleWords-1) << reg.SENSOR_SAMPLESIZE_SPIDSIZE_SH)
        // nSampleWords = 8
        val = 0;
        mDaumSPtr->read(SENSOR_SAMPLESIZE_ADDR, &val, 1 );
        BFN_SET( val, 4-1, SENSOR_SAMPLESIZE_SPIDSIZE );
        mDaumSPtr->write( &val, SENSOR_SAMPLESIZE_ADDR, 1 );

        // p.writeTOP(reg.SENSOR_ADDR_SPIFRAMESIZE, nSamples-1)
        // nSamples = mEcgFrameSize
        val = 0;
        mDaumSPtr->read(SENSOR_SPIFRAMESIZE_ADDR, &val, 1 );
        BFN_SET( val, mEcgFrameSize-1, SENSOR_SPIFRAMESIZE_SPISAMPLES );
        mDaumSPtr->write( &val, SENSOR_SPIFRAMESIZE_ADDR, 1);
    }

    // ti.evm_file()
    {
        uint32_t val;
        uint32_t daecgId = 0;

        val = 0;

        BFN_SET( val, 16-1, SENSOR_SPI0CFG_LENGTH );
        BFN_SET( val, 1,    SENSOR_SPI0CFG_MSB );
        BFN_SET( val, 3,    SENSOR_SPI0CFG_MODE );
        BFN_SET( val, 20,   SENSOR_SPI0CFG_CLKDIV );

        mDaumSPtr->write( &val, SENSOR_SPI0CFG_ADDR, 1 );

        uint32_t ecgBlob[ UpsReader::MAX_ECG_BLOB_SIZE ];
        // TODO: isInternal needs to come from the application layer
        //bool isInternal = true;
        // TODO: lead selection has to come from the application layer
        //uint32_t leadNum = 2;

        if (mImagingMode == IMAGING_MODE_PW)
            daecgId = 1;

        uint32_t ecgBlobSize = mUpsReaderSPtr->getEcgBlob(!mIsExternalECG, mLeadNumber, daecgId, ecgBlob);

        val = 0;
        BFN_SET( val, 1, SENSOR_SPIMCFG_SPIGO );
        BFN_SET( val, 0, SENSOR_SPIMCFG_SPIDEV );

        if (ecgBlobSize > 0)
        {
            for (uint32_t i = 0; i < ecgBlobSize; i++)
            {
                mDaumSPtr->write( &(ecgBlob[i]), SENSOR_SPI0WRDATA0_ADDR, 1);
                mDaumSPtr->write( &val, SENSOR_SPIMCFG_ADDR, 1 );
                LOGD("ECGBLOB[%d] = %08x", i, ecgBlob[i]);
                usleep(50);
            }
        }
        else
        {
            // TODO: handle error
        }
    }

    LOGD("DauInputManager::initDaEkg(), mEcgFrameSize used is %d", mEcgFrameSize );
}

void DauInputManager::openDaEkg()
{
    // Turn on EKG power
    uint32_t val = 0x40;
    mDaumSPtr->write( &val, SYS_GPIO_SET_ADDR, 1);
}

void DauInputManager::startDaEkg()
{
    uint32_t val;
    uint32_t afeClksPerEcgSample, afeClksPerDaSample;
    uint32_t daecgID = 0;

    // ti.start()
    {
        val = 0;

        BFN_SET( val, 16-1, SENSOR_SPI0CFG_LENGTH );
        BFN_SET( val, 1,    SENSOR_SPI0CFG_MSB );
        BFN_SET( val, 3,    SENSOR_SPI0CFG_MODE );
        BFN_SET( val, 20,   SENSOR_SPI0CFG_CLKDIV );

        mDaumSPtr->write( &val, SENSOR_SPI0CFG_ADDR, 1 );

        val = 1; // write 1 to TI ADS1293 address 0
        mDaumSPtr->write( &val, SENSOR_SPI0WRDATA0_ADDR, 1);

        val = 0;
        BFN_SET( val, 1, SENSOR_SPIMCFG_SPIGO );
        BFN_SET( val, 0, SENSOR_SPIMCFG_SPIDEV );
        mDaumSPtr->write( &val, SENSOR_SPIMCFG_ADDR, 1);
        usleep(5);
    }

    // ti.dataloop(4)
    {
        val = 0;
        mDaumSPtr->write( &val, SENSOR_SPI0WRDATA0_ADDR, 1 );
        val = 0xD0; // 0x80 | ads1293.kDATA_LOOP (0x50)
        mDaumSPtr->write( &val, SENSOR_SPI0WRDATA1_ADDR, 1 );

        // prep_streaming
        val = 0;
        BFN_SET( val, 40-1,  SENSOR_SPI0CFG_LENGTH );
        BFN_SET( val, 1,     SENSOR_SPI0CFG_MSB );
        BFN_SET( val, 3,     SENSOR_SPI0CFG_MODE );
        BFN_SET( val, 20,    SENSOR_SPI0CFG_CLKDIV );
        mDaumSPtr->write( &val, SENSOR_SPI0CFG_ADDR, 1);
    }

    if (mImagingMode == IMAGING_MODE_PW)
        daecgID = 1;

    mUpsReaderSPtr->getAfeClksPerDaEcgSample( daecgID,
                                              afeClksPerDaSample,
                                              afeClksPerEcgSample );

    // TODO: NEED TO UPDATE with DAECG UPS
    if (mImagingMode == IMAGING_MODE_CW)
    {
        afeClksPerEcgSample = 81920;
    }

    // Start ECG
    {
        configureEcgSERegs();
        if (!mUsbDataHandlerSPtr)
        {
            startDataType(DAU_DATA_TYPE_ECG);
        }
        val = afeClksPerEcgSample*4 - 1;
        mDaumSPtr->write( &val, SENSOR_SPIACQRATE_ADDR, 1 );

        val = 0;
        BFN_SET( val, 1, SENSOR_SPIMCFG_SPIRESET);
        mDaumSPtr->write( &val, SENSOR_SPIMCFG_ADDR, 1);

        val = 0;
        mDaumSPtr->write( &val, SENSOR_SPIMCFG_ADDR, 1);

        val = 0;
        mDaumSPtr->read(SENSOR_GBLCFG_ADDR, &val, 1 );
        BFN_SET( val, 1, SENSOR_GBLCFG_SPIAUTOMODE );
        mDaumSPtr->write( &val, SENSOR_GBLCFG_ADDR, 1 );

        val = 0;
        mDaumSPtr->read(SENSOR_AUTOCFG_ADDR, &val, 1 );
        // use 8 for EKG only.
        BFN_SET( val, 1, SENSOR_AUTOCFG_SPIENABLE ); // enable ch 0 only
        BFN_SET( val, 1, SENSOR_AUTOCFG_SPIAUTOGO );
        mDaumSPtr->write( &val, SENSOR_AUTOCFG_ADDR, 1 );
    }

    // Start DA
    {
        configureDaSERegs();
        if (!mUsbDataHandlerSPtr)
        {
            startDataType(DAU_DATA_TYPE_DA);
        }

        val = 1;
        mDaumSPtr->write( &val, SYS_TEST_ADDR, 1);

        // Turn on CW power supply by writing 0x100 to SYS_ADDR_GPIO_SET
        val = 0x100;
        mDaumSPtr->write( &val, SYS_GPIO_SET_ADDR, 1);

        // Turn on DA Amps
        val = 0x20;
        mDaumSPtr->write( &val, SYS_GPIO_SET_ADDR, 1 );


        uint32_t nSampleWords = 4;
        val = 0;
        mDaumSPtr->read(SENSOR_SAMPLESIZE_ADDR, &val, 1);
        BFN_SET( val, nSampleWords-1, SENSOR_SAMPLESIZE_DAEKGDSIZE );
        mDaumSPtr->write( &val, SENSOR_SAMPLESIZE_ADDR, 1 );

        val = mDaFrameSize - 1;
        mDaumSPtr->write( &val, SENSOR_DAEKGFRAMESIZE_ADDR, 1);

        // Set acquisition rate
        uint32_t nXmClkDivider = 25;
        val = 0;

        BFN_SET( val, (afeClksPerDaSample*4 - 1), SENSOR_ADC0CLKDIVS_ACQRATE);
        BFN_SET( val, (nXmClkDivider - 1), SENSOR_ADC0CLKDIVS_XMCLK_DIV);
        mDaumSPtr->write( &val, SENSOR_ADC0CLKDIVS_ADDR, 1);


        // Put ADC in reset
        // Select bias control, external VRef
        // Enable external sample clock
        val = 0;
        BFN_SET( val, 1, SENSOR_ADC0CFG_XRST);
        BFN_SET( val, 1, SENSOR_ADC0CFG_XPWDB);
        BFN_SET( val, 1, SENSOR_ADC0CFG_XVREFEN);
        BFN_SET( val, 1, SENSOR_ADC0CFG_XSAMPSEL);
        BFN_SET( val, 1, SENSOR_ADC0CFG_XIBC );
        mDaumSPtr->write( &val, SENSOR_ADC0CFG_ADDR, 1 );

        // Release reset
        BFN_SET( val, 0, SENSOR_ADC0CFG_XRST );
        mDaumSPtr->write( &val, SENSOR_ADC0CFG_ADDR, 1 );


        // Enable ADC auto mode
        // Only clock ADC when accessing
        // Select channels 1, 2 & 3 (DA only)
        BFN_SET( val, 1, SENSOR_ADC0CFG_XTCP1 );
        BFN_SET( val, 1, SENSOR_ADC0CFG_ADCAUTOGO );
        BFN_SET( val, 1, SENSOR_ADC0CFG_ADCMODE );
        BFN_SET( val, 0xF, SENSOR_ADC0CFG_XAINSEL );
        mDaumSPtr->write( &val, SENSOR_ADC0CFG_ADDR, 1 );
    }
}

void DauInputManager::initCwImaging()
{
    // CW Specific Registers update

    uint32_t val;

    // Power up AFE0 only
    val = 0x23;
    mDaumSPtr->write(&val, TBF_DP9 + TBF_BASE,1);

    // This will disable the HV fault monitor
    val = 0x00030201;
    mDaumSPtr->write(&val, TBF_REG_EN0 + TBF_BASE,1);
    val = 0x00000080;
    mDaumSPtr->write(&val, TBF_REG_TXRXT + TBF_BASE,1);
// The HV command voltage needs to be at 0 volts when HV EN is turned on
// then Hv ramps to final voltage
    uint32_t ps1;
    uint32_t ps2;
    mDauHwSPtr->convertHvToRegVals(0.0, 0.0, ps1, ps2);
    mDaumSPtr->write(&ps1, TBF_REG_HVPS1 + TBF_BASE,1);
    mDaumSPtr->write(&ps2, TBF_REG_HVPS2 + TBF_BASE,1);

    val = 0x00000023;
    mDaumSPtr->write(&val, TBF_REG_CK0 + TBF_BASE,1);
    val = 0x1000000F;
    mDaumSPtr->write(&val, TBF_REG_PS0 + TBF_BASE,1);

    usleep(1000*4); // Wait 4ms
    mDauHwSPtr->convertHvToRegVals(3.0, 3.0, ps1, ps2);
    mDaumSPtr->write(&ps1, TBF_REG_HVPS1 + TBF_BASE,1);
    mDaumSPtr->write(&ps2, TBF_REG_HVPS2 + TBF_BASE,1);

    usleep(1000*2); // Wait 2ms
    mDauHwSPtr->convertHvToRegVals(5.0, 5.0, ps1, ps2);
    mDaumSPtr->write(&ps1, TBF_REG_HVPS1 + TBF_BASE,1);
    mDaumSPtr->write(&ps2, TBF_REG_HVPS2 + TBF_BASE,1);
// End of HV ramp
    LOGI("============================================");
    LOGI("     Change ClkDiv and Enable CW Clks       ");
    LOGI("============================================");

    val = 0;
    mDaumBar2SPtr->read(FTSCU_PLLCTRL_ADDR, &val, 1);
    LOGD("initCwImaging - PLLCTRL: %08X", val);

    val = 0;
    mDaumBar2SPtr->read(FTSCU_PWRMOD_ADDR, &val, 1);
    LOGD("initCwImaging - PWRMOD: %08X", val);

    val = 0;
    mDaumBar2SPtr->read(FTSCU_LVDSTXCTL0_ADDR, &val, 1);
    LOGD("initCwImaging - LVDSTX: %08X", val);

    //////////////////////////////////////////////////////////
    // UPDATE PLL
    // Enable beamformer clock gating
    //val = 0x10F;
    //mDaumSPtr->write(&val, TBF_REG_CK0 + TBF_BASE,1);

    //
    // Add code to handshake with ASIC FW for clock rate switch to 250Mhz
    //
    // Then poll on PLLCTRL
    if (!mUsbDataHandlerSPtr)
    {
        if (configureFCSSwitchRegs(mDaumBar2SPtr, AX_FCS_250MHz) != THOR_OK)
        {
            ALOGE("Unable to configure FCS switch Regs 250");
        }
    }
    else
    {
        if (!mUsbDataHandlerSPtr->configureFCSSwitch(AX_FCS_250MHz))
        {
            ALOGE("Unable to configure FCS switch Regs 250");
        }
    }

    uint32_t nPLLCtrl;
    bool bLocked = false;
    uint32_t sleepCnt = 0;
    while (!bLocked) {
        mDaumBar2SPtr->read(FTSCU_PLLCTRL_ADDR, &nPLLCtrl, 1);
        bLocked = nPLLCtrl & FTSCU_PLLCTRL_PLLSTABLE_MASK;
        // timeout for 10 us
        usleep(10);
        sleepCnt++;
    }

    // LVDS Tx Control
    uint32_t nLvdStxCtl;
    mDaumBar2SPtr->read(FTSCU_LVDSTXCTL0_ADDR, &nLvdStxCtl, 1);

    uint32_t nLvdStxCtl_update = nLvdStxCtl | FTSCU_LVDSTXCTL0_PD_CLK_CW_16X_MASK | FTSCU_LVDSTXCTL0_PD_CLK_CW_1X_MASK;
    mDaumBar2SPtr->write(&nLvdStxCtl_update, FTSCU_LVDSTXCTL0_ADDR, 1);

    LOGI("============================================");
    LOGI("     PWR/PLL REGS after initCwImaging       ");
    LOGI("============================================");

    val = 0;
    mDaumBar2SPtr->read(FTSCU_PLLCTRL_ADDR, &val, 1);
    LOGD("initCwImaging - PLLCTRL: %08X", val);

    val = 0;
    mDaumBar2SPtr->read(FTSCU_PWRMOD_ADDR, &val, 1);
    LOGD("initCwImaging - PWRMOD: %08X", val);

    val = 0;
    mDaumBar2SPtr->read(FTSCU_LVDSTXCTL0_ADDR, &val, 1);
    LOGD("initCwImaging - LVDSTX: %08X", val);

    // Turn on CW power supply by writing 0x100 to SYS_ADDR_GPIO_SET
    val = 0x100;
    mDaumSPtr->write( &val, SYS_GPIO_SET_ADDR, 1);

    // reset SE
    val = 1;
    if (mUsbDataHandlerSPtr)
    {
        val |= BIT(SE_CTL_SLAVE_BIT);
    }
    mDaumSPtr->write( &val, SE_CTL_ADDR, 1 );

    // CW ADC configuration
    // GPIO RESET
    val = 0x100;
    mDaumSPtr->write( &val, SYS_GPIO_RESET_ADDR, 1);

    // I2S Configuration
    val = 0x80000000;
    mDaumSPtr->write( &val, I2S_I2SCONFIG_ADDR, 1);

    // I2S_START_ADDR
    val = 0x0000;
    mDaumSPtr->write( &val, I2S_START_ADDR, 1);

    // I2S Config -> setup Mode -RL | LJ | EN | Slave
    val = 0x0000000e;
    mDaumSPtr->write( &val, I2S_I2SCONFIG_ADDR, 1);

    // I2SCLKDIV -> SCLK = 195.3125 kHz
    val = 0x0009093f;
    mDaumSPtr->write( &val, I2S_I2SCLKDIV_ADDR, 1);

    // TODO: check the samples per frame
    // I2SFrameCGF -> num sample / frame  & word per sample
    val = (mNumCwSamplesPerLine*mNumCwLinesPerFrame - 1) + (1 << I2S_I2SFRAMECFG_SAMPLESIZE_BIT);
    mDaumSPtr->write( &val, I2S_I2SFRAMECFG_ADDR, 1);

    // START I2S
    val = 0x0001;
    mDaumSPtr->write( &val, I2S_START_ADDR, 1);

    // Clear ADC reset
    val = 0x100;
    mDaumSPtr->write( &val, SYS_GPIO_SET_ADDR, 1);

    // STOP I2S
    val = 0x0000;
    mDaumSPtr->write( &val, I2S_START_ADDR, 1);

    // reset SE
    val = 1;
    if (mUsbDataHandlerSPtr)
    {
        val |= BIT(SE_CTL_SLAVE_BIT);
    }
    mDaumSPtr->write( &val, SE_CTL_ADDR, 1 );

    // Start CW TX. Internal LS generates the tx_start pulse for the TX chips
    val = 0x0;
    mDaumSPtr->write(&val, TBF_TRIG_LS + TBF_BASE,1);

    // this timeout helps to stabilize CW signal path (it has a really long filter)
    // ideally need to wait 2 seconds or so.
    // TODO: update timout duration
    usleep(250*1000);
}

void DauInputManager::startCwImaging()
{
    uint32_t val;
    mDaumSPtr->read(SE_ST_ADDR, &val, 1);
    LOGD("StartCWImaging - SE STATUS: 0x%08X", val);
}


void DauInputManager::stopCwImaging()
{
    uint32_t val;

    // STOP I2S
    val = 0x0000;
    mDaumSPtr->write( &val, I2S_START_ADDR, 1);

    // Force AFE chips off
    val = 0x22;
    mDaumSPtr->write(&val, TBF_DP9 + TBF_BASE,1);
    // Change to managed mode
//    val = 0x0; // This causes both AFEs ON in with AFE_0 still in CW mode !
// DP9 is set back to managed mode by DauTbfManager::reinit()
    mDaumSPtr->write(&val, TBF_DP9 + TBF_BASE,1);

    // Reset TX chips
    val = 0x10;
    mDaumSPtr->write(&val, TBF_REG_TR0 + TBF_BASE,1);
    val = 0x0;
    mDaumSPtr->write(&val, TBF_REG_TR0 + TBF_BASE,1);

    // Turn back CW-related DivClk and others
    // UPDATE PLL

    //
    // Add code to handshake with ASIC FW for clock switch to 208MHZ
    //
    // Then poll on PLLCTRL
    if (!mUsbDataHandlerSPtr)
    {
        if (configureFCSSwitchRegs(mDaumBar2SPtr, AX_FCS_208MHz) != THOR_OK)
        {
            ALOGE("Unable to configure FCS switch Regs 208");
        }
    }
    else
    {
        if (!mUsbDataHandlerSPtr->configureFCSSwitch(AX_FCS_208MHz))
        {
            ALOGE("Unable to configure FCS switch Regs 208");
        }
    }

    uint32_t nPLLCtrl;
    bool bLocked = false;
    uint32_t sleepCnt = 0;
    uint32_t errCnt = 0;
    while (!bLocked && (errCnt < 5)){
        if (!mDaumBar2SPtr->read(FTSCU_PLLCTRL_ADDR, &nPLLCtrl, 1))
            errCnt++;

        bLocked = nPLLCtrl & FTSCU_PLLCTRL_PLLSTABLE_MASK;
        // timeout for 10 us
        usleep(10);
        sleepCnt++;
    }

    // LVDS Tx Control
    uint32_t nLvdStxCtl;
    mDaumBar2SPtr->read(FTSCU_LVDSTXCTL0_ADDR, &nLvdStxCtl, 1);

    uint32_t nLvdStxCtl_update = nLvdStxCtl & ~FTSCU_LVDSTXCTL0_PD_CLK_CW_16X_MASK & ~FTSCU_LVDSTXCTL0_PD_CLK_CW_1X_MASK;
    mDaumBar2SPtr->write(&nLvdStxCtl_update, FTSCU_LVDSTXCTL0_ADDR, 1);

    LOGI("============================================");
    LOGI("     PWR/PLL REGS at stopCWImaging()        ");
    LOGI("============================================");

    val = 0;
    mDaumBar2SPtr->read(FTSCU_PLLCTRL_ADDR, &val, 1);
    LOGD("stopCWImaging - PLLCTRL: %08X", val);

    val = 0;
    mDaumBar2SPtr->read(FTSCU_PWRMOD_ADDR, &val, 1);
    LOGD("stopCWImaging - PWRMOD: %08X", val);

    val = 0;
    mDaumBar2SPtr->read(FTSCU_LVDSTXCTL0_ADDR, &val, 1);
    LOGD("stopCWImaging - LVDSTX: %08X", val);
}

void DauInputManager::stopDaEkg()
{
    uint32_t val = 0;

    val = 0;
    mDaumSPtr->write( &val, SENSOR_GBLCFG_ADDR, 1 );
    usleep(50);
    mDaumSPtr->write( &val, SENSOR_AUTOCFG_ADDR, 1 );


    do {
        val = 0;
        mDaumSPtr->read(SENSOR_SPISTAT_ADDR, &val, 1);
    } while (val != 0);

    {
        val = 0;
        BFN_SET( val, 16-1,  SENSOR_SPI0CFG_LENGTH );
        BFN_SET( val, 1,     SENSOR_SPI0CFG_MSB );
        BFN_SET( val, 3,     SENSOR_SPI0CFG_MODE );
        BFN_SET( val, 20,    SENSOR_SPI0CFG_CLKDIV );
        mDaumSPtr->write( &val, SENSOR_SPI0CFG_ADDR, 1);
    }

    val = 0;
    mDaumSPtr->write( &val, SENSOR_SPI0WRDATA0_ADDR, 1 );
    BFN_SET( val, 1, SENSOR_SPIMCFG_SPIGO );
    BFN_SET( val, 0, SENSOR_SPIMCFG_SPIDEV );
    mDaumSPtr->write( &val, SENSOR_SPIMCFG_ADDR, 1 );

    val = 0;
    mDaumSPtr->write( &val, SENSOR_ADC0CFG_ADDR, 1 );

    stopDataType( DAU_DATA_TYPE_ECG );
    stopDataType( DAU_DATA_TYPE_DA );
}

void DauInputManager::setExternalECG(bool isExternal)
{
    mIsExternalECG = isExternal;
}

void DauInputManager::setLeadNumber(uint32_t leadNumber)
{
    mLeadNumber=leadNumber;
}

#endif
void DauInputManager::initTempSensors()
{
}

void DauInputManager::startTempSensors()
{
//    startDataType(DAU_DATA_TYPE_I2C);
}

void DauInputManager::stopTempSensors()
{
}

void DauInputManager::stop()
{
    uint32_t    val;

    // Disable the streaming engine for desired data types
    switch (mImagingMode)
    {
    case IMAGING_MODE_COLOR:
        stopDataType( DAU_DATA_TYPE_B );
        stopDataType( DAU_DATA_TYPE_COLOR );
        break;

    case IMAGING_MODE_B:
        stopDataType( DAU_DATA_TYPE_B );
        break;

    case IMAGING_MODE_M:
        stopDataType( DAU_DATA_TYPE_B );
        stopDataType( DAU_DATA_TYPE_M );
        break;
        
    case IMAGING_MODE_PW:
        stopDataType( DAU_DATA_TYPE_PW );
        break;
        
    case IMAGING_MODE_CW:
        stopDataType( DAU_DATA_TYPE_CW );
        break;

    default:  
        break;
    }
}

void DauInputManager::stopDataType( uint32_t dataType )
{
    uint32_t val;
    
    switch (dataType)
    {
    case DAU_DATA_TYPE_B:
        val = 0;
        BFN_SET(val, 0, SE_DT0_CTL_ENABLE);
        mDaumSPtr->write( &val, SE_DT0_CTL_ADDR, 1);
        break;

    case DAU_DATA_TYPE_COLOR:
        val = 0;
        BFN_SET(val, 0, SE_DT1_CTL_ENABLE);
        mDaumSPtr->write( &val, SE_DT1_CTL_ADDR, 1);
        break;

    case DAU_DATA_TYPE_M:
        val = 0;
        BFN_SET(val, 0, SE_DT3_CTL_ENABLE);
        mDaumSPtr->write( &val, SE_DT3_CTL_ADDR, 1);
        break;

    case DAU_DATA_TYPE_PW:
        val = 0;
        BFN_SET(val, 0, SE_DT2_CTL_ENABLE);
        mDaumSPtr->write( &val, SE_DT2_CTL_ADDR, 1);
        break;

    case DAU_DATA_TYPE_CW:
        val = 0;
        BFN_SET(val, 0, SE_DT4_CTL_ENABLE);
        mDaumSPtr->write( &val, SE_DT4_CTL_ADDR, 1);
        break;

    case DAU_DATA_TYPE_I2C:
        val = 0;
        BFN_SET(val, 0, SE_DT5_CTL_ENABLE);
        mDaumSPtr->write( &val, SE_DT5_CTL_ADDR, 1);
        break;

    case DAU_DATA_TYPE_ECG:
    case DAU_DATA_TYPE_GYRO:
        val = 0;
        BFN_SET(val, 0, SE_DT6_CTL_ENABLE);
        mDaumSPtr->write( &val, SE_DT6_CTL_ADDR, 1);
        break;

    case DAU_DATA_TYPE_DA:
        val = 0;
        BFN_SET(val, 0, SE_DT7_CTL_ENABLE);
        mDaumSPtr->write( &val, SE_DT7_CTL_ADDR, 1);
        break;

    default:
        LOGE("%d is unsupported DAU Data Type", dataType);
        break;
    }
}

bool DauInputManager::setImagingCase(uint32_t imagingCase, int imagingMode, bool isDaOn, bool isEcgOn, bool isUsOn)
{
    uint32_t val;
    LOGD("DauInputManager::setImagingCase(%d, %d, %d)", imagingCase, imagingMode, isDaOn);

    mImagingMode = imagingMode;

    val = IRQ2_BASE_BITS;
#ifdef ENABLE_EXTERNAL_ECG
    if (mDetectEcg)
    {
        LOGD("Detect ECG");
        val |= IRQ2_EXT_ECG_BIT;
    }
#endif
    mDaumSPtr->write( &val, SYS_MASK_INT1_ADDR, 1);

    switch (mImagingMode)
    {
    case IMAGING_MODE_B:
        mNumTxLinesB = mUpsReaderSPtr->getNumTxBeamsB( imagingCase, imagingMode );
        mMultiLineFactor = mUpsReaderSPtr->getNumParallel( imagingCase );
        configureDataTypeBSERegs(imagingCase);
        if (!mUsbDataHandlerSPtr)
        {
            if (isUsOn)
            {
                val = IRQ_BASE_BITS | IRQ_BMODE_BIT;
            }
            else if (isDaOn)
            {
                val = IRQ_BASE_BITS | IRQ_DA_BIT;
            }
            else
            {
                val = IRQ_BASE_BITS | IRQ_ECG_BIT;
            }
        }
        else
        {
            val = IRQ_BASE_BITS;
            if (isUsOn)
            {
                val |= IRQ_BMODE_BIT;
            }
            if (isDaOn)
            {
                val |= IRQ_DA_BIT;
            }
            if (isEcgOn)
            {
                val |= IRQ_ECG_BIT;
            }
        }
        mDaumSPtr->write( &val, SYS_MASK_INT0_ADDR, 1);
        break;

    case IMAGING_MODE_COLOR:
        mNumTxLinesB = mUpsReaderSPtr->getNumTxBeamsB( imagingCase, imagingMode );
        mMultiLineFactor = mUpsReaderSPtr->getNumParallel( imagingCase );
        configureDataTypeColorSERegs(imagingCase);
        configureDataTypeBSERegs(imagingCase);
        if (!mUsbDataHandlerSPtr)
        {
            if (isUsOn)
            {
                val = IRQ_BASE_BITS | IRQ_CMODE_BIT;
            }
            else if (isDaOn)
            {
                val = IRQ_BASE_BITS | IRQ_DA_BIT;
            }
            else
            {
                val = IRQ_BASE_BITS | IRQ_ECG_BIT;
            }
        }
        else
        {
            val = IRQ_BASE_BITS;
            if (isUsOn)
            {
                val |= IRQ_BMODE_BIT |IRQ_CMODE_BIT;
            }
            if (isDaOn)
            {
                val |= IRQ_DA_BIT;
            }
            if (isEcgOn)
            {
                val |= IRQ_ECG_BIT;
            }
        }
        mDaumSPtr->write( &val, SYS_MASK_INT0_ADDR, 1);
        break;

    case IMAGING_MODE_PW:
        configureDataTypePwSERegs();

        if (isUsOn)
        {
            val = IRQ_BASE_BITS | IRQ_PW_BIT;
        }
        else if (isDaOn )
        {
            val = IRQ_BASE_BITS | IRQ_DA_BIT;
        }
        else
        {
            val = IRQ_BASE_BITS | IRQ_ECG_BIT;
        }

        mDaumSPtr->write( &val, SYS_MASK_INT0_ADDR, 1);
        break;

    case IMAGING_MODE_M:
        mNumTxLinesB = mUpsReaderSPtr->getNumTxBeamsB( imagingCase, imagingMode );
        mMultiLineFactor = mUpsReaderSPtr->getNumParallel( imagingCase );
        configureDataTypeBSERegs(imagingCase);
        configureDataTypeMSERegs();
        if (!mUsbDataHandlerSPtr)
        {
            val = IRQ_BASE_BITS | IRQ_MMODE_BIT;
        }
        else
        {
            val = IRQ_BASE_BITS | IRQ_MMODE_BIT | IRQ_BMODE_BIT;
        }
        mDaumSPtr->write(&val, SYS_MASK_INT0_ADDR, 1);
        break;

    case IMAGING_MODE_CW:
        configureDataTypeCwSERegs();
        val = IRQ_BASE_BITS | IRQ_CW_BIT;
        mDaumSPtr->write( &val, SYS_MASK_INT0_ADDR, 1);
        break;

    default:
        LOGE("Invalid imaging mode %d", imagingMode);
        break;
    }

    return (true);
}

void DauInputManager::configureDataTypeBSERegs( uint32_t imagingCase )
{
    UNUSED(imagingCase);

    uint32_t val;
    uint32_t fifoAxAddr;
    uint32_t baseAxAddr;

    if (mUsbDataHandlerSPtr)
    {
        fifoAxAddr = SE_FB_MEM_START_ADDR;
        baseAxAddr = fifoAxAddr + SE_BUF_B_BASE;
    }
    else
    {
        fifoAxAddr = PCIE_MMAP_MEM_START_ADDR;
        baseAxAddr = fifoAxAddr + INPUT_BASE_OFFSET_B;
    }


    mDaumSPtr->write((uint32_t *)&baseAxAddr, SE_DT0_FADDR_ADDR, 1 );
    mBFrameStride = (mNumBSamples*mNumBLines) + sizeof(FrameHeader);

    LOGI("B-Mode frame stride is 0x%08x", mBFrameStride);

    val = mBFrameStride + (INPUT_FIFO_LENGTH_LOG2 << SE_DT0_FSTR_SIZE_BIT);
    mDaumSPtr->write( &val, SE_DT0_FSTR_ADDR, 1 );

    val = mNumBSamples*4;   // 4 bytes per sample, regardless of multiline factor
    mDaumSPtr->write( &val, SE_DT0_LSTR_ADDR, 1 );

    val = 0;
    mDaumSPtr->write( &val, SE_DT0_FIDX_ADDR, 1 ); // Starting frame index = 0

    val = SE_BUF_B_BASE + (SE_FRAMES_LOG2 << SE_DT0_CADDR_CBSIZE_BIT);
    mDaumSPtr->write( &val, SE_DT0_CADDR_ADDR, 1);
}

void DauInputManager::configureDataTypeColorSERegs( uint32_t imagingCase )
{
    uint32_t val;
    uint32_t fifoAxAddr;
    uint32_t baseAxAddr;

    if (mUsbDataHandlerSPtr)
    {
        fifoAxAddr = SE_FB_MEM_START_ADDR;
        baseAxAddr = fifoAxAddr + SE_BUF_COLOR_BASE;
    }
    else
    {
        fifoAxAddr = PCIE_MMAP_MEM_START_ADDR;
        baseAxAddr = fifoAxAddr + INPUT_BASE_OFFSET_COLOR;
    }
    mDaumSPtr->write((uint32_t *)&baseAxAddr, SE_DT1_FADDR_ADDR, 1 );

    mCFrameStride = (mNumCSamples*mNumCLines*16) + sizeof(FrameHeader);
    LOGI("C-Mode frame stride is 0x%08x", mCFrameStride);

    val = mCFrameStride + (INPUT_FIFO_LENGTH_LOG2 << SE_DT1_FSTR_SIZE_BIT);
    mDaumSPtr->write( &val, SE_DT1_FSTR_ADDR, 1 );

    // set line stride according to ensemble size
    {
        uint32_t ensSize = mUpsReaderSPtr->getEnsembleSize( imagingCase );
        if ( ensSize >= 16)
        {
            val = mNumCSamples*4; // 4 transfers per line
        }
        else if (ensSize >= 8)
        {
            val = mNumCSamples*8; // 2 transfers per line
        }
        else // less than 8
        {
            val = mNumCSamples*16; // 1 transfer per line
        }
        LOGI("Color Line stride = %d", val);
        mDaumSPtr->write( &val, SE_DT1_LSTR_ADDR, 1 );
    }
        
    val = 0;
    mDaumSPtr->write( &val, SE_DT1_FIDX_ADDR, 1 ); // Starting frame index = 0

    val = SE_BUF_COLOR_BASE + (SE_FRAMES_LOG2 << SE_DT1_CADDR_CBSIZE_BIT);
    mDaumSPtr->write( &val, SE_DT1_CADDR_ADDR, 1);
}

void DauInputManager::configureDataTypePwSERegs()
{
    uint32_t val;
    uint32_t fifoAxAddr = PCIE_MMAP_MEM_START_ADDR;
    uint32_t baseAxAddr = fifoAxAddr + INPUT_BASE_OFFSET_PW;

    mDaumSPtr->write((uint32_t *)&baseAxAddr, SE_DT2_FADDR_ADDR, 1 );
    mPwFrameStride = (mNumPwSamplesPerLine*mNumPwLinesPerFrame*16) + sizeof(FrameHeader);

    LOGI("PW-Mode frame stride is 0x%08x", mPwFrameStride);

    val = mPwFrameStride + (INPUT_FIFO_LENGTH_LOG2 << SE_DT2_FSTR_SIZE_BIT);
    mDaumSPtr->write( &val, SE_DT2_FSTR_ADDR, 1 );

    val = mNumPwSamplesPerLine*16;   // 16 bytes per sample
    mDaumSPtr->write( &val, SE_DT2_LSTR_ADDR, 1 );

    val = 0;
    mDaumSPtr->write( &val, SE_DT2_FIDX_ADDR, 1 ); // Starting frame index = 0

    val = SE_BUF_PW_BASE + (SE_FRAMES_LOG2 << SE_DT2_CADDR_CBSIZE_BIT);
    mDaumSPtr->write( &val, SE_DT2_CADDR_ADDR, 1);

}

void DauInputManager::configureDataTypeMSERegs()
{
    uint32_t val;
    uint32_t fifoAxAddr;
    uint32_t baseAxAddr;

    if (mUsbDataHandlerSPtr)
    {
        fifoAxAddr = SE_FB_MEM_START_ADDR;
        baseAxAddr = fifoAxAddr + SE_BUF_M_BASE;
    }
    else
    {
        fifoAxAddr = PCIE_MMAP_MEM_START_ADDR;
        baseAxAddr = fifoAxAddr + INPUT_BASE_OFFSET_M;
    }

    LOGI("configureDataTypeMSERegs():");
    mDaumSPtr->write((uint32_t *)&baseAxAddr, SE_DT3_FADDR_ADDR, 1 );

    mMFrameStride = (mNumBSamples*mNumBLines) + sizeof(FrameHeader) + 512;

    LOGI("M-Mode frame stride is 0x%08x", mMFrameStride);

    val = mMFrameStride + (INPUT_FIFO_LENGTH_LOG2 << SE_DT3_FSTR_SIZE_BIT);
    mDaumSPtr->write( &val, SE_DT3_FSTR_ADDR, 1 );

    val = mNumBSamples*4;   // 4 bytes per sample, regardless of multiline factor
    mDaumSPtr->write( &val, SE_DT3_LSTR_ADDR, 1 );

    val = 0;
    mDaumSPtr->write( &val, SE_DT3_FIDX_ADDR, 1 ); // Starting frame index = 0

    val = SE_BUF_M_BASE + (SE_FRAMES_LOG2 << SE_DT3_CADDR_CBSIZE_BIT);
    mDaumSPtr->write( &val, SE_DT3_CADDR_ADDR, 1);
}

void DauInputManager::configureDataTypeCwSERegs()
{
    uint32_t val;
    uint32_t fifoAxAddr = PCIE_MMAP_MEM_START_ADDR;
    uint32_t baseAxAddr = fifoAxAddr + INPUT_BASE_OFFSET_CW;

    mDaumSPtr->write((uint32_t *)&baseAxAddr, SE_DT4_FADDR_ADDR, 1 );
    mCwFrameStride = (mNumCwSamplesPerLine*mNumCwLinesPerFrame*8) + sizeof(FrameHeader);

    LOGI("CW-Mode frame stride is 0x%08x", mCwFrameStride);

    // TODO: update
    val = mCwFrameStride + ( ( INPUT_FIFO_LENGTH_LOG2 + 0 ) << SE_DT4_FSTR_SIZE_BIT);
    mDaumSPtr->write( &val, SE_DT4_FSTR_ADDR, 1 );

    val = mNumCwSamplesPerLine*8;   // 8 bytes per sample
    mDaumSPtr->write( &val, SE_DT4_LSTR_ADDR, 1 );

    // TODO: check -> 4?
    val = 0;
    mDaumSPtr->write( &val, SE_DT4_FIDX_ADDR, 1 ); // Starting frame index = 0

    val = SE_BUF_CW_BASE + (SE_FRAMES_LOG2 << SE_DT4_CADDR_CBSIZE_BIT);
    mDaumSPtr->write( &val, SE_DT4_CADDR_ADDR, 1);
}

void DauInputManager::configureDataTypeI2cSERegs()
{
}

ThorStatus DauInputManager::configureFCSSwitchRegs(std::shared_ptr<DauMemory> daumSPtr, FCSType fcsVal)
{
    ThorStatus retStat = THOR_ERROR;
    uint32_t val = 0;
    uint32_t respTimeOut = 2000;  // ~2 Sec timeout
    daumSPtr->write(&val, PCI_BIST_RESP_ADDR, 1);

    if(fcsVal == AX_FCS_250MHz || fcsVal == AX_FCS_208MHz)
    {
        val = fcsVal;
        val = val << 12;  // masked value with 0xF000
    } else{
        ALOGE("Invalid FCS Value: %d\n", fcsVal);
    }

    daumSPtr->write(&val, PCI_BIST_CMD_ADDR, 1);
    //Enable timer
    daumSPtr->read(FTTMR_TMCR_ADDR, &val, 1);
    val |= ( (1 << FTTMR_TMCR_TM2UPDOWN_BIT) | (1 << FTTMR_TMCR_TM2OFENABLE_BIT) \
	        | (1 << FTTMR_TMCR_TM2CLOCK_BIT) | (1 << FTTMR_TMCR_TM2ENABLE_BIT) ) ;
    daumSPtr->write(&val, FTTMR_TMCR_ADDR, 1);

    do {
        ALOGD("Waiting..");
        val = 0;
        daumSPtr->read(PCI_BIST_RESP_ADDR, &val, 1);

        switch (val) {
            case FCS_250_RESP_OK:
                ALOGD("\nSuccessfully Executed FCS 250\n");
                retStat = THOR_OK;
                break;

            case FCS_208_RESP_OK:
                ALOGD("\nSuccessfully Executed FCS 208\n");
                retStat = THOR_OK;
                break;

            case FCS_RESP_INVALID:
                ALOGE("\nInvalid FCS Value\n");
                goto err_ret;

            case FCS_RESP_ERR:
                ALOGE("\nFCS Switching Error\n");
                goto err_ret;

            case PCI_RESP_ERR:
                ALOGE("\nInvalid to Execute FCS Change\n");
                goto err_ret;
        }
        usleep(1000);
    } while ((val != FCS_250_RESP_OK) && (val != FCS_208_RESP_OK) && respTimeOut--);

    err_ret:
    return retStat;
}

void DauInputManager::configureEcgSERegs()
{
    uint32_t val;
    uint32_t fifoAxAddr;
    uint32_t baseAxAddr;

    if (mUsbDataHandlerSPtr)
    {
        fifoAxAddr = SE_FB_MEM_START_ADDR;
        baseAxAddr = fifoAxAddr + SE_BUF_ECG_BASE;
    }
    else
    {
        fifoAxAddr = PCIE_MMAP_MEM_START_ADDR;
        baseAxAddr = fifoAxAddr + INPUT_BASE_OFFSET_ECG;
    }

    mDaumSPtr->write((uint32_t *)&baseAxAddr, SE_DT6_FADDR_ADDR, 1 );

    // ECG only
    LOGI("ECG frame stride is 0x%08x", mEcgFrameSize*16);

    val = (mEcgFrameSize*16) + sizeof(FrameHeader) + (INPUT_FIFO_LENGTH_LOG2 << SE_DT6_FSTR_SIZE_BIT);
    mDaumSPtr->write( &val, SE_DT6_FSTR_ADDR, 1 );

    val = 16;    // 32 bytes: 4 words for ECG
    mDaumSPtr->write( &val, SE_DT6_LSTR_ADDR, 1 );

    val = 0;
    mDaumSPtr->write( &val, SE_DT6_FIDX_ADDR, 1 ); // Starting frame index = 0

    val = SE_BUF_ECG_BASE + (SE_FRAMES_LOG2 << SE_DT6_CADDR_CBSIZE_BIT);
    mDaumSPtr->write( &val, SE_DT6_CADDR_ADDR, 1);
}

void DauInputManager::configureDaSERegs()
{
    uint32_t val;
    uint32_t fifoAxAddr;
    uint32_t baseAxAddr;

    if (mUsbDataHandlerSPtr)
    {
        fifoAxAddr = SE_FB_MEM_START_ADDR;
        baseAxAddr = fifoAxAddr + SE_BUF_DA_BASE;
    }
    else
    {
        fifoAxAddr = PCIE_MMAP_MEM_START_ADDR;
        baseAxAddr = fifoAxAddr + INPUT_BASE_OFFSET_DA;
    }

    mDaumSPtr->write((uint32_t *)&baseAxAddr, SE_DT7_FADDR_ADDR, 1 );

    
    LOGI("DA frame stride is 0x%08x", mDaFrameSize*16);

    val = (mDaFrameSize*16) + sizeof(FrameHeader) + (INPUT_FIFO_LENGTH_LOG2 << SE_DT7_FSTR_SIZE_BIT);
    mDaumSPtr->write( &val, SE_DT7_FSTR_ADDR, 1 );

    val = 16;    // 16 bytes: 3 words for mic + 1 word padding/sample
    mDaumSPtr->write( &val, SE_DT7_LSTR_ADDR, 1 );

    val = 0;
    mDaumSPtr->write( &val, SE_DT7_FIDX_ADDR, 1 ); // Starting frame index = 0

    val = SE_BUF_DA_BASE + (SE_FRAMES_LOG2 << SE_DT7_CADDR_CBSIZE_BIT);
    mDaumSPtr->write( &val, SE_DT7_CADDR_ADDR, 1);
}

bool DauInputManager::deinterleaveBData(uint8_t* inputPtr, uint8_t* outputPtr)
{
    bool      retVal = true;
    uint8_t*  interleavedData = inputPtr + INPUT_HEADER_SIZE_BYTES;
    uint32_t  headerSizeWords = INPUT_HEADER_SIZE_BYTES >> 2;
    uint32_t  w;
    uint32_t* wPtr            = (uint32_t *)(interleavedData);

    switch (mMultiLineFactor)
    {
    case 4:
        for (uint32_t txIndex = 0; txIndex < mNumTxLinesB; txIndex++)
        {
            for (int sampleIndex = 0; sampleIndex < mNumBSamples; sampleIndex++)
            {
                w = *wPtr++;
                *(outputPtr + sampleIndex +              0) =         w & 0xff;
                *(outputPtr + sampleIndex +   mNumBSamples) = (w >>  8) & 0xff;
                *(outputPtr + sampleIndex + 2*mNumBSamples) = (w >> 16) & 0xff;
                *(outputPtr + sampleIndex + 3*mNumBSamples) = (w >> 24) & 0xff;
            }
            outputPtr += mMultiLineFactor*mNumBSamples;
        }
        break;

    case 2:
        for (uint32_t txIndex = 0; txIndex < mNumTxLinesB; txIndex++)
        {
            for (int sampleIndex = 0; sampleIndex < mNumBSamples; sampleIndex++)
            {
                w = *wPtr++;
                *(outputPtr + sampleIndex +              0) =         w & 0xff;
                *(outputPtr + sampleIndex +   mNumBSamples) = (w >> 16) & 0xff;
            }
            outputPtr += mMultiLineFactor*mNumBSamples;
        }
        break;

    case 1:
        for (uint32_t txIndex = 0; txIndex < mNumTxLinesB; txIndex++)
        {
            for (int sampleIndex = 0; sampleIndex < mNumBSamples; sampleIndex++)
            {
                w = *wPtr++;
                *(outputPtr + sampleIndex) = w & 0xff;
            }
            outputPtr += mNumBSamples;
        }
        break;

    default:
        retVal = false;
        LOGE("deinterleaveBData, invalid multilinefactor (%d)", mMultiLineFactor);
        break;
    }

    return(retVal);

}

bool DauInputManager::deinterleaveMData(uint8_t* inputPtr, uint32_t multiLineSelection, uint8_t* outputPtr)
{
    bool      retVal = true;
    uint8_t*  interleavedData = inputPtr + INPUT_HEADER_SIZE_BYTES;
    uint32_t  headerSizeWords = INPUT_HEADER_SIZE_BYTES >> 2;
    uint32_t  w;
    uint32_t* wPtr            = (uint32_t *)(interleavedData);

    if (multiLineSelection > 3) // Multiline factor of 4 assumed
    {
        LOGE("multiline index %d is invalid", multiLineSelection);
        return(false);
    }
    
    for (uint32_t txIndex = 0; txIndex < mNumTxLinesB; txIndex++)
    {
        for (uint32_t sampleIndex = 0; sampleIndex < mNumBSamples; sampleIndex++)
        {
            // select the byte of interest
            *outputPtr++ = ((*wPtr++) >> (8*multiLineSelection)) & 0xff; 
        }
    }
    return(retVal);

}

void DauInputManager::setBFrameSize( uint32_t numSamples, uint32_t numLines )
{
    mNumBSamples = numSamples;
    mNumBLines   = numLines;
}

void DauInputManager::setColorFrameSize( uint32_t numSamples, uint32_t numLines )
{
    mNumCSamples = numSamples;
    mNumCLines   = numLines;
}

void DauInputManager::setPwFrameSize( uint32_t samplesPerLine, uint32_t linesPerFrame )
{
    mNumPwSamplesPerLine = samplesPerLine;
    mNumPwLinesPerFrame  = linesPerFrame;
}

void DauInputManager::setCwFrameSize( uint32_t samplesPerLine, uint32_t linesPerFrame )
{
    mNumCwSamplesPerLine = samplesPerLine;
    mNumCwLinesPerFrame  = linesPerFrame;
}

void DauInputManager::setEcgFrameSize( uint32_t frameSize )
{
    mEcgFrameSize = frameSize;
}

void DauInputManager::setDaFrameSize( uint32_t frameSize )
{
    mDaFrameSize = frameSize;
}

bool DauInputManager::startDataType( uint32_t dauDataType )
{
    uint32_t val;
    bool retVal = true;
    
    switch (dauDataType)
    {
    case DAU_DATA_TYPE_B:
        val = 0;
        mDaumSPtr->write( &val, SE_DT0_FIDX_ADDR, 1 );

        BFN_SET(val, 1, SE_DT0_CTL_ENABLE);
        mDaumSPtr->write( &val, SE_DT0_CTL_ADDR, 1);
        break;

    case DAU_DATA_TYPE_COLOR:
        val = 0;
        mDaumSPtr->write( &val, SE_DT1_FIDX_ADDR, 1 );

        BFN_SET(val, 1, SE_DT1_CTL_ENABLE);
        mDaumSPtr->write( &val, SE_DT1_CTL_ADDR, 1);
        break;

    case DAU_DATA_TYPE_M:
        val = 0;
        mDaumSPtr->write( &val, SE_DT3_FIDX_ADDR, 1 );

        BFN_SET(val, 1, SE_DT3_CTL_ENABLE);
        mDaumSPtr->write( &val, SE_DT3_CTL_ADDR, 1);
        break;

    case DAU_DATA_TYPE_PW:
        val = 0;
        mDaumSPtr->write( &val, SE_DT2_FIDX_ADDR, 1 );

        BFN_SET(val, 1, SE_DT2_CTL_ENABLE);
        mDaumSPtr->write( &val, SE_DT2_CTL_ADDR, 1);
        break;

    case DAU_DATA_TYPE_CW:
        val = 0;
        mDaumSPtr->write( &val, SE_DT4_FIDX_ADDR, 1 );

        BFN_SET(val, 1, SE_DT4_CTL_ENABLE);
        mDaumSPtr->write( &val, SE_DT4_CTL_ADDR, 1);
        break;

    case DAU_DATA_TYPE_I2C:
        val = 0;
        mDaumSPtr->write( &val, SE_DT5_FIDX_ADDR, 1 );

        BFN_SET(val, 1, SE_DT5_CTL_ENABLE);
        mDaumSPtr->write( &val, SE_DT5_CTL_ADDR, 1);
        break;

    case DAU_DATA_TYPE_ECG:
    case DAU_DATA_TYPE_GYRO:
        val = 0;
        mDaumSPtr->write( &val, SE_DT6_FIDX_ADDR, 1 );

        BFN_SET(val, 1, SE_DT6_CTL_ENABLE);
        mDaumSPtr->write( &val, SE_DT6_CTL_ADDR, 1);
        break;

    case DAU_DATA_TYPE_DA:
        val = 0;
        mDaumSPtr->write( &val, SE_DT7_FIDX_ADDR, 1 );

        BFN_SET(val, 1, SE_DT7_CTL_ENABLE);
        mDaumSPtr->write( &val, SE_DT7_CTL_ADDR, 1);
        break;

    default:
        LOGE("%d is unsupported DAU Data Type", dauDataType);
        retVal = false;
        break;
    }

    return (retVal);
}
