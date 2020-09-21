/**
 * f2fs_format_utils.c
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *             http://www.samsung.com/
 *
 * Dual licensed under the GPL or LGPL version 2 licenses.
 */
#ifndef _LARGEFILE_SOURCE
#define _LARGEFILE_SOURCE
#endif
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <f2fs_fs.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#ifndef ANDROID_WINDOWS_HOST
#include <sys/ioctl.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>

#ifdef HAVE_LINUX_FS_H
#include <linux/fs.h>
#endif
#ifdef HAVE_LINUX_FALLOC_H
#include <linux/falloc.h>
#endif

#ifndef BLKDISCARD
#define BLKDISCARD	_IO(0x12,119)
#endif
#ifndef BLKSECDISCARD
#define BLKSECDISCARD	_IO(0x12,125)
#endif

static int trim_device(int i)
{
#ifndef ANDROID_WINDOWS_HOST
	unsigned long long range[2];
	struct stat *stat_buf;
	struct device_info *dev = c.devices + i;
	u_int64_t bytes = dev->total_sectors * dev->sector_size;
	int fd = dev->fd;

	stat_buf = malloc(sizeof(struct stat));
	if (fstat(fd, stat_buf) < 0 ) {
		MSG(1, "\tError: Failed to get the device stat!!!\n");
		free(stat_buf);
		return -1;
	}

	range[0] = 0;
	range[1] = bytes;

#if defined(WITH_BLKDISCARD) && defined(BLKDISCARD)
	MSG(0, "Info: [%s] Discarding device\n", dev->path);
	if (S_ISREG(stat_buf->st_mode)) {
#if defined(HAVE_FALLOCATE) && defined(FALLOC_FL_PUNCH_HOLE)
		if (fallocate(fd, FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE,
				range[0], range[1]) < 0) {
			MSG(0, "Info: fallocate(PUNCH_HOLE|KEEP_SIZE) is failed\n");
		}
#endif
		free(stat_buf);
		return 0;
	} else if (S_ISBLK(stat_buf->st_mode)) {
		if (dev->zoned_model != F2FS_ZONED_NONE) {
			free(stat_buf);
			return f2fs_reset_zones(i);
		}
#ifdef BLKSECDISCARD
		if (ioctl(fd, BLKSECDISCARD, &range) < 0) {
			MSG(0, "Info: This device doesn't support BLKSECDISCARD\n");
		} else {
			MSG(0, "Info: Secure Discarded %lu MB\n",
					(unsigned long)stat_buf->st_size >> 20);
			free(stat_buf);
			return 0;
		}
#endif
		if (ioctl(fd, BLKDISCARD, &range) < 0) {
			MSG(0, "Info: This device doesn't support BLKDISCARD\n");
		} else {
			MSG(0, "Info: Discarded %llu MB\n", range[1] >> 20);
		}
	} else {
		free(stat_buf);
		return -1;
	}
#endif
	free(stat_buf);
#endif
	return 0;
}

int f2fs_trim_devices(void)
{
	int i;

	for (i = 0; i < c.ndevs; i++)
		if (trim_device(i))
			return -1;
	c.trimmed = 1;
	return 0;
}
