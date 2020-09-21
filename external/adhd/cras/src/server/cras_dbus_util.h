/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <dbus/dbus.h>


/* Appends a key-value pair to the dbus message.
 * Args:
 *    key - the key (a string)
 *    type - the type of the value (for example, 'y')
 *    type_string - the type of the value in string form (for example, "y")
 *    value - a pointer to the value to be appended.
 * Returns:
 *    false if not enough memory.
*/
dbus_bool_t append_key_value(DBusMessageIter *iter, const char *key,
			     int type, const char *type_string,
			     void *value);
