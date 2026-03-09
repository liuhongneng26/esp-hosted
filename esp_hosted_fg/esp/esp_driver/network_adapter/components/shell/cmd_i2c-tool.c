#include "esp_console.h"
#include "esp_err.h"
#include "esp_system.h"
#include "esp_log.h"
#include "argtable3/argtable3.h"
#include "hal/i2c_types.h"
#include "sdkconfig.h"
#include <stdio.h>

#include "peripheral_i2c_tool.h"

#define TAG "sys"

static int do_i2cdetect_cmd(int argc, char **argv)
{
    if (argc <= 1)
    {
        int i;
        for (i = 0; i < I2C_NUM_MAX; i++)
        {
            printf("i2c-%d:\n", i);
            i2cdetect(i);
        }
    }
    else
    {
        i2cdetect(atoi(argv[1]));
    }

    return 0;
}

void register_i2cdetect(void)
{
    const esp_console_cmd_t i2cdetect_cmd = {
        .command = "i2cdetect",
        .help = "Scan I2C bus for devices",
        .hint = NULL,
        .func = &do_i2cdetect_cmd,
        .argtable = NULL
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&i2cdetect_cmd));
}