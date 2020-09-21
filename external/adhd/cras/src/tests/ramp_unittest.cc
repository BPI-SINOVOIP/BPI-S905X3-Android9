// Copyright 2016 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <gtest/gtest.h>

extern "C" {
#include "cras_ramp.c"
}

static int callback_called;
static void *callback_arg;

void ResetStubData() {
  callback_called = 0;
  callback_arg = NULL;
}

namespace {

TEST(RampTestSuite, Init) {
  struct cras_ramp *ramp;
  struct cras_ramp_action action;

  ResetStubData();

  ramp = cras_ramp_create();
  action = cras_ramp_get_current_action(ramp);

  EXPECT_EQ(action.type, CRAS_RAMP_ACTION_NONE);
  EXPECT_FLOAT_EQ(1.0, action.scaler);
  EXPECT_FLOAT_EQ(0.0, action.increment);

  cras_ramp_destroy(ramp);
}

TEST(RampTestSuite, RampUpInitialIncrement) {
  int ramp_up = 1;
  int duration_frames = 48000;
  float increment = 1.0 / 48000;
  cras_ramp *ramp;
  cras_ramp_action action;

  ResetStubData();

  ramp = cras_ramp_create();
  cras_ramp_start(ramp, ramp_up, duration_frames, NULL, NULL);

  action = cras_ramp_get_current_action(ramp);

  EXPECT_EQ(CRAS_RAMP_ACTION_PARTIAL, action.type);
  EXPECT_FLOAT_EQ(0.0, action.scaler);
  EXPECT_FLOAT_EQ(increment, action.increment);

  cras_ramp_destroy(ramp);
}

TEST(RampTestSuite, RampUpUpdateRampedFrames) {
  int ramp_up = 1;
  int duration_frames = 48000;
  float increment = 1.0 / 48000;
  int rc;
  int ramped_frames = 512;
  struct cras_ramp *ramp;
  struct cras_ramp_action action;
  float scaler = increment * ramped_frames;

  ResetStubData();

  ramp = cras_ramp_create();
  cras_ramp_start(ramp, ramp_up, duration_frames, NULL, NULL);

  rc = cras_ramp_update_ramped_frames(ramp, ramped_frames);

  action = cras_ramp_get_current_action(ramp);

  EXPECT_EQ(rc, 0);
  EXPECT_EQ(CRAS_RAMP_ACTION_PARTIAL, action.type);
  EXPECT_FLOAT_EQ(scaler, action.scaler);
  EXPECT_FLOAT_EQ(increment, action.increment);

  cras_ramp_destroy(ramp);
}

TEST(RampTestSuite, RampUpPassedRamp) {
  int ramp_up = 1;
  int duration_frames = 48000;
  int rc;
  int ramped_frames = 48000;
  struct cras_ramp *ramp;
  struct cras_ramp_action action;

  ResetStubData();

  ramp = cras_ramp_create();
  cras_ramp_start(ramp, ramp_up, duration_frames, NULL, NULL);

  rc = cras_ramp_update_ramped_frames(ramp, ramped_frames);

  action = cras_ramp_get_current_action(ramp);

  EXPECT_EQ(0, rc);
  EXPECT_EQ(CRAS_RAMP_ACTION_NONE, action.type);
  EXPECT_FLOAT_EQ(1.0, action.scaler);
  EXPECT_FLOAT_EQ(0.0, action.increment);

  cras_ramp_destroy(ramp);
}

TEST(RampTestSuite, RampUpWhileHalfWayRampDown) {
  int ramp_up;
  int duration_frames = 48000;
  int rc;
  int ramped_frames = 24000;
  struct cras_ramp *ramp;
  struct cras_ramp_action action;
  float down_increment = -1.0 / 48000;
  float up_increment;
  float scaler;

  ResetStubData();

  ramp = cras_ramp_create();
  ramp_up = 0;
  cras_ramp_start(ramp, ramp_up, duration_frames, NULL, NULL);

  rc = cras_ramp_update_ramped_frames(ramp, ramped_frames);

  // Get expected current scaler.
  scaler = 1 + down_increment * ramped_frames;
  // The increment will be calculated by ramping to 1 starting from scaler.
  up_increment = (1 - scaler) / 48000;

  ramp_up = 1;
  cras_ramp_start(ramp, ramp_up, duration_frames, NULL, NULL);

  action = cras_ramp_get_current_action(ramp);

  EXPECT_EQ(0, rc);
  EXPECT_EQ(CRAS_RAMP_ACTION_PARTIAL, action.type);
  EXPECT_FLOAT_EQ(scaler, action.scaler);
  EXPECT_FLOAT_EQ(up_increment, action.increment);

  cras_ramp_destroy(ramp);
}

TEST(RampTestSuite, RampUpWhileHalfWayRampUp) {
  int ramp_up;
  int duration_frames = 48000;
  int rc;
  int ramped_frames = 24000;
  struct cras_ramp *ramp;
  struct cras_ramp_action action;
  float first_increment = 1.0 / 48000;
  float second_increment;
  float scaler;

  ResetStubData();

  ramp = cras_ramp_create();
  ramp_up = 1;
  cras_ramp_start(ramp, ramp_up, duration_frames, NULL, NULL);

  rc = cras_ramp_update_ramped_frames(ramp, ramped_frames);

  // Get expected current scaler.
  scaler = first_increment * ramped_frames;
  // The increment will be calculated by ramping to 1 starting from scaler.
  second_increment = (1 - scaler) / 48000;

  ramp_up = 1;
  cras_ramp_start(ramp, ramp_up, duration_frames, NULL, NULL);

  action = cras_ramp_get_current_action(ramp);

  EXPECT_EQ(0, rc);
  EXPECT_EQ(CRAS_RAMP_ACTION_PARTIAL, action.type);
  EXPECT_FLOAT_EQ(scaler, action.scaler);
  EXPECT_FLOAT_EQ(second_increment, action.increment);

  cras_ramp_destroy(ramp);
}

TEST(RampTestSuite, RampDownInitialIncrement) {
  int ramp_up = 0;
  int duration_frames = 48000;
  float increment = -1.0 / 48000;
  cras_ramp *ramp;
  cras_ramp_action action;

  ResetStubData();

  ramp = cras_ramp_create();
  cras_ramp_start(ramp, ramp_up, duration_frames, NULL, NULL);

  action = cras_ramp_get_current_action(ramp);

  EXPECT_EQ(CRAS_RAMP_ACTION_PARTIAL, action.type);
  EXPECT_FLOAT_EQ(1.0, action.scaler);
  EXPECT_FLOAT_EQ(increment, action.increment);

  cras_ramp_destroy(ramp);
}

TEST(RampTestSuite, RampDownUpdateRampedFrames) {
  int ramp_up = 0;
  int duration_frames = 48000;
  float increment = -1.0 / 48000;
  int rc;
  int ramped_frames = 512;
  struct cras_ramp *ramp;
  struct cras_ramp_action action;
  float scaler = 1 + increment * ramped_frames;

  ResetStubData();

  ramp = cras_ramp_create();
  cras_ramp_start(ramp, ramp_up, duration_frames, NULL, NULL);

  rc = cras_ramp_update_ramped_frames(ramp, ramped_frames);

  action = cras_ramp_get_current_action(ramp);

  EXPECT_EQ(rc, 0);
  EXPECT_EQ(CRAS_RAMP_ACTION_PARTIAL, action.type);
  EXPECT_FLOAT_EQ(scaler, action.scaler);
  EXPECT_FLOAT_EQ(increment, action.increment);

  cras_ramp_destroy(ramp);
}

TEST(RampTestSuite, RampDownPassedRamp) {
  int ramp_up = 0;
  int duration_frames = 48000;
  int rc;
  int ramped_frames = 48000;
  struct cras_ramp *ramp;
  struct cras_ramp_action action;

  ResetStubData();

  ramp = cras_ramp_create();
  cras_ramp_start(ramp, ramp_up, duration_frames, NULL, NULL);

  rc = cras_ramp_update_ramped_frames(ramp, ramped_frames);

  action = cras_ramp_get_current_action(ramp);

  EXPECT_EQ(0, rc);
  EXPECT_EQ(CRAS_RAMP_ACTION_NONE, action.type);
  EXPECT_FLOAT_EQ(1.0, action.scaler);
  EXPECT_FLOAT_EQ(0.0, action.increment);

  cras_ramp_destroy(ramp);
}

TEST(RampTestSuite, RampDownWhileHalfWayRampUp) {
  int ramp_up;
  int duration_frames = 48000;
  int rc;
  int ramped_frames = 24000;
  struct cras_ramp *ramp;
  struct cras_ramp_action action;
  float up_increment = 1.0 / 48000;
  float down_increment;
  float scaler;

  ResetStubData();

  ramp = cras_ramp_create();
  // Ramp up first.
  ramp_up = 1;
  cras_ramp_start(ramp, ramp_up, duration_frames, NULL, NULL);

  rc = cras_ramp_update_ramped_frames(ramp, ramped_frames);

  // Get expected current scaler.
  scaler = up_increment * ramped_frames;
  // The increment will be calculated by ramping to 0 starting from scaler.
  down_increment = -scaler / duration_frames;


  // Ramp down will start from current scaler.
  ramp_up = 0;
  cras_ramp_start(ramp, ramp_up, duration_frames, NULL, NULL);

  action = cras_ramp_get_current_action(ramp);

  EXPECT_EQ(0, rc);
  EXPECT_EQ(CRAS_RAMP_ACTION_PARTIAL, action.type);
  EXPECT_FLOAT_EQ(scaler, action.scaler);
  EXPECT_FLOAT_EQ(down_increment, action.increment);

  cras_ramp_destroy(ramp);
}

TEST(RampTestSuite, RampDownWhileHalfWayRampDown) {
  int ramp_up;
  int duration_frames = 48000;
  int rc;
  int ramped_frames = 24000;
  struct cras_ramp *ramp;
  struct cras_ramp_action action;
  float down_increment = -1.0 / 48000;
  float second_down_increment;
  float scaler;

  ResetStubData();

  ramp = cras_ramp_create();
  // Ramp down.
  ramp_up = 0;
  cras_ramp_start(ramp, ramp_up, duration_frames, NULL, NULL);

  rc = cras_ramp_update_ramped_frames(ramp, ramped_frames);

  // Get expected current scaler.
  scaler = 1 + down_increment * ramped_frames;
  // The increment will be calculated by ramping to 0 starting from scaler.
  second_down_increment = -scaler / duration_frames;


  // Ramp down starting from current scaler.
  ramp_up = 0;
  cras_ramp_start(ramp, ramp_up, duration_frames, NULL, NULL);

  action = cras_ramp_get_current_action(ramp);

  EXPECT_EQ(0, rc);
  EXPECT_EQ(CRAS_RAMP_ACTION_PARTIAL, action.type);
  EXPECT_FLOAT_EQ(scaler, action.scaler);
  EXPECT_FLOAT_EQ(second_down_increment, action.increment);

  cras_ramp_destroy(ramp);
}

void ramp_callback(void *arg) {
  callback_called++;
  callback_arg = arg;
}

TEST(RampTestSuite, RampUpPassedRampCallback) {
  int ramp_up = 1;
  int duration_frames = 48000;
  int rc;
  int ramped_frames = 48000;
  struct cras_ramp *ramp;
  struct cras_ramp_action action;
  void *cb_data = reinterpret_cast<void*>(0x123);

  ResetStubData();

  ramp = cras_ramp_create();
  cras_ramp_start(ramp, ramp_up, duration_frames, ramp_callback, cb_data);

  rc = cras_ramp_update_ramped_frames(ramp, ramped_frames);

  action = cras_ramp_get_current_action(ramp);

  EXPECT_EQ(0, rc);
  EXPECT_EQ(CRAS_RAMP_ACTION_NONE, action.type);
  EXPECT_FLOAT_EQ(1.0, action.scaler);
  EXPECT_FLOAT_EQ(0.0, action.increment);
  EXPECT_EQ(1, callback_called);
  EXPECT_EQ(cb_data, callback_arg);

  cras_ramp_destroy(ramp);
}

TEST(RampTestSuite, RampDownPassedRampCallback) {
  int ramp_up = 0;
  int duration_frames = 48000;
  int rc;
  int ramped_frames = 48000;
  struct cras_ramp *ramp;
  struct cras_ramp_action action;
  void *cb_data = reinterpret_cast<void*>(0x123);

  ResetStubData();

  ramp = cras_ramp_create();
  cras_ramp_start(ramp, ramp_up, duration_frames, ramp_callback, cb_data);

  rc = cras_ramp_update_ramped_frames(ramp, ramped_frames);

  action = cras_ramp_get_current_action(ramp);

  EXPECT_EQ(0, rc);
  EXPECT_EQ(CRAS_RAMP_ACTION_NONE, action.type);
  EXPECT_FLOAT_EQ(1.0, action.scaler);
  EXPECT_FLOAT_EQ(0.0, action.increment);
  EXPECT_EQ(1, callback_called);
  EXPECT_EQ(cb_data, callback_arg);

  cras_ramp_destroy(ramp);
}

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  int rc = RUN_ALL_TESTS();

  return rc;
}
