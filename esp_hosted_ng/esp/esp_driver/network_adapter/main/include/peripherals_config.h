#ifndef __PERIPHERALS_CONFIG_H__
#define __PERIPHERALS_CONFIG_H__

#include "gpio_config.h"
#include "peripherals.h"

void gpio_hander(void *argc);

struct gpio_init gpio_init_list[] = 
{
    {"led board",   LED_BOARD,  GPIO_MODE_INPUT_OUTPUT,     0, GPIO_PULLDOWN_ONLY,  GPIO_INTR_DISABLE},
    {"led board1",  LED_BOARD1, GPIO_MODE_INPUT_OUTPUT,     0, GPIO_PULLDOWN_ONLY,  GPIO_INTR_DISABLE},
    {"ket boot",    KEY_BOOT,   GPIO_MODE_INPUT,            0, GPIO_PULLUP_ONLY,    GPIO_INTR_NEGEDGE, gpio_hander, NULL, 1},
};

struct i2c_target g_i2c_target[] = 
{

};

struct spi_target g_spi_target[] = 
{

};

#endif