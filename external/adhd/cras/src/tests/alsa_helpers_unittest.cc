// Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>
#include <vector>

extern "C" {
// For static function test.
#include "cras_alsa_helpers.c"
}

static int snd_pcm_sw_params_set_tstamp_type_called;
static int snd_pcm_sw_params_set_tstamp_mode_called;
static snd_pcm_uframes_t snd_pcm_htimestamp_avail_ret_val;
static timespec snd_pcm_htimestamp_tstamp_ret_val;
static std::vector<int> snd_pcm_sw_params_ret_vals;

static void ResetStubData() {
  snd_pcm_sw_params_set_tstamp_type_called = 0;
  snd_pcm_sw_params_set_tstamp_mode_called = 0;
  snd_pcm_htimestamp_avail_ret_val = 0;
  snd_pcm_htimestamp_tstamp_ret_val.tv_sec = 0;
  snd_pcm_htimestamp_tstamp_ret_val.tv_nsec = 0;
  snd_pcm_sw_params_ret_vals.clear();
}

namespace {

static snd_pcm_chmap_query_t *create_chmap_cap(snd_pcm_chmap_type type,
					       size_t channels)
{
  snd_pcm_chmap_query_t *c;
  c = (snd_pcm_chmap_query_t *)calloc(channels + 2, sizeof(int));
  c->type = type;
  c->map.channels = channels;
  return c;
}

TEST(AlsaHelper, MatchChannelMapCapabilityStereo) {
  snd_pcm_chmap_query_t **caps;
  snd_pcm_chmap_query_t *c;
  struct cras_audio_format *fmt;

  caps = (snd_pcm_chmap_query_t **)calloc(4, sizeof(*caps));

  /* Layout (CRAS_CH_RL, CRAS_CH_RR) corresponds to
   * ALSA channel map (5, 6)
   */
  int8_t channel_layout[CRAS_CH_MAX] =
      {-1, -1, 0, 1, -1, -1, -1, -1, -1, -1, -1};

  fmt = cras_audio_format_create(SND_PCM_FORMAT_S16_LE, 44100, 2);
  cras_audio_format_set_channel_layout(fmt, channel_layout);

  /* Create a list of capabilities */
  c = create_chmap_cap(SND_CHMAP_TYPE_FIXED, 3);
  c->map.pos[0] = 3;
  c->map.pos[1] = 4;
  c->map.pos[2] = 5;
  caps[0] = c;

  c = create_chmap_cap(SND_CHMAP_TYPE_VAR, 2);
  c->map.pos[0] = 5;
  c->map.pos[1] = 6;
  caps[1] = c;

  c = create_chmap_cap(SND_CHMAP_TYPE_VAR, 2);
  c->map.pos[0] = 9;
  c->map.pos[1] = 10;
  caps[2] = c;

  caps[3] = NULL;

  /* Test if there's a cap matches fmt */
  c = cras_chmap_caps_match(caps, fmt);
  ASSERT_NE((void *)NULL, c);

  caps[1]->map.pos[0] = 5;
  caps[1]->map.pos[1] = 7;

  c = cras_chmap_caps_match(caps, fmt);
  ASSERT_EQ((void *)NULL, c);

  free(caps[0]);
  free(caps[1]);
  free(caps[2]);
  free(caps[3]);
  free(caps);
  cras_audio_format_destroy(fmt);
}

TEST(AlsaHelper, MatchChannelMapCapability51) {
  snd_pcm_chmap_query_t **caps = NULL;
  snd_pcm_chmap_query_t *c = NULL;
  struct cras_audio_format *fmt;

  caps = (snd_pcm_chmap_query_t **)calloc(4, sizeof(*caps));

  /* Layout (CRAS_CH_FL, CRAS_CH_FR, CRAS_CH_RL, CRAS_CH_RR, CRAS_CH_FC)
   * corresponds to ALSA channel map (3, 4, 5, 6, 7)
   */
  int8_t channel_layout[CRAS_CH_MAX] =
      {0, 1, 2, 3, 4, 5, -1, -1, -1, -1, -1};

  fmt = cras_audio_format_create(SND_PCM_FORMAT_S16_LE, 44100, 6);
  cras_audio_format_set_channel_layout(fmt, channel_layout);

  /* Create a list of capabilities */
  c = create_chmap_cap(SND_CHMAP_TYPE_FIXED, 6);
  c->map.pos[0] = 3;
  c->map.pos[1] = 4;
  c->map.pos[2] = 5;
  c->map.pos[3] = 6;
  c->map.pos[4] = 7;
  c->map.pos[5] = 8;
  caps[0] = c;

  c = create_chmap_cap(SND_CHMAP_TYPE_VAR, 2);
  c->map.pos[0] = 6;
  c->map.pos[1] = 4;
  caps[1] = c;

  c = create_chmap_cap(SND_CHMAP_TYPE_VAR, 6);
  c->map.pos[0] = 9;
  c->map.pos[1] = 10;
  c->map.pos[2] = 5;
  c->map.pos[3] = 6;
  c->map.pos[4] = 7;
  c->map.pos[5] = 8;
  caps[2] = c;
  caps[3] = NULL;

  /* Test if there's a cap matches fmt */
  c = cras_chmap_caps_match(caps, fmt);
  ASSERT_NE((void *)NULL, c);

  caps[0]->map.pos[0] = 7;
  caps[0]->map.pos[1] = 8;
  caps[0]->map.pos[4] = 3;
  caps[0]->map.pos[5] = 4;
  c = cras_chmap_caps_match(caps, fmt);
  ASSERT_EQ((void *)NULL, c);

  caps[0]->type = SND_CHMAP_TYPE_PAIRED;
  c = cras_chmap_caps_match(caps, fmt);
  ASSERT_NE((void *)NULL, c);

  caps[0]->map.pos[0] = 8;
  caps[0]->map.pos[1] = 7;
  c = cras_chmap_caps_match(caps, fmt);
  ASSERT_EQ((void *)NULL, c);

  caps[0]->type = SND_CHMAP_TYPE_VAR;
  c = cras_chmap_caps_match(caps, fmt);
  ASSERT_NE((void *)NULL, c);

  free(caps[0]);
  free(caps[1]);
  free(caps[2]);
  free(caps[3]);
  free(caps);
  cras_audio_format_destroy(fmt);
}

TEST(AlsaHelper, Htimestamp) {
  snd_pcm_t *dummy_handle = reinterpret_cast<snd_pcm_t*>(0x1);
  snd_pcm_uframes_t used;
  snd_pcm_uframes_t severe_underrun_frames = 480;
  struct timespec tstamp;
  unsigned int underruns = 0;
  int htimestamp_enabled = 1;
  const char *dev_name = "dev_name";

  // Enable htimestamp use.
  ResetStubData();
  EXPECT_EQ(0, cras_alsa_set_swparams(dummy_handle, &htimestamp_enabled));
  EXPECT_EQ(snd_pcm_sw_params_set_tstamp_mode_called, 1);
  EXPECT_EQ(snd_pcm_sw_params_set_tstamp_type_called, 1);
  EXPECT_EQ(1, htimestamp_enabled);

  // Try to enable htimestamp use: not supported.
  ResetStubData();
  snd_pcm_sw_params_ret_vals.push_back(-EINVAL);
  EXPECT_EQ(0, cras_alsa_set_swparams(dummy_handle, &htimestamp_enabled));
  EXPECT_EQ(snd_pcm_sw_params_set_tstamp_mode_called, 2);
  EXPECT_EQ(snd_pcm_sw_params_set_tstamp_type_called, 2);
  EXPECT_EQ(0, htimestamp_enabled);

  // Disable htimestamp use.
  ResetStubData();
  EXPECT_EQ(0, cras_alsa_set_swparams(dummy_handle, &htimestamp_enabled));
  EXPECT_EQ(snd_pcm_sw_params_set_tstamp_mode_called, 0);
  EXPECT_EQ(snd_pcm_sw_params_set_tstamp_type_called, 0);

  ResetStubData();
  tstamp.tv_sec = 0;
  tstamp.tv_nsec = 0;
  snd_pcm_htimestamp_avail_ret_val = 20000;
  snd_pcm_htimestamp_tstamp_ret_val.tv_sec = 10;
  snd_pcm_htimestamp_tstamp_ret_val.tv_nsec = 10000;

  cras_alsa_get_avail_frames(dummy_handle, 48000, severe_underrun_frames,
                             dev_name, &used, &tstamp, &underruns);
  EXPECT_EQ(used, snd_pcm_htimestamp_avail_ret_val);
  EXPECT_EQ(tstamp.tv_sec, snd_pcm_htimestamp_tstamp_ret_val.tv_sec);
  EXPECT_EQ(tstamp.tv_nsec, snd_pcm_htimestamp_tstamp_ret_val.tv_nsec);
}

TEST(AlsaHelper, GetAvailFramesSevereUnderrun) {
  snd_pcm_t *dummy_handle = reinterpret_cast<snd_pcm_t*>(0x1);
  snd_pcm_uframes_t avail;
  snd_pcm_uframes_t severe_underrun_frames = 480;
  snd_pcm_uframes_t buffer_size = 48000;
  struct timespec tstamp;
  unsigned int underruns = 0;
  int rc;
  const char *dev_name = "dev_name";

  ResetStubData();
  snd_pcm_htimestamp_avail_ret_val = buffer_size + severe_underrun_frames + 1;
  rc = cras_alsa_get_avail_frames(dummy_handle, buffer_size,
                                  severe_underrun_frames, dev_name,
                                  &avail, &tstamp, &underruns);
  // Returns -EPIPE when severe underrun happens.
  EXPECT_EQ(rc, -EPIPE);
  EXPECT_EQ(1, underruns);

  ResetStubData();
  snd_pcm_htimestamp_avail_ret_val = buffer_size + severe_underrun_frames;
  rc = cras_alsa_get_avail_frames(dummy_handle, buffer_size,
                                  severe_underrun_frames, dev_name,
                                  &avail, &tstamp, &underruns);
  // Underrun which is not severe enough will be masked.
  // avail will be adjusted to buffer_size.
  EXPECT_EQ(avail, buffer_size);
  EXPECT_EQ(rc, 0);
  EXPECT_EQ(2, underruns);

  ResetStubData();
  snd_pcm_htimestamp_avail_ret_val = buffer_size;
  rc = cras_alsa_get_avail_frames(dummy_handle, buffer_size,
                                  severe_underrun_frames, dev_name,
                                  &avail, &tstamp, &underruns);
  // When avail == buffer_size, num_underruns will be increased.
  EXPECT_EQ(avail, buffer_size);
  EXPECT_EQ(rc, 0);
  EXPECT_EQ(3, underruns);

  ResetStubData();
  snd_pcm_htimestamp_avail_ret_val = buffer_size - 1;
  rc = cras_alsa_get_avail_frames(dummy_handle, buffer_size,
                                  severe_underrun_frames, dev_name,
                                  &avail, &tstamp, &underruns);
  // When avail < buffer_size, there is no underrun.
  EXPECT_EQ(avail, buffer_size - 1);
  EXPECT_EQ(rc, 0);
  EXPECT_EQ(3, underruns);
}
} // namespace

extern "C" {

int snd_pcm_sw_params_current(snd_pcm_t *pcm, snd_pcm_sw_params_t *params) {
  return 0;
}

int snd_pcm_sw_params_get_boundary(const snd_pcm_sw_params_t *params,
                                   snd_pcm_uframes_t *val) {
  return 0;
}

int snd_pcm_sw_params_set_stop_threshold(snd_pcm_t *pcm,
                                         snd_pcm_sw_params_t *params,
                                         snd_pcm_uframes_t val) {
  return 0;
}

int snd_pcm_sw_params_set_start_threshold(snd_pcm_t *pcm,
                                          snd_pcm_sw_params_t *params,
                                          snd_pcm_uframes_t val) {
  return 0;
}

int snd_pcm_sw_params_set_period_event(snd_pcm_t *pcm,
                                       snd_pcm_sw_params_t *params, int val) {
  return 0;
}

int snd_pcm_sw_params_set_tstamp_mode(snd_pcm_t *pcm,
                                      snd_pcm_sw_params_t *params,
                                      snd_pcm_tstamp_t val) {
  snd_pcm_sw_params_set_tstamp_mode_called++;
  return 0;
}

int snd_pcm_sw_params_set_tstamp_type(snd_pcm_t *pcm,
                                      snd_pcm_sw_params_t *params,
                                      snd_pcm_tstamp_type_t val) {
  snd_pcm_sw_params_set_tstamp_type_called++;
  return 0;
}

int snd_pcm_sw_params(snd_pcm_t *pcm, snd_pcm_sw_params_t *params) {
  int rc;

  if (snd_pcm_sw_params_ret_vals.size() == 0)
    return 0;
  rc = snd_pcm_sw_params_ret_vals.back();
  snd_pcm_sw_params_ret_vals.pop_back();
  return rc;
}

snd_pcm_sframes_t snd_pcm_avail(snd_pcm_t *pcm) {
  return snd_pcm_htimestamp_avail_ret_val;
}

int snd_pcm_htimestamp(snd_pcm_t *pcm, snd_pcm_uframes_t *avail,
                       snd_htimestamp_t *tstamp) {
  *avail = snd_pcm_htimestamp_avail_ret_val;
  *tstamp = snd_pcm_htimestamp_tstamp_ret_val;
  return 0;
}

}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
