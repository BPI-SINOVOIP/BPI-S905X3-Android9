/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_OBSERVER_H
#define CRAS_OBSERVER_H

#include "cras_observer_ops.h"

struct cras_observer_client;

/* Add an observer.
 * Args:
 *    ops - Set callback function pointers in the operations that should be
 *          called for state changes, or NULL otherwise.
 *    context - Context pointer passed to the callbacks.
 * Returns:
 *    Valid pointer to the client reference, or NULL on memory allocation
 *    error.
 */
struct cras_observer_client *cras_observer_add(
			const struct cras_observer_ops *ops,
			void *context);

/* Retrieve the observed state changes.
 * Args:
 *    client - The client to query.
 *    ops - Filled with the current values in the callback table.
 */
void cras_observer_get_ops(const struct cras_observer_client *client,
			   struct cras_observer_ops *ops);

/* Update the observed state changes.
 * Args:
 *    client - The client to modify.
 *    ops - Set callback function pointers in the operations that should be
 *          called for state changes, or NULL otherwise.
 */
void cras_observer_set_ops(struct cras_observer_client *client,
			   const struct cras_observer_ops *ops);

/* Returns non-zero if the given ops are empty. */
int cras_observer_ops_are_empty(const struct cras_observer_ops *ops);

/* Remove this observer client.
 * Args:
 *    client - The client to remove.
 */
void cras_observer_remove(struct cras_observer_client *client);

/* Initialize the observer server. */
int cras_observer_server_init();

/* Destroy the observer server. */
void cras_observer_server_free();

/* Notify observers of output volume change. */
void cras_observer_notify_output_volume(int32_t volume);

/* Notify observers of output mute change. */
void cras_observer_notify_output_mute(int muted, int user_muted,
				      int mute_locked);

/* Notify observers of capture gain change. */
void cras_observer_notify_capture_gain(int32_t gain);

/* Notify observers of capture mute change. */
void cras_observer_notify_capture_mute(int muted, int mute_locked);

/* Notify observers of a nodes list change. */
void cras_observer_notify_nodes(void);

/* Notify observers of active output node change. */
void cras_observer_notify_active_node(enum CRAS_STREAM_DIRECTION dir,
				      cras_node_id_t node_id);

/* Notify observers of output node volume change. */
void cras_observer_notify_output_node_volume(cras_node_id_t node_id,
					     int32_t volume);

/* Notify observers of node left-right swap change. */
void cras_observer_notify_node_left_right_swapped(cras_node_id_t node_id,
						  int swapped);

/* Notify observers of input node gain change. */
void cras_observer_notify_input_node_gain(cras_node_id_t node_id,
					  int32_t gain);

/* Notify observers of suspend state changed. */
void cras_observer_notify_suspend_changed(int suspended);

/* Notify observers of the number of active streams. */
void cras_observer_notify_num_active_streams(enum CRAS_STREAM_DIRECTION dir,
					     uint32_t num_active_streams);

#endif /* CRAS_OBSERVER_H */
