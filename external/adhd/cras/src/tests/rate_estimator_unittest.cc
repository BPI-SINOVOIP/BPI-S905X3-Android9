// Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>

extern "C" {
#include "rate_estimator.h"
}

static struct timespec window = {
  .tv_sec = 0,
  .tv_nsec = 10000000
};

TEST(RateEstimatorTest, EstimateOutputLinear) {
  struct rate_estimator *re;
  struct timespec t = {
    .tv_sec = 1,
    .tv_nsec = 0
  };
  int i, rc, level, tmp;

  re = rate_estimator_create(10000, &window, 0.0f);
  level = 240;
  for (i = 0; i < 20; i++) {
    rc = rate_estimator_check(re, level, &t);
    EXPECT_EQ(0, rc);

    /* Test that output device consumes 5 frames. */
    tmp = rand() % 10;
    rate_estimator_add_frames(re, 5 + tmp);
    level += tmp;
    t.tv_nsec += 500000;
  }
  t.tv_nsec += 1;
  rc = rate_estimator_check(re, level, &t);
  EXPECT_EQ(1, rc);
  EXPECT_GT(10000, rate_estimator_get_rate(re));
  EXPECT_LT(9999, rate_estimator_get_rate(re));

  rate_estimator_destroy(re);
}

TEST(RateEstimatorTest, EstimateOutputLinear2) {
  struct rate_estimator *re;
  struct timespec t = {
    .tv_sec = 1,
    .tv_nsec = 0
  };
  int level = 240;
  int i, rc, tmp;

  int interval_nsec[5] = {1000000, 1500000, 2000000, 2500000, 3000000};
  int frames_written[5] = {30, 25, 20, 15, 10};

  re = rate_estimator_create(7470, &window, 0.0f);
  for (i = 0; i < 5; i++) {
    rc = rate_estimator_check(re, level, &t);
    EXPECT_EQ(0, rc);

    tmp = rand() % 10;
    rate_estimator_add_frames(re, frames_written[i] + tmp);
    level += tmp;
    t.tv_nsec += interval_nsec[i];
  }
  t.tv_nsec += 1;
  rc = rate_estimator_check(re, level, &t);
  EXPECT_EQ(1, rc);
  /* Calculated rate is 7475.72 */
  EXPECT_GT(7476, rate_estimator_get_rate(re));
  EXPECT_LT(7475, rate_estimator_get_rate(re));

  rate_estimator_destroy(re);
}

TEST(RateEstimatorTest, EstimateRateSkewTooLarge) {
  struct rate_estimator *re;
  struct timespec t = {
    .tv_sec = 1,
    .tv_nsec = 0
  };
  int level = 240;
  int i, rc, tmp;

  int interval_nsec[5] = {1000000, 1500000, 2000000, 2500000, 3000000};
  int frames_written[5] = {30, 25, 20, 15, 10};

  re = rate_estimator_create(10000, &window, 0.0f);
  for (i = 0; i < 5; i++) {
    rc = rate_estimator_check(re, level, &t);
    EXPECT_EQ(0, rc);

    tmp = rand() % 10;
    rate_estimator_add_frames(re, frames_written[i] + tmp);
    level += tmp;
    t.tv_nsec += interval_nsec[i];
  }
  t.tv_nsec += 1;
  rc = rate_estimator_check(re, level, &t);
  EXPECT_EQ(1, rc);
  /* Estimated rate too far from allowed max rate skew */
  EXPECT_EQ(10000, rate_estimator_get_rate(re));

  rate_estimator_destroy(re);
}

TEST(RateEstimatorTest, EstimateOutputSmooth) {
  struct rate_estimator *re;
  struct timespec t;
  int rc;

  re = rate_estimator_create(10010, &window, 0.9f);
  t.tv_sec = 1;
  rc = rate_estimator_check(re, 240, &t);
  EXPECT_EQ(0, rc);

  /* Test that output device consumes 100 frames in
   * 10ms. */
  rate_estimator_add_frames(re, 55);
  t.tv_nsec += 5000000;
  rc = rate_estimator_check(re, 245, &t);
  EXPECT_EQ(0, rc);

  rate_estimator_add_frames(re, 55);
  t.tv_nsec += 5000001;
  rc = rate_estimator_check(re, 250, &t);
  EXPECT_EQ(1, rc);

  /* Assert the rate is smoothed 10010 * 0.9 + 10000 * 0.1 */
  EXPECT_LT(10008, rate_estimator_get_rate(re));
  EXPECT_GT(10009, rate_estimator_get_rate(re));

  rate_estimator_destroy(re);
}

TEST(RateEstimatorTest, EstimateInputLinear) {
  struct rate_estimator *re;
  struct timespec t;
  int i, rc, level, tmp;

  re = rate_estimator_create(10000, &window, 0.0f);
  t.tv_sec = 1;
  level = 1200;
  for (i = 0; i < 20; i++) {
    rc = rate_estimator_check(re, level, &t);
    EXPECT_EQ(0, rc);

    /* Test that stream consumes 5 frames. */
    tmp = rand() % 10;
    rate_estimator_add_frames(re, -(5 + tmp));
    level -= tmp;
    t.tv_nsec += 500000;
  }
  t.tv_nsec += 1;
  rc = rate_estimator_check(re, level, &t);
  EXPECT_EQ(1, rc);
  EXPECT_GT(10000, rate_estimator_get_rate(re));
  EXPECT_LT(9999, rate_estimator_get_rate(re));

  rate_estimator_destroy(re);
}

TEST(RateEstimatorTest, EstimateInputLinear2) {
  struct rate_estimator *re;
  struct timespec t;
  int rc;
  static struct timespec this_window = {
    .tv_sec = 0,
    .tv_nsec = 100000000
  };

  re = rate_estimator_create(10000, &this_window, 0.0f);
  t.tv_sec = 1;
  t.tv_nsec = 0;
  rc = rate_estimator_check(re, 200, &t);
  EXPECT_EQ(0, rc);

  t.tv_nsec += 50000000;
  rc = rate_estimator_check(re, 700, &t);
  EXPECT_EQ(0, rc);

  rate_estimator_add_frames(re, -100);

  t.tv_nsec += 50000000;
  rc = rate_estimator_check(re, 1100, &t);
  t.tv_nsec += 1;
  rc = rate_estimator_check(re, 1100, &t);
  EXPECT_EQ(1, rc);
  EXPECT_GT(10000, rate_estimator_get_rate(re));
  EXPECT_LT(9999, rate_estimator_get_rate(re));

  rate_estimator_destroy(re);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
