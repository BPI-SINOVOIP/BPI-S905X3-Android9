/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef TEST_IODEV_H_
#define TEST_IODEV_H_

#include "cras_types.h"

struct cras_iodev;

/* Initializes an test iodev.  The Test iodev is used to simulate hardware
 * iodevs when they aren't available.
 * Args:
 *    direciton - input or output.
 *    type - The test type.
 * Returns:
 *    A pointer to the newly created iodev if successful, NULL otherwise.
 */
struct cras_iodev *test_iodev_create(enum CRAS_STREAM_DIRECTION direction,
				     enum TEST_IODEV_TYPE type);

/* Destroys an test_iodev created with test_iodev_create. */
void test_iodev_destroy(struct cras_iodev *iodev);

/* Handle a test commdn to the given iodev. */
void test_iodev_command(struct cras_iodev *iodev,
			enum CRAS_TEST_IODEV_CMD command,
			unsigned int data_len,
			const uint8_t *data);

#endif /* TEST_IODEV_H_ */
