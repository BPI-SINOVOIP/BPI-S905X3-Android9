/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <syslog.h>
#include <sys/select.h>
#include <unistd.h>

#include "cras_client.h"
#include "cras_types.h"
#include "cras_util.h"
#include "cras_version.h"

static void output_volume_changed(void *context, int32_t volume)
{
	printf("output volume: %d/100\n", volume);
}

static void output_mute_changed(void *context, int muted,
				int user_muted, int mute_locked)
{
	printf("output mute: muted: %d, user_muted: %d, mute_locked: %d\n",
	       muted, user_muted, mute_locked);
}

static void capture_gain_changed(void *context, int32_t gain)
{
	printf("capture gain: %d\n", gain);
}

static void capture_mute_changed(void *context, int muted, int mute_locked)
{
	printf("capture mute: muted: %d, mute_locked: %d\n",
	       muted, mute_locked);
}

static void nodes_changed(void *context)
{
	printf("nodes changed\n");
}

static const char *string_for_direction(enum CRAS_STREAM_DIRECTION dir)
{
	switch(dir) {
		case CRAS_STREAM_OUTPUT:
			return "output";
		case CRAS_STREAM_INPUT:
			return "input";
		case CRAS_STREAM_POST_MIX_PRE_DSP:
			return "post_mix_pre_dsp";
		default:
			break;
	}

	return "undefined";
}

size_t node_array_index_of_node_id(struct cras_ionode_info *nodes,
				   size_t num_nodes,
				   cras_node_id_t node_id)
{
	uint32_t dev_index = dev_index_of(node_id);
	uint32_t node_index = node_index_of(node_id);
	size_t i;

	for (i = 0; i < num_nodes; i++) {
		if (nodes[i].iodev_idx == dev_index &&
		    nodes[i].ionode_idx == node_index)
			return i;
	}
	return CRAS_MAX_IONODES;
}

const char *node_name_for_node_id(struct cras_client *client,
				  enum CRAS_STREAM_DIRECTION dir,
				  cras_node_id_t node_id)
{
	struct cras_ionode_info nodes[CRAS_MAX_IONODES];
	struct cras_iodev_info devs[CRAS_MAX_IODEVS];
	size_t num_devs = CRAS_MAX_IODEVS;
	size_t num_nodes = CRAS_MAX_IONODES;
	uint32_t iodev_idx = dev_index_of(node_id);
	size_t node_index;
	char buf[1024];
	int rc;

	if (node_id == 0) {
		return strdup("none");
	} else if (iodev_idx <= 2) {
		return strdup("fallback");
	} else if (dir == CRAS_STREAM_POST_MIX_PRE_DSP) {
		snprintf(buf, sizeof(buf), "%s node: %" PRIu64 "\n",
			 string_for_direction(dir), node_id);
		return strdup(buf);
	} else if (dir == CRAS_STREAM_OUTPUT) {
		rc = cras_client_get_output_devices(client, devs, nodes,
					       &num_devs, &num_nodes);
	} else if (dir == CRAS_STREAM_INPUT) {
		rc = cras_client_get_input_devices(client, devs, nodes,
					      &num_devs, &num_nodes);
	} else {
		return strdup("unknown");
	}

	if (rc != 0) {
		syslog(LOG_ERR, "Couldn't get output devices: %s\n",
		       strerror(-rc));
		snprintf(buf, sizeof(buf), "%u:%u",
			 iodev_idx, node_index_of(node_id));
		return strdup(buf);
	}
	node_index = node_array_index_of_node_id(nodes, num_nodes, node_id);
	if (node_index >= num_nodes)
		snprintf(buf, sizeof(buf),
			 "unknown: %zu >= %zu", node_index, num_nodes);
	else
		snprintf(buf, sizeof(buf), "%u:%u: %s",
			 nodes[node_index].iodev_idx,
			 nodes[node_index].ionode_idx,
			 nodes[node_index].name);
	return strdup(buf);
}

static void active_node_changed(void *context,
				enum CRAS_STREAM_DIRECTION dir,
				cras_node_id_t node_id)
{
	struct cras_client *client = (struct cras_client *)context;
	const char *node_name = node_name_for_node_id(client, dir, node_id);
	printf("active node (%s): %s\n", string_for_direction(dir), node_name);
	free((void *)node_name);
}

static void output_node_volume_changed(void *context,
				       cras_node_id_t node_id, int32_t volume)
{
	struct cras_client *client = (struct cras_client *)context;
	const char *node_name =
		node_name_for_node_id(client, CRAS_STREAM_OUTPUT, node_id);
	printf("output node '%s' volume: %d\n", node_name, volume);
	free((void *)node_name);
}

static void node_left_right_swapped_changed(void *context,
					    cras_node_id_t node_id, int swapped)
{
	struct cras_client *client = (struct cras_client *)context;
	const char *node_name =
		node_name_for_node_id(client, CRAS_STREAM_OUTPUT, node_id);
	printf("output node '%s' left-right swapped: %d\n", node_name, swapped);
	free((void *)node_name);
}

static void input_node_gain_changed(void *context,
				    cras_node_id_t node_id, int32_t gain)
{
	struct cras_client *client = (struct cras_client *)context;
	const char *node_name =
		node_name_for_node_id(client, CRAS_STREAM_INPUT, node_id);
	printf("input node '%s' gain: %d\n", node_name, gain);
	free((void *)node_name);
}

static void num_active_streams_changed(void *context,
				       enum CRAS_STREAM_DIRECTION dir,
				       uint32_t num_active_streams)
{
	printf("num active %s streams: %u\n",
	       string_for_direction(dir), num_active_streams);
}

static void server_connection_callback(struct cras_client *client,
				       cras_connection_status_t status,
				       void *user_arg)
{
	const char *status_str = "undefined";
	switch (status) {
		case CRAS_CONN_STATUS_FAILED:
			status_str = "error";
			break;
		case CRAS_CONN_STATUS_DISCONNECTED:
			status_str = "disconnected";
			break;
		case CRAS_CONN_STATUS_CONNECTED:
			status_str = "connected";
			break;
	}
	printf("server %s\n", status_str);
}

static void print_usage(const char *command) {
	fprintf(stderr,
		"%s [options]\n"
		"  Where [options] are:\n"
		"    --sync|-s  - Use the synchronous connection functions.\n"
		"    --log-level|-l <n>  - Set the syslog level (7 == "
			"LOG_DEBUG).\n",
		command);
}

int main(int argc, char **argv)
{
	struct cras_client *client;
	int rc;
	int option_character;
	bool synchronous = false;
	int log_level = LOG_WARNING;
	static struct option long_options[] = {
		{"sync", no_argument, NULL, 's'},
		{"log-level", required_argument, NULL, 'l'},
		{NULL, 0, NULL, 0},
	};

	while(true) {
		int option_index = 0;

		option_character = getopt_long(argc, argv, "sl:",
					       long_options, &option_index);
		if (option_character == -1)
			break;
		switch (option_character) {
		case 's':
			synchronous = !synchronous;
			break;
		case 'l':
			log_level = atoi(optarg);
			if (log_level < 0)
				log_level = LOG_WARNING;
			else if (log_level > LOG_DEBUG)
				log_level = LOG_DEBUG;
			break;
		default:
			print_usage(argv[0]);
			return 1;
		}
	}

	if (optind < argc) {
		fprintf(stderr, "%s: Extra arguments.\n", argv[0]);
		print_usage(argv[0]);
		return 1;
	}

	openlog("cras_monitor", LOG_PERROR, LOG_USER);
	setlogmask(LOG_UPTO(log_level));

	rc = cras_client_create(&client);
	if (rc < 0) {
		syslog(LOG_ERR, "Couldn't create client.");
		return rc;
	}

	cras_client_set_connection_status_cb(
			client, server_connection_callback, NULL);

	if (synchronous) {
		rc = cras_client_connect(client);
		if (rc != 0) {
			syslog(LOG_ERR, "Could not connect to server.");
			return -rc;
		}
	}

	cras_client_set_output_volume_changed_callback(
			client, output_volume_changed);
	cras_client_set_output_mute_changed_callback(
			client, output_mute_changed);
	cras_client_set_capture_gain_changed_callback(
			client, capture_gain_changed);
	cras_client_set_capture_mute_changed_callback(
			client, capture_mute_changed);
	cras_client_set_nodes_changed_callback(
			client, nodes_changed);
	cras_client_set_active_node_changed_callback(
			client, active_node_changed);
	cras_client_set_output_node_volume_changed_callback(
			client, output_node_volume_changed);
	cras_client_set_node_left_right_swapped_changed_callback(
			client, node_left_right_swapped_changed);
	cras_client_set_input_node_gain_changed_callback(
			client, input_node_gain_changed);
	cras_client_set_num_active_streams_changed_callback(
			client, num_active_streams_changed);
	cras_client_set_state_change_callback_context(client, client);

	rc = cras_client_run_thread(client);
	if (rc != 0) {
		syslog(LOG_ERR, "Could not start thread.");
		return -rc;
	}

	if (!synchronous) {
		rc = cras_client_connect_async(client);
		if (rc) {
			syslog(LOG_ERR, "Couldn't connect to server.\n");
			goto destroy_exit;
		}
	}

	while(1) {
		int rc;
		char c;
		rc = read(STDIN_FILENO, &c, 1);
		if (rc < 0 || c == 'q')
			return 0;
	}

destroy_exit:
	cras_client_destroy(client);
	return 0;
}
