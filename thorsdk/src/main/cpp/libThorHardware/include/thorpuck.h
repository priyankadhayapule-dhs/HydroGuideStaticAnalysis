//
// Copyright 2018 EchoNous Inc.
//
//

#ifndef THORPUCK_H
#define THORPUCK_H

#include <stdint.h>
#include <stdbool.h>
#include <ThorError.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct thorpuck_s thorpuck_t;

// Button Action
typedef enum
{
    THORPUCK_BUTTON_NA = 0x0,           // Not Touched
    THORPUCK_BUTTON_SINGLE = 0x20,      // Single Click
    THORPUCK_BUTTON_DOUBLE = 0x22,      // Double Click
    THORPUCK_BUTTON_LONG = 0x40,        // Long Press
} thorpuck_button_action_t;


// Palm Action
typedef enum
{
    THORPUCK_PALM_NA = 0x0,             // Palm not touched
    THORPUCK_PALM_TOUCHED = 0x20,       // Palm touched
} thorpuck_palm_action_t;


// Scroll Action
typedef enum
{
    THORPUCK_SCROLL_NA = 0x0,           // Scroll not touched
    THORPUCK_SCROLL_UP = 0x50,          // Scroll up
    THORPUCK_SCROLL_DOWN = 0x58,        // Scroll down
} thorpuck_scroll_action_t;

// Structure to convey event information to the user.
typedef struct
{
    thorpuck_button_action_t    leftButtonAction;
    thorpuck_button_action_t    rightButtonAction;
    thorpuck_palm_action_t      palmAction;
    thorpuck_scroll_action_t    scrollAction;
    int8_t                      scrollAmount;
    uint32_t                    updateCurPos;
    uint32_t                    updateMaxPos;
    ThorStatus                  updateStatus;
} thorpuck_event_t;

// Structure to pass needed file descriptors that have been opened by
// a privileged process.
typedef struct
{
    int     i2cFd;
    int     irqFd;
} puck_context_t;

// Return codes from thorpuck_open
typedef enum
{
    THORPUCK_OPEN_OK = 0,       // Puck opened successfully
    THORPUCK_OPEN_FAIL = 1,     // Failed to open the Puck
    THORPUCK_OPEN_VER = 2       // Puck opened but has incompatible version.
} thorpuck_open_code_t;

// Asynchronous callback to inform user of events as they occur. This callback
// is called from a dedicated event thread. 
//
// handle - The thorpuck instance associated with this event
// priv - The private data pointer passed into thorpuck_open
// event - The current event to process
//
typedef void (*thorpuck_event_callback_t)(thorpuck_t* handle, void* priv, thorpuck_event_t* event);

// Allocate resources, probe the I2C device for Thorpuck, create event thread
// and start processing events. Return NULL if any resource allocation fails
// or if the Thorpuck device is not found.
//
// context - PuckContext containing needed file descriptors.  If NULL, then
//           needed files will be opened.
// cb - The callback to be called when an event occurs
// priv - private data pointer to be included in callback
//
thorpuck_open_code_t thorpuck_open(puck_context_t* contextPtr,
                                   thorpuck_event_callback_t cb,
                                   void* priv,
                                   bool bootloaderMode,
                                   thorpuck_t** handlePtrPtr);

// Free all resources used and stop event thread.
//
// t - The thorpuck_t instance handle to close.
//
void thorpuck_close(thorpuck_t* handle);

bool thorpuck_start_scanning(thorpuck_t* handle);
bool thorpuck_stop_scanning(thorpuck_t* handle);

bool thorpuck_set_buzzer_timer(thorpuck_t* handle, uint8_t timer);
bool thorpuck_get_buzzer_timer(thorpuck_t* handle, uint8_t* timerPtr);

bool thorpuck_set_single_click(thorpuck_t* handle, uint16_t min, uint16_t max);
bool thorpuck_get_single_click(thorpuck_t* handle, uint16_t* minPtr, uint16_t* maxPtr);

bool thorpuck_set_double_click(thorpuck_t* handle, uint16_t min, uint16_t max);
bool thorpuck_get_double_click(thorpuck_t* handle, uint16_t* minPtr, uint16_t* maxPtr);

bool thorpuck_set_long_press_max(thorpuck_t* handle, uint16_t max);
bool thorpuck_get_long_press_max(thorpuck_t* handle, uint16_t* maxPtr);

bool thorpuck_set_scroll_positions(thorpuck_t* handle,
                                   uint8_t pos1,
                                   uint8_t pos2,
                                   uint8_t pos3,
                                   uint8_t pos4);
bool thorpuck_get_scroll_positions(thorpuck_t* handle,
                                   uint8_t* pos1Ptr,
                                   uint8_t* pos2Ptr,
                                   uint8_t* pos3Ptr,
                                   uint8_t* pos4Ptr);

bool thorpuck_set_scroll_steps(thorpuck_t* handle,
                               uint8_t step1,
                               uint8_t step2,
                               uint8_t step3,
                               uint8_t step4);
bool thorpuck_get_scroll_steps(thorpuck_t* handle,
                               uint8_t* step1Ptr,
                               uint8_t* step2Ptr,
                               uint8_t* step3Ptr,
                               uint8_t* step4Ptr);

bool thorpuck_reset_factory(thorpuck_t* handle);
bool thorpuck_psoc_reset(thorpuck_t* handle);


bool thorpuck_get_fw_version(thorpuck_t* handle,
                             uint8_t* char1Ptr,
                             uint8_t* char2Ptr,
                             uint8_t* char3Ptr);

void thorpuck_get_lib_version(uint8_t* char1Ptr,
                              uint8_t* char2Ptr,
                              uint8_t* char3Ptr);

bool thorpuck_update_firmware(thorpuck_t* handle);
bool thorpuck_xfer_firmware(thorpuck_t* handle, char* path);


#ifdef __cplusplus
}
#endif

#endif //THORPUCK_H
