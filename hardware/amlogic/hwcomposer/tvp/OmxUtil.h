/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC OMX IOCTL WRAPPER
 */
#ifndef OMX_UTILS_H
#define OMX_UTILS_H

typedef unsigned int u32;

struct vframe_content_light_level_s {
    u32 present_flag;
    u32 max_content;
    u32 max_pic_average;
}; /* content_light_level from SEI */

typedef struct vframe_master_display_colour_s {
    u32 present_flag;
    u32 primaries[3][2];
    u32 white_point[2];
    u32 luminance[2];
    struct vframe_content_light_level_s
        content_light_level;
}vframe_master_display_colour_s_t; /* master_display_colour_info_volume from SEI */

int openamvideo();
void closeamvideo();
int setomxdisplaymode();
int setomxpts(int time_video);
int setomxpts(uint32_t* omx_info);
void set_omx_pts(char* data, int* handle);
int set_hdr_info(vframe_master_display_colour_s_t * vf_hdr);

#endif
