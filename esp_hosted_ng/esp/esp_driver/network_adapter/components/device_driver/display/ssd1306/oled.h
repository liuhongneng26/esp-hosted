#ifndef __OLED_H__
#define __OLED_H__

#include <stdint.h>

typedef  struct
{
    uint8_t c[8];
}oled_char;

typedef struct
{
    union
    {
	    oled_char char_16[16];
        uint8_t _segs[128]; //一页 128字节
    };
} PAGE_t;

int oled_init();

void oled_clean();
void oled_refresh();

void oled_page_text(int page, char * text, int text_len, int invert);

void oled_page_set(int page, oled_char * o_c, int size);

void oled_char_to_oled_char(char *c, oled_char * o_c, int size, int invert);

void oled_draw_bitmap(const int x, const int y, const uint8_t *data, const int data_w, const int data_h);

int oled_cfg_flip_get();
int oled_cfg_flip_set(int value);

int oled_fps_get();
int oled_cfg_show_fps_get();
int oled_cfg_show_fps_set(int value);

int oled_power(int value);
#endif