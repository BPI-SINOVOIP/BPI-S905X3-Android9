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
#include <sys/param.h>
#include <stdio.h>
#include <stdint.h>

int main(int argc, char **argv)
{
	struct cras_client *client;
	cras_stream_id_t stream_id;
	int rc = 0;
	int fd;
	const unsigned int num_channels = 2;
	const unsigned int rate = 48000;
	const unsigned int flags = 0;
	uint8_t *buffer;
	int nread;

	if (argc < 2)
		printf("Usage: %s filename\n", argv[0]);

	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		perror("failed to open file");
		return -errno;
	}

	buffer = malloc(48000 * 4 * 5);

	nread = read(fd, buffer, 48000 * 4 * 5);
	if (nread <= 0) {
		free(buffer);
		close(fd);
		return nread;
	}

	rc = cras_helper_create_connect(&client);
	if (rc < 0) {
		fprintf(stderr, "Couldn't create client.\n");
		free(buffer);
		close(fd);
		return rc;
	}

	rc = cras_helper_play_buffer(client, buffer, nread / 4,
			SND_PCM_FORMAT_S16_LE, rate, num_channels,
			cras_client_get_first_dev_type_idx(
				client, CRAS_NODE_TYPE_INTERNAL_SPEAKER,
				CRAS_STREAM_OUTPUT));
	if (rc < 0) {
		fprintf(stderr, "playing a buffer %d\n", rc);
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
	free(buffer);
	close(fd);
	return rc;
}
