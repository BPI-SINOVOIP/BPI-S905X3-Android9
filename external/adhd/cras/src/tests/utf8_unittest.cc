// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Some UTF character seqeuences in this file were taken from
// https://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt

#include <gtest/gtest.h>
#include <stdio.h>

extern "C" {
#include "cras_utf8.h"
}

namespace {

TEST(UTF8, ValidStress) {
  size_t pos;

  EXPECT_EQ(1, valid_utf8_string("The greek word 'kosme': "
                                 "\xce\xba\xe1\xbd\xb9\xcf\x83\xce"
                                 "\xbc\xce\xb5", &pos));
  EXPECT_EQ(35, pos);

  EXPECT_EQ(1, valid_utf8_string("Playback", &pos));
  EXPECT_EQ(8, pos);

  EXPECT_EQ(1, valid_utf8_string("The Euro sign: \xe2\x82\xac", &pos));
  EXPECT_EQ(18, pos);

  /* First possible sequence of a certain length. */
  EXPECT_EQ(1, valid_utf8_string("\x01", &pos));
  EXPECT_EQ(1, pos);
  EXPECT_EQ(1, valid_utf8_string("\xc2\x80", &pos));
  EXPECT_EQ(2, pos);
  EXPECT_EQ(1, valid_utf8_string("\xe0\xa0\x80", &pos));
  EXPECT_EQ(3, pos);
  EXPECT_EQ(1, valid_utf8_string("\xe1\x80\x80", &pos));
  EXPECT_EQ(3, pos);
  EXPECT_EQ(1, valid_utf8_string("\xf0\x90\x80\x80", &pos));
  EXPECT_EQ(4, pos);
  EXPECT_EQ(1, valid_utf8_string("\xf1\x80\x80\x80", &pos));
  EXPECT_EQ(4, pos);

  /* Last possible sequence of a certain length. */
  EXPECT_EQ(1, valid_utf8_string("\x7f", &pos));
  EXPECT_EQ(1, pos);
  EXPECT_EQ(1, valid_utf8_string("\xdf\xbf", &pos));
  EXPECT_EQ(2, pos);
  EXPECT_EQ(1, valid_utf8_string("\xef\xbf\xbf", &pos));
  EXPECT_EQ(3, pos);
  EXPECT_EQ(1, valid_utf8_string("\xf4\x8f\xbf\xbf", &pos));
  EXPECT_EQ(4, pos);

  /* Other boundary conditions. */
  EXPECT_EQ(1, valid_utf8_string("\xed\x9f\xbf", &pos));
  EXPECT_EQ(3, pos);
  EXPECT_EQ(1, valid_utf8_string("\xee\x80\x80", &pos));
  EXPECT_EQ(3, pos);
  EXPECT_EQ(1, valid_utf8_string("\xef\xbf\xbd", &pos));
  EXPECT_EQ(3, pos);
  EXPECT_EQ(1, valid_utf8_string("\xf0\xbf\xbf\xbf", &pos));
  EXPECT_EQ(4, pos);

  /* BOM sequence. */
  EXPECT_EQ(1, valid_utf8_string("\xef\xbb\xbf", &pos));
  EXPECT_EQ(3, pos);

  /* Valid UTF-8 that shouldn't appear in text; chose to allow
   * these characters anyway. */
  EXPECT_EQ(1, valid_utf8_string("U+FFFE: \xef\xbf\xbe", &pos));
  EXPECT_EQ(11, pos);
  EXPECT_EQ(1, valid_utf8_string("U+FDD0: \xef\xb7\x90", &pos));
  EXPECT_EQ(11, pos);
  EXPECT_EQ(1, valid_utf8_string("\xf0\x9f\xbf\xbe", &pos));
  EXPECT_EQ(4, pos);
}

TEST(UTF8, InvalidStress) {
  size_t pos;

  /* Malformed continuation bytes. */
  EXPECT_EQ(0, valid_utf8_string("\x80", &pos));
  EXPECT_EQ(0, pos);
  EXPECT_EQ(0, valid_utf8_string("\xbf", &pos));
  EXPECT_EQ(0, pos);
  EXPECT_EQ(0, valid_utf8_string("\x80\xbf", &pos));
  EXPECT_EQ(0, pos);
  EXPECT_EQ(0, valid_utf8_string("\xc2\x80\xbf", &pos));
  EXPECT_EQ(2, pos);

  /* Lonely start characters. */
  EXPECT_EQ(0, valid_utf8_string("\xc2 \xc3 \xc4 ", &pos));
  EXPECT_EQ(1, pos);

  /* Out of range cases. */
  EXPECT_EQ(0, valid_utf8_string("\xf4\x90\xbf\xbf", &pos));
  EXPECT_EQ(1, pos);
  EXPECT_EQ(0, valid_utf8_string(" \xf5\x80", &pos));
  EXPECT_EQ(1, pos);
  EXPECT_EQ(0, valid_utf8_string(" \xe0\x80\x80", &pos));
  EXPECT_EQ(2, pos);
  EXPECT_EQ(0, valid_utf8_string("\xf4\x80\x80\xcf", &pos));
  EXPECT_EQ(3, pos);

  /* Stop in mid-sequence. */
  EXPECT_EQ(0, valid_utf8_string("\xf4\x80", &pos));
  EXPECT_EQ(2, pos);

  /* Bad characters. */
  EXPECT_EQ(0, valid_utf8_string("\xff", &pos));
  EXPECT_EQ(0, pos);
  EXPECT_EQ(0, valid_utf8_string("\xfe", &pos));
  EXPECT_EQ(0, pos);

  /* Overlong representations of ASCII characters. */
  EXPECT_EQ(0, valid_utf8_string("This represents the / character with too"
                                 "many bytes: \xe0\x80\xaf", &pos));
  EXPECT_EQ(53, pos);
  EXPECT_EQ(0, valid_utf8_string("This represents the / character with too"
                                 "many bytes: \xf0\x80\x80\xaf", &pos));
  EXPECT_EQ(53, pos);

  /* Should not be interpreted as the ASCII NUL character. */
  EXPECT_EQ(0, valid_utf8_string("This represents the NUL character with too"
                                 "many bytes: \xe0\x80\x80", &pos));
  EXPECT_EQ(55, pos);
  EXPECT_EQ(0, valid_utf8_string("This represents the NUL character with too"
                                 "many bytes: \xf0\x80\x80\x80", &pos));
  EXPECT_EQ(55, pos);

  /* Single UTF-16 surrogates. */
  EXPECT_EQ(0, valid_utf8_string("\xed\xa0\x80", &pos));
  EXPECT_EQ(1, pos);
  EXPECT_EQ(0, valid_utf8_string("\xed\xad\xbf", &pos));
  EXPECT_EQ(1, pos);
  EXPECT_EQ(0, valid_utf8_string("\xed\xae\x80", &pos));
  EXPECT_EQ(1, pos);
  EXPECT_EQ(0, valid_utf8_string("\xed\xaf\xbf", &pos));
  EXPECT_EQ(1, pos);
  EXPECT_EQ(0, valid_utf8_string("\xed\xb0\x80", &pos));
  EXPECT_EQ(1, pos);
  EXPECT_EQ(0, valid_utf8_string("\xed\xbe\x80", &pos));
  EXPECT_EQ(1, pos);
  EXPECT_EQ(0, valid_utf8_string("\xed\xbf\xbf", &pos));
  EXPECT_EQ(1, pos);
}

}  //  namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
