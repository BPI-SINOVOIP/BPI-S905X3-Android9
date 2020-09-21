// Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>
#include <math.h>
#include "crossover.h"
#include "crossover2.h"
#include "drc.h"
#include "dsp_util.h"
#include "eq.h"
#include "eq2.h"

namespace {

/* Adds amplitude * sin(pi*freq*i + offset) to the data array. */
static void add_sine(float *data, size_t len, float freq, float offset,
                     float amplitude)
{
  for (size_t i = 0; i < len; i++)
    data[i] += amplitude * sinf((float)M_PI*freq*i + offset);
}

/* Calculates the magnitude at normalized frequency f. The output is
 * the result of DFT, multiplied by 2/len. */
static float magnitude_at(float *data, size_t len, float f)
{
  double re = 0, im = 0;
  f *= (float)M_PI;
  for (size_t i = 0; i < len; i++) {
    re += data[i] * cos(i * f);
    im += data[i] * sin(i * f);
  }
  return sqrt(re * re + im * im) * (2.0 / len);
}

TEST(InterleaveTest, All) {
  const int FRAMES = 12;
  const int SAMPLES = FRAMES * 2;

  /* Repeat the same data twice, so it will exercise neon/sse
   * optimized functions. */
  int16_t input[SAMPLES] = {
    -32768, -32767, -32766, -2, -1, 0, 1, 2, 3, 32765, 32766, 32767,
    -32768, -32767, -32766, -2, -1, 0, 1, 2, 3, 32765, 32766, 32767
  };

  float answer[SAMPLES] = {
    -1, -32766/32768.0f, -1/32768.0f, 1/32768.0f, 3/32768.0f, 32766/32768.0f,
    -1, -32766/32768.0f, -1/32768.0f, 1/32768.0f, 3/32768.0f, 32766/32768.0f,
    -32767/32768.0f, -2/32768.0f, 0, 2/32768.0f, 32765/32768.0f, 32767/32768.0f,
    -32767/32768.0f, -2/32768.0f, 0, 2/32768.0f, 32765/32768.0f, 32767/32768.0f
  };

  float output[SAMPLES];
  float *out_ptr[] = {output, output + FRAMES};

  dsp_util_deinterleave(input, out_ptr, 2, FRAMES);

  for (int i = 0 ; i < SAMPLES; i++) {
    EXPECT_EQ(answer[i], output[i]);
  }

  /* dsp_util_interleave() should round to nearest number. */
  for (int i = 0 ; i < SAMPLES; i += 2) {
    output[i] += 0.499 / 32768.0f;
    output[i + 1] -= 0.499 / 32768.0f;
  }

  int16_t output2[SAMPLES];
  dsp_util_interleave(out_ptr, output2, 2, FRAMES);
  for (int i = 0 ; i < SAMPLES; i++) {
    EXPECT_EQ(input[i], output2[i]);
  }
}

TEST(EqTest, All) {
  struct eq *eq;
  size_t len = 44100;
  float NQ = len / 2;
  float f_low = 10 / NQ;
  float f_mid = 100 / NQ;
  float f_high = 1000 / NQ;
  float *data = (float *)malloc(sizeof(float) * len);

  dsp_enable_flush_denormal_to_zero();
  /* low pass */
  memset(data, 0, sizeof(float) * len);
  add_sine(data, len, f_low, 0, 1);  // 10Hz sine, magnitude = 1
  EXPECT_FLOAT_EQ(1, magnitude_at(data, len, f_low));
  add_sine(data, len, f_high, 0, 1);  // 1000Hz sine, magnitude = 1
  EXPECT_FLOAT_EQ(1, magnitude_at(data, len, f_low));
  EXPECT_FLOAT_EQ(1, magnitude_at(data, len, f_high));

  eq = eq_new();
  EXPECT_EQ(0, eq_append_biquad(eq, BQ_LOWPASS, f_mid, 0, 0));
  eq_process(eq, data, len);
  EXPECT_NEAR(1, magnitude_at(data, len, f_low), 0.01);
  EXPECT_NEAR(0, magnitude_at(data, len, f_high), 0.01);

  /* Test for empty input */
  eq_process(eq, NULL, 0);

  eq_free(eq);

  /* high pass */
  memset(data, 0, sizeof(float) * len);
  add_sine(data, len, f_low, 0, 1);
  add_sine(data, len, f_high, 0, 1);

  eq = eq_new();
  EXPECT_EQ(0, eq_append_biquad(eq, BQ_HIGHPASS, f_mid, 0, 0));
  eq_process(eq, data, len);
  EXPECT_NEAR(0, magnitude_at(data, len, f_low), 0.01);
  EXPECT_NEAR(1, magnitude_at(data, len, f_high), 0.01);
  eq_free(eq);

  /* peaking */
  memset(data, 0, sizeof(float) * len);
  add_sine(data, len, f_low, 0, 1);
  add_sine(data, len, f_high, 0, 1);

  eq = eq_new();
  EXPECT_EQ(0, eq_append_biquad(eq, BQ_PEAKING, f_high, 5, 6)); // Q=5, 6dB gain
  eq_process(eq, data, len);
  EXPECT_NEAR(1, magnitude_at(data, len, f_low), 0.01);
  EXPECT_NEAR(2, magnitude_at(data, len, f_high), 0.01);
  eq_free(eq);

  free(data);

  /* Too many biquads */
  eq = eq_new();
  for (int i = 0; i < MAX_BIQUADS_PER_EQ; i++) {
    EXPECT_EQ(0, eq_append_biquad(eq, BQ_PEAKING, f_high, 5, 6));
  }
  EXPECT_EQ(-1, eq_append_biquad(eq, BQ_PEAKING, f_high, 5, 6));
  eq_free(eq);
}

TEST(Eq2Test, All) {
  struct eq2 *eq2;
  size_t len = 44100;
  float NQ = len / 2;
  float f_low = 10 / NQ;
  float f_mid = 100 / NQ;
  float f_high = 1000 / NQ;
  float *data0 = (float *)malloc(sizeof(float) * len);
  float *data1 = (float *)malloc(sizeof(float) * len);

  dsp_enable_flush_denormal_to_zero();

  /* a mixture of 10Hz an 1000Hz sine */
  memset(data0, 0, sizeof(float) * len);
  memset(data1, 0, sizeof(float) * len);
  add_sine(data0, len, f_low, 0, 1);  // 10Hz sine, magnitude = 1
  add_sine(data0, len, f_high, 0, 1);  // 1000Hz sine, magnitude = 1
  add_sine(data1, len, f_low, 0, 1);  // 10Hz sine, magnitude = 1
  add_sine(data1, len, f_high, 0, 1);  // 1000Hz sine, magnitude = 1

  /* low pass at left and high pass at right */
  eq2 = eq2_new();
  EXPECT_EQ(0, eq2_append_biquad(eq2, 0, BQ_LOWPASS, f_mid, 0, 0));
  EXPECT_EQ(0, eq2_append_biquad(eq2, 1, BQ_HIGHPASS, f_mid, 0, 0));
  eq2_process(eq2, data0, data1, len);
  EXPECT_NEAR(1, magnitude_at(data0, len, f_low), 0.01);
  EXPECT_NEAR(0, magnitude_at(data0, len, f_high), 0.01);
  EXPECT_NEAR(0, magnitude_at(data1, len, f_low), 0.01);
  EXPECT_NEAR(1, magnitude_at(data1, len, f_high), 0.01);

  /* Test for empty input */
  eq2_process(eq2, NULL, NULL, 0);
  eq2_free(eq2);

  /* a mixture of 10Hz and 1000Hz sine */
  memset(data0, 0, sizeof(float) * len);
  memset(data1, 0, sizeof(float) * len);
  add_sine(data0, len, f_low, 0, 1);
  add_sine(data0, len, f_high, 0, 1);
  add_sine(data1, len, f_low, 0, 1);
  add_sine(data1, len, f_high, 0, 1);

  /* one high-shelving biquad at left and two low-shelving biquads at right */
  eq2 = eq2_new();
  EXPECT_EQ(0, eq2_append_biquad(eq2, 0, BQ_HIGHSHELF, f_mid, 5, 6));
  EXPECT_EQ(0, eq2_append_biquad(eq2, 1, BQ_LOWSHELF, f_mid, 0, -6));
  EXPECT_EQ(0, eq2_append_biquad(eq2, 1, BQ_LOWSHELF, f_mid, 0, -6));

  eq2_process(eq2, data0, data1, len);
  EXPECT_NEAR(1, magnitude_at(data0, len, f_low), 0.01);
  EXPECT_NEAR(2, magnitude_at(data0, len, f_high), 0.01);
  EXPECT_NEAR(0.25, magnitude_at(data1, len, f_low), 0.01);
  EXPECT_NEAR(1, magnitude_at(data1, len, f_high), 0.01);
  eq2_free(eq2);

  free(data0);
  free(data1);

  /* Too many biquads */
  eq2 = eq2_new();
  for (int i = 0; i < MAX_BIQUADS_PER_EQ2; i++) {
    EXPECT_EQ(0, eq2_append_biquad(eq2, 0, BQ_PEAKING, f_high, 5, 6));
    EXPECT_EQ(0, eq2_append_biquad(eq2, 1, BQ_PEAKING, f_high, 5, 6));
  }
  EXPECT_EQ(-1, eq2_append_biquad(eq2, 0, BQ_PEAKING, f_high, 5, 6));
  EXPECT_EQ(-1, eq2_append_biquad(eq2, 1, BQ_PEAKING, f_high, 5, 6));
  eq2_free(eq2);
}

TEST(CrossoverTest, All) {
  struct crossover xo;
  size_t len = 44100;
  float NQ = len / 2;
  float f0 = 62.5 / NQ;
  float f1 = 250 / NQ;
  float f2 = 1000 / NQ;
  float f3 = 4000 / NQ;
  float f4 = 16000 / NQ;
  float *data = (float *)malloc(sizeof(float) * len);
  float *data1 = (float *)malloc(sizeof(float) * len);
  float *data2 = (float *)malloc(sizeof(float) * len);

  dsp_enable_flush_denormal_to_zero();
  crossover_init(&xo, f1, f3);
  memset(data, 0, sizeof(float) * len);
  add_sine(data, len, f0, 0, 1);
  add_sine(data, len, f2, 0, 1);
  add_sine(data, len, f4, 0, 1);

  crossover_process(&xo, len, data, data1, data2);

  // low band
  EXPECT_NEAR(1, magnitude_at(data, len, f0), 0.01);
  EXPECT_NEAR(0, magnitude_at(data, len, f2), 0.01);
  EXPECT_NEAR(0, magnitude_at(data, len, f4), 0.01);

  // mid band
  EXPECT_NEAR(0, magnitude_at(data1, len, f0), 0.01);
  EXPECT_NEAR(1, magnitude_at(data1, len, f2), 0.01);
  EXPECT_NEAR(0, magnitude_at(data1, len, f4), 0.01);

  // high band
  EXPECT_NEAR(0, magnitude_at(data2, len, f0), 0.01);
  EXPECT_NEAR(0, magnitude_at(data2, len, f2), 0.01);
  EXPECT_NEAR(1, magnitude_at(data2, len, f4), 0.01);

  /* Test for empty input */
  crossover_process(&xo, 0, NULL, NULL, NULL);

  free(data);
  free(data1);
  free(data2);
}

TEST(Crossover2Test, All) {
  struct crossover2 xo2;
  size_t len = 44100;
  float NQ = len / 2;
  float f0 = 62.5 / NQ;
  float f1 = 250 / NQ;
  float f2 = 1000 / NQ;
  float f3 = 4000 / NQ;
  float f4 = 16000 / NQ;
  float *data0L = (float *)malloc(sizeof(float) * len);
  float *data1L = (float *)malloc(sizeof(float) * len);
  float *data2L = (float *)malloc(sizeof(float) * len);
  float *data0R = (float *)malloc(sizeof(float) * len);
  float *data1R = (float *)malloc(sizeof(float) * len);
  float *data2R = (float *)malloc(sizeof(float) * len);

  dsp_enable_flush_denormal_to_zero();
  crossover2_init(&xo2, f1, f3);
  memset(data0L, 0, sizeof(float) * len);
  memset(data0R, 0, sizeof(float) * len);

  add_sine(data0L, len, f0, 0, 1);
  add_sine(data0L, len, f2, 0, 1);
  add_sine(data0L, len, f4, 0, 1);

  add_sine(data0R, len, f0, 0, 0.5);
  add_sine(data0R, len, f2, 0, 0.5);
  add_sine(data0R, len, f4, 0, 0.5);

  crossover2_process(&xo2, len, data0L, data0R, data1L, data1R, data2L, data2R);

  // left low band
  EXPECT_NEAR(1, magnitude_at(data0L, len, f0), 0.01);
  EXPECT_NEAR(0, magnitude_at(data0L, len, f2), 0.01);
  EXPECT_NEAR(0, magnitude_at(data0L, len, f4), 0.01);

  // left mid band
  EXPECT_NEAR(0, magnitude_at(data1L, len, f0), 0.01);
  EXPECT_NEAR(1, magnitude_at(data1L, len, f2), 0.01);
  EXPECT_NEAR(0, magnitude_at(data1L, len, f4), 0.01);

  // left high band
  EXPECT_NEAR(0, magnitude_at(data2L, len, f0), 0.01);
  EXPECT_NEAR(0, magnitude_at(data2L, len, f2), 0.01);
  EXPECT_NEAR(1, magnitude_at(data2L, len, f4), 0.01);

  // right low band
  EXPECT_NEAR(0.5, magnitude_at(data0R, len, f0), 0.005);
  EXPECT_NEAR(0, magnitude_at(data0R, len, f2), 0.005);
  EXPECT_NEAR(0, magnitude_at(data0R, len, f4), 0.005);

  // right mid band
  EXPECT_NEAR(0, magnitude_at(data1R, len, f0), 0.005);
  EXPECT_NEAR(0.5, magnitude_at(data1R, len, f2), 0.005);
  EXPECT_NEAR(0, magnitude_at(data1R, len, f4), 0.005);

  // right high band
  EXPECT_NEAR(0, magnitude_at(data2R, len, f0), 0.005);
  EXPECT_NEAR(0, magnitude_at(data2R, len, f2), 0.005);
  EXPECT_NEAR(0.5, magnitude_at(data2R, len, f4), 0.005);

  /* Test for empty input */
  crossover2_process(&xo2, 0, NULL, NULL, NULL, NULL, NULL, NULL);

  free(data0L);
  free(data1L);
  free(data2L);
  free(data0R);
  free(data1R);
  free(data2R);
}

TEST(DrcTest, All) {
  size_t len = 44100;
  float NQ = len / 2;
  float f0 = 62.5 / NQ;
  float f1 = 250 / NQ;
  float f2 = 1000 / NQ;
  float f3 = 4000 / NQ;
  float f4 = 16000 / NQ;
  float *data_left = (float *)malloc(sizeof(float) * len);
  float *data_right = (float *)malloc(sizeof(float) * len);
  float *data[] = {data_left, data_right};
  float *data_empty[] = {NULL, NULL};
  struct drc *drc;

  dsp_enable_flush_denormal_to_zero();
  drc = drc_new(44100);

  drc_set_param(drc, 0, PARAM_CROSSOVER_LOWER_FREQ, 0);
  drc_set_param(drc, 0, PARAM_ENABLED, 1);
  drc_set_param(drc, 0, PARAM_THRESHOLD, -30);
  drc_set_param(drc, 0, PARAM_KNEE, 0);
  drc_set_param(drc, 0, PARAM_RATIO, 3);
  drc_set_param(drc, 0, PARAM_ATTACK, 0.02);
  drc_set_param(drc, 0, PARAM_RELEASE, 0.2);
  drc_set_param(drc, 0, PARAM_POST_GAIN, 0);

  drc_set_param(drc, 1, PARAM_CROSSOVER_LOWER_FREQ, f1);
  drc_set_param(drc, 1, PARAM_ENABLED, 0);
  drc_set_param(drc, 1, PARAM_THRESHOLD, -30);
  drc_set_param(drc, 1, PARAM_KNEE, 0);
  drc_set_param(drc, 1, PARAM_RATIO, 3);
  drc_set_param(drc, 1, PARAM_ATTACK, 0.02);
  drc_set_param(drc, 1, PARAM_RELEASE, 0.2);
  drc_set_param(drc, 1, PARAM_POST_GAIN, 0);

  drc_set_param(drc, 2, PARAM_CROSSOVER_LOWER_FREQ, f3);
  drc_set_param(drc, 2, PARAM_ENABLED, 1);
  drc_set_param(drc, 2, PARAM_THRESHOLD, -30);
  drc_set_param(drc, 2, PARAM_KNEE, 0);
  drc_set_param(drc, 2, PARAM_RATIO, 1);
  drc_set_param(drc, 2, PARAM_ATTACK, 0.02);
  drc_set_param(drc, 2, PARAM_RELEASE, 0.2);
  drc_set_param(drc, 2, PARAM_POST_GAIN, 20);

  drc_init(drc);

  memset(data_left, 0, sizeof(float) * len);
  memset(data_right, 0, sizeof(float) * len);
  add_sine(data_left, len, f0, 0, 1);
  add_sine(data_left, len, f2, 0, 1);
  add_sine(data_left, len, f4, 0, 1);
  add_sine(data_right, len, f0, 0, 1);
  add_sine(data_right, len, f2, 0, 1);
  add_sine(data_right, len, f4, 0, 1);

  for (size_t start = 0; start < len; start += DRC_PROCESS_MAX_FRAMES) {
    int chunk = std::min(len - start, (size_t)DRC_PROCESS_MAX_FRAMES);
    drc_process(drc, data, chunk);
    data[0] += chunk;
    data[1] += chunk;
  }

  /* This is -8dB because there is a 12dB makeup (20dB^0.6) inside the DRC */
  EXPECT_NEAR(0.4, magnitude_at(data_right, len, f0), 0.1);

  /* This is 0dB because the DRC is disabled */
  EXPECT_NEAR(1, magnitude_at(data_right, len, f2), 0.1);

  /* This is 20dB because of the post gain */
  EXPECT_NEAR(10, magnitude_at(data_right, len, f4), 1);

  /* Test for empty input */
  drc_process(drc, data_empty, 0);

  drc_free(drc);
  free(data_left);
  free(data_right);
}

}  //  namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
