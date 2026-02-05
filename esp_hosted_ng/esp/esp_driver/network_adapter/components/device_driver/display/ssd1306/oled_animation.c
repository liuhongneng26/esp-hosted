#include "oled_animation.h"
#include "oled.h"
#include "oled_ui.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>


int ani_status = ANI_NULL;
int ani_count;

//win_shaders
int cur = 1; // 1-8
static int dir = 1;

int oled_anim_win_shades_start(int val)
{
    dir = val;
	cur = 1;
	ani_count = 0;
	ani_status = ANI_WIN_SHADES;
	return 0;
}

int crt_height;		//高度位置
int crt_lineWidth;	//线位置
int line_or_hight;	//高度变化还是线变化

int crt_dir;	//方向 关闭或开启

int oled_anim_crt_start(int dir)
{
	//开启
	if (dir)
	{
		crt_height = 32;
		crt_lineWidth = 0;
		line_or_hight = 0; //先高度变化，再线变化
		crt_dir = 1;	//开启方向
	}
	else//关闭
	{
		crt_height = 1;
		crt_lineWidth = 128;
		line_or_hight = 1; //先高度变化，再线变化
		crt_dir = 0;	//关闭方向
	}

	ani_status = ANI_CRT;
	return 0;
}

void anim_crt(PAGE_t *new, PAGE_t *last)
{
	int i;
	if (line_or_hight)
	{
		//高度变化
		if (crt_dir)
			crt_height -= 8;
		else
			crt_height += 4;

		if(crt_height >= 32 && crt_dir == 0)
		{
			crt_height = 32;
			line_or_hight = 0;	//进入线宽变化
		}

		if (crt_height <= 1 && crt_dir == 1)
		{
			crt_height = 1;
			ani_status = ANI_NULL;
		}
	
	}
	else
	{
		//线宽变化
		if(crt_dir)
			crt_lineWidth += 30;
		else
			crt_lineWidth -= 20;

		if(crt_lineWidth >= 128 && crt_dir == 1)
		{
			crt_lineWidth = 128;
			line_or_hight = 1;	//进入高度变化
		}

		if (crt_lineWidth <= 0 && crt_dir == 0)
		{
			crt_lineWidth = 0;
			ani_status = ANI_NULL;
			oled_ui_event(UI_EVENT_ANIM_FINISH);
		}

	}


	uint8_t page = crt_height / 8;
	for (i = 0; i < page; i++)
	{
		memset(&new[i], 0, 128);
		memset(&new[7 - i], 0, 128);
	}

	uint8_t prows = crt_height % 8;
	if (prows)
	{
		uint8_t a = (0xFF << prows);
		uint8_t b = (0xFF >> prows);
		uint8_t c = (0x01 << (prows - 1));
		uint8_t d = (0x80 >> (prows - 1));

		for (i = 0; i < 128; i++)
		{
			new[page]._segs[i] = (new[page]._segs[i] & a) | c;
			new[7 - page]._segs[i] = (new[7 - page]._segs[i] & b) | d;
		}
	}
	else
	{
		for (i = (64 - crt_lineWidth/2); i < (64 + crt_lineWidth/2); i++)
		{
			new[page - 1]._segs[i] = 0x80;
			new[8 - page]._segs[i] = 0x1;
		}
	}

	return;
}

void anim_win_shades(PAGE_t *new, PAGE_t *last)
{
	int page = 0, i;
	for (page = 0; page < 8; page++)
	{
		for (i = 0; i < 128; i++)
		{
			if(new[page]._segs[i] == 0 && last[page]._segs[i] == 0)
				continue;
			// << 内容向下偏移
			// >> 内容向上偏移
			new[page]._segs[i] = (dir? (last[page]._segs[i] << 1): (last[page]._segs[i] >> 1)) | \
               (dir? (new[page]._segs[i] >> (8 - cur)): (new[page]._segs[i] << (8 - cur)));
		}
	}

	ani_count++;
	if (ani_count > 0)
	{
		cur++;
		ani_count = 0;
	}

	if (cur >=8)
		ani_status = 0;
	
	return ;
}

int oled_animation_update(PAGE_t *new, PAGE_t *last)
{
	if (ani_status == 0)
	{
		return 0;
	}

	switch (ani_status)
	{
		case ANI_WIN_SHADES:
			anim_win_shades(new, last);
			break;

		case ANI_CRT:
			anim_crt(new, last);
			break;

		default:
			ani_status = ANI_NULL;
	}

    return 0;
}