/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <dbus/dbus.h>
#include <errno.h>
#include <poll.h>
#include <stdint.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/select.h>
#include <unistd.h>

#include "cras_system_state.h"
#include "cras_tm.h"

static void dbus_watch_callback(void *arg)
{
	DBusWatch *watch = (DBusWatch *)arg;
	int r, flags;
	struct pollfd pollfd;

	pollfd.fd = dbus_watch_get_unix_fd(watch);
	pollfd.events = POLLIN | POLLOUT;

	r = poll(&pollfd, 1, 0);
	if (r <= 0)
		return;

	flags = 0;
	if (pollfd.revents & POLLIN)
		flags |= DBUS_WATCH_READABLE;
	if (pollfd.revents & POLLOUT)
		flags |= DBUS_WATCH_WRITABLE;

	if (!dbus_watch_handle(watch, flags))
		syslog(LOG_WARNING, "Failed to handle D-Bus watch.");
}

static dbus_bool_t dbus_watch_add(DBusWatch *watch, void *data)
{
	int r;
	unsigned int flags = dbus_watch_get_flags(watch);

	/* Only select the read watch.
	 * TODO(hychao): select on write watch when we have a use case.
	 */
	if ((flags & DBUS_WATCH_READABLE) && dbus_watch_get_enabled(watch)) {
		r = cras_system_add_select_fd(dbus_watch_get_unix_fd(watch),
					      dbus_watch_callback,
					      watch);
		if (r != 0)
			return FALSE;
	}

	return TRUE;
}

static void dbus_watch_remove(DBusWatch *watch, void *data)
{
	unsigned int flags = dbus_watch_get_flags(watch);

	/* Only select the read watch. */
	if (flags & DBUS_WATCH_READABLE)
		cras_system_rm_select_fd(dbus_watch_get_unix_fd(watch));
}

static void dbus_watch_toggled(DBusWatch *watch, void *data)
{
	if (dbus_watch_get_enabled(watch)) {
		dbus_watch_add(watch, NULL);
	} else {
		dbus_watch_remove(watch, NULL);
	}
}


static void dbus_timeout_callback(struct cras_timer *t, void *data)
{
	struct cras_tm *tm = cras_system_state_get_tm();
	struct DBusTimeout *timeout = data;


	/* Timer is automatically removed after it fires.  Add a new one so this
	 * fires until it is removed by dbus. */
	t = cras_tm_create_timer(tm,
				 dbus_timeout_get_interval(timeout),
				 dbus_timeout_callback, timeout);
	dbus_timeout_set_data(timeout, t, NULL);

	if (!dbus_timeout_handle(timeout))
		syslog(LOG_WARNING, "Failed to handle D-Bus timeout.");
}

static dbus_bool_t dbus_timeout_add(DBusTimeout *timeout, void *arg)
{
	struct cras_tm *tm = cras_system_state_get_tm();
	struct cras_timer *t = dbus_timeout_get_data(timeout);

	if (t) {
		dbus_timeout_set_data(timeout, NULL, NULL);
		cras_tm_cancel_timer(tm, t);
	}

	if (dbus_timeout_get_enabled(timeout)) {
		t = cras_tm_create_timer(tm,
					 dbus_timeout_get_interval(timeout),
					 dbus_timeout_callback, timeout);
		dbus_timeout_set_data(timeout, t, NULL);
		if (t == NULL)
			return FALSE;

	}

	return TRUE;
}

static void dbus_timeout_remove(DBusTimeout *timeout, void *arg)
{
	struct cras_tm *tm = cras_system_state_get_tm();
	struct cras_timer *t = dbus_timeout_get_data(timeout);

	if (t) {
		dbus_timeout_set_data(timeout, NULL, NULL);
		cras_tm_cancel_timer(tm, t);
	}
}

static void dbus_timeout_toggled(DBusTimeout *timeout, void *arg)
{
	if (dbus_timeout_get_enabled(timeout))
		dbus_timeout_add(timeout, NULL);
	else
		dbus_timeout_remove(timeout, NULL);
}

DBusConnection *cras_dbus_connect_system_bus()
{
	DBusError dbus_error;
	DBusConnection *conn;
	int rc;

	dbus_error_init(&dbus_error);

	conn = dbus_bus_get(DBUS_BUS_SYSTEM, &dbus_error);
	if (!conn) {
		syslog(LOG_WARNING, "Failed to connect to D-Bus: %s",
		       dbus_error.message);
		dbus_error_free(&dbus_error);
		return NULL;
	}

	/* Request a name on the bus. */
	rc = dbus_bus_request_name(conn, "org.chromium.cras", 0, &dbus_error);
	if (dbus_error_is_set(&dbus_error)) {
		syslog(LOG_ERR, "Requesting dbus name %s", dbus_error.message);
		dbus_error_free(&dbus_error);
	}
	if (rc != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
		syslog(LOG_ERR, "Not primary owner of dbus name.");

	if (!dbus_connection_set_watch_functions(conn,
						 dbus_watch_add,
						 dbus_watch_remove,
						 dbus_watch_toggled,
						 NULL,
						 NULL))
		goto error;
	if (!dbus_connection_set_timeout_functions(conn,
						   dbus_timeout_add,
						   dbus_timeout_remove,
						   dbus_timeout_toggled,
						   NULL,
						   NULL))
		goto error;

	return conn;

error:
	syslog(LOG_WARNING, "Failed to setup D-Bus connection.");
	dbus_connection_unref(conn);
	return NULL;
}

void cras_dbus_dispatch(DBusConnection *conn)
{
	while (dbus_connection_dispatch(conn)
		== DBUS_DISPATCH_DATA_REMAINS)
		;
}

void cras_dbus_disconnect_system_bus(DBusConnection *conn)
{
	dbus_connection_unref(conn);
}
