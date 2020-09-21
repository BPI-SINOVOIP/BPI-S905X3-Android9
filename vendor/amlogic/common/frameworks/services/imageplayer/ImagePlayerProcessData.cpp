/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: C++ file
 */
#define LOG_NDEBUG 0
#define LOG_TAG "ImagePlayerProcessData"

#include "utils/Log.h"
#include "ImagePlayerProcessData.h"
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>

void copy_data_to_mmap_buf(int fd, char *user_data,
                           int width, int height,
                           int format)
{
	char *mbuf = NULL;
	char *p;
	char *q;
	char *ref;
	int size;
	int i, j;
	int bp = ((width + 0x1f) & ~0x1f) * 3;
	int frame_height = (height + 0xf) & ~0xf;

	size = bp * frame_height;
	mbuf = (char*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if ((mbuf == NULL) || (mbuf == MAP_FAILED)) {
		ALOGI("mmap failed, %s\n", strerror(errno));
		return;
	}

	switch (format) {
	case 0:	/* RGB */
		p = (char *)mbuf;
		q = (char *)user_data;

		for (j = 0; j < height; j++) {
			memcpy(p, q, width * 3);
			q += width * 3;
			p += bp;
		}
		ALOGD("RGB copy finish");
		break;
	case 1: /* RGBA */
		p = ref = (char *)mbuf;
		q = (char *)user_data;

		for (j = 0; j < height; j++) {
			p = ref;
			for (i = 0; i < width; i++) {
				memcpy(p, q, 3);
				q += 4;
				p += 3;
			}
			ref += bp;
		}
		ALOGD("RGBA copy finish");
		break;
	default:
		ALOGE("don't support format\n");
		break;
	}

	munmap(mbuf, size);
}
