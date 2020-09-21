/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cras_observer.h"

#include "cras_alert.h"
#include "cras_iodev_list.h"
#include "utlist.h"

struct cras_observer_client {
	struct cras_observer_ops ops;
	void *context;
	struct cras_observer_client *next, *prev;
};

struct cras_observer_alerts {
	struct cras_alert *output_volume;
	struct cras_alert *output_mute;
	struct cras_alert *capture_gain;
	struct cras_alert *capture_mute;
	struct cras_alert *nodes;
	struct cras_alert *active_node;
	struct cras_alert *output_node_volume;
	struct cras_alert *node_left_right_swapped;
	struct cras_alert *input_node_gain;
	struct cras_alert *suspend_changed;
	/* If all events for active streams went through a single alert then
         * we might miss some because the alert code does not send every
         * alert message. To ensure that the event sent contains the correct
         * number of active streams per direction, make the alerts
         * per-direciton. */
	struct cras_alert *num_active_streams[CRAS_NUM_DIRECTIONS];
};

struct cras_observer_server {
	struct cras_observer_alerts alerts;
	struct cras_observer_client *clients;
};

struct cras_observer_alert_data_volume {
	int32_t volume;
};

struct cras_observer_alert_data_mute {
	int muted;
	int user_muted;
	int mute_locked;
};

struct cras_observer_alert_data_active_node {
	enum CRAS_STREAM_DIRECTION direction;
	cras_node_id_t node_id;
};

struct cras_observer_alert_data_node_volume {
	cras_node_id_t node_id;
	int32_t volume;
};

struct cras_observer_alert_data_node_lr_swapped {
	cras_node_id_t node_id;
	int swapped;
};

struct cras_observer_alert_data_suspend {
	int suspended;
};

struct cras_observer_alert_data_streams {
	enum CRAS_STREAM_DIRECTION direction;
	uint32_t num_active_streams;
};

/* Global observer instance. */
static struct cras_observer_server *g_observer;

/* Empty observer ops. */
static struct cras_observer_ops g_empty_ops;

/*
 * Alert handlers for delayed callbacks.
 */

static void output_volume_alert(void *arg, void *data)
{
	struct cras_observer_client *client;
	struct cras_observer_alert_data_volume *volume_data =
		(struct cras_observer_alert_data_volume *)data;

	DL_FOREACH(g_observer->clients, client) {
		if (client->ops.output_volume_changed)
			client->ops.output_volume_changed(
					client->context,
					volume_data->volume);
	}
}

static void output_mute_alert(void *arg, void *data)
{
	struct cras_observer_client *client;
	struct cras_observer_alert_data_mute *mute_data =
		(struct cras_observer_alert_data_mute *)data;

	DL_FOREACH(g_observer->clients, client) {
		if (client->ops.output_mute_changed)
			client->ops.output_mute_changed(
					client->context,
					mute_data->muted,
					mute_data->user_muted,
					mute_data->mute_locked);
	}
}

static void capture_gain_alert(void *arg, void *data)
{
	struct cras_observer_client *client;
	struct cras_observer_alert_data_volume *volume_data =
		(struct cras_observer_alert_data_volume *)data;

	DL_FOREACH(g_observer->clients, client) {
		if (client->ops.capture_gain_changed)
			client->ops.capture_gain_changed(
					client->context,
					volume_data->volume);
	}
}

static void capture_mute_alert(void *arg, void *data)
{
	struct cras_observer_client *client;
	struct cras_observer_alert_data_mute *mute_data =
		(struct cras_observer_alert_data_mute *)data;

	DL_FOREACH(g_observer->clients, client) {
		if (client->ops.capture_mute_changed)
			client->ops.capture_mute_changed(
					client->context,
					mute_data->muted,
					mute_data->mute_locked);
	}
}

static void nodes_prepare(struct cras_alert *alert)
{
	cras_iodev_list_update_device_list();
}

static void nodes_alert(void *arg, void *data)
{
	struct cras_observer_client *client;

	DL_FOREACH(g_observer->clients, client) {
		if (client->ops.nodes_changed)
			client->ops.nodes_changed(client->context);
	}
}

static void active_node_alert(void *arg, void *data)
{
	struct cras_observer_client *client;
	struct cras_observer_alert_data_active_node *node_data =
		(struct cras_observer_alert_data_active_node *)data;

	DL_FOREACH(g_observer->clients, client) {
		if (client->ops.active_node_changed)
			client->ops.active_node_changed(
					client->context,
					node_data->direction,
					node_data->node_id);
	}
}

static void output_node_volume_alert(void *arg, void *data)
{
	struct cras_observer_client *client;
	struct cras_observer_alert_data_node_volume *node_data =
		(struct cras_observer_alert_data_node_volume *)data;

	DL_FOREACH(g_observer->clients, client) {
		if (client->ops.output_node_volume_changed)
			client->ops.output_node_volume_changed(
					client->context,
					node_data->node_id,
					node_data->volume);
	}
}

static void node_left_right_swapped_alert(void *arg, void *data)
{
	struct cras_observer_client *client;
	struct cras_observer_alert_data_node_lr_swapped *node_data =
		(struct cras_observer_alert_data_node_lr_swapped *)data;

	DL_FOREACH(g_observer->clients, client) {
		if (client->ops.node_left_right_swapped_changed)
			client->ops.node_left_right_swapped_changed(
					client->context,
					node_data->node_id,
					node_data->swapped);
	}
}

static void input_node_gain_alert(void *arg, void *data)
{
	struct cras_observer_client *client;
	struct cras_observer_alert_data_node_volume *node_data =
		(struct cras_observer_alert_data_node_volume *)data;

	DL_FOREACH(g_observer->clients, client) {
		if (client->ops.input_node_gain_changed)
			client->ops.input_node_gain_changed(
					client->context,
					node_data->node_id,
					node_data->volume);
	}
}

static void suspend_changed_alert(void *arg, void *data)
{
	struct cras_observer_client *client;
	struct cras_observer_alert_data_suspend *suspend_data =
		(struct cras_observer_alert_data_suspend *)data;

	DL_FOREACH(g_observer->clients, client) {
		if (client->ops.suspend_changed)
			client->ops.suspend_changed(
					client->context,
					suspend_data->suspended);
	}
}

static void num_active_streams_alert(void *arg, void *data)
{
	struct cras_observer_client *client;
	struct cras_observer_alert_data_streams *streams_data =
		(struct cras_observer_alert_data_streams *)data;

	DL_FOREACH(g_observer->clients, client) {
		if (client->ops.num_active_streams_changed)
			client->ops.num_active_streams_changed(
					client->context,
					streams_data->direction,
					streams_data->num_active_streams);
	}
}

static int cras_observer_server_set_alert(struct cras_alert **alert,
					  cras_alert_cb cb,
					  cras_alert_prepare prepare,
					  unsigned int flags)
{
	*alert = cras_alert_create(prepare, flags);
	if (!*alert)
		return -ENOMEM;
	return cras_alert_add_callback(*alert, cb, NULL);
}

#define CRAS_OBSERVER_SET_ALERT(alert,prepare,flags) \
	do { \
		rc = cras_observer_server_set_alert( \
			&g_observer->alerts.alert, alert##_alert, \
			prepare, flags); \
		if (rc) \
			goto error; \
	} while(0)

#define CRAS_OBSERVER_SET_ALERT_WITH_DIRECTION(alert,direction) \
	do { \
		rc = cras_observer_server_set_alert( \
			&g_observer->alerts.alert[direction], \
			alert##_alert, NULL, 0); \
		if (rc) \
			goto error; \
	} while(0)

/*
 * Public interface
 */

int cras_observer_server_init()
{
	int rc;

	memset(&g_empty_ops, 0, sizeof(g_empty_ops));
	g_observer = (struct cras_observer_server *)
			calloc(1, sizeof(struct cras_observer_server));
	if (!g_observer)
		return -ENOMEM;

	CRAS_OBSERVER_SET_ALERT(output_volume, NULL, 0);
	CRAS_OBSERVER_SET_ALERT(output_mute, NULL, 0);
	CRAS_OBSERVER_SET_ALERT(capture_gain, NULL, 0);
	CRAS_OBSERVER_SET_ALERT(capture_mute, NULL, 0);
	CRAS_OBSERVER_SET_ALERT(nodes, nodes_prepare, 0);
	CRAS_OBSERVER_SET_ALERT(active_node, nodes_prepare,
				CRAS_ALERT_FLAG_KEEP_ALL_DATA);
	CRAS_OBSERVER_SET_ALERT(output_node_volume, NULL, 0);
	CRAS_OBSERVER_SET_ALERT(node_left_right_swapped, NULL, 0);
	CRAS_OBSERVER_SET_ALERT(input_node_gain, NULL, 0);
	CRAS_OBSERVER_SET_ALERT(suspend_changed, NULL, 0);

	CRAS_OBSERVER_SET_ALERT_WITH_DIRECTION(
		num_active_streams, CRAS_STREAM_OUTPUT);
	CRAS_OBSERVER_SET_ALERT_WITH_DIRECTION(
		num_active_streams, CRAS_STREAM_INPUT);
	CRAS_OBSERVER_SET_ALERT_WITH_DIRECTION(
		num_active_streams, CRAS_STREAM_POST_MIX_PRE_DSP);
	return 0;

error:
	cras_observer_server_free();
	return rc;
}

void cras_observer_server_free()
{
	if (!g_observer)
		return;
	cras_alert_destroy(g_observer->alerts.output_volume);
	cras_alert_destroy(g_observer->alerts.output_mute);
	cras_alert_destroy(g_observer->alerts.capture_gain);
	cras_alert_destroy(g_observer->alerts.capture_mute);
	cras_alert_destroy(g_observer->alerts.nodes);
	cras_alert_destroy(g_observer->alerts.active_node);
	cras_alert_destroy(g_observer->alerts.output_node_volume);
	cras_alert_destroy(g_observer->alerts.node_left_right_swapped);
	cras_alert_destroy(g_observer->alerts.input_node_gain);
	cras_alert_destroy(g_observer->alerts.suspend_changed);
	cras_alert_destroy(g_observer->alerts.num_active_streams[
							CRAS_STREAM_OUTPUT]);
	cras_alert_destroy(g_observer->alerts.num_active_streams[
							CRAS_STREAM_INPUT]);
	cras_alert_destroy(g_observer->alerts.num_active_streams[
						CRAS_STREAM_POST_MIX_PRE_DSP]);
	free(g_observer);
	g_observer = NULL;
}

int cras_observer_ops_are_empty(const struct cras_observer_ops *ops)
{
	return memcmp(ops, &g_empty_ops, sizeof(*ops)) == 0;
}

void cras_observer_get_ops(const struct cras_observer_client *client,
			   struct cras_observer_ops *ops)
{
	if (!ops)
		return;
	if (!client)
		memset(ops, 0, sizeof(*ops));
	else
		memcpy(ops, &client->ops, sizeof(*ops));
}

void cras_observer_set_ops(struct cras_observer_client *client,
			   const struct cras_observer_ops *ops)
{
	if (!client)
		return;
	if (!ops)
		memset(&client->ops, 0, sizeof(client->ops));
	else
		memcpy(&client->ops, ops, sizeof(client->ops));
}

struct cras_observer_client *cras_observer_add(
			const struct cras_observer_ops *ops,
			void *context)
{
	struct cras_observer_client *client;

	client = (struct cras_observer_client *)calloc(1, sizeof(*client));
	if (!client)
		return NULL;
	client->context = context;
	DL_APPEND(g_observer->clients, client);
	cras_observer_set_ops(client, ops);
	return client;
}

void cras_observer_remove(struct cras_observer_client *client)
{
	if (!client)
		return;
	DL_DELETE(g_observer->clients, client);
	free(client);
}

/*
 * Public interface for notifiers.
 */

void cras_observer_notify_output_volume(int32_t volume)
{
	struct cras_observer_alert_data_volume data;

	data.volume = volume;
	cras_alert_pending_data(g_observer->alerts.output_volume,
				&data, sizeof(data));
}

void cras_observer_notify_output_mute(int muted, int user_muted,
				      int mute_locked)
{
	struct cras_observer_alert_data_mute data;

	data.muted = muted;
	data.user_muted = user_muted;
	data.mute_locked = mute_locked;
	cras_alert_pending_data(g_observer->alerts.output_mute,
				&data, sizeof(data));
}

void cras_observer_notify_capture_gain(int32_t gain)
{
	struct cras_observer_alert_data_volume data;

	data.volume = gain;
	cras_alert_pending_data(g_observer->alerts.capture_gain,
				&data, sizeof(data));
}

void cras_observer_notify_capture_mute(int muted, int mute_locked)
{
	struct cras_observer_alert_data_mute data;

	data.muted = muted;
	data.user_muted = 0;
	data.mute_locked = mute_locked;
	cras_alert_pending_data(g_observer->alerts.capture_mute,
				&data, sizeof(data));
}

void cras_observer_notify_nodes(void)
{
	cras_alert_pending(g_observer->alerts.nodes);
}

void cras_observer_notify_active_node(enum CRAS_STREAM_DIRECTION dir,
				      cras_node_id_t node_id)
{
	struct cras_observer_alert_data_active_node data;

	data.direction = dir;
	data.node_id = node_id;
	cras_alert_pending_data(g_observer->alerts.active_node,
				&data, sizeof(data));
}

void cras_observer_notify_output_node_volume(cras_node_id_t node_id,
					     int32_t volume)
{
	struct cras_observer_alert_data_node_volume data;

	data.node_id = node_id;
	data.volume = volume;
	cras_alert_pending_data(g_observer->alerts.output_node_volume,
				&data, sizeof(data));
}

void cras_observer_notify_node_left_right_swapped(cras_node_id_t node_id,
						  int swapped)
{
	struct cras_observer_alert_data_node_lr_swapped data;

	data.node_id = node_id;
	data.swapped = swapped;
	cras_alert_pending_data(g_observer->alerts.node_left_right_swapped,
				&data, sizeof(data));
}

void cras_observer_notify_input_node_gain(cras_node_id_t node_id,
					  int32_t gain)
{
	struct cras_observer_alert_data_node_volume data;

	data.node_id = node_id;
	data.volume = gain;
	cras_alert_pending_data(g_observer->alerts.input_node_gain,
				&data, sizeof(data));
}

void cras_observer_notify_suspend_changed(int suspended)
{
	struct cras_observer_alert_data_suspend data;

	data.suspended = suspended;
	cras_alert_pending_data(g_observer->alerts.suspend_changed,
				&data, sizeof(data));
}

void cras_observer_notify_num_active_streams(enum CRAS_STREAM_DIRECTION dir,
					     uint32_t num_active_streams)
{
	struct cras_observer_alert_data_streams data;
	struct cras_alert *alert;

	data.direction = dir;
	data.num_active_streams = num_active_streams;
	alert = g_observer->alerts.num_active_streams[dir];
	if (!alert)
		return;

	cras_alert_pending_data(alert, &data, sizeof(data));
}
