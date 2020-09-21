/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <syslog.h>

#include "cras_bt_io.h"
#include "cras_bt_device.h"
#include "cras_utf8.h"
#include "cras_iodev.h"
#include "cras_iodev_list.h"
#include "sfh.h"
#include "utlist.h"

#define DEFAULT_BT_DEVICE_NAME "BLUETOOTH"

/* Extends cras_ionode to hold bluetooth profile information
 * so that iodevs of different profile(A2DP or HFP/HSP) can be
 * associated with the same bt_io.
 * Members:
 *    base - The base class cras_ionode.
 *    profile_dev - Pointer to the profile specific iodev.
 *    profile - The bluetooth profile profile_dev runs on.
 */
struct bt_node {
	struct cras_ionode base;
	struct cras_iodev *profile_dev;
	unsigned int profile;
};

/* The structure represents a virtual input or output device of a
 * bluetooth audio device, speaker or headset for example. A node
 * will be added to this virtual iodev for each profile supported
 * by the bluetooth audio device.
 * Member:
 *    base - The base class cras_iodev
 *    next_node_id - The index will give to the next node
 */
struct bt_io {
	struct cras_iodev base;
	unsigned int next_node_id;
	struct cras_bt_device *device;
};

/* Returns the active profile specific iodev. */
static struct cras_iodev *active_profile_dev(const struct cras_iodev *iodev)
{
	struct bt_node *active = (struct bt_node *)iodev->active_node;

	return active->profile_dev;
}

/* Adds a profile specific iodev to btio. */
static struct cras_ionode *add_profile_dev(struct cras_iodev *bt_iodev,
					   struct cras_iodev *dev,
					   enum cras_bt_device_profile profile)
{
	struct bt_node *n;
	struct bt_io *btio = (struct bt_io *)bt_iodev;

	n = (struct bt_node *)calloc(1, sizeof(*n));
	if (!n)
		return NULL;

	n->base.dev = bt_iodev;
	n->base.idx = btio->next_node_id++;
	n->base.type = CRAS_NODE_TYPE_BLUETOOTH;
	n->base.volume = 100;
	n->base.stable_id = dev->info.stable_id;
	n->base.stable_id_new = dev->info.stable_id_new;
	n->base.max_software_gain = 0;
	gettimeofday(&n->base.plugged_time, NULL);

	strcpy(n->base.name, dev->info.name);
	n->profile_dev = dev;
	n->profile = profile;

	cras_iodev_add_node(bt_iodev, &n->base);
	return &n->base;
}

/* Forces bt device to switch to use the given profile. Note that if
 * it has already been open for streaming, the new active profile will
 * take effect after the related btio(s) are reopened.
 */
static void bt_switch_to_profile(struct cras_bt_device *device,
				 enum cras_bt_device_profile profile)
{
	switch (profile) {
	case CRAS_BT_DEVICE_PROFILE_HFP_AUDIOGATEWAY:
	case CRAS_BT_DEVICE_PROFILE_HSP_AUDIOGATEWAY:
		cras_bt_device_set_active_profile(device,
				CRAS_BT_DEVICE_PROFILE_HSP_AUDIOGATEWAY |
				CRAS_BT_DEVICE_PROFILE_HFP_AUDIOGATEWAY);
		break;
	case CRAS_BT_DEVICE_PROFILE_A2DP_SOURCE:
		cras_bt_device_set_active_profile(device,
				CRAS_BT_DEVICE_PROFILE_A2DP_SOURCE);
		break;
	default:
		syslog(LOG_ERR, "Unexpect profile %u", profile);
		break;
	}
}

/* Checks if bt device is active for the given profile.
 */
static int device_using_profile(struct cras_bt_device *device,
				unsigned int profile)
{
	return cras_bt_device_get_active_profile(device) & profile;
}

/* Checks if the condition is met to switch to a different profile
 * when trying to set the format to btio before open it. Base on two
 * rules:
 * (1) Prefer to use A2DP for output since the audio quality is better.
 * (2) Must use HFP/HSP for input since A2DP doesn't support audio input.
 *
 * If the profile switch happens, return non-zero error code, otherwise
 * return zero.
 */
static int update_supported_formats(struct cras_iodev *iodev)
{
	struct bt_io *btio = (struct bt_io *)iodev;
	struct cras_iodev *dev = active_profile_dev(iodev);
	int rc, length, i;

	/* Force to use HFP if opening input dev. */
	if (device_using_profile(btio->device,
				 CRAS_BT_DEVICE_PROFILE_A2DP_SOURCE) &&
	    iodev->direction == CRAS_STREAM_INPUT) {
		bt_switch_to_profile(btio->device,
				     CRAS_BT_DEVICE_PROFILE_HFP_AUDIOGATEWAY);
		cras_bt_device_switch_profile_enable_dev(btio->device, iodev);
		return -EAGAIN;
	}

	if (dev->format == NULL) {
		dev->format = (struct cras_audio_format *)
				malloc(sizeof(*dev->format));
		*dev->format = *iodev->format;
	}

	if (dev->update_supported_formats) {
		rc = dev->update_supported_formats(dev);
		if (rc)
			return rc;
	}

	/* Fill in the supported rates and channel counts. */
	for (length = 0; dev->supported_rates[length]; length++);
	free(iodev->supported_rates);
	iodev->supported_rates = (size_t *)malloc(
			(length + 1) * sizeof(*iodev->supported_rates));
	for (i = 0; i < length + 1; i++)
		iodev->supported_rates[i] = dev->supported_rates[i];

	for (length = 0; dev->supported_channel_counts[length]; length++);
	iodev->supported_channel_counts = (size_t *)malloc(
		(length + 1) * sizeof(*iodev->supported_channel_counts));
	for (i = 0; i < length + 1; i++)
		iodev->supported_channel_counts[i] =
			dev->supported_channel_counts[i];

	for (length = 0; dev->supported_formats[length]; length++);
	iodev->supported_formats = (snd_pcm_format_t *)malloc(
		(length + 1) * sizeof(*iodev->supported_formats));
	for (i = 0; i < length + 1; i++)
		iodev->supported_formats[i] =
			dev->supported_formats[i];
	return 0;
}

static int open_dev(struct cras_iodev *iodev)
{
	int rc;
	struct cras_iodev *dev = active_profile_dev(iodev);
	if (!dev)
		return -EINVAL;

	/* Fill back the format iodev is using. */
	*dev->format = *iodev->format;

	rc = dev->open_dev(dev);
	if (rc) {
		/* Free format here to assure the update_supported_format
		 * callback will be called before any future open_dev call. */
		cras_iodev_free_format(iodev);
		return rc;
	}

	iodev->buffer_size = dev->buffer_size;
	iodev->min_buffer_level = dev->min_buffer_level;
	return 0;
}

static int close_dev(struct cras_iodev *iodev)
{
	struct bt_io *btio = (struct bt_io *)iodev;
	int rc;
	struct cras_iodev *dev = active_profile_dev(iodev);

	/* Force back to A2DP if closing HFP. */
	if (device_using_profile(btio->device,
			CRAS_BT_DEVICE_PROFILE_HSP_AUDIOGATEWAY |
			CRAS_BT_DEVICE_PROFILE_HFP_AUDIOGATEWAY) &&
	    iodev->direction == CRAS_STREAM_INPUT &&
	    cras_bt_device_has_a2dp(btio->device)) {
		cras_bt_device_set_active_profile(btio->device,
				CRAS_BT_DEVICE_PROFILE_A2DP_SOURCE);
		cras_bt_device_switch_profile(btio->device, iodev);
	}

	rc = dev->close_dev(dev);
	if (rc < 0)
		return rc;
	cras_iodev_free_format(iodev);
	return 0;
}

static void set_bt_volume(struct cras_iodev *iodev)
{
	struct cras_iodev *dev = active_profile_dev(iodev);

	if (dev->active_node)
		dev->active_node->volume = iodev->active_node->volume;

	/* The parent bt_iodev could set software_volume_needed flag for cases
	 * that software volume provides better experience across profiles
	 * (HFP and A2DP). Otherwise, use the profile specific implementation
	 * to adjust volume. */
	if (dev->set_volume && !iodev->software_volume_needed)
		dev->set_volume(dev);
}

static int frames_queued(const struct cras_iodev *iodev,
			 struct timespec *tstamp)
{
	struct cras_iodev *dev = active_profile_dev(iodev);
	if (!dev)
		return -EINVAL;
	return dev->frames_queued(dev, tstamp);
}

static int delay_frames(const struct cras_iodev *iodev)
{
	struct cras_iodev *dev = active_profile_dev(iodev);
	if (!dev)
		return -EINVAL;
	return dev->delay_frames(dev);
}

static int get_buffer(struct cras_iodev *iodev,
		      struct cras_audio_area **area,
		      unsigned *frames)
{
	struct cras_iodev *dev = active_profile_dev(iodev);
	if (!dev)
		return -EINVAL;
	return dev->get_buffer(dev, area, frames);
}

static int put_buffer(struct cras_iodev *iodev, unsigned nwritten)
{
	struct cras_iodev *dev = active_profile_dev(iodev);
	if (!dev)
		return -EINVAL;
	return dev->put_buffer(dev, nwritten);
}

static int flush_buffer(struct cras_iodev *iodev)
{
	struct cras_iodev *dev = active_profile_dev(iodev);
	if (!dev)
		return -EINVAL;
	return dev->flush_buffer(dev);
}

/* If the first private iodev doesn't match the active profile stored on
 * device, select to the correct private iodev.
 */
static void update_active_node(struct cras_iodev *iodev, unsigned node_idx,
			       unsigned dev_enabled)
{
	struct bt_io *btio = (struct bt_io *)iodev;
	struct cras_ionode *node;
	struct bt_node *active = (struct bt_node *)iodev->active_node;

	if (device_using_profile(btio->device, active->profile))
		return;

	/* Switch to the correct dev using active_profile. */
	DL_FOREACH(iodev->nodes, node) {
		struct bt_node *n = (struct bt_node *)node;
		if (n == active)
			continue;

		if (device_using_profile(btio->device, n->profile)) {
			active->profile = n->profile;
			active->profile_dev = n->profile_dev;

			/* Set volume for the new profile. */
			set_bt_volume(iodev);
		}
	}
}

struct cras_iodev *cras_bt_io_create(struct cras_bt_device *device,
				     struct cras_iodev *dev,
				     enum cras_bt_device_profile profile)
{
	int err;
	struct bt_io *btio;
	struct cras_iodev *iodev;
	struct cras_ionode *node;
	struct bt_node *active;

	if (!dev)
		return NULL;

	btio = (struct bt_io *)calloc(1, sizeof(*btio));
	if (!btio)
		goto error;
	btio->device = device;

	iodev = &btio->base;
	iodev->direction = dev->direction;
	strcpy(iodev->info.name, dev->info.name);
	iodev->info.stable_id = dev->info.stable_id;
	iodev->info.stable_id_new = dev->info.stable_id_new;

	iodev->open_dev = open_dev;
	iodev->frames_queued = frames_queued;
	iodev->delay_frames = delay_frames;
	iodev->get_buffer = get_buffer;
	iodev->put_buffer = put_buffer;
	iodev->flush_buffer = flush_buffer;
	iodev->close_dev = close_dev;
	iodev->update_supported_formats = update_supported_formats;
	iodev->update_active_node = update_active_node;
	iodev->software_volume_needed = 1;
	iodev->set_volume = set_bt_volume;
	iodev->no_stream = cras_iodev_default_no_stream_playback;

	/* Create the dummy node set to plugged so it's the only node exposed
	 * to UI, and point it to the first profile dev. */
	active = (struct bt_node *)calloc(1, sizeof(*active));
	if (!active)
		return NULL;
	active->base.dev = iodev;
	active->base.idx = btio->next_node_id++;
	active->base.type = CRAS_NODE_TYPE_BLUETOOTH;
	active->base.volume = 100;
	active->base.plugged = 1;
	active->base.stable_id = SuperFastHash(
		cras_bt_device_object_path(device),
		strlen(cras_bt_device_object_path(device)),
		strlen(cras_bt_device_object_path(device)));
	active->base.stable_id_new = active->base.stable_id;
	active->profile = profile;
	active->profile_dev = dev;
	gettimeofday(&active->base.plugged_time, NULL);
	strcpy(active->base.name, dev->info.name);
	/* The node name exposed to UI should be a valid UTF8 string. */
	if (!is_utf8_string(active->base.name))
		strcpy(active->base.name, DEFAULT_BT_DEVICE_NAME);
	cras_iodev_add_node(iodev, &active->base);

	node = add_profile_dev(&btio->base, dev, profile);
	if (node == NULL)
		goto error;

	/* Default active profile to a2dp whenever it's allowed. */
	if (!cras_bt_device_get_active_profile(device) ||
	    (profile == CRAS_BT_DEVICE_PROFILE_A2DP_SOURCE &&
	     cras_bt_device_can_switch_to_a2dp(device)))
		bt_switch_to_profile(device, profile);

	if (iodev->direction == CRAS_STREAM_OUTPUT)
		err = cras_iodev_list_add_output(iodev);
	else
		err = cras_iodev_list_add_input(iodev);
	if (err)
		goto error;

	cras_iodev_set_active_node(iodev, &active->base);
	return &btio->base;

error:
	if (btio)
		free(btio);
	return NULL;
}

void cras_bt_io_destroy(struct cras_iodev *bt_iodev)
{
	int rc;
	struct bt_io *btio = (struct bt_io *)bt_iodev;
	struct cras_ionode *node;
	struct bt_node *n;

	if (bt_iodev->direction == CRAS_STREAM_OUTPUT)
		rc = cras_iodev_list_rm_output(bt_iodev);
	else
		rc = cras_iodev_list_rm_input(bt_iodev);
	if (rc == -EBUSY)
		return;

	DL_FOREACH(bt_iodev->nodes, node) {
		n = (struct bt_node *)node;
		cras_iodev_rm_node(bt_iodev, node);
		free(n);
	}
	free(btio);
}

struct cras_ionode *cras_bt_io_get_profile(struct cras_iodev *bt_iodev,
					   enum cras_bt_device_profile profile)
{
	struct cras_ionode *node;
	DL_FOREACH(bt_iodev->nodes, node) {
		struct bt_node *n = (struct bt_node *)node;
		if (n->profile & profile)
			return node;
	}
	return NULL;
}

int cras_bt_io_append(struct cras_iodev *bt_iodev,
		      struct cras_iodev *dev,
		      enum cras_bt_device_profile profile)
{
	struct cras_ionode *node;
	struct bt_io *btio = (struct bt_io *)bt_iodev;

	if (cras_bt_io_get_profile(bt_iodev, profile))
		return -EEXIST;

	node = add_profile_dev(bt_iodev, dev, profile);
	if (!node)
		return -ENOMEM;

	if (profile == CRAS_BT_DEVICE_PROFILE_A2DP_SOURCE &&
	    cras_bt_device_can_switch_to_a2dp(btio->device)) {
		bt_switch_to_profile(btio->device,
				     CRAS_BT_DEVICE_PROFILE_A2DP_SOURCE);
		cras_bt_device_switch_profile(btio->device, bt_iodev);
		syslog(LOG_ERR, "Switch to A2DP on append");
	}
	return 0;
}

int cras_bt_io_on_profile(struct cras_iodev *bt_iodev,
			  enum cras_bt_device_profile profile)
{
	struct bt_node *btnode = (struct bt_node *)bt_iodev->active_node;
	return !!(profile & btnode->profile);
}

int cras_bt_io_update_buffer_size(struct cras_iodev *bt_iodev)
{
	struct cras_iodev *dev = active_profile_dev(bt_iodev);
	if (!dev)
		return -EINVAL;

	bt_iodev->buffer_size = dev->buffer_size;
	return 0;
}

unsigned int cras_bt_io_try_remove(struct cras_iodev *bt_iodev,
				   struct cras_iodev *dev)
{
	struct cras_ionode *node;
	struct bt_node *active, *btnode;
	unsigned int try_profile = 0;

	active = (struct bt_node *)bt_iodev->active_node;

	if (active->profile_dev == dev) {
		DL_FOREACH(bt_iodev->nodes, node) {
			btnode = (struct bt_node *)node;
			/* Skip the active node and the node we're trying
			 * to remove. */
			if (btnode == active || btnode->profile_dev == dev)
				continue;
			try_profile = btnode->profile;
			break;
		}
	} else {
		try_profile = active->profile;
	}
	return try_profile;
}

int cras_bt_io_remove(struct cras_iodev *bt_iodev,
		      struct cras_iodev *dev)
{
	struct cras_ionode *node;
	struct bt_node *btnode;

	DL_FOREACH(bt_iodev->nodes, node) {
		btnode = (struct bt_node *)node;
		if (btnode->profile_dev != dev)
			continue;

		/* If this is the active node, reset it. Otherwise delete
		 * this node. */
		if (node == bt_iodev->active_node) {
			btnode->profile_dev = NULL;
			btnode->profile = 0;
		} else {
			DL_DELETE(bt_iodev->nodes, node);
			free(node);
		}
	}

	/* The node of active profile could have been removed, update it.
	 * Return err when fail to locate the active profile dev. */
	update_active_node(bt_iodev, 0, 1);
	btnode = (struct bt_node *)bt_iodev->active_node;
	if ((btnode->profile == 0) || (btnode->profile_dev == NULL))
		return -EINVAL;

	return 0;
}
