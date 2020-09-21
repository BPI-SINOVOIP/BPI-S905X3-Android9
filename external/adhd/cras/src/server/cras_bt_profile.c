/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <dbus/dbus.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "cras_bt_constants.h"
#include "cras_bt_device.h"
#include "cras_bt_profile.h"
#include "cras_dbus_util.h"
#include "utlist.h"

#define PROFILE_INTROSPECT_XML						\
	DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE			\
	"<node>\n"							\
	"  <interface name=\"org.bluez.Profile1\">\n"			\
	"    <method name=\"Release\">\n"				\
	"    </method>\n"						\
	"    <method name=\"NewConnection\">\n"				\
	"      <arg name=\"device\" type=\"o\" direction=\"in\">\n"	\
	"      <arg name=\"fd\" type=\"h\" direction=\"in\">\n"		\
	"      <arg name=\"fd_properties\" type=\"a{sv}\" direction=\"in\">\n"\
	"    </method>\n"						\
	"    <method name=\"RequestDisconnection\">\n"			\
	"      <arg name=\"device\" type=\"o\" direction=\"in\">\n"	\
	"    </method>\n"						\
	"    <method name=\"Cancel\">\n"				\
	"    </method>\n"						\
	"  <interface name=\"" DBUS_INTERFACE_INTROSPECTABLE "\">\n"	\
	"    <method name=\"Introspect\">\n"				\
	"      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"	\
	"    </method>\n"						\
	"  </interface>\n"						\
	"</node>\n"


/* Profiles */
static struct cras_bt_profile *profiles;

static DBusHandlerResult cras_bt_profile_handle_release(
		DBusConnection *conn,
		DBusMessage *message,
		void *arg)
{
	DBusMessage *reply;
	const char *profile_path;
	struct cras_bt_profile *profile;

	profile_path = dbus_message_get_path(message);

	profile = cras_bt_profile_get(profile_path);
	if (!profile)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	profile->release(profile);

	reply = dbus_message_new_method_return(message);
	if (!reply)
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	if (!dbus_connection_send(conn, reply, NULL)) {
		dbus_message_unref(reply);
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	}

	dbus_message_unref(reply);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult cras_bt_profile_handle_new_connection(
		DBusConnection *conn,
		DBusMessage *message,
		void *arg)
{
	DBusMessageIter message_iter;
	DBusMessage *reply;
	const char *profile_path, *object_path;
	int fd = -1;
	int err;
	struct cras_bt_profile *profile;
	struct cras_bt_device *device;

	profile_path = dbus_message_get_path(message);

	dbus_message_iter_init(message, &message_iter);
	dbus_message_iter_get_basic(&message_iter, &object_path);
	dbus_message_iter_next(&message_iter);

	if (dbus_message_iter_get_arg_type(&message_iter)
			!= DBUS_TYPE_UNIX_FD) {
		syslog(LOG_ERR, "Argument not a valid unix file descriptor");
		goto invalid;
	}

	dbus_message_iter_get_basic(&message_iter, &fd);
	dbus_message_iter_next(&message_iter);
	if (fd < 0)
		goto invalid;

	profile = cras_bt_profile_get(profile_path);
	if (!profile)
		goto invalid;

	device = cras_bt_device_get(object_path);
	if (!device) {
		syslog(LOG_ERR, "Device %s not found at %s new connection",
		       object_path, profile_path);
		device = cras_bt_device_create(conn, object_path);
	}

	err = profile->new_connection(conn, profile, device, fd);
	if (err) {
		syslog(LOG_INFO, "%s new connection rejected", profile->name);
		close(fd);
		reply = dbus_message_new_error(message,
				"org.chromium.Cras.Error.RejectNewConnection",
				"Possibly another headset already in use");
		if (!dbus_connection_send(conn, reply, NULL))
			return DBUS_HANDLER_RESULT_NEED_MEMORY;

		dbus_message_unref(reply);
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	reply = dbus_message_new_method_return(message);
	if (!reply)
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	if (!dbus_connection_send(conn, reply, NULL)) {
		dbus_message_unref(reply);
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	}

	dbus_message_unref(reply);
	return DBUS_HANDLER_RESULT_HANDLED;

invalid:
	if (fd >= 0)
		close(fd);
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static DBusHandlerResult cras_bt_profile_handle_request_disconnection(
		DBusConnection *conn,
		DBusMessage *message,
		void *arg)
{
	DBusMessageIter message_iter;
	DBusMessage *reply;
	const char *prpofile_path, *object_path;
	struct cras_bt_profile *profile;
	struct cras_bt_device *device;

	prpofile_path = dbus_message_get_path(message);

	dbus_message_iter_init(message, &message_iter);
	dbus_message_iter_get_basic(&message_iter, &object_path);
	dbus_message_iter_next(&message_iter);

	profile = cras_bt_profile_get(prpofile_path);
	if (!profile)
		goto invalid;

	device = cras_bt_device_get(object_path);
	if (!device)
		goto invalid;

	profile->request_disconnection(profile, device);

	reply = dbus_message_new_method_return(message);
	if (!reply)
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	if (!dbus_connection_send(conn, reply, NULL)) {
		dbus_message_unref(reply);
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	}

	dbus_message_unref(reply);

	return DBUS_HANDLER_RESULT_HANDLED;

invalid:
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static DBusHandlerResult cras_bt_profile_handle_cancel(
		DBusConnection *conn,
		DBusMessage *message,
		void *arg)
{
	DBusMessage *reply;
	const char *profile_path;
	struct cras_bt_profile *profile;

	profile_path = dbus_message_get_path(message);

	profile = cras_bt_profile_get(profile_path);
	if (!profile)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	profile->cancel(profile);

	reply = dbus_message_new_method_return(message);
	if (!reply)
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	if (!dbus_connection_send(conn, reply, NULL)) {
		dbus_message_unref(reply);
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	}

	dbus_message_unref(reply);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult cras_bt_handle_profile_messages(DBusConnection *conn,
							DBusMessage *message,
							void *arg)
{
	if (dbus_message_is_method_call(message,
					DBUS_INTERFACE_INTROSPECTABLE,
					"Introspect")) {
		DBusMessage *reply;
		const char *xml = PROFILE_INTROSPECT_XML;

		reply = dbus_message_new_method_return(message);
		if (!reply)
			return DBUS_HANDLER_RESULT_NEED_MEMORY;
		if (!dbus_message_append_args(reply,
					      DBUS_TYPE_STRING,
					      &xml,
					      DBUS_TYPE_INVALID)) {
			dbus_message_unref(reply);
			return DBUS_HANDLER_RESULT_NEED_MEMORY;
		}
		if (!dbus_connection_send(conn, reply, NULL)) {
			dbus_message_unref(reply);
			return DBUS_HANDLER_RESULT_NEED_MEMORY;
		}

		dbus_message_unref(reply);
		return DBUS_HANDLER_RESULT_HANDLED;
	} else if (dbus_message_is_method_call(message,
					       BLUEZ_INTERFACE_PROFILE,
					       "Release")) {
		return cras_bt_profile_handle_release(conn, message, arg);
	} else if (dbus_message_is_method_call(message,
					       BLUEZ_INTERFACE_PROFILE,
					       "NewConnection")) {
		return cras_bt_profile_handle_new_connection(conn, message, arg);
	} else if (dbus_message_is_method_call(message,
					       BLUEZ_INTERFACE_PROFILE,
					       "RequestDisconnection")) {
		return cras_bt_profile_handle_request_disconnection(conn,
								    message,
								    arg);
	} else if (dbus_message_is_method_call(message,
					       BLUEZ_INTERFACE_PROFILE,
					       "Cancel")) {
		return cras_bt_profile_handle_cancel(conn, message, arg);
	} else {
		syslog(LOG_ERR, "Unknown Profile message");
	}

	return DBUS_HANDLER_RESULT_HANDLED;
}

static void cras_bt_on_register_profile(DBusPendingCall *pending_call,
					void *data)
{
	DBusMessage *reply;

	reply = dbus_pending_call_steal_reply(pending_call);
	dbus_pending_call_unref(pending_call);

	if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR)
		syslog(LOG_ERR, "RegisterProfile returned error: %s",
		       dbus_message_get_error_name(reply));
	dbus_message_unref(reply);
}

int cras_bt_register_profile(DBusConnection *conn,
			     struct cras_bt_profile *profile)
{
	DBusMessage *method_call;
	DBusMessageIter message_iter;
	DBusMessageIter properties_array_iter;
	DBusPendingCall *pending_call;

	method_call = dbus_message_new_method_call(BLUEZ_SERVICE,
						   PROFILE_MANAGER_OBJ_PATH,
						   BLUEZ_PROFILE_MGMT_INTERFACE,
						   "RegisterProfile");

	if (!method_call)
		return -ENOMEM;

	dbus_message_iter_init_append(method_call, &message_iter);
	dbus_message_iter_append_basic(&message_iter, DBUS_TYPE_OBJECT_PATH,
				       &profile->object_path);
	dbus_message_iter_append_basic(&message_iter, DBUS_TYPE_STRING,
					       &profile->uuid);

	dbus_message_iter_open_container(&message_iter,
					 DBUS_TYPE_ARRAY,
					 DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
					 DBUS_TYPE_STRING_AS_STRING
					 DBUS_TYPE_VARIANT_AS_STRING
					 DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
					 &properties_array_iter);

	if (!append_key_value(&properties_array_iter, "Name", DBUS_TYPE_STRING,
			      DBUS_TYPE_STRING_AS_STRING, &profile->name)) {
		dbus_message_unref(method_call);
		return -ENOMEM;
	}

	if (profile->record &&
	    !append_key_value(&properties_array_iter, "ServiceRecord",
			      DBUS_TYPE_STRING, DBUS_TYPE_STRING_AS_STRING,
			      &profile->record)) {
		dbus_message_unref(method_call);
		return -ENOMEM;
	}

	if (!append_key_value(&properties_array_iter, "Version",
			      DBUS_TYPE_UINT16,
			      DBUS_TYPE_UINT16_AS_STRING, &profile->version)) {
		dbus_message_unref(method_call);
		return -ENOMEM;
	}

	if (profile->role && !append_key_value(&properties_array_iter, "Role",
					       DBUS_TYPE_STRING,
					       DBUS_TYPE_STRING_AS_STRING,
					       &profile->role)) {
		dbus_message_unref(method_call);
		return -ENOMEM;
	}

	if (profile->features && !append_key_value(&properties_array_iter,
						   "Features",
						   DBUS_TYPE_UINT16,
						   DBUS_TYPE_UINT16_AS_STRING,
						   &profile->features)) {
		dbus_message_unref(method_call);
		return -ENOMEM;
	}

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
					  cras_bt_on_register_profile,
					  NULL, NULL)) {
		dbus_pending_call_cancel(pending_call);
		dbus_pending_call_unref(pending_call);
		syslog(LOG_ERR, "register profile fail on set notify");
		return -ENOMEM;
	}

	return 0;
}

int cras_bt_register_profiles(DBusConnection *conn)
{
	struct cras_bt_profile *profile;
	int err;

	DL_FOREACH(profiles, profile) {
		err = cras_bt_register_profile(conn, profile);
		if (err)
			return err;
	}

	return 0;
}

int cras_bt_add_profile(DBusConnection *conn,
			struct cras_bt_profile *profile)
{
	static const DBusObjectPathVTable profile_vtable = {
		NULL,
		cras_bt_handle_profile_messages,
		NULL, NULL, NULL, NULL
	};

	DBusError dbus_error;

	dbus_error_init(&dbus_error);

	if (!dbus_connection_register_object_path(conn,
						  profile->object_path,
						  &profile_vtable,
						  &dbus_error)) {
		syslog(LOG_ERR, "Could not register BT profile %s: %s",
		       profile->object_path, dbus_error.message);
		dbus_error_free(&dbus_error);
		return -ENOMEM;
	}

	DL_APPEND(profiles, profile);

	return 0;
}

void cras_bt_profile_reset()
{
	struct cras_bt_profile *profile;

	DL_FOREACH(profiles, profile)
		profile->release(profile);
}

struct cras_bt_profile *cras_bt_profile_get(const char *path)
{
	struct cras_bt_profile *profile;
	DL_FOREACH(profiles, profile) {
		if (strcmp(profile->object_path, path) == 0)
			return profile;
	}

	return NULL;
}

void cras_bt_profile_on_device_disconnected(struct cras_bt_device *device)
{
	struct cras_bt_profile *profile;
	DL_FOREACH(profiles, profile)
		profile->request_disconnection(profile, device);
}
