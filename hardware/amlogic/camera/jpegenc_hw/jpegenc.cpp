/*
 * Copyright (C) 2010 The Android Open Source Project
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

 #define LOG_TAG "HW_JPEGENC"

#include "CameraHal.h"

#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>

#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>

#include "jpegenc.h"

#define ENCODE_DONE_TIMEOUT 5000

//#define DEBUG_TIME
#ifdef DEBUG_TIME
static struct timeval start_test, end_test;
#endif

static int RGBX32_To_RGB24Plane_NEON(unsigned char *src, unsigned char *dest, int width, int height)
{
    unsigned char *R;
    unsigned char *G;
    unsigned char *B;
    int canvas_w = ((width+31)>>5)<<5;
    int i, j;
    int aligned = canvas_w - width;
    
    if( !src || !dest )
        return -1;
    
    R = dest;
    G = R + canvas_w * height;
    B = G + canvas_w * height;
    
    for( i = 0; i < height; i += 1 ){
        for( j = 0; j < width; j += 8 ){
            asm volatile (
                "vld4.8     {d0, d1, d2, d3}, [%[src]]!      \n"  // load  8 more ABGR pixels.
                "vst1.8     {d0}, [%[R]]!                    \n"  // store R.    
                "vst1.8     {d1}, [%[G]]!                    \n"  // store G.
                "vst1.8     {d2}, [%[B]]!                    \n"  // store B.

                : [src] "+r" (src), [R] "+r" (R),
                  [G] "+r" (G), [B] "+r" (B)
                : 
                : "cc", "memory", "d0", "d1", "d2", "d3"
            );
        }
        if(aligned){
            R+=aligned;
            G+=aligned;
            B+=aligned;
        }
    }
    return canvas_w*height*3;
}

static int RGB24_To_RGB24Plane_NEON(unsigned char *src, unsigned char *dest, int width, int height)
{
    unsigned char *R;
    unsigned char *G;
    unsigned char *B;
    int i, j;
    int canvas_w = ((width+31)>>5)<<5;
    int aligned = canvas_w - width;

    if( !src || !dest )
        return -1;

#ifdef DEBUG_TIME
    unsigned total_time = 0;
    struct timeval start_test1, end_test1;
    gettimeofday(&start_test1, NULL);
#endif
    R = dest;
    G = R + canvas_w * height;
    B = G + canvas_w * height;
    
    for( i = 0; i < height; i += 1 ){
        for( j = 0; j < width; j += 8 ){
            asm volatile (
                "vld3.8     {d0, d1, d2}, [%[src]]!      \n"  // load 8 more BGR pixels.
                "vst1.8     {d0}, [%[R]]!                \n"  // store R.    
                "vst1.8     {d1}, [%[G]]!                \n"  // store G.
                "vst1.8     {d2}, [%[B]]!                \n"  // store B.
          
                : [src] "+r" (src), [R] "+r" (R),
                  [G] "+r" (G), [B] "+r" (B)
                :
                : "cc", "memory", "d0", "d1", "d2"
            );
        }
        if(aligned){
            R+=aligned;
            G+=aligned;
            B+=aligned;
        }
    }
#ifdef DEBUG_TIME
    gettimeofday(&end_test1, NULL);
    total_time = (end_test1.tv_sec - start_test1.tv_sec)*1000000 + end_test1.tv_usec -start_test1.tv_usec;
    ALOGD("RGB24_To_RGB24Plane_NEON: need time: %d us",total_time);
#endif
    return canvas_w*height*3;
}

static int encode_poll(int fd, int timeout)
{    
    struct pollfd poll_fd[1];    
    poll_fd[0].fd = fd;    
    poll_fd[0].events = POLLIN |POLLERR;    
    return poll(poll_fd, 1, timeout);
}

static unsigned copy_to_local(hw_jpegenc_t* hw_info)
{
    unsigned offset = 0;
    int bytes_per_line = 0, active = 0;;
    unsigned i = 0;
    unsigned total_size = 0;
    unsigned char* src = NULL;
    unsigned char* dst = NULL;
    int plane_num = 1;

    if((hw_info->bpp !=12)&&(hw_info->in_format != FMT_YUV444_PLANE)&&(hw_info->in_format != FMT_RGB888_PLANE))
        bytes_per_line = hw_info->width*hw_info->bpp/8;
    else
        bytes_per_line = hw_info->width;

    if((hw_info->in_format == FMT_YUV420)||(hw_info->in_format == FMT_YUV444_PLANE)||(hw_info->in_format == FMT_RGB888_PLANE))
        plane_num = 3;
    else if ((hw_info->in_format == FMT_NV12)||(hw_info->in_format == FMT_NV21))
        plane_num = 2;

    active = bytes_per_line;

    if(hw_info->in_format == FMT_YUV420)
        bytes_per_line = ((bytes_per_line+63)>>6)<<6;
    else if((hw_info->in_format == FMT_NV12)||(hw_info->in_format == FMT_NV21)||(hw_info->in_format == FMT_RGB888)
      ||(hw_info->in_format == FMT_YUV422_SINGLE)||(hw_info->in_format == FMT_YUV444_SINGLE)||(hw_info->in_format == FMT_YUV444_PLANE)
      ||(hw_info->in_format == FMT_RGB888_PLANE))
        bytes_per_line = ((bytes_per_line+31)>>5)<<5;
    
    src = (unsigned char*)hw_info->src;
    dst = hw_info->input_buf.addr;
    if(bytes_per_line != active){
        for(i =0; i<hw_info->height; i++){
            memcpy(dst, src,active);
            dst+=bytes_per_line;
            src+=active;
        }
    }else{ 
        memcpy(dst, src,hw_info->height*bytes_per_line);
    }

    if(plane_num == 2){
        offset = hw_info->height*bytes_per_line;
        src = (unsigned char*)(hw_info->src + hw_info->height*active);
        dst = (unsigned char*)(hw_info->input_buf.addr+offset);
        if(bytes_per_line != active){
            for(i =0; i<hw_info->height/2; i++){
                memcpy(dst, src,active);
                dst+=bytes_per_line;
                src+=active;
            }
        }else{ 
            memcpy(dst, src,hw_info->height*bytes_per_line/2);
        }
    }else if(plane_num == 3){
        unsigned temp_active = ((hw_info->in_format == FMT_YUV444_PLANE)||(hw_info->in_format == FMT_RGB888_PLANE))?active:active/2;
        unsigned temp_h = ((hw_info->in_format == FMT_YUV444_PLANE)||(hw_info->in_format == FMT_RGB888_PLANE))?hw_info->height:hw_info->height/2;
        unsigned temp_bytes = ((hw_info->in_format == FMT_YUV444_PLANE)||(hw_info->in_format == FMT_RGB888_PLANE))?bytes_per_line:bytes_per_line/2;
        offset = hw_info->height*bytes_per_line;
        src = (unsigned char*)(hw_info->src + hw_info->height*active);
        dst = (unsigned char*)(hw_info->input_buf.addr+offset);
        if(bytes_per_line != active){
            for(i =0; i<temp_h; i++){
                memcpy(dst, src,temp_active);
                dst+=temp_bytes;
                src+=temp_active;
            }
        }else{ 
            memcpy(dst, src,temp_bytes*temp_h);
        }
        offset = temp_h*temp_bytes+hw_info->height*bytes_per_line;
        src = (unsigned char*)(hw_info->src + hw_info->height*active +temp_h*temp_active);
        dst = (unsigned char*)(hw_info->input_buf.addr+offset);
        if(bytes_per_line != active){
            for(i =0; i<temp_h; i++){
                memcpy(dst, src,temp_active);
                dst+=temp_bytes;
                src+=temp_active;
            }
        }else{ 
            memcpy(dst, src,temp_bytes*temp_h);
        }
    }
    if((hw_info->bpp !=12)&&(hw_info->in_format != FMT_YUV444_PLANE)&&(hw_info->in_format != FMT_RGB888_PLANE))
        total_size = bytes_per_line*hw_info->height;
    else
        total_size = bytes_per_line*hw_info->height*hw_info->bpp/8;
    return total_size;
}

static size_t start_encoder(hw_jpegenc_t* hw_info)
{
    int i;
    int bpp;
    unsigned size = 0;
    unsigned cmd[7] , status;
    unsigned in_format = hw_info->in_format;
    if(hw_info->type == LOCAL_BUFF){
        if((hw_info->in_format != FMT_RGB888)&&(hw_info->in_format != FMT_RGBA8888)){
            cmd[5] = copy_to_local(hw_info);
        }else if(hw_info->in_format == FMT_RGB888){
            cmd[5] = RGB24_To_RGB24Plane_NEON(hw_info->src, hw_info->input_buf.addr, hw_info->width, hw_info->height);
            in_format = FMT_RGB888_PLANE;
        }else{
            cmd[5] = RGBX32_To_RGB24Plane_NEON(hw_info->src, hw_info->input_buf.addr, hw_info->width, hw_info->height);
            in_format = FMT_RGB888_PLANE;
        }
    }else{
        cmd[5] = hw_info->width*hw_info->height*hw_info->bpp/8;
    }

    cmd[0] = hw_info->type;     //input buffer type
    cmd[1] = in_format; 
    cmd[2] = hw_info->out_format; 
    cmd[3] = (unsigned)hw_info->input_buf.addr;
    cmd[4] = 0; 
    //cmd[5] = hw_info->width*hw_info->height*bpp/8;
    cmd[6] = 1; 

    ioctl(hw_info->fd, JPEGENC_IOC_NEW_CMD, cmd);
    if(encode_poll(hw_info->fd, ENCODE_DONE_TIMEOUT)<=0){
        ALOGE("hw_encode: poll fail");
        return 0;
    }

    ioctl(hw_info->fd, JPEGENC_IOC_GET_STAGE, &status);
    if(status == ENCODER_DONE){
        ioctl(hw_info->fd, JPEGENC_IOC_GET_OUTPUT_SIZE, &size);
        if((size < hw_info->output_buf.size)&&(size>0)&&(size<=hw_info->dst_size)){
            cmd[0] = JPEGENC_BUFFER_OUTPUT;
            cmd[1] = 0 ;
            cmd[2] = size ;
            ioctl(hw_info->fd, JPEGENC_IOC_FLUSH_DMA ,cmd);
            memcpy(hw_info->dst,hw_info->output_buf.addr,size);
            ALOGV("hw_encode: done size: %d ",size);
        }else{
            ALOGE("hw_encode: output buffer size error: bitstream buffer size: %d, jpeg size: %d, output buffer size: %d",hw_info->output_buf.size, size, hw_info->dst_size);
            size = 0;
        }
    }
    return size;
} 

size_t hw_encode(hw_jpegenc_t* hw_info) 
{
    unsigned buff_info[5];
    int ret;
    unsigned encoder_width = hw_info->width;
    unsigned encoder_height = hw_info->height;

#ifdef DEBUG_TIME
    unsigned total_time = 0;
    gettimeofday(&start_test, NULL);
#endif
    hw_info->jpeg_size = 0;
    hw_info->fd = open(ENCODER_PATH, O_RDWR);
    if(hw_info->fd < 0){
        ALOGD("hw_encode open device fail");
        goto EXIT;
    }

    memset(buff_info,0,sizeof(buff_info));
    ret = ioctl(hw_info->fd, JPEGENC_IOC_GET_BUFFINFO,&buff_info[0]);
    if((ret)||(buff_info[0]==0)){
        ALOGE("hw_encode -- no buffer information!");
        goto EXIT;
    }
    hw_info->mmap_buff.addr =  (unsigned char*)mmap(0,buff_info[0], PROT_READ|PROT_WRITE , MAP_SHARED ,hw_info->fd, 0);
    if (hw_info->mmap_buff.addr == MAP_FAILED) {
        ALOGE("hw_encode Error: failed to map framebuffer device to memory.\n");
        goto EXIT;
    }

    hw_info->mmap_buff.size = buff_info[0];
    hw_info->input_buf.addr = hw_info->mmap_buff.addr+buff_info[1];
    hw_info->input_buf.size = buff_info[3]-buff_info[1];
    hw_info->output_buf.addr = hw_info->mmap_buff.addr +buff_info[3];
    hw_info->output_buf.size = buff_info[4];

    hw_info->qtbl_id = 0;

    switch(hw_info->in_format){
        case FMT_NV21:	
            hw_info->bpp = 12;
            break;
        case FMT_RGB888:
            hw_info->bpp = 24;
            break;
        case FMT_YUV422_SINGLE:
            hw_info->bpp = 16;
            break;
        case FMT_YUV444_PLANE:
        case FMT_RGB888_PLANE:
            hw_info->bpp = 24;
            break;
        default:
            hw_info->bpp = 12;
    }

    ioctl(hw_info->fd, JPEGENC_IOC_SET_ENCODER_WIDTH ,&encoder_width);
    ioctl(hw_info->fd, JPEGENC_IOC_SET_ENCODER_HEIGHT ,&encoder_height);

    if((hw_info->width!= encoder_width)||(hw_info->height!= encoder_height)){
        ALOGE("hw_encode: set encode size fail. set as %dx%d, but max size is %dx%d.",hw_info->width,hw_info->height,encoder_width,encoder_height);
        goto EXIT;
    }

    ioctl(hw_info->fd, JPEGENC_IOC_SET_QUALITY ,&hw_info->quality);
    ioctl(hw_info->fd, JPEGENC_IOC_SEL_QUANT_TABLE ,&hw_info->qtbl_id);
    //ioctl(hw_info->fd, JPEGENC_IOC_SET_EXT_QUANT_TABLE ,NULL);

    ioctl(hw_info->fd, JPEGENC_IOC_CONFIG_INIT,NULL);

    hw_info->type = LOCAL_BUFF;
    hw_info->out_format = FMT_YUV420; //FMT_YUV422_SINGLE
    hw_info->jpeg_size = start_encoder(hw_info);
EXIT:
    if(hw_info->mmap_buff.addr){
        munmap(hw_info->mmap_buff.addr ,hw_info->mmap_buff.size);
    }
    close(hw_info->fd);
    hw_info->fd = -1;
#ifdef DEBUG_TIME
    gettimeofday(&end_test, NULL);
    total_time = (end_test.tv_sec - start_test.tv_sec)*1000000 + end_test.tv_usec -start_test.tv_usec;
    ALOGD("hw_encode: need time: %d us, jpeg size:%d",total_time,hw_info->jpeg_size);
#endif
    return hw_info->jpeg_size;
}

