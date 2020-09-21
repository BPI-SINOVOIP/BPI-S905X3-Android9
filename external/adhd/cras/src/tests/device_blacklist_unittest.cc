// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <gtest/gtest.h>

extern "C" {
#include "cras_device_blacklist.h"
}

namespace {

static const char CONFIG_PATH[] = CRAS_UT_TMPDIR;
static const char CONFIG_FILENAME[] = "device_blacklist";

void CreateConfigFile(const char* config_text) {
  FILE* f;
  char card_path[128];

  snprintf(card_path, sizeof(card_path), "%s/%s", CONFIG_PATH, CONFIG_FILENAME);
  f = fopen(card_path, "w");
  if (f == NULL)
    return;

  fprintf(f, "%s", config_text);

  fclose(f);
}

TEST(Blacklist, EmptyBlacklist) {
  static const char empty_config_text[] = "";
  struct cras_device_blacklist *blacklist;

  CreateConfigFile(empty_config_text);

  blacklist = cras_device_blacklist_create(CONFIG_PATH);
  ASSERT_NE(static_cast<cras_device_blacklist*>(NULL), blacklist);
  EXPECT_EQ(0, cras_device_blacklist_check(blacklist, 0x0d8c, 0x0008, 0, 0));

  cras_device_blacklist_destroy(blacklist);
}

TEST(Blacklist, BlackListOneUsbOutput) {
  static const char usb_output_config_text[] =
      "[USB_Outputs]\n"
      "0d8c_0008_00000012_0 = 1\n";
  struct cras_device_blacklist *blacklist;

  CreateConfigFile(usb_output_config_text);

  blacklist = cras_device_blacklist_create(CONFIG_PATH);
  ASSERT_NE(static_cast<cras_device_blacklist*>(NULL), blacklist);

  EXPECT_EQ(0, cras_device_blacklist_check(blacklist, 0x0d8d, 0x0008, 0x12, 0));
  EXPECT_EQ(0, cras_device_blacklist_check(blacklist, 0x0d8c, 0x0009, 0x12, 0));
  EXPECT_EQ(0, cras_device_blacklist_check(blacklist, 0x0d8c, 0x0008, 0x13, 0));
  EXPECT_EQ(0, cras_device_blacklist_check(blacklist, 0x0d8c, 0x0008, 0x12, 1));
  EXPECT_EQ(1, cras_device_blacklist_check(blacklist, 0x0d8c, 0x0008, 0x12, 0));

  cras_device_blacklist_destroy(blacklist);
}

TEST(Blacklist, BlackListTwoUsbOutput) {
  static const char usb_output_config_text[] =
      "[USB_Outputs]\n"
      "0d8c_0008_00000000_0 = 1\n"
      "0d8c_0009_00000000_0 = 1\n";
  struct cras_device_blacklist *blacklist;

  CreateConfigFile(usb_output_config_text);

  blacklist = cras_device_blacklist_create(CONFIG_PATH);
  ASSERT_NE(static_cast<cras_device_blacklist*>(NULL), blacklist);

  EXPECT_EQ(1, cras_device_blacklist_check(blacklist, 0x0d8c, 0x0009, 0, 0));
  EXPECT_EQ(1, cras_device_blacklist_check(blacklist, 0x0d8c, 0x0008, 0, 0));
  EXPECT_EQ(0, cras_device_blacklist_check(blacklist, 0x0d8c, 0x0008, 0, 1));

  cras_device_blacklist_destroy(blacklist);
}

}  //  namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
