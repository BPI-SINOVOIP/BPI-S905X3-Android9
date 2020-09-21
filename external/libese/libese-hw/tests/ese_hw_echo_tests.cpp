/*
 * Copyright (C) 2016 The Android Open Source Project
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
 *
 * Tests a very simple end to end T=1 using the echo backend.
 */

#include <vector>
#include <gtest/gtest.h>

#include <ese/ese.h>

ESE_INCLUDE_HW(ESE_HW_ECHO);

using ::testing::Test;

class EseInterfaceTest : public virtual Test {
 public:
  EseInterfaceTest() : ese_(ESE_INITIALIZER(ESE_HW_ECHO)) {
  }
  virtual ~EseInterfaceTest() { }
  virtual void SetUp() {
    ASSERT_EQ(0, ese_open(&ese_, NULL));
  }
  virtual void TearDown() {
    ese_close(&ese_);
  }
  struct EseInterface ese_;
};

TEST_F(EseInterfaceTest, EseTranscieveOneBlock) {
  std::vector<uint8_t> apdu;
  std::vector<uint8_t> apdu_reply;
  uint8_t apdu_len = 252;
  apdu.resize(apdu_len, 'A');
  apdu_reply.resize(256, 'B');
  EXPECT_EQ(apdu_len, ese_transceive(&ese_, apdu.data(), apdu_len, apdu_reply.data(), apdu_reply.size()));
  apdu_reply.resize(apdu_len);
  EXPECT_EQ(apdu, apdu_reply);
};
