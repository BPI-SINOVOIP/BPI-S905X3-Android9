/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef _CRAS_HELPERS_H
#define _CRAS_HELPERS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Creates and connects a client to the running server asynchronously.
 *
 * When the connection has been established the connection_cb is executed
 * with the appropriate state. See cras_connection_status_cb_t for more
 * information.
 *
 * Args:
 *    client - Filled with a pointer to the new client.
 *    connection_cb - The connection status callback function.
 *    user_arg - Argument passed to the connection status callback.
 * Returns:
 *    0 on success, or a negative error code on failure (from errno.h).
 */
int cras_helper_create_connect_async(struct cras_client **client,
				     cras_connection_status_cb_t connection_cb,
				     void *user_arg);

/* Creates and connects a client to the running server.
 *
 * Waits forever (or interrupt) for the server to be available.
 *
 * Args:
 *    client - Filled with a pointer to the new client.
 * Returns:
 *    0 on success, or a negative error code on failure (from errno.h).
 */
int cras_helper_create_connect(struct cras_client **client);

/* Adds a stream with the given parameters, no flags and a buffer size of 2048
 * Args:
 *    client - The client to add the stream to (from cras_client_create).
 *    direction - playback(CRAS_STREAM_OUTPUT) or capture(CRAS_STREAM_INPUT) or
 *        loopback(CRAS_STREAM_POST_MIX_PRE_DSP).
 *    user_data - Pointer that will be passed to the callback.
 *    unified_cb - Called for streams that do simultaneous input/output.
 *    err_cb - Called when there is an error with the stream.
 *    format - The type of the samples, ex. S16_LE.
 *    frame_rate - Sample rate.
 *    num_channels - Number of channels in the stream, should be 1 or 2 when
 *        using this API, for > 2 channel streams see cras_client.h.
 *    dev_idx - Set this to a negative number to play to the default device, if
 *        positive it is the index of the device to pin the stream to.
 *    stream_id_out - On success will be filled with the new stream id.
 *        Guaranteed to be set before any callbacks are made.
 * Returns:
 *    0 on success, negative error code on failure (from errno.h).
 */
int cras_helper_add_stream_simple(struct cras_client *client,
				  enum CRAS_STREAM_DIRECTION direction,
				  void *user_data,
				  cras_unified_cb_t unified_cb,
				  cras_error_cb_t err_cb,
				  snd_pcm_format_t format,
				  unsigned int frame_rate,
				  unsigned int num_channels,
				  int dev_idx,
				  cras_stream_id_t *stream_id_out);

/* Plays the given buffer at a default latency.
 * Args:
 *    client - The client to add the stream to (from cras_client_create).
 *    buffer - The audio samples.
 *    num_frames - The size of the buffer in number of samples.
 *    format - The type of the samples, ex. S16_LE.
 *    frame_rate - Sample rate.
 *    num_channels - Number of channels in the stream.
 *    dev_idx - Set this to a negative number to play to the default device, if
 *        positive it is the index of the device to pin the stream to.
 * Returns:
 *    0 on success, negative error code on failure (from errno.h).
 */
int cras_helper_play_buffer(struct cras_client *client,
			    const void *buffer,
			    unsigned int num_frames,
			    snd_pcm_format_t format,
			    unsigned int frame_rate,
			    unsigned int num_channels,
			    int dev_idx);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _CRAS_HELPERS_H */
