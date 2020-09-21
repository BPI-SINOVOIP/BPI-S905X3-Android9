/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <pthread.h>
#include <sys/param.h>
#include <syslog.h>

#include "byte_buffer.h"
#include "cras_audio_area.h"
#include "cras_config.h"
#include "cras_iodev.h"
#include "cras_iodev_list.h"
#include "cras_types.h"
#include "cras_util.h"
#include "sfh.h"
#include "utlist.h"

#define LOOPBACK_BUFFER_SIZE 8192

static const char *loopdev_names[LOOPBACK_NUM_TYPES] = {
	"Post Mix Pre DSP Loopback",
	"Post DSP Loopback",
};

static size_t loopback_supported_rates[] = {
	48000, 0
};

static size_t loopback_supported_channel_counts[] = {
	2, 0
};

static snd_pcm_format_t loopback_supported_formats[] = {
	SND_PCM_FORMAT_S16_LE,
	0
};

/* loopack iodev.  Keep state of a loopback device.
 *    sample_buffer - Pointer to sample buffer.
 */
struct loopback_iodev {
	struct cras_iodev base;
	enum CRAS_LOOPBACK_TYPE loopback_type;
	struct timespec last_filled;
	struct byte_buffer *sample_buffer;
};

static int sample_hook(const uint8_t *frames, unsigned int nframes,
		       const struct cras_audio_format *fmt,
		       void *cb_data)
{
	struct loopback_iodev *loopdev = (struct loopback_iodev *)cb_data;
	struct byte_buffer *sbuf = loopdev->sample_buffer;
	unsigned int frame_bytes = cras_get_format_bytes(fmt);
	unsigned int frames_to_copy, bytes_to_copy;
	struct cras_iodev *edev = cras_iodev_list_get_first_enabled_iodev(
			CRAS_STREAM_OUTPUT);

	/* If there's no active streams, the logic in frames_queued will fill
	 * zeros for loopback capture, do not accept zeros for draining device.
	 */
	if (!edev || !edev->streams)
		return 0;

	frames_to_copy = MIN(buf_writable_bytes(sbuf) / frame_bytes, nframes);
	if (!frames_to_copy)
		return 0;

	bytes_to_copy = frames_to_copy * frame_bytes;
	memcpy(buf_write_pointer(sbuf), frames, bytes_to_copy);
	buf_increment_write(sbuf, bytes_to_copy);
	clock_gettime(CLOCK_MONOTONIC_RAW, &loopdev->last_filled);

	return frames_to_copy;
}

static void register_loopback_hook(enum CRAS_LOOPBACK_TYPE loopback_type,
				   struct cras_iodev *iodev,
				   loopback_hook_t hook, void *cb_data)
{
	if (!iodev) {
		syslog(LOG_ERR, "Failed to register loopback hook.");
		return;
	}

	if (loopback_type == LOOPBACK_POST_MIX_PRE_DSP)
		cras_iodev_register_pre_dsp_hook(iodev, hook, cb_data);
	else if (loopback_type == LOOPBACK_POST_DSP)
		cras_iodev_register_post_dsp_hook(iodev, hook, cb_data);
}

static void device_enabled_hook(struct cras_iodev *iodev, int enabled,
				void *cb_data)
{
	struct loopback_iodev *loopdev = (struct loopback_iodev *)cb_data;
	struct cras_iodev *edev;

	if (iodev->direction != CRAS_STREAM_OUTPUT)
		return;

	if (!enabled) {
		/* Unregister loopback hook from disabled iodev. */
		register_loopback_hook(loopdev->loopback_type, iodev, NULL,
				       NULL);
	} else {
		/* Register loopback hook onto first enabled iodev. */
		edev = cras_iodev_list_get_first_enabled_iodev(
				CRAS_STREAM_OUTPUT);
		register_loopback_hook(loopdev->loopback_type, edev,
				       sample_hook, cb_data);
	}
}

/*
 * iodev callbacks.
 */

static int frames_queued(const struct cras_iodev *iodev,
			 struct timespec *hw_tstamp)
{
	struct loopback_iodev *loopdev = (struct loopback_iodev *)iodev;
	struct byte_buffer *sbuf = loopdev->sample_buffer;
	unsigned int frame_bytes = cras_get_format_bytes(iodev->format);
	struct cras_iodev *edev = cras_iodev_list_get_first_enabled_iodev(
			CRAS_STREAM_OUTPUT);

	if (!edev || !edev->streams) {
		unsigned int frames_since_last, frames_to_fill, bytes_to_fill;

		frames_since_last = cras_frames_since_time(
				&loopdev->last_filled,
				iodev->format->frame_rate);
		frames_to_fill = MIN(buf_writable_bytes(sbuf) / frame_bytes,
				     frames_since_last);
		if (frames_to_fill > 0) {
			bytes_to_fill = frames_to_fill * frame_bytes;
			memset(buf_write_pointer(sbuf), 0, bytes_to_fill);
			buf_increment_write(sbuf, bytes_to_fill);
			clock_gettime(CLOCK_MONOTONIC_RAW,
				      &loopdev->last_filled);
		}
	}
	*hw_tstamp = loopdev->last_filled;
	return buf_queued_bytes(sbuf) / frame_bytes;
}

static int delay_frames(const struct cras_iodev *iodev)
{
	struct timespec tstamp;

	return frames_queued(iodev, &tstamp);
}

static int close_record_dev(struct cras_iodev *iodev)
{
	struct loopback_iodev *loopdev = (struct loopback_iodev *)iodev;
	struct byte_buffer *sbuf = loopdev->sample_buffer;
	struct cras_iodev *edev;

	cras_iodev_free_format(iodev);
	cras_iodev_free_audio_area(iodev);
	buf_reset(sbuf);

	edev = cras_iodev_list_get_first_enabled_iodev(CRAS_STREAM_OUTPUT);
	register_loopback_hook(loopdev->loopback_type, edev, NULL, NULL);
	cras_iodev_list_set_device_enabled_callback(NULL, NULL);

	return 0;
}

static int open_record_dev(struct cras_iodev *iodev)
{
	struct loopback_iodev *loopdev = (struct loopback_iodev *)iodev;
	struct cras_iodev *edev;

	cras_iodev_init_audio_area(iodev, iodev->format->num_channels);
	clock_gettime(CLOCK_MONOTONIC_RAW, &loopdev->last_filled);

	edev = cras_iodev_list_get_first_enabled_iodev(CRAS_STREAM_OUTPUT);
	register_loopback_hook(loopdev->loopback_type, edev, sample_hook,
			       (void *)iodev);
	cras_iodev_list_set_device_enabled_callback(device_enabled_hook,
						    (void *)iodev);

	return 0;
}

static int get_record_buffer(struct cras_iodev *iodev,
		      struct cras_audio_area **area,
		      unsigned *frames)
{
	struct loopback_iodev *loopdev = (struct loopback_iodev *)iodev;
	struct byte_buffer *sbuf = loopdev->sample_buffer;
	unsigned int frame_bytes = cras_get_format_bytes(iodev->format);
	unsigned int avail_frames = buf_readable_bytes(sbuf) / frame_bytes;

	*frames = MIN(avail_frames, *frames);
	iodev->area->frames = *frames;
	cras_audio_area_config_buf_pointers(iodev->area, iodev->format,
					    buf_read_pointer(sbuf));
	*area = iodev->area;

	return 0;
}

static int put_record_buffer(struct cras_iodev *iodev, unsigned nframes)
{
	struct loopback_iodev *loopdev = (struct loopback_iodev *)iodev;
	struct byte_buffer *sbuf = loopdev->sample_buffer;
	unsigned int frame_bytes = cras_get_format_bytes(iodev->format);

	buf_increment_read(sbuf, nframes * frame_bytes);

	return 0;
}

static int flush_record_buffer(struct cras_iodev *iodev)
{
	struct loopback_iodev *loopdev = (struct loopback_iodev *)iodev;
	struct byte_buffer *sbuf = loopdev->sample_buffer;
	unsigned int queued_bytes = buf_queued_bytes(sbuf);
	buf_increment_read(sbuf, queued_bytes);
	return 0;
}

static void update_active_node(struct cras_iodev *iodev, unsigned node_idx,
			       unsigned dev_enabled)
{
}

static struct cras_iodev *create_loopback_iodev(enum CRAS_LOOPBACK_TYPE type)
{
	struct loopback_iodev *loopback_iodev;
	struct cras_iodev *iodev;

	loopback_iodev = calloc(1, sizeof(*loopback_iodev));
	if (loopback_iodev == NULL)
		return NULL;

	loopback_iodev->sample_buffer = byte_buffer_create(1024*16*4);
	if (loopback_iodev->sample_buffer == NULL) {
		free(loopback_iodev);
		return NULL;
	}

	loopback_iodev->loopback_type = type;

	iodev = &loopback_iodev->base;
	iodev->direction = CRAS_STREAM_INPUT;
	snprintf(iodev->info.name, ARRAY_SIZE(iodev->info.name), "%s",
		 loopdev_names[type]);
	iodev->info.name[ARRAY_SIZE(iodev->info.name) - 1] = '\0';
	iodev->info.stable_id = SuperFastHash(iodev->info.name,
					      strlen(iodev->info.name),
					      strlen(iodev->info.name));
	iodev->info.stable_id_new = iodev->info.stable_id;

	iodev->supported_rates = loopback_supported_rates;
	iodev->supported_channel_counts = loopback_supported_channel_counts;
	iodev->supported_formats = loopback_supported_formats;
	iodev->buffer_size = LOOPBACK_BUFFER_SIZE;

	iodev->frames_queued = frames_queued;
	iodev->delay_frames = delay_frames;
	iodev->update_active_node = update_active_node;
	iodev->open_dev = open_record_dev;
	iodev->close_dev = close_record_dev;
	iodev->get_buffer = get_record_buffer;
	iodev->put_buffer = put_record_buffer;
	iodev->flush_buffer = flush_record_buffer;

	return iodev;
}

/*
 * Exported Interface.
 */

struct cras_iodev *loopback_iodev_create(enum CRAS_LOOPBACK_TYPE type)
{
	struct cras_iodev *iodev;
	struct cras_ionode *node;
	enum CRAS_NODE_TYPE node_type;

	switch (type) {
	case LOOPBACK_POST_MIX_PRE_DSP:
		node_type = CRAS_NODE_TYPE_POST_MIX_PRE_DSP;
		break;
	case LOOPBACK_POST_DSP:
		node_type = CRAS_NODE_TYPE_POST_DSP;
		break;
	default:
		return NULL;
	}

	iodev = create_loopback_iodev(type);
	if (iodev == NULL)
		return NULL;

	/* Create a dummy ionode */
	node = (struct cras_ionode *)calloc(1, sizeof(*node));
	node->dev = iodev;
	node->type = node_type;
	node->plugged = 1;
	node->volume = 100;
	node->stable_id = iodev->info.stable_id;
	node->stable_id_new = iodev->info.stable_id_new;
	node->software_volume_needed = 0;
	node->max_software_gain = 0;
	strcpy(node->name, loopdev_names[type]);
	cras_iodev_add_node(iodev, node);
	cras_iodev_set_active_node(iodev, node);

	cras_iodev_list_add_input(iodev);

	return iodev;
}

void loopback_iodev_destroy(struct cras_iodev *iodev)
{
	struct loopback_iodev *loopdev = (struct loopback_iodev *)iodev;
	struct byte_buffer *sbuf = loopdev->sample_buffer;

	cras_iodev_list_rm_input(iodev);

	byte_buffer_destroy(sbuf);
	free(loopdev);
}
