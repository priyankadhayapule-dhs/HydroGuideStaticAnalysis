//
// Copyright 2018 EchoNous Inc.
//
//

#define LOG_TAG "thorpuck"

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "thorpuck_i2c.h"

void thorpuck_i2c_close(int bus)
{
    if(bus >= 0) {
        close(bus);
    }
}

int thorpuck_i2c_open(int busnum)
{
    char busname[32];
    int bus = -1;

    snprintf(busname, sizeof(busname), "/dev/i2c-%d", busnum);
    bus = open(busname, O_RDWR);
    if(bus < 0) {
        ALOGE("Failed to open i2c device %d", busnum);
    }

    return bus;
}

bool thorpuck_i2c_write_read(int bus, uint8_t write_addr, uint8_t* write_buffer, int write_len, uint8_t read_addr, uint8_t* read_buffer, int read_len)
{
    struct i2c_msg msgs[2] = {};
    int num_msgs = 0;
    struct i2c_rdwr_ioctl_data d;

    if(bus < 0) {
        goto err;
    }

    if(write_len >= 0) {
        msgs[num_msgs].addr = (write_addr >> 1);
        msgs[num_msgs].flags = 0;
        msgs[num_msgs].len = write_len;
        msgs[num_msgs].buf = write_buffer;
        num_msgs++;
    }

    if(read_len >= 0) {
        msgs[num_msgs].addr = (read_addr >> 1);
        msgs[num_msgs].flags = I2C_M_RD;
        msgs[num_msgs].len = read_len;
        msgs[num_msgs].buf = read_buffer;
        num_msgs++;
    }

    d.msgs = msgs;
    d.nmsgs = num_msgs;

    if(ioctl(bus, I2C_RDWR, &d) < 0) {
        ALOGE("Failed i2c transaction: (%d): %s", errno, strerror(errno));
        goto err;
    }

    return true;

err:
    return false;
}

bool thorpuck_i2c_write(int bus, uint8_t write_addr, uint8_t* write_buffer, int write_len)
{
    return thorpuck_i2c_write_read(bus, write_addr, write_buffer, write_len, 0, NULL, -1);
}

bool thorpuck_i2c_read(int bus, uint8_t read_addr, uint8_t* read_buffer, int read_len)
{
    return thorpuck_i2c_write_read(bus, 0, NULL, -1, read_addr, read_buffer, read_len);
}
