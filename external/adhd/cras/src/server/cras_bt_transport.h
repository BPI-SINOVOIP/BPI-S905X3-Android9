/* Copyright (c) 2013 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_BT_TRANSPORT_H_
#define CRAS_BT_TRANSPORT_H_

#include <dbus/dbus.h>
#include <stdint.h>

#include "cras_bt_device.h"

struct cras_bt_endpoint;
struct cras_bt_transport;

enum cras_bt_transport_state {
	CRAS_BT_TRANSPORT_STATE_IDLE,
	CRAS_BT_TRANSPORT_STATE_PENDING,
	CRAS_BT_TRANSPORT_STATE_ACTIVE
};


struct cras_bt_transport *cras_bt_transport_create(DBusConnection *conn,
						   const char *object_path);
void cras_bt_transport_set_endpoint(struct cras_bt_transport *transport,
				    struct cras_bt_endpoint *endpoint);
void cras_bt_transport_destroy(struct cras_bt_transport *transport);
void cras_bt_transport_reset();

struct cras_bt_transport *cras_bt_transport_get(const char *object_path);
size_t cras_bt_transport_get_list(
	struct cras_bt_transport ***transport_list_out);

const char *cras_bt_transport_object_path(
	const struct cras_bt_transport *transport);
struct cras_bt_device *cras_bt_transport_device(
	const struct cras_bt_transport *transport);
enum cras_bt_device_profile cras_bt_transport_profile(
	const struct cras_bt_transport *transport);
int cras_bt_transport_configuration(const struct cras_bt_transport *transport,
				    void *configuration, int len);
enum cras_bt_transport_state cras_bt_transport_state(
	const struct cras_bt_transport *transport);

int cras_bt_transport_fd(const struct cras_bt_transport *transport);
uint16_t cras_bt_transport_write_mtu(const struct cras_bt_transport *transport);

void cras_bt_transport_update_properties(
	struct cras_bt_transport *transport,
	DBusMessageIter *properties_array_iter,
	DBusMessageIter *invalidated_array_iter);

int cras_bt_transport_try_acquire(struct cras_bt_transport *transport);
int cras_bt_transport_acquire(struct cras_bt_transport *transport);

/* Releases the cras_bt_transport.
 * Args:
 *    transport - The transport object to release
 *    blocking - True to send release dbus message in blocking mode, otherwise
 *        in non-block mode.
 */
int cras_bt_transport_release(struct cras_bt_transport *transport,
			      unsigned int blocking);

/* Sets the volume to cras_bt_transport. Note that the volume gets applied
 * to BT headset only when the transport is in ACTIVE state.
 * Args:
 *    transport - The transport object to set volume to.
 *    volume - The desired volume value, range in [0-127].
 */
int cras_bt_transport_set_volume(struct cras_bt_transport *transport,
				 uint16_t volume);

#endif /* CRAS_BT_TRANSPORT_H_ */
