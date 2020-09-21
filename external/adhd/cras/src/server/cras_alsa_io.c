/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <alsa/asoundlib.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <syslog.h>
#include <time.h>

#include "audio_thread.h"
#include "cras_alsa_helpers.h"
#include "cras_alsa_io.h"
#include "cras_alsa_jack.h"
#include "cras_alsa_mixer.h"
#include "cras_alsa_ucm.h"
#include "cras_audio_area.h"
#include "cras_config.h"
#include "cras_utf8.h"
#include "cras_iodev.h"
#include "cras_iodev_list.h"
#include "cras_messages.h"
#include "cras_ramp.h"
#include "cras_rclient.h"
#include "cras_shm.h"
#include "cras_system_state.h"
#include "cras_types.h"
#include "cras_util.h"
#include "cras_volume_curve.h"
#include "sfh.h"
#include "softvol_curve.h"
#include "utlist.h"

#define MAX_ALSA_DEV_NAME_LENGTH 9 /* Alsa names "hw:XX,YY" + 1 for null. */
#define HOTWORD_DEV "Wake on Voice"
#define DEFAULT "(default)"
#define HDMI "HDMI"
#define INTERNAL_MICROPHONE "Internal Mic"
#define INTERNAL_SPEAKER "Speaker"
#define KEYBOARD_MIC "Keyboard Mic"
#define USB "USB"

/*
 * For USB, pad the output buffer.  This avoids a situation where there isn't a
 * complete URB's worth of audio ready to be transmitted when it is requested.
 * The URB interval does track directly to the audio clock, making it hard to
 * predict the exact interval.
 */
#define USB_EXTRA_BUFFER_FRAMES 768

/*
 * When snd_pcm_avail returns a value that is greater than buffer size,
 * we know there is an underrun. If the number of underrun samples
 * (avail - buffer_size) is greater than SEVERE_UNDERRUN_MS * rate,
 * it is a severe underrun. Main thread should disable and then enable
 * device to recover it from underrun.
 */
#define SEVERE_UNDERRUN_MS 5000

/*
 * This extends cras_ionode to include alsa-specific information.
 * Members:
 *    mixer_output - From cras_alsa_mixer.
 *    volume_curve - Volume curve for this node.
 *    jack - The jack associated with the node.
 */
struct alsa_output_node {
	struct cras_ionode base;
	struct mixer_control *mixer_output;
	struct cras_volume_curve *volume_curve;
	const struct cras_alsa_jack *jack;
};

struct alsa_input_node {
	struct cras_ionode base;
	struct mixer_control* mixer_input;
	const struct cras_alsa_jack *jack;
	int8_t *channel_layout;
};

/*
 * Child of cras_iodev, alsa_io handles ALSA interaction for sound devices.
 * base - The cras_iodev structure "base class".
 * dev - String that names this device (e.g. "hw:0,0").
 * dev_name - value from snd_pcm_info_get_name
 * dev_id - value from snd_pcm_info_get_id
 * device_index - ALSA index of device, Y in "hw:X:Y".
 * next_ionode_index - The index we will give to the next ionode. Each ionode
 *     have a unique index within the iodev.
 * card_type - the type of the card this iodev belongs.
 * is_first - true if this is the first iodev on the card.
 * fully_specified - true if this device and it's nodes were fully specified.
 *     That is, don't automatically create nodes for it.
 * enable_htimestamp - True when the device's htimestamp is used.
 * handle - Handle to the opened ALSA device.
 * num_underruns - Number of times we have run out of data (playback only).
 * num_severe_underruns - Number of times we have run out of data badly.
                          Unlike num_underruns which records for the duration
                          where device is opened, num_severe_underruns records
                          since device is created. When severe underrun occurs
                          a possible action is to close/open device.
 * alsa_stream - Playback or capture type.
 * mixer - Alsa mixer used to control volume and mute of the device.
 * config - Card config for this alsa device.
 * jack_list - List of alsa jack controls for this device.
 * ucm - CRAS use case manager, if configuration is found.
 * mmap_offset - offset returned from mmap_begin.
 * dsp_name_default - the default dsp name for the device. It can be overridden
 *     by the jack specific dsp name.
 * poll_fd - Descriptor used to block until data is ready.
 * dma_period_set_microsecs - If non-zero, the value to apply to the dma_period.
 * is_free_running - true if device is playing zeros in the buffer without
 *                   user filling meaningful data. The device buffer is filled
 *                   with zeros. In this state, appl_ptr remains the same
 *                   while hw_ptr keeps running ahead.
 * filled_zeros_for_draining - The number of zeros filled for draining.
 * severe_underrun_frames - The threshold for severe underrun.
 * default_volume_curve - Default volume curve that converts from an index
 *                        to dBFS.
 */
struct alsa_io {
	struct cras_iodev base;
	char *dev;
	char *dev_name;
	char *dev_id;
	uint32_t device_index;
	uint32_t next_ionode_index;
	enum CRAS_ALSA_CARD_TYPE card_type;
	int is_first;
	int fully_specified;
	int enable_htimestamp;
	snd_pcm_t *handle;
	unsigned int num_underruns;
	unsigned int num_severe_underruns;
	snd_pcm_stream_t alsa_stream;
	struct cras_alsa_mixer *mixer;
	const struct cras_card_config *config;
	struct cras_alsa_jack_list *jack_list;
	struct cras_use_case_mgr *ucm;
	snd_pcm_uframes_t mmap_offset;
	const char *dsp_name_default;
	int poll_fd;
	unsigned int dma_period_set_microsecs;
	int is_free_running;
	unsigned int filled_zeros_for_draining;
	snd_pcm_uframes_t severe_underrun_frames;
	struct cras_volume_curve *default_volume_curve;
};

static void init_device_settings(struct alsa_io *aio);

static int alsa_iodev_set_active_node(struct cras_iodev *iodev,
				      struct cras_ionode *ionode,
				      unsigned dev_enabled);

/*
 * Defines the default values of nodes.
 */
static const struct {
	const char *name;
	enum CRAS_NODE_TYPE type;
	enum CRAS_NODE_POSITION position;
} node_defaults[] = {
	{
		.name = DEFAULT,
		.type = CRAS_NODE_TYPE_UNKNOWN,
		.position = NODE_POSITION_INTERNAL,
	},
	{
		.name = INTERNAL_SPEAKER,
		.type = CRAS_NODE_TYPE_INTERNAL_SPEAKER,
		.position = NODE_POSITION_INTERNAL,
	},
	{
		.name = INTERNAL_MICROPHONE,
		.type = CRAS_NODE_TYPE_MIC,
		.position = NODE_POSITION_INTERNAL,
	},
	{
		.name = KEYBOARD_MIC,
		.type = CRAS_NODE_TYPE_MIC,
		.position = NODE_POSITION_KEYBOARD,
	},
	{
		.name = HDMI,
		.type = CRAS_NODE_TYPE_HDMI,
		.position = NODE_POSITION_EXTERNAL,
	},
	{
		.name = "IEC958",
		.type = CRAS_NODE_TYPE_HDMI,
		.position = NODE_POSITION_EXTERNAL,
	},
	{
		.name = "Headphone",
		.type = CRAS_NODE_TYPE_HEADPHONE,
		.position = NODE_POSITION_EXTERNAL,
	},
	{
		.name = "Front Headphone",
		.type = CRAS_NODE_TYPE_HEADPHONE,
		.position = NODE_POSITION_EXTERNAL,
	},
	{
		.name = "Front Mic",
		.type = CRAS_NODE_TYPE_MIC,
		.position = NODE_POSITION_FRONT,
	},
	{
		.name = "Rear Mic",
		.type = CRAS_NODE_TYPE_MIC,
		.position = NODE_POSITION_REAR,
	},
	{
		.name = "Mic",
		.type = CRAS_NODE_TYPE_MIC,
		.position = NODE_POSITION_EXTERNAL,
	},
	{
		.name = HOTWORD_DEV,
		.type = CRAS_NODE_TYPE_HOTWORD,
		.position = NODE_POSITION_INTERNAL,
	},
	{
		.name = "Haptic",
		.type = CRAS_NODE_TYPE_HAPTIC,
		.position = NODE_POSITION_INTERNAL,
	},
	{
		.name = "Rumbler",
		.type = CRAS_NODE_TYPE_HAPTIC,
		.position = NODE_POSITION_INTERNAL,
	},
	{
		.name = "Line Out",
		.type = CRAS_NODE_TYPE_LINEOUT,
		.position = NODE_POSITION_EXTERNAL,
	},
};

/*
 * iodev callbacks.
 */

static int frames_queued(const struct cras_iodev *iodev,
			 struct timespec *tstamp)
{
	struct alsa_io *aio = (struct alsa_io *)iodev;
	int rc;
	snd_pcm_uframes_t frames;

	rc = cras_alsa_get_avail_frames(aio->handle,
					aio->base.buffer_size,
					aio->severe_underrun_frames,
					iodev->info.name,
					&frames, tstamp,
					&aio->num_underruns);
	if (rc < 0) {
		if (rc == -EPIPE)
			aio->num_severe_underruns++;
		return rc;
	}
	if (!aio->enable_htimestamp)
		clock_gettime(CLOCK_MONOTONIC_RAW, tstamp);
	if (iodev->direction == CRAS_STREAM_INPUT)
		return (int)frames;

	/* For output, return number of frames that are used. */
	return iodev->buffer_size - frames;
}

static int delay_frames(const struct cras_iodev *iodev)
{
	struct alsa_io *aio = (struct alsa_io *)iodev;
	snd_pcm_sframes_t delay;
	int rc;

	rc = cras_alsa_get_delay_frames(aio->handle,
					iodev->buffer_size,
					&delay);
	if (rc < 0)
		return rc;

	return (int)delay;
}

static int close_dev(struct cras_iodev *iodev)
{
	struct alsa_io *aio = (struct alsa_io *)iodev;

	/* Removes audio thread callback from main thread. */
	if (aio->poll_fd >= 0)
		audio_thread_rm_callback_sync(
				cras_iodev_list_get_audio_thread(),
				aio->poll_fd);
	if (!aio->handle)
		return 0;
	cras_alsa_pcm_close(aio->handle);
	aio->handle = NULL;
	aio->is_free_running = 0;
	aio->filled_zeros_for_draining = 0;
	cras_iodev_free_format(&aio->base);
	cras_iodev_free_audio_area(&aio->base);
	return 0;
}

static int dummy_hotword_cb(void *arg)
{
	/* Only need this once. */
	struct alsa_io *aio = (struct alsa_io *)arg;
	audio_thread_rm_callback(aio->poll_fd);
	aio->poll_fd = -1;
	return 0;
}

static int open_dev(struct cras_iodev *iodev)
{
	struct alsa_io *aio = (struct alsa_io *)iodev;
	snd_pcm_t *handle;
	int period_wakeup;
	int rc;

	/* This is called after the first stream added so configure for it.
	 * format must be set before opening the device.
	 */
	if (iodev->format == NULL)
		return -EINVAL;
	aio->num_underruns = 0;
	aio->is_free_running = 0;
	aio->filled_zeros_for_draining = 0;
	aio->severe_underrun_frames =
			SEVERE_UNDERRUN_MS * iodev->format->frame_rate / 1000;

	cras_iodev_init_audio_area(iodev, iodev->format->num_channels);

	syslog(LOG_DEBUG, "Configure alsa device %s rate %zuHz, %zu channels",
	       aio->dev, iodev->format->frame_rate,
	       iodev->format->num_channels);
	handle = 0; /* Avoid unused warning. */
	rc = cras_alsa_pcm_open(&handle, aio->dev, aio->alsa_stream);
	if (rc < 0)
		return rc;

	/* If it's a wake on voice device, period_wakeups are required. */
	period_wakeup = (iodev->active_node->type == CRAS_NODE_TYPE_HOTWORD);

	rc = cras_alsa_set_hwparams(handle, iodev->format,
				    &iodev->buffer_size, period_wakeup,
				    aio->dma_period_set_microsecs);
	if (rc < 0) {
		cras_alsa_pcm_close(handle);
		return rc;
	}

	/* Set channel map to device */
	rc = cras_alsa_set_channel_map(handle,
				       iodev->format);
	if (rc < 0) {
		cras_alsa_pcm_close(handle);
		return rc;
	}

	/* Configure software params. */
	rc = cras_alsa_set_swparams(handle, &aio->enable_htimestamp);
	if (rc < 0) {
		cras_alsa_pcm_close(handle);
		return rc;
	}

	/* Assign pcm handle then initialize device settings. */
	aio->handle = handle;
	init_device_settings(aio);

	aio->poll_fd = -1;
	if (iodev->active_node->type == CRAS_NODE_TYPE_HOTWORD) {
		struct pollfd *ufds;
		int count, i;

		count = snd_pcm_poll_descriptors_count(handle);
		if (count <= 0) {
			syslog(LOG_ERR, "Invalid poll descriptors count\n");
			return count;
		}

		ufds = (struct pollfd *)malloc(sizeof(struct pollfd) * count);
		if (ufds == NULL)
			return -ENOMEM;

		rc = snd_pcm_poll_descriptors(handle, ufds, count);
		if (rc < 0) {
			syslog(LOG_ERR,
			       "Getting hotword poll descriptors: %s\n",
			       snd_strerror(rc));
			free(ufds);
			return rc;
		}

		for (i = 0; i < count; i++) {
			if (ufds[i].events & POLLIN) {
				aio->poll_fd = ufds[i].fd;
				break;
			}
		}
		free(ufds);

		if (aio->poll_fd >= 0)
			audio_thread_add_callback(aio->poll_fd,
						  dummy_hotword_cb,
						  aio);
	}

	/* Capture starts right away, playback will wait for samples. */
	if (aio->alsa_stream == SND_PCM_STREAM_CAPTURE)
		cras_alsa_pcm_start(aio->handle);

	return 0;
}

/*
 * Check if ALSA device is opened by checking if handle is valid.
 * Note that to fully open a cras_iodev, ALSA device is opened first, then there
 * are some device init settings to be done in init_device_settings.
 * Therefore, when setting volume/mute/gain in init_device_settings,
 * cras_iodev is not in CRAS_IODEV_STATE_OPEN yet. We need to check if handle
 * is valid when setting those properties, instead of checking
 * cras_iodev_is_open.
 */
static int has_handle(const struct alsa_io *aio)
{
	return !!aio->handle;
}

static int start(const struct cras_iodev *iodev)
{
	struct alsa_io *aio = (struct alsa_io *)iodev;
	snd_pcm_t *handle = aio->handle;
	int rc;

	if (snd_pcm_state(handle) == SND_PCM_STATE_RUNNING)
		return 0;

	if (snd_pcm_state(handle) == SND_PCM_STATE_SUSPENDED) {
		rc = cras_alsa_attempt_resume(handle);
		if (rc < 0) {
			syslog(LOG_ERR, "Resume error: %s", snd_strerror(rc));
			return rc;
		}
		cras_iodev_reset_rate_estimator(iodev);
	} else {
		rc = cras_alsa_pcm_start(handle);
		if (rc < 0) {
			syslog(LOG_ERR, "Start error: %s", snd_strerror(rc));
			return rc;
		}
	}

	return 0;
}

static int get_buffer(struct cras_iodev *iodev,
		      struct cras_audio_area **area,
		      unsigned *frames)
{
	struct alsa_io *aio = (struct alsa_io *)iodev;
	snd_pcm_uframes_t nframes = *frames;
	uint8_t *dst = NULL;
	size_t format_bytes;
	int rc;

	aio->mmap_offset = 0;
	format_bytes = cras_get_format_bytes(iodev->format);

	rc = cras_alsa_mmap_begin(aio->handle,
				  format_bytes,
				  &dst,
				  &aio->mmap_offset,
				  &nframes,
				  &aio->num_underruns);

	iodev->area->frames = nframes;
	cras_audio_area_config_buf_pointers(iodev->area, iodev->format, dst);

	*area = iodev->area;
	*frames = nframes;

	return rc;
}

static int put_buffer(struct cras_iodev *iodev, unsigned nwritten)
{
	struct alsa_io *aio = (struct alsa_io *)iodev;

	return cras_alsa_mmap_commit(aio->handle,
				     aio->mmap_offset,
				     nwritten,
				     &aio->num_underruns);
}

static int flush_buffer(struct cras_iodev *iodev)
{
	struct alsa_io *aio = (struct alsa_io *)iodev;
	snd_pcm_uframes_t nframes;

	if (iodev->direction == CRAS_STREAM_INPUT) {
		nframes = snd_pcm_forwardable(aio->handle);
		return snd_pcm_forward(aio->handle, nframes);
	}
	return 0;
}

/*
 * Gets the first plugged node in list. This is used as the
 * default node to set as active.
 */
static struct cras_ionode *first_plugged_node(struct cras_iodev *iodev)
{
	struct cras_ionode *n;

	/* When this is called at iodev creation, none of the nodes
	 * are selected. Just pick the first plugged one and let Chrome
	 * choose it later. */
	DL_FOREACH(iodev->nodes, n) {
		if (n->plugged)
			return n;
	}
	return iodev->nodes;
}

static void update_active_node(struct cras_iodev *iodev, unsigned node_idx,
			       unsigned dev_enabled)
{
	struct cras_ionode *n;

	/* If a node exists for node_idx, set it as active. */
	DL_FOREACH(iodev->nodes, n) {
		if (n->idx == node_idx) {
			alsa_iodev_set_active_node(iodev, n, dev_enabled);
			return;
		}
	}

	alsa_iodev_set_active_node(iodev, first_plugged_node(iodev),
				   dev_enabled);
}

static int update_channel_layout(struct cras_iodev *iodev)
{
	struct alsa_io *aio = (struct alsa_io *)iodev;
	snd_pcm_t *handle = NULL;
	snd_pcm_uframes_t buf_size = 0;
	int err = 0;

	/* If the capture channel map is specified in UCM, prefer it over
	 * what ALSA provides. */
	if (aio->ucm && (iodev->direction == CRAS_STREAM_INPUT)) {
		struct alsa_input_node *input =
			(struct alsa_input_node *)iodev->active_node;

		if (input->channel_layout) {
			memcpy(iodev->format->channel_layout,
			       input->channel_layout,
			       CRAS_CH_MAX * sizeof(*input->channel_layout));
			return 0;
		}
	}

	err = cras_alsa_pcm_open(&handle, aio->dev, aio->alsa_stream);
	if (err < 0) {
		syslog(LOG_ERR, "snd_pcm_open_failed: %s", snd_strerror(err));
		return err;
	}

	/* Sets frame rate and channel count to alsa device before
	 * we test channel mapping. */
	err = cras_alsa_set_hwparams(handle, iodev->format, &buf_size, 0,
				     aio->dma_period_set_microsecs);
	if (err < 0) {
		cras_alsa_pcm_close(handle);
		return err;
	}

	err = cras_alsa_get_channel_map(handle, iodev->format);

	cras_alsa_pcm_close(handle);
	return err;
}

static int set_hotword_model(struct cras_iodev *iodev, const char *model_name)
{
	struct alsa_io *aio = (struct alsa_io *)iodev;
	if (!aio->ucm)
		return -EINVAL;

	return ucm_set_hotword_model(aio->ucm, model_name);
}

static char *get_hotword_models(struct cras_iodev *iodev)
{
	struct alsa_io *aio = (struct alsa_io *)iodev;
	if (!aio->ucm)
		return NULL;

	return ucm_get_hotword_models(aio->ucm);
}

/*
 * Alsa helper functions.
 */

static struct alsa_output_node *get_active_output(const struct alsa_io *aio)
{
	return (struct alsa_output_node *)aio->base.active_node;
}

static struct alsa_input_node *get_active_input(const struct alsa_io *aio)
{
	return (struct alsa_input_node *)aio->base.active_node;
}

/*
 * Gets the curve for the active output node. If the node doesn't have volume
 * curve specified, return the default volume curve of the parent iodev.
 */
static const struct cras_volume_curve *get_curve_for_output_node(
		const struct alsa_io *aio,
		const struct alsa_output_node *node)
{
	if (node && node->volume_curve)
		return node->volume_curve;
	return aio->default_volume_curve;
}

/*
 * Gets the curve for the active output.
 */
static const struct cras_volume_curve *get_curve_for_active_output(
		const struct alsa_io *aio)
{
	struct alsa_output_node *node = get_active_output(aio);
	return get_curve_for_output_node(aio, node);
}

/*
 * Informs the system of the volume limits for this device.
 */
static void set_alsa_volume_limits(struct alsa_io *aio)
{
	const struct cras_volume_curve *curve;

	/* Only set the limits if the dev is active. */
	if (!has_handle(aio))
		return;

	curve = get_curve_for_active_output(aio);
	cras_system_set_volume_limits(
			curve->get_dBFS(curve, 1), /* min */
			curve->get_dBFS(curve, CRAS_MAX_SYSTEM_VOLUME));
}

/*
 * Sets the alsa mute control for this iodev.
 */
static void set_alsa_mute_control(const struct alsa_io *aio, int muted)
{
	struct alsa_output_node *aout;

	if (!has_handle(aio))
		return;

	aout = get_active_output(aio);
	cras_alsa_mixer_set_mute(
		aio->mixer,
		muted,
		aout ? aout->mixer_output : NULL);
}

/*
 * Sets the volume of the playback device to the specified level. Receives a
 * volume index from the system settings, ranging from 0 to 100, converts it to
 * dB using the volume curve, and sends the dB value to alsa.
 */
static void set_alsa_volume(struct cras_iodev *iodev)
{
	const struct alsa_io *aio = (const struct alsa_io *)iodev;
	const struct cras_volume_curve *curve;
	size_t volume;
	struct alsa_output_node *aout;

	assert(aio);
	if (aio->mixer == NULL)
		return;

	/* Only set the volume if the dev is active. */
	if (!has_handle(aio))
		return;

	volume = cras_system_get_volume();
	curve = get_curve_for_active_output(aio);
	if (curve == NULL)
		return;
	aout = get_active_output(aio);
	if (aout)
		volume = cras_iodev_adjust_node_volume(&aout->base, volume);

	/* Samples get scaled for devices using software volume, set alsa
	 * volume to 100. */
	if (cras_iodev_software_volume_needed(iodev))
		volume = 100;

	cras_alsa_mixer_set_dBFS(
		aio->mixer,
		curve->get_dBFS(curve, volume),
		aout ? aout->mixer_output : NULL);
}

static void set_alsa_mute(struct cras_iodev *iodev)
{
	/* Mute for zero. */
	const struct alsa_io *aio = (const struct alsa_io *)iodev;
	set_alsa_mute_control(aio, cras_system_get_mute());
}

/*
 * Sets the capture gain to the current system input gain level, given in dBFS.
 * Set mute based on the system mute state.  This gain can be positive or
 * negative and might be adjusted often if an app is running an AGC.
 */
static void set_alsa_capture_gain(struct cras_iodev *iodev)
{
	const struct alsa_io *aio = (const struct alsa_io *)iodev;
	struct alsa_input_node *ain;
	long gain;

	assert(aio);
	if (aio->mixer == NULL)
		return;

	/* Only set the volume if the dev is active. */
	if (!has_handle(aio))
		return;
	gain = cras_iodev_adjust_active_node_gain(
				iodev, cras_system_get_capture_gain());

	/* Set hardware gain to 0dB if software gain is needed. */
	if (cras_iodev_software_volume_needed(iodev))
		gain = 0;

	ain = get_active_input(aio);

	cras_alsa_mixer_set_capture_dBFS(
			aio->mixer,
			gain,
			ain ? ain->mixer_input : NULL);
	cras_alsa_mixer_set_capture_mute(aio->mixer,
					 cras_system_get_capture_mute(),
					 ain ? ain->mixer_input : NULL);
}

/*
 * Swaps the left and right channels of the given node.
 */
static int set_alsa_node_swapped(struct cras_iodev *iodev,
				 struct cras_ionode *node, int enable)
{
	const struct alsa_io *aio = (const struct alsa_io *)iodev;
	assert(aio);
	return ucm_enable_swap_mode(aio->ucm, node->name, enable);
}

/*
 * Initializes the device settings according to system volume, mute, gain
 * settings.
 * Updates system capture gain limits based on current active device/node.
 */
static void init_device_settings(struct alsa_io *aio)
{
	/* Register for volume/mute callback and set initial volume/mute for
	 * the device. */
	if (aio->base.direction == CRAS_STREAM_OUTPUT) {
		set_alsa_volume_limits(aio);
		set_alsa_volume(&aio->base);
		set_alsa_mute(&aio->base);
	} else {
		struct mixer_control *mixer_input = NULL;
		struct alsa_input_node *ain = get_active_input(aio);
		long min_capture_gain, max_capture_gain;

		if (ain)
			mixer_input = ain->mixer_input;

		if (cras_iodev_software_volume_needed(&aio->base)) {
			min_capture_gain = DEFAULT_MIN_CAPTURE_GAIN;
			max_capture_gain = cras_iodev_maximum_software_gain(
					&aio->base);
		} else {
			min_capture_gain =
				cras_alsa_mixer_get_minimum_capture_gain(
						aio->mixer, mixer_input);
			max_capture_gain =
				cras_alsa_mixer_get_maximum_capture_gain(
						aio->mixer, mixer_input);
		}
		cras_system_set_capture_gain_limits(min_capture_gain,
						    max_capture_gain);
		set_alsa_capture_gain(&aio->base);
	}
}

/*
 * Functions run in the main server context.
 */

/*
 * Frees resources used by the alsa iodev.
 * Args:
 *    iodev - the iodev to free the resources from.
 */
static void free_alsa_iodev_resources(struct alsa_io *aio)
{
	struct cras_ionode *node;
	struct alsa_output_node *aout;

	free(aio->base.supported_rates);
	free(aio->base.supported_channel_counts);
	free(aio->base.supported_formats);

	DL_FOREACH(aio->base.nodes, node) {
		if (aio->base.direction == CRAS_STREAM_OUTPUT) {
			aout = (struct alsa_output_node *)node;
			cras_volume_curve_destroy(aout->volume_curve);
		}
		cras_iodev_rm_node(&aio->base, node);
		free(node->softvol_scalers);
		free(node);
	}

	free((void *)aio->dsp_name_default);
	cras_iodev_free_resources(&aio->base);
	free(aio->dev);
	if (aio->dev_id)
		free(aio->dev_id);
	if (aio->dev_name)
		free(aio->dev_name);
}

/*
 * Returns true if this is the first internal device.
 */
static int first_internal_device(struct alsa_io *aio)
{
	return aio->is_first && aio->card_type == ALSA_CARD_TYPE_INTERNAL;
}

/*
 * Returns true if there is already a node created with the given name.
 */
static int has_node(struct alsa_io *aio, const char *name)
{
	struct cras_ionode *node;

	DL_FOREACH(aio->base.nodes, node)
		if (!strcmp(node->name, name))
			return 1;

	return 0;
}

/*
 * Returns true if string s ends with the given suffix.
 */
int endswith(const char *s, const char *suffix)
{
	size_t n = strlen(s);
	size_t m = strlen(suffix);
	return n >= m && !strcmp(s + (n - m), suffix);
}

/*
 * Drop the node name and replace it with node type.
 */
static void drop_node_name(struct cras_ionode *node)
{
	if (node->type == CRAS_NODE_TYPE_USB)
		strcpy(node->name, USB);
	else if (node->type == CRAS_NODE_TYPE_HDMI)
		strcpy(node->name, HDMI);
	else {
		/* Only HDMI or USB node might have invalid name to drop */
		syslog(LOG_ERR, "Unexpectedly drop node name for "
		       "node: %s, type: %d", node->name, node->type);
		strcpy(node->name, DEFAULT);
	}
}

/*
 * Sets the initial plugged state and type of a node based on its
 * name. Chrome will assign priority to nodes base on node type.
 */
static void set_node_initial_state(struct cras_ionode *node,
				   enum CRAS_ALSA_CARD_TYPE card_type)
{

	unsigned i;

	node->volume = 100;
	node->type = CRAS_NODE_TYPE_UNKNOWN;
	/* Go through the known names */
	for (i = 0; i < ARRAY_SIZE(node_defaults); i++)
		if (!strncmp(node->name, node_defaults[i].name,
			     strlen(node_defaults[i].name))) {
			node->position = node_defaults[i].position;
			node->plugged = (node->position
					!= NODE_POSITION_EXTERNAL);
			node->type = node_defaults[i].type;
			if (node->plugged)
				gettimeofday(&node->plugged_time, NULL);
			break;
		}

	/* If we didn't find a matching name above, but the node is a jack node,
	 * set its type to headphone/mic. This matches node names like "DAISY-I2S Mic
	 * Jack".
	 * If HDMI is in the node name, set its type to HDMI. This matches node names
	 * like "Rockchip HDMI Jack".
	 */
	if (i == ARRAY_SIZE(node_defaults)) {
		if (endswith(node->name, "Jack")) {
			if (node->dev->direction == CRAS_STREAM_OUTPUT)
				node->type = CRAS_NODE_TYPE_HEADPHONE;
			else
				node->type = CRAS_NODE_TYPE_MIC;
		}
		if (strstr(node->name, HDMI) &&
		    node->dev->direction == CRAS_STREAM_OUTPUT)
			node->type = CRAS_NODE_TYPE_HDMI;
	}

	/* Regardless of the node name of a USB headset (it can be "Speaker"),
	 * set it's type to usb.
	 */
	if (card_type == ALSA_CARD_TYPE_USB) {
		node->type = CRAS_NODE_TYPE_USB;
		node->position = NODE_POSITION_EXTERNAL;
	}

	if (!is_utf8_string(node->name))
		drop_node_name(node);
}

static int get_ucm_flag_integer(struct alsa_io *aio,
				const char *flag_name,
				int *result)
{
	char *value;
	int i;

	if (!aio->ucm)
		return -1;

	value = ucm_get_flag(aio->ucm, flag_name);
	if (!value)
		return -1;

	i = atoi(value);
	free(value);
	*result = i;
	return 0;
}

static int auto_unplug_input_node(struct alsa_io *aio)
{
	int result;
	if (get_ucm_flag_integer(aio, "AutoUnplugInputNode", &result))
		return 0;
	return result;
}

static int auto_unplug_output_node(struct alsa_io *aio)
{
	int result;
	if (get_ucm_flag_integer(aio, "AutoUnplugOutputNode", &result))
		return 0;
	return result;
}

static int no_create_default_input_node(struct alsa_io *aio)
{
	int result;
	if (get_ucm_flag_integer(aio, "NoCreateDefaultInputNode", &result))
		return 0;
	return result;
}

static int no_create_default_output_node(struct alsa_io *aio)
{
	int result;
	if (get_ucm_flag_integer(aio, "NoCreateDefaultOutputNode", &result))
		return 0;
	return result;
}

static void set_output_node_software_volume_needed(
	struct alsa_output_node *output, struct alsa_io *aio)
{

	struct cras_alsa_mixer *mixer = aio->mixer;
	long range = 0;

	if (aio->ucm && ucm_get_disable_software_volume(aio->ucm)) {
		output->base.software_volume_needed = 0;
		syslog(LOG_DEBUG, "Disable software volume for %s from ucm.",
		       output->base.name);
		return;
	}

	/* Use software volume for HDMI output and nodes without volume mixer
	 * control. */
	if ((output->base.type == CRAS_NODE_TYPE_HDMI) ||
	    (!cras_alsa_mixer_has_main_volume(mixer) &&
	     !cras_alsa_mixer_has_volume(output->mixer_output)))
		output->base.software_volume_needed = 1;

	/* Use software volume if the usb device's volume range is smaller
	 * than 40dB */
	if (output->base.type == CRAS_NODE_TYPE_USB) {
		range += cras_alsa_mixer_get_dB_range(mixer);
		range += cras_alsa_mixer_get_output_dB_range(
				output->mixer_output);
		if (range < 4000)
			output->base.software_volume_needed = 1;
	}
	if (output->base.software_volume_needed)
		syslog(LOG_DEBUG, "Use software volume for node: %s",
		       output->base.name);
}

static void set_input_node_software_volume_needed(
	struct alsa_input_node *input, struct alsa_io *aio)
{
	long max_software_gain;
	int rc;

	input->base.software_volume_needed = 0;
	input->base.max_software_gain = 0;

	/* Enable software gain only if max software gain is specified in UCM.*/
	if (!aio->ucm)
		return;

	rc = ucm_get_max_software_gain(aio->ucm, input->base.name,
	                               &max_software_gain);
	if (rc)
		return;

	input->base.software_volume_needed = 1;
	input->base.max_software_gain = max_software_gain;
	syslog(LOG_INFO,
	       "Use software gain for %s with max %ld because it is specified"
	       " in UCM", input->base.name, max_software_gain);
}

static void set_input_default_node_gain(struct alsa_input_node *input,
					struct alsa_io *aio)
{
	long default_node_gain;
	int rc;

	if (!aio->ucm)
		return;

	rc = ucm_get_default_node_gain(aio->ucm, input->base.name,
					 &default_node_gain);
	if (rc)
		return;

	input->base.capture_gain = default_node_gain;
}

static void check_auto_unplug_output_node(struct alsa_io *aio,
					  struct cras_ionode *node,
					  int plugged)
{
	struct cras_ionode *tmp;

	if (!auto_unplug_output_node(aio))
		return;

	/* Auto unplug internal speaker if any output node has been created */
	if (!strcmp(node->name, INTERNAL_SPEAKER) && plugged) {
		DL_FOREACH(aio->base.nodes, tmp)
			if (tmp->plugged && (tmp != node))
				cras_iodev_set_node_attr(node,
							 IONODE_ATTR_PLUGGED,
							 0);
	} else {
		DL_FOREACH(aio->base.nodes, tmp) {
			if (!strcmp(tmp->name, INTERNAL_SPEAKER))
				cras_iodev_set_node_attr(tmp,
							 IONODE_ATTR_PLUGGED,
							 !plugged);
		}
	}
}

/*
 * Callback for listing mixer outputs. The mixer will call this once for each
 * output associated with this device. Most commonly this is used to tell the
 * device it has Headphones and Speakers.
 */
static struct alsa_output_node *new_output(struct alsa_io *aio,
					   struct mixer_control *cras_output,
					   const char *name)
{
	struct alsa_output_node *output;
	syslog(LOG_DEBUG, "New output node for '%s'", name);
	if (aio == NULL) {
		syslog(LOG_ERR, "Invalid aio when listing outputs.");
		return NULL;
	}
	output = (struct alsa_output_node *)calloc(1, sizeof(*output));
	if (output == NULL) {
		syslog(LOG_ERR, "Out of memory when listing outputs.");
		return NULL;
	}
	output->base.dev = &aio->base;
	output->base.idx = aio->next_ionode_index++;
	output->base.stable_id = SuperFastHash(name,
					       strlen(name),
					       aio->base.info.stable_id);
	output->base.stable_id_new = SuperFastHash(name,
						   strlen(name),
						   aio->base.info.stable_id_new
						   );
	output->mixer_output = cras_output;

	/* Volume curve. */
	output->volume_curve = cras_card_config_get_volume_curve_for_control(
			aio->config,
			name ? name
			     : cras_alsa_mixer_get_control_name(cras_output));

	strncpy(output->base.name, name, sizeof(output->base.name) - 1);
	set_node_initial_state(&output->base, aio->card_type);
	set_output_node_software_volume_needed(output, aio);

	cras_iodev_add_node(&aio->base, &output->base);

	check_auto_unplug_output_node(aio, &output->base, output->base.plugged);
	return output;
}

static void new_output_by_mixer_control(struct mixer_control *cras_output,
				        void *callback_arg)
{
	struct alsa_io *aio = (struct alsa_io *)callback_arg;
	char node_name[CRAS_IODEV_NAME_BUFFER_SIZE];
	const char *ctl_name;

	ctl_name = cras_alsa_mixer_get_control_name(cras_output);
	if (!ctl_name)
	        return;

	if (aio->card_type == ALSA_CARD_TYPE_USB) {
		snprintf(node_name, sizeof(node_name), "%s: %s",
			aio->base.info.name, ctl_name);
		new_output(aio, cras_output, node_name);
	} else {
		new_output(aio, cras_output, ctl_name);
	}
}

static void check_auto_unplug_input_node(struct alsa_io *aio,
					 struct cras_ionode *node,
					 int plugged)
{
	struct cras_ionode *tmp;
	if (!auto_unplug_input_node(aio))
		return;

	/* Auto unplug internal mic if any input node has already
	 * been created */
	if (!strcmp(node->name, INTERNAL_MICROPHONE) && plugged) {
		DL_FOREACH(aio->base.nodes, tmp)
			if (tmp->plugged && (tmp != node))
				cras_iodev_set_node_attr(node,
							 IONODE_ATTR_PLUGGED,
							 0);
	} else {
		DL_FOREACH(aio->base.nodes, tmp)
			if (!strcmp(tmp->name, INTERNAL_MICROPHONE))
				cras_iodev_set_node_attr(tmp,
							 IONODE_ATTR_PLUGGED,
							 !plugged);
	}
}

static struct alsa_input_node *new_input(struct alsa_io *aio,
		struct mixer_control *cras_input, const char *name)
{
	struct alsa_input_node *input;
	char *mic_positions;
	int err;

	input = (struct alsa_input_node *)calloc(1, sizeof(*input));
	if (input == NULL) {
		syslog(LOG_ERR, "Out of memory when listing inputs.");
		return NULL;
	}
	input->base.dev = &aio->base;
	input->base.idx = aio->next_ionode_index++;
	input->base.stable_id = SuperFastHash(name,
					      strlen(name),
					      aio->base.info.stable_id);
	input->base.stable_id_new = SuperFastHash(name,
						  strlen(name),
						  aio->base.info.stable_id_new);
	input->mixer_input = cras_input;
	strncpy(input->base.name, name, sizeof(input->base.name) - 1);
	set_node_initial_state(&input->base, aio->card_type);
	set_input_node_software_volume_needed(input, aio);
	set_input_default_node_gain(input, aio);

	if (aio->ucm) {
		/* Check mic positions only for internal mic. */
		if ((input->base.type == CRAS_NODE_TYPE_MIC) &&
		    (input->base.position == NODE_POSITION_INTERNAL)) {
			mic_positions = ucm_get_mic_positions(aio->ucm);
			if (mic_positions) {
				strncpy(input->base.mic_positions,
					mic_positions,
					sizeof(input->base.mic_positions) - 1);
				free(mic_positions);
			}
		}

		/* Check if channel map is specified in UCM. */
		input->channel_layout = (int8_t *)malloc(
				CRAS_CH_MAX * sizeof(*input->channel_layout));
		err = ucm_get_capture_chmap_for_dev(aio->ucm, name,
						    input->channel_layout);
		if (err) {
			free(input->channel_layout);
			input->channel_layout = 0;
		}
	}

	cras_iodev_add_node(&aio->base, &input->base);
	check_auto_unplug_input_node(aio, &input->base,
				     input->base.plugged);
	return input;
}

static void new_input_by_mixer_control(struct mixer_control *cras_input,
				       void *callback_arg)
{
	struct alsa_io *aio = (struct alsa_io *)callback_arg;
	char node_name[CRAS_IODEV_NAME_BUFFER_SIZE];
	const char *ctl_name = cras_alsa_mixer_get_control_name(cras_input);

	if (aio->card_type == ALSA_CARD_TYPE_USB) {
		snprintf(node_name , sizeof(node_name), "%s: %s",
			 aio->base.info.name, ctl_name);
		new_input(aio, cras_input, node_name);
	} else {
		new_input(aio, cras_input, ctl_name);
	}
}

/*
 * Finds the output node associated with the jack. Returns NULL if not found.
 */
static struct alsa_output_node *get_output_node_from_jack(
		struct alsa_io *aio, const struct cras_alsa_jack *jack)
{
	struct mixer_control *mixer_output;
	struct cras_ionode *node = NULL;
	struct alsa_output_node *aout = NULL;

	/* Search by jack first. */
	DL_SEARCH_SCALAR_WITH_CAST(aio->base.nodes, node, aout,
				   jack, jack);
	if (aout)
		return aout;

	/* Search by mixer control next. */
	mixer_output = cras_alsa_jack_get_mixer_output(jack);
	if (mixer_output == NULL)
		return NULL;

	DL_SEARCH_SCALAR_WITH_CAST(aio->base.nodes, node, aout,
				   mixer_output, mixer_output);
	return aout;
}

static struct alsa_input_node *get_input_node_from_jack(
		struct alsa_io *aio, const struct cras_alsa_jack *jack)
{
	struct mixer_control *mixer_input;
	struct cras_ionode *node = NULL;
	struct alsa_input_node *ain = NULL;

	mixer_input = cras_alsa_jack_get_mixer_input(jack);
	if (mixer_input == NULL) {
		DL_SEARCH_SCALAR_WITH_CAST(aio->base.nodes, node, ain,
					   jack, jack);
		return ain;
	}

	DL_SEARCH_SCALAR_WITH_CAST(aio->base.nodes, node, ain,
				   mixer_input, mixer_input);
	return ain;
}

/*
 * Returns the dsp name specified in the ucm config. If there is a dsp
 * name specified for the jack of the active node, use that. Otherwise
 * use the default dsp name for the alsa_io device.
 */
static const char *get_active_dsp_name(struct alsa_io *aio)
{
	struct cras_ionode *node = aio->base.active_node;
	const struct cras_alsa_jack *jack;

	if (node == NULL)
		return NULL;

	if (aio->base.direction == CRAS_STREAM_OUTPUT)
		jack = ((struct alsa_output_node *) node)->jack;
	else
		jack = ((struct alsa_input_node *) node)->jack;

	return cras_alsa_jack_get_dsp_name(jack) ? : aio->dsp_name_default;
}

/*
 * Creates volume curve for the node associated with given jack.
 */
static struct cras_volume_curve *create_volume_curve_for_jack(
		const struct cras_card_config *config,
		const struct cras_alsa_jack *jack)
{
	struct cras_volume_curve *curve;
	const char *name;

	/* Use jack's UCM device name as key to get volume curve. */
	name = cras_alsa_jack_get_ucm_device(jack);
	curve = cras_card_config_get_volume_curve_for_control(config, name);
	if (curve)
		return curve;

	/* Use alsa jack's name as key to get volume curve. */
	name = cras_alsa_jack_get_name(jack);
	curve = cras_card_config_get_volume_curve_for_control(config, name);
	if (curve)
		return curve;

	return NULL;
}

/*
 * Callback that is called when an output jack is plugged or unplugged.
 */
static void jack_output_plug_event(const struct cras_alsa_jack *jack,
				    int plugged,
				    void *arg)
{
	struct alsa_io *aio;
	struct alsa_output_node *node;
	const char *jack_name;

	if (arg == NULL)
		return;

	aio = (struct alsa_io *)arg;
	node = get_output_node_from_jack(aio, jack);
	jack_name = cras_alsa_jack_get_name(jack);
	if (!strcmp(jack_name, "Speaker Phantom Jack"))
		jack_name = INTERNAL_SPEAKER;

	/* If there isn't a node for this jack, create one. */
	if (node == NULL) {
		if (aio->fully_specified) {
			/* When fully specified, can't have new nodes. */
			syslog(LOG_ERR, "No matching output node for jack %s!",
			       jack_name);
			return;
		}
		node = new_output(aio, NULL, jack_name);
		if (node == NULL)
			return;

		cras_alsa_jack_update_node_type(jack, &(node->base.type));
	}

	if (!node->jack) {
		if (aio->fully_specified)
			syslog(LOG_ERR,
			       "Jack '%s' was found to match output node '%s'."
			       " Please fix your UCM configuration to match.",
			       jack_name, node->base.name);

		/* If we already have the node, associate with the jack. */
		node->jack = jack;
		if (node->volume_curve == NULL)
			node->volume_curve = create_volume_curve_for_jack(
					aio->config, jack);
	}

	syslog(LOG_DEBUG, "%s plugged: %d, %s", jack_name, plugged,
	       cras_alsa_mixer_get_control_name(node->mixer_output));

	cras_alsa_jack_update_monitor_name(jack, node->base.name,
					   sizeof(node->base.name));
	/* The name got from jack might be an invalid UTF8 string. */
	if (!is_utf8_string(node->base.name))
		drop_node_name(&node->base);

	cras_iodev_set_node_attr(&node->base, IONODE_ATTR_PLUGGED, plugged);

	check_auto_unplug_output_node(aio, &node->base, plugged);
}

/*
 * Callback that is called when an input jack is plugged or unplugged.
 */
static void jack_input_plug_event(const struct cras_alsa_jack *jack,
				  int plugged,
				  void *arg)
{
	struct alsa_io *aio;
	struct alsa_input_node *node;
	struct mixer_control *cras_input;
	const char *jack_name;

	if (arg == NULL)
		return;
	aio = (struct alsa_io *)arg;
	node = get_input_node_from_jack(aio, jack);
	jack_name = cras_alsa_jack_get_name(jack);

	/* If there isn't a node for this jack, create one. */
	if (node == NULL) {
		if (aio->fully_specified) {
			/* When fully specified, can't have new nodes. */
			syslog(LOG_ERR, "No matching input node for jack %s!",
			       jack_name);
			return;
		}
		cras_input = cras_alsa_jack_get_mixer_input(jack);
		node = new_input(aio, cras_input, jack_name);
		if (node == NULL)
			return;
	}

	syslog(LOG_DEBUG, "%s plugged: %d, %s", jack_name, plugged,
	       cras_alsa_mixer_get_control_name(node->mixer_input));

	/* If we already have the node, associate with the jack. */
	if (!node->jack) {
		if (aio->fully_specified)
			syslog(LOG_ERR,
			       "Jack '%s' was found to match input node '%s'."
			       " Please fix your UCM configuration to match.",
			       jack_name, node->base.name);
		node->jack = jack;
	}

	cras_iodev_set_node_attr(&node->base, IONODE_ATTR_PLUGGED, plugged);

	check_auto_unplug_input_node(aio, &node->base, plugged);
}

/*
 * Sets the name of the given iodev, using the name and index of the card
 * combined with the device index and direction.
 */
static void set_iodev_name(struct cras_iodev *dev,
			   const char *card_name,
			   const char *dev_name,
			   size_t card_index,
			   size_t device_index,
			   enum CRAS_ALSA_CARD_TYPE card_type,
			   size_t usb_vid,
			   size_t usb_pid,
			   char *usb_serial_number)
{
	snprintf(dev->info.name,
		 sizeof(dev->info.name),
		 "%s: %s:%zu,%zu",
		 card_name,
		 dev_name,
		 card_index,
		 device_index);
	dev->info.name[ARRAY_SIZE(dev->info.name) - 1] = '\0';
	syslog(LOG_DEBUG, "Add device name=%s", dev->info.name);

	dev->info.stable_id = SuperFastHash(card_name,
					    strlen(card_name),
					    strlen(card_name));
	dev->info.stable_id = SuperFastHash(dev_name,
					    strlen(dev_name),
					    dev->info.stable_id);

	switch (card_type) {
	case ALSA_CARD_TYPE_INTERNAL:
		dev->info.stable_id = SuperFastHash((const char *)&device_index,
						    sizeof(device_index),
						    dev->info.stable_id);
		dev->info.stable_id_new = dev->info.stable_id;
		break;
	case ALSA_CARD_TYPE_USB:
		dev->info.stable_id = SuperFastHash((const char *)&usb_vid,
						    sizeof(usb_vid),
						    dev->info.stable_id);
		dev->info.stable_id = SuperFastHash((const char *)&usb_pid,
						    sizeof(usb_pid),
						    dev->info.stable_id);
		dev->info.stable_id_new =
			SuperFastHash(usb_serial_number,
				      strlen(usb_serial_number),
				      dev->info.stable_id);
		break;
	default:
		dev->info.stable_id_new = dev->info.stable_id;
		break;
	}
	syslog(LOG_DEBUG, "Stable ID=%08x, New Stable ID=%08x",
	       dev->info.stable_id, dev->info.stable_id_new);
}

static int get_fixed_rate(struct alsa_io *aio)
{
	const char *name;

	if (aio->base.direction == CRAS_STREAM_OUTPUT) {
		struct alsa_output_node *active = get_active_output(aio);
		if (!active)
			return -ENOENT;
		name = active->base.name;
	} else {
		struct alsa_input_node *active = get_active_input(aio);
		if (!active)
			return -ENOENT;
		name = active->base.name;
	}

	return ucm_get_sample_rate_for_dev(aio->ucm, name, aio->base.direction);
}

/*
 * Updates the supported sample rates and channel counts.
 */
static int update_supported_formats(struct cras_iodev *iodev)
{
	struct alsa_io *aio = (struct alsa_io *)iodev;
	int err;
	int fixed_rate;

	free(iodev->supported_rates);
	iodev->supported_rates = NULL;
	free(iodev->supported_channel_counts);
	iodev->supported_channel_counts = NULL;
	free(iodev->supported_formats);
	iodev->supported_formats = NULL;

	err = cras_alsa_fill_properties(aio->dev, aio->alsa_stream,
					&iodev->supported_rates,
					&iodev->supported_channel_counts,
					&iodev->supported_formats);
	if (err)
		return err;

	if (aio->ucm) {
		/* Allow UCM to override supplied rates. */
		fixed_rate = get_fixed_rate(aio);
		if (fixed_rate > 0) {
			free(iodev->supported_rates);
			iodev->supported_rates = (size_t*)malloc(
					2 * sizeof(iodev->supported_rates[0]));
			iodev->supported_rates[0] = fixed_rate;
			iodev->supported_rates[1] = 0;
		}
	}
	return 0;
}

/*
 * Builds software volume scalers for output nodes in the device.
 */
static void build_softvol_scalers(struct alsa_io *aio)
{
	struct cras_ionode *ionode;

	DL_FOREACH(aio->base.nodes, ionode) {
		struct alsa_output_node *aout;
		const struct cras_volume_curve *curve;

		aout = (struct alsa_output_node *)ionode;
		curve = get_curve_for_output_node(aio, aout);

		ionode->softvol_scalers = softvol_build_from_curve(curve);
	}
}

static void enable_active_ucm(struct alsa_io *aio, int plugged)
{
	const struct cras_alsa_jack *jack;
	const char *name;

	if (aio->base.direction == CRAS_STREAM_OUTPUT) {
		struct alsa_output_node *active = get_active_output(aio);
		if (!active)
			return;
		name = active->base.name;
		jack = active->jack;
	} else {
		struct alsa_input_node *active = get_active_input(aio);
		if (!active)
			return;
		name = active->base.name;
		jack = active->jack;
	}

	if (jack)
		cras_alsa_jack_enable_ucm(jack, plugged);
	else if (aio->ucm)
		ucm_set_enabled(aio->ucm, name, plugged);
}

static int fill_whole_buffer_with_zeros(struct cras_iodev *iodev)
{
	struct alsa_io *aio = (struct alsa_io *)iodev;
	int rc;
	uint8_t *dst = NULL;
	size_t format_bytes;

	/* Fill whole buffer with zeros. */
	rc = cras_alsa_mmap_get_whole_buffer(
			aio->handle, &dst, &aio->num_underruns);

	if (rc < 0) {
		syslog(LOG_ERR, "Failed to get whole buffer: %s",
		       snd_strerror(rc));
		return rc;
	}

	format_bytes = cras_get_format_bytes(iodev->format);
	memset(dst, 0, iodev->buffer_size * format_bytes);

	return 0;
}

static int adjust_appl_ptr(struct cras_iodev *odev)
{
	struct alsa_io *aio = (struct alsa_io *)odev;

	/* Move appl_ptr to min_buffer_level + min_cb_level frames ahead of
	 * hw_ptr when resuming from free run or adjusting appl_ptr from
	 * underrun. */
	return cras_alsa_resume_appl_ptr(
			aio->handle,
			odev->min_buffer_level + odev->min_cb_level);
}

static int alsa_output_underrun(struct cras_iodev *odev)
{
	int rc;
	/* Fill whole buffer with zeros. This avoids samples left in buffer causing
	 * noise when device plays them. */
	rc = fill_whole_buffer_with_zeros(odev);
	if (rc)
		return rc;
	/* Adjust appl_ptr to leave underrun. */
	return adjust_appl_ptr(odev);
}

static int possibly_enter_free_run(struct cras_iodev *odev)
{
	struct alsa_io *aio = (struct alsa_io *)odev;
	int rc;
	unsigned int hw_level, fr_to_write;
	unsigned int target_hw_level = odev->min_cb_level * 2;
	struct timespec hw_tstamp;

	if (aio->is_free_running)
		return 0;

	/* Check if all valid samples are played.
	 * If all valid samples are played, fill whole buffer with zeros. */
	rc = cras_iodev_frames_queued(odev, &hw_tstamp);
	if (rc < 0)
		return rc;
	hw_level = rc;

	if (hw_level < aio->filled_zeros_for_draining || hw_level == 0) {
		rc = fill_whole_buffer_with_zeros(odev);
		if (rc < 0)
			return rc;
		aio->is_free_running = 1;
		return 0;
	}

	/* Fill some zeros to drain valid samples. */
	fr_to_write = cras_iodev_buffer_avail(odev, hw_level);

	if (hw_level <= target_hw_level) {
		fr_to_write = MIN(target_hw_level - hw_level, fr_to_write);
		rc = cras_iodev_fill_odev_zeros(odev, fr_to_write);
		if (rc)
			return rc;
		aio->filled_zeros_for_draining += fr_to_write;
	}

	return 0;
}

static int leave_free_run(struct cras_iodev *odev)
{
	struct alsa_io *aio = (struct alsa_io *)odev;
	int rc;

	if (!aio->is_free_running)
		return 0;

	rc = adjust_appl_ptr(odev);
	if (rc) {
		syslog(LOG_ERR, "device %s failed to leave free run, rc = %d",
		       odev->info.name, rc);
		return rc;
	}
	aio->is_free_running = 0;
	aio->filled_zeros_for_draining = 0;

	return 0;
}

/*
 * Free run state is the optimization of no_stream playback on alsa_io.
 * The whole buffer will be filled with zeros. Device can play these zeros
 * indefinitely. When there is new meaningful sample, appl_ptr should be
 * resumed to some distance ahead of hw_ptr.
 */
static int no_stream(struct cras_iodev *odev, int enable)
{
	if (enable)
		return possibly_enter_free_run(odev);
	else
		return leave_free_run(odev);
}

static int output_should_wake(const struct cras_iodev *odev)
{
	struct alsa_io *aio = (struct alsa_io *)odev;
	if (aio->is_free_running)
		return 0;
	else
		return ((cras_iodev_state(odev) ==
					CRAS_IODEV_STATE_NO_STREAM_RUN) ||
		        (cras_iodev_state(odev) ==
					CRAS_IODEV_STATE_NORMAL_RUN));
}

static unsigned int get_num_underruns(const struct cras_iodev *iodev)
{
	const struct alsa_io *aio = (const struct alsa_io *)iodev;
	return aio->num_underruns;
}

static unsigned int get_num_severe_underruns(const struct cras_iodev *iodev)
{
	const struct alsa_io *aio = (const struct alsa_io *)iodev;
	return aio->num_severe_underruns;
}

static void set_default_hotword_model(struct cras_iodev *iodev)
{
	const char *default_model = "en_us";
	cras_node_id_t node_id;

	if (!iodev->active_node ||
	     iodev->active_node->type != CRAS_NODE_TYPE_HOTWORD)
		return;

	node_id = cras_make_node_id(iodev->info.idx, iodev->active_node->idx);
	/* This is a no-op if the default_model is not supported */
	cras_iodev_list_set_hotword_model(node_id, default_model);
}

/*
 * Exported Interface.
 */

struct cras_iodev *alsa_iodev_create(size_t card_index,
				     const char *card_name,
				     size_t device_index,
				     const char *dev_name,
				     const char *dev_id,
				     enum CRAS_ALSA_CARD_TYPE card_type,
				     int is_first,
				     struct cras_alsa_mixer *mixer,
				     const struct cras_card_config *config,
				     struct cras_use_case_mgr *ucm,
				     snd_hctl_t *hctl,
				     enum CRAS_STREAM_DIRECTION direction,
				     size_t usb_vid,
				     size_t usb_pid,
				     char *usb_serial_number)
{
	struct alsa_io *aio;
	struct cras_iodev *iodev;

	if (direction != CRAS_STREAM_INPUT && direction != CRAS_STREAM_OUTPUT)
		return NULL;

	aio = (struct alsa_io *)calloc(1, sizeof(*aio));
	if (!aio)
		return NULL;
	iodev = &aio->base;
	iodev->direction = direction;

	aio->device_index = device_index;
	aio->card_type = card_type;
	aio->is_first = is_first;
	aio->handle = NULL;
	aio->num_severe_underruns = 0;
	if (dev_name) {
		aio->dev_name = strdup(dev_name);
		if (!aio->dev_name)
			goto cleanup_iodev;
	}
	if (dev_id) {
		aio->dev_id = strdup(dev_id);
		if (!aio->dev_id)
			goto cleanup_iodev;
	}
	aio->is_free_running = 0;
	aio->filled_zeros_for_draining = 0;
	aio->dev = (char *)malloc(MAX_ALSA_DEV_NAME_LENGTH);
	if (aio->dev == NULL)
		goto cleanup_iodev;
	snprintf(aio->dev,
		 MAX_ALSA_DEV_NAME_LENGTH,
		 "hw:%zu,%zu",
		 card_index,
		 device_index);

	if (direction == CRAS_STREAM_INPUT) {
		aio->alsa_stream = SND_PCM_STREAM_CAPTURE;
		aio->base.set_capture_gain = set_alsa_capture_gain;
		aio->base.set_capture_mute = set_alsa_capture_gain;
	} else {
		aio->alsa_stream = SND_PCM_STREAM_PLAYBACK;
		aio->base.set_volume = set_alsa_volume;
		aio->base.set_mute = set_alsa_mute;
		aio->base.output_underrun = alsa_output_underrun;
	}
	iodev->open_dev = open_dev;
	iodev->close_dev = close_dev;
	iodev->update_supported_formats = update_supported_formats;
	iodev->frames_queued = frames_queued;
	iodev->delay_frames = delay_frames;
	iodev->get_buffer = get_buffer;
	iodev->put_buffer = put_buffer;
	iodev->flush_buffer = flush_buffer;
	iodev->start = start;
	iodev->update_active_node = update_active_node;
	iodev->update_channel_layout = update_channel_layout;
	iodev->set_hotword_model = set_hotword_model;
	iodev->get_hotword_models = get_hotword_models;
	iodev->no_stream = no_stream;
	iodev->output_should_wake = output_should_wake;
	iodev->get_num_underruns = get_num_underruns;
	iodev->get_num_severe_underruns = get_num_severe_underruns;
	iodev->set_swap_mode_for_node = cras_iodev_dsp_set_swap_mode_for_node;

	if (card_type == ALSA_CARD_TYPE_USB)
		iodev->min_buffer_level = USB_EXTRA_BUFFER_FRAMES;

	iodev->ramp = cras_ramp_create();
	if (iodev->ramp == NULL)
		goto cleanup_iodev;

	aio->mixer = mixer;
	aio->config = config;
	if (direction == CRAS_STREAM_OUTPUT) {
		aio->default_volume_curve =
				cras_card_config_get_volume_curve_for_control(
						config, "Default");
		if (aio->default_volume_curve == NULL)
			aio->default_volume_curve =
					cras_volume_curve_create_default();
	}
	aio->ucm = ucm;
	if (ucm) {
		unsigned int level;

		aio->dsp_name_default = ucm_get_dsp_name_default(ucm,
								 direction);
		/* Set callback for swap mode if it is supported
		 * in ucm modifier. */
		if (ucm_swap_mode_exists(ucm))
			aio->base.set_swap_mode_for_node =
				set_alsa_node_swapped;

		level = ucm_get_min_buffer_level(ucm);
		if (level && direction == CRAS_STREAM_OUTPUT)
			iodev->min_buffer_level = level;

		aio->enable_htimestamp =
			ucm_get_enable_htimestamp_flag(ucm);
	}

	set_iodev_name(iodev, card_name, dev_name, card_index, device_index,
		       card_type, usb_vid, usb_pid, usb_serial_number);

	aio->jack_list =
		cras_alsa_jack_list_create(
			card_index,
			card_name,
			device_index,
			is_first,
			mixer,
			ucm,
			hctl,
			direction,
			direction == CRAS_STREAM_OUTPUT ?
				     jack_output_plug_event :
				     jack_input_plug_event,
			aio);
	if (!aio->jack_list)
		goto cleanup_iodev;

	/* HDMI outputs don't have volume adjustment, do it in software. */
	if (direction == CRAS_STREAM_OUTPUT && strstr(dev_name, HDMI))
		iodev->software_volume_needed = 1;

	/* Add this now so that cleanup of the iodev (in case of error or card
	 * card removal will function as expected. */
	if (direction == CRAS_STREAM_OUTPUT)
		cras_iodev_list_add_output(&aio->base);
	else
		cras_iodev_list_add_input(&aio->base);
	return &aio->base;

cleanup_iodev:
	free_alsa_iodev_resources(aio);
	free(aio);
	return NULL;
}

int alsa_iodev_legacy_complete_init(struct cras_iodev *iodev)
{
	struct alsa_io *aio = (struct alsa_io *)iodev;
	const char *dev_name;
	const char *dev_id;
	enum CRAS_STREAM_DIRECTION direction;
	int err;
	int is_first;
	struct cras_alsa_mixer *mixer;

	if (!aio)
		return -EINVAL;
	direction = iodev->direction;
	dev_name = aio->dev_name;
	dev_id = aio->dev_id;
	is_first = aio->is_first;
	mixer = aio->mixer;

	/* Create output nodes for mixer controls, such as Headphone
	 * and Speaker, only for the first device. */
	if (direction == CRAS_STREAM_OUTPUT && is_first)
		cras_alsa_mixer_list_outputs(mixer,
				new_output_by_mixer_control, aio);
	else if (direction == CRAS_STREAM_INPUT && is_first)
		cras_alsa_mixer_list_inputs(mixer,
				new_input_by_mixer_control, aio);

	err = cras_alsa_jack_list_find_jacks_by_name_matching(aio->jack_list);
	if (err)
		return err;

	/* Create nodes for jacks that aren't associated with an
	 * already existing node. Get an initial read of the jacks for
	 * this device. */
	cras_alsa_jack_list_report(aio->jack_list);

	/* Make a default node if there is still no node for this
	 * device, or we still don't have the "Speaker"/"Internal Mic"
	 * node for the first internal device. Note that the default
	 * node creation can be supressed by UCM flags for platforms
	 * which really don't have an internal device. */
	if ((direction == CRAS_STREAM_OUTPUT) &&
			!no_create_default_output_node(aio)) {
		if (first_internal_device(aio) &&
		    !has_node(aio, INTERNAL_SPEAKER) &&
		    !has_node(aio, HDMI)) {
			if (strstr(aio->base.info.name, HDMI))
				new_output(aio, NULL, HDMI);
			else
				new_output(aio, NULL, INTERNAL_SPEAKER);
		} else if (!aio->base.nodes) {
			new_output(aio, NULL, DEFAULT);
		}
	} else if ((direction == CRAS_STREAM_INPUT) &&
			!no_create_default_input_node(aio)) {
		if (first_internal_device(aio) &&
		    !has_node(aio, INTERNAL_MICROPHONE))
			new_input(aio, NULL, INTERNAL_MICROPHONE);
		else if (strstr(dev_name, KEYBOARD_MIC))
			new_input(aio, NULL, KEYBOARD_MIC);
		else if (dev_id && strstr(dev_id, HOTWORD_DEV))
			new_input(aio, NULL, HOTWORD_DEV);
		else if (!aio->base.nodes)
			new_input(aio, NULL, DEFAULT);
	}

	/* Build software volume scalers. */
	if (direction == CRAS_STREAM_OUTPUT)
		build_softvol_scalers(aio);

	/* Set the active node as the best node we have now. */
	alsa_iodev_set_active_node(&aio->base,
				   first_plugged_node(&aio->base),
				   0);

	/* Set plugged for the first USB device per card when it appears. */
	if (aio->card_type == ALSA_CARD_TYPE_USB && is_first)
		cras_iodev_set_node_attr(iodev->active_node,
					 IONODE_ATTR_PLUGGED, 1);

	set_default_hotword_model(iodev);

	return 0;
}

int alsa_iodev_ucm_add_nodes_and_jacks(struct cras_iodev *iodev,
				       struct ucm_section *section)
{
	struct alsa_io *aio = (struct alsa_io *)iodev;
	struct mixer_control *control;
	struct alsa_input_node *input_node = NULL;
	struct cras_alsa_jack *jack;
	struct alsa_output_node *output_node = NULL;
	int rc;

	if (!aio || !section)
		return -EINVAL;
	if ((uint32_t)section->dev_idx != aio->device_index)
		return -EINVAL;

	/* This iodev is fully specified. Avoid automatic node creation. */
	aio->fully_specified = 1;

	/* Check here in case the DmaPeriodMicrosecs flag has only been
	 * specified on one of many device entries with the same PCM. */
	if (!aio->dma_period_set_microsecs)
		aio->dma_period_set_microsecs =
			ucm_get_dma_period_for_dev(aio->ucm, section->name);

	/* Create a node matching this section. If there is a matching
	 * control use that, otherwise make a node without a control. */
	control = cras_alsa_mixer_get_control_for_section(aio->mixer, section);
	if (iodev->direction == CRAS_STREAM_OUTPUT) {
		output_node = new_output(aio, control, section->name);
		if (!output_node)
			return -ENOMEM;
	} else if (iodev->direction == CRAS_STREAM_INPUT) {
		input_node = new_input(aio, control, section->name);
		if (!input_node)
			return -ENOMEM;
	}

	/* Find any jack controls for this device. */
	rc = cras_alsa_jack_list_add_jack_for_section(
					aio->jack_list, section, &jack);
	if (rc)
		return rc;

	/* Associated the jack with the node. */
	if (jack) {
		if (output_node) {
			output_node->jack = jack;
			if (!output_node->volume_curve)
				output_node->volume_curve =
					create_volume_curve_for_jack(
						aio->config, jack);
		} else if (input_node) {
			input_node->jack = jack;
		}
	}
	return 0;
}

void alsa_iodev_ucm_complete_init(struct cras_iodev *iodev)
{
	struct alsa_io *aio = (struct alsa_io *)iodev;

	if (!iodev)
		return;

	/* Get an initial read of the jacks for this device. */
	cras_alsa_jack_list_report(aio->jack_list);

	/* Build software volume scaler. */
	if (iodev->direction == CRAS_STREAM_OUTPUT)
		build_softvol_scalers(aio);

	/* Set the active node as the best node we have now. */
	alsa_iodev_set_active_node(&aio->base,
				   first_plugged_node(&aio->base),
				   0);

	/* Set plugged for the first USB device per card when it appears. */
	if (aio->card_type == ALSA_CARD_TYPE_USB && aio->is_first)
		cras_iodev_set_node_attr(iodev->active_node,
					 IONODE_ATTR_PLUGGED, 1);

	set_default_hotword_model(iodev);
}

void alsa_iodev_destroy(struct cras_iodev *iodev)
{
	struct alsa_io *aio = (struct alsa_io *)iodev;
	int rc;

	cras_alsa_jack_list_destroy(aio->jack_list);
	if (iodev->direction == CRAS_STREAM_INPUT)
		rc = cras_iodev_list_rm_input(iodev);
	else
		rc = cras_iodev_list_rm_output(iodev);

	if (rc == -EBUSY) {
		syslog(LOG_ERR, "Failed to remove iodev %s", iodev->info.name);
		return;
	}

	/* Free resources when device successfully removed. */
	free_alsa_iodev_resources(aio);
	cras_volume_curve_destroy(aio->default_volume_curve);
	free(iodev);
}

unsigned alsa_iodev_index(struct cras_iodev *iodev)
{
	struct alsa_io *aio = (struct alsa_io *)iodev;
	return aio->device_index;
}

int alsa_iodev_has_hctl_jacks(struct cras_iodev *iodev)
{
	struct alsa_io *aio = (struct alsa_io *)iodev;
	return cras_alsa_jack_list_has_hctl_jacks(aio->jack_list);
}

static void alsa_iodev_unmute_node(struct alsa_io *aio,
				   struct cras_ionode *ionode)
{
	struct alsa_output_node *active = (struct alsa_output_node *)ionode;
	struct mixer_control *mixer = active->mixer_output;
	struct alsa_output_node *output;
	struct cras_ionode *node;

	/* If this node is associated with mixer output, unmute the
	 * active mixer output and mute all others, otherwise just set
	 * the node as active and set the volume curve. */
	if (mixer) {
		set_alsa_mute_control(aio, 1);
		/* Unmute the active mixer output, mute all others. */
		DL_FOREACH(aio->base.nodes, node) {
			output = (struct alsa_output_node *)node;
			if (output->mixer_output)
				cras_alsa_mixer_set_output_active_state(
					output->mixer_output, node == ionode);
		}
	}
}

static int alsa_iodev_set_active_node(struct cras_iodev *iodev,
				      struct cras_ionode *ionode,
				      unsigned dev_enabled)
{
	struct alsa_io *aio = (struct alsa_io *)iodev;

	if (iodev->active_node == ionode) {
		enable_active_ucm(aio, dev_enabled);
		init_device_settings(aio);
		return 0;
	}

	/* Disable jack ucm before switching node. */
	enable_active_ucm(aio, 0);
	if (dev_enabled && (iodev->direction == CRAS_STREAM_OUTPUT))
		alsa_iodev_unmute_node(aio, ionode);

	cras_iodev_set_active_node(iodev, ionode);
	aio->base.dsp_name = get_active_dsp_name(aio);
	cras_iodev_update_dsp(iodev);
	enable_active_ucm(aio, dev_enabled);
	/* Setting the volume will also unmute if the system isn't muted. */
	init_device_settings(aio);
	return 0;
}
