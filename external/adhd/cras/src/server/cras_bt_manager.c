/* Copyright (c) 2013 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <dbus/dbus.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include "cras_bt_constants.h"
#include "cras_bt_manager.h"
#include "cras_bt_adapter.h"
#include "cras_bt_device.h"
#include "cras_bt_endpoint.h"
#include "cras_bt_player.h"
#include "cras_bt_profile.h"
#include "cras_bt_transport.h"
#include "utlist.h"


static void cras_bt_interface_added(DBusConnection *conn,
				    const char *object_path,
				    const char *interface_name,
				    DBusMessageIter *properties_array_iter)
{
	if (strcmp(interface_name, BLUEZ_INTERFACE_ADAPTER) == 0) {
		struct cras_bt_adapter *adapter;

		adapter = cras_bt_adapter_get(object_path);
		if (adapter) {
			cras_bt_adapter_update_properties(
				adapter, properties_array_iter, NULL);
		} else {
			adapter = cras_bt_adapter_create(object_path);
			if (adapter) {
				cras_bt_adapter_update_properties(
					adapter, properties_array_iter, NULL);
				cras_bt_register_endpoints(conn, adapter);
				cras_bt_register_player(conn, adapter);
				cras_bt_register_profiles(conn);

				syslog(LOG_INFO, "Bluetooth Adapter: %s added",
				       cras_bt_adapter_address(adapter));
			} else {
				syslog(LOG_WARNING,
				       "Failed to create Bluetooth Adapter: %s",
				       object_path);
			}
		}

	} else if (strcmp(interface_name, BLUEZ_INTERFACE_DEVICE) == 0) {
		struct cras_bt_device *device;

		device = cras_bt_device_get(object_path);
		if (device) {
			cras_bt_device_update_properties(
				device, properties_array_iter, NULL);
		} else {
			device = cras_bt_device_create(conn, object_path);
			if (device) {
				cras_bt_device_update_properties(
					device, properties_array_iter, NULL);

				syslog(LOG_INFO, "Bluetooth Device: %s added",
				       cras_bt_device_address(device));
			} else {
				syslog(LOG_WARNING,
				       "Failed to create Bluetooth Device: %s",
				       object_path);
			}
		}

	} else if (strcmp(interface_name,
			  BLUEZ_INTERFACE_MEDIA_TRANSPORT) == 0) {
		struct cras_bt_transport *transport;

		transport = cras_bt_transport_get(object_path);
		if (transport) {
			cras_bt_transport_update_properties(
				transport, properties_array_iter, NULL);
		} else {
			transport = cras_bt_transport_create(conn, object_path);
			if (transport) {
				cras_bt_transport_update_properties(
					transport, properties_array_iter, NULL);

				syslog(LOG_INFO,
				      "Bluetooth Transport: %s added",
				      cras_bt_transport_object_path(transport));
			} else {
				syslog(LOG_WARNING,
				       "Failed to create Bluetooth Transport: "
				       "%s",
				       object_path);
			}
		}

	}
}

static void cras_bt_interface_removed(DBusConnection *conn,
				      const char *object_path,
				      const char *interface_name)
{
	if (strcmp(interface_name, BLUEZ_INTERFACE_ADAPTER) == 0) {
		struct cras_bt_adapter *adapter;

		adapter = cras_bt_adapter_get(object_path);
		if (adapter) {
			syslog(LOG_INFO, "Bluetooth Adapter: %s removed",
			       cras_bt_adapter_address(adapter));
			cras_bt_adapter_destroy(adapter);
		}

	} else if (strcmp(interface_name, BLUEZ_INTERFACE_DEVICE) == 0) {
		struct cras_bt_device *device;

		device = cras_bt_device_get(object_path);
		if (device) {
			syslog(LOG_INFO, "Bluetooth Device: %s removed",
			       cras_bt_device_address(device));
			cras_bt_device_destroy(device);
		}

	} else if (strcmp(interface_name,
			  BLUEZ_INTERFACE_MEDIA_TRANSPORT) == 0) {
		struct cras_bt_transport *transport;

		transport = cras_bt_transport_get(object_path);
		if (transport) {
			syslog(LOG_INFO, "Bluetooth Transport: %s removed",
			       cras_bt_transport_object_path(transport));
			cras_bt_transport_destroy(transport);
		}

	}
}

static void cras_bt_update_properties(DBusConnection *conn,
				      const char *object_path,
				      const char *interface_name,
				      DBusMessageIter *properties_array_iter,
				      DBusMessageIter *invalidated_array_iter)
{
	if (strcmp(interface_name, BLUEZ_INTERFACE_ADAPTER) == 0) {
		struct cras_bt_adapter *adapter;

		adapter = cras_bt_adapter_get(object_path);
		if (adapter) {
			cras_bt_adapter_update_properties(
				adapter, properties_array_iter,
				invalidated_array_iter);
		}

	} else if (strcmp(interface_name, BLUEZ_INTERFACE_DEVICE) == 0) {
		struct cras_bt_device *device;

		device = cras_bt_device_get(object_path);
		if (device) {
			cras_bt_device_update_properties(
				device, properties_array_iter,
				invalidated_array_iter);
		}

	} else if (strcmp(interface_name,
			  BLUEZ_INTERFACE_MEDIA_TRANSPORT) == 0) {
		struct cras_bt_transport *transport;

		transport = cras_bt_transport_get(object_path);
		if (transport) {
			cras_bt_transport_update_properties(
				transport, properties_array_iter,
				invalidated_array_iter);
		}

	}
}

/* Destroys all bt related stuff. The reset functions must be called in
 * reverse order of the adapter -> device -> profile(s) hierarchy.
 */
static void cras_bt_reset()
{
	cras_bt_endpoint_reset();
	cras_bt_transport_reset();
	cras_bt_profile_reset();
	cras_bt_device_reset();
	cras_bt_adapter_reset();
}


static void cras_bt_on_get_managed_objects(DBusPendingCall *pending_call,
					   void *data)
{
	DBusConnection *conn = (DBusConnection *)data;
	DBusMessage *reply;
	DBusMessageIter message_iter, object_array_iter;

	reply = dbus_pending_call_steal_reply(pending_call);
	dbus_pending_call_unref(pending_call);

	if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
		syslog(LOG_WARNING, "GetManagedObjects returned error: %s",
		       dbus_message_get_error_name(reply));
		dbus_message_unref(reply);
		return;
	}

	if (!dbus_message_has_signature(reply, "a{oa{sa{sv}}}")) {
		syslog(LOG_WARNING, "Bad GetManagedObjects reply received");
		dbus_message_unref(reply);
		return;
	}

	dbus_message_iter_init(reply, &message_iter);
	dbus_message_iter_recurse(&message_iter, &object_array_iter);

	while (dbus_message_iter_get_arg_type(&object_array_iter) !=
		       DBUS_TYPE_INVALID) {
		DBusMessageIter object_dict_iter, interface_array_iter;
		const char *object_path;

		dbus_message_iter_recurse(&object_array_iter,
					  &object_dict_iter);

		dbus_message_iter_get_basic(&object_dict_iter, &object_path);
		dbus_message_iter_next(&object_dict_iter);

		dbus_message_iter_recurse(&object_dict_iter,
					  &interface_array_iter);

		while (dbus_message_iter_get_arg_type(&interface_array_iter) !=
			       DBUS_TYPE_INVALID) {
			DBusMessageIter interface_dict_iter;
			DBusMessageIter properties_array_iter;
			const char *interface_name;

			dbus_message_iter_recurse(&interface_array_iter,
						  &interface_dict_iter);

			dbus_message_iter_get_basic(&interface_dict_iter,
						    &interface_name);
			dbus_message_iter_next(&interface_dict_iter);

			dbus_message_iter_recurse(&interface_dict_iter,
						  &properties_array_iter);

			cras_bt_interface_added(conn,
						object_path, interface_name,
						&properties_array_iter);

			dbus_message_iter_next(&interface_array_iter);
		}

		dbus_message_iter_next(&object_array_iter);
	}

	dbus_message_unref(reply);
}

static int cras_bt_get_managed_objects(DBusConnection *conn)
{
	DBusMessage *method_call;
	DBusPendingCall *pending_call;

	method_call = dbus_message_new_method_call(
		BLUEZ_SERVICE,
		"/",
		DBUS_INTERFACE_OBJECT_MANAGER,
		"GetManagedObjects");
	if (!method_call)
		return -ENOMEM;

	pending_call = NULL;
	if (!dbus_connection_send_with_reply(conn, method_call,
					     &pending_call,
					     DBUS_TIMEOUT_USE_DEFAULT)) {
		dbus_message_unref(method_call);
		return -ENOMEM;
	}

	dbus_message_unref(method_call);
	if (!pending_call)
		return -EIO;

	if (!dbus_pending_call_set_notify(pending_call,
					  cras_bt_on_get_managed_objects,
					  conn, NULL)) {
		dbus_pending_call_cancel(pending_call);
		dbus_pending_call_unref(pending_call);
		return -ENOMEM;
	}

	return 0;
}


static DBusHandlerResult cras_bt_handle_name_owner_changed(DBusConnection *conn,
							   DBusMessage *message,
							   void *arg)
{
	DBusError dbus_error;
	const char *service_name, *old_owner, *new_owner;

	if (!dbus_message_is_signal(message, DBUS_INTERFACE_DBUS,
		    "NameOwnerChanged"))
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	dbus_error_init(&dbus_error);
	if (!dbus_message_get_args(message, &dbus_error,
				   DBUS_TYPE_STRING, &service_name,
				   DBUS_TYPE_STRING, &old_owner,
				   DBUS_TYPE_STRING, &new_owner,
				   DBUS_TYPE_INVALID)) {
		syslog(LOG_WARNING, "Bad NameOwnerChanged signal received: %s",
		       dbus_error.message);
		dbus_error_free(&dbus_error);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	syslog(LOG_INFO, "Bluetooth daemon disconnected from the bus.");
	cras_bt_reset();

	if (strlen(new_owner) > 0)
		cras_bt_get_managed_objects(conn);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult cras_bt_handle_interfaces_added(DBusConnection *conn,
							 DBusMessage *message,
							 void *arg)
{
	DBusMessageIter message_iter, interface_array_iter;
	const char *object_path;

	if (!dbus_message_is_signal(message, DBUS_INTERFACE_OBJECT_MANAGER,
			    "InterfacesAdded"))
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	if (!dbus_message_has_signature(message, "oa{sa{sv}}")) {
		syslog(LOG_WARNING, "Bad InterfacesAdded signal received");
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	dbus_message_iter_init(message, &message_iter);

	dbus_message_iter_get_basic(&message_iter, &object_path);
	dbus_message_iter_next(&message_iter);

	dbus_message_iter_recurse(&message_iter, &interface_array_iter);

	while (dbus_message_iter_get_arg_type(&interface_array_iter) !=
		       DBUS_TYPE_INVALID) {
		DBusMessageIter interface_dict_iter, properties_array_iter;
		const char *interface_name;

		dbus_message_iter_recurse(&interface_array_iter,
					  &interface_dict_iter);

		dbus_message_iter_get_basic(&interface_dict_iter,
					    &interface_name);
		dbus_message_iter_next(&interface_dict_iter);

		dbus_message_iter_recurse(&interface_dict_iter,
					  &properties_array_iter);

		cras_bt_interface_added(conn, object_path, interface_name,
					&properties_array_iter);

		dbus_message_iter_next(&interface_array_iter);
	}

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult cras_bt_handle_interfaces_removed(DBusConnection *conn,
							   DBusMessage *message,
							   void *arg)
{
	DBusMessageIter message_iter, interface_array_iter;
	const char *object_path;

	if (!dbus_message_is_signal(message, DBUS_INTERFACE_OBJECT_MANAGER,
			    "InterfacesRemoved"))
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	if (!dbus_message_has_signature(message, "oas")) {
		syslog(LOG_WARNING, "Bad InterfacesRemoved signal received");
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	dbus_message_iter_init(message, &message_iter);

	dbus_message_iter_get_basic(&message_iter, &object_path);
	dbus_message_iter_next(&message_iter);

	dbus_message_iter_recurse(&message_iter, &interface_array_iter);

	while (dbus_message_iter_get_arg_type(&interface_array_iter) !=
	       DBUS_TYPE_INVALID) {
		const char *interface_name;

		dbus_message_iter_get_basic(&interface_array_iter,
					    &interface_name);

		cras_bt_interface_removed(conn, object_path, interface_name);

		dbus_message_iter_next(&interface_array_iter);
	}

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult cras_bt_handle_properties_changed(DBusConnection *conn,
							   DBusMessage *message,
							   void *arg)
{
	DBusMessageIter message_iter, properties_array_iter;
	DBusMessageIter invalidated_array_iter;
	const char *object_path, *interface_name;

	if (!dbus_message_is_signal(message, DBUS_INTERFACE_PROPERTIES,
				    "PropertiesChanged"))
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	if (!dbus_message_has_signature(message, "sa{sv}as")) {
		syslog(LOG_WARNING, "Bad PropertiesChanged signal received");
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	object_path = dbus_message_get_path(message);

	dbus_message_iter_init(message, &message_iter);

	dbus_message_iter_get_basic(&message_iter, &interface_name);
	dbus_message_iter_next(&message_iter);

	dbus_message_iter_recurse(&message_iter, &properties_array_iter);
	dbus_message_iter_next(&message_iter);

	dbus_message_iter_recurse(&message_iter, &invalidated_array_iter);

	cras_bt_update_properties(conn, object_path, interface_name,
				  &properties_array_iter,
				  &invalidated_array_iter);

	return DBUS_HANDLER_RESULT_HANDLED;
}


void cras_bt_start(DBusConnection *conn)
{
	DBusError dbus_error;

	dbus_error_init(&dbus_error);

	/* Inform the bus daemon which signals we wish to receive. */
	dbus_bus_add_match(conn,
			   "type='signal',"
			   "sender='" DBUS_SERVICE_DBUS "',"
			   "interface='" DBUS_INTERFACE_DBUS "',"
			   "member='NameOwnerChanged',"
			   "arg0='" BLUEZ_SERVICE "'",
			   &dbus_error);
	if (dbus_error_is_set(&dbus_error))
		goto add_match_error;

	dbus_bus_add_match(conn,
			   "type='signal',"
			   "sender='" BLUEZ_SERVICE "',"
			   "interface='" DBUS_INTERFACE_OBJECT_MANAGER "',"
			   "member='InterfacesAdded'",
			   &dbus_error);
	if (dbus_error_is_set(&dbus_error))
		goto add_match_error;

	dbus_bus_add_match(conn,
			   "type='signal',"
			   "sender='" BLUEZ_SERVICE "',"
			   "interface='" DBUS_INTERFACE_OBJECT_MANAGER "',"
			   "member='InterfacesRemoved'",
			   &dbus_error);
	if (dbus_error_is_set(&dbus_error))
		goto add_match_error;

	dbus_bus_add_match(conn,
			   "type='signal',"
			   "sender='" BLUEZ_SERVICE "',"
			   "interface='" DBUS_INTERFACE_PROPERTIES "',"
			   "member='PropertiesChanged'",
			   &dbus_error);
	if (dbus_error_is_set(&dbus_error))
		goto add_match_error;

	/* Install filter functions to handle the signals we receive. */
	if (!dbus_connection_add_filter(conn, cras_bt_handle_name_owner_changed,
					NULL, NULL))
		goto add_filter_error;

	if (!dbus_connection_add_filter(conn, cras_bt_handle_interfaces_added,
					NULL, NULL))
		goto add_filter_error;

	if (!dbus_connection_add_filter(conn, cras_bt_handle_interfaces_removed,
					NULL, NULL))
		goto add_filter_error;

	if (!dbus_connection_add_filter(conn, cras_bt_handle_properties_changed,
					NULL, NULL))
		goto add_filter_error;

	cras_bt_get_managed_objects(conn);
	return;

add_match_error:
	syslog(LOG_WARNING, "Couldn't setup Bluetooth device monitoring: %s",
	       dbus_error.message);
	dbus_error_free(&dbus_error);
	cras_bt_stop(conn);
	return;

add_filter_error:
	syslog(LOG_WARNING, "Couldn't setup Bluetooth device monitoring: %s",
	       strerror(ENOMEM));
	cras_bt_stop(conn);
	return;
}

void cras_bt_stop(DBusConnection *conn)
{
	cras_bt_reset();

	dbus_bus_remove_match(conn,
			      "type='signal',"
			      "sender='" DBUS_SERVICE_DBUS "',"
			      "interface='" DBUS_INTERFACE_DBUS "',"
			      "member='NameOwnerChanged',"
			      "arg0='" BLUEZ_SERVICE "'",
			      NULL);
	dbus_bus_remove_match(conn,
			      "type='signal',"
			      "sender='" BLUEZ_SERVICE "',"
			      "interface='" DBUS_INTERFACE_OBJECT_MANAGER "',"
			      "member='InterfacesAdded'",
			      NULL);
	dbus_bus_remove_match(conn,
			      "type='signal',"
			      "sender='" BLUEZ_SERVICE "',"
			      "interface='" DBUS_INTERFACE_OBJECT_MANAGER "',"
			      "member='InterfacesRemoved'",
			      NULL);
	dbus_bus_remove_match(conn,
			      "type='signal',"
			      "sender='" BLUEZ_SERVICE "',"
			      "interface='" DBUS_INTERFACE_PROPERTIES "',"
			      "member='PropertiesChanged'",
			      NULL);

	dbus_connection_remove_filter(conn, cras_bt_handle_name_owner_changed,
				      NULL);
	dbus_connection_remove_filter(conn, cras_bt_handle_interfaces_added,
				      NULL);
	dbus_connection_remove_filter(conn, cras_bt_handle_interfaces_removed,
				      NULL);
	dbus_connection_remove_filter(conn, cras_bt_handle_properties_changed,
				      NULL);
}
