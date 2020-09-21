// Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <stdlib.h>
#include <gtest/gtest.h>

extern "C" {
#include "cras_audio_area.h"
#include "cras_iodev.h"
#include "cras_iodev_list.h"
#include "cras_loopback_iodev.h"
#include "cras_shm.h"
#include "cras_types.h"
#include "dev_stream.h"
}

namespace {

static const unsigned int kBufferFrames = 16384;
static const unsigned int kFrameBytes = 4;
static const unsigned int kBufferSize = kBufferFrames * kFrameBytes;

static struct timespec time_now;
static cras_audio_area *dummy_audio_area;
static loopback_hook_t loop_hook;
static void *loop_hook_cb_data;
static struct cras_iodev *enabled_dev;
static unsigned int cras_iodev_list_add_input_called;
static unsigned int cras_iodev_list_rm_input_called;
static unsigned int cras_iodev_list_set_device_enabled_callback_called;
static device_enabled_callback_t cras_iodev_list_set_device_enabled_callback_cb;
static void *cras_iodev_list_set_device_enabled_callback_cb_data;

class LoopBackTestSuite : public testing::Test{
  protected:
    virtual void SetUp() {
      dummy_audio_area = (cras_audio_area*)calloc(
          1, sizeof(*dummy_audio_area) + sizeof(cras_channel_area) * 2);
      for (unsigned int i = 0; i < kBufferSize; i++) {
        buf_[i] = rand();
      }
      fmt_.frame_rate = 48000;
      fmt_.num_channels = 2;
      fmt_.format = SND_PCM_FORMAT_S16_LE;

      loop_in_ = loopback_iodev_create(LOOPBACK_POST_MIX_PRE_DSP);
      EXPECT_EQ(1, cras_iodev_list_add_input_called);
      loop_in_->format = &fmt_;

      loop_hook = NULL;
      cras_iodev_list_add_input_called = 0;
      cras_iodev_list_rm_input_called = 0;
      cras_iodev_list_set_device_enabled_callback_called = 0;
    }

    virtual void TearDown() {
      loopback_iodev_destroy(loop_in_);
      EXPECT_EQ(1, cras_iodev_list_rm_input_called);
      EXPECT_EQ(NULL, cras_iodev_list_set_device_enabled_callback_cb);
    }

    uint8_t buf_[kBufferSize];
    struct cras_audio_format fmt_;
    struct cras_iodev *loop_in_;
};

TEST_F(LoopBackTestSuite, InstallLoopHook) {
  struct cras_iodev iodev;
  struct timespec tstamp;

  iodev.direction = CRAS_STREAM_OUTPUT;
  iodev.format = &fmt_;
  iodev.ext_format = &fmt_;
  enabled_dev = &iodev;

  // Open loopback devices.
  EXPECT_EQ(0, loop_in_->open_dev(loop_in_));
  EXPECT_EQ(1, cras_iodev_list_set_device_enabled_callback_called);

  // Signal an output device is enabled.
  cras_iodev_list_set_device_enabled_callback_cb(&iodev, 1,
      cras_iodev_list_set_device_enabled_callback_cb_data);

  // Expect that a hook was added to the iodev
  ASSERT_NE(reinterpret_cast<loopback_hook_t>(NULL), loop_hook);

  // Check zero frames queued.
  EXPECT_EQ(0, loop_in_->frames_queued(loop_in_, &tstamp));

  // Close loopback devices.
  EXPECT_EQ(0, loop_in_->close_dev(loop_in_));
  EXPECT_EQ(reinterpret_cast<loopback_hook_t>(NULL), loop_hook);
}

// Test how loopback works if there isn't any output devices open.
TEST_F(LoopBackTestSuite, OpenIdleSystem) {
  cras_audio_area *area;
  unsigned int nread = 1024;
  struct timespec tstamp;
  int rc;

  // No active output device.
  enabled_dev = NULL;
  time_now.tv_sec = 100;
  time_now.tv_nsec = 0;

  EXPECT_EQ(0, loop_in_->open_dev(loop_in_));
  EXPECT_EQ(1, cras_iodev_list_set_device_enabled_callback_called);

  // Should be 480 samples after 480/frame rate seconds
  time_now.tv_nsec += 480 * 1e9 / 48000;
  EXPECT_EQ(480, loop_in_->frames_queued(loop_in_, &tstamp));

  // Verify frames from loopback record.
  loop_in_->get_buffer(loop_in_, &area, &nread);
  EXPECT_EQ(480, nread);
  memset(buf_, 0, nread * kFrameBytes);
  rc = memcmp(area->channels[0].buf, buf_, nread * kFrameBytes);
  EXPECT_EQ(0, rc);
  loop_in_->put_buffer(loop_in_, nread);

  // Check zero frames queued.
  EXPECT_EQ(0, loop_in_->frames_queued(loop_in_, &tstamp));

  EXPECT_EQ(0, loop_in_->close_dev(loop_in_));
}

TEST_F(LoopBackTestSuite, SimpleLoopback) {
  cras_audio_area *area;
  unsigned int nframes = 1024;
  unsigned int nread = 1024;
  int rc;
  struct cras_iodev iodev;
  struct dev_stream stream;
  struct timespec tstamp;

  iodev.streams = &stream;
  enabled_dev = &iodev;

  loop_in_->open_dev(loop_in_);
  ASSERT_NE(reinterpret_cast<void *>(NULL), loop_hook);

  // Loopback callback for the hook.
  loop_hook(buf_, nframes, &fmt_, loop_hook_cb_data);

  // Verify frames from loopback record.
  loop_in_->get_buffer(loop_in_, &area, &nread);
  EXPECT_EQ(nframes, nread);
  rc = memcmp(area->channels[0].buf, buf_, nframes * kFrameBytes);
  EXPECT_EQ(0, rc);
  loop_in_->put_buffer(loop_in_, nread);

  // Check zero frames queued.
  EXPECT_EQ(0, loop_in_->frames_queued(loop_in_, &tstamp));

  EXPECT_EQ(0, loop_in_->close_dev(loop_in_));
}

// TODO(chinyue): Test closing last iodev while streaming loopback data.

/* Stubs */
extern "C" {

void cras_audio_area_config_buf_pointers(struct cras_audio_area *area,
                                         const struct cras_audio_format *fmt,
                                         uint8_t *base_buffer)
{
  dummy_audio_area->channels[0].buf = base_buffer;
}

void cras_iodev_free_audio_area(struct cras_iodev *iodev)
{
}

void cras_iodev_free_format(struct cras_iodev *iodev)
{
}

void cras_iodev_init_audio_area(struct cras_iodev *iodev, int num_channels)
{
  iodev->area = dummy_audio_area;
}

void cras_iodev_add_node(struct cras_iodev *iodev, struct cras_ionode *node)
{
}

void cras_iodev_set_active_node(struct cras_iodev *iodev,
                                struct cras_ionode *node)
{
}

void cras_iodev_register_pre_dsp_hook(struct cras_iodev *iodev,
				      loopback_hook_t loop_cb,
				      void *cb_data)
{
  loop_hook = loop_cb;
  loop_hook_cb_data = cb_data;
}

void cras_iodev_register_post_dsp_hook(struct cras_iodev *iodev,
				       loopback_hook_t loop_cb,
				       void *cb_data)
{
  loop_hook = loop_cb;
  loop_hook_cb_data = cb_data;
}

int cras_iodev_list_add_input(struct cras_iodev *input)
{
  cras_iodev_list_add_input_called++;
  return 0;
}

int cras_iodev_list_rm_input(struct cras_iodev *input)
{
  cras_iodev_list_rm_input_called++;
  return 0;
}

int cras_iodev_list_set_device_enabled_callback(device_enabled_callback_t cb,
                                                void *cb_data)
{
  cras_iodev_list_set_device_enabled_callback_called++;
  cras_iodev_list_set_device_enabled_callback_cb = cb;
  cras_iodev_list_set_device_enabled_callback_cb_data = cb_data;
  return 0;
}

int clock_gettime(clockid_t clk_id, struct timespec *tp) {
  *tp = time_now;
  return 0;
}

struct cras_iodev *cras_iodev_list_get_first_enabled_iodev(
    enum CRAS_STREAM_DIRECTION direction)
{
  return enabled_dev;
}

}  // extern "C"

}  //  namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
