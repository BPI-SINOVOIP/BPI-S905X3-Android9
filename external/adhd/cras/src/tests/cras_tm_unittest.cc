// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <gtest/gtest.h>

extern "C" {
#include "cras_tm.h"
#include "cras_types.h"
}

namespace {

class TimerTestSuite : public testing::Test{
  protected:
    virtual void SetUp() {
      tm_ = cras_tm_init();
      ASSERT_TRUE(tm_);
    }

    virtual void TearDown() {
      cras_tm_deinit(tm_);
    }

  struct cras_tm *tm_;
};

static struct timespec time_now;
static unsigned int test_cb_called;
static unsigned int test_cb2_called;

void test_cb(struct cras_timer *t, void *data) {
  test_cb_called++;
}

void test_cb2(struct cras_timer *t, void *data) {
  test_cb2_called++;
}

TEST_F(TimerTestSuite, InitNoTimers) {
  struct timespec ts;
  int timers_active;

  timers_active = cras_tm_get_next_timeout(tm_, &ts);
  EXPECT_FALSE(timers_active);
}

TEST_F(TimerTestSuite, AddTimer) {
  struct cras_timer *t;

  t = cras_tm_create_timer(tm_, 10, test_cb, this);
  EXPECT_TRUE(t);
}

TEST_F(TimerTestSuite, AddLongTimer) {
  struct timespec ts;
  struct cras_timer *t;
  int timers_active;

  time_now.tv_sec = 0;
  time_now.tv_nsec = 0;
  t = cras_tm_create_timer(tm_, 10000, test_cb, this);
  EXPECT_TRUE(t);

  timers_active = cras_tm_get_next_timeout(tm_, &ts);
  ASSERT_TRUE(timers_active);
  EXPECT_EQ(10, ts.tv_sec);
  EXPECT_EQ(0, ts.tv_nsec);

  // All timers already fired.
  time_now.tv_sec = 12;
  time_now.tv_nsec = 0;
  timers_active = cras_tm_get_next_timeout(tm_, &ts);
  ASSERT_TRUE(timers_active);
  EXPECT_EQ(0, ts.tv_sec);
  EXPECT_EQ(0, ts.tv_nsec);

  cras_tm_cancel_timer(tm_, t);
  timers_active = cras_tm_get_next_timeout(tm_, &ts);
  EXPECT_FALSE(timers_active);
}

TEST_F(TimerTestSuite, AddRemoveTimer) {
  struct timespec ts;
  struct cras_timer *t;
  int timers_active;

  time_now.tv_sec = 0;
  time_now.tv_nsec = 0;
  t = cras_tm_create_timer(tm_, 10, test_cb, this);
  EXPECT_TRUE(t);

  timers_active = cras_tm_get_next_timeout(tm_, &ts);
  ASSERT_TRUE(timers_active);
  EXPECT_EQ(0, ts.tv_sec);
  EXPECT_EQ(10 * 1000000, ts.tv_nsec);

  // All timers already fired.
  time_now.tv_sec = 1;
  time_now.tv_nsec = 0;
  timers_active = cras_tm_get_next_timeout(tm_, &ts);
  ASSERT_TRUE(timers_active);
  EXPECT_EQ(0, ts.tv_sec);
  EXPECT_EQ(0, ts.tv_nsec);

  cras_tm_cancel_timer(tm_, t);
  timers_active = cras_tm_get_next_timeout(tm_, &ts);
  EXPECT_FALSE(timers_active);
}

TEST_F(TimerTestSuite, AddTwoTimers) {
  struct timespec ts;
  struct cras_timer *t1, *t2;
  int timers_active;
  static const unsigned int t1_to = 10;
  static const unsigned int t2_offset = 5;
  static const unsigned int t2_to = 7;

  time_now.tv_sec = 0;
  time_now.tv_nsec = 0;
  t1 = cras_tm_create_timer(tm_, t1_to, test_cb, this);
  ASSERT_TRUE(t1);

  time_now.tv_sec = 0;
  time_now.tv_nsec = t2_offset;
  t2 = cras_tm_create_timer(tm_, t2_to, test_cb2, this);
  ASSERT_TRUE(t2);

  /* Check That the right calls are made at the right times. */
  test_cb_called = 0;
  test_cb2_called = 0;
  time_now.tv_sec = 0;
  time_now.tv_nsec = t2_to * 1000000 + t2_offset;
  cras_tm_call_callbacks(tm_);
  EXPECT_EQ(0, test_cb_called);
  EXPECT_EQ(1, test_cb2_called);
  timers_active = cras_tm_get_next_timeout(tm_, &ts);
  ASSERT_TRUE(timers_active);

  time_now.tv_sec = 0;
  time_now.tv_nsec = t2_offset;
  t2 = cras_tm_create_timer(tm_, t2_to, test_cb2, this);
  ASSERT_TRUE(t2);

  test_cb_called = 0;
  test_cb2_called = 0;
  time_now.tv_sec = 0;
  time_now.tv_nsec = t1_to * 1000000;
  cras_tm_call_callbacks(tm_);
  EXPECT_EQ(1, test_cb_called);
  EXPECT_EQ(1, test_cb2_called);
  timers_active = cras_tm_get_next_timeout(tm_, &ts);
  EXPECT_FALSE(timers_active);

  time_now.tv_sec = 0;
  time_now.tv_nsec = 0;
  t1 = cras_tm_create_timer(tm_, t1_to, test_cb, this);
  ASSERT_TRUE(t1);

  time_now.tv_sec = 0;
  time_now.tv_nsec = t2_offset;
  t2 = cras_tm_create_timer(tm_, t2_to, test_cb2, this);
  ASSERT_TRUE(t2);

  /* Timeout values returned are correct. */
  time_now.tv_sec = 0;
  time_now.tv_nsec = 50;
  timers_active = cras_tm_get_next_timeout(tm_, &ts);
  ASSERT_TRUE(timers_active);
  EXPECT_EQ(0, ts.tv_sec);
  EXPECT_EQ(t2_to * 1000000 + t2_offset - time_now.tv_nsec, ts.tv_nsec);

  cras_tm_cancel_timer(tm_, t2);

  time_now.tv_sec = 0;
  time_now.tv_nsec = 60;
  timers_active = cras_tm_get_next_timeout(tm_, &ts);
  ASSERT_TRUE(timers_active);
  EXPECT_EQ(0, ts.tv_sec);
  EXPECT_EQ(t1_to * 1000000 - time_now.tv_nsec, ts.tv_nsec);
  cras_tm_cancel_timer(tm_, t1);
}

/* Stubs */
extern "C" {

int clock_gettime(clockid_t clk_id, struct timespec *tp) {
  *tp = time_now;
  return 0;
}

}  // extern "C"

}  //  namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
