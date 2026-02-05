#ifndef __OLED_UI_H__
#define __OLED_UI_H__

#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "peripherals.h"

typedef  enum
{
    UI_EVENT_DOWN = -1,     //目标向下滚动
    UI_EVENT_REFRESH = 0,   //仅刷新
    UI_EVENT_UP = 1,        //目标向上滚动
    UI_EVENT_ACTION,        //执行目标
    UI_EVENT_RETURN,        //返回目标
    UI_EVENT_CTRL_DOWN,     //功能滚动向下
    UI_EVENT_CTRL_UP,       //功能滚动向上
    UI_EVENT_ANIM_FINISH,   //动画结束
}UI_EVENT;


typedef enum 
{
    UI_ACTION_NULL = 0, //无动作
    UI_ACTION_RETURN,   //返回目标
    UI_ACTION_FUNC,     //执行函数
}UI_ACTION;

typedef int (*ACTION_F)(void *argv, UI_EVENT event);
typedef int (*EXTRA_TEXT)(void *argv, char *buf, int len);
typedef int (*SINGLE_EVENT)(void *argv, UI_EVENT event);

typedef struct oled_ui_menu
{
    char        *text;
    UI_ACTION   type;
    ACTION_F    action;
    EXTRA_TEXT  ex_text;
    SINGLE_EVENT single;
    void        *argv;
}oled_ui_menu_t;

int oled_ui_init();

int oled_ui(const oled_ui_menu_t *list, const int num);
int oled_ui_terminal(void *argv, UI_EVENT event);


void oled_ui_event(UI_EVENT event);
int oled_ui_event_get(TickType_t xTicksToWait);

int oled_ui_terminal_printf(const char* fmt, ...);

int oled_ui_set_fps(int fps);
int oled_ui_set_loop(int value);

typedef enum 
{
    UI_CFG_FPS = 0, //fps
    UI_CFG_LOOP,    //列表循环 
    UI_CFG_FLIP,    //180度画面旋转
    UI_CFG_FPS_S,   //全局显示fps
}UI_CFG;

int ui_oled_set(void *argv, UI_EVENT event);
int ui_sys_main(void *argv, UI_EVENT event);
int ui_map_main(void *argv, UI_EVENT event);

#endif