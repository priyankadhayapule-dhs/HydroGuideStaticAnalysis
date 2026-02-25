//
// Copyright 2017 EchoNous Inc.
//
//
#pragma once

#include <cstdint>
#include <memory>
#include <ThorConstants.h>
#include <DauMemory.h>
#include <UpsReader.h>
#include <DmaBuffer.h>
#include <DauHw.h>
#include <BitfieldMacros.h>
#include <DauRegisters.h>
#include <UsbDataHandler.h>

class DauInputManager
{
public:
    typedef struct
    {
        uint32_t index          : 24; // frame index
        uint32_t rsrv1          :  8; // reserved

        uint32_t dataType		:  3; // data type
        uint32_t rsrv0          : 29; // reserved

        uint64_t timeStamp;           // time stamp
    } FrameHeader;

     static const int INPUT_HEADER_SIZE_BYTES = sizeof(FrameHeader);

    static const int B_FRAME_STRIDE_BYTES = \
        (MAX_B_LINES_PER_FRAME * MAX_B_SAMPLES_PER_LINE) + INPUT_HEADER_SIZE_BYTES + \
        512;  // Gaurd band of at least 512 bytes

    static const int COLOR_FRAME_STRIDE_BYTES = \
        (MAX_COLOR_LINES_PER_FRAME * (MAX_COLOR_SAMPLES_PER_LINE*16)) + INPUT_HEADER_SIZE_BYTES + \
        512; // Guard band of at least 512 bytes

    static const int INPUT_FIFO_LENGTH          = 16;
    static const int INPUT_FIFO_LENGTH_LOG2     = 4;  // streaming engine needs fifo length in this form
    static const int SE_FRAMES_LOG2             = 1;  // for setting ping-pong buffer scheme in SE memory
    static const int INPUT_BASE_OFFSET_B        = 0;
    static const int INPUT_BASE_OFFSET_PW       = 0;
    static const int INPUT_BASE_OFFSET_CW       = 0;
    static const int INPUT_BASE_OFFSET_COLOR    = INPUT_BASE_OFFSET_B + INPUT_FIFO_LENGTH*(MAX_B_FRAME_SIZE + 16);
    static const int INPUT_BASE_OFFSET_M        = INPUT_BASE_OFFSET_COLOR;
    static const int INPUT_BASE_OFFSET_ECG      = INPUT_BASE_OFFSET_COLOR + INPUT_FIFO_LENGTH*(MAX_COLOR_FRAME_SIZE*16 + 16);
    static const int INPUT_BASE_OFFSET_DA       = INPUT_BASE_OFFSET_ECG + INPUT_FIFO_LENGTH*(256*16+16);

    /*
     * Streaming Engine buffer size is 1 MBytes. This has to be shared for all active
     * data types. The following constants set the base address for each data type.
     *
     * B is always placed at the bottome.
     *
     * Color base is set with an offset of (256*512 + 16)*2. This means that when color is turned on
     * B frame size can not be greater than 256x512.
     *
     * Assumptions:
     * ===========
     * 0. when Color is on, B is on.
     * 1. when M is on Color is off
     * 2. when PW is on M, Color and CW are off
     * 3. when CW is on all other modes are off.
     */
    static const int SE_BUF_B_BASE        = 0;                                                          // Data Type 0
    static const int SE_BUF_COLOR_BASE    = SE_BUF_B_BASE         + (MAX_B_FRAME_SIZE + 16)*2;          // Data Type 1
    static const int SE_BUF_M_BASE        = SE_BUF_COLOR_BASE;                                          // Data Type 2
    static const int SE_BUF_PW_BASE       = 0;                                                          // Data Type 3
    static const int SE_BUF_CW_BASE       = 0;                                                          // Data Type 4
    static const int SE_BUF_ECG_BASE      = SE_BUF_COLOR_BASE     + (MAX_COLOR_FRAME_SIZE*16 + 16)*2;   // Data Type 6
    static const int SE_BUF_DA_BASE       = SE_BUF_ECG_BASE       + (256*16 + 16)*2;       // Data Type 7
    static const int SE_BUF_I2C_BASE      = SE_BUF_DA_BASE        + (4096*16 + 16)*2;      // Data Type 5 Temp sensors

public:
    DauInputManager(const std::shared_ptr<DauMemory>& daum,
                    const std::shared_ptr<DauMemory>& daumBar2,
                    const std::shared_ptr<UpsReader>& upsReader,
                    const std::shared_ptr<DmaBuffer>& dmaBuffer,
                    const std::shared_ptr<DauHw>& dauHw,
                    const std::shared_ptr<UsbDataHandler>& usbDataHandler,
                    bool detectEcg);
    ~DauInputManager();

    void start();
    bool startDataType( uint32_t dauDataType );

    void stop();
    void stopDataType( uint32_t dauDataType );

    bool setImagingCase(uint32_t imagingCase, int imagingMode, bool isDaOn, bool isEcgOn, bool isUsOn);

    void configureDataTypeBSERegs( uint32_t imagingCase );
    void configureDataTypeColorSERegs( uint32_t imagingCase );
    void configureDataTypePwSERegs();
    void configureDataTypeMSERegs();
    void configureDataTypeCwSERegs();
    void configureDataTypeI2cSERegs();
    ThorStatus configureFCSSwitchRegs(std::shared_ptr<DauMemory> daumSPtr, FCSType fcsVal);
    void configureEcgSERegs();
    void configureDaSERegs();

    bool deinterleaveBData(uint8_t* inputPtr, uint8_t* outputPtr);
    bool deinterleaveMData(uint8_t* inputPtr, uint32_t multiLineSelection, uint8_t* outputPtr);
    uint32_t getBFrameStride() { return (mBFrameStride); }
    uint32_t getCFrameStride() { return (mCFrameStride); }
    uint32_t getMFrameStride() { return (mMFrameStride); }
    uint32_t getPwFrameStride() { return(mPwFrameStride);}
    uint32_t getCwFrameStride() { return(mCwFrameStride);}
    uint32_t getEcgFrameStride() { return(mEcgFrameSize*16 + sizeof(FrameHeader));}
    uint32_t getDaFrameStride() { return(mDaFrameSize*16 + sizeof(FrameHeader));}
    void setBFrameSize(uint32_t samples, uint32_t lines);
    void setColorFrameSize(uint32_t samples, uint32_t lines);
    void setPwFrameSize(uint32_t samplesPerLine, uint32_t linesPerFrame);
    void setCwFrameSize(uint32_t samplesPerLine, uint32_t linesPerFrame);
    void setEcgFrameSize( uint32_t frameSize);
    void setDaFrameSize( uint32_t frameSize);
    void setExternalECG(bool isExternal);
    void setLeadNumber(uint32_t leadNumber);

    void initMotionSensors();
    void startMotionSensors();
    void stopMotionSensors();
    void initDaEkg(bool daIsOn, bool ecgIsOn);
    void startDaEkg();
    void stopDaEkg();
    void openDaEkg();
    void initTempSensors();
    void startTempSensors();
    void stopTempSensors();
    void initCwImaging();
    void startCwImaging();
    void stopCwImaging();


private:
    DauInputManager();

    static const int IRQ_BASE_BITS = BIT(SYS_INT0_MSI_ERR_BIT) |
                                     //BIT(SYS_INT0_SE_ERR_BIT) | // Getting erroneous errors from this
                                     BIT(SYS_INT0_SEQ_ERR_BIT) |
                                     BIT(SYS_INT0_PCIE_ERR0_BIT) |
                                     BIT(SYS_INT0_PCIE_ERR1_BIT) |
                                     BIT(SYS_INT0_PCIE_ERR2_BIT) |
                                     BIT(SYS_INT0_PCIE_ERR3_BIT) |
                                     // BIT(SYS_INT0_PCIE_ERR4_BIT) | // This is commented out to ignore PHY related errors with V2 DAU
                                     BIT(SYS_INT0_PCIE_ERR5_BIT) |
                                     BIT(SYS_INT0_PCIE_ERR6_BIT) |
                                     BIT(SYS_INT0_PMON_BIT) |
                                     BIT(SYS_INT0_TXC_BIT) |
                                     BIT(SYS_INT0_BERR_BIT);

    // These bits are not defined in DauRegisters.h.  They are only specified in
    // the comments of that file.
    static const int IRQ_BMODE_BIT     = BIT(SYS_INT0_SE_FRAME_BIT + 0);
    static const int IRQ_CMODE_BIT     = BIT(SYS_INT0_SE_FRAME_BIT + 1);
    static const int IRQ_PW_BIT        = BIT(SYS_INT0_SE_FRAME_BIT + 2);
    static const int IRQ_MMODE_BIT     = BIT(SYS_INT0_SE_FRAME_BIT + 3);
    static const int IRQ_CW_BIT        = BIT(SYS_INT0_SE_FRAME_BIT + 4);
    static const int IRQ_I2C_BIT       = BIT(SYS_INT0_SE_FRAME_BIT + 5);
    static const int IRQ_ECG_BIT       = BIT(SYS_INT0_SE_FRAME_BIT + 6);
    static const int IRQ_DA_BIT        = BIT(SYS_INT0_SE_FRAME_BIT + 7);

    static const int IRQ2_BASE_BITS    = BIT(SYS_INT1_SERR_BIT);

    static const int IRQ2_EXT_ECG_BIT  = BIT(SYS_INT1_GPIO_BIT + 1);

private:
    uint32_t                            mNumTxLinesB;
    uint32_t                            mMultiLineFactor;
    uint32_t                            mImagingMode;
    uint32_t                            mBFrameStride;
    uint32_t                            mCFrameStride;
    uint32_t                            mMFrameStride;
    uint32_t                            mNumBSamples;
    uint32_t                            mNumBLines;
    uint32_t                            mNumCSamples;
    uint32_t                            mNumCLines;
    uint32_t                            mNumPwSamplesPerLine;
    uint32_t                            mNumPwLinesPerFrame;
    uint32_t                            mPwFrameStride;
    uint32_t                            mCwFrameStride;
    uint32_t                            mNumCwSamplesPerLine;
    uint32_t                            mNumCwLinesPerFrame;
    uint32_t                            mEcgFrameSize;
    uint32_t                            mDaFrameSize;
    std::shared_ptr<DauMemory>          mDaumSPtr;
    std::shared_ptr<DauMemory>          mDaumBar2SPtr;
    std::shared_ptr<UpsReader>          mUpsReaderSPtr;
    std::shared_ptr<DmaBuffer>          mDmaBufferSPtr;
    std::shared_ptr<DauHw>              mDauHwSPtr;
    std::shared_ptr<UsbDataHandler>     mUsbDataHandlerSPtr;
    bool                                mIsExternalECG;
    uint32_t                            mLeadNumber;
    bool                                mDetectEcg;
};
