/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <sys/ioctl.h>
#include <pthread.h>
#include <sys/param.h>
#include <syslog.h>

#include "audio_thread.h"
#include "byte_buffer.h"
#include "cras_audio_area.h"
#include "cras_config.h"
#include "cras_iodev.h"
#include "cras_iodev_list.h"
#include "cras_types.h"
#include "cras_util.h"
#include "test_iodev.h"
#include "utlist.h"

#define TEST_BUFFER_SIZE (16 * 1024)

static size_t test_supported_rates[] = {
	16000, 0
};

static size_t test_supported_channel_counts[] = {
	1, 0
};

static snd_pcm_format_t test_supported_formats[] = {
	SND_PCM_FORMAT_S16_LE,
	0
};

struct test_iodev {
	struct cras_iodev base;
	int fd;
	struct byte_buffer *audbuff;
	unsigned int fmt_bytes;
};

/*
 * iodev callbacks.
 */

static int frames_queued(const struct cras_iodev *iodev,
			 struct timespec *tstamp)
{
	struct test_iodev *testio = (struct test_iodev *)iodev;
	int available;

	if (testio->fd < 0)
		return 0;
	ioctl(testio->fd, FIONREAD, &available);
	clock_gettime(CLOCK_MONOTONIC_RAW, tstamp);
	return available / testio->fmt_bytes;
}

static int delay_frames(const struct cras_iodev *iodev)
{
	return 0;
}

static int close_dev(struct cras_iodev *iodev)
{
	struct test_iodev *testio = (struct test_iodev *)iodev;

	byte_buffer_destroy(testio->audbuff);
	testio->audbuff = NULL;
	cras_iodev_free_audio_area(iodev);
	return 0;
}

static int open_dev(struct cras_iodev *iodev)
{
	struct test_iodev *testio = (struct test_iodev *)iodev;

	if (iodev->format == NULL)
		return -EINVAL;

	cras_iodev_init_audio_area(iodev, iodev->format->num_channels);
	testio->fmt_bytes = cras_get_format_bytes(iodev->format);
	testio->audbuff = byte_buffer_create(TEST_BUFFER_SIZE *
						testio->fmt_bytes);

	return 0;
}

static int get_buffer(struct cras_iodev *iodev,
		      struct cras_audio_area **area,
		      unsigned *frames)
{
	struct test_iodev *testio = (struct test_iodev *)iodev;
	unsigned int readable;
	uint8_t *buff;

	buff = buf_read_pointer_size(testio->audbuff, &readable);
	*frames = MIN(*frames, readable);

	iodev->area->frames = *frames;
	cras_audio_area_config_buf_pointers(iodev->area, iodev->format, buff);
	*area = iodev->area;
	return 0;
}

static int put_buffer(struct cras_iodev *iodev, unsigned frames)
{
	struct test_iodev *testio = (struct test_iodev *)iodev;

	/* Input */
	buf_increment_read(testio->audbuff, frames * testio->fmt_bytes);

	return 0;
}

static int get_buffer_fd_read(struct cras_iodev *iodev,
			      struct cras_audio_area **area,
			      unsigned *frames)
{
	struct test_iodev *testio = (struct test_iodev *)iodev;
	int nread;
	uint8_t *write_ptr;
	unsigned int avail;

	if (testio->fd < 0) {
		*frames = 0;
		return 0;
	}

	write_ptr = buf_write_pointer_size(testio->audbuff, &avail);
	avail = MIN(avail, *frames * testio->fmt_bytes);
	nread = read(testio->fd, write_ptr, avail);
	if (nread <= 0) {
		*frames = 0;
		audio_thread_rm_callback(testio->fd);
		close(testio->fd);
		testio->fd = -1;
		return 0;
	}
	buf_increment_write(testio->audbuff, nread);
	*frames = nread / testio->fmt_bytes;
	iodev->area->frames = *frames;
	cras_audio_area_config_buf_pointers(iodev->area, iodev->format,
					    write_ptr);
	*area = iodev->area;
	return nread;
}

static void update_active_node(struct cras_iodev *iodev, unsigned node_idx,
			       unsigned dev_enabled)
{
}

static void play_file_as_hotword(struct test_iodev *testio, const char *path)
{
	if (testio->fd >= 0) {
		/* Remove audio thread callback from main thread. */
		audio_thread_rm_callback_sync(
				cras_iodev_list_get_audio_thread(),
				testio->fd);
		close(testio->fd);
	}

	testio->fd = open(path, O_RDONLY);
	buf_reset(testio->audbuff);
}

/*
 * Exported Interface.
 */

struct cras_iodev *test_iodev_create(enum CRAS_STREAM_DIRECTION direction,
				     enum TEST_IODEV_TYPE type)
{
	struct test_iodev *testio;
	struct cras_iodev *iodev;
	struct cras_ionode *node;

	if (direction != CRAS_STREAM_INPUT || type != TEST_IODEV_HOTWORD)
		return NULL;

	testio = calloc(1, sizeof(*testio));
	if (testio == NULL)
		return NULL;
	iodev = &testio->base;
	iodev->direction = direction;
	testio->fd = -1;

	iodev->supported_rates = test_supported_rates;
	iodev->supported_channel_counts = test_supported_channel_counts;
	iodev->supported_formats = test_supported_formats;
	iodev->buffer_size = TEST_BUFFER_SIZE;

	iodev->open_dev = open_dev;
	iodev->close_dev = close_dev;
	iodev->frames_queued = frames_queued;
	iodev->delay_frames = delay_frames;
	if (type == TEST_IODEV_HOTWORD)
		iodev->get_buffer = get_buffer_fd_read;
	else
		iodev->get_buffer = get_buffer;
	iodev->put_buffer = put_buffer;
	iodev->update_active_node = update_active_node;

	/* Create a dummy ionode */
	node = (struct cras_ionode *)calloc(1, sizeof(*node));
	node->dev = iodev;
	node->plugged = 1;
	if (type == TEST_IODEV_HOTWORD)
		node->type = CRAS_NODE_TYPE_HOTWORD;
	else
		node->type = CRAS_NODE_TYPE_UNKNOWN;
	node->volume = 100;
	node->software_volume_needed = 0;
	node->max_software_gain = 0;
	strcpy(node->name, "(default)");
	cras_iodev_add_node(iodev, node);
	cras_iodev_set_active_node(iodev, node);

	/* Finally add it to the appropriate iodev list. */
	snprintf(iodev->info.name, ARRAY_SIZE(iodev->info.name), "Tester");
	iodev->info.name[ARRAY_SIZE(iodev->info.name) - 1] = '\0';
	cras_iodev_list_add_input(iodev);

	return iodev;
}

void test_iodev_destroy(struct cras_iodev *iodev)
{
	struct test_iodev *testio = (struct test_iodev *)iodev;

	cras_iodev_list_rm_input(iodev);
	free(iodev->active_node);
	cras_iodev_free_resources(iodev);
	free(testio);
}

unsigned int test_iodev_add_samples(struct test_iodev *testio,
				    uint8_t *samples,
				    unsigned int count)
{
	unsigned int avail;
	uint8_t *write_ptr;

	write_ptr = buf_write_pointer_size(testio->audbuff, &avail);
	count = MIN(count, avail);
	memcpy(write_ptr, samples, count * testio->fmt_bytes);
	buf_increment_write(testio->audbuff, count * testio->fmt_bytes);
	return count;
}

void test_iodev_command(struct cras_iodev *iodev,
			enum CRAS_TEST_IODEV_CMD command,
			unsigned int data_len,
			const uint8_t *data)
{
	struct test_iodev *testio = (struct test_iodev *)iodev;

	if (!cras_iodev_is_open(iodev))
		return;

	switch (command) {
	case TEST_IODEV_CMD_HOTWORD_TRIGGER:
		play_file_as_hotword(testio, (char *)data);
		break;
	default:
		break;
	}
}
