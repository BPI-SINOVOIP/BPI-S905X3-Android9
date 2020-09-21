// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <gtest/gtest.h>

extern "C" {
#include "cras_rstream.h"
#include "stream_list.h"
}

namespace {

static unsigned int add_called;
static int added_cb(struct cras_rstream *rstream) {
  add_called++;
  return 0;
}

static unsigned int rm_called;
static struct cras_rstream *rmed_stream;
static int removed_cb(struct cras_rstream *rstream) {
  rm_called++;
  rmed_stream = rstream;
  return 0;
}

static unsigned int create_called;
static struct cras_rstream_config *create_config;
static struct cras_rstream dummy_rstream;
static int create_rstream_cb(struct cras_rstream_config *stream_config,
                             struct cras_rstream **stream) {
  create_called++;
  create_config = stream_config;
  *stream = &dummy_rstream;
  dummy_rstream.stream_id = 0x3003;
  return 0;
}

static unsigned int destroy_called;
static struct cras_rstream *destroyed_stream;
static void destroy_rstream_cb(struct cras_rstream *rstream) {
  destroy_called++;
  destroyed_stream = rstream;
}

static void reset_test_data() {
  add_called = 0;
  rm_called = 0;
  create_called = 0;
  destroy_called = 0;
}

TEST(StreamList, AddRemove) {
  struct stream_list *l;
  struct cras_rstream *s1;
  struct cras_rstream_config s1_config;

  reset_test_data();
  l = stream_list_create(added_cb, removed_cb, create_rstream_cb,
                         destroy_rstream_cb, NULL);
  stream_list_add(l, &s1_config, &s1);
  EXPECT_EQ(1, add_called);
  EXPECT_EQ(1, create_called);
  EXPECT_EQ(&s1_config, create_config);
  EXPECT_EQ(0, stream_list_rm(l, 0x3003));
  EXPECT_EQ(1, rm_called);
  EXPECT_EQ(s1, rmed_stream);
  EXPECT_EQ(1, destroy_called);
  EXPECT_EQ(s1, destroyed_stream);
  stream_list_destroy(l);
}

extern "C" {

struct cras_timer *cras_tm_create_timer(
                struct cras_tm *tm,
                unsigned int ms,
                void (*cb)(struct cras_timer *t, void *data),
                void *cb_data) {
  return reinterpret_cast<struct cras_timer *>(0x404);
}

void cras_tm_cancel_timer(struct cras_tm *tm, struct cras_timer *t) {
}

}

} // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
