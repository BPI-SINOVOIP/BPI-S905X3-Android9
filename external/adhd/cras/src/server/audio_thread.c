/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* for ppoll */
#endif

#include <pthread.h>
#include <poll.h>
#include <sys/param.h>
#include <syslog.h>

#include "cras_audio_area.h"
#include "audio_thread_log.h"
#include "cras_config.h"
#include "cras_fmt_conv.h"
#include "cras_iodev.h"
#include "cras_rstream.h"
#include "cras_server_metrics.h"
#include "cras_system_state.h"
#include "cras_types.h"
#include "cras_util.h"
#include "dev_stream.h"
#include "audio_thread.h"
#include "utlist.h"

#define MIN_PROCESS_TIME_US 500 /* 0.5ms - min amount of time to mix/src. */
#define SLEEP_FUZZ_FRAMES 10 /* # to consider "close enough" to sleep frames. */
#define MIN_READ_WAIT_US 2000 /* 2ms */
static const struct timespec playback_wake_fuzz_ts = {
	0, 500 * 1000 /* 500 usec. */
};

/* Messages that can be sent from the main context to the audio thread. */
enum AUDIO_THREAD_COMMAND {
	AUDIO_THREAD_ADD_OPEN_DEV,
	AUDIO_THREAD_RM_OPEN_DEV,
	AUDIO_THREAD_ADD_STREAM,
	AUDIO_THREAD_DISCONNECT_STREAM,
	AUDIO_THREAD_STOP,
	AUDIO_THREAD_DUMP_THREAD_INFO,
	AUDIO_THREAD_DRAIN_STREAM,
	AUDIO_THREAD_CONFIG_GLOBAL_REMIX,
	AUDIO_THREAD_DEV_START_RAMP,
	AUDIO_THREAD_REMOVE_CALLBACK,
};

struct audio_thread_msg {
	size_t length;
	enum AUDIO_THREAD_COMMAND id;
};

struct audio_thread_config_global_remix {
	struct audio_thread_msg header;
	struct cras_fmt_conv *fmt_conv;
};

struct audio_thread_open_device_msg {
	struct audio_thread_msg header;
	struct cras_iodev *dev;
};

struct audio_thread_rm_callback_msg {
	struct audio_thread_msg header;
	int fd;
};

struct audio_thread_add_rm_stream_msg {
	struct audio_thread_msg header;
	struct cras_rstream *stream;
	struct cras_iodev **devs;
	unsigned int num_devs;
};

struct audio_thread_dump_debug_info_msg {
	struct audio_thread_msg header;
	struct audio_debug_info *info;
};

struct audio_thread_dev_start_ramp_msg {
	struct audio_thread_msg header;
	struct cras_iodev *dev;
	enum CRAS_IODEV_RAMP_REQUEST request;
};

/* Audio thread logging. */
struct audio_thread_event_log *atlog;
/* Global fmt converter used to remix output channels. */
static struct cras_fmt_conv *remix_converter = NULL;

static struct iodev_callback_list *iodev_callbacks;
static struct timespec longest_wake;

struct iodev_callback_list {
	int fd;
	int is_write;
	int enabled;
	thread_callback cb;
	void *cb_data;
	struct pollfd *pollfd;
	struct iodev_callback_list *prev, *next;
};

static void _audio_thread_add_callback(int fd, thread_callback cb,
				       void *data, int is_write)
{
	struct iodev_callback_list *iodev_cb;

	/* Don't add iodev_cb twice */
	DL_FOREACH(iodev_callbacks, iodev_cb)
		if (iodev_cb->fd == fd && iodev_cb->cb_data == data)
			return;

	iodev_cb = (struct iodev_callback_list *)calloc(1, sizeof(*iodev_cb));
	iodev_cb->fd = fd;
	iodev_cb->cb = cb;
	iodev_cb->cb_data = data;
	iodev_cb->enabled = 1;
	iodev_cb->is_write = is_write;

	DL_APPEND(iodev_callbacks, iodev_cb);
}

void audio_thread_add_callback(int fd, thread_callback cb,
				void *data)
{
	_audio_thread_add_callback(fd, cb, data, 0);
}

void audio_thread_add_write_callback(int fd, thread_callback cb,
				     void *data)
{
	_audio_thread_add_callback(fd, cb, data, 1);
}

void audio_thread_rm_callback(int fd)
{
	struct iodev_callback_list *iodev_cb;

	DL_FOREACH(iodev_callbacks, iodev_cb) {
		if (iodev_cb->fd == fd) {
			DL_DELETE(iodev_callbacks, iodev_cb);
			free(iodev_cb);
			return;
		}
	}
}

void audio_thread_enable_callback(int fd, int enabled)
{
	struct iodev_callback_list *iodev_cb;

	DL_FOREACH(iodev_callbacks, iodev_cb) {
		if (iodev_cb->fd == fd) {
			iodev_cb->enabled = !!enabled;
			return;
		}
	}
}

/* Sends a response (error code) from the audio thread to the main thread.
 * Indicates that the last message sent to the audio thread has been handled
 * with an error code of rc.
 * Args:
 *    thread - thread responding to command.
 *    rc - Result code to send back to the main thread.
 * Returns:
 *    The number of bytes written to the main thread.
 */
static int audio_thread_send_response(struct audio_thread *thread, int rc)
{
	return write(thread->to_main_fds[1], &rc, sizeof(rc));
}

/* Reads a command from the main thread.  Called from the playback/capture
 * thread.  This will read the next available command from the main thread and
 * put it in buf.
 * Args:
 *    thread - thread reading the command.
 *    buf - Message is stored here on return.
 *    max_len - maximum length of message to put into buf.
 * Returns:
 *    0 on success, negative error code on failure.
 */
static int audio_thread_read_command(struct audio_thread *thread,
				     uint8_t *buf,
				     size_t max_len)
{
	int to_read, nread, rc;
	struct audio_thread_msg *msg = (struct audio_thread_msg *)buf;

	/* Get the length of the message first */
	nread = read(thread->to_thread_fds[0], buf, sizeof(msg->length));
	if (nread < 0)
		return nread;
	if (msg->length > max_len)
		return -ENOMEM;

	to_read = msg->length - nread;
	rc = read(thread->to_thread_fds[0], &buf[0] + nread, to_read);
	if (rc < 0)
		return rc;
	return 0;
}

/* Builds an initial buffer to avoid an underrun. Adds min_level of latency. */
static void fill_odevs_zeros_min_level(struct cras_iodev *odev)
{
	cras_iodev_fill_odev_zeros(odev, odev->min_buffer_level);
}

static void thread_rm_open_adev(struct audio_thread *thread,
				struct open_dev *adev);

static void delete_stream_from_dev(struct cras_iodev *dev,
				   struct cras_rstream *stream)
{
	struct dev_stream *out;

	out = cras_iodev_rm_stream(dev, stream);
	if (out)
		dev_stream_destroy(out);
}

/* Append a new stream to a specified set of iodevs. */
static int append_stream(struct audio_thread *thread,
			 struct cras_rstream *stream,
			 struct cras_iodev **iodevs,
			 unsigned int num_iodevs)
{
	struct open_dev *open_dev;
	struct cras_iodev *dev;
	struct dev_stream *out;
	struct timespec init_cb_ts;
	const struct timespec *stream_ts;
	unsigned int i;
	int rc = 0;

	for (i = 0; i < num_iodevs; i++) {
		DL_SEARCH_SCALAR(thread->open_devs[stream->direction], open_dev,
				dev, iodevs[i]);
		if (!open_dev)
			continue;

		dev = iodevs[i];
		DL_SEARCH_SCALAR(dev->streams, out, stream, stream);
		if (out)
			continue;

		/* If open device already has stream, get the first stream
		 * and use its next callback time to align with. Otherwise
		 * use the timestamp now as the initial callback time for
		 * new stream.
		 */
		if (dev->streams &&
		    (stream_ts = dev_stream_next_cb_ts(dev->streams)))
			init_cb_ts = *stream_ts;
		else
			clock_gettime(CLOCK_MONOTONIC_RAW, &init_cb_ts);

		out = dev_stream_create(stream, dev->info.idx,
					dev->ext_format, dev, &init_cb_ts);
		if (!out) {
			rc = -EINVAL;
			break;
		}

		/* When the first input stream is added, flush the input buffer
		 * so that we can read from multiple input devices of the same
		 * buffer level.
		 */
		if ((stream->direction == CRAS_STREAM_INPUT) && !dev->streams) {
			int num_flushed = dev->flush_buffer(dev);
			if (num_flushed < 0) {
				rc = num_flushed;
				break;
			}
		}

		cras_iodev_add_stream(dev, out);

		/* For multiple inputs case, if the new stream is not the first
		 * one to append, copy the 1st stream's offset to it so that
		 * future read offsets can be aligned across all input streams
		 * to avoid the deadlock scenario when multiple streams reading
		 * from multiple devices.
		 */
		if ((stream->direction == CRAS_STREAM_INPUT) &&
		    (dev->streams != out)) {
			unsigned int offset =
				cras_iodev_stream_offset(dev, dev->streams);
			if (offset > stream->cb_threshold)
				offset = stream->cb_threshold;
			cras_iodev_stream_written(dev, out, offset);

			offset = cras_rstream_dev_offset(dev->streams->stream,
							 dev->info.idx);
			if (offset > stream->cb_threshold)
				offset = stream->cb_threshold;
			cras_rstream_dev_offset_update(stream, offset,
						       dev->info.idx);
		}
	}

	if (rc) {
		DL_FOREACH(thread->open_devs[stream->direction], open_dev) {
			dev = open_dev->dev;
			DL_SEARCH_SCALAR(dev->streams, out, stream, stream);
			if (!out)
				continue;

			cras_iodev_rm_stream(dev, stream);
			dev_stream_destroy(out);
		}
	}

	return rc;
}

/* Handles messages from main thread to add a new active device. */
static int thread_add_open_dev(struct audio_thread *thread,
			       struct cras_iodev *iodev)
{
	struct open_dev *adev;

	DL_SEARCH_SCALAR(thread->open_devs[iodev->direction],
			 adev, dev, iodev);
	if (adev)
		return -EEXIST;

	adev = (struct open_dev *)calloc(1, sizeof(*adev));
	adev->dev = iodev;

	/*
	 * Start output devices by padding the output. This avoids a burst of
	 * audio callbacks when the stream starts
	 */
	if (iodev->direction == CRAS_STREAM_OUTPUT)
		fill_odevs_zeros_min_level(iodev);
	else
		adev->input_streaming = 0;

	ATLOG(atlog,
				    AUDIO_THREAD_DEV_ADDED,
				    iodev->info.idx, 0, 0);

	DL_APPEND(thread->open_devs[iodev->direction], adev);

	return 0;
}

static struct open_dev *find_adev(struct open_dev *adev_list,
				  struct cras_iodev *dev)
{
	struct open_dev *adev;
	DL_FOREACH(adev_list, adev)
		if (adev->dev == dev)
			return adev;
	return NULL;
}

static void thread_rm_open_adev(struct audio_thread *thread,
				struct open_dev *dev_to_rm)
{
	enum CRAS_STREAM_DIRECTION dir = dev_to_rm->dev->direction;
	struct open_dev *adev;
	struct dev_stream *dev_stream;

	/* Do nothing if dev_to_rm wasn't already in the active dev list. */
	adev = find_adev(thread->open_devs[dir], dev_to_rm->dev);
	if (!adev)
		return;

	DL_DELETE(thread->open_devs[dir], dev_to_rm);

	ATLOG(atlog,
				    AUDIO_THREAD_DEV_REMOVED,
				    dev_to_rm->dev->info.idx, 0, 0);

	DL_FOREACH(dev_to_rm->dev->streams, dev_stream) {
		cras_iodev_rm_stream(dev_to_rm->dev, dev_stream->stream);
		dev_stream_destroy(dev_stream);
	}

	free(dev_to_rm);
}

/* Handles messages from the main thread to remove an active device. */
static int thread_rm_open_dev(struct audio_thread *thread,
			      struct cras_iodev *iodev)
{
	struct open_dev *adev = find_adev(
			thread->open_devs[iodev->direction], iodev);
	if (!adev)
		return -EINVAL;

	thread_rm_open_adev(thread, adev);
	return 0;
}

/* Handles messages from the main thread to start ramping on a device. */
static int thread_dev_start_ramp(struct audio_thread *thread,
				 struct cras_iodev *iodev,
				 enum CRAS_IODEV_RAMP_REQUEST request)
{
	/* Do nothing if device wasn't already in the active dev list. */
	struct open_dev *adev = find_adev(
			thread->open_devs[iodev->direction], iodev);
	if (!adev)
		return -EINVAL;
	return cras_iodev_start_ramp(iodev, request);
}


/* Return non-zero if the stream is attached to any device. */
static int thread_find_stream(struct audio_thread *thread,
			      struct cras_rstream *rstream)
{
	struct open_dev *open_dev;
	struct dev_stream *s;

	DL_FOREACH(thread->open_devs[rstream->direction], open_dev) {
		DL_FOREACH(open_dev->dev->streams, s) {
			if (s->stream == rstream)
				return 1;
		}
	}
	return 0;
}

/* Remove stream from the audio thread. If this is the last stream to be
 * removed close the device.
 */
static int thread_remove_stream(struct audio_thread *thread,
				struct cras_rstream *stream,
				struct cras_iodev *dev)
{
	struct open_dev *open_dev;
	struct timespec delay;
	unsigned fetch_delay_msec;

	/* Metrics log the longest fetch delay of this stream. */
	if (timespec_after(&stream->longest_fetch_interval,
			   &stream->sleep_interval_ts)) {
		subtract_timespecs(&stream->longest_fetch_interval,
				   &stream->sleep_interval_ts,
				   &delay);
		fetch_delay_msec = delay.tv_sec * 1000 +
				   delay.tv_nsec / 1000000;
		if (fetch_delay_msec)
			cras_server_metrics_longest_fetch_delay(
					fetch_delay_msec);
	}

	ATLOG(atlog,
				    AUDIO_THREAD_STREAM_REMOVED,
				    stream->stream_id, 0, 0);

	if (dev == NULL) {
		DL_FOREACH(thread->open_devs[stream->direction], open_dev) {
			delete_stream_from_dev(open_dev->dev, stream);
		}
	} else {
		delete_stream_from_dev(dev, stream);
	}

	return 0;
}

/* Handles the disconnect_stream message from the main thread. */
static int thread_disconnect_stream(struct audio_thread* thread,
				    struct cras_rstream* stream,
				    struct cras_iodev *dev)
{
	int rc;

	if (!thread_find_stream(thread, stream))
		return 0;

	rc = thread_remove_stream(thread, stream, dev);

	return rc;
}

/* Initiates draining of a stream or returns the status of a draining stream.
 * If the stream has completed draining the thread forfeits ownership and must
 * never reference it again.  Returns the number of milliseconds it will take to
 * finish draining, a minimum of one ms if any samples remain.
 */
static int thread_drain_stream_ms_remaining(struct audio_thread *thread,
					    struct cras_rstream *rstream)
{
	int fr_in_buff;
	struct cras_audio_shm *shm;

	if (rstream->direction != CRAS_STREAM_OUTPUT)
		return 0;

	shm = cras_rstream_output_shm(rstream);
	fr_in_buff = cras_shm_get_frames(shm);

	if (fr_in_buff <= 0)
		return 0;

	cras_rstream_set_is_draining(rstream, 1);

	return 1 + cras_frames_to_ms(fr_in_buff, rstream->format.frame_rate);
}

/* Handles a request to begin draining and return the amount of time left to
 * draing a stream.
 */
static int thread_drain_stream(struct audio_thread *thread,
			       struct cras_rstream *rstream)
{
	int ms_left;

	if (!thread_find_stream(thread, rstream))
		return 0;

	ms_left = thread_drain_stream_ms_remaining(thread, rstream);
	if (ms_left == 0)
		thread_remove_stream(thread, rstream, NULL);

	return ms_left;
}

/* Handles the add_stream message from the main thread. */
static int thread_add_stream(struct audio_thread *thread,
			     struct cras_rstream *stream,
			     struct cras_iodev **iodevs,
			     unsigned int num_iodevs)
{
	int rc;

	rc = append_stream(thread, stream, iodevs, num_iodevs);
	if (rc < 0)
		return rc;

	ATLOG(atlog,
				    AUDIO_THREAD_STREAM_ADDED,
				    stream->stream_id,
				    num_iodevs ? iodevs[0]->info.idx : 0,
				    num_iodevs);
	return 0;
}

/* Reads any pending audio message from the socket. */
static void flush_old_aud_messages(struct cras_audio_shm *shm, int fd)
{
	struct audio_message msg;
	struct pollfd pollfd;
	int err;

	pollfd.fd = fd;
	pollfd.events = POLLIN;

	do {
		err = poll(&pollfd, 1, 0);
		if (pollfd.revents & POLLIN) {
			err = read(fd, &msg, sizeof(msg));
			cras_shm_set_callback_pending(shm, 0);
		}
	} while (err > 0);
}

/* Asks any stream with room for more data. Sets the time stamp for all streams.
 * Args:
 *    thread - The thread to fetch samples for.
 *    adev - The output device streams are attached to.
 * Returns:
 *    0 on success, negative error on failure. If failed, can assume that all
 *    streams have been removed from the device.
 */
static int fetch_streams(struct audio_thread *thread,
			 struct open_dev *adev)
{
	struct dev_stream *dev_stream;
	struct cras_iodev *odev = adev->dev;
	int rc;
	int delay;

	delay = cras_iodev_delay_frames(odev);
	if (delay < 0)
		return delay;

	DL_FOREACH(adev->dev->streams, dev_stream) {
		struct cras_rstream *rstream = dev_stream->stream;
		struct cras_audio_shm *shm =
			cras_rstream_output_shm(rstream);
		int fd = cras_rstream_get_audio_fd(rstream);
		const struct timespec *next_cb_ts;
		struct timespec now;

		clock_gettime(CLOCK_MONOTONIC_RAW, &now);

		if (cras_shm_callback_pending(shm) && fd >= 0) {
			flush_old_aud_messages(shm, fd);
			cras_rstream_record_fetch_interval(dev_stream->stream,
							   &now);
		}

		if (cras_shm_get_frames(shm) < 0)
			cras_rstream_set_is_draining(rstream, 1);

		if (cras_rstream_get_is_draining(dev_stream->stream))
			continue;

		next_cb_ts = dev_stream_next_cb_ts(dev_stream);
		if (!next_cb_ts)
			continue;

		/* Check if it's time to get more data from this stream.
		 * Allowing for waking up half a little early. */
		add_timespecs(&now, &playback_wake_fuzz_ts);
		if (!timespec_after(&now, next_cb_ts))
			continue;

		if (!dev_stream_can_fetch(dev_stream)) {
			ATLOG(
				atlog, AUDIO_THREAD_STREAM_SKIP_CB,
				rstream->stream_id,
				shm->area->write_offset[0],
				shm->area->write_offset[1]);
			continue;
		}

		dev_stream_set_delay(dev_stream, delay);

		ATLOG(
				atlog,
				AUDIO_THREAD_FETCH_STREAM,
				rstream->stream_id,
				cras_rstream_get_cb_threshold(rstream), delay);

		rc = dev_stream_request_playback_samples(dev_stream, &now);
		if (rc < 0) {
			syslog(LOG_ERR, "fetch err: %d for %x",
			       rc, rstream->stream_id);
			cras_rstream_set_is_draining(rstream, 1);
		}
	}

	return 0;
}

/* Fill the buffer with samples from the attached streams.
 * Args:
 *    thread - The thread object the device is attached to.
 *    adev - The device to write to.
 *    dst - The buffer to put the samples in (returned from snd_pcm_mmap_begin)
 *    write_limit - The maximum number of frames to write to dst.
 *
 * Returns:
 *    The number of frames rendered on success, a negative error code otherwise.
 *    This number of frames is the minimum of the amount of frames each stream
 *    could provide which is the maximum that can currently be rendered.
 */
static int write_streams(struct audio_thread *thread,
			 struct open_dev *adev,
			 uint8_t *dst,
			 size_t write_limit)
{
	struct cras_iodev *odev = adev->dev;
	struct dev_stream *curr;
	unsigned int max_offset = 0;
	unsigned int frame_bytes = cras_get_format_bytes(odev->ext_format);
	unsigned int num_playing = 0;
	unsigned int drain_limit = write_limit;

	/* Mix as much as we can, the minimum fill level of any stream. */
	max_offset = cras_iodev_max_stream_offset(odev);

        /* Mix as much as we can, the minimum fill level of any stream. */
	DL_FOREACH(adev->dev->streams, curr) {
		int dev_frames;

		/* If this is a single output dev stream, updates the latest
		 * number of frames for playback. */
		if (dev_stream_attached_devs(curr) == 1)
			dev_stream_update_frames(curr);

		dev_frames = dev_stream_playback_frames(curr);
		if (dev_frames < 0) {
			thread_remove_stream(thread, curr->stream, NULL);
			continue;
		}
		ATLOG(atlog,
				AUDIO_THREAD_WRITE_STREAMS_STREAM,
				curr->stream->stream_id,
				dev_frames,
				cras_shm_callback_pending(cras_rstream_output_shm(curr->stream)));
		if (cras_rstream_get_is_draining(curr->stream)) {
			drain_limit = MIN((size_t)dev_frames, drain_limit);
			if (!dev_frames)
				thread_remove_stream(thread, curr->stream,
						     NULL);
		} else {
			write_limit = MIN((size_t)dev_frames, write_limit);
			num_playing++;
		}
	}

	if (!num_playing)
		write_limit = drain_limit;

	if (write_limit > max_offset)
		memset(dst + max_offset * frame_bytes, 0,
		       (write_limit - max_offset) * frame_bytes);

	ATLOG(atlog, AUDIO_THREAD_WRITE_STREAMS_MIX,
				    write_limit, max_offset, 0);

	DL_FOREACH(adev->dev->streams, curr) {
		unsigned int offset;
		int nwritten;

		offset = cras_iodev_stream_offset(odev, curr);
		if (offset >= write_limit)
			continue;
		nwritten = dev_stream_mix(curr, odev->ext_format,
					  dst + frame_bytes * offset,
					  write_limit - offset);

		if (nwritten < 0) {
			thread_remove_stream(thread, curr->stream, NULL);
			continue;
		}

		cras_iodev_stream_written(odev, curr, nwritten);
	}

	write_limit = cras_iodev_all_streams_written(odev);

	ATLOG(atlog, AUDIO_THREAD_WRITE_STREAMS_MIXED,
				    write_limit, 0, 0);

	return write_limit;
}

/* Gets the max delay frames of open input devices. */
static int input_delay_frames(struct open_dev *adevs)
{
	struct open_dev *adev;
	int delay;
	int max_delay = 0;

	DL_FOREACH(adevs, adev) {
		if (!cras_iodev_is_open(adev->dev))
			continue;
		delay = cras_iodev_delay_frames(adev->dev);
		if (delay < 0)
			return delay;
		if (delay > max_delay)
			max_delay = delay;
	}
	return max_delay;
}

/* Stop the playback thread */
static void terminate_pb_thread()
{
	pthread_exit(0);
}

static void append_dev_dump_info(struct audio_dev_debug_info *di,
				 struct open_dev *adev)
{
	struct cras_audio_format *fmt = adev->dev->ext_format;
	strncpy(di->dev_name, adev->dev->info.name, sizeof(di->dev_name));
	di->buffer_size = adev->dev->buffer_size;
	di->min_buffer_level = adev->dev->min_buffer_level;
	di->min_cb_level = adev->dev->min_cb_level;
	di->max_cb_level = adev->dev->max_cb_level;
	di->direction = adev->dev->direction;
	di->num_underruns = cras_iodev_get_num_underruns(adev->dev);
	di->num_severe_underruns = cras_iodev_get_num_severe_underruns(
			adev->dev);
	if (fmt) {
		di->frame_rate = fmt->frame_rate;
		di->num_channels = fmt->num_channels;
		di->est_rate_ratio = cras_iodev_get_est_rate_ratio(adev->dev);
	} else {
		di->frame_rate = 0;
		di->num_channels = 0;
		di->est_rate_ratio = 0;
	}
}

/* Put stream info for the given stream into the info struct. */
static void append_stream_dump_info(struct audio_debug_info *info,
				    struct dev_stream *stream,
				    unsigned int dev_idx,
				    int index)
{
	struct audio_stream_debug_info *si;

	si = &info->streams[index];

	si->stream_id = stream->stream->stream_id;
	si->dev_idx = dev_idx;
	si->direction = stream->stream->direction;
	si->stream_type = stream->stream->stream_type;
	si->buffer_frames = stream->stream->buffer_frames;
	si->cb_threshold = stream->stream->cb_threshold;
	si->frame_rate = stream->stream->format.frame_rate;
	si->num_channels = stream->stream->format.num_channels;
	memcpy(si->channel_layout, stream->stream->format.channel_layout,
	       sizeof(si->channel_layout));
	si->longest_fetch_sec = stream->stream->longest_fetch_interval.tv_sec;
	si->longest_fetch_nsec = stream->stream->longest_fetch_interval.tv_nsec;
	si->num_overruns = cras_shm_num_overruns(&stream->stream->shm);

	longest_wake.tv_sec = 0;
	longest_wake.tv_nsec = 0;
}

/* Handle a message sent to the playback thread */
static int handle_playback_thread_message(struct audio_thread *thread)
{
	uint8_t buf[256];
	struct audio_thread_msg *msg = (struct audio_thread_msg *)buf;
	int ret = 0;
	int err;

	err = audio_thread_read_command(thread, buf, 256);
	if (err < 0)
		return err;

	ATLOG(atlog, AUDIO_THREAD_PB_MSG, msg->id, 0, 0);

	switch (msg->id) {
	case AUDIO_THREAD_ADD_STREAM: {
		struct audio_thread_add_rm_stream_msg *amsg;
		amsg = (struct audio_thread_add_rm_stream_msg *)msg;
		ATLOG(
			atlog,
			AUDIO_THREAD_WRITE_STREAMS_WAIT,
			amsg->stream->stream_id, 0, 0);
		ret = thread_add_stream(thread, amsg->stream, amsg->devs,
				amsg->num_devs);
		break;
	}
	case AUDIO_THREAD_DISCONNECT_STREAM: {
		struct audio_thread_add_rm_stream_msg *rmsg;

		rmsg = (struct audio_thread_add_rm_stream_msg *)msg;

		ret = thread_disconnect_stream(thread, rmsg->stream,
				rmsg->devs[0]);
		break;
	}
	case AUDIO_THREAD_ADD_OPEN_DEV: {
		struct audio_thread_open_device_msg *rmsg;

		rmsg = (struct audio_thread_open_device_msg *)msg;
		ret = thread_add_open_dev(thread, rmsg->dev);
		break;
	}
	case AUDIO_THREAD_RM_OPEN_DEV: {
		struct audio_thread_open_device_msg *rmsg;

		rmsg = (struct audio_thread_open_device_msg *)msg;
		ret = thread_rm_open_dev(thread, rmsg->dev);
		break;
	}
	case AUDIO_THREAD_STOP:
		ret = 0;
		err = audio_thread_send_response(thread, ret);
		if (err < 0)
			return err;
		terminate_pb_thread();
		break;
	case AUDIO_THREAD_DUMP_THREAD_INFO: {
		struct dev_stream *curr;
		struct open_dev *adev;
		struct audio_thread_dump_debug_info_msg *dmsg;
		struct audio_debug_info *info;
		unsigned int num_streams = 0;
		unsigned int num_devs = 0;

		ret = 0;
		dmsg = (struct audio_thread_dump_debug_info_msg *)msg;
		info = dmsg->info;

		/* Go through all open devices. */
		DL_FOREACH(thread->open_devs[CRAS_STREAM_OUTPUT], adev) {
			append_dev_dump_info(&info->devs[num_devs], adev);
			if (++num_devs == MAX_DEBUG_DEVS)
				break;
			DL_FOREACH(adev->dev->streams, curr) {
				if (num_streams == MAX_DEBUG_STREAMS)
					break;
				append_stream_dump_info(info, curr,
							adev->dev->info.idx,
							num_streams++);
			}
		}
		DL_FOREACH(thread->open_devs[CRAS_STREAM_INPUT], adev) {
			if (num_devs == MAX_DEBUG_DEVS)
				break;
			append_dev_dump_info(&info->devs[num_devs], adev);
			DL_FOREACH(adev->dev->streams, curr) {
				if (num_streams == MAX_DEBUG_STREAMS)
					break;
				append_stream_dump_info(info, curr,
							adev->dev->info.idx,
							num_streams++);
			}
			++num_devs;
		}
		info->num_devs = num_devs;

		info->num_streams = num_streams;

		memcpy(&info->log, atlog, sizeof(info->log));
		break;
	}
	case AUDIO_THREAD_DRAIN_STREAM: {
		struct audio_thread_add_rm_stream_msg *rmsg;

		rmsg = (struct audio_thread_add_rm_stream_msg *)msg;
		ret = thread_drain_stream(thread, rmsg->stream);
		break;
	}
	case AUDIO_THREAD_REMOVE_CALLBACK: {
		struct audio_thread_rm_callback_msg *rmsg;

		rmsg = (struct audio_thread_rm_callback_msg *)msg;
		audio_thread_rm_callback(rmsg->fd);
		break;
	}
	case AUDIO_THREAD_CONFIG_GLOBAL_REMIX: {
		struct audio_thread_config_global_remix *rmsg;
		void *rsp;

		/* Respond the pointer to the old remix converter, so it can be
		 * freed later in main thread. */
		rsp = (void *)remix_converter;

		rmsg = (struct audio_thread_config_global_remix *)msg;
		remix_converter = rmsg->fmt_conv;

		return write(thread->to_main_fds[1], &rsp, sizeof(rsp));
	}
	case AUDIO_THREAD_DEV_START_RAMP: {
		struct audio_thread_dev_start_ramp_msg *rmsg;

		rmsg = (struct audio_thread_dev_start_ramp_msg*)msg;
		ret = thread_dev_start_ramp(thread, rmsg->dev, rmsg->request);
		break;
	}
	default:
		ret = -EINVAL;
		break;
	}

	err = audio_thread_send_response(thread, ret);
	if (err < 0)
		return err;
	return ret;
}

/* Fills the time that the next stream needs to be serviced. */
static int get_next_stream_wake_from_list(struct dev_stream *streams,
					  struct timespec *min_ts)
{
	struct dev_stream *dev_stream;
	int ret = 0; /* The total number of streams to wait on. */

	DL_FOREACH(streams, dev_stream) {
		const struct timespec *next_cb_ts;

		if (cras_rstream_get_is_draining(dev_stream->stream) &&
		    dev_stream_playback_frames(dev_stream) <= 0)
			continue;
		if (!dev_stream_can_fetch(dev_stream))
			continue;

		next_cb_ts = dev_stream_next_cb_ts(dev_stream);
		if (!next_cb_ts)
			continue;

		ATLOG(atlog,
					    AUDIO_THREAD_STREAM_SLEEP_TIME,
					    dev_stream->stream->stream_id,
					    next_cb_ts->tv_sec,
					    next_cb_ts->tv_nsec);
		if (timespec_after(min_ts, next_cb_ts))
			*min_ts = *next_cb_ts;
		ret++;
	}

	return ret;
}

static int get_next_output_wake(struct audio_thread *thread,
				 struct timespec *min_ts,
				 const struct timespec *now)
{
	struct open_dev *adev;
	struct timespec sleep_time;
	double est_rate;
	int ret = 0;
	unsigned int frames_to_play_in_sleep;
	unsigned int hw_level = 0;

	DL_FOREACH(thread->open_devs[CRAS_STREAM_OUTPUT], adev)
		ret += get_next_stream_wake_from_list(
				adev->dev->streams,
				min_ts);

	DL_FOREACH(thread->open_devs[CRAS_STREAM_OUTPUT], adev) {
		if (!cras_iodev_odev_should_wake(adev->dev))
			continue;

		frames_to_play_in_sleep = cras_iodev_frames_to_play_in_sleep(
				adev->dev, &hw_level, &adev->wake_ts);
		if (!timespec_is_nonzero(&adev->wake_ts))
			adev->wake_ts = *now;

		est_rate = adev->dev->ext_format->frame_rate *
				cras_iodev_get_est_rate_ratio(adev->dev);

		ATLOG(atlog,
	              AUDIO_THREAD_SET_DEV_WAKE,
		      adev->dev->info.idx,
		      hw_level,
		      frames_to_play_in_sleep);

		cras_frames_to_time_precise(
				frames_to_play_in_sleep,
				est_rate,
				&sleep_time);

		add_timespecs(&adev->wake_ts, &sleep_time);

		ret++;
		ATLOG(atlog,
					    AUDIO_THREAD_DEV_SLEEP_TIME,
					    adev->dev->info.idx,
					    adev->wake_ts.tv_sec,
					    adev->wake_ts.tv_nsec);
		if (timespec_after(min_ts, &adev->wake_ts))
			*min_ts = adev->wake_ts;
	}

	return ret;
}

static int input_adev_ignore_wake(const struct open_dev *adev)
{
	if (!cras_iodev_is_open(adev->dev))
		return 1;

	if (!adev->dev->active_node)
		return 1;

	if (adev->dev->active_node->type == CRAS_NODE_TYPE_HOTWORD &&
	    !adev->input_streaming)
		return 1;

	return 0;
}

static int get_next_input_wake(struct audio_thread *thread,
			       struct timespec *min_ts,
			       const struct timespec *now)
{
	struct open_dev *adev;
	int ret = 0; /* The total number of devices to wait on. */

	DL_FOREACH(thread->open_devs[CRAS_STREAM_INPUT], adev) {
		if (input_adev_ignore_wake(adev))
			continue;
		ret++;
		ATLOG(atlog,
					    AUDIO_THREAD_DEV_SLEEP_TIME,
					    adev->dev->info.idx,
					    adev->wake_ts.tv_sec,
					    adev->wake_ts.tv_nsec);
		if (timespec_after(min_ts, &adev->wake_ts))
			*min_ts = adev->wake_ts;
	}

	return ret;
}

static int output_stream_fetch(struct audio_thread *thread)
{
	struct open_dev *odev_list = thread->open_devs[CRAS_STREAM_OUTPUT];
	struct open_dev *adev;

	DL_FOREACH(odev_list, adev) {
		if (!cras_iodev_is_open(adev->dev))
			continue;
		fetch_streams(thread, adev);
	}

	return 0;
}

static int wait_pending_output_streams(struct audio_thread *thread)
{
	/* TODO(dgreid) - is this needed? */
	return 0;
}

/* Gets the master device which the stream is attached to. */
static inline
struct cras_iodev *get_master_dev(const struct dev_stream *stream)
{
	return (struct cras_iodev *)stream->stream->master_dev.dev_ptr;
}

/* Updates the estimated sample rate of open device to all attached
 * streams.
 */
static void update_estimated_rate(struct audio_thread *thread,
				  struct open_dev *adev)
{
	struct cras_iodev *master_dev;
	struct cras_iodev *dev = adev->dev;
	struct dev_stream *dev_stream;

	DL_FOREACH(dev->streams, dev_stream) {
		master_dev = get_master_dev(dev_stream);
		if (master_dev == NULL) {
			syslog(LOG_ERR, "Fail to find master open dev.");
			continue;
		}

		dev_stream_set_dev_rate(dev_stream,
				dev->ext_format->frame_rate,
				cras_iodev_get_est_rate_ratio(dev),
				cras_iodev_get_est_rate_ratio(master_dev),
				adev->coarse_rate_adjust);
	}
}

/* Returns 0 on success negative error on device failure. */
static int write_output_samples(struct audio_thread *thread,
				struct open_dev *adev)
{
	struct cras_iodev *odev = adev->dev;
	unsigned int hw_level;
	struct timespec hw_tstamp;
	unsigned int frames, fr_to_req;
	snd_pcm_sframes_t written;
	snd_pcm_uframes_t total_written = 0;
	int rc;
	uint8_t *dst = NULL;
	struct cras_audio_area *area = NULL;

	/* Possibly fill zeros for no_stream state and possibly transit state.
	 */
	rc = cras_iodev_prepare_output_before_write_samples(odev);
	if (rc < 0) {
		syslog(LOG_ERR, "Failed to prepare output dev for write");
		return rc;
	}

	if (cras_iodev_state(odev) != CRAS_IODEV_STATE_NORMAL_RUN)
		return 0;

	rc = cras_iodev_frames_queued(odev, &hw_tstamp);
	if (rc < 0)
		return rc;
	hw_level = rc;

	ATLOG(atlog, AUDIO_THREAD_FILL_AUDIO_TSTAMP, adev->dev->info.idx,
	      hw_tstamp.tv_sec, hw_tstamp.tv_nsec);
	if (timespec_is_nonzero(&hw_tstamp)) {
		if (hw_level < odev->min_cb_level / 2)
			adev->coarse_rate_adjust = 1;
		else if (hw_level > odev->max_cb_level * 2)
			adev->coarse_rate_adjust = -1;
		else
			adev->coarse_rate_adjust = 0;

		if (cras_iodev_update_rate(odev, hw_level, &hw_tstamp))
			update_estimated_rate(thread, adev);
	}
	ATLOG(atlog, AUDIO_THREAD_FILL_AUDIO,
				    adev->dev->info.idx, hw_level, 0);

	/* Don't request more than hardware can hold. Note that min_buffer_level
	 * has been subtracted from the actual hw_level so we need to take it
	 * into account here. */
	fr_to_req = cras_iodev_buffer_avail(odev, hw_level);

	/* Have to loop writing to the device, will be at most 2 loops, this
	 * only happens when the circular buffer is at the end and returns us a
	 * partial area to write to from mmap_begin */
	while (total_written < fr_to_req) {
		frames = fr_to_req - total_written;
		rc = cras_iodev_get_output_buffer(odev, &area, &frames);
		if (rc < 0)
			return rc;

		/* TODO(dgreid) - This assumes interleaved audio. */
		dst = area->channels[0].buf;
		written = write_streams(thread, adev, dst, frames);
		if (written < 0) /* pcm has been closed */
			return (int)written;

		if (written < (snd_pcm_sframes_t)frames)
			/* Got all the samples from client that we can, but it
			 * won't fill the request. */
			fr_to_req = 0; /* break out after committing samples */

		rc = cras_iodev_put_output_buffer(odev, dst, written);
		if (rc < 0)
			return rc;
		total_written += written;
	}

	/* Empty hardware and nothing written, zero fill it if it is running. */
	if (!hw_level && !total_written &&
	    odev->min_cb_level < odev->buffer_size)
		cras_iodev_output_underrun(odev);

	ATLOG(atlog, AUDIO_THREAD_FILL_AUDIO_DONE,
			hw_level, total_written, odev->min_cb_level);
	return 0;
}

static int do_playback(struct audio_thread *thread)
{
	struct open_dev *adev;
	struct dev_stream *curr;
	int rc;

	/* For multiple output case, update the number of queued frames in shm
	 * of all streams before starting write output samples. */
	adev = thread->open_devs[CRAS_STREAM_OUTPUT];
	if (adev && adev->next) {
		DL_FOREACH(thread->open_devs[CRAS_STREAM_OUTPUT], adev) {
			DL_FOREACH(adev->dev->streams, curr)
				dev_stream_update_frames(curr);
		}
	}

	DL_FOREACH(thread->open_devs[CRAS_STREAM_OUTPUT], adev) {
		if (!cras_iodev_is_open(adev->dev))
			continue;

		rc = write_output_samples(thread, adev);
		if (rc < 0) {
			if (rc == -EPIPE) {
				/* Handle severe underrun. */
				ATLOG(atlog, AUDIO_THREAD_SEVERE_UNDERRUN,
				      adev->dev->info.idx, 0, 0);
				cras_iodev_reset_request(adev->dev);
			} else {
				/* Device error, close it. */
				thread_rm_open_adev(thread, adev);
			}
		}
	}

	/* TODO(dgreid) - once per rstream, not once per dev_stream. */
	DL_FOREACH(thread->open_devs[CRAS_STREAM_OUTPUT], adev) {
		struct dev_stream *stream;
		if (!cras_iodev_is_open(adev->dev))
			continue;
		DL_FOREACH(adev->dev->streams, stream) {
			dev_stream_playback_update_rstream(stream);
		}
	}

	return 0;
}

/* Gets the minimum amount of space available for writing across all streams.
 * Args:
 *    adev - The device to capture from.
 *    write_limit - Initial limit to number of frames to capture.
 */
static unsigned int get_stream_limit_set_delay(struct open_dev *adev,
					      unsigned int write_limit)
{
	struct cras_rstream *rstream;
	struct cras_audio_shm *shm;
	struct dev_stream *stream;
	int delay;
	unsigned int avail;

	/* TODO(dgreid) - Setting delay from last dev only. */
	delay = input_delay_frames(adev);

	DL_FOREACH(adev->dev->streams, stream) {
		rstream = stream->stream;

		shm = cras_rstream_input_shm(rstream);
		if (cras_shm_check_write_overrun(shm))
			ATLOG(atlog, AUDIO_THREAD_READ_OVERRUN,
			      adev->dev->info.idx, rstream->stream_id,
			      shm->area->num_overruns);
		dev_stream_set_delay(stream, delay);
		avail = dev_stream_capture_avail(stream);
		write_limit = MIN(write_limit, avail);
	}

	return write_limit;
}

/* Read samples from an input device to the specified stream.
 * Args:
 *    adev - The device to capture samples from.
 * Returns 0 on success.
 */
static int capture_to_streams(struct audio_thread *thread,
			      struct open_dev *adev)
{
	struct cras_iodev *idev = adev->dev;
	snd_pcm_uframes_t remainder, hw_level, cap_limit;
	struct timespec hw_tstamp;
	int rc;

	rc = cras_iodev_frames_queued(idev, &hw_tstamp);
	if (rc < 0)
		return rc;
	hw_level = rc;

	ATLOG(atlog, AUDIO_THREAD_READ_AUDIO_TSTAMP, idev->info.idx,
	      hw_tstamp.tv_sec, hw_tstamp.tv_nsec);
	if (timespec_is_nonzero(&hw_tstamp)) {
		if (hw_level)
			adev->input_streaming = 1;

		if (hw_level < idev->min_cb_level / 2)
			adev->coarse_rate_adjust = 1;
		else if (hw_level > idev->max_cb_level * 2)
			adev->coarse_rate_adjust = -1;
		else
			adev->coarse_rate_adjust = 0;
		if (cras_iodev_update_rate(idev, hw_level, &hw_tstamp))
			update_estimated_rate(thread, adev);
	}

	cap_limit = get_stream_limit_set_delay(adev, hw_level);
	remainder = MIN(hw_level, cap_limit);

	ATLOG(atlog, AUDIO_THREAD_READ_AUDIO,
				    idev->info.idx, hw_level, remainder);

	if (cras_iodev_state(idev) != CRAS_IODEV_STATE_NORMAL_RUN)
		return 0;

	while (remainder > 0) {
		struct cras_audio_area *area = NULL;
		struct dev_stream *stream;
		unsigned int nread, total_read;

		nread = remainder;

		rc = cras_iodev_get_input_buffer(idev, &area, &nread);
		if (rc < 0 || nread == 0)
			return rc;

		DL_FOREACH(adev->dev->streams, stream) {
			unsigned int this_read;
			unsigned int area_offset;

			area_offset = cras_iodev_stream_offset(idev, stream);
			this_read = dev_stream_capture(
				stream, area, area_offset,
				cras_iodev_get_software_gain_scaler(idev));

			cras_iodev_stream_written(idev, stream, this_read);
		}
		if (adev->dev->streams)
			total_read = cras_iodev_all_streams_written(idev);
		else
			total_read = nread; /* No streams, drop. */

		rc = cras_iodev_put_input_buffer(idev, total_read);
		if (rc < 0)
			return rc;
		remainder -= nread;

		if (total_read < nread)
			break;
	}

	ATLOG(atlog, AUDIO_THREAD_READ_AUDIO_DONE,
				    remainder, 0, 0);

	return 0;
}

static int do_capture(struct audio_thread *thread)
{
	struct open_dev *idev_list = thread->open_devs[CRAS_STREAM_INPUT];
	struct open_dev *adev;

	DL_FOREACH(idev_list, adev) {
		if (!cras_iodev_is_open(adev->dev))
			continue;
		if (capture_to_streams(thread, adev) < 0)
			thread_rm_open_adev(thread, adev);
	}

	return 0;
}

/*
 * Set wake_ts for this device to be the earliest wake up time for
 * dev_streams.
 */
static int set_input_dev_wake_ts(struct open_dev *adev)
{
	int rc;
	struct timespec level_tstamp, wake_time_out, min_ts, now;
	unsigned int curr_level;
	struct dev_stream *stream;

	/* Limit the sleep time to 20 seconds. */
	min_ts.tv_sec = 20;
	min_ts.tv_nsec = 0;
	clock_gettime(CLOCK_MONOTONIC_RAW, &now);
	add_timespecs(&min_ts, &now);

	curr_level = cras_iodev_frames_queued(adev->dev, &level_tstamp);
	if (!timespec_is_nonzero(&level_tstamp))
		clock_gettime(CLOCK_MONOTONIC_RAW, &level_tstamp);

	/*
	 * Loop through streams to find the earliest time audio thread
	 * should wake up.
	 */
	DL_FOREACH(adev->dev->streams, stream) {
		rc = dev_stream_wake_time(
			stream,
			curr_level,
			&level_tstamp,
			&wake_time_out);

		if (rc < 0)
			return rc;

		if (timespec_after(&min_ts, &wake_time_out)) {
			min_ts = wake_time_out;
		}
	}
	adev->wake_ts = min_ts;
	return 0;
}

static int send_captured_samples(struct audio_thread *thread)
{
	struct open_dev *idev_list = thread->open_devs[CRAS_STREAM_INPUT];
	struct open_dev *adev;
	int rc;

	// TODO(dgreid) - once per rstream, not once per dev_stream.
	DL_FOREACH(idev_list, adev) {
		struct dev_stream *stream;

		if (!cras_iodev_is_open(adev->dev))
			continue;

		/* Post samples to rstream if there are enough samples. */
		DL_FOREACH(adev->dev->streams, stream) {
			dev_stream_capture_update_rstream(stream);
		}

		/* Set wake_ts for this device. */
		rc = set_input_dev_wake_ts(adev);
		if (rc < 0)
			return rc;
	}

	return 0;
}

/* Reads and/or writes audio sampels from/to the devices. */
static int stream_dev_io(struct audio_thread *thread)
{
	output_stream_fetch(thread);
	do_capture(thread);
	send_captured_samples(thread);
	wait_pending_output_streams(thread);
	do_playback(thread);

	return 0;
}

int fill_next_sleep_interval(struct audio_thread *thread, struct timespec *ts)
{
	struct timespec min_ts;
	struct timespec now;
	int ret; /* The sum of active streams and devices. */

	ts->tv_sec = 0;
	ts->tv_nsec = 0;
	/* Limit the sleep time to 20 seconds. */
	min_ts.tv_sec = 20;
	min_ts.tv_nsec = 0;
	clock_gettime(CLOCK_MONOTONIC_RAW, &now);
	add_timespecs(&min_ts, &now);
	ret = get_next_output_wake(thread, &min_ts, &now);
	ret += get_next_input_wake(thread, &min_ts, &now);
	if (timespec_after(&min_ts, &now))
		subtract_timespecs(&min_ts, &now, ts);

	return ret;
}

/* For playback, fill the audio buffer when needed, for capture, pull out
 * samples when they are ready.
 * This thread will attempt to run at a high priority to allow for low latency
 * streams.  This thread sleeps while the device plays back or captures audio,
 * it will wake up as little as it can while avoiding xruns.  It can also be
 * woken by sending it a message using the "audio_thread_post_message" function.
 */
static void *audio_io_thread(void *arg)
{
	struct audio_thread *thread = (struct audio_thread *)arg;
	struct open_dev *adev;
	struct dev_stream *curr;
	struct timespec ts, now, last_wake;
	struct pollfd *pollfds;
	unsigned int num_pollfds;
	unsigned int pollfds_size = 32;
	int msg_fd;
	int rc;

	msg_fd = thread->to_thread_fds[0];

	/* Attempt to get realtime scheduling */
	if (cras_set_rt_scheduling(CRAS_SERVER_RT_THREAD_PRIORITY) == 0)
		cras_set_thread_priority(CRAS_SERVER_RT_THREAD_PRIORITY);

	last_wake.tv_sec = 0;
	longest_wake.tv_sec = 0;
	longest_wake.tv_nsec = 0;

	pollfds = (struct pollfd *)malloc(sizeof(*pollfds) * pollfds_size);
	pollfds[0].fd = msg_fd;
	pollfds[0].events = POLLIN;

	while (1) {
		struct timespec *wait_ts;
		struct iodev_callback_list *iodev_cb;

		wait_ts = NULL;
		num_pollfds = 1;

		/* device opened */
		rc = stream_dev_io(thread);
		if (rc < 0)
			syslog(LOG_ERR, "audio cb error %d", rc);

		if (fill_next_sleep_interval(thread, &ts))
			wait_ts = &ts;

restart_poll_loop:
		num_pollfds = 1;

		DL_FOREACH(iodev_callbacks, iodev_cb) {
			if (!iodev_cb->enabled)
				continue;
			pollfds[num_pollfds].fd = iodev_cb->fd;
			iodev_cb->pollfd = &pollfds[num_pollfds];
			if (iodev_cb->is_write)
				pollfds[num_pollfds].events = POLLOUT;
			else
				pollfds[num_pollfds].events = POLLIN;
			num_pollfds++;
			if (num_pollfds >= pollfds_size) {
				pollfds_size *= 2;
				pollfds = (struct pollfd *)realloc(pollfds,
					sizeof(*pollfds) * pollfds_size);
				goto restart_poll_loop;
			}
		}

		/* TODO(dgreid) - once per rstream not per dev_stream */
		DL_FOREACH(thread->open_devs[CRAS_STREAM_OUTPUT], adev) {
			DL_FOREACH(adev->dev->streams, curr) {
				int fd = dev_stream_poll_stream_fd(curr);
				if (fd < 0)
					continue;
				pollfds[num_pollfds].fd = fd;
				pollfds[num_pollfds].events = POLLIN;
				num_pollfds++;
				if (num_pollfds >= pollfds_size) {
					pollfds_size *= 2;
					pollfds = (struct pollfd *)realloc(
							pollfds,
							sizeof(*pollfds) *
								pollfds_size);
					goto restart_poll_loop;
				}
			}
		}

		if (last_wake.tv_sec) {
			struct timespec this_wake;
			clock_gettime(CLOCK_MONOTONIC_RAW, &now);
			subtract_timespecs(&now, &last_wake, &this_wake);
			if (timespec_after(&this_wake, &longest_wake))
				longest_wake = this_wake;
		}

		ATLOG(atlog, AUDIO_THREAD_SLEEP,
					    wait_ts ? wait_ts->tv_sec : 0,
					    wait_ts ? wait_ts->tv_nsec : 0,
					    longest_wake.tv_nsec);
		rc = ppoll(pollfds, num_pollfds, wait_ts, NULL);
		clock_gettime(CLOCK_MONOTONIC_RAW, &last_wake);
		ATLOG(atlog, AUDIO_THREAD_WAKE, rc, 0, 0);
		if (rc <= 0)
			continue;

		if (pollfds[0].revents & POLLIN) {
			rc = handle_playback_thread_message(thread);
			if (rc < 0)
				syslog(LOG_INFO, "handle message %d", rc);
		}

		DL_FOREACH(iodev_callbacks, iodev_cb) {
			if (iodev_cb->pollfd &&
			    iodev_cb->pollfd->revents & (POLLIN | POLLOUT)) {
				ATLOG(
					atlog, AUDIO_THREAD_IODEV_CB,
					iodev_cb->is_write, 0, 0);
				iodev_cb->cb(iodev_cb->cb_data);
			}
		}
	}

	return NULL;
}

/* Write a message to the playback thread and wait for an ack, This keeps these
 * operations synchronous for the main server thread.  For instance when the
 * RM_STREAM message is sent, the stream can be deleted after the function
 * returns.  Making this synchronous also allows the thread to return an error
 * code that can be handled by the caller.
 * Args:
 *    thread - thread to receive message.
 *    msg - The message to send.
 * Returns:
 *    A return code from the message handler in the thread.
 */
static int audio_thread_post_message(struct audio_thread *thread,
				     struct audio_thread_msg *msg)
{
	int err;
	void *rsp;

	err = write(thread->to_thread_fds[1], msg, msg->length);
	if (err < 0) {
		syslog(LOG_ERR, "Failed to post message to thread.");
		return err;
	}
	/* Synchronous action, wait for response. */
	err = read(thread->to_main_fds[0], &rsp, sizeof(rsp));
	if (err < 0) {
		syslog(LOG_ERR, "Failed to read reply from thread.");
		return err;
	}

	return (intptr_t)rsp;
}

static void init_open_device_msg(struct audio_thread_open_device_msg *msg,
				 enum AUDIO_THREAD_COMMAND id,
				 struct cras_iodev *dev)
{
	memset(msg, 0, sizeof(*msg));
	msg->header.id = id;
	msg->header.length = sizeof(*msg);
	msg->dev = dev;
}

static void init_add_rm_stream_msg(struct audio_thread_add_rm_stream_msg *msg,
				   enum AUDIO_THREAD_COMMAND id,
				   struct cras_rstream *stream,
				   struct cras_iodev **devs,
				   unsigned int num_devs)
{
	memset(msg, 0, sizeof(*msg));
	msg->header.id = id;
	msg->header.length = sizeof(*msg);
	msg->stream = stream;
	msg->devs = devs;
	msg->num_devs = num_devs;
}

static void init_dump_debug_info_msg(
		struct audio_thread_dump_debug_info_msg *msg,
		struct audio_debug_info *info)
{
	memset(msg, 0, sizeof(*msg));
	msg->header.id = AUDIO_THREAD_DUMP_THREAD_INFO;
	msg->header.length = sizeof(*msg);
	msg->info = info;
}

static void init_config_global_remix_msg(
		struct audio_thread_config_global_remix *msg)
{
	memset(msg, 0, sizeof(*msg));
	msg->header.id = AUDIO_THREAD_CONFIG_GLOBAL_REMIX;
	msg->header.length = sizeof(*msg);
}

static void init_device_start_ramp_msg(
		struct audio_thread_dev_start_ramp_msg *msg,
		enum AUDIO_THREAD_COMMAND id,
		struct cras_iodev *dev,
		enum CRAS_IODEV_RAMP_REQUEST request)
{
	memset(msg, 0, sizeof(*msg));
	msg->header.id = id;
	msg->header.length = sizeof(*msg);
	msg->dev = dev;
	msg->request = request;
}

/* Exported Interface */

int audio_thread_add_stream(struct audio_thread *thread,
			    struct cras_rstream *stream,
			    struct cras_iodev **devs,
			    unsigned int num_devs)
{
	struct audio_thread_add_rm_stream_msg msg;

	assert(thread && stream);

	if (!thread->started)
		return -EINVAL;

	init_add_rm_stream_msg(&msg, AUDIO_THREAD_ADD_STREAM, stream,
			       devs, num_devs);
	return audio_thread_post_message(thread, &msg.header);
}

int audio_thread_disconnect_stream(struct audio_thread *thread,
				   struct cras_rstream *stream,
				   struct cras_iodev *dev)
{
	struct audio_thread_add_rm_stream_msg msg;

	assert(thread && stream);

	init_add_rm_stream_msg(&msg, AUDIO_THREAD_DISCONNECT_STREAM, stream,
			       &dev, 0);
	return audio_thread_post_message(thread, &msg.header);
}

int audio_thread_drain_stream(struct audio_thread *thread,
			      struct cras_rstream *stream)
{
	struct audio_thread_add_rm_stream_msg msg;

	assert(thread && stream);

	init_add_rm_stream_msg(&msg, AUDIO_THREAD_DRAIN_STREAM, stream,
			       NULL, 0);
	return audio_thread_post_message(thread, &msg.header);
}

int audio_thread_dump_thread_info(struct audio_thread *thread,
				  struct audio_debug_info *info)
{
	struct audio_thread_dump_debug_info_msg msg;

	init_dump_debug_info_msg(&msg, info);
	return audio_thread_post_message(thread, &msg.header);
}

int audio_thread_rm_callback_sync(struct audio_thread *thread, int fd) {
	struct audio_thread_rm_callback_msg msg;

	memset(&msg, 0, sizeof(msg));
	msg.header.id = AUDIO_THREAD_REMOVE_CALLBACK;
	msg.header.length = sizeof(msg);
	msg.fd = fd;

	return audio_thread_post_message(thread, &msg.header);
}

int audio_thread_config_global_remix(struct audio_thread *thread,
				     unsigned int num_channels,
				     const float *coefficient)
{
	int err;
	int identity_remix = 1;
	unsigned int i, j;
	struct audio_thread_config_global_remix msg;
	void *rsp;

	init_config_global_remix_msg(&msg);

	/* Check if the coefficients represent an identity matrix for remix
	 * conversion, which means no remix at all. If so then leave the
	 * converter as NULL. */
	for (i = 0; i < num_channels; i++) {
		if (coefficient[i * num_channels + i] != 1.0f) {
			identity_remix = 0;
			break;
		}
		for (j = i + 1; j < num_channels; j++) {
			if (coefficient[i * num_channels + j] != 0 ||
			    coefficient[j * num_channels + i] != 0)
				identity_remix = 0;
				break;
		}
	}

	if (!identity_remix) {
		msg.fmt_conv = cras_channel_remix_conv_create(num_channels,
							      coefficient);
		if (NULL == msg.fmt_conv)
			return -ENOMEM;
	}

	err = write(thread->to_thread_fds[1], &msg, msg.header.length);
	if (err < 0) {
		syslog(LOG_ERR, "Failed to post message to thread.");
		return err;
	}
	/* Synchronous action, wait for response. */
	err = read(thread->to_main_fds[0], &rsp, sizeof(rsp));
	if (err < 0) {
		syslog(LOG_ERR, "Failed to read reply from thread.");
		return err;
	}

	if (rsp)
		cras_fmt_conv_destroy((struct cras_fmt_conv *)rsp);
	return 0;
}

struct cras_fmt_conv *audio_thread_get_global_remix_converter()
{
	return remix_converter;
}

struct audio_thread *audio_thread_create()
{
	int rc;
	struct audio_thread *thread;

	thread = (struct audio_thread *)calloc(1, sizeof(*thread));
	if (!thread)
		return NULL;

	thread->to_thread_fds[0] = -1;
	thread->to_thread_fds[1] = -1;
	thread->to_main_fds[0] = -1;
	thread->to_main_fds[1] = -1;

	/* Two way pipes for communication with the device's audio thread. */
	rc = pipe(thread->to_thread_fds);
	if (rc < 0) {
		syslog(LOG_ERR, "Failed to pipe");
		free(thread);
		return NULL;
	}
	rc = pipe(thread->to_main_fds);
	if (rc < 0) {
		syslog(LOG_ERR, "Failed to pipe");
		free(thread);
		return NULL;
	}

	atlog = audio_thread_event_log_init();

	return thread;
}

int audio_thread_add_open_dev(struct audio_thread *thread,
				struct cras_iodev *dev)
{
	struct audio_thread_open_device_msg msg;

	assert(thread && dev);

	if (!thread->started)
		return -EINVAL;

	init_open_device_msg(&msg, AUDIO_THREAD_ADD_OPEN_DEV, dev);
	return audio_thread_post_message(thread, &msg.header);
}

int audio_thread_rm_open_dev(struct audio_thread *thread,
			     struct cras_iodev *dev)
{
	struct audio_thread_open_device_msg msg;

	assert(thread && dev);
	if (!thread->started)
		return -EINVAL;

	init_open_device_msg(&msg, AUDIO_THREAD_RM_OPEN_DEV, dev);
	return audio_thread_post_message(thread, &msg.header);
}

int audio_thread_dev_start_ramp(struct audio_thread *thread,
				struct cras_iodev *dev,
				enum CRAS_IODEV_RAMP_REQUEST request)
{
	struct audio_thread_dev_start_ramp_msg msg;

	assert(thread && dev);

	if (!thread->started)
		return -EINVAL;

	init_device_start_ramp_msg(&msg, AUDIO_THREAD_DEV_START_RAMP,
				   dev, request);
	return audio_thread_post_message(thread, &msg.header);
}

int audio_thread_start(struct audio_thread *thread)
{
	int rc;

	rc = pthread_create(&thread->tid, NULL, audio_io_thread, thread);
	if (rc) {
		syslog(LOG_ERR, "Failed pthread_create");
		return rc;
	}

	thread->started = 1;

	return 0;
}

void audio_thread_destroy(struct audio_thread *thread)
{
	audio_thread_event_log_deinit(atlog);

	if (thread->started) {
		struct audio_thread_msg msg;

		msg.id = AUDIO_THREAD_STOP;
		msg.length = sizeof(msg);
		audio_thread_post_message(thread, &msg);
		pthread_join(thread->tid, NULL);
	}

	if (thread->to_thread_fds[0] != -1) {
		close(thread->to_thread_fds[0]);
		close(thread->to_thread_fds[1]);
	}
	if (thread->to_main_fds[0] != -1) {
		close(thread->to_main_fds[0]);
		close(thread->to_main_fds[1]);
	}

	if (remix_converter)
		cras_fmt_conv_destroy(remix_converter);

	free(thread);
}
