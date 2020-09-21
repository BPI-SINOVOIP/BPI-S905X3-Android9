/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_BT_IODEV_H_
#define CRAS_BT_IODEV_H_

#include "cras_bt_device.h"

struct cras_iodev;

/* Creates a bluetooth iodev. */
struct cras_iodev *cras_bt_io_create(struct cras_bt_device *device,
				     struct cras_iodev *dev,
				     enum cras_bt_device_profile profile);

/* Destroys a bluetooth iodev. */
void cras_bt_io_destroy(struct cras_iodev *bt_iodev);

/* Looks up for the node of given profile, returns NULL if doesn't exist. */
struct cras_ionode *cras_bt_io_get_profile(
		struct cras_iodev *bt_iodev,
		enum cras_bt_device_profile profile);

/* Appends a profile specific iodev to bt_iodev. */
int cras_bt_io_append(struct cras_iodev *bt_iodev,
		      struct cras_iodev *dev,
		      enum cras_bt_device_profile profile);

/* Checks if the active node of bt_io matches a profile. */
int cras_bt_io_on_profile(struct cras_iodev *bt_iodev,
			  enum cras_bt_device_profile profile);

/* Updates the buffer size to the profile specific iodev. */
int cras_bt_io_update_buffer_size(struct cras_iodev *bt_iodev);

/* Dry-run the profile device removal from bt_iodev.
 * Returns:
 *    0 if the bt_iodev will be empty and should to be destroied
 *    after the removal, othersie the value of the next preffered
 *    profile to use.
 */
unsigned int cras_bt_io_try_remove(struct cras_iodev *bt_iodev,
				   struct cras_iodev *dev);

/* Removes a profile specific iodev from bt_iodev.
 * Returns:
 *    0 if dev is removed and bt_iodev successfully updated to
 *    the new profile, otherwise return negative error code. */
int cras_bt_io_remove(struct cras_iodev *bt_iodev, struct cras_iodev *dev);

#endif /* CRAS_BT_IODEV_H_ */
