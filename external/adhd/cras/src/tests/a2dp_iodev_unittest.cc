// Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <stdint.h>
#include <gtest/gtest.h>

extern "C" {

#include "a2dp-codecs.h"
#include "cras_audio_area.h"
#include "audio_thread.h"
#include "audio_thread_log.h"
#include "cras_bt_transport.h"
#include "cras_iodev.h"

#include "cras_a2dp_iodev.h"
}

#define FAKE_OBJECT_PATH "/fake/obj/path"

#define MAX_A2DP_ENCODE_CALLS 8
#define MAX_A2DP_WRITE_CALLS 4

static struct cras_bt_transport *fake_transport;
static cras_audio_format format;
static size_t cras_bt_device_append_iodev_called;
static size_t cras_bt_device_rm_iodev_called;
static size_t cras_iodev_add_node_called;
static size_t cras_iodev_rm_node_called;
static size_t cras_iodev_set_active_node_called;
static size_t cras_bt_transport_acquire_called;
static size_t cras_bt_transport_configuration_called;
static size_t cras_bt_transport_release_called;
static size_t init_a2dp_called;
static int init_a2dp_return_val;
static size_t destroy_a2dp_called;
static size_t drain_a2dp_called;
static size_t a2dp_block_size_called;
static size_t a2dp_queued_frames_val;
static size_t cras_iodev_free_format_called;
static size_t cras_iodev_free_resources_called;
static int pcm_buf_size_val[MAX_A2DP_ENCODE_CALLS];
static unsigned int a2dp_encode_processed_bytes_val[MAX_A2DP_ENCODE_CALLS];
static unsigned int a2dp_encode_index;
static int a2dp_write_return_val[MAX_A2DP_WRITE_CALLS];
static unsigned int a2dp_write_index;
static cras_audio_area *dummy_audio_area;
static thread_callback write_callback;
static void *write_callback_data;
static const char *fake_device_name = "fake device name";
static const char *cras_bt_device_name_ret;
static unsigned int cras_bt_transport_write_mtu_ret;

void ResetStubData() {
  cras_bt_device_append_iodev_called = 0;
  cras_bt_device_rm_iodev_called = 0;
  cras_iodev_add_node_called = 0;
  cras_iodev_rm_node_called = 0;
  cras_iodev_set_active_node_called = 0;
  cras_bt_transport_acquire_called = 0;
  cras_bt_transport_configuration_called = 0;
  cras_bt_transport_release_called = 0;
  init_a2dp_called = 0;
  init_a2dp_return_val = 0;
  destroy_a2dp_called = 0;
  drain_a2dp_called = 0;
  a2dp_block_size_called = 0;
  a2dp_queued_frames_val = 0;
  cras_iodev_free_format_called = 0;
  cras_iodev_free_resources_called = 0;
  memset(a2dp_encode_processed_bytes_val, 0,
         sizeof(a2dp_encode_processed_bytes_val));
  a2dp_encode_index = 0;
  a2dp_write_index = 0;
  cras_bt_transport_write_mtu_ret = 800;

  fake_transport = reinterpret_cast<struct cras_bt_transport *>(0x123);

  if (!dummy_audio_area) {
    dummy_audio_area = (cras_audio_area*)calloc(1,
        sizeof(*dummy_audio_area) + sizeof(cras_channel_area) * 2);
  }

  write_callback = NULL;
}

int iodev_set_format(struct cras_iodev *iodev,
                     struct cras_audio_format *fmt)
{
  fmt->format = SND_PCM_FORMAT_S16_LE;
  fmt->num_channels = 2;
  fmt->frame_rate = 44100;
  iodev->format = fmt;
  return 0;
}

namespace {

static struct timespec time_now;

TEST(A2dpIoInit, InitializeA2dpIodev) {
  struct cras_iodev *iodev;

  atlog = (audio_thread_event_log *)calloc(1, sizeof(audio_thread_event_log));

  ResetStubData();

  cras_bt_device_name_ret = NULL;
  iodev = a2dp_iodev_create(fake_transport);

  ASSERT_NE(iodev, (void *)NULL);
  ASSERT_EQ(iodev->direction, CRAS_STREAM_OUTPUT);
  ASSERT_EQ(1, cras_bt_transport_configuration_called);
  ASSERT_EQ(1, init_a2dp_called);
  ASSERT_EQ(1, cras_bt_device_append_iodev_called);
  ASSERT_EQ(1, cras_iodev_add_node_called);
  ASSERT_EQ(1, cras_iodev_set_active_node_called);

  /* Assert iodev name matches the object path when bt device doesn't
   * have its readable name populated. */
  ASSERT_STREQ(FAKE_OBJECT_PATH, iodev->info.name);

  a2dp_iodev_destroy(iodev);

  ASSERT_EQ(1, cras_bt_device_rm_iodev_called);
  ASSERT_EQ(1, cras_iodev_rm_node_called);
  ASSERT_EQ(1, destroy_a2dp_called);
  ASSERT_EQ(1, cras_iodev_free_resources_called);

  cras_bt_device_name_ret = fake_device_name;
  /* Assert iodev name matches the bt device's name */
  iodev = a2dp_iodev_create(fake_transport);
  ASSERT_STREQ(fake_device_name, iodev->info.name);

  a2dp_iodev_destroy(iodev);
}

TEST(A2dpIoInit, InitializeFail) {
  struct cras_iodev *iodev;

  ResetStubData();

  init_a2dp_return_val = -1;
  iodev = a2dp_iodev_create(fake_transport);

  ASSERT_EQ(iodev, (void *)NULL);
  ASSERT_EQ(1, cras_bt_transport_configuration_called);
  ASSERT_EQ(1, init_a2dp_called);
  ASSERT_EQ(0, cras_bt_device_append_iodev_called);
  ASSERT_EQ(0, cras_iodev_add_node_called);
  ASSERT_EQ(0, cras_iodev_set_active_node_called);
  ASSERT_EQ(0, cras_iodev_rm_node_called);
}

TEST(A2dpIoInit, OpenIodev) {
  struct cras_iodev *iodev;

  ResetStubData();
  iodev = a2dp_iodev_create(fake_transport);

  iodev_set_format(iodev, &format);
  iodev->open_dev(iodev);

  ASSERT_EQ(1, cras_bt_transport_acquire_called);

  iodev->close_dev(iodev);
  ASSERT_EQ(1, cras_bt_transport_release_called);
  ASSERT_EQ(1, drain_a2dp_called);
  ASSERT_EQ(1, cras_iodev_free_format_called);

  a2dp_iodev_destroy(iodev);
}

TEST(A2dpIoInit, GetPutBuffer) {
  struct cras_iodev *iodev;
  struct cras_audio_area *area1, *area2, *area3;
  uint8_t *area1_buf;
  unsigned frames;

  ResetStubData();
  iodev = a2dp_iodev_create(fake_transport);

  iodev_set_format(iodev, &format);
  iodev->open_dev(iodev);
  ASSERT_NE(write_callback, (void *)NULL);

  frames = 256;
  iodev->get_buffer(iodev, &area1, &frames);
  ASSERT_EQ(256, frames);
  ASSERT_EQ(256, area1->frames);
  area1_buf = area1->channels[0].buf;

  /* Test 100 frames(400 bytes) put and all processed. */
  a2dp_encode_processed_bytes_val[0] = 4096 * 4;
  a2dp_encode_processed_bytes_val[1] = 400;
  a2dp_write_index = 0;
  a2dp_write_return_val[0] = -EAGAIN;
  a2dp_write_return_val[1] = 400;
  iodev->put_buffer(iodev, 100);
  write_callback(write_callback_data);
  // Start with 4k frames.
  EXPECT_EQ(4096, pcm_buf_size_val[0]);
  EXPECT_EQ(400, pcm_buf_size_val[1]);

  iodev->get_buffer(iodev, &area2, &frames);
  ASSERT_EQ(256, frames);
  ASSERT_EQ(256, area2->frames);

  /* Assert buf2 points to the same position as buf1 */
  ASSERT_EQ(400, area2->channels[0].buf - area1_buf);

  /* Test 100 frames(400 bytes) put, only 360 bytes processed,
   * 40 bytes left in pcm buffer.
   */
  a2dp_encode_index = 0;
  a2dp_encode_processed_bytes_val[0] = 360;
  a2dp_encode_processed_bytes_val[1] = 0;
  a2dp_write_index = 0;
  a2dp_write_return_val[0] = 360;
  a2dp_write_return_val[1] = 0;
  iodev->put_buffer(iodev, 100);
  write_callback(write_callback_data);
  EXPECT_EQ(400, pcm_buf_size_val[0]);
  ASSERT_EQ(40, pcm_buf_size_val[1]);

  iodev->get_buffer(iodev, &area3, &frames);

  /* Existing buffer not completed processed, assert new buffer starts from
   * current write pointer.
   */
  ASSERT_EQ(256, frames);
  EXPECT_EQ(800, area3->channels[0].buf - area1_buf);

  a2dp_iodev_destroy(iodev);
}

TEST(A2dpIoInif, FramesQueued) {
  struct cras_iodev *iodev;
  struct cras_audio_area *area;
  struct timespec tstamp;
  unsigned frames;

  ResetStubData();
  iodev = a2dp_iodev_create(fake_transport);

  iodev_set_format(iodev, &format);
  time_now.tv_sec = 0;
  time_now.tv_nsec = 0;
  iodev->open_dev(iodev);
  ASSERT_NE(write_callback, (void *)NULL);

  frames = 256;
  iodev->get_buffer(iodev, &area, &frames);
  ASSERT_EQ(256, frames);
  ASSERT_EQ(256, area->frames);

  /* Put 100 frames, proccessed 400 bytes to a2dp buffer.
   * Assume 200 bytes written out, queued 50 frames in a2dp buffer.
   */
  a2dp_encode_processed_bytes_val[0] = 400;
  a2dp_encode_processed_bytes_val[1] = 0;
  a2dp_write_return_val[0] = 200;
  a2dp_write_return_val[1] = -EAGAIN;
  a2dp_queued_frames_val = 50;
  time_now.tv_sec = 0;
  time_now.tv_nsec = 1000000;
  iodev->put_buffer(iodev, 300);
  write_callback(write_callback_data);
  EXPECT_EQ(350, iodev->frames_queued(iodev, &tstamp));
  EXPECT_EQ(tstamp.tv_sec, time_now.tv_sec);
  EXPECT_EQ(tstamp.tv_nsec, time_now.tv_nsec);

  /* After writing another 200 frames, check for correct buffer level. */
  time_now.tv_sec = 0;
  time_now.tv_nsec = 2000000;
  a2dp_encode_index = 0;
  a2dp_write_index = 0;
  a2dp_encode_processed_bytes_val[0] = 800;
  write_callback(write_callback_data);
  /* 1000000 nsec has passed, estimated queued frames adjusted by 44 */
  EXPECT_EQ(256, iodev->frames_queued(iodev, &tstamp));
  EXPECT_EQ(1200, pcm_buf_size_val[0]);
  EXPECT_EQ(400, pcm_buf_size_val[1]);
  EXPECT_EQ(tstamp.tv_sec, time_now.tv_sec);
  EXPECT_EQ(tstamp.tv_nsec, time_now.tv_nsec);

  /* Queued frames and new put buffer are all written */
  a2dp_encode_processed_bytes_val[0] = 400;
  a2dp_encode_processed_bytes_val[1] = 0;
  a2dp_encode_index = 0;
  a2dp_write_return_val[0] = 400;
  a2dp_write_return_val[1] = -EAGAIN;
  a2dp_write_index = 0;

  /* Add wnother 200 samples, get back to the original level. */
  time_now.tv_sec = 0;
  time_now.tv_nsec = 50000000;
  a2dp_encode_processed_bytes_val[0] = 600;
  iodev->put_buffer(iodev, 200);
  EXPECT_EQ(1200, pcm_buf_size_val[0]);
  EXPECT_EQ(200, iodev->frames_queued(iodev, &tstamp));
  EXPECT_EQ(tstamp.tv_sec, time_now.tv_sec);
  EXPECT_EQ(tstamp.tv_nsec, time_now.tv_nsec);
}

TEST(A2dpIo, FlushAtLowBufferLevel) {
  struct cras_iodev *iodev;
  struct cras_audio_area *area;
  struct timespec tstamp;
  unsigned frames;

  ResetStubData();
  iodev = a2dp_iodev_create(fake_transport);

  iodev_set_format(iodev, &format);
  time_now.tv_sec = 0;
  time_now.tv_nsec = 0;
  iodev->open_dev(iodev);
  ASSERT_NE(write_callback, (void *)NULL);

  ASSERT_EQ(iodev->min_buffer_level, 400);

  frames = 700;
  iodev->get_buffer(iodev, &area, &frames);
  ASSERT_EQ(700, frames);
  ASSERT_EQ(700, area->frames);

  /* Fake 111 frames in pre-fill*/
  a2dp_encode_processed_bytes_val[0] = 111;
  a2dp_write_return_val[0] = -EAGAIN;

  /* First call to a2dp_encode() processed 800 bytes. */
  a2dp_encode_processed_bytes_val[1] = 800;
  a2dp_encode_processed_bytes_val[2] = 0;
  a2dp_write_return_val[1] = 200;

  /* put_buffer shouldn't trigger the 2nd call to a2dp_encode() because
   * buffer is low. Fake some data to make sure this test case will fail
   * when a2dp_encode() called twice.
   */
  a2dp_encode_processed_bytes_val[3] = 800;
  a2dp_encode_processed_bytes_val[4] = 0;
  a2dp_write_return_val[2] = -EAGAIN;

  time_now.tv_nsec = 10000000;
  iodev->put_buffer(iodev, 700);

  time_now.tv_nsec = 20000000;
  EXPECT_EQ(500, iodev->frames_queued(iodev, &tstamp));
  EXPECT_EQ(tstamp.tv_sec, time_now.tv_sec);
  EXPECT_EQ(tstamp.tv_nsec, time_now.tv_nsec);
}

} // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

extern "C" {

int cras_bt_transport_configuration(const struct cras_bt_transport *transport,
                                    void *configuration, int len)
{
  cras_bt_transport_configuration_called++;
  return 0;
}

int cras_bt_transport_acquire(struct cras_bt_transport *transport)
{
  cras_bt_transport_acquire_called++;
  return 0;
}

int cras_bt_transport_release(struct cras_bt_transport *transport,
    unsigned int blocking)
{
  cras_bt_transport_release_called++;
  return 0;
}

int cras_bt_transport_fd(const struct cras_bt_transport *transport)
{
  return 0;
}

const char *cras_bt_transport_object_path(
		const struct cras_bt_transport *transport)
{
  return FAKE_OBJECT_PATH;
}

uint16_t cras_bt_transport_write_mtu(const struct cras_bt_transport *transport)
{
  return cras_bt_transport_write_mtu_ret;
}

int cras_bt_transport_set_volume(struct cras_bt_transport *transport,
    uint16_t volume)
{
  return 0;
}

void cras_iodev_free_format(struct cras_iodev *iodev)
{
  cras_iodev_free_format_called++;
}

void cras_iodev_free_resources(struct cras_iodev *iodev)
{
  cras_iodev_free_resources_called++;
}

// Cras iodev
void cras_iodev_add_node(struct cras_iodev *iodev, struct cras_ionode *node)
{
  cras_iodev_add_node_called++;
  iodev->nodes = node;
}

void cras_iodev_rm_node(struct cras_iodev *iodev, struct cras_ionode *node)
{
  cras_iodev_rm_node_called++;
  iodev->nodes = NULL;
}

void cras_iodev_set_active_node(struct cras_iodev *iodev,
				struct cras_ionode *node)
{
  cras_iodev_set_active_node_called++;
  iodev->active_node = node;
}

// From cras_bt_transport
struct cras_bt_device *cras_bt_transport_device(
	const struct cras_bt_transport *transport)
{
  return reinterpret_cast<struct cras_bt_device *>(0x456);;
}

enum cras_bt_device_profile cras_bt_transport_profile(
  const struct cras_bt_transport *transport)
{
  return CRAS_BT_DEVICE_PROFILE_A2DP_SOURCE;
}

// From cras_bt_device
const char *cras_bt_device_name(const struct cras_bt_device *device)
{
  return cras_bt_device_name_ret;
}

const char *cras_bt_device_object_path(const struct cras_bt_device *device) {
  return "/org/bluez/hci0/dev_1A_2B_3C_4D_5E_6F";
}

void cras_bt_device_append_iodev(struct cras_bt_device *device,
                                 struct cras_iodev *iodev,
                                 enum cras_bt_device_profile profile)
{
  cras_bt_device_append_iodev_called++;
}

void cras_bt_device_rm_iodev(struct cras_bt_device *device,
                             struct cras_iodev *iodev)
{
  cras_bt_device_rm_iodev_called++;
}

int cras_bt_device_get_use_hardware_volume(struct cras_bt_device *device)
{
  return 0;
}

int cras_bt_device_cancel_suspend(struct cras_bt_device *device)
{
  return 0;
}

int cras_bt_device_schedule_suspend(struct cras_bt_device *device,
                                    unsigned int msec)
{
  return 0;
}

int init_a2dp(struct a2dp_info *a2dp, a2dp_sbc_t *sbc)
{
  init_a2dp_called++;
  return init_a2dp_return_val;
}

void destroy_a2dp(struct a2dp_info *a2dp)
{
  destroy_a2dp_called++;
}

int a2dp_codesize(struct a2dp_info *a2dp)
{
  return 512;
}

int a2dp_block_size(struct a2dp_info *a2dp, int encoded_bytes)
{
  a2dp_block_size_called++;

  // Assumes a2dp block size is 1:1 before/after encode.
  return encoded_bytes;
}

int a2dp_queued_frames(struct a2dp_info *a2dp)
{
  return a2dp_queued_frames_val;
}

void a2dp_drain(struct a2dp_info *a2dp)
{
  drain_a2dp_called++;
}

int a2dp_encode(struct a2dp_info *a2dp, const void *pcm_buf, int pcm_buf_size,
                int format_bytes, size_t link_mtu) {
  unsigned int processed;

  if (a2dp_encode_index == MAX_A2DP_ENCODE_CALLS)
    return 0;
  processed = a2dp_encode_processed_bytes_val[a2dp_encode_index];
  pcm_buf_size_val[a2dp_encode_index] = pcm_buf_size;
  a2dp_encode_index++;
  return processed;
}

int a2dp_write(struct a2dp_info *a2dp, int stream_fd, size_t link_mtu) {
  return a2dp_write_return_val[a2dp_write_index++];;
}

int clock_gettime(clockid_t clk_id, struct timespec *tp) {
  *tp = time_now;
  return 0;
}

void cras_iodev_init_audio_area(struct cras_iodev *iodev,
                                int num_channels) {
  iodev->area = dummy_audio_area;
}

void cras_iodev_free_audio_area(struct cras_iodev *iodev) {
}

void cras_audio_area_config_buf_pointers(struct cras_audio_area *area,
					 const struct cras_audio_format *fmt,
					 uint8_t *base_buffer)
{
  dummy_audio_area->channels[0].buf = base_buffer;
}

struct audio_thread *cras_iodev_list_get_audio_thread()
{
  return NULL;
}
// From audio_thread
struct audio_thread_event_log *atlog;

void audio_thread_add_write_callback(int fd, thread_callback cb, void *data) {
  write_callback = cb;
  write_callback_data = data;
}

int audio_thread_rm_callback_sync(struct audio_thread *thread, int fd) {
  return 0;
}

void audio_thread_enable_callback(int fd, int enabled) {
}

}
