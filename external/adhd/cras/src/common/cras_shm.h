/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_SHM_H_
#define CRAS_SHM_H_

#include <assert.h>
#include <stdint.h>
#include <sys/param.h>

#include "cras_types.h"
#include "cras_util.h"

#define CRAS_NUM_SHM_BUFFERS 2U /* double buffer */
#define CRAS_SHM_BUFFERS_MASK (CRAS_NUM_SHM_BUFFERS - 1)

/* Configuration of the shm area.
 *
 *  used_size - The size in bytes of the sample area being actively used.
 *  frame_bytes - The size of each frame in bytes.
 */
struct __attribute__ ((__packed__)) cras_audio_shm_config {
	uint32_t used_size;
	uint32_t frame_bytes;
};

/* Structure that is shared as shm between client and server.
 *
 *  config - Size config data.  A copy of the config shared with clients.
 *  read_buf_idx - index of the current buffer to read from (0 or 1 if double
 *    buffered).
 *  write_buf_idx - index of the current buffer to write to (0 or 1 if double
 *    buffered).
 *  read_offset - offset of the next sample to read (one per buffer).
 *  write_offset - offset of the next sample to write (one per buffer).
 *  write_in_progress - non-zero when a write is in progress.
 *  volume_scaler - volume scaling factor (0.0-1.0).
 *  muted - bool, true if stream should be muted.
 *  num_overruns - Starting at 0 this is incremented very time data is over
 *    written because too much accumulated before a read.
 *  ts - For capture, the time stamp of the next sample at read_index.  For
 *    playback, this is the time that the next sample written will be played.
 *    This is only valid in audio callbacks.
 *  samples - Audio data - a double buffered area that is used to exchange
 *    audio samples.
 */
struct __attribute__ ((__packed__)) cras_audio_shm_area {
	struct cras_audio_shm_config config;
	uint32_t read_buf_idx; /* use buffer A or B */
	uint32_t write_buf_idx;
	uint32_t read_offset[CRAS_NUM_SHM_BUFFERS];
	uint32_t write_offset[CRAS_NUM_SHM_BUFFERS];
	int32_t write_in_progress[CRAS_NUM_SHM_BUFFERS];
	float volume_scaler;
	int32_t mute;
	int32_t callback_pending;
	uint32_t num_overruns;
	struct cras_timespec ts;
	uint8_t samples[];
};

/* Structure that holds the config for and a pointer to the audio shm area.
 *
 *  config - Size config data, kept separate so it can be checked.
 *  area - Acutal shm region that is shared.
 */
struct cras_audio_shm {
	struct cras_audio_shm_config config;
	struct cras_audio_shm_area *area;
};

/* Get a pointer to the buffer at idx. */
static inline uint8_t *cras_shm_buff_for_idx(const struct cras_audio_shm *shm,
					     size_t idx)
{
	assert_on_compile_is_power_of_2(CRAS_NUM_SHM_BUFFERS);
	idx = idx & CRAS_SHM_BUFFERS_MASK;
	return shm->area->samples + shm->config.used_size * idx;
}

/* Limit a read offset to within the buffer size. */
static inline
unsigned cras_shm_check_read_offset(const struct cras_audio_shm *shm,
				    unsigned offset)
{
	/* The offset is allowed to be the total size, indicating that the
	 * buffer is full. If read pointer is invalid assume it is at the
	 * beginning. */
	if (offset > shm->config.used_size)
		return 0;
	return offset;
}

/* Limit a write offset to within the buffer size. */
static inline
unsigned cras_shm_check_write_offset(const struct cras_audio_shm *shm,
				     unsigned offset)
{
	/* The offset is allowed to be the total size, indicating that the
	 * buffer is full. If write pointer is invalid assume it is at the
	 * end. */
	if (offset > shm->config.used_size)
		return shm->config.used_size;
	return offset;
}

/* Get the number of frames readable in current read buffer */
static inline
unsigned cras_shm_get_curr_read_frames(const struct cras_audio_shm *shm)
{
	unsigned i = shm->area->read_buf_idx & CRAS_SHM_BUFFERS_MASK;
	unsigned read_offset, write_offset;

	read_offset =
		cras_shm_check_read_offset(shm, shm->area->read_offset[i]);
	write_offset =
		cras_shm_check_write_offset(shm, shm->area->write_offset[i]);

	if (read_offset > write_offset)
		return 0;
	else
		return (write_offset - read_offset) / shm->config.frame_bytes;
}

/* Get the base of the current read buffer. */
static inline
uint8_t *cras_shm_get_read_buffer_base(const struct cras_audio_shm *shm)
{
	unsigned i = shm->area->read_buf_idx & CRAS_SHM_BUFFERS_MASK;
	return cras_shm_buff_for_idx(shm, i);
}

/* Get the base of the current write buffer. */
static inline
uint8_t *cras_shm_get_write_buffer_base(const struct cras_audio_shm *shm)
{
	unsigned i = shm->area->write_buf_idx & CRAS_SHM_BUFFERS_MASK;

	return cras_shm_buff_for_idx(shm, i);
}

/* Get a pointer to the next buffer to write */
static inline
uint8_t *cras_shm_get_writeable_frames(const struct cras_audio_shm *shm,
				       unsigned limit_frames,
				       unsigned *frames)
{
	unsigned i = shm->area->write_buf_idx & CRAS_SHM_BUFFERS_MASK;
	unsigned write_offset;
	const unsigned frame_bytes = shm->config.frame_bytes;

	write_offset = cras_shm_check_write_offset(shm,
						   shm->area->write_offset[i]);
	if (frames)
		*frames = limit_frames - (write_offset / frame_bytes);

	return cras_shm_buff_for_idx(shm, i) + write_offset;
}

/* Get a pointer to the current read buffer plus an offset.  The offset might be
 * in the next buffer. 'frames' is filled with the number of frames that can be
 * copied from the returned buffer.
 */
static inline
uint8_t *cras_shm_get_readable_frames(const struct cras_audio_shm *shm,
				      size_t offset,
				      size_t *frames)
{
	unsigned buf_idx = shm->area->read_buf_idx & CRAS_SHM_BUFFERS_MASK;
	unsigned read_offset, write_offset, final_offset;

	assert(frames != NULL);

	read_offset =
		cras_shm_check_read_offset(shm,
					   shm->area->read_offset[buf_idx]);
	write_offset =
		cras_shm_check_write_offset(shm,
					    shm->area->write_offset[buf_idx]);
	final_offset = read_offset + offset * shm->config.frame_bytes;
	if (final_offset >= write_offset) {
		final_offset -= write_offset;
		assert_on_compile_is_power_of_2(CRAS_NUM_SHM_BUFFERS);
		buf_idx = (buf_idx + 1) & CRAS_SHM_BUFFERS_MASK;
		write_offset = cras_shm_check_write_offset(
				shm, shm->area->write_offset[buf_idx]);
	}
	if (final_offset >= write_offset) {
		/* Past end of samples. */
		*frames = 0;
		return NULL;
	}
	*frames = (write_offset - final_offset) / shm->config.frame_bytes;
	return cras_shm_buff_for_idx(shm, buf_idx) + final_offset;
}

/* How many bytes are queued? */
static inline size_t cras_shm_get_bytes_queued(const struct cras_audio_shm *shm)
{
	size_t total, i;
	const unsigned used_size = shm->config.used_size;

	total = 0;
	for (i = 0; i < CRAS_NUM_SHM_BUFFERS; i++) {
		unsigned read_offset, write_offset;

		read_offset = MIN(shm->area->read_offset[i], used_size);
		write_offset = MIN(shm->area->write_offset[i], used_size);

		if (write_offset > read_offset)
			total += write_offset - read_offset;
	}
	return total;
}

/* How many frames are queued? */
static inline int cras_shm_get_frames(const struct cras_audio_shm *shm)
{
	size_t bytes;

	bytes = cras_shm_get_bytes_queued(shm);
	if (bytes % shm->config.frame_bytes != 0)
		return -EIO;
	return bytes / shm->config.frame_bytes;
}

/* How many frames in the current buffer? */
static inline
size_t cras_shm_get_frames_in_curr_buffer(const struct cras_audio_shm *shm)
{
	size_t buf_idx = shm->area->read_buf_idx & CRAS_SHM_BUFFERS_MASK;
	unsigned read_offset, write_offset;
	const unsigned used_size = shm->config.used_size;

	read_offset = MIN(shm->area->read_offset[buf_idx], used_size);
	write_offset = MIN(shm->area->write_offset[buf_idx], used_size);

	if (write_offset <= read_offset)
		return 0;

	return (write_offset - read_offset) / shm->config.frame_bytes;
}

/* Return 1 if there is an empty buffer in the list. */
static inline int cras_shm_is_buffer_available(const struct cras_audio_shm *shm)
{
	size_t buf_idx = shm->area->write_buf_idx & CRAS_SHM_BUFFERS_MASK;

	return (shm->area->write_offset[buf_idx] == 0);
}

/* How many are available to be written? */
static inline
size_t cras_shm_get_num_writeable(const struct cras_audio_shm *shm)
{
	/* Not allowed to write to a buffer twice. */
	if (!cras_shm_is_buffer_available(shm))
		return 0;

	return shm->config.used_size / shm->config.frame_bytes;
}

/* Flags an overrun if writing would cause one and reset the write offset.
 * Return 1 if overrun happens, otherwise return 0. */
static inline int cras_shm_check_write_overrun(struct cras_audio_shm *shm)
{
	int ret = 0;
	size_t write_buf_idx = shm->area->write_buf_idx & CRAS_SHM_BUFFERS_MASK;

	if (!shm->area->write_in_progress[write_buf_idx]) {
		unsigned int used_size = shm->config.used_size;

		if (shm->area->write_offset[write_buf_idx]) {
			shm->area->num_overruns++; /* Will over-write unread */
			ret = 1;
		}

		memset(cras_shm_buff_for_idx(shm, write_buf_idx), 0, used_size);

		shm->area->write_in_progress[write_buf_idx] = 1;
		shm->area->write_offset[write_buf_idx] = 0;
	}
	return ret;
}

/* Increment the write pointer for the current buffer. */
static inline
void cras_shm_buffer_written(struct cras_audio_shm *shm, size_t frames)
{
	size_t buf_idx = shm->area->write_buf_idx & CRAS_SHM_BUFFERS_MASK;

	if (frames == 0)
		return;

	shm->area->write_offset[buf_idx] += frames * shm->config.frame_bytes;
	shm->area->read_offset[buf_idx] = 0;
}

/* Returns the number of frames that have been written to the current buffer. */
static inline
unsigned int cras_shm_frames_written(const struct cras_audio_shm *shm)
{
	size_t buf_idx = shm->area->write_buf_idx & CRAS_SHM_BUFFERS_MASK;

	return shm->area->write_offset[buf_idx] / shm->config.frame_bytes;
}

/* Signals the writing to this buffer is complete and moves to the next one. */
static inline void cras_shm_buffer_write_complete(struct cras_audio_shm *shm)
{
	size_t buf_idx = shm->area->write_buf_idx & CRAS_SHM_BUFFERS_MASK;

	shm->area->write_in_progress[buf_idx] = 0;

	assert_on_compile_is_power_of_2(CRAS_NUM_SHM_BUFFERS);
	buf_idx = (buf_idx + 1) & CRAS_SHM_BUFFERS_MASK;
	shm->area->write_buf_idx = buf_idx;
}

/* Set the write pointer for the current buffer and complete the write. */
static inline
void cras_shm_buffer_written_start(struct cras_audio_shm *shm, size_t frames)
{
	size_t buf_idx = shm->area->write_buf_idx & CRAS_SHM_BUFFERS_MASK;

	shm->area->write_offset[buf_idx] = frames * shm->config.frame_bytes;
	shm->area->read_offset[buf_idx] = 0;
	cras_shm_buffer_write_complete(shm);
}

/* Increment the read pointer.  If it goes past the write pointer for this
 * buffer, move to the next buffer. */
static inline
void cras_shm_buffer_read(struct cras_audio_shm *shm, size_t frames)
{
	size_t buf_idx = shm->area->read_buf_idx & CRAS_SHM_BUFFERS_MASK;
	size_t remainder;
	struct cras_audio_shm_area *area = shm->area;
	struct cras_audio_shm_config *config = &shm->config;

	if (frames == 0)
		return;

	area->read_offset[buf_idx] += frames * config->frame_bytes;
	if (area->read_offset[buf_idx] >= area->write_offset[buf_idx]) {
		remainder = area->read_offset[buf_idx] -
				area->write_offset[buf_idx];
		area->read_offset[buf_idx] = 0;
		area->write_offset[buf_idx] = 0;
		assert_on_compile_is_power_of_2(CRAS_NUM_SHM_BUFFERS);
		buf_idx = (buf_idx + 1) & CRAS_SHM_BUFFERS_MASK;
		if (remainder < area->write_offset[buf_idx]) {
			area->read_offset[buf_idx] = remainder;
		} else {
			area->read_offset[buf_idx] = 0;
			area->write_offset[buf_idx] = 0;
			if (remainder) {
				/* Read all of this buffer too. */
				buf_idx = (buf_idx + 1) & CRAS_SHM_BUFFERS_MASK;
			}
		}
		area->read_buf_idx = buf_idx;
	}
}

/* Read from the current buffer. This is similar to cras_shm_buffer_read(), but
 * it doesn't check for the case we may read from two buffers. */
static inline
void cras_shm_buffer_read_current(struct cras_audio_shm *shm, size_t frames)
{
	size_t buf_idx = shm->area->read_buf_idx & CRAS_SHM_BUFFERS_MASK;
	struct cras_audio_shm_area *area = shm->area;
	struct cras_audio_shm_config *config = &shm->config;

	area->read_offset[buf_idx] += frames * config->frame_bytes;
	if (area->read_offset[buf_idx] >= area->write_offset[buf_idx]) {
		area->read_offset[buf_idx] = 0;
		area->write_offset[buf_idx] = 0;
		buf_idx = (buf_idx + 1) & CRAS_SHM_BUFFERS_MASK;
		area->read_buf_idx = buf_idx;
	}
}

/* Sets the volume for the stream.  The volume level is a scaling factor that
 * will be applied to the stream before mixing. */
static inline
void cras_shm_set_volume_scaler(struct cras_audio_shm *shm, float volume_scaler)
{
	volume_scaler = MAX(volume_scaler, 0.0);
	shm->area->volume_scaler = MIN(volume_scaler, 1.0);
}

/* Returns the volume of the stream(0.0-1.0). */
static inline float cras_shm_get_volume_scaler(const struct cras_audio_shm *shm)
{
	return shm->area->volume_scaler;
}

/* Indicates that the stream should be muted/unmuted */
static inline void cras_shm_set_mute(struct cras_audio_shm *shm, size_t mute)
{
	shm->area->mute = !!mute;
}

/* Returns the mute state of the stream.  0 if not muted, non-zero if muted. */
static inline size_t cras_shm_get_mute(const struct cras_audio_shm *shm)
{
	return shm->area->mute;
}

/* Sets the size of a frame in bytes. */
static inline void cras_shm_set_frame_bytes(struct cras_audio_shm *shm,
					    unsigned frame_bytes)
{
	shm->config.frame_bytes = frame_bytes;
	if (shm->area)
		shm->area->config.frame_bytes = frame_bytes;
}

/* Returns the size of a frame in bytes. */
static inline unsigned cras_shm_frame_bytes(const struct cras_audio_shm *shm)
{
	return shm->config.frame_bytes;
}

/* Sets if a callback is pending with the client. */
static inline
void cras_shm_set_callback_pending(struct cras_audio_shm *shm, int pending)
{
	shm->area->callback_pending = !!pending;
}

/* Returns non-zero if a callback is pending for this shm region. */
static inline int cras_shm_callback_pending(const struct cras_audio_shm *shm)
{
	return shm->area->callback_pending;
}

/* Sets the used_size of the shm region.  This is the maximum number of bytes
 * that is exchanged each time a buffer is passed from client to server.
 */
static inline
void cras_shm_set_used_size(struct cras_audio_shm *shm, unsigned used_size)
{
	shm->config.used_size = used_size;
	if (shm->area)
		shm->area->config.used_size = used_size;
}

/* Returns the used size of the shm region in bytes. */
static inline unsigned cras_shm_used_size(const struct cras_audio_shm *shm)
{
	return shm->config.used_size;
}

/* Returns the used size of the shm region in frames. */
static inline unsigned cras_shm_used_frames(const struct cras_audio_shm *shm)
{
	return shm->config.used_size / shm->config.frame_bytes;
}

/* Returns the total size of the shared memory region. */
static inline unsigned cras_shm_total_size(const struct cras_audio_shm *shm)
{
	return cras_shm_used_size(shm) * CRAS_NUM_SHM_BUFFERS +
			sizeof(*shm->area);
}

/* Gets the counter of over-runs. */
static inline
unsigned cras_shm_num_overruns(const struct cras_audio_shm *shm)
{
	return shm->area->num_overruns;
}

/* Copy the config from the shm region to the local config.  Used by clients
 * when initially setting up the region.
 */
static inline void cras_shm_copy_shared_config(struct cras_audio_shm *shm)
{
	memcpy(&shm->config, &shm->area->config, sizeof(shm->config));
}

/* Open a read/write shared memory area with the given name.
 * Args:
 *    name - Name of the shared-memory area.
 *    size - Size of the shared-memory area.
 * Returns:
 *    >= 0 file descriptor value, or negative errno value on error.
 */
int cras_shm_open_rw (const char *name, size_t size);

/* Reopen an existing shared memory area read-only.
 * Args:
 *    name - Name of the shared-memory area.
 *    fd - Existing file descriptor.
 * Returns:
 *    >= 0 new file descriptor value, or negative errno value on error.
 */
int cras_shm_reopen_ro (const char *name, int fd);

/* Close and delete a shared memory area.
 * Args:
 *    name - Name of the shared-memory area.
 *    fd - Existing file descriptor.
 * Returns:
 *    >= 0 new file descriptor value, or negative errno value on error.
 */
void cras_shm_close_unlink (const char *name, int fd);

#endif /* CRAS_SHM_H_ */
