// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <gtest/gtest.h>

extern "C" {
#include "cras_alert.h"
#include "cras_system_state.h"
#include "cras_types.h"
}

namespace {
static struct cras_alsa_card* kFakeAlsaCard;
size_t cras_alsa_card_create_called;
size_t cras_alsa_card_destroy_called;
static size_t add_stub_called;
static size_t rm_stub_called;
static size_t callback_stub_called;
static void *select_data_value;
static size_t add_callback_called;
static cras_alert_cb add_callback_cb;
static void *add_callback_arg;
static size_t rm_callback_called;
static cras_alert_cb rm_callback_cb;
static void *rm_callback_arg;
static size_t alert_pending_called;
static char* device_config_dir;
static const char* cras_alsa_card_config_dir;
static size_t cras_observer_notify_output_volume_called;
static size_t cras_observer_notify_output_mute_called;
static size_t cras_observer_notify_capture_gain_called;
static size_t cras_observer_notify_capture_mute_called;
static size_t cras_observer_notify_suspend_changed_called;
static size_t cras_observer_notify_num_active_streams_called;

static void ResetStubData() {
  cras_alsa_card_create_called = 0;
  cras_alsa_card_destroy_called = 0;
  kFakeAlsaCard = reinterpret_cast<struct cras_alsa_card*>(0x33);
  add_stub_called = 0;
  rm_stub_called = 0;
  callback_stub_called = 0;
  add_callback_called = 0;
  rm_callback_called = 0;
  alert_pending_called = 0;
  device_config_dir = reinterpret_cast<char *>(3);
  cras_alsa_card_config_dir = NULL;
  cras_observer_notify_output_volume_called = 0;
  cras_observer_notify_output_mute_called = 0;
  cras_observer_notify_capture_gain_called = 0;
  cras_observer_notify_capture_mute_called = 0;
  cras_observer_notify_suspend_changed_called = 0;
  cras_observer_notify_num_active_streams_called = 0;
}

static int add_stub(int fd, void (*cb)(void *data),
                    void *callback_data, void *select_data) {
  add_stub_called++;
  select_data_value = select_data;
  return 0;
}

static void rm_stub(int fd, void *select_data) {
  rm_stub_called++;
  select_data_value = select_data;
}

static void callback_stub(void *data) {
  callback_stub_called++;
}

TEST(SystemStateSuite, DefaultVolume) {
  cras_system_state_init(device_config_dir);
  EXPECT_EQ(100, cras_system_get_volume());
  EXPECT_EQ(2000, cras_system_get_capture_gain());
  EXPECT_EQ(0, cras_system_get_mute());
  EXPECT_EQ(0, cras_system_get_capture_mute());
  cras_system_state_deinit();
}

TEST(SystemStateSuite, SetVolume) {
  cras_system_state_init(device_config_dir);
  cras_system_set_volume(0);
  EXPECT_EQ(0, cras_system_get_volume());
  cras_system_set_volume(50);
  EXPECT_EQ(50, cras_system_get_volume());
  cras_system_set_volume(CRAS_MAX_SYSTEM_VOLUME);
  EXPECT_EQ(CRAS_MAX_SYSTEM_VOLUME, cras_system_get_volume());
  cras_system_set_volume(CRAS_MAX_SYSTEM_VOLUME + 1);
  EXPECT_EQ(CRAS_MAX_SYSTEM_VOLUME, cras_system_get_volume());
  cras_system_state_deinit();
  EXPECT_EQ(4, cras_observer_notify_output_volume_called);
}

TEST(SystemStateSuite, SetMinMaxVolume) {
  cras_system_state_init(device_config_dir);
  cras_system_set_volume_limits(-10000, -600);
  EXPECT_EQ(-10000, cras_system_get_min_volume());
  EXPECT_EQ(-600, cras_system_get_max_volume());
  cras_system_state_deinit();
}

TEST(SystemStateSuite, SetCaptureVolume) {
  cras_system_state_init(device_config_dir);
  cras_system_set_capture_gain(0);
  EXPECT_EQ(0, cras_system_get_capture_gain());
  cras_system_set_capture_gain(3000);
  EXPECT_EQ(3000, cras_system_get_capture_gain());
  // Check that it is limited to the minimum allowed gain.
  cras_system_set_capture_gain(-10000);
  EXPECT_EQ(-5000, cras_system_get_capture_gain());
  cras_system_state_deinit();
  EXPECT_EQ(3, cras_observer_notify_capture_gain_called);
}

TEST(SystemStateSuite, SetCaptureVolumeStoreTarget) {
  cras_system_state_init(device_config_dir);
  cras_system_set_capture_gain_limits(-2000, 2000);
  cras_system_set_capture_gain(3000);
  // Gain is within the limit.
  EXPECT_EQ(2000, cras_system_get_capture_gain());

  // Assume the range is changed.
  cras_system_set_capture_gain_limits(-4000, 4000);

  // Gain is also changed because target gain is re-applied.
  EXPECT_EQ(3000, cras_system_get_capture_gain());

  cras_system_state_deinit();
}

TEST(SystemStateSuite, SetMinMaxCaptureGain) {
  cras_system_state_init(device_config_dir);
  cras_system_set_capture_gain(3000);
  cras_system_set_capture_gain_limits(-2000, 2000);
  EXPECT_EQ(-2000, cras_system_get_min_capture_gain());
  EXPECT_EQ(2000, cras_system_get_max_capture_gain());
  // Current gain is adjusted for range.
  EXPECT_EQ(2000, cras_system_get_capture_gain());
  cras_system_state_deinit();
}

TEST(SystemStateSuite, SetUserMute) {
  ResetStubData();
  cras_system_state_init(device_config_dir);

  EXPECT_EQ(0, cras_system_get_mute());

  cras_system_set_user_mute(0);
  EXPECT_EQ(0, cras_system_get_mute());
  EXPECT_EQ(0, cras_observer_notify_output_mute_called);

  cras_system_set_user_mute(1);
  EXPECT_EQ(1, cras_system_get_mute());
  EXPECT_EQ(1, cras_observer_notify_output_mute_called);

  cras_system_set_user_mute(22);
  EXPECT_EQ(1, cras_system_get_mute());
  EXPECT_EQ(1, cras_observer_notify_output_mute_called);

  cras_system_state_deinit();
}

TEST(SystemStateSuite, SetMute) {
  ResetStubData();
  cras_system_state_init(device_config_dir);

  EXPECT_EQ(0, cras_system_get_mute());

  cras_system_set_mute(0);
  EXPECT_EQ(0, cras_system_get_mute());
  EXPECT_EQ(0, cras_observer_notify_output_mute_called);

  cras_system_set_mute(1);
  EXPECT_EQ(1, cras_system_get_mute());
  EXPECT_EQ(1, cras_observer_notify_output_mute_called);

  cras_system_set_mute(22);
  EXPECT_EQ(1, cras_system_get_mute());
  EXPECT_EQ(1, cras_observer_notify_output_mute_called);

  cras_system_state_deinit();
}

TEST(SystemStateSuite, CaptureMuteChangedCallbackMultiple) {
  cras_system_state_init(device_config_dir);
  ResetStubData();

  cras_system_set_capture_mute(1);
  EXPECT_EQ(1, cras_system_get_capture_mute());
  EXPECT_EQ(1, cras_observer_notify_capture_mute_called);
  cras_system_set_capture_mute(0);
  EXPECT_EQ(0, cras_system_get_capture_mute());
  EXPECT_EQ(2, cras_observer_notify_capture_mute_called);

  cras_system_state_deinit();
}

TEST(SystemStateSuite, MuteLocked) {
  cras_system_state_init(device_config_dir);
  ResetStubData();

  cras_system_set_mute(1);
  EXPECT_EQ(1, cras_system_get_mute());
  EXPECT_EQ(0, cras_system_get_mute_locked());
  EXPECT_EQ(1, cras_observer_notify_output_mute_called);

  cras_system_set_mute_locked(1);
  cras_system_set_mute(0);
  EXPECT_EQ(1, cras_system_get_mute());
  EXPECT_EQ(1, cras_system_get_mute_locked());
  EXPECT_EQ(2, cras_observer_notify_output_mute_called);

  cras_system_set_capture_mute(1);
  EXPECT_EQ(1, cras_system_get_capture_mute());
  EXPECT_EQ(0, cras_system_get_capture_mute_locked());
  EXPECT_EQ(1, cras_observer_notify_capture_mute_called);

  cras_system_set_capture_mute_locked(1);
  cras_system_set_capture_mute(0);
  EXPECT_EQ(1, cras_system_get_capture_mute());
  EXPECT_EQ(1, cras_system_get_capture_mute_locked());
  cras_system_state_deinit();
  EXPECT_EQ(2, cras_observer_notify_capture_mute_called);
}

TEST(SystemStateSuite, Suspend) {
  cras_system_state_init(device_config_dir);
  ResetStubData();

  cras_system_set_suspended(1);
  EXPECT_EQ(1, cras_observer_notify_suspend_changed_called);
  EXPECT_EQ(1, cras_system_get_suspended());

  cras_system_set_suspended(0);
  EXPECT_EQ(2, cras_observer_notify_suspend_changed_called);
  EXPECT_EQ(0, cras_system_get_suspended());

  cras_system_state_deinit();
}

TEST(SystemStateSuite, AddCardFailCreate) {
  ResetStubData();
  kFakeAlsaCard = NULL;
  cras_alsa_card_info info;

  info.card_type = ALSA_CARD_TYPE_INTERNAL;
  info.card_index = 0;
  cras_system_state_init(device_config_dir);
  EXPECT_EQ(-ENOMEM, cras_system_add_alsa_card(&info));
  EXPECT_EQ(1, cras_alsa_card_create_called);
  EXPECT_EQ(cras_alsa_card_config_dir, device_config_dir);
  cras_system_state_deinit();
}

TEST(SystemStateSuite, AddCard) {
  ResetStubData();
  cras_alsa_card_info info;

  info.card_type = ALSA_CARD_TYPE_INTERNAL;
  info.card_index = 0;
  cras_system_state_init(device_config_dir);
  EXPECT_EQ(0, cras_system_add_alsa_card(&info));
  EXPECT_EQ(1, cras_alsa_card_create_called);
  EXPECT_EQ(cras_alsa_card_config_dir, device_config_dir);
  // Adding the same card again should fail.
  ResetStubData();
  EXPECT_NE(0, cras_system_add_alsa_card(&info));
  EXPECT_EQ(0, cras_alsa_card_create_called);
  // Removing card should destroy it.
  cras_system_remove_alsa_card(0);
  EXPECT_EQ(1, cras_alsa_card_destroy_called);
  cras_system_state_deinit();
}

TEST(SystemSettingsRegisterSelectDescriptor, AddSelectFd) {
  void *stub_data = reinterpret_cast<void *>(44);
  void *select_data = reinterpret_cast<void *>(33);
  int rc;

  ResetStubData();
  cras_system_state_init(device_config_dir);
  rc = cras_system_add_select_fd(7, callback_stub, stub_data);
  EXPECT_NE(0, rc);
  EXPECT_EQ(0, add_stub_called);
  EXPECT_EQ(0, rm_stub_called);
  rc = cras_system_set_select_handler(add_stub, rm_stub, select_data);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(0, add_stub_called);
  EXPECT_EQ(0, rm_stub_called);
  rc = cras_system_set_select_handler(add_stub, rm_stub, select_data);
  EXPECT_EQ(-EEXIST, rc);
  EXPECT_EQ(0, add_stub_called);
  EXPECT_EQ(0, rm_stub_called);
  rc = cras_system_add_select_fd(7, callback_stub, stub_data);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(1, add_stub_called);
  EXPECT_EQ(select_data, select_data_value);
  cras_system_rm_select_fd(7);
  EXPECT_EQ(1, rm_stub_called);
  EXPECT_EQ(0, callback_stub_called);
  EXPECT_EQ(select_data, select_data_value);
  cras_system_state_deinit();
}

TEST(SystemSettingsStreamCount, StreamCount) {
  ResetStubData();
  cras_system_state_init(device_config_dir);

  EXPECT_EQ(0, cras_system_state_get_active_streams());
  cras_system_state_stream_added(CRAS_STREAM_OUTPUT);
  EXPECT_EQ(1, cras_system_state_get_active_streams());
  struct cras_timespec ts1;
  cras_system_state_get_last_stream_active_time(&ts1);
  cras_system_state_stream_removed(CRAS_STREAM_OUTPUT);
  EXPECT_EQ(0, cras_system_state_get_active_streams());
  struct cras_timespec ts2;
  cras_system_state_get_last_stream_active_time(&ts2);
  EXPECT_NE(0, memcmp(&ts1, &ts2, sizeof(ts1)));
  cras_system_state_deinit();
}

TEST(SystemSettingsStreamCount, StreamCountByDirection) {
  ResetStubData();
  cras_system_state_init(device_config_dir);

  EXPECT_EQ(0, cras_system_state_get_active_streams());
  cras_system_state_stream_added(CRAS_STREAM_OUTPUT);
  cras_system_state_stream_added(CRAS_STREAM_INPUT);
  cras_system_state_stream_added(CRAS_STREAM_POST_MIX_PRE_DSP);
  EXPECT_EQ(1,
	cras_system_state_get_active_streams_by_direction(
		CRAS_STREAM_OUTPUT));
  EXPECT_EQ(1,
	cras_system_state_get_active_streams_by_direction(
		CRAS_STREAM_INPUT));
  EXPECT_EQ(1,
	cras_system_state_get_active_streams_by_direction(
		CRAS_STREAM_POST_MIX_PRE_DSP));
  EXPECT_EQ(3, cras_system_state_get_active_streams());
  EXPECT_EQ(3, cras_observer_notify_num_active_streams_called);
  cras_system_state_stream_removed(CRAS_STREAM_OUTPUT);
  cras_system_state_stream_removed(CRAS_STREAM_INPUT);
  cras_system_state_stream_removed(CRAS_STREAM_POST_MIX_PRE_DSP);
  EXPECT_EQ(0,
	cras_system_state_get_active_streams_by_direction(
		CRAS_STREAM_OUTPUT));
  EXPECT_EQ(0,
	cras_system_state_get_active_streams_by_direction(
		CRAS_STREAM_INPUT));
  EXPECT_EQ(0,
	cras_system_state_get_active_streams_by_direction(
		CRAS_STREAM_POST_MIX_PRE_DSP));
  EXPECT_EQ(0, cras_system_state_get_active_streams());
  EXPECT_EQ(6, cras_observer_notify_num_active_streams_called);

  cras_system_state_deinit();
}

extern "C" {


struct cras_alsa_card *cras_alsa_card_create(struct cras_alsa_card_info *info,
	const char *device_config_dir,
	struct cras_device_blacklist *blacklist) {
  cras_alsa_card_create_called++;
  cras_alsa_card_config_dir = device_config_dir;
  return kFakeAlsaCard;
}

void cras_alsa_card_destroy(struct cras_alsa_card *alsa_card) {
  cras_alsa_card_destroy_called++;
}

size_t cras_alsa_card_get_index(const struct cras_alsa_card *alsa_card) {
  return 0;
}

struct cras_device_blacklist *cras_device_blacklist_create(
		const char *config_path)
{
	return NULL;
}

void cras_device_blacklist_destroy(struct cras_device_blacklist *blacklist)
{
}

struct cras_alert *cras_alert_create(cras_alert_prepare prepare,
                                     unsigned int flags)
{
  return NULL;
}

void cras_alert_destroy(struct cras_alert *alert)
{
}

int cras_alert_add_callback(struct cras_alert *alert, cras_alert_cb cb,
			    void *arg)
{
  add_callback_called++;
  add_callback_cb = cb;
  add_callback_arg = arg;
  return 0;
}

int cras_alert_rm_callback(struct cras_alert *alert, cras_alert_cb cb,
			   void *arg)
{
  rm_callback_called++;
  rm_callback_cb = cb;
  rm_callback_arg = arg;
  return 0;
}

void cras_alert_pending(struct cras_alert *alert)
{
  alert_pending_called++;
}

cras_tm *cras_tm_init() {
  return static_cast<cras_tm*>(malloc(sizeof(unsigned int)));
}

void cras_tm_deinit(cras_tm *tm) {
  free(tm);
}

void cras_observer_notify_output_volume(int32_t volume)
{
  cras_observer_notify_output_volume_called++;
}

void cras_observer_notify_output_mute(int muted, int user_muted,
				      int mute_locked)
{
  cras_observer_notify_output_mute_called++;
}

void cras_observer_notify_capture_gain(int32_t gain)
{
  cras_observer_notify_capture_gain_called++;
}

void cras_observer_notify_capture_mute(int muted, int mute_locked)
{
  cras_observer_notify_capture_mute_called++;
}

void cras_observer_notify_suspend_changed(int suspended)
{
  cras_observer_notify_suspend_changed_called++;
}

void cras_observer_notify_num_active_streams(enum CRAS_STREAM_DIRECTION dir,
					     uint32_t num_active_streams)
{
  cras_observer_notify_num_active_streams_called++;
}

}  // extern "C"
}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
