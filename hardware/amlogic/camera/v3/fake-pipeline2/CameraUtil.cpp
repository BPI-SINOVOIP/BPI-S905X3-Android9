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
#include <utils/Log.h>
#include <string.h>

#include "CameraUtil.h"
#include "libyuv.h"
#include "libyuv/scale.h"
//#include "ge2d_stream.h"

#include "NV12_resize.h"

#ifndef ALIGN
#define ALIGN(b,w) (((b)+((w)-1))/(w)*(w))
#endif

/** A few utility functions for math, normal distributions */

// Take advantage of IEEE floating-point format to calculate an approximate
// square root. Accurate to within +-3.6%
float CameraUtil::sqrtf_approx(float r) {
    // Modifier is based on IEEE floating-point representation; the
    // manipulations boil down to finding approximate log2, dividing by two, and
    // then inverting the log2. A bias is added to make the relative error
    // symmetric about the real answer.
    const int32_t modifier = 0x1FBB4000;

    int32_t r_i = *(int32_t*)(&r);
    r_i = (r_i >> 1) + modifier;

    return *(float*)(&r_i);
}

void CameraUtil::rgb24_memcpy(unsigned char *dst, unsigned char *src, int width, int height)
{
        int stride = (width + 31) & ( ~31);
        int h;
        for (h=0; h<height; h++)
        {
                memcpy( dst, src, width*3);
                dst += width*3;
                src += stride*3;
        }
}

inline void CameraUtil::yuv_to_rgb24(unsigned char y,unsigned char u,unsigned char v,unsigned char *rgb)
{
    int r,g,b;
    int rgb24;

    r = (1192 * (y - 16) + 1634 * (v - 128) ) >> 10;
    g = (1192 * (y - 16) - 833 * (v - 128) - 400 * (u -128) ) >> 10;
    b = (1192 * (y - 16) + 2066 * (u - 128) ) >> 10;

    r = r > 255 ? 255 : r < 0 ? 0 : r;
    g = g > 255 ? 255 : g < 0 ? 0 : g;
    b = b > 255 ? 255 : b < 0 ? 0 : b;

    rgb24 = (int)((r << 16) | (g  << 8)| b);

    *rgb = (unsigned char)r;
    rgb++;
    *rgb = (unsigned char)g;
    rgb++;
    *rgb = (unsigned char)b;
}

void CameraUtil::yuyv422_to_rgb24(unsigned char *buf, unsigned char *rgb, int width, int height)
{
    int y,z=0;
    int blocks;

    blocks = (width * height) * 2;

    for (y = 0,z = 0; y < blocks; y += 4,z += 6) {
        unsigned char Y1, Y2, U, V;

        Y1 = buf[y + 0];
        U = buf[y + 1];
        Y2 = buf[y + 2];
        V = buf[y + 3];

        yuv_to_rgb24(Y1, U, V, &rgb[z]);
        yuv_to_rgb24(Y2, U, V, &rgb[z + 3]);
    }
}

void CameraUtil::nv21_to_rgb24(unsigned char *buf, unsigned char *rgb, int width, int height)
{
    int y,z = 0;
    int h;
    int blocks;
    unsigned char Y1, Y2, U, V;

    blocks = (width * height) * 2;

    for (h = 0, z = 0; h < height; h += 2) {
        for (y = 0; y < width * 2; y += 2) {

            Y1 = buf[ h * width + y + 0];
            V = buf[ blocks/2 + h * width/2 + y % width + 0 ];
            Y2 = buf[ h * width + y + 1];
            U = buf[ blocks/2 + h * width/2 + y % width + 1 ];

            yuv_to_rgb24(Y1, U, V, &rgb[z]);
            yuv_to_rgb24(Y2, U, V, &rgb[z + 3]);
            z += 6;
        }
    }
}

void CameraUtil::nv21_memcpy_align32(unsigned char *dst, unsigned char *src, int width, int height)
{
        int stride = (width + 31) & ( ~31);
        int h;
        for (h = 0; h < height* 3/2; h++)
        {
                memcpy( dst, src, width);
                dst += width;
                src += stride;
        }
}

void CameraUtil::yv12_memcpy_align32(unsigned char *dst, unsigned char *src, int width, int height)
{
        int new_width = (width + 63) & ( ~63);
        int stride;
        int h;
        for (h = 0; h < height; h++)
        {
                memcpy( dst, src, width);
                dst += width;
                src += new_width;
        }
        stride = ALIGN( width/2, 16);
        for (h = 0; h < height; h++)
        {
                memcpy( dst, src, width/2);
                dst += stride;
                src += new_width/2;
        }
}

void CameraUtil::YUYVToNV21(uint8_t *src, uint8_t *dst, int width, int height)
{
    for (int i = 0; i < width * height * 2; i += 2) {
        *dst++ = *(src + i);
    }

    for (int y = 0; y < height - 1; y +=2) {
        for (int j = 0; j < width * 2; j += 4) {
            *dst++ = (*(src + 3 + j) + *(src + 3 + j + width * 2) + 1) >> 1;    //v
            *dst++ = (*(src + 1 + j) + *(src + 1 + j + width * 2) + 1) >> 1;    //u
        }
        src += width * 2 * 2;
    }

    if (height & 1)
        for (int j = 0; j < width * 2; j += 4) {
            *dst++ = *(src + 3 + j);    //v
            *dst++ = *(src + 1 + j);    //u
        }
}

void CameraUtil::YUYVToYV12(uint8_t *src, uint8_t *dst, int width, int height)
{
	//width should be an even number.
	//uv ALIGN 32.
	int i,j,c_stride,c_size,y_size,cb_offset,cr_offset;
	unsigned char *dst_copy,*src_copy;

	dst_copy = dst;
	src_copy = src;

	y_size = width*height;
	c_stride = ALIGN(width/2, 16);
	c_size = c_stride * height/2;
	cr_offset = y_size;
	cb_offset = y_size+c_size;

	for (i=0;i< y_size;i++) {
		*dst++ = *src;
		src += 2;
	}

	dst = dst_copy;
	src = src_copy;

	for (i=0;i<height;i+=2) {
		for (j=1;j<width*2;j+=4) {//one line has 2*width bytes for yuyv.
			//ceil(u1+u2)/2
			*(dst+cr_offset+j/4)= (*(src+j+2) + *(src+j+2+width*2) + 1)/2;
			*(dst+cb_offset+j/4)= (*(src+j) + *(src+j+width*2) + 1)/2;
		}
		dst += c_stride;
		src += width*4;
	}
}

int CameraUtil::align(int x, int y) {
	// y must be a power of 2.
	return (x + y - 1) & ~(y - 1);
}

void CameraUtil::ReSizeNV21(uint8_t *src, uint8_t *img,
				uint32_t width, uint32_t height,
				uint32_t stride, uint32_t input_width, uint32_t input_height)
{
    structConvImage input = {(mmInt32)input_width,
                         (mmInt32)input_height,
                         (mmInt32)input_width,
                         IC_FORMAT_YCbCr420_lp,
                         (mmByte *) src,
                         (mmByte *) src + input_width * input_height,
                         0};

    structConvImage output = {(mmInt32)width,
                              (mmInt32)height,
                              (mmInt32)stride,
                              IC_FORMAT_YCbCr420_lp,
                              (mmByte *) img,
                              (mmByte *) img + stride * height,
                              0};

    if (!VT_resizeFrame_Video_opt2_lp(&input, &output, NULL, 0))
        ALOGE("Sclale NV21 frame down failed!\n");
}
int CameraUtil::ScaleYV12(uint8_t* src, int src_width, int src_height,
	uint8_t* dst, int dst_width, int dst_height) {
	            ALOGI("Sclale YV12 frame down \n");
            int ret = libyuv::I420Scale(src, src_width,
                                        src + src_width * src_height, src_width / 2,
                                        src + src_width * src_height + src_width * src_height / 4, src_width / 2,
                                        src_width, src_height,
                                        dst, dst_width,
                                        dst + dst_width * dst_height, dst_width / 2,
                                        dst + dst_width * dst_height + dst_width * dst_height / 4, dst_width / 2,
                                        dst_width, dst_height,
                                        libyuv::kFilterNone);
            if (ret < 0)
                ALOGE("Sclale YV12 frame down failed!\n");
			return ret;
}
int CameraUtil::MJPEGToNV21(uint8_t* src, int src_len,int src_width, int src_height,
				uint8_t* dst, int dst_width, int dst_height, int dst_stride, uint8_t* tmpBuf) {

	#if ANDROID_PLATFORM_SDK_VERSION > 23
                uint8_t *vBuffer = new uint8_t[src_width * src_height / 4];
                if (vBuffer == NULL) {
                    ALOGE("alloc temperary v buffer failed\n");
					return -1;
                }
                uint8_t *uBuffer = new uint8_t[src_width * src_width / 4];
                if (uBuffer == NULL) {
                    if (vBuffer != NULL) {
                        delete []vBuffer;
                    }
                    ALOGE("alloc temperary u buffer failed\n");
					return -1;
                }
				if (src_width == dst_width && src_height == dst_height) {
					if (ConvertToI420(src, src_len, dst, dst_stride, uBuffer, (dst_stride + 1) / 2,
					vBuffer, (dst_stride + 1) / 2, 0, 0, src_width, src_height,
					src_width, src_height, libyuv::kRotate0, libyuv::FOURCC_MJPG) != 0) {
						ALOGE("Decode MJPEG frame failed\n");
						delete []vBuffer;
						delete []uBuffer;
						return -1;
					}
					uint8_t *pUVBuffer = dst + dst_stride * src_height;
					for (int i = 0; i < (int)(dst_stride * src_height / 4); i++) {
						*pUVBuffer++ = *(vBuffer + i);
						*pUVBuffer++ = *(uBuffer + i);
						}
					delete []vBuffer;
					delete []uBuffer;
				}else{
	                if (ConvertToI420(src, src_len, tmpBuf, src_width, uBuffer, (src_width + 1) / 2,
	                      vBuffer, (src_width + 1) / 2, 0, 0, src_width, src_height,
	                      src_width, src_height, libyuv::kRotate0, libyuv::FOURCC_MJPG) != 0) {
	                    ALOGE("Decode MJPEG frame failed\n");
	                    delete []vBuffer;
	                    delete []uBuffer;
						return -1;
	                }
	                uint8_t *pUVBuffer = tmpBuf + src_width * src_height;
	                for (int i = 0; i < (int)(src_width * src_height / 4); i++) {
	                    *pUVBuffer++ = *(vBuffer + i);
	                    *pUVBuffer++ = *(uBuffer + i);
	                }
	                delete []vBuffer;
	                delete []uBuffer;
					ReSizeNV21(tmpBuf, dst, dst_width,
						dst_height, dst_stride,src_width,src_height);
				}
#else
                if (ConvertMjpegToNV21(src, src_len, tmpBuf,
                            src_width, tmpBuf + src_width * src_height,
                            (src_width + 1) / 2, src_width,src_height, src_width,
                            src_height, libyuv::FOURCC_MJPG) != 0) {
                    return -1;
                }
				if ((src_width == dst_width) && (src_height == dst_height)) {
					memcpy(dst, tmpBuf, dst_width * dst_height * 3/2);
				}else {
	                if ((dst_height % 2) != 0) {
	                    ALOGD("%d, b.height = %d", __LINE__, dst_height);
	                    dst_height = dst_height - 1;
	                }
	                ReSizeNV21(tmpBuf, dst, dst_width, dst_height,
								dst_stride,src_width,src_height);
				}

#endif
	return 0;
}

int CameraUtil::MJPEGToRGB(uint8_t* src, int src_len,int src_width, int src_height,
		uint8_t* dst) {

		uint8_t *tmpBuf = new uint8_t[src_width * src_height * 3 / 2];
		if ( tmpBuf == NULL) {
						ALOGE("new buffer failed!\n");
						return -1;
		}

	#if ANDROID_PLATFORM_SDK_VERSION > 23
			uint8_t *vBuffer = new uint8_t[src_width * src_height / 4];
			if (vBuffer == NULL)
				ALOGE("alloc temperary v buffer failed\n");
			uint8_t *uBuffer = new uint8_t[src_width * src_height / 4];
			if (uBuffer == NULL) {
				 if (vBuffer != NULL) {
	                    delete []vBuffer;
	                }
				ALOGE("alloc temperary u buffer failed\n");
				return -1;
			}

			if (ConvertToI420(src, src_len, tmpBuf, src_width, uBuffer, (src_width + 1) / 2,
						  vBuffer, (src_width + 1) / 2, 0, 0, src_width, src_height,
						  src_width, src_height, libyuv::kRotate0, libyuv::FOURCC_MJPG) != 0) {
				ALOGE("Decode MJPEG frame failed\n");
				delete []vBuffer;
				delete []uBuffer;
				return -1;
			} else {

				uint8_t *pUVBuffer = tmpBuf + src_width * src_height;
				for (int i = 0; i < (int)(src_width * src_height / 4); i++) {
					*pUVBuffer++ = *(vBuffer + i);
					*pUVBuffer++ = *(uBuffer + i);
				}

				delete []vBuffer;
				delete []uBuffer;
				nv21_to_rgb24(tmpBuf,dst,src_width,src_height);
				if (tmpBuf != NULL)
					delete [] tmpBuf;
			}
#else
			if (ConvertMjpegToNV21(src, src_len, tmpBuf,
				src_width, tmpBuf + src_width * src_height, (src_width + 1) / 2, src_width,
				src_height, src_width, src_height, libyuv::FOURCC_MJPG) != 0) {
				ALOGE("Decode MJPEG frame failed\n");
				return -1;
			} else {
				nv21_to_rgb24(tmpBuf,dst,src_width,src_height);
				if (tmpBuf != NULL)
					delete [] tmpBuf;
			}
#endif
	return 0;
}

int CameraUtil::YUYVScaleYV12(uint8_t* src, int src_width, int src_height,
	uint8_t* dst, int dst_width, int dst_height) {
			int ret = 0;
			uint8_t *tmp_buffer = new uint8_t[src_width * src_height * 3 / 2];

			if ( tmp_buffer == NULL) {
			    ALOGE("new buffer failed!\n");
			    return -1;
			}

			YUYVToYV12(src, tmp_buffer, src_width, src_height);
			ret = ScaleYV12(tmp_buffer,src_width, src_height,
								dst,dst_width,dst_height);
			if (ret < 0)
			    ALOGE("Sclale YV12 frame down failed!\n");
			delete [] tmp_buffer;

			return ret;
}

int CameraUtil::MJPEGScaleYV12(uint8_t* src, int src_len,int src_width, int src_height,
	uint8_t* dst, int dst_width, int dst_height, bool scale) {

	int ret = 0;

	if (scale) {
		uint8_t *tmp_buffer = new uint8_t[src_width * src_height * 3 / 2];

		if ( tmp_buffer == NULL) {
			ALOGE("new buffer failed!\n");
			return -1;
		}

		if (ConvertToI420(src,src_len, tmp_buffer, src_width,
		   tmp_buffer + src_width * src_height + src_width * src_height / 4, (src_width + 1) / 2,
		   tmp_buffer + src_width * src_height, (src_width + 1) / 2, 0, 0, src_width, src_height,
		   src_width, src_height, libyuv::kRotate0, libyuv::FOURCC_MJPG) != 0) {
			ALOGE("Decode MJPEG frame failed\n");
			return -1;
		}

		ret = libyuv::I420Scale(tmp_buffer, src_width,
		                        tmp_buffer + src_width * src_height, src_width / 2,
		                        tmp_buffer + src_width * src_height + src_width * src_height / 4, src_width / 2,
		                        src_width, src_height,
		                        dst, dst_width,
		                        dst + dst_width * dst_height, dst_width / 2,
		                        dst + dst_width * dst_height + dst_width * dst_height / 4, dst_width / 2,
		                        dst_width, dst_height,
		                        libyuv::kFilterNone);
		if (ret < 0)
		ALOGE("Sclale YV12 frame down failed!\n");

		delete [] tmp_buffer;
		return ret;
	}else {
			if (ConvertToI420(src,src_len,dst, src_width, dst + src_width * src_height + src_width * src_height / 4, (src_width + 1) / 2,
			        dst + src_width * src_height, (src_width + 1) / 2, 0, 0, src_width, src_height,
			        src_width, src_height, libyuv::kRotate0, libyuv::FOURCC_MJPG) != 0) {
				ALOGE("Decode MJPEG frame failed\n");
				return -1;
			}
	}
	return ret;
}
