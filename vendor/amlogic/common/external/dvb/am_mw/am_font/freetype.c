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
 * \brief FreeType2 字体文件驱动
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-10-21: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_mem.h>
#include "am_font_internal.h"
#include <ft2build.h>
#include FT_FREETYPE_H 

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define DRAW_8BPP_BITMAP

#define FT_FLOOR(X)	((X & -64) / 64)
#define FT_CEIL(X)	(((X + 63) & -64) / 64)

/****************************************************************************
 * Type definition
 ***************************************************************************/

typedef struct
{
	FT_Face face;
	int     width;
	int     height;
} FONT_FTData_t;

/****************************************************************************
 * Static data
 ***************************************************************************/

static AM_ErrorCode_t ft_init (void);
static AM_ErrorCode_t ft_open_file (const char *path, AM_FONT_File_t *file);
static AM_ErrorCode_t ft_match_size (AM_FONT_File_t *file, int *width, int *height);
static AM_ErrorCode_t ft_open_font (AM_FONT_t *f, AM_FONT_File_t *file);
static AM_ErrorCode_t ft_char_size (AM_FONT_t *f, AM_FONT_File_t *file, const char *ch, int size, AM_FONT_Glyph_t *glyph);
static AM_ErrorCode_t ft_char_glyph (AM_FONT_t *f, AM_FONT_File_t *file, const char *ch, int size, AM_FONT_Glyph_t *glyph);
static AM_ErrorCode_t ft_close_file (AM_FONT_File_t *file);
static AM_ErrorCode_t ft_quit (void);

const AM_FONT_Driver_t ft_font_drv =
{
.name       = "freetype",
.init       = ft_init,
.open_file  = ft_open_file,
.match_size = ft_match_size,
.open_font  = ft_open_font,
.char_size  = ft_char_size,
.char_glyph = ft_char_glyph,
.close_file = ft_close_file,
.quit       = ft_quit
};

static FT_Library library;

/****************************************************************************
 * Functions
 ***************************************************************************/

static AM_ErrorCode_t ft_resize(FONT_FTData_t *data, AM_FONT_t *font)
{
	AM_Bool_t reset = AM_FALSE;
	int w = data->width, h = data->height;
	
	if(font->attr.width && (font->attr.width!=data->width))
	{
		w = font->attr.width;
		reset = AM_TRUE;
	}
	
	if(font->attr.height && (font->attr.height!=data->height))
	{
		h = font->attr.height;
		reset = AM_TRUE;
	}
	
	if(reset)
	{
		if(FT_IS_SCALABLE(data->face))
		{
			FT_Set_Pixel_Sizes(data->face, w, h);
		}
		else
		{
			int size_id = (int)font->drv_data;
			FT_Set_Pixel_Sizes(data->face,
				data->face->available_sizes[size_id].height,
				data->face->available_sizes[size_id].width);
		}
		data->width  = w;
		data->height = h;
	}
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t ft_init (void)
{
	FT_Error error;
	
	error = FT_Init_FreeType(&library);
	if(error)
	{
		AM_DEBUG(1, "FT_Init_FreeType failed");
	}
	
	return error?AM_FAILURE:AM_SUCCESS;
}

static AM_ErrorCode_t ft_open_file (const char *path, AM_FONT_File_t *file)
{
	FT_Error error;
	FONT_FTData_t *data = NULL;
	int flags = 0;
	
	data = (FONT_FTData_t*)malloc(sizeof(FONT_FTData_t));
	if(!data)
	{
		AM_DEBUG(1, "not enough memory");
		return AM_FONT_ERR_NO_MEM;
	}
	
	error = FT_New_Face(library, path, 0, &data->face);
	if(error)
	{
		AM_DEBUG(1, "FT_New_Face \"%s\" failed", path);
		free(data);
		return AM_FAILURE;
	}
	
	if(!FT_IS_SCALABLE(data->face))
		flags |= AM_FONT_FL_FIXED_SIZE;
	if(data->face->face_flags&FT_FACE_FLAG_FIXED_WIDTH)
		flags |= AM_FONT_FL_FIXED_WIDTH;
	if(data->face->style_flags&FT_STYLE_FLAG_ITALIC)
		flags |= AM_FONT_FL_ITALIC;
	if(data->face->style_flags&FT_STYLE_FLAG_BOLD)
		flags |= AM_FONT_FL_BOLD;
	
	file->attr.flags    = flags;
	file->attr.encoding = AM_FONT_ENC_UTF16;
	file->drv_data      = data;
	return AM_SUCCESS;
}

static AM_ErrorCode_t ft_match_size (AM_FONT_File_t *file, int *width, int *height)
{
	FONT_FTData_t *data = (FONT_FTData_t*)file->drv_data;
	FT_Face face = data->face;
	int i, mw, mh, aw, ah, min = 0x7fffffff, diff, mid = 0;
	
	if(face->face_flags&FT_FACE_FLAG_SCALABLE)
		return AM_SUCCESS;
	
	mw = *width;
	mh = *height;
	
	for(i=0; i<face->num_fixed_sizes; i++)
	{
		aw = FT_CEIL(face->available_sizes[i].width);
		ah = FT_CEIL(face->available_sizes[i].height);
		
		diff = 0;
		if(mw)
			diff += AM_ABS(mw-aw);
		if(mh)
			diff += AM_ABS(mh-ah);
		if(diff<min)
		{
			min = diff;
			mid = i;
		}
	}
	
	*width  = face->available_sizes[mid].width;
	*height = face->available_sizes[mid].height;
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t ft_open_font (AM_FONT_t *f, AM_FONT_File_t *file)
{
	FONT_FTData_t *data = (FONT_FTData_t*)file->drv_data;
	FT_Face face = data->face;
	int lineskip;
	
	if(!FT_IS_SCALABLE(data->face))
	{
		int i, min=0x7FFFFFFF, diff, size_id=0;
		
		for(i=0; i<face->num_fixed_sizes; i++)
		{
			diff = 0;
			if(f->attr.width)
				diff += AM_ABS(f->attr.width-FT_CEIL(face->available_sizes[i].width));
			if(f->attr.height)
				diff += AM_ABS(f->attr.height-FT_CEIL(face->available_sizes[i].height));
			if(diff<min)
			{
				min = diff;
				size_id = i;
			}
		}
		
		f->drv_data = (void*)size_id;
		lineskip = face->available_sizes[size_id].height;
	}
	else
	{
		lineskip = f->attr.height?f->attr.height:f->attr.width;
	}
	
	f->lineskip = AM_MAX(f->lineskip, lineskip);
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t ft_char_size (AM_FONT_t *f, AM_FONT_File_t *file, const char *ch, int size, AM_FONT_Glyph_t *glyph)
{
	FONT_FTData_t *data = (FONT_FTData_t*)file->drv_data;
	FT_Face face = data->face;
	AM_ErrorCode_t ret;
	uint16_t code;
	FT_Error error;
	FT_GlyphSlot ftglyph;
	FT_Glyph_Metrics *metrics;
	int height, advance, ascent, descent;
	
	ret = ft_resize(data, f);
	if(ret!=AM_SUCCESS)
		return ret;
	
	code = (ch[0]<<8)|ch[1];
	error = FT_Load_Char(face, code, 0);
	if(error)
	{
		AM_DEBUG(1, "FT_Load_char failed");
		return AM_FAILURE;
	}
	
	ftglyph = face->glyph;
	metrics = &ftglyph->metrics;
	
	if(FT_IS_SCALABLE(face))
	{
		height  = FT_FLOOR(metrics->height);
		advance = FT_CEIL(metrics->horiAdvance);
		ascent  = FT_CEIL(metrics->horiBearingY);
		descent = height-ascent;
	}
	else
	{
		int size_id = (int)f->drv_data;
		
		height  = FT_FLOOR(face->available_sizes[size_id].height);
		advance = FT_CEIL(metrics->horiAdvance);
		ascent  = height;
		descent = 0;
	}
	
	glyph->advance = advance;
	glyph->ascent  = ascent;
	glyph->descent = descent;
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t ft_char_glyph (AM_FONT_t *f, AM_FONT_File_t *file, const char *ch, int size, AM_FONT_Glyph_t *glyph)
{
	FONT_FTData_t *data = (FONT_FTData_t*)file->drv_data;
	FT_Face face = data->face;
	AM_ErrorCode_t ret;
	uint16_t code;
	FT_Error error;
	FT_GlyphSlot ftglyph;
	FT_Glyph_Metrics *metrics;
	FT_Bitmap *bmp;
	int height, advance, ascent, descent, left;
	
	ret = ft_resize(data, f);
	if(ret!=AM_SUCCESS)
		return ret;
	
	code = (ch[0]<<8)|ch[1];
	error = FT_Load_Char(face, code, 0);
	if(error)
	{
		AM_DEBUG(1, "FT_Load_char failed");
		return AM_FAILURE;
	}
	
	ftglyph = face->glyph;
	metrics = &ftglyph->metrics;

#ifdef DRAW_8BPP_BITMAP
	error = FT_Render_Glyph(ftglyph, ft_render_mode_normal);
#else
	error = FT_Render_Glyph(ftglyph, ft_render_mode_mono);
#endif
	if(error)
	{
		AM_DEBUG(1, "FT_Render_Glyph failed");
		return AM_FAILURE;
	}
	
	bmp = &ftglyph->bitmap;
	
	if(FT_IS_SCALABLE(face))
	{
		left    = FT_CEIL(metrics->horiBearingX);
		height  = FT_FLOOR(metrics->height);
		advance = FT_CEIL(metrics->horiAdvance);
		ascent  = FT_CEIL(metrics->horiBearingY);
		descent = height-ascent;
	}
	else
	{
		int size_id = (int)f->drv_data;
		
		left    = FT_CEIL(metrics->horiBearingX);
		height  = FT_FLOOR(face->available_sizes[size_id].height);
		advance = FT_CEIL(metrics->horiAdvance);
		ascent  = height;
		descent = 0;
	}

#ifdef DRAW_8BPP_BITMAP
	glyph->type    = AM_FONT_BITMAP_8BPP;
#else
	glyph->type    = AM_FONT_BITMAP_1BPP;
#endif
	glyph->buffer  = (char*)bmp->buffer;
	glyph->width   = bmp->width;
	glyph->height  = bmp->rows;
	glyph->pitch   = bmp->pitch;
	glyph->left    = left;
	glyph->ascent  = ascent;
	glyph->advance = advance;
	glyph->descent = descent;
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t ft_close_file (AM_FONT_File_t *file)
{
	FONT_FTData_t *data = (FONT_FTData_t*)file->drv_data;
	
	FT_Done_Face(data->face);
	free(data);
	return AM_SUCCESS;
}

static AM_ErrorCode_t ft_quit (void)
{
	FT_Done_FreeType(library);
	return AM_SUCCESS;
}

