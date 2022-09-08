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

#ifndef __UTIL_H
#define __UTIL_H
class CameraUtil {
	public:
		CameraUtil(){};
		~CameraUtil(){};
	public:
		inline void yuv_to_rgb24(unsigned char y,unsigned char u,unsigned char v,unsigned char *rgb);
		void yuyv422_to_rgb24(unsigned char *buf, unsigned char *rgb, int width, int height);
		void nv21_to_rgb24(unsigned char *buf, unsigned char *rgb, int width, int height);
		void nv21_memcpy_align32(unsigned char *dst, unsigned char *src, int width, int height);
		void yv12_memcpy_align32(unsigned char *dst, unsigned char *src, int width, int height);
		void YUYVToNV21(uint8_t *src, uint8_t *dst, int width, int height);
		void YUYVToYV12(uint8_t *src, uint8_t *dst, int width, int height);
		void rgb24_memcpy(unsigned char *dst, unsigned char *src, int width, int height);
		float sqrtf_approx(float r);
		void ReSizeNV21(uint8_t *src, uint8_t *img,
				uint32_t width, uint32_t height,
				uint32_t stride,uint32_t input_width,
				uint32_t input_height);

		int MJPEGToRGB(uint8_t* src, int src_len,int src_width, int src_height,
				uint8_t* dst);

		int MJPEGToNV21(uint8_t* src, int src_len,int src_width, int src_height,
							uint8_t* dst, int dst_width, int dst_height,
							int dst_stride,uint8_t* tmpBuf);
		int ScaleYV12(uint8_t* src, int src_width, int src_height,
							uint8_t* dst, int dst_width, int dst_height);

		int YUYVScaleYV12(uint8_t* src, int src_width, int src_height,
				uint8_t* dst, int dst_width, int dst_height);

		int MJPEGScaleYV12(uint8_t* src, int src_len,int src_width, int src_height,
			uint8_t* dst, int dst_width, int dst_height,bool scale);
	private:
		int align(int x, int y);
};
#endif
