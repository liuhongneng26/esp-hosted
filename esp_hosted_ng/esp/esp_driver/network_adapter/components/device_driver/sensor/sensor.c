#include "driver/temperature_sensor.h"

temperature_sensor_handle_t temp_sensor = NULL;

int tem_internal_init()
{
    temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(10, 50);
    temperature_sensor_install(&temp_sensor_config, &temp_sensor);
    if (temp_sensor == NULL)
    {
        printf("temperature_sensor_install error!\n");
        return -1;
    }

    temperature_sensor_enable(temp_sensor);

    return 0;
}

float tem_internal_get()
{
    float tsens_value = 0;
    if (temp_sensor != NULL)
        temperature_sensor_get_celsius(temp_sensor, &tsens_value);

    return tsens_value;
}

int tem_internal_deinit()
{
    if (temp_sensor !=  NULL)
    {
        temperature_sensor_disable(temp_sensor);
        temperature_sensor_uninstall(temp_sensor);
        temp_sensor = NULL;
    }
    return 0;
}