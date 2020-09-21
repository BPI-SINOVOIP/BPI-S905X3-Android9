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
#include <jpeglib.h>

static void
init_source(j_decompress_ptr cinfo)
{
	buffer_t *inptr = (buffer_t*)cinfo->client_data;
	
	cinfo->src->next_input_byte = inptr->start;
	cinfo->src->bytes_in_buffer = inptr->size;
}

static boolean
fill_input_buffer(j_decompress_ptr cinfo)
{
	return TRUE;
}

static void
skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
	buffer_t *inptr = (buffer_t*)cinfo->client_data;
	
	if (num_bytes >= inptr->size)
		return;
	cinfo->src->next_input_byte += num_bytes;
	cinfo->src->bytes_in_buffer -= num_bytes;
}

static boolean
resync_to_restart(j_decompress_ptr cinfo, int desired)
{
	return jpeg_resync_to_restart(cinfo, desired);
}

static void
term_source(j_decompress_ptr cinfo)
{
	return;
}

int
GdDecodeJPEG(buffer_t* src, const AM_IMG_DecodePara_t *para, AM_OSD_Surface_t **image, int fast_grayscale)
{
	int ret = 2;		/* image load error */
	unsigned char magic[8];
	struct jpeg_source_mgr smgr;
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	AM_OSD_Surface_t *s = NULL;
	int width, height, pitch, bpp, bytesperpixel, palsize, flags;
	AM_OSD_PixelFormatType_t fmt;

	/* first determine if JPEG file since decoder will error if not */
	GdImageBufferSeekTo(src, 0UL);
	if (GdImageBufferRead(src, magic, 2) != 2
	 || magic[0] != 0xFF || magic[1] != 0xD8)
		return 0;	/* not JPEG image */

	if (GdImageBufferRead(src, magic, 8) != 8
	 || (strncmp((char*)magic+4, "JFIF", 4) != 0
          && strncmp((char*)magic+4, "Exif", 4) != 0))
		return 0;	/* not JPEG image */

	GdImageBufferSeekTo(src, 0);
	
	cinfo.client_data = src;
	
	/* Step 1: allocate and initialize JPEG decompression object */
	/* We set up the normal JPEG error routines. */
	cinfo.err = jpeg_std_error(&jerr);

	/* Now we can initialize the JPEG decompression object. */
	jpeg_create_decompress(&cinfo);

	/* Step 2:  Setup the source manager */
	if(src->size)
	{
		smgr.init_source = (void *) init_source;
		smgr.fill_input_buffer = (void *) fill_input_buffer;
		smgr.skip_input_data = (void *) skip_input_data;
		smgr.resync_to_restart = (void *) resync_to_restart;
		smgr.term_source = (void *) term_source;
		cinfo.src = &smgr;
	}
	else
	{
		jpeg_stdio_src(&cinfo, (FILE*)src->start);
	}
	
	/* Step 3: read file parameters with jpeg_read_header() */
	jpeg_read_header(&cinfo, TRUE);
	
	/* Step 4: set parameters for decompression */
	cinfo.out_color_space = fast_grayscale? JCS_GRAYSCALE: JCS_RGB;
	cinfo.quantize_colors = FALSE;
	
	if (fast_grayscale) {
		/* Grayscale output asked */
		cinfo.quantize_colors = TRUE;
		cinfo.out_color_space = JCS_GRAYSCALE;
		cinfo.desired_number_of_colors = 256;
	}
	jpeg_calc_output_dimensions(&cinfo);

	width = cinfo.output_width;
	height = cinfo.output_height;
	bpp = fast_grayscale? 8: cinfo.output_components*8;
	GdComputeImagePitch(bpp, width, &pitch, &bytesperpixel);
	palsize = (bpp == 8)? 256: 0;
	fmt = GdGetPixelFormatType(bpp, 0);
	if(fmt==-1)
		return 0;
	
	flags = para?(para->surface_flags&(AM_OSD_SURFACE_FL_HW|AM_OSD_SURFACE_FL_OPENGL)):0;
	if(AM_OSD_CreateSurface(fmt, width, height, flags, &s)!=AM_SUCCESS)
		return 0;

	/* Step 5: Start decompressor */
	jpeg_start_decompress (&cinfo);

	/* Step 6: while (scan lines remain to be read) */
	while(cinfo.output_scanline < cinfo.output_height) {
		JSAMPROW rowptr[1];
		rowptr[0] = (JSAMPROW)(s->buffer +
			cinfo.output_scanline * s->pitch);
		jpeg_read_scanlines (&cinfo, rowptr, 1);
	}
	ret = 1;

	/* Step 7: Finish decompression */
	jpeg_finish_decompress (&cinfo);

	/* Step 8: Release JPEG decompression object */
	jpeg_destroy_decompress (&cinfo);

	/* May want to check to see whether any corrupt-data
	 * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
	 */
	if(ret==1)
		*image = s;
	else if(s)
		AM_OSD_DestroySurface(s);
	return ret;
}

