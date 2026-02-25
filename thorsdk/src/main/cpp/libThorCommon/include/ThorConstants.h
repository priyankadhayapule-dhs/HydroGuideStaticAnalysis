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
#define DAU_DATA_TYPE_PW_ADO  12    // PW Audio
#define DAU_DATA_TYPE_CW_ADO  13    // CW Audio
#define DAU_DATA_TYPE_DOP_PM  14    // Doppler Peak Mean
#define DAU_DATA_TYPE_DPMAX_SCR     15      // Doppler Peak MAX - concatenated data
#define DAU_DATA_TYPE_DPMEAN_SCR    16      // Doppler Peak Mean

// Probe Types
#define DEV_PROBE_MASK          0x7FFFFFFF
#define PROBE_TYPE_TORSO3       0x3
#define PROBE_TYPE_TORSO1       0x4
#define PROBE_TYPE_LINEAR       0x5

// Probe Shape
#define PROBE_SHAPE_PHASED_ARRAY 0
#define PROBE_SHAPE_LINEAR       1
#define PROBE_SHAPE_CURVILINEAR  2
#define PROBE_SHAPE_PHASED_ARRAY_FLATTOP    4

// Global ID in dB
#define BLADDER_GLOBAL_ID  9
#define TCD_GLOBAL_ID      10

// TCD PRF Limit
#define TCD_LIMIT_PRFS     5u

// Color Mode
#define COLOR_MODE_CVD 0
#define COLOR_MODE_CPD 1

// Input size limits
#define MAX_B_LINES_PER_FRAME	      256
#define MAX_B_SAMPLES_PER_LINE        512

#define MAX_COLOR_LINES_PER_FRAME     90
#define MAX_COLOR_SAMPLES_PER_LINE    240

#define MAX_M_LINES_PER_FRAME         32
#define MAX_M_SAMPLES_PER_LINE        MAX_B_SAMPLES_PER_LINE

#define MAX_B_FRAME_SIZE              (MAX_B_LINES_PER_FRAME*MAX_B_SAMPLES_PER_LINE)
#define MAX_COLOR_FRAME_SIZE          (MAX_COLOR_LINES_PER_FRAME*MAX_COLOR_SAMPLES_PER_LINE)

#define MAX_M_FRAME_SIZE              (MAX_M_LINES_PER_FRAME*MAX_M_SAMPLES_PER_LINE)

#define MAX_TEX_FRAME_SIZE            (1920*1080)       // 1080p
#define MAX_DA_SCR_FRAME_SIZE         (1920*540)        // half 1080p
#define MAX_ECG_SCR_FRAME_SIZE        (1920*540)        // half 1080p

#define MAX_DA_FRAME_SIZE             (4096*4)        // BUGBUG: Need correct value
#define MAX_DA_FRAME_SIZE_LEGACY      (2048*4)        // BUGBUG: Need correct value
#define MAX_ECG_FRAME_SIZE            (512*4)         // BUGBUG: Need correct value

#define MAX_HVPS_VOLTS                35.0f
#define MIN_HVPS_VOLTS                3.0f

#define MAX_PW_INPUT_SAMPLES_PER_LINE 26
#define MAX_PW_INPUT_LINES_PER_FRAME  768
#define PW_INPUT_BYTES_PER_SAMPLE     16
#define MAX_PW_INPUT_FRAME_SIZE       (MAX_PW_INPUT_SAMPLES_PER_LINE * MAX_PW_INPUT_LINES_PER_FRAME * PW_INPUT_BYTES_PER_SAMPLE)

#define MAX_DOP_WALL_FILTER_TAP       128
#define MAX_FFT_SIZE                  512

#define MAX_PW_FFT_SIZE               256
#define MAX_PW_FFT_WINDOW_SIZE        128
#define MAX_PW_HILBERT_FILTER_TAP     64
#define MAX_PW_AUDIO_LP_FILTER_TAP    128

#define MAX_PW_FRAME_SIZE             (MAX_PW_FFT_SIZE*48*4)                    // MAX FFT size * 48 * 4 (float)
#define MAX_PW_ADO_FRAME_SIZE         (MAX_PW_INPUT_LINES_PER_FRAME*4*2)        // 2ch float

#define MAX_CW_INPUT_LINES_PER_FRAME  1312
#define MAX_CW_FRAME_SIZE             (MAX_PW_FFT_SIZE*48*4)                    // MAX FFT size * 48 * 4 (float) - same as PW
#define MAX_CW_ADO_FRAME_SIZE         (MAX_CW_INPUT_LINES_PER_FRAME*4*2)        // 2ch float

#define MAX_CW_BANDPASS_FILTER_TAP    256
#define MAX_CW_SAMPLES_PER_FRAME_PRE_DECIM      8000      // 7800 + 200 extra
#define MAX_CW_SAMPLES_PER_FRAME                1400      // 7800 with decimation factor 6 + 100 extra

#define MAX_DOPPLER_NUM_LINES_OUT_PER_FRAME     64        // number of lines output after FFT per frame
#define MAX_DOPPLER_SMOOTH_KERNEL_SIZE          16

#define MAX_DOP_PEAK_MEAN_SIZE         (MAX_DOPPLER_NUM_LINES_OUT_PER_FRAME * 2 * 4)    // Float Peak and Mean interleaved
#define MAX_DOP_PEAK_MEAN_SCR_SAMPLE_SIZE 8000
#define MAX_DOP_PEAK_MEAN_SCR_SIZE     (MAX_DOP_PEAK_MEAN_SCR_SAMPLE_SIZE * 4 * 2)

/************************************************************
* \brief PCIe Command FCS Response
/************************************************************/
#define FCS_250_RESP_OK     0XFFFF0101
#define FCS_208_RESP_OK     0XFFFF0102
#define FCS_RESP_ERR    	0XFFFF010E
#define FCS_RESP_INVALID    0XFFFF010F

/************************************************************
* \enum defined FCS type
/************************************************************/
enum FCSType
{
    AX_FCS_250MHz = 1,
    AX_FCS_208MHz
};