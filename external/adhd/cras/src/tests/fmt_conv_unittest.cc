// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>
#include <sys/param.h>

extern "C" {
#include "cras_fmt_conv.h"
#include "cras_types.h"
}

static int mono_channel_layout[CRAS_CH_MAX] =
  {-1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1};
static int stereo_channel_layout[CRAS_CH_MAX] =
  {0, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
static int surround_channel_layout[CRAS_CH_MAX] =
	{0, 1, 2, 3, 4, 5, -1, -1, -1, -1, -1};
static int linear_resampler_needed_val;
static double linear_resampler_ratio = 1.0;

void ResetStub() {
  linear_resampler_needed_val = 0;
  linear_resampler_ratio = 1.0;
}

// Like malloc or calloc, but fill the memory with random bytes.
static void *ralloc(size_t size) {
  unsigned char *buf = (unsigned char *)malloc(size);
  while (size--)
    buf[size] = rand() & 0xff;
  return buf;
}

static void swap_channel_layout(int8_t *layout,
                                CRAS_CHANNEL a,
				CRAS_CHANNEL b) {
	int8_t tmp = layout[a];
	layout[a] = layout[b];
	layout[b] = tmp;
}

// Only support LE, BE should fail.
TEST(FormatConverterTest,  InvalidParamsOnlyLE) {
  struct cras_audio_format in_fmt;
  struct cras_audio_format out_fmt;
  struct cras_fmt_conv *c;

  ResetStub();
  in_fmt.format = out_fmt.format = SND_PCM_FORMAT_S32_BE;
  in_fmt.num_channels = out_fmt.num_channels = 2;
  c = cras_fmt_conv_create(&in_fmt, &out_fmt, 4096, 0);
  EXPECT_EQ(NULL, c);
}

// Test Mono to Stereo mix.
TEST(FormatConverterTest, MonoToStereo) {
  struct cras_fmt_conv *c;
  struct cras_audio_format in_fmt;
  struct cras_audio_format out_fmt;

  size_t out_frames;
  int16_t *in_buff;
  int16_t *out_buff;
  const size_t buf_size = 4096;
  unsigned int in_buf_size = 4096;

  ResetStub();
  in_fmt.format = SND_PCM_FORMAT_S16_LE;
  out_fmt.format = SND_PCM_FORMAT_S16_LE;
  in_fmt.num_channels = 1;
  out_fmt.num_channels = 2;
  in_fmt.frame_rate = 48000;
  out_fmt.frame_rate = 48000;

  c = cras_fmt_conv_create(&in_fmt, &out_fmt, buf_size, 0);
  ASSERT_NE(c, (void *)NULL);

  out_frames = cras_fmt_conv_out_frames_to_in(c, buf_size);
  EXPECT_EQ(buf_size, out_frames);

  out_frames = cras_fmt_conv_in_frames_to_out(c, buf_size);
  EXPECT_EQ(buf_size, out_frames);

  in_buff = (int16_t *)ralloc(buf_size * 2 * cras_get_format_bytes(&in_fmt));
  out_buff = (int16_t *)ralloc(buf_size * 2 * cras_get_format_bytes(&out_fmt));
  out_frames = cras_fmt_conv_convert_frames(c,
                                            (uint8_t *)in_buff,
                                            (uint8_t *)out_buff,
                                            &in_buf_size,
                                            buf_size);
  EXPECT_EQ(buf_size, out_frames);
  for (size_t i = 0; i < buf_size; i++) {
    if (in_buff[i] != out_buff[i*2] ||
        in_buff[i] != out_buff[i*2 + 1]) {
      EXPECT_TRUE(false);
      break;
    }
  }

  cras_fmt_conv_destroy(c);
  free(in_buff);
  free(out_buff);
}

// Test Stereo to Mono mix.
TEST(FormatConverterTest, StereoToMono) {
  struct cras_fmt_conv *c;
  struct cras_audio_format in_fmt;
  struct cras_audio_format out_fmt;

  size_t out_frames;
  int16_t *in_buff;
  int16_t *out_buff;
  unsigned int i;
  const size_t buf_size = 4096;
  unsigned int in_buf_size = 4096;

  ResetStub();
  in_fmt.format = SND_PCM_FORMAT_S16_LE;
  out_fmt.format = SND_PCM_FORMAT_S16_LE;
  in_fmt.num_channels = 2;
  out_fmt.num_channels = 1;
  in_fmt.frame_rate = 48000;
  out_fmt.frame_rate = 48000;

  c = cras_fmt_conv_create(&in_fmt, &out_fmt, buf_size, 0);
  ASSERT_NE(c, (void *)NULL);

  out_frames = cras_fmt_conv_out_frames_to_in(c, buf_size);
  EXPECT_EQ(buf_size, out_frames);

  out_frames = cras_fmt_conv_in_frames_to_out(c, buf_size);
  EXPECT_EQ(buf_size, out_frames);

  in_buff = (int16_t *)malloc(buf_size * 2 * cras_get_format_bytes(&in_fmt));
  out_buff = (int16_t *)malloc(buf_size * cras_get_format_bytes(&out_fmt));
  for (i = 0; i < buf_size; i++) {
    in_buff[i * 2] = 13450;
    in_buff[i * 2 + 1] = -13449;
  }
  out_frames = cras_fmt_conv_convert_frames(c,
                                            (uint8_t *)in_buff,
                                            (uint8_t *)out_buff,
                                            &in_buf_size,
                                            buf_size);
  EXPECT_EQ(buf_size, out_frames);
  for (i = 0; i < buf_size; i++) {
    EXPECT_EQ(1, out_buff[i]);
  }

  cras_fmt_conv_destroy(c);
  free(in_buff);
  free(out_buff);
}

// Test Stereo to Mono mix 24 and 32 bit.
TEST(FormatConverterTest, StereoToMono24bit) {
  struct cras_fmt_conv *c;
  struct cras_audio_format in_fmt;
  struct cras_audio_format out_fmt;

  size_t out_frames;
  int32_t *in_buff;
  int32_t *out_buff;
  unsigned int i;
  const size_t buf_size = 100;
  unsigned int in_buf_size = 100;
  unsigned int test;

  for (test = 0; test < 2; test++) {
    ResetStub();
    if (test == 0) {
      in_fmt.format = SND_PCM_FORMAT_S24_LE;
      out_fmt.format = SND_PCM_FORMAT_S24_LE;
    } else {
      in_fmt.format = SND_PCM_FORMAT_S32_LE;
      out_fmt.format = SND_PCM_FORMAT_S32_LE;
    }
    in_fmt.num_channels = 2;
    out_fmt.num_channels = 1;
    in_fmt.frame_rate = 48000;
    out_fmt.frame_rate = 48000;

    c = cras_fmt_conv_create(&in_fmt, &out_fmt, buf_size, 0);
    ASSERT_NE(c, (void *)NULL);

    out_frames = cras_fmt_conv_out_frames_to_in(c, buf_size);
    EXPECT_EQ(buf_size, out_frames);

    out_frames = cras_fmt_conv_in_frames_to_out(c, buf_size);
    EXPECT_EQ(buf_size, out_frames);

    in_buff = (int32_t *)malloc(buf_size * cras_get_format_bytes(&in_fmt));
    out_buff = (int32_t *)malloc(buf_size * cras_get_format_bytes(&out_fmt));
    // TODO(dgreid) - s/0x10000/1/ once it stays full bits the whole way.
    for (i = 0; i < buf_size; i++) {
	    in_buff[i * 2] = 13450 << 16;
	    in_buff[i * 2 + 1] = -in_buff[i * 2] + 0x10000;
    }
    out_frames = cras_fmt_conv_convert_frames(c,
		    (uint8_t *)in_buff,
		    (uint8_t *)out_buff,
		    &in_buf_size,
		    buf_size);
    EXPECT_EQ(buf_size, out_frames);
    for (i = 0; i < buf_size; i++) {
	    EXPECT_EQ(0x10000, out_buff[i]);
    }

    cras_fmt_conv_destroy(c);
    free(in_buff);
    free(out_buff);
  }
}

// Test 5.1 to Stereo mix.
TEST(FormatConverterTest, SurroundToStereo) {
  struct cras_fmt_conv *c;
  struct cras_audio_format in_fmt;
  struct cras_audio_format out_fmt;

  size_t out_frames;
  int16_t *in_buff;
  int16_t *out_buff;
  unsigned int i;
  const size_t buf_size = 4096;
  unsigned int in_buf_size = 4096;

  ResetStub();
  in_fmt.format = SND_PCM_FORMAT_S16_LE;
  out_fmt.format = SND_PCM_FORMAT_S16_LE;
  in_fmt.num_channels = 6;
  out_fmt.num_channels = 2;
  in_fmt.frame_rate = 48000;
  out_fmt.frame_rate = 48000;
  for (i = 0; i < CRAS_CH_MAX; i++)
    in_fmt.channel_layout[i] = surround_channel_layout[i];

  c = cras_fmt_conv_create(&in_fmt, &out_fmt, buf_size, 0);
  ASSERT_NE(c, (void *)NULL);

  out_frames = cras_fmt_conv_out_frames_to_in(c, buf_size);
  EXPECT_EQ(buf_size, out_frames);

  out_frames = cras_fmt_conv_in_frames_to_out(c, buf_size);
  EXPECT_EQ(buf_size, out_frames);

  in_buff = (int16_t *)malloc(buf_size * 2 * cras_get_format_bytes(&in_fmt));

  /* Swap channel to FL = 13450, RL = -100.
   * Assert right channel is silent.
   */
  for (i = 0; i < buf_size; i++) {
    in_buff[i * 6] = 13450;
    in_buff[i * 6 + 1] = 0;
    in_buff[i * 6 + 2] = -100;
    in_buff[i * 6 + 3] = 0;
    in_buff[i * 6 + 4] = 0;
    in_buff[i * 6 + 5] = 0;
  }
  out_buff = (int16_t *)malloc(buf_size * 2 * cras_get_format_bytes(&out_fmt));
  out_frames = cras_fmt_conv_convert_frames(c,
                                            (uint8_t *)in_buff,
                                            (uint8_t *)out_buff,
                                            &in_buf_size,
                                            buf_size);
  EXPECT_EQ(buf_size, out_frames);
  for (i = 0; i < buf_size; i++)
    EXPECT_LT(0, out_buff[i * 2]);

  /* Swap channel to FR = 13450, RR = -100.
   * Assert left channel is silent.
   */
  swap_channel_layout(in_fmt.channel_layout, CRAS_CH_FL, CRAS_CH_FR);
  swap_channel_layout(in_fmt.channel_layout, CRAS_CH_RL, CRAS_CH_RR);
  c = cras_fmt_conv_create(&in_fmt, &out_fmt, buf_size, 0);
  out_frames = cras_fmt_conv_convert_frames(c,
                                            (uint8_t *)in_buff,
                                            (uint8_t *)out_buff,
                                            &in_buf_size,
                                            buf_size);
  EXPECT_EQ(buf_size, out_frames);
  for (i = 0; i < buf_size; i++)
    EXPECT_LT(0, out_buff[i * 2 + 1]);

  /* Swap channel to FC = 13450, LFE = -100.
   * Assert output left and right has equal magnitude.
   */
  swap_channel_layout(in_fmt.channel_layout, CRAS_CH_FR, CRAS_CH_FC);
  swap_channel_layout(in_fmt.channel_layout, CRAS_CH_RR, CRAS_CH_LFE);
  c = cras_fmt_conv_create(&in_fmt, &out_fmt, buf_size, 0);
  out_frames = cras_fmt_conv_convert_frames(c,
                                            (uint8_t *)in_buff,
                                            (uint8_t *)out_buff,
                                            &in_buf_size,
                                            buf_size);
  EXPECT_EQ(buf_size, out_frames);
  for (i = 0; i < buf_size; i++) {
    EXPECT_NE(0, out_buff[i * 2]);
    EXPECT_EQ(out_buff[i * 2], out_buff[i * 2 + 1]);
  }

  /* Swap channel to FR = 13450, FL = -100.
   * Assert output left is positive and right is negative. */
  swap_channel_layout(in_fmt.channel_layout, CRAS_CH_LFE, CRAS_CH_FR);
  swap_channel_layout(in_fmt.channel_layout, CRAS_CH_FC, CRAS_CH_FL);
  c = cras_fmt_conv_create(&in_fmt, &out_fmt, buf_size, 0);
  out_frames = cras_fmt_conv_convert_frames(c,
                                            (uint8_t *)in_buff,
                                            (uint8_t *)out_buff,
                                            &in_buf_size,
                                            buf_size);
  EXPECT_EQ(buf_size, out_frames);
  for (i = 0; i < buf_size; i++) {
    EXPECT_LT(0, out_buff[i * 2]);
    EXPECT_GT(0, out_buff[i * 2 + 1]);
  }

  cras_fmt_conv_destroy(c);
  free(in_buff);
  free(out_buff);
}

// Test 2 to 1 SRC.
TEST(FormatConverterTest,  Convert2To1) {
  struct cras_fmt_conv *c;
  struct cras_audio_format in_fmt;
  struct cras_audio_format out_fmt;

  size_t out_frames;
  int16_t *in_buff;
  int16_t *out_buff;
  const size_t buf_size = 4096;
  unsigned int in_buf_size = 4096;

  ResetStub();
  in_fmt.format = out_fmt.format = SND_PCM_FORMAT_S16_LE;
  in_fmt.num_channels = out_fmt.num_channels = 2;
  in_fmt.frame_rate = 96000;
  out_fmt.frame_rate = 48000;

  c = cras_fmt_conv_create(&in_fmt, &out_fmt, buf_size, 0);
  ASSERT_NE(c, (void *)NULL);

  out_frames = cras_fmt_conv_in_frames_to_out(c, buf_size);
  EXPECT_EQ(buf_size/2, out_frames);

  in_buff = (int16_t *)ralloc(buf_size * 2 * cras_get_format_bytes(&in_fmt));
  out_buff = (int16_t *)ralloc(buf_size/2 * cras_get_format_bytes(&out_fmt));
  out_frames = cras_fmt_conv_convert_frames(c,
                                            (uint8_t *)in_buff,
                                            (uint8_t *)out_buff,
                                            &in_buf_size,
                                            buf_size / 2);
  cras_fmt_conv_destroy(c);
  free(in_buff);
  free(out_buff);
}

// Test 1 to 2 SRC.
TEST(FormatConverterTest,  Convert1To2) {
  struct cras_fmt_conv *c;
  struct cras_audio_format in_fmt;
  struct cras_audio_format out_fmt;
  size_t out_frames;
  int16_t *in_buff;
  int16_t *out_buff;
  const size_t buf_size = 4096;
  unsigned int in_buf_size = 4096;

  ResetStub();
  in_fmt.format = out_fmt.format = SND_PCM_FORMAT_S16_LE;
  in_fmt.num_channels = out_fmt.num_channels = 2;
  in_fmt.frame_rate = 22050;
  out_fmt.frame_rate = 44100;

  c = cras_fmt_conv_create(&in_fmt, &out_fmt, buf_size, 0);
  ASSERT_NE(c, (void *)NULL);

  out_frames = cras_fmt_conv_in_frames_to_out(c, buf_size);
  EXPECT_EQ(buf_size*2, out_frames);

  in_buff = (int16_t *)ralloc(buf_size * 2 * cras_get_format_bytes(&in_fmt));
  out_buff = (int16_t *)ralloc(buf_size*2 * cras_get_format_bytes(&out_fmt));
  out_frames = cras_fmt_conv_convert_frames(c,
                                            (uint8_t *)in_buff,
                                            (uint8_t *)out_buff,
                                            &in_buf_size,
                                            buf_size * 2);
  cras_fmt_conv_destroy(c);
  free(in_buff);
  free(out_buff);
}

// Test 1 to 2 SRC with mono to stereo conversion.
TEST(FormatConverterTest,  Convert1To2MonoToStereo) {
  struct cras_fmt_conv *c;
  struct cras_audio_format in_fmt;
  struct cras_audio_format out_fmt;
  size_t out_frames;
  int16_t *in_buff;
  int16_t *out_buff;
  const size_t buf_size = 4096;
  unsigned int in_buf_size = 4096;

  ResetStub();
  in_fmt.format = out_fmt.format = SND_PCM_FORMAT_S16_LE;
  in_fmt.num_channels = 1;
  out_fmt.num_channels = 2;
  in_fmt.frame_rate = 22050;
  out_fmt.frame_rate = 44100;

  c = cras_fmt_conv_create(&in_fmt, &out_fmt, buf_size, 0);
  ASSERT_NE(c, (void *)NULL);

  out_frames = cras_fmt_conv_out_frames_to_in(c, buf_size);
  EXPECT_EQ(buf_size / 2, out_frames);

  out_frames = cras_fmt_conv_in_frames_to_out(c, buf_size);
  EXPECT_EQ(buf_size * 2, out_frames);

  in_buff = (int16_t *)ralloc(buf_size * 2 * cras_get_format_bytes(&in_fmt));
  out_buff = (int16_t *)ralloc(buf_size * 2 * cras_get_format_bytes(&out_fmt));
  out_frames = cras_fmt_conv_convert_frames(c,
                                            (uint8_t *)in_buff,
                                            (uint8_t *)out_buff,
                                            &in_buf_size,
                                            buf_size * 2);
  cras_fmt_conv_destroy(c);
  free(in_buff);
  free(out_buff);
}

// Test 32 to 16 bit conversion.
TEST(FormatConverterTest, ConvertS32LEToS16LE) {
  struct cras_fmt_conv *c;
  struct cras_audio_format in_fmt;
  struct cras_audio_format out_fmt;

  size_t out_frames;
  int32_t *in_buff;
  int16_t *out_buff;
  const size_t buf_size = 4096;
  unsigned int in_buf_size = 4096;

  ResetStub();
  in_fmt.format = SND_PCM_FORMAT_S32_LE;
  out_fmt.format = SND_PCM_FORMAT_S16_LE;
  in_fmt.num_channels = out_fmt.num_channels = 2;
  in_fmt.frame_rate = 48000;
  out_fmt.frame_rate = 48000;

  c = cras_fmt_conv_create(&in_fmt, &out_fmt, buf_size, 0);
  ASSERT_NE(c, (void *)NULL);

  out_frames = cras_fmt_conv_in_frames_to_out(c, buf_size);
  EXPECT_EQ(buf_size, out_frames);

  in_buff = (int32_t *)ralloc(buf_size * 2 * cras_get_format_bytes(&in_fmt));
  out_buff = (int16_t *)ralloc(buf_size * cras_get_format_bytes(&out_fmt));
  out_frames = cras_fmt_conv_convert_frames(c,
                                            (uint8_t *)in_buff,
                                            (uint8_t *)out_buff,
                                            &in_buf_size,
                                            buf_size);
  EXPECT_EQ(buf_size, out_frames);
  for (unsigned int i = 0; i < buf_size; i++)
    EXPECT_EQ((int16_t)(in_buff[i] >> 16), out_buff[i]);

  cras_fmt_conv_destroy(c);
  free(in_buff);
  free(out_buff);
}

// Test 24 to 16 bit conversion.
TEST(FormatConverterTest, ConvertS24LEToS16LE) {
  struct cras_fmt_conv *c;
  struct cras_audio_format in_fmt;
  struct cras_audio_format out_fmt;

  size_t out_frames;
  int32_t *in_buff;
  int16_t *out_buff;
  const size_t buf_size = 4096;
  unsigned int in_buf_size = 4096;

  ResetStub();
  in_fmt.format = SND_PCM_FORMAT_S24_LE;
  out_fmt.format = SND_PCM_FORMAT_S16_LE;
  in_fmt.num_channels = out_fmt.num_channels = 2;
  in_fmt.frame_rate = 48000;
  out_fmt.frame_rate = 48000;

  c = cras_fmt_conv_create(&in_fmt, &out_fmt, buf_size, 0);
  ASSERT_NE(c, (void *)NULL);

  out_frames = cras_fmt_conv_in_frames_to_out(c, buf_size);
  EXPECT_EQ(buf_size, out_frames);

  in_buff = (int32_t *)ralloc(buf_size * 2 * cras_get_format_bytes(&in_fmt));
  out_buff = (int16_t *)ralloc(buf_size * cras_get_format_bytes(&out_fmt));
  out_frames = cras_fmt_conv_convert_frames(c,
                                            (uint8_t *)in_buff,
                                            (uint8_t *)out_buff,
                                            &in_buf_size,
                                            buf_size);
  EXPECT_EQ(buf_size, out_frames);
  for (unsigned int i = 0; i < buf_size; i++)
    EXPECT_EQ((int16_t)(in_buff[i] >> 8), out_buff[i]);

  cras_fmt_conv_destroy(c);
  free(in_buff);
  free(out_buff);
}

// Test 8 to 16 bit conversion.
TEST(FormatConverterTest, ConvertU8LEToS16LE) {
  struct cras_fmt_conv *c;
  struct cras_audio_format in_fmt;
  struct cras_audio_format out_fmt;

  size_t out_frames;
  uint8_t *in_buff;
  int16_t *out_buff;
  const size_t buf_size = 4096;
  unsigned int in_buf_size = 4096;

  ResetStub();
  in_fmt.format = SND_PCM_FORMAT_U8;
  out_fmt.format = SND_PCM_FORMAT_S16_LE;
  in_fmt.num_channels = 2;
  out_fmt.num_channels = 2;
  in_fmt.frame_rate = 48000;
  out_fmt.frame_rate = 48000;

  c = cras_fmt_conv_create(&in_fmt, &out_fmt, buf_size, 0);
  ASSERT_NE(c, (void *)NULL);

  out_frames = cras_fmt_conv_in_frames_to_out(c, buf_size);
  EXPECT_EQ(buf_size, out_frames);

  in_buff = (uint8_t *)ralloc(buf_size * 2 * cras_get_format_bytes(&in_fmt));
  out_buff = (int16_t *)ralloc(buf_size * cras_get_format_bytes(&out_fmt));
  out_frames = cras_fmt_conv_convert_frames(c,
                                            (uint8_t *)in_buff,
                                            (uint8_t *)out_buff,
                                            &in_buf_size,
                                            buf_size);
  EXPECT_EQ(buf_size, out_frames);
  for (unsigned int i = 0; i < buf_size; i++)
    EXPECT_EQ(((int16_t)(in_buff[i] - 128) << 8), out_buff[i]);

  cras_fmt_conv_destroy(c);
  free(in_buff);
  free(out_buff);
}

// Test 16 to 32 bit conversion.
TEST(FormatConverterTest, ConvertS16LEToS32LE) {
  struct cras_fmt_conv *c;
  struct cras_audio_format in_fmt;
  struct cras_audio_format out_fmt;

  size_t out_frames;
  int16_t *in_buff;
  int32_t *out_buff;
  const size_t buf_size = 4096;
  unsigned int in_buf_size = 4096;

  ResetStub();
  in_fmt.format = SND_PCM_FORMAT_S16_LE;
  out_fmt.format = SND_PCM_FORMAT_S32_LE;
  in_fmt.num_channels = out_fmt.num_channels = 2;
  in_fmt.frame_rate = 48000;
  out_fmt.frame_rate = 48000;

  c = cras_fmt_conv_create(&in_fmt, &out_fmt, buf_size, 0);
  ASSERT_NE(c, (void *)NULL);

  out_frames = cras_fmt_conv_in_frames_to_out(c, buf_size);
  EXPECT_EQ(buf_size, out_frames);

  in_buff = (int16_t *)ralloc(buf_size * cras_get_format_bytes(&in_fmt));
  out_buff = (int32_t *)ralloc(buf_size * cras_get_format_bytes(&out_fmt));
  out_frames = cras_fmt_conv_convert_frames(c,
                                            (uint8_t *)in_buff,
                                            (uint8_t *)out_buff,
                                            &in_buf_size,
                                            buf_size);
  EXPECT_EQ(buf_size, out_frames);
  for (unsigned int i = 0; i < buf_size; i++)
    EXPECT_EQ(((int32_t)in_buff[i] << 16), out_buff[i]);

  cras_fmt_conv_destroy(c);
  free(in_buff);
  free(out_buff);
}

// Test 16 to 24 bit conversion.
TEST(FormatConverterTest, ConvertS16LEToS24LE) {
  struct cras_fmt_conv *c;
  struct cras_audio_format in_fmt;
  struct cras_audio_format out_fmt;

  size_t out_frames;
  int16_t *in_buff;
  int32_t *out_buff;
  const size_t buf_size = 4096;
  unsigned int in_buf_size = 4096;

  ResetStub();
  in_fmt.format = SND_PCM_FORMAT_S16_LE;
  out_fmt.format = SND_PCM_FORMAT_S24_LE;
  in_fmt.num_channels = out_fmt.num_channels = 2;
  in_fmt.frame_rate = 48000;
  out_fmt.frame_rate = 48000;

  c = cras_fmt_conv_create(&in_fmt, &out_fmt, buf_size, 0);
  ASSERT_NE(c, (void *)NULL);

  out_frames = cras_fmt_conv_in_frames_to_out(c, buf_size);
  EXPECT_EQ(buf_size, out_frames);

  in_buff = (int16_t *)ralloc(buf_size * cras_get_format_bytes(&in_fmt));
  out_buff = (int32_t *)ralloc(buf_size * 2 * cras_get_format_bytes(&out_fmt));
  out_frames = cras_fmt_conv_convert_frames(c,
                                            (uint8_t *)in_buff,
                                            (uint8_t *)out_buff,
                                            &in_buf_size,
                                            buf_size);
  EXPECT_EQ(buf_size, out_frames);
  for (unsigned int i = 0; i < buf_size; i++)
    EXPECT_EQ(((int32_t)in_buff[i] << 8), out_buff[i]);

  cras_fmt_conv_destroy(c);
  free(in_buff);
  free(out_buff);
}

// Test 16 to 8 bit conversion.
TEST(FormatConverterTest, ConvertS16LEToU8) {
  struct cras_fmt_conv *c;
  struct cras_audio_format in_fmt;
  struct cras_audio_format out_fmt;

  size_t out_frames;
  int16_t *in_buff;
  uint8_t *out_buff;
  const size_t buf_size = 4096;
  unsigned int in_buf_size = 4096;

  ResetStub();
  in_fmt.format = SND_PCM_FORMAT_S16_LE;
  out_fmt.format = SND_PCM_FORMAT_U8;
  in_fmt.num_channels = 2;
  out_fmt.num_channels = 2;
  in_fmt.frame_rate = 48000;
  out_fmt.frame_rate = 48000;

  c = cras_fmt_conv_create(&in_fmt, &out_fmt, buf_size, 0);
  ASSERT_NE(c, (void *)NULL);

  out_frames = cras_fmt_conv_in_frames_to_out(c, buf_size);
  EXPECT_EQ(buf_size, out_frames);

  in_buff = (int16_t *)ralloc(buf_size * cras_get_format_bytes(&in_fmt));
  out_buff = (uint8_t *)ralloc(buf_size * cras_get_format_bytes(&out_fmt));
  out_frames = cras_fmt_conv_convert_frames(c,
                                            (uint8_t *)in_buff,
                                            (uint8_t *)out_buff,
                                            &in_buf_size,
                                            buf_size);
  EXPECT_EQ(buf_size, out_frames);
  for (unsigned int i = 0; i < buf_size; i++)
    EXPECT_EQ((in_buff[i] >> 8) + 128, out_buff[i]);

  cras_fmt_conv_destroy(c);
  free(in_buff);
  free(out_buff);
}

// Test 32 bit 5.1 to 16 bit stereo conversion.
TEST(FormatConverterTest, ConvertS32LEToS16LEDownmix51ToStereo) {
  struct cras_fmt_conv *c;
  struct cras_audio_format in_fmt;
  struct cras_audio_format out_fmt;

  size_t out_frames;
  int32_t *in_buff;
  int16_t *out_buff;
  const size_t buf_size = 4096;
  unsigned int in_buf_size = 4096;
  int i;

  ResetStub();
  in_fmt.format = SND_PCM_FORMAT_S32_LE;
  out_fmt.format = SND_PCM_FORMAT_S16_LE;
  in_fmt.num_channels = 6;
  out_fmt.num_channels = 2;
  in_fmt.frame_rate = 48000;
  out_fmt.frame_rate = 48000;
  for (i = 0; i < CRAS_CH_MAX; i++)
    in_fmt.channel_layout[i] = surround_channel_layout[i];

  c = cras_fmt_conv_create(&in_fmt, &out_fmt, buf_size, 0);
  ASSERT_NE(c, (void *)NULL);

  out_frames = cras_fmt_conv_in_frames_to_out(c, buf_size);
  EXPECT_EQ(buf_size, out_frames);

  in_buff = (int32_t *)ralloc(buf_size * 2 * cras_get_format_bytes(&in_fmt));
  out_buff = (int16_t *)ralloc(buf_size * cras_get_format_bytes(&out_fmt));
  out_frames = cras_fmt_conv_convert_frames(c,
                                            (uint8_t *)in_buff,
                                            (uint8_t *)out_buff,
                                            &in_buf_size,
                                            buf_size);
  EXPECT_EQ(buf_size, out_frames);

  cras_fmt_conv_destroy(c);
  free(in_buff);
  free(out_buff);
}

// Test 16 bit stereo to 5.1 conversion.
TEST(FormatConverterTest, ConvertS16LEToS16LEStereoTo51) {
  struct cras_fmt_conv *c;
  struct cras_audio_format in_fmt;
  struct cras_audio_format out_fmt;

  size_t out_frames;
  int16_t *in_buff;
  int16_t *out_buff;
  const size_t buf_size = 4096;
  unsigned int in_buf_size = 4096;
  int i;

  ResetStub();
  in_fmt.format = SND_PCM_FORMAT_S16_LE;
  out_fmt.format = SND_PCM_FORMAT_S16_LE;
  in_fmt.num_channels = 2;
  out_fmt.num_channels = 6;
  in_fmt.frame_rate = 48000;
  out_fmt.frame_rate = 48000;
  for (i = 0; i < CRAS_CH_MAX; i++)
    out_fmt.channel_layout[i] = surround_channel_layout[i];

  c = cras_fmt_conv_create(&in_fmt, &out_fmt, buf_size, 0);
  ASSERT_NE(c, (void *)NULL);

  out_frames = cras_fmt_conv_in_frames_to_out(c, buf_size);
  EXPECT_EQ(buf_size, out_frames);

  in_buff = (int16_t *)ralloc(buf_size * cras_get_format_bytes(&in_fmt));
  out_buff = (int16_t *)ralloc(buf_size * cras_get_format_bytes(&out_fmt));
  out_frames = cras_fmt_conv_convert_frames(c,
                                            (uint8_t *)in_buff,
                                            (uint8_t *)out_buff,
                                            &in_buf_size,
                                            buf_size);
  EXPECT_EQ(buf_size, out_frames);
  for (unsigned int i = 0; i < buf_size; i++) {
    /* Check mono be converted to CRAS_CH_FL and CRAS_CH_FR */
    EXPECT_EQ(in_buff[2 * i], out_buff[6 * i]);
    EXPECT_EQ(in_buff[2 * i + 1], out_buff[6 * i + 1]);
    EXPECT_EQ(0, out_buff[6 * i + 2]);
    EXPECT_EQ(0, out_buff[6 * i + 3]);
    EXPECT_EQ(0, out_buff[6 * i + 4]);
    EXPECT_EQ(0, out_buff[6 * i + 5]);
  }

  cras_fmt_conv_destroy(c);
  free(in_buff);
  free(out_buff);
}

// Test 16 bit mono to 5.1 conversion.
TEST(FormatConverterTest, ConvertS16LEToS16LEMonoTo51) {
  struct cras_fmt_conv *c;
  struct cras_audio_format in_fmt;
  struct cras_audio_format out_fmt;

  size_t out_frames;
  int16_t *in_buff;
  int16_t *out_buff;
  const size_t buf_size = 4096;
  unsigned int in_buf_size = 4096;
  int i;

  ResetStub();
  in_fmt.format = SND_PCM_FORMAT_S16_LE;
  out_fmt.format = SND_PCM_FORMAT_S16_LE;
  in_fmt.num_channels = 1;
  out_fmt.num_channels = 6;
  in_fmt.frame_rate = 48000;
  out_fmt.frame_rate = 48000;
  for (i = 0; i < CRAS_CH_MAX; i++)
    out_fmt.channel_layout[i] = surround_channel_layout[i];

  c = cras_fmt_conv_create(&in_fmt, &out_fmt, buf_size, 0);
  ASSERT_NE(c, (void *)NULL);

  out_frames = cras_fmt_conv_in_frames_to_out(c, buf_size);
  EXPECT_EQ(buf_size, out_frames);

  in_buff = (int16_t *)ralloc(buf_size * cras_get_format_bytes(&in_fmt));
  out_buff = (int16_t *)ralloc(buf_size * cras_get_format_bytes(&out_fmt));
  out_frames = cras_fmt_conv_convert_frames(c,
                                            (uint8_t *)in_buff,
                                            (uint8_t *)out_buff,
                                            &in_buf_size,
                                            buf_size);
  EXPECT_EQ(buf_size, out_frames);
  for (unsigned int i = 0; i < buf_size; i++) {
    /* Check mono be converted to CRAS_CH_FC */
    EXPECT_EQ(in_buff[i], out_buff[6 * i + 4]);
    EXPECT_EQ(0, out_buff[6 * i + 0]);
    EXPECT_EQ(0, out_buff[6 * i + 1]);
    EXPECT_EQ(0, out_buff[6 * i + 2]);
    EXPECT_EQ(0, out_buff[6 * i + 3]);
    EXPECT_EQ(0, out_buff[6 * i + 5]);
  }

  cras_fmt_conv_destroy(c);
  free(in_buff);
  free(out_buff);
}

// Test 32 bit 5.1 to 16 bit stereo conversion with SRC 1 to 2.
TEST(FormatConverterTest, ConvertS32LEToS16LEDownmix51ToStereo48To96) {
  struct cras_fmt_conv *c;
  struct cras_audio_format in_fmt;
  struct cras_audio_format out_fmt;

  size_t out_frames;
  int32_t *in_buff;
  int16_t *out_buff;
  const size_t buf_size = 4096;
  unsigned int in_buf_size = 4096;
  int i;

  ResetStub();
  in_fmt.format = SND_PCM_FORMAT_S32_LE;
  out_fmt.format = SND_PCM_FORMAT_S16_LE;
  in_fmt.num_channels = 6;
  out_fmt.num_channels = 2;
  in_fmt.frame_rate = 48000;
  out_fmt.frame_rate = 96000;
  for (i = 0; i < CRAS_CH_MAX; i++)
    in_fmt.channel_layout[i] = surround_channel_layout[i];

  c = cras_fmt_conv_create(&in_fmt, &out_fmt, buf_size, 0);
  ASSERT_NE(c, (void *)NULL);

  out_frames = cras_fmt_conv_in_frames_to_out(c, buf_size);
  EXPECT_EQ(buf_size * 2, out_frames);

  in_buff = (int32_t *)ralloc(buf_size * cras_get_format_bytes(&in_fmt));
  out_buff = (int16_t *)ralloc(buf_size * 2 * cras_get_format_bytes(&out_fmt));
  out_frames = cras_fmt_conv_convert_frames(c,
                                            (uint8_t *)in_buff,
                                            (uint8_t *)out_buff,
                                            &in_buf_size,
                                            buf_size * 2);
  EXPECT_EQ(buf_size * 2, out_frames);

  cras_fmt_conv_destroy(c);
  free(in_buff);
  free(out_buff);
}

// Test 32 bit 5.1 to 16 bit stereo conversion with SRC 2 to 1.
TEST(FormatConverterTest, ConvertS32LEToS16LEDownmix51ToStereo96To48) {
  struct cras_fmt_conv *c;
  struct cras_audio_format in_fmt;
  struct cras_audio_format out_fmt;

  size_t out_frames;
  int32_t *in_buff;
  int16_t *out_buff;
  const size_t buf_size = 4096;
  unsigned int in_buf_size = 4096;
  int i;

  ResetStub();
  in_fmt.format = SND_PCM_FORMAT_S32_LE;
  out_fmt.format = SND_PCM_FORMAT_S16_LE;
  in_fmt.num_channels = 6;
  out_fmt.num_channels = 2;
  in_fmt.frame_rate = 96000;
  out_fmt.frame_rate = 48000;
  for (i = 0; i < CRAS_CH_MAX; i++)
    in_fmt.channel_layout[i] = surround_channel_layout[i];

  c = cras_fmt_conv_create(&in_fmt, &out_fmt, buf_size, 0);
  ASSERT_NE(c, (void *)NULL);

  out_frames = cras_fmt_conv_in_frames_to_out(c, buf_size);
  EXPECT_EQ(buf_size / 2, out_frames);

  in_buff = (int32_t *)ralloc(buf_size * cras_get_format_bytes(&in_fmt));
  out_buff = (int16_t *)ralloc(buf_size / 2 * cras_get_format_bytes(&out_fmt));
  out_frames = cras_fmt_conv_convert_frames(c,
                                            (uint8_t *)in_buff,
                                            (uint8_t *)out_buff,
                                            &in_buf_size,
                                            buf_size / 2);
  EXPECT_EQ(buf_size / 2, out_frames);

  cras_fmt_conv_destroy(c);
  free(in_buff);
  free(out_buff);
}

// Test 32 bit 5.1 to 16 bit stereo conversion with SRC 48 to 44.1.
TEST(FormatConverterTest, ConvertS32LEToS16LEDownmix51ToStereo48To441) {
  struct cras_fmt_conv *c;
  struct cras_audio_format in_fmt;
  struct cras_audio_format out_fmt;

  size_t out_frames;
  size_t ret_frames;
  int32_t *in_buff;
  int16_t *out_buff;
  const size_t buf_size = 4096;
  unsigned int in_buf_size = 4096;
  int i;

  ResetStub();
  in_fmt.format = SND_PCM_FORMAT_S32_LE;
  out_fmt.format = SND_PCM_FORMAT_S16_LE;
  in_fmt.num_channels = 6;
  out_fmt.num_channels = 2;
  in_fmt.frame_rate = 48000;
  out_fmt.frame_rate = 44100;
  for (i = 0; i < CRAS_CH_MAX; i++)
    in_fmt.channel_layout[i] = surround_channel_layout[i];

  c = cras_fmt_conv_create(&in_fmt, &out_fmt, buf_size, 0);
  ASSERT_NE(c, (void *)NULL);

  out_frames = cras_fmt_conv_in_frames_to_out(c, buf_size);
  EXPECT_LT(out_frames, buf_size);

  in_buff = (int32_t *)ralloc(buf_size * cras_get_format_bytes(&in_fmt));
  out_buff = (int16_t *)ralloc(out_frames * cras_get_format_bytes(&out_fmt));
  ret_frames = cras_fmt_conv_convert_frames(c,
                                            (uint8_t *)in_buff,
                                            (uint8_t *)out_buff,
                                            &in_buf_size,
                                            out_frames);
  EXPECT_EQ(out_frames, ret_frames);

  cras_fmt_conv_destroy(c);
  free(in_buff);
  free(out_buff);
}

// Test 32 bit 5.1 to 16 bit stereo conversion with SRC 441 to 48.
TEST(FormatConverterTest, ConvertS32LEToS16LEDownmix51ToStereo441To48) {
  struct cras_fmt_conv *c;
  struct cras_audio_format in_fmt;
  struct cras_audio_format out_fmt;

  size_t out_frames;
  size_t ret_frames;
  int32_t *in_buff;
  int16_t *out_buff;
  const size_t buf_size = 4096;
  unsigned int in_buf_size = 4096;
  int i;

  ResetStub();
  in_fmt.format = SND_PCM_FORMAT_S32_LE;
  out_fmt.format = SND_PCM_FORMAT_S16_LE;
  in_fmt.num_channels = 6;
  out_fmt.num_channels = 2;
  in_fmt.frame_rate = 44100;
  out_fmt.frame_rate = 48000;
  for (i = 0; i < CRAS_CH_MAX; i++)
    in_fmt.channel_layout[i] = surround_channel_layout[i];

  c = cras_fmt_conv_create(&in_fmt, &out_fmt, buf_size, 0);
  ASSERT_NE(c, (void *)NULL);

  out_frames = cras_fmt_conv_in_frames_to_out(c, buf_size);
  EXPECT_GT(out_frames, buf_size);

  in_buff = (int32_t *)ralloc(buf_size * cras_get_format_bytes(&in_fmt));
  out_buff = (int16_t *)ralloc((out_frames - 1) *
                               cras_get_format_bytes(&out_fmt));
  ret_frames = cras_fmt_conv_convert_frames(c,
                                            (uint8_t *)in_buff,
                                            (uint8_t *)out_buff,
                                            &in_buf_size,
                                            out_frames - 1);
  EXPECT_EQ(out_frames - 1, ret_frames);

  cras_fmt_conv_destroy(c);
  free(in_buff);
  free(out_buff);
}

// Test Invalid buffer length just truncates.
TEST(FormatConverterTest, ConvertS32LEToS16LEDownmix51ToStereo96To48Short) {
  struct cras_fmt_conv *c;
  struct cras_audio_format in_fmt;
  struct cras_audio_format out_fmt;

  size_t out_frames;
  size_t ret_frames;
  int32_t *in_buff;
  int16_t *out_buff;
  const size_t buf_size = 4096;
  unsigned int in_buf_size = 4096;
  int i;

  ResetStub();
  in_fmt.format = SND_PCM_FORMAT_S32_LE;
  out_fmt.format = SND_PCM_FORMAT_S16_LE;
  in_fmt.num_channels = 6;
  out_fmt.num_channels = 2;
  in_fmt.frame_rate = 96000;
  out_fmt.frame_rate = 48000;
  for (i = 0; i < CRAS_CH_MAX; i++)
    in_fmt.channel_layout[i] = surround_channel_layout[i];

  c = cras_fmt_conv_create(&in_fmt, &out_fmt, buf_size, 0);
  ASSERT_NE(c, (void *)NULL);

  out_frames = cras_fmt_conv_in_frames_to_out(c, buf_size);
  EXPECT_EQ(buf_size / 2, out_frames);

  in_buff = (int32_t *)ralloc(buf_size * cras_get_format_bytes(&in_fmt));
  out_buff = (int16_t *)ralloc((out_frames - 2) *
                               cras_get_format_bytes(&out_fmt));
  ret_frames = cras_fmt_conv_convert_frames(c,
                                            (uint8_t *)in_buff,
                                            (uint8_t *)out_buff,
                                            &in_buf_size,
                                            out_frames - 2);
  EXPECT_EQ(out_frames - 2, ret_frames);

  cras_fmt_conv_destroy(c);
  free(in_buff);
  free(out_buff);
}

// Test format convert pre linear resample and then follows SRC from 96 to 48.
TEST(FormatConverterTest, Convert96to48PreLinearResample) {
  struct cras_fmt_conv *c;
  struct cras_audio_format in_fmt;
  struct cras_audio_format out_fmt;

  size_t out_frames;
  int32_t *in_buff;
  int16_t *out_buff;
  const size_t buf_size = 4096;
  unsigned int in_buf_size = 4096;
  unsigned int expected_fr;
  int i;

  ResetStub();
  in_fmt.format = SND_PCM_FORMAT_S16_LE;
  out_fmt.format = SND_PCM_FORMAT_S16_LE;
  in_fmt.num_channels = 2;
  out_fmt.num_channels = 2;
  in_fmt.frame_rate = 96000;
  out_fmt.frame_rate = 48000;
  for (i = 0; i < CRAS_CH_MAX; i++) {
    in_fmt.channel_layout[i] = surround_channel_layout[i];
    out_fmt.channel_layout[i] = surround_channel_layout[i];
  }

  c = cras_fmt_conv_create(&in_fmt, &out_fmt, buf_size * 2, 1);
  ASSERT_NE(c, (void *)NULL);

  linear_resampler_needed_val = 1;
  linear_resampler_ratio = 1.01;
  expected_fr = buf_size / 2 * linear_resampler_ratio;
  out_frames = cras_fmt_conv_in_frames_to_out(c, buf_size);
  EXPECT_EQ(expected_fr, out_frames);

  in_buff = (int32_t *)ralloc(buf_size * cras_get_format_bytes(&in_fmt));
  out_buff = (int16_t *)ralloc(buf_size * cras_get_format_bytes(&out_fmt));
  out_frames = cras_fmt_conv_convert_frames(c,
                                            (uint8_t *)in_buff,
                                            (uint8_t *)out_buff,
                                            &in_buf_size,
                                            out_frames);
  EXPECT_EQ(expected_fr, out_frames);

  cras_fmt_conv_destroy(c);
  free(in_buff);
  free(out_buff);
}

// Test format convert SRC from 96 to 48 and then post linear resample.
TEST(FormatConverterTest, Convert96to48PostLinearResample) {
  struct cras_fmt_conv *c;
  struct cras_audio_format in_fmt;
  struct cras_audio_format out_fmt;

  size_t out_frames;
  int32_t *in_buff;
  int16_t *out_buff;
  const size_t buf_size = 4096;
  unsigned int in_buf_size = 4096;
  unsigned int expected_fr;
  int i;

  ResetStub();
  in_fmt.format = SND_PCM_FORMAT_S16_LE;
  out_fmt.format = SND_PCM_FORMAT_S16_LE;
  in_fmt.num_channels = 2;
  out_fmt.num_channels = 2;
  in_fmt.frame_rate = 96000;
  out_fmt.frame_rate = 48000;
  for (i = 0; i < CRAS_CH_MAX; i++) {
    in_fmt.channel_layout[i] = surround_channel_layout[i];
    out_fmt.channel_layout[i] = surround_channel_layout[i];
  }

  c = cras_fmt_conv_create(&in_fmt, &out_fmt, buf_size * 2, 0);
  ASSERT_NE(c, (void *)NULL);

  linear_resampler_needed_val = 1;
  linear_resampler_ratio = 0.99;
  expected_fr = buf_size / 2 * linear_resampler_ratio;
  out_frames = cras_fmt_conv_in_frames_to_out(c, buf_size);
  EXPECT_EQ(expected_fr, out_frames);

  in_buff = (int32_t *)ralloc(buf_size * cras_get_format_bytes(&in_fmt));
  out_buff = (int16_t *)ralloc(buf_size * cras_get_format_bytes(&out_fmt));
  out_frames = cras_fmt_conv_convert_frames(c,
                                            (uint8_t *)in_buff,
                                            (uint8_t *)out_buff,
                                            &in_buf_size,
                                            buf_size);
  EXPECT_EQ(expected_fr, out_frames);

  cras_fmt_conv_destroy(c);
  free(in_buff);
  free(out_buff);
}

// Test format converter created in config_format_converter
TEST(FormatConverterTest, ConfigConverter) {
  int i;
  struct cras_fmt_conv *c = NULL;
  struct cras_audio_format in_fmt;
  struct cras_audio_format out_fmt;

  ResetStub();
  in_fmt.format = SND_PCM_FORMAT_S16_LE;
  out_fmt.format = SND_PCM_FORMAT_S16_LE;
  in_fmt.num_channels = 1;
  out_fmt.num_channels = 2;
  in_fmt.frame_rate = 96000;
  out_fmt.frame_rate = 48000;
  for (i = 0; i < CRAS_CH_MAX; i++) {
    in_fmt.channel_layout[i] = mono_channel_layout[i];
    out_fmt.channel_layout[i] = stereo_channel_layout[i];
  }

  config_format_converter(&c, CRAS_STREAM_OUTPUT, &in_fmt, &out_fmt, 4096);
  ASSERT_NE(c, (void *)NULL);

  cras_fmt_conv_destroy(c);
}

// Test format converter not created when in/out format conversion is not
// needed.
TEST(FormatConverterTest, ConfigConverterNoNeed) {
  int i;
  struct cras_fmt_conv *c = NULL;
  struct cras_audio_format in_fmt;
  struct cras_audio_format out_fmt;

  ResetStub();
  in_fmt.format = SND_PCM_FORMAT_S16_LE;
  out_fmt.format = SND_PCM_FORMAT_S16_LE;
  in_fmt.num_channels = 2;
  out_fmt.num_channels = 2;
  in_fmt.frame_rate = 48000;
  out_fmt.frame_rate = 48000;
  for (i = 0; i < CRAS_CH_MAX; i++) {
    in_fmt.channel_layout[i] = stereo_channel_layout[i];
    out_fmt.channel_layout[i] = stereo_channel_layout[i];
  }

  config_format_converter(&c, CRAS_STREAM_OUTPUT, &in_fmt, &out_fmt, 4096);
  EXPECT_NE(c, (void *)NULL);
  EXPECT_EQ(0, cras_fmt_conversion_needed(c));
}

// Test format converter not created for input when in/out format differs
// at channel count or layout.
TEST(FormatConverterTest, ConfigConverterNoNeedForInput) {
  static int kmic_channel_layout[CRAS_CH_MAX] =
    {0, 1, -1, -1, 2, -1, -1, -1, -1, -1, -1};
  int i;
  struct cras_fmt_conv *c = NULL;
  struct cras_audio_format in_fmt;
  struct cras_audio_format out_fmt;

  ResetStub();
  in_fmt.format = SND_PCM_FORMAT_S16_LE;
  out_fmt.format = SND_PCM_FORMAT_S16_LE;
  in_fmt.num_channels = 2;
  out_fmt.num_channels = 3;
  in_fmt.frame_rate = 48000;
  out_fmt.frame_rate = 48000;
  for (i = 0; i < CRAS_CH_MAX; i++) {
    in_fmt.channel_layout[i] = stereo_channel_layout[i];
    out_fmt.channel_layout[i] = kmic_channel_layout[i];
  }

  config_format_converter(&c, CRAS_STREAM_INPUT, &in_fmt, &out_fmt, 4096);
  EXPECT_NE(c, (void *)NULL);
  EXPECT_EQ(0, cras_fmt_conversion_needed(c));
}

TEST(ChannelRemixTest, ChannelRemixAppliedOrNot) {
  float coeff[4] = {0.5, 0.5, 0.26, 0.73};
  struct cras_fmt_conv *conv;
  struct cras_audio_format fmt;
  int16_t *buf, *res;
  unsigned i;

  fmt.num_channels = 2;
  conv = cras_channel_remix_conv_create(2, coeff);

  buf = (int16_t *)ralloc(50 * 4);
  res = (int16_t *)malloc(50 * 4);

  for (i = 0; i < 100; i += 2) {
    res[i] = coeff[0] * buf[i];
    res[i] += coeff[1] * buf[i + 1];
    res[i + 1] = coeff[2] * buf[i];
    res[i + 1] += coeff[3] * buf[i + 1];
  }

  cras_channel_remix_convert(conv, &fmt, (uint8_t *)buf, 50);
  for (i = 0; i < 100; i++)
    EXPECT_EQ(res[i],  buf[i]);

  /* If num_channels not match, remix conversion will not apply. */
  fmt.num_channels = 6;
  cras_channel_remix_convert(conv, &fmt, (uint8_t *)buf, 50);
  for (i = 0; i < 100; i++)
    EXPECT_EQ(res[i],  buf[i]);

  cras_fmt_conv_destroy(conv);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

extern "C" {
float** cras_channel_conv_matrix_alloc(size_t in_ch, size_t out_ch)
{
  int i;
  float** conv_mtx;
  conv_mtx = (float **)calloc(CRAS_CH_MAX, sizeof(*conv_mtx));
  for (i = 0; i < CRAS_CH_MAX; i++)
    conv_mtx[i] = (float *)calloc(CRAS_CH_MAX, sizeof(*conv_mtx[i]));
  return conv_mtx;
}
void cras_channel_conv_matrix_destroy(float **mtx, size_t out_ch)
{
  int i;
  for (i = 0; i < CRAS_CH_MAX; i++)
    free(mtx[i]);
  free(mtx);
}
float **cras_channel_conv_matrix_create(const struct cras_audio_format *in,
					const struct cras_audio_format *out)
{
  return cras_channel_conv_matrix_alloc(in->num_channels,
					out->num_channels);
}
struct linear_resampler *linear_resampler_create(unsigned int num_channels,
             unsigned int format_bytes,
             float src_rate,
             float dst_rate)
{
  return reinterpret_cast<struct linear_resampler*>(0x33);;
}

int linear_resampler_needed(struct linear_resampler *lr)
{
  return linear_resampler_needed_val;
}

void linear_resampler_set_rates(struct linear_resampler *lr,
                                unsigned int from,
                                unsigned int to)
{
}

unsigned int linear_resampler_out_frames_to_in(struct linear_resampler *lr,
                                               unsigned int frames)
{
  return (double)frames / linear_resampler_ratio;
}

/* Converts the frames count from input rate to output rate. */
unsigned int linear_resampler_in_frames_to_out(struct linear_resampler *lr,
                                               unsigned int frames)
{
  return (double)frames * linear_resampler_ratio;
}

unsigned int linear_resampler_resample(struct linear_resampler *lr,
           uint8_t *src,
           unsigned int *src_frames,
           uint8_t *dst,
           unsigned dst_frames)
{
  unsigned int resampled_fr = *src_frames * linear_resampler_ratio;

  if (resampled_fr > dst_frames) {
    resampled_fr = dst_frames;
    *src_frames = dst_frames / linear_resampler_ratio;
  }

  return resampled_fr;
}

void linear_resampler_destroy(struct linear_resampler *lr)
{
}
} // extern "C"
