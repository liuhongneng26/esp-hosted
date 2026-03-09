#ifndef __GPIO_CONFIG_H__
#define __GPIO_CONFIG_H__

#include "soc/gpio_num.h"

#define LED_D4  12 
#define LED_D5  13

#define LED_BOARD   LED_D4
#define LED_BOARD_ON 1
#define LED_HEARTBEAT LED_BOARD
#define LED_HEARTBEAT_ON LED_BOARD_ON

#define KEY_BOOT    GPIO_NUM_9

#endif