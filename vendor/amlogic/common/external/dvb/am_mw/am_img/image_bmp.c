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
 * Image decode routine for BMP files
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

/* BMP stuff*/
#define BI_RGB		0L
#define BI_RLE8		1L
#define BI_RLE4		2L
#define BI_BITFIELDS	3L

typedef uint8_t         BYTE;
typedef uint16_t        WORD;
typedef uint32_t        DWORD;
typedef int32_t         LONG;

typedef struct {
	/* BITMAPFILEHEADER*/
	BYTE	bfType[2];
	DWORD	bfSize;
	WORD	bfReserved1;
	WORD	bfReserved2;
	DWORD	bfOffBits;
} BMPFILEHEAD;

#define FILEHEADSIZE 14

/* windows style*/
typedef struct {
	/* BITMAPINFOHEADER*/
	DWORD	BiSize;
	DWORD	BiWidth;
	DWORD	BiHeight;
	WORD	BiPlanes;
	WORD	BiBitCount;
	DWORD	BiCompression;
	DWORD	BiSizeImage;
	DWORD	BiXpelsPerMeter;
	DWORD	BiYpelsPerMeter;
	DWORD	BiClrUsed;
	DWORD	BiClrImportant;
} BMPINFOHEAD;

#define INFOHEADSIZE 40

/* os/2 style*/
typedef struct {
	/* BITMAPCOREHEADER*/
	DWORD	bcSize;
	WORD	bcWidth;
	WORD	bcHeight;
	WORD	bcPlanes;
	WORD	bcBitCount;
} BMPCOREHEAD;

#define COREHEADSIZE 12

static int	DecodeRLE8(MWUCHAR *buf, buffer_t *src);
static int	DecodeRLE4(MWUCHAR *buf, buffer_t *src);

/*
 * BMP decoding routine
 */

int
GdDecodeBMP(buffer_t *src, const AM_IMG_DecodePara_t *para, AM_OSD_Surface_t **image)
{
	int		h, i, compression;
	int		headsize;
	MWUCHAR		*imagebits;
	BMPFILEHEAD	bmpf;
	BMPINFOHEAD	bmpi;
	BMPCOREHEAD	bmpc;
	MWUCHAR 	headbuffer[INFOHEADSIZE];
	AM_OSD_Surface_t *s = NULL;
	int             bpp, bytesperpixel, width, height, pitch, palsize = 0, flags;
	AM_OSD_PixelFormatType_t fmt;

	GdImageBufferSeekTo(src, 0UL);

	/* read BMP file header*/
	if (GdImageBufferRead(src, &headbuffer, FILEHEADSIZE) != FILEHEADSIZE)
		return 0;

	bmpf.bfType[0] = headbuffer[0];
	bmpf.bfType[1] = headbuffer[1];

	/* Is it really a bmp file ? */
	if((bmpf.bfType[0]!='B') || (bmpf.bfType[1]!='M'))
		return 0;	/* not bmp image*/

	/*bmpf.bfSize = dwswap(dwread(&headbuffer[2]));*/
	bmpf.bfOffBits = dwswap(dwread(&headbuffer[10]));

	/* Read remaining header size */
	if (GdImageBufferRead(src,&headsize,sizeof(DWORD)) != sizeof(DWORD))
		return 0;	/* not bmp image*/
	headsize = dwswap(headsize);

	/* might be windows or os/2 header */
	if(headsize == COREHEADSIZE) {

		/* read os/2 header */
		if(GdImageBufferRead(src, &headbuffer, COREHEADSIZE-sizeof(DWORD)) !=
			COREHEADSIZE-sizeof(DWORD))
				return 0;	/* not bmp image*/

		/* Get data */
		bmpc.bcWidth = wswap(*(WORD*)&headbuffer[0]);
		bmpc.bcHeight = wswap(*(WORD*)&headbuffer[2]);
		bmpc.bcPlanes = wswap(*(WORD*)&headbuffer[4]);
		bmpc.bcBitCount = wswap(*(WORD*)&headbuffer[6]);
		
		width = (int)bmpc.bcWidth;
		height = (int)bmpc.bcHeight;
		bpp = bmpc.bcBitCount;
		if (bpp <= 8)
			palsize = 1 << bpp;
		else palsize = 0;
		compression = BI_RGB;
	} else {
		/* read windows header */
		if(GdImageBufferRead(src, &headbuffer, INFOHEADSIZE-sizeof(DWORD)) !=
			INFOHEADSIZE-sizeof(DWORD))
				return 0;	/* not bmp image*/

		/* Get data */
		bmpi.BiWidth = dwswap(*(DWORD*)&headbuffer[0]);
		bmpi.BiHeight = dwswap(*(DWORD*)&headbuffer[4]);
		bmpi.BiPlanes = wswap(*(WORD*)&headbuffer[8]);
		bmpi.BiBitCount = wswap(*(WORD*)&headbuffer[10]);
		bmpi.BiCompression = dwswap(*(DWORD*)&headbuffer[12]);
		bmpi.BiSizeImage = dwswap(*(DWORD*)&headbuffer[16]);
		bmpi.BiXpelsPerMeter = dwswap(*(DWORD*)&headbuffer[20]);
		bmpi.BiYpelsPerMeter = dwswap(*(DWORD*)&headbuffer[24]);
		bmpi.BiClrUsed = dwswap(*(DWORD*)&headbuffer[28]);
		bmpi.BiClrImportant = dwswap(*(DWORD*)&headbuffer[32]);
		width = (int)bmpi.BiWidth;
		height = (int)bmpi.BiHeight;
		bpp = bmpi.BiBitCount;
		palsize = (int)bmpi.BiClrUsed;
		if (palsize > 256)
			palsize = 0;
		else if(palsize == 0 && bpp <= 8)
			palsize = 1 << bpp;
		compression = bmpi.BiCompression;
	}
	
	/* only 8, 16, 24 and 32 bpp bitmaps*/
	fmt = GdGetPixelFormatType(bpp, 1);
	if(fmt==-1)
	{
		EPRINTF("GdDecodeBMP: image bpp not 8, 16, 24 or 32\n");
		return 2;	/* image loading error*/
	}

	/* compute byte line size and bytes per pixel*/
	GdComputeImagePitch(bpp, width, &pitch, &bytesperpixel);

	/* Allocate image */
	flags = para?(para->surface_flags&(AM_OSD_SURFACE_FL_HW|AM_OSD_SURFACE_FL_OPENGL)):0;
	if(bpp<=8)
		flags |= AM_OSD_SURFACE_FL_PRIV_FMT;
	
	if(AM_OSD_CreateSurface(fmt, width, height, flags, &s)!=AM_SUCCESS)
		goto err;
	
	/* get colormap*/
	if(bpp <= 8) {
		for(i=0; i<palsize; i++) {
			if(i>=s->format->palette.color_count)
				continue;
			s->format->palette.colors[i].b = GdImageBufferGetChar(src);
			s->format->palette.colors[i].g = GdImageBufferGetChar(src);
			s->format->palette.colors[i].r = GdImageBufferGetChar(src);
			s->format->palette.colors[i].a = 0xFF;
			if(headsize != COREHEADSIZE)
				GdImageBufferGetChar(src);
		}
	}

	/* determine 16bpp 5/5/5 or 5/6/5 format*/
	if (bpp == 16) {
		unsigned long format = 0x7c00;		/* default is 5/5/5*/

		if (compression == BI_BITFIELDS) {
			MWUCHAR	buf[4];

			if (GdImageBufferRead(src, &buf, sizeof(DWORD)) != sizeof(DWORD))
				goto err;
			format = dwswap(dwread(buf));
		}
		if (format == 0x7c00)
		{
			AM_OSD_GetFormatPara(AM_OSD_FMT_COLOR_1555, &s->format);
		}
	}

	/* decode image data*/
	GdImageBufferSeekTo(src, bmpf.bfOffBits);

	h = s->height;
	/* For every row ... */
	while (--h >= 0) {
		/* turn image rightside up*/
		imagebits = s->buffer + h*s->pitch;

		/* Get row data from file */
		if(compression == BI_RLE8) {
			if(!DecodeRLE8(imagebits, src))
				break;
		} else if(compression == BI_RLE4) {
			if(!DecodeRLE4(imagebits, src))
				break;
		} else {
			if(GdImageBufferRead(src, imagebits, s->pitch) !=
				s->pitch)
					goto err;
		}
	}
	*image = s;
	return 1;		/* bmp image ok*/
	
err:
	EPRINTF("GdDecodeBMP: image loading error\n");
	if(s)
		AM_OSD_DestroySurface(s);
	return 2;		/* bmp image error*/
}

/*
 * Decode one line of RLE8, return 0 when done with all bitmap data
 */
static int
DecodeRLE8(MWUCHAR * buf, buffer_t *src)
{
	int c, n;
	MWUCHAR *p = buf;

	for (;;) {
		switch (n = GdImageBufferGetChar(src)) {
		case EOF:
			return 0;
		case 0:	/* 0 = escape */
			switch (n = GdImageBufferGetChar(src)) {
			case 0:		/* 0 0 = end of current scan line */
				return 1;
			case 1:		/* 0 1 = end of data */
				return 1;
			case 2:		/* 0 2 xx yy delta mode NOT SUPPORTED */
				(void) GdImageBufferGetChar(src);
				(void) GdImageBufferGetChar(src);
				continue;
			default:	/* 0 3..255 xx nn uncompressed data */
				for (c = 0; c < n; c++)
					*p++ = GdImageBufferGetChar(src);
				if (n & 1)
					(void) GdImageBufferGetChar(src);
				continue;
			}
		default:
			c = GdImageBufferGetChar(src);
			while (n--)
				*p++ = c;
			continue;
		}
	}
}

/*
 * Decode one line of RLE4, return 0 when done with all bitmap data
 */
typedef struct {
	MWUCHAR *p;
	int once;
} RLE4Context_t;

static void
put4_real(int b, RLE4Context_t *ctx)
{
	static int last;

	last = (last << 4) | b;
	if (++ctx->once == 2) {
		*ctx->p++ = last;
		ctx->once = 0;
	}
}

#define put4(b)    put4_real(b, &ctx)

static int
DecodeRLE4(MWUCHAR * buf, buffer_t * src)
{
	int c, n, c1, c2;
	RLE4Context_t ctx;
	
	ctx.p = buf;
	ctx.once = 0;
	c1 = 0;

	for (;;) {
		switch (n = GdImageBufferGetChar(src)) {
		case EOF:
			return 0;
		case 0:	/* 0 = escape */
			switch (n = GdImageBufferGetChar(src)) {
			case 0:		/* 0 0 = end of current scan line */
				if (ctx.once)
					put4(0);
				return 1;
			case 1:		/* 0 1 = end of data */
				if (ctx.once)
					put4(0);
				return 1;
			case 2:		/* 0 2 xx yy delta mode NOT SUPPORTED */
				(void) GdImageBufferGetChar(src);
				(void) GdImageBufferGetChar(src);
				continue;
			default:	/* 0 3..255 xx nn uncompressed data */
				c2 = (n + 3) & ~3;
				for (c = 0; c < c2; c++) {
					if ((c & 1) == 0)
						c1 = GdImageBufferGetChar(src);
					if (c < n)
						put4((c1 >> 4) & 0x0f);
					c1 <<= 4;
				}
				continue;
			}
		default:
			c = GdImageBufferGetChar(src);
			c1 = (c >> 4) & 0x0f;
			c2 = c & 0x0f;
			for (c = 0; c < n; c++)
				put4((c & 1) ? c2 : c1);
			continue;
		}
	}
}

