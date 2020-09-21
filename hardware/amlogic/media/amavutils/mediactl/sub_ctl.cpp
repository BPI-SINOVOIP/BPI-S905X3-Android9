/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#define LOG_TAG "media_ctl"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <cutils/log.h>
#include <amports/amstream.h>
#include "common_ctl.h"
#include <sub_ctl.h>

#ifdef  __cplusplus
extern "C" {
#endif

int media_get_subtitle_fps()
{
	return media_sub_getinfo(SUB_FPS);
}

int media_set_subtitle_fps(int fps)
{
	return media_sub_setinfo(SUB_FPS, fps);
}

int media_get_subtitle_total()
{
	return media_sub_getinfo(SUB_TOTAL);
}

int media_set_subtitle_total(int total)
{
	return media_sub_setinfo(SUB_TOTAL, total);
}

int media_get_subtitle_enable()
{
	return media_sub_getinfo(SUB_ENABLE);
}

int media_set_subtitle_enable(int enable)
{
	return media_sub_setinfo(SUB_ENABLE, enable);
}

int media_get_subtitle_index()
{
	return media_sub_getinfo(SUB_INDEX);
}

int media_set_subtitle_index(int index)
{
	return media_sub_setinfo(SUB_INDEX, index);
}

int media_get_subtitle_width()
{
	return media_sub_getinfo(SUB_WIDTH);
}

int media_set_subtitle_width(int width)
{
	return media_sub_setinfo(SUB_WIDTH, width);
}

int media_get_subtitle_height()
{
	return media_sub_getinfo(SUB_HEIGHT);
}

int media_set_subtitle_height(int height)
{
	return media_sub_setinfo(SUB_HEIGHT, height);
}

int media_get_subtitle_type()
{
	return media_sub_getinfo(SUB_TYPE);
}

int media_set_subtitle_type(int type)
{
	return media_sub_setinfo(SUB_TYPE, type);
}

int media_get_subtitle_subtype()
{
	return media_sub_getinfo(SUB_SUBTYPE);
}

int media_set_subtitle_subtype(int type)
{
	return media_sub_setinfo(SUB_SUBTYPE, type);
}

int media_get_subtitle_curr()
{
	return media_sub_getinfo(SUB_CURRENT);
}

int media_set_subtitle_curr(int num)
{
	return media_sub_setinfo(SUB_CURRENT, num);
}

int media_get_subtitle_startpts()
{
	return media_sub_getinfo(SUB_START_PTS);
}

int media_set_subtitle_startpts(int startpts)
{
	return media_sub_setinfo(SUB_START_PTS, startpts);
}

int media_get_subtitle_reset()
{
	return media_sub_getinfo(SUB_RESET);
}

int media_set_subtitle_reset(int res)
{
	return media_sub_setinfo(SUB_RESET, res);
}

int media_get_subtitle_size()
{
	return media_sub_getinfo(SUB_DATA_T_SIZE);
}

int media_set_subtitle_size(int size)
{
	return media_sub_setinfo(SUB_DATA_T_SIZE, size);
}

#ifdef  __cplusplus
}
#endif
