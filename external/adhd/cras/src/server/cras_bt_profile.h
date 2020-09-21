/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_BT_PROFILE_H_
#define CRAS_BT_PROFILE_H_

#include <dbus/dbus.h>

#include "cras_bt_device.h"

#define PROFILE_MANAGER_OBJ_PATH "/org/bluez"

/* Structure in cras to represent an external profile of bluez. All members
 * and functions are documented in bluez/doc/profile-api.txt, more options
 * can be put into this structure when we need it.
 */
struct cras_bt_profile {
	const char *name;
	const char *object_path;
	const char *uuid;
	const char *role;
	const char *record;
	int version;
	int features;
	void (*release)(struct cras_bt_profile *profile);
	int (*new_connection)(DBusConnection *conn,
			      struct cras_bt_profile *profile,
			      struct cras_bt_device *device,
			      int rfcomm_fd);
	void (*request_disconnection)(struct cras_bt_profile *profile,
				      struct cras_bt_device *device);
	void (*cancel)(struct cras_bt_profile *profile);
	struct cras_bt_profile *prev, *next;
};

/* Adds the profile to cras and registers it with bluez.
 * Args:
 *    conn - The dbus connection.
 *    profile - Pointer to the profile structure to be add.
 */
int cras_bt_add_profile(DBusConnection *conn,
			struct cras_bt_profile *profile);

/* Gets the profile by object path.
 * Args:
 *    path - The object path of the desired profile.
 *
 * Returns:
 *    The profile of the requested object path, or NULL if it
 *    does not exist.
 */
struct cras_bt_profile *cras_bt_profile_get(const char *path);

/* Resets all added profiles. */
void cras_bt_profile_reset();

/* Notifies all profiles when a device is disconnected. */
void cras_bt_profile_on_device_disconnected(struct cras_bt_device *device);

/* Registers all added profiles.
 * Args:
 *    conn - The dbus connection.
 * Returns:
 *    0 on success, or negative error code on failure.
 */
int cras_bt_register_profiles(DBusConnection *conn);

#endif /* CRAS_BT_PROFILE_H_ */
