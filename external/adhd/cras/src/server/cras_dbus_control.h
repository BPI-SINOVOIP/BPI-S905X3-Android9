/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_DBUS_CONTROL_H_
#define CRAS_DBUS_CONTROL_H_

/* Starts the dbus control interface, begins listening for incoming messages. */
void cras_dbus_control_start(DBusConnection *conn);

/* Stops monitoring the dbus interface for command messages. */
void cras_dbus_control_stop();

#endif /* CRAS_DBUS_CONTROL_H_ */
