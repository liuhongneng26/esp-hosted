#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "sensor.h"

i2c_master_dev_handle_t dev_handle;

#define FLOAT_ONE(x) (((int)(x * 10)) / 10.0f)

int aht_int(i2c_port_t i2c_port)
{
    int ret ;
    ret = i2c_master_add(i2c_port, &dev_handle, 0x38, 100000);
    if (ret != 0)
    {
        printf("aht init error!\n");
    }

    return 0;
}

int aht_read(float *temperature, float *humidity)
{
    uint8_t buf[6];
    uint32_t raw;
    memset(buf, 0, sizeof(uint8_t) * 6);

    uint8_t tir[3] =
        {
            [0] = 0xAC,
            [1] = 0x33,
            [2] = 0x00,
        };
    p_i2c_master_transmit(dev_handle, tir, 3, -1);

    usleep(80000);

    // 读取
    p_i2c_master_receive(dev_handle, buf, 6, -1);

    if (humidity)
    {
        raw = ((uint32_t)buf[1] << 12) | ((uint32_t)buf[2] << 4) | (buf[3] >> 4);
        *humidity = (float)raw * 100 / 0x100000;

        if (*humidity < 0)
            *humidity = 100;

        if (*humidity > 100.0)
            *humidity = 100;

        *humidity = FLOAT_ONE(*humidity);
    }

    if (temperature)
    {
        raw = ((uint32_t)(buf[3] & 0x0f) << 16) | ((uint32_t)buf[4] << 8) | buf[5];
        *temperature = (float)raw * 200 / 0x100000 - 50;
    
        if (*temperature < -40)
            *temperature = -40;

        if (*temperature > 85)
            *temperature = 85;

        *temperature = FLOAT_ONE(*temperature);
    }

    return 0;
}