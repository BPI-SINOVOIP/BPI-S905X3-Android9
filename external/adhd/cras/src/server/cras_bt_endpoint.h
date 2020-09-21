/* Copyright (c) 2013 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_BT_ENDPOINT_H_
#define CRAS_BT_ENDPOINT_H_

#include <dbus/dbus.h>
#include <stdint.h>

#include "cras_bt_adapter.h"

struct cras_bt_transport;

struct cras_bt_endpoint {
	const char *object_path;
	const char *uuid;
	uint8_t codec;

	int (*get_capabilities)(struct cras_bt_endpoint *endpoint,
				 void *capabilities, int *len);
	int (*select_configuration)(struct cras_bt_endpoint *endpoint,
				    void *capabilities, int len,
				    void *configuration);

	void (*set_configuration)(struct cras_bt_endpoint *endpoint,
				  struct cras_bt_transport *transport);
	void (*suspend)(struct cras_bt_endpoint *endpoint,
			struct cras_bt_transport *transport);

	void (*transport_state_changed)(struct cras_bt_endpoint *endpoint,
					struct cras_bt_transport *transport);

	struct cras_bt_transport *transport;
	struct cras_bt_endpoint *prev, *next;
};

int cras_bt_register_endpoint(DBusConnection *conn,
			      const struct cras_bt_adapter *adapter,
			      struct cras_bt_endpoint *endpoint);

int cras_bt_unregister_endpoint(DBusConnection *conn,
				const struct cras_bt_adapter *adapter,
				struct cras_bt_endpoint *endpoint);

int cras_bt_register_endpoints(DBusConnection *conn,
			       const struct cras_bt_adapter *adapter);

int cras_bt_endpoint_add(DBusConnection *conn,
			 struct cras_bt_endpoint *endpoint);
void cras_bt_endpoint_rm(DBusConnection *conn,
			 struct cras_bt_endpoint *endpoint);

void cras_bt_endpoint_reset();

struct cras_bt_endpoint *cras_bt_endpoint_get(const char *object_path);

#endif /* CRAS_BT_ENDPOINT_H_ */
