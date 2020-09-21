/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef _CRAS_GPIO_JACK_H
#define _CRAS_GPIO_JACK_H

#include "cras_types.h"

struct mixer_name;

int gpio_switch_open(const char *pathname);
int gpio_switch_read(int fd, void *buf, size_t n_bytes);

int gpio_switch_eviocgbit(int fd, void *buf, size_t n_bytes);
int gpio_switch_eviocgsw(int fd, void *bits, size_t n_bytes);

/* sys_input_get_device_name:
 *
 *   Returns the heap-allocated device name of a /dev/input/event*
 *   pathname.  Caller is responsible for releasing.
 */
char *sys_input_get_device_name(const char *path);

/* List for each callback function.
 *
 * Args:
 *    dev_path - Full path to the GPIO device.
 *    dev_name - The name of the GPIO device.
 *    arg - The argument passed to gpio_switch_list_for_each.
 *
 * Returns:
 *    0 to continue searching, non-zero otherwise.
 */
typedef int (*gpio_switch_list_callback)(const char *dev_path,
				         const char *dev_name,
				         void *arg);

/* Execute the given callback on each GPIO device.
 *
 * Args:
 *    callback - The callback to execute.
 *    arg - An argument to pass to the callback.
 */
void gpio_switch_list_for_each(gpio_switch_list_callback callback, void *arg);

#endif
