#ifndef __SENSOR_H__
#define __SENSOR_H__

#include "peripherals.h"

int aht_int(i2c_port_t i2c_port);
int aht_read(float *temperature, float *humidity);

int bh1750_init(i2c_port_t i2c_port);
int bh1750_read();

int tem_internal_init();
float tem_internal_get();
int tem_internal_deinit();

int bmp280_int(i2c_port_t i2c_port);
int bmp280_read(float *temperature, float *pressure, float *humidity);

enum value_tyoe_e
{
    VALUE_TYPE_INT,
    VALYE_TYPE_FLOAT,
};

struct sensor_value
{
    char                *name;
    enum value_tyoe_e   type;
    union
    {
        int     int_value;
        float   float_value;
    };
};

struct sensor_target
{
    char *name;
    struct sensor_value *value;
    int value_num;
};

#endif