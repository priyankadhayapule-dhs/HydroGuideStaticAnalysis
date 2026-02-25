//
// Copyright 2018 EchoNous Inc.
//
//

#define LOG_TAG "thorpuck"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/epoll.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <pthread.h>

#include "thorpuck.h"
#include "thorpuck_i2c.h"

#include "cybtldr_parse.h"
#include "cybtldr_api.h"
#include "cybtldr_command.h"
#include "cybtldr_utils.h"

#include "ThorPuckImage.h"

#define THORPUCK_I2C_BUSNUM 9
#define THORPUCK_I2C_EVENT_ADDR (0x58<<1)
#define THORPUCK_I2C_CMD_ADDR (0x59<<1)
#define THORPUCK_I2C_BOOTLOADER_ADDR (0x08<<1)
#define INT_GPIONUM "22"
#define INT_GPIONUM_LEN (3) // strlen(INT_GPIONUM)+1

#ifndef THORPUCK_INT_ACTIVE_HIGH
#define THORPUCK_INT_ACTIVE_HIGH 0
#endif

#if THORPUCK_INT_ACTIVE_HIGH
#define INT_VALUE '1'
#define EDGE_TRIGGER "rising"
#define EDGE_TRIGGER_LEN (7) // strlen(EDGE_TRIGGER)+1
#else
#define INT_VALUE '0'
#define EDGE_TRIGGER "falling"
#define EDGE_TRIGGER_LEN (8) // strlen(EDGE_TRIGGER)+1
#endif


//#define PUCK_DEBUG 1

#ifdef PUCK_DEBUG
#define DEBUG_OUT(...) (printf(__VA_ARGS__));
#else
#define DEBUG_OUT(...) ((void)0)
#endif

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof((a)) / sizeof(((a)[0])))
#endif


typedef enum
{
    SCAN_START          = 0x01,
    SCAN_STOP           = 0x02,
    ENTER_BOOTLOAD      = 0x03,
    BUZZER_TIMER_SET    = 0x04,
    BUZZER_TIMER_GET    = 0x05,
    GET_FW_VERSION      = 0x06,
    PSOC_RESET          = 0x07,
    SINGLE_CLICK_SET    = 0x08,
    SINGLE_CLICK_GET    = 0x09,
    DBL_CLICK_SET       = 0x0A,
    DBL_CLICK_GET       = 0x0B,
    LONG_PRESS_SET      = 0x0C,
    LONG_PRESS_GET      = 0x0D,
    SCROLL_POS_SET      = 0x0E,
    SCROLL_POS_GET      = 0x0F,
    SCROLL_STEP_SET     = 0x10,
    SCROLL_STEP_GET     = 0x11,
    RESET_FACTORY       = 0x12
} HOST_COMMAND_t;

struct thorpuck_s
{
    int i2c_bus;

    pthread_t thread;
    bool thread_running;

    int irq_fd;
    int irq_epfd;
    struct epoll_event irq_ev;

    thorpuck_event_callback_t cb;
    void* priv;
};

//-----------------------------------------------------------------------------
static uint8_t gen_checksum(uint8_t* cmdArray, int len)
{
    uint8_t sum = 0;

    for (int index = 0; index < len; index++)
    {
        sum += cmdArray[index];
    }

    return(0xFF - sum);
}

//-----------------------------------------------------------------------------
static bool validate_checksum(uint8_t* rspArray, int len)
{
    uint8_t sum = 0;

    for (int index = 0; index < len; index++)
    {
        sum += rspArray[index];
    }

    return((sum & 0xFF) == 0xFF);
}

//-----------------------------------------------------------------------------
static bool send_command(thorpuck_t* handle,
                         uint8_t* cmdArray, int cmdLen,
                         uint8_t* rspArray, int rspLen)
{
    bool    retVal = false;

    if(!handle || handle->i2c_bus < 0)
    {
        ALOGE("%s: Failed to access i2c bus", __func__);
        goto err_ret;
    }

    cmdArray[cmdLen - 1] = gen_checksum(cmdArray, cmdLen - 1);

    if(!thorpuck_i2c_write(handle->i2c_bus, THORPUCK_I2C_CMD_ADDR, cmdArray, cmdLen))
    {
        ALOGE("%s: Failed to write command\n", __func__);
        goto err_ret;
    }

    usleep(100000);

    if (NULL != rspArray)
    {
        if(!thorpuck_i2c_read(handle->i2c_bus, THORPUCK_I2C_CMD_ADDR, rspArray, rspLen))
        {
            ALOGE("%s: Failed to read response\n", __func__);
            goto err_ret;
        }

        if (!validate_checksum(rspArray, rspLen))
        {
            ALOGE("%s: Response checksum failed\n", __func__);
            goto err_ret;
        }
    }

    retVal = true;

err_ret:
    return(retVal); 
}

//-----------------------------------------------------------------------------
void* thorpuck_event_thread(void* param)
{
    thorpuck_t* t = (thorpuck_t*)param;

    while(t->thread_running)
    {
        struct epoll_event events;

        // wait for the gpio to have a edge trigger
        int n = epoll_wait(t->irq_epfd, &events, 1, 100);
        if(n < 0)
        {
            ALOGE("Failed epoll_wait: (%d) %s\n", errno, strerror(errno));
            goto err;
        }
        else if(n > 0)
        {
            char v;

            do
            {
                //seek back to beginning
                if(lseek(t->irq_fd, 0, SEEK_SET) < 0)
                {
                    ALOGE("Failed to seek: (%d) %s\n", errno, strerror(errno));
                    goto err;
                }

                //read the gpio value
                if(read(t->irq_fd, &v, 1) <= 0)
                {
                    ALOGE("Failed to read: (%d) %s\n", errno, strerror(errno));
                    goto err;
                }

                //if it is INT_VALUE we have an event pending
                if(v == INT_VALUE)
                {
                    DEBUG_OUT("Puck IRQ\n");

                    //read the pending i2c event
                    uint8_t buf[9];

                    memset(buf, 0xFF, sizeof(buf));

                    if (thorpuck_i2c_read(t->i2c_bus, THORPUCK_I2C_EVENT_ADDR, buf, sizeof(buf)))
                    {
                        if (t->cb)
                        {
                            thorpuck_event_t event;

                            memset(&event, 0, sizeof(event));

                            //decode the i2c packet into local struct for to send
                            //to the user callback
                            DEBUG_OUT("Raw Data: %02X %02X %02X   %02X %02X   %02X %02X %02X %02X\n",
                                    buf[0],
                                    buf[1],
                                    buf[2],
                                    buf[3],
                                    buf[4],
                                    buf[5],
                                    buf[6],
                                    buf[7],
                                    buf[8]);

                            // Left Button Action
                            if(buf[0] != 0)
                            {
                                event.leftButtonAction = buf[0];
                            }

                            // Right Button Action
                            if(buf[1] != 0)
                            {
                                event.rightButtonAction = buf[1];
                            }

                            // Palm Action
                            if(buf[2] != 0)
                            {
                                event.palmAction = buf[2];
                            }

                            // Scroll Action
                            if(buf[3] != 0)
                            {
                                event.scrollAction = buf[3];
                                event.scrollAmount = buf[4];
                            }

                            t->cb(t, t->priv, &event);
                        }
                    }
                }
            } while(v == INT_VALUE);
        }
    }

    return NULL;

err:
    t->thread_running = false;
    return NULL;

}

//-----------------------------------------------------------------------------
unsigned char BootloadStringImage(CyBtldr_CommunicationsData* comm,
                                  const char *bootloadImagePtr[],
                                  unsigned int lineCount )
{
    unsigned char err;
    unsigned char arrayId; 
    unsigned short rowNum;
    unsigned short rowSize; 
    unsigned char checksum ;
    unsigned char checksum2;
    unsigned long blVer=0;
    //rowData buffer size should be equal to the length of data to be send for each flash row Equals 128
    unsigned char rowData[128];
    unsigned int lineLen;
    unsigned long  siliconID;
    unsigned char siliconRev;
    unsigned char packetChkSumType;
    unsigned int lineCntr;
    thorpuck_t* handle = (thorpuck_t*) comm->priv;
    thorpuck_event_callback_t callback = NULL;

    if (handle && handle->cb)
    {
        callback = handle->cb;
    }

    // Initialize line counter
    lineCntr = 0u;

    // Get length of the first line in cyacd file
    lineLen = strlen(bootloadImagePtr[lineCntr]);

    // Parse the first line(header) of cyacd file to extract siliconID, siliconRev and packetChkSumType
    err = CyBtldr_ParseHeader(lineLen ,(unsigned char *)bootloadImagePtr[lineCntr] , &siliconID , &siliconRev ,&packetChkSumType);

    // Set the packet checksum type for communicating with bootloader. The packet checksum type to be used 
    // is determined from the cyacd file header information
    CyBtldr_SetCheckSumType((CyBtldr_ChecksumType)packetChkSumType);

    if(err == CYRET_SUCCESS)
    {
        // Start Bootloader operation
        err = CyBtldr_StartBootloadOperation(comm, siliconID, siliconRev, &blVer);
        lineCntr++ ;
        while((err == CYRET_SUCCESS)&& ( lineCntr <  lineCount ))
        {
            // Get the string length for the line
            lineLen =  strlen(bootloadImagePtr[lineCntr]);
            //Parse row data
            err = CyBtldr_ParseRowData((unsigned int)lineLen,(unsigned char *)bootloadImagePtr[lineCntr], &arrayId, &rowNum, rowData, &rowSize, &checksum);

            if (CYRET_SUCCESS == err)
            {
                // Program Row
                err = CyBtldr_ProgramRow(arrayId, rowNum, rowData, rowSize);
                if (CYRET_SUCCESS == err)
                {
                    // Verify Row . Check whether the checksum received from bootloader matches
                    // the expected row checksum stored in cyacd file
                    checksum2 = (unsigned char)(checksum + arrayId + rowNum + (rowNum >> 8) + rowSize + (rowSize >> 8));
                    err = CyBtldr_VerifyRow(arrayId, rowNum, checksum2);
                }
            }

            // Increment the linCntr
            lineCntr++;

            if (callback)
            {
                thorpuck_event_t event;

                memset(&event, 0, sizeof(event));

                event.updateCurPos = lineCntr;
                event.updateMaxPos = lineCount;
                event.updateStatus = TER_TABLET_PUCK_UPDATE_FIRMWARE_BUSY;

                callback(handle, handle->priv, &event);
            }
        }
        // End Bootloader Operation
        CyBtldr_EndBootloadOperation();
        usleep(500000);
    }

    if (callback)
    {
        thorpuck_event_t event;

        memset(&event, 0, sizeof(event));

        event.updateCurPos = lineCntr;
        event.updateMaxPos = lineCount;

        if (CYRET_SUCCESS == err)
        {
            event.updateStatus = TER_TABLET_PUCK_UPDATE_FIRMWARE_COMPLETE;
        }
        else
        {
            event.updateStatus = TER_TABLET_PUCK_UPDATE_FIRMWARE;
        }
        callback(handle, handle->priv, &event);
    }

    return(err);
}

//-----------------------------------------------------------------------------
unsigned char transferFirmware(CyBtldr_CommunicationsData* comm,
                               FILE* filePtr,
                               unsigned int lineCount )
{
    unsigned char err;
    unsigned char arrayId; 
    unsigned short rowNum;
    unsigned short rowSize; 
    unsigned char checksum ;
    unsigned char checksum2;
    unsigned long blVer=0;
    //rowData buffer size should be equal to the length of data to be send for each flash row Equals 128
    unsigned char rowData[128];
    unsigned long  siliconID;
    unsigned char siliconRev;
    unsigned char packetChkSumType;
    unsigned int lineCntr;
    char* linePtr = NULL;
    size_t lineLen = 0;
    size_t lineCap = 0;
    thorpuck_t* handle = (thorpuck_t*) comm->priv;
    thorpuck_event_callback_t callback = NULL;

    if (handle && handle->cb)
    {
        callback = handle->cb;
    }

    // Initialize line counter
    lineCntr = 0u;

    // Get length of the first line in cyacd file
    lineLen = getline(&linePtr, &lineCap, filePtr);
    linePtr[lineLen - 2] = '\0';    // Remove carriage-return / linefeed chars
    lineLen -= 2;

    // Parse the first line(header) of cyacd file to extract siliconID, siliconRev and packetChkSumType
    err = CyBtldr_ParseHeader(lineLen ,(unsigned char *)linePtr, &siliconID , &siliconRev ,&packetChkSumType);

    // Set the packet checksum type for communicating with bootloader. The packet checksum type to be used 
    // is determined from the cyacd file header information
    CyBtldr_SetCheckSumType((CyBtldr_ChecksumType)packetChkSumType);

    if(err == CYRET_SUCCESS)
    {
        // Start Bootloader operation
        err = CyBtldr_StartBootloadOperation(comm, siliconID, siliconRev, &blVer);
        lineCntr++ ;
        while((err == CYRET_SUCCESS)&& ( lineCntr <  lineCount ))
        {
            // Get the string length for the line
            lineLen = getline(&linePtr, &lineCap, filePtr);
            linePtr[lineLen - 2] = '\0';
            lineLen -= 2;

            //Parse row data
            err = CyBtldr_ParseRowData((unsigned int) lineLen,
                                       (unsigned char*) linePtr,
                                       &arrayId,
                                       &rowNum,
                                       rowData,
                                       &rowSize,
                                       &checksum);

            if (CYRET_SUCCESS == err)
            {
                // Program Row
                err = CyBtldr_ProgramRow(arrayId, rowNum, rowData, rowSize);
                if (CYRET_SUCCESS == err)
                {
                    // Verify Row . Check whether the checksum received from bootloader matches
                    // the expected row checksum stored in cyacd file
                    checksum2 = (unsigned char)(checksum + arrayId + rowNum + (rowNum >> 8) + rowSize + (rowSize >> 8));
                    err = CyBtldr_VerifyRow(arrayId, rowNum, checksum2);
                }
            }

            // Increment the linCntr
            lineCntr++;

            if (callback)
            {
                thorpuck_event_t event;

                memset(&event, 0, sizeof(event));

                event.updateCurPos = lineCntr;
                event.updateMaxPos = lineCount;
                event.updateStatus = TER_TABLET_PUCK_UPDATE_FIRMWARE_BUSY;

                callback(handle, handle->priv, &event);
            }
        }
        // End Bootloader Operation
        CyBtldr_EndBootloadOperation();
        usleep(500000);
    }

    if (NULL != linePtr)
    {
        free(linePtr);
        linePtr = NULL;
    }

    if (callback)
    {
        thorpuck_event_t event;

        memset(&event, 0, sizeof(event));

        event.updateCurPos = lineCntr;
        event.updateMaxPos = lineCount;

        if (CYRET_SUCCESS == err)
        {
            event.updateStatus = TER_TABLET_PUCK_UPDATE_FIRMWARE_COMPLETE;
        }
        else
        {
            event.updateStatus = TER_TABLET_PUCK_UPDATE_FIRMWARE;
        }
        callback(handle, handle->priv, &event);
    }

    return (err);
}

//-----------------------------------------------------------------------------
static bool check_needs_update(thorpuck_t* t)
{
    uint8_t cmd[6];
    uint8_t rsp[7] = {0,0,0,0,0,0,0};

    if(!t || t->i2c_bus < 0)
    {
        return false;
    }

    cmd[0] = 0x00;
    cmd[1] = 0x00;
    cmd[2] = GET_FW_VERSION;
    cmd[3] = 0x00;
    cmd[4] = 0x00;
    cmd[5] = 0xFF-GET_FW_VERSION;

    if(!thorpuck_i2c_write(t->i2c_bus, THORPUCK_I2C_CMD_ADDR, cmd, sizeof(cmd)))
    {
        ALOGE("failed to write cmd\n");
        return false;
    }

    usleep(100000);

    if(!thorpuck_i2c_read(t->i2c_bus, THORPUCK_I2C_CMD_ADDR, rsp, sizeof(rsp)))
    {
        ALOGE("failed to read rsp\n");
        return false;
    }

    if(((rsp[0]+rsp[1]+rsp[2]+rsp[3]+rsp[4]+rsp[5]+rsp[6])&0xFF) != 0xFF)
    {
        ALOGE("rsp checksum failed\n");
        return false;
    }

    if(rsp[0] != GET_FW_VERSION)
    {
        ALOGE("rsp type wrong\n");
        return false;
    }

    uint16_t len = (rsp[2] << 8) | rsp[1];
    if(len < 3)
    {
        ALOGE("rsp len too small\n");
        return false;
    }

    if(rsp[3] == Firmware_Version[0] && rsp[4] == Firmware_Version[1] && rsp[5] == Firmware_Version[2])
    {
        ALOGD("ThorPuck v%c%c%c up to date\n", rsp[3], rsp[4], rsp[5]);
        return false;
    }
    
    ALOGD("Update available for ThorPuck v%c%c%c => v%c%c%c\n",
	  rsp[3], rsp[4], rsp[5],
	  Firmware_Version[0], Firmware_Version[1], Firmware_Version[2]);
    return true;
}

//-----------------------------------------------------------------------------
static int OpenConnection(void* priv)
{
    UNUSED(priv);

    return(CYRET_SUCCESS);
}

//-----------------------------------------------------------------------------
static int CloseConnection(void* priv)
{
    UNUSED(priv);

    return(CYRET_SUCCESS);
}

//-----------------------------------------------------------------------------
static int ReadData(void* priv, unsigned char* rdData, int byteCnt)
{
    thorpuck_t* t = (thorpuck_t*)priv;
    if(!t || t->i2c_bus < 0)
    {
        return CYRET_ABORT;
    }

    if(!thorpuck_i2c_read(t->i2c_bus, THORPUCK_I2C_BOOTLOADER_ADDR, rdData, byteCnt))
    {
        return CYRET_ABORT;
    }

    return(CYRET_SUCCESS);
}

//-----------------------------------------------------------------------------
static int WriteData(void* priv, unsigned char* wrData, int byteCnt)
{
    thorpuck_t* t = (thorpuck_t*)priv;
    if(!t || t->i2c_bus < 0)
    {
        return CYRET_ABORT;
    }

    if(!thorpuck_i2c_write(t->i2c_bus, THORPUCK_I2C_BOOTLOADER_ADDR, wrData, byteCnt))
    {
        return CYRET_ABORT;
    }

    usleep(25000);

    return(CYRET_SUCCESS);
	
}

//-----------------------------------------------------------------------------
static bool do_update(thorpuck_t* t)
{
    CyBtldr_CommunicationsData comm;
    uint8_t cmd[6];

    comm.OpenConnection = &OpenConnection;
    comm.CloseConnection = &CloseConnection;
    comm.ReadData = &ReadData;
    comm.WriteData = &WriteData;
    comm.MaxTransferSize = 64u;
    comm.priv = t;

    if (t->thread_running)
    {
        ALOGD("Entering bootloader mode\n");

        // Send command to psoc to get into bootloader mode
        cmd[0] = 0x00;
        cmd[1] = 0x00;
        cmd[2] = ENTER_BOOTLOAD;
        cmd[3] = 0x00;
        cmd[4] = 0x00;
        cmd[5] = 0xFF-ENTER_BOOTLOAD;
        if(!thorpuck_i2c_write(t->i2c_bus, THORPUCK_I2C_CMD_ADDR, cmd, sizeof(cmd)))
        {
            return false;
        }

        usleep(1000000);
    }

    ALOGD("Updating FW\n");

    // Call BootloadStringImage function to bootload stringImage_0 application
    return (BootloadStringImage(&comm, stringImage_0,LINE_CNT_0 ) == CYRET_SUCCESS);
}

//-----------------------------------------------------------------------------
thorpuck_open_code_t thorpuck_open(puck_context_t* contextPtr,
                                   thorpuck_event_callback_t cb,
                                   void* priv,
                                   bool bootloaderMode,
                                   thorpuck_t** handlePtrPtr)
{
    thorpuck_open_code_t retCode = THORPUCK_OPEN_OK;
    thorpuck_t* t = malloc(sizeof(thorpuck_t));
    if(t)
    {
        int fd;

        memset(t, 0, sizeof(thorpuck_t));
        t->i2c_bus = -1;
        t->irq_fd = -1;
        t->irq_epfd = -1;

        t->cb = cb;
        t->priv = priv;

        // Open the i2c bus for communication
        if (NULL != contextPtr)
        {
            t->i2c_bus = dup(contextPtr->i2cFd);
            t->irq_fd = dup(contextPtr->irqFd);
        }
        else
        {
            t->i2c_bus = thorpuck_i2c_open(THORPUCK_I2C_BUSNUM);
            if(t->i2c_bus < 0)
            {
                ALOGE("Failed to open I2C Bus\n");
                goto err;
            }

            // Open the irq gpio in sysfs
            fd = open("/sys/class/gpio/export", O_WRONLY);
            if(fd < 0)
            {
                ALOGE("Failed to open gpio export interface\n");
                goto err;
            }
            //purposefully not checking errors because if its already exported
            //errors the write since it isn't changing anything
            write(fd, INT_GPIONUM, INT_GPIONUM_LEN);
            close(fd);

            // Set the gpio to trigger on defined edge
            fd = open("/sys/class/gpio/gpio" INT_GPIONUM "/edge", O_RDWR);
            if(fd < 0)
            {
                ALOGE("Failed to open irq gpio edge interface\n");
                goto err;
            }
            //purposefully not checking errors because if its already set it
            //errors the write since it isn't changing anything
            write(fd, EDGE_TRIGGER, EDGE_TRIGGER_LEN);
            close(fd);

            // Open the gpio to process irq events
            t->irq_fd = open("/sys/class/gpio/gpio" INT_GPIONUM "/value", O_RDONLY | O_NONBLOCK);
            if(t->irq_fd < 0)
            {
                ALOGE("Failed to open gpio for interrupt\n");
                goto err;
            }
        }

        // If bootloaderMode is set, generally means the PSOC is stuck in the bootloader
        // and a firmware update is needed.  Only getting the file descriptors above
        // is needed.  The remaining operations below would fail otherise.
        if (!bootloaderMode)
        {
            // Scanning is on by default and can result in reception of touch event
            // from a previous session.
            if (!thorpuck_stop_scanning(t))
            {
                ALOGE("Unable to communicate with PSOC");
                goto err;
            }

            if (check_needs_update(t))
            {
                // The firmware needs updating, but allow normal operation.
                retCode = THORPUCK_OPEN_VER;
            }

            //create an epoll instance
            t->irq_epfd = epoll_create(1);
            if(t->irq_epfd < 0)
            {
                ALOGE("Failed to create epoll event\n");
                goto err;
            }

            //set our event options to priority for less interrupt latency
            t->irq_ev.events = EPOLLPRI|EPOLLERR;

            //connect our epoll event to our gpio file descriptor
            t->irq_ev.data.fd = t->irq_fd;
            if(epoll_ctl(t->irq_epfd, EPOLL_CTL_ADD, t->irq_fd, &t->irq_ev) != 0)
            {
                ALOGE("Failed to add irq fd to epoll event\n");
                goto err;
            }

            //start the thread
            t->thread_running = true;
            if(pthread_create(&t->thread, NULL, thorpuck_event_thread, t) != 0)
            {
                t->thread_running = false;
                ALOGE("Failed to create event thread\n");
                goto err;
            }
        }
    }

    *handlePtrPtr = t;
    return(retCode);

err:
    thorpuck_close(t);
    retCode = THORPUCK_OPEN_FAIL;
    return(retCode);
}

//-----------------------------------------------------------------------------
void thorpuck_close(thorpuck_t* handle)
{
    if (handle)
    {
        //stop the event thread
        if(handle->thread_running)
        {
            handle->thread_running = false;
            pthread_join(handle->thread, NULL);
        }

        //clean up epoll resources
        if(handle->irq_epfd >= 0)
        {
            close(handle->irq_epfd);
            handle->irq_epfd = -1;
        }

        //clean up gpio resources
        if(handle->irq_fd >= 0)
        {
            close(handle->irq_fd);
            handle->irq_fd = -1;
        }

        //close the i2c bus
        if(handle->i2c_bus >= 0)
        {
            thorpuck_i2c_close(handle->i2c_bus);
            handle->i2c_bus = -1;
        }

        free(handle);
    }
}

//-----------------------------------------------------------------------------
bool thorpuck_start_scanning(thorpuck_t* handle)
{
    bool    retVal = false;
    uint8_t cmd[6];

    cmd[0] = 0x00;
    cmd[1] = 0x00;
    cmd[2] = SCAN_START;
    cmd[3] = 0x00;
    cmd[4] = 0x00;

    if (!send_command(handle, cmd, ARRAY_SIZE(cmd), NULL, 0))
    {
        ALOGE("%s: Failed to start scanning.", __func__);
        goto err_ret;
    }
     
    retVal = true;

err_ret:
    return(retVal); 
}

//-----------------------------------------------------------------------------
bool thorpuck_stop_scanning(thorpuck_t* handle)
{
    bool    retVal = false;

    uint8_t cmd[6];

    cmd[0] = 0x00;
    cmd[1] = 0x00;
    cmd[2] = SCAN_STOP;
    cmd[3] = 0x00;
    cmd[4] = 0x00;

    if (!send_command(handle, cmd, ARRAY_SIZE(cmd), NULL, 0))
    {
        ALOGE("%s: Failed to stop scanning.", __func__);
        goto err_ret;
    }
     
    retVal = true;

err_ret:
    return(retVal); 
}

//-----------------------------------------------------------------------------
bool thorpuck_set_buzzer_timer(thorpuck_t* handle, uint8_t timer)
{
    bool    retVal = false;
    uint8_t cmd[7];

    cmd[0] = 0x00;
    cmd[1] = 0x00;
    cmd[2] = BUZZER_TIMER_SET;
    cmd[3] = 0x01;
    cmd[4] = 0x00;
    cmd[5] = timer;

    if (!send_command(handle, cmd, ARRAY_SIZE(cmd), NULL, 0))
    {
        ALOGE("%s: Failed to set buzzer timer.", __func__);
        goto err_ret;
    }
     
    retVal = true;

err_ret:
    return(retVal); 
}

//-----------------------------------------------------------------------------
bool thorpuck_get_buzzer_timer(thorpuck_t* handle, uint8_t* timerPtr)
{
    bool    retVal = false;
    uint8_t cmd[6];
    uint8_t rsp[5] = {0};

    cmd[0] = 0x00;
    cmd[1] = 0x00;
    cmd[2] = BUZZER_TIMER_GET;
    cmd[3] = 0x00;
    cmd[4] = 0x00;

    if (!send_command(handle, cmd, ARRAY_SIZE(cmd), rsp, ARRAY_SIZE(rsp)))
    {
        ALOGE("%s: Failed to get buzzer timer.", __func__);
        goto err_ret;
    }
     
    *timerPtr = rsp[3];
    retVal = true;

err_ret:
    return(retVal); 
}

//-----------------------------------------------------------------------------
bool thorpuck_set_single_click(thorpuck_t* handle, uint16_t min, uint16_t max)
{
    bool    retVal = false;
    uint8_t cmd[10];

    cmd[0] = 0x00;
    cmd[1] = 0x00;
    cmd[2] = SINGLE_CLICK_SET;
    cmd[3] = 0x04;
    cmd[4] = 0x00;
    cmd[5] = (min & 0xFF);
    cmd[6] = (min >> 8);
    cmd[7] = (max & 0xFF);
    cmd[8] = (max >> 8);

    if (!send_command(handle, cmd, ARRAY_SIZE(cmd), NULL, 0))
    {
        ALOGE("%s: Failed to set single click.", __func__);
        goto err_ret;
    }

    retVal = true;

err_ret:
    return(retVal); 
}

//-----------------------------------------------------------------------------
bool thorpuck_get_single_click(thorpuck_t* handle, uint16_t* minPtr, uint16_t* maxPtr)
{
    bool    retVal = false;
    uint8_t cmd[6];
    uint8_t rsp[8] = {0};

    cmd[0] = 0x00;
    cmd[1] = 0x00;
    cmd[2] = SINGLE_CLICK_GET;
    cmd[3] = 0x00;
    cmd[4] = 0x00;

    if (!send_command(handle, cmd, ARRAY_SIZE(cmd), rsp, ARRAY_SIZE(rsp)))
    {
        ALOGE("%s: Failed to get single click.", __func__);
        goto err_ret;
    }

    DEBUG_OUT("Single Click Data: 0x%02X 0x%02X 0x%02X 0x%02X\n", rsp[3], rsp[4], rsp[5], rsp[6]);

    *minPtr = (rsp[4] << 8) + rsp[3];
    *maxPtr = (rsp[6] << 8) + rsp[5];
    retVal = true;
     
err_ret:
    return(retVal); 
}

//-----------------------------------------------------------------------------
bool thorpuck_set_double_click(thorpuck_t* handle, uint16_t min, uint16_t max)
{
    bool    retVal = false;
    uint8_t cmd[10];

    cmd[0] = 0x00;
    cmd[1] = 0x00;
    cmd[2] = DBL_CLICK_SET;
    cmd[3] = 0x04;
    cmd[4] = 0x00;
    cmd[5] = (min & 0xFF);
    cmd[6] = (min >> 8);
    cmd[7] = (max & 0xFF);
    cmd[8] = (max >> 8);

    if (!send_command(handle, cmd, ARRAY_SIZE(cmd), NULL, 0))
    {
        ALOGE("%s: Failed to set double click.", __func__);
        goto err_ret;
    }

    retVal = true;

err_ret:
    return(retVal); 
}

//-----------------------------------------------------------------------------
bool thorpuck_get_double_click(thorpuck_t* handle, uint16_t* minPtr, uint16_t* maxPtr)
{
    bool    retVal = false;
    uint8_t cmd[6];
    uint8_t rsp[8] = {0};

    cmd[0] = 0x00;
    cmd[1] = 0x00;
    cmd[2] = DBL_CLICK_GET;
    cmd[3] = 0x00;
    cmd[4] = 0x00;

    if (!send_command(handle, cmd, ARRAY_SIZE(cmd), rsp, ARRAY_SIZE(rsp)))
    {
        ALOGE("%s: Failed to get double click.", __func__);
        goto err_ret;
    }

    DEBUG_OUT("Double Click Data: 0x%02X 0x%02X 0x%02X 0x%02X\n", rsp[3], rsp[4], rsp[5], rsp[6]);

    *minPtr = (rsp[4] << 8) + rsp[3];
    *maxPtr = (rsp[6] << 8) + rsp[5];
    retVal = true;
     
err_ret:
    return(retVal); 
}

//-----------------------------------------------------------------------------
bool thorpuck_set_long_press_max(thorpuck_t* handle, uint16_t max)
{
    bool    retVal = false;
    uint8_t cmd[8];

    cmd[0] = 0x00;
    cmd[1] = 0x00;
    cmd[2] = LONG_PRESS_SET;
    cmd[3] = 0x02;
    cmd[4] = 0x00;
    cmd[5] = (max & 0xFF);
    cmd[6] = (max >> 8);

    if (!send_command(handle, cmd, ARRAY_SIZE(cmd), NULL, 0))
    {
        ALOGE("%s: Failed to set long press max value.", __func__);
        goto err_ret;
    }

    retVal = true;

err_ret:
    return(retVal); 
}

//-----------------------------------------------------------------------------
bool thorpuck_get_long_press_max(thorpuck_t* handle, uint16_t* maxPtr)
{
    bool    retVal = false;
    uint8_t cmd[6];
    uint8_t rsp[6] = {0};

    cmd[0] = 0x00;
    cmd[1] = 0x00;
    cmd[2] = LONG_PRESS_GET;
    cmd[3] = 0x00;
    cmd[4] = 0x00;

    if (!send_command(handle, cmd, ARRAY_SIZE(cmd), rsp, ARRAY_SIZE(rsp)))
    {
        ALOGE("%s: Failed to get long press max value.", __func__);
        goto err_ret;
    }

    DEBUG_OUT("Long Press Data: 0x%02X 0x%02X\n", rsp[3], rsp[4]);

    *maxPtr = (rsp[4] << 8) + rsp[3];
    retVal = true;
     
err_ret:
    return(retVal); 
}

//-----------------------------------------------------------------------------
bool thorpuck_set_scroll_positions(thorpuck_t* handle,
                                   uint8_t pos1,
                                   uint8_t pos2,
                                   uint8_t pos3,
                                   uint8_t pos4)
{
    bool    retVal = false;
    uint8_t cmd[10];

    cmd[0] = 0x00;
    cmd[1] = 0x00;
    cmd[2] = SCROLL_POS_SET;
    cmd[3] = 0x04;
    cmd[4] = 0x00;
    cmd[5] = pos1;
    cmd[6] = pos2;
    cmd[7] = pos3;
    cmd[8] = pos4;

    if (!send_command(handle, cmd, ARRAY_SIZE(cmd), NULL, 0))
    {
        ALOGE("%s: Failed to set scroll positions.", __func__);
        goto err_ret;
    }

    retVal = true;
     
err_ret:
    return(retVal); 
}

//-----------------------------------------------------------------------------
bool thorpuck_get_scroll_positions(thorpuck_t* handle,
                                   uint8_t* pos1Ptr,
                                   uint8_t* pos2Ptr,
                                   uint8_t* pos3Ptr,
                                   uint8_t* pos4Ptr)
{
    bool    retVal = false;
    uint8_t cmd[6];
    uint8_t rsp[8] = {0};

    cmd[0] = 0x00;
    cmd[1] = 0x00;
    cmd[2] = SCROLL_POS_GET;
    cmd[3] = 0x00;
    cmd[4] = 0x00;

    if (!send_command(handle, cmd, ARRAY_SIZE(cmd), rsp, ARRAY_SIZE(rsp)))
    {
        ALOGE("%s: Failed to get scroll positions.", __func__);
        goto err_ret;
    }

    DEBUG_OUT("Scroll Position Threshold Data: 0x%02X 0x%02X 0x%02X 0x%02X\n", 
              rsp[3], rsp[4], rsp[5], rsp[6]);

    *pos1Ptr = rsp[3];
    *pos2Ptr = rsp[4];
    *pos3Ptr = rsp[5];
    *pos4Ptr = rsp[6];
    retVal = true;
     
err_ret:
    return(retVal); 
}

//-----------------------------------------------------------------------------
bool thorpuck_set_scroll_steps(thorpuck_t* handle,
                               uint8_t step1,
                               uint8_t step2,
                               uint8_t step3,
                               uint8_t step4)
{
    bool    retVal = false;
    uint8_t cmd[10];

    cmd[0] = 0x00;
    cmd[1] = 0x00;
    cmd[2] = SCROLL_STEP_SET;
    cmd[3] = 0x04;
    cmd[4] = 0x00;
    cmd[5] = step1;
    cmd[6] = step2;
    cmd[7] = step3;
    cmd[8] = step4;

    if (!send_command(handle, cmd, ARRAY_SIZE(cmd), NULL, 0))
    {
        ALOGE("%s: Failed to set scroll steps.", __func__);
        goto err_ret;
    }

    retVal = true;
     
err_ret:
    return(retVal); 
}

//-----------------------------------------------------------------------------
bool thorpuck_get_scroll_steps(thorpuck_t* handle,
                               uint8_t* step1Ptr,
                               uint8_t* step2Ptr,
                               uint8_t* step3Ptr,
                               uint8_t* step4Ptr)
{
    bool    retVal = false;
    uint8_t cmd[6];
    uint8_t rsp[8] = {0};

    cmd[0] = 0x00;
    cmd[1] = 0x00;
    cmd[2] = SCROLL_STEP_GET;
    cmd[3] = 0x00;
    cmd[4] = 0x00;

    if (!send_command(handle, cmd, ARRAY_SIZE(cmd), rsp, ARRAY_SIZE(rsp)))
    {
        ALOGE("%s: Failed to get scroll steps.", __func__);
        goto err_ret;
    }

    DEBUG_OUT("Scroll Step Data: 0x%02X 0x%02X 0x%02X 0x%02X\n", 
              rsp[3], rsp[4], rsp[5], rsp[6]);

    *step1Ptr = rsp[3];
    *step2Ptr = rsp[4];
    *step3Ptr = rsp[5];
    *step4Ptr = rsp[6];
    retVal = true;
     
err_ret:
    return(retVal); 
}

//-----------------------------------------------------------------------------
bool thorpuck_reset_factory(thorpuck_t* handle)
{
    bool    retVal = false;
    uint8_t cmd[6];

    cmd[0] = 0x00;
    cmd[1] = 0x00;
    cmd[2] = RESET_FACTORY;
    cmd[3] = 0x00;
    cmd[4] = 0x00;

    if (!send_command(handle, cmd, ARRAY_SIZE(cmd), NULL, 0))
    {
        ALOGE("%s: Failed to perform factory reset.", __func__);
        goto err_ret;
    }

    retVal = true;
     
err_ret:
    return(retVal); 
}

//-----------------------------------------------------------------------------
bool thorpuck_psoc_reset(thorpuck_t* handle)
{
    bool    retVal = false;
    uint8_t cmd[6];

    cmd[0] = 0x00;
    cmd[1] = 0x00;
    cmd[2] = PSOC_RESET;
    cmd[3] = 0x00;
    cmd[4] = 0x00;

    if (!send_command(handle, cmd, ARRAY_SIZE(cmd), NULL, 0))
    {
        ALOGE("%s: Failed to perform PSOC reset.", __func__);
        goto err_ret;
    }

    // It takes time for the PSOC to resume normal operations.  This number
    // was found experimentally.
    usleep(500000);

    retVal = true;
     
err_ret:
    return(retVal); 
}

//-----------------------------------------------------------------------------
bool thorpuck_get_fw_version(thorpuck_t* handle,
                             uint8_t* char1Ptr,
                             uint8_t* char2Ptr,
                             uint8_t* char3Ptr)
{
    bool    retVal = false;
    uint8_t cmd[6];
    uint8_t rsp[7] = {0};

    cmd[0] = 0x00;
    cmd[1] = 0x00;
    cmd[2] = GET_FW_VERSION;
    cmd[3] = 0x00;
    cmd[4] = 0x00;

    if (!send_command(handle, cmd, ARRAY_SIZE(cmd), rsp, ARRAY_SIZE(rsp)))
    {
        ALOGE("%s: Failed to get fw version info.", __func__);
        goto err_ret;
    }

    DEBUG_OUT("Version Data: 0x%02X 0x%02X 0x%02X\n", 
              rsp[3], rsp[4], rsp[5]);

    *char1Ptr = rsp[3];
    *char2Ptr = rsp[4];
    *char3Ptr = rsp[5];
    retVal = true;
     
err_ret:
    return(retVal); 
}

//-----------------------------------------------------------------------------
void thorpuck_get_lib_version(uint8_t* char1Ptr,
                              uint8_t* char2Ptr,
                              uint8_t* char3Ptr)
{
    *char1Ptr = Firmware_Version[0];
    *char2Ptr = Firmware_Version[1];
    *char3Ptr = Firmware_Version[2];
}

//-----------------------------------------------------------------------------
bool thorpuck_update_firmware(thorpuck_t* handle)
{
    return(do_update(handle));
}

//-----------------------------------------------------------------------------
bool thorpuck_xfer_firmware(thorpuck_t* handle, char* path)
{
    bool            retVal = false;
    FILE*           filePtr = NULL;
    char*           linePtr = NULL;
    size_t          lineCap = 0;
    ssize_t         lineLen;
    unsigned int    lineCount;
    unsigned int    lineNum = 1;

    filePtr = fopen(path, "r");
    if (NULL == filePtr)
    {
        ALOGE("Failed to open file: %s\n", path);
        goto err_ret;
    }

    // Get the number of data "lines"
    lineLen = getline(&linePtr, &lineCap, filePtr);
    if (-1 == lineLen)
    {
        ALOGE("Failed to read firmware file");
        goto err_ret;
    }

    // The firmware file originates from a Windows computer, so expect
    // carriage-return / linefeed combos.
    if (0x0A != linePtr[lineLen - 1] || 0x0D != linePtr[lineLen - 2])
    {
        ALOGE("Invalid file format");
        goto err_ret;
    }

    lineCount = strtol(linePtr, NULL, 10);

    // Put the PSOC into bootload mode
    if(handle->thread_running)
    {
        uint8_t         cmd[6];

        cmd[0] = 0x00;
        cmd[1] = 0x00;
        cmd[2] = ENTER_BOOTLOAD;
        cmd[3] = 0x00;
        cmd[4] = 0x00;
    
        if (!send_command(handle, cmd, ARRAY_SIZE(cmd), NULL, 0))
        {
            ALOGE("%s: Failed to put PSOC in bootload mode.", __func__);
            goto err_ret;
        }

        // Need to wait longer than normal for PSOC to go into bootload mode
        usleep(1000000);

        ALOGD("Updating firmware\n");
    }

    // Send the new firmware
    {
        CyBtldr_CommunicationsData  comm;
        unsigned char               retCode;

        comm.OpenConnection = &OpenConnection;
        comm.CloseConnection = &CloseConnection;
        comm.ReadData = &ReadData;
        comm.WriteData = &WriteData;
        comm.MaxTransferSize = 64u;
        comm.priv = handle;

        retCode = transferFirmware(&comm, filePtr, lineCount );
        if (CYRET_SUCCESS == retCode)
        {
            ALOGD("Firmware update complete\n");
            retVal = true;
        }
        else
        {
            ALOGD("Firmware update failed\n");
        }
    }

err_ret:
    if (NULL != filePtr)
    {
        fclose(filePtr);
        filePtr = NULL;
    }
    if (NULL != linePtr)
    {
        free(linePtr);
        linePtr = NULL;
    }

    return(retVal);
}

