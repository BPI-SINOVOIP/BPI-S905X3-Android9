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
 * \brief 字体管理模块
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-10-20: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_mem.h>
#include <am_thread.h>
#include "am_font_internal.h"
#include "../../am_adp/am_osd/am_osd_internal.h"
#include <assert.h>
#include <am_iconv.h>
#include <errno.h>


/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/****************************************************************************
 * Static data
 ***************************************************************************/

static pthread_mutex_t lock;
static AM_FONT_File_t *font_files;
static AM_FONT_t      *fonts;
static iconv_t         gbk2unicode;
static iconv_t         unicode2gbk;
static AM_FONT_t      *def_font;

#ifdef FONT_FREETYPE
extern const AM_FONT_Driver_t ft_font_drv;
#endif

static const AM_FONT_Driver_t* font_drivers[] =
{
#ifdef FONT_FREETYPE
	&ft_font_drv,
#endif
};

#define FONT_DRV_COUNT    AM_ARRAY_SIZE(font_drivers)

/****************************************************************************
 * Static functions
 ***************************************************************************/

/**\brief 返回一个字符集的ID*/
static int charset_id(AM_FONT_Charset_t set)
{
	return (set==AM_FONT_CHARSET_ASCII)?0:1;
}

/**\brief 从字符串中取一个字符*/
static const char* next_char(const char *str, int size, AM_FONT_Encoding_t enc, AM_FONT_Charset_t *set)
{
	AM_FONT_Charset_t s;
	const char *nc;
	uint32_t c;
	
	switch(enc)
	{
		case AM_FONT_ENC_GBK:
			if(!(str[0]&0x80))
			{
				nc = str+1;
				s = AM_FONT_CHARSET_ASCII;
			}
			else if(size<2)
			{
				return NULL;
			}
			else
			{
				nc = str+2;
				s = AM_FONT_CHARSET_GBK;
			}
		break;
		case AM_FONT_ENC_UTF8:
			if(!(str[0]&0x80))
			{
				nc = str+1;
				s = AM_FONT_CHARSET_ASCII;
			}
			else if((size>1) && ((str[0]&0xE0)==0xC0))
			{
				nc = str+2;
				s = AM_FONT_CHARSET_GBK;
			}
			else if((size>2) && ((str[0]&0xF0)==0xE0))
			{
				nc = str+3;
				s = AM_FONT_CHARSET_GBK;
			}
			else if((size>3) && ((str[0]&0xF8)==0xF0))
			{
				nc = str+4;
				s = AM_FONT_CHARSET_GBK;
			}
			else
				return NULL;
		break;
		case AM_FONT_ENC_UTF16:
			if(size<2) return NULL;
			nc = str+2;
			c = (str[0]<<8)|str[1];
			s = (c<128)?AM_FONT_CHARSET_ASCII:AM_FONT_CHARSET_GBK;
		break;
		default:
			return NULL;
	}
	
	*set = s;
	return nc;
}

/**\brief 转换一个字符的编码*/
static AM_ErrorCode_t conv_char(const char *from, int fs, AM_FONT_Encoding_t fe, char *to, int *ts, AM_FONT_Encoding_t te)
{
	uint32_t c = 0;
	
	switch(fe)
	{
		case AM_FONT_ENC_GBK:
			if(!(from[0]&0x80))
				c = from[0];
			else if(-1!=(int)gbk2unicode)
			{
				char *fbuf = (char*)from;
				char *buf = (char*)&c;
				size_t fsize = fs;
				size_t size = sizeof(c);
				if(iconv(gbk2unicode, &fbuf, &fsize, &buf, &size)==-1)
					return AM_FONT_ERR_ILLEGAL_CODE;
			}
			else
				return AM_FONT_ERR_ILLEGAL_CODE;
		break;
		case AM_FONT_ENC_UTF8:
			if(!(from[0]&0x80))
				c = from[0];
			else if((from[0]&0xE0)==0xC0)
				c = ((from[0]&0x1F)<<6)|(from[1]&0x3F);
			else if((from[0]&0xF0)==0xE0)
				c = ((from[0]&0x0F)<<12)|((from[1]&0x3F)<<6)|(from[2]&0x3F);
			else if((from[0]&0xF8)==0xF0)
				c = ((from[0]&0x07)<<18)|((from[1]&0x3F)<<12)|((from[2]&0x3F)<<6)|(from[3]&0x3F);
			else
				c = 0x20;
		break;
		case AM_FONT_ENC_UTF16:
			c = (from[0]<<8)|from[1];
		break;
	}
	
	switch(te)
	{
		case AM_FONT_ENC_GBK:
			if(!(c&0x80))
			{
				to[0] = c;
				*ts = 1;
			}
			else if(-1!=(int)unicode2gbk)
			{
				char *buf = (char*)&c;
				size_t size = sizeof(c);
				size_t osize = *ts;
				
				if(iconv(unicode2gbk, &buf, &size, &to, &osize)==-1)
					return AM_FONT_ERR_ILLEGAL_CODE;
				
				*ts = 2;
			}
			else
				return AM_FONT_ERR_NOT_SUPPORTED;
		break;
		case AM_FONT_ENC_UTF8:
			if(c>0xFFFF)
			{
				to[0] = (c>>18)|0xF0;
				to[1] = ((c>>12)&0x3F)|0x80;
				to[2] = ((c>>6)&0x3F)|0x80;
				to[3] = (c&0x3F)|0x80;
				*ts = 4;
			}
			else if(c>0x7FF)
			{
				to[0] = (c>>12)|0xE0;
				to[1] = ((c>>6)&0x3F)|0x80;
				to[2] = (c&0x3F)|0x80;
				*ts = 3;
			}
			else if(c>0x7F)
			{
				to[0] = (c>>6)|0xC0;
				to[1] = (c&0x3F)|0x80;
				*ts = 2;
			}
			else
			{
				to[0] = c;
				*ts = 1;
			}
		break;
		case AM_FONT_ENC_UTF16:
			to[0] = c >> 8;
			to[1] = c;
			*ts = 2;
		break;
	}
	
	return AM_SUCCESS;
}

/**\brief 绘制字符*/
static AM_ErrorCode_t draw_glyph(AM_OSD_Surface_t *s, AM_FONT_Glyph_t *glyph, int x, int y, uint32_t pixel)
{
	int minx, miny, maxx, maxy;
	int sx, sy, sw, sh;
	
	minx = AM_MAX(s->clip.x, x);
	miny = AM_MAX(s->clip.y, y);
	maxx = AM_MIN(s->clip.x+s->clip.w, x+glyph->width);
	maxy = AM_MIN(s->clip.y+s->clip.h, y+glyph->height);
	
	if((minx>=maxx) || (miny>=maxy))
		return AM_SUCCESS;
	
	sx = minx-x;
	sy = miny-y;
	sw = maxx-minx;
	sh = maxy-miny;
	
	if(glyph->type==AM_FONT_BITMAP_1BPP)
	{
		uint8_t *sline = (uint8_t*)glyph->buffer+sy*glyph->pitch+(sx>>3);
		uint8_t *dline = (uint8_t*)s->buffer+miny*s->pitch+minx*s->format->bytes_per_pixel;
		uint8_t *src, *dst;
		int h = sh;
		int w;
		
		while(h)
		{
			int bit = sx&7;
			
			src = sline;
			dst = dline;
			w = sw;
			while(w)
			{
				if(src[0]&(1<<(7-bit)))
				{
					switch(s->format->bytes_per_pixel)
					{
						case 1:
							*dst = pixel;
						break;
						case 2:
							*(uint16_t*)dst = pixel;
						break;
						case 3:
							AM_PUT_3BPP_PIXEL(dst, pixel);
						break;
						case 4:
							*(uint32_t*)dst = pixel;
						break;
					}
				}
				bit++;
				if(bit==8)
				{
					src++;
					bit=0;
				}
				dst += s->format->bytes_per_pixel;
				w--;
			}
			sline += glyph->pitch;
			dline += s->pitch;
			h--;
		}
	}
	else
	{
		uint8_t *sline = (uint8_t*)glyph->buffer+sy*glyph->pitch+sx;
		uint8_t *dline = (uint8_t*)s->buffer+miny*s->pitch+minx*s->format->bytes_per_pixel;
		uint8_t *src, *dst;
		int h = sh;
		int w;
		AM_OSD_Color_t col;
		
		if(!AM_OSD_PIXEL_FORMAT_TYPE_IS_PALETTE(s->format->type))
		{
			AM_RGB_PIXEL_TO_COLOR(s->format, pixel, &col);
		}
		
		while(h)
		{
			src = sline;
			dst = dline;
			w = sw;
			while(w)
			{
				if(src[0])
				{
					uint32_t dp;
					
					if(AM_OSD_PIXEL_FORMAT_TYPE_IS_PALETTE(s->format->type))
					{
						dp = pixel;
					}
					else if(src[0]==0xFF)
					{
						dp = pixel;
					}
					else
					{
						AM_OSD_Color_t dc;
						
						switch(s->format->bytes_per_pixel)
						{
							case 1:
								dp = *dst;
							break;
							case 2:
								dp = *(uint16_t*)dst;
							break;
							case 3:
								dp = AM_GET_3BPP_PIXEL(dst);
							break;
							case 4:
								dp = *(uint32_t*)dst;
							break;
						}
						
						AM_RGB_PIXEL_TO_COLOR(s->format, dp, &dc);
						
						dc.r = (col.r*src[0]+dc.r*(255-src[0]))>>8;
						dc.g = (col.g*src[0]+dc.g*(255-src[0]))>>8;
						dc.b = (col.b*src[0]+dc.b*(255-src[0]))>>8;
						dc.a = AM_MAX(src[0], dc.a);
						
						dp = AM_COLOR_TO_RGB_PIXEL(s->format, &dc);
					}
					
					switch(s->format->bytes_per_pixel)
					{
						case 1:
							*dst = dp;
						break;
						case 2:
							*(uint16_t*)dst = dp;
						break;
						case 3:
							AM_PUT_3BPP_PIXEL(dst, dp);
						break;
						case 4:
							*(uint32_t*)dst = dp;
						break;
					}
				}
				
				src++;
				dst += s->format->bytes_per_pixel;
				w--;
			}
			sline += glyph->pitch;
			dline += s->pitch;
			h--;
		}
	}
	
	return AM_SUCCESS;
}

/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief 初始化字体管理模块
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_font.h)
 */
AM_ErrorCode_t AM_FONT_Init(void)
{
	const AM_FONT_Driver_t *drv;
	AM_ErrorCode_t ret = AM_SUCCESS;
	int i;
	
	for(i=0; i<FONT_DRV_COUNT; i++)
	{
		drv = font_drivers[i];
		if(drv && drv->init)
		{
			ret = drv->init();
			if(ret!=AM_SUCCESS)
			{
				AM_DEBUG(1, "init font driver %d failed", i);
			}
		}
	}
	
	gbk2unicode = iconv_open("utf-8", "GBK");
	if(-1==(int)gbk2unicode)
		AM_DEBUG(1, "cannot create gbk->unicode converter:%s", strerror(errno));
	
	unicode2gbk = iconv_open("GBK", "utf-8");
	if(-1==(int)unicode2gbk)
		AM_DEBUG(1, "cannot create unicode->gbk converter:%s", strerror(errno));
	
	pthread_mutex_init(&lock, NULL);
	
	return AM_SUCCESS;
}

/**\brief 加载一个字库文件
 * \param[in] path 字体文件名
 * \param[in] drv_name 字体驱动名
 * \param[in] attr 字体文件属性
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_font.h)
 */
AM_ErrorCode_t AM_FONT_LoadFile(const char *path, const char *drv_name, const AM_FONT_Attr_t *attr)
{
	const AM_FONT_Driver_t *drv = NULL;
	AM_FONT_File_t *file;
	AM_ErrorCode_t ret = AM_SUCCESS;
	int i;
	
	assert(path && drv_name && attr);
	
	pthread_mutex_lock(&lock);
	
	for(i=0; i<FONT_DRV_COUNT; i++)
	{
		if(!strcasecmp(font_drivers[i]->name, drv_name))
		{
			drv = font_drivers[i];
			break;
		}
	}
	
	if(drv)
	{
		file = (AM_FONT_File_t*)malloc(sizeof(AM_FONT_File_t));
		if(!file)
		{
			AM_DEBUG(1, "not enough memory");
			ret = AM_FONT_ERR_NO_MEM;
		}
		
		memset(file, 0, sizeof(AM_FONT_File_t));
		file->attr = *attr;
		
		if(ret==AM_SUCCESS)
		{
			if(drv->open_file)
			{
				ret = drv->open_file(path, file);
				if(ret!=AM_SUCCESS)
					free(file);
			}
		}
		
		if(ret==AM_SUCCESS)
		{
			file->attr.name = attr->name?strdup(attr->name):NULL;
			file->drv  = drv;
			file->next = font_files;
			font_files = file;
		}
	}
	else
	{
		AM_DEBUG(1, "not supported font driver \"%s\"", drv_name);
		ret = AM_FONT_ERR_NOT_SUPPORTED;
	}
	
	pthread_mutex_unlock(&lock);
	
	return ret;
}

/**\brief 设定一个字体为缺省字体
 * \param font_id 缺省字体句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_font.h)
 */
AM_ErrorCode_t AM_FONT_SetDefault(int font_id)
{
	AM_FONT_t *f = (AM_FONT_t*)font_id;
	
	assert(f);
	
	pthread_mutex_lock(&lock);
	
	def_font = f;
	
	pthread_mutex_unlock(&lock);
	
	return AM_SUCCESS;
}

/**\brief 取得当前的缺省字体
 * \param[out] font_id 缺省字体句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_font.h)
 */
AM_ErrorCode_t AM_FONT_GetDefault(int *font_id)
{
	assert(font_id);
	
	pthread_mutex_lock(&lock);
	
	*font_id = (int)def_font;
	
	pthread_mutex_unlock(&lock);
	
	return AM_SUCCESS;
}

/**\brief 创建一个字体
 * \param[in] attr 要创建的字体属性
 * \param[out] font_id 返回创建的字体句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_font.h)
 */
AM_ErrorCode_t AM_FONT_Create(const AM_FONT_Attr_t *attr, int *font_id)
{
	AM_FONT_File_t *file;
	AM_FONT_t *f = NULL;
	AM_ErrorCode_t ret = AM_SUCCESS;
	int match[FONT_FILE_REF_CNT];
	int score, ref;
	
	assert(attr && font_id);
	
	f = (AM_FONT_t*)malloc(sizeof(AM_FONT_t));
	if(!f)
	{
		AM_DEBUG(1, "not enough memory");
		return AM_FONT_ERR_NO_MEM;
	}
	
	memset(f, 0, sizeof(AM_FONT_t));
	f->attr = *attr;
	
	if(!f->attr.width)
		f->attr.width = f->attr.height;
	else if(!f->attr.height)
		f->attr.height = f->attr.width;
	
	pthread_mutex_lock(&lock);
	
	/*Get the matched font files*/
	for(file=font_files; file; file=file->next)
	{
		score = 0;
		if(!(attr->charset&file->attr.charset))
			continue;
		
		if(attr->name)
		{

			if(file->attr.name && !strcasecmp(attr->name, file->attr.name))
				score += 50;
		}
		
		if(file->attr.flags&AM_FONT_FL_FIXED_SIZE)
		{
			score += 200;
		}
		else
		{
			int width, height;
			
			if(file->drv->match_size)
			{
				width  = attr->width;
				height = attr->height;
				file->drv->match_size(file, &width, &height);
			}
			else
			{
				width  = file->attr.width;
				height = file->attr.height;
			}
			
			if(attr->width)
				score += 100-AM_MIN(100,AM_ABS(attr->width-width));
			else
				score += 100;
			if(attr->height)
				score += 100-AM_MIN(100,AM_ABS(attr->height-height));
			else
				score += 100;
		}

#define CHECK_FLAG(a,c)\
	AM_MACRO_BEGIN\
	if((file->attr.flags&(a))==(attr->flags&(a)))\
		score += (c);\
	AM_MACRO_END
		CHECK_FLAG(AM_FONT_FL_FIXED_WIDTH,10);
		CHECK_FLAG(AM_FONT_FL_FIXED_HEIGHT,10);
		CHECK_FLAG(AM_FONT_FL_FIXED_SIZE,10);
		CHECK_FLAG(AM_FONT_FL_ITALIC,20);
		CHECK_FLAG(AM_FONT_FL_BOLD,20);
		CHECK_FLAG(AM_FONT_FL_BASELINE,20);
		
		ref = charset_id(file->attr.charset);
		if(!f->refs[ref] || (match[ref]<score))
		{
			f->refs[ref] = file;
			match[ref] = score;
		}
	}
	
	/*Initialize the font*/
	for(ref=0; ref<FONT_FILE_REF_CNT; ref++)
	{
		AM_FONT_File_t *file = f->refs[ref];
		
		f->lineskip = 0;
		if(file && file->drv->open_font)
		{
			if(file->drv->open_font(f, file)!=AM_SUCCESS)
			{
				f->refs[ref] = NULL;
			}
		}
	}
	
	/*Add the font to the list*/
	f->prev = NULL;
	f->next = fonts;
	if(fonts)
		fonts->prev = f;
	fonts = f;
	
	pthread_mutex_unlock(&lock);
	
	*font_id = (int)f;
	
	return ret;
}

/**\brief 取得字体属性
 * \param font_id 字体句柄
 * \param[in] attr 要创建的字体属性
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_font.h)
 */
AM_ErrorCode_t AM_FONT_GetAttr(int font_id, AM_FONT_Attr_t *attr)
{
	AM_FONT_t *f = (AM_FONT_t*)font_id;
	
	pthread_mutex_lock(&lock);
	
	if(!f) f = def_font;
	
	assert(f && attr);
	
	*attr = f->attr;
	
	pthread_mutex_unlock(&lock);
	
	return AM_SUCCESS;
}

/**\brief 计算绘制指定字体字符串的高度和宽度
 * \param font_id 字体句柄
 * \param[in] text 待计算的字符串指针
 * \param len 字符串长度
 * \param[out] info 返回字符串信息
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_font.h)
 */
AM_ErrorCode_t AM_FONT_Size(int font_id, const char *text, int len, AM_FONT_TextInfo_t *info)
{
	AM_FONT_t *f = (AM_FONT_t*)font_id;
	AM_ErrorCode_t ret = AM_SUCCESS;
	const char *pch = text;
	int left = len;
	AM_FONT_Glyph_t glyph;
	int width, ascent, descent, bytes=0;
	
	width   = 0;
	ascent  = 0;
	descent = 0;
	
	pthread_mutex_lock(&lock);
	
	if(!f) f = def_font;
	
	assert(f && text && info);
	
	while(left)
	{
		const char *nch, *code;
		char chbuf[8];
		int csize, i, rsize;
		AM_FONT_Charset_t set = 0;
		AM_FONT_File_t *file = NULL;
		
		/*取下一个字符*/
		nch = next_char(pch, left, f->attr.encoding, &set);
		if(!nch)
		{
			AM_DEBUG(1, "illegal code in the string");
			break;
		}
		
		/*找到对应的字体文件*/
		for(i=0; i<FONT_FILE_REF_CNT; i++)
		{
			if(f->refs[i] && f->refs[i]->attr.charset&set)
			{
				file = f->refs[i];
				break;
			}
		}
		
		code  = pch;
		csize = nch - pch;
		rsize = csize;
		
		if(!file)
		{
			AM_DEBUG(1, "unsupported character");
			goto next_char;
		}
		
		/*转换编码*/
		if(file->attr.encoding!=f->attr.encoding)
		{
			rsize = sizeof(chbuf);
			ret = conv_char(code, csize, f->attr.encoding, chbuf, &rsize, file->attr.encoding);
			if(ret!=AM_SUCCESS)
				goto next_char;
			code  = chbuf;
			rsize = csize;
		}
		
		/*取字形信息*/
		ret = file->drv->char_size(f, file, code, rsize, &glyph);
		if(ret!=AM_SUCCESS)
			goto next_char;
		
		if((info->max_width>0) && bytes && ((width+glyph.advance)>info->max_width))
			break;
		
		width  += glyph.advance;
		ascent  = AM_MAX(ascent, glyph.ascent);
		descent = AM_MAX(descent, glyph.descent);
		
		if(info->stop_code)
		{
			int code = 0;
			
			switch(csize)
			{
				case 1:
					code = pch[0];
				break;
				case 2:
					code = (pch[0]<<8)|pch[1];
				break;
				case 3:
					code = (pch[0]<<16)|(pch[1]<<8)|pch[2];
				break;
				case 4:
					code = (pch[0]<<24)|(pch[1]<<16)|(pch[2]<<8)|pch[3];
				break;
			}
			
			if(code==info->stop_code)
			{
				bytes += csize;
				break;
			}
		}
next_char:
		bytes += csize;
		left  -= csize;
		pch   = nch;
	}
	
	pthread_mutex_unlock(&lock);
	
	info->width  = width;
	info->ascent = ascent;
	info->descent= descent;
	info->height = ascent+descent;
	info->bytes  = bytes;
	
	return ret;
}

/**\brief 用指定字体绘制字符串
 * \param font_id 字体句柄
 * \param[in] surface 绘图表面
 * \param[in] text 需要绘制的字符串
 * \param len 字符串长度
 * \param[in] pos 绘制字符串的位置
 * \param pixel 绘制文字使用的像素值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_font.h)
 */
AM_ErrorCode_t AM_FONT_Draw(int font_id, AM_OSD_Surface_t *surface, const char *text, int len, const AM_FONT_TextPos_t *pos, uint32_t pixel)
{
	AM_FONT_t *f = (AM_FONT_t*)font_id;
	AM_ErrorCode_t ret = AM_SUCCESS;
	const char *pch = text;
	int left = len;
	AM_FONT_Glyph_t glyph;
	int x, base, y;
	AM_OSD_Rect_t clip;
	
	assert(surface && text && pos);
	
	clip = surface->clip;
	x = pos->rect.x;
	base = pos->rect.y+pos->rect.h-pos->base;
	
	if(pos->rect.y>=(clip.y+clip.h))
		return AM_SUCCESS;
	if((pos->rect.y+pos->rect.h)<=clip.y)
		return AM_SUCCESS;
	
	pthread_mutex_lock(&lock);
	
	if (!f) f = def_font;
	
	assert(f);
	
	while(left)
	{
		const char *nch, *code;
		char chbuf[8];
		int csize, i, rsize;
		AM_FONT_Charset_t set = 0;
		AM_FONT_File_t *file = NULL;
		
		if(x>=clip.x+clip.w)
			break;
		
		/*取下一个字符*/
		nch = next_char(pch, left, f->attr.encoding, &set);
		if(!nch)
		{
			AM_DEBUG(1, "illegal code in the string");
			break;
		}
		
		/*找到对应的字体文件*/
		for(i=0; i<FONT_FILE_REF_CNT; i++)
		{
			if(f->refs[i] && f->refs[i]->attr.charset&set)
			{
				file = f->refs[i];
				break;
			}
		}
		
		code  = pch;
		csize = nch - pch;
		rsize = csize;
		
		if(!f)
		{
			AM_DEBUG(1, "unsupported character");
			goto next_char;
		}
		
		/*转换编码*/
		if(file->attr.encoding!=f->attr.encoding)
		{
			rsize = sizeof(chbuf);
			ret = conv_char(code, csize, f->attr.encoding, chbuf, &rsize, file->attr.encoding);
			if(ret!=AM_SUCCESS)
				goto next_char;
			code  = chbuf;
			rsize = csize;
		}
		
		/*取字形信息*/
		ret = file->drv->char_glyph(f, file, code, rsize, &glyph);
		if(ret!=AM_SUCCESS)
			goto next_char;
		
		y = base-glyph.ascent;
		
		/*绘制字符*/
		draw_glyph(surface, &glyph, x+glyph.left, y, pixel);
		
		x += glyph.advance;
		
next_char:
		left -= csize;
		pch = nch;
	}
	
	pthread_mutex_unlock(&lock);
	
	return ret;
}

/**\brief 销毁一个字体
 * \param font_id 要销毁的字体句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_font.h)
 */
AM_ErrorCode_t AM_FONT_Destroy(int font_id)
{
	AM_FONT_t *font = (AM_FONT_t*)font_id;
	int ref;
	
	assert(font);
	
	pthread_mutex_lock(&lock);
	
	if(font->prev)
		font->prev->next = font->next;
	else
		fonts = font->next;
	if(font->next)
		font->next->prev = font->prev;
	
	/*Relase the font resource*/
	for(ref=0; ref<FONT_FILE_REF_CNT; ref++)
	{
		AM_FONT_File_t *file = font->refs[ref];
		if(file && file->drv->close_font)
		{
			file->drv->close_font(font, file);
		}
	}
	
	if(def_font==font)
		def_font = NULL;
	
	pthread_mutex_unlock(&lock);
	
	free(font);
	
	return AM_SUCCESS;
}

/**\brief 终止字体管理模块
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_font.h)
 */
AM_ErrorCode_t AM_FONT_Quit(void)
{
	const AM_FONT_Driver_t *drv;
	AM_FONT_File_t *file, *nfile;
	AM_FONT_t *f, *nf;
	int i;
	
	/*Unload fonts*/
	f = fonts;
	while(f)
	{
		nf = f->next;
		free(f);
		f = nf;
	}
	
	/*Unload files*/
	file = font_files;
	while(file)
	{
		nfile = file->next;
		if(file->drv->close_file)
			file->drv->close_file(file);
		if(file->attr.name)
			free(file->attr.name);
		free(file);
		file = nfile;
	}
	font_files=0;
	/*Release drivers*/
	for(i=0; i<FONT_DRV_COUNT; i++)
	{
		drv = font_drivers[i];
		if(drv && drv->quit)
		{
			drv->quit();
		}
	}
	
	if(-1!=(int)gbk2unicode)
		iconv_close(gbk2unicode);
	if(-1!=(int)unicode2gbk)
		iconv_close(unicode2gbk);
	
	pthread_mutex_destroy(&lock);
	
	return AM_SUCCESS;
}

/**\brief 取指定字体的行间距
 * \param font_id 字体句柄
 * \param[out] 返回行间距值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_font.h)
 */
AM_ErrorCode_t AM_FONT_GetLineSkip(int font_id, int *lineskip)
{
	AM_FONT_t *f = (AM_FONT_t*)font_id;
	
	if(!f) f = def_font;
	
	pthread_mutex_lock(&lock);
	
	assert(f && lineskip);
	
	*lineskip = f->lineskip;
	
	pthread_mutex_unlock(&lock);
	
	return AM_SUCCESS;
}

