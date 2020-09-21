/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_OBSERVER_OPS_H
#define CRAS_OBSERVER_OPS_H

#include "cras_types.h"

/* Observation of CRAS state.
 * Unless otherwise specified, all notifications only contain the data value
 * reflecting the current state: it is possible that multiple notifications
 * are queued within CRAS before being sent to the client.
 */
struct cras_observer_ops {
	/* System output volume changed. */
	void (*output_volume_changed)(void *context, int32_t volume);
	/* System output mute changed. */
	void (*output_mute_changed)(void *context,
				    int muted, int user_muted, int mute_locked);
	/* System input/capture gain changed. */
	void (*capture_gain_changed)(void *context, int32_t gain);
	/* System input/capture mute changed. */
	void (*capture_mute_changed)(void *context, int muted, int mute_locked);
	/* Device or node topology changed. */
	void (*nodes_changed)(void *context);
	/* Active node changed. A notification is sent for every change.
	 * When there is no active node, node_id is 0. */
	void (*active_node_changed)(void *context,
				    enum CRAS_STREAM_DIRECTION dir,
				    cras_node_id_t node_id);
	/* Output node volume changed. */
	void (*output_node_volume_changed)(void *context,
					   cras_node_id_t node_id,
					   int32_t volume);
	/* Node left/right swapped state change. */
	void (*node_left_right_swapped_changed)(void *context,
						cras_node_id_t node_id,
						int swapped);
	/* Input gain changed. */
	void (*input_node_gain_changed)(void *context,
					cras_node_id_t node_id,
					int32_t gain);
	/* Suspend state changed. */
	void (*suspend_changed)(void *context,
				int suspended);
	/* Number of active streams changed. */
	void (*num_active_streams_changed)(void *context,
					   enum CRAS_STREAM_DIRECTION dir,
					   uint32_t num_active_streams);
};

#endif /* CRAS_OBSERVER_OPS_H */
