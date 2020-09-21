/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <errno.h>
#include <../mediaconfig/media_config.h>
#include <linux/ioctl.h>

int print_usage(int argc, char **argv)
{
    printf("usage:\n");
    printf("\t%s -a # listall\n", argv[0]);
    printf("\t%s -s media.xxxx 1 #set media.xxxx to 1 \n", argv[0]);
    printf("\t%s  media.xxx #get media.xxx setting\n", argv[0]);
    exit(0);
}
int media_open(const char *path, int flags)
{
    int fd = 0;
    fd = open(path, flags);
    if (fd < 0) {
        printf("open [%s] failed,ret = %d errno=%d\n", path, fd, errno);
        return fd;
    }
    return fd;
}

int media_close(int fd)
{
    int res = 0;
    if (fd) {
        res = close(fd);
    }
    return res;
}

int main(int argc, char **argv)
{
    char *buf = (char *)malloc(128 * 1024);
    if (argc <= 1) {
        print_usage(argc, argv);
        return 0;
    }
    if (!buf) {
        printf("no mem....\n");
        return 0;
    }
    if (!strcmp(argv[1], "-a")) {
        int i;
        i = media_config_list(buf, 128 * 1024);
        if (i > 0) {
            puts(buf);
        } else {
            printf("dump failed.%s(%d)\n", strerror(-i), i);
        }
        return 0;
    } else if (!strcmp(argv[1], "-s") && (argc == 3 || argc == 4)) {
        int i = -1;
        if (argc == 4) {
            i = media_set_cmd_str(argv[2], argv[3]);
        } else if (argc == 3 && strstr(argv[2], "=") != NULL) {
            i = media_set_cmd_str(argv[2], strstr(argv[2], "=") + 1);
        } else {
            print_usage(argc, argv);
        }
        if (i < 0) {
            printf("set config failed.%s(%d)\n", strerror(-i), i);
        } else {
            i = media_get_cmd_str(argv[2], buf, 128 * 1024);
            if (i > 0)
                printf("after set:[%s]=[%s]\n",argv[2],buf);
            else
                printf("set ok,bug getfailed.%s(%d)\n", strerror(i), i);
        }
    } else if (argc == 2) {
        int i = media_get_cmd_str(argv[1], buf, 128 * 1024);
        if (i > 0) {
            printf("get: %s=%s\n", argv[1], buf);
        } else {
            printf("get config failed.%s(%d)\n", strerror(-i), i);
        }
    } else {
        print_usage(argc, argv);
    }
    free(buf);
    return 0;
}

