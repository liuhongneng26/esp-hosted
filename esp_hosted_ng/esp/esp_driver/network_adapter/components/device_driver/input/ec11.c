#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "driver/gpio.h"
#include "driver/gpio_filter.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "iot_button.h"
#include "button_gpio.h"

#include "ec11.h"
#include "oled_ui.h"

#define TAG "EC11"
#define EC11_DEBUG 0


gpio_num_t g_ec11_l;
gpio_num_t g_ec11_r;

int press = 0;

void ec11_key_event(void *arg, void *data)
{
    button_event_t event = iot_button_get_event(arg);

#if (EC11_DEBUG == 1)
    ESP_DRAM_LOGI(TAG, "BUTTON_event %d\n", event);
#endif

    switch (event)
    {
        case BUTTON_SINGLE_CLICK:
            oled_ui_event(UI_EVENT_ACTION);
#if (EC11_DEBUG == 1)
            ESP_DRAM_LOGI(TAG, "BUTTON_SINGLE_CLICK");
#endif
            break;
        case BUTTON_DOUBLE_CLICK:
            oled_ui_event(UI_EVENT_RETURN);
#if (EC11_DEBUG == 1)
            ESP_DRAM_LOGI(TAG, "BUTTON_DOUBLE_CLICK");
#endif
            break;
        case BUTTON_PRESS_DOWN:
            press = 1;
            break;
        case BUTTON_PRESS_UP:
            press = 0;
            break;
        default:
            break;
    }

    return;
}

uint8_t dir[2];
uint8_t stat = 0;

void ec11_l_event(void *arg, void *data)
{
    button_event_t event = iot_button_get_event(arg);
#if (EC11_DEBUG == 1)
    ESP_DRAM_LOGI(TAG, "l %d\n", event);
#endif
    if (event == BUTTON_PRESS_DOWN)
    {
        stat |= (1 << 0);
        if (dir[0] == 0)
            dir[0] = 1;
        
    }
    else if (event == BUTTON_PRESS_UP)
    {
        stat &= ~(1 << 0);
        if (dir[1] == 0)
            dir[1] = 1;

        if (stat == 0)
        {
            goto clean;
        }
    }

    return;

clean:
#if (EC11_DEBUG == 1)
    ESP_DRAM_LOGI(TAG, "dir %d %d\n", dir[0], dir[1]);
#endif

    if (dir[0] == dir[1])
    {
        if (dir[0] == 1)
            oled_ui_event(press? UI_EVENT_CTRL_UP: UI_EVENT_UP);
        else
            oled_ui_event(press? UI_EVENT_CTRL_DOWN: UI_EVENT_DOWN);
    }

    stat = 0;
    dir[0] = 0;
    dir[1] = 0;
    return;
}

void ec11_r_event(void *arg, void *data)
{
    button_event_t event = iot_button_get_event(arg);
#if (EC11_DEBUG == 1)
    ESP_DRAM_LOGI(TAG, "r %d\n", event);
#endif
    if (event == BUTTON_PRESS_DOWN)
    {
        stat |= (1 << 1);
        if (dir[0] == 0)
            dir[0] = -1;
    }
    else if (event == BUTTON_PRESS_UP)
    {
        stat &= ~(1 << 1);
        if (dir[1] == 0)
            dir[1] = -1;

        if (stat == 0)
        {
            goto clean;
        }
    }

    return;

clean:

#if (EC11_DEBUG == 1)
    ESP_DRAM_LOGI(TAG, "dir %d %d\n", dir[0], dir[1]);
#endif

    if (dir[0] == dir[1])
    {
        if (dir[0] == 1)
            oled_ui_event(press? UI_EVENT_CTRL_UP: UI_EVENT_UP);
        else
            oled_ui_event(press? UI_EVENT_CTRL_DOWN: UI_EVENT_DOWN);
    }

    stat = 0;
    dir[0] = 0;
    dir[1] = 0;
    return;

}

int ec11_ir_init()
{
    const button_config_t btn_cfg[2] = {0};
    button_handle_t btn[2] = {0};

    button_gpio_config_t btn_gpio_cfg =
    {
        .gpio_num = g_ec11_l,
        .active_level = 0,
    };

    esp_err_t ret = iot_button_new_gpio_device(&btn_cfg[0], &btn_gpio_cfg, &btn[0]);
    if (ret != ESP_OK)
    {
        return -1;
    }
    iot_button_register_cb(btn[0], BUTTON_PRESS_DOWN, NULL, ec11_l_event, (void *)BUTTON_PRESS_DOWN);
    iot_button_register_cb(btn[0], BUTTON_PRESS_UP, NULL, ec11_l_event, (void *)BUTTON_PRESS_UP);

    btn_gpio_cfg.gpio_num = g_ec11_r;

    ret = iot_button_new_gpio_device(&btn_cfg[1], &btn_gpio_cfg, &btn[1]);
    if (ret != ESP_OK)
    {
        return -1;
    }
    iot_button_register_cb(btn[1], BUTTON_PRESS_DOWN, NULL, ec11_r_event, (void *)BUTTON_PRESS_DOWN);
    iot_button_register_cb(btn[1], BUTTON_PRESS_UP, NULL, ec11_r_event, (void *)BUTTON_PRESS_UP);

    return 0;
}

int ec11_init(int gpio_l, int gpio_r, int gpio_k)
{

    g_ec11_l = gpio_l;
    g_ec11_r = gpio_r;

    const button_config_t btn_cfg = {0};
    const button_gpio_config_t btn_gpio_cfg = {
        .gpio_num = gpio_k,
        .active_level = 0,
    };

    button_handle_t btn = NULL;
    esp_err_t ret = iot_button_new_gpio_device(&btn_cfg, &btn_gpio_cfg, &btn);
    if (ret != ESP_OK)
    {
        return -1;
    }

    esp_err_t err = 0;

    assert(btn);
    err = iot_button_register_cb(btn, BUTTON_PRESS_DOWN, NULL, ec11_key_event, (void *)BUTTON_PRESS_DOWN);
    err |= iot_button_register_cb(btn, BUTTON_PRESS_UP, NULL, ec11_key_event, (void *)BUTTON_PRESS_UP);
    // err |= iot_button_register_cb(btn, BUTTON_PRESS_REPEAT, ec11_key_event, (void *)BUTTON_PRESS_REPEAT);
    // err |= iot_button_register_cb(btn, BUTTON_PRESS_REPEAT_DONE, ec11_key_event, (void *)BUTTON_PRESS_REPEAT_DONE);
    err |= iot_button_register_cb(btn, BUTTON_SINGLE_CLICK, NULL, ec11_key_event, (void *)BUTTON_SINGLE_CLICK);
    err |= iot_button_register_cb(btn, BUTTON_DOUBLE_CLICK, NULL, ec11_key_event, (void *)BUTTON_DOUBLE_CLICK);
    // err |= iot_button_register_cb(btn, BUTTON_LONG_PRESS_START, ec11_key_event, (void *)BUTTON_LONG_PRESS_START);
    // err |= iot_button_register_cb(btn, BUTTON_LONG_PRESS_HOLD, ec11_key_event, (void *)BUTTON_LONG_PRESS_HOLD);
    // err |= iot_button_register_cb(btn, BUTTON_LONG_PRESS_UP, ec11_key_event, (void *)BUTTON_LONG_PRESS_UP);
    ESP_ERROR_CHECK(err);

    ec11_ir_init();
    printf("EC11 init ok!\n");
    return 0;
}