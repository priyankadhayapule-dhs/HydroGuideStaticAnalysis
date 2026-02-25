//
// Copyright 2017 EchoNous Inc.
//
//

#pragma once


// Imaging modes defined below those defined in the UPS (ImagingModes)
#define IMAGING_MODE_B        0		// B-Mode
#define IMAGING_MODE_COLOR    1     // Color Flow
#define IMAGING_MODE_PW       2     // Pulse-Wave Doppler
#define IMAGING_MODE_M        3     // M-Mode
#define IMAGING_MODE_CW       4     // Continuous Wave Doppler

// DAU ouput data types
#define DAU_DATA_TYPE_B       0     
#define DAU_DATA_TYPE_COLOR   1
#define DAU_DATA_TYPE_PW      2
#define DAU_DATA_TYPE_M       3
#define DAU_DATA_TYPE_CW      4
#define DAU_DATA_TYPE_I2C     5
// Continuation of DAU data types.  These are internal pseudo types.
#define DAU_DATA_TYPE_TEX     6
#define DAU_DATA_TYPE_DA      7
#define DAU_DATA_TYPE_ECG     8
#define DAU_DATA_TYPE_GYRO    9
#define DAU_DATA_TYPE_DA_SCR  10    // DA TEX/SCReen - concatenated data
#define DAU_DATA_TYPE_ECG_SCR 11    // ECG TEX/SCReen - concatenated data

// Input size limits
#define MAX_B_LINES_PER_FRAME	      256
#define MAX_B_SAMPLES_PER_LINE        512

#define MAX_COLOR_LINES_PER_FRAME     90
#define MAX_COLOR_SAMPLES_PER_LINE    224

#define MAX_M_LINES_PER_FRAME         32
#define MAX_M_SAMPLES_PER_LINE        MAX_B_SAMPLES_PER_LINE

#define MAX_B_FRAME_SIZE              (MAX_B_LINES_PER_FRAME*MAX_B_SAMPLES_PER_LINE)
#define MAX_COLOR_FRAME_SIZE          (MAX_COLOR_LINES_PER_FRAME*MAX_COLOR_SAMPLES_PER_LINE)

#define MAX_M_FRAME_SIZE              (MAX_M_LINES_PER_FRAME*MAX_M_SAMPLES_PER_LINE)

#define MAX_TEX_FRAME_SIZE            (1920*1080)     // 1080p
#define MAX_DA_SCR_FRAME_SIZE         (1920*540)      // half 1080p
#define MAX_ECG_SCR_FRAME_SIZE        (1920*540)      // half 1080p

#define MAX_DA_FRAME_SIZE             (2048*4)        // BUGBUG: Need correct value
#define MAX_ECG_FRAME_SIZE            (512*4)         // BUGBUG: Need correct value

#define MAX_HVPS_VOLTS                30.0f
#define MIN_HVPS_VOLTS                 4.0f



