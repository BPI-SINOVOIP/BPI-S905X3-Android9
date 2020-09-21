/* Copyright 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <gtest/gtest.h>
#include <stdint.h>
#include <time.h>

extern "C" {
  #include "cras_hfp_info.c"
}

static struct hfp_info *info;
static struct cras_iodev dev;
static cras_audio_format format;

static thread_callback thread_cb;
static void *cb_data;
static timespec ts;

void ResetStubData() {
  format.format = SND_PCM_FORMAT_S16_LE;
  format.num_channels = 1;
  format.frame_rate = 8000;
  dev.format = &format;
}

namespace {

TEST(HfpInfo, AddRmDev) {
  info = hfp_info_create();
  ASSERT_NE(info, (void *)NULL);
  dev.direction = CRAS_STREAM_OUTPUT;

  /* Test add dev */
  ASSERT_EQ(0, hfp_info_add_iodev(info, &dev));
  ASSERT_TRUE(hfp_info_has_iodev(info));

  /* Test remove dev */
  ASSERT_EQ(0, hfp_info_rm_iodev(info, &dev));
  ASSERT_FALSE(hfp_info_has_iodev(info));

  hfp_info_destroy(info);
}

TEST(HfpInfo, AddRmDevInvalid) {
  info = hfp_info_create();
  ASSERT_NE(info, (void *)NULL);

  dev.direction = CRAS_STREAM_OUTPUT;

  /* Remove an iodev which doesn't exist */
  ASSERT_NE(0, hfp_info_rm_iodev(info, &dev));

  /* Adding an iodev twice returns error code */
  ASSERT_EQ(0, hfp_info_add_iodev(info, &dev));
  ASSERT_NE(0, hfp_info_add_iodev(info, &dev));

  hfp_info_destroy(info);
}

TEST(HfpInfo, AcquirePlaybackBuffer) {
  unsigned buffer_frames, buffer_frames2, queued;
  uint8_t *samples;

  ResetStubData();

  info = hfp_info_create();
  ASSERT_NE(info, (void *)NULL);

  hfp_info_start(1, 48, info);
  dev.direction = CRAS_STREAM_OUTPUT;
  ASSERT_EQ(0, hfp_info_add_iodev(info, &dev));

  buffer_frames = 500;
  hfp_buf_acquire(info, &dev, &samples, &buffer_frames);
  ASSERT_EQ(500, buffer_frames);

  hfp_buf_release(info, &dev, 500);
  ASSERT_EQ(500, hfp_buf_queued(info, &dev));

  /* Assert the amount of frames of available buffer + queued buf is
   * greater than or equal to the buffer size, 2 bytes per frame
   */
  queued = hfp_buf_queued(info, &dev);
  buffer_frames = 500;
  hfp_buf_acquire(info, &dev, &samples, &buffer_frames);
  ASSERT_GE(info->playback_buf->used_size / 2, buffer_frames + queued);

  /* Consume all queued data from read buffer */
  buf_increment_read(info->playback_buf, queued * 2);

  queued = hfp_buf_queued(info, &dev);
  ASSERT_EQ(0, queued);

  /* Assert consecutive acquire buffer will acquire full used size of buffer */
  buffer_frames = 500;
  hfp_buf_acquire(info, &dev, &samples, &buffer_frames);
  hfp_buf_release(info, &dev, buffer_frames);

  buffer_frames2 = 500;
  hfp_buf_acquire(info, &dev, &samples, &buffer_frames2);
  hfp_buf_release(info, &dev, buffer_frames2);

  ASSERT_GE(info->playback_buf->used_size / 2, buffer_frames + buffer_frames2);

  hfp_info_destroy(info);
}

TEST(HfpInfo, AcquireCaptureBuffer) {
  unsigned buffer_frames, buffer_frames2;
  uint8_t *samples;

  ResetStubData();

  info = hfp_info_create();
  ASSERT_NE(info, (void *)NULL);

  hfp_info_start(1, 48, info);
  dev.direction = CRAS_STREAM_INPUT;
  ASSERT_EQ(0, hfp_info_add_iodev(info, &dev));

  /* Put fake data 100 bytes(50 frames) in capture buf for test */
  buf_increment_write(info->capture_buf, 100);

  /* Assert successfully acquire and release 100 bytes of data */
  buffer_frames = 50;
  hfp_buf_acquire(info, &dev, &samples, &buffer_frames);
  ASSERT_EQ(50, buffer_frames);

  hfp_buf_release(info, &dev, buffer_frames);
  ASSERT_EQ(0, hfp_buf_queued(info, &dev));

  /* Push fake data to capture buffer */
  buf_increment_write(info->capture_buf, info->capture_buf->used_size - 100);
  buf_increment_write(info->capture_buf, 100);

  /* Assert consecutive acquire call will consume the whole buffer */
  buffer_frames = 1000;
  hfp_buf_acquire(info, &dev, &samples, &buffer_frames);
  hfp_buf_release(info, &dev, buffer_frames);
  ASSERT_GE(1000, buffer_frames);

  buffer_frames2 = 1000;
  hfp_buf_acquire(info, &dev, &samples, &buffer_frames2);
  hfp_buf_release(info, &dev, buffer_frames2);

  ASSERT_GE(info->capture_buf->used_size / 2, buffer_frames + buffer_frames2);

  hfp_info_destroy(info);
}

TEST(HfpInfo, HfpReadWriteFD) {
  int rc;
  int sock[2];
  uint8_t sample[480];
  uint8_t *buf;
  unsigned buffer_count;

  ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sock));

  info = hfp_info_create();
  ASSERT_NE(info, (void *)NULL);

  dev.direction = CRAS_STREAM_INPUT;
  hfp_info_start(sock[1], 48, info);
  ASSERT_EQ(0, hfp_info_add_iodev(info, &dev));

  /* Mock the sco fd and send some fake data */
  send(sock[0], sample, 48, 0);

  rc = hfp_read(info);
  ASSERT_EQ(48, rc);

  rc = hfp_buf_queued(info, &dev);
  ASSERT_EQ(48 / 2, rc);

  /* Fill the write buffer*/
  buffer_count = info->capture_buf->used_size;
  buf = buf_write_pointer_size(info->capture_buf, &buffer_count);
  buf_increment_write(info->capture_buf, buffer_count);
  ASSERT_NE((void *)NULL, buf);

  rc = hfp_read(info);
  ASSERT_EQ(0, rc);

  ASSERT_EQ(0, hfp_info_rm_iodev(info, &dev));
  dev.direction = CRAS_STREAM_OUTPUT;
  ASSERT_EQ(0, hfp_info_add_iodev(info, &dev));

  /* Initial buffer is empty */
  rc = hfp_write(info);
  ASSERT_EQ(0, rc);

  buffer_count = 1024;
  buf = buf_write_pointer_size(info->playback_buf, &buffer_count);
  buf_increment_write(info->playback_buf, buffer_count);

  rc = hfp_write(info);
  ASSERT_EQ(48, rc);

  rc = recv(sock[0], sample, 48, 0);
  ASSERT_EQ(48, rc);

  hfp_info_destroy(info);
}

TEST(HfpInfo, StartHfpInfo) {
  int sock[2];

  ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sock));

  info = hfp_info_create();
  ASSERT_NE(info, (void *)NULL);

  hfp_info_start(sock[0], 48, info);
  ASSERT_EQ(1, hfp_info_running(info));
  ASSERT_EQ(cb_data, (void *)info);

  hfp_info_stop(info);
  ASSERT_EQ(0, hfp_info_running(info));
  ASSERT_EQ(NULL, cb_data);

  hfp_info_destroy(info);
}

TEST(HfpInfo, StartHfpInfoAndRead) {
  int rc;
  int sock[2];
  uint8_t sample[480];

  ResetStubData();

  ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sock));

  info = hfp_info_create();
  ASSERT_NE(info, (void *)NULL);

  /* Start and send two chunk of fake data */
  hfp_info_start(sock[1], 48, info);
  send(sock[0], sample ,48, 0);
  send(sock[0], sample ,48, 0);

  /* Trigger thread callback */
  thread_cb((struct hfp_info *)cb_data);

  dev.direction = CRAS_STREAM_INPUT;
  ASSERT_EQ(0, hfp_info_add_iodev(info, &dev));

  /* Expect no data read, since no idev present at previous thread callback */
  rc = hfp_buf_queued(info, &dev);
  ASSERT_EQ(0, rc);

  /* Trigger thread callback after idev added. */
  ts.tv_sec = 0;
  ts.tv_nsec = 5000000;
  thread_cb((struct hfp_info *)cb_data);

  rc = hfp_buf_queued(info, &dev);
  ASSERT_EQ(48 / 2, rc);

  /* Assert wait time is unchanged. */
  ASSERT_EQ(0, ts.tv_sec);
  ASSERT_EQ(5000000, ts.tv_nsec);

  hfp_info_stop(info);
  ASSERT_EQ(0, hfp_info_running(info));

  hfp_info_destroy(info);
}

TEST(HfpInfo, StartHfpInfoAndWrite) {
  int rc;
  int sock[2];
  uint8_t sample[480];

  ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sock));

  info = hfp_info_create();
  ASSERT_NE(info, (void *)NULL);

  hfp_info_start(sock[1], 48, info);
  send(sock[0], sample ,48, 0);
  send(sock[0], sample ,48, 0);

  /* Trigger thread callback */
  thread_cb((struct hfp_info *)cb_data);

  dev.direction = CRAS_STREAM_OUTPUT;
  ASSERT_EQ(0, hfp_info_add_iodev(info, &dev));

  /* Assert queued samples unchanged before output device added */
  ASSERT_EQ(0, hfp_buf_queued(info, &dev));

  /* Put some fake data and trigger thread callback again */
  buf_increment_write(info->playback_buf, 1008);
  thread_cb((struct hfp_info *)cb_data);

  /* Assert some samples written */
  rc = recv(sock[0], sample ,48, 0);
  ASSERT_EQ(48, rc);
  ASSERT_EQ(480, hfp_buf_queued(info, &dev));

  hfp_info_stop(info);
  hfp_info_destroy(info);
}

} // namespace

extern "C" {

struct audio_thread *cras_iodev_list_get_audio_thread()
{
  return NULL;
}

void audio_thread_add_callback(int fd, thread_callback cb,
                               void *data)
{
  thread_cb = cb;
  cb_data = data;
  return;
}

int audio_thread_rm_callback_sync(struct audio_thread *thread, int fd)
{
  thread_cb = NULL;
  cb_data = NULL;
  return 0;
}
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
