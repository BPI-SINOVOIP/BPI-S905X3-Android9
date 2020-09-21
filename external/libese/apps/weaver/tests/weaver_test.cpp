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

#include <gtest/gtest.h>

#include <include/ese/app/weaver.h>

#include <esecpp/NxpPn80tNqNci.h>
using EseInterfaceImpl = android::NxpPn80tNqNci;

const uint8_t KEY[kEseWeaverKeySize] = {
  1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
const uint8_t WRONG_KEY[kEseWeaverKeySize] = {0};
const uint8_t VALUE[kEseWeaverValueSize] = {
  16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};

struct WeaverTest : public ::testing::Test {
  EseInterfaceImpl mEse;
  EseWeaverSession mSession;

  virtual void SetUp() override {
    mEse.init();
    if (mEse.open() < 0) {
      std::string errMsg = "Failed to open connection to eSE";
      if (mEse.error()) {
          errMsg += " (" + std::to_string(mEse.error_code()) + "): " + mEse.error_message();
      } else {
          errMsg += ": reason unknown";
      }
      FAIL() << errMsg;
    }

    ese_weaver_session_init(&mSession);
    ASSERT_EQ(ESE_APP_RESULT_OK, ese_weaver_session_open(mEse.ese_interface(), &mSession));
  }

  virtual void TearDown() override {
    ASSERT_EQ(ESE_APP_RESULT_OK, ese_weaver_session_close(&mSession));
    mEse.close();
  }
};

TEST_F(WeaverTest, getNumSlots) {
  uint32_t numSlots;
  ASSERT_EQ(ESE_APP_RESULT_OK, ese_weaver_get_num_slots(&mSession, &numSlots));
  ASSERT_EQ(uint32_t{64}, numSlots);
}

TEST_F(WeaverTest, writeAndReadWithCorrectKey) {
  const uint32_t slotId = 3;
  ASSERT_EQ(ese_weaver_write(&mSession, slotId, KEY, VALUE), ESE_APP_RESULT_OK);

  uint8_t readValue[kEseWeaverValueSize];
  uint32_t timeout;
  ASSERT_EQ(ESE_APP_RESULT_OK, ese_weaver_read(&mSession, slotId, KEY, readValue, &timeout));
  ASSERT_EQ(0, memcmp(VALUE, readValue, kEseWeaverValueSize));
}

TEST_F(WeaverTest, writeAndReadWithIncorrectKey) {
  const uint32_t slotId = 7;
  ASSERT_EQ(ese_weaver_write(&mSession, slotId, KEY, VALUE), ESE_APP_RESULT_OK);

  uint8_t readValue[kEseWeaverValueSize];
  uint32_t timeout;
  ASSERT_EQ(ESE_WEAVER_READ_WRONG_KEY,
            ese_weaver_read(&mSession, slotId, WRONG_KEY, readValue, &timeout));
  ASSERT_NE(0, memcmp(VALUE, readValue, kEseWeaverValueSize));
  ASSERT_EQ(uint32_t{0}, timeout); // First timeout is 0
}

TEST_F(WeaverTest, writeAndEraseValue) {
  const uint32_t slotId = 0;
  ASSERT_EQ(ese_weaver_write(&mSession, slotId, KEY, VALUE), ESE_APP_RESULT_OK);
  ASSERT_EQ(ese_weaver_erase_value(&mSession, slotId), ESE_APP_RESULT_OK);

  // The read should be successful as the key is unchanged but the value should
  // be all zeros
  uint8_t readValue[kEseWeaverValueSize];
  uint32_t timeout;
  ASSERT_EQ(ESE_APP_RESULT_OK, ese_weaver_read(&mSession, slotId, KEY, readValue, &timeout));

  const uint8_t expectedValue[kEseWeaverValueSize] = {0};
  ASSERT_EQ(0, memcmp(readValue, expectedValue, kEseWeaverValueSize));
}
