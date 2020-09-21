#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "image.h"

#include <png.h>

/* png_jmpbuf() macro is not defined prior to libpng-1.0.6*/
#ifndef png_jmpbuf
#define png_jmpbuf(png_ptr)	((png_ptr)->jmpbuf)
#endif

/* This is a quick user defined function to read from the buffer instead of from the file pointer */
static void
png_read_buffer(png_structp pstruct, png_bytep pointer, png_size_t size)
{
	GdImageBufferRead(pstruct->io_ptr, pointer, size);
}

int
GdDecodePNG(buffer_t * src, const AM_IMG_DecodePara_t *para, AM_OSD_Surface_t **image)
{
	AM_OSD_Surface_t *s = NULL;
	unsigned char hdr[8], **rows;
	png_structp state;
	png_infop pnginfo;
	png_uint_32 width, height;
	int bit_depth, color_type, i;
	double file_gamma;
	int channels, alpha_present;
	png_bytep trans;
	int num_trans;
	png_color_16p trans_values;
	AM_OSD_PixelFormatType_t fmt;
	int flags = 0;
	
	GdImageBufferSeekTo(src, 0UL);

	if(GdImageBufferRead(src, hdr, 8) != 8)
		return 0;

	if(png_sig_cmp(hdr, 0, 8))
		return 0;

	if(!(state = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, 
						NULL, NULL))) goto nomem;

	if(!(pnginfo = png_create_info_struct(state))) {
		png_destroy_read_struct(&state, NULL, NULL);
		goto nomem;
	}

	if(setjmp(png_jmpbuf(state))) {
		png_destroy_read_struct(&state, &pnginfo, NULL);
		return 2;
	}

	/* Set up the input function */
	png_set_read_fn(state, src, png_read_buffer);
	/* png_init_io(state, src); */

	png_set_sig_bytes(state, 8);

	png_read_info(state, pnginfo);
	png_get_IHDR(state, pnginfo, &width, &height, &bit_depth, &color_type,
							NULL, NULL, NULL);
	/* set-up the transformations */
	/* transform paletted images into full-color rgb */
	if (color_type == PNG_COLOR_TYPE_PALETTE)
	    png_set_expand (state);
	/* expand images to bit-depth 8 (only applicable for grayscale images) */
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
	    png_set_expand (state);
	/* transform transparency maps into full alpha-channel */
	if (png_get_valid (state, pnginfo, PNG_INFO_tRNS))
	    png_set_expand (state);
	
	/* downgrade 16-bit images to 8 bit */
	if (bit_depth == 16)
	    png_set_strip_16 (state);
	/* Handle transparency... */
	if (png_get_valid(state, pnginfo, PNG_INFO_tRNS))
	    png_set_tRNS_to_alpha(state);
	/* transform grayscale images into full-color */
	if (color_type == PNG_COLOR_TYPE_GRAY ||
	    color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
	    png_set_gray_to_rgb (state);
	/* only if file has a file gamma, we do a correction */
	if (png_get_gAMA (state, pnginfo, &file_gamma))
	    png_set_gamma (state, (double) 2.2, file_gamma);

	/* all transformations have been registered; now update pnginfo data,
	 * get rowbytes and channels, and allocate image memory */

	png_read_update_info (state, pnginfo);

	/* get the new color-type and bit-depth (after expansion/stripping) */
	png_get_IHDR (state, pnginfo, &width, &height, &bit_depth, &color_type,
	    NULL, NULL, NULL);

	/* calculate new number of channels and store alpha-presence */
	if (color_type == PNG_COLOR_TYPE_GRAY)
	    channels = 1;
	else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
	    channels = 2;
	else if (color_type == PNG_COLOR_TYPE_RGB)
	    channels = 3;
	else if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
	    channels = 4;
	else
	    channels = 0; /* should never happen */
	alpha_present = (channels - 1) % 2;
	
	/* old code */
	fmt = GdGetPixelFormatType(channels*8, 0);
	if(para)
		flags |= para->surface_flags&(AM_OSD_SURFACE_FL_HW|AM_OSD_SURFACE_FL_OPENGL);
	
	if(color_type&PNG_COLOR_MASK_ALPHA)
		flags |= AM_OSD_SURFACE_FL_ALPHA;
	
	png_set_swap_alpha(state);
	
	if(AM_OSD_CreateSurface(fmt, width, height, flags, &s)!=AM_SUCCESS)
		goto nomem;
	
	if (alpha_present)
		s->flags |= AM_OSD_SURFACE_FL_ALPHA; /*FIXME - add MWIMAGE_RGB*/
	
        if(!(rows = malloc(s->height * sizeof(unsigned char *)))) {
		png_destroy_read_struct(&state, &pnginfo, NULL);
		goto nomem;
        }
	for(i = 0; i < s->height; i++)
		rows[i] = s->buffer + (i * s->pitch);

	png_read_image(state, rows);
	png_read_end(state, NULL);
	free(rows);
	png_destroy_read_struct(&state, &pnginfo, NULL);
	
	*image = s;
	return 1;

nomem:
	EPRINTF("GdDecodePNG: Out of memory\n");
	return 2;
}

