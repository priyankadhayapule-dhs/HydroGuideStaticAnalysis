//
// Copyright 2020 EchoNous Inc.
//


#define LOG_TAG "DauArrayTest"
#include <DauArrayTest.h>
#include <TbfRegs.h>
#include <cmath>
#include <UpsReader.h>
#include <ThorConstants.h>

DauArrayTest::DauArrayTest(const std::shared_ptr<DauMemory>& daum,
                           const std::shared_ptr<UpsReader>& upsReader,
                           const std::shared_ptr<DauHw>& dauHw,
                           uint32_t probe_type) :
        mInitialized(false),
        mDaumSPtr(daum),
        mUpsReaderSPtr(upsReader),
        mDauHwSPtr(dauHw)
{
    mIsElementCheckRunning = false;
    if (mUpsReaderSPtr->getReverbTestParams(mRmsLow,
					    mRmsHigh,
					    mBistwf,
					    mHvps1,
					    mHvps2,
					    mTxrxt,
					    mAfetr,
					    mSamples))
    {
        mInitialized = true;
    }

    //
    // Get probe type
    //
    probetype = probe_type & DEV_PROBE_MASK;
    if (probetype == PROBE_TYPE_LINEAR) {
       tx_chips = 8;
    }
    else {
       tx_chips = 4;
    }

}


DauArrayTest::~DauArrayTest()
{
}

uint32_t DauArrayTest::run()
{
    float rms_low = mRmsLow;
    float rms_high = mRmsHigh;
    // Init element min/max values
    rms_element_min = 8192.0f;
    rms_element_max = 0.0f;

    uint32_t val;
    mIsElementCheckRunning = true;
    if (!(probetype==PROBE_TYPE_LINEAR))
    {
       // Transmit waveform data for TORSO phase array transducer
       bistwf[0] = 0x000003FF;
       bistwf[1] = 0x000003FF;
       bistwf[2] = 0x0000001A;
       bistwf[3] = 0x0000001A;
       bistwf[4] = 0x000003FF;
       bistwf[5] = 0x000003FF;
       bistwf[6] = 0x000003FF;
       bistwf[7] = 0x000003FF;
       bistwf[8] = 0x0000001F;
       bistwf[9] = 0x0000001F;
       bistwf[10] = 0x0000001F;
       bistwf[11] = 0x0000001F;
       bistwf[12] = 0x0040001E;
    }

    else
    {
       // Transmit waveform data for linear array transducer
       bistwf[0] = 0x000003EF;
       bistwf[1] = 0x000003EF;
       bistwf[2] = 0x000003EF;
       bistwf[3] = 0x0000007A;
       bistwf[4] = 0x00000075;
       bistwf[5] = 0x000007FF;
       bistwf[6] = 0x0000001E;
       bistwf[7] = 0x0000001E;
       bistwf[8] = 0x0000001E;
       bistwf[9] = 0x0000001E;
       bistwf[10] = 0x0000001E;
       bistwf[11] = 0x0000001E;
       bistwf[12] = 0x0040001E;
    }

    //
    // Top level register config
    //
    val = 0x11;
    mDaumSPtr->write(&val, TBF_REG_TR0 + TBF_BASE,1);
    usleep(1000);
    val = 0x0;
    mDaumSPtr->write(&val, TBF_REG_LS + TBF_BASE,1);
    mDaumSPtr->write(&val, TBF_REG_TR0 + TBF_BASE,1);
    val = 0x000302E3;
    mDaumSPtr->write(&val, TBF_REG_EN0 + TBF_BASE,1);
    val = 0x00000100;
    mDaumSPtr->write(&val, TBF_REG_CK0 + TBF_BASE,1);
    val = 0x00000031;
    mDaumSPtr->write(&val, TBF_REG_HVPS0 + TBF_BASE,1);
    val = 0x00002F40;
    mDaumSPtr->write(&val, TBF_DP24 + TBF_BASE,1);
    val = 0x00001B1B;
    mDaumSPtr->write(&val, TBF_DP17 + TBF_BASE,1);
    //
    // Beam parameters
    //
    val = 0x000003FF;
    mDaumSPtr->write(&val, TBF_REG_BP0 + TBF_BASE,1);
    val = 0x10000020;
    mDaumSPtr->write(&val, TBF_REG_FPS + TBF_BASE,1);
    val = 0x00180003;
    mDaumSPtr->write(&val, TBF_REG_BENABLE + TBF_BASE,1);
    val = 0x00000100;
    mDaumSPtr->write(&val, TBF_REG_TXLEN + TBF_BASE,1);
    val = 0xFFFFFFFF;
    mDaumSPtr->write(&val, TBF_REG_RXAP_LOW + TBF_BASE,1);
    mDaumSPtr->write(&val, TBF_REG_RXAP_HIGH + TBF_BASE,1);
    //
    if (!(probetype==PROBE_TYPE_LINEAR))
    {
        val = 0x01780100; // Phase array TX/RX time
        mDaumSPtr->write(&val, TBF_REG_TXRXT + TBF_BASE,1);
    }
    else
    {
        val = 0x016A0100; // Linear TX/RX time
        mDaumSPtr->write(&val, TBF_REG_TXRXT + TBF_BASE,1);
    }

    //
    val = 0x00000140; // UPS
    mDaumSPtr->write(&val, BF_AFETR_ADDR + TBF_BASE,1);
    val = 0x00000100;
    mDaumSPtr->write(&val, TBF_REG_TXLEN + TBF_BASE,1);
    val = 0x00000600;
    mDaumSPtr->write(&val, TBF_REG_BPRI + TBF_BASE,1);
    val = 0x03E00028;
    mDaumSPtr->write(&val, TBF_REG_FPLUT + TBF_BASE,1);
    //
    // HV config to +/- 3V
    //
    val = 0x0E05800A;
    mDaumSPtr->write(&val, TBF_REG_HVPS1 + TBF_BASE,1);
    val = 0x00061001;
    mDaumSPtr->write(&val, TBF_REG_HVPS2 + TBF_BASE,1);
    val = 0x1000000F;
    mDaumSPtr->write(&val, TBF_REG_PS0 + TBF_BASE,1);
    usleep(5000);
    //
    // AFE power up to standby
    //
    val = 0x00000011;
    mDaumSPtr->write(&val, TBF_DP9 + TBF_BASE,1);
    usleep(10000); // Wait for AFE power up
    val = 0x0F000010;
    mDaumSPtr->write(&val, TBF_LUT_MSRTAFE + TBF_BASE,1);
    usleep(40); // Wait for AFE SPI write
    val = 0x0FB50000;
    mDaumSPtr->write(&val, TBF_LUT_MSRTAFE + TBF_BASE,1);
    usleep(40); // Wait for AFE SPI write
    val = 0x0FB68000;
    mDaumSPtr->write(&val, TBF_LUT_MSRTAFE + TBF_BASE,1);
    usleep(40); // Wait for AFE SPI write
    val = 0x0F000000;
    mDaumSPtr->write(&val, TBF_LUT_MSRTAFE + TBF_BASE,1);
    usleep(10000); // Wait for AFE power up
    // TX chip configuration
    TxBaseConfig();

    TxWaveformConfig();

    TxDelayConfig();

    TxSWTrig();

    // Select initial aperture for 64 or 128 element arrays
    TxAperture(0);

    // Enable HV supply
    SetHV();

    val = 0x00020000; // 2 words for each SPI burst write
    mDaumSPtr->write(&val, TBF_REG_TXLUT + TBF_BASE,1);

    DoPings(3);

    AfeOnStandby();

    // Set AFE gain to 10dB
    val = 0x0F000010;
    mDaumSPtr->write(&val, TBF_LUT_MSRTAFE + TBF_BASE,1);
    usleep(40); // Wait for AFE SPI write
    val = 0x0FB50000;
    mDaumSPtr->write(&val, TBF_LUT_MSRTAFE + TBF_BASE,1);
    usleep(40); // Wait for AFE SPI write
    val = 0x0FB68000;
    mDaumSPtr->write(&val, TBF_LUT_MSRTAFE + TBF_BASE,1);
    usleep(40); // Wait for AFE SPI write
    val = 0x0F000000;
    mDaumSPtr->write(&val, TBF_LUT_MSRTAFE + TBF_BASE,1);

    // We capture from one AFE at a time to reduce power
    GetAfeData(0); // Channel [31:0] data
    GetAfeData(1); // Channel [63:0] data

    if (!(probetype==PROBE_TYPE_LINEAR))
    {
       AfeOff();
    }

    error_count = CheckRmsLevel(0);

    if (probetype==PROBE_TYPE_LINEAR)
    {
       TxAperture(64);
       DoPings(3);
       GetAfeData(0); // Channel [95:64] data
       GetAfeData(1); // Channel [127:96] data
       error_count += CheckRmsLevel(64);
    }

    SetHVOff();

    // Reset beamformer
    // Note: This does not clear delay memory
    val = 0x11;
    mDaumSPtr->write(&val, TBF_REG_TR0 + TBF_BASE,1);
    usleep(100);
    val = 0x00;
    mDaumSPtr->write(&val, TBF_REG_TR0 + TBF_BASE,1);
    if (error_count>0)
    {
       LOGD("Reverb BIST test fail %d elements",error_count);
    }
    else
    {
       LOGD("Rever BIST tst pass");
    }
    LOGD("Reverb RMS element max %f limit %f ",rms_element_max,rms_high);
    LOGD("Reverb RMS element min %f limit %f ",rms_element_min,rms_low);

    mIsElementCheckRunning = false;
    return (error_count);
}

bool DauArrayTest::isElementCheckRunning() const {
    LOGD("%s returned %s", __func__, mIsElementCheckRunning ? "true" : "false" );
    return mIsElementCheckRunning;
}


void DauArrayTest::SetHVOff()
{
    //
    // Shut down HV
    //
    uint32_t val = 0x0E05400E;
    mDaumSPtr->write(&val, TBF_REG_HVPS1 + TBF_BASE,1);
    val = 0x0005F003;
    mDaumSPtr->write(&val, TBF_REG_HVPS2 + TBF_BASE,1);
    usleep(1000);
    val = 0x0000000F;
    mDaumSPtr->write(&val, TBF_REG_PS0 + TBF_BASE,1);
    usleep(10000);
}

void DauArrayTest::AfeOnStandby()
{
    //
    // AFE power up to standby
    //
    uint32_t val = 0x00000011;
    mDaumSPtr->write(&val, TBF_DP9 + TBF_BASE,1);
    usleep(10000); // Wait for AFE power up
}

void DauArrayTest::AfeOff()
{
    //
    // Power down AFEs
    //
    uint32_t val = 0x00000022;
    mDaumSPtr->write(&val, TBF_DP9 + TBF_BASE,1);
    val = 0x00000000;
    mDaumSPtr->write(&val, TBF_DP5 + TBF_BASE,1);
    val = 0x00000000;
    mDaumSPtr->write(&val, TBF_DP6 + TBF_BASE,1);
}


void DauArrayTest::GetAfeData(uint32_t afe)
{
    //
    // Put both AFE parts in standby mode for 1mS
    //
    uint32_t val = 0x00000011;
    mDaumSPtr->write(&val, TBF_DP9 + TBF_BASE,1);
    usleep(1000); // Standby for 1mS

    if (afe==1)
    {
       //
       // Power up AFE1 to full power for elements [31:0]
       //
       val = 0x00000031;
       mDaumSPtr->write(&val, TBF_DP9 + TBF_BASE,1);
       val = 0x00000000;
       mDaumSPtr->write(&val, TBF_DP5 + TBF_BASE,1);
       val = 0xFFFFFFFF;
       mDaumSPtr->write(&val, TBF_DP6 + TBF_BASE,1);
    }

    if (afe==0)
    {
       //
       // Power up AFE0 to full power for elements [63:32]
       //
       val = 0x00000013;
       mDaumSPtr->write(&val, TBF_DP9 + TBF_BASE,1);
       val = 0xFFFFFFFF;
       mDaumSPtr->write(&val, TBF_DP5 + TBF_BASE,1);
       val = 0x00000000;
       mDaumSPtr->write(&val, TBF_DP6 + TBF_BASE,1);
    }
    usleep(5000); // Wait for AFE power up
    //
    // Do SW LS
    //
    DoPings(1);
    usleep(100); // Wait for completion of ADC capture
    //
    // Put both AFE parts in standby mode for 1mS
    //
    val = 0x00000011;
    mDaumSPtr->write(&val, TBF_DP9 + TBF_BASE,1);
    usleep(1000); // Wait AFE power up
}

//
// Set HV
//
void DauArrayTest::SetHV()
{
   if (!(probetype==PROBE_TYPE_LINEAR))
   {
     // Ramp negative supply to -19V
     uint32_t ps1;
     uint32_t ps2;
     uint32_t val = 0x1000000F;
     mDaumSPtr->write(&val, TBF_REG_PS0 + TBF_BASE,1);
     float hv_i = 0;
     for (int i = 0; i < 16; i++){
         mDauHwSPtr->convertHvToRegVals(0.0f, hv_i, ps1, ps2);
         mDaumSPtr->write(&ps1, TBF_REG_HVPS1 + TBF_BASE, 1);
         mDaumSPtr->write(&ps2, TBF_REG_HVPS2 + TBF_BASE, 1);
         hv_i = hv_i+1;
         usleep(1*1000);
     }
     usleep(5000);
       LOGD("Reverb set HV to -19V");
   }
   else
   {
     // Ramp negative supply to +/-20V
     uint32_t ps1;
     uint32_t ps2;
     float hv_i = 0;
     for (int i = 0; i < 20; i++){
         mDauHwSPtr->convertHvToRegVals(hv_i, hv_i, ps1, ps2);
         mDaumSPtr->write(&ps1, TBF_REG_HVPS1 + TBF_BASE, 1);
         mDaumSPtr->write(&ps2, TBF_REG_HVPS2 + TBF_BASE, 1);
         hv_i = hv_i+1;
         usleep(1*1000);
     }
     usleep(5000);
     LOGD("Reverb set HV to +/- 20V");
   }
}


//
// Do prep pings
//
void DauArrayTest::DoPings(uint32_t pings)
{
    TxDummy();
    uint32_t val = 0x0;
    for (int p = 0; p < pings; p +=1)
    {
       mDaumSPtr->write(&val, TBF_TRIG_LS + TBF_BASE,1);
       usleep(10*1000); // 10mS delay per ping
    }
}

//
// TX SW trigger
//
void DauArrayTest::TxSWTrig()
{
    //
    // TX chip SW TX trigger
    //
    uint32_t spiheader = 0x00201008;
    uint32_t val = 0x00020000; // 2 word burst write
    mDaumSPtr->write(&val, TBF_REG_HVSPI0 + TBF_BASE,1);
    for (int tc = 0; tc < tx_chips; tc += 1) // TX chips
    {
        val = spiheader + (tc << 16);
        mDaumSPtr->write(&val, TBF_LUT_MSSTTXC + TBF_BASE,1);

        val = 0x00400003;
        mDaumSPtr->write(&val, TBF_LUT_MSSTTXC + 0x4 + TBF_BASE,1);
        val = 0x00000000;
        mDaumSPtr->write(&val, TBF_TRIG_HVSPI + TBF_BASE,1);
        usleep(100); // Wait for completion of SPI writes

        val = 0x00400002;
        mDaumSPtr->write(&val, TBF_LUT_MSSTTXC + 0x4 + TBF_BASE,1);
        val = 0x00000000;
        mDaumSPtr->write(&val, TBF_TRIG_HVSPI + TBF_BASE,1);
        usleep(100); // Wait for completion of SPI writes
    }
}

//
// TX delay
//
void DauArrayTest::TxDelayConfig()
{
    uint32_t val = 0x00110000;
    mDaumSPtr->write(&val, TBF_REG_HVSPI0 + TBF_BASE,1);
    uint32_t spiheader = 0x0020100A;
    // Fill 16 delays values in lut
    for (int tc = 0; tc < tx_chips; tc += 1) // TX chips
    {
        val = spiheader + (tc << 16);
        mDaumSPtr->write(&val, TBF_LUT_MSSTTXC + TBF_BASE, 1);
        for (int t = 0; t < 16; t += 1) // Channel delay
        {
            if (t == 15) {
                val = 0x00400004; // Burst end
            } else {
                val = 0x00000004;
            }
            mDaumSPtr->write(&val, TBF_LUT_MSSTTXC + 4 * t + 4 + TBF_BASE, 1);
        }
        val = 0x00000000;
        mDaumSPtr->write(&val, TBF_TRIG_HVSPI + TBF_BASE,1);
        usleep(100); // Wait for completion of SPI writes
    }
}

//
// TX waveform
//
void DauArrayTest::TxWaveformConfig()
{
    uint32_t val = 0x000E0000; // 14 words for each SPI burst write
    mDaumSPtr->write(&val, TBF_REG_HVSPI0 + TBF_BASE,1);
    uint32_t spiheader = 0x00200000;
    for (int c = 0; c < tx_chips; c += 1) // TX chips
    {
        val = spiheader + (c << 16);
        mDaumSPtr->write(&val, TBF_LUT_MSSTTXC + TBF_BASE,1);
        for (int t = 0; t < 13; t += 1)
            {
                val = bistwf[t];
                mDaumSPtr->write(&val, TBF_LUT_MSSTTXC + 4*t + 4 + TBF_BASE,1);
            }
        val = 0x00000000;
        mDaumSPtr->write(&val, TBF_TRIG_HVSPI + TBF_BASE,1);
        usleep(100); // Wait for completion of SPI writes
    }
}

//
// Dummy PRI TX config write to chip 0 register 0x1000
//
void DauArrayTest::TxDummy()
{
    uint32_t val = 0x00201000;
    mDaumSPtr->write(&val, TBF_LUT_MSSTTXC + TBF_BASE,1);
    val = 0x00400000;
    mDaumSPtr->write(&val, TBF_LUT_MSSTTXC + 0x4 + TBF_BASE,1);
    val = 0x00020000; // 2 words for each SPI burst write
    mDaumSPtr->write(&val, TBF_REG_TXLUT + TBF_BASE,1);
}



//
// TX chip base config
//
void DauArrayTest::TxBaseConfig()
{
    uint32_t val = 0x000B0000; // 11 words for each SPI burst write
    mDaumSPtr->write(&val, TBF_REG_HVSPI0 + TBF_BASE,1);
    uint32_t spiheader = 0x00201000;
    for (int c = 0; c < tx_chips; c += 1) // TX chips
    {
        // Set device ID
        val = spiheader + (c << 16);
        mDaumSPtr->write(&val, TBF_LUT_MSSTTXC + TBF_BASE,1);
        val = 0x00000000; // REVERB_BASE_CONFIG1 word 0
        mDaumSPtr->write(&val, TBF_LUT_MSSTTXC + 0x4 + TBF_BASE,1);
        val = 0x00000000; // REVERB_BASE_CONFIG1 word 1
        mDaumSPtr->write(&val, TBF_LUT_MSSTTXC + 0x8 + TBF_BASE,1);
        val = 0x0000030C; // REVERB_BASE_CONFIG1 word 2
        mDaumSPtr->write(&val, TBF_LUT_MSSTTXC + 0xC + TBF_BASE,1);
        val = 0x000000F7; // REVERB_BASE_CONFIG1 word 3
        mDaumSPtr->write(&val, TBF_LUT_MSSTTXC + 0x10 + TBF_BASE,1);
        val = 0x00000000; // REVERB_BASE_CONFIG1 word 4
        mDaumSPtr->write(&val, TBF_LUT_MSSTTXC + 0x14 + TBF_BASE,1);
        val = 0x0000000A; // REVERB_BASE_CONFIG1 word 5
        mDaumSPtr->write(&val, TBF_LUT_MSSTTXC + 0x18 + TBF_BASE,1);
        val = 0x00000000; // REVERB_BASE_CONFIG1 word 6
        mDaumSPtr->write(&val, TBF_LUT_MSSTTXC + 0x1C + TBF_BASE,1);
        val = 0x00000000; // REVERB_BASE_CONFIG1 word 7
        mDaumSPtr->write(&val, TBF_LUT_MSSTTXC + 0x20 + TBF_BASE,1);
        val = 0x00002002; // REVERB_BASE_CONFIG1 word 8
        mDaumSPtr->write(&val, TBF_LUT_MSSTTXC + 0x24 + TBF_BASE,1);
        val = 0x00401E00; // REVERB_BASE_CONFIG1 word 9
        mDaumSPtr->write(&val, TBF_LUT_MSSTTXC + 0x28 + TBF_BASE,1);

        val = 0x00000000;
        mDaumSPtr->write(&val, TBF_TRIG_HVSPI + TBF_BASE,1);
        usleep(100); // Wait for completion of SPI writes
    }
}

//
// Select TX aperture
//
void DauArrayTest::TxAperture(uint32_t txstart)
{
    // Configure TX chip TX and RX enable bits
    // Only aperture [63:0] and [127:64] available for linear
    uint32_t val = 0x00030000; // 3 words for each SPI burst write
    mDaumSPtr->write(&val, TBF_REG_HVSPI0 + TBF_BASE,1);
    uint32_t spiheader = 0x00201006;

    if (probetype==PROBE_TYPE_LINEAR) {
        if (txstart == 0) {
            // Enbale TX/RX for elements [63:0]
            val = 0x000000FF; // Register 0x1006
            mDaumSPtr->write(&val, TBF_LUT_MSSTTXC + 0x4 + TBF_BASE, 1);
            val = 0x004000FF; // Register 0x1007
            mDaumSPtr->write(&val, TBF_LUT_MSSTTXC + 0x8 + TBF_BASE, 1);
        }
        if (txstart == 64) {
            // Enbale TX/RX for elements [127:64]
            val = 0x0000FF00; // Register 0x1006
            mDaumSPtr->write(&val, TBF_LUT_MSSTTXC + 0x4 + TBF_BASE, 1);
            val = 0x0040FF00; // Register 0x1007
            mDaumSPtr->write(&val, TBF_LUT_MSSTTXC + 0x8 + TBF_BASE, 1);
        }
    }
    else {
        val = 0x0000FFFF; // Register 0x1006
        mDaumSPtr->write(&val, TBF_LUT_MSSTTXC + 0x4 + TBF_BASE, 1);
        val = 0x0040FFFF; // Register 0x1007
        mDaumSPtr->write(&val, TBF_LUT_MSSTTXC + 0x8 + TBF_BASE, 1);
    }
    for (int c = 0; c < tx_chips; c += 1) // TX chips
    {
        // Set device ID
        val = spiheader + (c << 16);
        mDaumSPtr->write(&val, TBF_LUT_MSSTTXC + TBF_BASE,1);

        val = 0x00000000;
        mDaumSPtr->write(&val, TBF_TRIG_HVSPI + TBF_BASE,1);
        usleep(100); // Wait for completion of SPI writes
    }
}

uint32_t DauArrayTest::CheckRmsLevel(uint32_t efirst)
{
    uint32_t dc_samples = 128;   // Samples used for ADC DC offset calculation
    uint32_t dc_start   = 512;   // Starting sample used for ADC DC offset calculation
    uint32_t samples    = 64;    // Actual samples used for RMS calc. starting from samples 0

    int32_t dc_dmemche = 0;
    int32_t dc_dmemcho = 0;
    int32_t dmemch      [64][64] = {0}; // DMEM channel pair
    int32_t dmem_sum    [64] = {0};
    float dmem_fps_av   [64] = {0};     // Average of squared values
    float dmem_rms      [64] = {0};     // RMS values
    bool chpass         [64] = {true}; // Channel pass/fail status
    uint32_t dmem_val        = 0;
    float dmem_average  [64] = {0.0};
    uint32_t error_count     = 0;



    for (int i = 0; i < 32; i += 1) // Channel pairs
    {
        //
        // ADC DC offset calculation
        //
            for (int j = 0; j < dc_samples; j += 1) // Delay memory samples
            {
            mDaumSPtr->read(BF_DMEM_MEM_START_ADDR + 4*(j+dc_start) +1024*4*i, (uint32_t *)&dmem_val, 1);
            // Even/odd chanels of 16 bits in each 32 bit word, 2s compliment
            dc_dmemche      = ((int32_t)(dmem_val & 0x0000FFFF) << 16)>>16;
            dc_dmemcho      = ((int32_t)(dmem_val & 0xFFFF0000))>>16;
            dmem_sum[i*2]   += dc_dmemche;
            dmem_sum[i*2+1] += dc_dmemcho;
            }
        dmem_average[i*2]   = (float)(dmem_sum[i*2])/(float)(dc_samples);
        dmem_average[i*2+1] = (float)(dmem_sum[i*2+1])/(float)(dc_samples);
        //
        // Waveform RMS calculation
        //
        for (int j = 0; j < samples; j += 1) // Delay memory samples
        {
            mDaumSPtr->read(BF_DMEM_MEM_START_ADDR + 4*j +1024*4*i, (uint32_t *)&dmem_val, 1);
            // Even/odd chanels of 16 bits in each 32 bit word, 2s compliment
            dmemch[i*2][j]      = ((int32_t)(dmem_val & 0x0000FFFF) << 16)>>16;
            dmemch[i*2+1][j]    = ((int32_t)(dmem_val & 0xFFFF0000))>>16;
        }
        for (int j = 0; j < samples; j += 1)
        {
            // Even channel
            dmem_fps_av[i*2]   += pow((float)(dmemch[i*2][j]-dmem_average[i*2]),2);
            // Odd channel
            dmem_fps_av[i*2+1] += pow((float)(dmemch[i*2+1][j]-dmem_average[i*2+1]),2);
        }
        dmem_rms[i*2]   = sqrt(dmem_fps_av[i*2]/(float)samples);   // Even channel
        dmem_rms[i*2+1] = sqrt(dmem_fps_av[i*2+1]/(float)samples); // Odd channel
        //
        // Check RMS bounds and log results
        //
        if ((dmem_rms[i*2] > mRmsHigh) || (dmem_rms[i*2] < mRmsLow))
        {
            chpass[i*2] = false;
            LOGD("Reverb BIST fail: element %d RMS value %f",efirst+i*2,dmem_rms[i*2]);
            error_count += 1;
        }
        else
        {
            LOGD("Reverb BIST pass: element %d RMS value %f",efirst+i*2,dmem_rms[i*2]);
        }

        if ((dmem_rms[i*2+1]>mRmsHigh) || (dmem_rms[i*2+1]<mRmsLow))
        {
            chpass[i*2+1] = false;
            LOGD("Reverb BIST fail: element %d RMS value %f",efirst+i*2+1,dmem_rms[i*2+1]);
            error_count += 1;
        }
        else
        {
            LOGD("Reverb BIST pass: element %d RMS value %f",efirst+i*2+1,dmem_rms[i*2+1]);
        }
    }
    // High and low RMS levels for log
    for (int i = 0; i < 64; i += 1) // Elements
    {
        if (dmem_rms[i] < rms_element_min)
        {
            rms_element_min = dmem_rms[i];
        }
        if (dmem_rms[i] > rms_element_max)
        {
            rms_element_max = dmem_rms[i];
        }
    }
    return (error_count);
}
