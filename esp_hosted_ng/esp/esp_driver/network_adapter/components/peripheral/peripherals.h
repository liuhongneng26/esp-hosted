#ifndef __PERIPHERALS_H__
#define __PERIPHERALS_H__

#include "esp_log.h"
#include "sdkconfig.h"

#include "peripheral_i2c.h"
#include "peripheral_spi.h"
#include "peripheral_gpio.h"

int peripherals_init();

#endif