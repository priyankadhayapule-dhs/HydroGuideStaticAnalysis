//
// Copyright 2017 EchoNous Inc.
//
// Thor specific error codes
//
// This file was automatically generated.  Do not edit!
//
#pragma once


#include <stdint.h>


//
//  31                27              15              0
//  ---------------------------------------------------
//  | x | ERR | x | x |      TES      |     Code      |
//  ---------------------------------------------------
//
//  Label     Bit(s)         Description
//  -----------------------------------------------------------
//   x      28, 29, 31   Reserved for future use
//   ERR        30       Error bit.  1 = error, 0 = success
//   TES     16 - 27     Thor Error Source
//   Code    0 - 15      Error Code
//

//
// Common Defines
//

typedef uint32_t ThorStatus;

#define THOR_OK             0x0
#define THOR_ERROR          0x40000000
#define IS_THOR_ERROR(x)    (x & (THOR_ERROR))

#define DEFINE_THOR_ERROR(source, code) (THOR_ERROR | (source << 16) | code)

//
// Thor Error Sources (TES)
//

// Miscellaneous: Contains all miscellaneous and generic errors
#define TES_MISC		0x1

// Linux: Standard Linux errors from errno
#define TES_LINUX		0x2

// Dau Hardware: Errors generated from the Dau hardware
#define TES_DAU_HW		0x3

// Tablet Hardware: Errors unique to the Thor tablet hardware
#define TES_TABLET_HW		0x4

// Thermal: Thermal warnings/errors from Thor tablet and Dau
#define TES_THERMAL		0x5

// Imaging Engine: Errors from the Ultrasound Imaging Engine
#define TES_IMAGE_ENG		0x6

// Ultrasound Manager: Errors from the Ultrasound Manager
#define TES_ULTRASOUND_MGR		0x7

// AP_I Manager: Errors from the AP_I Manager
#define TES_API_MANAGER		0x8

// AI System: Errors from the AI System
#define TES_AI_SYSTEM		0x9

//
// Thor Error Definitions
//

//
// Miscellaneous
//
// Operation Aborted: The operation was aborted
#define TER_MISC_ABORT		0x40010001
// File Create Failed: The file could not be created
#define TER_MISC_CREATE_FILE_FAILED		0x40010002
// File Open Failed: The file could not be opened
#define TER_MISC_OPEN_FILE_FAILED		0x40010003
// Invalid File: The file has invalid format
#define TER_MISC_INVALID_FILE		0x40010004
// File Not Open: The file is not open
#define TER_MISC_FILE_NOT_OPEN		0x40010005
// Write Failed: The write operation failed
#define TER_MISC_WRITE_FAILED		0x40010006
// Not Implemented: The method is not implemented
#define TER_MISC_NOT_IMPLEMENTED		0x40010007
// Busy: Busy: The operation could not be performed
#define TER_MISC_BUSY		0x40010008
// Invalid Parameter: One or more parameters are either null or out of range
#define TER_MISC_PARAM		0x40010009
// Invalid Permission: Permission is not granted to perform the operation
#define TER_MISC_PERMISSION		0x4001000A

//
// Dau Hardware
//
// Power Error: HV power supply monitor fault detected
#define TER_DAU_PWR		0x40030001
// MSI Error: There was an MSI error in the Dau
#define TER_DAU_MSI		0x40030002
// Transmitter Fault: There was a fault in the transmitter
#define TER_DAU_TXC		0x40030003
// PCIe Error: There was PCIe error detected from the Dau
#define TER_DAU_PCIE		0x40030004
// Streaming Error: An error occurred in the Dau while Streaming
#define TER_DAU_STREAMING		0x40030005
// Sequence Error: An error occurred in the Dau while processing the Sequence
#define TER_DAU_SEQUENCE		0x40030006
// Motion Sensor Error: Invalid WHO_AM_I register reading
#define TER_DAU_MOTION_SENSOR		0x40030007
// Memory Error: Dau Register or memory access error
#define TER_DAU_MEM_ACCESS		0x40030008
// Unknown Error: Unknown Dau error
#define TER_DAU_UNKNOWN		0x40030009
// ASIC Error: ASIC integrity check fail after multiple power up retry cycles
#define TER_DAU_ASIC_CHECK		0x4003000A

//
// Tablet Hardware
//
// Puck Open Failure: Failed to access the Puck
#define TER_TABLET_PUCK_OPEN		0x40040001
// Puck Scan Failure: Failed to put the Puck in scanning mode
#define TER_TABLET_PUCK_SCAN		0x40040002
// Puck Configuration Failure: Failed to configure the Puck
#define TER_TABLET_PUCK_CONFIG		0x40040003
// Puck Firmware: The Puck firmware needs to be updated
#define TER_TABLET_PUCK_FIRMWARE		0x40040004
// Puck Update Firmware: The Puck firmware update failed
#define TER_TABLET_PUCK_UPDATE_FIRMWARE		0x40040005
// Puck Update Firmware Busy: The Puck firmware is already in progress
#define TER_TABLET_PUCK_UPDATE_FIRMWARE_BUSY		0x40040006
// Puck Update Firmware Update Complete: The Puck firmware update completed
#define TER_TABLET_PUCK_UPDATE_FIRMWARE_COMPLETE		0x40007
// Audio Stream: Unable to create an audio stream. Possible audio hardware issue.
#define TER_TABLET_AUDIO_STREAM		0x40040008

//
// Thermal
//
// Probe Thermal High: The probe is too hot
#define TER_THERMAL_PROBE		0x40050001
// Tablet Thermal High: The tablet is too hot
#define TER_THERMAL_TABLET		0x40050002
// Probe Thermal Fail: The probe thermal sensor failed
#define TER_THERMAL_PROBE_FAIL		0x40050003
// Tablet Thermal Fail: The tablet thermal sensor failed
#define TER_THERMAL_TABLET_FAIL		0x40050004

//
// Imaging Engine
//
// Underflow: Data Underflow
#define TER_IE_UNDERFLOW		0x40060001
// Overflow: Data Overflow
#define TER_IE_OVERFLOW		0x40060002
// Scan Converter Sources: Unable to load the Scan Converter Sources
#define TER_IE_SCAN_CONVERTER_SOURCES		0x40060003
// UPS Extract: Unable to extract the UPS
#define TER_IE_UPS_EXTRACT		0x40060004
// UPS Open: Unable to open the UPS
#define TER_IE_UPS_OPEN		0x40060005
// UPS Access: Error accessing the UPS
#define TER_IE_UPS_ACCESS		0x40060006
// DMA Open: Unable to open the DMA buffer
#define TER_IE_DMA_OPEN		0x40060007
// DAU Power: Unable to power up the DAU
#define TER_IE_DAU_POWER		0x40060008
// DAU Power: A reboot is needed to restore connection to Dau
#define TER_IE_DAU_POWER_REBOOT		0x40060009
// DAU Memory: Unable to map DAU memory
#define TER_IE_DAU_MEM		0x4006000A
// Signal Handler Init: Unable to initialize the Signal Handler
#define TER_IE_SIGHANDLE_INIT		0x4006000B
// TBF Init: Unable to initialize the TBF
#define TER_IE_TBF_INIT		0x4006000C
// SSE Init: Unable to initialize the SSE
#define TER_IE_SSE_INIT		0x4006000D
// DAU Closed: The DAU is closed
#define TER_IE_DAU_CLOSED		0x4006000E
// Invalid Gain Value: Gain value must be between 0 and 100
#define TER_IE_GAIN		0x4006000F
// Invalid Image Parameters: Cannot locate an Imaging Case from the supplied parameters
#define TER_IE_IMAGE_PARAM		0x40060010
// Invalid Operation - Running: The operation is invalid while running
#define TER_IE_OPERATION_RUNNING		0x40060011
// Invalid Operation - Stopped: The operation is invalid while stopped
#define TER_IE_OPERATION_STOPPED		0x40060012
// Image Processor - Init Failed: Failed to initialize Image Processor
#define TER_IE_IMAGE_PROCESSOR_INIT		0x40060013
// Cine Buffer - Init Failed: Failed to initialize Cine Buffer
#define TER_IE_CINE_BUFFER_INIT		0x40060014
// Cine Viewer - Init Failed: Failed to initialize Cine Viewer
#define TER_IE_CINE_VIEWER_INIT		0x40060015
// Inference Engine - Init Failed: Failed to initialize Inference Engine
#define TER_IE_INFERENCE_ENGINE_INIT		0x40060016
// Null FIFO: The DMA FIFO is Null
#define TER_IE_DMA_FIFO_NULL		0x40060017
// Cine Viewer - Refresh Failed: Failed to refresh Cine Viewer
#define TER_IE_CINE_VIEWER_REFRESH		0x40060018
// USB Init: Unable to initialize the USB Dau
#define TER_IE_USB_INIT		0x400600B0
// USB Start: Unable to start imaging on USB Dau
#define TER_IE_USB_START		0x400600B1
// USB Error: There was a USB error communicating with the Dau
#define TER_IE_USB_ERROR		0x400600B2
// USB Timed Out: USB transfer timed out
#define TER_IE_USB_TIMED_OUT		0x400600B3
// USB Stalled: USB transfer stalled
#define TER_IE_USB_STALL		0x400600B4
// USB Overflow: USB data overflowed
#define TER_IE_USB_OVERFLOW		0x400600B5
// Sequencer Stalled: Sequencer did not stop in expected time frame
#define TER_IE_SEQ_STALLED		0x400600B6
// Sequence blob error: Failed to read sequence blob from UPS
#define TER_IE_UPS_NULL_BLOB		0x400600B7
// Invalid sequence blob: Size of sequence blob is invalid
#define TER_IE_UPS_INVALID_BLOB		0x400600B8
// LLE Sequence load error: Failed to load sequence linked list
#define TER_IE_SEQ_LLE_LOAD_FAILURE		0x400600B9
// PLD Sequence load error: Failed to load sequence payload
#define TER_IE_SEQ_PLD_LOAD_FAILURE		0x400600BA
// UPS read error: Error occurred retrieving a parameter from UPS
#define TER_IE_UPS_READ_FAILURE		0x400600BB
// Invalid FOV: Error occurred retrieving FOV
#define TER_IE_INVALID_FOV		0x400600BC
// No Active Stages: Image Processor is engaged with no active processing stage
#define TER_IE_NO_ACTIVE_STAGES		0x400600BD
// Invalid Header: Frame header count did not match the expected value
#define TER_IE_INVALID_HEADER		0x400600BE
// Unsupported Imaging Mode: Unsupported imaging mode
#define TER_IE_IMAGING_MODE		0x400600C1
// UPS Integrity: UPS integrity check failed
#define TER_IE_UPS_INTEGRITY		0x400600C2
// Font File Extract: Fail to extract the font file
#define TER_IE_FONT_EXTRACT		0x400600D0
// Font File Open: Unable to open the font file
#define TER_IE_FONT_OPEN		0x400600D1
// DA/ECG Frame Index Mismatch: DA/ECG frame index did not match image frame index
#define TER_IE_DAECG_FRAME_INDEX		0x400600D2
// Invalid Colorflow parameter: Error in the computation of colorflow beam parameters
#define TER_IE_COLORFLOW_PARAM		0x400600D3
// USB Cable Disconnected: USB Device disconnected from Host
#define TER_IE_USB_NO_DEVICE		0x400600D4
// Invalid TLV Detected: Error in TLV format
#define TER_IE_USB_TLV_INVALID		0x400600D5
// Invalid TLV Structure: Error in TLV Parsing
#define TER_IE_USB_TLV_ERROR		0x400600D6
// Invalid TLV parameter: Error in TLV Protocol Parameters
#define TER_IE_USB_TLV_BADPARAMETER		0x400600D7
// Unsupported Probe: Unsupported probe
#define TER_IE_UNSUPPORTED_PROBE		0x400600D8

//
// Ultrasound Manager
//
// Context Failure: Unable to create or initialize the Context
#define TER_UMGR_CONTEXT		0x40070001
// Remote Exception: There was a RemoteException error.  Check the cause.
#define TER_UMGR_REMOTE		0x40070002
// Dau Closed: The Dau is closed.  A new instance must be created and opened.
#define TER_UMGR_DAU_CLOSED		0x40070003
// Invalid Parameter: One of the required parameters is null
#define TER_UMGR_INVALID_PARAM		0x40070004
// Native Window: Unable to get native window from supplied surface
#define TER_UMGR_NATIVE_WINDOW		0x40070005
// Asset Manager: Unable to get native Asset Manager from supplied parameter
#define TER_UMGR_ASSETMGR		0x40070006
// Dau Exists: A Dau instance already exists
#define TER_UMGR_DAU_EXISTS		0x40070007
// Service Access: Could not access the Ultrasound Service
#define TER_UMGR_SVC_ACCESS		0x40070008
// Player Not Attached: The Ultrasound Player is not attached to the Cine Buffer
#define TER_UMGR_PLAYER_NOT_ATTACHED		0x40070009
// Puck Exists: A Puck instance already exists
#define TER_UMGR_PUCK_EXISTS		0x4007000A
// Thor Only: This operation is only available on the Thor Tablet
#define TER_UMGR_THOR_ONLY		0x4007000B
// Puck Closed: The Puck is closed.  A new instance must be created and opened.
#define TER_UMGR_PUCK_CLOSED		0x4007000C
// Dau Not Attached: The Dau is not connected.
#define TER_UMGR_DAU_ATTACHED		0x4007000D
// USB Manager: Could not access USB Manager.
#define TER_UMGR_USB_MANAGER		0x4007000E
// USB Device: Could not open the USB device.
#define TER_UMGR_USB_DEVICE		0x4007000F

//
// AP_I Manager
//
// Too many UTPs: Number of UTPs exceeds limit
#define TER_APIMGR_NUM_UTPS		0x40080001
// Invalid UTP string: Parsing of UTP string obtained from API table failed
#define TER_APIMGR_UTP_STRING		0x40080002
// UTP ID not found: Failed to find a matching UTP ID in UPS
#define TER_APIMGR_UTPID		0x40080003
// Input Vector Setup: Failed to setup input vector
#define TER_APIMGR_INPUT_VECTOR		0x40080004
// AP&I Reference Data: AP&I reference data might be corrupted
#define TER_APIMGR_CORRUPTED_REF_DATA		0x40080005

//
// AI System
//
// Model load error: An AI model failed to load
#define TER_AI_MODEL_LOAD		0x40090001
// Model run error: An AI model failed to run
#define TER_AI_MODEL_RUN		0x40090002
// Frame ID failure: Failed to identify either ED or ES frame (or both)
#define TER_AI_FRAME_ID		0x40090003
// Frame ID warning: Detected ED or ES frame may be invalid. Recommend manual verification
#define TER_AI_FRAME_ID_WARN		0x40090004
// Segmentation failure: Failed to segment the requested item
#define TER_AI_SEGMENTATION		0x40090005
// Smart capture clip too short: Current clip is too short for smart capture
#define TER_AI_SC_NOFRAMES		0x40090006
// Smart capture no range found: No clip range met smart capture requirements
#define TER_AI_SC_NO_VALID_RANGE		0x40090007
// LV not found: No LV region found by AI
#define TER_AI_LV_NOT_FOUND		0x40090008
// Multiple LV: Multiple possible LV regions found by AI
#define TER_AI_MULTIPLE_LV		0x40090009
// LV too small: LV region is too small
#define TER_AI_LV_TOO_SMALL		0x4009000A
// LV uncertainty error: Uncertainty in LV segmentation above threshold
#define TER_AI_LV_UNCERTAINTY_ERROR		0x4009000B
// A4C foreshortened: A4C major axis is significantly shorter than A2C major axis
#define TER_AI_A4C_FORESHORTENED		0x4009000C
// A2C foreshortened: A2C major axis is significantly shorter than A4C major axis
#define TER_AI_A2C_FORESHORTENED		0x4009000D

