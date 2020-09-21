// Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "cras_alert.h"

namespace {

void callback1(void *arg, void *data);
void callback2(void *arg, void *data);
void prepare(struct cras_alert *alert);

static int cb1_called = 0;
static void *cb1_data;
static int cb2_called = 0;
static int cb2_set_pending = 0;
static int prepare_called = 0;

void ResetStub() {
  cb1_called = 0;
  cb2_called = 0;
  cb2_set_pending = 0;
  prepare_called = 0;
}

TEST(Alert, OneCallback) {
  struct cras_alert *alert = cras_alert_create(NULL, 0);
  cras_alert_add_callback(alert, &callback1, NULL);
  ResetStub();
  cras_alert_pending(alert);
  EXPECT_EQ(0, cb1_called);
  cras_alert_process_all_pending_alerts();
  EXPECT_EQ(1, cb1_called);
  cras_alert_destroy(alert);
}

TEST(Alert, OneCallbackPost2Call1) {
  struct cras_alert *alert = cras_alert_create(NULL, 0);
  cras_alert_add_callback(alert, &callback1, NULL);
  ResetStub();
  // Alert twice, callback should only be called once.
  cras_alert_pending(alert);
  cras_alert_pending(alert);
  EXPECT_EQ(0, cb1_called);
  cras_alert_process_all_pending_alerts();
  EXPECT_EQ(1, cb1_called);
  cras_alert_destroy(alert);
}

TEST(Alert, OneCallbackWithData) {
  struct cras_alert *alert = cras_alert_create(NULL, 0);
  const char *data = "ThisIsMyData";
  cras_alert_add_callback(alert, &callback1, NULL);
  ResetStub();
  cras_alert_pending_data(alert, (void *)data, strlen(data) + 1);
  EXPECT_EQ(0, cb1_called);
  cras_alert_process_all_pending_alerts();
  EXPECT_EQ(1, cb1_called);
  EXPECT_EQ(0, strcmp(data, (const char *)cb1_data));
  cras_alert_destroy(alert);
}

TEST(Alert, OneCallbackTwoDataCalledOnce) {
  struct cras_alert *alert = cras_alert_create(NULL, 0);
  const char *data = "ThisIsMyData";
  const char *data2 = "ThisIsMyData2";
  cras_alert_add_callback(alert, &callback1, NULL);
  ResetStub();
  // Callback called with last data only.
  cras_alert_pending_data(alert, (void *)data, strlen(data) + 1);
  cras_alert_pending_data(alert, (void *)data2, strlen(data2) + 1);
  EXPECT_EQ(0, cb1_called);
  cras_alert_process_all_pending_alerts();
  EXPECT_EQ(1, cb1_called);
  EXPECT_EQ(0, strcmp(data2, (const char *)cb1_data));
  cras_alert_destroy(alert);
}

TEST(Alert, OneCallbackTwoDataKeepAll) {
  struct cras_alert *alert = cras_alert_create(
                                 NULL, CRAS_ALERT_FLAG_KEEP_ALL_DATA);
  const char *data = "ThisIsMyData";
  const char *data2 = "ThisIsMyData2";
  cras_alert_add_callback(alert, &callback1, NULL);
  ResetStub();
  // Callbacks with data should each be called.
  cras_alert_pending_data(alert, (void *)data, strlen(data) + 1);
  cras_alert_pending_data(alert, (void *)data2, strlen(data2) + 1);
  EXPECT_EQ(0, cb1_called);
  cras_alert_process_all_pending_alerts();
  EXPECT_EQ(2, cb1_called);
  EXPECT_EQ(0, strcmp(data2, (const char *)cb1_data));
  cras_alert_destroy(alert);
}

TEST(Alert, TwoCallbacks) {
  struct cras_alert *alert = cras_alert_create(NULL, 0);
  cras_alert_add_callback(alert, &callback1, NULL);
  cras_alert_add_callback(alert, &callback2, NULL);
  ResetStub();
  cras_alert_pending(alert);
  EXPECT_EQ(0, cb1_called);
  EXPECT_EQ(0, cb2_called);
  cras_alert_process_all_pending_alerts();
  EXPECT_EQ(1, cb1_called);
  EXPECT_EQ(1, cb2_called);
  cras_alert_destroy(alert);
}

TEST(Alert, NoPending) {
  struct cras_alert *alert = cras_alert_create(NULL, 0);
  cras_alert_add_callback(alert, &callback1, NULL);
  ResetStub();
  EXPECT_EQ(0, cb1_called);
  cras_alert_process_all_pending_alerts();
  EXPECT_EQ(0, cb1_called);
  cras_alert_destroy(alert);
}

TEST(Alert, PendingInCallback) {
  struct cras_alert *alert1 = cras_alert_create(NULL, 0);
  struct cras_alert *alert2 = cras_alert_create(NULL, 0);
  cras_alert_add_callback(alert1, &callback1, NULL);
  cras_alert_add_callback(alert2, &callback2, alert1);
  ResetStub();
  cras_alert_pending(alert2);
  EXPECT_EQ(0, cb1_called);
  EXPECT_EQ(0, cb2_called);
  cb2_set_pending = 1;
  cras_alert_process_all_pending_alerts();
  EXPECT_EQ(1, cb1_called);
  EXPECT_EQ(1, cb2_called);
  cras_alert_destroy(alert1);
  cras_alert_destroy(alert2);
}

TEST(Alert, Prepare) {
  struct cras_alert *alert = cras_alert_create(prepare, 0);
  cras_alert_add_callback(alert, &callback1, NULL);
  ResetStub();
  cras_alert_pending(alert);
  EXPECT_EQ(0, cb1_called);
  cras_alert_process_all_pending_alerts();
  EXPECT_EQ(1, prepare_called);
  EXPECT_EQ(1, cb1_called);
  cras_alert_destroy(alert);
}

TEST(Alert, TwoAlerts) {
  struct cras_alert *alert1 = cras_alert_create(prepare, 0);
  struct cras_alert *alert2 = cras_alert_create(prepare, 0);
  cras_alert_add_callback(alert1, &callback1, NULL);
  cras_alert_add_callback(alert2, &callback2, NULL);

  ResetStub();
  cras_alert_pending(alert1);
  EXPECT_EQ(0, cb1_called);
  EXPECT_EQ(0, cb2_called);
  cras_alert_process_all_pending_alerts();
  EXPECT_EQ(1, prepare_called);
  EXPECT_EQ(1, cb1_called);
  EXPECT_EQ(0, cb2_called);

  ResetStub();
  cras_alert_pending(alert2);
  EXPECT_EQ(0, cb1_called);
  EXPECT_EQ(0, cb2_called);
  cras_alert_process_all_pending_alerts();
  EXPECT_EQ(1, prepare_called);
  EXPECT_EQ(0, cb1_called);
  EXPECT_EQ(1, cb2_called);

  ResetStub();
  cras_alert_pending(alert1);
  cras_alert_pending(alert2);
  EXPECT_EQ(0, cb1_called);
  EXPECT_EQ(0, cb2_called);
  cras_alert_process_all_pending_alerts();
  EXPECT_EQ(2, prepare_called);
  EXPECT_EQ(1, cb1_called);
  EXPECT_EQ(1, cb2_called);

  cras_alert_destroy_all();
}

void callback1(void *arg, void *data)
{
  cb1_called++;
  cb1_data = data;
}

void callback2(void *arg, void *data)
{
  cb2_called++;
  if (cb2_set_pending) {
    cb2_set_pending = 0;
    cras_alert_pending((struct cras_alert *)arg);
  }
}

void prepare(struct cras_alert *alert)
{
  prepare_called++;
  return;
}

}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
