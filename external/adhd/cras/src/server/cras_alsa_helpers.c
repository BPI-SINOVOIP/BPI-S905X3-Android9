/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <alsa/asoundlib.h>
#include <limits.h>
#include <stdlib.h>
#include <syslog.h>

#include "cras_alsa_helpers.h"
#include "cras_audio_format.h"
#include "cras_util.h"

/* Macro to convert between snd_pcm_chmap_position(defined in
 * alsa-lib since 1.0.27) and CRAS_CHANNEL, values of which are
 * of the same order but shifted by 3.
 */
#define CH_TO_ALSA(ch) ((ch) + 3)
#define CH_TO_CRAS(ch) ((ch) - 3)

/* Assert the channel is defined in CRAS_CHANNELS. */
#define ALSA_CH_VALID(ch) ((ch >= SND_CHMAP_FL) && (ch <= SND_CHMAP_FRC))

/* Time difference between two consecutive underrun logs. */
#define UNDERRUN_LOG_TIME_SECS 30

/* Chances to give mmap_begin to work. */
static const size_t MAX_MMAP_BEGIN_ATTEMPTS = 3;
/* Time to sleep between resume attempts. */
static const size_t ALSA_SUSPENDED_SLEEP_TIME_US = 250000;

/* What rates should we check for on this dev?
 * Listed in order of preference. 0 terminalted. */
static const size_t test_sample_rates[] = {
	44100,
	48000,
	32000,
	96000,
	22050,
	16000,
	8000,
	4000,
	192000,
	0
};

/* What channel counts shoud be checked on this dev?
 * Listed in order of preference. 0 terminalted. */
static const size_t test_channel_counts[] = {
	10,
	6,
	4,
	2,
	1,
	0
};

static const snd_pcm_format_t test_formats[] = {
	SND_PCM_FORMAT_S16_LE,
	SND_PCM_FORMAT_S24_LE,
	SND_PCM_FORMAT_S32_LE,
	SND_PCM_FORMAT_S24_3LE,
	(snd_pcm_format_t)0
};

/* Looks up the list of channel map for the one can exactly matches
 * the layout specified in fmt.
 */
static snd_pcm_chmap_query_t *cras_chmap_caps_match(
		snd_pcm_chmap_query_t **chmaps,
		struct cras_audio_format *fmt)
{
	size_t ch, i;
	int idx, matches;
	snd_pcm_chmap_query_t **chmap;

	/* Search for channel map that already matches the order */
	for (chmap = chmaps; *chmap; chmap++) {
		if ((*chmap)->map.channels != fmt->num_channels)
			continue;

		matches = 1;
		for (ch = 0; ch < CRAS_CH_MAX; ch++) {
			idx = fmt->channel_layout[ch];
			if (idx == -1)
				continue;
			if ((unsigned)idx >= (*chmap)->map.channels)
				continue;
			if ((*chmap)->map.pos[idx] != CH_TO_ALSA(ch)) {
				matches = 0;
				break;
			}
		}
		if (matches)
			return *chmap;
	}

	/* Search for channel map that can arbitrarily swap order */
	for (chmap = chmaps; *chmap; chmap++) {
		if ((*chmap)->type == SND_CHMAP_TYPE_FIXED ||
		    (*chmap)->map.channels != fmt->num_channels)
			continue;

		matches = 1;
		for (ch = 0; ch < CRAS_CH_MAX; ch++) {
			idx = fmt->channel_layout[ch];
			if (idx == -1)
				continue;
			int found = 0;
			for (i = 0; i < fmt->num_channels; i++) {
				if ((*chmap)->map.pos[i] == CH_TO_ALSA(ch)) {
					found = 1;
					break;
				}
			}
			if (found == 0) {
				matches = 0;
				break;
			}
		}
		if (matches && (*chmap)->type == SND_CHMAP_TYPE_VAR)
			return *chmap;

		/* Check if channel map is a match by arbitrarily swap
		 * pair order */
		matches = 1;
		for (i = 0; i < fmt->num_channels; i += 2) {
			ch = CH_TO_CRAS((*chmap)->map.pos[i]);
			if (fmt->channel_layout[ch] & 0x01) {
				matches = 0;
				break;
			}

			if (fmt->channel_layout[ch] + 1 !=
			    fmt->channel_layout[CH_TO_CRAS(
					    (*chmap)->map.pos[i + 1])]) {
				matches = 0;
				break;
			}
		}
		if (matches)
			return *chmap;
	}

	return NULL;
}

/* When the exact match does not exist, select the best valid
 * channel map which can be supported by means of channel conversion
 * matrix.
 */
static snd_pcm_chmap_query_t *cras_chmap_caps_conv_matrix(
		snd_pcm_chmap_query_t **chmaps,
		struct cras_audio_format *fmt)
{
	float **conv_mtx;
	size_t i;
	snd_pcm_chmap_query_t **chmap;
	struct cras_audio_format *conv_fmt;

	conv_fmt = cras_audio_format_create(fmt->format,
			fmt->frame_rate, fmt->num_channels);

	for (chmap = chmaps; *chmap; chmap++) {
		if ((*chmap)->map.channels != fmt->num_channels)
			continue;
		for (i = 0; i < CRAS_CH_MAX; i++)
			conv_fmt->channel_layout[i] = -1;
		for (i = 0; i < conv_fmt->num_channels; i++) {
			if (!ALSA_CH_VALID((*chmap)->map.pos[i]))
				continue;
			conv_fmt->channel_layout[CH_TO_CRAS(
					(*chmap)->map.pos[i])] = i;
		}

		/* Examine channel map by test creating a conversion matrix
		 * for each candidate. Once a non-null matrix is created,
		 * that channel map is considered supported and select it as
		 * the best match one.
		 */
		conv_mtx = cras_channel_conv_matrix_create(fmt, conv_fmt);
		if (conv_mtx) {
			cras_channel_conv_matrix_destroy(conv_mtx,
						 conv_fmt->num_channels);
			cras_audio_format_destroy(conv_fmt);
			return *chmap;
		}
	}

	cras_audio_format_destroy(conv_fmt);
	return NULL;
}

/* Finds the best channel map for given format and list of channel
 * map capability.
 */
static snd_pcm_chmap_query_t *cras_chmap_caps_best(
		snd_pcm_t *handle,
		snd_pcm_chmap_query_t **chmaps,
		struct cras_audio_format *fmt)
{
	snd_pcm_chmap_query_t **chmap;
	snd_pcm_chmap_query_t *match;

	match = cras_chmap_caps_match(chmaps, fmt);
	if (match)
		return match;

	match = cras_chmap_caps_conv_matrix(chmaps, fmt);
	if (match)
		return match;

	/* For capture stream, choose the first chmap matching channel
	 * count. Channel positions reported in this chmap will be used
	 * to fill correspond channels into client stream.
	 */
	if (snd_pcm_stream(handle) == SND_PCM_STREAM_CAPTURE)
		for (chmap = chmaps; *chmap; chmap++)
			if ((*chmap)->map.channels == fmt->num_channels)
				return *chmap;
	return NULL;
}

int cras_alsa_pcm_open(snd_pcm_t **handle, const char *dev,
		       snd_pcm_stream_t stream)
{
	int rc;
	int retries = 3;
	static const unsigned int OPEN_RETRY_DELAY_US = 100000;

retry_open:
	rc = snd_pcm_open(handle,
			  dev,
			  stream,
			  SND_PCM_NONBLOCK |
			  SND_PCM_NO_AUTO_RESAMPLE |
			  SND_PCM_NO_AUTO_CHANNELS |
			  SND_PCM_NO_AUTO_FORMAT);
	if (rc == -EBUSY && --retries) {
		usleep(OPEN_RETRY_DELAY_US);
		goto retry_open;
	}

	return rc;
}

int cras_alsa_pcm_close(snd_pcm_t *handle)
{
	return snd_pcm_close(handle);
}

int cras_alsa_pcm_start(snd_pcm_t *handle)
{
	return snd_pcm_start(handle);
}

int cras_alsa_pcm_drain(snd_pcm_t *handle)
{
	return snd_pcm_drain(handle);
}

int cras_alsa_resume_appl_ptr(snd_pcm_t *handle, snd_pcm_uframes_t ahead)
{
	int rc;
	snd_pcm_uframes_t period_frames, buffer_frames;
	snd_pcm_sframes_t to_move, avail_frames;
	rc = snd_pcm_avail(handle);
	if (rc == -EPIPE || rc == -ESTRPIPE) {
		cras_alsa_attempt_resume(handle);
		avail_frames = 0;
	} else if (rc < 0) {
		syslog(LOG_ERR, "Fail to get avail frames: %s",
		       snd_strerror(rc));
		return rc;
	} else {
		avail_frames = rc;
	}

	rc = snd_pcm_get_params(handle, &buffer_frames, &period_frames);
	if (rc < 0) {
		syslog(LOG_ERR, "Fail to get buffer size: %s",
		       snd_strerror(rc));
		return rc;
	}

	to_move = avail_frames - buffer_frames + ahead;
	if (to_move > 0) {
		rc = snd_pcm_forward(handle, to_move);
	} else if (to_move < 0) {
		rc = snd_pcm_rewind(handle, -to_move);
	} else {
		return 0;
	}

	if (rc < 0) {
		syslog(LOG_ERR, "Fail to resume appl_ptr: %s",
		       snd_strerror(rc));
		return rc;
	}
	return 0;
}

int cras_alsa_set_channel_map(snd_pcm_t *handle,
			      struct cras_audio_format *fmt)
{
	int rc = 0;
	size_t i, ch;
	snd_pcm_chmap_query_t **chmaps;
	snd_pcm_chmap_query_t *match;

	if (fmt->num_channels <= 2)
		return 0;

	chmaps = snd_pcm_query_chmaps(handle);
	if (chmaps == NULL) {
		syslog(LOG_WARNING, "No chmap queried! Skip chmap set");
		rc = 0;
		goto done;
	}

	match = cras_chmap_caps_best(handle, chmaps, fmt);
	if (!match) {
		syslog(LOG_ERR, "Unable to find the best channel map");
		rc = -1;
		goto done;
	}

	/* A channel map could match the layout after channels
	 * pair/arbitrary swapped. Modified the channel positions
	 * before set to HW.
	 */
	for (i = 0; i < fmt->num_channels; i++) {
		for (ch = 0; ch < CRAS_CH_MAX; ch++)
			if (fmt->channel_layout[ch] == (int)i)
				break;
		if (ch != CRAS_CH_MAX)
			match->map.pos[i] = CH_TO_ALSA(ch);
	}
	rc = snd_pcm_set_chmap(handle, &match->map);

done:
	snd_pcm_free_chmaps(chmaps);
	return rc;
}

int cras_alsa_get_channel_map(snd_pcm_t *handle,
			      struct cras_audio_format *fmt)
{
	snd_pcm_chmap_query_t **chmaps;
	snd_pcm_chmap_query_t *match;
	int rc = 0;
	size_t i;

	chmaps = snd_pcm_query_chmaps(handle);
	if (chmaps == NULL) {
		syslog(LOG_ERR, "No chmap queried!");
		rc = -EINVAL;
		goto done;
	}

	match = cras_chmap_caps_best(handle, chmaps, fmt);
	if (!match) {
		syslog(LOG_ERR, "Unable to find the best channel map");
		rc = -1;
		goto done;
	}

	/* Fill back the selected channel map so channel converter can
	 * handle it. */
	for (i = 0; i < CRAS_CH_MAX; i++)
		fmt->channel_layout[i] = -1;
	for (i = 0; i < fmt->num_channels; i++) {
		if (!ALSA_CH_VALID(match->map.pos[i]))
			continue;
		fmt->channel_layout[CH_TO_CRAS(match->map.pos[i])] = i;
	}

	/* Handle the special channel map {SND_CHMAP_MONO} */
	if (match->map.channels == 1 && match->map.pos[0] == SND_CHMAP_MONO)
		fmt->channel_layout[CRAS_CH_FC] = 0;

done:
	snd_pcm_free_chmaps(chmaps);
	return rc;
}

int cras_alsa_fill_properties(const char *dev, snd_pcm_stream_t stream,
			      size_t **rates, size_t **channel_counts,
			      snd_pcm_format_t **formats)
{
	int rc;
	snd_pcm_t *handle;
	size_t i, num_found;
	snd_pcm_hw_params_t *params;

	snd_pcm_hw_params_alloca(&params);

	rc = cras_alsa_pcm_open(&handle,
				dev,
				stream);
	if (rc < 0) {
		syslog(LOG_ERR, "snd_pcm_open_failed: %s", snd_strerror(rc));
		return rc;
	}

	rc = snd_pcm_hw_params_any(handle, params);
	if (rc < 0) {
		snd_pcm_close(handle);
		syslog(LOG_ERR, "snd_pcm_hw_params_any: %s", snd_strerror(rc));
		return rc;
	}

	*rates = (size_t *)malloc(sizeof(test_sample_rates));
	if (*rates == NULL) {
		snd_pcm_close(handle);
		return -ENOMEM;
	}
	*channel_counts = (size_t *)malloc(sizeof(test_channel_counts));
	if (*channel_counts == NULL) {
		free(*rates);
		snd_pcm_close(handle);
		return -ENOMEM;
	}
	*formats = (snd_pcm_format_t *)malloc(sizeof(test_formats));
	if (*formats == NULL) {
		free(*channel_counts);
		free(*rates);
		snd_pcm_close(handle);
		return -ENOMEM;
	}

	num_found = 0;
	for (i = 0; test_sample_rates[i] != 0; i++) {
		rc = snd_pcm_hw_params_test_rate(handle, params,
						 test_sample_rates[i], 0);
		if (rc == 0)
			(*rates)[num_found++] = test_sample_rates[i];
	}
	(*rates)[num_found] = 0;

	num_found = 0;
	for (i = 0; test_channel_counts[i] != 0; i++) {
		rc = snd_pcm_hw_params_test_channels(handle, params,
						     test_channel_counts[i]);
		if (rc == 0)
			(*channel_counts)[num_found++] = test_channel_counts[i];
	}
	(*channel_counts)[num_found] = 0;

	num_found = 0;
	for (i = 0; test_formats[i] != 0; i++) {
		rc = snd_pcm_hw_params_test_format(handle, params,
						   test_formats[i]);
		if (rc == 0)
			(*formats)[num_found++] = test_formats[i];
	}
	(*formats)[num_found] = (snd_pcm_format_t)0;

	snd_pcm_close(handle);

	return 0;
}

int cras_alsa_set_hwparams(snd_pcm_t *handle, struct cras_audio_format *format,
			   snd_pcm_uframes_t *buffer_frames, int period_wakeup,
			   unsigned int dma_period_time)
{
	unsigned int rate, ret_rate;
	int err;
	snd_pcm_hw_params_t *hwparams;

	rate = format->frame_rate;
	snd_pcm_hw_params_alloca(&hwparams);

	err = snd_pcm_hw_params_any(handle, hwparams);
	if (err < 0) {
		syslog(LOG_ERR, "hw_params_any failed %s\n", snd_strerror(err));
		return err;
	}
	/* Disable hardware resampling. */
	err = snd_pcm_hw_params_set_rate_resample(handle, hwparams, 0);
	if (err < 0) {
		syslog(LOG_ERR, "Disabling resampling %s\n", snd_strerror(err));
		return err;
	}
	/* Always interleaved. */
	err = snd_pcm_hw_params_set_access(handle, hwparams,
					   SND_PCM_ACCESS_MMAP_INTERLEAVED);
	if (err < 0) {
		syslog(LOG_ERR, "Setting interleaved %s\n", snd_strerror(err));
		return err;
	}
	/* If period_wakeup flag is not set, try to disable ALSA wakeups,
	 * we'll keep a timer. */
	if (!period_wakeup &&
	    snd_pcm_hw_params_can_disable_period_wakeup(hwparams)) {
		err = snd_pcm_hw_params_set_period_wakeup(handle, hwparams, 0);
		if (err < 0)
			syslog(LOG_WARNING, "disabling wakeups %s\n",
			       snd_strerror(err));
	}
	/* Setup the period time so that the hardware pulls the right amount
	 * of data at the right time. */
	if (dma_period_time) {
		int dir = 0;
		unsigned int original = dma_period_time;

		err = snd_pcm_hw_params_set_period_time_near(
				handle, hwparams, &dma_period_time, &dir);
		if (err < 0) {
			syslog(LOG_ERR, "could not set period time: %s",
			       snd_strerror(err));
			return err;
		} else if (original != dma_period_time) {
			syslog(LOG_DEBUG, "period time set to: %u",
			       dma_period_time);
		}
	}
	/* Set the sample format. */
	err = snd_pcm_hw_params_set_format(handle, hwparams,
					   format->format);
	if (err < 0) {
		syslog(LOG_ERR, "set format %s\n", snd_strerror(err));
		return err;
	}
	/* Set the stream rate. */
	ret_rate = rate;
	err = snd_pcm_hw_params_set_rate_near(handle, hwparams, &ret_rate, 0);
	if (err < 0) {
		syslog(LOG_ERR, "set_rate_near %iHz %s\n", rate,
		       snd_strerror(err));
		return err;
	}
	if (ret_rate != rate) {
		syslog(LOG_ERR, "tried for %iHz, settled for %iHz)\n", rate,
		       ret_rate);
		return -EINVAL;
	}
	/* Set the count of channels. */
	err = snd_pcm_hw_params_set_channels(handle, hwparams,
					     format->num_channels);
	if (err < 0) {
		syslog(LOG_ERR, "set_channels %s\n", snd_strerror(err));
		return err;
	}

	/* Make sure buffer frames is even, or snd_pcm_hw_params will
	 * return invalid argument error. */
	err = snd_pcm_hw_params_get_buffer_size_max(hwparams,
						    buffer_frames);
	if (err < 0)
		syslog(LOG_WARNING, "get buffer max %s\n", snd_strerror(err));

	*buffer_frames &= ~0x01;
	err = snd_pcm_hw_params_set_buffer_size_max(handle, hwparams,
						    buffer_frames);
	if (err < 0) {
		syslog(LOG_ERR, "set_buffer_size_max %s", snd_strerror(err));
		return err;
	}

	syslog(LOG_DEBUG, "buffer size set to %u\n",
	       (unsigned int)*buffer_frames);

	/* Finally, write the parameters to the device. */
	err = snd_pcm_hw_params(handle, hwparams);
	if (err < 0) {
		syslog(LOG_ERR, "hw_params: %s: rate: %u, ret_rate: %u, "
		       "channel: %zu, format: %u\n", snd_strerror(err), rate,
		       ret_rate, format->num_channels, format->format);
		return err;
	}
	return 0;
}

int cras_alsa_set_swparams(snd_pcm_t *handle, int *enable_htimestamp)
{
	int err;
	snd_pcm_sw_params_t *swparams;
	snd_pcm_uframes_t boundary;

	snd_pcm_sw_params_alloca(&swparams);

	err = snd_pcm_sw_params_current(handle, swparams);
	if (err < 0) {
		syslog(LOG_ERR, "sw_params_current: %s\n", snd_strerror(err));
		return err;
	}
	err = snd_pcm_sw_params_get_boundary(swparams, &boundary);
	if (err < 0) {
		syslog(LOG_ERR, "get_boundary: %s\n", snd_strerror(err));
		return err;
	}
	err = snd_pcm_sw_params_set_stop_threshold(handle, swparams, boundary);
	if (err < 0) {
		syslog(LOG_ERR, "set_stop_threshold: %s\n", snd_strerror(err));
		return err;
	}
	/* Don't auto start. */
	err = snd_pcm_sw_params_set_start_threshold(handle, swparams, LONG_MAX);
	if (err < 0) {
		syslog(LOG_ERR, "set_stop_threshold: %s\n", snd_strerror(err));
		return err;
	}

	/* Disable period events. */
	err = snd_pcm_sw_params_set_period_event(handle, swparams, 0);
	if (err < 0) {
		syslog(LOG_ERR, "set_period_event: %s\n", snd_strerror(err));
		return err;
	}

	if (*enable_htimestamp) {
		/* Use MONOTONIC_RAW time-stamps. */
		err = snd_pcm_sw_params_set_tstamp_type(
				handle, swparams,
				SND_PCM_TSTAMP_TYPE_MONOTONIC_RAW);
		if (err < 0) {
			syslog(LOG_ERR, "set_tstamp_type: %s\n",
			       snd_strerror(err));
			return err;
		}
		err = snd_pcm_sw_params_set_tstamp_mode(
				handle, swparams, SND_PCM_TSTAMP_ENABLE);
		if (err < 0) {
			syslog(LOG_ERR, "set_tstamp_mode: %s\n",
			       snd_strerror(err));
			return err;
		}
	}

	/* This hack is required because ALSA-LIB does not provide any way to
	 * detect whether MONOTONIC_RAW timestamps are supported by the kernel.
	 * In ALSA-LIB, the code checks the hardware protocol version. */
	err = snd_pcm_sw_params(handle, swparams);
	if (err == -EINVAL && *enable_htimestamp) {
		*enable_htimestamp = 0;
		syslog(LOG_WARNING,
		       "MONOTONIC_RAW timestamps are not supported.");

		err = snd_pcm_sw_params_set_tstamp_type(
				handle, swparams,
				SND_PCM_TSTAMP_TYPE_GETTIMEOFDAY);
		if (err < 0) {
			syslog(LOG_ERR, "set_tstamp_type: %s\n",
			       snd_strerror(err));
			return err;
		}
		err = snd_pcm_sw_params_set_tstamp_mode(
				handle, swparams, SND_PCM_TSTAMP_NONE);
		if (err < 0) {
			syslog(LOG_ERR, "set_tstamp_mode: %s\n",
			       snd_strerror(err));
			return err;
		}

		err = snd_pcm_sw_params(handle, swparams);
	}

	if (err < 0) {
		syslog(LOG_ERR, "sw_params: %s\n", snd_strerror(err));
		return err;
	}
	return 0;
}

int cras_alsa_get_avail_frames(snd_pcm_t *handle, snd_pcm_uframes_t buf_size,
			       snd_pcm_uframes_t severe_underrun_frames,
			       const char *dev_name,
			       snd_pcm_uframes_t *avail,
			       struct timespec *tstamp,
			       unsigned int *underruns)
{
	snd_pcm_sframes_t frames;
	int rc = 0;
	static struct timespec tstamp_last_underrun_log =
			{.tv_sec = 0, .tv_nsec = 0};

	/* Use snd_pcm_avail still to ensure that the hardware pointer is
	 * up to date. Otherwise, we could use the deprecated snd_pcm_hwsync().
	 * IMO this is a deficiency in the ALSA API.
	 */
	frames = snd_pcm_avail(handle);
	if (frames >= 0)
		rc = snd_pcm_htimestamp(handle, avail, tstamp);
	else
		rc = frames;
	if (rc == -EPIPE || rc == -ESTRPIPE) {
		cras_alsa_attempt_resume(handle);
		rc = 0;
		goto error;
	} else if (rc < 0) {
		syslog(LOG_ERR, "pcm_avail error %s, %s\n",
		       dev_name, snd_strerror(rc));
		goto error;
	} else if (frames >= (snd_pcm_sframes_t)buf_size) {
		struct timespec tstamp_now;
		*underruns = *underruns + 1;
		clock_gettime(CLOCK_MONOTONIC_RAW, &tstamp_now);
		/* Limit the log rate. */
		if ((tstamp_now.tv_sec - tstamp_last_underrun_log.tv_sec) >
		    UNDERRUN_LOG_TIME_SECS) {
			syslog(LOG_ERR,
			       "pcm_avail returned frames larger than buf_size: "
			       "%s: %ld > %lu for %u times\n",
			       dev_name, frames, buf_size, *underruns);
			tstamp_last_underrun_log.tv_sec = tstamp_now.tv_sec;
			tstamp_last_underrun_log.tv_nsec = tstamp_now.tv_nsec;
		}
		if ((frames - (snd_pcm_sframes_t)buf_size) >
		    (snd_pcm_sframes_t)severe_underrun_frames) {
			rc = -EPIPE;
			goto error;
		} else {
			frames = buf_size;
		}
	}
	*avail = frames;
	return 0;

error:
	*avail = 0;
	tstamp->tv_sec = 0;
	tstamp->tv_nsec = 0;
	return rc;
}

int cras_alsa_get_delay_frames(snd_pcm_t *handle, snd_pcm_uframes_t buf_size,
			       snd_pcm_sframes_t *delay)
{
	int rc;

	rc = snd_pcm_delay(handle, delay);
	if (rc < 0)
		return rc;
	if (*delay > (snd_pcm_sframes_t)buf_size)
		*delay = buf_size;
	if (*delay < 0)
		*delay = 0;
	return 0;
}

int cras_alsa_attempt_resume(snd_pcm_t *handle)
{
	int rc;

	syslog(LOG_INFO, "System suspended.");
	while ((rc = snd_pcm_resume(handle)) == -EAGAIN)
		usleep(ALSA_SUSPENDED_SLEEP_TIME_US);
	if (rc < 0) {
		syslog(LOG_INFO, "System suspended, failed to resume %s.",
		       snd_strerror(rc));
		rc = snd_pcm_prepare(handle);
		if (rc < 0)
			syslog(LOG_INFO, "Suspended, failed to prepare: %s.",
			       snd_strerror(rc));
	}
	return rc;
}

int cras_alsa_mmap_get_whole_buffer(snd_pcm_t *handle, uint8_t **dst,
				    unsigned int *underruns)
{
	snd_pcm_uframes_t offset;
	/* The purpose of calling cras_alsa_mmap_begin is to get the base
	 * address of the buffer. The requested and retrieved frames are not
	 * meaningful here.
	 * However, we need to set a non-zero requested frames to get a
	 * non-zero retrieved frames. This is to avoid the error checking in
	 * snd_pcm_mmap_begin, where it judges retrieved frames being 0 as a
	 * failure.
	 */
	snd_pcm_uframes_t frames = 1;

	return cras_alsa_mmap_begin(
			handle, 0, dst, &offset, &frames, underruns);
}

int cras_alsa_mmap_begin(snd_pcm_t *handle, unsigned int format_bytes,
			 uint8_t **dst, snd_pcm_uframes_t *offset,
			 snd_pcm_uframes_t *frames, unsigned int *underruns)
{
	int rc;
	unsigned int attempts = 0;
	const snd_pcm_channel_area_t *my_areas;

	while (attempts++ < MAX_MMAP_BEGIN_ATTEMPTS) {
		rc = snd_pcm_mmap_begin(handle, &my_areas, offset, frames);
		if (rc == -ESTRPIPE) {
			/* First handle suspend/resume. */
			rc = cras_alsa_attempt_resume(handle);
			if (rc < 0)
				return rc;
			continue; /* Recovered from suspend, try again. */
		} else if (rc < 0) {
			*underruns = *underruns + 1;
			/* If we can recover, continue and try again. */
			if (snd_pcm_recover(handle, rc, 0) == 0)
				continue;
			syslog(LOG_INFO, "recover failed begin: %s\n",
			       snd_strerror(rc));
			return rc;
		}
		/* Available frames could be zero right after input pcm handle
		 * resumed. As for output pcm handle, some error has occurred
		 * when mmap_begin return zero frames, return -EIO for that
		 * case.
		 */
		if (snd_pcm_stream(handle) == SND_PCM_STREAM_PLAYBACK &&
				*frames == 0) {
			syslog(LOG_INFO, "mmap_begin set frames to 0.");
			return -EIO;
		}
		*dst = (uint8_t *)my_areas[0].addr + (*offset) * format_bytes;
		return 0;
	}
	return -EIO;
}

int cras_alsa_mmap_commit(snd_pcm_t *handle, snd_pcm_uframes_t offset,
			  snd_pcm_uframes_t frames, unsigned int *underruns)
{
	int rc;
	snd_pcm_sframes_t res;

	res = snd_pcm_mmap_commit(handle, offset, frames);
	if (res != (snd_pcm_sframes_t)frames) {
		res = res >= 0 ? (int)-EPIPE : res;
		if (res == -ESTRPIPE) {
			/* First handle suspend/resume. */
			rc = cras_alsa_attempt_resume(handle);
			if (rc < 0)
				return rc;
		} else {
			*underruns = *underruns + 1;
			/* If we can recover, continue and try again. */
			rc = snd_pcm_recover(handle, res, 0);
			if (rc < 0) {
				syslog(LOG_ERR,
				       "mmap_commit: pcm_recover failed: %s\n",
				       snd_strerror(rc));
				return rc;
			}
		}
	}
	return 0;
}
