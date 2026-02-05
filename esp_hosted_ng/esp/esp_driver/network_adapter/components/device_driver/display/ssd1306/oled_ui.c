#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>

#include "esp_timer.h"

#include "oled.h"
#include "oled_ui.h"

QueueHandle_t ui_event_queue = NULL;
esp_timer_handle_t g_oled_ui_timer;

int g_ui_cfg_target_fps = 30;   //目标帧数
int g_ui_cfg_list_loop = 1;    //列表循环滚动

#define LINE 16
#define ROw  8


int char_roll(char *in, int *offset, int *dir, int *count)
{
    int len = strlen(in);
    if (len <= 16)
    {
        return len;
    }

    if ((len - (*offset)/8) <= 16)
    {
        *dir = -1;
    }

    (*count)++;
    if (*count >= g_ui_cfg_target_fps/16)
    {
        *offset += *dir;
        *count = 0;
    }

    if (*offset < 0)
    {
        *offset = 0;
        *dir = 1;
    }

    return 16;
}

int oled_ui(const oled_ui_menu_t *list, const int num)
{
    UI_EVENT event;
    int do_refresh = 0;
    int ret;
    char display[32];
    oled_char oled_display[32]; 
    int display_len = 0;
    int i;
    int ch;

    int target = 0;
    int ui_target = 0;
    int offset = 0;

    int c_offset = 0;
    int c_dir = 1;
    int c_count = 0;

    const int max_target = num - 1;
    const int max_ui_target = ((max_target >= 7)? 7: max_target);

    while (1)
    {
        if (target < 0)
        {
            if (g_ui_cfg_list_loop)
            {
                target = max_target;
                ui_target = max_ui_target;
            }
            else
                target = 0;

        }
        else if (target > max_target)
        {
            if (g_ui_cfg_list_loop)
            {
                target = 0;
                ui_target = 0;
            }
            else
                target = max_target;
        }

        if (ui_target < 0)
        {
            ui_target = 0;
        }
        else if (ui_target >= max_ui_target)
        {
            ui_target = max_ui_target;
        }

        offset = target - ui_target;
        // printf("UI: %d-%d-%d\n", target, ui_target, offset);

        oled_clean();
        for (i = 0; i <= 7; i++)
        {
            ch = 0;
            if (i == ui_target)
                ch = 1;
            
            if (i+offset >= num)
                break;
            
            snprintf(display, 32, "%s", list[i+offset].text);
            display_len = strlen(list[i+offset].text);
            if (list[i+offset].ex_text != NULL)
            {
                list[i+offset].ex_text(list[i+offset].argv, &display[display_len], sizeof(display)-display_len);
            }

            oled_char_to_oled_char(display, oled_display, strlen(display), ch);
            if (ch)
            {
                uint8_t *p;
                p  = (uint8_t*)oled_display;

                display_len = char_roll(display, &c_offset, &c_dir, &c_count);
                p += c_offset;


                oled_page_set(i, (oled_char *)p, strlen(display));
            }
            else
            {
                oled_page_set(i, oled_display, strlen(display));
            }
                

        }
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
                    switch (list[target].type)
                    {
                        case UI_ACTION_RETURN:
                            return target;
                            break;
                        case UI_ACTION_FUNC:
                            if (list[target].action != NULL)
                                list[target].action(list[target].argv, event);
                            break;
                        default:
                            break;
                    }
                    break;
                case UI_EVENT_RETURN:
                    return target;
                    break;
                case UI_EVENT_UP:
                    target++;
                    ui_target++;
                    c_offset = 0;
                    c_dir = 1;
                    break;
                case UI_EVENT_DOWN:
                    target--;
                    ui_target--;
                    c_offset = 0;
                    c_dir = 1;
                    break;
                case UI_EVENT_CTRL_DOWN:
                    if (list[target].single != NULL)
                        list[target].single(list[target].argv, event);
                    break;
                case UI_EVENT_CTRL_UP:
                    if (list[target].single != NULL)
                        list[target].single(list[target].argv, event);
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

}

void oled_ui_event(UI_EVENT event)
{
    xQueueSendFromISR(ui_event_queue, &event, NULL);
}

int oled_ui_event_get(TickType_t xTicksToWait)
{
    int event;
    xQueueReceive(ui_event_queue, &event, xTicksToWait);
    return event;
}

void oled_ui_refresh(void *argv)
{
    oled_ui_event(UI_EVENT_REFRESH);
    return ;
}

int oled_ui_set_fps(int fps)
{
    if (fps <= 0)
        g_ui_cfg_target_fps = 1;
    else
        g_ui_cfg_target_fps = fps;

    printf("oled fps set %d\n", g_ui_cfg_target_fps);
    esp_timer_restart(g_oled_ui_timer, 1000000/g_ui_cfg_target_fps);
    return 0;
}

int oled_ui_set_loop(int value)
{
    g_ui_cfg_list_loop = value;
    return 0;
}

int oled_ui_init()
{
    esp_timer_create_args_t oled_ui_timer = 
    {
        .name = "oled_ui",
        .callback = oled_ui_refresh,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
    };

    ui_event_queue = xQueueCreate(30, sizeof(uint32_t));

    esp_timer_create(&oled_ui_timer, &g_oled_ui_timer);

    esp_timer_start_periodic(g_oled_ui_timer, 1000000/g_ui_cfg_target_fps);

    return 0;
}

int ui_oled_cfg_get(void *argv, char *buf, int len)
{
    UI_CFG cfg = (UI_CFG) argv;

    switch (cfg)
    {
        case UI_CFG_FPS:
            snprintf(buf, len, "%d-%d", g_ui_cfg_target_fps, oled_fps_get());
            break;
        case UI_CFG_FPS_S:
            if (oled_cfg_show_fps_get())
                snprintf(buf, len, "ON");
            else
                snprintf(buf, len, "OFF");
            break;
        case UI_CFG_LOOP:
            if (g_ui_cfg_list_loop)
                snprintf(buf, len, "NO");
            else
                snprintf(buf, len, "OFF");
            break;
        case UI_CFG_FLIP:
            if (oled_cfg_flip_get())
                snprintf(buf, len, "180");
            else
                snprintf(buf, len, "0");
        default:

            break;
    }

    return 0;
}

int ui_oled_cfg_set(void *argv, UI_EVENT event)
{
    UI_CFG cfg = (UI_CFG) argv;

    switch (cfg)
    {
        case UI_CFG_FPS:
            switch (event)
            {
                case UI_EVENT_CTRL_DOWN:
                    oled_ui_set_fps(g_ui_cfg_target_fps - 1);
                    break;
                case UI_EVENT_CTRL_UP:
                    oled_ui_set_fps(g_ui_cfg_target_fps + 1);
                default:
                    break;
            }
            break;
        case UI_CFG_LOOP:
            oled_ui_set_loop(!g_ui_cfg_list_loop);
            break;
        case UI_CFG_FLIP:
            oled_cfg_flip_set(!oled_cfg_flip_get());
            break;
        case UI_CFG_FPS_S:
            oled_cfg_show_fps_set(!oled_cfg_show_fps_get());
            break;
        default:

            break;
    }

    return 0;
}

int ui_oled_set(void *argv, UI_EVENT event)
{
    oled_ui_menu_t menu[] = 
    {
        {"<<<", UI_ACTION_RETURN},
        {"loop ", UI_ACTION_FUNC, ui_oled_cfg_set,  ui_oled_cfg_get, ui_oled_cfg_set, (void *)UI_CFG_LOOP},
        {"fps  ", UI_ACTION_NULL, NULL,             ui_oled_cfg_get, ui_oled_cfg_set, (void *)UI_CFG_FPS},
        {"fps  ", UI_ACTION_FUNC, ui_oled_cfg_set,  ui_oled_cfg_get, ui_oled_cfg_set, (void *)UI_CFG_FPS_S},
        {"flip ", UI_ACTION_FUNC, ui_oled_cfg_set,  ui_oled_cfg_get, ui_oled_cfg_set, (void *)UI_CFG_FLIP},
    }; 

    oled_ui(menu, sizeof(menu)/sizeof(menu[0]));

    return 0;
}