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
 * Defines the PN80T spidev device and platform wrappers consumed in
 * the common code.
 */

#include <fcntl.h>
#include <limits.h>
#include <linux/spi/spidev.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "../include/ese/hw/nxp/pn80t/common.h"
#include "../include/ese/hw/nxp/spi_board.h"

struct Handle {
  int spi_fd;
  struct NxpSpiBoard *board;
};

int gpio_set(int num, int val) {
  char val_path[256];
  char val_chr = (val ? '1' : '0');
  int fd;
  if (num < 0) {
    return 0;
  }
  if (snprintf(val_path, sizeof(val_path), "/sys/class/gpio/gpio%d/value",
               num) >= (int)sizeof(val_path)) {
    return -1;
  }
  printf("Gpio @ %s\n", val_path);
  fd = open(val_path, O_WRONLY);
  if (fd < 0) {
    return -1;
  }
  if (write(fd, &val_chr, 1) < 0) {
    close(fd);
    return -1;
  }
  close(fd);
  return 0;
}

int platform_toggle_ven(void *blob, int val) {
  struct Handle *handle = blob;
  printf("Toggling VEN: %d\n", val);
  return gpio_set(handle->board->gpios[kBoardGpioNfcVen], val);
}

int platform_toggle_reset(void *blob, int val) {
  struct Handle *handle = blob;
  printf("Toggling RST: %d\n", val);
  return gpio_set(handle->board->gpios[kBoardGpioEseRst], val);
}

int platform_toggle_power_req(void *blob, int val) {
  struct Handle *handle = blob;
  printf("Toggling SVDD_PWR_REQ: %d\n", val);
  return gpio_set(handle->board->gpios[kBoardGpioEseSvddPwrReq], val);
}

int gpio_configure(int num, int out, int val) {
  char dir_path[256];
  char numstr[8];
  char dir[5];
  int fd;
  /* <0 is unmapped. No work to do! */
  if (num < 0) {
    return 0;
  }
  if (snprintf(dir, sizeof(dir), "%s", (out ? "out" : "in")) >=
      (int)sizeof(dir)) {
    return -1;
  }
  if (snprintf(dir_path, sizeof(dir_path), "/sys/class/gpio/gpio%d/direction",
               num) >= (int)sizeof(dir_path)) {
    return -1;
  }
  if (snprintf(numstr, sizeof(numstr), "%d", num) >= (int)sizeof(numstr)) {
    return -1;
  }
  fd = open("/sys/class/gpio/export", O_WRONLY);
  if (fd < 0) {
    return -1;
  }
  /* Exporting can only happen once, so instead of stat()ing, just ignore
   * errors. */
  (void)write(fd, numstr, strlen(numstr));
  close(fd);

  fd = open(dir_path, O_WRONLY);
  if (fd < 0) {
    return -1;
  }
  if (write(fd, dir, strlen(dir)) < 0) {
    close(fd);
    return -1;
  }
  close(fd);
  return gpio_set(num, val);
}

void *platform_init(void *hwopts) {
  struct NxpSpiBoard *board = hwopts;
  struct Handle *handle;
  int gpio = 0;

  handle = malloc(sizeof(*handle));
  if (!handle) {
    return NULL;
  }
  handle->board = board;

  /* Initialize the mapped GPIOs */
  for (; gpio < kBoardGpioMax; ++gpio) {
    if (gpio_configure(board->gpios[gpio], 1, 1) < 0) {
      free(handle);
      return NULL;
    }
  }

  handle->spi_fd = open(board->dev_path, O_RDWR);
  if (handle->spi_fd < 0) {
    free(handle);
    return NULL;
  }
  /* If we need anything fancier, we'll need MODE32 in the headers. */
  if (ioctl(handle->spi_fd, SPI_IOC_WR_MODE, &board->mode) < 0) {
    close(handle->spi_fd);
    free(handle);
    return NULL;
  }
  if (ioctl(handle->spi_fd, SPI_IOC_WR_BITS_PER_WORD, &board->bits) < 0) {
    close(handle->spi_fd);
    free(handle);
    return NULL;
  }
  if (ioctl(handle->spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &board->speed) < 0) {
    close(handle->spi_fd);
    free(handle);
    return NULL;
  }
  printf("Linux SPIDev initialized\n");
  return (void *)handle;
}

int platform_release(void *blob) {
  struct Handle *handle = blob;
  close(handle->spi_fd);
  free(handle);
  /* Note, we don't unconfigure the GPIOs. */
  return 0;
}

int platform_wait(void *blob __attribute__((unused)), long usec) {
  return usleep((useconds_t)usec);
}

uint32_t spidev_transmit(struct EseInterface *ese, const uint8_t *buf,
                         uint32_t len, int complete) {
  struct NxpState *ns = NXP_PN80T_STATE(ese);
  struct Handle *handle = ns->handle;
  struct spi_ioc_transfer tr = {
      .tx_buf = (unsigned long)buf,
      .rx_buf = 0,
      .len = (uint32_t)len,
      .delay_usecs = 0,
      .speed_hz = 0,
      .bits_per_word = 0,
      .cs_change = !!complete,
  };
  ssize_t ret = -1;
  ALOGV("spidev:%s: called [%d]", __func__, len);
  if (len > INT_MAX) {
    ese_set_error(ese, kNxpPn80tErrorTransmitSize);
    ALOGE("Unexpectedly large transfer attempted: %u", len);
    return 0;
  }
  ret = ioctl(handle->spi_fd, SPI_IOC_MESSAGE(1), &tr);
  if (ret < 1) {
    ese_set_error(ese, kNxpPn80tErrorTransmit);
    ALOGE("%s: failed to write to hw (ret=%zd)", __func__, ret);
    return 0;
  }
  return len;
}

uint32_t spidev_receive(struct EseInterface *ese, uint8_t *buf, uint32_t len,
                        int complete) {
  struct NxpState *ns = NXP_PN80T_STATE(ese);
  struct Handle *handle = ns->handle;
  ssize_t ret = -1;
  struct spi_ioc_transfer tr = {
      .tx_buf = 0,
      .rx_buf = (unsigned long)buf,
      .len = (uint32_t)len,
      .delay_usecs = 0,
      .speed_hz = 0,
      .bits_per_word = 0,
      .cs_change = !!complete,
  };
  ALOGV("spidev:%s: called [%d]", __func__, len);
  if (len > INT_MAX) {
    ese_set_error(ese, kNxpPn80tErrorReceiveSize);
    ALOGE("Unexpectedly large receive attempted: %u", len);
    return 0;
  }
  ret = ioctl(handle->spi_fd, SPI_IOC_MESSAGE(1), &tr);
  if (ret < 1) {
    ALOGE("%s: failed to read from hw (ret=%zd)", __func__, ret);
    ese_set_error(ese, kNxpPn80tErrorReceive);
    return 0;
  }
  ALOGV("%s: read bytes: %zd", __func__, len);
  return len;
}

static const struct Pn80tPlatform kPn80tLinuxSpidevPlatform = {
    .initialize = &platform_init,
    .release = &platform_release,
    .toggle_reset = &platform_toggle_reset,
    .toggle_ven = &platform_toggle_ven,
    .toggle_power_req = &platform_toggle_power_req,
    .toggle_bootloader = NULL,
    .wait = &platform_wait,
};

static const struct EseOperations ops = {
    .name = "NXP PN80T/PN81A (PN553)",
    .open = &nxp_pn80t_open,
    .hw_receive = &spidev_receive,
    .hw_transmit = &spidev_transmit,
    .hw_reset = &nxp_pn80t_reset,
    .transceive = &nxp_pn80t_transceive,
    .poll = &nxp_pn80t_poll,
    .close = &nxp_pn80t_close,
    .opts = &kPn80tLinuxSpidevPlatform,
    .errors = kNxpPn80tErrorMessages,
    .errors_count = kNxpPn80tErrorMax,
};
__attribute__((visibility("default")))
ESE_DEFINE_HW_OPS(ESE_HW_NXP_PN80T_SPIDEV, ops);
