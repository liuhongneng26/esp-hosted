#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "driver/gpio_filter.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "hal/i2c_types.h"
#include "oled_ui.h"
#include "sdkconfig.h"
#include "driver/temperature_sensor.h"
#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"

#include <oled.h>
#include "peripherals.h"
#include "oled_animation.h"
#include "font8x8_basic.h"

PAGE_t frame_a[8];
PAGE_t frame_b[8];

PAGE_t *_page = frame_a;
PAGE_t *last_page = frame_b;

esp_lcd_panel_io_handle_t io_handle = NULL;
esp_lcd_panel_handle_t panel_handle = NULL;
int flip = 0;

time_t fps_last = 0, fps_now = 0;
int g_oled_fps = 0;      //fps计数器
int g_oled_fps_count = 0;
int g_oled_cfg_fps_show = 1;

void ssd1306_invert(uint8_t *buf, size_t blen)
{
	uint8_t wk;
	for(int i=0; i<blen; i++){
		wk = buf[i];
		buf[i] = ~wk;
	}
}

uint8_t ssd1306_rotate_byte(uint8_t ch1)
{
	uint8_t ch2 = 0;
	for (int j=0;j<8;j++) {
		ch2 = (ch2 << 1) + (ch1 & 0x01);
		ch1 = ch1 >> 1;
	}
	return ch2;
}

void ssd1306_flip(uint8_t *buf, size_t blen)
{
	for(int i=0; i<blen; i++){
		buf[i] = ssd1306_rotate_byte(buf[i]);
	}
}

void oled_page_text(int page, char * text, int text_len, int invert)
{
	if (page >= 8)
		return;
	int _text_len = text_len;
	if (_text_len > 16) _text_len = 16;

	uint8_t seg = 0;
	uint8_t image[8];
	for (uint8_t i = 0; i < _text_len; i++)
    {
		memcpy(image, font8x8_basic_tr[(uint8_t)text[i]], 8);

		if (invert) ssd1306_invert(image, 8);
		if (flip) ssd1306_flip(image, 8);

        memcpy(&_page[page]._segs[seg], image, 8);
		seg = seg + 8;
	}
}

void oled_clean()
{
	PAGE_t *tmp;
	tmp = last_page;
	last_page = _page;
	_page = tmp;
	memset(_page, 0, 8 * 128);
}

void oled_show_fps()
{
	char fps_c[3];
	oled_char fps_o[2];
	uint8_t fps_num = g_oled_fps;
	if (fps_num > 99)
		fps_num = 99;

	snprintf(fps_c, 3, "%d", fps_num);

	oled_char_to_oled_char(fps_c, fps_o, 2, 1);

	oled_draw_bitmap((128 - 2*8), 0, (uint8_t *)fps_o, 24, 8);
	return;
} 

void oled_refresh()
{

	oled_animation_update(_page, last_page);

	if(g_oled_cfg_fps_show)
		oled_show_fps();

	i2c_master_mutex(1);
	esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, 128, 64, _page);
	i2c_master_mutex(0);

    fps_now = time(NULL);
    if (fps_now != fps_last)
    {
        fps_last = fps_now;
        g_oled_fps = g_oled_fps_count;
        g_oled_fps_count = 1;
    }
    else
    {
        g_oled_fps_count++;
    }

	return;
}

int oled_fps_get()
{
	return g_oled_fps;
}

int oled_cfg_show_fps_get()
{
	return g_oled_cfg_fps_show;
}

int oled_cfg_show_fps_set(int value)
{
	if (value)
		g_oled_cfg_fps_show = 1;
	else
		g_oled_cfg_fps_show = 0;

	return g_oled_cfg_fps_show;
}

void oled_page_set(int page, oled_char * o_c, int size)
{
	if (size > 16) size = 16;
	memcpy(&_page[page], o_c, 8 * size);	
}

void oled_char_to_oled_char(char *c, oled_char * o_c, int size, int invert)
{
	int i;
	for(i = 0; i < size; i++)
	{
		memcpy(o_c[i].c, font8x8_basic_tr[(uint8_t)c[i]], 8);
		if (invert) ssd1306_invert(o_c[i].c, 8);
	}

	return;
}


void oled_draw_bitmap_negative(const int x, const int y, const uint8_t *data, const int data_w, const int data_h)
{
	// y<0 向下对齐
	// int page_up = 0;
	int page_down = ((y + data_h) > 0)? ((y + data_h) / 8): 0;
	int page_down_micro =((y + data_h) > 0)? ((y + data_h) % 8): 0;

	//bitmap所跨越的页面数量
	const int page_num = (page_down_micro? page_down + 1: page_down);

	const int y_offset = abs(y % 8);
	const int x_offset = (x >= 0)? x: 0;

	int	useful_data_h = data_h - abs(y / 8) * 8;
	if (useful_data_h <= 0)//整个bitmap都在页面之外
		return;

	uint8_t *p_useful_data = (uint8_t *)data + abs(y / 8) * data_w;
	uint8_t *cur_of_data = NULL;
	uint8_t *next_of_data = NULL;

	int data_len = 0;
	int i, j;
	for (i = 0; i < page_num; i++)
	{
		if (i >= 8)
			break;
		
		cur_of_data = p_useful_data + i * data_w;
		next_of_data = ((useful_data_h - 8 * (i + 1)) >= 8)? cur_of_data + data_w: NULL;

		data_len = x_offset - x;
		if (data_len >= data_w)
			continue;

		if (next_of_data == NULL)
		{
			//最后一页
			for (j = x_offset; j < 128; j++)
			{
				uint8_t save_bit = 0xFF <<  (8 - y_offset);

				_page[i]._segs[j] = (_page[i]._segs[j] & save_bit) | \
											(cur_of_data[data_len] >> y_offset);
				data_len++;
				if (data_len >= data_w)
					break;
			}
		}
		else
		{
			for (j = x_offset; j < 128; j++)
			{
				_page[i]._segs[j] = (next_of_data[data_len] << (8 - y_offset)) | \
											(cur_of_data[data_len] >> y_offset);
				data_len++;
				if (data_len >= data_w)
					break;
			}
		}
	}

	return;
}

//data_h需要是8的倍数
//覆盖或融合
//多行数
void oled_draw_bitmap(const int x, const int y, const uint8_t *data, const int data_w, const int data_h)
{

	if (y < 0)
		return oled_draw_bitmap_negative(x, y, data, data_w, data_h);

	// y>=0 向上对齐
	int page_up = y / 8;
	int page_down = (y + data_h) / 8;
	int page_down_micro = (y + data_h) % 8;

	//bitmap所跨越的页面数量
	const int page_num = (page_down_micro? page_down + 1: page_down) - page_up;

	const int y_offset = y % 8;
	const int x_offset = (x >= 0)? x: 0;

	uint8_t *p_data = (uint8_t *)data;
	uint8_t *last_of_data = NULL;
	uint8_t *cur_of_data = NULL;

	int data_len = 0;
	int i, j;
	for (i = 0; i < page_num; i++)
	{
		if ((page_up + i) >= 8)
			break;

		last_of_data = cur_of_data;
		cur_of_data	= ((data_h - 8 * (i + 1)) >= 0)?p_data + data_w * i: NULL;

		data_len = x_offset - x;
		if (data_len >= data_w)
			continue;

		//第一页
		if (last_of_data == NULL)
		{
			uint8_t save_bit = ~(0xFF << y_offset);
			for (j = x_offset; j < 128; j++)
			{
				_page[page_up+i]._segs[j] = (_page[page_up+i]._segs[j] & save_bit) | \
											(cur_of_data[data_len] << y_offset);
				data_len++;
				if (data_len >= data_w)
					break;
			}
		}
		else if (cur_of_data == NULL)//最后一页
		{
			uint8_t save_bit = (0xFF << y_offset);
			for (j = x_offset; j < 128; j++)
			{
				_page[page_up+i]._segs[j] = (_page[page_up+i]._segs[j] & save_bit) | \
											(last_of_data[data_len] >> (8 - y_offset));
				data_len++;
				if (data_len >= data_w)
					break;
			}
		}
		else //中间页面
		{
			for (j = x_offset; j < 128; j++)
			{
				_page[page_up+i]._segs[j] = (last_of_data[data_len] >> (8 - y_offset)) | \
											(cur_of_data[data_len] << y_offset);
				data_len++;
				if (data_len >= data_w)
					break;
			}
		}

	}

}

int oled_cfg_flip = 0;

int oled_init()
{
    esp_lcd_panel_io_i2c_config_t io_config =
	{
        .dev_addr = 0x3c,
        .scl_speed_hz = 400 * 1000,
        .control_phase_bytes = 1,   // According to SSD1306 datasheet
        .lcd_cmd_bits = 8,   		// According to SSD1306 datasheet
        .lcd_param_bits = 8, 		// According to SSD1306 datasheet
        .dc_bit_offset = 6,         // According to SSD1306 datasheet
    };

	i2c_master_mutex(1);
	i2c_master_add_lcd(I2C_NUM_0, &io_config, &io_handle);

    esp_lcd_panel_dev_config_t panel_config =
	{
        .bits_per_pixel = 1,
        .reset_gpio_num = -1,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

	//esp_lcd_panel_swap_xy(panel_handle, true);
	esp_lcd_panel_mirror(panel_handle, true, true);
	oled_cfg_flip = 0;

	i2c_master_mutex(0);

    oled_ui_init();

    return 0;
}

int oled_cfg_flip_get()
{
	return oled_cfg_flip;
}

int oled_cfg_flip_set(int value)
{
	i2c_master_mutex(1);
	if(value == 1)
	{
		oled_cfg_flip = 1;
		esp_lcd_panel_mirror(panel_handle, false, false);
	}
	else
	{
		oled_cfg_flip = 0;
		esp_lcd_panel_mirror(panel_handle, true, true);
	}
	i2c_master_mutex(0);
	return oled_cfg_flip;
}

int oled_power(int value)
{
	if (value)
	{
		esp_lcd_panel_disp_on_off(panel_handle, true);
	}
	else
	{
		esp_lcd_panel_disp_on_off(panel_handle, false);
	}

	return 0;
}