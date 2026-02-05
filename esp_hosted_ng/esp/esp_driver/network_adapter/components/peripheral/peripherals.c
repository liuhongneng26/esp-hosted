
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "peripherals.h"
#include "peripherals_config.h"

#define TAG "peripherals"


void inside_gpio_hander(void *argc)
{
    int gpio = (int) argc;
    ESP_DRAM_LOGI(TAG, "gpio_hander %d\n", gpio);
    return ;
}

esp_err_t gpio_master_init(void)
{
    int i = 0;
    gpio_pin_glitch_filter_config_t filter_config;

    for (i = 0; i < sizeof(gpio_init_list)/sizeof(gpio_init_list[0]); i++)
    {
        if (gpio_init_list[i].pin < 0 || gpio_init_list[i].pin >= GPIO_NUM_MAX)
            continue;

        gpio_reset_pin(gpio_init_list[i].pin);
        gpio_set_direction(gpio_init_list[i].pin, gpio_init_list[i].dir);
        gpio_set_pull_mode(gpio_init_list[i].pin, gpio_init_list[i].pull);

        if (gpio_init_list[i].dir == GPIO_MODE_OUTPUT || gpio_init_list[i].dir ==  GPIO_MODE_INPUT_OUTPUT)
            gpio_set_level(gpio_init_list[i].pin, gpio_init_list[i].def_level);

        if (gpio_init_list[i].intr_type != GPIO_INTR_DISABLE)
        {
            gpio_set_intr_type(gpio_init_list[i].pin, gpio_init_list[i].intr_type);
            gpio_install_isr_service(i);

            if (gpio_init_list[i].isr_handler != NULL)
                gpio_isr_handler_add(gpio_init_list[i].pin, gpio_init_list[i].isr_handler, gpio_init_list[i].args);
            else
                gpio_isr_handler_add(gpio_init_list[i].pin, inside_gpio_hander, gpio_init_list[i].args);
        }

        if (gpio_init_list[i].filter)
        {
            filter_config.gpio_num = gpio_init_list[i].pin;
            gpio_new_pin_glitch_filter(&filter_config, &gpio_init_list[i].filter_handle);
            gpio_glitch_filter_enable(gpio_init_list[i].filter_handle);
        }

        printf("[%d]GPIO%d init success: %s\n", i, gpio_init_list[i].pin, gpio_init_list[i].name);
    }

    return 0;
}

int peripherals_init()
{
    int i, ret;

    gpio_master_init();

    for (i = 0; i < sizeof(g_i2c_target)/sizeof(g_i2c_target[0]); i++)
    {
        if (g_i2c_target[i].port < 0 || g_i2c_target[i].port >= I2C_NUM_MAX)
        {
            continue;
        }

        ret = i2c_master_init(g_i2c_target[i].port, g_i2c_target[i].sda, g_i2c_target[i].scl);
        if (ret != ESP_OK)
        {
            printf("[%d]I2C%d init error %d\n", i, g_spi_target[i].port, ret);
        }
        else
        {
            printf("[%d]I2C%d init success\n", i, g_spi_target[i].port);
        }
    }


    for (i = 0; i < sizeof(g_spi_target)/sizeof(g_spi_target[0]); i++)
    {
        if (g_spi_target[i].port < 0 || g_spi_target[i].port >= SPI_HOST_MAX)
        {
            continue;
        }

        ret = spi_master_init(g_spi_target[i].port, g_spi_target[i].miso, g_spi_target[i].mosi,g_spi_target[i].clk);
        if (ret != ESP_OK)
        {
            printf("[%d]SPI%d init error %d\n", i, g_spi_target[i].port, ret);
        }
        else
        {
            printf("[%d]SPI%d init success\n", i, g_spi_target[i].port);
        }
    }


    return 0;
}