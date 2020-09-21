/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: c file
*/
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <android/log.h>
#include "amstream.h"
#define  LOG_TAG    "sub_control"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

int subtitle_poll_sub_fd(int sub_fd, int timeout)
{
    struct pollfd sub_poll_fd[1];
    if (sub_fd <= 0)
    {
        return 0;
    }
    sub_poll_fd[0].fd = sub_fd;
    sub_poll_fd[0].events = POLLOUT;
    return poll(sub_poll_fd, 1, timeout);
}

int subtitle_get_sub_size_fd(int sub_fd)
{
    unsigned long sub_size;
    int r;
    r = ioctl(sub_fd, AMSTREAM_IOC_SUB_LENGTH, &sub_size);
    if (r < 0)
        return 0;
    else
        return sub_size;
}

int subtitle_read_sub_data_fd(int sub_fd, char *buf, int length)
{
    int data_size = length, r, read_done = 0;
    while (data_size)
    {
        r = read(sub_fd, buf + read_done, data_size);
        if (r < 0)
            return 0;
        else
        {
            data_size -= r;
            read_done += r;
        }
    }
    return 0;
}

#if 0
int update_read_pointer(int sub_handle, int flag)
{
    if (sub_handle < 0)
    {
        LOGE("pgs handle invalid\n\n");
        return sub_handle;
    }
    int r = ioctl(sub_handle, AMSTREAM_IOC_UPDATE_POINTER, flag);
    if (r < 0)
    {
        LOGE("send AMSTREAM_IOC_UPDATE_POINTER failed\n");
        return r;
    }
    return 0;
}

int subtitle_read_sub_data_fd_update(int sub_fd, unsigned int *length)
{
    int data_size = *length, r, read_done = 0;
    if (data_size <= 0)
        return 0;
    char *skip_buf = malloc(data_size);
    if (skip_buf == NULL)
        return 0;
    update_read_pointer(sub_fd, 0);
    while (data_size)
    {
        r = read(sub_fd, skip_buf + read_done, data_size);
        if (r < 0)
        {
            *length = 0;
            free(skip_buf);
            update_read_pointer(sub_fd, 0);
            return 0;
        }
        else
        {
            data_size -= r;
            read_done += r;
            if (data_size > 0)
            {
                LOGI("still data_size %d, %d\n\n\n", data_size,
                     read_done);
            }
        }
    }
    *length = 0;
    free(skip_buf);
    update_read_pointer(sub_fd, 1);
    return 0;
}
#endif
