#ifndef __OLED_ANIMATION_H__
#define __OLED_ANIMATION_H__

#include "oled.h"

enum
{
    ANI_NULL,
    ANI_WIN_SHADES,
    ANI_CRT,
};


int oled_anim_win_shades_start(int dir);
int oled_anim_crt_start(int dir);

int oled_animation_update(PAGE_t *new, PAGE_t *last);

#endif