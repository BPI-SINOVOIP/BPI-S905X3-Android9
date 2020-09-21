/* Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * This example plays a file.  The filename is the only argument.  The file is
 * assumed to contain raw stereo 16-bit PCM data to be played at 48kHz.
 * usage:  cplay <filename>
 */

#include <cras_client.h>
#include <cras_helpers.h>
#include <stdio.h>
#include <stdint.h>

/* Used as a cookie for the playing stream. */
struct stream_data {
	int fd;
	unsigned int frame_bytes;
};

/* Run from callback thread. */
static int put_samples(struct cras_client *client,
		       cras_stream_id_t stream_id,
		       uint8_t *captured_samples,
		       uint8_t *playback_samples,
		       unsigned int frames,
		       const struct timespec *captured_time,
		       const struct timespec *playback_time,
		       void *user_arg)
{
	struct stream_data *data = (struct stream_data *)user_arg;
	int nread;

	nread = read(data->fd, playback_samples, frames * data->frame_bytes);
	if (nread <= 0)
		return EOF;

	return nread / data->frame_bytes;
}

/* Run from callback thread. */
static int stream_error(struct cras_client *client,
			cras_stream_id_t stream_id,
			int err,
			void *arg)
{
	printf("Stream error %d\n", err);
	exit(err);
	return 0;
}

int main(int argc, char **argv)
{
	struct cras_client *client;
	cras_stream_id_t stream_id;
	struct stream_data *data;
	int rc = 0;
	int fd;
	const unsigned int block_size = 4800;
	const unsigned int num_channels = 2;
	const unsigned int rate = 48000;
	const unsigned int flags = 0;

	if (argc < 2)
		printf("Usage: %s filename\n", argv[0]);

	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		perror("failed to open file");
		return -errno;
	}

	rc = cras_helper_create_connect(&client);
	if (rc < 0) {
		fprintf(stderr, "Couldn't create client.\n");
		close(fd);
		return rc;
	}

	data = malloc(sizeof(*data));
	data->fd = fd;
	data->frame_bytes = 4;

	rc = cras_helper_add_stream_simple(client, CRAS_STREAM_OUTPUT, data,
			put_samples, stream_error, SND_PCM_FORMAT_S16_LE, rate,
			num_channels, NO_DEVICE, &stream_id);
	if (rc < 0) {
		fprintf(stderr, "adding a stream %d\n", rc);
		goto destroy_exit;
	}

	/* At this point the stream has been added and audio callbacks will
	 * start to fire.  This app can now go off and do other things, but this
	 * example just loops forever. */
	while (1) {
		sleep(1);
	}

destroy_exit:
	cras_client_destroy(client);
	close(fd);
	free(data);
	return rc;
}
