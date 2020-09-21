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
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <cutils/log.h>
#include <sys/ioctl.h>
#include <cutils/properties.h>
#include <pthread.h>
#include "media_config_hw.h"

#define MEDIA_FD_INDEX          0 /*media.*/
#define MEDIA_DECODER_FD_INDEX  1 /*media.decoder*/
#define MEDIA_PARSER_FD_INDEX   2 /*media.parser*/
#define MEDIA_VIDEO_FD_INDEX        3 /*media.video*/
#define MEDIA_SYNC_FD_INDEX         4 /*media.sync*/
#define MEDIA_CODEC_MM_FD_INDEX     5 /*media.code_mm*/
#define MEDIA_AUDIO_FD_INDEX        6 /*media.audio*/
#define MEDIA_FD_INDEX_MAX          7 /*MAX*/

struct device_info_t {
    const char *dev;
    pthread_mutex_t lock;
    int fd;
};

static struct device_info_t device_info[] = {
    {"/dev/media", PTHREAD_MUTEX_INITIALIZER, -1},
    {"/dev/media.decoder", PTHREAD_MUTEX_INITIALIZER, -1},
    {"/dev/media.parser", PTHREAD_MUTEX_INITIALIZER, -1},
    {"/dev/media.video", PTHREAD_MUTEX_INITIALIZER, -1},
    {"/dev/media.tsync", PTHREAD_MUTEX_INITIALIZER, -1},
    {"/dev/media.codec_mm", PTHREAD_MUTEX_INITIALIZER, -1},
    {"/dev/media.audio", PTHREAD_MUTEX_INITIALIZER, -1},
    {"nal", PTHREAD_MUTEX_INITIALIZER, -1},
};


static int require_fd_device_lock(struct device_info_t *dev)
{
    pthread_mutex_lock(&dev->lock);
    if (dev->fd < 0) {
        dev->fd = media_config_open(dev->dev, O_RDWR);
    }
    if (dev->fd < 0) {
        pthread_mutex_unlock(&dev->lock);
    }
    return dev->fd;
}
static int release_fd_device_unlock(struct device_info_t *dev)
{
    pthread_mutex_unlock(&dev->lock);
    return 0;
}

static int local_media_set_cmd_str(int dev_index, const char *cmd, const char *val)
{
    int err;
    struct device_info_t *dev;

    if (dev_index < 0 || dev_index >= MEDIA_FD_INDEX_MAX) {
        return -1;
    }
    dev = &device_info[dev_index];
    err = require_fd_device_lock(dev);
    if (err < 0) {
        return err;
    }
    err = media_config_set_str(dev->fd, cmd, val);
    release_fd_device_unlock(dev);
    return err;
}

static int local_media_get_cmd_str(int dev_index, const char *cmd, char *val, int len)
{
    int err;
    struct device_info_t *dev;
    if (dev_index < 0 || dev_index >= MEDIA_FD_INDEX_MAX) {
        return -1;
    }
    dev = &device_info[dev_index];
    err = require_fd_device_lock(dev);
    if (err < 0) {
        return err;
    }
    err =  media_config_get_str(dev->fd, cmd, val, len);
    release_fd_device_unlock(dev);
    return err;
}

static int local_media_config_list(int dev_index, char *val, int len)
{
    int err;
    struct device_info_t *dev;
    if (dev_index < 0 || dev_index >= MEDIA_FD_INDEX_MAX) {
        return -1;
    }
    dev = &device_info[dev_index];
    err = require_fd_device_lock(dev);
    if (err < 0) {
        return err;
    }
    err =  media_config_list_cmd(dev->fd, val, len);
    release_fd_device_unlock(dev);
    return err;
}

int media_config_list(char *val, int len)
{
    return local_media_config_list(MEDIA_FD_INDEX, val, len);
}


int media_set_cmd_str(const char *cmd, const char *val)
{

    return local_media_set_cmd_str(MEDIA_FD_INDEX, cmd, val);
}
int media_get_cmd_str(const char *cmd, char *val, int len)
{
    return local_media_get_cmd_str(MEDIA_FD_INDEX, cmd, val, len);
}

int media_decoder_set_cmd_str(const char *cmd, const char *val)
{
    return local_media_set_cmd_str(MEDIA_DECODER_FD_INDEX, cmd, val);
}
int media_decoder_get_cmd_str(const char *cmd, char *val, int len)
{
    return local_media_get_cmd_str(MEDIA_DECODER_FD_INDEX, cmd, val, len);
}

int media_parser_set_cmd_str(const char *cmd, const char *val)
{
    return local_media_set_cmd_str(MEDIA_PARSER_FD_INDEX, cmd, val);
}

int media_parser_get_cmd_str(const char *cmd, char *val, int len)
{
    return local_media_get_cmd_str(MEDIA_PARSER_FD_INDEX, cmd, val, len);
}

int media_video_set_cmd_str(const char *cmd, const char *val)
{
    return local_media_set_cmd_str(MEDIA_VIDEO_FD_INDEX, cmd, val);
}

int media_video_get_cmd_str(const char *cmd, char *val, int len)
{
    return local_media_get_cmd_str(MEDIA_VIDEO_FD_INDEX, cmd, val, len);
}

int media_sync_set_cmd_str(const char *cmd, const char *val)
{
    return local_media_set_cmd_str(MEDIA_SYNC_FD_INDEX, cmd, val);
}

int media_sync_get_cmd_str(const char *cmd, char *val, int len)
{
    return local_media_get_cmd_str(MEDIA_SYNC_FD_INDEX, cmd, val, len);
}

int media_codecmm_set_cmd_str(const char *cmd, const char *val)
{
    return local_media_set_cmd_str(MEDIA_CODEC_MM_FD_INDEX, cmd, val);
}

int media_codecmm_get_cmd_str(const char *cmd, char *val, int len)
{
    return local_media_get_cmd_str(MEDIA_CODEC_MM_FD_INDEX, cmd, val, len);
}

int media_audio_set_cmd_str(const char *cmd, const char *val)
{
    return local_media_set_cmd_str(MEDIA_AUDIO_FD_INDEX, cmd, val);
}

int media_audio_get_cmd_str(const char *cmd, char *val, int len)
{
    return local_media_get_cmd_str(MEDIA_AUDIO_FD_INDEX, cmd, val, len);
}





