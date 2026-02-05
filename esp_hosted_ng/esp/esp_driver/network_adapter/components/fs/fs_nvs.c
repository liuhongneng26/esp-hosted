#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "fs_nvs.h"

int fs_nvs_init()
{
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    // Open
    nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    else
    {
        // Read
        int32_t restart_counter = 0;
        err = nvs_get_i32(my_handle, "restart_counter", &restart_counter);
        switch (err)
        {
            case ESP_OK:
                printf("Restart counter = %" PRIu32 "\n", restart_counter);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                printf("The value is not initialized yet!\n");
                break;
            default :
                printf("Error (%s) reading!\n", esp_err_to_name(err));
        }

        // Write
        restart_counter++;
        err = nvs_set_i32(my_handle, "restart_counter", restart_counter);
        if (err != ESP_OK)
        {
            printf("update restart_counter error %d\n", err);
        }

        err = nvs_commit(my_handle);
        if (err != ESP_OK)
        {
            printf("commit restart_counter error %d\n", err);
        }

        // Close
        nvs_close(my_handle);
    }

    return 0;
}
