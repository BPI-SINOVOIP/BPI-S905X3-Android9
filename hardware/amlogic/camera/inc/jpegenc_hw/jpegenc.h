/*
 * Copyright (C) 2011 The Android Open Source Project
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
 *
 * Description:
 */

#ifndef AML_JPEG_ENCODER_M8_
#define AML_JPEG_ENCODER_M8_

#define JPEGENC_IOC_MAGIC  'E'

#define JPEGENC_IOC_GET_DEVINFO 				_IOW(JPEGENC_IOC_MAGIC, 0xf0, unsigned int)

#define JPEGENC_IOC_GET_ADDR			 		_IOW(JPEGENC_IOC_MAGIC, 0x00, unsigned int)
#define JPEGENC_IOC_INPUT_UPDATE				_IOW(JPEGENC_IOC_MAGIC, 0x01, unsigned int)
#define JPEGENC_IOC_GET_STATUS					_IOW(JPEGENC_IOC_MAGIC, 0x02, unsigned int)
#define JPEGENC_IOC_NEW_CMD						_IOW(JPEGENC_IOC_MAGIC, 0x03, unsigned int)
#define JPEGENC_IOC_GET_STAGE					_IOW(JPEGENC_IOC_MAGIC, 0x04, unsigned int)
#define JPEGENC_IOC_GET_OUTPUT_SIZE				_IOW(JPEGENC_IOC_MAGIC, 0x05, unsigned int)
#define JPEGENC_IOC_SET_QUALITY 					_IOW(JPEGENC_IOC_MAGIC, 0x06, unsigned int)
#define JPEGENC_IOC_SET_ENCODER_WIDTH 			_IOW(JPEGENC_IOC_MAGIC, 0x07, unsigned int)
#define JPEGENC_IOC_SET_ENCODER_HEIGHT 			_IOW(JPEGENC_IOC_MAGIC, 0x08, unsigned int)
#define JPEGENC_IOC_CONFIG_INIT 				_IOW(JPEGENC_IOC_MAGIC, 0x09, unsigned int)
#define JPEGENC_IOC_FLUSH_CACHE 				_IOW(JPEGENC_IOC_MAGIC, 0x0a, unsigned int)
#define JPEGENC_IOC_FLUSH_DMA 					_IOW(JPEGENC_IOC_MAGIC, 0x0b, unsigned int)
#define JPEGENC_IOC_GET_BUFFINFO 				_IOW(JPEGENC_IOC_MAGIC, 0x0c, unsigned int)
#define JPEGENC_IOC_INPUT_FORMAT 				_IOW(JPEGENC_IOC_MAGIC, 0x0d, unsigned int)
#define JPEGENC_IOC_SEL_QUANT_TABLE 				_IOW(JPEGENC_IOC_MAGIC, 0x0e, unsigned int)
#define JPEGENC_IOC_SET_EXT_QUANT_TABLE 				_IOW(JPEGENC_IOC_MAGIC, 0x0f, unsigned int)

//---------------------------------------------------
// ENCODER_STATUS define
//---------------------------------------------------
#define ENCODER_IDLE                0
#define ENCODER_START               1
//#define ENCODER_SOS_HEADER          2
#define ENCODER_MCU                 3
#define ENCODER_DONE                4


#define JPEGENC_BUFFER_INPUT              0
#define JPEGENC_BUFFER_OUTPUT           1

typedef enum{
    LOCAL_BUFF = 0,
    CANVAS_BUFF,
    PHYSICAL_BUFF,
    MAX_BUFF_TYPE 
}jpegenc_mem_type;

typedef enum{
    FMT_YUV422_SINGLE = 0,
    FMT_YUV444_SINGLE,
    FMT_NV21,
    FMT_NV12,
    FMT_YUV420,    
    FMT_YUV444_PLANE,
    FMT_RGB888,
    FMT_RGB888_PLANE,
    FMT_RGB565,
    FMT_RGBA8888,
    MAX_FRAME_FMT 
}jpegenc_frame_fmt;

#define ENCODER_PATH       "/dev/jpegenc"

typedef struct{
    unsigned char* addr;
    unsigned size;
}jpegenc_buff_t;

typedef struct hw_jpegenc_s {
    uint8_t* src;
    unsigned src_size;
    uint8_t* dst;
    unsigned dst_size;
    int quality;
    int qtbl_id;
    unsigned width;
    unsigned height;
    int bpp;
    jpegenc_mem_type type;
    jpegenc_frame_fmt in_format;
    jpegenc_frame_fmt out_format;
    size_t jpeg_size;

    int fd;
    jpegenc_buff_t mmap_buff;
    jpegenc_buff_t input_buf;
    jpegenc_buff_t output_buf;
 }hw_jpegenc_t;

extern size_t hw_encode(hw_jpegenc_t* hw_info);
#endif