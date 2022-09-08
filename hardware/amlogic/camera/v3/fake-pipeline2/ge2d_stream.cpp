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

#define LOG_NDEBUG 0
#define LOG_NNDEBUG 0

#define LOG_TAG "ge2d_stream"
#if defined(LOG_NNDEBUG) && LOG_NNDEBUG == 0
#define ALOGVV ALOGV
#else
#define ALOGVV(...) ((void)0)
#endif

#include <hardware/camera3.h>
#include <DebugUtils.h>
#include "ge2d_stream.h"


namespace android {
int ge2dDevice::fillStream(VideoInfo *src, uintptr_t physAddr, const android::StreamBuffer &dst) {
    /* data operating. */
#if 0
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
#endif
    return 0;
}

int ge2dDevice::ge2d_copy(int dst_fd, int src_fd, size_t width, size_t height)
{
    ALOGVV("%s: E", __FUNCTION__);
    int ret = 0;
    ge2d_copy_internal(dst_fd, AML_GE2D_MEM_ION,src_fd,
                        AML_GE2D_MEM_ION, width, height);
    return ret;
}

int ge2dDevice::ge2d_copy_dma(int dst_fd, int src_fd, size_t width, size_t height)
{
    ALOGVV("%s: E", __FUNCTION__);
    int ret = 0;
    ret = ge2d_copy_internal(dst_fd, AML_GE2D_MEM_ION,src_fd,
                        AML_GE2D_MEM_DMABUF, width, height);
    return ret;
}
/*function: this funtion copy image from one buffer to another buffer.
  dst_fd : the share fd related to destination buffer
  dst_alloc_type: where dose the destination buffer come from. etc ION or dma buffer
  src_fd : the share fd related to source buffer
  src_alloc_type: where dose the source buffer come from. etc ION or dma buffer
  width: the width of image
  height: the height of image
 */
int ge2dDevice::ge2d_copy_internal(int dst_fd, int dst_alloc_type,int src_fd,
                                           int src_alloc_type, size_t width, size_t height)
{
    ALOGVV("%s: E", __FUNCTION__);
    aml_ge2d_t amlge2d;
    memset(&amlge2d,0,sizeof(aml_ge2d_t));
    memset(&(amlge2d.ge2dinfo.src_info[0]), 0, sizeof(buffer_info_t));
    memset(&(amlge2d.ge2dinfo.src_info[1]), 0, sizeof(buffer_info_t));
    memset(&(amlge2d.ge2dinfo.dst_info), 0, sizeof(buffer_info_t));

    amlge2d.ge2dinfo.src_info[0].canvas_w = width;
    amlge2d.ge2dinfo.src_info[0].canvas_h = height;
    amlge2d.ge2dinfo.src_info[0].format = PIXEL_FORMAT_YCbCr_420_SP_NV12;
    amlge2d.ge2dinfo.src_info[0].plane_number = 1;
    amlge2d.ge2dinfo.src_info[0].shared_fd[0] = src_fd;

    amlge2d.ge2dinfo.src_info[1].canvas_w = width;
    amlge2d.ge2dinfo.src_info[1].canvas_h = height;
    amlge2d.ge2dinfo.src_info[1].format = PIXEL_FORMAT_YCbCr_420_SP_NV12;
    amlge2d.ge2dinfo.src_info[1].plane_number = 1;
    amlge2d.ge2dinfo.src_info[1].shared_fd[0] = -1;

    amlge2d.ge2dinfo.dst_info.canvas_w = width;
    amlge2d.ge2dinfo.dst_info.canvas_h = height;
    amlge2d.ge2dinfo.dst_info.format = PIXEL_FORMAT_YCbCr_420_SP_NV12;
    amlge2d.ge2dinfo.dst_info.plane_number = 1;
    amlge2d.ge2dinfo.dst_info.shared_fd[0] = dst_fd;

    amlge2d.ge2dinfo.dst_info.rotation = GE2D_ROTATION_0;
    amlge2d.ge2dinfo.offset = 0;
    amlge2d.ge2dinfo.ge2d_op = AML_GE2D_STRETCHBLIT;
    amlge2d.ge2dinfo.blend_mode = BLEND_MODE_PREMULTIPLIED;

    amlge2d.ge2dinfo.src_info[0].memtype = GE2D_CANVAS_ALLOC;
    amlge2d.ge2dinfo.src_info[1].memtype = /*GE2D_CANVAS_ALLOC;//*/GE2D_CANVAS_TYPE_INVALID;
    amlge2d.ge2dinfo.dst_info.memtype = GE2D_CANVAS_ALLOC;
    amlge2d.ge2dinfo.src_info[0].mem_alloc_type = src_alloc_type;
    amlge2d.ge2dinfo.src_info[1].mem_alloc_type = /*AML_GE2D_MEM_ION;//*/AML_GE2D_MEM_INVALID;
    amlge2d.ge2dinfo.dst_info.mem_alloc_type = dst_alloc_type;


    int ret = aml_ge2d_init(&amlge2d);
    if (ret < 0) {
        aml_ge2d_exit(&amlge2d);
        ALOGVV("%s: %s", __FUNCTION__,strerror(errno));
        return ret;
    }

    amlge2d.ge2dinfo.src_info[0].rect.x = 0;
    amlge2d.ge2dinfo.src_info[0].rect.y = 0;
    amlge2d.ge2dinfo.src_info[0].rect.w = width;
    amlge2d.ge2dinfo.src_info[0].rect.h = height;
    amlge2d.ge2dinfo.src_info[0].shared_fd[0] = src_fd;
    amlge2d.ge2dinfo.src_info[0].layer_mode = 0;
    amlge2d.ge2dinfo.src_info[0].plane_number = 1;
    amlge2d.ge2dinfo.src_info[0].plane_alpha = 0xff;

    amlge2d.ge2dinfo.dst_info.rect.x = 0;
    amlge2d.ge2dinfo.dst_info.rect.y = 0;
    amlge2d.ge2dinfo.dst_info.rect.w = width;
    amlge2d.ge2dinfo.dst_info.rect.h = height;
    amlge2d.ge2dinfo.dst_info.rotation = GE2D_ROTATION_0;
    amlge2d.ge2dinfo.dst_info.shared_fd[0] = dst_fd;
    amlge2d.ge2dinfo.dst_info.plane_number = 1;

    ret = aml_ge2d_process(&amlge2d.ge2dinfo);
    if (ret < 0) {
        aml_ge2d_exit(&amlge2d);
        ALOGVV("%s: %s", __FUNCTION__,strerror(errno));
        return ret;
    }
    aml_ge2d_exit(&amlge2d);
    return 0;

}

int ge2dDevice::imageScaler()
{
    return 0;
}

/*function: make image rotation some degree using ge2d device
  dst_fd : the share fd of destination buffer.
  src_w  : the width of source image.
  src_h  : the height of source image
  fmt    : the pixel format of source image
  degree : the rotation degree
  amlge2d: the ge2d device object which has allocate buffer for source image.
*/
int ge2dDevice::ge2d_rotation(int dst_fd,size_t src_w,
                size_t src_h,int fmt, int degree, aml_ge2d_t& amlge2d) {

    //ALOGVV("%s: src_w=%d, src_h=%d E", __FUNCTION__,src_w,src_h);
    amlge2d.ge2dinfo.dst_info.canvas_w = src_w;
    amlge2d.ge2dinfo.dst_info.canvas_h = src_h;
    switch (fmt) {
        case NV12:
            amlge2d.ge2dinfo.dst_info.format = PIXEL_FORMAT_YCbCr_420_SP_NV12;
            break;
        case RGB:
            amlge2d.ge2dinfo.dst_info.format = PIXEL_FORMAT_RGB_888;
            break;
        default:
            amlge2d.ge2dinfo.dst_info.format = PIXEL_FORMAT_YCbCr_420_SP_NV12;
            break;
    }
    amlge2d.ge2dinfo.dst_info.plane_number = 1;
    amlge2d.ge2dinfo.dst_info.shared_fd[0] = dst_fd;
    switch (degree) {
        case 0:
            amlge2d.ge2dinfo.dst_info.rotation = GE2D_ROTATION_0;
            break;
        case 90:
            amlge2d.ge2dinfo.dst_info.rotation = GE2D_ROTATION_90;
            break;
        case 180:
            amlge2d.ge2dinfo.dst_info.rotation = GE2D_ROTATION_180;
            break;
        case 270:
            amlge2d.ge2dinfo.dst_info.rotation = GE2D_ROTATION_270;
            break;
        default:
            break;
    }
    amlge2d.ge2dinfo.offset = 0;

    amlge2d.ge2dinfo.blend_mode = BLEND_MODE_PREMULTIPLIED;

    amlge2d.ge2dinfo.src_info[0].memtype = GE2D_CANVAS_ALLOC;
    amlge2d.ge2dinfo.src_info[0].mem_alloc_type = AML_GE2D_MEM_ION;
    amlge2d.ge2dinfo.dst_info.memtype = GE2D_CANVAS_ALLOC;
    amlge2d.ge2dinfo.dst_info.mem_alloc_type = AML_GE2D_MEM_ION;

    amlge2d.ge2dinfo.src_info[0].rect.x = 0;
    amlge2d.ge2dinfo.src_info[0].rect.y = 0;
    amlge2d.ge2dinfo.src_info[0].rect.w = src_w;
    amlge2d.ge2dinfo.src_info[0].rect.h = src_h;
    amlge2d.ge2dinfo.src_info[0].layer_mode = 0;
    amlge2d.ge2dinfo.src_info[0].plane_number = 1;
    amlge2d.ge2dinfo.src_info[0].plane_alpha = 0xff;

    amlge2d.ge2dinfo.dst_info.shared_fd[0] = dst_fd;
    amlge2d.ge2dinfo.dst_info.plane_number = 1;

    amlge2d.ge2dinfo.dst_info.rect.x = 0;
    amlge2d.ge2dinfo.dst_info.rect.y = 0;
    amlge2d.ge2dinfo.dst_info.rect.w = src_w;
    amlge2d.ge2dinfo.dst_info.rect.h = src_h;
    amlge2d.ge2dinfo.dst_info.rotation = GE2D_ROTATION_0;
    switch (fmt) {
        case NV12:
            amlge2d.ge2dinfo.color = 0x008080ff;
            break;
        case RGB:
            amlge2d.ge2dinfo.color = 0;
            break;
        default:
            amlge2d.ge2dinfo.color = 0x008080ff;
            break;
    }
    amlge2d.ge2dinfo.ge2d_op = AML_GE2D_FILLRECTANGLE;
    int ret = aml_ge2d_process(&amlge2d.ge2dinfo);
    if (ret < 0) {
        aml_ge2d_exit(&amlge2d);
        ALOGVV("%s: %s", __FUNCTION__,strerror(errno));
        return ret;
    }

    amlge2d.ge2dinfo.ge2d_op = AML_GE2D_STRETCHBLIT;
    float ratio = (src_h*1.0)/(src_w*1.0);
    switch (degree) {
        case 0:
            amlge2d.ge2dinfo.dst_info.rect.x = 0;
            amlge2d.ge2dinfo.dst_info.rect.y = 0;
            amlge2d.ge2dinfo.dst_info.rect.w = src_w;
            amlge2d.ge2dinfo.dst_info.rect.h = src_h;
            amlge2d.ge2dinfo.dst_info.rotation = GE2D_ROTATION_0;
            break;
        case 90:
            amlge2d.ge2dinfo.dst_info.rect.x = (src_w - (ratio*src_h))/2;
            amlge2d.ge2dinfo.dst_info.rect.y = 0;
            amlge2d.ge2dinfo.dst_info.rect.w = ratio*src_h;
            amlge2d.ge2dinfo.dst_info.rect.h = src_h;
            amlge2d.ge2dinfo.dst_info.rotation = GE2D_ROTATION_90;
            break;
        case 180:
            amlge2d.ge2dinfo.dst_info.rect.x = 0;
            amlge2d.ge2dinfo.dst_info.rect.y = 0;
            amlge2d.ge2dinfo.dst_info.rect.w = src_w;
            amlge2d.ge2dinfo.dst_info.rect.h = src_h;
            amlge2d.ge2dinfo.dst_info.rotation = GE2D_ROTATION_180;
            break;
        case 270:
            amlge2d.ge2dinfo.dst_info.rect.x = (src_w - (ratio*src_h))/2;
            amlge2d.ge2dinfo.dst_info.rect.y = 0;
            amlge2d.ge2dinfo.dst_info.rect.w = ratio*src_h;
            amlge2d.ge2dinfo.dst_info.rect.h = src_h;
            amlge2d.ge2dinfo.dst_info.rotation = GE2D_ROTATION_270;
            break;
        default:
            break;
    }


    ret = aml_ge2d_process(&amlge2d.ge2dinfo);
    if (ret < 0) {
        aml_ge2d_exit(&amlge2d);
        ALOGVV("%s: %s", __FUNCTION__,strerror(errno));
        return ret;
    }

    amlge2d.ge2dinfo.dst_info.memtype = GE2D_CANVAS_TYPE_INVALID;
    amlge2d.ge2dinfo.dst_info.mem_alloc_type = AML_GE2D_MEM_INVALID;
    amlge2d.ge2dinfo.dst_info.plane_number = 0;
    amlge2d.ge2dinfo.dst_info.shared_fd[0] = -1;

    aml_ge2d_mem_free(&amlge2d);

    aml_ge2d_exit(&amlge2d);

    return 0;
}
/*
function: allocate dma buffer for image by ge2d object
width : the width of image
height : the height of image
share_fd : the share fd of dma buffer for image
fmt: the pixel format of image
amlge2d:the ge2d object which is used to allocate buffer for image
note:
the allocated buffer is binded with the ge2d object, so if you want use this buffer,
you must hold the related ge2d object.
*/
char* ge2dDevice::ge2d_alloc(size_t width, size_t height,int*share_fd,int fmt,aml_ge2d_t& amlge2d) {

    memset(&amlge2d,0x0,sizeof(aml_ge2d_t));
    memset(&(amlge2d.ge2dinfo.src_info[0]), 0, sizeof(buffer_info_t));
    memset(&(amlge2d.ge2dinfo.src_info[1]), 0, sizeof(buffer_info_t));
    memset(&(amlge2d.ge2dinfo.dst_info), 0, sizeof(buffer_info_t));

    amlge2d.ge2dinfo.src_info[0].memtype = GE2D_CANVAS_ALLOC;
    amlge2d.ge2dinfo.src_info[0].mem_alloc_type = AML_GE2D_MEM_ION;
    switch (fmt) {
        case NV12:
            amlge2d.ge2dinfo.src_info[0].format = PIXEL_FORMAT_YCbCr_420_SP_NV12;
            break;
        case RGB:
            amlge2d.ge2dinfo.src_info[0].format = PIXEL_FORMAT_RGB_888;
            break;
        default:
            amlge2d.ge2dinfo.src_info[0].format = PIXEL_FORMAT_YCbCr_420_SP_NV12;
            break;
    }
    amlge2d.ge2dinfo.src_info[0].canvas_h = height;
    amlge2d.ge2dinfo.src_info[0].canvas_w = width;
    amlge2d.ge2dinfo.src_info[0].plane_number = 1;

    amlge2d.ge2dinfo.src_info[1].memtype = GE2D_CANVAS_TYPE_INVALID;
    amlge2d.ge2dinfo.src_info[1].mem_alloc_type = AML_GE2D_MEM_INVALID;
    amlge2d.ge2dinfo.src_info[1].plane_number = 0;
    amlge2d.ge2dinfo.src_info[1].shared_fd[0] = -1;

    amlge2d.ge2dinfo.dst_info.memtype = GE2D_CANVAS_TYPE_INVALID;
    amlge2d.ge2dinfo.dst_info.mem_alloc_type = AML_GE2D_MEM_INVALID;
    amlge2d.ge2dinfo.dst_info.plane_number = 0;
    amlge2d.ge2dinfo.dst_info.shared_fd[0] = -1;

    int ret = aml_ge2d_init(&amlge2d);
    if (ret < 0) {
        aml_ge2d_exit(&amlge2d);
        ALOGVV("%s: ge2d init fail,%s", __FUNCTION__,strerror(errno));
        return nullptr;
    }

    ret = aml_ge2d_mem_alloc(&amlge2d);
    if (ret < 0) {
        aml_ge2d_exit(&amlge2d);
        ALOGVV("%s: alloc mem fail, %s", __FUNCTION__,strerror(errno));
        return nullptr;
    }

    *share_fd = amlge2d.ge2dinfo.src_info[0].shared_fd[0];
    return amlge2d.ge2dinfo.src_info[0].vaddr[0];
}

/*
 function: this function is used to free buffer which related with ge2d object.
 amlge2d: the ge2d object which is used to allocate buffer.
 */
int ge2dDevice::ge2d_free(aml_ge2d_t& amlge2d) {

    amlge2d.ge2dinfo.dst_info.memtype = GE2D_CANVAS_TYPE_INVALID;
    amlge2d.ge2dinfo.dst_info.mem_alloc_type = AML_GE2D_MEM_INVALID;
    amlge2d.ge2dinfo.dst_info.plane_number = 0;
    amlge2d.ge2dinfo.dst_info.shared_fd[0] = -1;

    aml_ge2d_mem_free(&amlge2d);

    aml_ge2d_exit(&amlge2d);

    return 0;
}

}// namespace android
