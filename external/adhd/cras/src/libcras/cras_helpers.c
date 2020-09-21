/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <stdint.h>
#include <sys/param.h>

#include "cras_client.h"
#include "cras_util.h"

struct buffer_data {
	const uint8_t *buffer;
	unsigned int offset;
	unsigned int frame_bytes;
	unsigned int len;
};

static int play_buffer_callback(struct cras_client *client,
				cras_stream_id_t stream_id,
				uint8_t *captured_samples,
				uint8_t *playback_samples,
				unsigned int frames,
				const struct timespec *captured_time,
				const struct timespec *playback_time,
				void *user_arg)
{
	struct buffer_data *data = (struct buffer_data *)user_arg;
	int to_copy = data->len - data->offset;

	if (to_copy <= 0) {
		free(user_arg);
		return EOF;
	}

	to_copy = MIN(to_copy, frames * data->frame_bytes);

	memcpy(playback_samples, data->buffer + data->offset, to_copy);

	data->offset += to_copy;

	return to_copy / data->frame_bytes;
}

static int play_buffer_error(struct cras_client *client,
			     cras_stream_id_t stream_id,
			     int error,
			     void *user_arg)
{
	free(user_arg);
	return 0;
}

int cras_helper_create_connect_async(struct cras_client **client,
				     cras_connection_status_cb_t connection_cb,
				     void *user_arg)
{
	int rc;

	rc = cras_client_create(client);
	if (rc < 0)
		return rc;

	cras_client_set_connection_status_cb(*client, connection_cb, user_arg);

	rc = cras_client_run_thread(*client);
	if (rc < 0)
		goto client_start_error;

	rc = cras_client_connect_async(*client);
	if (rc < 0)
		goto client_start_error;

	return 0;

client_start_error:
	cras_client_destroy(*client);
	return rc;
}

int cras_helper_create_connect(struct cras_client **client)
{
	int rc;

	rc = cras_client_create(client);
	if (rc < 0)
		return rc;

	rc = cras_client_connect(*client);
	if (rc < 0)
		goto client_start_error;

	rc = cras_client_run_thread(*client);
	if (rc < 0)
		goto client_start_error;

	rc = cras_client_connected_wait(*client);
	if (rc < 0)
		goto client_start_error;

	return 0;

client_start_error:
	cras_client_destroy(*client);
	return rc;
}

int cras_helper_add_stream_simple(struct cras_client *client,
				  enum CRAS_STREAM_DIRECTION direction,
				  void *user_data,
				  cras_unified_cb_t unified_cb,
				  cras_error_cb_t err_cb,
				  snd_pcm_format_t format,
				  unsigned int frame_rate,
				  unsigned int num_channels,
				  int dev_idx,
				  cras_stream_id_t *stream_id_out)
{
	struct cras_audio_format *aud_format;
	struct cras_stream_params *params;
	int rc;

	aud_format = cras_audio_format_create(format, frame_rate, num_channels);
	if (!aud_format)
		return -ENOMEM;

	params = cras_client_unified_params_create(CRAS_STREAM_OUTPUT,
			2048, CRAS_STREAM_TYPE_DEFAULT, 0, user_data,
			unified_cb, err_cb, aud_format);
	if (!params) {
		rc = -ENOMEM;
		goto done_add_stream;
	}

	if (dev_idx < 0)
		dev_idx = NO_DEVICE;
	rc = cras_client_add_pinned_stream(client, dev_idx, stream_id_out,
					   params);

done_add_stream:
	cras_audio_format_destroy(aud_format);
	cras_client_stream_params_destroy(params);
	return rc;
}

int cras_helper_play_buffer(struct cras_client *client,
			    const void *buffer,
			    unsigned int frames,
			    snd_pcm_format_t format,
			    unsigned int frame_rate,
			    unsigned int num_channels,
			    int dev_idx)
{
	struct buffer_data *data;
	cras_stream_id_t stream_id;

	data = malloc(sizeof(*data));

	data->buffer = buffer;
	data->frame_bytes = num_channels * PCM_FORMAT_WIDTH(format) / 8;
	data->offset = 0;
	data->len = frames * data->frame_bytes;

	return cras_helper_add_stream_simple(client, CRAS_STREAM_OUTPUT, data,
			play_buffer_callback, play_buffer_error, format,
			frame_rate, num_channels, dev_idx, &stream_id);
}
