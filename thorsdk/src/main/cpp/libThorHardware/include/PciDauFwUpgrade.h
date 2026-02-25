//
// Copyright (C) 2017 EchoNous, Inc.
//

#pragma once

#include <DauFwUpgrade.h>
#include <DauContext.h>
#include <PciDauMemory.h>
#include <DauPower.h>

/************************************************************/
/*!
* \brief SPI Flash 16MB Boundry region
*/
/************************************************************/
#define SPI_FLASH_16MB_BOUND     0x1000000

/************************************************************/
/*!
* \brief SPI Flash Max ID Length
*/
/************************************************************/
#define SPI_FLASH_MAX_ID_LEN    20

/************************************************************/
/*!
* \brief SPI Control Register
*/
/************************************************************/
#define CR_XIP_PORT             (1 << 20)
#define CR_CMD_PORT             ~(1 << 20)
#define CR_READY_LOC_MASK       ~(0x7 << 16)
#define CR_READY_LOC(x)         (((x) & 0x7) << 16)
#define CR_ABORT                (1 << 8)
#define CR_XIP_IDLE             (1 << 7)
#define CR_CLK_MODE_0           0
#define CR_CLK_MODE_3           (1 << 4)
#define CR_CLK_DIVIDER_MASK     ~(3 << 0)
#define CR_CLK_DIVIDER_2        (0 << 0)
#define CR_CLK_DIVIDER_4        (1 << 0)
#define CR_CLK_DIVIDER_6        (2 << 0)
#define CR_CLK_DIVIDER_8        (3 << 0)

/************************************************************/
/*!
* \brief Command 1 Register (Command queue second word)
*/
/************************************************************/
#define CMD1_CREAD              (1 << 28)
#define CMD1_ILEN(x)            (((x) & 0x03) << 24)
#define CMD1_DCYC(x)            (((x) & 0xff) << 16)
#define CMD1_ALEN(x)            ((x) & 0x07)

/************************************************************/
/*!
* \brief Command 3 Register(Command queue fourth word)
*/
/************************************************************/
#define CMD3_OPC(x)             (((x) & 0xff) << 24)
#define CMD3_OPC_CREAD(x)       (((x) & 0xff) << 16)
#define CMD3_CS0                (((0 << 0) & 0x3) << 8)
#define CMD3_CS1                (((1 << 0) & 0x3) << 8)
#define CMD3_CS2                (((2 << 0) & 0x3) << 8)
#define CMD3_CS3                (((3 << 0) & 0x3) << 8)
#define CMD3_SERIAL             (0 << 5)
#define CMD3_DUAL               (1 << 5)
#define CMD3_QUAD               (2 << 5)
#define CMD3_DUAL_IO            (3 << 5)
#define CMD3_QUAD_IO            (4 << 5)
#define CMD3_DTR                (1 << 4)
#define CMD3_RDST_SW            (1 << 3)
#define CMD3_RDST_HW            0
#define CMD3_RDST               (1 << 2)
#define CMD3_WRITE              (1 << 1)
#define CMD3_READ               0
#define CMD3_CMDIRQ             0

/************************************************************/
/*!
* \brief Read commands
*/
/************************************************************/
#define OPCODE_RDSR                 0x05 /* Read status register */
#define OPCODE_NORM_READ            0x03 /* Read (low freq.) */
#define OPCODE_NORM_READ4           0x13 /* Read (low freq., 4 bytes addr) */
#define OPCODE_FAST_READ            0x0b /* Read (high freq.) */
#define OPCODE_FAST_READ4           0x0c /* Read (high freq., 4 bytes addr) */
#define OPCODE_FAST_READ_DUAL       0x3b /* Read (high freq.) */
#define OPCODE_FAST_READ4_DUAL      0x3c /* Read (high freq., 4 bytes addr) */
#define OPCODE_FAST_READ_QUAD       0x6b /* Read (high freq.) */
#define OPCODE_FAST_READ4_QUAD      0x6c /* Read (high freq. 4 bytes addr) */
#define OPCODE_RDID                 0x9f /* Read JEDEC ID */
#define OPCODE_EBSY                 0x70 /* Enable SO to output RY  */
#define OPCODE_READ_DUAL_IO_FAST    0xbb /* Dual I/O Fast Read */
#define OPCODE_READ_QUAD_IO_FAST    0xeb /* Quad I/O Fast Read */

#define CMD_READ_ARRAY_SLOW         OPCODE_NORM_READ
#define CMD_READ_ARRAY_FAST         OPCODE_FAST_READ
#define CMD_READ_DUAL_OUTPUT_FAST   OPCODE_FAST_READ_DUAL
#define CMD_READ_QUAD_OUTPUT_FAST   OPCODE_FAST_READ_QUAD
#define CMD_READ_DUAL_IO_FAST       OPCODE_READ_DUAL_IO_FAST
#define CMD_READ_QUAD_IO_FAST       OPCODE_READ_QUAD_IO_FAST

/************************************************************/
/*!
* \brief SPI Status Register
*/
/************************************************************/
#define SR_RFR                  (1 << 1) /* RX FIFO Ready */
#define SR_TFR                  (1 << 0) /* TX FIFO Ready */

/************************************************************/
/*!
* \brief Interrupt Status Register
*/
/************************************************************/
#define ISR_CMD                 (1 << 0) /* Command Complete/Finish  */

/************************************************************/
/*!
* \brief Erase commands
*/
/************************************************************/
#define OPCODE_BE_4K                0x20 /* Erase 4KiB block */
#define OPCODE_BE_4K4               0x21 /* Erase 4KiB block (4 bytes addr) */
#define OPCODE_BE_32K               0x52 /* Erase 32KiB block */
#define OPCODE_BE_32K4              0x5C /* Erase 32KiB block(4 bytes addr) */
#define OPCODE_CHIP_ERASE           0xc7 /* Erase whole flash chip */
#define OPCODE_SE                   0xd8 /* Sector erase */
#define OPCODE_SE4                  0xdc /* Sector erase (4 bytes addr) */

#define CMD_ERASE_4K                OPCODE_BE_4K
#define CMD_ERASE_32K               OPCODE_BE_32K
#define CMD_ERASE_64K               OPCODE_SE
#define CMD_ERASE_CHIP              OPCODE_CHIP_ERASE

/************************************************************/
/*!
* \brief Write commands
*/
/************************************************************/
#define OPCODE_WREN                 0x06 /* Write enable    */
#define OPCODE_WRDIS                0x04 /* Write Disable   */
#define OPCODE_WRSR                 0x01 /* Write status register 1 byte */
#define OPCODE_PP                   0x02 /* Page program */
#define OPCODE_QUAD_PP              0x32 /* QUAD Page program */
#define OPCODE_DUAL_PP              0xA2 /* Dual Page program */
#define OPCODE_PP4                  0x12 /* Page program (4 bytes addr) */
#define OPCODE_QUAD_PP4             0x34 /* Page program (4 bytes addr) */

#define CMD_WRITE_STATUS            OPCODE_WRSR
#define CMD_PAGE_PROGRAM            OPCODE_PP
#define CMD_DUAL_PAGE_PROGRAM       OPCODE_DUAL_PP
#define CMD_QUAD_PAGE_PROGRAM       OPCODE_QUAD_PP

/************************************************************/
/*!
* \brief Bank addr access commands
*/
/************************************************************/
#define OPCODE_EXTNADDR_WREAR       0xC5 /* Bank register write*/
#define OPCODE_EXTNADDR_RDEAR       0xC8 /* Bank register write*/
#define OPCODE_READ_CONFIG          0x35 /*	Read configuration */

#define CMD_EXTNADDR_WREAR     OPCODE_EXTNADDR_WREAR
#define CMD_EXTNADDR_RDEAR     OPCODE_EXTNADDR_RDEAR

/************************************************************/
/*!
* \brief common status bits
*/
/************************************************************/
#define STATUS_WIP          (1 << 0)
#define STATUS_QEB_WINSPAN  (1 << 1)
#define STATUS_QEB_MXIC     (1 << 6)
#define STATUS_PEC          (1 << 7)
#define SR_BP0              (1 << 2)  /* Block protect 0 */
#define SR_BP1              (1 << 3)  /* Block protect 1 */
#define SR_BP2              (1 << 4)  /* Block protect 2 */

/*****************************************************/
/*!
 * \brief minimum number between two number
 */
/*****************************************************/
#define min(a, b) (a < b ? a : b)

/************************************************************/
/*!
* \enum defined for SPI Slave port
*/
/************************************************************/
typedef enum
{
    CMD_SLAVE_PORT=0,
    XIP_SLAVE_PORT
} SpiSlaveId;

/*****************************************************/
/*!
 * \enum defined for SPI NOR flags
 */
/*****************************************************/
typedef enum
{
    SNOR_F_SST_WR       = (1 << 0),
    SNOR_F_USE_FSR      = (1 << 1),
    SNOR_F_USE_UPAGE    = (1 << 3),
} spi_nor_option_flags;

/**
 * struct SpiFlash - SPI flash structure
 *
 * @name:       Name of SPI flash
 * @shift:      Flash shift useful in dual parallel
 * @flags:      Indication of spi flash flags
 * @size:       Total flash size
 * @page_size:      Write (page) size
 * @sector_size:    Sector size
 * @erase_size:     Erase size
 * @erase_cmd:      Erase cmd 4K, 32K, 64K
 * @read_cmd:       Read cmd - Array Fast, Extn read and quad read.
 * @write_cmd:      Write cmd - page and quad program.
 * @dummy_byte:     Dummy cycles for read operation.
 * return 0 - Success, 1 - Failure
 */

typedef struct {
    const char *name;
    uint8_t shift;
    uint16_t flags;

    uint32_t size;
    uint32_t page_size;
    uint32_t sector_size;
    uint32_t erase_size;
    uint8_t bank_read_cmd;
    uint8_t bank_write_cmd;
    uint8_t bank_curr;
    uint8_t erase_cmd;
    uint8_t read_cmd;
    uint8_t write_cmd;
    uint8_t dummy_byte;
}SpiFlash;

/**
 * struct SpiFlashInfo - SPI flash info structure
 *
 * @name:           Name of SPI flash
 * @id:             ID of the flash device
 * @ext_id:         Extended ID of the flash device
 * @sector_size:    Sector size in Bytes
 * @n_sectors:      No. of the sectors
 * @page_size:      Page size in Bytes
 * @flags:          Flags for SPI operation
 */

typedef struct
{
    const char  *name;
    uint32_t    id;
    uint32_t    ext_id;
    uint32_t    sector_size;
    uint32_t    n_sectors;
    uint32_t    page_size;
    uint32_t    flags;

#define SECT_4K         (1 << 0)  /* CMD_ERASE_4K works uniformly */
#define E_FSR           (1 << 1)  /* use flag status register for */
#define SST_WR          (1 << 2)  /* use SST byte/word programming */
#define WR_QPP          (1 << 3)  /* use Quad Page Program */
#define RD_QUAD         (1 << 4)  /* use Quad Read */
#define RD_DUAL         (1 << 5)  /* use Dual Read */
#define RD_QUADIO       (1 << 6)  /* use Quad IO Read */
#define RD_DUALIO       (1 << 7)  /* use Dual IO Read */
#define RD_FULL         (RD_QUAD | RD_DUAL | RD_QUADIO | RD_DUALIO)
} SpiFlashInfo;

class PciDauFwUpgrade : public DauFwUpgrade {
public: // Methods

    PciDauFwUpgrade();
    ~PciDauFwUpgrade();

    bool     init(DauContext *dauContextPtr);
    bool     deinit();
    bool     getFwInfo(FwInfo* fwInfo);
    bool     uploadImage(uint32_t* srcAddress, uint32_t dauAddress, uint32_t imgLen);
    bool     rebootDevice();
    bool     readFactData(uint32_t* rData,uint32_t dauAddress, uint32_t factDataLen);
    bool     writeFactData(uint32_t* srcAddress, uint32_t dauAddress, uint32_t factDataLen);

    uint32_t computeCRC(uint8_t* buf, uint32_t len);

    static const char* const DEFAULT_BIN_FILE;
    static const char* const DEFAULT_BL_BIN_FILE;

private: // Properties
    DauContext*         mDauContextPtr;
    PciDauMemory        dauMem;
    DauPower            dauPow;
    bool                mIsInitialize;

    /* Flash Initialization */
    int      spiFlashInit();
    int      ftSpi020Init();
    int      ftSpi020SwitchSlavePort(SpiSlaveId sid);
    int      spiFlashReadId();
    int      ftSpi020ReadId(void *);
    int      ftSpi020ReadBar(SpiFlash *flash);
    int      ftSpi020ReadReg(uint8_t cmd, uint32_t *val);

    /* Flash Erase */
    int      spiFlashErase(uint32_t offset, uint32_t len);
    int      ftSpi020Erase(uint8_t cmd, uint32_t offset, uint32_t len);
    int      ftSpi020WriteEnable();
    int      ftSpi020WriteReg(uint8_t cmd, uint32_t *val);
    uint8_t  ftSpi020WrBar(SpiFlash *flash, uint32_t offset);
    uint32_t ftSpi020ReadStatus();
    uint32_t ftSpi020ReadFlagStatus();

    /* Flash Write */
    int      spiFlashWrite(uint32_t offset, uint32_t len, uint32_t *buf);
    int      ftSpi020Write(uint8_t cmd, uint32_t off, uint32_t len, uint32_t *buf);

    /* Flash Read */
    int      spiFlashRead(uint32_t offset, uint32_t len, uint32_t *readbuf);
    int      ftSpi020Read(uint8_t cmd, uint32_t off, uint32_t len, uint32_t *buf);
};