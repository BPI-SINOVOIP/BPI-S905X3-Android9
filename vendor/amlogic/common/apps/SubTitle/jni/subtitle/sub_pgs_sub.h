/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: h file
*/
#ifndef _PGS_SUB_H
#define _PGS_SUB_H

typedef struct
{
    int start_time;
    int end_time;
    int x;
    int y;
    int width;
    int height;
    char fpsCode;
    int objectCount;

    char state;
    char palette_update_flag;
    char palette_Id_ref;
    char number;
    char item_flag;     //cropped |=0x80, forced |= 0x40

    /**/ int window_width_offset;
    int window_height_offset;
    int window_width;
    int window_height;
    /**/ int image_width;
    int image_height;
    /**/ int palette[0x100];
    unsigned char *rle_buf;
    int rle_buf_size;
    int rle_rd_off;
    /**/
} pgs_info_t;

typedef struct
{
    off_t pos;
    unsigned char *pgs_data_buf;
    int pgs_data_buf_size;
    unsigned char pgs_time_head_buf[13];
} pgs_buf_stru_t;

typedef struct
{
    int start_time;
    int end_time;
    off_t pos;
} pgs_item_t;

/* PGS subtitle API */
typedef struct
{
    int x;
    int y;
    int width;
    int height;

    int window_width_offset;
    int window_height_offset;
    int window_width;
    int window_height;

    int image_width;
    int image_height;

    int *palette;
    unsigned char *rle_buf;
    unsigned char *result_buf;
    int rle_buf_size;
    int render_height;
    int pts;
} PGS_subtitle_showdata;

typedef struct
{
    /* Add more members here */
    pgs_info_t *pgs_info;
    char *cur_idx_url;
    pgs_buf_stru_t pgs_buf_stru;

    int fd;
    off_t file_pos;
    pgs_item_t *pgs_item_table;
    int pgs_item_count;
    int pgs_display_index;

    /**/ PGS_subtitle_showdata showdata;
    /*end */
} subtitlepgs_t;

typedef struct draw_result
{
    unsigned char *buf;
    short x;
    short y;
    unsigned short w;
    unsigned short h;
} draw_result_t;
typedef int (*draw_pixel_fun_t)(int x, int y, unsigned pixel, void *arg);
int get_pgs_spu(AML_SPUVAR *spu, int read_handle);
int init_pgs_subtitle();
int close_pgs_subtitle();

#endif
