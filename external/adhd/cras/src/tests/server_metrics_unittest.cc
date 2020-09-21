// Copyright 2016 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <gtest/gtest.h>

extern "C" {
#include "cras_server_metrics.c"
#include "cras_main_message.h"
}

static enum CRAS_MAIN_MESSAGE_TYPE type_set;
static struct cras_server_metrics_message *sent_msg;

void ResetStubData() {
  type_set = (enum CRAS_MAIN_MESSAGE_TYPE)0;
}

namespace {

TEST(ServerMetricsTestSuite, Init) {
  ResetStubData();

  cras_server_metrics_init();

  EXPECT_EQ(type_set, CRAS_MAIN_METRICS);
}

TEST(ServerMetricsTestSuite, SetMetrics) {
  ResetStubData();
  unsigned int delay = 100;
  sent_msg = (struct cras_server_metrics_message *)calloc(1, sizeof(*sent_msg));

  cras_server_metrics_longest_fetch_delay(delay);

  EXPECT_EQ(sent_msg->header.type, CRAS_MAIN_METRICS);
  EXPECT_EQ(sent_msg->header.length, sizeof(*sent_msg));
  EXPECT_EQ(sent_msg->metrics_type, LONGEST_FETCH_DELAY);
  EXPECT_EQ(sent_msg->data, delay);

  free(sent_msg);
}

extern "C" {

int cras_main_message_add_handler(enum CRAS_MAIN_MESSAGE_TYPE type,
                                  cras_message_callback callback,
                                  void *callback_data) {
  type_set = type;
  return 0;
}

void cras_metrics_log_histogram(const char *name, int sample, int min,
                                int max, int nbuckets) {
}

int cras_main_message_send(struct cras_main_message *msg) {
  // Copy the sent message so we can examine it in the test later.
  memcpy(sent_msg, msg, sizeof(*sent_msg));
  return 0;
};

}  // extern "C"
}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  int rc = RUN_ALL_TESTS();

  return rc;
}
