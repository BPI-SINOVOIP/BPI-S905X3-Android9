/* Copyright (c) 2013 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <stdint.h>
#include <syslog.h>

#include "a2dp-codecs.h"
#include "cras_a2dp_endpoint.h"
#include "cras_a2dp_iodev.h"
#include "cras_iodev.h"
#include "cras_bt_constants.h"
#include "cras_bt_endpoint.h"
#include "cras_system_state.h"
#include "cras_util.h"

#define A2DP_SOURCE_ENDPOINT_PATH "/org/chromium/Cras/Bluetooth/A2DPSource"
#define A2DP_SINK_ENDPOINT_PATH   "/org/chromium/Cras/Bluetooth/A2DPSink"


/* Pointers for the only connected a2dp device. */
static struct a2dp {
	struct cras_iodev *iodev;
	struct cras_bt_device *device;
} connected_a2dp;

static int cras_a2dp_get_capabilities(struct cras_bt_endpoint *endpoint,
				      void *capabilities, int *len)
{
	a2dp_sbc_t *sbc_caps = capabilities;

	if (*len < sizeof(*sbc_caps))
		return -ENOSPC;

	*len = sizeof(*sbc_caps);

	/* Return all capabilities. */
	sbc_caps->channel_mode = SBC_CHANNEL_MODE_MONO |
			SBC_CHANNEL_MODE_DUAL_CHANNEL |
			SBC_CHANNEL_MODE_STEREO |
			SBC_CHANNEL_MODE_JOINT_STEREO;
	sbc_caps->frequency = SBC_SAMPLING_FREQ_16000 |
			SBC_SAMPLING_FREQ_32000 |
			SBC_SAMPLING_FREQ_44100 |
			SBC_SAMPLING_FREQ_48000;
	sbc_caps->allocation_method = SBC_ALLOCATION_SNR |
			SBC_ALLOCATION_LOUDNESS;
	sbc_caps->subbands = SBC_SUBBANDS_4 | SBC_SUBBANDS_8;
	sbc_caps->block_length = SBC_BLOCK_LENGTH_4 |
			SBC_BLOCK_LENGTH_8 |
			SBC_BLOCK_LENGTH_12 |
			SBC_BLOCK_LENGTH_16;
	sbc_caps->min_bitpool = MIN_BITPOOL;
	sbc_caps->max_bitpool = MAX_BITPOOL;

	return 0;
}

static int cras_a2dp_select_configuration(struct cras_bt_endpoint *endpoint,
					  void *capabilities, int len,
					  void *configuration)
{
	a2dp_sbc_t *sbc_caps = capabilities;
	a2dp_sbc_t *sbc_config = configuration;

	/* Pick the highest configuration. */
	if (sbc_caps->channel_mode & SBC_CHANNEL_MODE_JOINT_STEREO) {
		sbc_config->channel_mode = SBC_CHANNEL_MODE_JOINT_STEREO;
	} else if (sbc_caps->channel_mode & SBC_CHANNEL_MODE_STEREO) {
		sbc_config->channel_mode = SBC_CHANNEL_MODE_STEREO;
	} else if (sbc_caps->channel_mode & SBC_CHANNEL_MODE_DUAL_CHANNEL) {
		sbc_config->channel_mode = SBC_CHANNEL_MODE_DUAL_CHANNEL;
	} else if (sbc_caps->channel_mode & SBC_CHANNEL_MODE_MONO) {
		sbc_config->channel_mode = SBC_CHANNEL_MODE_MONO;
	} else {
		syslog(LOG_WARNING, "No supported channel modes.");
		return -ENOSYS;
	}

	if (sbc_caps->frequency & SBC_SAMPLING_FREQ_48000) {
		sbc_config->frequency = SBC_SAMPLING_FREQ_48000;
	} else if (sbc_caps->frequency & SBC_SAMPLING_FREQ_44100) {
		sbc_config->frequency = SBC_SAMPLING_FREQ_44100;
	} else if (sbc_caps->frequency & SBC_SAMPLING_FREQ_32000) {
		sbc_config->frequency = SBC_SAMPLING_FREQ_32000;
	} else if (sbc_caps->frequency & SBC_SAMPLING_FREQ_16000) {
		sbc_config->frequency = SBC_SAMPLING_FREQ_16000;
	} else {
		syslog(LOG_WARNING, "No supported sampling frequencies.");
		return -ENOSYS;
	}

	if (sbc_caps->allocation_method & SBC_ALLOCATION_LOUDNESS) {
		sbc_config->allocation_method = SBC_ALLOCATION_LOUDNESS;
	} else if (sbc_caps->allocation_method & SBC_ALLOCATION_SNR) {
		sbc_config->allocation_method = SBC_ALLOCATION_SNR;
	} else {
		syslog(LOG_WARNING, "No supported allocation method.");
		return -ENOSYS;
	}

	if (sbc_caps->subbands & SBC_SUBBANDS_8) {
		sbc_config->subbands = SBC_SUBBANDS_8;
	} else if (sbc_caps->subbands & SBC_SUBBANDS_4) {
		sbc_config->subbands = SBC_SUBBANDS_4;
	} else {
		syslog(LOG_WARNING, "No supported subbands.");
		return -ENOSYS;
	}

	if (sbc_caps->block_length & SBC_BLOCK_LENGTH_16) {
		sbc_config->block_length = SBC_BLOCK_LENGTH_16;
	} else if (sbc_caps->block_length & SBC_BLOCK_LENGTH_12) {
		sbc_config->block_length = SBC_BLOCK_LENGTH_12;
	} else if (sbc_caps->block_length & SBC_BLOCK_LENGTH_8) {
		sbc_config->block_length = SBC_BLOCK_LENGTH_8;
	} else if (sbc_caps->block_length & SBC_BLOCK_LENGTH_4) {
		sbc_config->block_length = SBC_BLOCK_LENGTH_4;
	} else {
		syslog(LOG_WARNING, "No supported block length.");
		return -ENOSYS;
	}

	sbc_config->min_bitpool = (sbc_caps->min_bitpool > MIN_BITPOOL
				   ? sbc_caps->min_bitpool : MIN_BITPOOL);
	sbc_config->max_bitpool = (sbc_caps->max_bitpool < MAX_BITPOOL
				   ? sbc_caps->max_bitpool : MAX_BITPOOL);

	return 0;
}

static void cras_a2dp_set_configuration(struct cras_bt_endpoint *endpoint,
			    struct cras_bt_transport *transport)
{
	struct cras_bt_device *device;

	device = cras_bt_transport_device(transport);
	cras_bt_device_a2dp_configured(device);
}

static void cras_a2dp_suspend(struct cras_bt_endpoint *endpoint,
			      struct cras_bt_transport *transport)
{
	struct cras_bt_device *device = cras_bt_transport_device(transport);
	cras_a2dp_suspend_connected_device(device);
}

static void a2dp_transport_state_changed(struct cras_bt_endpoint *endpoint,
					 struct cras_bt_transport *transport)
{
	if (connected_a2dp.iodev && transport) {
		/* When pending message is received in bluez, try to aquire
		 * the transport. */
		if (cras_bt_transport_fd(transport) != -1 &&
		    cras_bt_transport_state(transport) ==
				CRAS_BT_TRANSPORT_STATE_PENDING)
			cras_bt_transport_try_acquire(transport);
	}
}

static struct cras_bt_endpoint cras_a2dp_endpoint = {
	/* BlueZ connects the device A2DP Sink to our A2DP Source endpoint,
	 * and the device A2DP Source to our A2DP Sink. It's best if you don't
	 * think about it too hard.
	 */
	.object_path = A2DP_SOURCE_ENDPOINT_PATH,
	.uuid = A2DP_SOURCE_UUID,
	.codec = A2DP_CODEC_SBC,

	.get_capabilities = cras_a2dp_get_capabilities,
	.select_configuration = cras_a2dp_select_configuration,
	.set_configuration = cras_a2dp_set_configuration,
	.suspend = cras_a2dp_suspend,
	.transport_state_changed = a2dp_transport_state_changed
};

int cras_a2dp_endpoint_create(DBusConnection *conn)
{
	return cras_bt_endpoint_add(conn, &cras_a2dp_endpoint);
}

void cras_a2dp_start(struct cras_bt_device *device)
{
	struct cras_bt_transport *transport = cras_a2dp_endpoint.transport;

	if (!transport || device != cras_bt_transport_device(transport)) {
		syslog(LOG_ERR, "Device and active transport not match.");
		return;
	}

	if (connected_a2dp.iodev) {
		syslog(LOG_WARNING,
		       "Replacing existing endpoint configuration");
		a2dp_iodev_destroy(connected_a2dp.iodev);
	}

	connected_a2dp.iodev = a2dp_iodev_create(transport);
	connected_a2dp.device = cras_bt_transport_device(transport);

	if (!connected_a2dp.iodev)
		syslog(LOG_WARNING, "Failed to create a2dp iodev");
}

struct cras_bt_device *cras_a2dp_connected_device()
{
	return connected_a2dp.device;
}

void cras_a2dp_suspend_connected_device(struct cras_bt_device *device)
{
	if (connected_a2dp.device != device)
		return;

	if (connected_a2dp.iodev) {
		syslog(LOG_INFO, "Destroying iodev for A2DP device");
		a2dp_iodev_destroy(connected_a2dp.iodev);
		connected_a2dp.iodev = NULL;
		connected_a2dp.device = NULL;
	}
}
