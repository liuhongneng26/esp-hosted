#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "peripheral_gpio.h"
#include "gpio_config.h"

#define TAG "peripherals_gpio"


void gpio_heartbeat(int value)
{
#ifdef LED_HEARTBEAT
#ifdef LED_HEARTBEAT_ON
    if (value)
        gpio_set_level(LED_HEARTBEAT, LED_HEARTBEAT_ON);
    else
        gpio_set_level(LED_HEARTBEAT, !LED_HEARTBEAT_ON);
#endif
#endif
    return;
}