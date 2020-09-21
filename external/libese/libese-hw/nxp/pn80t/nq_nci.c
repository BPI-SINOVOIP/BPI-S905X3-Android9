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
 *
 * Platform implementation for a nq-nci extension driver.
 *
 * The driver presents the following interface on a miscdev:
 * - ioctl():
 *   - for setting and getting power.
 *     This handles SVDD_PWR_REQ and NFC_VEN muxing.
 *     (ESE_RST is not connected in this case.)
 * - read():
 *   - For reading arbitrary amounts of data.
 *     CS is asserted and deasserted on each call, but the clock
 *     also appears to do the same which keeps the ese data available
 *     as far as I can tell.
 * - write():
 *   - For writing arbitrary amounts of data.
 *     CS is asserted as with read() calls, so the less fragmented
 *     the better.
 *
 * All GPIO toggling and chip select requirements are handled behind this
 * interface.
 *
 */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "../include/ese/hw/nxp/pn80t/common.h"

#ifndef UNUSED
#define UNUSED(x) x __attribute__((unused))
#endif

/* From kernel/drivers/nfc/nq-nci.h */
#define ESE_SET_PWR _IOW(0xE9, 0x02, unsigned int)
#define ESE_GET_PWR _IOR(0xE9, 0x03, unsigned int)
#define ESE_CLEAR_GPIO _IOW(0xE9, 0x11, unsigned int)

static const char kDevicePath[] = "/dev/pn81a";

struct PlatformHandle {
  int fd;
};

int platform_toggle_bootloader(void *blob, int val) {
  const struct PlatformHandle *handle = blob;
  if (!handle) {
    return -1;
  }
  return ioctl(handle->fd, ESE_CLEAR_GPIO, val);
}

int platform_toggle_reset(void *blob, int val) {
  const struct PlatformHandle *handle = blob;
  if (!handle) {
    return -1;
  }
  /* 0=power and 1=no power in the kernel. */
  return ioctl(handle->fd, ESE_SET_PWR, !val);
}

void *platform_init(void *hwopts) {
  /* TODO(wad): It may make sense to pass in the dev path here. */
  if (hwopts != NULL) {
    return NULL;
  }

  struct PlatformHandle *handle = calloc(1, sizeof(*handle));
  if (!handle) {
    ALOGE("%s: unable to allocate memory for handle", __func__);
    return NULL;
  }
  handle->fd = open(kDevicePath, O_RDWR);
  if (handle->fd < 0) {
    ALOGE("%s: opening '%s' failed: %s", __func__, kDevicePath,
          strerror(errno));
    free(handle);
    return NULL;
  }
  return handle;
}

int platform_release(void *blob) {
  struct PlatformHandle *handle = blob;
  if (!handle) {
    return -1;
  }
  /* Power off and cooldown should've happened via common code. */
  close(handle->fd);
  free(handle);
  return 0;
}

int platform_wait(void *UNUSED(blob), long usec) {
  return usleep((useconds_t)usec);
}

uint32_t nq_transmit(struct EseInterface *ese, const uint8_t *buf, uint32_t len,
                     int UNUSED(complete)) {
  struct NxpState *ns = NXP_PN80T_STATE(ese);
  const struct Pn80tPlatform *platform = ese->ops->opts;
  uint32_t bytes = 0;
  ALOGV("nq_nci:%s: called [%d]", __func__, len);
  if (len > INT_MAX) {
    ese_set_error(ese, kNxpPn80tErrorTransmitSize);
    ALOGE("Unexpectedly large transfer attempted: %u", len);
    return 0;
  }
  if (len == 0)
    return len;
  const struct PlatformHandle *handle = ns->handle;
  while (bytes < len) {
    ssize_t ret = write(handle->fd, (void *)(buf + bytes), len - bytes);
    if (ret < 0) {
      if (errno == EAGAIN || errno == EINTR) {
        continue;
      }
      ese_set_error(ese, kNxpPn80tErrorTransmit);
      ALOGE("%s: failed to write to hw (ret=%zd, errno=%d)", __func__, ret,
            errno);
      return 0;
    }
    bytes += ret;
  }
  return len;
}

uint32_t nq_receive(struct EseInterface *ese, uint8_t *buf, uint32_t len,
                    int UNUSED(complete)) {
  const struct Pn80tPlatform *platform = ese->ops->opts;
  struct NxpState *ns = NXP_PN80T_STATE(ese);
  ALOGV("nq_nci:%s: called [%d]", __func__, len);
  if (!ns) {
    ALOGE("NxpState was NULL");
    return 0;
  }
  if (len > INT_MAX) {
    ese_set_error(ese, kNxpPn80tErrorReceiveSize);
    ALOGE("Unexpectedly large receive attempted: %u", len);
    return 0;
  }
  const struct PlatformHandle *handle = ns->handle;
  if (len == 0) {
    return 0;
  }
  uint32_t bytes = 0;
  while (bytes < len) {
    ssize_t ret = read(handle->fd, (void *)(buf + bytes), len - bytes);
    if (ret < 0) {
      if (errno == EAGAIN || errno == EINTR) {
        continue;
      }
      ALOGE("%s: failed to read from hw (ret=%zd, errno=%d)", __func__, ret,
            errno);
      ese_set_error(ese, kNxpPn80tErrorReceive);
      return 0;
    }
    bytes += ret;
  }
  ALOGV("%s: read bytes: %u", __func__, bytes);
  return len;
}

static const struct Pn80tPlatform kPn80tNqNciPlatform = {
    .initialize = &platform_init,
    .release = &platform_release,
    .toggle_reset = &platform_toggle_reset,
    .toggle_ven = NULL,
    .toggle_power_req = NULL,
    .toggle_bootloader = &platform_toggle_bootloader,
    .wait = &platform_wait,
};

static const struct EseOperations ops = {
    .name = "NXP PN80T/PN81A (NQ-NCI:PN553)",
    .open = &nxp_pn80t_open,
    .hw_receive = &nq_receive,
    .hw_transmit = &nq_transmit,
    .hw_reset = &nxp_pn80t_reset,
    .transceive = &nxp_pn80t_transceive,
    .poll = &nxp_pn80t_poll,
    .close = &nxp_pn80t_close,
    .opts = &kPn80tNqNciPlatform,
    .errors = kNxpPn80tErrorMessages,
    .errors_count = kNxpPn80tErrorMax,
};
__attribute__((visibility("default")))
ESE_DEFINE_HW_OPS(ESE_HW_NXP_PN80T_NQ_NCI, ops);
