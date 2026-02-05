#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "driver/gpio_filter.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "driver/temperature_sensor.h"
#include "driver/i2c_master.h"
#include "esp_timer.h"

#include "peripherals.h"
#include "oled.h"
#include "oled_ui.h"

extern QueueHandle_t ui_event_queue;

pthread_mutex_t g_printf_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct
{
    char string[17];
}str_s;


str_s g_buf[60];
int g_target_num = 0; //0 - 59
int g_target_last = 0;

int oled_ui_terminal(void *argv, UI_EVENT event)
{
    int do_refresh, ret;
    int i = 0;

    int target;

    do
    {
        pthread_mutex_lock(&g_printf_mutex);

        target = g_target_last - 1;
        if (target < 0)
            target = 60 - 1;
        
        oled_clean();
        for (i = 0; i <= 7; i++)
        {
            oled_page_text(7-i, g_buf[target].string, strlen(g_buf[target].string),0);
            target--;
            if (target < 0)
                target = 60 - 1;

        }
        oled_refresh();

        pthread_mutex_unlock(&g_printf_mutex);

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

                    break;
                case UI_EVENT_DOWN:

                    break;
                case UI_EVENT_CTRL_DOWN:

                    break;
                case UI_EVENT_CTRL_UP:

                    break;
                case UI_EVENT_REFRESH:
                    do_refresh = 1;
                    break;
                default:

                    break;
            }
        }
        while(1);
    }
    while(1);

    return 0;
}

int oled_ui_terminal_printf(const char* fmt, ...)
{
    pthread_mutex_lock(&g_printf_mutex);
    char tmp_buf[128];
    int len;
    int i;
    int count = 0;

	va_list ap;
	va_start(ap, fmt);
	vsnprintf(tmp_buf, 128, fmt, ap);
	va_end(ap);	

    len = strlen(tmp_buf);
    if (len <= 0)
        goto func_return;

    for (i = 0; i < len; i++)
    {
        //todo support \t
        if (tmp_buf[i] == '\n' || tmp_buf[i] == '\r' || count >= 16)
        {
            g_buf[g_target_last].string[count] = '\0';
            count = 0;
            g_target_last++;
            if (g_target_last >= 60)
            {
                g_target_last = 0;
            }

        }
        else
        {
            g_buf[g_target_last].string[count++] = tmp_buf[i];
        }
    }
    //末尾自动换行
    g_buf[g_target_last].string[count] = '\0';
    count = 0;
    g_target_last++;
    if (g_target_last >= 60)
    {
        g_target_last = 0;
    }
func_return:

    pthread_mutex_unlock(&g_printf_mutex);

    return 0;
}