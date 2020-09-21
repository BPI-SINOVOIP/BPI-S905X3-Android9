/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Alsa Helpers - Keeps the interface to alsa localized to this file.
 */

#ifndef _CRAS_ALSA_HELPERS_H
#define _CRAS_ALSA_HELPERS_H

#include <alsa/asoundlib.h>
#include <stdint.h>
#include <stdlib.h>

struct cras_audio_format;


/* Sets the channel layout from given format to the pcm handle.
 * Args:
 *    handle - Pointer to the opened pcm to set channel map to.
 *    fmt - The format containing the channel layout info.
 * Returns:
 *    0 if a matched channel map is set to HW, -1 otherwise.
 */
int cras_alsa_set_channel_map(snd_pcm_t *handle,
			      struct cras_audio_format *fmt);

/*  Gets the supported channel mapping of the pcm handle which matches
 *  the channel layout in the format.
 *  Args:
 *     handle - Pointer to the opened pcm to get channel map info.
 *     fmt - The format to fill channel layout into.
 *  Returns:
 *     0 if an exactly matched channel map is found, -1 otherwise.
 */
int cras_alsa_get_channel_map(snd_pcm_t *handle,
			      struct cras_audio_format *fmt);

/* Opens an alsa device, thin wrapper to snd_pcm_open.
 * Args:
 *    handle - Filled with a pointer to the opened pcm.
 *    dev - Path to the alsa device to test.
 *    stream - Alsa stream type, input or output.
 * Returns:
 *    See docs for snd_pcm_open.
 */
int cras_alsa_pcm_open(snd_pcm_t **handle, const char *dev,
		       snd_pcm_stream_t stream);

/* Closes an alsa device, thin wrapper to snd_pcm_close.
 * Args:
 *    handle - Filled with a pointer to the opened pcm.
 * Returns:
 *    See docs for snd_pcm_close.
 */
int cras_alsa_pcm_close(snd_pcm_t *handle);

/* Starts an alsa device, thin wrapper to snd_pcm_start.
 * Args:
 *    handle - Filled with a pointer to the opened pcm.
 * Returns:
 *    See docs for snd_pcm_start.
 */
int cras_alsa_pcm_start(snd_pcm_t *handle);

/* Drains an alsa device, thin wrapper to snd_pcm_drain.
 * Args:
 *    handle - Filled with a pointer to the opened pcm.
 * Returns:
 *    See docs for snd_pcm_drain.
 */
int cras_alsa_pcm_drain(snd_pcm_t *handle);

/* Forward/rewind appl_ptr so it becomes ahead of hw_ptr by fuzz samples.
 * After moving appl_ptr, device can play the new samples as quick as possible.
 *    avail = buffer_frames - appl_ptr + hw_ptr
 * => hw_ptr - appl_ptr = avail - buffer_frames.
 * The difference between hw_ptr and app_ptr can be inferred from snd_pcm_avail.
 * So the amount of frames to forward appl_ptr is
 * avail - buffer_frames + fuzz.
 * When hw_ptr is wrapped around boundary, this value may be negative. Use
 * snd_pcm_rewind to move appl_ptr backward.
 *
 * Case 1: avail - buffer_frames + fuzz > 0
 *
 * -------|----------|-----------------------------------
 *      app_ptr     hw_ptr
 *        |------------->| forward target
 *
 * Case 2: avail - buffer_frames + fuzz < 0
 *
 * -------|----------|-----------------------------------
 *      hw_ptr      app_ptr
 *           |<------| rewind target
 *
 * Args:
 *    handle - Filled with a pointer to the opened pcm.
 *    ahead - Number of frames appl_ptr should be ahead of hw_ptr.
 * Returns:
 *    0 on success. A negative error code on failure.
 */
int cras_alsa_resume_appl_ptr(snd_pcm_t *handle, snd_pcm_uframes_t ahead);

/* Probes properties of the alsa device.
 * Args:
 *    dev - Path to the alsa device to test.
 *    stream - Alsa stream type, input or output.
 *    rates - Pointer that will be set to the arrary of valid samples rates.
 *            Must be freed by the caller.
 *    channel_counts - Pointer that will be set to the array of valid channel
 *                     counts.  Must be freed by the caller.
 *    formats - Pointer that will be set to the arrary of valid PCM formats.
 *              Must be freed by the caller.
 * Returns:
 *   0 on success.  On failure an error code from alsa or -ENOMEM.
 */
int cras_alsa_fill_properties(const char *dev, snd_pcm_stream_t stream,
			      size_t **rates, size_t **channel_counts,
			      snd_pcm_format_t **formats);

/* Sets up the hwparams to alsa.
 * Args:
 *    handle - The open PCM to configure.
 *    format - The audio format desired for playback/capture.
 *    buffer_frames - Number of frames in the ALSA buffer.
 *    period_wakeup - Flag to determine if period_wakeup is required
 *                      0 - disable, 1 - enable
 *    dma_period_time - If non-zero, set the dma period time to this value
 *                      (in microseconds).
 * Returns:
 *    0 on success, negative error on failure.
 */
int cras_alsa_set_hwparams(snd_pcm_t *handle, struct cras_audio_format *format,
			   snd_pcm_uframes_t *buffer_frames, int period_wakeup,
			   unsigned int dma_period_time);

/* Sets up the swparams to alsa.
 * Args:
 *    handle - The open PCM to configure.
 *    enable_htimestamp - If non-zero, enable and configure hardware timestamps,
 *                        updated to reflect whether MONOTONIC RAW htimestamps
 *                        are supported by the kernel implementation.
 * Returns:
 *    0 on success, negative error on failure.
 */
int cras_alsa_set_swparams(snd_pcm_t *handle, int *enable_htimestamp);

/* Get the number of used frames in the alsa buffer.
 *
 * When underrun is not severe, this function masks the underrun situation
 * and set avail as 0. When underrun is severe, returns -EPIPE so caller
 * can handle it.
 * Args:
 *    handle[in] - The open PCM to configure.
 *    buf_size[in] - Number of frames in the ALSA buffer.
 *    severe_underrun_frames[in] - Number of frames as the threshold for severe
 *                                 underrun.
 *    dev_name[in] - Device name for logging.
 *    avail[out] - Filled with the number of frames available in the buffer.
 *    tstamp[out] - Filled with the hardware timestamp for the available frames.
 *                  This value is {0, 0} when the device hasn't actually started
 *                  reading or writing frames.
 *    underruns[in,out] - Pointer to the underrun counter updated if there was
 *                        an underrun.
 * Returns:
 *    0 on success, negative error on failure. -EPIPE if severe underrun
 *    happens.
 */
int cras_alsa_get_avail_frames(snd_pcm_t *handle, snd_pcm_uframes_t buf_size,
			       snd_pcm_uframes_t severe_underrun_frames,
			       const char *dev_name,
			       snd_pcm_uframes_t *avail,
			       struct timespec *tstamp,
			       unsigned int *underruns);

/* Get the current alsa delay, make sure it's no bigger than the buffer size.
 * Args:
 *    handle - The open PCM to configure.
 *    buf_size - Number of frames in the ALSA buffer.
 *    delay - Filled with the number of delay frames.
 * Returns:
 *    0 on success, negative error on failure.
 */
int cras_alsa_get_delay_frames(snd_pcm_t *handle, snd_pcm_uframes_t buf_size,
			       snd_pcm_sframes_t *delay);

/* Wrapper for snd_pcm_mmap_begin where only buffer is concerned.
 * Offset and frames from cras_alsa_mmap_begin are neglected.
 * Args:
 *    handle - The open PCM to configure.
 *    dst - Pointer set to the area for reading/writing the audio.
 *    underruns - counter to increment if an under-run occurs.
 * Returns:
 *    zero on success, negative error code for fatal errors.
 */
int cras_alsa_mmap_get_whole_buffer(snd_pcm_t *handle, uint8_t **dst,
				    unsigned int *underrun);

/* Wrapper for snd_pcm_mmap_begin
 * Args:
 *    handle - The open PCM to configure.
 *    format_bytes - Number of bytes in a single frame.
 *    dst - Pointer set to the area for reading/writing the audio.
 *    offset - Filled with the offset to pass back to commit.
 *    frames - Passed with the max number of frames to request. Filled with the
 *        max number to use.
 *    underruns - counter to increment if an under-run occurs.
 * Returns:
 *    zero on success, negative error code for fatal
 *    errors.
 */
int cras_alsa_mmap_begin(snd_pcm_t *handle, unsigned int format_bytes,
			 uint8_t **dst, snd_pcm_uframes_t *offset,
			 snd_pcm_uframes_t *frames, unsigned int *underruns);

/* Wrapper for snd_pcm_mmap_commit
 * Args:
 *    handle - The open PCM to configure.
 *    offset - offset from call to mmap_begin.
 *    frames - # of frames written/read.
 *    underruns - counter to increment if an under-run occurs.
 * Returns:
 *    zero on success, negative error code for fatal
 *    errors.
 */
int cras_alsa_mmap_commit(snd_pcm_t *handle, snd_pcm_uframes_t offset,
			  snd_pcm_uframes_t frames, unsigned int *underruns);

/* When the stream is suspended, due to a system suspend, loop until we can
 * resume it. Won't actually loop very much because the system will be
 * suspended.
 * Args:
 *    handle - The open PCM to configure.
 * Returns:
 *    zero on success, negative error code for fatal
 *    errors.
 */
int cras_alsa_attempt_resume(snd_pcm_t *handle);

#endif /* _CRAS_ALSA_HELPERS_H */
