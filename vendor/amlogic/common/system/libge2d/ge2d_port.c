/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <linux/ge2d.h>
#include "ge2d_port.h"
#include "ge2d_com.h"

#define   FILE_NAME_GE2D        "/dev/ge2d"

#define CANVAS_ALIGNED(x)        (((x) + 31) & ~31)
#define YV12_Y_ALIGNED(x)        (((x) + 63) & ~63)


#define GE2D_BPP_32  32
#define GE2D_BPP_24  24
#define GE2D_BPP_16  16
#define GE2D_BPP_12  12
#define GE2D_BPP_8   8

static int b_src_swap = 0;
static int  pixel_to_ge2d_format(int img_format, int *pge2d_format,int *p_bpp)
{
    int is_rgb = -1;

    switch (img_format) {
        case PIXEL_FORMAT_RGBA_8888:
        case PIXEL_FORMAT_RGBX_8888:
        *pge2d_format = GE2D_FORMAT_S32_ABGR;
        *p_bpp = GE2D_BPP_32;
        is_rgb = 1;
        break;
        case PIXEL_FORMAT_BGRA_8888:
        *pge2d_format = GE2D_FORMAT_S32_ARGB;
        *p_bpp = GE2D_BPP_32;
        is_rgb = 1;
        break;
        case PIXEL_FORMAT_RGB_888:
        *pge2d_format = GE2D_FORMAT_S24_RGB;
        *p_bpp = GE2D_BPP_24;
        is_rgb = 1;
        break;
        case PIXEL_FORMAT_RGB_565:
        *pge2d_format = GE2D_FORMAT_S16_RGB_565;
        *p_bpp = GE2D_BPP_16;
        is_rgb = 1;
        break;
        case PIXEL_FORMAT_YCrCb_420_SP:
        *pge2d_format = GE2D_FORMAT_M24_NV21;
        *p_bpp = GE2D_BPP_8;
        is_rgb = 0;
        break;
        case  PIXEL_FORMAT_YV12:
        *pge2d_format = GE2D_FORMAT_M24_YUV420;
        *p_bpp = GE2D_BPP_8;
        is_rgb = 0;
        break;
        case  PIXEL_FORMAT_Y8:
        *pge2d_format = GE2D_FORMAT_S8_Y;
        *p_bpp = GE2D_BPP_8;
        is_rgb = 0;
        break;
        case PIXEL_FORMAT_YCbCr_422_I:
        *pge2d_format = GE2D_FORMAT_S16_YUV422;
        *p_bpp = GE2D_BPP_8;
        is_rgb = 0;
        break;
        default:
        E_GE2D("Image format %d not supported!", img_format);
        *pge2d_format = 0xffffffff;
        *p_bpp = GE2D_BPP_32;
        break;
    }
    return is_rgb;
}

int cap_attr;


static void ge2d_set_canvas(int bpp, int w,int h, int *canvas_w, int *canvas_h)
{
   *canvas_w = (CANVAS_ALIGNED(w * bpp >> 3))/(bpp >> 3);
   *canvas_h = h;
}


static inline unsigned blendop(unsigned color_blending_mode,
        unsigned color_blending_src_factor,
        unsigned color_blending_dst_factor,
        unsigned alpha_blending_mode,
        unsigned alpha_blending_src_factor,
        unsigned alpha_blending_dst_factor)
{
    return (color_blending_mode << 24) |
        (color_blending_src_factor << 20) |
        (color_blending_dst_factor << 16) |
        (alpha_blending_mode << 8) |
        (alpha_blending_src_factor << 4) | (alpha_blending_dst_factor << 0);
}


static int is_no_alpha(int format)
{
    if ((format == PIXEL_FORMAT_RGBX_8888) ||
        (format == PIXEL_FORMAT_RGB_565) ||
        (format == PIXEL_FORMAT_RGB_888) ||
        (format == PIXEL_FORMAT_YCrCb_420_SP)||
        (format == PIXEL_FORMAT_Y8)||
        (format == PIXEL_FORMAT_YV12) ||
        (format == PIXEL_FORMAT_YCbCr_422_I)||
        (format == PIXEL_FORMAT_YCbCr_422_SP))
        return 1;
    else
        return 0;
}

static int is_need_swap_src2(int format,buffer_info_t *src2, buffer_info_t *dst)
{
    int ret = 0;
    /* src2 not support nv21/nv12/yv12, swap src1 and src2 */
    if ((format == PIXEL_FORMAT_YCrCb_420_SP) ||
        (format == PIXEL_FORMAT_YV12))
        ret = 1;
    else
        ret = 0;

    /* src2 scaler, swap src1 and src2 */
    if (!ret) {
        if ((src2->rect.w < dst->rect.w) ||
            (src2->rect.h < dst->rect.h))
            ret = 1;
        else
            ret = 0;
    }
    return ret;
}

static int is_rect_valid(buffer_info_t *pbuffer_info)
{
    int ret = 1;
    if (CANVAS_ALLOC == pbuffer_info->memtype)
        if (((unsigned int)pbuffer_info->rect.w > pbuffer_info->canvas_w) ||
            ((unsigned int)pbuffer_info->rect.h > pbuffer_info->canvas_h)) {
            E_GE2D("rect.w,h:[%d,%d] out of range!\n",pbuffer_info->rect.w,
                pbuffer_info->rect.h);
            ret = 0;
        }
    return ret;
}

static int ge2d_fillrectangle_config_ex_ion(int fd,aml_ge2d_info_t *pge2dinfo)
{
    int ret = -1;
    struct config_para_ex_ion_s ge2d_config_ex;
    int src_format = 0xffffffff,dst_format = 0xffffffff;
    int s_canvas_w = 0;
    int s_canvas_h = 0;
    int d_canvas_w = 0;
    int d_canvas_h = 0;
    int bpp = 0;
    int is_rgb = -1;
    buffer_info_t* input_buffer_info = &(pge2dinfo->src_info[0]);
    buffer_info_t* output_buffer_info = &(pge2dinfo->dst_info);

    memset(&ge2d_config_ex, 0, sizeof(struct config_para_ex_ion_s ));

    if ((CANVAS_ALLOC == output_buffer_info->memtype)) {
        is_rgb = pixel_to_ge2d_format(output_buffer_info->format,&dst_format,&bpp);
        dst_format |= GE2D_LITTLE_ENDIAN;
        if ((int)0xffffffff == dst_format) {
            E_GE2D("can't get proper ge2d format\n" );
            return ge2d_fail;
        }
        ge2d_set_canvas(bpp,output_buffer_info->canvas_w,output_buffer_info->canvas_h,&d_canvas_w,&d_canvas_h);

        pixel_to_ge2d_format(input_buffer_info->format,&src_format,&bpp);
        src_format |= GE2D_LITTLE_ENDIAN;
        ge2d_set_canvas(bpp,input_buffer_info->canvas_w,input_buffer_info->canvas_h,&s_canvas_w,&s_canvas_h);
    }else {
        is_rgb = pixel_to_ge2d_format(output_buffer_info->format,&dst_format,&bpp);
        dst_format |= GE2D_LITTLE_ENDIAN;
        ge2d_set_canvas(bpp,output_buffer_info->canvas_w,output_buffer_info->canvas_h,&d_canvas_w,&d_canvas_h);

        pixel_to_ge2d_format(input_buffer_info->format,&src_format,&bpp);
        src_format |= GE2D_LITTLE_ENDIAN;
        ge2d_set_canvas(bpp,input_buffer_info->canvas_w,input_buffer_info->canvas_h,&s_canvas_w,&s_canvas_h);
    }
    D_GE2D("ge2d_fillrectangle_config_ex,memtype=%x,src_format=%x,s_canvas_w=%d,s_canvas_h=%d,rotation=%d\n",
        input_buffer_info->memtype,src_format,s_canvas_w,s_canvas_h,input_buffer_info->rotation);

    D_GE2D("ge2d_fillrectangle_config_ex,memtype=%x,dst_format=%x,d_canvas_w=%d,d_canvas_h=%d,rotation=%d\n",
        output_buffer_info->memtype,dst_format,d_canvas_w,d_canvas_h,output_buffer_info->rotation);

    ge2d_config_ex.src_para.mem_type = CANVAS_TYPE_INVALID;
    ge2d_config_ex.src_para.format = src_format;
    ge2d_config_ex.src_para.left = 0;
    ge2d_config_ex.src_para.top = 0;
    ge2d_config_ex.src_para.width = s_canvas_w;
    ge2d_config_ex.src_para.height = s_canvas_h;

    ge2d_config_ex.dst_para.mem_type = output_buffer_info->memtype;
    ge2d_config_ex.dst_para.format = dst_format;
    ge2d_config_ex.dst_para.left = 0;
    ge2d_config_ex.dst_para.top = 0;
    ge2d_config_ex.dst_para.width = d_canvas_w;
    ge2d_config_ex.dst_para.height = d_canvas_h;

    ge2d_config_ex.src2_para.mem_type = CANVAS_TYPE_INVALID;

    switch (pge2dinfo->dst_info.rotation) {
        case GE2D_ROTATION_0:
            break;
        case GE2D_ROTATION_90:
            ge2d_config_ex.dst_xy_swap = 1;
            ge2d_config_ex.dst_para.x_rev = 1;
            break;
        case GE2D_ROTATION_180:
            ge2d_config_ex.dst_para.x_rev = 1;
            ge2d_config_ex.dst_para.y_rev = 1;
            break;
        case GE2D_ROTATION_270:
            ge2d_config_ex.dst_xy_swap = 1;
            ge2d_config_ex.dst_para.y_rev = 1;
            break;
        default:
            break;
    }


    if (CANVAS_ALLOC == output_buffer_info->memtype) {
        if (is_rgb == 1) {
            ge2d_config_ex.dst_planes[0].addr = output_buffer_info->offset;
            ge2d_config_ex.dst_planes[0].shared_fd = output_buffer_info->shared_fd;
            ge2d_config_ex.dst_planes[0].w = d_canvas_w;
            ge2d_config_ex.dst_planes[0].h = d_canvas_h;
        } else if (output_buffer_info->format == PIXEL_FORMAT_YCrCb_420_SP) {
            ge2d_config_ex.dst_planes[0].addr = output_buffer_info->offset;
            ge2d_config_ex.dst_planes[0].shared_fd = output_buffer_info->shared_fd;
            ge2d_config_ex.dst_planes[0].w = d_canvas_w;
            ge2d_config_ex.dst_planes[0].h = d_canvas_h;
            ge2d_config_ex.dst_planes[1].addr = (d_canvas_w * d_canvas_h);
            ge2d_config_ex.dst_planes[1].w = d_canvas_w;
            ge2d_config_ex.dst_planes[1].h = d_canvas_h/2;
        }  else if (output_buffer_info->format == PIXEL_FORMAT_Y8) {
            ge2d_config_ex.dst_planes[0].addr = output_buffer_info->offset;
            ge2d_config_ex.dst_planes[0].shared_fd = output_buffer_info->shared_fd;
            ge2d_config_ex.dst_planes[0].w = d_canvas_w;
            ge2d_config_ex.dst_planes[0].h = d_canvas_h;
        } else if (output_buffer_info->format == PIXEL_FORMAT_YV12) {
            ge2d_config_ex.dst_planes[0].addr = output_buffer_info->offset;
            ge2d_config_ex.dst_planes[0].shared_fd = output_buffer_info->shared_fd;
            ge2d_config_ex.dst_planes[0].w = d_canvas_w;
            ge2d_config_ex.dst_planes[0].h = d_canvas_h;
            /* android is ycrcb,kernel is ycbcr,swap the addr */
            ge2d_config_ex.dst_planes[1].addr = YV12_Y_ALIGNED(d_canvas_w) *
                d_canvas_h + CANVAS_ALIGNED(d_canvas_w/2) * d_canvas_h/2;
            ge2d_config_ex.dst_planes[1].w = CANVAS_ALIGNED(d_canvas_w/2);
            ge2d_config_ex.dst_planes[1].h = d_canvas_h/2;
            ge2d_config_ex.dst_planes[2].addr = YV12_Y_ALIGNED(d_canvas_w) * d_canvas_h;
            ge2d_config_ex.dst_planes[2].w = CANVAS_ALIGNED(d_canvas_w/2);
            ge2d_config_ex.dst_planes[2].h = d_canvas_h/2;
            if ( !(d_canvas_w % 64) || !(d_canvas_w % 32))
                D_GE2D("yv12 src canvas_w not alligned\n ");
        } else if (output_buffer_info->format == PIXEL_FORMAT_YCbCr_422_SP) {
            ge2d_config_ex.dst_planes[0].addr = output_buffer_info->offset;
            ge2d_config_ex.dst_planes[0].shared_fd = output_buffer_info->shared_fd;
            ge2d_config_ex.dst_planes[0].w = d_canvas_w;
            ge2d_config_ex.dst_planes[0].h = d_canvas_h;
            ge2d_config_ex.dst_planes[1].addr = (d_canvas_w * d_canvas_h);
            ge2d_config_ex.dst_planes[1].w = d_canvas_w;
            ge2d_config_ex.dst_planes[1].h = d_canvas_h;
        } else if (output_buffer_info->format == PIXEL_FORMAT_YCbCr_422_I) {
            ge2d_config_ex.dst_planes[0].addr = output_buffer_info->offset;
            ge2d_config_ex.dst_planes[0].shared_fd = output_buffer_info->shared_fd;
            ge2d_config_ex.dst_planes[0].w = d_canvas_w*2;
            ge2d_config_ex.dst_planes[0].h = d_canvas_h;
        }
        else {
            E_GE2D("format is not match, should config dst_planes correct.\n");
            return ge2d_fail;
        }
    }
    ge2d_config_ex.alu_const_color = pge2dinfo->const_color;
    ge2d_config_ex.src1_gb_alpha = input_buffer_info->plane_alpha & 0xff;
    if (cap_attr == -1)
        ret = ioctl(fd, GE2D_CONFIG_EX_ION_EN, &ge2d_config_ex);
    else
        ret = ioctl(fd, GE2D_CONFIG_EX_ION, &ge2d_config_ex);
    if (ret < 0) {
        E_GE2D("ge2d config ex ion failed. \n");
        return ge2d_fail;
    }
    return ge2d_success;
}

static int ge2d_blit_config_ex_ion(int fd,aml_ge2d_info_t *pge2dinfo)
{
    int ret = -1;
    struct config_para_ex_ion_s ge2d_config_ex;
    int src_format = 0xffffffff,dst_format = 0xffffffff;
    int s_canvas_w = 0;
    int s_canvas_h = 0;
    int d_canvas_w = 0;
    int d_canvas_h = 0;
    int bpp = 0;
    int is_rgb_input = -1;
    int is_rgb_output = -1;
    buffer_info_t* input_buffer_info = &(pge2dinfo->src_info[0]);
    buffer_info_t* output_buffer_info = &(pge2dinfo->dst_info);

    memset(&ge2d_config_ex, 0, sizeof(struct config_para_ex_ion_s ));

    if ((CANVAS_ALLOC == input_buffer_info->memtype)) {
        is_rgb_input = pixel_to_ge2d_format(input_buffer_info->format,&src_format,&bpp);
        src_format |= GE2D_LITTLE_ENDIAN;
        if ((int)0xffffffff == src_format) {
            E_GE2D("can't get proper ge2d format\n" );
            return ge2d_fail;
        }
        ge2d_set_canvas(bpp,input_buffer_info->canvas_w,input_buffer_info->canvas_h,&s_canvas_w,&s_canvas_h);
    }

    if ((CANVAS_ALLOC == output_buffer_info->memtype)) {
        is_rgb_output = pixel_to_ge2d_format(output_buffer_info->format,&dst_format,&bpp);
        dst_format |= GE2D_LITTLE_ENDIAN;
        if ((int)0xffffffff == dst_format) {
            E_GE2D("can't get proper ge2d format\n" );
            return ge2d_fail;
        }
        ge2d_set_canvas(bpp,output_buffer_info->canvas_w,output_buffer_info->canvas_h,&d_canvas_w,&d_canvas_h);
    }
    D_GE2D("ge2d_blit_config_ex,memtype=%x,src_format=%x,s_canvas_w=%d,s_canvas_h=%d,rotation=%d\n",
        input_buffer_info->memtype,src_format,s_canvas_w,s_canvas_h,input_buffer_info->rotation);

    D_GE2D("ge2d_blit_config_ex,memtype=%x,dst_format=%x,d_canvas_w=%d,d_canvas_h=%d,rotation=%d\n",
        output_buffer_info->memtype,dst_format,d_canvas_w,d_canvas_h,output_buffer_info->rotation);

    ge2d_config_ex.src_para.mem_type = input_buffer_info->memtype;
    ge2d_config_ex.src_para.format = src_format;
    ge2d_config_ex.src_para.left = input_buffer_info->rect.x;
    ge2d_config_ex.src_para.top = input_buffer_info->rect.y;
    ge2d_config_ex.src_para.width = input_buffer_info->rect.w;
    ge2d_config_ex.src_para.height = input_buffer_info->rect.h;

    ge2d_config_ex.src2_para.mem_type = CANVAS_TYPE_INVALID;

    ge2d_config_ex.dst_para.mem_type = output_buffer_info->memtype;
    ge2d_config_ex.dst_para.format = dst_format;
    ge2d_config_ex.dst_para.left = 0;
    ge2d_config_ex.dst_para.top = 0;
    ge2d_config_ex.dst_para.width = d_canvas_w;
    ge2d_config_ex.dst_para.height = d_canvas_h;
    if (input_buffer_info->layer_mode == LAYER_MODE_PREMULTIPLIED)
        ge2d_config_ex.src1_cmult_asel = 2;
    else if (input_buffer_info->layer_mode == LAYER_MODE_COVERAGE)
        ge2d_config_ex.src1_cmult_asel = 0;
    else if (input_buffer_info->layer_mode == LAYER_MODE_NON)
        ge2d_config_ex.src1_cmult_asel = 0;

    switch (pge2dinfo->src_info[0].rotation) {
        case GE2D_ROTATION_0:
            break;
        case GE2D_ROTATION_90:
            ge2d_config_ex.dst_xy_swap = 1;
            ge2d_config_ex.src_para.y_rev = 1;
            break;
        case GE2D_ROTATION_180:
            ge2d_config_ex.src_para.x_rev = 1;
            ge2d_config_ex.src_para.y_rev = 1;
            break;
        case GE2D_ROTATION_270:
            ge2d_config_ex.dst_xy_swap = 1;
            ge2d_config_ex.src_para.x_rev = 1;
            break;
        default:
            break;
    }

    switch (pge2dinfo->dst_info.rotation) {
        case GE2D_ROTATION_0:
            break;
        case GE2D_ROTATION_90:
            ge2d_config_ex.dst_xy_swap = 1;
            ge2d_config_ex.dst_para.x_rev = 1;
            break;
        case GE2D_ROTATION_180:
            ge2d_config_ex.dst_para.x_rev = 1;
            ge2d_config_ex.dst_para.y_rev = 1;
            break;
        case GE2D_ROTATION_270:
            ge2d_config_ex.dst_xy_swap = 1;
            ge2d_config_ex.dst_para.y_rev = 1;
            break;
        default:
            break;
    }

    if (CANVAS_ALLOC == input_buffer_info->memtype) {
        if (is_rgb_input) {
            ge2d_config_ex.src_planes[0].addr = input_buffer_info->offset;
            ge2d_config_ex.src_planes[0].shared_fd = input_buffer_info->shared_fd;
            ge2d_config_ex.src_planes[0].w = s_canvas_w;
            ge2d_config_ex.src_planes[0].h = s_canvas_h;
        } else if (input_buffer_info->format == PIXEL_FORMAT_YCrCb_420_SP) {
            ge2d_config_ex.src_planes[0].addr = input_buffer_info->offset;
            ge2d_config_ex.src_planes[0].shared_fd = input_buffer_info->shared_fd;
            ge2d_config_ex.src_planes[0].w = s_canvas_w;
            ge2d_config_ex.src_planes[0].h = s_canvas_h;
            ge2d_config_ex.src_planes[1].addr = (s_canvas_w * s_canvas_h);
            ge2d_config_ex.src_planes[1].w = s_canvas_w;
            ge2d_config_ex.src_planes[1].h = s_canvas_h/2;
        } else if (input_buffer_info->format == PIXEL_FORMAT_Y8) {
            ge2d_config_ex.src_planes[0].addr = input_buffer_info->offset;
            ge2d_config_ex.src_planes[0].shared_fd = input_buffer_info->shared_fd;
            ge2d_config_ex.src_planes[0].w = s_canvas_w;
            ge2d_config_ex.src_planes[0].h = s_canvas_h;
        } else if (input_buffer_info->format == PIXEL_FORMAT_YV12) {
            ge2d_config_ex.src_planes[0].addr = input_buffer_info->offset;
            ge2d_config_ex.src_planes[0].shared_fd = input_buffer_info->shared_fd;
            ge2d_config_ex.src_planes[0].w = s_canvas_w;
            ge2d_config_ex.src_planes[0].h = s_canvas_h;
            /* android is ycrcb,kernel is ycbcr,swap the addr */
            ge2d_config_ex.src_planes[1].addr = YV12_Y_ALIGNED(s_canvas_w) *
                s_canvas_h + CANVAS_ALIGNED(s_canvas_w/2) * s_canvas_h/2;
            ge2d_config_ex.src_planes[1].w = CANVAS_ALIGNED(s_canvas_w/2);
            ge2d_config_ex.src_planes[1].h = s_canvas_h/2;
            ge2d_config_ex.src_planes[2].addr = YV12_Y_ALIGNED(s_canvas_w) * s_canvas_h;
            ge2d_config_ex.src_planes[2].w = CANVAS_ALIGNED(s_canvas_w/2);
            ge2d_config_ex.src_planes[2].h = s_canvas_h/2;
            if ( !(d_canvas_w % 64) || !(d_canvas_w % 32))
                D_GE2D("yv12 src canvas_w not alligned\n ");
        } else if (input_buffer_info->format == PIXEL_FORMAT_YCbCr_422_SP) {
            ge2d_config_ex.src_planes[0].addr = input_buffer_info->offset;
            ge2d_config_ex.src_planes[0].shared_fd = input_buffer_info->shared_fd;
            ge2d_config_ex.src_planes[0].w = s_canvas_w;
            ge2d_config_ex.src_planes[0].h = s_canvas_h;
            ge2d_config_ex.src_planes[1].addr = (s_canvas_w * s_canvas_h);
            ge2d_config_ex.src_planes[1].w = s_canvas_w;
            ge2d_config_ex.src_planes[1].h = s_canvas_h;
        } else if (input_buffer_info->format == PIXEL_FORMAT_YCbCr_422_I) {
            ge2d_config_ex.src_planes[0].addr = input_buffer_info->offset;
            ge2d_config_ex.src_planes[0].shared_fd = input_buffer_info->shared_fd;
            ge2d_config_ex.src_planes[0].w = s_canvas_w*2;
            ge2d_config_ex.src_planes[0].h = s_canvas_h;
        }
        else {
            E_GE2D("format is not match, should config src_planes correct.\n");
            return ge2d_fail;
        }
    }

    if (CANVAS_ALLOC == output_buffer_info->memtype) {
        if (is_rgb_output) {
            ge2d_config_ex.dst_planes[0].addr = output_buffer_info->offset;
            ge2d_config_ex.dst_planes[0].shared_fd = output_buffer_info->shared_fd;
            ge2d_config_ex.dst_planes[0].w = d_canvas_w;
            ge2d_config_ex.dst_planes[0].h = d_canvas_h;
        } else if (output_buffer_info->format == PIXEL_FORMAT_YCrCb_420_SP) {
            ge2d_config_ex.dst_planes[0].addr = output_buffer_info->offset;
            ge2d_config_ex.dst_planes[0].shared_fd = output_buffer_info->shared_fd;
            ge2d_config_ex.dst_planes[0].w = d_canvas_w;
            ge2d_config_ex.dst_planes[0].h = d_canvas_h;
            ge2d_config_ex.dst_planes[1].addr = (d_canvas_w * d_canvas_h);
            ge2d_config_ex.dst_planes[1].w = d_canvas_w;
            ge2d_config_ex.dst_planes[1].h = d_canvas_h/2;
        } else if (output_buffer_info->format == PIXEL_FORMAT_Y8) {
            ge2d_config_ex.dst_planes[0].addr = output_buffer_info->offset;
            ge2d_config_ex.dst_planes[0].shared_fd = output_buffer_info->shared_fd;
            ge2d_config_ex.dst_planes[0].w = d_canvas_w;
            ge2d_config_ex.dst_planes[0].h = d_canvas_h;
        } else if (output_buffer_info->format == PIXEL_FORMAT_YV12) {
            ge2d_config_ex.dst_planes[0].addr = output_buffer_info->offset;
            ge2d_config_ex.dst_planes[0].shared_fd = output_buffer_info->shared_fd;
            ge2d_config_ex.dst_planes[0].w = d_canvas_w;
            ge2d_config_ex.dst_planes[0].h = d_canvas_h;
            /* android is ycrcb,kernel is ycbcr,swap the addr */
            ge2d_config_ex.dst_planes[1].addr = YV12_Y_ALIGNED(d_canvas_w) *
                d_canvas_h + CANVAS_ALIGNED(d_canvas_w/2) * d_canvas_h/2;
            ge2d_config_ex.dst_planes[1].w = CANVAS_ALIGNED(d_canvas_w/2);
            ge2d_config_ex.dst_planes[1].h = d_canvas_h/2;
            ge2d_config_ex.dst_planes[2].addr = YV12_Y_ALIGNED(d_canvas_w) * d_canvas_h;
            ge2d_config_ex.dst_planes[2].w = CANVAS_ALIGNED(d_canvas_w/2);
            ge2d_config_ex.dst_planes[2].h = d_canvas_h/2;
            if ( !(d_canvas_w % 64) || !(d_canvas_w % 32))
                D_GE2D("yv12 dst canvas_w not alligned\n ");
        } else if (output_buffer_info->format == PIXEL_FORMAT_YCbCr_422_SP) {
            ge2d_config_ex.dst_planes[0].addr = output_buffer_info->offset;
            ge2d_config_ex.dst_planes[0].shared_fd = output_buffer_info->shared_fd;
            ge2d_config_ex.dst_planes[0].w = d_canvas_w;
            ge2d_config_ex.dst_planes[0].h = d_canvas_h;
            ge2d_config_ex.dst_planes[1].addr = (d_canvas_w * d_canvas_h);
            ge2d_config_ex.dst_planes[1].w = d_canvas_w;
            ge2d_config_ex.dst_planes[1].h = d_canvas_h;
        } else if (output_buffer_info->format == PIXEL_FORMAT_YCbCr_422_I) {
            ge2d_config_ex.dst_planes[0].addr = output_buffer_info->offset;
            ge2d_config_ex.dst_planes[0].shared_fd = output_buffer_info->shared_fd;
            ge2d_config_ex.dst_planes[0].w = d_canvas_w*2;
            ge2d_config_ex.dst_planes[0].h = d_canvas_h;
        }
        else {
            E_GE2D("format is not match, should config dst_planes correct.\n");
            return ge2d_fail;
        }
    }

    ge2d_config_ex.alu_const_color = pge2dinfo->const_color;
    ge2d_config_ex.src1_gb_alpha = input_buffer_info->plane_alpha & 0xff;
    ge2d_config_ex.src1_gb_alpha_en = 1;
    D_GE2D("bilt:src1_cmult_asel=%d,src2_cmult_ase2=%d,gl_alpha=%x,src1_gb_alpha_en=%d\n",
        ge2d_config_ex.src1_cmult_asel,
        ge2d_config_ex.src2_cmult_asel,
        ge2d_config_ex.src1_gb_alpha,
        ge2d_config_ex.src1_gb_alpha_en);
    if (cap_attr == -1)
        ret = ioctl(fd, GE2D_CONFIG_EX_ION_EN, &ge2d_config_ex);
    else
        ret = ioctl(fd, GE2D_CONFIG_EX_ION, &ge2d_config_ex);
    if (ret < 0) {
        E_GE2D("ge2d config ex ion failed. \n");
        return ge2d_fail;
    }
    return ge2d_success;
}


static int ge2d_blend_config_ex_ion(int fd,aml_ge2d_info_t *pge2dinfo)
{
    int ret = -1;
    struct config_para_ex_ion_s ge2d_config_ex;
    int src_format = 0xffffffff,src2_format = 0xffffffff,dst_format = 0xffffffff;
    int s_canvas_w = 0;
    int s_canvas_h = 0;
    int s2_canvas_w = 0;
    int s2_canvas_h = 0;
    int d_canvas_w = 0;
    int d_canvas_h = 0;
    int is_rgb_input = -1;
    int is_rgb_input2 = -1;
    int is_rgb_output = -1;
    int bpp = 0;
    buffer_info_t* input_buffer_info = &pge2dinfo->src_info[0];
    buffer_info_t* input2_buffer_info = &pge2dinfo->src_info[1];
    buffer_info_t* output_buffer_info = &pge2dinfo->dst_info;
    /* src2 not support nv21/nv12/yv12, swap src1 and src2 */
    if (is_need_swap_src2(input2_buffer_info->format, input2_buffer_info, output_buffer_info)) {
        input_buffer_info = &pge2dinfo->src_info[1];
        input2_buffer_info = &pge2dinfo->src_info[0];
        b_src_swap = 1;
        D_GE2D("NOTE:src2 not support nv21/nv12, swap src1 and src2!\n");
    }
    else if ((cap_attr == 0) && (input_buffer_info->plane_alpha == 0xff) &&
        (input2_buffer_info->plane_alpha != 0xff) &&
        (input_buffer_info->layer_mode != LAYER_MODE_NON) &&
        (input2_buffer_info->layer_mode == LAYER_MODE_PREMULTIPLIED)) {
        /* swap src1 and src2 buffer to process plane alpha*/
        /* TODO: need check the src1 rect and src2 rect */
        input_buffer_info = &pge2dinfo->src_info[1];
        input2_buffer_info = &pge2dinfo->src_info[0];
        //pge2dinfo->gl_alpha &= SRC1_GB_ALPHA_ENABLE;
        //pge2dinfo->gl_alpha |= (input_buffer_info->plane_alpha & 0xff);
        b_src_swap = 1;
    }
    else
        b_src_swap = 0;
    D_GE2D("b_src_swap=%d\n",b_src_swap);
    memset(&ge2d_config_ex, 0, sizeof(struct config_para_ex_ion_s ));

    if (CANVAS_ALLOC == input_buffer_info->memtype) {
        is_rgb_input = pixel_to_ge2d_format(input_buffer_info->format,&src_format,&bpp);
        src_format |= GE2D_LITTLE_ENDIAN;
        if ((int)0xffffffff == src_format) {
            E_GE2D("can't get proper ge2d format\n" );
            return ge2d_fail;
        }
        ge2d_set_canvas(bpp,input_buffer_info->canvas_w,input_buffer_info->canvas_h,&s_canvas_w,&s_canvas_h);
    }
    if ((CANVAS_ALLOC == input2_buffer_info->memtype)) {
        is_rgb_input2 = pixel_to_ge2d_format(input2_buffer_info->format,&src2_format,&bpp);
        src2_format |= GE2D_LITTLE_ENDIAN;
        if ((int)0xffffffff == src2_format) {
            E_GE2D("can't get proper ge2d format\n" );
            return ge2d_fail;
        }
        ge2d_set_canvas(bpp,input2_buffer_info->canvas_w,input2_buffer_info->canvas_h,&s2_canvas_w,&s2_canvas_h);
    }

    if ((CANVAS_ALLOC == output_buffer_info->memtype)) {
        is_rgb_output = pixel_to_ge2d_format(output_buffer_info->format,&dst_format,&bpp);
        dst_format |= GE2D_LITTLE_ENDIAN;
        if ((int)0xffffffff == dst_format) {
            E_GE2D("can't get proper ge2d format\n" );
            return ge2d_fail;
        }
        ge2d_set_canvas(bpp,output_buffer_info->canvas_w,output_buffer_info->canvas_h,&d_canvas_w,&d_canvas_h);

    }
    D_GE2D("ge2d_blit_config_ex,memtype=%x,src_format=%x,s_canvas_w=%d,s_canvas_h=%d,rotation=%d\n",
        input_buffer_info->memtype,src_format,s_canvas_w,s_canvas_h,input_buffer_info->rotation);

    D_GE2D("ge2d_blit_config_ex,memtype=%x,src2_format=%x,s2_canvas_w=%d,s2_canvas_h=%d,rotation=%d\n",
        input2_buffer_info->memtype,src2_format,s2_canvas_w,s2_canvas_h,input2_buffer_info->rotation);

    D_GE2D("ge2d_blit_config_ex,memtype=%x,dst_format=%x,d_canvas_w=%d,d_canvas_h=%d,rotation=%d\n",
        output_buffer_info->memtype,dst_format,d_canvas_w,d_canvas_h,output_buffer_info->rotation);

    ge2d_config_ex.src_para.mem_type = input_buffer_info->memtype;
    ge2d_config_ex.src_para.format = src_format;
    ge2d_config_ex.src_para.left = input_buffer_info->rect.x;
    ge2d_config_ex.src_para.top = input_buffer_info->rect.y;
    ge2d_config_ex.src_para.width = input_buffer_info->rect.w;
    ge2d_config_ex.src_para.height = input_buffer_info->rect.h;
    ge2d_config_ex.src_para.color = input_buffer_info->def_color;
    ge2d_config_ex.src_para.fill_color_en = input_buffer_info->fill_color_en;

    if (input_buffer_info->layer_mode == LAYER_MODE_PREMULTIPLIED)
        ge2d_config_ex.src1_cmult_asel = 2;
    else if (input_buffer_info->layer_mode == LAYER_MODE_COVERAGE)
        ge2d_config_ex.src1_cmult_asel = 0;
    else if (input_buffer_info->layer_mode == LAYER_MODE_NON)
        ge2d_config_ex.src1_cmult_asel = 0;

    ge2d_config_ex.src2_para.mem_type = input2_buffer_info->memtype;
    ge2d_config_ex.src2_para.format = src2_format;
    ge2d_config_ex.src2_para.left = input2_buffer_info->rect.x;
    ge2d_config_ex.src2_para.top = input2_buffer_info->rect.y;
    ge2d_config_ex.src2_para.width = input2_buffer_info->rect.w;
    ge2d_config_ex.src2_para.height = input2_buffer_info->rect.h;
    ge2d_config_ex.src2_para.color = input2_buffer_info->def_color;
    ge2d_config_ex.src2_para.fill_color_en = input2_buffer_info->fill_color_en;
    if (cap_attr == 0x1) {
        if (input2_buffer_info->layer_mode == LAYER_MODE_PREMULTIPLIED)
            ge2d_config_ex.src2_cmult_asel = 2;
        else if (input2_buffer_info->layer_mode == LAYER_MODE_COVERAGE)
            ge2d_config_ex.src2_cmult_asel = 1;
        else if (input2_buffer_info->layer_mode == LAYER_MODE_NON)
            ge2d_config_ex.src2_cmult_asel= 0;
        if ((input_buffer_info->layer_mode == LAYER_MODE_PREMULTIPLIED) && (input2_buffer_info->layer_mode == LAYER_MODE_PREMULTIPLIED))
            ge2d_config_ex.src2_cmult_ad = 0;
        else if ((input_buffer_info->layer_mode == LAYER_MODE_COVERAGE) && (input2_buffer_info->layer_mode == LAYER_MODE_PREMULTIPLIED))
            ge2d_config_ex.src2_cmult_ad = 0;
        else if ((input_buffer_info->layer_mode == LAYER_MODE_PREMULTIPLIED) && (input2_buffer_info->layer_mode == LAYER_MODE_COVERAGE))
            ge2d_config_ex.src2_cmult_ad = 1;
        else if ((input_buffer_info->layer_mode == LAYER_MODE_COVERAGE) && (input2_buffer_info->layer_mode == LAYER_MODE_COVERAGE))
            ge2d_config_ex.src2_cmult_ad = 1;
        else if ((input_buffer_info->layer_mode == LAYER_MODE_COVERAGE) && (input2_buffer_info->layer_mode == LAYER_MODE_NON))
            ge2d_config_ex.src2_cmult_ad = 3;
        else
            ge2d_config_ex.src2_cmult_ad = 0;
    } else
        ge2d_config_ex.src2_cmult_asel= 0;
    ge2d_config_ex.dst_para.mem_type = output_buffer_info->memtype;
    ge2d_config_ex.dst_para.format = dst_format;
    ge2d_config_ex.dst_para.left = 0;
    ge2d_config_ex.dst_para.top = 0;
    ge2d_config_ex.dst_para.width = d_canvas_w;
    ge2d_config_ex.dst_para.height = d_canvas_h;
    switch (pge2dinfo->src_info[0].rotation) {
        case GE2D_ROTATION_0:
            break;
        case GE2D_ROTATION_90:
            ge2d_config_ex.dst_xy_swap = 1;
            ge2d_config_ex.src_para.y_rev = 1;
            break;
        case GE2D_ROTATION_180:
            ge2d_config_ex.src_para.x_rev = 1;
            ge2d_config_ex.src_para.y_rev = 1;
            break;
        case GE2D_ROTATION_270:
            ge2d_config_ex.dst_xy_swap = 1;
            ge2d_config_ex.src_para.x_rev = 1;
            break;
        default:
            break;
    }

    switch (pge2dinfo->src_info[1].rotation) {
        case GE2D_ROTATION_0:
            break;
        case GE2D_ROTATION_90:
            ge2d_config_ex.dst_xy_swap = 1;
            ge2d_config_ex.src2_para.y_rev = 1;
            break;
        case GE2D_ROTATION_180:
            ge2d_config_ex.src2_para.x_rev = 1;
            ge2d_config_ex.src2_para.y_rev = 1;
            break;
        case GE2D_ROTATION_270:
            ge2d_config_ex.dst_xy_swap = 1;
            ge2d_config_ex.src2_para.x_rev = 1;
            break;
        default:
            break;
    }

    switch (pge2dinfo->dst_info.rotation) {
         case GE2D_ROTATION_0:
             break;
         case GE2D_ROTATION_90:
             ge2d_config_ex.dst_xy_swap = 1;
             ge2d_config_ex.dst_para.x_rev = 1;
             break;
         case GE2D_ROTATION_180:
             ge2d_config_ex.dst_para.x_rev = 1;
             ge2d_config_ex.dst_para.y_rev = 1;
             break;
         case GE2D_ROTATION_270:
             ge2d_config_ex.dst_xy_swap = 1;
             ge2d_config_ex.dst_para.y_rev = 1;
             break;
         default:
             break;
    }

    if (CANVAS_ALLOC == input_buffer_info->memtype) {
        if (is_rgb_input) {
            ge2d_config_ex.src_planes[0].addr = input_buffer_info->offset;
            ge2d_config_ex.src_planes[0].shared_fd = input_buffer_info->shared_fd;
            ge2d_config_ex.src_planes[0].w = s_canvas_w;
            ge2d_config_ex.src_planes[0].h = s_canvas_h;
        } else if (input_buffer_info->format == PIXEL_FORMAT_YCrCb_420_SP) {
            ge2d_config_ex.src_planes[0].addr = input_buffer_info->offset;
            ge2d_config_ex.src_planes[0].shared_fd = input_buffer_info->shared_fd;
            ge2d_config_ex.src_planes[0].w = s_canvas_w;
            ge2d_config_ex.src_planes[0].h = s_canvas_h;
            ge2d_config_ex.src_planes[1].addr = (s_canvas_w * s_canvas_h);
            ge2d_config_ex.src_planes[1].w = s_canvas_w;
            ge2d_config_ex.src_planes[1].h = s_canvas_h/2;
        } else if (input_buffer_info->format == PIXEL_FORMAT_Y8) {
            ge2d_config_ex.src_planes[0].addr = input_buffer_info->offset;
            ge2d_config_ex.src_planes[0].shared_fd = input_buffer_info->shared_fd;
            ge2d_config_ex.src_planes[0].w = s_canvas_w;
            ge2d_config_ex.src_planes[0].h = s_canvas_h;
        } else if (input_buffer_info->format == PIXEL_FORMAT_YV12) {
            ge2d_config_ex.src_planes[0].addr = input_buffer_info->offset;
            ge2d_config_ex.src_planes[0].shared_fd = input_buffer_info->shared_fd;
            ge2d_config_ex.src_planes[0].w = s_canvas_w;
            ge2d_config_ex.src_planes[0].h = s_canvas_h;
            /* android is ycrcb,kernel is ycbcr,swap the addr */
            ge2d_config_ex.src_planes[1].addr = YV12_Y_ALIGNED(s_canvas_w) *
                s_canvas_h + CANVAS_ALIGNED(s_canvas_w/2 ) * s_canvas_h/2;
            ge2d_config_ex.src_planes[1].w = CANVAS_ALIGNED(s_canvas_w/2);
            ge2d_config_ex.src_planes[1].h = s_canvas_h/2;
            ge2d_config_ex.src_planes[2].addr = YV12_Y_ALIGNED(s_canvas_w) * s_canvas_h;
            ge2d_config_ex.src_planes[2].w = CANVAS_ALIGNED(s_canvas_w/2);
            ge2d_config_ex.src_planes[2].h = s_canvas_h/2;
            if ( !(d_canvas_w % 64) || !(d_canvas_w % 32))
                D_GE2D("yv12 src canvas_w not alligned\n ");
        } else if (input_buffer_info->format == PIXEL_FORMAT_YCbCr_422_SP) {
            ge2d_config_ex.src_planes[0].addr = input_buffer_info->offset;
            ge2d_config_ex.src_planes[0].shared_fd = input_buffer_info->shared_fd;
            ge2d_config_ex.src_planes[0].w = s_canvas_w;
            ge2d_config_ex.src_planes[0].h = s_canvas_h;
            ge2d_config_ex.src_planes[1].addr = (s_canvas_w * s_canvas_h);
            ge2d_config_ex.src_planes[1].w = s_canvas_w;
            ge2d_config_ex.src_planes[1].h = s_canvas_h;
        } else if (input_buffer_info->format == PIXEL_FORMAT_YCbCr_422_I) {
            ge2d_config_ex.src_planes[0].addr = input_buffer_info->offset;
            ge2d_config_ex.src_planes[0].shared_fd = input_buffer_info->shared_fd;
            ge2d_config_ex.src_planes[0].w = s_canvas_w*2;
            ge2d_config_ex.src_planes[0].h = s_canvas_h;
        }
        else {
            E_GE2D("format is not match, should config src_planes correct.\n");
            return ge2d_fail;
        }
    }


    if (CANVAS_ALLOC == input2_buffer_info->memtype) {
        if (is_rgb_input2) {
            ge2d_config_ex.src2_planes[0].addr = input2_buffer_info->offset;
            ge2d_config_ex.src2_planes[0].shared_fd = input2_buffer_info->shared_fd;
            ge2d_config_ex.src2_planes[0].w = s2_canvas_w;
            ge2d_config_ex.src2_planes[0].h = s2_canvas_h;
        } else if (input2_buffer_info->format == PIXEL_FORMAT_YCrCb_420_SP) {
            ge2d_config_ex.src2_planes[0].addr = input2_buffer_info->offset;
            ge2d_config_ex.src2_planes[0].shared_fd = input2_buffer_info->shared_fd;
            ge2d_config_ex.src2_planes[0].w = s2_canvas_w;
            ge2d_config_ex.src2_planes[0].h = s2_canvas_h;
            ge2d_config_ex.src2_planes[1].addr = (s2_canvas_w * s2_canvas_h);
            ge2d_config_ex.src2_planes[1].w = s2_canvas_w;
            ge2d_config_ex.src2_planes[1].h = s2_canvas_h/2;
        } else if (input2_buffer_info->format == PIXEL_FORMAT_Y8) {
            ge2d_config_ex.src2_planes[0].addr = input2_buffer_info->offset;
            ge2d_config_ex.src2_planes[0].shared_fd = input2_buffer_info->shared_fd;
            ge2d_config_ex.src2_planes[0].w = s2_canvas_w;
            ge2d_config_ex.src2_planes[0].h = s2_canvas_h;
        } else if (input2_buffer_info->format == PIXEL_FORMAT_YV12) {
            ge2d_config_ex.src2_planes[0].addr = input2_buffer_info->offset;
            ge2d_config_ex.src2_planes[0].shared_fd = input2_buffer_info->shared_fd;
            ge2d_config_ex.src2_planes[0].w = s2_canvas_w;
            ge2d_config_ex.src2_planes[0].h = s2_canvas_h;
            /* android is ycrcb,kernel is ycbcr,swap the addr */
            ge2d_config_ex.src2_planes[1].addr = YV12_Y_ALIGNED(s2_canvas_w) *
                s2_canvas_h + CANVAS_ALIGNED(s2_canvas_w/2)* s2_canvas_h/2;
            ge2d_config_ex.src2_planes[1].w = CANVAS_ALIGNED(s2_canvas_w/2);
            ge2d_config_ex.src2_planes[1].h = s2_canvas_h/2;
            ge2d_config_ex.src2_planes[2].addr = YV12_Y_ALIGNED(s2_canvas_w) * s2_canvas_h;
            ge2d_config_ex.src2_planes[2].w = CANVAS_ALIGNED(s2_canvas_w/2);
            ge2d_config_ex.src2_planes[2].h = s2_canvas_h/2;
            if ( !(d_canvas_w % 64) || !(d_canvas_w % 32))
                D_GE2D("yv12 src2 canvas_w not alligned\n ");
        } else if (input2_buffer_info->format == PIXEL_FORMAT_YCbCr_422_SP) {
            ge2d_config_ex.src2_planes[0].addr = input2_buffer_info->offset;
            ge2d_config_ex.src2_planes[0].shared_fd = input2_buffer_info->shared_fd;
            ge2d_config_ex.src2_planes[0].w = s2_canvas_w;
            ge2d_config_ex.src2_planes[0].h = s2_canvas_h;
            ge2d_config_ex.src2_planes[1].addr = (s2_canvas_w * s2_canvas_h);
            ge2d_config_ex.src2_planes[1].w = s2_canvas_w;
            ge2d_config_ex.src2_planes[1].h = s2_canvas_h;
        } else if (input2_buffer_info->format == PIXEL_FORMAT_YCbCr_422_I) {
            ge2d_config_ex.src2_planes[0].addr = input2_buffer_info->offset;
            ge2d_config_ex.src2_planes[0].shared_fd = input2_buffer_info->shared_fd;
            ge2d_config_ex.src2_planes[0].w = s2_canvas_w*2;
            ge2d_config_ex.src2_planes[0].h = s2_canvas_h;
        }
        else {
            E_GE2D("format is not match, should config src2_planes correct.\n");
            return ge2d_fail;
        }
    }

    if (CANVAS_ALLOC == output_buffer_info->memtype) {
        if (is_rgb_output) {
            ge2d_config_ex.dst_planes[0].addr = output_buffer_info->offset;
            ge2d_config_ex.dst_planes[0].shared_fd = output_buffer_info->shared_fd;
            ge2d_config_ex.dst_planes[0].w = d_canvas_w;
            ge2d_config_ex.dst_planes[0].h = d_canvas_h;
        } else if (output_buffer_info->format == PIXEL_FORMAT_YCrCb_420_SP) {
            ge2d_config_ex.dst_planes[0].addr = output_buffer_info->offset;
            ge2d_config_ex.dst_planes[0].shared_fd = output_buffer_info->shared_fd;
            ge2d_config_ex.dst_planes[0].w = d_canvas_w;
            ge2d_config_ex.dst_planes[0].h = d_canvas_h;
            ge2d_config_ex.dst_planes[1].addr = (d_canvas_w * d_canvas_h);
            ge2d_config_ex.dst_planes[1].w = d_canvas_w;
            ge2d_config_ex.dst_planes[1].h = d_canvas_h/2;
        }  else if (output_buffer_info->format == PIXEL_FORMAT_Y8) {
            ge2d_config_ex.dst_planes[0].addr = output_buffer_info->offset;
            ge2d_config_ex.dst_planes[0].shared_fd = output_buffer_info->shared_fd;
            ge2d_config_ex.dst_planes[0].w = d_canvas_w;
            ge2d_config_ex.dst_planes[0].h = d_canvas_h;
        } else if (output_buffer_info->format == PIXEL_FORMAT_YV12) {
            ge2d_config_ex.dst_planes[0].addr = output_buffer_info->offset;
            ge2d_config_ex.dst_planes[0].shared_fd = output_buffer_info->shared_fd;
            ge2d_config_ex.dst_planes[0].w = d_canvas_w;
            ge2d_config_ex.dst_planes[0].h = d_canvas_h;
            /* android is ycrcb,kernel is ycbcr,swap the addr */
            ge2d_config_ex.dst_planes[1].addr = YV12_Y_ALIGNED(d_canvas_w) *
                d_canvas_h + CANVAS_ALIGNED(d_canvas_w/2) * d_canvas_h/2;
            ge2d_config_ex.dst_planes[1].w = CANVAS_ALIGNED(d_canvas_w/2);
            ge2d_config_ex.dst_planes[1].h = d_canvas_h/2;
            ge2d_config_ex.dst_planes[2].addr = YV12_Y_ALIGNED(d_canvas_w) * d_canvas_h;
            ge2d_config_ex.dst_planes[2].w = CANVAS_ALIGNED(d_canvas_w/2);
            ge2d_config_ex.dst_planes[2].h = d_canvas_h/2;
            if ( !(d_canvas_w % 64) || !(d_canvas_w % 32))
                D_GE2D("yv12 dst canvas_w not alligned\n ");
        } else if (output_buffer_info->format == PIXEL_FORMAT_YCbCr_422_SP) {
            ge2d_config_ex.dst_planes[0].addr = output_buffer_info->offset;
            ge2d_config_ex.dst_planes[0].shared_fd = output_buffer_info->shared_fd;
            ge2d_config_ex.dst_planes[0].w = d_canvas_w;
            ge2d_config_ex.dst_planes[0].h = d_canvas_h;
            ge2d_config_ex.dst_planes[1].addr = (d_canvas_w * d_canvas_h);
            ge2d_config_ex.dst_planes[1].w = d_canvas_w;
            ge2d_config_ex.dst_planes[1].h = d_canvas_h;
        } else if (output_buffer_info->format == PIXEL_FORMAT_YCbCr_422_I) {
            ge2d_config_ex.dst_planes[0].addr = output_buffer_info->offset;
            ge2d_config_ex.dst_planes[0].shared_fd = output_buffer_info->shared_fd;
            ge2d_config_ex.dst_planes[0].w = d_canvas_w*2;
            ge2d_config_ex.dst_planes[0].h = d_canvas_h;
        }
        else {
            E_GE2D("format is not match, should config dst_planes correct.\n");
            return ge2d_fail;
        }
    }
    ge2d_config_ex.alu_const_color = pge2dinfo->const_color;
    ge2d_config_ex.src1_gb_alpha = input_buffer_info->plane_alpha & 0xff;
    ge2d_config_ex.src1_gb_alpha_en = 1;
    if (cap_attr == 0x1) {
        ge2d_config_ex.src2_gb_alpha = input2_buffer_info->plane_alpha & 0xff;
        ge2d_config_ex.src2_gb_alpha_en = 1;
    }
    D_GE2D("blend:src1_cmult_asel=%d,src2_cmult_ase2=%d,gl_alpha=%x,src1_gb_alpha_en=%d, src2_gb_alpha=%x, src2_gb_alpha_en=%d, src2_cmult_ad=%d\n",
        ge2d_config_ex.src1_cmult_asel,
        ge2d_config_ex.src2_cmult_asel,
        ge2d_config_ex.src1_gb_alpha,
        ge2d_config_ex.src1_gb_alpha_en,
        ge2d_config_ex.src2_gb_alpha,
         ge2d_config_ex.src2_gb_alpha_en,
        ge2d_config_ex.src2_cmult_ad);
    if (cap_attr == -1)
        ret = ioctl(fd, GE2D_CONFIG_EX_ION_EN, &ge2d_config_ex);
    else
        ret = ioctl(fd, GE2D_CONFIG_EX_ION, &ge2d_config_ex);
    if (ret < 0) {
        E_GE2D("ge2d config ex ion failed. \n");
        return ge2d_fail;
    }
    return ge2d_success;
}

static int ge2d_fillrectangle(int fd,rectangle_t *rect,unsigned int color)
{
    int ret;
    ge2d_op_para_t op_ge2d_info;
    memset(&op_ge2d_info, 0, sizeof(ge2d_op_para_t));
    D_GE2D("ge2d_fillrectangle:rect x is %d, y is %d, w is %d, h is %d\n",
        rect->x, rect->y, rect->w, rect->h);
    D_GE2D("color is %d\n", color);

    op_ge2d_info.src1_rect.x = rect->x;
    op_ge2d_info.src1_rect.y = rect->y;
    op_ge2d_info.src1_rect.w = rect->w;
    op_ge2d_info.src1_rect.h = rect->h;

    op_ge2d_info.dst_rect.x = rect->x;
    op_ge2d_info.dst_rect.y = rect->y;
    op_ge2d_info.dst_rect.w = rect->w;
    op_ge2d_info.dst_rect.h = rect->h;
    op_ge2d_info.color = color;

    ret = ioctl(fd, GE2D_FILLRECTANGLE, &op_ge2d_info);
    if (ret != 0) {
        E_GE2D("%s,%d,ret %d,ioctl failed!\n",__FUNCTION__,__LINE__, ret);
        return ge2d_fail;
    }
    return ge2d_success;
}



static int ge2d_blit(int fd,rectangle_t *rect,unsigned int dx,unsigned int dy)
{
    int ret;
    ge2d_op_para_t op_ge2d_info;
    memset(&op_ge2d_info, 0, sizeof(ge2d_op_para_t));
    D_GE2D("ge2d_blit:rect x is %d, y is %d, w is %d, h is %d\n",
        rect->x, rect->y, rect->w, rect->h);
    D_GE2D("dx is %d, dy is %d\n", dx, dy);

    op_ge2d_info.src1_rect.x = rect->x;
    op_ge2d_info.src1_rect.y = rect->y;
    op_ge2d_info.src1_rect.w = rect->w;
    op_ge2d_info.src1_rect.h = rect->h;

    op_ge2d_info.dst_rect.x = dx;
    op_ge2d_info.dst_rect.y = dy;
    op_ge2d_info.dst_rect.w = rect->w;
    op_ge2d_info.dst_rect.h = rect->h;

    ret = ioctl(fd, GE2D_BLIT, &op_ge2d_info);
    if (ret != 0) {
        E_GE2D("%s,%d,ret %d,ioctl failed!\n",__FUNCTION__,__LINE__, ret);
        return ge2d_fail;
    }
    return ge2d_success;
}

static int ge2d_blit_noalpha(int fd,rectangle_t *rect,unsigned int dx,unsigned int dy)
{
    int ret;
    ge2d_op_para_t op_ge2d_info;
    memset(&op_ge2d_info, 0, sizeof(ge2d_op_para_t));
    D_GE2D("ge2d_blit_noalpha:rect x is %d, y is %d, w is %d, h is %d\n",
        rect->x, rect->y, rect->w, rect->h);

    D_GE2D("dx is %d, dy is %d\n", dx, dy);

    op_ge2d_info.src1_rect.x = rect->x;
    op_ge2d_info.src1_rect.y = rect->y;
    op_ge2d_info.src1_rect.w = rect->w;
    op_ge2d_info.src1_rect.h = rect->h;

    op_ge2d_info.dst_rect.x = dx;
    op_ge2d_info.dst_rect.y = dy;
    op_ge2d_info.dst_rect.w = rect->w;
    op_ge2d_info.dst_rect.h = rect->h;

    ret = ioctl(fd, GE2D_BLIT_NOALPHA, &op_ge2d_info);
    if ( ret != 0) {
        E_GE2D("%s,%d,ret %d,ioctl failed!\n",__FUNCTION__,__LINE__, ret);
        return ge2d_fail;
    }
    return ge2d_success;
}

static int ge2d_strechblit(int fd,rectangle_t *srect,rectangle_t *drect)
{
    int ret;
    ge2d_op_para_t op_ge2d_info;
    memset(&op_ge2d_info, 0, sizeof(ge2d_op_para_t));
    D_GE2D("stretchblit srect[%d %d %d %d] drect[%d %d %d %d]\n",
        srect->x,srect->y,srect->w,srect->h,drect->x,drect->y,drect->w,drect->h);

    op_ge2d_info.src1_rect.x = srect->x;
    op_ge2d_info.src1_rect.y = srect->y;
    op_ge2d_info.src1_rect.w = srect->w;
    op_ge2d_info.src1_rect.h = srect->h;

    op_ge2d_info.dst_rect.x = drect->x;
    op_ge2d_info.dst_rect.y = drect->y;
    op_ge2d_info.dst_rect.w = drect->w;
    op_ge2d_info.dst_rect.h = drect->h;

    ret = ioctl(fd, GE2D_STRETCHBLIT, &op_ge2d_info);
    if (ret != 0) {
        E_GE2D("%s,%d,ret %d,ioctl failed!\n",__FUNCTION__,__LINE__, ret);
        return ge2d_fail;
    }
    return ge2d_success;
}


static int ge2d_strechblit_noalpha(int fd,rectangle_t *srect,rectangle_t *drect)
{
    int ret;
    ge2d_op_para_t op_ge2d_info;
    memset(&op_ge2d_info, 0, sizeof(ge2d_op_para_t));
    D_GE2D("stretchblit srect[%d %d %d %d] drect[%d %d %d %d]\n",
        srect->x,srect->y,srect->w,srect->h,drect->x,drect->y,drect->w,drect->h);

    op_ge2d_info.src1_rect.x = srect->x;
    op_ge2d_info.src1_rect.y = srect->y;
    op_ge2d_info.src1_rect.w = srect->w;
    op_ge2d_info.src1_rect.h = srect->h;

    op_ge2d_info.dst_rect.x = drect->x;
    op_ge2d_info.dst_rect.y = drect->y;
    op_ge2d_info.dst_rect.w = drect->w;
    op_ge2d_info.dst_rect.h = drect->h;

    ret = ioctl(fd, GE2D_STRETCHBLIT_NOALPHA, &op_ge2d_info);
    if (ret != 0) {
        E_GE2D("%s,%d,ret %d,ioctl failed!\n",__FUNCTION__,__LINE__, ret);
        return ge2d_fail;
    }
    return ge2d_success;
}


static int ge2d_blend(int fd,rectangle_t *srect,rectangle_t *srect2,rectangle_t *drect, unsigned int op)
{
    int ret;
    ge2d_op_para_t op_ge2d_info;
    ge2d_blend_op blend_op;
    int max_d_w, max_d_h;
    D_GE2D("ge2d_blend srect[%d %d %d %d], s2rect[%d %d %d %d],drect[%d %d %d %d]\n",
        srect->x,srect->y,srect->w,srect->h,
        srect2->x,srect2->y,srect2->w,srect2->h,
        drect->x,drect->y,drect->w,drect->h);
    D_GE2D("ge2d_blend:blend_mode=%d\n",op);
    memset(&blend_op,0,sizeof(ge2d_blend_op));
    #if 0
    max_d_w = (srect->w > srect2->w) ? srect2->w : srect->w;
    max_d_h = (srect->h > srect2->h) ? srect2->h : srect->h;
    if ((drect->w > max_d_w) || (drect->h > max_d_h)) {
        /* dst rect must be min(srect,srect2),otherwise ge2d will timeout */
        E_GE2D("dst rect w=%d,h=%d out of range\n",drect->w,drect->h);
        return ge2d_fail;
    }
    #endif
    op_ge2d_info.src1_rect.x = srect->x;
    op_ge2d_info.src1_rect.y = srect->y;
    op_ge2d_info.src1_rect.w = srect->w;
    op_ge2d_info.src1_rect.h = srect->h;

    op_ge2d_info.src2_rect.x = srect2->x;
    op_ge2d_info.src2_rect.y = srect2->y;
    op_ge2d_info.src2_rect.w = srect2->w;
    op_ge2d_info.src2_rect.h = srect2->h;

    op_ge2d_info.dst_rect.x = drect->x;
    op_ge2d_info.dst_rect.y = drect->y;
    op_ge2d_info.dst_rect.w = drect->w;
    op_ge2d_info.dst_rect.h = drect->h;

    blend_op.color_blending_mode = OPERATION_ADD;
    blend_op.alpha_blending_mode = OPERATION_ADD;
    /* b_src_swap = 1:src1 & src2 swap, so blend factor used dst instead src */
    switch (op) {
        case BLEND_MODE_NONE:
            if (b_src_swap) {
                blend_op.color_blending_src_factor = COLOR_FACTOR_ZERO;
                blend_op.color_blending_dst_factor = COLOR_FACTOR_ONE;
                blend_op.alpha_blending_src_factor = ALPHA_FACTOR_ZERO;
                blend_op.alpha_blending_dst_factor = ALPHA_FACTOR_ONE;
            }
            else {
                blend_op.color_blending_src_factor = COLOR_FACTOR_ONE;
                blend_op.color_blending_dst_factor = COLOR_FACTOR_ZERO;
                blend_op.alpha_blending_src_factor = ALPHA_FACTOR_ONE;
                blend_op.alpha_blending_dst_factor = ALPHA_FACTOR_ZERO;
            }
            break;
        case BLEND_MODE_PREMULTIPLIED:
            if (b_src_swap) {
                blend_op.color_blending_src_factor = COLOR_FACTOR_ONE_MINUS_DST_ALPHA;
                blend_op.color_blending_dst_factor = COLOR_FACTOR_ONE;
                blend_op.alpha_blending_src_factor = ALPHA_FACTOR_ONE_MINUS_DST_ALPHA;
                blend_op.alpha_blending_dst_factor = ALPHA_FACTOR_ONE;
            }
            else {
                blend_op.color_blending_src_factor = COLOR_FACTOR_ONE;
                blend_op.color_blending_dst_factor = COLOR_FACTOR_ONE_MINUS_SRC_ALPHA;
                blend_op.alpha_blending_src_factor = ALPHA_FACTOR_ONE;
                blend_op.alpha_blending_dst_factor = ALPHA_FACTOR_ONE_MINUS_SRC_ALPHA;
            }
            break;
        case BLEND_MODE_COVERAGE:
            if (b_src_swap) {
                blend_op.color_blending_src_factor = COLOR_FACTOR_ONE_MINUS_DST_ALPHA;
                blend_op.color_blending_dst_factor = COLOR_FACTOR_DST_ALPHA;
                blend_op.alpha_blending_src_factor = ALPHA_FACTOR_ONE_MINUS_DST_ALPHA;
                blend_op.alpha_blending_dst_factor = ALPHA_FACTOR_DST_ALPHA;
            }
            else {
                blend_op.color_blending_src_factor = COLOR_FACTOR_SRC_ALPHA;
                blend_op.color_blending_dst_factor = COLOR_FACTOR_ONE_MINUS_SRC_ALPHA;
                blend_op.alpha_blending_src_factor = ALPHA_FACTOR_SRC_ALPHA;
                blend_op.alpha_blending_dst_factor = ALPHA_FACTOR_ONE_MINUS_SRC_ALPHA;
            }
            break;
        case BLEND_MODE_INVALID:
            return ge2d_fail;
    }

    op_ge2d_info.op = blendop(
            blend_op.color_blending_mode,
            blend_op.color_blending_src_factor,
            blend_op.color_blending_dst_factor,
            blend_op.alpha_blending_mode,
            blend_op.alpha_blending_src_factor,
            blend_op.alpha_blending_dst_factor);

    D_GE2D("ge2d_blend op_ge2d_info.op=%x\n",op_ge2d_info.op);
    ret = ioctl(fd, GE2D_BLEND, &op_ge2d_info);
    if (ret != 0) {
        E_GE2D("%s,%d,ret %d,ioctl failed!\n",__FUNCTION__,__LINE__, ret);
        return ge2d_fail;
    }
    return ge2d_success;
}


static int ge2d_blend_noalpha(int fd,rectangle_t *srect,rectangle_t *srect2,rectangle_t *drect, unsigned int op)
{
    int ret;
    ge2d_op_para_t op_ge2d_info;
    ge2d_blend_op blend_op;
    int max_d_w, max_d_h;
    D_GE2D("ge2d_blend srect[%d %d %d %d], s2rect[%d %d %d %d],drect[%d %d %d %d]\n",
        srect->x,srect->y,srect->w,srect->h,
        srect2->x,srect2->y,srect2->w,srect2->h,
        drect->x,drect->y,drect->w,drect->h);
    D_GE2D("ge2d_blend_noalpha:blend_mode=%d\n",op);
    memset(&blend_op,0,sizeof(ge2d_blend_op));
    #if 0
    max_d_w = (srect->w > srect2->w) ? srect2->w : srect->w;
    max_d_h = (srect->h > srect2->h) ? srect2->h : srect->h;
    if ((drect->w > max_d_w) || (drect->h > max_d_h)) {
        /* dst rect must be min(srect,srect2),otherwise ge2d will timeout */
        E_GE2D("dst rect w=%d,h=%d out of range\n",drect->w,drect->h);
        return ge2d_fail;
    }
    #endif
    op_ge2d_info.src1_rect.x = srect->x;
    op_ge2d_info.src1_rect.y = srect->y;
    op_ge2d_info.src1_rect.w = srect->w;
    op_ge2d_info.src1_rect.h = srect->h;

    op_ge2d_info.src2_rect.x = srect2->x;
    op_ge2d_info.src2_rect.y = srect2->y;
    op_ge2d_info.src2_rect.w = srect2->w;
    op_ge2d_info.src2_rect.h = srect2->h;

    op_ge2d_info.dst_rect.x = drect->x;
    op_ge2d_info.dst_rect.y = drect->y;
    op_ge2d_info.dst_rect.w = drect->w;
    op_ge2d_info.dst_rect.h = drect->h;

    blend_op.color_blending_mode = OPERATION_ADD;
    blend_op.alpha_blending_mode = OPERATION_ADD;
    /* b_src_swap = 1:src1 & src2 swap, so blend factor used dst instead src */
    switch (op) {
        case BLEND_MODE_NONE:
            if (b_src_swap) {
                blend_op.color_blending_src_factor = COLOR_FACTOR_ZERO;
                blend_op.color_blending_dst_factor = COLOR_FACTOR_ONE;
                blend_op.alpha_blending_src_factor = ALPHA_FACTOR_ZERO;
                blend_op.alpha_blending_dst_factor = ALPHA_FACTOR_ONE;
            }
            else {
                blend_op.color_blending_src_factor = COLOR_FACTOR_ONE;
                blend_op.color_blending_dst_factor = COLOR_FACTOR_ZERO;
                blend_op.alpha_blending_src_factor = ALPHA_FACTOR_ONE;
                blend_op.alpha_blending_dst_factor = ALPHA_FACTOR_ZERO;
            }
            break;
        case BLEND_MODE_PREMULTIPLIED:
            if (b_src_swap) {
                blend_op.color_blending_src_factor = COLOR_FACTOR_ONE_MINUS_DST_ALPHA;
                blend_op.color_blending_dst_factor = COLOR_FACTOR_ONE;
                blend_op.alpha_blending_src_factor = ALPHA_FACTOR_ONE_MINUS_DST_ALPHA;
                blend_op.alpha_blending_dst_factor = ALPHA_FACTOR_ONE;
            }
            else {
                blend_op.color_blending_src_factor = COLOR_FACTOR_ONE;
                blend_op.color_blending_dst_factor = COLOR_FACTOR_ONE_MINUS_SRC_ALPHA;
                blend_op.alpha_blending_src_factor = ALPHA_FACTOR_ONE;
                blend_op.alpha_blending_dst_factor = ALPHA_FACTOR_ONE_MINUS_SRC_ALPHA;
            }
            break;
        case BLEND_MODE_COVERAGE:
            if (b_src_swap) {
                blend_op.color_blending_src_factor = COLOR_FACTOR_ONE_MINUS_DST_ALPHA;
                blend_op.color_blending_dst_factor = COLOR_FACTOR_DST_ALPHA;
                blend_op.alpha_blending_src_factor = ALPHA_FACTOR_ONE_MINUS_SRC_ALPHA;
                blend_op.alpha_blending_dst_factor = ALPHA_FACTOR_DST_ALPHA;
            }
            else {
                blend_op.color_blending_src_factor = COLOR_FACTOR_SRC_ALPHA;
                blend_op.color_blending_dst_factor = COLOR_FACTOR_ONE_MINUS_SRC_ALPHA;
                blend_op.alpha_blending_src_factor = ALPHA_FACTOR_SRC_ALPHA;
                blend_op.alpha_blending_dst_factor = ALPHA_FACTOR_ONE_MINUS_SRC_ALPHA;
            }
            break;
        case BLEND_MODE_INVALID:
            return ge2d_fail;
    }


    op_ge2d_info.op = blendop(
            blend_op.color_blending_mode,
            blend_op.color_blending_src_factor,
            blend_op.color_blending_dst_factor,
            blend_op.alpha_blending_mode,
            blend_op.alpha_blending_src_factor,
            blend_op.alpha_blending_dst_factor);

    D_GE2D("ge2d_blend op_ge2d_info.op=%x\n",op_ge2d_info.op);
    ret = ioctl(fd, GE2D_BLEND_NOALPHA, &op_ge2d_info);
    if (ret != 0) {
        E_GE2D("%s,%d,ret %d,ioctl failed!\n",__FUNCTION__,__LINE__, ret);
        return ge2d_fail;
    }
    return ge2d_success;
}



int ge2d_open(void)
{
    int fd = -1;
    fd = open(FILE_NAME_GE2D, O_RDWR);
    if (fd < 0) {
        E_GE2D("open %s failed!error no %d\n",FILE_NAME_GE2D,errno);
    }
    return fd;
}



int ge2d_close(int fd)
{
    int ret = -1;
    ret = close(fd);
    if (ret < 0)
        return -errno;
    return ret;
}

int ge2d_process(int fd,aml_ge2d_info_t *pge2dinfo)
{
    rectangle_t src_rect[2];
    rectangle_t dst_rect;
    int dx = 0, dy = 0;
    int ret = -1;

    if (!pge2dinfo) {
        E_GE2D("pge2dinfo is NULL!\n");
        return ge2d_fail;
    }

    switch (pge2dinfo->ge2d_op) {
        case AML_GE2D_FILLRECTANGLE:
            dst_rect.w = pge2dinfo->dst_info.rect.w;
            dst_rect.h = pge2dinfo->dst_info.rect.h;
            dst_rect.x =  pge2dinfo->dst_info.rect.x;
            dst_rect.y = pge2dinfo->offset + pge2dinfo->dst_info.rect.y;
            ret = ge2d_fillrectangle_config_ex_ion(fd,pge2dinfo);
            if (ret == ge2d_success)
                ge2d_fillrectangle(fd,&dst_rect,pge2dinfo->color);
            break;
        case AML_GE2D_BLIT:
            if (!is_rect_valid(&pge2dinfo->src_info[0]))
                return ge2d_fail;
            if (!is_rect_valid(&pge2dinfo->dst_info))
                return ge2d_fail;

            dx = pge2dinfo->dst_info.rect.x;
            dy = pge2dinfo->offset + pge2dinfo->dst_info.rect.y;
            ret = ge2d_blit_config_ex_ion(fd,pge2dinfo);
            if (ret == ge2d_success) {
                if (is_no_alpha(pge2dinfo->src_info[0].format))
                    ge2d_blit_noalpha(fd,&pge2dinfo->src_info[0].rect,dx,dy);
                else
                    ge2d_blit(fd,&pge2dinfo->src_info[0].rect,dx,dy);
            }
            break;
        case AML_GE2D_STRETCHBLIT:
            if (!is_rect_valid(&pge2dinfo->src_info[0]))
                return ge2d_fail;
            dst_rect.w = pge2dinfo->dst_info.rect.w;
            dst_rect.h = pge2dinfo->dst_info.rect.h;
            dst_rect.x =  pge2dinfo->dst_info.rect.x;
            dst_rect.y = pge2dinfo->offset + pge2dinfo->dst_info.rect.y;
            ret = ge2d_blit_config_ex_ion(fd,pge2dinfo);
            if (ret == ge2d_success) {
                if (is_no_alpha(pge2dinfo->src_info[0].format))
                    ge2d_strechblit_noalpha(fd,&pge2dinfo->src_info[0].rect,&dst_rect);
                else
                    ge2d_strechblit(fd,&pge2dinfo->src_info[0].rect,&dst_rect);
            }
            break;
        case AML_GE2D_BLEND:
            if ((pge2dinfo->dst_info.memtype == CANVAS_OSD0) && (pge2dinfo->src_info[1].memtype == CANVAS_OSD0)) {
                memcpy(&pge2dinfo->src_info[1],&pge2dinfo->dst_info,sizeof(buffer_info_t));
                pge2dinfo->src_info[1].rect.y = pge2dinfo->offset + pge2dinfo->src_info[1].rect.y;
            }
            if (!is_rect_valid(&pge2dinfo->src_info[0]))
                return ge2d_fail;
            if (!is_rect_valid(&pge2dinfo->src_info[1]))
                return ge2d_fail;
            if (!is_rect_valid(&pge2dinfo->dst_info))
                return ge2d_fail;
            if (pge2dinfo->src_info[0].layer_mode == LAYER_MODE_PREMULTIPLIED)
                pge2dinfo->blend_mode = BLEND_MODE_PREMULTIPLIED;
            else if (pge2dinfo->src_info[0].layer_mode == LAYER_MODE_COVERAGE)
                pge2dinfo->blend_mode = BLEND_MODE_COVERAGE;
            else
                pge2dinfo->blend_mode = BLEND_MODE_NONE;
            dst_rect.w = pge2dinfo->dst_info.rect.w;
            dst_rect.h = pge2dinfo->dst_info.rect.h;
            dst_rect.x =  pge2dinfo->dst_info.rect.x;
            dst_rect.y = pge2dinfo->offset + pge2dinfo->dst_info.rect.y;
            ret = ge2d_blend_config_ex_ion(fd,pge2dinfo);
            if (ret == ge2d_success) {
                if ((is_no_alpha(pge2dinfo->src_info[0].format))
                    || (is_no_alpha(pge2dinfo->src_info[1].format))
                    || (pge2dinfo->src_info[0].layer_mode == LAYER_MODE_NON)) {
                    if (b_src_swap)
                        ge2d_blend_noalpha(fd,&(pge2dinfo->src_info[1].rect),
                            &(pge2dinfo->src_info[0].rect),
                            &dst_rect,pge2dinfo->blend_mode);
                    else
                        ge2d_blend_noalpha(fd,&(pge2dinfo->src_info[0].rect),
                            &(pge2dinfo->src_info[1].rect),
                            &dst_rect,pge2dinfo->blend_mode);
                } else {
                    if (b_src_swap)
                        ge2d_blend(fd,&(pge2dinfo->src_info[1].rect),
                            &(pge2dinfo->src_info[0].rect),
                            &dst_rect,pge2dinfo->blend_mode);
                    else
                        ge2d_blend(fd,&(pge2dinfo->src_info[0].rect),
                            &(pge2dinfo->src_info[1].rect),
                            &dst_rect,pge2dinfo->blend_mode);
                }
            }
            break;
        default:
            E_GE2D("ge2d(%d) opration not support!\n",pge2dinfo->ge2d_op);
            return ge2d_fail;
    }

    return ge2d_success;
}


