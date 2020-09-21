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
#include <unistd.h>

#include "cras_bt_device.h"
#include "cras_bt_endpoint.h"
#include "cras_bt_transport.h"
#include "cras_bt_constants.h"
#include "utlist.h"


struct cras_bt_transport {
	DBusConnection *conn;
	char *object_path;
	struct cras_bt_device *device;
	enum cras_bt_device_profile profile;
	int codec;
	void *configuration;
	int configuration_len;
	enum cras_bt_transport_state state;
	int fd;
	uint16_t read_mtu;
	uint16_t write_mtu;
	int volume;

	struct cras_bt_endpoint *endpoint;
	struct cras_bt_transport *prev, *next;
};

static struct cras_bt_transport *transports;

struct cras_bt_transport *cras_bt_transport_create(DBusConnection *conn,
						   const char *object_path)
{
	struct cras_bt_transport *transport;

	transport = calloc(1, sizeof(*transport));
	if (transport == NULL)
		return NULL;

	transport->object_path = strdup(object_path);
	if (transport->object_path == NULL) {
		free(transport);
		return NULL;
	}

	transport->conn = conn;
	dbus_connection_ref(transport->conn);

	transport->fd = -1;
	transport->volume = -1;

	DL_APPEND(transports, transport);

	return transport;
}

void cras_bt_transport_set_endpoint(struct cras_bt_transport *transport,
				    struct cras_bt_endpoint *endpoint) {
	transport->endpoint = endpoint;
}

void cras_bt_transport_destroy(struct cras_bt_transport *transport)
{
	DL_DELETE(transports, transport);

	dbus_connection_unref(transport->conn);

	if (transport->fd >= 0)
		close(transport->fd);

	free(transport->object_path);
	free(transport->configuration);
	free(transport);
}

void cras_bt_transport_reset()
{
	while (transports) {
		syslog(LOG_INFO, "Bluetooth Transport: %s removed",
		       transports->object_path);
		cras_bt_transport_destroy(transports);
	}
}


struct cras_bt_transport *cras_bt_transport_get(const char *object_path)
{
	struct cras_bt_transport *transport;

	DL_FOREACH(transports, transport) {
		if (strcmp(transport->object_path, object_path) == 0)
			return transport;
	}

	return NULL;
}

size_t cras_bt_transport_get_list(
	struct cras_bt_transport ***transport_list_out)
{
	struct cras_bt_transport *transport;
	struct cras_bt_transport **transport_list = NULL;
	size_t num_transports = 0;

	DL_FOREACH(transports, transport) {
		struct cras_bt_transport **tmp;

		tmp = realloc(transport_list,
			      sizeof(transport_list[0]) * (num_transports + 1));
		if (!tmp) {
			free(transport_list);
			return -ENOMEM;
		}

		transport_list = tmp;
		transport_list[num_transports++] = transport;
	}

	*transport_list_out = transport_list;
	return num_transports;
}

const char *cras_bt_transport_object_path(
	const struct cras_bt_transport *transport)
{
	return transport->object_path;
}

struct cras_bt_device *cras_bt_transport_device(
	const struct cras_bt_transport *transport)
{
	return transport->device;
}

enum cras_bt_device_profile cras_bt_transport_profile(
	const struct cras_bt_transport *transport)
{
	return transport->profile;
}

int cras_bt_transport_configuration(const struct cras_bt_transport *transport,
				    void *configuration, int len)
{
	if (len < transport->configuration_len)
		return -ENOSPC;

	memcpy(configuration, transport->configuration,
	       transport->configuration_len);

	return 0;
}

enum cras_bt_transport_state cras_bt_transport_state(
	const struct cras_bt_transport *transport)
{
	return transport->state;
}

int cras_bt_transport_fd(const struct cras_bt_transport *transport)
{
	return transport->fd;
}

uint16_t cras_bt_transport_write_mtu(const struct cras_bt_transport *transport)
{
	return transport->write_mtu;
}

static enum cras_bt_transport_state cras_bt_transport_state_from_string(
	const char *value)
{
	if (strcmp("idle", value) == 0)
		return CRAS_BT_TRANSPORT_STATE_IDLE;
	else if (strcmp("pending", value) == 0)
		return CRAS_BT_TRANSPORT_STATE_PENDING;
	else if (strcmp("active", value) == 0)
		return CRAS_BT_TRANSPORT_STATE_ACTIVE;
	else
		return CRAS_BT_TRANSPORT_STATE_IDLE;
}

static void cras_bt_transport_state_changed(struct cras_bt_transport *transport)
{
	if (transport->endpoint &&
	    transport->endpoint->transport_state_changed)
		transport->endpoint->transport_state_changed(
				transport->endpoint,
				transport);
}

/* Updates bt_device when certain transport property has changed. */
static void cras_bt_transport_update_device(struct cras_bt_transport *transport)
{
	if (!transport->device)
		return;

	/* When the transport has non-negaive volume, it means the remote
	 * BT audio devices supports AVRCP absolute volume. Set the flag in bt
	 * device to use hardware volume. Also map the volume value from 0-127
	 * to 0-100.
	 */
	if (transport->volume != -1) {
		cras_bt_device_set_use_hardware_volume(transport->device, 1);
		cras_bt_device_update_hardware_volume(
				transport->device,
				transport->volume * 100 / 127);
	}
}

void cras_bt_transport_update_properties(
	struct cras_bt_transport *transport,
	DBusMessageIter *properties_array_iter,
	DBusMessageIter *invalidated_array_iter)
{
	while (dbus_message_iter_get_arg_type(properties_array_iter) !=
	       DBUS_TYPE_INVALID) {
		DBusMessageIter properties_dict_iter, variant_iter;
		const char *key;
		int type;

		dbus_message_iter_recurse(properties_array_iter,
					  &properties_dict_iter);

		dbus_message_iter_get_basic(&properties_dict_iter, &key);
		dbus_message_iter_next(&properties_dict_iter);

		dbus_message_iter_recurse(&properties_dict_iter, &variant_iter);
		type = dbus_message_iter_get_arg_type(&variant_iter);

		if (type == DBUS_TYPE_STRING) {
			const char *value;

			dbus_message_iter_get_basic(&variant_iter, &value);

			if (strcmp(key, "UUID") == 0) {
				transport->profile =
					cras_bt_device_profile_from_uuid(value);

			} else if (strcmp(key, "State") == 0) {
				enum cras_bt_transport_state old_state =
					transport->state;
				transport->state =
					cras_bt_transport_state_from_string(
						value);
				if (transport->state != old_state)
					cras_bt_transport_state_changed(
						transport);
			}

		} else if (type == DBUS_TYPE_BYTE) {
			int value;

			dbus_message_iter_get_basic(&variant_iter, &value);

			if (strcmp(key, "Codec") == 0)
				transport->codec = value;
		} else if (type == DBUS_TYPE_OBJECT_PATH) {
			const char *obj_path;

			/* Property: object Device [readonly] */
			dbus_message_iter_get_basic(&variant_iter, &obj_path);
			transport->device = cras_bt_device_get(obj_path);
			if (!transport->device) {
				syslog(LOG_ERR, "Device %s not found at update"
				       "transport properties",
				       obj_path);
				transport->device =
					cras_bt_device_create(transport->conn,
							      obj_path);
				cras_bt_transport_update_device(transport);
			}
		} else if (strcmp(
				dbus_message_iter_get_signature(&variant_iter),
				"ay") == 0 &&
			   strcmp(key, "Configuration") == 0) {
			DBusMessageIter value_iter;
			char *value;
			int len;

			dbus_message_iter_recurse(&variant_iter, &value_iter);
			dbus_message_iter_get_fixed_array(&value_iter,
							  &value, &len);

			free(transport->configuration);
			transport->configuration_len = 0;

			transport->configuration = malloc(len);
			if (transport->configuration) {
				memcpy(transport->configuration, value, len);
				transport->configuration_len = len;
			}

		} else if (strcmp(key, "Volume") == 0) {
			uint16_t volume;

			dbus_message_iter_get_basic(&variant_iter, &volume);
			transport->volume = volume;
			cras_bt_transport_update_device(transport);
		}

		dbus_message_iter_next(properties_array_iter);
	}

	while (invalidated_array_iter &&
	       dbus_message_iter_get_arg_type(invalidated_array_iter) !=
	       DBUS_TYPE_INVALID) {
		const char *key;

		dbus_message_iter_get_basic(invalidated_array_iter, &key);

		if (strcmp(key, "Device") == 0) {
			transport->device = NULL;
		} else if (strcmp(key, "UUID") == 0) {
			transport->profile = 0;
		} else if (strcmp(key, "State") == 0) {
			transport->state = CRAS_BT_TRANSPORT_STATE_IDLE;
		} else if (strcmp(key, "Codec") == 0) {
			transport->codec = 0;
		} else if (strcmp(key, "Configuration") == 0) {
			free(transport->configuration);
			transport->configuration = NULL;
			transport->configuration_len = 0;
		}

		dbus_message_iter_next(invalidated_array_iter);
	}
}

static void on_transport_volume_set(DBusPendingCall *pending_call, void *data)
{
	DBusMessage *reply;

	reply = dbus_pending_call_steal_reply(pending_call);
	dbus_pending_call_unref(pending_call);

	if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR)
		syslog(LOG_ERR, "Set absolute volume returned error: %s",
		       dbus_message_get_error_name(reply));
	dbus_message_unref(reply);
}

int cras_bt_transport_set_volume(struct cras_bt_transport *transport,
				 uint16_t volume)
{
	const char *key = "Volume";
	const char *interface = BLUEZ_INTERFACE_MEDIA_TRANSPORT;
	DBusMessage *method_call;
	DBusMessageIter message_iter, variant;
	DBusPendingCall *pending_call;

	method_call = dbus_message_new_method_call(
		BLUEZ_SERVICE,
		transport->object_path,
		DBUS_INTERFACE_PROPERTIES,
		"Set");
	if (!method_call)
		return -ENOMEM;

	dbus_message_iter_init_append(method_call, &message_iter);

	dbus_message_iter_append_basic(&message_iter, DBUS_TYPE_STRING,
				       &interface);
	dbus_message_iter_append_basic(&message_iter, DBUS_TYPE_STRING, &key);

	dbus_message_iter_open_container(&message_iter, DBUS_TYPE_VARIANT,
					 DBUS_TYPE_UINT16_AS_STRING, &variant);
	dbus_message_iter_append_basic(&variant, DBUS_TYPE_UINT16, &volume);
	dbus_message_iter_close_container(&message_iter, &variant);

	if (!dbus_connection_send_with_reply(transport->conn, method_call,
					     &pending_call,
					     DBUS_TIMEOUT_USE_DEFAULT)) {
		dbus_message_unref(method_call);
		return -ENOMEM;
	}

	dbus_message_unref(method_call);
	if (!pending_call)
		return -EIO;

	if (!dbus_pending_call_set_notify(pending_call,
					  on_transport_volume_set,
					  NULL, NULL)) {
		dbus_pending_call_cancel(pending_call);
		dbus_pending_call_unref(pending_call);
		return -ENOMEM;
	}

	return 0;
}

int cras_bt_transport_acquire(struct cras_bt_transport *transport)
{
	DBusMessage *method_call, *reply;
	DBusError dbus_error;

	if (transport->fd >= 0)
		return 0;

	method_call = dbus_message_new_method_call(
		BLUEZ_SERVICE,
		transport->object_path,
		BLUEZ_INTERFACE_MEDIA_TRANSPORT,
		"Acquire");
	if (!method_call)
		return -ENOMEM;

	dbus_error_init(&dbus_error);

	reply = dbus_connection_send_with_reply_and_block(
		transport->conn,
		method_call,
		DBUS_TIMEOUT_USE_DEFAULT,
		&dbus_error);
	if (!reply) {
		syslog(LOG_ERR, "Failed to acquire transport %s: %s",
		       transport->object_path, dbus_error.message);
		dbus_error_free(&dbus_error);
		dbus_message_unref(method_call);
		return -EIO;
	}

	dbus_message_unref(method_call);

	if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
		syslog(LOG_ERR, "Acquire returned error: %s",
		       dbus_message_get_error_name(reply));
		dbus_message_unref(reply);
		return -EIO;
	}

	if (!dbus_message_get_args(reply, &dbus_error,
				   DBUS_TYPE_UNIX_FD, &(transport->fd),
				   DBUS_TYPE_UINT16, &(transport->read_mtu),
				   DBUS_TYPE_UINT16, &(transport->write_mtu),
				   DBUS_TYPE_INVALID)) {
		syslog(LOG_ERR, "Bad Acquire reply received: %s",
		       dbus_error.message);
		dbus_error_free(&dbus_error);
		dbus_message_unref(reply);
		return -EINVAL;
	}

	dbus_message_unref(reply);
	return 0;
}

int cras_bt_transport_try_acquire(struct cras_bt_transport *transport)
{
	DBusMessage *method_call, *reply;
	DBusError dbus_error;
	int fd, read_mtu, write_mtu;

	method_call = dbus_message_new_method_call(
			BLUEZ_SERVICE,
			transport->object_path,
			BLUEZ_INTERFACE_MEDIA_TRANSPORT,
			"TryAcquire");
	if (!method_call)
		return -ENOMEM;

	dbus_error_init(&dbus_error);

	reply = dbus_connection_send_with_reply_and_block(
		transport->conn,
		method_call,
		DBUS_TIMEOUT_USE_DEFAULT,
		&dbus_error);
	if (!reply) {
		syslog(LOG_ERR, "Failed to try acquire transport %s: %s",
		       transport->object_path, dbus_error.message);
		dbus_error_free(&dbus_error);
		dbus_message_unref(method_call);
		return -EIO;
	}

	dbus_message_unref(method_call);

	if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
		syslog(LOG_ERR, "TryAcquire returned error: %s",
		       dbus_message_get_error_name(reply));
		dbus_message_unref(reply);
		return -EIO;
	}

	if (!dbus_message_get_args(reply, &dbus_error,
				   DBUS_TYPE_UNIX_FD, &fd,
				   DBUS_TYPE_UINT16, &read_mtu,
				   DBUS_TYPE_UINT16, &write_mtu,
				   DBUS_TYPE_INVALID)) {
		syslog(LOG_ERR, "Bad TryAcquire reply received: %s",
		       dbus_error.message);
		dbus_error_free(&dbus_error);
		dbus_message_unref(reply);
		return -EINVAL;
	}

	/* Done TryAcquired the transport so it won't be released in bluez,
	 * no need for the new file descriptor so close it. */
	if (transport->fd != fd)
		close(fd);

	dbus_message_unref(reply);
	return 0;
}

/* Callback to trigger when transport release completed. */
static void cras_bt_on_transport_release(DBusPendingCall *pending_call,
					 void *data)
{
	DBusMessage *reply;

	reply = dbus_pending_call_steal_reply(pending_call);
	dbus_pending_call_unref(pending_call);

	if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
		syslog(LOG_WARNING, "Release transport returned error: %s",
		       dbus_message_get_error_name(reply));
		dbus_message_unref(reply);
		return;
	}

	dbus_message_unref(reply);
}

int cras_bt_transport_release(struct cras_bt_transport *transport,
			      unsigned int blocking)
{
	DBusMessage *method_call, *reply;
	DBusPendingCall *pending_call;
	DBusError dbus_error;

	if (transport->fd < 0)
		return 0;

	/* Close the transport on our end no matter whether or not the server
	 * gives us an error.
	 */
	close(transport->fd);
	transport->fd = -1;

	method_call = dbus_message_new_method_call(
		BLUEZ_SERVICE,
		transport->object_path,
		BLUEZ_INTERFACE_MEDIA_TRANSPORT,
		"Release");
	if (!method_call)
		return -ENOMEM;

	if (blocking) {
		dbus_error_init(&dbus_error);

		reply = dbus_connection_send_with_reply_and_block(
			transport->conn,
			method_call,
			DBUS_TIMEOUT_USE_DEFAULT,
			&dbus_error);
		if (!reply) {
			syslog(LOG_ERR, "Failed to release transport %s: %s",
			       transport->object_path, dbus_error.message);
			dbus_error_free(&dbus_error);
			dbus_message_unref(method_call);
			return -EIO;
		}

		dbus_message_unref(method_call);

		if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
			syslog(LOG_ERR, "Release returned error: %s",
			       dbus_message_get_error_name(reply));
			dbus_message_unref(reply);
			return -EIO;
		}

		dbus_message_unref(reply);
	} else {
		if (!dbus_connection_send_with_reply(
				transport->conn,
				method_call,
				&pending_call,
				DBUS_TIMEOUT_USE_DEFAULT)) {
			dbus_message_unref(method_call);
			return -ENOMEM;
		}

		dbus_message_unref(method_call);
		if (!pending_call)
			return -EIO;

		if (!dbus_pending_call_set_notify(pending_call,
						  cras_bt_on_transport_release,
						  transport, NULL)) {
			dbus_pending_call_cancel(pending_call);
			dbus_pending_call_unref(pending_call);
			return -ENOMEM;
		}
	}
	return 0;
}
