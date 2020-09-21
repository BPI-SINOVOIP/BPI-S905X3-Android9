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
 *
 */

#include <vector>
#include <gtest/gtest.h>

#include <ese/ese.h>
#include <ese/app/boot.h>
#include "../boot_private.h"

#include "ese_operations_interface.h"
#include "ese_operations_wrapper.h"

using ::testing::Test;

class FakeTransceive : public EseOperationsInterface {
 public:
  FakeTransceive() { }
  virtual ~FakeTransceive() { }

  virtual int EseOpen(struct EseInterface *ese, void *data) {
    return 0;
  }
  virtual uint32_t EseHwReceive(struct EseInterface *ese, uint8_t *data,
                                uint32_t len, int complete) { return -1; }
  virtual uint32_t EseHwTransmit(struct EseInterface *ese, const uint8_t *data,
                                 uint32_t len, int complete) { return -1; }
  virtual int EseReset(struct EseInterface *ese) { return -1; }
  virtual int EsePoll(struct EseInterface *ese, uint8_t poll_for, float timeout, int complete) {
    return -1;
  }
  virtual void EseClose(struct EseInterface *ese) { }

  virtual uint32_t EseTransceive(struct EseInterface *ese, const struct EseSgBuffer *tx_sg, uint32_t tx_nsg,
                                 struct EseSgBuffer *rx_sg, uint32_t rx_nsg) {
    // Get this calls expected data.
    EXPECT_NE(0UL, invocations.size());
    if (!invocations.size())
      return 0;
    const struct Invocation &invocation = invocations.at(0);

    uint32_t tx_total = ese_sg_length(tx_sg, tx_nsg);
    EXPECT_EQ(invocation.expected_tx.size(), tx_total);
    std::vector<uint8_t> incoming(tx_total);
    ese_sg_to_buf(tx_sg, tx_nsg, 0, tx_total, incoming.data());
    EXPECT_EQ(0, memcmp(incoming.data(), invocation.expected_tx.data(), tx_total));

    // Supply the golden return data and pop off the invocation.
    ese_sg_from_buf(rx_sg, rx_nsg, 0, invocation.rx.size(), invocation.rx.data());
    uint32_t rx_total = invocation.rx.size();
    invocations.erase(invocations.begin());
    return rx_total;
  }

  struct Invocation {
    std::vector<uint8_t> rx;
    std::vector<uint8_t> expected_tx;
  };

  std::vector<Invocation> invocations;
};

class BootAppTest : public virtual Test {
 public:
  BootAppTest() { }
  virtual ~BootAppTest() { }

  void SetUp() {
    // Configure ese with our internal ops.
    EseOperationsWrapper::InitializeEse(&ese_, &trans_);
  }

  void TearDown() {
    trans_.invocations.resize(0);
  }

 protected:
  FakeTransceive trans_;
  EseInterface ese_;
};

TEST_F(BootAppTest, EseBootSessionOpenSuccess) {
  EXPECT_EQ(0, ese_open(&ese_, NULL));
  struct EseBootSession session;
  ese_boot_session_init(&session);

  trans_.invocations.resize(2);

  trans_.invocations[0].expected_tx.resize(kManageChannelOpenLength);
  memcpy(trans_.invocations[0].expected_tx.data(), kManageChannelOpen,
    kManageChannelOpenLength);
  trans_.invocations[0].rx.resize(3);
  trans_.invocations[0].rx[0] = 0x01;  // Channel
  trans_.invocations[0].rx[1] = 0x90;  // Return code
  trans_.invocations[0].rx[2] = 0x00;

  trans_.invocations[1].expected_tx.resize(kSelectAppletLength);
  memcpy(trans_.invocations[1].expected_tx.data(), kSelectApplet,
    kSelectAppletLength);
  trans_.invocations[1].expected_tx[0] |= 0x01;  // Channel
  trans_.invocations[1].rx.resize(2);
  trans_.invocations[1].rx[0] = 0x90;
  trans_.invocations[1].rx[1] = 0x00;
  EXPECT_EQ(ESE_APP_RESULT_OK, ese_boot_session_open(&ese_, &session));
};

TEST_F(BootAppTest, EseBootSessionOpenCooldown) {
  EXPECT_EQ(0, ese_open(&ese_, NULL));
  struct EseBootSession session;
  ese_boot_session_init(&session);

  trans_.invocations.resize(1);

  trans_.invocations[0].expected_tx.resize(kManageChannelOpenLength);
  memcpy(trans_.invocations[0].expected_tx.data(), kManageChannelOpen,
    kManageChannelOpenLength);
  trans_.invocations[0].rx.resize(2);
  trans_.invocations[0].rx[0] = 0x66;  // Return code
  trans_.invocations[0].rx[1] = 0xA5;
  // This return code should allow a subsequent call of
  // ese_boot_cooldown_values();
  EXPECT_EQ(ESE_APP_RESULT_ERROR_COOLDOWN, ese_boot_session_open(&ese_, &session));
};

TEST_F(BootAppTest, EseBootSessionOpenSelectFailure) {
  EXPECT_EQ(0, ese_open(&ese_, NULL));
  struct EseBootSession session;
  ese_boot_session_init(&session);

  trans_.invocations.resize(2);

  trans_.invocations[0].expected_tx.resize(kManageChannelOpenLength);
  memcpy(trans_.invocations[0].expected_tx.data(), kManageChannelOpen,
    kManageChannelOpenLength);
  trans_.invocations[0].rx.resize(3);
  trans_.invocations[0].rx[0] = 0x01;  // Channel
  trans_.invocations[0].rx[1] = 0x90;  // Return code
  trans_.invocations[0].rx[2] = 0x00;

  trans_.invocations[1].expected_tx.resize(kSelectAppletLength);
  memcpy(trans_.invocations[1].expected_tx.data(), kSelectApplet,
    kSelectAppletLength);
  trans_.invocations[1].expected_tx[0] |= 0x01;  // Channel
  trans_.invocations[1].rx.resize(2);
  trans_.invocations[1].rx[0] = 0x90;
  trans_.invocations[1].rx[1] = 0x01;
  EXPECT_EQ(ESE_APP_RESULT_ERROR_OS, ese_boot_session_open(&ese_, &session));
};
