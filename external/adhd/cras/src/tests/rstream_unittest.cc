// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <gtest/gtest.h>

extern "C" {
#include "cras_audio_area.h"
#include "cras_messages.h"
#include "cras_rstream.h"
#include "cras_shm.h"
}

namespace {

class RstreamTestSuite : public testing::Test {
  protected:
    virtual void SetUp() {
      fmt_.format = SND_PCM_FORMAT_S16_LE;
      fmt_.frame_rate = 48000;
      fmt_.num_channels = 2;

      config_.stream_id = 555;
      config_.stream_type = CRAS_STREAM_TYPE_DEFAULT;
      config_.direction = CRAS_STREAM_OUTPUT;
      config_.dev_idx = NO_DEVICE;
      config_.flags = 0;
      config_.format = &fmt_;
      config_.buffer_frames = 4096;
      config_.cb_threshold = 2048;
      config_.audio_fd = 1;
      config_.client = NULL;
    }

    static bool format_equal(cras_audio_format *fmt1, cras_audio_format *fmt2) {
      return fmt1->format == fmt2->format &&
          fmt1->frame_rate == fmt2->frame_rate &&
          fmt1->num_channels == fmt2->num_channels;
    }

    struct cras_audio_format fmt_;
    struct cras_rstream_config config_;
};

TEST_F(RstreamTestSuite, InvalidDirection) {
  struct cras_rstream *s;
  int rc;

  config_.direction = (enum CRAS_STREAM_DIRECTION)66;
  rc = cras_rstream_create(&config_, &s);
  EXPECT_NE(0, rc);
}

TEST_F(RstreamTestSuite, InvalidStreamType) {
  struct cras_rstream *s;
  int rc;

  config_.stream_type = (enum CRAS_STREAM_TYPE)7;
  rc = cras_rstream_create(&config_, &s);
  EXPECT_NE(0, rc);
}

TEST_F(RstreamTestSuite, InvalidBufferSize) {
  struct cras_rstream *s;
  int rc;

  config_.buffer_frames = 3;
  rc = cras_rstream_create(&config_, &s);
  EXPECT_NE(0, rc);
}

TEST_F(RstreamTestSuite, InvalidCallbackThreshold) {
  struct cras_rstream *s;
  int rc;

  config_.cb_threshold = 3;
  rc = cras_rstream_create(&config_, &s);
  EXPECT_NE(0, rc);
}

TEST_F(RstreamTestSuite, InvalidStreamPointer) {
  int rc;

  rc = cras_rstream_create(&config_, NULL);
  EXPECT_NE(0, rc);
}

TEST_F(RstreamTestSuite, CreateOutput) {
  struct cras_rstream *s;
  struct cras_audio_format fmt_ret;
  struct cras_audio_shm *shm_ret;
  struct cras_audio_shm shm_mapped;
  int rc, fd_ret;
  size_t shm_size;

  rc = cras_rstream_create(&config_, &s);
  EXPECT_EQ(0, rc);
  EXPECT_NE((void *)NULL, s);
  EXPECT_EQ(4096, cras_rstream_get_buffer_frames(s));
  EXPECT_EQ(2048, cras_rstream_get_cb_threshold(s));
  EXPECT_EQ(CRAS_STREAM_TYPE_DEFAULT, cras_rstream_get_type(s));
  EXPECT_EQ(CRAS_STREAM_OUTPUT, cras_rstream_get_direction(s));
  EXPECT_NE((void *)NULL, cras_rstream_output_shm(s));
  rc = cras_rstream_get_format(s, &fmt_ret);
  EXPECT_EQ(0, rc);
  EXPECT_TRUE(format_equal(&fmt_ret, &fmt_));

  // Check if shm is really set up.
  shm_ret = cras_rstream_output_shm(s);
  ASSERT_NE((void *)NULL, shm_ret);
  fd_ret = cras_rstream_output_shm_fd(s);
  shm_size = cras_rstream_get_total_shm_size(s);
  EXPECT_GT(shm_size, 4096);
  shm_mapped.area = (struct cras_audio_shm_area *)mmap(
      NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_ret, 0);
  EXPECT_NE((void *)NULL, shm_mapped.area);
  cras_shm_copy_shared_config(&shm_mapped);
  EXPECT_EQ(cras_shm_used_size(&shm_mapped), cras_shm_used_size(shm_ret));
  munmap(shm_mapped.area, shm_size);

  cras_rstream_destroy(s);
}

TEST_F(RstreamTestSuite, CreateInput) {
  struct cras_rstream *s;
  struct cras_audio_format fmt_ret;
  struct cras_audio_shm *shm_ret;
  struct cras_audio_shm shm_mapped;
  int rc, fd_ret;
  size_t shm_size;

  config_.direction = CRAS_STREAM_INPUT;
  rc = cras_rstream_create(&config_, &s);
  EXPECT_EQ(0, rc);
  EXPECT_NE((void *)NULL, s);
  EXPECT_EQ(4096, cras_rstream_get_buffer_frames(s));
  EXPECT_EQ(2048, cras_rstream_get_cb_threshold(s));
  EXPECT_EQ(CRAS_STREAM_TYPE_DEFAULT, cras_rstream_get_type(s));
  EXPECT_EQ(CRAS_STREAM_INPUT, cras_rstream_get_direction(s));
  EXPECT_NE((void *)NULL, cras_rstream_input_shm(s));
  rc = cras_rstream_get_format(s, &fmt_ret);
  EXPECT_EQ(0, rc);
  EXPECT_TRUE(format_equal(&fmt_ret, &fmt_));

  // Check if shm is really set up.
  shm_ret = cras_rstream_input_shm(s);
  ASSERT_NE((void *)NULL, shm_ret);
  fd_ret = cras_rstream_input_shm_fd(s);
  shm_size = cras_rstream_get_total_shm_size(s);
  EXPECT_GT(shm_size, 4096);
  shm_mapped.area = (struct cras_audio_shm_area *)mmap(
      NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_ret, 0);
  EXPECT_NE((void *)NULL, shm_mapped.area);
  cras_shm_copy_shared_config(&shm_mapped);
  EXPECT_EQ(cras_shm_used_size(&shm_mapped), cras_shm_used_size(shm_ret));
  munmap(shm_mapped.area, shm_size);

  cras_rstream_destroy(s);
}

TEST_F(RstreamTestSuite, VerifyStreamTypes) {
  struct cras_rstream *s;
  int rc;

  config_.stream_type = CRAS_STREAM_TYPE_DEFAULT;
  rc = cras_rstream_create(&config_, &s);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(CRAS_STREAM_TYPE_DEFAULT, cras_rstream_get_type(s));
  EXPECT_NE(CRAS_STREAM_TYPE_MULTIMEDIA, cras_rstream_get_type(s));
  cras_rstream_destroy(s);

  config_.stream_type = CRAS_STREAM_TYPE_VOICE_COMMUNICATION;
  rc = cras_rstream_create(&config_, &s);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(CRAS_STREAM_TYPE_VOICE_COMMUNICATION, cras_rstream_get_type(s));
  cras_rstream_destroy(s);

  config_.direction = CRAS_STREAM_INPUT;
  config_.stream_type = CRAS_STREAM_TYPE_SPEECH_RECOGNITION;
  rc = cras_rstream_create(&config_, &s);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(CRAS_STREAM_TYPE_SPEECH_RECOGNITION, cras_rstream_get_type(s));
  cras_rstream_destroy(s);

  config_.stream_type = CRAS_STREAM_TYPE_PRO_AUDIO;
  rc = cras_rstream_create(&config_, &s);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(CRAS_STREAM_TYPE_PRO_AUDIO, cras_rstream_get_type(s));
  cras_rstream_destroy(s);
}

}  //  namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

/* stubs */
extern "C" {

struct cras_audio_area *cras_audio_area_create(int num_channels) {
  return NULL;
}

void cras_audio_area_destroy(struct cras_audio_area *area) {
}

void cras_audio_area_config_channels(struct cras_audio_area *area,
                                     const struct cras_audio_format *fmt) {
}

struct buffer_share *buffer_share_create(unsigned int buf_sz) {
  return NULL;
}

void buffer_share_destroy(struct buffer_share *mix) {
}

int buffer_share_offset_update(struct buffer_share *mix, unsigned int id,
                               unsigned int frames) {
  return 0;
}

unsigned int buffer_share_get_new_write_point(struct buffer_share *mix) {
  return 0;
}

int buffer_share_add_id(struct buffer_share *mix, unsigned int id) {
  return 0;
}

int buffer_share_rm_id(struct buffer_share *mix, unsigned int id) {
  return 0;
}

unsigned int buffer_share_id_offset(const struct buffer_share *mix,
                                    unsigned int id)
{
  return 0;
}

void cras_system_state_stream_added(enum CRAS_STREAM_DIRECTION direction) {
}

void cras_system_state_stream_removed(enum CRAS_STREAM_DIRECTION direction) {
}

}
