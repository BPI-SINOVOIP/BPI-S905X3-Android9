// Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>

extern "C" {
#include "cras_bt_io.h"
#include "cras_bt_device.h"
#include "cras_iodev.h"
#include "cras_main_message.h"

#define FAKE_OBJ_PATH "/obj/path"
}

static struct cras_iodev *cras_bt_io_create_profile_ret;
static struct cras_iodev *cras_bt_io_append_btio_val;
static struct cras_ionode* cras_bt_io_get_profile_ret;
static unsigned int cras_bt_io_create_called;
static unsigned int cras_bt_io_append_called;
static unsigned int cras_bt_io_remove_called;
static unsigned int cras_bt_io_destroy_called;
static enum cras_bt_device_profile cras_bt_io_create_profile_val;
static enum cras_bt_device_profile cras_bt_io_append_profile_val;
static unsigned int cras_bt_io_try_remove_ret;

static cras_main_message *cras_main_message_send_msg;
static cras_message_callback cras_main_message_add_handler_callback;
static void *cras_main_message_add_handler_callback_data;

void ResetStubData() {
  cras_bt_io_get_profile_ret = NULL;
  cras_bt_io_create_called = 0;
  cras_bt_io_append_called = 0;
  cras_bt_io_remove_called = 0;
  cras_bt_io_destroy_called = 0;
  cras_bt_io_try_remove_ret = 0;
}

namespace {

class BtDeviceTestSuite : public testing::Test {
  protected:
    virtual void SetUp() {
      ResetStubData();
      bt_iodev1.direction = CRAS_STREAM_OUTPUT;
      bt_iodev1.update_active_node = update_active_node;
      bt_iodev2.direction = CRAS_STREAM_INPUT;
      bt_iodev2.update_active_node = update_active_node;
      d1_.direction = CRAS_STREAM_OUTPUT;
      d1_.update_active_node = update_active_node;
      d2_.direction = CRAS_STREAM_OUTPUT;
      d2_.update_active_node = update_active_node;
      d3_.direction = CRAS_STREAM_INPUT;
      d3_.update_active_node = update_active_node;
    }

    static void update_active_node(struct cras_iodev *iodev,
                                   unsigned node_idx,
                                   unsigned dev_enabled) {
    }

    struct cras_iodev bt_iodev1;
    struct cras_iodev bt_iodev2;
    struct cras_iodev d3_;
    struct cras_iodev d2_;
    struct cras_iodev d1_;
};

TEST(BtDeviceSuite, CreateBtDevice) {
  struct cras_bt_device *device;

  device = cras_bt_device_create(NULL, FAKE_OBJ_PATH);
  EXPECT_NE((void *)NULL, device);

  device = cras_bt_device_get(FAKE_OBJ_PATH);
  EXPECT_NE((void *)NULL, device);

  cras_bt_device_destroy(device);
  device = cras_bt_device_get(FAKE_OBJ_PATH);
  EXPECT_EQ((void *)NULL, device);
}

TEST_F(BtDeviceTestSuite, AppendRmIodev) {
  struct cras_bt_device *device;
  device = cras_bt_device_create(NULL, FAKE_OBJ_PATH);
  bt_iodev1.nodes = reinterpret_cast<struct cras_ionode*>(0x123);
  cras_bt_io_create_profile_ret = &bt_iodev1;
  cras_bt_device_append_iodev(device, &d1_,
      CRAS_BT_DEVICE_PROFILE_A2DP_SOURCE);
  EXPECT_EQ(1, cras_bt_io_create_called);
  EXPECT_EQ(0, cras_bt_io_append_called);
  EXPECT_EQ(CRAS_BT_DEVICE_PROFILE_A2DP_SOURCE,
            cras_bt_io_create_profile_val);
  cras_bt_device_set_active_profile(device,
      CRAS_BT_DEVICE_PROFILE_A2DP_SOURCE);

  cras_bt_device_append_iodev(device, &d2_,
      CRAS_BT_DEVICE_PROFILE_HFP_AUDIOGATEWAY);
  EXPECT_EQ(1, cras_bt_io_create_called);
  EXPECT_EQ(1, cras_bt_io_append_called);
  EXPECT_EQ(CRAS_BT_DEVICE_PROFILE_HFP_AUDIOGATEWAY,
  	    cras_bt_io_append_profile_val);
  EXPECT_EQ(&bt_iodev1, cras_bt_io_append_btio_val);

  /* Test HFP disconnected and switch to A2DP. */
  cras_bt_io_get_profile_ret = bt_iodev1.nodes;
  cras_bt_io_try_remove_ret = CRAS_BT_DEVICE_PROFILE_A2DP_SOURCE;
  cras_bt_device_set_active_profile(
      device, CRAS_BT_DEVICE_PROFILE_HFP_AUDIOGATEWAY);
  cras_bt_device_rm_iodev(device, &d2_);
  EXPECT_EQ(1, cras_bt_io_remove_called);

  /* Test A2DP disconnection will cause bt_io destroy. */
  cras_bt_io_try_remove_ret = 0;
  cras_bt_device_rm_iodev(device, &d1_);
  EXPECT_EQ(1, cras_bt_io_remove_called);
  EXPECT_EQ(1, cras_bt_io_destroy_called);
  EXPECT_EQ(0, cras_bt_device_get_active_profile(device));
}

TEST_F(BtDeviceTestSuite, SwitchProfile) {
  struct cras_bt_device *device;

  ResetStubData();
  device = cras_bt_device_create(NULL, FAKE_OBJ_PATH);
  cras_bt_io_create_profile_ret = &bt_iodev1;
  cras_bt_device_append_iodev(device, &d1_,
      CRAS_BT_DEVICE_PROFILE_A2DP_SOURCE);
  cras_bt_io_create_profile_ret = &bt_iodev2;
  cras_bt_device_append_iodev(device, &d3_,
      CRAS_BT_DEVICE_PROFILE_HFP_AUDIOGATEWAY);

  cras_bt_device_start_monitor();
  cras_bt_device_switch_profile_enable_dev(device, &bt_iodev1);

  /* Two bt iodevs were all active. */
  cras_main_message_add_handler_callback(
      cras_main_message_send_msg,
      cras_main_message_add_handler_callback_data);

  /* One bt iodev was active, the other was not. */
  cras_bt_device_switch_profile_enable_dev(device, &bt_iodev2);
  cras_main_message_add_handler_callback(
      cras_main_message_send_msg,
      cras_main_message_add_handler_callback_data);

  /* Output bt iodev wasn't active, close the active input iodev. */
  cras_bt_device_switch_profile(device, &bt_iodev2);
  cras_main_message_add_handler_callback(
      cras_main_message_send_msg,
      cras_main_message_add_handler_callback_data);
}

/* Stubs */
extern "C" {

/* From bt_io */
struct cras_iodev *cras_bt_io_create(
        struct cras_bt_device *device,
				struct cras_iodev *dev,
				enum cras_bt_device_profile profile)
{
  cras_bt_io_create_called++;
  cras_bt_io_create_profile_val = profile;
  return cras_bt_io_create_profile_ret;
}
void cras_bt_io_destroy(struct cras_iodev *bt_iodev)
{
  cras_bt_io_destroy_called++;
}
struct cras_ionode* cras_bt_io_get_profile(
    struct cras_iodev *bt_iodev,
    enum cras_bt_device_profile profile)
{
  return cras_bt_io_get_profile_ret;
}
int cras_bt_io_append(struct cras_iodev *bt_iodev,
		      struct cras_iodev *dev,
		      enum cras_bt_device_profile profile)
{
  cras_bt_io_append_called++;
  cras_bt_io_append_profile_val = profile;
  cras_bt_io_append_btio_val = bt_iodev;
  return 0;
}
int cras_bt_io_on_profile(struct cras_iodev *bt_iodev,
                          enum cras_bt_device_profile profile)
{
  return 0;
}
int cras_bt_io_update_buffer_size(struct cras_iodev *bt_iodev)
{
  return 0;
}
unsigned int cras_bt_io_try_remove(struct cras_iodev *bt_iodev,
           struct cras_iodev *dev)
{
  return cras_bt_io_try_remove_ret;
}
int cras_bt_io_remove(struct cras_iodev *bt_iodev,
		                  struct cras_iodev *dev)
{
  cras_bt_io_remove_called++;
  return 0;
}

/* From bt_adapter */
struct cras_bt_adapter *cras_bt_adapter_get(const char *object_path)
{
  return NULL;
}
const char *cras_bt_adapter_address(const struct cras_bt_adapter *adapter)
{
  return NULL;
}

int cras_bt_adapter_on_usb(struct cras_bt_adapter *adapter)
{
  return 1;
}

/* From bt_profile */
void cras_bt_profile_on_device_disconnected(struct cras_bt_device *device)
{
}

/* From hfp_ag_profile */
struct hfp_slc_handle *cras_hfp_ag_get_slc(struct cras_bt_device *device)
{
  return NULL;
}

void cras_hfp_ag_suspend_connected_device(struct cras_bt_device *device)
{
}

void cras_a2dp_suspend_connected_device(struct cras_bt_device *device)
{
}

void cras_a2dp_start(struct cras_bt_device *device)
{
}

int cras_hfp_ag_start(struct cras_bt_device *device)
{
  return 0;
}

void cras_hfp_ag_suspend()
{
}

/* From hfp_slc */
int hfp_event_speaker_gain(struct hfp_slc_handle *handle, int gain)
{
  return 0;
}

/* From iodev_list */

int cras_iodev_open(struct cras_iodev *dev, unsigned int cb_level) {
  return 0;
}

int cras_iodev_close(struct cras_iodev *dev) {
  return 0;
}

int cras_iodev_list_dev_is_enabled(const struct cras_iodev *dev)
{
  return 0;
}

void cras_iodev_list_disable_dev(struct cras_iodev *dev)
{
}

void cras_iodev_list_enable_dev(struct cras_iodev *dev)
{
}

void cras_iodev_list_notify_node_volume(struct cras_ionode *node)
{
}

int cras_main_message_send(struct cras_main_message *msg)
{
  cras_main_message_send_msg = msg;
  return 0;
}

int cras_main_message_add_handler(enum CRAS_MAIN_MESSAGE_TYPE type,
          cras_message_callback callback,
          void *callback_data)
{
  cras_main_message_add_handler_callback = callback;
  cras_main_message_add_handler_callback_data = callback_data;
  return 0;
}

/* From cras_system_state */
struct cras_tm *cras_system_state_get_tm()
{
  return NULL;
}

/* From cras_tm */
struct cras_timer *cras_tm_create_timer(
    struct cras_tm *tm,
    unsigned int ms,
    void (*cb)(struct cras_timer *t, void *data),
    void *cb_data)
{
  return NULL;
}

void cras_tm_cancel_timer(struct cras_tm *tm, struct cras_timer *t)
{
}

} // extern "C"
} // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


