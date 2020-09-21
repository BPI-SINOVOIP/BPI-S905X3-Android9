/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <alsa/asoundlib.h>
#include <alsa/pcm_external.h>
#include <cras_client.h>
#include <sys/socket.h>

/* Holds configuration for the alsa plugin.
 *  io - ALSA ioplug object.
 *  fd - Wakes users with polled io.
 *  stream_playing - Indicates if the stream is playing/capturing.
 *  hw_ptr - Current read or write position.
 *  channels - Number of channels.
 *  stream_id - CRAS ID of the playing/capturing stream.
 *  bytes_per_frame - number of bytes in an audio frame.
 *  direction - input or output.
 *  areas - ALSA areas used to read from/write to.
 *  client - CRAS client object.
 *  capture_sample_index - The sample tracked for capture latency calculation.
 *  playback_sample_index - The sample tracked for playback latency calculation.
 *  capture_sample_time - The time when capture_sample_index was captured.
 *  playback_sample_time - The time when playback_sample_index was captured.
 */
struct snd_pcm_cras {
	snd_pcm_ioplug_t io;
	int fd;
	int stream_playing;
	unsigned int hw_ptr;
	unsigned int channels;
	cras_stream_id_t stream_id;
	size_t bytes_per_frame;
	enum CRAS_STREAM_DIRECTION direction;
	snd_pcm_channel_area_t *areas;
	struct cras_client *client;
	int capture_sample_index;
	int playback_sample_index;
	struct timespec capture_sample_time;
	struct timespec playback_sample_time;
};

/* Frees all resources allocated during use. */
static void snd_pcm_cras_free(struct snd_pcm_cras *pcm_cras)
{
	if (pcm_cras == NULL)
		return;
	assert(!pcm_cras->stream_playing);
	if (pcm_cras->fd >= 0)
		close(pcm_cras->fd);
	if (pcm_cras->io.poll_fd >= 0)
		close(pcm_cras->io.poll_fd);
	cras_client_destroy(pcm_cras->client);
	free(pcm_cras->areas);
	free(pcm_cras);
}

/* Stops a playing or capturing CRAS plugin. */
static int snd_pcm_cras_stop(snd_pcm_ioplug_t *io)
{
	struct snd_pcm_cras *pcm_cras = io->private_data;

	if (pcm_cras->stream_playing) {
		cras_client_rm_stream(pcm_cras->client, pcm_cras->stream_id);
		cras_client_stop(pcm_cras->client);
		pcm_cras->stream_playing = 0;
	}
	return 0;
}

/* Close a CRAS plugin opened with snd_pcm_cras_open. */
static int snd_pcm_cras_close(snd_pcm_ioplug_t *io)
{
	struct snd_pcm_cras *pcm_cras = io->private_data;

	if (pcm_cras->stream_playing)
		snd_pcm_cras_stop(io);
	snd_pcm_cras_free(pcm_cras);
	return 0;
}

/* Poll callback used to wait for data ready (playback) or space available
 * (capture). */
static int snd_pcm_cras_poll_revents(snd_pcm_ioplug_t *io,
				     struct pollfd *pfds,
				     unsigned int nfds,
				     unsigned short *revents)
{
	static char buf[1];
	int rc;

	if (pfds == NULL || nfds != 1 || revents == NULL)
		return -EINVAL;
	rc = read(pfds[0].fd, buf, 1);
	if (rc < 0 && errno != EWOULDBLOCK && errno != EAGAIN) {
		fprintf(stderr, "%s read failed %d\n", __func__, errno);
		return errno;
	}
	*revents = pfds[0].revents & ~(POLLIN | POLLOUT);
	if (pfds[0].revents & POLLIN)
		*revents |= (io->stream == SND_PCM_STREAM_PLAYBACK) ? POLLOUT
								    : POLLIN;
	return 0;
}

/* Callback to return the location of the write (playback) or read (capture)
 * pointer. */
static snd_pcm_sframes_t snd_pcm_cras_pointer(snd_pcm_ioplug_t *io)
{
	struct snd_pcm_cras *pcm_cras = io->private_data;
	return pcm_cras->hw_ptr;
}

/* Main callback for processing audio.  This is called by CRAS when more samples
 * are needed (playback) or ready (capture).  Copies bytes between ALSA and CRAS
 * buffers. */
static int pcm_cras_process_cb(struct cras_client *client,
			       cras_stream_id_t stream_id,
			       uint8_t *capture_samples,
			       uint8_t *playback_samples,
			       unsigned int nframes,
			       const struct timespec *capture_ts,
			       const struct timespec *playback_ts,
			       void *arg)
{
	snd_pcm_ioplug_t *io;
	struct snd_pcm_cras *pcm_cras;
	const snd_pcm_channel_area_t *areas;
	snd_pcm_uframes_t copied_frames;
	char dummy_byte;
	size_t chan, frame_bytes, sample_bytes;
	int rc;
	uint8_t *samples;
	const struct timespec *sample_time;

	samples = capture_samples ? : playback_samples;
	sample_time = capture_ts ? : playback_ts;

	io = (snd_pcm_ioplug_t *)arg;
	pcm_cras = (struct snd_pcm_cras *)io->private_data;
	frame_bytes = pcm_cras->bytes_per_frame;
	sample_bytes = snd_pcm_format_physical_width(io->format) / 8;

	if (io->stream == SND_PCM_STREAM_PLAYBACK) {
		if (io->state != SND_PCM_STATE_RUNNING) {
			memset(samples, 0, nframes * frame_bytes);
			return nframes;
		}
		/* Only take one period of data at a time. */
		if (nframes > io->period_size)
			nframes = io->period_size;

		/* Keep track of the first transmitted sample index and the time
		 * it will be played. */
		pcm_cras->playback_sample_index = io->hw_ptr;
		pcm_cras->playback_sample_time = *sample_time;
	} else {
		/* Keep track of the first read sample index and the time it
		 * was captured. */
		pcm_cras->capture_sample_index = io->hw_ptr;
		pcm_cras->capture_sample_time = *sample_time;
	}

	/* CRAS always takes interleaved samples. */
	for (chan = 0; chan < io->channels; chan++) {
		pcm_cras->areas[chan].addr = samples + chan * sample_bytes;
		pcm_cras->areas[chan].first = 0;
		pcm_cras->areas[chan].step =
			snd_pcm_format_physical_width(io->format) *
			io->channels;
	}

	areas = snd_pcm_ioplug_mmap_areas(io);

	copied_frames = 0;
	while (copied_frames < nframes) {
		snd_pcm_uframes_t frames = nframes - copied_frames;
		snd_pcm_uframes_t remain = io->buffer_size - pcm_cras->hw_ptr;

		if (frames > remain)
			frames = remain;

		for (chan = 0; chan < io->channels; chan++)
			if (io->stream == SND_PCM_STREAM_PLAYBACK)
				snd_pcm_area_copy(&pcm_cras->areas[chan],
						  copied_frames,
						  &areas[chan],
						  pcm_cras->hw_ptr,
						  frames,
						  io->format);
			else
				snd_pcm_area_copy(&areas[chan],
						  pcm_cras->hw_ptr,
						  &pcm_cras->areas[chan],
						  copied_frames,
						  frames,
						  io->format);

		pcm_cras->hw_ptr += frames;
		pcm_cras->hw_ptr %= io->buffer_size;
		copied_frames += frames;
	}

	rc = write(pcm_cras->fd, &dummy_byte, 1); /* Wake up polling clients. */
	if (rc < 0 && errno != EWOULDBLOCK && errno != EAGAIN)
		fprintf(stderr, "%s write failed %d\n", __func__, errno);

	return nframes;
}

/* Callback from CRAS for stream errors. */
static int pcm_cras_error_cb(struct cras_client *client,
			     cras_stream_id_t stream_id,
			     int err,
			     void *arg)
{
	fprintf(stderr, "Stream error %d\n", err);
	return 0;
}

/* ALSA calls this automatically when the stream enters the
 * SND_PCM_STATE_PREPARED state. */
static int snd_pcm_cras_prepare(snd_pcm_ioplug_t *io)
{
	struct snd_pcm_cras *pcm_cras = io->private_data;

	return cras_client_connect(pcm_cras->client);
}

/* Get the pcm boundary value. */
static int get_boundary(snd_pcm_t *pcm, snd_pcm_uframes_t *boundary)
{
	snd_pcm_sw_params_t *sw_params;
	int rc;

	snd_pcm_sw_params_alloca(&sw_params);

	rc = snd_pcm_sw_params_current(pcm, sw_params);
	if (rc < 0) {
		fprintf(stderr, "sw_params_current: %s\n", snd_strerror(rc));
		return rc;
	}

	rc = snd_pcm_sw_params_get_boundary(sw_params, boundary);
	if (rc < 0) {
		fprintf(stderr, "get_boundary: %s\n", snd_strerror(rc));
		return rc;
	}

	return 0;
}

/* Called when an ALSA stream is started. */
static int snd_pcm_cras_start(snd_pcm_ioplug_t *io)
{
	struct snd_pcm_cras *pcm_cras = io->private_data;
	struct cras_stream_params *params;
	struct cras_audio_format *audio_format;
	int rc;

	audio_format = cras_audio_format_create(io->format, io->rate,
						io->channels);
	if (audio_format == NULL)
		return -ENOMEM;

	params = cras_client_unified_params_create(
			pcm_cras->direction,
			io->period_size,
			0,
			0,
			io,
			pcm_cras_process_cb,
			pcm_cras_error_cb,
			audio_format);
	if (params == NULL) {
		rc = -ENOMEM;
		goto error_out;
	}

	rc = cras_client_run_thread(pcm_cras->client);
	if (rc < 0)
		goto error_out;

	pcm_cras->bytes_per_frame =
		cras_client_format_bytes_per_frame(audio_format);

	rc = cras_client_add_stream(pcm_cras->client,
				    &pcm_cras->stream_id,
				    params);
	if (rc < 0) {
		fprintf(stderr, "CRAS add failed\n");
		goto error_out;
	}
	pcm_cras->stream_playing = 1;

error_out:
	cras_audio_format_destroy(audio_format);
	cras_client_stream_params_destroy(params);
	return rc;
}

static int snd_pcm_cras_delay(snd_pcm_ioplug_t *io, snd_pcm_sframes_t *delayp)
{
	snd_pcm_uframes_t limit;
	int rc;
	struct snd_pcm_cras *pcm_cras;
	struct timespec latency;

	pcm_cras = (struct snd_pcm_cras *)io->private_data;

	rc = get_boundary(io->pcm, &limit);
	if ((rc < 0) || (limit == 0)) {
		*delayp = 0;
		return -EINVAL;
	}

	/* To get the delay, first calculate the latency between now and the
	 * playback/capture sample time for the old hw_ptr we tracked, note that
	 * for playback path this latency could be negative. Then add it up with
	 * the difference between appl_ptr and the old hw_ptr.
	 */
	if (io->stream == SND_PCM_STREAM_PLAYBACK) {
		/* Do not compute latency if playback_sample_time is not set */
		if (pcm_cras->playback_sample_time.tv_sec == 0 &&
		    pcm_cras->playback_sample_time.tv_nsec == 0) {
			latency.tv_sec = 0;
			latency.tv_nsec = 0;
		} else {
			cras_client_calc_playback_latency(
					&pcm_cras->playback_sample_time,
					&latency);
		}

		*delayp = limit +
			  io->appl_ptr -
			  pcm_cras->playback_sample_index +
			  latency.tv_sec * io->rate +
			  latency.tv_nsec / (1000000000L / (long)io->rate);
	} else {
		/* Do not compute latency if capture_sample_time is not set */
		if (pcm_cras->capture_sample_time.tv_sec == 0 &&
		    pcm_cras->capture_sample_time.tv_nsec == 0) {
			latency.tv_sec = 0;
			latency.tv_nsec = 0;
		} else {
			cras_client_calc_capture_latency(
					&pcm_cras->capture_sample_time,
					&latency);
		}

		*delayp = limit +
			  pcm_cras->capture_sample_index -
			  io->appl_ptr +
			  latency.tv_sec * io->rate +
			  latency.tv_nsec / (1000000000L / (long)io->rate);
	}

	/* Both appl and hw pointers wrap at the pcm boundary. */
	*delayp %= limit;

	return 0;
}

static snd_pcm_ioplug_callback_t cras_pcm_callback = {
	.close = snd_pcm_cras_close,
	.delay = snd_pcm_cras_delay,
	.start = snd_pcm_cras_start,
	.stop = snd_pcm_cras_stop,
	.pointer = snd_pcm_cras_pointer,
	.prepare = snd_pcm_cras_prepare,
	.poll_revents = snd_pcm_cras_poll_revents,
};

/* Set constraints for hw_params.  This lists the handled formats, sample rates,
 * access patters, and buffer/period sizes.  These are enforce in
 * snd_pcm_set_params(). */
static int set_hw_constraints(struct snd_pcm_cras *pcm_cras)
{
	static const unsigned int access_list[] = {
		SND_PCM_ACCESS_MMAP_INTERLEAVED,
		SND_PCM_ACCESS_MMAP_NONINTERLEAVED,
		SND_PCM_ACCESS_RW_INTERLEAVED,
		SND_PCM_ACCESS_RW_NONINTERLEAVED
	};
	static const unsigned int format_list[] = {
		SND_PCM_FORMAT_U8,
		SND_PCM_FORMAT_S16_LE,
		SND_PCM_FORMAT_S24_LE,
		SND_PCM_FORMAT_S32_LE,
		SND_PCM_FORMAT_S24_3LE,
	};
	int rc;

	rc = snd_pcm_ioplug_set_param_list(&pcm_cras->io,
					   SND_PCM_IOPLUG_HW_ACCESS,
					   ARRAY_SIZE(access_list),
					   access_list);
	if (rc < 0)
		return rc;
	rc = snd_pcm_ioplug_set_param_list(&pcm_cras->io,
					   SND_PCM_IOPLUG_HW_FORMAT,
					   ARRAY_SIZE(format_list),
					   format_list);
	if (rc < 0)
		return rc;
	rc = snd_pcm_ioplug_set_param_minmax(&pcm_cras->io,
					     SND_PCM_IOPLUG_HW_CHANNELS,
					     1,
					     pcm_cras->channels);
	if (rc < 0)
		return rc;
	rc = snd_pcm_ioplug_set_param_minmax(&pcm_cras->io,
					    SND_PCM_IOPLUG_HW_RATE,
					    8000,
					    48000);
	if (rc < 0)
		return rc;
	rc = snd_pcm_ioplug_set_param_minmax(&pcm_cras->io,
					     SND_PCM_IOPLUG_HW_BUFFER_BYTES,
					     64,
					     2 * 1024 * 1024);
	if (rc < 0)
		return rc;
	rc = snd_pcm_ioplug_set_param_minmax(&pcm_cras->io,
					     SND_PCM_IOPLUG_HW_PERIOD_BYTES,
					     64,
					     2 * 1024 * 1024);
	if (rc < 0)
		return rc;
	rc = snd_pcm_ioplug_set_param_minmax(&pcm_cras->io,
					     SND_PCM_IOPLUG_HW_PERIODS,
					     1,
					     2048);
	return rc;
}

/* Called by snd_pcm_open().  Creates a CRAS client and an ioplug plugin. */
static int snd_pcm_cras_open(snd_pcm_t **pcmp, const char *name,
			     snd_pcm_stream_t stream, int mode)
{
	struct snd_pcm_cras *pcm_cras;
	int rc;
	int fd[2];

	assert(pcmp);
	pcm_cras = calloc(1, sizeof(*pcm_cras));
	if (!pcm_cras)
		return -ENOMEM;

	pcm_cras->fd = -1;
	pcm_cras->io.poll_fd = -1;
	pcm_cras->channels = 2;
	pcm_cras->direction = (stream == SND_PCM_STREAM_PLAYBACK)
				? CRAS_STREAM_OUTPUT : CRAS_STREAM_INPUT;

	rc = cras_client_create(&pcm_cras->client);
	if (rc != 0 || pcm_cras->client == NULL) {
		fprintf(stderr, "Couldn't create CRAS client\n");
		free(pcm_cras);
		return rc;
	}

	pcm_cras->areas = calloc(pcm_cras->channels,
				 sizeof(snd_pcm_channel_area_t));
	if (pcm_cras->areas == NULL) {
		snd_pcm_cras_free(pcm_cras);
		return -ENOMEM;
	}

	socketpair(AF_LOCAL, SOCK_STREAM, 0, fd);

	cras_make_fd_nonblocking(fd[0]);
	cras_make_fd_nonblocking(fd[1]);

	pcm_cras->fd = fd[0];

	pcm_cras->io.version = SND_PCM_IOPLUG_VERSION;
	pcm_cras->io.name = "ALSA to CRAS Plugin";
	pcm_cras->io.callback = &cras_pcm_callback;
	pcm_cras->io.private_data = pcm_cras;
	pcm_cras->io.poll_fd = fd[1];
	pcm_cras->io.poll_events = POLLIN;
	pcm_cras->io.mmap_rw = 1;

	rc = snd_pcm_ioplug_create(&pcm_cras->io, name, stream, mode);
	if (rc < 0) {
		snd_pcm_cras_free(pcm_cras);
		return rc;
	}

	rc = set_hw_constraints(pcm_cras);
	if (rc < 0) {
		snd_pcm_ioplug_delete(&pcm_cras->io);
		return rc;
	}

	*pcmp = pcm_cras->io.pcm;

	return 0;
}


SND_PCM_PLUGIN_DEFINE_FUNC(cras)
{
	return snd_pcm_cras_open(pcmp, name, stream, mode);
}

SND_PCM_PLUGIN_SYMBOL(cras);
