#include <stdio.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/_intsup.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>
#include <sys/unistd.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"

#include "peripherals.h"
#include "gpio_config.h"
#include "shell.h"

#define TAG "main"

void gpio_hander(void *argc)
{
    return ;
}

void ram_info()
{
    ESP_LOGI(TAG, "heap     %lu", esp_get_free_heap_size());
    ESP_LOGI(TAG, "heap dma %lu", heap_caps_get_free_size(MALLOC_CAP_DMA));
    ESP_LOGI(TAG, "heap 8   %lu", heap_caps_get_free_size(MALLOC_CAP_8BIT));
    ESP_LOGI(TAG, "heap 32  %lu", heap_caps_get_free_size(MALLOC_CAP_32BIT));
    ESP_LOGI(TAG, "heap rtc %lu", heap_caps_get_free_size(MALLOC_CAP_RTCRAM));

    ESP_LOGI(TAG, "heap dma %lu", heap_caps_get_largest_free_block(MALLOC_CAP_DMA));
    ESP_LOGI(TAG, "heap 8   %lu", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    ESP_LOGI(TAG, "heap 32  %lu", heap_caps_get_largest_free_block(MALLOC_CAP_32BIT));
    ESP_LOGI(TAG, "heap rtc %lu", heap_caps_get_largest_free_block(MALLOC_CAP_RTCRAM));
    heap_caps_print_heap_info(MALLOC_CAP_DMA);

    // heap_caps_dump_all();
}


void ex_app_main(void)
{

    ESP_LOGI(TAG, "\n\nreset_reason %d", esp_reset_reason());

    ram_info();

    peripherals_init();

    shell_init();

    ESP_LOGI(TAG, "Initial set up done heap %lu\n\n", esp_get_free_heap_size());
}