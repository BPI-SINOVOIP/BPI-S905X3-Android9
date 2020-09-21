/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#define LOG_TAG "meda_config_hw"

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <cutils/log.h>
#include <sys/ioctl.h>
#include <cutils/properties.h>
#include "configs_api.h"

int media_config_open(const char *path, int flags)
{
    int fd = open(path, flags);
    if (fd < 0) {
        if (-errno < 0)
            fd = -errno;
        ALOGE("open %s, failed %d, err=%s(%d)\n",
            path, fd, strerror(errno),
            -errno);
    }
    return fd;
}
int media_config_close(int fd)
{
    if (fd >= 0) {
        close(fd);
    }
    return 0;
}

int media_config_set_str(int fd, const char *cmd, const char *val)
{
    struct media_config_io_str io;
    int ret;
    io.subcmd = 0;
    if (!cmd || !val) {
        return -EIO;
    }
    strncpy(io.cmd_path, cmd, sizeof(io.cmd_path));
    strncpy(io.val, val, sizeof(io.val));
    ret = ioctl(fd, MEDIA_CONFIG_SET_CMD_STR, &io);
    return ret;
}
int media_config_get_str(int fd, const char *cmd, char *val, int len)
{
    struct media_config_io_str io;
    int ret;
    io.subcmd = 0;
    if (!cmd || !val) {
        return -EIO;
    }
    strncpy(io.cmd_path, cmd, sizeof(io.cmd_path));
    io.val[0] = '\0';
    ret = ioctl(fd, MEDIA_CONFIG_GET_CMD_STR, &io);
    if (ret == 0) {
        int ret_len = io.ret;
        if (ret_len > len) {
            ret_len = len - 1;
        }
        strncpy(val, io.val, ret_len);
        val[ret_len] = '\0';
    }
    return ret;
}
int media_config_list_cmd(int fd, char *val, int len)
{
    int ret;
    lseek(fd, 0, SEEK_SET);
    ret = read(fd, val, len);
    return ret;
}


