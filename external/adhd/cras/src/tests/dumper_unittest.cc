// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <syslog.h>
#include <gtest/gtest.h>

#include "dumper.h"

namespace {

TEST(DumperTest, SyslogDumper) {
  struct dumper *dumper = syslog_dumper_create(LOG_ERR);
  dumpf(dumper, "hello %d", 1);
  dumpf(dumper, "world %d\n123", 2);
  dumpf(dumper, "456\n");
  // The following should appear in syslog:
  // dumper_unittest: hello 1world 2
  // dumper_unittest: 123456
  syslog_dumper_free(dumper);
}

TEST(DumperTest, MemDumper) {
  struct dumper *dumper = mem_dumper_create();
  char *buf;
  int size, i;

  mem_dumper_get(dumper, &buf, &size);
  EXPECT_STREQ("", buf);

  dumpf(dumper, "hello %d\n", 1);
  mem_dumper_get(dumper, &buf, &size);
  EXPECT_STREQ("hello 1\n", buf);
  EXPECT_EQ(8, size);

  dumpf(dumper, "world %d", 2);
  mem_dumper_get(dumper, &buf, &size);
  EXPECT_STREQ("hello 1\nworld 2", buf);
  EXPECT_EQ(15, size);

  mem_dumper_clear(dumper);
  mem_dumper_get(dumper, &buf, &size);
  EXPECT_STREQ("", buf);
  EXPECT_EQ(0, size);

  for (i = 0; i < 1000; i++) {
    dumpf(dumper, "a");
  }
  mem_dumper_get(dumper, &buf, &size);
  EXPECT_EQ(1000, strlen(buf));
  EXPECT_EQ(1000, 1000);

  mem_dumper_free(dumper);
}

}  //  namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
