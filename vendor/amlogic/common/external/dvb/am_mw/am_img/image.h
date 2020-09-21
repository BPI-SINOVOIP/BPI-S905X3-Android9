/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/
#ifndef __IMAGE_H
#define __IMAGE_H

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <endian.h>
#include <am_osd.h>
#include <am_mem.h>
#include <am_img.h>

#if __BYTE_ORDER == __BIG_ENDIAN
# define wswap(x)       ((((x) << 8) & 0xff00) | (((x) >> 8) & 0x00ff))
# define dwswap(x)      ((((x) << 24) & 0xff000000L) | \
                         (((x) <<  8) & 0x00ff0000L) | \
                         (((x) >>  8) & 0x0000ff00L) | \
                         (((x) >> 24) & 0x000000ffL) )
#else
# define wswap(x)      (x)
# define dwswap(x)     (x)
#endif

/**
 * Read little endian format 32-bit number from buffer, possibly not
 * aligned, and convert to the host CPU format.
 */
#define dwread(addr)    ((((unsigned char *)(addr))[0] | \
                         (((unsigned char *)(addr))[1] << 8) | \
                         (((unsigned char *)(addr))[2] << 16) | \
                         (((unsigned char *)(addr))[3] << 24)))

#define EPRINTF         printf

typedef unsigned char MWUCHAR;

/* Buffered input functions to replace stdio functions*/
typedef struct {  /* structure for reading images from buffer   */
	unsigned char *start;   /* The pointer to the beginning of the buffer */
	unsigned long offset;   /* The current offset within the buffer       */
	unsigned long size;     /* The total size of the buffer               */
} buffer_t;

extern int GdImageBufferRead(buffer_t *src, void* buf, int size);

extern int GdImageBufferGetChar(buffer_t *src);

extern char* GdImageBufferGetString(buffer_t *src, void *buf, int size);

extern int GdImageBufferSeekTo(buffer_t *src, int off);

extern int GdComputeImagePitch(int bpp, int width, int *pitch, int *bytesperpixel);

extern AM_OSD_PixelFormatType_t GdGetPixelFormatType(int bpp, int bgr_mode);

#endif

