/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_EMPTY_IO_H_
#define CRAS_EMPTY_IO_H_

#include "cras_types.h"

struct cras_iodev;

/* Initializes an empty iodev.  Empty iodevs are used when there are no other
 * iodevs available.  They give the attached streams a temporary place to live
 * until a new iodev becomes available.
 * Args:
 *    direciton - input or output.
 * Returns:
 *    A pointer to the newly created iodev if successful, NULL otherwise.
 */
struct cras_iodev *empty_iodev_create(enum CRAS_STREAM_DIRECTION direction);

/* Destroys an empty_iodev created with empty_iodev_create. */
void empty_iodev_destroy(struct cras_iodev *iodev);

#endif /* CRAS_EMPTY_IO_H_ */
