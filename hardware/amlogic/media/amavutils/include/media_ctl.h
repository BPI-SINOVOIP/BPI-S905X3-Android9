/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#ifndef MEDIA_CTL_H
#define MEDIA_CTL_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <cutils/log.h>
#include <../mediaconfig/media_config.h>
#include <amports/amstream.h>
#include "../mediactl/common_ctl.h"
#ifdef  __cplusplus
extern "C" {
#endif

typedef struct{
    const char *device;
	const char *path;
    int(*mediactl_setval)(const char* path, int val);
	int(*mediactl_getval)(const char* path);
	int(*mediactl_setstr)(const char* path, char* val);
	int(*mediactl_getstr)(const char* path, char* buf,int size);
}MediaCtlPool;

int mediactl_set_str_func(const char* path, char* val);
int mediactl_get_str_func(const char* path, char* valstr, int size);
int mediactl_set_int_func(const char* path, int val);
int mediactl_get_int_func(const char* path);

#ifdef  __cplusplus
}
#endif
#endif
