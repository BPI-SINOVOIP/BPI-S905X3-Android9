/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#ifndef MEDIA_CONFIG_HW___
#define MEDIA_CONFIG_HW___
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <cutils/log.h>
#include <sys/ioctl.h>
#include <cutils/properties.h>
int media_config_open(const char *path, int flags);
int media_config_close(int fd);
int media_config_set_str(int fd, const char *cmd, const char *val);
int media_config_get_str(int fd, const char *cmd, char *val, int len);
int media_config_list_cmd(int fd, char *val, int len);
#endif

