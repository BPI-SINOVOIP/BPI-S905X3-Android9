/* Copyright (c) 2013 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_BT_DEVICE_H_
#define CRAS_BT_DEVICE_H_

#include <dbus/dbus.h>

struct cras_bt_adapter;
struct cras_bt_device;
struct cras_iodev;
struct cras_timer;

enum cras_bt_device_profile {
	CRAS_BT_DEVICE_PROFILE_A2DP_SOURCE	= (1 << 0),
	CRAS_BT_DEVICE_PROFILE_A2DP_SINK	= (1 << 1),
	CRAS_BT_DEVICE_PROFILE_AVRCP_REMOTE	= (1 << 2),
	CRAS_BT_DEVICE_PROFILE_AVRCP_TARGET	= (1 << 3),
	CRAS_BT_DEVICE_PROFILE_HFP_HANDSFREE	= (1 << 4),
	CRAS_BT_DEVICE_PROFILE_HFP_AUDIOGATEWAY	= (1 << 5),
	CRAS_BT_DEVICE_PROFILE_HSP_HEADSET	= (1 << 6),
	CRAS_BT_DEVICE_PROFILE_HSP_AUDIOGATEWAY = (1 << 7)
};

enum cras_bt_device_profile cras_bt_device_profile_from_uuid(const char *uuid);

struct cras_bt_device *cras_bt_device_create(DBusConnection *conn,
					     const char *object_path);
void cras_bt_device_destroy(struct cras_bt_device *device);
void cras_bt_device_reset();

struct cras_bt_device *cras_bt_device_get(const char *object_path);
size_t cras_bt_device_get_list(struct cras_bt_device ***device_list_out);

const char *cras_bt_device_object_path(const struct cras_bt_device *device);
struct cras_bt_adapter *cras_bt_device_adapter(
	const struct cras_bt_device *device);
const char *cras_bt_device_address(const struct cras_bt_device *device);
const char *cras_bt_device_name(const struct cras_bt_device *device);
int cras_bt_device_paired(const struct cras_bt_device *device);
int cras_bt_device_trusted(const struct cras_bt_device *device);
int cras_bt_device_connected(const struct cras_bt_device *device);

void cras_bt_device_update_properties(struct cras_bt_device *device,
				      DBusMessageIter *properties_array_iter,
				      DBusMessageIter *invalidated_array_iter);

/* Sets the append_iodev_cb to bt device. */
void cras_bt_device_set_append_iodev_cb(struct cras_bt_device *device,
					void (*cb)(void *data));

/* Checks if profile is claimed supported by the device. */
int cras_bt_device_supports_profile(const struct cras_bt_device *device,
				    enum cras_bt_device_profile profile);

/* Sets if the BT audio device should use hardware volume.
 * Args:
 *    device - The remote bluetooth audio device.
 *    use_hardware_volume - Set to true to indicate hardware volume
 *        is preferred over software volume.
 */
void cras_bt_device_set_use_hardware_volume(struct cras_bt_device *device,
					    int use_hardware_volume);

/* Gets if the BT audio device should use hardware volume. */
int cras_bt_device_get_use_hardware_volume(struct cras_bt_device *device);

/* Forces disconnect the bt device. Used when handling audio error
 * that we want to make the device be completely disconnected from
 * host to reflect the state that an error has occurred.
 * Args:
 *    conn - The dbus connection.
 *    device - The bt device to disconnect.
 */
int cras_bt_device_disconnect(DBusConnection *conn,
			      struct cras_bt_device *device);

/* Gets the SCO socket for the device.
 * Args:
 *     device - The device object to get SCO socket for.
 */
int cras_bt_device_sco_connect(struct cras_bt_device *device);

/* Queries the preffered mtu value for SCO socket. */
int cras_bt_device_sco_mtu(struct cras_bt_device *device, int sco_socket);

/* Appends an iodev to bt device.
 * Args:
 *    device - The device to append iodev to.
 *    iodev - The iodev to add.
 *    profile - The profile of the iodev about to add.
 */
void cras_bt_device_append_iodev(struct cras_bt_device *device,
				 struct cras_iodev *iodev,
				 enum cras_bt_device_profile profile);

/* Removes an iodev from bt device.
 * Args:
 *    device - The device to remove iodev from.
 *    iodev - The iodev to remove.
 */
void cras_bt_device_rm_iodev(struct cras_bt_device *device,
			     struct cras_iodev *iodev);

/* Gets the active profile of the bt device. */
int cras_bt_device_get_active_profile(const struct cras_bt_device *device);

/* Sets the active profile of the bt device. */
void cras_bt_device_set_active_profile(struct cras_bt_device *device,
				       unsigned int profile);

/* Switches profile after the active profile of bt device has changed and
 * enables bt iodev immediately. This function is used for profile switching
 * at iodev open.
 * Args:
 *    device - The bluetooth device.
 *    bt_iodev - The iodev triggers the reactivaion.
 */
int cras_bt_device_switch_profile_enable_dev(struct cras_bt_device *device,
					     struct cras_iodev *bt_iodev);

/* Switches profile after the active profile of bt device has changed. This
 * function is used when we want to switch profile without changing the
 * iodev's status.
 * Args:
 *    device - The bluetooth device.
 *    bt_iodev - The iodev triggers the reactivaion.
 */
int cras_bt_device_switch_profile(struct cras_bt_device *device,
				  struct cras_iodev *bt_iodev);

/* Calls this function when the buffer size of an underlying profile iodev
 * has changed and update it for the virtual bt iodev. */
void cras_bt_device_iodev_buffer_size_changed(struct cras_bt_device *device);

void cras_bt_device_start_monitor();

/* Checks if the device has an iodev for A2DP. */
int cras_bt_device_has_a2dp(struct cras_bt_device *device);

/* Returns true if and only if device has an iodev for A2DP and the bt device
 * is not opening for audio capture.
 */
int cras_bt_device_can_switch_to_a2dp(struct cras_bt_device *device);

/* Updates the volume to bt_device when a volume change event is reported. */
void cras_bt_device_update_hardware_volume(struct cras_bt_device *device,
					   int volume);

/* Notifies bt_device that a2dp connection is configured. */
void cras_bt_device_a2dp_configured(struct cras_bt_device *device);

/* Cancels any scheduled suspension of device. */
int cras_bt_device_cancel_suspend(struct cras_bt_device *device);

/* Schedules device to suspend after given delay. */
int cras_bt_device_schedule_suspend(struct cras_bt_device *device,
				    unsigned int msec);

/* Notifies bt device that audio gateway is initialized.
 * Args:
 *   device - The bluetooth device.
 * Returns:
 *   0 on success, error code otherwise.
 */
int cras_bt_device_audio_gateway_initialized(struct cras_bt_device *device);

#endif /* CRAS_BT_DEVICE_H_ */
