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

#ifndef __HW_CAMERA_IO_H__
#define __HW_CAMERA_IO_H__
#include <linux/videodev2.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h>             /* getopt_long() */

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <stdbool.h>

#include <linux/videodev2.h>
#include <DebugUtils.h>

#define IO_PREVIEW_BUFFER 2
#define IO_PICTURE_BUFFER 3
#define CLEAR(x) memset(&(x), 0, sizeof(x))

#define V4L2_ROTATE_ID 0x980922  //V4L2_CID_ROTATE

namespace android {
typedef struct FrameV4L2Info {
	struct	v4l2_format format;
	struct	v4l2_buffer buf;
	struct	v4l2_requestbuffers rb;
}FrameV4L2Info;

struct VideoInfoBuffer {
	void* addr;
	int size;
};

class CVideoInfo {
	public:
		CVideoInfo();
		virtual ~CVideoInfo(){};
		struct	v4l2_capability cap;
		FrameV4L2Info preview;
		FrameV4L2Info picture;
		struct VideoInfoBuffer mem[IO_PREVIEW_BUFFER];
		struct VideoInfoBuffer mem_pic[IO_PICTURE_BUFFER];
		//unsigned int canvas[IO_PREVIEW_BUFFER];
		bool isStreaming;
		bool isPicture;
		bool canvas_mode;
		int width;
		int height;
		int formatIn;
		int framesizeIn;
		uint32_t idVendor;
		uint32_t idProduct;

		int idx;
		int fd;

		int tempbuflen;
		int dev_status;
		char sensor_type[64];
	public:
		int camera_init(void);
		int setBuffersFormat(void);
		int start_capturing(void);
		int start_picture(int rotate);
		void stop_picture();
		void releasebuf_and_stop_picture();
		int stop_capturing();
		int releasebuf_and_stop_capturing();
		uintptr_t get_frame_phys();
		void set_device_status();
		int get_device_status();
		void *get_frame();
		void *get_picture();
		int putback_frame();
		int putback_picture_frame();
		int EnumerateFormat(uint32_t pixelformat);
		bool IsSupportRotation();
	private:
	    int set_rotate(int camera_fd, int value);
		int get_frame_index(FrameV4L2Info& info);
};
}
#endif

