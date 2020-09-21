/** @file RGBPicture.h
 *  @par Copyright:
 *  - Copyright 2011 Amlogic Inc as unpublished work
 *  All Rights Reserved
 *  - The information contained herein is the confidential property
 *  of Amlogic.  The use, copying, transfer or disclosure of such information
 *  is prohibited except by express written agreement with Amlogic Inc.
 *  @author   Tellen Yu
 *  @version  1.0
 *  @date     2014/04/26
 *  @par function description:
 *  - 1 save rgb data to picture
 *  @warning This class may explode in your face.
 *  @note If you inherit anything from this class, you're doomed.
 */

#ifndef _RGB_PICTURE_H_
#define _RGB_PICTURE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    unsigned int width;
    unsigned int height;
#define FB_FORMAT_RGB565    0
#define FB_FORMAT_ARGB8888  1
    unsigned int format;
    char* data;
} rc_fb_t;

typedef struct rgb888 {
    char r;
    char g;
    char b;
} rc_rgb888_t;

typedef rc_rgb888_t rc_rgb24_t;

typedef struct rgb565 {
    short b: 5;
    short g: 6;
    short r: 5;
} rc_rgb565_t;

#pragma pack(1)
typedef struct {
    unsigned short  bf_type;
    unsigned long   bf_size;
    unsigned short  bf_reserved1;
    unsigned short  bf_reserved2;
    unsigned long   bf_offbits;
} BmpFileHeader_t;

typedef struct {
    unsigned long   bi_size;
    unsigned long   bi_width;
    unsigned long   bi_height;

    unsigned short  bi_planes;
    unsigned short  bi_bitcount;

    unsigned long   bi_compression;

    unsigned long   bi_sizeimage;
    unsigned long   bi_xpelspermeter;
    unsigned long   bi_ypelspermeter;
    unsigned long   bi_clrused;
    unsigned long   bi_clrimportant;
} BmpInfoHeader_t;

/*
    PhotoRGBColorTable_t bmp_colors[3];

    bmp_colors[0].rgb_blue      =   0;
    bmp_colors[0].rgb_green     =   0xF8;
    bmp_colors[0].rgb_red       =   0;
    bmp_colors[0].rgb_reserved  =   0;
    bmp_colors[1].rgb_blue      =   0xE0;
    bmp_colors[1].rgb_green     =   0x07;
    bmp_colors[1].rgb_red       =   0;
    bmp_colors[1].rgb_reserved  =   0;
    bmp_colors[2].rgb_blue      =   0x1F;
    bmp_colors[2].rgb_green     =   0;
    bmp_colors[2].rgb_red       =   0;
    bmp_colors[2].rgb_reserved  =   0;
*/
typedef struct { //color table
    unsigned char rgb_blue;
    unsigned char rgb_green;
    unsigned char rgb_red;
    unsigned char rgb_reserved;
} BmpColorTable_t;
#pragma pack()

int RGBA2bmp(char *buf, int width, int height, char* filePath);

#ifdef __cplusplus
}
#endif

#endif/*_RGB_PICTURE_H_*/
