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
 *
 * Image decode routine for TIFF files
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
#include <tiffio.h>

int
GdDecodeTIFF(const char *path, const AM_IMG_DecodePara_t *para, AM_OSD_Surface_t **image)
{
	AM_OSD_Surface_t *s = NULL;
	TIFF 	*tif;
	int	w, h;
	long	size;
	int     flags;

	tif = TIFFOpen(path, "r");
	if (!tif)
		return 0;

	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
	
	flags = para?(para->surface_flags&(AM_OSD_SURFACE_FL_HW|AM_OSD_SURFACE_FL_OPENGL)):0;
	
	if(AM_OSD_CreateSurface(AM_OSD_FMT_COLOR_8888, w, h, flags, &s)!=AM_SUCCESS)
		return 0;
	
	TIFFReadRGBAImage(tif, s->width, s->height,
		(uint32_t *)s->buffer, 0);

#if 0
	{
		/* FIXME alpha channel should be blended with destination*/
		int i;
		uint32	*rgba;
		uint32	rgba_r, rgba_g, rgba_b, rgba_a;
		rgba = (uint32 *)pimage->imagebits;
		for (i = 0; i < size; ++i, ++rgba) {
			if ((rgba_a = TIFFGetA(*rgba) + 1) == 256)
				continue;
			rgba_r = (TIFFGetR(*rgba) * rgba_a)>>8;
			rgba_g = (TIFFGetG(*rgba) * rgba_a)>>8;
			rgba_b = (TIFFGetB(*rgba) * rgba_a)>>8;
			*rgba = 0xff000000|(rgba_b<<16)|(rgba_g<<8)|(rgba_r);
		}
	}
#endif
	TIFFClose(tif);
	return 1;

err:
	EPRINTF("GdDecodeTIFF: image loading error\n");
	if (tif)
		TIFFClose(tif);
	if(s)
		AM_OSD_DestroySurface(s);
	return 2;		/* image error*/
}

