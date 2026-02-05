#ifndef __PERIPHERAL_GPIO_H__
#define __PERIPHERAL_GPIO_H__

#include "soc/gpio_num.h"
#include "driver/gpio.h"
#include "driver/gpio_filter.h"
#include "soc/gpio_num.h"

struct gpio_init
{
    char *name;
    gpio_num_t          pin;
    gpio_mode_t         dir;
    uint32_t            def_level;
    gpio_pull_mode_t    pull;

    gpio_int_type_t             intr_type;
    gpio_isr_t                  isr_handler;
    void                        *args;

    int                         filter;
    gpio_glitch_filter_handle_t filter_handle;
};

#endif