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
#ifndef NOS_DEVICE_H
#define NOS_DEVICE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Max data size for read/write.
 * TODO: Yes, it's a magic number. */
#define MAX_DEVICE_TRANSFER 2044

struct nos_device_ops {
  /**
   * Read a datagram from the device.
   *
   * Return 0 on success and a negative value on failure.
   */
  int (*read)(void* ctx, uint32_t command, uint8_t *buf, uint32_t len);

  /**
   * Write a datagram to the device.
   *
   * Return 0 on success and a negative value on failure.
   */
  int (*write)(void *ctx, uint32_t command, const uint8_t *buf, uint32_t len);

  /**
   * Block until an event has happened on the device, or until timed out.
   *
   * Values for msecs
   *  <0 wait forever
   *   0 return immediately (why?)
   *  >0 timeout after this many milliseconds
   *
   * Returns:
   *  <0 on error
   *   0 timed out
   *  >0 interrupt occurred
   */
  int (*wait_for_interrupt)(void *ctx, int msecs);

  /**
   * Reset the device.
   *
   * Return 0 on success and a negative value on failure.
   */
  int (*reset)(void *ctx);

  /**
   * Close the connection to the device.
   *
   * The device must not be used after closing.
   */
  void (*close)(void *ctx);
};

struct nos_device {
  void *ctx;
  struct nos_device_ops ops;
};

/*
 * Open a connection to a Nugget device.
 *
 * The name parameter identifies which Nugget device to connect to. Passing
 * NULL connects to the default device.
 *
 * This function is implemented by the host specific variants of this library.
 *
 * Returns 0 on success or negative on failure.
 */
int nos_device_open(const char *name, struct nos_device *device);

#ifdef __cplusplus
}
#endif

#endif /* NOS_DEVICE_H */
