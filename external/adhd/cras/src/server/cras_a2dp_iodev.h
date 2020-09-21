/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_A2DP_IODEV_H_
#define CRAS_A2DP_IODEV_H_

#include "cras_bt_transport.h"

struct cras_iodev;

/*
 * Creates an a2dp iodev from transport object.
 * Args:
 *    transport - The transport to create a2dp iodev for
 */
struct cras_iodev *a2dp_iodev_create(
		struct cras_bt_transport *transport);

/*
 * Destroys a2dp iodev.
 */
void a2dp_iodev_destroy(struct cras_iodev *iodev);

#endif /* CRS_A2DP_IODEV_H_ */
