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

#include <ese/ese.h>
#include <gtest/gtest.h>

ESE_INCLUDE_HW(ESE_HW_FAKE);

using ::testing::Test;

class EseInterfaceTest : public virtual Test {
 public:
  EseInterfaceTest() : ese_(ESE_INITIALIZER(ESE_HW_FAKE)) {
  }
  virtual ~EseInterfaceTest() { }
  virtual void SetUp() { }
  virtual void TearDown() { }
  struct EseInterface ese_;
};

TEST_F(EseInterfaceTest, EseNameNull) {
  EXPECT_STREQ("NULL EseInterface", ese_name(NULL));
};

TEST_F(EseInterfaceTest, EseNameOk) {
  EXPECT_STREQ("eSE Fake Hardware", ese_name(&ese_));
};

TEST_F(EseInterfaceTest, EseNameUnknown) {
  struct EseOperations dummy_ops = {
    .name = NULL,
  };
  struct EseInterface dummy = {
    .ops = &dummy_ops
  };
  EXPECT_STREQ("unknown hw", ese_name(&dummy));
};

TEST_F(EseInterfaceTest, EseOpenNull) {
  EXPECT_EQ(-1, ese_open(NULL, NULL));
};

TEST_F(EseInterfaceTest, EseOpenNoOp) {
  struct EseOperations dummy_ops = {
    .open = NULL,
  };
  struct EseInterface dummy = {
    .ops = &dummy_ops
  };
  EXPECT_EQ(0, ese_open(&dummy, NULL));
};

TEST_F(EseInterfaceTest, EseOpenOk) {
  EXPECT_EQ(0, ese_open(&ese_, NULL));
};

TEST_F(EseInterfaceTest, EseCloseNull) {
  ese_close(NULL);
};

TEST_F(EseInterfaceTest, EseCloseNoOp) {
  struct EseOperations dummy_ops = {
    .close = NULL,
  };
  struct EseInterface dummy = {
    .ops = &dummy_ops
  };
  /* Will pass without an open first. */
  ese_close(&dummy);
};

TEST_F(EseInterfaceTest, EseCloseOk) {
  EXPECT_EQ(0, ese_open(&ese_, NULL));
  ese_close(&ese_);
};

TEST_F(EseInterfaceTest, EseClosePending) {
  EXPECT_EQ(0, ese_open(&ese_, NULL));
  ese_.ops->hw_transmit(&ese_, NULL, 0, 0);
  ese_close(&ese_);
  EXPECT_EQ(0, ese_open(&ese_, NULL));
  ese_.ops->hw_transmit(&ese_, NULL, 0, 1);
  ese_.ops->hw_receive(&ese_, NULL, 0, 0);
  ese_close(&ese_);
  EXPECT_EQ(0, ese_open(&ese_, NULL));
  ese_.ops->hw_receive(&ese_, NULL, 0, 1);
  ese_close(&ese_);
};


TEST_F(EseInterfaceTest, EseTransceiveSendNothing) {
  EXPECT_EQ(0, ese_open(&ese_, NULL));
  // Relying on hw transmit/recieve alone is not supported.
  uint8_t *tx = NULL;
  uint8_t *rx = NULL;
  EXPECT_EQ(-1, ese_transceive(&ese_, tx, 0, rx, 0));
  EXPECT_EQ(-1, ese_transceive(&ese_, tx+1, 0, rx, 0));
  EXPECT_EQ(-1, ese_transceive(&ese_, tx, 0, rx+1, 0));
  EXPECT_EQ(-1, ese_transceive(&ese_, tx, 0, rx, 100));
  EXPECT_EQ(-1, ese_transceive(&ese_, tx, 220, rx, 100));
  EXPECT_EQ(-1, ese_transceive(&ese_, tx+1, 0, rx+1, 0));
  ese_close(&ese_);
};

TEST_F(EseInterfaceTest, EseTransceiveNoOps) {
  struct EseOperations dummy_ops = {
    .open = NULL,
    .close = NULL,
    .transceive = NULL,
    .hw_transmit = NULL,
    .hw_receive = NULL,
  };
  struct EseInterface dummy = {
    .ops = &dummy_ops
  };
  /* Will pass without an open first. */
  EXPECT_EQ(-1, ese_transceive(&dummy, NULL, 0, NULL, 0));
  EXPECT_EQ(1, ese_error(&dummy));
  EXPECT_EQ(-1, ese_error_code(&dummy));
  EXPECT_STREQ("Hardware supplied no transceive implementation.",
               ese_error_message(&dummy));
};


