/*
 * Copyright (C) 2014 The Android Open Source Project
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
 */

//#define LOG_NDEBUG 0
//#define LOG_NNDEBUG 0

#define LOG_TAG "Camera_Stream"
#include <hardware/camera3.h>
#include <DebugUtils.h>
#include "ge2d_stream.h"

//namespace android {

#ifdef __cplusplus
//extern "C"{
#endif
int cameraConfigureStreams(struct VideoInfo *vinfo, camera3_stream_configuration *streamList) {
#if 0

    int src_width = vinfo->preview.format.fmt.pix.width;
    int src_height = vinfo->preview.format.fmt.pix.height;
    int src_top = 0;
    int src_left = 0;

    //TODO check the pointers
    if (vinfo->num_configs == 0) {
        vinfo->ge2d_config = (config_para_ex_t *) malloc (sizeof(config_para_ex_t)
                            *streamList->num_streams);
        vinfo->num_configs = streamList->num_streams;
    } else if (vinfo->num_configs <= streamList->num_streams) {
        vinfo->ge2d_config = (config_para_ex_t *) realloc (sizeof(config_para_ex_t)
                            *streamList->num_streams);
        vinfo->num_configs = streamList->num_streams;
    }

    config_para_ex_t *cfg = vinfo->ge2d_config;

    for (size_t i = 0; i < streamList->num_streams; i++) {
        camera3_stream_t *newStream = streamList->streams[i];

        /* data operating. */ 
        cfg->alu_const_color= 0;//0x000000ff;
        cfg->bitmask_en  = 0;
        cfg->src1_gb_alpha = 0;//0xff;
        cfg->dst_xy_swap = 0;

        cfg->src_planes[0].addr = 0
        cfg->src_planes[0].w = src_width;
        cfg->src_planes[0].h = src_height;
        cfg->src_planes[1].addr = 0;
        cfg->src_planes[1].w = src_width/2;
        cfg->src_planes[1].h = src_height/2;
        cfg->src_planes[2].addr = 0;
        cfg->src_planes[2].w = src_width/2;
        cfg->src_planes[2].h = src_height/2;
        //canvas_read(output_para.index&0xff,&cd);
        cfg->dst_planes[0].addr = 0;//cd.addr;
        cfg->dst_planes[0].w = newStream->width;
        cfg->dst_planes[0].h = newStream->height;
        cfg->src_key.key_enable = 0;
        cfg->src_key.key_mask = 0;
        cfg->src_key.key_mode = 0;
        cfg->src_para.canvas_index=0;//TODO vf->canvas0Addr;
        cfg->src_para.mem_type = CANVAS_ALLOC;
        cfg->src_para.format =  GE2D_FORMAT_S16_YUV422;
            //get_input_format(vf);
        cfg->src_para.fill_color_en = 0;
        cfg->src_para.fill_mode = 0;
        cfg->src_para.x_rev = 0;
        cfg->src_para.y_rev = 0;
        cfg->src_para.color = 0xffffffff;
        cfg->src_para.top = 0;
        cfg->src_para.left = 0;
        cfg->src_para.width = src_width;
        cfg->src_para.height = src_height;
        /* printk("vf_width is %d , vf_height is %d \n",vf->width ,vf->height); */
        //cfg->dst_para.canvas_index = output_para.index&0xff;

        cfg.dst_para.mem_type = CANVAS_ALLOC;
        cfg.dst_para.fill_color_en = 0;
        cfg.dst_para.fill_mode = 0;
        cfg.dst_para.format  = GE2D_FORMAT_M24_NV21|GE2D_LITTLE_ENDIAN;
        cfg.dst_para.x_rev = 0;
        cfg.dst_para.y_rev = 0;
        cfg.dst_para.color = 0;
        cfg.dst_para.top = 0;
        cfg.dst_para.left = 0;
        cfg.dst_para.width = newStream->width;
        cfg.dst_para.height = newStream->height;

        cfg ++;
    }
    CAMHAL_LOGDB("numcfgs = %d, num streams=%d\n",
            vinfo->num_configs, streamList->num_streams);
#endif
    return 0;
}

int fillStream(struct VideoInfo *src, uintptr_t physAddr, const android::StreamBuffer &dst) {
    /* data operating. */ 
    int ge2d_fd;
    config_para_ex_t cfg;
    ge2d_para_t  para;

    int32_t src_width = src->preview.format.fmt.pix.width;
    int32_t src_height =  src->preview.format.fmt.pix.height;

    ge2d_fd = open("/dev/ge2d", O_RDWR);
    if (ge2d_fd < 0) {
        CAMHAL_LOGEA("open ge2d failed\n");
        return -1;
    }

    //ioctl(ge2d_fd, GE2D_ANTIFLICKER_ENABLE,0);
    memset(&cfg, 0, sizeof(cfg));
    cfg.alu_const_color= 0;
    cfg.bitmask_en  = 0;
    cfg.src1_gb_alpha = 0; //0xff;
    cfg.dst_xy_swap = 0;

    cfg.src_planes[0].addr = physAddr;
    cfg.src_planes[0].w = src_width;
    cfg.src_planes[0].h = src_height;
    //NV21, YV12 need to add
    cfg.src_planes[1].addr = physAddr + src_width *src_height;
    cfg.src_planes[1].w = src_width;
    cfg.src_planes[1].h = src_height/2;
    //cfg.src_planes[2].addr = 0;
    //cfg.src_planes[2].w = src_width/2;
    //cfg.src_planes[2].h = src_height/2;
    //canvas_read(output_para.index&0xff,&cd);
    cfg.dst_planes[0].addr = dst.share_fd;//cd.addr;
    cfg.dst_planes[0].w = dst.width;
    cfg.dst_planes[0].h = dst.height;

    cfg.dst_planes[1].addr = dst.share_fd;//cd.addr;
    cfg.dst_planes[1].w = dst.width;
    cfg.dst_planes[1].h = dst.height/2;
    cfg.src_key.key_enable = 0;
    cfg.src_key.key_mask = 0;
    cfg.src_key.key_mode = 0;
    cfg.src_para.mem_type = CANVAS_ALLOC;
    cfg.src_para.format = GE2D_FORMAT_M24_NV21|GE2D_LITTLE_ENDIAN;
    cfg.src_para.fill_color_en = 0;
    cfg.src_para.fill_mode = 0;
    cfg.src_para.x_rev = 0;
    cfg.src_para.y_rev = 0;
    cfg.src_para.color = 0xffffffff;// 0xffff;
    cfg.src_para.top = 0;
    cfg.src_para.left = 0;
    cfg.src_para.width = src_width;
    cfg.src_para.height = src_height;
    cfg.src2_para.mem_type = CANVAS_TYPE_INVALID;
    /* printk("vf_width is %d , vf_height is %d \n",vf.width ,vf.height); */
    //cfg.dst_para.canvas_index = output_para.index&0xff;

    cfg.dst_para.mem_type = CANVAS_ALLOC;
    cfg.dst_para.fill_color_en = 0;
    cfg.dst_para.fill_mode = 0;
    cfg.dst_para.format  = GE2D_FORMAT_M24_NV21|GE2D_LITTLE_ENDIAN;
    cfg.dst_para.x_rev = 0;
    cfg.dst_para.y_rev = 0;
    cfg.dst_para.color = 0;
    cfg.dst_para.top = 0;
    cfg.dst_para.left = 0;
    cfg.dst_para.width = dst.width;
    cfg.dst_para.height = dst.height;


    ioctl(ge2d_fd, GE2D_CONFIG_EX, &cfg);

    para.src1_rect.x = 0;
    para.src1_rect.y = 0;
    para.src1_rect.w = src_width;
    para.src1_rect.h = src_height;
    para.dst_rect.x  = 0;
    para.dst_rect.y  = 0;
    para.dst_rect.w  = dst.width;
    para.dst_rect.h  = dst.height;
    //Y
    ioctl(ge2d_fd, GE2D_STRETCHBLIT_NOALPHA, &para);
    //Cb
    //Cr

    close (ge2d_fd);
    ge2d_fd = -1;

    return 0;
}
#ifdef __cplusplus
//}
#endif
//}// namespace android
