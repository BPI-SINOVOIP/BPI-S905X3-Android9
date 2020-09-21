/** @file RGBPicture.c
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


#define LOG_TAG "ImagePlayerService"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <errno.h>
#include "utils/Log.h"

#include "RGBPicture.h"

#if 0
/*---------------------------------------------------------------
* FUNCTION NAME: RGB565toRGB888
* DESCRIPTION:
*               RGB565 to RGB888
* ARGUMENTS:
*               const char* src:src data
*               char* dst;dest data
*               size_t pixel:data length
* Return:
*               -1:fail 0:success
* Note:
*
*---------------------------------------------------------------*/
int RGB565toRGB888(const char* src, char* dst, size_t pixel) {
    struct rgb565  *from;
    struct rgb888  *to;

    from = (struct rgb565 *) src;
    to = (struct rgb888 *) dst;

    int i = 0;

    /* traverse pixel of the row */
    while (i++ < (int)pixel) {
        to->r = from->r;
        to->g = from->g;
        to->b = from->b;
        /* scale */
        to->r <<= 3;
        to->g <<= 2;
        to->b <<= 3;

        to++;
        from++;
    }

    return 0;
}

/*---------------------------------------------------------------
* FUNCTION NAME: ARGB8888_to_RGB888
* DESCRIPTION:
*               RGB8888 to RGB888
* ARGUMENTS:
*               const char* src:src data
*               char* dst;dest data
*               size_t pixel:data length
* Return:
*               -1:fail 0:success
* Note:
*
*---------------------------------------------------------------*/
int ARGB8888_to_RGB888(const char* src, char* dst, size_t pixel) {
    int i, j;

    //bitmap888 need BGR
    //RGBA -> BGR
    for (i = 0; i < (int)pixel; i++) {
        for (j = 0; j < 3; j++) {
            //dst[3*i+j] = src[4*i+j];
            //dst[3*i+j] = (((src[4*i+j]*src[4*i+3])>>8)&0xff);
            //dst[3*i+j] = (((src[4*i+ 2 - j]*src[4*i+3])>>8)&0xff);
            dst[3 * i + j] = src[4 * i + 2 - j];
        }
    }

    return 0;
}

/*---------------------------------------------------------------
* FUNCTION NAME: ARGB8888_to_RGB565
* DESCRIPTION:
*               RGB8888 to RGB565
* ARGUMENTS:
*               const char* src:src data
*               char* dst;dest data
*               size_t pixel:data length
* Return:
*               -1:fail 0:success
* Note:
*
*---------------------------------------------------------------*/
int ARGB8888_to_RGB565(const char* src, char* dst, size_t pixel) {
    int i;

    //BGRA -> RGB
    for (i = 0; i < (int)pixel; i++) {
#if 1
        int id1 = i << 2;
        int id2 = i << 1;

        int R = src[id1 + 0];
        int G = src[id1 + 1];
        int B = src[id1 + 2];

        dst[id2 + 1] = ((R & 0xF8) | ((G >> 5) & 0x07));
        dst[id2] = (((G << 3) & 0xE0) | ((B >> 3) & 0x1F));
#else

        int R = src[4 * i + 0];
        int G = src[4 * i + 1];
        int B = src[4 * i + 2];

        dst[2 * i + 1] = ((R & 0xF8) | ((G >> 5) & 0x07));
        dst[2 * i] = (((G << 3) & 0xE0) | ((B >> 3) & 0x1F));
#endif
    }

    return 0;
}

/*---------------------------------------------------------------
* FUNCTION NAME: ARGB2bmp
* DESCRIPTION:
*               ARGB raw data to bmp file
* ARGUMENTS:
*               void
* Return:
*               <0:fail 0:success
* Note:
*
*---------------------------------------------------------------*/
int ARGB2bmp(char *buf, int width, int height) {
    char *rgb_matrix;
    int i, ret;
    int len;

    len = width * height * 3;
    rgb_matrix = (char *)malloc(len);

    if (!rgb_matrix) {
        ALOGE("RGB2bmp, malloc rgb_matrix memory error");
        return -2;
    }

    ARGB8888_to_RGB888(buf, rgb_matrix, width * height);

    unsigned char *p_bmp_data = NULL;
    int file_header_len, info_header_len;
    BmpFileHeader_t bmp_file_header;
    BmpInfoHeader_t bmp_info_header;

    file_header_len = sizeof( BmpFileHeader_t );
    info_header_len = sizeof( BmpInfoHeader_t );

    memset( &bmp_file_header, 0, file_header_len );
    bmp_file_header.bf_type = 'MB';
    bmp_file_header.bf_size = file_header_len + len + info_header_len;
    bmp_file_header.bf_offbits = file_header_len + info_header_len;

    memset( &bmp_info_header, 0, info_header_len );
    bmp_info_header.bi_size = info_header_len;
    bmp_info_header.bi_width = width;
    bmp_info_header.bi_height = -height;
    bmp_info_header.bi_planes = 1;
    bmp_info_header.bi_bitcount = 24; //RGB 888

    p_bmp_data = (unsigned char *)malloc(bmp_file_header.bf_size);

    if ( NULL == p_bmp_data ) {
        ALOGE("RGB2bmp, [error]memory not enough:NULL == p_bmp_data\n");
        return -1;
    }

    memset(p_bmp_data, 0, bmp_file_header.bf_size);
    memcpy(p_bmp_data, &bmp_file_header, file_header_len);//copy bmp file header
    memcpy(p_bmp_data + file_header_len, &bmp_info_header,
           info_header_len);//copy bmp info header
    memcpy(p_bmp_data + file_header_len + info_header_len, rgb_matrix,
           len);//copy bmp data

    {
        FILE *fp;
        fp = fopen("/sdcard/fb888.bmp", "wb");

        if ( NULL == fp ) {
            ALOGE("fb, open file error\n");
            return -2;
        }

        fwrite(p_bmp_data, bmp_file_header.bf_size, 1, fp);
        fclose(fp);
        ALOGI("fb, write file len = %d\n", (int)bmp_file_header.bf_size);
    }

    if ( NULL != p_bmp_data ) {
        free(p_bmp_data);
        p_bmp_data = NULL;
    }

    free(rgb_matrix);
    return 0;
}

/*---------------------------------------------------------------
* FUNCTION NAME: Bitmap2bmp
* DESCRIPTION:
*
* ARGUMENTS:
*               unsigned long *buf_len:buffer len
*               unsigned int pic_w:request picture width
*               unsigned int pic_h:request picture height
* Return:
*               char *:unsigned char pointer
* Note:
*
*---------------------------------------------------------------*/
void Bitmap2bmp(char *buf, unsigned int pic_w, unsigned int pic_h,
                int format_888) {
    char *rgbMatrix = NULL;
    unsigned char *p_bmp_data = NULL;

    int file_header_len, info_header_len;
    BmpFileHeader_t bmp_file_header;
    BmpInfoHeader_t bmp_info_header;

    if (1 == format_888) {
        int len = pic_w * pic_h * 3;
        rgbMatrix = (char *)malloc(len);

        if (!rgbMatrix) {
            ALOGE("malloc rgbMatrix memory error");
            return;
        }

        ARGB8888_to_RGB888(buf, rgbMatrix, pic_w * pic_h);

        file_header_len = sizeof( BmpFileHeader_t );
        info_header_len = sizeof( BmpInfoHeader_t );

        memset( &bmp_file_header, 0, file_header_len );
        bmp_file_header.bf_type = 'MB';
        bmp_file_header.bf_size = file_header_len + len + info_header_len;
        bmp_file_header.bf_offbits = file_header_len + info_header_len;

        memset( &bmp_info_header, 0, info_header_len );
        bmp_info_header.bi_size = info_header_len;
        bmp_info_header.bi_width = pic_w;
        bmp_info_header.bi_height = -pic_h;
        bmp_info_header.bi_planes = 1;
        bmp_info_header.bi_bitcount = 24; //RGB 888

        p_bmp_data = (unsigned char *)malloc(bmp_file_header.bf_size);

        if ( NULL == p_bmp_data ) {
            ALOGE("[error]memory not enough:NULL == p_bmp_data");
            goto MEM_ERR;
        }

        memset(p_bmp_data, 0, bmp_file_header.bf_size);
        memcpy(p_bmp_data, &bmp_file_header, file_header_len);//copy bmp file header
        memcpy(p_bmp_data + file_header_len, &bmp_info_header,
               info_header_len);//copy bmp info header
        memcpy(p_bmp_data + file_header_len + info_header_len, rgbMatrix,
               len);//copy bmp data
    } else {
        int len = pic_w * pic_h * 2;
        rgbMatrix = (char *)malloc(len);

        if (!rgbMatrix) {
            ALOGE("malloc rgbMatrix memory error");
            return;
        }

        ARGB8888_to_RGB565(buf, rgbMatrix, pic_w * pic_h);

        //unsigned char *p_bmp_data = NULL;
        int color_table_len;
        BmpColorTable_t bmp_colors[3];

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

        file_header_len = sizeof( BmpFileHeader_t );
        info_header_len = sizeof( BmpInfoHeader_t );
        color_table_len = sizeof(bmp_colors);

        memset( &bmp_file_header, 0, file_header_len );
        bmp_file_header.bf_type = 'MB';
        bmp_file_header.bf_size = file_header_len + len + info_header_len +
                                  color_table_len; //RGB 565
        bmp_file_header.bf_offbits = file_header_len + info_header_len +
                                     color_table_len;

        memset( &bmp_info_header, 0, info_header_len );
        bmp_info_header.bi_size = info_header_len;
        bmp_info_header.bi_width = pic_w;
        bmp_info_header.bi_height = -pic_h;
        bmp_info_header.bi_planes = 1;
        bmp_info_header.bi_bitcount = 16; //RGB 565

        //use to RGB565
        bmp_info_header.bi_compression = 3;//BI_BITFIELDS;

        p_bmp_data = (unsigned char *)malloc(bmp_file_header.bf_size);

        if ( NULL == p_bmp_data ) {
            ALOGE("[error]memory not enough:NULL == p_bmp_data");
            goto MEM_ERR;
        }

        memset(p_bmp_data, 0, bmp_file_header.bf_size);
        memcpy(p_bmp_data, &bmp_file_header, file_header_len);//copy bmp file header
        memcpy(p_bmp_data + file_header_len, &bmp_info_header,
               info_header_len);//copy bmp info header
        memcpy(p_bmp_data + file_header_len + info_header_len, bmp_colors,
               color_table_len);//copy color table
        memcpy(p_bmp_data + file_header_len + info_header_len + color_table_len,
               rgbMatrix, len);//copy bmp data
    }

    {
        FILE *fp;

        if (1 == format_888)
            fp = fopen("/sdcard/bitmap888.bmp", "wb");
        else
            fp = fopen("/sdcard/bitmap565.bmp", "wb");

        if ( NULL == fp ) {
            ALOGE("open file error\n");
        }

        fwrite(p_bmp_data, bmp_file_header.bf_size, 1, fp);
        fclose(fp);
    }

    free(rgbMatrix);

    if ( NULL != p_bmp_data ) {
        free(p_bmp_data);
        p_bmp_data = NULL;
    }

MEM_ERR:

    if ( NULL != rgbMatrix) {
        free(rgbMatrix);
        rgbMatrix = NULL;
    }
}
#endif

/*---------------------------------------------------------------
* FUNCTION NAME: RGBA8888_to_RGB888
* DESCRIPTION:
*               RGBA8888 to RGB888
* ARGUMENTS:
*               const char* src:src data
*               char* dst;dest data
*               size_t pixel:data length
* Return:
*               -1:fail 0:success
* Note:
*
*---------------------------------------------------------------*/
int RGBA8888_to_RGB888(const char* src, char* dst, size_t pixel) {
    int i, j;

    //bitmap888 need BGR
    //RGBA -> BGR
    for (i = 0; i < (int)pixel; i++) {
        for (j = 0; j < 3; j++) {
            dst[3 * i + j] = src[4 * i + 2 - j];

            //ARGB -> BGR
            //dst[3*i+j] = src[4*i + 3 - j];
        }
    }

    return 0;
}

/*---------------------------------------------------------------
* FUNCTION NAME: RGBA2bmp
* DESCRIPTION:
*               RGBA raw data to bmp file
* ARGUMENTS:
*               char *buf: RGB data
*               int width: picture width
*               int height: picture height
*               char* filePath: save file path
* Return:
*               <0:fail 0:success
* Note:
*
*---------------------------------------------------------------*/
int RGBA2bmp(char *buf, int width, int height, char* filePath) {
    FILE *fp;
    char *rgb_matrix = NULL;
    unsigned char *p_bmp_data = NULL;
    int ret = 0;
    int len;

    if (NULL == buf) {
        ALOGE("RGBA2bmp, [error]RGB raw data is NULL");
        ret = -1;
        goto _ERR;
    }

    len = width * height * 3;
    rgb_matrix = (char *)malloc(len);

    if (NULL == rgb_matrix) {
        ALOGE("RGBA2bmp, [error]memory not enough:NULL == rgb_matrix");
        ret = -1;
        goto _ERR;
    }

    RGBA8888_to_RGB888(buf, rgb_matrix, width * height);

    int file_header_len, info_header_len;
    BmpFileHeader_t bmp_file_header;
    BmpInfoHeader_t bmp_info_header;

    file_header_len = sizeof( BmpFileHeader_t );
    info_header_len = sizeof( BmpInfoHeader_t );

    memset( &bmp_file_header, 0, file_header_len );
    bmp_file_header.bf_type = 'MB';
    bmp_file_header.bf_size = file_header_len + len + info_header_len;
    bmp_file_header.bf_offbits = file_header_len + info_header_len;

    memset( &bmp_info_header, 0, info_header_len );
    bmp_info_header.bi_size = info_header_len;
    bmp_info_header.bi_width = width;
    bmp_info_header.bi_height = -height;
    bmp_info_header.bi_planes = 1;
    bmp_info_header.bi_bitcount = 24; //RGB 888

    p_bmp_data = (unsigned char *)malloc(bmp_file_header.bf_size);

    if ( NULL == p_bmp_data ) {
        ALOGE("RGBA2bmp, [error]memory not enough:NULL == p_bmp_data\n");
        ret = -1;
        goto _ERR;
    }

    memset(p_bmp_data, 0, bmp_file_header.bf_size);
    memcpy(p_bmp_data, &bmp_file_header, file_header_len);//copy bmp file header
    memcpy(p_bmp_data + file_header_len, &bmp_info_header,
           info_header_len);//copy bmp info header
    memcpy(p_bmp_data + file_header_len + info_header_len, rgb_matrix,
           len);//copy bmp data

    fp = fopen(filePath, "wb");

    if ( NULL == fp ) {
        ALOGE("RGBA2bmp, [error]open file:%s failure error: '%s' (%d)\n", filePath,
              strerror(errno), errno);
        ret = -2;
        goto _ERR;
    }

    fwrite(p_bmp_data, bmp_file_header.bf_size, 1, fp);
    fclose(fp);
    ALOGI("RGBA2bmp, save file success, file len = %d\n",
          (int)bmp_file_header.bf_size);

_ERR:

    if ( NULL != rgb_matrix )
        free(rgb_matrix);

    if ( NULL != p_bmp_data )
        free(p_bmp_data);

    return ret;
}

