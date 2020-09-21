/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>

#include <ese/ese_sg.h>
#include <gtest/gtest.h>

using ::testing::Test;

class ScatterGatherTest : public virtual Test {
 public:
  ScatterGatherTest() {
  }
  virtual ~ScatterGatherTest() { }
  virtual void SetUp() { }
  virtual void TearDown() { }
};

TEST_F(ScatterGatherTest, OneToBuf) {
  uint8_t sg_data[] = "HELLO WORLD";
  struct EseSgBuffer sg = {
    .base = &sg_data[0],
    .len = sizeof(sg_data),
  };
  uint8_t dst[sizeof(sg_data) * 2];
  uint32_t copied;
  copied = ese_sg_to_buf(&sg, 1, 0, sizeof(dst), dst);
  EXPECT_EQ(copied, sizeof(sg_data));
  EXPECT_STREQ(reinterpret_cast<char *>(dst),
               reinterpret_cast<char *>(sg_data));
}

TEST_F(ScatterGatherTest, OneFromBuf) {
  uint8_t src[] = "HELLO WORLD!";
  uint8_t sg_data[sizeof(src) * 2];
  struct EseSgBuffer sg = {
    .base = &sg_data[0],
    .len = sizeof(sg_data),
  };
  uint32_t copied;
  copied = ese_sg_from_buf(&sg, 1, 0, sizeof(src), src);
  EXPECT_EQ(copied, sizeof(src));
  EXPECT_STREQ(reinterpret_cast<char *>(src),
               reinterpret_cast<char *>(sg_data));
}


TEST_F(ScatterGatherTest, ThreeToBuf) {
  uint8_t one[] = {'H', 'E', 'L'};
  uint8_t two[] = {'L', 'O', ' '};
  uint8_t three[] = "WORLD";
  struct EseSgBuffer sg[] = {
    {
      .base = one,
      .len = sizeof(one),
    },
    {
      .base = two,
      .len = sizeof(two),
    },
    {
      .base = three,
      .len = sizeof(three),
    },
  };
  uint8_t dst[256];
  uint32_t copied;
  copied = ese_sg_to_buf(sg, 3, 0, sizeof(dst), dst);
  EXPECT_EQ(copied, strlen("HELLO WORLD") + 1);
  EXPECT_STREQ(reinterpret_cast<char *>(dst),
               "HELLO WORLD");
  // Again but offset by the first bufs.
  copied = ese_sg_to_buf(sg, 3, sizeof(one) + sizeof(two) - 2, sizeof(dst),
                         dst);
  EXPECT_EQ(copied, (strlen("HELLO WORLD") + 1) -
                     (sizeof(one) + sizeof(two) - 2));
  EXPECT_STREQ(reinterpret_cast<char *>(dst),
               "O WORLD");
}


TEST_F(ScatterGatherTest, ThreeFromBuf) {
  uint8_t one[3];
  uint8_t two[3];
  uint8_t three[6];
  struct EseSgBuffer sg[] = {
    {
      .base = one,
      .len = sizeof(one),
    },
    {
      .base = two,
      .len = sizeof(two),
    },
    {
      .base = three,
      .len = sizeof(three),
    },
  };
  uint8_t src[] = "HELLO WORLD";
  uint32_t copied;
  copied = ese_sg_from_buf(sg, 3, 0, sizeof(src), src);
  EXPECT_EQ(copied, sizeof(one)  + sizeof(two) + sizeof(three));
  EXPECT_EQ(one[0], 'H');
  EXPECT_EQ(one[1], 'E');
  EXPECT_EQ(one[2], 'L');
  EXPECT_STREQ(reinterpret_cast<char *>(three), "WORLD");
  // Again but offset.
  copied = ese_sg_from_buf(sg, 3, 6, sizeof(src), src);
  EXPECT_EQ(copied, ese_sg_length(sg, 3) - 6);
  EXPECT_EQ(three[5], ' ');
  three[5] = 0; // NUL terminate for the test below.
  EXPECT_STREQ(reinterpret_cast<char *>(three), "HELLO");
}

