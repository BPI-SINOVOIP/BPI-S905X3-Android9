/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <fcntl.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <syslog.h>

#include "cras_audio_area.h"
#include "cras_config.h"
#include "cras_messages.h"
#include "cras_rclient.h"
#include "cras_rstream.h"
#include "cras_shm.h"
#include "cras_types.h"
#include "buffer_share.h"
#include "cras_system_state.h"

/* Configure the shm area for the stream. */
static int setup_shm(struct cras_rstream *stream,
		     struct cras_audio_shm *shm,
		     struct rstream_shm_info *shm_info)
{
	size_t used_size, samples_size, frame_bytes;
	const struct cras_audio_format *fmt = &stream->format;

	if (shm->area != NULL) /* already setup */
		return -EEXIST;

	frame_bytes = snd_pcm_format_physical_width(fmt->format) / 8 *
			fmt->num_channels;
	used_size = stream->buffer_frames * frame_bytes;
	samples_size = used_size * CRAS_NUM_SHM_BUFFERS;
	shm_info->length = sizeof(struct cras_audio_shm_area) + samples_size;

	snprintf(shm_info->shm_name, sizeof(shm_info->shm_name),
		 "/cras-%d-stream-%08x", getpid(), stream->stream_id);

	shm_info->shm_fd = cras_shm_open_rw(shm_info->shm_name, shm_info->length);
	if (shm_info->shm_fd < 0)
		return shm_info->shm_fd;

	/* mmap shm. */
	shm->area = mmap(NULL, shm_info->length,
			 PROT_READ | PROT_WRITE, MAP_SHARED,
			 shm_info->shm_fd, 0);
	if (shm->area == (struct cras_audio_shm_area *)-1) {
		close(shm_info->shm_fd);
		return errno;
	}

	cras_shm_set_volume_scaler(shm, 1.0);
	/* Set up config and copy to shared area. */
	cras_shm_set_frame_bytes(shm, frame_bytes);
	shm->config.frame_bytes = frame_bytes;
	cras_shm_set_used_size(shm, used_size);
	memcpy(&shm->area->config, &shm->config, sizeof(shm->config));
	return 0;
}

/* Setup the shared memory area used for audio samples. */
static inline int setup_shm_area(struct cras_rstream *stream)
{
	int rc;

	rc = setup_shm(stream, &stream->shm,
			&stream->shm_info);
	if (rc)
		return rc;
	stream->audio_area =
		cras_audio_area_create(stream->format.num_channels);
	cras_audio_area_config_channels(stream->audio_area, &stream->format);

	return 0;
}

static inline int buffer_meets_size_limit(size_t buffer_size, size_t rate)
{
	return buffer_size > (CRAS_MIN_BUFFER_TIME_IN_US * rate) / 1000000;
}

/* Verifies that the given stream parameters are valid. */
static int verify_rstream_parameters(enum CRAS_STREAM_DIRECTION direction,
				     const struct cras_audio_format *format,
				     enum CRAS_STREAM_TYPE stream_type,
				     size_t buffer_frames,
				     size_t cb_threshold,
				     struct cras_rclient *client,
				     struct cras_rstream **stream_out)
{
	if (!buffer_meets_size_limit(buffer_frames, format->frame_rate)) {
		syslog(LOG_ERR, "rstream: invalid buffer_frames %zu\n",
		       buffer_frames);
		return -EINVAL;
	}
	if (stream_out == NULL) {
		syslog(LOG_ERR, "rstream: stream_out can't be NULL\n");
		return -EINVAL;
	}
	if (format == NULL) {
		syslog(LOG_ERR, "rstream: format can't be NULL\n");
		return -EINVAL;
	}
	if ((format->format != SND_PCM_FORMAT_S16_LE) &&
	    (format->format != SND_PCM_FORMAT_S32_LE) &&
	    (format->format != SND_PCM_FORMAT_U8) &&
	    (format->format != SND_PCM_FORMAT_S24_LE)) {
		syslog(LOG_ERR, "rstream: format %d not supported\n",
		       format->format);
		return -EINVAL;
	}
	if (direction != CRAS_STREAM_OUTPUT && direction != CRAS_STREAM_INPUT) {
		syslog(LOG_ERR, "rstream: Invalid direction.\n");
		return -EINVAL;
	}
	if (stream_type < CRAS_STREAM_TYPE_DEFAULT ||
	    stream_type >= CRAS_STREAM_NUM_TYPES) {
		syslog(LOG_ERR, "rstream: Invalid stream type.\n");
		return -EINVAL;
	}
	if (!buffer_meets_size_limit(cb_threshold, format->frame_rate)) {
		syslog(LOG_ERR, "rstream: cb_threshold too low\n");
		return -EINVAL;
	}
	return 0;
}

/* Exported functions */

int cras_rstream_create(struct cras_rstream_config *config,
			struct cras_rstream **stream_out)
{
	struct cras_rstream *stream;
	int rc;

	rc = verify_rstream_parameters(config->direction, config->format,
				       config->stream_type,
				       config->buffer_frames,
				       config->cb_threshold, config->client,
				       stream_out);
	if (rc < 0)
		return rc;

	stream = calloc(1, sizeof(*stream));
	if (stream == NULL)
		return -ENOMEM;

	stream->stream_id = config->stream_id;
	stream->stream_type = config->stream_type;
	stream->direction = config->direction;
	stream->flags = config->flags;
	stream->format = *config->format;
	stream->buffer_frames = config->buffer_frames;
	stream->cb_threshold = config->cb_threshold;
	stream->client = config->client;
	stream->shm.area = NULL;
	stream->master_dev.dev_id = NO_DEVICE;
	stream->master_dev.dev_ptr = NULL;
	stream->is_pinned = (config->dev_idx != NO_DEVICE);
	stream->pinned_dev_idx = config->dev_idx;
	stream->fd = config->audio_fd;

	rc = setup_shm_area(stream);
	if (rc < 0) {
		syslog(LOG_ERR, "failed to setup shm %d\n", rc);
		free(stream);
		return rc;
	}

	stream->buf_state = buffer_share_create(stream->buffer_frames);

	syslog(LOG_DEBUG, "stream %x frames %zu, cb_thresh %zu",
	       config->stream_id, config->buffer_frames, config->cb_threshold);
	*stream_out = stream;

	cras_system_state_stream_added(stream->direction);

	return 0;
}

void cras_rstream_destroy(struct cras_rstream *stream)
{
	cras_system_state_stream_removed(stream->direction);
	close(stream->fd);
	if (stream->shm.area != NULL) {
		munmap(stream->shm.area, stream->shm_info.length);
		cras_shm_close_unlink(stream->shm_info.shm_name,
				      stream->shm_info.shm_fd);
		cras_audio_area_destroy(stream->audio_area);
	}
	buffer_share_destroy(stream->buf_state);
	free(stream);
}

void cras_rstream_record_fetch_interval(struct cras_rstream *rstream,
					const struct timespec *now)
{
	struct timespec ts;

	if (rstream->last_fetch_ts.tv_sec || rstream->last_fetch_ts.tv_nsec) {
		subtract_timespecs(now, &rstream->last_fetch_ts, &ts);
		if (timespec_after(&ts, &rstream->longest_fetch_interval))
			rstream->longest_fetch_interval = ts;
	}
}

static void init_audio_message(struct audio_message *msg,
			       enum CRAS_AUDIO_MESSAGE_ID id,
			       uint32_t frames)
{
	memset(msg, 0, sizeof(*msg));
	msg->id = id;
	msg->frames = frames;
}

int cras_rstream_request_audio(struct cras_rstream *stream,
			       const struct timespec *now)
{
	struct audio_message msg;
	int rc;

	/* Only request samples from output streams. */
	if (stream->direction != CRAS_STREAM_OUTPUT)
		return 0;

	stream->last_fetch_ts = *now;

	init_audio_message(&msg, AUDIO_MESSAGE_REQUEST_DATA,
			   stream->cb_threshold);
	rc = write(stream->fd, &msg, sizeof(msg));
	if (rc < 0)
		return -errno;
	return rc;
}

int cras_rstream_audio_ready(struct cras_rstream *stream, size_t count)
{
	struct audio_message msg;
	int rc;

	cras_shm_buffer_write_complete(&stream->shm);

	init_audio_message(&msg, AUDIO_MESSAGE_DATA_READY, count);
	rc = write(stream->fd, &msg, sizeof(msg));
	if (rc < 0)
		return -errno;
	return rc;
}

int cras_rstream_get_audio_request_reply(const struct cras_rstream *stream)
{
	struct audio_message msg;
	int rc;

	rc = read(stream->fd, &msg, sizeof(msg));
	if (rc < 0)
		return -errno;
	if (msg.error < 0)
		return msg.error;
	return 0;
}

void cras_rstream_dev_attach(struct cras_rstream *rstream,
			     unsigned int dev_id,
			     void *dev_ptr)
{
	if (buffer_share_add_id(rstream->buf_state, dev_id, dev_ptr) == 0)
		rstream->num_attached_devs++;

	/* TODO(hychao): Handle master device assignment for complicated
	 * routing case.
	 */
	if (rstream->master_dev.dev_id == NO_DEVICE) {
		rstream->master_dev.dev_id = dev_id;
		rstream->master_dev.dev_ptr = dev_ptr;
	}
}

void cras_rstream_dev_detach(struct cras_rstream *rstream, unsigned int dev_id)
{
	if (buffer_share_rm_id(rstream->buf_state, dev_id) == 0)
		rstream->num_attached_devs--;

	if (rstream->master_dev.dev_id == dev_id) {
		int i;
		struct id_offset *o;

		/* Choose the first device id as master. */
		rstream->master_dev.dev_id = NO_DEVICE;
		rstream->master_dev.dev_ptr = NULL;
		for (i = 0; i < rstream->buf_state->id_sz; i++) {
			o = &rstream->buf_state->wr_idx[i];
			if (o->used) {
				rstream->master_dev.dev_id = o->id;
				rstream->master_dev.dev_ptr = o->data;
				break;
			}
		}
	}
}

void cras_rstream_dev_offset_update(struct cras_rstream *rstream,
				    unsigned int frames,
				    unsigned int dev_id)
{
	buffer_share_offset_update(rstream->buf_state, dev_id, frames);
}

void cras_rstream_update_input_write_pointer(struct cras_rstream *rstream)
{
	struct cras_audio_shm *shm = cras_rstream_input_shm(rstream);
	unsigned int nwritten = buffer_share_get_new_write_point(
					rstream->buf_state);

	cras_shm_buffer_written(shm, nwritten);
}

void cras_rstream_update_output_read_pointer(struct cras_rstream *rstream)
{
	struct cras_audio_shm *shm = cras_rstream_input_shm(rstream);
	unsigned int nwritten = buffer_share_get_new_write_point(
					rstream->buf_state);

	cras_shm_buffer_read(shm, nwritten);
}

unsigned int cras_rstream_dev_offset(const struct cras_rstream *rstream,
				     unsigned int dev_id)
{
	return buffer_share_id_offset(rstream->buf_state, dev_id);
}

void cras_rstream_update_queued_frames(struct cras_rstream *rstream)
{
	const struct cras_audio_shm *shm = cras_rstream_output_shm(rstream);
	rstream->queued_frames = MIN(cras_shm_get_frames(shm),
				     rstream->buffer_frames);
}

unsigned int cras_rstream_playable_frames(struct cras_rstream *rstream,
					  unsigned int dev_id)
{
	return rstream->queued_frames -
			cras_rstream_dev_offset(rstream, dev_id);
}

float cras_rstream_get_volume_scaler(struct cras_rstream *rstream)
{
	const struct cras_audio_shm *shm = cras_rstream_output_shm(rstream);

	return cras_shm_get_volume_scaler(shm);
}

uint8_t *cras_rstream_get_readable_frames(struct cras_rstream *rstream,
					  unsigned int offset,
					  size_t *frames)
{
	return cras_shm_get_readable_frames(&rstream->shm, offset, frames);
}

int cras_rstream_get_mute(const struct cras_rstream *rstream)
{
	return cras_shm_get_mute(&rstream->shm);
}
