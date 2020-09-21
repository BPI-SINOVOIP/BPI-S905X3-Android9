#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief 字体测试程序
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-10-22: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_font.h>
#include <string.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define OSD_DEV_NO 0
#define OSD_WIDTH  1280
#define OSD_HEIGHT 720

/****************************************************************************
 * Functions
 ***************************************************************************/

int main(int argc, char **argv)
{
	AM_OSD_OpenPara_t para;
	AM_OSD_Surface_t *s;
	AM_FONT_Attr_t attr;
	AM_FONT_TextInfo_t info;
	AM_FONT_TextPos_t pos;
	AM_OSD_Color_t col;
	AM_OSD_Rect_t r;
	uint32_t pixel;
	int fid;
	char *str = "0123456789abcdefg甲乙丙丁";
	int len = strlen(str);
	char *path;
	
	if(argc<2)
	{
		printf("usage: am_font_test FONT_PATH\n");
		return 1;
	}
	
	path = argv[1];
	
	memset(&para, 0, sizeof(para));
	
	para.format = AM_OSD_FMT_COLOR_8888;
	para.width  = OSD_WIDTH;
	para.height = OSD_HEIGHT;
	
	AM_OSD_Open(OSD_DEV_NO, &para);
	
	AM_OSD_GetSurface(OSD_DEV_NO, &s);
	
	AM_OSD_Update(OSD_DEV_NO, NULL);
	
	AM_FONT_Init();
	
	attr.name   = "default";
	attr.width  = 0;
	attr.height = 0;
	attr.encoding = AM_FONT_ENC_UTF16;
	attr.charset = AM_FONT_CHARSET_UNICODE;
	attr.flags  = 0;
	
	if(AM_FONT_LoadFile(path, "freetype", &attr)!=AM_SUCCESS)
		goto end;
	
	attr.width  = 64;
	attr.height = 64;
	attr.encoding = AM_FONT_ENC_UTF8;
	if(AM_FONT_Create(&attr, &fid)!=AM_SUCCESS)
		goto end;
	
	if(AM_FONT_Size(fid, str, len, &info)!=AM_SUCCESS)
		goto end;
	
	printf("text:\"%s\"\n", str);
	printf("width:%d height:%d ascent:%d descent:%d\n", info.width, info.height, info.ascent, info.descent);
	
	col.r = 0;
	col.g = 0;
	col.b = 0;
	col.a = 0;
	AM_OSD_MapColor(s->format, &col, &pixel);
	r.x = 0;
	r.y = 0;
	r.w = s->width;
	r.h = s->height;
	AM_OSD_DrawFilledRect(s, &r, pixel);
	
	col.r = 255;
	col.g = 255;
	col.b = 255;
	col.a = 255;
	AM_OSD_MapColor(s->format, &col, &pixel);
	
	pos.rect.x = 50;
	pos.rect.w = pos.rect.x+info.width;
	pos.rect.y = 100;
	pos.rect.h = pos.rect.y+info.height;
	pos.base   = info.ascent;
	AM_FONT_Draw(fid, s, str, len, &pos, pixel);
	AM_FONT_Destroy(fid);
	
	attr.width  = 24;
	attr.height = 24;
	AM_FONT_Create(&attr, &fid);
	pos.rect.y += 60;
	AM_FONT_Draw(fid, s, str, len, &pos, pixel);
	AM_FONT_Destroy(fid);
	
	attr.width  = 12;
	attr.height = 12;
	AM_FONT_Create(&attr, &fid);
	pos.rect.y += 30;
	AM_FONT_Draw(fid, s, str, len, &pos, pixel);
	AM_FONT_Destroy(fid);
	
	attr.width  = 40;
	attr.height = 24;
	AM_FONT_Create(&attr, &fid);
	pos.rect.y += 30;
	AM_FONT_Draw(fid, s, str, len, &pos, pixel);
	AM_FONT_Destroy(fid);
	
	attr.width  = 24;
	attr.height = 40;
	AM_FONT_Create(&attr, &fid);
	pos.rect.y += 40;
	AM_FONT_Draw(fid, s, str, len, &pos, pixel);
	AM_FONT_Destroy(fid);
	
	AM_OSD_Update(OSD_DEV_NO, NULL);
	
	
	getchar();
end:
	AM_FONT_Quit();
	AM_OSD_Close(OSD_DEV_NO);
	
	return 0;
}

