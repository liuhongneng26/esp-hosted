#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_pthread.h"

#include "oled_ui.h"
#include "oled.h"
#include "oled_animation.h"

int bit_map_x = 0;
int bit_map_y = 0;
int *bit_map_c = &bit_map_x;
extern QueueHandle_t ui_event_queue;

int ui_map_main(void *argv, UI_EVENT event)
{
    int ret;
    int do_refresh;

    uint8_t data[] = {0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF,
                      0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF,
                      0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF,
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    bit_map_x = 0;
    bit_map_y = 0;
    bit_map_c = &bit_map_x;
    
    while (1)
    {
        oled_clean();

        oled_draw_bitmap(bit_map_x, bit_map_y, data, 16, 16);

        oled_refresh();
        
        do_refresh = 0;
        do 
        {
            ret = xQueueReceive(ui_event_queue, &event, 0);
            if (ret == pdFAIL)
            {
                if (do_refresh)
                    break;
                else
                    xQueueReceive(ui_event_queue, &event, portMAX_DELAY); 
            }

            switch (event)
            {
                case UI_EVENT_ACTION:
                    break;
                case UI_EVENT_RETURN:
                    return 0;
                    break;
                case UI_EVENT_UP:
                    bit_map_x++;
                    break;
                case UI_EVENT_DOWN:
                     bit_map_x--;
                    break;
                case UI_EVENT_CTRL_DOWN:
                    bit_map_y++;
                    break;
                case UI_EVENT_CTRL_UP:
                    bit_map_y--;
                    break;
                case UI_EVENT_REFRESH:
                    do_refresh = 1;
                    break;
                case UI_EVENT_ANIM_FINISH:
                    return -1;
                    break;
                default:

                    break;
            }
        }
        while(1);
    }
    return 0;
}