/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_IODEV_INFO_H_
#define CRAS_IODEV_INFO_H_

#include <stddef.h>
#include <sys/time.h>

#define CRAS_IODEV_NAME_BUFFER_SIZE 64
#define CRAS_NODE_TYPE_BUFFER_SIZE 32
#define CRAS_NODE_MIC_POS_BUFFER_SIZE 128
#define CRAS_NODE_NAME_BUFFER_SIZE 64
#define CRAS_NODE_HOTWORD_MODEL_BUFFER_SIZE 16

/* Identifying information about an IO device.
 *    idx - iodev index.
 *    name - Name displayed to the user.
 *    stable_id - ID that does not change due to device plug/unplug or reboot.
 *    stable_id_new - New stable_id, it will be deprecated and be put on
 *        stable_id.
 */
struct __attribute__ ((__packed__)) cras_iodev_info {
	uint32_t idx;
	char name[CRAS_IODEV_NAME_BUFFER_SIZE];
	uint32_t stable_id;
	uint32_t stable_id_new;
};

/* Identifying information about an ionode on an iodev.
 *    iodev_idx - Index of the device this node belongs.
 *    ionode_idx - Index of this node on the device.
 *    plugged - Set true if this node is known to be plugged in.
 *    plugged_time - If plugged is true, this is the time it was attached.
 *    active - If this is the node currently being used.
 *    volume - per-node volume (0-100)
 *    capture_gain - per-node capture gain/attenuation (in 100*dBFS)
 *    left_right_swapped - Set true if left and right channels are swapped.
 *    stable_id - ID that does not change due to device plug/unplug or reboot.
 *    stable_id_new - New stable_id, it will be deprecated and be put on
 *        stable_id.
 *    mic_positions - Positions of the mic array.
 *    type - Type displayed to the user.
 *    name - Name displayed to the user.
 *    active_hotword_model - name of the currently selected hotword model.
 */
struct __attribute__ ((__packed__)) cras_ionode_info {
	uint32_t iodev_idx;
	uint32_t ionode_idx;
	int32_t plugged;
	int32_t active;
	struct { int64_t tv_sec; int64_t tv_usec; } plugged_time;
	uint32_t volume;
	int32_t capture_gain;
	int32_t left_right_swapped;
	uint32_t type_enum;
	uint32_t stable_id;
	uint32_t stable_id_new;
	char mic_positions[CRAS_NODE_MIC_POS_BUFFER_SIZE];
	char type[CRAS_NODE_TYPE_BUFFER_SIZE];
	char name[CRAS_NODE_NAME_BUFFER_SIZE];
	char active_hotword_model[CRAS_NODE_HOTWORD_MODEL_BUFFER_SIZE];
};

/* This is used in the cras_client_set_node_attr API.
 *    IONODE_ATTR_PLUGGED - set the node as plugged/unplugged.
 *    IONODE_ATTR_VOLUME - set the node's output volume.
 *    IONODE_ATTR_CAPTURE_GAIN - set the node's capture gain.
 *    IONODE_ATTR_SWAP_LEFT_RIGHT - Swap the node's left and right channel.
 */
enum ionode_attr {
	IONODE_ATTR_PLUGGED,
	IONODE_ATTR_VOLUME,
	IONODE_ATTR_CAPTURE_GAIN,
	IONODE_ATTR_SWAP_LEFT_RIGHT
};

#endif /* CRAS_IODEV_INFO_H_ */
