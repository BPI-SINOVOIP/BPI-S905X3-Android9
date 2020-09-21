/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <dbus/dbus.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include "audio_thread.h"
#include "cras_dbus.h"
#include "cras_dbus_control.h"
#include "cras_dbus_util.h"
#include "cras_iodev_list.h"
#include "cras_observer.h"
#include "cras_system_state.h"
#include "cras_util.h"
#include "utlist.h"

#define CRAS_CONTROL_INTERFACE "org.chromium.cras.Control"
#define CRAS_ROOT_OBJECT_PATH "/org/chromium/cras"
#define CONTROL_INTROSPECT_XML                                          \
    DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE                           \
    "<node>\n"                                                          \
    "  <interface name=\""CRAS_CONTROL_INTERFACE"\">\n"                 \
    "    <method name=\"SetOutputVolume\">\n"                           \
    "      <arg name=\"volume\" type=\"i\" direction=\"in\"/>\n"        \
    "    </method>\n"                                                   \
    "    <method name=\"SetOutputNodeVolume\">\n"                       \
    "      <arg name=\"node_id\" type=\"t\" direction=\"in\"/>\n"       \
    "      <arg name=\"volume\" type=\"i\" direction=\"in\"/>\n"        \
    "    </method>\n"                                                   \
    "    <method name=\"SwapLeftRight\">\n"                             \
    "      <arg name=\"node_id\" type=\"t\" direction=\"in\"/>\n"       \
    "      <arg name=\"swap\" type=\"b\" direction=\"in\"/>\n"          \
    "    </method>\n"                                                   \
    "    <method name=\"SetOutputMute\">\n"                             \
    "      <arg name=\"mute_on\" type=\"b\" direction=\"in\"/>\n"       \
    "    </method>\n"                                                   \
    "    <method name=\"SetOutputUserMute\">\n"                         \
    "      <arg name=\"mute_on\" type=\"b\" direction=\"in\"/>\n"       \
    "    </method>\n"                                                   \
    "    <method name=\"SetSuspendAudio\">\n"                           \
    "      <arg name=\"suspend\" type=\"b\" direction=\"in\"/>\n"       \
    "    </method>\n"                                                   \
    "    <method name=\"SetInputGain\">\n"                              \
    "      <arg name=\"gain\" type=\"i\" direction=\"in\"/>\n"          \
    "    </method>\n"                                                   \
    "    <method name=\"SetInputNodeGain\">\n"                          \
    "      <arg name=\"node_id\" type=\"t\" direction=\"in\"/>\n"       \
    "      <arg name=\"gain\" type=\"i\" direction=\"in\"/>\n"          \
    "    </method>\n"                                                   \
    "    <method name=\"SetInputMute\">\n"                              \
    "      <arg name=\"mute_on\" type=\"b\" direction=\"in\"/>\n"       \
    "    </method>\n"                                                   \
    "    <method name=\"GetVolumeState\">\n"                            \
    "      <arg name=\"output_volume\" type=\"i\" direction=\"out\"/>\n"\
    "      <arg name=\"output_mute\" type=\"b\" direction=\"out\"/>\n"  \
    "      <arg name=\"input_gain\" type=\"i\" direction=\"out\"/>\n"   \
    "      <arg name=\"input_mute\" type=\"b\" direction=\"out\"/>\n"   \
    "      <arg name=\"output_user_mute\" type=\"b\" direction=\"out\"/>\n"\
    "    </method>\n"                                                   \
    "    <method name=\"GetNodes\">\n"                                  \
    "      <arg name=\"nodes\" type=\"a{sv}\" direction=\"out\"/>\n"    \
    "    </method>\n"                                                   \
    "    <method name=\"SetActiveOutputNode\">\n"                       \
    "      <arg name=\"node_id\" type=\"t\" direction=\"in\"/>\n"       \
    "    </method>\n"                                                   \
    "    <method name=\"SetActiveInputNode\">\n"                        \
    "      <arg name=\"node_id\" type=\"t\" direction=\"in\"/>\n"       \
    "    </method>\n"                                                   \
    "    <method name=\"AddActiveInputNode\">\n"                        \
    "      <arg name=\"node_id\" type=\"t\" direction=\"in\"/>\n"       \
    "    </method>\n"                                                   \
    "    <method name=\"AddActiveOutputNode\">\n"                       \
    "      <arg name=\"node_id\" type=\"t\" direction=\"in\"/>\n"       \
    "    </method>\n"                                                   \
    "    <method name=\"RemoveActiveInputNode\">\n"                     \
    "      <arg name=\"node_id\" type=\"t\" direction=\"in\"/>\n"       \
    "    </method>\n"                                                   \
    "    <method name=\"RemoveActiveOutputNode\">\n"                    \
    "      <arg name=\"node_id\" type=\"t\" direction=\"in\"/>\n"       \
    "    </method>\n"                                                   \
    "    <method name=\"GetNumberOfActiveStreams\">\n"                  \
    "      <arg name=\"num\" type=\"i\" direction=\"out\"/>\n"          \
    "    </method>\n"                                                   \
    "    <method name=\"GetNumberOfActiveOutputStreams\">\n"            \
    "      <arg name=\"num\" type=\"i\" direction=\"out\"/>\n"          \
    "    </method>\n"                                                   \
    "    <method name=\"GetNumberOfActiveInputStreams\">\n"             \
    "      <arg name=\"num\" type=\"i\" direction=\"out\"/>\n"          \
    "    </method>\n"                                                   \
    "    <method name=\"SetGlobalOutputChannelRemix\">\n"               \
    "      <arg name=\"num_channels\" type=\"i\" direction=\"in\"/>\n"  \
    "      <arg name=\"coefficient\" type=\"ad\" direction=\"in\"/>\n"  \
    "    </method>\n"                                                   \
    "    <method name=\"SetHotwordModel\">\n"                           \
    "      <arg name=\"node_id\" type=\"t\" direction=\"in\"/>\n"       \
    "      <arg name=\"model_name\" type=\"s\" direction=\"in\"/>\n"    \
    "    </method>\n"                                                   \
    "  </interface>\n"                                                  \
    "  <interface name=\"" DBUS_INTERFACE_INTROSPECTABLE "\">\n"        \
    "    <method name=\"Introspect\">\n"                                \
    "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"         \
    "    </method>\n"                                                   \
    "  </interface>\n"                                                  \
    "</node>\n"

struct cras_dbus_control {
	DBusConnection *conn;
	struct cras_observer_client *observer;
};
static struct cras_dbus_control dbus_control;

/* helper to extract a single argument from a DBus message. */
static int get_single_arg(DBusMessage *message, int dbus_type, void *arg)
{
	DBusError dbus_error;

	dbus_error_init(&dbus_error);

	if (!dbus_message_get_args(message, &dbus_error,
				   dbus_type, arg,
				   DBUS_TYPE_INVALID)) {
		syslog(LOG_WARNING,
		       "Bad method received: %s",
		       dbus_error.message);
		dbus_error_free(&dbus_error);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	return 0;
}

/* Helper to send an empty reply. */
static void send_empty_reply(DBusConnection *conn, DBusMessage *message)
{
	DBusMessage *reply;
	dbus_uint32_t serial = 0;

	reply = dbus_message_new_method_return(message);
	if (!reply)
		return;

	dbus_connection_send(conn, reply, &serial);

	dbus_message_unref(reply);
}

/* Helper to send an int32 reply. */
static void send_int32_reply(DBusConnection *conn,
			     DBusMessage *message,
			     dbus_int32_t value)
{
	DBusMessage *reply;
	dbus_uint32_t serial = 0;

	reply = dbus_message_new_method_return(message);
	if (!reply)
		return;

	dbus_message_append_args(reply,
				 DBUS_TYPE_INT32, &value,
				 DBUS_TYPE_INVALID);
	dbus_connection_send(conn, reply, &serial);

	dbus_message_unref(reply);
}

/* Handlers for exported DBus method calls. */
static DBusHandlerResult handle_set_output_volume(
	DBusConnection *conn,
	DBusMessage *message,
	void *arg)
{
	int rc;
	dbus_int32_t new_vol;

	rc = get_single_arg(message, DBUS_TYPE_INT32, &new_vol);
	if (rc)
		return rc;

	cras_system_set_volume(new_vol);

	send_empty_reply(conn, message);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult handle_set_output_node_volume(
	DBusConnection *conn,
	DBusMessage *message,
	void *arg)
{
	dbus_int32_t new_vol;
	cras_node_id_t id;
	DBusError dbus_error;

	dbus_error_init(&dbus_error);

	if (!dbus_message_get_args(message, &dbus_error,
				   DBUS_TYPE_UINT64, &id,
				   DBUS_TYPE_INT32, &new_vol,
				   DBUS_TYPE_INVALID)) {
		syslog(LOG_WARNING,
		       "Bad method received: %s",
		       dbus_error.message);
		dbus_error_free(&dbus_error);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	cras_iodev_list_set_node_attr(id, IONODE_ATTR_VOLUME, new_vol);

	send_empty_reply(conn, message);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult handle_swap_left_right(
	DBusConnection *conn,
	DBusMessage *message,
	void *arg)
{
	cras_node_id_t id;
	dbus_bool_t swap;
	DBusError dbus_error;

	dbus_error_init(&dbus_error);

	if (!dbus_message_get_args(message, &dbus_error,
				   DBUS_TYPE_UINT64, &id,
				   DBUS_TYPE_BOOLEAN, &swap,
				   DBUS_TYPE_INVALID)) {
		syslog(LOG_WARNING,
		       "Bad method received: %s",
		       dbus_error.message);
		dbus_error_free(&dbus_error);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	cras_iodev_list_set_node_attr(id, IONODE_ATTR_SWAP_LEFT_RIGHT,
				      swap);

	send_empty_reply(conn, message);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult handle_set_output_mute(
	DBusConnection *conn,
	DBusMessage *message,
	void *arg)
{
	int rc;
	dbus_bool_t new_mute;

	rc = get_single_arg(message, DBUS_TYPE_BOOLEAN, &new_mute);
	if (rc)
		return rc;

	cras_system_set_mute(new_mute);

	send_empty_reply(conn, message);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult handle_set_output_user_mute(
	DBusConnection *conn,
	DBusMessage *message,
	void *arg)
{
	int rc;
	dbus_bool_t new_mute;

	rc = get_single_arg(message, DBUS_TYPE_BOOLEAN, &new_mute);
	if (rc)
		return rc;

	cras_system_set_user_mute(new_mute);

	send_empty_reply(conn, message);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult handle_set_suspend_audio(
	DBusConnection *conn,
	DBusMessage *message,
	void *arg)
{
	int rc;
	dbus_bool_t suspend;
	rc = get_single_arg(message, DBUS_TYPE_BOOLEAN, &suspend);
	if (rc)
		return rc;

	cras_system_set_suspended(suspend);

	send_empty_reply(conn, message);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult handle_set_input_gain(
	DBusConnection *conn,
	DBusMessage *message,
	void *arg)
{
	int rc;
	dbus_int32_t new_gain;

	rc = get_single_arg(message, DBUS_TYPE_INT32, &new_gain);
	if (rc)
		return rc;

	cras_system_set_capture_gain(new_gain);

	send_empty_reply(conn, message);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult handle_set_input_node_gain(
	DBusConnection *conn,
	DBusMessage *message,
	void *arg)
{
	dbus_int32_t new_gain;
	cras_node_id_t id;
	DBusError dbus_error;

	dbus_error_init(&dbus_error);

	if (!dbus_message_get_args(message, &dbus_error,
				   DBUS_TYPE_UINT64, &id,
				   DBUS_TYPE_INT32, &new_gain,
				   DBUS_TYPE_INVALID)) {
		syslog(LOG_WARNING,
		       "Bad method received: %s",
		       dbus_error.message);
		dbus_error_free(&dbus_error);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	cras_iodev_list_set_node_attr(id, IONODE_ATTR_CAPTURE_GAIN, new_gain);

	send_empty_reply(conn, message);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult handle_set_input_mute(
	DBusConnection *conn,
	DBusMessage *message,
	void *arg)
{
	int rc;
	dbus_bool_t new_mute;

	rc = get_single_arg(message, DBUS_TYPE_BOOLEAN, &new_mute);
	if (rc)
		return rc;

	cras_system_set_capture_mute(new_mute);

	send_empty_reply(conn, message);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult handle_get_volume_state(
	DBusConnection *conn,
	DBusMessage *message,
	void *arg)
{
	DBusMessage *reply;
	dbus_uint32_t serial = 0;
	dbus_int32_t volume;
	dbus_bool_t system_muted;
	dbus_bool_t user_muted;
	dbus_int32_t capture_gain;
	dbus_bool_t capture_muted;

	reply = dbus_message_new_method_return(message);

	volume = cras_system_get_volume();
	system_muted = cras_system_get_system_mute();
	user_muted = cras_system_get_user_mute();
	capture_gain = cras_system_get_capture_gain();
	capture_muted = cras_system_get_capture_mute();

	dbus_message_append_args(reply,
				 DBUS_TYPE_INT32, &volume,
				 DBUS_TYPE_BOOLEAN, &system_muted,
				 DBUS_TYPE_INT32, &capture_gain,
				 DBUS_TYPE_BOOLEAN, &capture_muted,
				 DBUS_TYPE_BOOLEAN, &user_muted,
				 DBUS_TYPE_INVALID);

	dbus_connection_send(conn, reply, &serial);

	dbus_message_unref(reply);

	return DBUS_HANDLER_RESULT_HANDLED;
}

/* Appends the information about a node to the dbus message. Returns
 * false if not enough memory. */
static dbus_bool_t append_node_dict(DBusMessageIter *iter,
				    const struct cras_iodev_info *dev,
				    const struct cras_ionode_info *node,
				    enum CRAS_STREAM_DIRECTION direction)
{
	DBusMessageIter dict;
	dbus_bool_t is_input;
	dbus_uint64_t id;
	const char *dev_name = dev->name;
	dbus_uint64_t stable_dev_id = node->stable_id;
	dbus_uint64_t stable_dev_id_new = node->stable_id_new;
	const char *node_type = node->type;
	const char *node_name = node->name;
	const char *mic_positions = node->mic_positions;
	dbus_bool_t active;
	dbus_uint64_t plugged_time = node->plugged_time.tv_sec * 1000000ULL +
		node->plugged_time.tv_usec;
	dbus_uint64_t node_volume = node->volume;
	dbus_int64_t node_capture_gain = node->capture_gain;
	char *models, *empty_models = "";

	is_input = (direction == CRAS_STREAM_INPUT);
	id = node->iodev_idx;
	id = (id << 32) | node->ionode_idx;
	active = !!node->active;

	if (!dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, "{sv}",
					      &dict))
		return FALSE;
	if (!append_key_value(&dict, "IsInput", DBUS_TYPE_BOOLEAN,
			      DBUS_TYPE_BOOLEAN_AS_STRING, &is_input))
		return FALSE;
	if (!append_key_value(&dict, "Id", DBUS_TYPE_UINT64,
			      DBUS_TYPE_UINT64_AS_STRING, &id))
		return FALSE;
	if (!append_key_value(&dict, "DeviceName", DBUS_TYPE_STRING,
			      DBUS_TYPE_STRING_AS_STRING, &dev_name))
		return FALSE;
	if (!append_key_value(&dict, "StableDeviceId", DBUS_TYPE_UINT64,
			      DBUS_TYPE_UINT64_AS_STRING, &stable_dev_id))
		return FALSE;
	if (!append_key_value(&dict, "StableDeviceIdNew", DBUS_TYPE_UINT64,
			      DBUS_TYPE_UINT64_AS_STRING, &stable_dev_id_new))
		return FALSE;
	if (!append_key_value(&dict, "Type", DBUS_TYPE_STRING,
			      DBUS_TYPE_STRING_AS_STRING, &node_type))
		return FALSE;
	if (!append_key_value(&dict, "Name", DBUS_TYPE_STRING,
			      DBUS_TYPE_STRING_AS_STRING, &node_name))
		return FALSE;
	if (!append_key_value(&dict, "MicPositions", DBUS_TYPE_STRING,
			      DBUS_TYPE_STRING_AS_STRING, &mic_positions))
		return FALSE;
	if (!append_key_value(&dict, "Active", DBUS_TYPE_BOOLEAN,
			      DBUS_TYPE_BOOLEAN_AS_STRING, &active))
		return FALSE;
	if (!append_key_value(&dict, "PluggedTime", DBUS_TYPE_UINT64,
			      DBUS_TYPE_UINT64_AS_STRING, &plugged_time))
		return FALSE;
	if (!append_key_value(&dict, "NodeVolume", DBUS_TYPE_UINT64,
			      DBUS_TYPE_UINT64_AS_STRING, &node_volume))
		return FALSE;
	if (!append_key_value(&dict, "NodeCaptureGain", DBUS_TYPE_INT64,
			      DBUS_TYPE_INT64_AS_STRING, &node_capture_gain))
		return FALSE;

	models = cras_iodev_list_get_hotword_models(id);
	if (!append_key_value(&dict, "HotwordModels", DBUS_TYPE_STRING,
			      DBUS_TYPE_STRING_AS_STRING,
			      models ? &models : &empty_models)) {
		free(models);
		return FALSE;
	}
	free(models);

	if (!dbus_message_iter_close_container(iter, &dict))
		return FALSE;

	return TRUE;
}

/* Appends the information about all nodes in a given direction. Returns false
 * if not enough memory. */
static dbus_bool_t append_nodes(enum CRAS_STREAM_DIRECTION direction,
				DBusMessageIter *array)
{
	const struct cras_iodev_info *devs;
	const struct cras_ionode_info *nodes;
	int ndevs, nnodes;
	int i, j;

	if (direction == CRAS_STREAM_OUTPUT) {
		ndevs = cras_system_state_get_output_devs(&devs);
		nnodes = cras_system_state_get_output_nodes(&nodes);
	} else {
		ndevs = cras_system_state_get_input_devs(&devs);
		nnodes = cras_system_state_get_input_nodes(&nodes);
	}

	for (i = 0; i < nnodes; i++) {
		/* Don't reply unplugged nodes. */
		if (!nodes[i].plugged)
			continue;
		/* Find the device for this node. */
		for (j = 0; j < ndevs; j++)
			if (devs[j].idx == nodes[i].iodev_idx)
				break;
		if (j == ndevs)
			continue;
		/* Send information about this node. */
		if (!append_node_dict(array, &devs[j], &nodes[i], direction))
			return FALSE;
	}

	return TRUE;
}

static DBusHandlerResult handle_get_nodes(DBusConnection *conn,
					  DBusMessage *message,
					  void *arg)
{
	DBusMessage *reply;
	DBusMessageIter array;
	dbus_uint32_t serial = 0;

	reply = dbus_message_new_method_return(message);
	dbus_message_iter_init_append(reply, &array);
	if (!append_nodes(CRAS_STREAM_OUTPUT, &array))
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	if (!append_nodes(CRAS_STREAM_INPUT, &array))
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	dbus_connection_send(conn, reply, &serial);
	dbus_message_unref(reply);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_set_active_node(DBusConnection *conn,
		       DBusMessage *message,
		       void *arg,
		       enum CRAS_STREAM_DIRECTION direction)
{
	int rc;
	cras_node_id_t id;

	rc = get_single_arg(message, DBUS_TYPE_UINT64, &id);
	if (rc)
		return rc;

	cras_iodev_list_select_node(direction, id);

	send_empty_reply(conn, message);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_add_active_node(DBusConnection *conn,
		       DBusMessage *message,
		       void *arg,
                       enum CRAS_STREAM_DIRECTION direction)
{
	int rc;
	cras_node_id_t id;

	rc = get_single_arg(message, DBUS_TYPE_UINT64, &id);
	if (rc)
		return rc;

	cras_iodev_list_add_active_node(direction, id);

	send_empty_reply(conn, message);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_rm_active_node(DBusConnection *conn,
                      DBusMessage *message,
                      void *arg,
                      enum CRAS_STREAM_DIRECTION direction)
{
        int rc;
        cras_node_id_t id;

        rc = get_single_arg(message, DBUS_TYPE_UINT64, &id);
        if (rc)
                return rc;

        cras_iodev_list_rm_active_node(direction, id);

        send_empty_reply(conn, message);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult handle_get_num_active_streams(
	DBusConnection *conn,
	DBusMessage *message,
	void *arg)
{
	send_int32_reply(conn, message, cras_system_state_get_active_streams());
	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult handle_get_num_active_streams_use_input_hw(
	DBusConnection *conn,
	DBusMessage *message,
	void *arg)
{
	dbus_int32_t num = 0;
	unsigned i;

	for (i = 0; i < CRAS_NUM_DIRECTIONS; i++) {
		if (cras_stream_uses_input_hw(i))
			num += cras_system_state_get_active_streams_by_direction(i);
	}
	send_int32_reply(conn, message, num);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult handle_get_num_active_streams_use_output_hw(
	DBusConnection *conn,
	DBusMessage *message,
	void *arg)
{
	dbus_int32_t num = 0;
	unsigned i;

	for (i = 0; i < CRAS_NUM_DIRECTIONS; i++) {
		if (cras_stream_uses_output_hw(i))
			num += cras_system_state_get_active_streams_by_direction(i);
	}
	send_int32_reply(conn, message, num);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult handle_set_global_output_channel_remix(
	DBusConnection *conn,
	DBusMessage *message,
	void *arg)
{
	dbus_int32_t num_channels;
	double *coeff_array;
	dbus_int32_t count;
	DBusError dbus_error;
	float *coefficient;
	int i;

	dbus_error_init(&dbus_error);

	if (!dbus_message_get_args(message, &dbus_error,
			DBUS_TYPE_INT32, &num_channels,
			DBUS_TYPE_ARRAY,
			      DBUS_TYPE_DOUBLE, &coeff_array, &count,
			      DBUS_TYPE_INVALID)) {
		syslog(LOG_WARNING, "Set global output channel remix error: %s",
			dbus_error.message);
		dbus_error_free(&dbus_error);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	coefficient = (float *)calloc(count, sizeof(*coefficient));
	if (!coefficient)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	for (i = 0; i < count; i++)
		coefficient[i] = coeff_array[i];

	audio_thread_config_global_remix(
			cras_iodev_list_get_audio_thread(),
			num_channels,
			coefficient);

	send_empty_reply(conn, message);
	free(coefficient);
	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult handle_set_hotword_model(
	DBusConnection *conn,
	DBusMessage *message,
	void *arg)
{
	cras_node_id_t id;
	const char *model_name;
	DBusError dbus_error;
	dbus_int32_t ret;

	dbus_error_init(&dbus_error);

	if (!dbus_message_get_args(message, &dbus_error,
				   DBUS_TYPE_UINT64, &id,
				   DBUS_TYPE_STRING, &model_name,
				   DBUS_TYPE_INVALID)) {
		syslog(LOG_WARNING,
		       "Bad method received: %s",
		       dbus_error.message);
		dbus_error_free(&dbus_error);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	ret = cras_iodev_list_set_hotword_model(id, model_name);
	send_int32_reply(conn, message, ret);

	return DBUS_HANDLER_RESULT_HANDLED;
}

/* Handle incoming messages. */
static DBusHandlerResult handle_control_message(DBusConnection *conn,
						DBusMessage *message,
						void *arg)
{
	syslog(LOG_DEBUG, "Control message: %s %s %s",
	       dbus_message_get_path(message),
	       dbus_message_get_interface(message),
	       dbus_message_get_member(message));

	if (dbus_message_is_method_call(message,
					DBUS_INTERFACE_INTROSPECTABLE,
					"Introspect")) {
		DBusMessage *reply;
		const char *xml = CONTROL_INTROSPECT_XML;

		reply = dbus_message_new_method_return(message);
		if (!reply)
			return DBUS_HANDLER_RESULT_NEED_MEMORY;
		if (!dbus_message_append_args(reply,
					      DBUS_TYPE_STRING, &xml,
					      DBUS_TYPE_INVALID))
			return DBUS_HANDLER_RESULT_NEED_MEMORY;
		if (!dbus_connection_send(conn, reply, NULL))
			return DBUS_HANDLER_RESULT_NEED_MEMORY;

		dbus_message_unref(reply);
		return DBUS_HANDLER_RESULT_HANDLED;

	} else if (dbus_message_is_method_call(message,
					       CRAS_CONTROL_INTERFACE,
					       "SetOutputVolume")) {
		return handle_set_output_volume(conn, message, arg);
	} else if (dbus_message_is_method_call(message,
					       CRAS_CONTROL_INTERFACE,
					       "SetOutputNodeVolume")) {
		return handle_set_output_node_volume(conn, message, arg);
	} else if (dbus_message_is_method_call(message,
					       CRAS_CONTROL_INTERFACE,
					       "SwapLeftRight")) {
		return handle_swap_left_right(conn, message, arg);
	} else if (dbus_message_is_method_call(message,
					       CRAS_CONTROL_INTERFACE,
					       "SetOutputMute")) {
		return handle_set_output_mute(conn, message, arg);
	} else if (dbus_message_is_method_call(message,
					       CRAS_CONTROL_INTERFACE,
					       "SetOutputUserMute")) {
		return handle_set_output_user_mute(conn, message, arg);
	} else if (dbus_message_is_method_call(message,
					       CRAS_CONTROL_INTERFACE,
					       "SetSuspendAudio")) {
		return handle_set_suspend_audio(conn, message, arg);
	} else if (dbus_message_is_method_call(message,
					       CRAS_CONTROL_INTERFACE,
					       "SetInputGain")) {
		return handle_set_input_gain(conn, message, arg);
	} else if (dbus_message_is_method_call(message,
					       CRAS_CONTROL_INTERFACE,
					       "SetInputNodeGain")) {
		return handle_set_input_node_gain(conn, message, arg);
	} else if (dbus_message_is_method_call(message,
					       CRAS_CONTROL_INTERFACE,
					       "SetInputMute")) {
		return handle_set_input_mute(conn, message, arg);
	} else if (dbus_message_is_method_call(message,
					       CRAS_CONTROL_INTERFACE,
					       "GetVolumeState")) {
		return handle_get_volume_state(conn, message, arg);
	} else if (dbus_message_is_method_call(message,
					       CRAS_CONTROL_INTERFACE,
					       "GetNodes")) {
		return handle_get_nodes(conn, message, arg);
	} else if (dbus_message_is_method_call(message,
					       CRAS_CONTROL_INTERFACE,
					       "SetActiveOutputNode")) {
		return handle_set_active_node(conn, message, arg,
					      CRAS_STREAM_OUTPUT);
	} else if (dbus_message_is_method_call(message,
					       CRAS_CONTROL_INTERFACE,
					       "SetActiveInputNode")) {
		return handle_set_active_node(conn, message, arg,
					      CRAS_STREAM_INPUT);
	} else if (dbus_message_is_method_call(message,
					       CRAS_CONTROL_INTERFACE,
					       "AddActiveInputNode")) {
		return handle_add_active_node(conn, message, arg,
                                              CRAS_STREAM_INPUT);
        } else if (dbus_message_is_method_call(message,
                                               CRAS_CONTROL_INTERFACE,
                                               "AddActiveOutputNode")) {
                return handle_add_active_node(conn, message, arg,
                                              CRAS_STREAM_OUTPUT);
        } else if (dbus_message_is_method_call(message,
                                               CRAS_CONTROL_INTERFACE,
                                               "RemoveActiveInputNode")) {
                return handle_rm_active_node(conn, message, arg,
                                             CRAS_STREAM_INPUT);
        } else if (dbus_message_is_method_call(message,
                                               CRAS_CONTROL_INTERFACE,
                                               "RemoveActiveOutputNode")) {
                return handle_rm_active_node(conn, message, arg,
                                             CRAS_STREAM_OUTPUT);
	} else if (dbus_message_is_method_call(message,
					       CRAS_CONTROL_INTERFACE,
					       "GetNumberOfActiveStreams")) {
		return handle_get_num_active_streams(conn, message, arg);
	} else if (dbus_message_is_method_call(message,
						   CRAS_CONTROL_INTERFACE,
						   "GetNumberOfActiveInputStreams")) {
		return handle_get_num_active_streams_use_input_hw(
				conn, message, arg);
	} else if (dbus_message_is_method_call(message,
						   CRAS_CONTROL_INTERFACE,
						   "GetNumberOfActiveOutputStreams")) {
		return handle_get_num_active_streams_use_output_hw(
				conn, message, arg);
	} else if (dbus_message_is_method_call(message,
					       CRAS_CONTROL_INTERFACE,
					       "SetGlobalOutputChannelRemix")) {
		return handle_set_global_output_channel_remix(
				conn, message, arg);
	} else if (dbus_message_is_method_call(message,
					       CRAS_CONTROL_INTERFACE,
					       "SetHotwordModel")) {
		return handle_set_hotword_model(conn, message, arg);
	}


	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

/* Creates a new DBus message, must be freed with dbus_message_unref. */
static DBusMessage *create_dbus_message(const char *name)
{
	DBusMessage *msg;
	msg = dbus_message_new_signal(CRAS_ROOT_OBJECT_PATH,
				      CRAS_CONTROL_INTERFACE,
				      name);
	if (!msg)
		syslog(LOG_ERR, "Failed to create signal");

	return msg;
}

/* Handlers for system updates that generate DBus signals. */

static void signal_output_volume(void *context, int32_t volume)
{
	struct cras_dbus_control *control = (struct cras_dbus_control *)context;
	dbus_uint32_t serial = 0;
	DBusMessage *msg;

	msg = create_dbus_message("OutputVolumeChanged");
	if (!msg)
		return;

	volume = cras_system_get_volume();
	dbus_message_append_args(msg,
				 DBUS_TYPE_INT32, &volume,
				 DBUS_TYPE_INVALID);
	dbus_connection_send(control->conn, msg, &serial);
	dbus_message_unref(msg);
}

static void signal_output_mute(void *context, int muted, int user_muted,
			       int mute_locked)
{
	struct cras_dbus_control *control = (struct cras_dbus_control *)context;
	dbus_uint32_t serial = 0;
	DBusMessage *msg;

	msg = create_dbus_message("OutputMuteChanged");
	if (!msg)
		return;

	muted = cras_system_get_system_mute();
	user_muted = cras_system_get_user_mute();
	dbus_message_append_args(msg,
				 DBUS_TYPE_BOOLEAN, &muted,
				 DBUS_TYPE_BOOLEAN, &user_muted,
				 DBUS_TYPE_INVALID);
	dbus_connection_send(control->conn, msg, &serial);
	dbus_message_unref(msg);
}

static void signal_capture_gain(void *context, int32_t gain)
{
	struct cras_dbus_control *control = (struct cras_dbus_control *)context;
	dbus_uint32_t serial = 0;
	DBusMessage *msg;

	msg = create_dbus_message("InputGainChanged");
	if (!msg)
		return;

	dbus_message_append_args(msg,
				 DBUS_TYPE_INT32, &gain,
				 DBUS_TYPE_INVALID);
	dbus_connection_send(control->conn, msg, &serial);
	dbus_message_unref(msg);
}

static void signal_capture_mute(void *context, int muted, int mute_locked)
{
	struct cras_dbus_control *control = (struct cras_dbus_control *)context;
	dbus_uint32_t serial = 0;
	DBusMessage *msg;

	msg = create_dbus_message("InputMuteChanged");
	if (!msg)
		return;

	dbus_message_append_args(msg,
				 DBUS_TYPE_BOOLEAN, &muted,
				 DBUS_TYPE_INVALID);
	dbus_connection_send(control->conn, msg, &serial);
	dbus_message_unref(msg);
}

static void signal_nodes_changed(void *context)
{
	struct cras_dbus_control *control = (struct cras_dbus_control *)context;
	dbus_uint32_t serial = 0;
	DBusMessage *msg;

	msg = create_dbus_message("NodesChanged");
	if (!msg)
		return;

	dbus_connection_send(control->conn, msg, &serial);
	dbus_message_unref(msg);
}

static void signal_active_node_changed(void *context,
				       enum CRAS_STREAM_DIRECTION dir,
				       cras_node_id_t node_id)
{
	struct cras_dbus_control *control = (struct cras_dbus_control *)context;
	DBusMessage *msg;
	dbus_uint32_t serial = 0;

	msg = create_dbus_message((dir == CRAS_STREAM_OUTPUT)
			? "ActiveOutputNodeChanged"
			: "ActiveInputNodeChanged");
	if (!msg)
		return;
	dbus_message_append_args(msg,
				 DBUS_TYPE_UINT64, &node_id,
				 DBUS_TYPE_INVALID);
	dbus_connection_send(control->conn, msg, &serial);
	dbus_message_unref(msg);
}

/* Called by iodev_list when a node volume changes. */
static void signal_node_volume_changed(void *context,
				       cras_node_id_t node_id,
				       int32_t volume)
{
	struct cras_dbus_control *control = (struct cras_dbus_control *)context;
	dbus_uint32_t serial = 0;
	DBusMessage *msg;

	msg = create_dbus_message("OutputNodeVolumeChanged");
	if (!msg)
		return;

	dbus_message_append_args(msg,
				 DBUS_TYPE_UINT64, &node_id,
				 DBUS_TYPE_INT32, &volume,
				 DBUS_TYPE_INVALID);
	dbus_connection_send(control->conn, msg, &serial);
	dbus_message_unref(msg);
}

static void signal_node_capture_gain_changed(void *context,
					     cras_node_id_t node_id,
					     int capture_gain)
{
	struct cras_dbus_control *control = (struct cras_dbus_control *)context;
	dbus_uint32_t serial = 0;
	DBusMessage *msg;

	msg = create_dbus_message("InputNodeGainChanged");
	if (!msg)
		return;

	dbus_message_append_args(msg,
				 DBUS_TYPE_UINT64, &node_id,
				 DBUS_TYPE_INT32, &capture_gain,
				 DBUS_TYPE_INVALID);
	dbus_connection_send(control->conn, msg, &serial);
	dbus_message_unref(msg);
}

static void signal_node_left_right_swapped_changed(void *context,
						   cras_node_id_t node_id,
						   int swapped)
{
	struct cras_dbus_control *control = (struct cras_dbus_control *)context;
	dbus_uint32_t serial = 0;
	DBusMessage *msg;

	msg = create_dbus_message("NodeLeftRightSwappedChanged");
	if (!msg)
		return;

	dbus_message_append_args(msg,
				 DBUS_TYPE_UINT64, &node_id,
				 DBUS_TYPE_BOOLEAN, &swapped,
				 DBUS_TYPE_INVALID);
	dbus_connection_send(control->conn, msg, &serial);
	dbus_message_unref(msg);
}

static void signal_num_active_streams_changed(void *context,
					      enum CRAS_STREAM_DIRECTION dir,
					      uint32_t num_active_streams)
{
	struct cras_dbus_control *control = (struct cras_dbus_control *)context;
	dbus_uint32_t serial = 0;
	DBusMessage *msg;
	dbus_int32_t num;

	msg = create_dbus_message("NumberOfActiveStreamsChanged");
	if (!msg)
		return;

	num = cras_system_state_get_active_streams();
	dbus_message_append_args(msg,
				 DBUS_TYPE_INT32, &num,
				 DBUS_TYPE_INVALID);
	dbus_connection_send(control->conn, msg, &serial);
	dbus_message_unref(msg);
}

/* Exported Interface */

void cras_dbus_control_start(DBusConnection *conn)
{
	static const DBusObjectPathVTable control_vtable = {
		.message_function = handle_control_message,
	};

	DBusError dbus_error;
	struct cras_observer_ops observer_ops;

	dbus_control.conn = conn;
	dbus_connection_ref(dbus_control.conn);

	if (!dbus_connection_register_object_path(conn,
						  CRAS_ROOT_OBJECT_PATH,
						  &control_vtable,
						  &dbus_error)) {
		syslog(LOG_WARNING,
		       "Couldn't register CRAS control: %s: %s",
		       CRAS_ROOT_OBJECT_PATH, dbus_error.message);
		dbus_error_free(&dbus_error);
		return;
	}

	memset(&observer_ops, 0, sizeof(observer_ops));
	observer_ops.output_volume_changed = signal_output_volume;
	observer_ops.output_mute_changed = signal_output_mute;
	observer_ops.capture_gain_changed = signal_capture_gain;
	observer_ops.capture_mute_changed = signal_capture_mute;
	observer_ops.num_active_streams_changed =
			signal_num_active_streams_changed;
	observer_ops.nodes_changed = signal_nodes_changed;
	observer_ops.active_node_changed = signal_active_node_changed;
	observer_ops.input_node_gain_changed = signal_node_capture_gain_changed;
	observer_ops.output_node_volume_changed = signal_node_volume_changed;
	observer_ops.node_left_right_swapped_changed =
			signal_node_left_right_swapped_changed;

	dbus_control.observer = cras_observer_add(&observer_ops, &dbus_control);
}

void cras_dbus_control_stop()
{
	if (!dbus_control.conn)
		return;

	dbus_connection_unregister_object_path(dbus_control.conn,
					       CRAS_ROOT_OBJECT_PATH);

	dbus_connection_unref(dbus_control.conn);
	dbus_control.conn = NULL;
	cras_observer_remove(dbus_control.observer);
	dbus_control.observer = NULL;
}
