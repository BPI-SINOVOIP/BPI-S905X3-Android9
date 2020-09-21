// Copyright 2016 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <gtest/gtest.h>

extern "C" {
#include "cras_device_monitor.c"
#include "cras_iodev.h"
#include "cras_main_message.h"
}

static enum CRAS_MAIN_MESSAGE_TYPE type_set;
static struct cras_device_monitor_message *sent_msg;
static int enable_dev_called;
static cras_iodev *enable_dev;
static int disable_dev_called;
static cras_iodev *disable_dev;
static int set_mute_called;
static cras_iodev *mute_dev;

void ResetStubData() {
  type_set = (enum CRAS_MAIN_MESSAGE_TYPE)0;
  enable_dev_called = 0;
  enable_dev = NULL;
  disable_dev_called = 0;
  disable_dev = NULL;
  set_mute_called = 0;
  mute_dev = NULL;
}

namespace {

TEST(DeviceMonitorTestSuite, Init) {
  ResetStubData();

  cras_device_monitor_init();

  EXPECT_EQ(type_set, CRAS_MAIN_MONITOR_DEVICE);
}

TEST(DeviceMonitorTestSuite, ResetDevice) {
  struct cras_iodev dev;
  ResetStubData();
  // sent_msg will be filled with message content in cras_main_message_send.
  sent_msg = (struct cras_device_monitor_message *)calloc(1, sizeof(*sent_msg));

  cras_device_monitor_reset_device(&dev);

  EXPECT_EQ(sent_msg->header.type, CRAS_MAIN_MONITOR_DEVICE);
  EXPECT_EQ(sent_msg->header.length, sizeof(*sent_msg));
  EXPECT_EQ(sent_msg->message_type, RESET_DEVICE);
  EXPECT_EQ(sent_msg->iodev, &dev);

  free(sent_msg);
}

TEST(DeviceMonitorTestSuite, HandleResetDevice) {
  struct cras_iodev dev;
  struct cras_device_monitor_message msg;
  struct cras_main_message *main_message =
      reinterpret_cast<struct cras_main_message *>(&msg);

  ResetStubData();

  // Filled msg with message content for resetting device.
  init_device_msg(&msg, RESET_DEVICE, &dev);
  // Assume the pipe works fine and main message handler receives the same
  // message.
  handle_device_message(main_message, NULL);

  // Verify that disable/enable functions are called with correct device.
  EXPECT_EQ(enable_dev_called, 1);
  EXPECT_EQ(enable_dev, &dev);
  EXPECT_EQ(disable_dev_called, 1);
  EXPECT_EQ(disable_dev, &dev);
}

TEST(DeviceMonitorTestSuite, MuteDevice) {
  struct cras_iodev dev;
  ResetStubData();
  // sent_msg will be filled with message content in cras_main_message_send.
  sent_msg = (struct cras_device_monitor_message *)calloc(1, sizeof(*sent_msg));

  cras_device_monitor_set_device_mute_state(&dev);

  EXPECT_EQ(sent_msg->header.type, CRAS_MAIN_MONITOR_DEVICE);
  EXPECT_EQ(sent_msg->header.length, sizeof(*sent_msg));
  EXPECT_EQ(sent_msg->message_type, SET_MUTE_STATE);
  EXPECT_EQ(sent_msg->iodev, &dev);

  free(sent_msg);
}

TEST(DeviceMonitorTestSuite, HandleMuteDevice) {
  struct cras_iodev dev;
  struct cras_device_monitor_message msg;
  struct cras_main_message *main_message =
      reinterpret_cast<struct cras_main_message *>(&msg);

  ResetStubData();

  // Filled msg with message content for device mute/unmute.
  init_device_msg(&msg, SET_MUTE_STATE, &dev);
  // Assume the pipe works fine and main message handler receives the same
  // message.
  handle_device_message(main_message, NULL);

  // Verify that cras_iodev_set_mute is called with correct device.
  EXPECT_EQ(set_mute_called, 1);
  EXPECT_EQ(mute_dev, &dev);
}

extern "C" {

int cras_main_message_add_handler(enum CRAS_MAIN_MESSAGE_TYPE type,
                                  cras_message_callback callback,
                                  void *callback_data) {
  type_set = type;
  return 0;
}

int cras_main_message_send(struct cras_main_message *msg) {
  // Copy the sent message so we can examine it in the test later.
  memcpy(sent_msg, msg, sizeof(*sent_msg));
  return 0;
};

void cras_iodev_list_enable_dev(struct cras_iodev *dev) {
  enable_dev_called++;
  enable_dev = dev;
}

void cras_iodev_list_disable_dev(struct cras_iodev *dev) {
  disable_dev_called++;
  disable_dev = dev;
}

int cras_iodev_set_mute(struct cras_iodev *dev) {
  set_mute_called++;
  mute_dev = dev;
  return 0;
}

}  // extern "C"
}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  int rc = RUN_ALL_TESTS();

  return rc;
}
