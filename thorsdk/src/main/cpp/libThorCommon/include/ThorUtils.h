//
// Copyright 2016 EchoNous, Inc.
//
#pragma once

#include <stdint.h>
#include <stdlib.h>

// These functions will output 32-bit data as hexadecimal values in a form for
// easy readability.
//
// The output takes the following form:
//
//      <address00>: <data0> <data1> <data2> <data3> <data4> <data5> <data6> <data7>
//      <address20>: <data8> ...
//
// There are two variants: One with and one without a reference Address
//
// Using the variant with the reference address allows output to reflect the
// address scheme used by the hardware.  Otherwise the addresses are of the
// input buffer of the process' virtual memory space.
//
// Example:
//
//  Assume there is physical memory allocated at address 0x82000000.
//  That same memory is mapped to the application at address 0xFA123400.
//  Assume that the value 0x1234 is stored at this memory location.
//
//  {
//      uint32_t*   bufPtr = 0xFA123400
//
//      *bufPtr = 0x1234;
//
//      printBuffer32(bufPtr, 1);
//      printBuffer32(bufPtr, 1, 0x82000000);
//  }
//
//  Results in the following output:
//
//      0xFA123400: 0x1234
//      0x82000000: 0x1234
//

// Will dump the buffer to console (stdout) as 32 bit hex values
void printBuffer32(uint32_t* bufPtr, uint32_t bufSize, size_t refAddr);

void printBuffer32(uint32_t* bufPtr, uint32_t bufSize);


//
// Same as above, except the output goes to logcat
//
void logBuffer32(uint32_t* bufPtr, uint32_t bufSize, size_t refAddr);

void logBuffer32(uint32_t* bufPtr, uint32_t bufSize);

// Get default Dau data type bitfield based on imaging mode
uint32_t getDauDataTypes(uint32_t imagingMode, bool isDaOn, bool isEcgOn, bool isUsOn = true);

// Get timeSpan from Sweep Speed Index
float    getTimeSpan(uint32_t sweepSpeedIndex, bool isTCD);

// Get sweep Speed in mm/sec from Sweep Speed Index
float    getSweepSpeedMmSec(uint32_t sweepSpeedIndex, bool isTCD);

// Get MLines to average from Sweep Speed Index
uint32_t  getMLinesToAverage(uint32_t sweepSpeedIndex);

//
// To get around compiler errors for unused parameters.  
//
// Example: 
//
//      void func(int x)
//      {
//          UNUSED(x);
//          ...
//      }
//
#define UNUSED(x) (void)(x)

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif


#ifndef NELEM
#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif

//
// Macros for logcat output
//

#ifndef ANDROID
#include <cstdio>
#define  LOGE(...)  printf(__VA_ARGS__)
#define  LOGW(...)  printf(__VA_ARGS__)
#define  LOGD(...)  printf(__VA_ARGS__)
#define  LOGI(...)  printf(__VA_ARGS__)

#define  ALOGE(...)  printf(__VA_ARGS__)
#define  ALOGW(...)  printf(__VA_ARGS__)
#define  ALOGD(...)  printf(__VA_ARGS__)
#define  ALOGI(...)  printf(__VA_ARGS__)
#else /* ANDROID */
#ifndef ALOGE
#include <android/log.h>
#ifndef LOG_TAG
#define LOG_TAG    "ThorUtils"
#endif

#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)

#define  ALOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  ALOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define  ALOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  ALOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#endif /* ALOGE */
#endif /* ANDROID */


#ifndef LOG_FATAL_IF
#define LOG_FATAL_IF(cond, ...) ((void)0)
#endif
#ifndef LOG_FATAL
#define LOG_FATAL(...) ((void)0)
#endif

// Return an error (or any other value) and print a log message
#define RETURN_ERROR(err, fmt, ...) do{LOGE("Error in \"%s\" line %d: " fmt, __func__, __LINE__, ##__VA_ARGS__); return (err);}while(0)

#ifndef ALOG_ASSERT
#define ALOG_ASSERT(cond, ...) LOG_FATAL_IF(!(cond), ## __VA_ARGS__)
#endif
