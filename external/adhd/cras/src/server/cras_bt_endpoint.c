/* Copyright (c) 2013 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <dbus/dbus.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include "cras_bt_constants.h"
#include "cras_bt_adapter.h"
#include "cras_bt_endpoint.h"
#include "cras_bt_transport.h"
#include "utlist.h"

/* Defined by doc/media-api.txt in the BlueZ source */
#define ENDPOINT_INTROSPECT_XML						\
	DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE			\
	"<node>\n"							\
	"  <interface name=\"org.bluez.MediaEndpoint\">\n"		\
	"    <method name=\"SetConfiguration\">\n"			\
	"      <arg name=\"transport\" type=\"o\" direction=\"in\"/>\n"	\
	"      <arg name=\"configuration\" type=\"a{sv}\" direction=\"in\"/>\n"\
	"    </method>\n"						\
	"    <method name=\"SelectConfiguration\">\n"			\
	"      <arg name=\"capabilities\" type=\"ay\" direction=\"in\"/>\n"\
	"      <arg name=\"configuration\" type=\"ay\" direction=\"out\"/>\n"\
	"    </method>\n"						\
	"    <method name=\"ClearConfiguration\">\n"			\
	"    </method>\n"						\
	"    <method name=\"Release\">\n"				\
	"    </method>\n"						\
	"  </interface>\n"						\
	"  <interface name=\"" DBUS_INTERFACE_INTROSPECTABLE "\">\n"	\
	"    <method name=\"Introspect\">\n"				\
	"      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"	\
	"    </method>\n"						\
	"  </interface>\n"						\
	"</node>\n"


static void cras_bt_endpoint_suspend(struct cras_bt_endpoint *endpoint)
{
	if (!endpoint->transport)
		return;

	endpoint->suspend(endpoint, endpoint->transport);

	cras_bt_transport_set_endpoint(endpoint->transport, NULL);
	endpoint->transport = NULL;
}

static DBusHandlerResult cras_bt_endpoint_set_configuration(
	DBusConnection *conn,
	DBusMessage *message,
	void *arg)
{
	DBusMessageIter message_iter, properties_array_iter;
	const char *endpoint_path, *transport_path;
	struct cras_bt_endpoint *endpoint;
	struct cras_bt_transport *transport;
	DBusMessage *reply;

	syslog(LOG_DEBUG, "SetConfiguration: %s",
	       dbus_message_get_path(message));

	endpoint_path = dbus_message_get_path(message);
	endpoint = cras_bt_endpoint_get(endpoint_path);
	if (!endpoint)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	if (!dbus_message_has_signature(message, "oa{sv}")) {
		syslog(LOG_WARNING, "Bad SetConfiguration message received.");
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	dbus_message_iter_init(message, &message_iter);

	dbus_message_iter_get_basic(&message_iter, &transport_path);
	dbus_message_iter_next(&message_iter);

	dbus_message_iter_recurse(&message_iter, &properties_array_iter);

	transport = cras_bt_transport_get(transport_path);
	if (transport) {
		cras_bt_transport_update_properties(transport,
						    &properties_array_iter,
						    NULL);
	} else {
		transport = cras_bt_transport_create(conn, transport_path);
		if (transport) {
			cras_bt_transport_update_properties(
				transport,
				&properties_array_iter,
				NULL);
			syslog(LOG_INFO, "Bluetooth Transport: %s added",
			       cras_bt_transport_object_path(transport));
		}
	}

	if (!cras_bt_transport_device(transport)) {
		syslog(LOG_ERR, "Do device found for transport %s",
		       cras_bt_transport_object_path(transport));
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	cras_bt_transport_set_endpoint(transport, endpoint);
	endpoint->transport = transport;
	endpoint->set_configuration(endpoint, transport);

	reply = dbus_message_new_method_return(message);
	if (!reply)
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	if (!dbus_connection_send(conn, reply, NULL))
		return DBUS_HANDLER_RESULT_NEED_MEMORY;

	dbus_message_unref(reply);
	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult cras_bt_endpoint_select_configuration(
	DBusConnection *conn,
	DBusMessage *message,
	void *arg)
{
	DBusError dbus_error;
	const char *endpoint_path;
	struct cras_bt_endpoint *endpoint;
	char buf[4];
	void *capabilities, *configuration = buf;
	int len;
	DBusMessage *reply;

	syslog(LOG_DEBUG, "SelectConfiguration: %s",
	       dbus_message_get_path(message));

	endpoint_path = dbus_message_get_path(message);
	endpoint = cras_bt_endpoint_get(endpoint_path);
	if (!endpoint)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	dbus_error_init(&dbus_error);

	if (!dbus_message_get_args(message, &dbus_error,
				   DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
				   &capabilities, &len,
				   DBUS_TYPE_INVALID)) {
		syslog(LOG_WARNING, "Bad SelectConfiguration method call: %s",
		       dbus_error.message);
		dbus_error_free(&dbus_error);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	if (len > sizeof(configuration) ||
	    endpoint->select_configuration(endpoint, capabilities, len,
					   configuration) < 0) {
		reply = dbus_message_new_error(
			message,
			"org.chromium.Cras.Error.UnsupportedConfiguration",
			"Unable to select configuration from capabilities");

		if (!dbus_connection_send(conn, reply, NULL))
			return DBUS_HANDLER_RESULT_NEED_MEMORY;

		dbus_message_unref(reply);
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	reply = dbus_message_new_method_return(message);
	if (!reply)
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	if (!dbus_message_append_args(reply,
				      DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
				      &configuration, len,
				      DBUS_TYPE_INVALID))
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	if (!dbus_connection_send(conn, reply, NULL))
		return DBUS_HANDLER_RESULT_NEED_MEMORY;

	dbus_message_unref(reply);
	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult cras_bt_endpoint_clear_configuration(
	DBusConnection *conn,
	DBusMessage *message,
	void *arg)
{
	DBusError dbus_error;
	const char *endpoint_path, *transport_path;
	struct cras_bt_endpoint *endpoint;
	struct cras_bt_transport *transport;
	DBusMessage *reply;

	syslog(LOG_DEBUG, "ClearConfiguration: %s",
	       dbus_message_get_path(message));

	endpoint_path = dbus_message_get_path(message);
	endpoint = cras_bt_endpoint_get(endpoint_path);
	if (!endpoint)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	dbus_error_init(&dbus_error);

	if (!dbus_message_get_args(message, &dbus_error,
				   DBUS_TYPE_OBJECT_PATH, &transport_path,
				   DBUS_TYPE_INVALID)) {
		syslog(LOG_WARNING, "Bad ClearConfiguration method call: %s",
		       dbus_error.message);
		dbus_error_free(&dbus_error);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	transport = cras_bt_transport_get(transport_path);

	if (transport == endpoint->transport)
		cras_bt_endpoint_suspend(endpoint);

	reply = dbus_message_new_method_return(message);
	if (!reply)
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	if (!dbus_connection_send(conn, reply, NULL))
		return DBUS_HANDLER_RESULT_NEED_MEMORY;

	dbus_message_unref(reply);
	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult cras_bt_endpoint_release(DBusConnection *conn,
						  DBusMessage *message,
						  void *arg)
{
	const char *endpoint_path;
	struct cras_bt_endpoint *endpoint;
	DBusMessage *reply;

	syslog(LOG_DEBUG, "Release: %s",
	       dbus_message_get_path(message));

	endpoint_path = dbus_message_get_path(message);
	endpoint = cras_bt_endpoint_get(endpoint_path);
	if (!endpoint)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	cras_bt_endpoint_suspend(endpoint);

	reply = dbus_message_new_method_return(message);
	if (!reply)
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	if (!dbus_connection_send(conn, reply, NULL))
		return DBUS_HANDLER_RESULT_NEED_MEMORY;

	dbus_message_unref(reply);
	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult cras_bt_handle_endpoint_message(DBusConnection *conn,
							 DBusMessage *message,
							 void *arg)
{
	syslog(LOG_DEBUG, "Endpoint message: %s %s %s",
	       dbus_message_get_path(message),
	       dbus_message_get_interface(message),
	       dbus_message_get_member(message));

	if (dbus_message_is_method_call(message,
					DBUS_INTERFACE_INTROSPECTABLE,
					"Introspect")) {
		DBusMessage *reply;
		const char *xml = ENDPOINT_INTROSPECT_XML;

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
					       BLUEZ_INTERFACE_MEDIA_ENDPOINT,
					       "SetConfiguration")) {
		return cras_bt_endpoint_set_configuration(conn, message, arg);

	} else if (dbus_message_is_method_call(message,
					       BLUEZ_INTERFACE_MEDIA_ENDPOINT,
					       "SelectConfiguration")) {
		return cras_bt_endpoint_select_configuration(
			conn, message, arg);

	} else if (dbus_message_is_method_call(message,
					       BLUEZ_INTERFACE_MEDIA_ENDPOINT,
					       "ClearConfiguration")) {
		return cras_bt_endpoint_clear_configuration(conn, message, arg);

	} else if (dbus_message_is_method_call(message,
					       BLUEZ_INTERFACE_MEDIA_ENDPOINT,
					       "Release")) {
		return cras_bt_endpoint_release(conn, message, arg);

	} else {
		syslog(LOG_DEBUG, "%s: %s.%s: Unknown MediaEndpoint message",
		       dbus_message_get_path(message),
		       dbus_message_get_interface(message),
		       dbus_message_get_member(message));
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}
}


static void cras_bt_on_register_endpoint(DBusPendingCall *pending_call,
					 void *data)
{
	DBusMessage *reply;

	reply = dbus_pending_call_steal_reply(pending_call);
	dbus_pending_call_unref(pending_call);

	if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
		syslog(LOG_WARNING, "RegisterEndpoint returned error: %s",
		       dbus_message_get_error_name(reply));
		dbus_message_unref(reply);
		return;
	}

	dbus_message_unref(reply);
}

int cras_bt_register_endpoint(DBusConnection *conn,
			      const struct cras_bt_adapter *adapter,
			      struct cras_bt_endpoint *endpoint)
{
	const char *adapter_path, *key;
	DBusMessage *method_call;
	DBusMessageIter message_iter;
	DBusMessageIter properties_array_iter, properties_dict_iter;
	DBusMessageIter variant_iter, bytes_iter;
	DBusPendingCall *pending_call;
	char buf[4];
	void *capabilities = buf;
	int len = sizeof(buf);
	int error;

	error = endpoint->get_capabilities(endpoint, capabilities, &len);
	if (error < 0)
		return error;

	adapter_path = cras_bt_adapter_object_path(adapter);

	method_call = dbus_message_new_method_call(BLUEZ_SERVICE,
						   adapter_path,
						   BLUEZ_INTERFACE_MEDIA,
						   "RegisterEndpoint");
	if (!method_call)
		return -ENOMEM;

	dbus_message_iter_init_append(method_call, &message_iter);
	dbus_message_iter_append_basic(&message_iter,
				       DBUS_TYPE_OBJECT_PATH,
				       &endpoint->object_path);

	dbus_message_iter_open_container(&message_iter,
					 DBUS_TYPE_ARRAY,
					 DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
					 DBUS_TYPE_STRING_AS_STRING
					 DBUS_TYPE_VARIANT_AS_STRING
					 DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
					 &properties_array_iter);

	key = "UUID";
	dbus_message_iter_open_container(&properties_array_iter,
					 DBUS_TYPE_DICT_ENTRY, NULL,
					 &properties_dict_iter);
	dbus_message_iter_append_basic(&properties_dict_iter,
				       DBUS_TYPE_STRING, &key);
	dbus_message_iter_open_container(&properties_dict_iter,
					 DBUS_TYPE_VARIANT,
					 DBUS_TYPE_STRING_AS_STRING,
					 &variant_iter);
	dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING,
				       &endpoint->uuid);
	dbus_message_iter_close_container(&properties_dict_iter, &variant_iter);
	dbus_message_iter_close_container(&properties_array_iter,
					  &properties_dict_iter);

	key = "Codec";
	dbus_message_iter_open_container(&properties_array_iter,
					 DBUS_TYPE_DICT_ENTRY, NULL,
					 &properties_dict_iter);
	dbus_message_iter_append_basic(&properties_dict_iter,
				       DBUS_TYPE_STRING, &key);
	dbus_message_iter_open_container(&properties_dict_iter,
					 DBUS_TYPE_VARIANT,
					 DBUS_TYPE_BYTE_AS_STRING,
					 &variant_iter);
	dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_BYTE,
				       &endpoint->codec);
	dbus_message_iter_close_container(&properties_dict_iter, &variant_iter);
	dbus_message_iter_close_container(&properties_array_iter,
					  &properties_dict_iter);

	key = "Capabilities";
	dbus_message_iter_open_container(&properties_array_iter,
					 DBUS_TYPE_DICT_ENTRY, NULL,
					 &properties_dict_iter);
	dbus_message_iter_append_basic(&properties_dict_iter,
				       DBUS_TYPE_STRING, &key);
	dbus_message_iter_open_container(&properties_dict_iter,
					 DBUS_TYPE_VARIANT,
					 DBUS_TYPE_ARRAY_AS_STRING
					 DBUS_TYPE_BYTE_AS_STRING,
					 &variant_iter);
	dbus_message_iter_open_container(&variant_iter, DBUS_TYPE_ARRAY,
					 DBUS_TYPE_BYTE_AS_STRING,
					 &bytes_iter);
	dbus_message_iter_append_fixed_array(&bytes_iter, DBUS_TYPE_BYTE,
					     &capabilities, len);
	dbus_message_iter_close_container(&variant_iter, &bytes_iter);
	dbus_message_iter_close_container(&properties_dict_iter, &variant_iter);
	dbus_message_iter_close_container(&properties_array_iter,
					  &properties_dict_iter);

	dbus_message_iter_close_container(&message_iter,
					  &properties_array_iter);

	if (!dbus_connection_send_with_reply(conn, method_call, &pending_call,
					     DBUS_TIMEOUT_USE_DEFAULT)) {
		dbus_message_unref(method_call);
		return -ENOMEM;
	}

	dbus_message_unref(method_call);
	if (!pending_call)
		return -EIO;

	if (!dbus_pending_call_set_notify(pending_call,
					  cras_bt_on_register_endpoint,
					  NULL, NULL)) {
		dbus_pending_call_cancel(pending_call);
		dbus_pending_call_unref(pending_call);
		return -ENOMEM;
	}

	return 0;
}

static void cras_bt_on_unregister_endpoint(DBusPendingCall *pending_call,
					   void *data)
{
	DBusMessage *reply;

	reply = dbus_pending_call_steal_reply(pending_call);
	dbus_pending_call_unref(pending_call);

	if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
		syslog(LOG_WARNING, "UnregisterEndpoint returned error: %s",
		       dbus_message_get_error_name(reply));
		dbus_message_unref(reply);
		return;
	}

	dbus_message_unref(reply);
}

int cras_bt_unregister_endpoint(DBusConnection *conn,
				const struct cras_bt_adapter *adapter,
				struct cras_bt_endpoint *endpoint)
{
	const char *adapter_path;
	DBusMessage *method_call;
	DBusPendingCall *pending_call;

	adapter_path = cras_bt_adapter_object_path(adapter);

	method_call = dbus_message_new_method_call(BLUEZ_SERVICE,
						   adapter_path,
						   BLUEZ_INTERFACE_MEDIA,
						   "UnregisterEndpoint");
	if (!method_call)
		return -ENOMEM;

	if (!dbus_message_append_args(method_call,
				      DBUS_TYPE_OBJECT_PATH,
				      &endpoint->object_path,
				      DBUS_TYPE_INVALID))
		return -ENOMEM;

	if (!dbus_connection_send_with_reply(conn, method_call, &pending_call,
					     DBUS_TIMEOUT_USE_DEFAULT)) {
		dbus_message_unref(method_call);
		return -ENOMEM;
	}

	dbus_message_unref(method_call);
	if (!pending_call)
		return -EIO;

	if (!dbus_pending_call_set_notify(pending_call,
					  cras_bt_on_unregister_endpoint,
					  NULL, NULL)) {
		dbus_pending_call_cancel(pending_call);
		dbus_pending_call_unref(pending_call);
		return -ENOMEM;
	}

	return 0;
}


/* Available endpoints */
static struct cras_bt_endpoint *endpoints;

int cras_bt_register_endpoints(DBusConnection *conn,
			       const struct cras_bt_adapter *adapter)
{
	struct cras_bt_endpoint *endpoint;

	DL_FOREACH(endpoints, endpoint)
		cras_bt_register_endpoint(conn, adapter, endpoint);

	return 0;
}

int cras_bt_endpoint_add(DBusConnection *conn,
			 struct cras_bt_endpoint *endpoint)
{
	static const DBusObjectPathVTable endpoint_vtable = {
		.message_function = cras_bt_handle_endpoint_message
	};

	DBusError dbus_error;
	struct cras_bt_adapter **adapters;
	size_t num_adapters, i;

	DL_APPEND(endpoints, endpoint);

	dbus_error_init(&dbus_error);

	if (!dbus_connection_register_object_path(conn,
						  endpoint->object_path,
						  &endpoint_vtable,
						  &dbus_error)) {
		syslog(LOG_WARNING,
		       "Couldn't register Bluetooth endpoint: %s: %s",
		       endpoint->object_path, dbus_error.message);
		dbus_error_free(&dbus_error);
		return -ENOMEM;
	}

	num_adapters = cras_bt_adapter_get_list(&adapters);
	for (i = 0; i < num_adapters; ++i)
		cras_bt_register_endpoint(conn, adapters[i], endpoint);
	free(adapters);

	return 0;
}

void cras_bt_endpoint_rm(DBusConnection *conn,
			 struct cras_bt_endpoint *endpoint)
{
	struct cras_bt_adapter **adapters;
	size_t num_adapters, i;

	num_adapters = cras_bt_adapter_get_list(&adapters);
	for (i = 0; i < num_adapters; ++i)
		cras_bt_unregister_endpoint(conn, adapters[i], endpoint);
	free(adapters);

	dbus_connection_unregister_object_path(conn, endpoint->object_path);

	DL_DELETE(endpoints, endpoint);
}

void cras_bt_endpoint_reset()
{
	struct cras_bt_endpoint *endpoint;

	DL_FOREACH(endpoints, endpoint)
		cras_bt_endpoint_suspend(endpoint);
}

struct cras_bt_endpoint *cras_bt_endpoint_get(const char *object_path)
{
	struct cras_bt_endpoint *endpoint;

	DL_FOREACH(endpoints, endpoint) {
		if (strcmp(endpoint->object_path, object_path) == 0)
			return endpoint;
	}

	return NULL;
}
