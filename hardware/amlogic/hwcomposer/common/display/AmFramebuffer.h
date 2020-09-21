/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef AM_FRAMEBUFFER_H
#define AM_FRAMEBUFFER_H
#include <linux/fb.h>
#include <linux/ioctl.h>

/*
Amlogic defined ioctl, masks.
*/
#define HWC_ALIGN( value, base ) (((value) + ((base) - 1)) & ~((base) - 1))

#define DISPLAY_MODE_LEN (64)
#define AXIS_STR_LEN            (32)

enum {
    OSD_HW_CURSOR     = (1 << 0),
    OSD_UBOOT_LOGO    = (1 << 1),
    OSD_ZORDER        = (1 << 2),
    OSD_PRIMARY       = (1 << 3),
    OSD_FREESCALE     = (1 << 4),
    OSD_AFBC          = (1 << 5),
    OSD_VIU2          = (1 << 29),
    OSD_VIU1          = (1 << 30),
    OSD_LAYER_ENABLE  = (1 << 31)
};

enum {
    GLES_COMPOSE_MODE = 0,
    DIRECT_COMPOSE_MODE = 1,
    GE2D_COMPOSE_MODE = 2,
};

enum {
    OSD_BLANK_OP_BIT = 0x00000001,
};

enum {
    OSD_SYNC_REQUEST_MAGIC            = 0x54376812,
    OSD_SYNC_REQUEST_RENDER_MAGIC_V1  = 0x55386816,
    OSD_SYNC_REQUEST_RENDER_MAGIC_V2  = 0x55386817,
};

/*Osd ioctl*/
#define FB_IOC_MAGIC   'O'

#define FBIO_WAITFORVSYNC_64        _IOW('F', 0x21, __u32)
#define FBIOPUT_OSD_CURSOR          _IOWR(FB_IOC_MAGIC, 0x0,  struct fb_cursor)

#define FBIOPUT_OSD_REVERSE          0x4515
#define FBIOPUT_OSD_SYNC_BLANK       0x451c
#define FBIOPUT_OSD_SYNC_RENDER_ADD  0x4519
#define FBIOPUT_OSD_HWC_ENABLE       0x451a
#define FBIOGET_OSD_CAPBILITY        0x451e
#define FBIOPUT_OSD_DO_HWC 0x451b

/*AmVideo ioctl*/
#define AMSTREAM_IOC_MAGIC  'S'
#define AMSTREAM_IOC_GLOBAL_GET_VIDEO_OUTPUT \
    _IOR(AMSTREAM_IOC_MAGIC, 0x21, int)
#define AMSTREAM_IOC_GLOBAL_SET_VIDEO_OUTPUT \
    _IOW(AMSTREAM_IOC_MAGIC, 0x22, int)
#define AMSTREAM_IOC_GET_VIDEO_DISABLE_MODE \
    _IOR((AMSTREAM_IOC_MAGIC), 0x48, int)
#define AMSTREAM_IOC_SET_VIDEO_DISABLE_MODE \
    _IOW((AMSTREAM_IOC_MAGIC), 0x49, int)
#define AMSTREAM_IOC_GET_OMX_INFO \
    _IOR((AMSTREAM_IOC_MAGIC), 0xb2, unsigned int)

#define AMSTREAM_IOC_GLOBAL_GET_VIDEOPIP_OUTPUT \
    _IOR(AMSTREAM_IOC_MAGIC, 0x2b, int)
#define AMSTREAM_IOC_GLOBAL_SET_VIDEOPIP_OUTPUT \
    _IOW(AMSTREAM_IOC_MAGIC, 0x2c, int)
#define AMSTREAM_IOC_GET_VIDEOPIP_DISABLE \
    _IOR((AMSTREAM_IOC_MAGIC), 0x2d, int)
#define AMSTREAM_IOC_SET_VIDEOPIP_DISABLE \
    _IOW((AMSTREAM_IOC_MAGIC), 0x2e, int)

#define AMSTREAM_IOC_SET_ZORDER \
    _IOW((AMSTREAM_IOC_MAGIC), 0x38, unsigned int)
#define AMSTREAM_IOC_SET_PIP_ZORDER \
    _IOW((AMSTREAM_IOC_MAGIC), 0x36, unsigned int)

/*Legacy fb sysfs*/
#define SYSFS_DISPLAY_MODE              "/sys/class/display/mode"
#define SYSFS_DISPLAY_AXIS              "/sys/class/display/axis"
#define SYSFS_VIDEO_AXIS                "/sys/class/video/axis"
#define SYSFS_VIDEO_AXIS_PIP            "/sys/class/video/axis_pip"
#define SYSFS_VIDEO_CROP                "/sys/class/video/crop"
#define SYSFS_VIDEO_CROP_PIP                "/sys/class/video/crop_pip"
#define SYSFS_PPMGR_ANGLE                   "/sys/class/ppmgr/angle"
#define DISPLAY_FB1_SCALE_AXIS          "/sys/class/graphics/fb1/scale_axis"
#define DISPLAY_FB1_SCALE               "/sys/class/graphics/fb1/scale"

/*legacy logo switch*/
#define DISPLAY_LOGO_INDEX              "/sys/module/fb/parameters/osd_logo_index"
#define DISPLAY_FB0_FREESCALE_SWTICH    "/sys/class/graphics/fb0/free_scale_switch"

/*plane struct*/
typedef struct osd_plane_info_t {
    int             magic;
    int             len;
    unsigned int    xoffset;
    unsigned int    yoffset;
    int             in_fen_fd;
    int             out_fen_fd;
    int             width;
    int             height;
    int             format;
    int             shared_fd;
    unsigned int    op;
    unsigned int    type; /*direct render or ge2d*/
    unsigned int    dst_x;
    unsigned int    dst_y;
    unsigned int    dst_w;
    unsigned int    dst_h;
    int             byte_stride;
    int             pixel_stride;
    int             afbc_inter_format;
    unsigned int    zorder;
    unsigned int    blend_mode;
    unsigned char   plane_alpha;
    unsigned char   dim_layer;
    unsigned int    dim_color;
    int             fb_width;
    int             fb_height;
    int             reserve;
} osd_plane_info_t;

typedef struct cursor_plane_info_t {
    int             fbSize;
    int             transform;
    unsigned int    dst_x, dst_y;
    unsigned int    zorder;

    //buffer handle
    int             format;
    int             shared_fd;
    int             stride;
    int             buf_w, buf_h;

    //information get from osd
    struct fb_var_screeninfo info;
    struct fb_fix_screeninfo finfo;
} cursor_plane_info_t;

typedef struct osd_page_flip_info_t {
    int           out_fen_fd;
    unsigned char hdr_mode;
    unsigned int  background_w;
    unsigned int  background_h;
    unsigned int  fullScreen_w;
    unsigned int  fullScreen_h;
    unsigned int  curPosition_x;
    unsigned int  curPosition_y;
    unsigned int  curPosition_w;
    unsigned int  curPosition_h;
} osd_page_flip_info_t;

/*fake index for display components.*/
#define CONNECTOR_IDX_MIN (20)
#define OSD_PLANE_IDX_MIN (30)
#define VIDEO_PLANE_IDX_MIN (40)

#define CURSOR_PLANE_FIXED_ZORDER 0xFFF
#define OSD_PLANE_FIXED_ZORDER 0x100
#define LEGACY_VIDEO_PLANE_FIXED_ZORDER 0x80

#define OSD_INPUT_MAX_WIDTH (1920)
#define OSD_INPUT_MAX_HEIGHT (1080)
#define OSD_INPUT_MIN_WIDTH (32)
#define OSD_INPUT_MIN_HEIGHT (32)

#endif/*AM_FRAMEBUFFER_H*/
