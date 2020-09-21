/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

#include "cras_client.h"
#include "cras_types.h"
#include "cras_util.h"
#include "cras_version.h"

#define PLAYBACK_BUFFERED_TIME_IN_NS (5000000)

#define BUF_SIZE 32768

static int keep_looping = 1;
static int pipefd[2];
struct cras_audio_format *aud_format;

static int terminate_stream_loop(void)
{
	keep_looping = 0;
	return write(pipefd[1], "1", 1);
}

static size_t get_block_size(uint64_t buffer_time_in_ns, size_t rate)
{
	static struct timespec t;

	t.tv_nsec = buffer_time_in_ns;
	t.tv_sec = 0;
	return (size_t)cras_time_to_frames(&t, rate);
}

/* Run from callback thread. */
static int got_samples(struct cras_client *client,
		       cras_stream_id_t stream_id,
		       uint8_t *captured_samples,
		       uint8_t *playback_samples,
		       unsigned int frames,
		       const struct timespec *captured_time,
		       const struct timespec *playback_time,
		       void *user_arg)
{
	int *fd = (int *)user_arg;
	int ret;
	int write_size;
	int frame_bytes;

	frame_bytes = cras_client_format_bytes_per_frame(aud_format);
	write_size = frames * frame_bytes;
	ret = write(*fd, captured_samples, write_size);
	if (ret != write_size)
		printf("Error writing file\n");
	return frames;
}

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
	uint32_t frame_bytes = cras_client_format_bytes_per_frame(aud_format);
	int fd = *(int *)user_arg;
	uint8_t buff[BUF_SIZE];
	int nread;

	nread = read(fd, buff, MIN(frames * frame_bytes, BUF_SIZE));
	if (nread <= 0) {
		terminate_stream_loop();
		return nread;
	}

	memcpy(playback_samples, buff, nread);
	return nread / frame_bytes;
}

static int stream_error(struct cras_client *client,
			cras_stream_id_t stream_id,
			int err,
			void *arg)
{
	printf("Stream error %d\n", err);
	terminate_stream_loop();
	return 0;
}

static int start_stream(struct cras_client *client,
			cras_stream_id_t *stream_id,
			struct cras_stream_params *params,
			float stream_volume)
{
	int rc;

	rc = cras_client_add_stream(client, stream_id, params);
	if (rc < 0) {
		fprintf(stderr, "adding a stream %d\n", rc);
		return rc;
	}
	return cras_client_set_stream_volume(client,
					     *stream_id,
					     stream_volume);
}

static int run_file_io_stream(struct cras_client *client,
			      int fd,
			      int loop_fd,
			      enum CRAS_STREAM_DIRECTION direction,
			      size_t block_size,
			      size_t rate,
			      size_t num_channels)
{
	struct cras_stream_params *params;
	cras_stream_id_t stream_id = 0;
	int stream_playing = 0;
	int *pfd = malloc(sizeof(*pfd));
	*pfd = fd;
	float volume_scaler = 1.0;

	if (pipe(pipefd) == -1) {
		perror("failed to open pipe");
		return -errno;
	}
	aud_format = cras_audio_format_create(SND_PCM_FORMAT_S16_LE, rate,
					      num_channels);
	if (aud_format == NULL)
		return -ENOMEM;

	params = cras_client_unified_params_create(direction,
						   block_size,
						   0,
						   0,
						   pfd,
						   got_samples,
						   stream_error,
						   aud_format);
	if (params == NULL)
		return -ENOMEM;

	cras_client_run_thread(client);
	stream_playing =
		start_stream(client, &stream_id, params, volume_scaler) == 0;
	if (!stream_playing)
		return -EINVAL;

	int *pfd1 = malloc(sizeof(*pfd1));
	*pfd1 = loop_fd;
	struct cras_stream_params *loop_params;
	cras_stream_id_t loop_stream_id = 0;

	direction = CRAS_STREAM_OUTPUT;

	loop_params = cras_client_unified_params_create(direction,
							block_size,
							0,
							0,
							pfd1,
							put_samples,
							stream_error,
							aud_format);
	stream_playing =
		start_stream(client, &loop_stream_id,
			     loop_params, volume_scaler) == 0;
	if (!stream_playing)
		return -EINVAL;

	fd_set poll_set;

	FD_ZERO(&poll_set);
	FD_SET(pipefd[0], &poll_set);
	pselect(pipefd[0] + 1, &poll_set, NULL, NULL, NULL, NULL);
	cras_client_stop(client);
	cras_audio_format_destroy(aud_format);
	cras_client_stream_params_destroy(params);
	free(pfd);

	close(pipefd[0]);
	close(pipefd[1]);

	return 0;
}

static struct option long_options[] = {
	{"help", no_argument, 0, 'h'},
	{"rate", required_argument, 0, 'r'},
	{0, 0, 0, 0}
};

static void show_usage(void)
{
	printf("--help - shows this message and exits\n");
	printf("--rate <N> - desired sample rate\n\n");
	printf("Running cras_router will run a loop through ");
	printf("from the currently set input to the currently set output.\n");
	printf("Use cras_test_client --dump_s to see all avaiable nodes and");
	printf(" cras_test_client --set_input/output to set a node.\n");
}

int main(int argc, char **argv)
{
	struct cras_client *client;
	size_t rate = 44100;
	size_t num_channels = 2;
	size_t block_size;
	int rc = 0;
	int c, option_index;

	option_index = 0;

	rc = cras_client_create(&client);
	if (rc < 0) {
		fprintf(stderr, "Couldn't create client.\n");
		return rc;
	}

	rc = cras_client_connect(client);
	if (rc) {
		fprintf(stderr, "Couldn't connect to server.\n");
		goto destroy_exit;
	}

	while (1) {
		c = getopt_long(argc, argv, "hr:",
				long_options, &option_index);
		if (c == -1)
			break;
		switch (c) {
		case 'h':
			show_usage();
			goto destroy_exit;
		case 'r':
			rate = atoi(optarg);
			break;
		default:
		break;
		}
	}

	block_size = get_block_size(PLAYBACK_BUFFERED_TIME_IN_NS, rate);

	/* Run loopthrough */
	int pfd[2];

	rc = pipe(pfd);
	if (rc < 0) {
		fprintf(stderr, "Couldn't create loopthrough pipe.\n");
		return rc;
	}
	run_file_io_stream(client, pfd[1], pfd[0], CRAS_STREAM_INPUT,
			   block_size, rate, num_channels);

destroy_exit:
	cras_client_destroy(client);
	return rc;
}
