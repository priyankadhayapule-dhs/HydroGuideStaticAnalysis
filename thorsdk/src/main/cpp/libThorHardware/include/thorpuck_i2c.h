//
// Copyright 2018 EchoNous Inc.
//
//

#ifndef THORPUCK_I2C_H
#define THORPUCK_I2C_H

#include <stdint.h>
#include <stdbool.h>

#ifndef ALOGE
#include <android/log.h>

#define  ALOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  ALOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define  ALOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  ALOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#endif

int thorpuck_i2c_open(int busnum);
void thorpuck_i2c_close(int bus);

bool thorpuck_i2c_write_read(int bus, uint8_t write_addr, uint8_t* write_buffer, int write_len, uint8_t read_addr, uint8_t* read_buffer, int read_len);
bool thorpuck_i2c_write(int bus, uint8_t write_addr, uint8_t* write_buffer, int write_len);
bool thorpuck_i2c_read(int bus, uint8_t read_addr, uint8_t* read_buffer, int read_len);

#endif //THORPUCK_I2C_H
