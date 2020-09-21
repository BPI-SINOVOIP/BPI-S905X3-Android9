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
#include <video_ctl.h>

#ifdef  __cplusplus
extern "C" {
#endif
int media_set_vfm_map(const char* val)
{
    return media_set_vfm_map_str(val);
}
int media_get_vfm_map(char* val, int size)
{
    media_get_vfm_map_str(val,size);
	return 0;
}

int media_rm_vfm_map(const char* name, const char* val)
{
	return media_rm_vfm_map_str(name,val);
}

int media_add_vfm_map(const char* name, const char* val)
{
	return media_add_vfm_map_str(name, val);
}

#ifdef  __cplusplus
}
#endif

