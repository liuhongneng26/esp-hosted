#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#include "esp_system.h"
#include "oled_ui.h"

#include "esp_mac.h"
#include "esp_chip_info.h"
#include "esp_idf_version.h"
#include "driver/usb_serial_jtag.h"

#include "sensor.h"

int get_sys_tem(void *argv, char *buf, int len)
{
    snprintf(buf, len, "%0.1f", tem_internal_get());
    return 0;
}

int get_sys_heap(void *argv, char *buf, int len)
{
    snprintf(buf, len, "%lu", esp_get_free_heap_size());
    return 0;
}

int get_sys_mac(void *argv, char *buf, int len)
{
    uint8_t base_mac[6];
    esp_read_mac(base_mac, ESP_MAC_BASE);

    snprintf(buf, len, "%02X:%02X:%02X:%02X:%02X:%02X", base_mac[0], base_mac[1], base_mac[2], \
        base_mac[3], base_mac[4], base_mac[5]);
    
    return 0;
}

int get_sys_chip_info(void *argv, char *buf, int len)
{
    esp_chip_info_t chip_i;
    esp_chip_info(&chip_i);

    switch (chip_i.model)
    {
        case CHIP_ESP32C3:
            snprintf(buf, len, "ESP32C3");
            break;
        case CHIP_ESP32S3:
            snprintf(buf, len, "ESP32S3");
            break;
        default:
            snprintf(buf, len, "ESP32 ?");
            break;
    }

    return 0;
}

int ui_sys_get_idf_version(void *argv, char *buf, int len)
{
    snprintf(buf, len, "%s", esp_get_idf_version());
    return 0;
}

int ui_sys_get_usb_jtag(void *argv, char *buf, int len)
{
    if (usb_serial_jtag_is_connected())
    {
        snprintf(buf, len, "ON");
    }
    else
    {
        snprintf(buf, len, "OFF");
    }
    return 0;
}

int ui_func_sys_reboot(void *argv, UI_EVENT event)
{
    esp_restart();
    return 0;
}



int ui_sys_main(void *argv, UI_EVENT event)
{
    oled_ui_menu_t menu[] = 
    {
        {"<<<", UI_ACTION_RETURN, NULL},
        {"", UI_ACTION_NULL, NULL, get_sys_chip_info},
        {"tem  ", UI_ACTION_NULL, NULL, get_sys_tem},
        {"heap ", UI_ACTION_NULL, NULL, get_sys_heap},
        {"mac  ", UI_ACTION_NULL, NULL, get_sys_mac},
        {"idf  ", UI_ACTION_NULL, NULL, ui_sys_get_idf_version},
        {"JTAG ", UI_ACTION_NULL, NULL, ui_sys_get_usb_jtag},
        {"reboot", UI_ACTION_FUNC, ui_func_sys_reboot},
        {"self", UI_ACTION_FUNC, ui_sys_main},
    };

    oled_ui(menu, sizeof(menu)/sizeof(menu[0]));

    return 0;
}