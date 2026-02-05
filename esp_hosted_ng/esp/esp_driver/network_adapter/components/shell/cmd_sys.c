#include "esp_console.h"
#include "esp_err.h"
#include "esp_system.h"
#include "esp_log.h"
#include "argtable3/argtable3.h"
#include "sdkconfig.h"
#include <stdio.h>


int cmd_sys_free(int argc, char **argv)
{
    uint32_t free_heap = esp_get_free_heap_size();
    printf("%lu, %lu\n", free_heap, free_heap/1024);
    free_heap = esp_get_minimum_free_heap_size();
    printf("%lu, %lu\n", free_heap, free_heap/1024);
    return 0;
}

int cmd_sys_reboot(int argc, char **argv)
{
    esp_restart();
    return 0;
}

const esp_console_cmd_t cmd[] = 
{
    {
            .command = "free",
            .help = "",
            .hint = NULL,
            .func = cmd_sys_free,
        },
    {
            .command = "reboot",
            .help = "",
            .hint = NULL,
            .func = cmd_sys_reboot,
        },

};

void register_cmd_sys(void)
{
    int i;
    esp_err_t ret;

    for (i = 0; i < sizeof(cmd)/sizeof(cmd[0]); i++)
    {
        ret = esp_console_cmd_register(&cmd[i]);
        if (ret != ESP_OK)
        {
            printf("cmd %s register error %d\n", cmd[i].command, ret);
        }
    }

    return;
}