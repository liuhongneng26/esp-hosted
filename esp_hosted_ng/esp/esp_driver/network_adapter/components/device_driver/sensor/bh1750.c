#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "sensor.h"

i2c_master_dev_handle_t bh1750_dev_handle = NULL;

int bh1750_init(i2c_port_t i2c_port)
{
    int ret;
    ret = i2c_master_add(i2c_port, &bh1750_dev_handle, 0x23, 100000);
    if (ret != 0)
    {
        printf("aht init error!\n");
    }
    return 0;
}

int bh1750_read()
{
    if (bh1750_dev_handle == NULL)
        return -1;


    uint8_t cmd = 0x10;

    cmd = 0x01;
    p_i2c_master_transmit(bh1750_dev_handle, &cmd, 1, -1);

    cmd = 0x10;
    p_i2c_master_transmit(bh1750_dev_handle, &cmd, 1, -1);

    usleep(100000 * 2);

	uint8_t data[2];

    data[0] = 0;
    data[1] = 0;

    // 读取
    p_i2c_master_receive(bh1750_dev_handle, data, 2, -1);

    int value = (data[0]<<8) + data[1];

    return value;
}