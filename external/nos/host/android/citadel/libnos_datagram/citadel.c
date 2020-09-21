/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "libnos_datagram"
#include <log/log.h>
#include <nos/device.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/types.h>
#include <poll.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

/*****************************************************************************/
/* TODO: #include <linux/citadel.h> */
#define CITADEL_IOC_MAGIC               'c'
struct citadel_ioc_tpm_datagram {
    __u64        buf;
    __u32        len;
    __u32        command;
};
#define CITADEL_IOC_TPM_DATAGRAM    _IOW(CITADEL_IOC_MAGIC, 1, \
                         struct citadel_ioc_tpm_datagram)
#define CITADEL_IOC_RESET           _IO(CITADEL_IOC_MAGIC, 2)
/*****************************************************************************/

#define DEV_CITADEL "/dev/citadel0"

static uint8_t in_buf[MAX_DEVICE_TRANSFER];
static int read_datagram(void *ctx, uint32_t command, uint8_t *buf, uint32_t len) {
    struct citadel_ioc_tpm_datagram dg = {
        .buf = (unsigned long)in_buf,
        .len = len,
        .command = command,
    };
    int ret;
    int fd;

    if (!ctx) {

        ALOGE("%s: invalid (NULL) device\n", __func__);
        return -ENODEV;
    }
    fd = *(int *)ctx;
    if (fd < 0) {
        ALOGE("%s: invalid device\n", __func__);
        return -ENODEV;
    }

    if (len > MAX_DEVICE_TRANSFER) {
        ALOGE("%s: invalid len (%d > %d)\n", __func__,
            len, MAX_DEVICE_TRANSFER);
        return -E2BIG;
    }

    ret = ioctl(fd, CITADEL_IOC_TPM_DATAGRAM, &dg);
    if (ret < 0) {
        ALOGE("can't send spi message: %s", strerror(errno));
        return -errno;
    }

    memcpy(buf, in_buf, len);

    return 0;
}

static uint8_t out_buf[MAX_DEVICE_TRANSFER];
static int write_datagram(void *ctx, uint32_t command, const uint8_t *buf, uint32_t len) {
    struct citadel_ioc_tpm_datagram dg = {
        .buf = (unsigned long)out_buf,
        .len = len,
        .command = command,
    };
    int ret;
    int fd;

    if (!ctx) {
        ALOGE("%s: invalid (NULL) device\n", __func__);
        return -ENODEV;
    }
    fd = *(int *)ctx;
    if (fd < 0) {
        ALOGE("%s: invalid device\n", __func__);
        return -ENODEV;
    }

    if (len > MAX_DEVICE_TRANSFER) {
        ALOGE("%s: invalid len (%d > %d)\n", __func__, len,
            MAX_DEVICE_TRANSFER);
        return -E2BIG;
    }

    memcpy(out_buf, buf, len);

    ret = ioctl(fd, CITADEL_IOC_TPM_DATAGRAM, &dg);
    if (ret < 0) {
        ALOGE("can't send spi message: %s", strerror(errno));
        return -errno;
    }

    return 0;
}

static int wait_for_interrupt(void *ctx, int msecs) {
    int fd = *(int *)ctx;
    struct pollfd fds = {fd, POLLIN, 0};
    int rv;

    rv = poll(&fds, 1 /*nfds*/, msecs);
    if (rv < 0) {
        ALOGE("poll: %s", strerror(errno));
    }

    return rv;
}

static int reset(void *ctx) {
    int ret;
    int fd;

    if (!ctx) {

        ALOGE("%s: invalid (NULL) device\n", __func__);
        return -ENODEV;
    }
    fd = *(int *)ctx;
    if (fd < 0) {
        ALOGE("%s: invalid device\n", __func__);
        return -ENODEV;
    }

    ret = ioctl(fd, CITADEL_IOC_RESET);
    if (ret < 0) {
        ALOGE("can't reset Citadel: %s", strerror(errno));
        return -errno;
    }
    return 0;
}

static void close_device(void *ctx) {
    int fd;

    if (!ctx) {
        ALOGE("%s: invalid (NULL) device (ignored)\n", __func__);
        return;
    }
    fd = *(int *)ctx;
    if (fd < 0) {
        ALOGE("%s: invalid device (ignored)\n", __func__);
        return;
    }

    if (close(fd) < 0)
        ALOGE("Problem closing device (ignored): %s", strerror(errno));
    free(ctx);
}

int nos_device_open(const char *device_name, struct nos_device *dev) {
    int fd, *new_fd;

    fd = open(device_name ? device_name : DEV_CITADEL, O_RDWR);
    if (fd < 0) {
        ALOGE("can't open device: %s", strerror(errno));
        return -errno;
    }

    new_fd = (int *)malloc(sizeof(int));
    if (!new_fd) {
        ALOGE("can't malloc new fd: %s", strerror(errno));
        close(fd);
        return -ENOMEM;
    }
    *new_fd = fd;

    dev->ctx = new_fd;
    dev->ops.read = read_datagram;
    dev->ops.write = write_datagram;
    dev->ops.wait_for_interrupt = wait_for_interrupt;
    dev->ops.reset = reset;
    dev->ops.close = close_device;
    return 0;
}
