/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdint.h>
#include <syslog.h>
#include <sys/socket.h>
#include <unistd.h>

#include "cras_a2dp_endpoint.h"
#include "cras_bt_adapter.h"
#include "cras_bt_constants.h"
#include "cras_bt_profile.h"
#include "cras_hfp_ag_profile.h"
#include "cras_hfp_info.h"
#include "cras_hfp_iodev.h"
#include "cras_hfp_slc.h"
#include "cras_system_state.h"
#include "utlist.h"

#define STR(s) #s
#define VSTR(id) STR(id)

#define HFP_AG_PROFILE_NAME "Hands-Free Voice gateway"
#define HFP_AG_PROFILE_PATH "/org/chromium/Cras/Bluetooth/HFPAG"
#define HFP_VERSION_1_5 0x0105
#define HSP_AG_PROFILE_NAME "Headset Voice gateway"
#define HSP_AG_PROFILE_PATH "/org/chromium/Cras/Bluetooth/HSPAG"
#define HSP_VERSION_1_2 0x0102

#define HSP_AG_RECORD 							\
	"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"			\
	"<record>"							\
	"  <attribute id=\"0x0001\">"					\
	"    <sequence>"						\
	"      <uuid value=\"" HSP_AG_UUID "\" />"			\
	"      <uuid value=\"" GENERIC_AUDIO_UUID "\" />"		\
	"    </sequence>"						\
	"  </attribute>"						\
	"  <attribute id=\"0x0004\">"					\
	"    <sequence>"						\
	"      <sequence>"						\
	"        <uuid value=\"0x0100\" />"				\
	"      </sequence>"						\
	"      <sequence>"						\
	"        <uuid value=\"0x0003\" />"				\
	"        <uint8 value=\"0x0c\" />"				\
	"      </sequence>"						\
	"    </sequence>"						\
	"  </attribute>"						\
	"  <attribute id=\"0x0005\">"					\
	"    <sequence>"						\
	"      <uuid value=\"0x1002\" />"				\
	"    </sequence>"						\
	"  </attribute>"						\
	"  <attribute id=\"0x0009\">"					\
	"    <sequence>"						\
	"      <sequence>"						\
	"        <uuid value=\"" HSP_HS_UUID "\" />"			\
	"        <uint16 value=\"" VSTR(HSP_VERSION_1_2) "\" />"	\
	"      </sequence>"						\
	"    </sequence>"						\
	"  </attribute>"						\
	"  <attribute id=\"0x0100\">"					\
	"    <text value=\"" HSP_AG_PROFILE_NAME "\" />"		\
	"  </attribute>"						\
	"  <attribute id=\"0x0301\" >"					\
	"    <uint8 value=\"0x01\" />"					\
	"  </attribute>"						\
	"</record>"


/* Object representing the audio gateway role for HFP/HSP.
 * Members:
 *    idev - The input iodev for HFP/HSP.
 *    odev - The output iodev for HFP/HSP.
 *    info - The hfp_info object for SCO audio.
 *    slc_handle - The service level connection.
 *    device - The bt device associated with this audio gateway.
 *    a2dp_delay_retries - The number of retries left to delay starting
 *        the hfp/hsp audio gateway to wait for a2dp connection.
 *    conn - The dbus connection used to send message to bluetoothd.
 *    profile - The profile enum of this audio gateway.
 */
struct audio_gateway {
	struct cras_iodev *idev;
	struct cras_iodev *odev;
	struct hfp_info *info;
	struct hfp_slc_handle *slc_handle;
	struct cras_bt_device *device;
	int a2dp_delay_retries;
	DBusConnection *conn;
	enum cras_bt_device_profile profile;
	struct audio_gateway *prev, *next;
};

static struct audio_gateway *connected_ags;

static void destroy_audio_gateway(struct audio_gateway *ag)
{
	DL_DELETE(connected_ags, ag);

	if (ag->idev)
		hfp_iodev_destroy(ag->idev);
	if (ag->odev)
		hfp_iodev_destroy(ag->odev);
	if (ag->info) {
		if (hfp_info_running(ag->info))
			hfp_info_stop(ag->info);
		hfp_info_destroy(ag->info);
	}
	if (ag->slc_handle)
		hfp_slc_destroy(ag->slc_handle);

	/* If the bt device is not using a2dp, do a deeper clean up
	 * to force disconnect it. */
	if (!cras_bt_device_has_a2dp(ag->device))
		cras_bt_device_disconnect(ag->conn, ag->device);

	free(ag);
}

/* Checks if there already a audio gateway connected for device. */
static int has_audio_gateway(struct cras_bt_device *device)
{
	struct audio_gateway *ag;
	DL_FOREACH(connected_ags, ag) {
		if (ag->device == device)
			return 1;
	}
	return 0;
}

static void cras_hfp_ag_release(struct cras_bt_profile *profile)
{
	cras_hfp_ag_suspend();
}

/* Callback triggered when SLC is initialized.  */
static int cras_hfp_ag_slc_initialized(struct hfp_slc_handle *handle)
{
	struct audio_gateway *ag;

	DL_SEARCH_SCALAR(connected_ags, ag, slc_handle, handle);
	if (!ag)
		return -EINVAL;

	/* Defer the starting of audio gateway to bt_device. */
	return cras_bt_device_audio_gateway_initialized(ag->device);
}

static int cras_hfp_ag_slc_disconnected(struct hfp_slc_handle *handle)
{
	struct audio_gateway *ag;

	DL_SEARCH_SCALAR(connected_ags, ag, slc_handle, handle);
	if (!ag)
		return -EINVAL;

	destroy_audio_gateway(ag);
	return 0;
}

static int check_for_conflict_ag(struct cras_bt_device *new_connected)
{
	struct audio_gateway *ag;

	/* Check if there's already an A2DP/HFP device. */
	DL_FOREACH(connected_ags, ag) {
		if (cras_bt_device_has_a2dp(ag->device))
			return -1;
	}

	/* Check if there's already an A2DP-only device. */
	if (cras_a2dp_connected_device() &&
		cras_bt_device_supports_profile(
			new_connected, CRAS_BT_DEVICE_PROFILE_A2DP_SINK))
		return -1;

	return 0;
}

static void possibly_remove_conflict_dev(void *data)
{
	struct cras_bt_device *device = (struct cras_bt_device *)data;
	struct audio_gateway *ag, *new_ag = NULL;
	struct cras_bt_device *a2dp_device;

	/* Check if the device is still connected. */
	DL_FOREACH(connected_ags, ag) {
		if (ag->device == device)
			new_ag = ag;
	}
	if (!new_ag)
		return;

	/* Kick out any previously connected hfp iodev. */
	DL_FOREACH(connected_ags, ag) {
		if (ag == new_ag)
			continue;
		destroy_audio_gateway(ag);
	}

	/* Kick out any previously connected a2dp iodev. */
	a2dp_device = cras_a2dp_connected_device();
	if (a2dp_device && a2dp_device != device) {
		cras_a2dp_suspend_connected_device(a2dp_device);
		cras_bt_device_disconnect(new_ag->conn, a2dp_device);
	}
}

static int cras_hfp_ag_new_connection(DBusConnection *conn,
					   struct cras_bt_profile *profile,
				       struct cras_bt_device *device,
				       int rfcomm_fd)
{
	struct audio_gateway *ag;

	if (has_audio_gateway(device)) {
		syslog(LOG_ERR, "Audio gateway exists when %s connects for profile %s",
			cras_bt_device_name(device), profile->name);
		close(rfcomm_fd);
		return 0;
	}

	if (check_for_conflict_ag(device))
		return -1;

	cras_bt_device_set_append_iodev_cb(device, possibly_remove_conflict_dev);
	ag = (struct audio_gateway *)calloc(1, sizeof(*ag));
	ag->device = device;
	ag->conn = conn;
	ag->profile = cras_bt_device_profile_from_uuid(profile->uuid);
	ag->slc_handle = hfp_slc_create(rfcomm_fd,
					0,
					device,
					cras_hfp_ag_slc_initialized,
					cras_hfp_ag_slc_disconnected);
	DL_APPEND(connected_ags, ag);
	return 0;
}

static void cras_hfp_ag_request_disconnection(struct cras_bt_profile *profile,
					      struct cras_bt_device *device)
{
	struct audio_gateway *ag;
	DL_FOREACH(connected_ags, ag) {
		if (ag->slc_handle && ag->device == device)
			destroy_audio_gateway(ag);
	}
}

static void cras_hfp_ag_cancel(struct cras_bt_profile *profile)
{
}

static struct cras_bt_profile cras_hfp_ag_profile = {
	.name = HFP_AG_PROFILE_NAME,
	.object_path = HFP_AG_PROFILE_PATH,
	.uuid = HFP_AG_UUID,
	.version = HFP_VERSION_1_5,
	.role = NULL,
	.features = HFP_SUPPORTED_FEATURE & 0x1F,
	.record = NULL,
	.release = cras_hfp_ag_release,
	.new_connection = cras_hfp_ag_new_connection,
	.request_disconnection = cras_hfp_ag_request_disconnection,
	.cancel = cras_hfp_ag_cancel
};

int cras_hfp_ag_profile_create(DBusConnection *conn)
{
	return cras_bt_add_profile(conn, &cras_hfp_ag_profile);
}

static int cras_hsp_ag_new_connection(DBusConnection *conn,
					   struct cras_bt_profile *profile,
				       struct cras_bt_device *device,
				       int rfcomm_fd)
{
	struct audio_gateway *ag;

	if (has_audio_gateway(device)) {
		syslog(LOG_ERR, "Audio gateway exists when %s connects for profile %s",
			cras_bt_device_name(device), profile->name);
		close(rfcomm_fd);
		return 0;
	}

	if (check_for_conflict_ag(device))
		return -1;

	cras_bt_device_set_append_iodev_cb(device, possibly_remove_conflict_dev);
	ag = (struct audio_gateway *)calloc(1, sizeof(*ag));
	ag->device = device;
	ag->conn = conn;
	ag->profile = cras_bt_device_profile_from_uuid(profile->uuid);
	ag->slc_handle = hfp_slc_create(rfcomm_fd, 1, device, NULL,
					cras_hfp_ag_slc_disconnected);
	DL_APPEND(connected_ags, ag);
	cras_hfp_ag_slc_initialized(ag->slc_handle);
	return 0;
}

static struct cras_bt_profile cras_hsp_ag_profile = {
	.name = HSP_AG_PROFILE_NAME,
	.object_path = HSP_AG_PROFILE_PATH,
	.uuid = HSP_AG_UUID,
	.version = HSP_VERSION_1_2,
	.role = NULL,
	.record = HSP_AG_RECORD,
	.release = cras_hfp_ag_release,
	.new_connection = cras_hsp_ag_new_connection,
	.request_disconnection = cras_hfp_ag_request_disconnection,
	.cancel = cras_hfp_ag_cancel
};

int cras_hfp_ag_start(struct cras_bt_device *device)
{
	struct audio_gateway *ag;

	DL_SEARCH_SCALAR(connected_ags, ag, device, device);
	if (ag == NULL)
		return -EEXIST;

	ag->info = hfp_info_create();
	ag->idev = hfp_iodev_create(CRAS_STREAM_INPUT, ag->device,
				    ag->slc_handle,
				    ag->profile, ag->info);
	ag->odev = hfp_iodev_create(CRAS_STREAM_OUTPUT, ag->device,
				    ag->slc_handle,
				    ag->profile, ag->info);

	if (!ag->idev && !ag->odev) {
		destroy_audio_gateway(ag);
		return -ENOMEM;
	}

	return 0;
}

void cras_hfp_ag_suspend()
{
	struct audio_gateway *ag;
	DL_FOREACH(connected_ags, ag)
		destroy_audio_gateway(ag);
}

void cras_hfp_ag_suspend_connected_device(struct cras_bt_device *device)
{
	struct audio_gateway *ag;

	DL_SEARCH_SCALAR(connected_ags, ag, device, device);
	if (ag)
		destroy_audio_gateway(ag);
}

struct hfp_slc_handle *cras_hfp_ag_get_active_handle()
{
	/* Returns the first handle for HFP qualification. In future we
	 * might want this to return the HFP device user is selected. */
	return connected_ags ? connected_ags->slc_handle : NULL;
}

struct hfp_slc_handle *cras_hfp_ag_get_slc(struct cras_bt_device *device)
{
	struct audio_gateway *ag;
	DL_FOREACH(connected_ags, ag) {
		if (ag->device == device)
			return ag->slc_handle;
	}
	return NULL;
}

int cras_hsp_ag_profile_create(DBusConnection *conn)
{
	return cras_bt_add_profile(conn, &cras_hsp_ag_profile);
}
