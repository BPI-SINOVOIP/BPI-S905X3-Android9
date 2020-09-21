/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: c file
*/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <android/log.h>
#include "sub_subtitle.h"
#include "sub_pgs_sub.h"
#include "vob_sub.h"
//#include "sub_control.h"

#include "sub_io.h"

#define  LOG_TAG    "sub_pgs"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
VOB_SPUVAR uVobSPU;

static unsigned SPU_RD_HOLD_SIZE = 0x20;
static int read_pgs_byte = 0;
static draw_pixel_fun_t pgs_draw_pixel_fun_ = NULL;
static subtitlepgs_t subtitle_pgs;
extern int subtitle_status;


int init_pgs_subtitle()
{
    LOGI("init pgs_subtitle info\n");
    memset(&subtitle_pgs, 0x0, sizeof(subtitlepgs_t));
    subtitle_pgs.pgs_info = malloc(sizeof(pgs_info_t));
    if (subtitle_pgs.pgs_info == NULL)
    {
        LOGE("malloc pgs_info failed\n");
        return -1;
    }
    memset(subtitle_pgs.pgs_info, 0x0, sizeof(pgs_info_t));
    return 0;
}

int close_pgs_subtitle()
{
    if (subtitle_pgs.pgs_info) {
        free(subtitle_pgs.pgs_info);
        subtitle_pgs.pgs_info = NULL;
    }
    return 0;
}

static int read_spu_byte(int read_handle, char *byte)
{
    int ret = 0;
    if (uVobSPU.spu_cache_pos > 0)
    {
        *byte = uVobSPU.spu_cache[--uVobSPU.spu_cache_pos];
        ret = 1;
    }
    else
    {
        if (getSize(read_handle) < SPU_RD_HOLD_SIZE)
        {
            LOGI("current pgs sub buffer size %d\n", getSize(read_handle));
            ret = 0;
        }
        else
        {
            getData(read_handle, &(uVobSPU.spu_cache[0]), 8);
            int i = 0;
            for (i = 0; i < 4; i++)
            {
                char value = uVobSPU.spu_cache[i];
                uVobSPU.spu_cache[i] = uVobSPU.spu_cache[7 - i];
                uVobSPU.spu_cache[7 - i] = value;
            }
            read_pgs_byte += 8;
            uVobSPU.spu_cache_pos = 8;
            *byte = uVobSPU.spu_cache[--uVobSPU.spu_cache_pos];
            ret = 1;
        }
    }
    return ret;
}

static int read_spu_buf(int read_handle, char *buf, int len)
{
    int i;
#if 0
    for (i = 0; i < len; i++)
    {
        if (read_spu_byte(read_handle, buf + i) == 0)
            break;
    }
#endif
    getData(read_handle, buf, len);
    read_pgs_byte += len;
    return len;
}

static unsigned char read_time_header(unsigned char **buf_start_ptr,
                                      int *size_i, int *start_time,
                                      int *end_time)
{
    int type;
    unsigned char *buf = *buf_start_ptr;
    int size;
    if (buf[0] == 'P' && buf[1] == 'G')
    {
        *start_time =
            (buf[2] << 24) | (buf[3] << 16) | (buf[4] << 8) | (buf[5]);
        *end_time =
            (buf[6] << 24) | (buf[7] << 16) | (buf[8] << 8) | (buf[9]);
        type = buf[10];
        //LOGI("type is %d\n", type);
        size = (buf[11] << 8) | buf[12];
        //LOGI("size is %d\n", size);
        *buf_start_ptr = buf + 13 + size;
        *size_i = size;
        return type;
    }
    return 0;
}

static void read_subpictureHeader(unsigned char *buf, int size,
                                  pgs_info_t *pgs_info)
{
    pgs_info->width = (buf[0] << 8) | buf[1];
    pgs_info->height = (buf[2] << 8) | buf[3];
    pgs_info->fpsCode = buf[4];
    pgs_info->objectCount = (buf[5] << 8) | buf[6];
    pgs_info->state = buf[7];   //0x80
    pgs_info->palette_update_flag = buf[8];
    pgs_info->palette_Id_ref = buf[9];
    pgs_info->number = buf[0xa];
    pgs_info->item_flag = buf[0xe]; //cropped |=0x80, forced |= 0x40
    pgs_info->x = (buf[0xf] << 8) | buf[0x10];
    pgs_info->y = (buf[0x11] << 8) | buf[0x12];
}

static void read_windowHeader(unsigned char *buf, int size,
                              pgs_info_t *pgs_info)
{
    pgs_info->window_width_offset = (buf[2] << 8) | buf[3];
    pgs_info->window_height_offset = (buf[4] << 8) | buf[5];
    pgs_info->window_width = (buf[6] << 8) | buf[7];
    pgs_info->window_height = (buf[8] << 8) | buf[9];
}

#define SCALEBITS 10
#define ONE_HALF  (1 << (SCALEBITS - 1))
#define FIX(x)    ((int) ((x) * (1<<SCALEBITS) + 0.5))
#define MAX_NEG_CROP 1024
static uint8_t pgs_ff_cropTbl[256 + 2 * MAX_NEG_CROP] = { 0, };

#define cm (pgs_ff_cropTbl + MAX_NEG_CROP)
#define PGS_YUV_TO_RGB1(cb1, cr1)\
    {\
        cb = (cb1) - 128;\
        cr = (cr1) - 128;\
        r_add = FIX(1.40200) * cr + ONE_HALF;\
        g_add = - FIX(0.34414) * cb - FIX(0.71414) * cr + ONE_HALF;\
        b_add = FIX(1.77200) * cb + ONE_HALF;\
    }

#define PGS_YUV_TO_RGB2(r, g, b, y1)\
    {\
        y = (y1) << SCALEBITS;\
        r = cm[(y + r_add) >> SCALEBITS];\
        g = cm[(y + g_add) >> SCALEBITS];\
        b = cm[(y + b_add) >> SCALEBITS];\
    }
#define PGS_RGBA(r,g,b,a) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))

static void read_color_table(unsigned char *buf, int size, pgs_info_t *pgs_info)
{
    int pos;
    for (pos = 2; pos < size; pos += 5)
    {
#if 1
        unsigned char r, g, b;
        unsigned char y, u, v;
        y = buf[pos + 1];
        u = buf[pos + 2];
        v = buf[pos + 3];
        y -= 16;
        u -= 128;
        v -= 128;
        r = (298 * y + 409 * v + 128) >> 8;
        g = (298 * y - 100 * u - 208 * v + 128) >> 8;
        b = (298 * y + 516 * u + 128) >> 8;
        /*
        R = Y + 1.140V
        G = Y - 0.395U - 0.581V
        B = Y + 2.032U
        */
        pgs_info->palette[buf[pos]] =
            (r << 24) | (g << 16) | (b << 8) | (buf[pos + 4]);
#else
        int color_id;
        int y, cb, cr, alpha;
        int r, g, b, r_add, g_add, b_add;
        color_id = buf[pos];
        y = buf[pos + 1];
        cb = buf[pos + 2];
        cr = buf[pos + 3];
        alpha = buf[pos + 4];
        PGS_YUV_TO_RGB1(cb, cr);
        PGS_YUV_TO_RGB2(r, g, b, y);
        /* Store color in palette */
        //ctx->clut[color_id] = RGBA(r,g,b,alpha);
        pgs_info->palette[buf[pos]] = PGS_RGBA(r, g, b, alpha);
#endif
    }
}

static unsigned char read_bitmap(unsigned char *buf, int size,
                                 pgs_info_t *pgs_info)
{
    char object_id, version_number;
    char continue_flag;
    int object_size;
    int rleBytes;
    object_id = buf[1];
    version_number = buf[2];
    continue_flag = buf[3];
    if (continue_flag & 0x80)   //first
    {
        object_size = (buf[4] << 16) | (buf[5] << 8) | (buf[6]);
        pgs_info->image_width = (buf[7] << 8) | (buf[8]);
        pgs_info->image_height = (buf[9] << 8) | (buf[10]);
        LOGI("read_bitmap values are %d,%d,%d\n", object_size,
             pgs_info->image_width, pgs_info->image_height);
        //if (pgs_info->rle_buf)
        //    free(pgs_info->rle_buf);
        pgs_info->rle_buf_size = 0;
        pgs_info->rle_rd_off = 0;
        pgs_info->rle_buf = (unsigned char *)malloc(object_size);
        if (pgs_info->rle_buf)
        {
            pgs_info->rle_buf_size = object_size;
        }
        rleBytes = size - 11;
    }
    else            //last
    {
        rleBytes = size - 4;
    }
    if ((pgs_info->rle_buf)
            && ((pgs_info->rle_rd_off + rleBytes) <= pgs_info->rle_buf_size))
    {
        memcpy(pgs_info->rle_buf + pgs_info->rle_rd_off,
               buf + size - rleBytes, rleBytes);
        pgs_info->rle_rd_off += rleBytes;
    }
    return (continue_flag & 0x40);  //last, 0x40
}

int draw_pixel_fun(int x, int y, unsigned pixel, void *arg)
{
    PGS_subtitle_showdata *result = (PGS_subtitle_showdata *) arg;
    int line_size = result->image_width * 4;
    char *buf;
    //if(x&1)
    //buf=result->result_buf+line_size*y+(x-1)*4;
    //else
    //buf=result->result_buf+line_size*y+(x+1)*4;
    buf = result->result_buf + line_size * y + (x) * 4;
    if ((x >= result->image_width) || (y >= result->image_height))
        return 0;
    //    buf[3]=pixel>>24;
    //    buf[2]=(pixel>>16)&0xff;
    //    buf[1]=(pixel>>8)&0xff;
    //    buf[0]=pixel&0xff;
    buf[0] = pixel >> 24;
    buf[1] = (pixel >> 16) & 0xff;
    buf[2] = (pixel >> 8) & 0xff;
    buf[3] = pixel & 0xff;
    return 0;
}

void af_pgs_subtitle_register_draw_pixel_fun(draw_pixel_fun_t draw_pixel_fun)
{
    pgs_draw_pixel_fun_ = draw_pixel_fun;
}

void af_pgs_subtitle_rlebitmap_render(PGS_subtitle_showdata *showdata,
                                      void *arg, char mode)
{
    unsigned char *ptr = NULL;
    unsigned char *end_buf = NULL;
    int x, y, count;
    unsigned char color_index;
    if ((showdata == NULL))
        return;

    ptr = showdata->rle_buf;
    end_buf = showdata->rle_buf + showdata->rle_buf_size;
    showdata->render_height = 0;
    y = 0;
    while ((y < showdata->image_height) && (ptr < end_buf))
    {
        x = 0;
        while ((x < showdata->image_width) && (ptr < end_buf))
        {
            if ((*ptr == 0) && (*(ptr + 1) == 0))
            {
                if (x > 0)
                {
                    for (; x < showdata->image_width; x++)
                    {
                        draw_pixel_fun(x, y, 0, arg);
                        //dbg_dump_pixels(x,y,0);
                    }
                }
                //draw_pixel_fun(x,y,mode?showdata->palette[0]:0,arg);
                //append_pixels(y, showdata->palette[0], showdata->image_width-x);
                //x=showdata->image_width;
                ptr += 2;
            }
            else if (*ptr)
            {
                draw_pixel_fun(x, y,
                               mode ? showdata->
                               palette[*ptr] : *ptr, arg);
                //dbg_dump_pixels(x,y,showdata->palette[*ptr]);
                x++;
                ptr++;
            }
            else
            {
                ptr++;
                if (*ptr & 0x40)
                {
                    count =
                        ((*ptr & 0x3f) << 8) | (*(ptr + 1));
                    if (*ptr & 0x80)
                    {
                        color_index = *(ptr + 2);
                        ptr++;
                    }
                    else
                    {
                        color_index = 0;
                    }
                    ptr += 2;
                }
                else
                {
                    count = *ptr & 0x3f;
                    if (*ptr & 0x80)
                    {
                        color_index = *(ptr + 1);
                        ptr++;
                    }
                    else
                    {
                        color_index = 0;
                    }
                    ptr++;
                }
                for (; count > 0; count--, x++)
                {
                    draw_pixel_fun(x, y,
                                   mode ? showdata->
                                   palette[color_index] :
                                   color_index, arg);
                    //dbg_dump_pixels(x,y,showdata->palette[color_index]);
                }
                //append_pixels(y, showdata->palette[color_index], count);
                //x+=count;
            }
        }
        y++;
    }
    showdata->render_height = y;
}

int parser_one_pgs(AML_SPUVAR *spu)
{
    int buffer_size = 0;
    buffer_size = (subtitle_pgs.showdata.image_width *
                   subtitle_pgs.showdata.image_height * 4);
    if (buffer_size <= 0)
        return -1;
    subtitle_pgs.showdata.result_buf = malloc(buffer_size);
    if (subtitle_pgs.showdata.result_buf == NULL)
    {
        LOGE("malloc pgs result buf failed \n");
        return -1;
    }
    memset(subtitle_pgs.showdata.result_buf, 0x0, buffer_size);
    if (subtitle_status == SUB_STOP) {
        return 0;
    }
    af_pgs_subtitle_rlebitmap_render(&(subtitle_pgs.showdata),
                                     &subtitle_pgs.showdata, 1);
    if (subtitle_pgs.showdata.image_width == 1920 &&
            subtitle_pgs.showdata.image_height == 1080)
    {
        unsigned char *cut_buffer = NULL;
        cut_buffer = malloc(buffer_size / 4);
        if (cut_buffer != NULL)
        {
            memset(cut_buffer, 0x0, buffer_size / 4);
            memcpy(cut_buffer,
                   subtitle_pgs.showdata.result_buf +
                   buffer_size * 3 / 4, buffer_size / 4);
            free(subtitle_pgs.showdata.result_buf);
            subtitle_pgs.showdata.result_buf = cut_buffer;
            subtitle_pgs.showdata.image_height /= 4;
        }
        else
            LOGI("malloc cut buffer failed \n ");
    }
    spu->subtitle_type = SUBTITLE_PGS;
    spu->spu_data = subtitle_pgs.showdata.result_buf;
    spu->spu_width = subtitle_pgs.showdata.image_width;
    spu->spu_height = subtitle_pgs.showdata.image_height;
    spu->pts = subtitle_pgs.showdata.pts;
    spu->buffer_size = spu->spu_width * spu->spu_height * 4;
    if (spu->buffer_size > 0 && spu->spu_data != NULL)
    {
        write_subtitle_file(spu);
    }
    else
    {
        LOGI("spu buffer size %d, spu->spu_data %x\n", spu->buffer_size,
             spu->spu_data);
        free(subtitle_pgs.showdata.result_buf);
    }
    subtitle_pgs.showdata.result_buf = NULL;
    return 1;
}

static int pgs_decode(AML_SPUVAR *spu, unsigned char *buf)
{
    unsigned char *cur_buf = buf;
    int size;
    int start_time, end_time;
    unsigned char type;
    pgs_info_t *pgs_info = subtitle_pgs.pgs_info;
    type = read_time_header(&cur_buf, &size, &start_time, &end_time);
    switch (type)
    {
        case 0x16:
            if (size == 0x13)   //subpicture header
            {
                LOGI("enter type 0x16,0x13, %d\n", read_pgs_byte);
                read_subpictureHeader(cur_buf - size, size, pgs_info);
            }
            else if (size == 0xb)   //clearSubpictureHeader
            {
                LOGI("enter type 0x16,0xb, %d %d\n", start_time,
                     end_time);
                spu->subtitle_type = SUBTITLE_PGS;
                spu->pts = start_time;
                write_subtitle_file(spu);
                //add_pgs_end_time(start_time);
                //subtitle_pgs_send_msg_bplay_show_subtitle(subtitle_pgs.cntl, BROADCAST_ALL, SUBTITLE_TYPE_PGS, 0);
            }
            else
            {
            }
            break;
        case 0x17:      //window
            if (size == 0xa)
            {
                LOGI("enter type 0x17, %d\n", read_pgs_byte);
                read_windowHeader(cur_buf - size, size, pgs_info);
            }
            else
            {
            }
            break;
        case 0x14:      //color table
            LOGI("enter type 0x14 %d\n", read_pgs_byte);
            read_color_table(cur_buf - size, size, pgs_info);
            break;
        case 0x15:      //bitmap
            LOGI("enter type 0x15 %d\n", read_pgs_byte);
            if (read_bitmap(cur_buf - size, size, pgs_info))
            {
                LOGI("success read_bitmap \n ");
                //render it
                subtitle_pgs.showdata.x = subtitle_pgs.pgs_info->x;
                subtitle_pgs.showdata.y = subtitle_pgs.pgs_info->y;
                subtitle_pgs.showdata.width =
                    subtitle_pgs.pgs_info->width;
                subtitle_pgs.showdata.height =
                    subtitle_pgs.pgs_info->height;
                subtitle_pgs.showdata.window_width_offset =
                    subtitle_pgs.pgs_info->window_width_offset;
                subtitle_pgs.showdata.window_height_offset =
                    subtitle_pgs.pgs_info->window_height_offset;
                subtitle_pgs.showdata.window_width =
                    subtitle_pgs.pgs_info->window_width;
                subtitle_pgs.showdata.window_height =
                    subtitle_pgs.pgs_info->window_height;
                subtitle_pgs.showdata.image_width =
                    subtitle_pgs.pgs_info->image_width;
                subtitle_pgs.showdata.image_height =
                    subtitle_pgs.pgs_info->image_height;
                subtitle_pgs.showdata.palette =
                    subtitle_pgs.pgs_info->palette;
                subtitle_pgs.showdata.rle_buf =
                    subtitle_pgs.pgs_info->rle_buf;
                subtitle_pgs.showdata.rle_buf_size =
                    subtitle_pgs.pgs_info->rle_buf_size;
                LOGI("decoder pgs data to show\n\n");
                parser_one_pgs(spu);

                if (pgs_info->rle_buf) {
                    free(pgs_info->rle_buf);
                    pgs_info->rle_buf = NULL;
                }
                //return 0;
                return 1;
            }
            break;
        case 0x80:      //trailer
            LOGI("enter type 0x80\n");
            break;
        default:
            break;
    }
    return 0;
}

int get_pgs_spu(AML_SPUVAR *spu, int read_handle)
{
    char tmpbuf[256];
    unsigned int pgs_pts = 0, pgs_dts = 0, pgs_pts_end;
    unsigned int pgs_temp_pts, pgs_temp_dts;
    int pgs_packet_length = 0, pgs_pes_header_length = 0;
    int64_t packet_header = 0;
    char skip_packet_flag = 0;
    int decode_one_frame_done = 0;  // For soft demuxer
    read_pgs_byte = 0;
    LOGI("enter get_pgs_spu\n");
    int pgs_ret = 0;
    if (subtitle_pgs.pgs_info == NULL)
    {
        LOGI("pgs_info is NULL \n");
        return 0;
    }
DECODE_START:
    while (1)
    {
        if (subtitle_status == SUB_STOP) {
            return 0;
        }
        pgs_pts = pgs_dts = 0;
        pgs_packet_length = pgs_pes_header_length = 0;
        packet_header = 0;
        skip_packet_flag = 0;
        if (getSize(read_handle) < SPU_RD_HOLD_SIZE)
        {
            LOGI("current pgs sub buffer size %d\n", getSize(read_handle));
            break;
        }
        uVobSPU.spu_cache_pos = 0;
        while (read_spu_buf(read_handle, tmpbuf, 1) == 1)
        {
            if (subtitle_status == SUB_STOP)
                return 0;
            packet_header = (packet_header << 8) | tmpbuf[0];
            LOGI("## get_pgs_spu %x,%x,%x,%x,%llx,-------------\n",
                 tmpbuf[0], tmpbuf[1], tmpbuf[2], tmpbuf[3],
                 packet_header);
            if ((packet_header & 0xffffffff) == 0x000001bd)
            {
                LOGI("## 222  get_pgs_spu hardware demux pgs %x,%llx,-----------\n", tmpbuf[0], packet_header & 0xffffffffff);
                break;
            }
            else if ((packet_header & 0xffffffffff) == 0x414d4c5577 || (packet_header & 0xffffffffff) == 0x414d4c55aa)
            {
                LOGI("## 222  get_pgs_spu soft demux pgs %x,%llx,-----------\n", tmpbuf[0], packet_header & 0xffffffffff);
                goto aml_soft_demux;
            }
        }
        if ((packet_header & 0xffffffff) == 0x000001bd)
        {
            LOGI("find header 0x000001bd\n");
            if (read_spu_buf(read_handle, tmpbuf, 2) == 2)
            {
                pgs_packet_length =
                    (tmpbuf[0] << 8) | tmpbuf[1];
#if 0
                if ((uVobSPU.mem_rp < uVobSPU.mem_wp) &&
                        (uVobSPU.mem_rp2 < uVobSPU.mem_wp) &&
                        (uVobSPU.mem_rp2 + pgs_packet_length >
                         uVobSPU.mem_wp))
                {
                    uVobSPU.mem_rp = uVobSPU.mem_rp2;
                    break;
                }
                else if ((uVobSPU.mem_rp > uVobSPU.mem_wp) &&
                         (uVobSPU.mem_rp2 > uVobSPU.mem_wp) &&
                         (uVobSPU.mem_rp2 +
                          pgs_packet_length >
                          uVobSPU.mem_wp + uVobSPU.mem_end -
                          uVobSPU.mem_start))
                {
                    uVobSPU.mem_rp = uVobSPU.mem_rp2;
                    break;
                }
                if ((uVobSPU.mem_rp < uVobSPU.mem_wp) &&
                        (uVobSPU.mem_rp2 < uVobSPU.mem_wp) &&
                        (uVobSPU.mem_rp2 < uVobSPU.mem_rp))
                {
                    uVobSPU.mem_rp = uVobSPU.mem_rp2;
                    break;
                }
                else if ((uVobSPU.mem_rp < uVobSPU.mem_wp) &&
                         (uVobSPU.mem_rp2 > uVobSPU.mem_wp))
                {
                    uVobSPU.mem_rp2 = uVobSPU.mem_rp;
                    break;
                }
#endif
                if (pgs_packet_length >= 3)
                {
                    if (read_spu_buf(read_handle, tmpbuf, 3)
                            == 3)
                    {
                        pgs_packet_length -= 3;
                        pgs_pes_header_length =
                            tmpbuf[2];
                        if (pgs_packet_length >=
                                pgs_pes_header_length)
                        {
                            if ((tmpbuf[1] & 0xc0)
                                    == 0x80)
                            {
                                if (read_spu_buf
                                        (read_handle,
                                         tmpbuf,
                                         pgs_pes_header_length)
                                        ==
                                        pgs_pes_header_length)
                                {
                                    pgs_temp_pts
                                        = 0;
                                    pgs_temp_pts
                                        =
                                            pgs_temp_pts
                                            |
                                            ((tmpbuf[0] & 0xe) << 29);
                                    pgs_temp_pts
                                        =
                                            pgs_temp_pts
                                            |
                                            ((tmpbuf[1] & 0xff) << 22);
                                    pgs_temp_pts
                                        =
                                            pgs_temp_pts
                                            |
                                            ((tmpbuf[2] & 0xfe) << 14);
                                    pgs_temp_pts
                                        =
                                            pgs_temp_pts
                                            |
                                            ((tmpbuf[3] & 0xff) << 7);
                                    pgs_temp_pts
                                        =
                                            pgs_temp_pts
                                            |
                                            ((tmpbuf[4] & 0xfe) >> 1);
                                    pgs_pts = pgs_temp_pts; // - pts_aligned;
                                    pgs_packet_length
                                    -=
                                        pgs_pes_header_length;
                                }
                            }
                            else if ((tmpbuf[1] &
                                      0xc0) ==
                                     0xc0)
                            {
                                if (read_spu_buf
                                        (read_handle,
                                         tmpbuf,
                                         pgs_pes_header_length)
                                        ==
                                        pgs_pes_header_length)
                                {
                                    pgs_temp_pts
                                        = 0;
                                    pgs_temp_pts
                                        =
                                            pgs_temp_pts
                                            |
                                            ((tmpbuf[0] & 0xe) << 29);
                                    pgs_temp_pts
                                        =
                                            pgs_temp_pts
                                            |
                                            ((tmpbuf[1] & 0xff) << 22);
                                    pgs_temp_pts
                                        =
                                            pgs_temp_pts
                                            |
                                            ((tmpbuf[2] & 0xfe) << 14);
                                    pgs_temp_pts
                                        =
                                            pgs_temp_pts
                                            |
                                            ((tmpbuf[3] & 0xff) << 7);
                                    pgs_temp_pts
                                        =
                                            pgs_temp_pts
                                            |
                                            ((tmpbuf[4] & 0xfe) >> 1);
                                    pgs_pts = pgs_temp_pts; //- pts_aligned;
                                    pgs_temp_dts
                                        = 0;
                                    pgs_temp_dts
                                        =
                                            pgs_temp_dts
                                            |
                                            ((tmpbuf[5] & 0xe) << 29);
                                    pgs_temp_dts
                                        =
                                            pgs_temp_dts
                                            |
                                            ((tmpbuf[6] & 0xff) << 22);
                                    pgs_temp_dts
                                        =
                                            pgs_temp_dts
                                            |
                                            ((tmpbuf[7] & 0xfe) << 14);
                                    pgs_temp_dts
                                        =
                                            pgs_temp_dts
                                            |
                                            ((tmpbuf[8] & 0xff) << 7);
                                    pgs_temp_dts
                                        =
                                            pgs_temp_dts
                                            |
                                            ((tmpbuf[9] & 0xfe) >> 1);
                                    pgs_dts = pgs_temp_dts; // - pts_aligned;
                                    pgs_packet_length
                                    -=
                                        pgs_pes_header_length;
                                }
                            }
                            else
                            {
                                skip_packet_flag
                                    = 1;
                            }
                        }
                        else
                        {
                            skip_packet_flag = 1;
                        }
                    }
                }
                else
                {
                    skip_packet_flag = 1;
                }
                if (skip_packet_flag)
                {
                    int iii;
                    char tmp;
                    for (iii = 0; iii < pgs_packet_length;
                            iii++)
                    {
                        if (read_spu_byte
                                (read_handle, &tmp) == 0)
                            break;
                    }
                }
                else if ((pgs_pts) && (pgs_packet_length > 0))
                {
                    char *buf = NULL;
                    if ((8 + 2 + pgs_packet_length) >
                            (OSD_HALF_SIZE * 4))
                    {
                        LOGE("pgs packet is too big\n\n");
                        break;
                    }
                    else if ((uVobSPU.
                              spu_decoding_start_pos + 8 +
                              2 + pgs_packet_length) >
                             (OSD_HALF_SIZE * 4))
                    {
                        uVobSPU.spu_decoding_start_pos =
                            0;
                    }
                    buf = malloc(8 + 2 + pgs_packet_length);
                    LOGI("pgs_packet_length is %d\n",
                         pgs_packet_length);
                    subtitle_pgs.showdata.pts = pgs_dts;
                    if (getSize(read_handle) < pgs_packet_length ||
                            subtitle_status == SUB_STOP)
                    {
                        pgs_ret = 0;
                        if (buf) {
                            free(buf);
                            buf = NULL;
                        }
                        goto pgs_decode_end;
                    }

                    if (buf)
                    {
                        memset(buf, 0x0,
                               8 + 2 +
                               pgs_packet_length);
                        buf[0] = 'P';
                        buf[1] = 'G';
                        buf[2] = (pgs_pts >> 24) & 0xff;
                        buf[3] = (pgs_pts >> 16) & 0xff;
                        buf[4] = (pgs_pts >> 8) & 0xff;
                        buf[5] = pgs_pts & 0xff;
                        buf[6] = (pgs_pts >> 24) & 0xff;
                        buf[7] = (pgs_pts >> 16) & 0xff;
                        buf[8] = (pgs_pts >> 8) & 0xff;
                        buf[9] = pgs_pts & 0xff;
                        if (read_spu_buf
                                (read_handle, buf + 10,
                                 pgs_packet_length) ==
                                pgs_packet_length)
                        {
                            LOGI("start decode pgs subtitle\n\n");
                            pgs_ret =
                                pgs_decode(spu,
                                           buf);
                        }
                        free(buf);
                        buf = NULL;
                    }
                    if (pgs_ret == 1)
                        goto pgs_decode_end;
                    continue;
                }
            }
        }
        else
        {
            LOGI("header is not 0x000001bd\n");
            break;
        }
    }
aml_soft_demux:
    if ((packet_header & 0xffffffffff) == 0x414d4c5577 || (packet_header & 0xffffffffff) == 0x414d4c55aa)
    {
        int read_data_len = 0, data_len = 0;
        int pgs_packet_type = 0;
        char *data = NULL;
        char *pdata = NULL;
        LOGI("## 333 get_pgs_spu %x,%x,%x,%x,%x,-------------\n",
             tmpbuf[0], tmpbuf[1], tmpbuf[2], tmpbuf[3]);
        if (read_spu_buf(read_handle, tmpbuf, 15) == 15)
        {
            data_len = tmpbuf[3] << 24;
            data_len |= tmpbuf[4] << 16;
            data_len |= tmpbuf[5] << 8;
            data_len |= tmpbuf[6];
            pgs_temp_pts = tmpbuf[7] << 24;
            pgs_temp_pts |= tmpbuf[8] << 16;
            pgs_temp_pts |= tmpbuf[9] << 8;
            pgs_temp_pts |= tmpbuf[10];
            pgs_pts = pgs_temp_pts;
            pgs_temp_pts = 0;
            pgs_temp_pts = tmpbuf[11] << 24;
            pgs_temp_pts |= tmpbuf[12] << 16;
            pgs_temp_pts |= tmpbuf[13] << 8;
            pgs_temp_pts |= tmpbuf[14];
            spu->m_delay = pgs_temp_pts;
            if (pgs_temp_pts != 0)
            {
                spu->m_delay += pgs_pts;
            }
            pgs_pts_end = pgs_pts + pgs_temp_pts;
            pgs_dts = pgs_pts;
            spu->subtitle_type = SUBTITLE_PGS;
            spu->pts = pgs_pts;
            LOGI("## 4444 %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,--%d,%x,%x,-------------\n", tmpbuf[0], tmpbuf[1], tmpbuf[2], tmpbuf[3], tmpbuf[4], tmpbuf[5], tmpbuf[6], tmpbuf[7], tmpbuf[8], tmpbuf[9], tmpbuf[10], tmpbuf[11], tmpbuf[12], tmpbuf[13], tmpbuf[14], data_len, pgs_pts, pgs_pts_end);
            data = malloc(data_len);
            if (subtitle_status == SUB_STOP) {
                if (data) {
                    free(data);
                    data = NULL;
                }
                return 0;
            }
            if (data)
            {
                memset(data, 0x0, data_len);
            }
            else
            {
                LOGE("malloc data = malloc(data_len) failed \n");
                return -1;
            }
            int ret = 0;
            ret = read_spu_buf(read_handle, data, data_len);
            read_pgs_byte += ret;
            LOGI("## ret=%d,data_len=%d,%x,%x,%x,%x,%x,%x,%x,%x,---------\n", ret, data_len, data[0], data[1], data[2], data[3], data[data_len - 4], data[data_len - 3], data[data_len - 2], data[data_len - 1]);
            pdata = data;
        }
        while (read_data_len < data_len)
        {
            LOGI("## %x,%x,%x, \n", data[0], data[1], data[2]);
            pgs_packet_type = data[0];
            pgs_packet_length = (data[1] << 8) | data[2];
            read_data_len += 3;
            LOGI("## read:%d, data_len:%d, len is %d\n",
                 read_data_len, data_len, pgs_packet_length);
            if (read_data_len + pgs_packet_length > data_len)
            {
                LOGI("## data fault ! ---\n");
                break;
            }
            if ((pgs_pts) && (pgs_packet_length > 0))
            {
                char *buf = NULL;
                if ((8 + 2 + pgs_packet_length) >
                        (OSD_HALF_SIZE * 4))
                {
                    LOGE("pgs packet is too big\n\n");
                    break;
                }
                else if ((uVobSPU.spu_decoding_start_pos + 8 +
                          2 + pgs_packet_length) >
                         (OSD_HALF_SIZE * 4))
                {
                    uVobSPU.spu_decoding_start_pos = 0;
                }
                buf = malloc(8 + 2 + 3 + pgs_packet_length);
                LOGI("pgs_packet_length is %d, %x,\n",
                     pgs_packet_length, buf);
                subtitle_pgs.showdata.pts = pgs_dts;
                if (subtitle_status == SUB_STOP)
                {
                    pgs_ret = 0;
                    if (buf) {
                        free(buf);
                        buf = NULL;
                    }
                    goto pgs_decode_end;
                }
                if (buf)
                {
                    LOGI("## 555 get_pgs_spu ------------\n");
                    memset(buf, 0x0,
                           8 + 2 + 3 + pgs_packet_length);
                    buf[0] = 'P';
                    buf[1] = 'G';
                    buf[2] = (pgs_pts >> 24) & 0xff;
                    buf[3] = (pgs_pts >> 16) & 0xff;
                    buf[4] = (pgs_pts >> 8) & 0xff;
                    buf[5] = pgs_pts & 0xff;
                    buf[6] = (pgs_pts_end >> 24) & 0xff;
                    buf[7] = (pgs_pts_end >> 16) & 0xff;
                    buf[8] = (pgs_pts_end >> 8) & 0xff;
                    buf[9] = pgs_pts_end & 0xff;
                    buf[10] =
                        pgs_packet_type & 0xff, buf[11] =
                            (pgs_packet_length >> 8) & 0xff,
                            buf[12] = pgs_packet_length & 0xff;
                    memcpy(buf + 13, data + 3,
                           pgs_packet_length);
                    LOGI("## start decode pgs subtitle %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,\n\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14]);
                    pgs_ret = pgs_decode(spu, buf);
                    read_data_len += pgs_packet_length;
                    data += pgs_packet_length + 3;
                    free(buf);
                    buf = NULL;
                    if (pgs_ret == 1)
                        decode_one_frame_done = 1;
                }
            }
        }
        LOGI("## break, get_pgs_spu read_data_len=%d,data_len=%d,spu->spu_data=%x,------------\n\n", read_data_len, data_len, spu->spu_data);
        if (pdata)
        {
            free(pdata);
            pdata = NULL;
        }
        if (decode_one_frame_done == 0)
            goto DECODE_START;
    }
pgs_decode_end:
    return pgs_ret;
}
