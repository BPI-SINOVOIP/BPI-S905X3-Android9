/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cras_dbus_util.h"

dbus_bool_t append_key_value(DBusMessageIter *iter, const char *key,
			     int type, const char *type_string,
			     void *value)
{
       DBusMessageIter entry, variant;

       if (!dbus_message_iter_open_container(iter, DBUS_TYPE_DICT_ENTRY, NULL,
                                             &entry))
               return FALSE;
       if (!dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key))
               return FALSE;
       if (!dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT,
                                             type_string, &variant))
               return FALSE;
       if (!dbus_message_iter_append_basic(&variant, type, value))
               return FALSE;
       if (!dbus_message_iter_close_container(&entry, &variant))
               return FALSE;
       if (!dbus_message_iter_close_container(iter, &entry))
               return FALSE;

       return TRUE;
}
