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

#define TAG "peripherals_gpio"


int gpio_heartbeat()
{
    return 0;
}