/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * IO list manages the list of inputs and outputs available.
 */
#ifndef CRAS_IODEV_LIST_H_
#define CRAS_IODEV_LIST_H_

#include <stdint.h>

#include "cras_types.h"

struct cras_iodev;
struct cras_iodev_info;
struct cras_ionode;
struct cras_rclient;
struct cras_rstream;
struct cras_audio_format;
struct stream_list;

/* Device enabled/disabled callback.
 * enabled=1 when a device is enabled, enabled=0 when a device is disabled.
 */
typedef void (*device_enabled_callback_t)(struct cras_iodev *dev, int enabled,
					  void *cb_data);

/* Initialize the list of iodevs. */
void cras_iodev_list_init();

/* Clean up any resources used by iodev. */
void cras_iodev_list_deinit();

/* Adds an output to the output list.
 * Args:
 *    output - the output to add.
 * Returns:
 *    0 on success, negative error on failure.
 */
int cras_iodev_list_add_output(struct cras_iodev *output);

/* Adds an input to the input list.
 * Args:
 *    input - the input to add.
 * Returns:
 *    0 on success, negative error on failure.
 */
int cras_iodev_list_add_input(struct cras_iodev *input);

/* Removes an output from the output list.
 * Args:
 *    output - the output to remove.
 * Returns:
 *    0 on success, negative error on failure.
 */
int cras_iodev_list_rm_output(struct cras_iodev *output);

/* Removes an input from the input list.
 * Args:
 *    output - the input to remove.
 * Returns:
 *    0 on success, negative error on failure.
 */
int cras_iodev_list_rm_input(struct cras_iodev *input);

/* Gets a list of outputs. Callee must free the list when finished.  If list_out
 * is NULL, this function can be used to return the number of outputs.
 * Args:
 *    list_out - This will be set to the malloc'd area containing the list of
 *        devices.  Ignored if NULL.
 * Returns:
 *    The number of devices on the list.
 */
int cras_iodev_list_get_outputs(struct cras_iodev_info **list_out);

/* Gets a list of inputs. Callee must free the list when finished.  If list_out
 * is NULL, this function can be used to return the number of inputs.
 * Args:
 *    list_out - This will be set to the malloc'd area containing the list of
 *        devices.  Ignored if NULL.
 * Returns:
 *    The number of devices on the list.
 */
int cras_iodev_list_get_inputs(struct cras_iodev_info **list_out);

/* Returns the first enabled device.
 * Args:
 *    direction - Playback or capture.
 * Returns:
 *    Pointer to the first enabled device of direction.
 */
struct cras_iodev *cras_iodev_list_get_first_enabled_iodev(
	enum CRAS_STREAM_DIRECTION direction);

/* Returns the active node id.
 * Args:
 *    direction - Playback or capture.
 * Returns:
 *    The id of the active node.
 */
cras_node_id_t cras_iodev_list_get_active_node_id(
	enum CRAS_STREAM_DIRECTION direction);

/* Stores the following data to the shared memory server state region:
 * (1) device list
 * (2) node list
 * (3) selected nodes
 */
void cras_iodev_list_update_device_list();

/* Stores the node list in the shared memory server state region. */
void cras_iodev_list_update_node_list();

/* Gets the supported hotword models of an ionode. Caller should free
 * the returned string after use. */
char *cras_iodev_list_get_hotword_models(cras_node_id_t node_id);

/* Sets the desired hotword model to an ionode. */
int cras_iodev_list_set_hotword_model(cras_node_id_t id,
				      const char *model_name);

/* Notify that nodes are added/removed. */
void cras_iodev_list_notify_nodes_changed();

/* Notify that active node is changed for the given direction.
 * Args:
 *    direction - Direction of the node.
 */
void cras_iodev_list_notify_active_node_changed(
		enum CRAS_STREAM_DIRECTION direction);

/* Sets an attribute of an ionode on a device.
 * Args:
 *    id - the id of the ionode.
 *    node_index - Index of the ionode on the device.
 *    attr - the attribute we want to change.
 *    value - the value we want to set.
 */
int cras_iodev_list_set_node_attr(cras_node_id_t id,
				  enum ionode_attr attr, int value);

/* Select a node as the preferred node.
 * Args:
 *    direction - Playback or capture.
 *    node_id - the id of the ionode to be selected. As a special case, if
 *        node_id is 0, don't select any node in this direction.
 */
void cras_iodev_list_select_node(enum CRAS_STREAM_DIRECTION direction,
				 cras_node_id_t node_id);

/* Checks if an iodev is enabled. */
int cras_iodev_list_dev_is_enabled(const struct cras_iodev *dev);

/* Enables an iodev. If the fallback device was already enabled, this
 * call will disable it. */
void cras_iodev_list_enable_dev(struct cras_iodev *dev);

/* Disables an iodev. If this is the last device to disable, the
 * fallback devices will be enabled accordingly. */
void cras_iodev_list_disable_dev(struct cras_iodev *dev);

/* Adds a node to the active devices list.
 * Args:
 *    direction - Playback or capture.
 *    node_id - The id of the ionode to be added.
 */
void cras_iodev_list_add_active_node(enum CRAS_STREAM_DIRECTION direction,
				     cras_node_id_t node_id);

/* Removes a node from the active devices list.
 * Args:
 *    direction - Playback or capture.
 *    node_id - The id of the ionode to be removed.
 */
void cras_iodev_list_rm_active_node(enum CRAS_STREAM_DIRECTION direction,
				    cras_node_id_t node_id);

/* Returns 1 if the node is selected, 0 otherwise. */
int cras_iodev_list_node_selected(struct cras_ionode *node);

/* Notify the current volume of the given node. */
void cras_iodev_list_notify_node_volume(struct cras_ionode *node);

/* Notify the current capture gain of the given node. */
void cras_iodev_list_notify_node_capture_gain(struct cras_ionode *node);

/* Notify the current left right channel swapping state of the given node. */
void cras_iodev_list_notify_node_left_right_swapped(struct cras_ionode *node);

/* Handles the adding and removing of test iodevs. */
void cras_iodev_list_add_test_dev(enum TEST_IODEV_TYPE type);

/* Handles sending a command to a test iodev. */
void cras_iodev_list_test_dev_command(unsigned int iodev_idx,
				      enum CRAS_TEST_IODEV_CMD command,
				      unsigned int data_len,
				      const uint8_t *data);

/* Gets the audio thread used by the devices. */
struct audio_thread *cras_iodev_list_get_audio_thread();

/* Gets the list of all active audio streams attached to devices. */
struct stream_list *cras_iodev_list_get_stream_list();

/* Sets the function to call when a device is enabled or disabled. */
int cras_iodev_list_set_device_enabled_callback(
		device_enabled_callback_t device_enabled_cb, void *cb_data);

/* For unit test only. */
void cras_iodev_list_reset();

#endif /* CRAS_IODEV_LIST_H_ */
