/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>
#include <stdlib.h>
#include <syslog.h>

#include <dbus/dbus.h>

#include "cras_telephony.h"
#include "cras_hfp_ag_profile.h"
#include "cras_hfp_slc.h"

#define CRAS_TELEPHONY_INTERFACE "org.chromium.cras.Telephony"
#define CRAS_TELEPHONY_OBJECT_PATH "/org/chromium/cras/telephony"
#define TELEPHONY_INTROSPECT_XML					\
	DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE			\
	"<node>\n"							\
	"  <interface name=\"" CRAS_TELEPHONY_INTERFACE "\">\n"		\
	"    <method name=\"AnswerCall\">\n"				\
	"    </method>\n"						\
	"    <method name=\"IncomingCall\">\n"				\
	"      <arg name=\"value\" type=\"s\" direction=\"in\"/>\n"	\
	"    </method>\n"						\
	"    <method name=\"TerminateCall\">\n"				\
	"    </method>\n"						\
	"    <method name=\"SetBatteryLevel\">\n"				\
	"      <arg name=\"value\" type=\"i\" direction=\"in\"/>\n"	\
	"    </method>\n"						\
	"    <method name=\"SetSignalStrength\">\n"				\
	"      <arg name=\"value\" type=\"i\" direction=\"in\"/>\n"	\
	"    </method>\n"						\
	"    <method name=\"SetServiceAvailability\">\n"				\
	"      <arg name=\"value\" type=\"i\" direction=\"in\"/>\n"	\
	"    </method>\n"						\
	"    <method name=\"SetDialNumber\">\n"				\
	"      <arg name=\"value\" type=\"s\" direction=\"in\"/>\n"	\
	"    </method>\n"						\
	"    <method name=\"SetCallheld\">\n"				\
	"      <arg name=\"value\" type=\"i\" direction=\"in\"/>\n"	\
	"    </method>\n"						\
	"  </interface>\n"						\
	"  <interface name=\"" DBUS_INTERFACE_INTROSPECTABLE "\">\n"	\
	"    <method name=\"Introspect\">\n"				\
	"      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"	\
	"    </method>\n"						\
	"  </interface>\n"						\
	"</node>\n"

static struct cras_telephony_handle telephony_handle;

/* Helper to extract a single argument from a DBus message. */
static DBusHandlerResult get_single_arg(DBusMessage *message,
					int dbus_type, void *arg)
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

	return DBUS_HANDLER_RESULT_HANDLED;
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

static DBusHandlerResult handle_incoming_call(DBusConnection *conn,
					      DBusMessage *message,
					      void *arg)
{
	struct hfp_slc_handle *handle;
	DBusHandlerResult rc;
	const char* number;

	rc = get_single_arg(message, DBUS_TYPE_STRING, &number);
	if (rc != DBUS_HANDLER_RESULT_HANDLED)
		return rc;

	handle = cras_hfp_ag_get_active_handle();

	telephony_handle.callsetup = 1;

	if (handle) {
		hfp_event_update_callsetup(handle);
		hfp_event_incoming_call(handle, number, 129);
	}

	send_empty_reply(conn, message);
	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult handle_terminate_call(DBusConnection *conn,
					       DBusMessage *message,
					       void *arg)
{
	cras_telephony_event_terminate_call();

	send_empty_reply(conn, message);
	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult handle_answer_call(DBusConnection *conn,
					    DBusMessage *message,
					    void *arg)
{
	cras_telephony_event_answer_call();

	send_empty_reply(conn, message);
	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult handle_set_dial_number(DBusConnection *conn,
						DBusMessage *message,
						void *arg)
{
	DBusHandlerResult rc;
	const char *number;

	rc = get_single_arg(message, DBUS_TYPE_STRING, &number);
	if (rc != DBUS_HANDLER_RESULT_HANDLED)
		return rc;

	cras_telephony_store_dial_number(strlen(number), number);

	send_empty_reply(conn, message);
	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult handle_set_battery(DBusConnection *conn,
					    DBusMessage *message,
					    void *arg)
{
	struct hfp_slc_handle *handle;
	DBusHandlerResult rc;
	int value;

	rc = get_single_arg(message, DBUS_TYPE_INT32, &value);
	if (rc != DBUS_HANDLER_RESULT_HANDLED)
		return rc;

	handle = cras_hfp_ag_get_active_handle();
	if (handle)
		hfp_event_set_battery(handle, value);

	send_empty_reply(conn, message);
	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult handle_set_signal(DBusConnection *conn,
					   DBusMessage *message,
					   void *arg)
{
	struct hfp_slc_handle *handle;
	DBusHandlerResult rc;
	int value;

	rc = get_single_arg(message, DBUS_TYPE_INT32, &value);
	if (rc != DBUS_HANDLER_RESULT_HANDLED)
		return rc;

	handle = cras_hfp_ag_get_active_handle();
	if (handle)
		hfp_event_set_signal(handle, value);

	send_empty_reply(conn, message);
	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult handle_set_service(DBusConnection *conn,
					    DBusMessage *message,
					    void *arg)
{
	struct hfp_slc_handle *handle;
	DBusHandlerResult rc;
	int value;

	rc = get_single_arg(message, DBUS_TYPE_INT32, &value);
	if (rc != DBUS_HANDLER_RESULT_HANDLED)
		return rc;

	handle = cras_hfp_ag_get_active_handle();
	if (handle)
		hfp_event_set_service(handle, value);

	send_empty_reply(conn, message);
	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult handle_set_callheld(DBusConnection *conn,
					     DBusMessage *message,
					     void *arg)
{
	struct hfp_slc_handle *handle;
	DBusHandlerResult rc;
	int value;

	rc = get_single_arg(message, DBUS_TYPE_INT32, &value);
	if (rc != DBUS_HANDLER_RESULT_HANDLED)
		return rc;

	telephony_handle.callheld = value;
	handle = cras_hfp_ag_get_active_handle();
	if (handle)
		hfp_event_update_callheld(handle);

	send_empty_reply(conn, message);
	return DBUS_HANDLER_RESULT_HANDLED;
}

/* Handle incoming messages. */
static DBusHandlerResult handle_telephony_message(DBusConnection *conn,
						  DBusMessage *message,
						  void *arg)
{
	syslog(LOG_ERR, "Telephony message: %s %s %s",
			dbus_message_get_path(message),
			dbus_message_get_interface(message),
			dbus_message_get_member(message));

	if (dbus_message_is_method_call(message,
					DBUS_INTERFACE_INTROSPECTABLE,
					"Introspect")) {
		DBusMessage *reply;
		const char *xml = TELEPHONY_INTROSPECT_XML;

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
					       CRAS_TELEPHONY_INTERFACE,
					       "IncomingCall")) {
		return handle_incoming_call(conn, message, arg);
	} else if (dbus_message_is_method_call(message,
					       CRAS_TELEPHONY_INTERFACE,
					       "TerminateCall")) {
		return handle_terminate_call(conn, message, arg);
	} else if (dbus_message_is_method_call(message,
					       CRAS_TELEPHONY_INTERFACE,
					       "AnswerCall")) {
		return handle_answer_call(conn, message, arg);
	} else if (dbus_message_is_method_call(message,
					       CRAS_TELEPHONY_INTERFACE,
					       "SetDialNumber")) {
		return handle_set_dial_number(conn, message, arg);
	} else if (dbus_message_is_method_call(message,
					       CRAS_TELEPHONY_INTERFACE,
					       "SetBatteryLevel")) {
		return handle_set_battery(conn, message, arg);
	} else if (dbus_message_is_method_call(message,
					       CRAS_TELEPHONY_INTERFACE,
					       "SetSignalStrength")) {
		return handle_set_signal(conn, message, arg);
	} else if (dbus_message_is_method_call(message,
					       CRAS_TELEPHONY_INTERFACE,
					       "SetServiceAvailability")) {
		return handle_set_service(conn, message, arg);
	} else if (dbus_message_is_method_call(message,
					       CRAS_TELEPHONY_INTERFACE,
					       "SetCallheld")) {
		return handle_set_callheld(conn, message, arg);
	}

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

/* Exported Interface */

void cras_telephony_start(DBusConnection *conn)
{
	static const DBusObjectPathVTable control_vtable = {
		.message_function = handle_telephony_message,
	};

	DBusError dbus_error;

	telephony_handle.dbus_conn = conn;
	dbus_connection_ref(telephony_handle.dbus_conn);

	if (!dbus_connection_register_object_path(conn,
						  CRAS_TELEPHONY_OBJECT_PATH,
						  &control_vtable,
						  &dbus_error)) {
		syslog(LOG_ERR,
		       "Couldn't register telephony control: %s: %s",
		       CRAS_TELEPHONY_OBJECT_PATH, dbus_error.message);
		dbus_error_free(&dbus_error);
		return;
	}
}

void cras_telephony_stop()
{
	if (!telephony_handle.dbus_conn)
		return;

	dbus_connection_unregister_object_path(telephony_handle.dbus_conn,
					       CRAS_TELEPHONY_OBJECT_PATH);
	dbus_connection_unref(telephony_handle.dbus_conn);
	telephony_handle.dbus_conn = NULL;
}

struct cras_telephony_handle* cras_telephony_get()
{
	return &telephony_handle;
}

/* Procedure to answer a call from AG.
 *
 * HF(hands-free)                             AG(audio gateway)
 *                                                     <-- Call answered
 *                 <-- +CIEV: (call = 1)
 *                 <-- +CIEV: (callsetup = 0)
 */
int cras_telephony_event_answer_call()
{
	int rc;

	struct hfp_slc_handle *handle;

	handle = cras_hfp_ag_get_active_handle();

	if (telephony_handle.call == 0) {
		telephony_handle.call = 1;
		if (handle) {
			rc = hfp_event_update_call(handle);
			if (rc)
				return rc;
		}
	}

	telephony_handle.callsetup = 0;
	if (handle) {
		rc = hfp_event_update_callsetup(handle);
		if (rc)
			return rc;
	}

	return 0;
}

/* Procedure to terminate a call from AG.
 *
 * HF(hands-free)                             AG(audio gateway)
 *                                                     <-- Call dropped
 *                 <-- +CIEV: (call = 0)
 */
int cras_telephony_event_terminate_call()
{
	int rc;
	struct hfp_slc_handle *handle;

	handle = cras_hfp_ag_get_active_handle();

	if (telephony_handle.call) {
		telephony_handle.call = 0;
		if (handle) {
			rc = hfp_event_update_call(handle);
			if (rc)
				return rc;
		}
	}
	if (telephony_handle.callsetup) {
		telephony_handle.callsetup = 0;
		if (handle) {
			rc = hfp_event_update_callsetup(handle);
			if (rc)
				return rc;
		}
	}
	return 0;
}

void cras_telephony_store_dial_number(int len,
				      const char *number)
{
	if (telephony_handle.dial_number != NULL) {
		free(telephony_handle.dial_number);
		telephony_handle.dial_number = NULL;
	}

	if (len == 0)
		return ;

	telephony_handle.dial_number =
			(char *) calloc(len + 1,
					sizeof(*telephony_handle.dial_number));
	strncpy(telephony_handle.dial_number, number, len);

	syslog(LOG_ERR,
	       "store dial_number: \"%s\"", telephony_handle.dial_number);
}
