#include "app_main_ex.h"
#include "shell.h"
#include "peripherals.h"
#include "gpio_config.h"
#include <sys/time.h>
#include <sys/unistd.h>
#include <unistd.h>

#define TAG "main_ex"

void gpio_hander(void *argc)
{
    
}

void ex_app_main()
{
    peripherals_init();

    shell_init();

    ESP_LOGI(TAG, "Initial set up don\n\n\ne");
    uint8_t mod = 0x50;
    int i;
    #define FREQ 8
    while (1)
    {
        for (i = 0; i < FREQ; i++)
        {
            if (mod & (1 << i))
                gpio_set_level(LED_BOARD, 1);
            else
                gpio_set_level(LED_BOARD, 0);

            usleep(1000000/FREQ);
        }
    }
}