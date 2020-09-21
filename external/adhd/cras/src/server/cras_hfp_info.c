/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <syslog.h>

#include "audio_thread.h"
#include "byte_buffer.h"
#include "cras_iodev_list.h"
#include "cras_hfp_info.h"
#include "utlist.h"

/* The max buffer size. Note that the actual used size must set to multiple
 * of SCO packet size, and the packet size does not necessarily be equal to
 * MTU.
 */
#define MAX_HFP_BUF_SIZE_BYTES 16384

/* rate(8kHz) * sample_size(2 bytes) * channels(1) */
#define HFP_BYTE_RATE 16000

/* Structure to hold variables for a HFP connection. Since HFP supports
 * bi-direction audio, two iodevs should share one hfp_info if they
 * represent two directions of the same HFP headset
 * Members:
 *     fd - The file descriptor for SCO socket.
 *     started - If the hfp_info has started to read/write SCO data.
 *     mtu - The max transmit unit reported from BT adapter.
 *     packet_size - The size of SCO packet to read/write preferred by
 *         adapter, could be different than mtu.
 *     capture_buf - The buffer to hold samples read from SCO socket.
 *     playback_buf - The buffer to hold samples about to write to SCO socket.
 *     idev - The input iodev using this hfp_info.
 *     odev - The output iodev using this hfp_info.
 *     packet_size_changed_cbs - The callbacks to trigger when SCO packet
 *         size changed.
 */
struct hfp_info {
	int fd;
	int started;
	unsigned int mtu;
	unsigned int packet_size;
	struct byte_buffer *capture_buf;
	struct byte_buffer *playback_buf;

	struct cras_iodev *idev;
	struct cras_iodev *odev;
	struct hfp_packet_size_changed_callback *packet_size_changed_cbs;
};

int hfp_info_add_iodev(struct hfp_info *info, struct cras_iodev *dev)
{
	if (dev->direction == CRAS_STREAM_OUTPUT) {
		if (info->odev)
			goto invalid;
		info->odev = dev;

		buf_reset(info->playback_buf);
	} else if (dev->direction == CRAS_STREAM_INPUT) {
		if (info->idev)
			goto invalid;
		info->idev = dev;

		buf_reset(info->capture_buf);
	}

	return 0;

invalid:
	return -EINVAL;
}

int hfp_info_rm_iodev(struct hfp_info *info, struct cras_iodev *dev)
{
	if (dev->direction == CRAS_STREAM_OUTPUT && info->odev == dev) {
		info->odev = NULL;
	} else if (dev->direction == CRAS_STREAM_INPUT && info->idev == dev){
		info->idev = NULL;
	} else
		return -EINVAL;

	return 0;
}

int hfp_info_has_iodev(struct hfp_info *info)
{
	return info->odev || info->idev;
}

void hfp_buf_acquire(struct hfp_info *info, struct cras_iodev *dev,
		     uint8_t **buf, unsigned *count)
{
	size_t format_bytes;
	unsigned int buf_avail;
	format_bytes = cras_get_format_bytes(dev->format);

	*count *= format_bytes;

	if (dev->direction == CRAS_STREAM_OUTPUT)
		*buf = buf_write_pointer_size(info->playback_buf, &buf_avail);
	else
		*buf = buf_read_pointer_size(info->capture_buf, &buf_avail);

	if (*count > buf_avail)
		*count = buf_avail;
	*count /= format_bytes;
}

int hfp_buf_size(struct hfp_info *info, struct cras_iodev *dev)
{
	return info->playback_buf->used_size / cras_get_format_bytes(dev->format);
}

void hfp_buf_release(struct hfp_info *info, struct cras_iodev *dev,
		     unsigned written_frames)
{
	size_t format_bytes;
	format_bytes = cras_get_format_bytes(dev->format);

	written_frames *= format_bytes;

	if (dev->direction == CRAS_STREAM_OUTPUT)
		buf_increment_write(info->playback_buf, written_frames);
	else
		buf_increment_read(info->capture_buf, written_frames);
}

int hfp_buf_queued(struct hfp_info *info, const struct cras_iodev *dev)
{
	size_t format_bytes;
	format_bytes = cras_get_format_bytes(dev->format);

	if (dev->direction == CRAS_STREAM_OUTPUT)
		return buf_queued_bytes(info->playback_buf) / format_bytes;
	else
		return buf_queued_bytes(info->capture_buf) / format_bytes;
}

int hfp_write(struct hfp_info *info)
{
	int err = 0;
	unsigned to_send;
	uint8_t *samples;

	/* Write something */
	samples = buf_read_pointer_size(info->playback_buf, &to_send);
	if (to_send < info->packet_size)
		return 0;
	to_send = info->packet_size;

send_sample:
	err = send(info->fd, samples, to_send, 0);
	if (err < 0) {
		if (errno == EINTR)
			goto send_sample;

		return err;
	}

	if (err != (int)info->packet_size) {
		syslog(LOG_ERR,
		       "Partially write %d bytes for SCO packet size %u",
		       err, info->packet_size);
		return -1;
	}

	buf_increment_read(info->playback_buf, to_send);

	return err;
}


static void hfp_info_set_packet_size(struct hfp_info *info,
				     unsigned int packet_size)
{
	struct hfp_packet_size_changed_callback *callback;
	unsigned int used_size =
		MAX_HFP_BUF_SIZE_BYTES / packet_size * packet_size;
	info->packet_size = packet_size;
	byte_buffer_set_used_size(info->playback_buf, used_size);
	byte_buffer_set_used_size(info->capture_buf, used_size);

	DL_FOREACH(info->packet_size_changed_cbs, callback)
		callback->cb(callback->data);
}

void hfp_register_packet_size_changed_callback(struct hfp_info *info,
					       void (*cb)(void *data),
					       void *data)
{
	struct hfp_packet_size_changed_callback *callback =
		(struct hfp_packet_size_changed_callback *)calloc(1,
			sizeof(struct hfp_packet_size_changed_callback));
	callback->data = data;
	callback->cb = cb;
	DL_APPEND(info->packet_size_changed_cbs, callback);
}

void hfp_unregister_packet_size_changed_callback(struct hfp_info *info,
						 void *data)
{
	struct hfp_packet_size_changed_callback *callback;
	DL_FOREACH(info->packet_size_changed_cbs, callback) {
		if (data == callback->data) {
			DL_DELETE(info->packet_size_changed_cbs, callback);
			free(callback);
		}
	}
}

int hfp_read(struct hfp_info *info)
{
	int err = 0;
	unsigned to_read;
	uint8_t *capture_buf;

	capture_buf = buf_write_pointer_size(info->capture_buf, &to_read);

	if (to_read < info->packet_size)
		return 0;
	to_read = info->packet_size;

recv_sample:
	err = recv(info->fd, capture_buf, to_read, 0);
	if (err < 0) {
		syslog(LOG_ERR, "Read error %s", strerror(errno));
		if (errno == EINTR)
			goto recv_sample;

		return err;
	}

	if (err != (int)info->packet_size) {
		/* Allow the SCO packet size be modified from the default MTU
		 * value to the size of SCO data we first read. This is for
		 * some adapters who prefers a different value than MTU for
		 * transmitting SCO packet.
		 */
		if (err && (info->packet_size == info->mtu)) {
			hfp_info_set_packet_size(info, err);
		} else {
			syslog(LOG_ERR, "Partially read %d bytes for %u size SCO packet",
			       err, info->packet_size);
			return -1;
		}
	}

	buf_increment_write(info->capture_buf, err);

	return err;
}

/* Callback function to handle sample read and write.
 * Note that we poll the SCO socket for read sample, since it reflects
 * there is actual some sample to read while the socket always reports
 * writable even when device buffer is full.
 * The strategy is to synchronize read & write operations:
 * 1. Read one chunk of MTU bytes of data.
 * 2. When input device not attached, ignore the data just read.
 * 3. When output device attached, write one chunk of MTU bytes of data.
 */
static int hfp_info_callback(void *arg)
{
	struct hfp_info *info = (struct hfp_info *)arg;
	int err;

	if (!info->started)
		goto read_write_error;

	err = hfp_read(info);
	if (err < 0) {
		syslog(LOG_ERR, "Read error");
		goto read_write_error;
	}

	/* Ignore the MTU bytes just read if input dev not in present */
	if (!info->idev)
		buf_increment_read(info->capture_buf, info->packet_size);

	if (info->odev) {
		err = hfp_write(info);
		if (err < 0) {
			syslog(LOG_ERR, "Write error");
			goto read_write_error;
		}
	}

	return 0;

read_write_error:
	hfp_info_stop(info);

	return 0;
}

struct hfp_info *hfp_info_create()
{
	struct hfp_info *info;
	info = (struct hfp_info *)calloc(1, sizeof(*info));
	if (!info)
		goto error;

	info->capture_buf = byte_buffer_create(MAX_HFP_BUF_SIZE_BYTES);
	if (!info->capture_buf)
		goto error;

	info->playback_buf = byte_buffer_create(MAX_HFP_BUF_SIZE_BYTES);
	if (!info->playback_buf)
		goto error;

	return info;

error:
	if (info) {
		if (info->capture_buf)
			byte_buffer_destroy(info->capture_buf);
		if (info->playback_buf)
			byte_buffer_destroy(info->playback_buf);
		free(info);
	}
	return NULL;
}

int hfp_info_running(struct hfp_info *info)
{
	return info->started;
}

int hfp_info_start(int fd, unsigned int mtu, struct hfp_info *info)
{
	info->fd = fd;
	info->mtu = mtu;

	/* Make sure buffer size is multiple of packet size, which initially
	 * set to MTU. */
	hfp_info_set_packet_size(info, mtu);
	buf_reset(info->playback_buf);
	buf_reset(info->capture_buf);

	audio_thread_add_callback(info->fd, hfp_info_callback, info);

	info->started = 1;

	return 0;
}

int hfp_info_stop(struct hfp_info *info)
{
	if (!info->started)
		return 0;

	audio_thread_rm_callback_sync(
		cras_iodev_list_get_audio_thread(),
		info->fd);

	close(info->fd);
	info->fd = 0;
	info->started = 0;

	return 0;
}

void hfp_info_destroy(struct hfp_info *info)
{
	if (info->capture_buf)
		byte_buffer_destroy(info->capture_buf);

	if (info->playback_buf)
		byte_buffer_destroy(info->playback_buf);

	free(info);
}
