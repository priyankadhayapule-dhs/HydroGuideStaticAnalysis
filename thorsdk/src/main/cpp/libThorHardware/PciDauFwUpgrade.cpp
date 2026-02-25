//
// Copyright (C) 2017 EchoNous, Inc.
//

#define LOG_TAG "PciDauFwUpgrade"

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <CriticalSection.h>
#include <PciDauFwUpgrade.h>
#include <ThorUtils.h>
#include <PciDauMemory.h>
#include <DauRegisters.h>

#define REG_READ_SLEEP  10

static SpiFlash *gflash;
SpiFlashInfo gSpiFlashIds[] =
        {
                {"n25q512a", 0x20bb20, 0x0, 64 * 1024, 1024, 0x100, (RD_FULL | WR_QPP | E_FSR |
                                                                     SECT_4K)},
                {"n25q256a", 0x20bb19, 0x0, 32 * 1024, 1024, 0x100, (RD_FULL | WR_QPP | E_FSR |
                                                                     SECT_4K)},
                {}
        };

const char* const PciDauFwUpgrade::DEFAULT_BIN_FILE = "FwUpgrade.bin";
const char* const PciDauFwUpgrade::DEFAULT_BL_BIN_FILE = "BLFwUpgrade.bin";

//-----------------------------------------------------------------------------
PciDauFwUpgrade::PciDauFwUpgrade() : mDauContextPtr(nullptr), mIsInitialize(false)
{
    ALOGD("*** PciDauFwUpgrade - constructor\n");
}

//-----------------------------------------------------------------------------
PciDauFwUpgrade::~PciDauFwUpgrade()
{
    ALOGD("*** PciDauFwUpgrade - destructor\n");
    free(gflash);
}

//-----------------------------------------------------------------------------
bool PciDauFwUpgrade::init(DauContext *dauContextPtr)
{
    int ret = 0;

    dauMem.selectBar(2);
    dauMem.useInternalAddress(true);

    if (!dauMem.map()) {
        ALOGE("PciDauMemory Mapping Failed");
        return false;
    }

    ret = spiFlashInit();
    if (ret < 0)
    {
        ALOGE("Flash Init Failed\n");
        return false;
    }

    mIsInitialize = true;
    return true;
}

//-----------------------------------------------------------------------------
bool PciDauFwUpgrade::deinit()
{
    if(mIsInitialize) {
        int ret = -1;
        uint32_t reg;
        ret = ftSpi020SwitchSlavePort(XIP_SLAVE_PORT);
        if (ret < 0) {
            ALOGE("Failed to switch on XIP mode");
            return false;
        }
    }
    mIsInitialize = false;
    return true;
}

//-----------------------------------------------------------------------------
int PciDauFwUpgrade::ftSpi020ReadReg(uint8_t cmd, uint32_t *val)
{
    uint32_t reg, spiTimeout;
    int ret = 0;

    /* clear interrupt_status */
    reg = ISR_CMD;
    dauMem.write(&reg, FTSPI_ISR_ADDR, 1);

    /* issue command */
    reg = 0;
    dauMem.write(&reg, FTSPI_CMD_W0_ADDR, 1);
    reg = CMD1_ILEN(1);
    dauMem.write(&reg, FTSPI_CMD_W1_ADDR, 1);
    reg = 1;
    dauMem.write(&reg, FTSPI_CMD_W2_ADDR, 1);
    reg = (CMD3_OPC(cmd) | CMD3_CS0 | CMD3_CMDIRQ);
    dauMem.write(&reg, FTSPI_CMD_W3_ADDR, 1);

    /* wait until rx ready */
    spiTimeout = 0XFFFFFFF;
    do
    {
        dauMem.read(FTSPI_SR_ADDR, &reg, 1);
        usleep(REG_READ_SLEEP);
    } while ((!(reg & SR_RFR)) && --spiTimeout);

    if (spiTimeout == 0)
    {
        ALOGE("[%s] %d:SPI Device Connection Timeout", __func__, __LINE__);
        ret = -1;
        goto err_ret;
    }
    dauMem.read(FTSPI_DR_ADDR, (uint32_t *) &val, 1);

    /* wait until command finish */
    spiTimeout = 0XFFFFFFF;
    do
    {
        reg = dauMem.read(FTSPI_ISR_ADDR, &reg, 1);
        usleep(REG_READ_SLEEP);
    } while ((!(reg & ISR_CMD)) && --spiTimeout);

    if (spiTimeout == 0)
    {
        ALOGE("[%s] %d:SPI Device Connection Timeout", __func__, __LINE__);
        ret = -1;
        goto err_ret;
    }

    /* clear isr */
    reg = ISR_CMD;
    dauMem.write(&reg, FTSPI_ISR_ADDR, 1);

    err_ret:
    return ret;
}

//-----------------------------------------------------------------------------
int PciDauFwUpgrade::ftSpi020ReadBar(SpiFlash *flash)
{
    uint32_t curr_bank = 0;
    uint8_t cmd;
    int ret = 0;

    if (flash->size <= SPI_FLASH_16MB_BOUND)
        goto bar_end;

    cmd = flash->bank_read_cmd;
    ftSpi020ReadReg(cmd, &curr_bank);

    bar_end:
    flash->bank_curr = (curr_bank & 0x03);
    return ret;
}

//-----------------------------------------------------------------------------
int PciDauFwUpgrade::ftSpi020SwitchSlavePort(SpiSlaveId sid) {
    int ret = 0;
    uint32_t reg;
    uint32_t spiTimeout;

    /*  Flush all the command/FIFO and reset the SM */
    reg = CR_ABORT;
    dauMem.write(&reg, FTSPI_CR_ADDR, 1);

    spiTimeout = 0XFFFFFFF;
    do {
        spiTimeout--;
        dauMem.read(FTSPI_CR_ADDR, &reg, 1);
        usleep(REG_READ_SLEEP);
    } while ((reg & CR_ABORT) && spiTimeout);

    if (spiTimeout == 0)
    {
        ALOGE("[%s] %d:SPI Device Connection Timeout", __func__, __LINE__);
        ret = -1;
        goto err_ret;
    }

    /*  Check whether controller is ready for switching */
    spiTimeout = 0XFFFFFFF;
    do
    {
        spiTimeout--;
        dauMem.read(FTSPI_CR_ADDR, &reg, 1);
        usleep(REG_READ_SLEEP);
    } while (((reg & CR_ABORT) | !(reg & CR_XIP_IDLE)) && spiTimeout);

    if (spiTimeout == 0)
    {
        ALOGE("[%s] %d:SPI Device Connection Timeout", __func__, __LINE__);
        ret = -1;
        goto err_ret;
    }

    switch (sid)
    {
        case CMD_SLAVE_PORT:
            dauMem.read(FTSPI_CR_ADDR, &reg, 1);
            reg &= CR_CMD_PORT;
            dauMem.write(&reg, FTSPI_CR_ADDR, 1);
            break;
        case XIP_SLAVE_PORT:
            dauMem.read(FTSPI_CR_ADDR, &reg, 1);
            reg |= CR_XIP_PORT;
            dauMem.write(&reg, FTSPI_CR_ADDR, 1);
            break;
        default:
            ALOGE("Can't Switch Port");
            ret = -1;
            goto err_ret;
    }

    /*  Check whether port is switched or not */
    spiTimeout = 0XFFFFFFF;

    do
    {
        spiTimeout--;
        dauMem.read(FTSPI_CR_ADDR, &reg, 1);
        usleep(REG_READ_SLEEP);
    } while (((reg & CR_ABORT) | !(reg & CR_XIP_IDLE)) && spiTimeout);

    if (spiTimeout == 0)
    {
        ALOGE("[%s] %d:SPI Device Connection Timeout", __func__, __LINE__);
        ret = -1;
        goto err_ret;
    }

    err_ret:
    return ret;
}

//-----------------------------------------------------------------------------
int PciDauFwUpgrade::ftSpi020Init()
{
    int ret = 0;
    uint32_t reg;

    ret = ftSpi020SwitchSlavePort(CMD_SLAVE_PORT);
    if (ret < 0)
    {
        ALOGE("SPI Slave port is not switched");
        goto err_ret;
    }

    /*  Clear the interrupt control reg. */
    reg = 0;
    dauMem.write(&reg, FTSPI_ICR_ADDR, 1);

    /*  Clear the interrupt status  */
    reg = ISR_CMD;
    dauMem.write(&reg, FTSPI_ISR_ADDR, 1);

    /*  Set the SPI mode  */
    reg = CR_CLK_MODE_0;
    dauMem.write(&reg, FTSPI_CR_ADDR, 1);

    /*  Set the clock divider */
    reg = CR_CLK_DIVIDER_8;
    dauMem.write(&reg, FTSPI_CR_ADDR, 1);

    /* AC timing: worst trace delay and cs delay */
    reg = 0xF;
    dauMem.write(&reg, FTSPI_ACTR_ADDR, 1);

    err_ret:
    return ret;
}

//-----------------------------------------------------------------------------
int PciDauFwUpgrade::spiFlashInit()
{
    int ret = 0;

    gflash = (SpiFlash *) malloc(sizeof(SpiFlash));
    if (!gflash)
    {
        ALOGE("Mem not allocated for flash");
        ret = -1;
        goto err_ret;
    }

    memset(gflash, 0, sizeof(SpiFlash));

    /*  Initialize the SPI controller   */
    ret = ftSpi020Init();
    if (ret < 0)
    {
        ALOGE("SPI flash is not initialized\n");
        goto err_ret;
    }

    /* Compute the flash size */
    gflash->name = gSpiFlashIds->name;
    gflash->page_size = gSpiFlashIds->page_size;
    gflash->sector_size = gSpiFlashIds->sector_size;
    gflash->size = gflash->sector_size * gSpiFlashIds->n_sectors;

    /* Compute erase sector and command */
    if (gSpiFlashIds->flags & SECT_4K)
    {
        gflash->erase_cmd = CMD_ERASE_4K;
        gflash->erase_size = 4096;
    }
    else
    {
        gflash->erase_cmd = CMD_ERASE_64K;
        gflash->erase_size = gflash->sector_size;
    }

    /* Now erase size becomes valid sector size */
    gflash->sector_size = gflash->erase_size;

    /* Look for read commands */
    gflash->read_cmd = CMD_READ_ARRAY_FAST;

    /* Look for write commands */
    if (gSpiFlashIds->flags & WR_QPP)
        gflash->write_cmd = CMD_QUAD_PAGE_PROGRAM;
    else
        /* Go for default supported write cmd */
        gflash->write_cmd = CMD_PAGE_PROGRAM;

    /* Read dummy_byte: dummy byte is determined based on the
     * dummy cycles of a particular command.
     * Fast commands - dummy_byte = dummy_cycles/8
     * I/O commands- dummy_byte = (dummy_cycles * no.of lines)/8
     * For I/O commands except cmd[0] everything goes on no.of lines
     * based on particular command but incase of fast commands except
     * data all go on single line irrespective of command.
     */
    switch (gflash->read_cmd)
    {
        case CMD_READ_QUAD_IO_FAST:
            gflash->dummy_byte = 2;
            break;
        case CMD_READ_ARRAY_SLOW:
            gflash->dummy_byte = 0;
            break;
        default:
            gflash->dummy_byte = 1;
            break;
    }

    if (gSpiFlashIds->flags & E_FSR)
        gflash->flags |= SNOR_F_USE_FSR;

    /* Configure the BAR - discover bank cmds and read current bank */
    gflash->bank_read_cmd = CMD_EXTNADDR_RDEAR;
    gflash->bank_write_cmd = CMD_EXTNADDR_WREAR;

    ret = ftSpi020ReadBar(gflash);
    if (ret < 0)
    {
        ALOGE("SPI flash read banks failed");
        goto err_ret;
    }

    ALOGD("Detected %s with page size:%d\n", gflash->name, gflash->page_size);
    ALOGD("Erase size:%d flash size:%d\n", gflash->erase_size, gflash->size);

    err_ret:
    return ret;
}

//-----------------------------------------------------------------------------
int PciDauFwUpgrade::ftSpi020WriteReg(uint8_t cmd, uint32_t *val)
{
    uint32_t reg, spiTimeout;
    int ret = 0;

    /* clear interrupt_status */
    reg = ISR_CMD;
    dauMem.write(&reg, FTSPI_ISR_ADDR, 1);

    /* issue command */
    reg = 0;
    dauMem.write(&reg, FTSPI_CMD_W0_ADDR, 1);
    reg = CMD1_ILEN(1);
    dauMem.write(&reg, FTSPI_CMD_W1_ADDR, 1);
    reg = 1;
    dauMem.write(&reg, FTSPI_CMD_W2_ADDR, 1);
    reg = (CMD3_OPC(cmd) | CMD3_CS0 | CMD3_CMDIRQ);
    dauMem.write(&reg, FTSPI_CMD_W3_ADDR, 1);

    /* wait until rx ready */
    spiTimeout = 0XFFFFFFF;
    do
    {
        dauMem.read(FTSPI_ISR_ADDR, &reg, 1);
        usleep(REG_READ_SLEEP);
    } while ((!(reg & SR_TFR)) && --spiTimeout);

    if (spiTimeout == 0)
    {
        ALOGE("[%s] %d:SPI Device Connection Timeout", __func__, __LINE__);
        ret = -1;
        goto err_ret;
    }

    reg = *val;
    dauMem.write(&reg, FTSPI_DR_ADDR, 1);

    /* wait until command finish */
    spiTimeout = 0XFFFFFFF;
    do
    {
        dauMem.read(FTSPI_ISR_ADDR, &reg, 1);
        usleep(REG_READ_SLEEP);
    } while ((!(reg & ISR_CMD)) && --spiTimeout);

    if (spiTimeout == 0)
    {
        ALOGE("[%s] %d:SPI Device Connection Timeout", __func__, __LINE__);
        ret = -1;
        goto err_ret;
    }

    /* clear isr */
    reg = ISR_CMD;
    dauMem.write(&reg, FTSPI_ISR_ADDR, 1);

    err_ret:
    return ret;
}

//-----------------------------------------------------------------------------
int PciDauFwUpgrade::ftSpi020WriteEnable()
{
    uint32_t reg, spiTimeout;
    int ret = 0;

    /* clear interrupt_status */
    reg = ISR_CMD;
    dauMem.write(&reg, FTSPI_ISR_ADDR, 1);

    /* issue command (WE) */
    reg = 0;
    dauMem.write(&reg, FTSPI_CMD_W0_ADDR, 1);
    reg = CMD1_ILEN(1);
    dauMem.write(&reg, FTSPI_CMD_W1_ADDR, 1);
    reg = 0;
    dauMem.write(&reg, FTSPI_CMD_W2_ADDR, 1);
    reg = (CMD3_OPC(OPCODE_WREN) | CMD3_CS0 | CMD3_WRITE | CMD3_CMDIRQ);
    dauMem.write(&reg, FTSPI_CMD_W3_ADDR, 1);

    /* wait until command finish */
    spiTimeout = 0XFFFFFFF;
    do
    {
        dauMem.read(FTSPI_ISR_ADDR, &reg, 1);
        usleep(REG_READ_SLEEP);
    } while ((!(reg & ISR_CMD)) && --spiTimeout);

    if (spiTimeout == 0)
    {
        ALOGE("[%s] %d:SPI Device Connection Timeout", __func__, __LINE__);
        ret = -1;
        goto err_ret;
    }

    /* clear interrupt_status */
    reg = ISR_CMD;
    dauMem.write(&reg, FTSPI_ISR_ADDR, 1);

    err_ret:
    return ret;
}

//-----------------------------------------------------------------------------
uint32_t PciDauFwUpgrade::ftSpi020ReadStatus()
{
    uint32_t st, reg, spiTimeout;

    /* clear interrupt_status */
    reg = ISR_CMD;
    dauMem.write(&reg, FTSPI_ISR_ADDR, 1);

    /* issue command */
    reg = 0;
    dauMem.write(&reg, FTSPI_CMD_W0_ADDR, 1);
    reg = CMD1_ILEN(1);
    dauMem.write(&reg, FTSPI_CMD_W1_ADDR, 1);
    reg = 1;
    dauMem.write(&reg, FTSPI_CMD_W2_ADDR, 1);
    reg = (CMD3_OPC(OPCODE_RDSR) | CMD3_CS0 | CMD3_RDST | CMD3_CMDIRQ);
    dauMem.write(&reg, FTSPI_CMD_W3_ADDR, 1);

    /* wait until rx ready */
    dauMem.read(FTSPI_SPISR_ADDR, &reg, 1);
    st = (reg << 24);

    /* wait until command finish */
    spiTimeout = 0XFFFFFFF;
    do
    {
        dauMem.read(FTSPI_ISR_ADDR, &reg, 1);
        usleep(REG_READ_SLEEP);
    } while ((!(reg & ISR_CMD)) && --spiTimeout);

    if (spiTimeout == 0)
    {
        ALOGE("[%s] %d:SPI Device Connection Timeout", __func__, __LINE__);
        return -1;
    }

    /* clear isr */
    reg = ISR_CMD;
    dauMem.write(&reg, FTSPI_ISR_ADDR, 1);

    return st;
}

//-----------------------------------------------------------------------------
uint32_t PciDauFwUpgrade::ftSpi020ReadFlagStatus()
{
    uint32_t fsr, reg, spiTimeout;

    /* clear interrupt_status */
    reg = ISR_CMD;
    dauMem.write(&reg, FTSPI_ISR_ADDR, 1);

    /* issue command */
    reg = 0;
    dauMem.write(&reg, FTSPI_CMD_W0_ADDR, 1);
    reg = CMD1_ILEN(1);
    dauMem.write(&reg, FTSPI_CMD_W1_ADDR, 1);
    reg = 1;
    dauMem.write(&reg, FTSPI_CMD_W2_ADDR, 1);
    reg = (CMD3_OPC(OPCODE_EBSY) | CMD3_CS0 | CMD3_RDST | CMD3_CMDIRQ);
    dauMem.write(&reg, FTSPI_CMD_W3_ADDR, 1);

    /* wait until rx ready */
    dauMem.read(FTSPI_SPISR_ADDR, &reg, 1);
    fsr = reg;

    /* wait until command finish */
    spiTimeout = 0XFFFFFFF;
    do
    {
        dauMem.read(FTSPI_ISR_ADDR, &reg, 1);
        usleep(REG_READ_SLEEP);
    } while ((!(reg & ISR_CMD)) && --spiTimeout);

    /* clear isr */
    reg = ISR_CMD;
    dauMem.write(&reg, FTSPI_ISR_ADDR, 1);

    return fsr;
}

//-----------------------------------------------------------------------------
uint8_t PciDauFwUpgrade::ftSpi020WrBar(SpiFlash *flash, uint32_t offset)
{
    uint8_t cmd;
    uint32_t bank_sel, spiTimeout;
    bank_sel = offset / SPI_FLASH_16MB_BOUND;

    if (bank_sel == flash->bank_curr)
        goto bar_end;

    /* Send the write enable command    */
    ftSpi020WriteEnable();

    /*  Issue the coammnd and value     */
    cmd = flash->bank_write_cmd;
    ftSpi020WriteReg(cmd, &bank_sel);

    /*  Wait till Write in Progress */
    spiTimeout = 0XFFFFFFF;
    while ((ftSpi020ReadStatus() & STATUS_WIP) && --spiTimeout);

    if (spiTimeout == 0)
    {
        ALOGE("[%s] %d:SPI Device Connection Timeout", __func__, __LINE__);
        return -1;
    }

    /*  Wait till Progg/Erase controller ready  */
    spiTimeout = 0XFFFFFFF;
    if (flash->flags & SNOR_F_USE_FSR)
    {
        while ((!(ftSpi020ReadFlagStatus() & STATUS_PEC)) && --spiTimeout);
        if (spiTimeout == 0)
        {
            ALOGE("[%s] %d:SPI Device Connection Timeout", __func__, __LINE__);
            return -1;
        }
    }

    bar_end:
    flash->bank_curr = bank_sel;
    return flash->bank_curr;
}

//-----------------------------------------------------------------------------
int PciDauFwUpgrade::ftSpi020Erase(uint8_t cmd, uint32_t offset, uint32_t len) {
    uint32_t reg, spiTimeout;
    int ret = 0;

    /* issue command (SE) */
    reg = offset;
    dauMem.write(&reg, FTSPI_CMD_W0_ADDR, 1);
    reg = 0x00;
    dauMem.write(&reg, FTSPI_CMD_W2_ADDR, 1);

    if (offset < 0x1000000)
    {
        reg = (CMD1_ILEN(1) | CMD1_ALEN(3));
        dauMem.write(&reg, FTSPI_CMD_W1_ADDR, 1);

        if (cmd == OPCODE_BE_4K)
        {
            reg = (CMD3_OPC(OPCODE_BE_4K) | CMD3_CS0 | CMD3_WRITE | CMD3_CMDIRQ);
            dauMem.write(&reg, FTSPI_CMD_W3_ADDR, 1);
        }
        else if (cmd == OPCODE_BE_32K)
        {
            reg = (CMD3_OPC(OPCODE_BE_32K) | CMD3_CS0 | CMD3_WRITE | CMD3_CMDIRQ);
            dauMem.write(&reg, FTSPI_CMD_W3_ADDR, 1);
        }
        else if (cmd == OPCODE_SE)
        {
            reg = (CMD3_OPC(OPCODE_SE) | CMD3_CS0 | CMD3_WRITE | CMD3_CMDIRQ);
            dauMem.write(&reg, FTSPI_CMD_W3_ADDR, 1);
        }
        else
        {
            reg = (CMD3_OPC(OPCODE_CHIP_ERASE) | CMD3_CS0 | CMD3_WRITE | CMD3_CMDIRQ);
            dauMem.write(&reg, FTSPI_CMD_W3_ADDR, 1);
        }
    }
    else
    {
        reg = (CMD1_ILEN(1) | CMD1_ALEN(4));
        dauMem.write(&reg, FTSPI_CMD_W1_ADDR, 1);

        if (cmd == OPCODE_BE_4K)
        {
            reg = (CMD3_OPC(OPCODE_BE_4K4) | CMD3_CS0 | CMD3_WRITE | CMD3_CMDIRQ);
            dauMem.write(&reg, FTSPI_CMD_W3_ADDR, 1);
        }
        else if (cmd == OPCODE_BE_32K)
        {
            reg = (CMD3_OPC(OPCODE_BE_32K4) | CMD3_CS0 | CMD3_WRITE | CMD3_CMDIRQ);
            dauMem.write(&reg, FTSPI_CMD_W3_ADDR, 1);
        }
        else if (cmd == OPCODE_SE)
        {
            reg = (CMD3_OPC(OPCODE_SE4) | CMD3_CS0 | CMD3_WRITE | CMD3_CMDIRQ);
            dauMem.write(&reg, FTSPI_CMD_W3_ADDR, 1);
        }
        else
        {
            ALOGE("Command not supported:%x", cmd);
            ret = -1;
            goto err_ret;
        }
    }

    /* wait until command finish */
    spiTimeout = 0XFFFFFFF;
    do
    {
        dauMem.read(FTSPI_ISR_ADDR, &reg, 1);
        usleep(REG_READ_SLEEP);
    } while ((!(reg & ISR_CMD)) && --spiTimeout);

    if (spiTimeout == 0)
    {
        ALOGE("[%s] %d:SPI Device Connection Timeout", __func__, __LINE__);
        return -1;
    }

    /* clear isr */
    reg = ISR_CMD;
    dauMem.write(&reg, FTSPI_ISR_ADDR, 1);

    err_ret:
    return ret;
}

//-----------------------------------------------------------------------------
int PciDauFwUpgrade::spiFlashErase(uint32_t offset, uint32_t len)
{
    int ret = 0;
    uint32_t erase_size;
    uint32_t spiTimeout;
    uint8_t cmd;

    /* Consistency checking */
    if (offset + len > gflash->size)
    {
        ALOGE("ERROR: Off+len:%x flash size:%x", offset + len, gflash->size);
        ret = -1;
        goto err_ret;
    }

    erase_size = gflash->erase_size;
    if (offset % erase_size || len % erase_size)
    {
        ALOGE("SF: Erase offset/length not multiple of erase size\n");
        ret = -1;
        goto err_ret;
    }

    cmd = gflash->erase_cmd;

    while (len)
    {
        ret = ftSpi020WrBar(gflash, offset);
        if (ret < 0)
            break;

        /* Send the write enable command  */
        ftSpi020WriteEnable();

        /*  Issue the coammnd   */
        ftSpi020Erase(cmd, offset, len);

        /*  wait till erase is in progress  */
        spiTimeout = 0XFFFFFFF;
        while ((ftSpi020ReadStatus() & STATUS_WIP) && --spiTimeout);

        if (spiTimeout == 0)
        {
            ALOGE("[%s] %d:SPI Device Connection Timeout", __func__, __LINE__);
            ret = -1;
            goto err_ret;
        }

        /*  wait till PEC is ready  */
        spiTimeout = 0XFFFFFFF;
        if (gflash->flags & SNOR_F_USE_FSR)
        {
            while ((!(ftSpi020ReadFlagStatus() & STATUS_PEC)) && --spiTimeout);
            if (spiTimeout == 0)
            {
                ALOGE("[%s] %d:SPI Device Connection Timeout", __func__, __LINE__);
                ret = -1;
                goto err_ret;
            }
        }

        offset += erase_size;
        len -= erase_size;
    }

    ALOGD("Flash: %d bytes @ %x Erased\n", len, offset);

    if (ret < 0)
    {
        ALOGE("Erase ERROR\n");
        ret = -1;
        goto err_ret;
    }
    else
    {
        ALOGD("Erase OK\n");
    }
    err_ret:
    return ret;
}

//-----------------------------------------------------------------------------
int PciDauFwUpgrade::ftSpi020Write(uint8_t cmd, uint32_t off, uint32_t len, uint32_t *buf) {
    int i, ret = 0;
    uint32_t reg, spiTimeout;

    /* issue command (WE) */
    reg = off;
    dauMem.write(&reg, FTSPI_CMD_W0_ADDR, 1);

    reg = len;
    dauMem.write(&reg,FTSPI_CMD_W2_ADDR,1);

    if (off < 0x1000000)
    {
        reg = (CMD1_ILEN(1) | CMD1_ALEN(3));
        dauMem.write(&reg, FTSPI_CMD_W1_ADDR, 1);

        if (cmd == OPCODE_QUAD_PP)
        {
            reg = (CMD3_OPC(OPCODE_QUAD_PP) | CMD3_CS0 | CMD3_WRITE | CMD3_QUAD | CMD3_CMDIRQ);
            dauMem.write(&reg, FTSPI_CMD_W3_ADDR, 1);
        }
        else if (cmd == OPCODE_PP)
        {
            reg = (CMD3_OPC(OPCODE_PP) | CMD3_CS0 | CMD3_WRITE | CMD3_CMDIRQ);
            dauMem.write(&reg, FTSPI_CMD_W3_ADDR, 1);
        }
        else if (cmd == OPCODE_DUAL_PP)
        {
            reg = (CMD3_OPC(OPCODE_DUAL_PP) | CMD3_CS0 | CMD3_WRITE | CMD3_DUAL | CMD3_CMDIRQ);
            dauMem.write(&reg, FTSPI_CMD_W3_ADDR, 1);
        }
        else
        {
            ALOGE("Unsupported command:%x", cmd);
            ret = -1;
            goto err_ret;
        }
    }
    else
    {
        reg = (CMD1_ILEN(1) | CMD1_ALEN(4));
        dauMem.write(&reg, FTSPI_CMD_W1_ADDR, 1);

        if (cmd == OPCODE_QUAD_PP)
        {
            reg = (CMD3_OPC(OPCODE_QUAD_PP4) | CMD3_CS0 | CMD3_WRITE | CMD3_QUAD | CMD3_CMDIRQ);
            dauMem.write(&reg, FTSPI_CMD_W3_ADDR, 1);
        }
        else if (cmd == OPCODE_PP)
        {
            reg = (CMD3_OPC(OPCODE_PP4) | CMD3_CS0 | CMD3_WRITE | CMD3_CMDIRQ);
            dauMem.write(&reg, FTSPI_CMD_W3_ADDR, 1);
        }
        else
        {
            ALOGE("Unsupported command:%x", cmd);
            ret = -1;
            goto err_ret;
        }
    }

    /* data phase */
    for (i = 0; i < len; i += 4)
    {
        /* wait until tx ready */
        spiTimeout = 0XFFFFFFF;
        do
        {
            dauMem.read(FTSPI_SR_ADDR, &reg, 1);
            usleep(REG_READ_SLEEP);
        } while ((!(reg & SR_TFR)) && --spiTimeout);

        if (spiTimeout == 0)
        {
            ALOGE("[%s] %d:SPI Device Connection Timeout", __func__, __LINE__);
            ret = -1;
            goto err_ret;
        }

        switch (len)
        {
            case 1:
                reg = *(uint8_t *) buf;
                dauMem.write(&reg, FTSPI_DR_ADDR, 1);
                break;
            case 2:
                reg = *(uint16_t *) buf;
                dauMem.write(&reg, FTSPI_DR_ADDR, 1);
                break;
            default:
                reg = *(buf);
                dauMem.write(&reg, FTSPI_DR_ADDR, 1);
                break;
        }
        buf++;
    }

    /* wait until command finish */
    spiTimeout = 0XFFFFFFF;
    do
    {
        dauMem.read(FTSPI_ISR_ADDR, &reg, 1);
        usleep(REG_READ_SLEEP);
    } while ((!(reg & ISR_CMD)) && --spiTimeout);

    if (spiTimeout == 0)
    {
        ALOGE("[%s] %d:SPI Device Connection Timeout", __func__, __LINE__);
        ret = -1;
        goto err_ret;
    }

    /* clear isr */
    reg = ISR_CMD;
    dauMem.write(&reg, FTSPI_ISR_ADDR, 1);

    err_ret:
    return ret;
}

//-----------------------------------------------------------------------------
int PciDauFwUpgrade::spiFlashWrite(uint32_t offset, uint32_t len, uint32_t *writebuf)
{
    uint32_t byte_addr = 0, page_size = 0;
    uint32_t chunk_len = 0, actual, spiTimeout;
    uint8_t cmd;
    int ret = 0;

    /* Consistency checking */
    if (offset + len > gflash->size)
    {
        ALOGE("ERROR: Off+len:%x flash size:%x\n", offset + len, gflash->size);
        ret = -1;
        goto err_ret;
    }
    cmd = gflash->write_cmd;
    page_size = gflash->page_size;

    for (actual = 0; actual < len; actual += chunk_len)
    {
        /*  Write bank reg as per offset range */
        ret = ftSpi020WrBar(gflash, offset);
        if (ret < 0)
            goto err_ret;

        byte_addr = offset % page_size;
        chunk_len = min(len - actual, (page_size - byte_addr));

        /* Send the write enable command  */
        ftSpi020WriteEnable();

        /*  Issue the command   */
        ftSpi020Write(cmd, offset, chunk_len, writebuf);
        writebuf = writebuf + 64;

        /*  wait till write is in progress  */
        spiTimeout = 0XFFFFFFF;
        while ((ftSpi020ReadStatus() & STATUS_WIP) && --spiTimeout);

        if (spiTimeout == 0)
        {
            ALOGE("[%s] %d:SPI Device Connection Timeout", __func__, __LINE__);
            ret = -1;
            goto err_ret;
        }

        /*  wait till PEC is ready  */
        spiTimeout = 0XFFFFFFF;
        if (gflash->flags & SNOR_F_USE_FSR)
        {
            while ((!(ftSpi020ReadFlagStatus() & STATUS_PEC)) && --spiTimeout);
            if (spiTimeout == 0)
            {
                ALOGE("[%s] %d:SPI Device Connection Timeout", __func__, __LINE__);
                ret = -1;
                goto err_ret;
            }
        }
        offset += chunk_len;
    }

    ALOGD("Flash: %d bytes @ %x Written: \n", len, offset);
    if (ret)
    {
        ALOGE("Write ERROR\n");
        goto err_ret;
    }
    else
    {
        ALOGD("Write OK\n");
    }

    err_ret:
    return ret;
}

//-----------------------------------------------------------------------------
int PciDauFwUpgrade::ftSpi020Read(uint8_t cmd, uint32_t off, uint32_t len, uint32_t *buf)
{
    uint32_t i, v, reg, k = 0;
    int ret = 0;
    uint32_t spiTimeout;

    /*  clear the isr   */
    reg = ISR_CMD;
    dauMem.write(&reg, FTSPI_ISR_ADDR, 1);

    /*  Issue the command   */
    reg = off;
    dauMem.write(&reg, FTSPI_CMD_W0_ADDR, 1);

    /*  TODO: Add the logic for dummy cycles calculation */
    if (off < 0x1000000)
    {
        if (cmd == OPCODE_FAST_READ_QUAD)
        {
            reg = (CMD1_ILEN(1) | CMD1_DCYC(8) | CMD1_ALEN(3));
            dauMem.write(&reg, FTSPI_CMD_W1_ADDR, 1);

            reg = len;
            dauMem.write(&reg, FTSPI_CMD_W2_ADDR, 1);

            reg = (CMD3_OPC(OPCODE_FAST_READ_QUAD) | CMD3_CS0 | CMD3_QUAD | CMD3_CMDIRQ);
            dauMem.write(&reg, FTSPI_CMD_W3_ADDR, 1);
        }
        else if (cmd == OPCODE_FAST_READ_DUAL)
        {
            reg = (CMD1_ILEN(1) | CMD1_DCYC(8) | CMD1_ALEN(3));
            dauMem.write(&reg, FTSPI_CMD_W1_ADDR, 1);

            reg = len;
            dauMem.write(&reg, FTSPI_CMD_W2_ADDR, 1);

            reg = (CMD3_OPC(OPCODE_FAST_READ_DUAL) | CMD3_CS0 | CMD3_DUAL | CMD3_CMDIRQ);
            dauMem.write(&reg, FTSPI_CMD_W3_ADDR, 1);
        }
        else if (cmd == OPCODE_FAST_READ)
        {
            reg = (CMD1_ILEN(1) | CMD1_DCYC(8) | CMD1_ALEN(3));
            dauMem.write(&reg, FTSPI_CMD_W1_ADDR, 1);

            reg = len;
            dauMem.write(&reg, FTSPI_CMD_W2_ADDR, 1);

            reg = (CMD3_OPC(OPCODE_FAST_READ) | CMD3_CS0 | CMD3_CMDIRQ);
            dauMem.write(&reg, FTSPI_CMD_W3_ADDR, 1);
        }
        else
        {
            reg = (CMD1_ILEN(1) | CMD1_ALEN(3));
            dauMem.write(&reg, FTSPI_CMD_W1_ADDR, 1);

            reg = len;
            dauMem.write(&reg, FTSPI_CMD_W2_ADDR, 1);

            reg = (CMD3_OPC(OPCODE_NORM_READ) | CMD3_CS0 | CMD3_CMDIRQ);
            dauMem.write(&reg, FTSPI_CMD_W3_ADDR, 1);
        }
    }
    else
    {
        if (cmd == OPCODE_FAST_READ_QUAD)
        {
            reg = (CMD1_ILEN(1) | CMD1_DCYC(8) | CMD1_ALEN(4));
            dauMem.write(&reg, FTSPI_CMD_W1_ADDR, 1);

            reg = len;
            dauMem.write(&reg, FTSPI_CMD_W2_ADDR, 1);

            reg = (CMD3_OPC(OPCODE_FAST_READ4_QUAD) | CMD3_CS0 | CMD3_QUAD | CMD3_CMDIRQ);
            dauMem.write(&reg, FTSPI_CMD_W3_ADDR, 1);
        }
        else if (cmd == OPCODE_FAST_READ_DUAL)
        {
            reg = (CMD1_ILEN(1) | CMD1_DCYC(8) | CMD1_ALEN(4));
            dauMem.write(&reg, FTSPI_CMD_W1_ADDR, 1);

            reg = len;
            dauMem.write(&reg, FTSPI_CMD_W2_ADDR, 1);

            reg = (CMD3_OPC(OPCODE_FAST_READ4_DUAL) | CMD3_CS0 | CMD3_DUAL | CMD3_CMDIRQ);
            dauMem.write(&reg, FTSPI_CMD_W3_ADDR, 1);
        }
        else if (cmd == OPCODE_FAST_READ)
        {
            reg = (CMD1_ILEN(1) | CMD1_DCYC(8) | CMD1_ALEN(4));
            dauMem.write(&reg, FTSPI_CMD_W1_ADDR, 1);

            reg = len;
            dauMem.write(&reg, FTSPI_CMD_W2_ADDR, 1);

            reg = (CMD3_OPC(OPCODE_FAST_READ4) | CMD3_CS0 | CMD3_CMDIRQ);
            dauMem.write(&reg, FTSPI_CMD_W3_ADDR, 1);
        }
        else
        {
            reg = (CMD1_ILEN(1) | CMD1_ALEN(4));
            dauMem.write(&reg, FTSPI_CMD_W1_ADDR, 1);

            reg = len;
            dauMem.write(&reg, FTSPI_CMD_W2_ADDR, 1);

            reg = (CMD3_OPC(OPCODE_NORM_READ4) | CMD3_CS0 | CMD3_CMDIRQ);
            dauMem.write(&reg, FTSPI_CMD_W3_ADDR, 1);
        }
    }

    /*  Data phase  */
    for (i = 0; i < (len & 0xFFFFFFFC);)
    {
        /* wait until rx ready */
        spiTimeout = 0XFFFFFFF;
        do
        {
            dauMem.read(FTSPI_SR_ADDR, &reg, 1);
            usleep(REG_READ_SLEEP);
        } while ((!(reg & SR_RFR)) && --spiTimeout);

        if (spiTimeout == 0)
        {
            ALOGE("[%s] %d:SPI Device Connection Timeout", __func__, __LINE__);
            ret = -1;
            goto err_ret;
        }

        dauMem.read(FTSPI_DR_ADDR, &reg, 1);

        *(buf + k) = reg;
        k++;
        i += 4;
    }

    if (len & 0x03)
    {
        /* wait until rx ready */
        spiTimeout = 0XFFFFFFF;
        do
        {
            dauMem.read(FTSPI_SR_ADDR, &reg, 1);
            usleep(REG_READ_SLEEP);
        } while ((!(reg & SR_RFR)) && --spiTimeout);

        if (spiTimeout == 0)
        {
            ALOGE("[%s] %d:SPI Device Connection Timeout", __func__, __LINE__);
            ret = -1;
            goto err_ret;
        }

        dauMem.read(FTSPI_DR_ADDR, &v, 1);

        for (i = 0; i < (len & 0x03); ++i)
        {
            ((uint8_t *) buf)[i] = ((uint8_t *) &v)[i];
        }
    }

    /* wait until command finish */
    spiTimeout = 0XFFFFFFF;
    do
    {
        dauMem.read(FTSPI_ISR_ADDR, &reg, 1);
        usleep(REG_READ_SLEEP);
    } while ((!(reg & ISR_CMD)) && --spiTimeout);

    if (spiTimeout == 0)
    {
        ALOGE("[%s] %d:SPI Device Connection Timeout", __func__, __LINE__);
        ret = -1;
        goto err_ret;
    }

    /*  clear the isr   */
    reg = ISR_CMD;
    dauMem.write(&reg, FTSPI_ISR_ADDR, 1);

    err_ret:
    return ret;
}

//-----------------------------------------------------------------------------
int PciDauFwUpgrade::spiFlashRead(uint32_t offset, uint32_t len, uint32_t *readbuf)
{
    uint8_t cmd = 0;
    uint32_t remain_len = 0, read_len = 0;
    int bank_sel = 0, ret = 0;

    /* Consistency checking */
    if (offset + len > gflash->size)
    {
        ALOGE("ERROR: Off+len:%x flash size:%x\n", offset + len, gflash->size);
        ret = -1;
        return ret;
    }

    ALOGD("Read: %d bytes @ %x\n", len, offset);
    cmd = gflash->read_cmd;

    while (len)
    {
        ret = ftSpi020WrBar(gflash, offset);
        if (ret < 0)
            break;

        bank_sel = gflash->bank_curr;
        remain_len = (SPI_FLASH_16MB_BOUND * (bank_sel + 1)) - offset;

        if (len < remain_len)
        {
            read_len = len;
        }
        else
        {
            read_len = remain_len;
        }

        ret = ftSpi020Read(cmd, offset, read_len, readbuf);
        if (ret < 0)
        {
            ALOGE("SF: read failed\n");
            break;
        }

        offset += read_len;
        len -= read_len;
        readbuf += read_len;
    }

    ALOGD("Read %d bytes @ %x \n", len, offset);

    if (ret)
    {
        ALOGE("Read ERROR\n");
    }
    else
    {
        ALOGD("Read OK\n");
    }
    return ret;
}

//-----------------------------------------------------------------------------
bool PciDauFwUpgrade::readFactData(uint32_t *rData,uint32_t dauAddress, uint32_t factDataLen)
{
    int ret = 0;

    ret = spiFlashRead(dauAddress, factDataLen, rData);
    if (ret < 0)
    {
        ALOGE("spiFlash Read Failed\n");
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
bool PciDauFwUpgrade::writeFactData(uint32_t *srcAddress, uint32_t dauAddress, uint32_t factDataLen)
{
    uint32_t *srcAddressPtr = srcAddress;
    uint32_t addr = dauAddress;
    int ret = 0;

    ret = spiFlashErase(addr, FACTORY_DATA_SIZE);
    if (ret < 0)
    {
        ALOGE("Flash Erase failed\n");
        return false;
    }

    ret = spiFlashWrite(addr, factDataLen, srcAddressPtr);
    if (ret < 0)
    {
        ALOGE("Flash Write failed\n");
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
bool PciDauFwUpgrade::getFwInfo(FwInfo *fwInfo)
{
    int ret = 0;

    ret = spiFlashRead(FW_INFO_START, sizeof(FwInfo),(uint32_t*) fwInfo);
    if (ret < 0)
    {
        ALOGE("spiFlash Read Failed\n");
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
bool PciDauFwUpgrade::rebootDevice()
{
    if (dauPow.getPowerState() == DauPowerState::On)
    {
        dauPow.powerOff();
        dauPow.powerOn();
    }
    else if (dauPow.getPowerState() == DauPowerState::Off)
    {
        ALOGE("Dau is off. Start the Dau First\n");
    }
    else if (dauPow.getPowerState() == DauPowerState::Reboot)
    {
        ALOGE("Dau is in Reboot State now\n");
    }
    else
    {
        ALOGE("ERROR: Dau State is undefined");
    }
    return true;
}

//-----------------------------------------------------------------------------
uint32_t PciDauFwUpgrade::computeCRC(uint8_t *buf, uint32_t len)
{
    uint32_t crc = 0xffffffff;
    while (len--)
    {
        crc = (crc << 8) ^ crc32Table[((crc >> 24) ^ *buf) & 255];
        buf++;
    }
    return crc;
}

//-----------------------------------------------------------------------------
bool PciDauFwUpgrade::uploadImage(uint32_t *srcAddress, uint32_t dauAddress, uint32_t binSize)
{
    int ret = 0;
    bool retVal = true;
    FwInfo fwInfo;
    ImageHeader *ih;
    uint32_t bufLength = binSize - (sizeof(ImageHeader));
    char flashBuffer[bufLength], verifyBuffer[bufLength];//, tBuff[sizeof(FwInfo)];

    /* Image Header Extracted from Binary File */
    ih = (ImageHeader *) srcAddress;
    /* Copy Binary File Without Image header*/
    memcpy((uint32_t *) flashBuffer, srcAddress + (sizeof(ImageHeader)/sizeof(uint32_t)), bufLength);

    ret = spiFlashErase(dauAddress, sizeof(flashBuffer));
    if (ret < 0)
    {
        ALOGE("Flash Erase failed\n");
        return false;
    }

    /* Write Binary File */
    ret = spiFlashWrite(dauAddress, sizeof(flashBuffer),(uint32_t *) flashBuffer);
    if (ret < 0)
    {
        ALOGE("Flash Write failed\n");
        return false;
    }

    /* ReadBack Binary File to Verify*/
    ret = spiFlashRead(dauAddress, sizeof(verifyBuffer),(uint32_t *)  verifyBuffer);
    if (ret < 0)
    {
        ALOGE("spiFlash Read Failed\n");
        return false;
    }

    /* Compare original & ReadBack Binary File to Verify*/
    if(memcmp(flashBuffer, verifyBuffer, bufLength) == 0)
    {
        getFwInfo(&fwInfo);

        if (dauAddress == FLASH_BASE_ADDRESS)
        {
            fwInfo.bootLoaderVersion.majorV = ih->appVersion.majorV;
            fwInfo.bootLoaderVersion.minorV = ih->appVersion.minorV;
            fwInfo.bootLoaderVersion.patchV = ih->appVersion.patchV;
            ALOGD("Boot-loader\n");
        }
        else
        {
            if (fwInfo.bootUpdateFlag2 == INEXECUTION)
            {
                fwInfo.bootUpdateFlag1 = 1;
                fwInfo.bootUpdateFlag2 = 0;

                fwInfo.app1Version.majorV = ih->appVersion.majorV;
                fwInfo.app1Version.minorV = ih->appVersion.minorV;
                fwInfo.app1Version.patchV = ih->appVersion.patchV;

                ALOGD("App Partition 1\n");
            }
            else if (fwInfo.bootUpdateFlag1 == INEXECUTION)
            {
                fwInfo.bootUpdateFlag2 = 1;
                fwInfo.bootUpdateFlag1 = 0;

                fwInfo.app2Version.majorV = ih->appVersion.majorV;
                fwInfo.app2Version.minorV = ih->appVersion.minorV;
                fwInfo.app2Version.patchV = ih->appVersion.patchV;

                ALOGD("App Partition 2\n");
            }
            else
            {
                ALOGE("Invalid Partition Add: 0x%x\n", dauAddress);
                return false;
            }
            fwInfo.appImgCntr++;
        }

        ret = spiFlashErase(FW_INFO_START, FW_INFO_SIZE);
        if (ret < 0) {
            ALOGE("Flash Erase failed\n");

            return false;
        }

        ret = spiFlashWrite(FW_INFO_START, sizeof(FwInfo), (uint32_t *) &fwInfo);

        if (ret < 0)
        {
            ALOGE("Flash Write failed\n");
            return false;
        }
        ALOGE("Upgraded Successfully\n");
    }
    else
    {
        ALOGE("Fw Upgrade Fail\n");
    }

    return retVal;
}
