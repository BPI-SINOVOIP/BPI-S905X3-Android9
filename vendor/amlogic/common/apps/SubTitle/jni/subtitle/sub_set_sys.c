/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: c file
*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <log_print.h>
#include "linux/ioctl.h"
#include "amstream.h"
#include <Amsysfsutils.h>
#include "sub_io.h"

#define CODEC_AMSUBTITLE_DEVICE     "/dev/amsubtitle"

#define msleep(n)   usleep(n*1000)

int set_sysfs_int(const char *path, int val)
{
    return amsysfs_set_sysfs_int(path, val);
}

int get_sysfs_int(const char *path)
{
    return amsysfs_get_sysfs_int(path);
}

int get_sysfs_str(const char *path, char *valstr, int size)
{
    return amsysfs_get_sysfs_str(path, valstr, size);
}

int set_subtitle_enable(int enable)
{
    log_print("[%s::%d] %d,----- \n", __FUNCTION__, __LINE__, enable);
    return set_sysfs_int("/sys/class/subtitle/enable", enable);
}

int get_subtitle_enable()
{
    log_print("[%s::%d] %d,------ \n", __FUNCTION__, __LINE__,
              get_sysfs_int("/sys/class/subtitle/enable"));
    return get_sysfs_int("/sys/class/subtitle/enable");
}

int get_subtitle_num()
{
    int ret = 0;
    if (getIOType() == IO_TYPE_DEV) {
        ret = get_sysfs_int("/sys/class/subtitle/total");
    }
    else if (getIOType() == IO_TYPE_SOCKET) {
        ret = getInfo(TYPE_TOTAL);
    }
    return ret;
}

int get_subtitle_language(char *valstr, int size)
{
    return get_sysfs_str("/sys/class/subtitle/sub_language", valstr, size);
}

int get_subtitle_title_info(char *valstr, int size)
{
    return get_sysfs_str("/sys/class/subtitle/sub_title_info", valstr,
                         size);
}

int set_subtitle_curr(int curr)
{
    return set_sysfs_int("/sys/class/subtitle/curr", curr);
}

int get_subtitle_curr()
{
    return get_sysfs_int("/sys/class/subtitle/curr");
}

int set_subtitle_height(int heitht)
{
    return set_sysfs_int("/sys/class/subtitle/heitht", heitht);
}

int set_subtitle_width(int width)
{
    return set_sysfs_int("/sys/class/subtitle/width", width);
}

int set_subtitle_size(int size)
{
    return set_sysfs_int("/sys/class/subtitle/size", size);
}

int set_subtitle_errnums(int errnums){
    log_print("[%s::%d] errnums:%d----- \n", __FUNCTION__, __LINE__, errnums);
    return set_sysfs_int("/sys/class/subtitle/errnums", errnums);
}

int get_subtitle_size()
{
    return get_sysfs_int("/sys/class/subtitle/size");
}

int set_subtitle_data(int data)
{
    return set_sysfs_int("/sys/class/subtitle/data", data);
}

int get_subtitle_data()
{
    return get_sysfs_int("/sys/class/subtitle/data");
}

int get_subtitle_startpts()
{
    int ret = 0;

    if (getIOType() == IO_TYPE_DEV) {
        ret = get_sysfs_int("/sys/class/subtitle/startpts");
    }
    else if (getIOType() == IO_TYPE_SOCKET) {
        ret = getInfo(TYPE_STARTPTS);
    }

    return ret;
}

int get_subtitle_fps()
{
    return get_sysfs_int("/sys/class/subtitle/fps");
}

int get_subtitle_subtype()
{
    int ret = 0;

    if (getIOType() == IO_TYPE_DEV) {
        ret = get_sysfs_int("/sys/class/subtitle/subtype");
    }
    else if (getIOType() == IO_TYPE_SOCKET) {
        ret = getInfo(TYPE_SUBTYPE);
    }

    return ret;
}
