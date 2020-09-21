/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <pthread.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/time.h>
#include <syslog.h>
#include <time.h>

#include "audio_thread.h"
#include "audio_thread_log.h"
#include "buffer_share.h"
#include "cras_audio_area.h"
#include "cras_device_monitor.h"
#include "cras_dsp.h"
#include "cras_dsp_pipeline.h"
#include "cras_fmt_conv.h"
#include "cras_iodev.h"
#include "cras_iodev_list.h"
#include "cras_mix.h"
#include "cras_ramp.h"
#include "cras_rstream.h"
#include "cras_system_state.h"
#include "cras_util.h"
#include "dev_stream.h"
#include "utlist.h"
#include "rate_estimator.h"
#include "softvol_curve.h"

static const float RAMP_UNMUTE_DURATION_SECS = 0.5;
static const float RAMP_NEW_STREAM_DURATION_SECS = 0.01;
static const float RAMP_MUTE_DURATION_SECS = 0.1;

static const struct timespec rate_estimation_window_sz = {
	20, 0 /* 20 sec. */
};
static const double rate_estimation_smooth_factor = 0.9f;

static void cras_iodev_alloc_dsp(struct cras_iodev *iodev);

static int default_no_stream_playback(struct cras_iodev *odev)
{
	int rc;
	unsigned int hw_level, fr_to_write;
	unsigned int target_hw_level = odev->min_cb_level * 2;
	struct timespec hw_tstamp;

	/* The default action for no stream playback is to fill zeros. */
	rc = cras_iodev_frames_queued(odev, &hw_tstamp);
	if (rc < 0)
		return rc;
	hw_level = rc;

	ATLOG(atlog, AUDIO_THREAD_ODEV_DEFAULT_NO_STREAMS,
	      odev->info.idx, hw_level, target_hw_level);

	fr_to_write = cras_iodev_buffer_avail(odev, hw_level);
	if (hw_level <= target_hw_level) {
		fr_to_write = MIN(target_hw_level - hw_level, fr_to_write);
		return cras_iodev_fill_odev_zeros(odev, fr_to_write);
	}
	return 0;
}

static int cras_iodev_start(struct cras_iodev *iodev)
{
	int rc;
	if (!cras_iodev_is_open(iodev))
		return -EPERM;
	if (!iodev->start) {
		syslog(LOG_ERR,
		       "start called on device %s not supporting start ops",
		       iodev->info.name);
		return -EINVAL;
	}
	rc = iodev->start(iodev);
	if (rc)
		return rc;
	iodev->state = CRAS_IODEV_STATE_NORMAL_RUN;
	return 0;
}

/* Gets the number of frames ready for this device to play.
 * It is the minimum number of available samples in dev_streams.
 */
static unsigned int dev_playback_frames(struct cras_iodev* odev)
{
	struct dev_stream *curr;
	int frames = 0;

	DL_FOREACH(odev->streams, curr) {
		int dev_frames;

		/* If this is a single output dev stream, updates the latest
		 * number of frames for playback. */
		if (dev_stream_attached_devs(curr) == 1)
			dev_stream_update_frames(curr);

		dev_frames = dev_stream_playback_frames(curr);
		/* Do not handle stream error or end of draining in this
		 * function because they should be handled in write_streams. */
		if (dev_frames < 0)
			continue;
		if (!dev_frames) {
			if(cras_rstream_get_is_draining(curr->stream))
				continue;
			else
				return 0;
		}
		if (frames == 0)
			frames = dev_frames;
		else
			frames = MIN(dev_frames, frames);
	}
	return frames;
}

/* Let device enter/leave no stream playback.
 * Args:
 *    iodev[in] - The output device.
 *    enable[in] - 1 to enter no stream playback, 0 to leave.
 * Returns:
 *    0 on success. Negative error code on failure.
 */
static int cras_iodev_no_stream_playback_transition(struct cras_iodev *odev,
						    int enable)
{
	int rc;

	if (odev->direction != CRAS_STREAM_OUTPUT)
		return -EINVAL;

	/* This function is for transition between normal run and
	 * no stream run state.
	 */
	if ((odev->state != CRAS_IODEV_STATE_NORMAL_RUN) &&
	    (odev->state != CRAS_IODEV_STATE_NO_STREAM_RUN))
		return -EINVAL;

	if (enable) {
		ATLOG(atlog, AUDIO_THREAD_ODEV_NO_STREAMS,
		      odev->info.idx, 0, 0);
	} else {
		ATLOG(atlog, AUDIO_THREAD_ODEV_LEAVE_NO_STREAMS,
		      odev->info.idx, 0, 0);
	}

	rc = odev->no_stream(odev, enable);
	if (rc < 0)
		return rc;
	if (enable)
		odev->state = CRAS_IODEV_STATE_NO_STREAM_RUN;
	else
		odev->state = CRAS_IODEV_STATE_NORMAL_RUN;
	return 0;
}

/* Determines if the output device should mute. It considers system mute,
 * system volume, and active node volume on the device. */
static int output_should_mute(struct cras_iodev *odev)
{
	/* System mute has highest priority. */
	if (cras_system_get_mute())
		return 1;

	/* consider system volume and active node volume. */
	return cras_iodev_is_zero_volume(odev);
}

int cras_iodev_is_zero_volume(const struct cras_iodev *odev)
{
	size_t system_volume;
	unsigned int adjusted_node_volume;

	system_volume = cras_system_get_volume();
	if (odev->active_node) {
		adjusted_node_volume = cras_iodev_adjust_node_volume(
				odev->active_node, system_volume);
		return (adjusted_node_volume == 0);
	}
	return (system_volume == 0);
}

/* Output device state transition diagram:
 *
 *                           ----------------
 *  -------------<-----------| S0  Closed   |------<-------.
 *  |                        ----------------              |
 *  |                           |   iodev_list enables     |
 *  |                           |   device and adds to     |
 *  |                           V   audio thread           | iodev_list removes
 *  |                        ----------------              | device from
 *  |                        | S1  Open     |              | audio_thread and
 *  |                        ----------------              | closes device
 *  | Device with dummy start       |                      |
 *  | ops transits into             | Sample is ready      |
 *  | no stream state right         V                      |
 *  | after open.            ----------------              |
 *  |                        | S2  Normal   |              |
 *  |                        ----------------              |
 *  |                           |        ^                 |
 *  |       There is no stream  |        | Sample is ready |
 *  |                           V        |                 |
 *  |                        ----------------              |
 *  ------------->-----------| S3 No Stream |------->------
 *                           ----------------
 *
 *  Device in open_devs can be in one of S1, S2, S3.
 *
 * cras_iodev_output_event_sample_ready change device state from S1 or S3 into
 * S2.
 */
static int cras_iodev_output_event_sample_ready(struct cras_iodev *odev)
{
	if (odev->state == CRAS_IODEV_STATE_OPEN ||
	    odev->state == CRAS_IODEV_STATE_NO_STREAM_RUN) {
		/* Starts ramping up if device should not be muted.
		 * Both mute and volume are taken into consideration.
		 */
		if (odev->ramp && !output_should_mute(odev))
			cras_iodev_start_ramp(
				odev,
				CRAS_IODEV_RAMP_REQUEST_UP_START_PLAYBACK);
	}

	if (odev->state == CRAS_IODEV_STATE_OPEN) {
		/* S1 => S2:
		 * If device is not started yet, and there is sample ready from
		 * stream, fill 1 min_cb_level of zeros first and fill sample
		 * from stream later.
		 * Starts the device here to finish state transition. */
		cras_iodev_fill_odev_zeros(odev, odev->min_cb_level);
		ATLOG(atlog, AUDIO_THREAD_ODEV_START,
				odev->info.idx, odev->min_cb_level, 0);
		return cras_iodev_start(odev);
	} else if (odev->state == CRAS_IODEV_STATE_NO_STREAM_RUN) {
		/* S3 => S2:
		 * Device in no stream state get sample ready. Leave no stream
		 * state and transit to normal run state.*/
		return cras_iodev_no_stream_playback_transition(odev, 0);
	} else {
		syslog(LOG_ERR,
		       "Device %s in state %d received sample ready event",
		       odev->info.name, odev->state);
		return -EINVAL;
	}
	return 0;
}

/*
 * Exported Interface.
 */

/* Finds the supported sample rate that best suits the requested rate, "rrate".
 * Exact matches have highest priority, then integer multiples, then the default
 * rate for the device. */
static size_t get_best_rate(struct cras_iodev *iodev, size_t rrate)
{
	size_t i;
	size_t best;

	if (iodev->supported_rates[0] == 0) /* No rates supported */
		return 0;

	for (i = 0, best = 0; iodev->supported_rates[i] != 0; i++) {
		if (rrate == iodev->supported_rates[i] &&
		    rrate >= 44100)
			return rrate;
		if (best == 0 && (rrate % iodev->supported_rates[i] == 0 ||
				  iodev->supported_rates[i] % rrate == 0))
			best = iodev->supported_rates[i];
	}

	if (best)
		return best;
	return iodev->supported_rates[0];
}

/* Finds the best match for the channel count.  The following match rules
 * will apply in order and return the value once matched:
 * 1. Match the exact given channel count.
 * 2. Match the preferred channel count.
 * 3. The first channel count in the list.
 */
static size_t get_best_channel_count(struct cras_iodev *iodev, size_t count)
{
	static const size_t preferred_channel_count = 2;
	size_t i;

	assert(iodev->supported_channel_counts[0] != 0);

	for (i = 0; iodev->supported_channel_counts[i] != 0; i++) {
		if (iodev->supported_channel_counts[i] == count)
			return count;
	}

	/* If provided count is not supported, search for preferred
	 * channel count to which we're good at converting.
	 */
	for (i = 0; iodev->supported_channel_counts[i] != 0; i++) {
		if (iodev->supported_channel_counts[i] ==
				preferred_channel_count)
			return preferred_channel_count;
	}

	return iodev->supported_channel_counts[0];
}

/* finds the best match for the current format. If no exact match is
 * found, use the first. */
static snd_pcm_format_t get_best_pcm_format(struct cras_iodev *iodev,
					    snd_pcm_format_t fmt)
{
	size_t i;

	for (i = 0; iodev->supported_formats[i] != 0; i++) {
		if (fmt == iodev->supported_formats[i])
			return fmt;
	}

	return iodev->supported_formats[0];
}

/* Set default channel layout to an iodev. */
static void set_default_channel_layout(struct cras_iodev *iodev)
{
	int8_t default_layout[CRAS_CH_MAX];
	size_t i;

	for (i = 0; i < CRAS_CH_MAX; i++)
		default_layout[i] = i < iodev->format->num_channels ? i : -1;

	cras_audio_format_set_channel_layout(iodev->format, default_layout);
	cras_audio_format_set_channel_layout(iodev->ext_format, default_layout);
}

/* Applies the DSP to the samples for the iodev if applicable. */
static void apply_dsp(struct cras_iodev *iodev, uint8_t *buf, size_t frames)
{
	struct cras_dsp_context *ctx;
	struct pipeline *pipeline;

	ctx = iodev->dsp_context;
	if (!ctx)
		return;

	pipeline = cras_dsp_get_pipeline(ctx);
	if (!pipeline)
		return;

	cras_dsp_pipeline_apply(pipeline,
				buf,
				frames);

	cras_dsp_put_pipeline(ctx);
}

static void cras_iodev_free_dsp(struct cras_iodev *iodev)
{
	if (iodev->dsp_context) {
		cras_dsp_context_free(iodev->dsp_context);
		iodev->dsp_context = NULL;
	}
}

/* Modifies the number of channels in device format to the one that will be
 * presented to the device after any channel changes from the DSP. */
static inline void adjust_dev_channel_for_dsp(const struct cras_iodev *iodev)
{
	struct cras_dsp_context *ctx = iodev->dsp_context;

	if (!ctx || !cras_dsp_get_pipeline(ctx))
		return;

	if (iodev->direction == CRAS_STREAM_OUTPUT) {
		iodev->format->num_channels =
			cras_dsp_num_output_channels(ctx);
		iodev->ext_format->num_channels =
			cras_dsp_num_input_channels(ctx);
	} else {
		iodev->format->num_channels =
			cras_dsp_num_input_channels(ctx);
		iodev->ext_format->num_channels =
			cras_dsp_num_output_channels(ctx);
	}

	cras_dsp_put_pipeline(ctx);
}

/* Updates channel layout based on the number of channels set by a
 * client stream. When successful we need to update the new channel
 * layout to ext_format, otherwise we should set a default value
 * to both format and ext_format.
 */
static void update_channel_layout(struct cras_iodev *iodev)
{
	int rc;

	/*
	 * Output devices like internal speakers and headphones are 2-channel
	 * and do not need to update channel layout.
	 * For HDMI and USB devices that might have more than 2 channels, update
	 * channel layout only if more than 2 channel is requested.
	 */
	if (iodev->direction == CRAS_STREAM_OUTPUT &&
	    iodev->format->num_channels <= 2) {
		set_default_channel_layout(iodev);
		return;
	}

	if (iodev->update_channel_layout == NULL)
		return;

	rc = iodev->update_channel_layout(iodev);
	if (rc < 0) {
		set_default_channel_layout(iodev);
	} else {
		cras_audio_format_set_channel_layout(
				iodev->ext_format,
				iodev->format->channel_layout);
	}
}

int cras_iodev_set_format(struct cras_iodev *iodev,
			  const struct cras_audio_format *fmt)
{
	size_t actual_rate, actual_num_channels;
	snd_pcm_format_t actual_format;
	int rc;

	/* If this device isn't already using a format, try to match the one
	 * requested in "fmt". */
	if (iodev->format == NULL) {
		iodev->format = malloc(sizeof(struct cras_audio_format));
		iodev->ext_format = malloc(sizeof(struct cras_audio_format));
		if (!iodev->format || !iodev->ext_format)
			return -ENOMEM;
		*iodev->format = *fmt;
		*iodev->ext_format = *fmt;

		if (iodev->update_supported_formats) {
			rc = iodev->update_supported_formats(iodev);
			if (rc) {
				syslog(LOG_ERR, "Failed to update formats");
				goto error;
			}
		}

		/* Finds the actual rate of device before allocating DSP
		 * because DSP needs to use the rate of device, not rate of
		 * stream. */
		actual_rate = get_best_rate(iodev, fmt->frame_rate);
		iodev->format->frame_rate = actual_rate;
		iodev->ext_format->frame_rate = actual_rate;

		cras_iodev_alloc_dsp(iodev);
		if (iodev->dsp_context)
			adjust_dev_channel_for_dsp(iodev);

		actual_num_channels = get_best_channel_count(iodev,
					iodev->format->num_channels);
		actual_format = get_best_pcm_format(iodev, fmt->format);
		if (actual_rate == 0 || actual_num_channels == 0 ||
		    actual_format == 0) {
			/* No compatible frame rate found. */
			rc = -EINVAL;
			goto error;
		}
		iodev->format->format = actual_format;
		iodev->ext_format->format = actual_format;
		if (iodev->format->num_channels != actual_num_channels) {
			/* If the DSP for this device doesn't match, drop it. */
			iodev->format->num_channels = actual_num_channels;
			iodev->ext_format->num_channels = actual_num_channels;
			cras_iodev_free_dsp(iodev);
		}

		update_channel_layout(iodev);

		if (!iodev->rate_est)
			iodev->rate_est = rate_estimator_create(
						actual_rate,
						&rate_estimation_window_sz,
						rate_estimation_smooth_factor);
		else
			rate_estimator_reset_rate(iodev->rate_est, actual_rate);
	}

	return 0;

error:
	free(iodev->format);
	free(iodev->ext_format);
	iodev->format = NULL;
	iodev->ext_format = NULL;
	return rc;
}

void cras_iodev_update_dsp(struct cras_iodev *iodev)
{
	char swap_lr_disabled = 1;

	if (!iodev->dsp_context)
		return;

	cras_dsp_set_variable_string(iodev->dsp_context, "dsp_name",
				     iodev->dsp_name ? : "");

	if (iodev->active_node && iodev->active_node->left_right_swapped)
		swap_lr_disabled = 0;

	cras_dsp_set_variable_boolean(iodev->dsp_context, "swap_lr_disabled",
				      swap_lr_disabled);

	cras_dsp_load_pipeline(iodev->dsp_context);
}


int cras_iodev_dsp_set_swap_mode_for_node(struct cras_iodev *iodev,
					   struct cras_ionode *node, int enable)
{
	if (node->left_right_swapped == enable)
		return 0;

	/* Sets left_right_swapped property on the node. It will be used
	 * when cras_iodev_update_dsp is called. */
	node->left_right_swapped = enable;

	/* Possibly updates dsp if the node is active on the device and there
	 * is dsp context. If dsp context is not created yet,
	 * cras_iodev_update_dsp returns right away. */
	if (iodev->active_node == node)
		cras_iodev_update_dsp(iodev);
	return 0;
}

void cras_iodev_free_format(struct cras_iodev *iodev)
{
	free(iodev->format);
	free(iodev->ext_format);
	iodev->format = NULL;
	iodev->ext_format = NULL;
}


void cras_iodev_init_audio_area(struct cras_iodev *iodev,
				int num_channels)
{
	if (iodev->area)
		cras_iodev_free_audio_area(iodev);

	iodev->area = cras_audio_area_create(num_channels);
	cras_audio_area_config_channels(iodev->area, iodev->format);
}

void cras_iodev_free_audio_area(struct cras_iodev *iodev)
{
	if (!iodev->area)
		return;

	cras_audio_area_destroy(iodev->area);
	iodev->area = NULL;
}

void cras_iodev_free_resources(struct cras_iodev *iodev)
{
	cras_iodev_free_dsp(iodev);
	rate_estimator_destroy(iodev->rate_est);
	if (iodev->ramp)
		cras_ramp_destroy(iodev->ramp);
}

static void cras_iodev_alloc_dsp(struct cras_iodev *iodev)
{
	const char *purpose;

	if (iodev->direction == CRAS_STREAM_OUTPUT)
		purpose = "playback";
	else
		purpose = "capture";

	cras_iodev_free_dsp(iodev);
	iodev->dsp_context = cras_dsp_context_new(iodev->format->frame_rate,
						  purpose);
	cras_iodev_update_dsp(iodev);
}

void cras_iodev_fill_time_from_frames(size_t frames,
				      size_t frame_rate,
				      struct timespec *ts)
{
	uint64_t to_play_usec;

	ts->tv_sec = 0;
	/* adjust sleep time to target our callback threshold */
	to_play_usec = (uint64_t)frames * 1000000L / (uint64_t)frame_rate;

	while (to_play_usec > 1000000) {
		ts->tv_sec++;
		to_play_usec -= 1000000;
	}
	ts->tv_nsec = to_play_usec * 1000;
}

/* This is called when a node is plugged/unplugged */
static void plug_node(struct cras_ionode *node, int plugged)
{
	if (node->plugged == plugged)
		return;
	node->plugged = plugged;
	if (plugged) {
		gettimeofday(&node->plugged_time, NULL);
	} else if (node == node->dev->active_node) {
		cras_iodev_list_disable_dev(node->dev);
	}
	cras_iodev_list_notify_nodes_changed();
}

static void set_node_volume(struct cras_ionode *node, int value)
{
	struct cras_iodev *dev = node->dev;
	unsigned int volume;

	if (dev->direction != CRAS_STREAM_OUTPUT)
		return;

	volume = (unsigned int)MIN(value, 100);
	node->volume = volume;
	if (dev->set_volume)
		dev->set_volume(dev);

	cras_iodev_list_notify_node_volume(node);
}

static void set_node_capture_gain(struct cras_ionode *node, int value)
{
	struct cras_iodev *dev = node->dev;

	if (dev->direction != CRAS_STREAM_INPUT)
		return;

	node->capture_gain = (long)value;
	if (dev->set_capture_gain)
		dev->set_capture_gain(dev);

	cras_iodev_list_notify_node_capture_gain(node);
}

static void set_node_left_right_swapped(struct cras_ionode *node, int value)
{
	struct cras_iodev *dev = node->dev;
	int rc;

	if (!dev->set_swap_mode_for_node)
		return;
	rc = dev->set_swap_mode_for_node(dev, node, value);
	if (rc) {
		syslog(LOG_ERR,
		       "Failed to set swap mode on node %s to %d; error %d",
		       node->name, value, rc);
		return;
	}
	node->left_right_swapped = value;
	cras_iodev_list_notify_node_left_right_swapped(node);
	return;
}

int cras_iodev_set_node_attr(struct cras_ionode *ionode,
			     enum ionode_attr attr, int value)
{
	switch (attr) {
	case IONODE_ATTR_PLUGGED:
		plug_node(ionode, value);
		break;
	case IONODE_ATTR_VOLUME:
		set_node_volume(ionode, value);
		break;
	case IONODE_ATTR_CAPTURE_GAIN:
		set_node_capture_gain(ionode, value);
		break;
	case IONODE_ATTR_SWAP_LEFT_RIGHT:
		set_node_left_right_swapped(ionode, value);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

void cras_iodev_add_node(struct cras_iodev *iodev, struct cras_ionode *node)
{
	DL_APPEND(iodev->nodes, node);
	cras_iodev_list_notify_nodes_changed();
}

void cras_iodev_rm_node(struct cras_iodev *iodev, struct cras_ionode *node)
{
	DL_DELETE(iodev->nodes, node);
	cras_iodev_list_notify_nodes_changed();
}

void cras_iodev_set_active_node(struct cras_iodev *iodev,
				struct cras_ionode *node)
{
	iodev->active_node = node;
	cras_iodev_list_notify_active_node_changed(iodev->direction);
}

float cras_iodev_get_software_volume_scaler(struct cras_iodev *iodev)
{
	unsigned int volume;

	volume = cras_iodev_adjust_active_node_volume(
			iodev, cras_system_get_volume());

	if (iodev->active_node && iodev->active_node->softvol_scalers)
		return iodev->active_node->softvol_scalers[volume];
	return softvol_get_scaler(volume);
}

float cras_iodev_get_software_gain_scaler(const struct cras_iodev *iodev) {
	float scaler = 1.0f;
	if (cras_iodev_software_volume_needed(iodev)) {
		long gain = cras_iodev_adjust_active_node_gain(
				iodev, cras_system_get_capture_gain());
		scaler = convert_softvol_scaler_from_dB(gain);
	}
	return scaler;
}

int cras_iodev_add_stream(struct cras_iodev *iodev,
			  struct dev_stream *stream)
{
	unsigned int cb_threshold = dev_stream_cb_threshold(stream);
	DL_APPEND(iodev->streams, stream);

	if (!iodev->buf_state)
		iodev->buf_state = buffer_share_create(iodev->buffer_size);
	buffer_share_add_id(iodev->buf_state, stream->stream->stream_id, NULL);

	iodev->min_cb_level = MIN(iodev->min_cb_level, cb_threshold);
	iodev->max_cb_level = MAX(iodev->max_cb_level, cb_threshold);
	return 0;
}

struct dev_stream *cras_iodev_rm_stream(struct cras_iodev *iodev,
					const struct cras_rstream *rstream)
{
	struct dev_stream *out;
	struct dev_stream *ret = NULL;
	unsigned int cb_threshold;
	unsigned int old_min_cb_level = iodev->min_cb_level;

	iodev->min_cb_level = iodev->buffer_size / 2;
	iodev->max_cb_level = 0;
	DL_FOREACH(iodev->streams, out) {
		if (out->stream == rstream) {
			buffer_share_rm_id(iodev->buf_state,
					   rstream->stream_id);
			ret = out;
			DL_DELETE(iodev->streams, out);
			continue;
		}
		cb_threshold = dev_stream_cb_threshold(out);
		iodev->min_cb_level = MIN(iodev->min_cb_level, cb_threshold);
		iodev->max_cb_level = MAX(iodev->max_cb_level, cb_threshold);
	}

	if (!iodev->streams) {
		buffer_share_destroy(iodev->buf_state);
		iodev->buf_state = NULL;
		iodev->min_cb_level = old_min_cb_level;
		/* Let output device transit into no stream state if it's
		 * in normal run state now. Leave input device in normal
		 * run state. */
		if ((iodev->direction == CRAS_STREAM_OUTPUT) &&
		    (iodev->state == CRAS_IODEV_STATE_NORMAL_RUN))
			cras_iodev_no_stream_playback_transition(iodev, 1);
	}
	return ret;
}

unsigned int cras_iodev_stream_offset(struct cras_iodev *iodev,
				      struct dev_stream *stream)
{
	return buffer_share_id_offset(iodev->buf_state,
				      stream->stream->stream_id);
}

void cras_iodev_stream_written(struct cras_iodev *iodev,
			       struct dev_stream *stream,
			       unsigned int nwritten)
{
	buffer_share_offset_update(iodev->buf_state,
				   stream->stream->stream_id, nwritten);
}

unsigned int cras_iodev_all_streams_written(struct cras_iodev *iodev)
{
	if (!iodev->buf_state)
		return 0;
	return buffer_share_get_new_write_point(iodev->buf_state);
}

unsigned int cras_iodev_max_stream_offset(const struct cras_iodev *iodev)
{
	unsigned int max = 0;
	struct dev_stream *curr;

	DL_FOREACH(iodev->streams, curr) {
		max = MAX(max,
			  buffer_share_id_offset(iodev->buf_state,
						 curr->stream->stream_id));
	}

	return max;
}

int cras_iodev_open(struct cras_iodev *iodev, unsigned int cb_level)
{
	int rc;

	rc = iodev->open_dev(iodev);
	if (rc < 0)
		return rc;

	/* Make sure the min_cb_level doesn't get too large. */
	iodev->min_cb_level = MIN(iodev->buffer_size / 2, cb_level);
	iodev->max_cb_level = 0;

	iodev->reset_request_pending = 0;
	iodev->state = CRAS_IODEV_STATE_OPEN;

	if (iodev->direction == CRAS_STREAM_OUTPUT) {
		/* If device supports start ops, device can be in open state.
		 * Otherwise, device starts running right after opening. */
		if (iodev->start)
			iodev->state = CRAS_IODEV_STATE_OPEN;
		else
			iodev->state = CRAS_IODEV_STATE_NO_STREAM_RUN;
	} else {
		/* Input device starts running right after opening.
		 * No stream state is only for output device. Input device
		 * should be in normal run state. */
		iodev->state = CRAS_IODEV_STATE_NORMAL_RUN;
	}

	return 0;
}

enum CRAS_IODEV_STATE cras_iodev_state(const struct cras_iodev *iodev)
{
	return iodev->state;
}

int cras_iodev_close(struct cras_iodev *iodev)
{
	int rc;
	if (!cras_iodev_is_open(iodev))
		return 0;

	rc = iodev->close_dev(iodev);
	if (rc)
		return rc;
	iodev->state = CRAS_IODEV_STATE_CLOSE;
	if (iodev->ramp)
		cras_ramp_reset(iodev->ramp);
	return 0;
}

int cras_iodev_put_input_buffer(struct cras_iodev *iodev, unsigned int nframes)
{
	rate_estimator_add_frames(iodev->rate_est, -nframes);
	return iodev->put_buffer(iodev, nframes);
}

int cras_iodev_put_output_buffer(struct cras_iodev *iodev, uint8_t *frames,
				 unsigned int nframes)
{
	const struct cras_audio_format *fmt = iodev->format;
	struct cras_fmt_conv * remix_converter =
			audio_thread_get_global_remix_converter();
	struct cras_ramp_action ramp_action = {
		.type = CRAS_RAMP_ACTION_NONE,
		.scaler = 0.0f,
		.increment = 0.0f,
	};
	float software_volume_scaler;
	int software_volume_needed = cras_iodev_software_volume_needed(iodev);

	if (iodev->pre_dsp_hook)
		iodev->pre_dsp_hook(frames, nframes, iodev->ext_format,
				    iodev->pre_dsp_hook_cb_data);

	if (iodev->ramp) {
		ramp_action = cras_ramp_get_current_action(iodev->ramp);
	}

	/* Mute samples if adjusted volume is 0 or system is muted, plus
	 * that this device is not ramping. */
	if (output_should_mute(iodev) &&
	    ramp_action.type != CRAS_RAMP_ACTION_PARTIAL) {
		const unsigned int frame_bytes = cras_get_format_bytes(fmt);
		cras_mix_mute_buffer(frames, frame_bytes, nframes);
	} else {
		apply_dsp(iodev, frames, nframes);

		if (iodev->post_dsp_hook)
			iodev->post_dsp_hook(frames, nframes, fmt,
					     iodev->post_dsp_hook_cb_data);

		/* Compute scaler for software volume if needed. */
		if (software_volume_needed) {
			software_volume_scaler =
				cras_iodev_get_software_volume_scaler(iodev);
		}

		if (ramp_action.type == CRAS_RAMP_ACTION_PARTIAL) {
			/* Scale with increment for ramp and possibly
			 * software volume using cras_scale_buffer_increment.*/
			float starting_scaler = ramp_action.scaler;
			float increment = ramp_action.increment;

			if (software_volume_needed) {
				starting_scaler *= software_volume_scaler;
				increment *= software_volume_scaler;
			}

			cras_scale_buffer_increment(
					fmt->format, frames, nframes,
					starting_scaler, increment,
					fmt->num_channels);
			cras_ramp_update_ramped_frames(iodev->ramp, nframes);
		} else if (software_volume_needed) {
			/* Just scale for software volume using
			 * cras_scale_buffer. */
			unsigned int nsamples = nframes * fmt->num_channels;
			cras_scale_buffer(fmt->format, frames,
					  nsamples, software_volume_scaler);
		}
	}

	if (remix_converter)
		cras_channel_remix_convert(remix_converter,
				   iodev->format,
				   frames,
				   nframes);
	rate_estimator_add_frames(iodev->rate_est, nframes);
	return iodev->put_buffer(iodev, nframes);
}

int cras_iodev_get_input_buffer(struct cras_iodev *iodev,
				struct cras_audio_area **area,
				unsigned *frames)
{
	const struct cras_audio_format *fmt = iodev->format;
	const unsigned int frame_bytes = cras_get_format_bytes(fmt);
	uint8_t *hw_buffer;
	int rc;
	unsigned frame_requested = *frames;

	rc = iodev->get_buffer(iodev, area, frames);
	if (rc < 0 || *frames == 0)
		return rc;
	if (*frames > frame_requested) {
		syslog(LOG_ERR,
		       "frames returned from get_buffer is greater than "
		       "requested: %u > %u", *frames, frame_requested);
		return -EINVAL;
	}

	/* TODO(dgreid) - This assumes interleaved audio. */
	hw_buffer = (*area)->channels[0].buf;

	if (cras_system_get_capture_mute())
		cras_mix_mute_buffer(hw_buffer, frame_bytes, *frames);
	else
		apply_dsp(iodev, hw_buffer, *frames); /* TODO-applied 2x */

	return rc;
}

int cras_iodev_get_output_buffer(struct cras_iodev *iodev,
				 struct cras_audio_area **area,
				 unsigned *frames)
{
	int rc;
	unsigned frame_requested = *frames;

	rc = iodev->get_buffer(iodev, area, frames);
	if (*frames > frame_requested) {
		syslog(LOG_ERR,
		       "frames returned from get_buffer is greater than "
		       "requested: %u > %u", *frames, frame_requested);
		return -EINVAL;
	}
	return rc;
}

int cras_iodev_update_rate(struct cras_iodev *iodev, unsigned int level,
			   struct timespec *level_tstamp)
{
	return rate_estimator_check(iodev->rate_est, level, level_tstamp);
}

int cras_iodev_reset_rate_estimator(const struct cras_iodev *iodev)
{
	rate_estimator_reset_rate(iodev->rate_est,
				  iodev->ext_format->frame_rate);
	return 0;
}

double cras_iodev_get_est_rate_ratio(const struct cras_iodev *iodev)
{
	return rate_estimator_get_rate(iodev->rate_est) /
			iodev->ext_format->frame_rate;
}

int cras_iodev_get_dsp_delay(const struct cras_iodev *iodev)
{
	struct cras_dsp_context *ctx;
	struct pipeline *pipeline;
	int delay;

	ctx = iodev->dsp_context;
	if (!ctx)
		return 0;

	pipeline = cras_dsp_get_pipeline(ctx);
	if (!pipeline)
		return 0;

	delay = cras_dsp_pipeline_get_delay(pipeline);

	cras_dsp_put_pipeline(ctx);
	return delay;
}

int cras_iodev_frames_queued(struct cras_iodev *iodev,
			     struct timespec *hw_tstamp)
{
	int rc;

	rc = iodev->frames_queued(iodev, hw_tstamp);
	if (rc < 0 || iodev->direction == CRAS_STREAM_INPUT)
		return rc;

	if (rc < iodev->min_buffer_level)
		return 0;

	return rc - iodev->min_buffer_level;
}

int cras_iodev_buffer_avail(struct cras_iodev *iodev, unsigned hw_level)
{
	if (iodev->direction == CRAS_STREAM_INPUT)
		return hw_level;

	if (hw_level + iodev->min_buffer_level > iodev->buffer_size)
		return 0;

	return iodev->buffer_size - iodev->min_buffer_level - hw_level;
}

void cras_iodev_register_pre_dsp_hook(struct cras_iodev *iodev,
				      loopback_hook_t loop_cb,
				      void *cb_data)
{
	iodev->pre_dsp_hook = loop_cb;
	iodev->pre_dsp_hook_cb_data = cb_data;
}

void cras_iodev_register_post_dsp_hook(struct cras_iodev *iodev,
				       loopback_hook_t loop_cb,
				       void *cb_data)
{
	iodev->post_dsp_hook = loop_cb;
	iodev->post_dsp_hook_cb_data = cb_data;
}

int cras_iodev_fill_odev_zeros(struct cras_iodev *odev, unsigned int frames)
{
	struct cras_audio_area *area = NULL;
	unsigned int frame_bytes, frames_written;
	int rc;
	uint8_t *buf;

	if (odev->direction != CRAS_STREAM_OUTPUT)
		return -EINVAL;

	ATLOG(atlog, AUDIO_THREAD_FILL_ODEV_ZEROS, odev->info.idx, frames, 0);

	frame_bytes = cras_get_format_bytes(odev->ext_format);
	while (frames > 0) {
		frames_written = frames;
		rc = cras_iodev_get_output_buffer(odev, &area, &frames_written);
		if (rc < 0) {
			syslog(LOG_ERR, "fill zeros fail: %d", rc);
			return rc;
		}

		/* This assumes consecutive channel areas. */
		buf = area->channels[0].buf;
		memset(buf, 0, frames_written * frame_bytes);
		cras_iodev_put_output_buffer(odev, buf, frames_written);
		frames -= frames_written;
	}

	return 0;
}

int cras_iodev_output_underrun(struct cras_iodev *odev) {
	if (odev->output_underrun)
		return odev->output_underrun(odev);
	else
		return cras_iodev_fill_odev_zeros(odev, odev->min_cb_level);
}

int cras_iodev_odev_should_wake(const struct cras_iodev *odev)
{
	if (odev->direction != CRAS_STREAM_OUTPUT)
		return 0;

	if (odev->output_should_wake)
		return odev->output_should_wake(odev);

	/* Do not wake up for device not started yet. */
	return (odev->state == CRAS_IODEV_STATE_NORMAL_RUN ||
	        odev->state == CRAS_IODEV_STATE_NO_STREAM_RUN);
}

unsigned int cras_iodev_frames_to_play_in_sleep(struct cras_iodev *odev,
						unsigned int *hw_level,
						struct timespec *hw_tstamp)
{
	int rc;

	rc = cras_iodev_frames_queued(odev, hw_tstamp);
	*hw_level = (rc < 0) ? 0 : rc;

	if (odev->streams) {
		/* Schedule that audio thread will wake up when
		 * hw_level drops to 0.
		 * This should not cause underrun because audio thread
		 * should be waken up by the reply from client. */
		return *hw_level;
	}
	/* When this device has no stream, schedule audio thread to wake up
	 * when hw_level drops to min_cb_level so audio thread can fill
	 * zeros to it. */
	if (*hw_level > odev->min_cb_level)
		return *hw_level - odev->min_cb_level;
	else
		return 0;
}

int cras_iodev_default_no_stream_playback(struct cras_iodev *odev, int enable)
{
	if (enable)
		return default_no_stream_playback(odev);
	return 0;
}

int cras_iodev_prepare_output_before_write_samples(struct cras_iodev *odev)
{
	int may_enter_normal_run;
	enum CRAS_IODEV_STATE state;

	if (odev->direction != CRAS_STREAM_OUTPUT)
		return -EINVAL;

	state = cras_iodev_state(odev);

	may_enter_normal_run = (state == CRAS_IODEV_STATE_OPEN ||
		                state == CRAS_IODEV_STATE_NO_STREAM_RUN);

	if (may_enter_normal_run && dev_playback_frames(odev))
		return cras_iodev_output_event_sample_ready(odev);

	/* no_stream ops is called every cycle in no_stream state. */
	if (state == CRAS_IODEV_STATE_NO_STREAM_RUN)
		return odev->no_stream(odev, 1);

	return 0;
}

unsigned int cras_iodev_get_num_underruns(const struct cras_iodev *iodev)
{
	if (iodev->get_num_underruns)
		return iodev->get_num_underruns(iodev);
	return 0;
}

unsigned int cras_iodev_get_num_severe_underruns(const struct cras_iodev *iodev)
{
	if (iodev->get_num_severe_underruns)
		return iodev->get_num_severe_underruns(iodev);
	return 0;
}

int cras_iodev_reset_request(struct cras_iodev* iodev)
{
	/* Ignore requests if there is a pending request.
	 * This function sends the request from audio thread to main
	 * thread when audio thread finds a device is in a bad state
	 * e.g. severe underrun. Before main thread receives the
	 * request and resets device, audio thread might try to send
	 * multiple requests because it finds device is still in bad
	 * state. We should ignore requests in this cause. Otherwise,
	 * main thread will reset device multiple times.
	 * The flag is cleared in cras_iodev_open.
	 * */
	if (iodev->reset_request_pending)
		return 0;
	iodev->reset_request_pending = 1;
	return cras_device_monitor_reset_device(iodev);
}

static void ramp_mute_callback(void *data)
{
	struct cras_iodev *odev = (struct cras_iodev *)data;
	cras_device_monitor_set_device_mute_state(odev);
}

/* Used in audio thread. Check the docstrings of CRAS_IODEV_RAMP_REQUEST. */
int cras_iodev_start_ramp(struct cras_iodev *odev,
			  enum CRAS_IODEV_RAMP_REQUEST request)
{
	cras_ramp_cb cb = NULL;
	void *cb_data = NULL;
	int rc, up;
	float duration_secs;

	/* Ignores request if device is closed. */
	if (!cras_iodev_is_open(odev))
		return 0;

	switch (request) {
	case CRAS_IODEV_RAMP_REQUEST_UP_UNMUTE:
		up = 1;
		duration_secs = RAMP_UNMUTE_DURATION_SECS;
		break;
	case CRAS_IODEV_RAMP_REQUEST_UP_START_PLAYBACK:
		up = 1;
		duration_secs = RAMP_NEW_STREAM_DURATION_SECS;
		break;
	/* Unmute -> mute. Callback to set mute state should be called after
	 * ramping is done. */
	case CRAS_IODEV_RAMP_REQUEST_DOWN_MUTE:
		up = 0;
		duration_secs = RAMP_MUTE_DURATION_SECS;
		cb = ramp_mute_callback;
		cb_data = (void*)odev;
		break;
	default:
		return -EINVAL;
	}

	/* Starts ramping. */
	rc = cras_ramp_start(
			odev->ramp, up,
			duration_secs * odev->format->frame_rate,
			cb, cb_data);

	if (rc)
		return rc;

	/* Mute -> unmute case, unmute state should be set after ramping is
	 * started so device can start playing with samples close to 0. */
	if (request == CRAS_IODEV_RAMP_REQUEST_UP_UNMUTE)
		cras_device_monitor_set_device_mute_state(odev);

	return 0;
}

int cras_iodev_set_mute(struct cras_iodev* iodev)
{
	if (!cras_iodev_is_open(iodev))
		return 0;

	if (iodev->set_mute)
		iodev->set_mute(iodev);
	return 0;
}
