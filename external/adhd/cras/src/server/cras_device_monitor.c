/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <syslog.h>

#include "cras_device_monitor.h"
#include "cras_iodev_list.h"
#include "cras_main_message.h"

enum CRAS_DEVICE_MONITOR_MSG_TYPE {
	RESET_DEVICE,
	SET_MUTE_STATE,
};

struct cras_device_monitor_message {
	struct cras_main_message header;
	enum CRAS_DEVICE_MONITOR_MSG_TYPE message_type;
	struct cras_iodev *iodev;
};

static void init_device_msg(
		struct cras_device_monitor_message *msg,
		enum CRAS_DEVICE_MONITOR_MSG_TYPE type,
		struct cras_iodev *iodev)
{
	memset(msg, 0, sizeof(*msg));
	msg->header.type = CRAS_MAIN_MONITOR_DEVICE;
	msg->header.length = sizeof(*msg);
	msg->message_type = type;
	msg->iodev = iodev;
}

int cras_device_monitor_reset_device(struct cras_iodev *iodev)
{
	struct cras_device_monitor_message msg;
	int err;

	init_device_msg(&msg, RESET_DEVICE, iodev);
	err = cras_main_message_send((struct cras_main_message *)&msg);
	if (err < 0) {
		syslog(LOG_ERR, "Failed to send device message %d",
		       RESET_DEVICE);
		return err;
	}
	return 0;
}

int cras_device_monitor_set_device_mute_state(struct cras_iodev *iodev)
{
	struct cras_device_monitor_message msg;
	int err;

	init_device_msg(&msg, SET_MUTE_STATE, iodev);
	err = cras_main_message_send((struct cras_main_message *)&msg);
	if (err < 0) {
		syslog(LOG_ERR, "Failed to send device message %d",
		       SET_MUTE_STATE);
		return err;
	}
	return 0;
}


/* When device is in a bad state, e.g. severe underrun,
 * it might break how audio thread works and cause busy wake up loop.
 * Resetting the device can bring device back to normal state.
 * Let main thread follow the disable/enable sequence in iodev_list
 * to properly close/open the device while enabling/disabling fallback
 * device.
 */
static void handle_device_message(struct cras_main_message *msg, void *arg)
{
	struct cras_device_monitor_message *device_msg =
			(struct cras_device_monitor_message *)msg;
	struct cras_iodev *iodev = device_msg->iodev;

	switch (device_msg->message_type) {
	case RESET_DEVICE:
		syslog(LOG_ERR, "trying to recover device 0x%x by resetting it",
		       iodev->info.idx);
		cras_iodev_list_disable_dev(iodev);
		cras_iodev_list_enable_dev(iodev);
		break;
	case SET_MUTE_STATE:
		cras_iodev_set_mute(iodev);
		break;
	default:
		syslog(LOG_ERR, "Unknown device message type %u",
		       device_msg->message_type);
		break;
	}
}

int cras_device_monitor_init()
{
	cras_main_message_add_handler(CRAS_MAIN_MONITOR_DEVICE,
				      handle_device_message, NULL);
	return 0;
}
