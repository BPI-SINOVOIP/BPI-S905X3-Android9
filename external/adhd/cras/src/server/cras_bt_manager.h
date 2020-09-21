/* Copyright (c) 2013 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_BT_MANAGER_H_
#define CRAS_BT_MANAGER_H_

#include <dbus/dbus.h>

void cras_bt_start(DBusConnection *conn);
void cras_bt_stop(DBusConnection *conn);

#endif /* CRAS_BT_MANAGER_H_ */
