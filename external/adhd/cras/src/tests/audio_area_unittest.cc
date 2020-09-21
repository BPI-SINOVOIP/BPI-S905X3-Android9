// Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <gtest/gtest.h>

extern "C" {
#include "cras_audio_format.h"
#include "cras_audio_area.h"
}

static const int8_t stereo[CRAS_CH_MAX] = {
  0, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};
static const int8_t mono[CRAS_CH_MAX] = {
  -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1,
};
static const int8_t kb_mic[CRAS_CH_MAX] = {
  0, 1, -1, -1, 2, -1, -1, -1, -1, -1, -1,
};

static uint16_t buf1[32];
static uint16_t buf2[32];
struct cras_audio_area *a1;
struct cras_audio_area *a2;

namespace {

TEST(AudioArea, CopyAudioArea) {
  struct cras_audio_format fmt;
  int i;

  fmt.num_channels = 2;
  fmt.format = SND_PCM_FORMAT_S16_LE;
  for (i = 0; i < CRAS_CH_MAX; i++)
    fmt.channel_layout[i] = stereo[i];

  a1 = cras_audio_area_create(2);
  a2 = cras_audio_area_create(2);
  cras_audio_area_config_channels(a1, &fmt);
  cras_audio_area_config_channels(a2, &fmt);
  cras_audio_area_config_buf_pointers(a1, &fmt, (uint8_t *)buf1);
  cras_audio_area_config_buf_pointers(a2, &fmt, (uint8_t *)buf2);
  a1->frames = 16;
  a2->frames = 16;

  memset(buf1, 0, 32 * 2);
  for (i = 0; i < 32; i++)
    buf2[i] = rand();
  cras_audio_area_copy(a1, 0, &fmt, a2, 0, 1.0);
  for (i = 0; i < 32; i++)
    EXPECT_EQ(buf1[i], buf2[i]);

  cras_audio_area_destroy(a1);
  cras_audio_area_destroy(a2);
}

TEST(AudioArea, CopyAudioAreaWithGain) {
  struct cras_audio_format fmt;
  int i;
  /* Check a gain of 10x can be applied. */
  float gain_scaler = 10.0f;

  fmt.num_channels = 2;
  fmt.format = SND_PCM_FORMAT_S16_LE;
  for (i = 0; i < CRAS_CH_MAX; i++)
    fmt.channel_layout[i] = stereo[i];

  a1 = cras_audio_area_create(2);
  a2 = cras_audio_area_create(2);
  cras_audio_area_config_channels(a1, &fmt);
  cras_audio_area_config_channels(a2, &fmt);
  cras_audio_area_config_buf_pointers(a1, &fmt, (uint8_t *)buf1);
  cras_audio_area_config_buf_pointers(a2, &fmt, (uint8_t *)buf2);
  a1->frames = 16;
  a2->frames = 16;

  memset(buf1, 0, 32 * 2);
  /* Let src has some samples smaller than 32768/10 and some samples larger than
   * 32768/10 to test clipping. */
  for (i = 0; i < 16; i++)
    buf2[i] = rand() % 3270;
  for (i = 17; i < 32; i++)
    buf2[i] = 3280 + rand() % 3200;
  cras_audio_area_copy(a1, 0, &fmt, a2, 0, gain_scaler);
  for (i = 0; i < 32; i++) {
    int32_t expected_value = buf2[i] * gain_scaler;
    if (expected_value > INT16_MAX)
      expected_value = INT16_MAX;
    EXPECT_EQ(buf1[i], expected_value);
  }

  cras_audio_area_destroy(a1);
  cras_audio_area_destroy(a2);
}
TEST(AudioArea, CopyAudioAreaOffset) {
  struct cras_audio_format fmt;
  int i;

  fmt.num_channels = 2;
  fmt.format = SND_PCM_FORMAT_S16_LE;
  for (i = 0; i < CRAS_CH_MAX; i++)
    fmt.channel_layout[i] = stereo[i];

  a1 = cras_audio_area_create(2);
  a2 = cras_audio_area_create(2);
  cras_audio_area_config_channels(a1, &fmt);
  cras_audio_area_config_channels(a2, &fmt);
  cras_audio_area_config_buf_pointers(a1, &fmt, (uint8_t *)buf1);
  cras_audio_area_config_buf_pointers(a2, &fmt, (uint8_t *)buf2);
  a1->frames = 16;
  a2->frames = 14;

  memset(buf1, 0, 32 * 2);
  for (i = 0; i < 32; i++)
    buf2[i] = rand();
  cras_audio_area_copy(a1, 2, &fmt, a2, 0, 1.0);
  EXPECT_EQ(buf1[0], 0);
  EXPECT_EQ(buf1[1], 0);
  EXPECT_EQ(buf1[2], 0);
  EXPECT_EQ(buf1[3], 0);
  for (i = 4; i < 32; i++)
    EXPECT_EQ(buf1[i], buf2[i-4]);

  cras_audio_area_destroy(a1);
  cras_audio_area_destroy(a2);
}

TEST(AudioArea, CopyAudioAreaOffsetLimit) {
  struct cras_audio_format fmt;
  int i;

  fmt.num_channels = 2;
  fmt.format = SND_PCM_FORMAT_S16_LE;
  for (i = 0; i < CRAS_CH_MAX; i++)
    fmt.channel_layout[i] = stereo[i];

  a1 = cras_audio_area_create(2);
  a2 = cras_audio_area_create(2);
  cras_audio_area_config_channels(a1, &fmt);
  cras_audio_area_config_channels(a2, &fmt);
  cras_audio_area_config_buf_pointers(a1, &fmt, (uint8_t *)buf1);
  cras_audio_area_config_buf_pointers(a2, &fmt, (uint8_t *)buf2);
  a1->frames = 14;
  a2->frames = 14;

  memset(buf1, 0, 32 * 2);
  for (i = 0; i < 32; i++)
    buf2[i] = rand();
  cras_audio_area_copy(a1, 2, &fmt, a2, 0, 1.0);
  EXPECT_EQ(buf1[0], 0);
  EXPECT_EQ(buf1[1], 0);
  EXPECT_EQ(buf1[2], 0);
  EXPECT_EQ(buf1[3], 0);
  for (i = 4; i < 28; i++)
    EXPECT_EQ(buf1[i], buf2[i-4]);
  EXPECT_EQ(buf1[28], 0);
  EXPECT_EQ(buf1[29], 0);
  EXPECT_EQ(buf1[30], 0);
  EXPECT_EQ(buf1[31], 0);

  cras_audio_area_destroy(a1);
  cras_audio_area_destroy(a2);
}

TEST(AudioArea, CopyMonoToStereo) {
  struct cras_audio_format dst_fmt;
  struct cras_audio_format src_fmt;
  int i;

  dst_fmt.num_channels = 2;
  dst_fmt.format = SND_PCM_FORMAT_S16_LE;
  for (i = 0; i < CRAS_CH_MAX; i++)
    dst_fmt.channel_layout[i] = stereo[i];
  a1 = cras_audio_area_create(2);
  a1->frames = 16;
  cras_audio_area_config_channels(a1, &dst_fmt);
  cras_audio_area_config_buf_pointers(a1, &dst_fmt, (uint8_t *)buf1);

  src_fmt.num_channels = 1;
  src_fmt.format = SND_PCM_FORMAT_S16_LE;
  for (i = 0; i < CRAS_CH_MAX; i++)
    src_fmt.channel_layout[i] = mono[i];
  a2 = cras_audio_area_create(1);
  a2->frames = 16;
  cras_audio_area_config_channels(a2, &src_fmt);
  cras_audio_area_config_buf_pointers(a2, &src_fmt, (uint8_t *)buf2);

  memset(buf1, 0, 32 * 2);
  for (i = 0; i < 32; i++)
    buf2[i] = rand();
  cras_audio_area_copy(a1, 0, &dst_fmt, a2, 0, 1.0);
  for (i = 0; i < 16; i++) {
    EXPECT_EQ(buf1[i * 2], buf2[i]);
    EXPECT_EQ(buf1[i * 2 + 1], buf2[i]);
  }

  cras_audio_area_destroy(a1);
  cras_audio_area_destroy(a2);
}

TEST(AudioArea, CopyStereoToMono) {
  struct cras_audio_format fmt;
  int i;

  fmt.num_channels = 1;
  fmt.format = SND_PCM_FORMAT_S16_LE;
  for (i = 0; i < CRAS_CH_MAX; i++)
    fmt.channel_layout[i] = mono[i];
  a1 = cras_audio_area_create(1);
  a1->frames = 16;
  cras_audio_area_config_channels(a1, &fmt);
  cras_audio_area_config_buf_pointers(a1, &fmt, (uint8_t *)buf1);

  fmt.num_channels = 2;
  for (i = 0; i < CRAS_CH_MAX; i++)
    fmt.channel_layout[i] = stereo[i];
  a2 = cras_audio_area_create(2);
  a2->frames = 16;
  cras_audio_area_config_channels(a2, &fmt);
  cras_audio_area_config_buf_pointers(a2, &fmt, (uint8_t *)buf2);

  memset(buf1, 0, 32 * 2);
  for (i = 0; i < 32; i++)
    buf2[i] = rand() % 10000;
  cras_audio_area_copy(a1, 0, &fmt, a2, 0, 1.0);
  for (i = 0; i < 16; i++)
    EXPECT_EQ(buf1[i], buf2[i * 2] + buf2[i * 2 + 1]);

  cras_audio_area_destroy(a1);
  cras_audio_area_destroy(a2);
}

TEST(AudioArea, KeyboardMicCopyStereo) {
  struct cras_audio_format fmt;
  int i;

  fmt.num_channels = 3;
  fmt.format = SND_PCM_FORMAT_S16_LE;
  for (i = 0; i < CRAS_CH_MAX; i++)
    fmt.channel_layout[i] = kb_mic[i];
  a1 = cras_audio_area_create(3);
  a1->frames = 10;
  cras_audio_area_config_channels(a1, &fmt);
  cras_audio_area_config_buf_pointers(a1, &fmt, (uint8_t *)buf1);

  fmt.num_channels = 2;
  for (i = 0; i < CRAS_CH_MAX; i++)
    fmt.channel_layout[i] = stereo[i];
  a2 = cras_audio_area_create(2);
  a2->frames = 10;
  cras_audio_area_config_channels(a2, &fmt);
  cras_audio_area_config_buf_pointers(a2, &fmt, (uint8_t *)buf2);

  memset(buf1, 0, 32 * 2);
  for (i = 0; i < 32; i++)
    buf2[i] = rand();
  cras_audio_area_copy(a1, 0, &fmt, a2, 0, 1.0);
  for (i = 0; i < 10; i++) {
    EXPECT_EQ(buf1[i * 3], buf2[i * 2]);
    EXPECT_EQ(buf1[i * 3 + 1], buf2[i * 2 + 1]);
    EXPECT_EQ(buf1[i * 3 + 2], 0);
  }

  cras_audio_area_destroy(a1);
  cras_audio_area_destroy(a2);
}

TEST(AudioArea, KeyboardMicCopyFrontCenter) {
  struct cras_audio_format dst_fmt;
  struct cras_audio_format src_fmt;
  int i;

  dst_fmt.num_channels = 3;
  dst_fmt.format = SND_PCM_FORMAT_S16_LE;
  for (i = 0; i < CRAS_CH_MAX; i++)
    dst_fmt.channel_layout[i] = kb_mic[i];
  a1 = cras_audio_area_create(3);
  a1->frames = 10;
  cras_audio_area_config_channels(a1, &dst_fmt);
  cras_audio_area_config_buf_pointers(a1, &dst_fmt, (uint8_t *)buf1);

  /* Test 2 channels area with only front center in layout. */
  src_fmt.num_channels = 2;
  src_fmt.format = SND_PCM_FORMAT_S16_LE;
  for (i = 0; i < CRAS_CH_MAX; i++)
    src_fmt.channel_layout[i] = -1;
  src_fmt.channel_layout[CRAS_CH_FC] = 0;
  a2 = cras_audio_area_create(2);
  a2->frames = 10;
  cras_audio_area_config_channels(a2, &src_fmt);
  cras_audio_area_config_buf_pointers(a2, &src_fmt, (uint8_t *)buf2);

  memset(buf1, 0, 32 * 2);
  for (i = 0; i < 32; i++)
    buf2[i] = rand();
  cras_audio_area_copy(a1, 0, &dst_fmt, a2, 0, 1.0);
  for (i = 0; i < 10; i++) {
    EXPECT_EQ(buf1[i * 3], 0);
    EXPECT_EQ(buf1[i * 3 + 1], 0);
    EXPECT_EQ(buf1[i * 3 + 2], buf2[i * 2]);
  }

  cras_audio_area_destroy(a1);
  cras_audio_area_destroy(a2);
}

}  //  namespace

extern "C" {

void cras_mix_add_scale_stride(snd_pcm_format_t fmt, uint8_t *dst, uint8_t *src,
			 unsigned int count, unsigned int dst_stride,
			 unsigned int src_stride, float scaler)
{
	unsigned int i;

	for (i = 0; i < count; i++) {
		int32_t sum;
		sum = *(int16_t *)dst + *(int16_t *)src * scaler;
		if (sum > INT16_MAX)
			sum = INT16_MAX;
		else if (sum < INT16_MIN)
			sum = INT16_MIN;
		*(int16_t*)dst = sum;
		dst += dst_stride;
		src += src_stride;
	}
}

}  //  extern "C"

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
