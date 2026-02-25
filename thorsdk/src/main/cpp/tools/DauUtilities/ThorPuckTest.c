#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "thorpuck.h"

char gScrollMsg[128];


//-----------------------------------------------------------------------------
void usage()
{
    printf("Usage: thorpucktest [Option]\n");
    printf("\nAvailable Options:\n");
    printf("  scan\t\t\tBegin scanning.\n\n");
    printf("  get\t\t\tGet configuration data.\n");
    printf("  reset\t\t\tReset PSOC.\n");
    printf("  default\t\tReset configuration data to factory defaults.\n\n");
    printf("  firmware\t\tUpdate the firmware using built-in image\n");
    printf("  firmware-file\t\tUpdate the firmware from the specified file\n\n");
    printf("  set-buzzer <time>\t\tSet buzzer time in msec.\n");
    printf("  set-longpress <time>\t\tSet long press max time in msec.\n");
    printf("  set-singleclick <min> <max>\tSet single click min and max duration in msec.\n");
    printf("  set-doubleclick <min> <max>\tSet double click min and max duration in msec.\n\n");
    printf("  set-scrollpos <pos1> <pos2> <pos3> <pos4>\t");
    printf("\tSet scroll position thresholds.\n");
    printf("  set-scrollstep <step1> <step2> <step3> <step4>\t");
    printf("Set scroll step sizes.\n");
}


//-----------------------------------------------------------------------------
static const char* thorpuck_button_string(thorpuck_button_action_t action)
{
    switch (action)
    {
        case THORPUCK_BUTTON_NA:
            return("");

        case THORPUCK_BUTTON_SINGLE:
            return("Single Click");

        case THORPUCK_BUTTON_DOUBLE:
            return("Double Click");

        case THORPUCK_BUTTON_LONG:
            return("Long Press");

        default:
            return("Unknown Button Action");
    }
}

//-----------------------------------------------------------------------------
static const char* thorpuck_palm_string(thorpuck_palm_action_t action)
{
    switch (action)
    {
        case THORPUCK_PALM_NA:
            return("");

        case THORPUCK_PALM_TOUCHED:
            return("Touched");

        default:
            return("Unknown Palm Action");
    }
}

//-----------------------------------------------------------------------------
static const char* thorpuck_scroll_action(thorpuck_scroll_action_t action,
                                          int8_t amount)
{
    switch (action)
    {
        case THORPUCK_SCROLL_NA:
            return("");

        case THORPUCK_SCROLL_UP:
            memset(gScrollMsg, 0, sizeof(gScrollMsg));
            sprintf(gScrollMsg, "Scroll Up %d lines", amount);
            return(gScrollMsg); 

        case THORPUCK_SCROLL_DOWN:
            memset(gScrollMsg, 0, sizeof(gScrollMsg));
            sprintf(gScrollMsg, "Scroll Down %d lines", amount);
            return(gScrollMsg); 

        default:
            return("Unknown Palm Action");
    }
}

//-----------------------------------------------------------------------------
static void thorpuck_event_cb(thorpuck_t* handle, void* priv, thorpuck_event_t* event)
{
    if (event)
    {
        if (event->updateMaxPos)
        {
            switch (event->updateStatus)
            {
                case TER_TABLET_PUCK_UPDATE_FIRMWARE_COMPLETE:
                    printf("\nUpdate completed\n");
                    fflush(stdout);
                    break;

                case TER_TABLET_PUCK_UPDATE_FIRMWARE_BUSY:
                    printf("\rUpdating firmware: %u/%u", event->updateCurPos, event->updateMaxPos);
                    fflush(stdout);
                    break;

                case TER_TABLET_PUCK_UPDATE_FIRMWARE:
                default:
                    printf("\nUpdate failed\n");
                    fflush(stdout);
                    break;
            }
        }
        else
        {
            printf("Left Button:  %s\n"
                   "Right Button: %s\n"
                   "Palm:         %s\n"
                   "Scroll        %s\n\n",
                   thorpuck_button_string(event->leftButtonAction),
                   thorpuck_button_string(event->rightButtonAction),
                   thorpuck_palm_string(event->palmAction),
                   thorpuck_scroll_action(event->scrollAction, event->scrollAmount));
        }
    }
}

//-----------------------------------------------------------------------------
void showVersion(thorpuck_t* puckHandle)
{
    uint8_t     libVersion[3];
    uint8_t     fwVersion[3];

    thorpuck_get_lib_version(&libVersion[0], &libVersion[1], &libVersion[2]);
    thorpuck_get_fw_version(puckHandle, &fwVersion[0], &fwVersion[1], &fwVersion[2]);

    printf("The firwmare version does not match the expected version.\n");
    printf("A firmware upgrade is needed.\n\n");
    printf("\tFirmware Version: %c%c%c\n", fwVersion[0], fwVersion[1], fwVersion[2]);
    printf("\tExpected Version: %c%c%c\n", libVersion[0], libVersion[1], libVersion[2]);
}

//-----------------------------------------------------------------------------
void scan()
{
    thorpuck_t*             puckHandle = NULL;
    thorpuck_open_code_t    retCode;
    int                     key = 0;
    int                     qkey = 'q';

    printf("Opening Puck\n");

    retCode = thorpuck_open(NULL, thorpuck_event_cb, NULL, false, &puckHandle);
    switch (retCode)
    {
        case THORPUCK_OPEN_VER:
        case THORPUCK_OPEN_OK:
            printf("Scanning... Press 'q' + <enter> to quit.\n\n");
            if (thorpuck_start_scanning(puckHandle))
            {
                do
                {
                    key = getchar();
                } while (qkey != key);

                thorpuck_stop_scanning(puckHandle);
            }
            else
            {
                printf("Failed to start scanning\n");
            }
            break;

        case THORPUCK_OPEN_FAIL:
            printf("Failed to open puck.\n");
            break;
    }

    if (NULL != puckHandle)
    {
        printf("\nClosing Puck\n");
        thorpuck_close(puckHandle);
        puckHandle = NULL;
    }
}

//-----------------------------------------------------------------------------
void getConfig()
{
    thorpuck_t*             puckHandle = NULL;
    thorpuck_open_code_t    retCode;

    printf("Opening Puck\n\n");

    retCode = thorpuck_open(NULL, thorpuck_event_cb, NULL, false, &puckHandle);
    switch (retCode)
    {
        case THORPUCK_OPEN_VER:
        case THORPUCK_OPEN_OK:
        {
            uint8_t     val;
            uint8_t     val1;
            uint8_t     val2;
            uint8_t     val3;
            uint8_t     val4;
            uint16_t    minVal;
            uint16_t    maxVal;

            thorpuck_get_fw_version(puckHandle, &val1, &val2, &val3);
            printf("FW Version: %c%c%c\n", val1, val2, val3);

            thorpuck_get_lib_version(&val1, &val2, &val3);
            printf("Library Version: %c%c%c\n", val1, val2, val3);

            thorpuck_get_buzzer_timer(puckHandle, &val);
            printf("Buzzer Timer: %ums\n", val);

            thorpuck_get_single_click(puckHandle, &minVal, &maxVal);
            printf("Single Click Values\n");
            printf("                    Min: %ums\n", minVal);
            printf("                    Max: %ums\n", maxVal);

            thorpuck_get_double_click(puckHandle, &minVal, &maxVal);
            printf("Double Click Values\n");
            printf("                    Min: %ums\n", minVal);
            printf("                    Max: %ums\n", maxVal);

            thorpuck_get_long_press_max(puckHandle, &maxVal);
            printf("Long Press Max: %ums\n", maxVal);

            thorpuck_get_scroll_positions(puckHandle, &val1, &val2, &val3, &val4);
            printf("Scroll Positions\n");
            printf("                 Pos 1: %u\n", val1);
            printf("                 Pos 2: %u\n", val2);
            printf("                 Pos 3: %u\n", val3);
            printf("                 Pos 4: %u\n", val4);

            thorpuck_get_scroll_steps(puckHandle, &val1, &val2, &val3, &val4);
            printf("Scroll Steps\n");
            printf("             Step 1: %u\n", val1);
            printf("             Step 2: %u\n", val2);
            printf("             Step 3: %u\n", val3);
            printf("             Step 4: %u\n", val4);
            break;
        }

        case THORPUCK_OPEN_FAIL:
            printf("Failed to open puck.\n");
            break;
    }

    if (NULL != puckHandle)
    {
        printf("\nClosing Puck\n");
        thorpuck_close(puckHandle);
        puckHandle = NULL;
    }
}

//-----------------------------------------------------------------------------
void reset()
{
    thorpuck_t*             puckHandle = NULL;
    thorpuck_open_code_t    retCode;

    printf("Opening Puck\n");

    retCode = thorpuck_open(NULL, thorpuck_event_cb, NULL, false, &puckHandle);
    switch (retCode)
    {
        case THORPUCK_OPEN_VER:
        case THORPUCK_OPEN_OK:
            printf("Resetting the PSOC.\n");
            thorpuck_psoc_reset(puckHandle);
            break;

        case THORPUCK_OPEN_FAIL:
            printf("Failed to open puck.\n");
            break;
    }

    if (NULL != puckHandle)
    {
        printf("\nClosing Puck\n");
        thorpuck_close(puckHandle);
        puckHandle = NULL;
    }
}

//-----------------------------------------------------------------------------
void setDefaults()
{
    thorpuck_t*             puckHandle = NULL;
    thorpuck_open_code_t    retCode;

    printf("Opening Puck\n");

    retCode = thorpuck_open(NULL, thorpuck_event_cb, NULL, false, &puckHandle);
    switch (retCode)
    {
        case THORPUCK_OPEN_VER:
        case THORPUCK_OPEN_OK:
            printf("Resetting to factory defaults.\n");
            thorpuck_reset_factory(puckHandle);
            break;

        case THORPUCK_OPEN_FAIL:
            printf("Failed to open puck.\n");
            break;
    }

    if (NULL != puckHandle)
    {
        printf("\nClosing Puck\n");
        thorpuck_close(puckHandle);
        puckHandle = NULL;
    }
}

//-----------------------------------------------------------------------------
void updateFirmware()
{
    thorpuck_t*             puckHandle = NULL;
    thorpuck_open_code_t    retCode;

    printf("Opening Puck\n");

    retCode = thorpuck_open(NULL, thorpuck_event_cb, NULL, false, &puckHandle);
    switch (retCode)
    {
        case THORPUCK_OPEN_OK:
            printf("The firmware is already up to date.\n");
            break;

        case THORPUCK_OPEN_FAIL:
            printf("Failed to open puck, trying bootloader directly.\n");
            thorpuck_open(NULL, thorpuck_event_cb, NULL, true, &puckHandle);

        case THORPUCK_OPEN_VER:
            if (thorpuck_update_firmware(puckHandle))
            {
                printf("\nFirmware update succeeded\n");
            }
            else
            {
                printf("\nFirmware update failed\n");
            }
            break;
    }

    if (NULL != puckHandle)
    {
        printf("\nClosing Puck\n");
        thorpuck_close(puckHandle);
        puckHandle = NULL;
    }
}

//-----------------------------------------------------------------------------
void updateFirmwareFile(char* path)
{
    thorpuck_t*             puckHandle = NULL;
    thorpuck_open_code_t    retCode;

    printf("Opening Puck\n");

    retCode = thorpuck_open(NULL, thorpuck_event_cb, NULL, false, &puckHandle);
    switch (retCode)
    {
        case THORPUCK_OPEN_FAIL:
            printf("Failed to open puck, trying bootloader directly.\n");
            thorpuck_open(NULL, thorpuck_event_cb, NULL, true, &puckHandle);

        case THORPUCK_OPEN_OK:
        case THORPUCK_OPEN_VER:
            if (thorpuck_xfer_firmware(puckHandle, path))
            {
                printf("\nFirmware update succeeded\n");
            }
            else
            {
                printf("\nFirmware update failed\n");
            }
            break;
    }

    if (NULL != puckHandle)
    {
        printf("\nClosing Puck\n");
        thorpuck_close(puckHandle);
        puckHandle = NULL;
    }
}

//-----------------------------------------------------------------------------
void setBuzzer(uint8_t val)
{
    thorpuck_t*             puckHandle = NULL;
    thorpuck_open_code_t    retCode;

    printf("Opening Puck\n");

    retCode = thorpuck_open(NULL, thorpuck_event_cb, NULL, false, &puckHandle);
    switch (retCode)
    {
        case THORPUCK_OPEN_VER:
        case THORPUCK_OPEN_OK:
            printf("Setting buzzer time to %ums\n", val);
            thorpuck_set_buzzer_timer(puckHandle, val);
            break;

        case THORPUCK_OPEN_FAIL:
            printf("Failed to open puck.\n");
            break;
    }

    if (NULL != puckHandle)
    {
        printf("\nClosing Puck\n");
        thorpuck_close(puckHandle);
        puckHandle = NULL;
    }
}

//-----------------------------------------------------------------------------
void setLongPress(uint16_t val)
{
    thorpuck_t*             puckHandle = NULL;
    thorpuck_open_code_t    retCode;

    printf("Opening Puck\n");

    retCode = thorpuck_open(NULL, thorpuck_event_cb, NULL, false, &puckHandle);
    switch (retCode)
    {
        case THORPUCK_OPEN_VER:
        case THORPUCK_OPEN_OK:
            printf("Setting long press max time to %ums\n", val);
            thorpuck_set_long_press_max(puckHandle, val);
            break;

        case THORPUCK_OPEN_FAIL:
            printf("Failed to open puck.\n");
            break;
    }

    if (NULL != puckHandle)
    {
        printf("\nClosing Puck\n");
        thorpuck_close(puckHandle);
        puckHandle = NULL;
    }
}

//-----------------------------------------------------------------------------
void setSingleClick(uint16_t min, uint16_t max)
{
    thorpuck_t*             puckHandle = NULL;
    thorpuck_open_code_t    retCode;

    printf("Opening Puck\n");

    retCode = thorpuck_open(NULL, thorpuck_event_cb, NULL, false, &puckHandle);
    switch (retCode)
    {
        case THORPUCK_OPEN_VER:
        case THORPUCK_OPEN_OK:
            printf("Setting single click params to min: %ums, max: %ums\n",
                   min, max);
            thorpuck_set_single_click(puckHandle, min, max);
            break;

        case THORPUCK_OPEN_FAIL:
            printf("Failed to open puck.\n");
            break;
    }

    if (NULL != puckHandle)
    {
        printf("\nClosing Puck\n");
        thorpuck_close(puckHandle);
        puckHandle = NULL;
    }
}

//-----------------------------------------------------------------------------
void setDoubleClick(uint16_t min, uint16_t max)
{
    thorpuck_t*             puckHandle = NULL;
    thorpuck_open_code_t    retCode;

    printf("Opening Puck\n");

    retCode = thorpuck_open(NULL, thorpuck_event_cb, NULL, false, &puckHandle);
    switch (retCode)
    {
        case THORPUCK_OPEN_VER:
        case THORPUCK_OPEN_OK:
            printf("Setting double click params to min: %ums, max: %ums\n",
                   min, max);
            thorpuck_set_double_click(puckHandle, min, max);
            break;

        case THORPUCK_OPEN_FAIL:
            printf("Failed to open puck.\n");
            break;
    }

    if (NULL != puckHandle)
    {
        printf("\nClosing Puck\n");
        thorpuck_close(puckHandle);
        puckHandle = NULL;
    }
}

//-----------------------------------------------------------------------------
void setScrollPos(uint8_t pos1, uint8_t pos2, uint8_t pos3, uint8_t pos4)
{
    thorpuck_t*             puckHandle = NULL;
    thorpuck_open_code_t    retCode;

    printf("Opening Puck\n");

    retCode = thorpuck_open(NULL, thorpuck_event_cb, NULL, false, &puckHandle);
    switch (retCode)
    {
        case THORPUCK_OPEN_VER:
        case THORPUCK_OPEN_OK:
            printf("Setting scroll position params to pos1: %u, pos2: %u, pos3: %u, pos4: %u\n",
                   pos1, pos2, pos3, pos4);
            thorpuck_set_scroll_positions(puckHandle, pos1, pos2, pos3, pos4);
            break;

        case THORPUCK_OPEN_FAIL:
            printf("Failed to open puck.\n");
            break;
    }

    if (NULL != puckHandle)
    {
        printf("\nClosing Puck\n");
        thorpuck_close(puckHandle);
        puckHandle = NULL;
    }
}

//-----------------------------------------------------------------------------
void setScrollStep(uint8_t step1, uint8_t step2, uint8_t step3, uint8_t step4)
{
    thorpuck_t*             puckHandle = NULL;
    thorpuck_open_code_t    retCode;

    printf("Opening Puck\n");

    retCode = thorpuck_open(NULL, thorpuck_event_cb, NULL, false, &puckHandle);
    switch (retCode)
    {
        case THORPUCK_OPEN_VER:
        case THORPUCK_OPEN_OK:
            printf("Setting scroll step params to step1: %u, step2: %u, step3: %u, step4: %u\n",
                   step1, step2, step3, step4);
            thorpuck_set_scroll_steps(puckHandle, step1, step2, step3, step4);
            break;

        case THORPUCK_OPEN_FAIL:
            printf("Failed to open puck.\n");
            break;
    }

    if (NULL != puckHandle)
    {
        printf("\nClosing Puck\n");
        thorpuck_close(puckHandle);
        puckHandle = NULL;
    }
}

//-----------------------------------------------------------------------------
int main(int argc, char** argv)
{
    const char*     cmdScan = "scan";
    const char*     cmdGet = "get";
    const char*     cmdReset = "reset";
    const char*     cmdDefault = "default";
    const char*     cmdBuzzer = "set-buzzer";
    const char*     cmdLongPress = "set-longpress";
    const char*     cmdSingleClick = "set-singleclick";
    const char*     cmdDoubleClick = "set-doubleclick";
    const char*     cmdScrollPos = "set-scrollpos";
    const char*     cmdScrollStep = "set-scrollstep";
    const char*     cmdFirmware = "firmware";
    const char*     cmdFirmwareFile = "firmware-file";

    switch (argc)
    {
        // scan, reset, default, get configuration, and firmware
        case 2:
            if (0 == strncasecmp(cmdScan, argv[1], strlen(cmdScan)))
            {
                scan();
            }
            else if (0 == strncasecmp(cmdGet, argv[1], strlen(cmdGet)))
            {
                getConfig();
            }
            else if (0 == strncasecmp(cmdReset, argv[1], strlen(cmdReset)))
            {
                reset();
            }
            else if (0 == strncasecmp(cmdDefault, argv[1], strlen(cmdDefault)))
            {
                setDefaults();
            }
            else if (0 == strncasecmp(cmdFirmware, argv[1], strlen(cmdFirmware)))
            {
                updateFirmware();
            }
            else
            {
                usage();
            }
            break;

        // Set buzzer, long press, and firmware-file
        case 3:
            if (0 == strncasecmp(cmdBuzzer, argv[1], strlen(cmdBuzzer)))
            {
                setBuzzer(strtol(argv[2], NULL, 10));
            }
            else if (0 == strncasecmp(cmdLongPress, argv[1], strlen(cmdLongPress)))
            {
                setLongPress(strtol(argv[2], NULL, 10));
            }
            else if (0 == strncasecmp(cmdFirmwareFile, argv[1], strlen(cmdFirmwareFile)))
            {
                updateFirmwareFile(argv[2]);
            }
            else
            {
                usage();
            }
            break;

        // Set single click, and double click
        case 4:
            if (0 == strncasecmp(cmdSingleClick, argv[1], strlen(cmdSingleClick)))
            {
                setSingleClick(strtol(argv[2], NULL, 10), strtol(argv[3], NULL, 10));
            }
            else if (0 == strncasecmp(cmdDoubleClick, argv[1], strlen(cmdDoubleClick)))
            {
                setDoubleClick(strtol(argv[2], NULL, 10), strtol(argv[3], NULL, 10));
            }
            else
            {
                usage();
            }
            break;

        // Set scroll position, and scroll steps
        case 6:
            if (0 == strncasecmp(cmdScrollPos, argv[1], strlen(cmdScrollPos)))
            {
                setScrollPos(strtol(argv[2], NULL, 10),
                             strtol(argv[3], NULL, 10),
                             strtol(argv[4], NULL, 10),
                             strtol(argv[5], NULL, 10));
            }
            else if (0 == strncasecmp(cmdScrollStep, argv[1], strlen(cmdScrollStep)))
            {
                setScrollStep(strtol(argv[2], NULL, 10),
                              strtol(argv[3], NULL, 10),
                              strtol(argv[4], NULL, 10),
                              strtol(argv[5], NULL, 10));
            }
            else
            {
                usage();
            }
            break;

        default:
            usage();
            break;
    }

    return(0);
}
