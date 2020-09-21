// Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>

extern "C" {

// To test static functions.
#include "cras_bt_io.c"
}

static struct cras_bt_device *fake_device =
    reinterpret_cast<struct cras_bt_device*>(0x123);
static unsigned int cras_iodev_add_node_called;
static unsigned int cras_iodev_rm_node_called;
static unsigned int cras_iodev_free_format_called;
static unsigned int cras_iodev_set_active_node_called;
static unsigned int cras_iodev_list_add_output_called;
static unsigned int cras_iodev_list_rm_output_called;
static unsigned int cras_iodev_list_add_input_called;
static unsigned int cras_iodev_list_rm_input_called;
static unsigned int cras_bt_device_set_active_profile_called;
static unsigned int cras_bt_device_set_active_profile_val;
static int cras_bt_device_get_active_profile_ret;
static int cras_bt_device_switch_profile_enable_dev_called;
static int cras_bt_device_switch_profile_called;
static int cras_bt_device_can_switch_to_a2dp_ret;
static int cras_bt_device_has_a2dp_ret;
static int is_utf8_string_ret_value;

void ResetStubData() {
  cras_iodev_add_node_called = 0;
  cras_iodev_rm_node_called = 0;
  cras_iodev_free_format_called = 0;
  cras_iodev_set_active_node_called = 0;
  cras_iodev_list_add_output_called = 0;
  cras_iodev_list_rm_output_called = 0;
  cras_iodev_list_add_input_called = 0;
  cras_iodev_list_rm_input_called = 0;
  cras_bt_device_set_active_profile_called = 0;
  cras_bt_device_set_active_profile_val = 0;
  cras_bt_device_get_active_profile_ret = 0;
  cras_bt_device_switch_profile_enable_dev_called= 0;
  cras_bt_device_switch_profile_called = 0;
  cras_bt_device_can_switch_to_a2dp_ret = 0;
  cras_bt_device_has_a2dp_ret = 0;
  is_utf8_string_ret_value = 1;
}

namespace {

class BtIoBasicSuite : public testing::Test {
  protected:
    virtual void SetUp() {
      ResetStubData();
      SetUpIodev(&iodev_, CRAS_STREAM_OUTPUT);
      SetUpIodev(&iodev2_, CRAS_STREAM_OUTPUT);

      update_supported_formats_called_ = 0;
      frames_queued_called_ = 0;
      delay_frames_called_ = 0;
      get_buffer_called_ = 0;
      put_buffer_called_ = 0;
      open_dev_called_ = 0;
      close_dev_called_ = 0;
    }

    virtual void TearDown() {
    }

    static void SetUpIodev(struct cras_iodev *d,
                           enum CRAS_STREAM_DIRECTION dir) {
      d->direction = dir;
      d->update_supported_formats = update_supported_formats;
      d->frames_queued = frames_queued;
      d->delay_frames = delay_frames;
      d->get_buffer = get_buffer;
      d->put_buffer = put_buffer;
      d->open_dev = open_dev;
      d->close_dev = close_dev;
    }

    // Stub functions for the iodev structure.
    static int update_supported_formats(struct cras_iodev *iodev) {
      iodev->supported_rates = (size_t *)calloc(
          2, sizeof(*iodev->supported_rates));
      iodev->supported_rates[0] = 48000;
      iodev->supported_rates[1] = 0;
      iodev->supported_channel_counts = (size_t *)calloc(
          2, sizeof(*iodev->supported_channel_counts));
      iodev->supported_channel_counts[0] = 2;
      iodev->supported_channel_counts[1] = 0;
      iodev->supported_formats = (snd_pcm_format_t *)calloc(
          2, sizeof(*iodev->supported_formats));
      iodev->supported_formats[0] = SND_PCM_FORMAT_S16_LE;
      iodev->supported_formats[1] = (snd_pcm_format_t)0;
      update_supported_formats_called_++;
      return 0;
    }
    static int frames_queued(const cras_iodev* iodev,
                             struct timespec *tstamp) {
      frames_queued_called_++;
      return 0;
    }
    static int delay_frames(const cras_iodev* iodev) {
      delay_frames_called_++;
      return 0;
    }
    static int get_buffer(cras_iodev* iodev,
                          struct cras_audio_area** area,
                          unsigned int* num) {
      get_buffer_called_++;
      return 0;
    }
    static int put_buffer(cras_iodev* iodev,
                          unsigned int num) {
      put_buffer_called_++;
      return 0;
    }
    static int open_dev(cras_iodev* iodev) {
      open_dev_called_++;
      return 0;
    }
    static int close_dev(cras_iodev* iodev) {
      close_dev_called_++;
      return 0;
    }

  static struct cras_iodev *bt_iodev;
  static struct cras_iodev iodev_;
  static struct cras_iodev iodev2_;
  static unsigned int update_supported_formats_called_;
  static unsigned int frames_queued_called_;
  static unsigned int delay_frames_called_;
  static unsigned int get_buffer_called_;
  static unsigned int put_buffer_called_;
  static unsigned int open_dev_called_;
  static unsigned int close_dev_called_;
};

struct cras_iodev *BtIoBasicSuite::bt_iodev;
struct cras_iodev BtIoBasicSuite::iodev_;
struct cras_iodev BtIoBasicSuite::iodev2_;
unsigned int BtIoBasicSuite::update_supported_formats_called_;
unsigned int BtIoBasicSuite::frames_queued_called_;
unsigned int BtIoBasicSuite::delay_frames_called_;
unsigned int BtIoBasicSuite::get_buffer_called_;
unsigned int BtIoBasicSuite::put_buffer_called_;
unsigned int BtIoBasicSuite::open_dev_called_;
unsigned int BtIoBasicSuite::close_dev_called_;

TEST_F(BtIoBasicSuite, CreateBtIo) {
  struct cras_audio_area *fake_area;
  struct cras_audio_format fake_fmt;
  struct timespec tstamp;
  unsigned fr;
  bt_iodev = cras_bt_io_create(fake_device, &iodev_,
      CRAS_BT_DEVICE_PROFILE_A2DP_SOURCE);
  EXPECT_NE((void *)NULL, bt_iodev);
  EXPECT_EQ(&iodev_, active_profile_dev(bt_iodev));
  EXPECT_EQ(1, cras_iodev_list_add_output_called);
  bt_iodev->format = &fake_fmt;
  bt_iodev->update_supported_formats(bt_iodev);
  EXPECT_EQ(1, update_supported_formats_called_);

  bt_iodev->open_dev(bt_iodev);
  EXPECT_EQ(1, open_dev_called_);
  bt_iodev->frames_queued(bt_iodev, &tstamp);
  EXPECT_EQ(1, frames_queued_called_);
  bt_iodev->get_buffer(bt_iodev, &fake_area, &fr);
  EXPECT_EQ(1, get_buffer_called_);
  bt_iodev->put_buffer(bt_iodev, fr);
  EXPECT_EQ(1, put_buffer_called_);
  bt_iodev->close_dev(bt_iodev);
  EXPECT_EQ(1, close_dev_called_);
  EXPECT_EQ(1, cras_iodev_free_format_called);
  cras_bt_io_destroy(bt_iodev);
  EXPECT_EQ(1, cras_iodev_list_rm_output_called);
}

TEST_F(BtIoBasicSuite, SwitchProfileOnUpdateFormatForInputDev) {
  ResetStubData();
  iodev_.direction = CRAS_STREAM_INPUT;
  bt_iodev = cras_bt_io_create(fake_device, &iodev_,
      CRAS_BT_DEVICE_PROFILE_HFP_AUDIOGATEWAY);

  cras_bt_device_get_active_profile_ret = CRAS_BT_DEVICE_PROFILE_A2DP_SOURCE;
  bt_iodev->update_supported_formats(bt_iodev);

  EXPECT_EQ(CRAS_BT_DEVICE_PROFILE_HSP_AUDIOGATEWAY |
            CRAS_BT_DEVICE_PROFILE_HFP_AUDIOGATEWAY,
            cras_bt_device_set_active_profile_val);
  EXPECT_EQ(1, cras_bt_device_switch_profile_enable_dev_called);
}

TEST_F(BtIoBasicSuite, NoSwitchProfileOnUpdateFormatForInputDevAlreadyOnHfp) {
  ResetStubData();
  iodev_.direction = CRAS_STREAM_INPUT;
  bt_iodev = cras_bt_io_create(fake_device, &iodev_,
      CRAS_BT_DEVICE_PROFILE_HFP_AUDIOGATEWAY);

  /* No need to switch profile if already on HFP. */
  cras_bt_device_get_active_profile_ret =
      CRAS_BT_DEVICE_PROFILE_HFP_AUDIOGATEWAY;
  bt_iodev->update_supported_formats(bt_iodev);

  EXPECT_EQ(0, cras_bt_device_switch_profile_enable_dev_called);
}

TEST_F(BtIoBasicSuite, SwitchProfileOnCloseInputDev) {
  ResetStubData();
  iodev_.direction = CRAS_STREAM_INPUT;
  bt_iodev = cras_bt_io_create(fake_device, &iodev_,
      CRAS_BT_DEVICE_PROFILE_HFP_AUDIOGATEWAY);

  cras_bt_device_get_active_profile_ret =
      CRAS_BT_DEVICE_PROFILE_HFP_AUDIOGATEWAY |
      CRAS_BT_DEVICE_PROFILE_HSP_AUDIOGATEWAY;
  cras_bt_device_has_a2dp_ret = 1;
  bt_iodev->close_dev(bt_iodev);

  EXPECT_EQ(CRAS_BT_DEVICE_PROFILE_A2DP_SOURCE,
            cras_bt_device_set_active_profile_val);
  EXPECT_EQ(1, cras_bt_device_switch_profile_called);
}

TEST_F(BtIoBasicSuite, NoSwitchProfileOnCloseInputDevNoSupportA2dp) {
  ResetStubData();
  iodev_.direction = CRAS_STREAM_INPUT;
  bt_iodev = cras_bt_io_create(fake_device, &iodev_,
      CRAS_BT_DEVICE_PROFILE_HFP_AUDIOGATEWAY);

  cras_bt_device_get_active_profile_ret =
      CRAS_BT_DEVICE_PROFILE_HFP_AUDIOGATEWAY |
      CRAS_BT_DEVICE_PROFILE_HSP_AUDIOGATEWAY;
  cras_bt_device_has_a2dp_ret = 0;
  bt_iodev->close_dev(bt_iodev);

  EXPECT_EQ(0, cras_bt_device_switch_profile_called);
}

TEST_F(BtIoBasicSuite, SwitchProfileOnAppendA2dpDev) {
  ResetStubData();
  bt_iodev = cras_bt_io_create(fake_device, &iodev_,
      CRAS_BT_DEVICE_PROFILE_HFP_AUDIOGATEWAY);

  cras_bt_device_can_switch_to_a2dp_ret = 1;
  cras_bt_io_append(bt_iodev, &iodev2_,
      CRAS_BT_DEVICE_PROFILE_A2DP_SOURCE);

  EXPECT_EQ(CRAS_BT_DEVICE_PROFILE_A2DP_SOURCE,
            cras_bt_device_set_active_profile_val);
  EXPECT_EQ(0, cras_bt_device_switch_profile_enable_dev_called);
  EXPECT_EQ(1, cras_bt_device_switch_profile_called);
}

TEST_F(BtIoBasicSuite, NoSwitchProfileOnAppendHfpDev) {
  ResetStubData();
  bt_iodev = cras_bt_io_create(fake_device, &iodev_,
      CRAS_BT_DEVICE_PROFILE_A2DP_SOURCE);

  cras_bt_device_can_switch_to_a2dp_ret = 1;
  cras_bt_io_append(bt_iodev, &iodev2_,
      CRAS_BT_DEVICE_PROFILE_HFP_AUDIOGATEWAY);

  EXPECT_EQ(0, cras_bt_device_switch_profile_enable_dev_called);
}

TEST_F(BtIoBasicSuite, CreateSetDeviceActiveProfileToA2DP) {
  ResetStubData();
  cras_bt_device_get_active_profile_ret =
      CRAS_BT_DEVICE_PROFILE_HFP_AUDIOGATEWAY;
  cras_bt_device_can_switch_to_a2dp_ret = 1;
  bt_iodev = cras_bt_io_create(fake_device, &iodev_,
      CRAS_BT_DEVICE_PROFILE_A2DP_SOURCE);

  EXPECT_EQ(1, cras_bt_device_set_active_profile_called);
  EXPECT_EQ(CRAS_BT_DEVICE_PROFILE_A2DP_SOURCE,
      cras_bt_device_set_active_profile_val);
  cras_bt_io_destroy(bt_iodev);
}

TEST_F(BtIoBasicSuite, CreateNoSetDeviceActiveProfileToA2DP) {
  ResetStubData();
  cras_bt_device_get_active_profile_ret =
      CRAS_BT_DEVICE_PROFILE_HFP_AUDIOGATEWAY;
  cras_bt_device_can_switch_to_a2dp_ret = 0;
  bt_iodev = cras_bt_io_create(fake_device, &iodev_,
      CRAS_BT_DEVICE_PROFILE_A2DP_SOURCE);

  EXPECT_EQ(0, cras_bt_device_set_active_profile_called);
  cras_bt_io_destroy(bt_iodev);
}

TEST_F(BtIoBasicSuite, CreateSetDeviceActiveProfileToHFP) {
  ResetStubData();
  cras_bt_device_get_active_profile_ret = 0;
  bt_iodev = cras_bt_io_create(fake_device, &iodev_,
      CRAS_BT_DEVICE_PROFILE_HFP_AUDIOGATEWAY);

  EXPECT_EQ(
      CRAS_BT_DEVICE_PROFILE_HFP_AUDIOGATEWAY |
          CRAS_BT_DEVICE_PROFILE_HSP_AUDIOGATEWAY,
      cras_bt_device_set_active_profile_val);
  cras_bt_io_destroy(bt_iodev);
}

TEST_F(BtIoBasicSuite, CreateDeviceWithInvalidUTF8Name) {
  ResetStubData();
  strcpy(iodev_.info.name, "Something BT");
  iodev_.info.name[0] = 0xfe;
  is_utf8_string_ret_value = 0;
  bt_iodev = cras_bt_io_create(fake_device, &iodev_,
      CRAS_BT_DEVICE_PROFILE_A2DP_SOURCE);

  ASSERT_STREQ("BLUETOOTH", bt_iodev->active_node->name);
}

} // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

extern "C" {

// Cras iodev
void cras_iodev_add_node(struct cras_iodev *iodev, struct cras_ionode *node)
{
  cras_iodev_add_node_called++;
  iodev->nodes = node;
}

void cras_iodev_rm_node(struct cras_iodev *iodev, struct cras_ionode *node)
{
  cras_iodev_rm_node_called++;
  iodev->nodes = NULL;
}

void cras_iodev_free_format(struct cras_iodev *iodev)
{
  cras_iodev_free_format_called++;
}

void cras_iodev_set_active_node(struct cras_iodev *iodev,
        struct cras_ionode *node)
{
  cras_iodev_set_active_node_called++;
  iodev->active_node = node;
}

int cras_iodev_set_node_attr(struct cras_ionode *ionode,
           enum ionode_attr attr, int value)
{
  return 0;
}

//  From iodev list.
int cras_iodev_list_add_output(struct cras_iodev *output)
{
  cras_iodev_list_add_output_called++;
  return 0;
}

int cras_iodev_list_rm_output(struct cras_iodev *dev)
{
  cras_iodev_list_rm_output_called++;
  return 0;
}

int cras_iodev_list_add_input(struct cras_iodev *output)
{
  cras_iodev_list_add_input_called++;
  return 0;
}

int cras_iodev_list_rm_input(struct cras_iodev *dev)
{
  cras_iodev_list_rm_input_called++;
  return 0;
}

// From bt device
int cras_bt_device_get_active_profile(const struct cras_bt_device *device)
{
  return cras_bt_device_get_active_profile_ret;
}

void cras_bt_device_set_active_profile(struct cras_bt_device *device,
                                       unsigned int profile)
{
  cras_bt_device_set_active_profile_called++;
  cras_bt_device_set_active_profile_val = profile;
}

int cras_bt_device_has_a2dp(struct cras_bt_device *device)
{
  return cras_bt_device_has_a2dp_ret;
}

int cras_bt_device_can_switch_to_a2dp(struct cras_bt_device *device)
{
  return cras_bt_device_can_switch_to_a2dp_ret;
}

int cras_bt_device_switch_profile(struct cras_bt_device *device,
            struct cras_iodev *bt_iodev)
{
  cras_bt_device_switch_profile_called++;
  return 0;
}

int cras_bt_device_switch_profile_enable_dev(struct cras_bt_device *device,
            struct cras_iodev *bt_iodev)
{
  cras_bt_device_switch_profile_enable_dev_called++;
  return 0;
}

const char *cras_bt_device_object_path(const struct cras_bt_device *device)
{
  return "/fake/object/path";
}

int is_utf8_string(const char* string)
{
  return is_utf8_string_ret_value;
}

int cras_iodev_default_no_stream_playback(struct cras_iodev *odev, int enable)
{
  return 0;
}

} // extern "C"
