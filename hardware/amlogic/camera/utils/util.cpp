/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Description:
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include "util.h"

#ifdef AMLOGIC_USB_CAMERA_SUPPORT
#define swap_cbcr
static void convert_rgb16_to_nv21(uint8_t *rgb, uint8_t *yuv, int width, int height)
{
	int iy =0, iuv = 0;
    uint8_t* buf_y = yuv;
    uint8_t* buf_uv = buf_y + width * height;
    uint16_t* buf_rgb = (uint16_t *)rgb;
	int h,w,val_rgb,val_r,val_g,val_b;
	int y,u,v;
    for (h = 0; h < height; h++) {
        for (w = 0; w < width; w++) {
            val_rgb = buf_rgb[h * width + w];
            val_r = ((val_rgb & (0x1f << 11)) >> 11)<<3;
            val_g = ((val_rgb & (0x3f << 5)) >> 5)<<2;
            val_b = ((val_rgb & (0x1f << 0)) >> 0)<<3;
            y = 0.30078 * val_r + 0.5859 * val_g + 0.11328 * val_b;
            if (y > 255) {
                y = 255;
            } else if (y < 0) {
                y = 0;
            }
            buf_y[iy++] = y;
            if (0 == h % 2 && 0 == w % 2) {
                u = -0.11328 * val_r - 0.33984 * val_g + 0.51179 * val_b + 128;
                if (u > 255) {
                    u = 255;
                } else if (u < 0) {
                    u = 0;
                }
                v = 0.51179 * val_r - 0.429688 * val_g - 0.08203 * val_b  + 128;
                if (v > 255) {
                    v = 255;
                } else if (v < 0) {
                    v = 0;
                }
#ifdef swap_cbcr
                buf_uv[iuv++] = v;
                buf_uv[iuv++] = u;
#else
                buf_uv[iuv++] = u;
                buf_uv[iuv++] = v;
#endif
            }
        }
}
}

static inline void yuv_to_rgb16(unsigned char y,unsigned char u,unsigned char v,unsigned char *rgb)
{
    register int r,g,b;
    int rgb16;

    r = (1192 * (y - 16) + 1634 * (v - 128) ) >> 10;
    g = (1192 * (y - 16) - 833 * (v - 128) - 400 * (u -128) ) >> 10;
    b = (1192 * (y - 16) + 2066 * (u - 128) ) >> 10;

    r = r > 255 ? 255 : r < 0 ? 0 : r;
    g = g > 255 ? 255 : g < 0 ? 0 : g;
    b = b > 255 ? 255 : b < 0 ? 0 : b;

    rgb16 = (int)(((r >> 3)<<11) | ((g >> 2) << 5)| ((b >> 3) << 0));

    *rgb = (unsigned char)(rgb16 & 0xFF);
    rgb++;
    *rgb = (unsigned char)((rgb16 & 0xFF00) >> 8);

}

void yuyv422_to_rgb16(unsigned char *from, unsigned char *to, int size)
{
    int x,y,z=0;

    for (y = 0; y < size; y+=4) {
        unsigned char Y1, Y2, U, V;

        Y1 = from[y + 0];
        U = from[y + 1];
        Y2 = from[y + 2];
        V = from[y + 3];

        yuv_to_rgb16(Y1, U, V, &to[y]);
        yuv_to_rgb16(Y2, U, V, &to[y + 2]);
    }
}

void yuyv422_to_rgb16(unsigned char *from, unsigned char *to, int width, int height)
{
    yuyv422_to_rgb16(from,to,(width * height) * 2);
}

void yuyv422_to_nv21(unsigned char *bufsrc, unsigned char *bufdest, int width, int height)
{
    unsigned char *ptrsrcy1, *ptrsrcy2;
    unsigned char *ptrsrcy3, *ptrsrcy4;
    unsigned char *ptrsrccb1, *ptrsrccb2;
    unsigned char *ptrsrccb3, *ptrsrccb4;
    unsigned char *ptrsrccr1, *ptrsrccr2;
    unsigned char *ptrsrccr3, *ptrsrccr4;
    int srcystride, srcccstride;

    ptrsrcy1  = bufsrc ;
    ptrsrcy2  = bufsrc + (width<<1) ;
    ptrsrcy3  = bufsrc + (width<<1)*2 ;
    ptrsrcy4  = bufsrc + (width<<1)*3 ;

    ptrsrccb1 = bufsrc + 1;
    ptrsrccb2 = bufsrc + (width<<1) + 1;
    ptrsrccb3 = bufsrc + (width<<1)*2 + 1;
    ptrsrccb4 = bufsrc + (width<<1)*3 + 1;

    ptrsrccr1 = bufsrc + 3;
    ptrsrccr2 = bufsrc + (width<<1) + 3;
    ptrsrccr3 = bufsrc + (width<<1)*2 + 3;
    ptrsrccr4 = bufsrc + (width<<1)*3 + 3;

    srcystride  = (width<<1)*3;
    srcccstride = (width<<1)*3;

    unsigned char *ptrdesty1, *ptrdesty2;
    unsigned char *ptrdesty3, *ptrdesty4;
    unsigned char *ptrdestcb1, *ptrdestcb2;
    unsigned char *ptrdestcr1, *ptrdestcr2;
    int destystride, destccstride;

    ptrdesty1 = bufdest;
    ptrdesty2 = bufdest + width;
    ptrdesty3 = bufdest + width*2;
    ptrdesty4 = bufdest + width*3;

    ptrdestcb1 = bufdest + width*height;
    ptrdestcb2 = bufdest + width*height + width;

    ptrdestcr1 = bufdest + width*height + 1;
    ptrdestcr2 = bufdest + width*height + width + 1;

    destystride  = (width)*3;
    destccstride = width;

    int i, j;

    for(j=0; j<(height/4); j++)
    {
        for(i=0;i<(width/2);i++)
        {
            (*ptrdesty1++) = (*ptrsrcy1);
            (*ptrdesty2++) = (*ptrsrcy2);
            (*ptrdesty3++) = (*ptrsrcy3);
            (*ptrdesty4++) = (*ptrsrcy4);

            ptrsrcy1 += 2;
            ptrsrcy2 += 2;
            ptrsrcy3 += 2;
            ptrsrcy4 += 2;

            (*ptrdesty1++) = (*ptrsrcy1);
            (*ptrdesty2++) = (*ptrsrcy2);
            (*ptrdesty3++) = (*ptrsrcy3);
            (*ptrdesty4++) = (*ptrsrcy4);

            ptrsrcy1 += 2;
            ptrsrcy2 += 2;
            ptrsrcy3 += 2;
            ptrsrcy4 += 2;

#if 0
            (*ptrdestcb1) = (*ptrsrccb1);
            (*ptrdestcb2) = (*ptrsrccb3);
#else 
            (*ptrdestcb1) = (*ptrsrccr1);
            (*ptrdestcb2) = (*ptrsrccr3);
#endif

#if 0
            (*ptrdestcr1) = (*ptrsrccr1);
            (*ptrdestcr2) = (*ptrsrccr3);
#else 
            (*ptrdestcr1) = (*ptrsrccb1);
            (*ptrdestcr2) = (*ptrsrccb3);
#endif
            
            ptrdestcb1 += 2;
            ptrdestcb2 += 2;
            ptrdestcr1 += 2;
            ptrdestcr2 += 2;

            ptrsrccb1 += 4;
            ptrsrccb3 += 4;
            ptrsrccr1 += 4;
            ptrsrccr3 += 4;

        }


        /* Update src pointers */
        ptrsrcy1  += srcystride;
        ptrsrcy2  += srcystride;
        ptrsrcy3  += srcystride;
        ptrsrcy4  += srcystride;

        ptrsrccb1 += srcccstride;
        ptrsrccb3 += srcccstride;

        ptrsrccr1 += srcccstride;
        ptrsrccr3 += srcccstride;


        /* Update dest pointers */
        ptrdesty1 += destystride;
        ptrdesty2 += destystride;
        ptrdesty3 += destystride;
        ptrdesty4 += destystride;

        ptrdestcb1 += destccstride;
        ptrdestcb2 += destccstride;

        ptrdestcr1 += destccstride;
        ptrdestcr2 += destccstride;

    }
}

static inline void yuv_to_rgb24(unsigned char y,unsigned char u,unsigned char v,unsigned char *rgb)
{
    register int r,g,b;
    int rgb24;

    r = (1192 * (y - 16) + 1634 * (v - 128) ) >> 10;
    g = (1192 * (y - 16) - 833 * (v - 128) - 400 * (u -128) ) >> 10;
    b = (1192 * (y - 16) + 2066 * (u - 128) ) >> 10;

    r = r > 255 ? 255 : r < 0 ? 0 : r;
    g = g > 255 ? 255 : g < 0 ? 0 : g;
    b = b > 255 ? 255 : b < 0 ? 0 : b;

    rgb24 = (int)((r <<16) | (g  << 8)| b);

    *rgb = (unsigned char)r;
    rgb++;
    *rgb = (unsigned char)g;
    rgb++;
    *rgb = (unsigned char)b;
}

void yuyv422_to_rgb24(unsigned char *buf, unsigned char *rgb, int width, int height)
{
    int x,y,z=0;
    int blocks;

    blocks = (width * height) * 2;

    for (y = 0,z=0; y < blocks; y+=4,z+=6) {
        unsigned char Y1, Y2, U, V;

        Y1 = buf[y + 0];
        U = buf[y + 1];
        Y2 = buf[y + 2];
        V = buf[y + 3];

        yuv_to_rgb24(Y1, U, V, &rgb[z]);
        yuv_to_rgb24(Y2, U, V, &rgb[z + 3]);

    }
}

void convert_rgb24_to_rgb16(uint8_t *src, uint8_t *dst, int width, int height)
{
	int src_len = width*height*3;
    int i = 0;
	int j = 0;
    
    for (i = 0; i < src_len; i += 3)
    {
		dst[j] = (src[i]&0x1f) | (src[i+1]>>5);
		dst[j+1] = ((src[i+1]>>2)<<5) | (src[i+2]>>3);
        j += 2;
    }
}
void yuyv_to_yv12(unsigned char *src, unsigned char *dst, int width, int height)
{
	//width should be an even number.
	//uv ALIGN 32.
	int i,j,stride,c_stride,c_size,y_size,cb_offset,cr_offset;
	unsigned char *dst_copy,*src_copy;

	dst_copy = dst;
	src_copy = src;

	y_size = width*height;
	c_stride = ALIGN(width/2, 16);
	c_size = c_stride * height/2;
	cr_offset = y_size;
	cb_offset = y_size+c_size;

	for(i=0;i< y_size;i++){
		*dst++ = *src;
		src += 2;
	}

	dst = dst_copy;
	src = src_copy;

	for(i=0;i<height;i+=2){
		for(j=1;j<width*2;j+=4){//one line has 2*width bytes for yuyv.
			//ceil(u1+u2)/2
			*(dst+cr_offset+j/4)= (*(src+j+2) + *(src+j+2+width*2) + 1)/2;
			*(dst+cb_offset+j/4)= (*(src+j) + *(src+j+width*2) + 1)/2;
		}
		dst += c_stride;
		src += width*4;
	}
}
#endif
void rgb24_memcpy(unsigned char *dst, unsigned char *src, int width, int height)
{
        int stride = (width + 31) & ( ~31);
        int w, h;
        for (h=0; h<height; h++)
        {
                memcpy( dst, src, width*3);
                dst += width*3;
                src += stride*3;
        }
}

void nv21_memcpy_align32(unsigned char *dst, unsigned char *src, int width, int height)
{
        int stride = (width + 31) & ( ~31);
        int w, h;
        for (h=0; h<height*3/2; h++)
        {
                memcpy( dst, src, width);
                dst += width;
                src += stride;
        }
}

void yv12_memcpy_align32(unsigned char *dst, unsigned char *src, int width, int height)
{
        int new_width = (width + 63) & ( ~63);
        int stride;
        int w, h;
        for (h=0; h<height; h++)
        {
                memcpy( dst, src, width);
                dst += width;
                src += new_width;
        }

	stride = ALIGN(width/2, 16);
        for (h=0; h<height; h++)
        {
                memcpy( dst, src, width/2);
                dst += stride;
                src += new_width/2;
        }
}

void yv12_adjust_memcpy(unsigned char *dst, unsigned char *src, int width, int height)
{
	//width should be an even number.
	int i,stride;
	memcpy( dst, src, width*height);
	src += width*height;
	dst += width*height;

	stride = ALIGN(width/2, 16);
	for(i =0; i< height; i++)
	{
		memcpy(dst,src, stride);
		src+=width/2;
		dst+=stride;
	}
}

void nv21_memcpy_canvas1080(unsigned char *dst, unsigned char *src, int width, int height)
{
        int h;
        for (h=0; h<height; h++){
                memcpy( dst, src, width);
                dst += width;
                src += width;
        }
        src+=width*8;
        for (h=0; h<height/2; h++){
                memcpy( dst, src, width);
                dst += width;
                src += width;
        }
}

void yv12_memcpy_canvas1080(unsigned char *dst, unsigned char *src, int width, int height)
{
        int h;
        for (h=0; h<height; h++){
                memcpy( dst, src, width);
                dst += width;
                src += width;
        }
        src+=width*8;
        for (h=0; h<height/2; h++){
                memcpy( dst, src, width/2);
                dst += width/2;
                src += width/2;
        }
        src+=width*2;
        for (h=0; h<height/2; h++){
                memcpy( dst, src, width/2);
                dst += width/2;
                src += width/2;
        }
}
