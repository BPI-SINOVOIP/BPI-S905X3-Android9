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

#define	MAXCOLORMAPSIZE		256
#define	MAX_LWZ_BITS		12
#define INTERLACE		0x40
#define LOCALCOLORMAP		0x80

#define CM_RED		0
#define CM_GREEN	1
#define CM_BLUE		2

#define TRUE            1
#define FALSE           0

#define BitSet(byte, bit)	(((byte) & (bit)) == (bit))
#define	ReadOK(src,buffer,len)	GdImageBufferRead(src, buffer, len)
#define LM_to_uint(a,b)		(((b)<<8)|(a))

typedef struct {
    struct {
	    unsigned int Width;
	    unsigned int Height;
	    unsigned char ColorMap[3][MAXCOLORMAPSIZE];
	    unsigned int BitPixel;
	    unsigned int ColorResolution;
	    unsigned int Background;
	    unsigned int AspectRatio;
	    int GrayScale;
    } GifScreen;
    struct {
	    long transparent;
	    int delayTime;
	    int inputFlag;
	    int disposal;
    } Gif89;
    int ZeroDataBlock;
} GifContext_t;

static int ReadColorMap(GifContext_t *ctx, buffer_t *src, int number,
		unsigned char buffer[3][MAXCOLORMAPSIZE], int *flag);
static int DoExtension(GifContext_t *ctx, buffer_t *src, int label);
static int GetDataBlock(GifContext_t *ctx, buffer_t *src, unsigned char *buf);
static int GetCode(GifContext_t *ctx, buffer_t *src, int code_size, int flag);
static int LWZReadByte(GifContext_t *ctx, buffer_t *src, int flag, int input_code_size);
static int ReadImage(GifContext_t *ctx, buffer_t *src, const AM_IMG_DecodePara_t *para, AM_OSD_Surface_t **s, int len, int height, int,
		unsigned char cmap[3][MAXCOLORMAPSIZE],
		int gray, int interlace, int ignore);

int
GdDecodeGIF(buffer_t *src, const AM_IMG_DecodePara_t *para, AM_OSD_Surface_t **image)
{
    AM_OSD_Surface_t *s = NULL;
    unsigned char buf[16];
    unsigned char c;
    unsigned char localColorMap[3][MAXCOLORMAPSIZE];
    int grayScale;
    int useGlobalColormap;
    int bitPixel;
    int imageCount = 0;
    char version[4];
    int imageNumber = 1;
    int ok = 0;
    GifContext_t ctx;
    
    ctx.ZeroDataBlock = FALSE;

    GdImageBufferSeekTo(src, 0UL);

    if (!ReadOK(src, buf, 6))
        return 0;		/* not gif image*/
    if (strncmp((char *) buf, "GIF", 3) != 0)
        return 0;
    strncpy(version, (char *) buf + 3, 3);
    version[3] = '\0';

    if (strcmp(version, "87a") != 0 && strcmp(version, "89a") != 0) {
	EPRINTF("GdDecodeGIF: GIF version number not 87a or 89a\n");
        return 2;		/* image loading error*/
    }
    ctx.Gif89.transparent = -1;
    ctx.Gif89.delayTime = -1;
    ctx.Gif89.inputFlag = -1;
    ctx.Gif89.disposal = 0;

    if (!ReadOK(src, buf, 7)) {
	EPRINTF("GdDecodeGIF: bad screen descriptor\n");
        return 2;		/* image loading error*/
    }
    ctx.GifScreen.Width = LM_to_uint(buf[0], buf[1]);
    ctx.GifScreen.Height = LM_to_uint(buf[2], buf[3]);
    ctx.GifScreen.BitPixel = 2 << (buf[4] & 0x07);
    ctx.GifScreen.ColorResolution = (((buf[4] & 0x70) >> 3) + 1);
    ctx.GifScreen.Background = buf[5];
    ctx.GifScreen.AspectRatio = buf[6];

    if (BitSet(buf[4], LOCALCOLORMAP)) {	/* Global Colormap */
	if (ReadColorMap(&ctx, src, ctx.GifScreen.BitPixel, ctx.GifScreen.ColorMap,
			 &ctx.GifScreen.GrayScale)) {
	    EPRINTF("GdDecodeGIF: bad global colormap\n");
            return 2;		/* image loading error*/
	}
    }

    do {
	if (!ReadOK(src, &c, 1)) {
	    EPRINTF("GdDecodeGIF: EOF on image data\n");
            goto done;
	}
	if (c == ';') {		/* GIF terminator */
	    if (imageCount < imageNumber) {
		EPRINTF("GdDecodeGIF: no image %d of %d\n", imageNumber,imageCount);
                goto done;
	    }
	}
	if (c == '!') {		/* Extension */
	    if (!ReadOK(src, &c, 1)) {
		EPRINTF("GdDecodeGIF: EOF on extension function code\n");
                goto done;
	    }
	    DoExtension(&ctx, src, c);
	    continue;
	}
	if (c != ',') {		/* Not a valid start character */
	    continue;
	}
	++imageCount;

	if (!ReadOK(src, buf, 9)) {
	    EPRINTF("GdDecodeGIF: bad image size\n");
            goto done;
	}
	useGlobalColormap = !BitSet(buf[8], LOCALCOLORMAP);

	bitPixel = 1 << ((buf[8] & 0x07) + 1);

	if (!useGlobalColormap) {
	    if (ReadColorMap(&ctx, src, bitPixel, localColorMap, &grayScale)) {
		EPRINTF("GdDecodeGIF: bad local colormap\n");
                goto done;
	    }
	    ok = ReadImage(&ctx, src, para, &s, LM_to_uint(buf[4], buf[5]),
			      LM_to_uint(buf[6], buf[7]),
			      bitPixel, localColorMap, grayScale,
			      BitSet(buf[8], INTERLACE),
			      imageCount != imageNumber);
	} else {
	    ok = ReadImage(&ctx, src, para, &s, LM_to_uint(buf[4], buf[5]),
			      LM_to_uint(buf[6], buf[7]),
			      ctx.GifScreen.BitPixel, ctx.GifScreen.ColorMap,
			      ctx.GifScreen.GrayScale, BitSet(buf[8], INTERLACE),
			      imageCount != imageNumber);
	}
    } while (ok == 0);

    if (ok)
    {
    	if(ctx.Gif89.transparent!=-1)
    	{
    		s->flags |= AM_OSD_SURFACE_FL_COLOR_KEY;
    		s->color_key = ctx.Gif89.transparent;
    	}
        *image = s;
        return 1;		/* image load ok*/
    }
done:
    if(s)
    	AM_OSD_DestroySurface(s);
    return 2;			/* image load error*/
}

static int
ReadColorMap(GifContext_t *ctx, buffer_t *src, int number, unsigned char buffer[3][MAXCOLORMAPSIZE],
    int *gray)
{
    int i;
    unsigned char rgb[3];
    int flag;

    flag = TRUE;

    for (i = 0; i < number; ++i) {
	if (!ReadOK(src, rgb, sizeof(rgb)))
	    return 1;
	buffer[CM_RED][i] = rgb[0];
	buffer[CM_GREEN][i] = rgb[1];
	buffer[CM_BLUE][i] = rgb[2];
	flag &= (rgb[0] == rgb[1] && rgb[1] == rgb[2]);
    }

#if 0
    if (flag)
	*gray = (number == 2) ? PBM_TYPE : PGM_TYPE;
    else
	*gray = PPM_TYPE;
#else
    *gray = 0;
#endif

    return FALSE;
}

static int
DoExtension(GifContext_t *ctx, buffer_t *src, int label)
{
    static unsigned char buf[256];

    switch (label) {
    case 0x01:			/* Plain Text Extension */
	break;
    case 0xff:			/* Application Extension */
	break;
    case 0xfe:			/* Comment Extension */
	while (GetDataBlock(ctx, src, (unsigned char *) buf) != 0);
	return FALSE;
    case 0xf9:			/* Graphic Control Extension */
	GetDataBlock(ctx, src, (unsigned char *) buf);
	ctx->Gif89.disposal = (buf[0] >> 2) & 0x7;
	ctx->Gif89.inputFlag = (buf[0] >> 1) & 0x1;
	ctx->Gif89.delayTime = LM_to_uint(buf[1], buf[2]);
	if ((buf[0] & 0x1) != 0)
	    ctx->Gif89.transparent = buf[3];

	while (GetDataBlock(ctx, src, (unsigned char *) buf) != 0);
	return FALSE;
    default:
	break;
    }

    while (GetDataBlock(ctx, src, (unsigned char *) buf) != 0);

    return FALSE;
}

static int
GetDataBlock(GifContext_t *ctx, buffer_t *src, unsigned char *buf)
{
    unsigned char count;

    if (!ReadOK(src, &count, 1))
	return -1;
    ctx->ZeroDataBlock = count == 0;

    if ((count != 0) && (!ReadOK(src, buf, count)))
	return -1;
    return count;
}

static int
GetCode(GifContext_t *ctx, buffer_t *src, int code_size, int flag)
{
    static unsigned char buf[280];
    static int curbit, lastbit, done, last_byte;
    int i, j, ret;
    unsigned char count;

    if (flag) {
	curbit = 0;
	lastbit = 0;
	done = FALSE;
	return 0;
    }
    if ((curbit + code_size) >= lastbit) {
	if (done) {
	    if (curbit >= lastbit)
		EPRINTF("GdDecodeGIF: bad decode\n");
	    return -1;
	}
	buf[0] = buf[last_byte - 2];
	buf[1] = buf[last_byte - 1];

	if ((count = GetDataBlock(ctx, src, &buf[2])) == 0)
	    done = TRUE;

	last_byte = 2 + count;
	curbit = (curbit - lastbit) + 16;
	lastbit = (2 + count) * 8;
    }
    ret = 0;
    for (i = curbit, j = 0; j < code_size; ++i, ++j)
	ret |= ((buf[i / 8] & (1 << (i % 8))) != 0) << j;

    curbit += code_size;

    return ret;
}

static int
LWZReadByte(GifContext_t *ctx, buffer_t *src, int flag, int input_code_size)
{
    int code, incode;
    register int i;
    static int fresh = FALSE;
    static int code_size, set_code_size;
    static int max_code, max_code_size;
    static int firstcode, oldcode;
    static int clear_code, end_code;
    static int table[2][(1 << MAX_LWZ_BITS)];
    static int stack[(1 << (MAX_LWZ_BITS)) * 2], *sp;

    if (flag) {
	set_code_size = input_code_size;
	code_size = set_code_size + 1;
	clear_code = 1 << set_code_size;
	end_code = clear_code + 1;
	max_code_size = 2 * clear_code;
	max_code = clear_code + 2;

	GetCode(ctx, src, 0, TRUE);

	fresh = TRUE;

	for (i = 0; i < clear_code; ++i) {
	    table[0][i] = 0;
	    table[1][i] = i;
	}
	for (; i < (1 << MAX_LWZ_BITS); ++i)
	    table[0][i] = table[1][0] = 0;

	sp = stack;

	return 0;
    } else if (fresh) {
	fresh = FALSE;
	do {
	    firstcode = oldcode = GetCode(ctx, src, code_size, FALSE);
	} while (firstcode == clear_code);
	return firstcode;
    }
    if (sp > stack)
	return *--sp;

    while ((code = GetCode(ctx, src, code_size, FALSE)) >= 0) {
	if (code == clear_code) {
	    for (i = 0; i < clear_code; ++i) {
		table[0][i] = 0;
		table[1][i] = i;
	    }
	    for (; i < (1 << MAX_LWZ_BITS); ++i)
		table[0][i] = table[1][i] = 0;
	    code_size = set_code_size + 1;
	    max_code_size = 2 * clear_code;
	    max_code = clear_code + 2;
	    sp = stack;
	    firstcode = oldcode = GetCode(ctx, src, code_size, FALSE);
	    return firstcode;
	} else if (code == end_code) {
	    int count;
	    unsigned char buf[260];

	    if (ctx->ZeroDataBlock)
		return -2;

	    while ((count = GetDataBlock(ctx, src, buf)) > 0);

	    if (count != 0) {
		/*
		 * EPRINTF("missing EOD in data stream (common occurence)");
		 */
	    }
	    return -2;
	}
	incode = code;

	if (code >= max_code) {
	    *sp++ = firstcode;
	    code = oldcode;
	}
	while (code >= clear_code) {
	    *sp++ = table[1][code];
	    if (code == table[0][code])
		EPRINTF("GdDecodeGIF: circular table entry\n");
	    code = table[0][code];
	}

	*sp++ = firstcode = table[1][code];

	if ((code = max_code) < (1 << MAX_LWZ_BITS)) {
	    table[0][code] = oldcode;
	    table[1][code] = firstcode;
	    ++max_code;
	    if ((max_code >= max_code_size) &&
		(max_code_size < (1 << MAX_LWZ_BITS))) {
		max_code_size *= 2;
		++code_size;
	    }
	}
	oldcode = incode;

	if (sp > stack)
	    return *--sp;
    }
    return code;
}

static int
ReadImage(GifContext_t *ctx, buffer_t* src, const AM_IMG_DecodePara_t *para,
	  AM_OSD_Surface_t **image, int len, int height, int cmapSize,
	  unsigned char cmap[3][MAXCOLORMAPSIZE],
	  int gray, int interlace, int ignore)
{
    AM_OSD_Surface_t *s = NULL;
    unsigned char c;
    int i, v, flags;
    int xpos = 0, ypos = 0, pass = 0;
    int pitch, bpp, bytesperpixel, palsize;

    /*
     *	Initialize the compression routines
     */
    if (!ReadOK(src, &c, 1)) {
	EPRINTF("GdDecodeGIF: EOF on image data\n");
	return 0;
    }
    if (LWZReadByte(ctx, src, TRUE, c) < 0) {
	EPRINTF("GdDecodeGIF: error reading image\n");
	return 0;
    }

    /*
     *	If this is an "uninteresting picture" ignore it.
     */
    if (ignore) {
	while (LWZReadByte(ctx, src, FALSE, c) >= 0);
	return 0;
    }
    /*image = ImageNewCmap(len, height, cmapSize);*/
    bpp = 8;
    GdComputeImagePitch(8, len, &pitch, &bytesperpixel);
    palsize = cmapSize;
    flags = para?(para->surface_flags&(AM_OSD_SURFACE_FL_HW|AM_OSD_SURFACE_FL_OPENGL)):0;
    flags |= AM_OSD_SURFACE_FL_PRIV_FMT;
    if(AM_OSD_CreateSurface(AM_OSD_FMT_PALETTE_256, len, height, flags, &s)!=AM_SUCCESS)
    	return 0;

    for (i = 0; i < cmapSize; i++) {
	/*ImageSetCmap(image, i, cmap[CM_RED][i],
		     cmap[CM_GREEN][i], cmap[CM_BLUE][i]);*/
	s->format->palette.colors[i].r = cmap[CM_RED][i];
	s->format->palette.colors[i].g = cmap[CM_GREEN][i];
	s->format->palette.colors[i].b = cmap[CM_BLUE][i];
	s->format->palette.colors[i].a = 0xFF;
    }

    while ((v = LWZReadByte(ctx, src, FALSE, c)) >= 0) {
	s->buffer[ypos * s->pitch + xpos] = v;

	++xpos;
	if (xpos == len) {
	    xpos = 0;
	    if (interlace) {
		switch (pass) {
		case 0:
		case 1:
		    ypos += 8;
		    break;
		case 2:
		    ypos += 4;
		    break;
		case 3:
		    ypos += 2;
		    break;
		}

		if (ypos >= height) {
		    ++pass;
		    switch (pass) {
		    case 1:
			ypos = 4;
			break;
		    case 2:
			ypos = 2;
			break;
		    case 3:
			ypos = 1;
			break;
		    default:
			goto fini;
		    }
		}
	    } else {
		++ypos;
	    }
	}
	if (ypos >= height)
	    break;
    }

fini:
    *image = s;
    return 1;
}

