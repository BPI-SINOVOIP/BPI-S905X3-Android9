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
 * Tests a very simple end to end T=1 using the echo backend.
 */

#include <string.h>

#include <vector>
#include <gtest/gtest.h>

#include <ese/ese.h>
#include <ese/teq1.h>
#define LOG_TAG "TEQ1_UNITTESTS"
#include <ese/log.h>

#include "ese_operations_interface.h"
#include "ese_operations_wrapper.h"

#include "teq1_private.h"

#define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))

using ::testing::Test;

// TODO:
// - Unittests of each function
// - teq1_rules matches Annex A of ISO 7816-3

// Tests teq1_frame_error_check to avoid testing every combo that
// ends in 255 in the rule engine.
class Teq1FrameErrorCheck : public virtual Test {
 public:
  Teq1FrameErrorCheck() { }
  virtual ~Teq1FrameErrorCheck() { }

  struct Teq1Frame tx_frame_, rx_frame_;
  struct Teq1State state_;
  struct Teq1CardState card_state_;
};

TEST_F(Teq1FrameErrorCheck, info_parity) {
  static const uint8_t kRxPCBs[] = {
    TEQ1_I(0, 0),
    TEQ1_I(1, 0),
    TEQ1_I(0, 1),
    TEQ1_I(1, 1),
    255,
  };
  const uint8_t *pcb = &kRxPCBs[0];
  /* The PCBs above are all valid for a sent unchained I block with advancing
   * sequence #s.
   */
  tx_frame_.header.PCB = TEQ1_I(0, 0);
  state_.card_state = &card_state_;
  state_.card_state->seq.card = 1;
  while (*pcb != 255) {
    rx_frame_.header.PCB = *pcb;
    rx_frame_.header.LEN = 2;
    rx_frame_.INF[0] = 'A';
    rx_frame_.INF[1] = 'B';
    rx_frame_.INF[2] = teq1_compute_LRC(&rx_frame_);
    EXPECT_EQ(0, teq1_frame_error_check(&state_, &tx_frame_, &rx_frame_)) << teq1_pcb_to_name(rx_frame_.header.PCB);
    rx_frame_.INF[2] = teq1_compute_LRC(&rx_frame_) - 1;
    // Reset so we check the LRC error instead of a wrong seq.
    state_.card_state->seq.card = !state_.card_state->seq.card;
    EXPECT_EQ(TEQ1_R(0, 0, 1), teq1_frame_error_check(&state_, &tx_frame_, &rx_frame_));
    state_.card_state->seq.card = !state_.card_state->seq.card;
    pcb++;
  }
};

TEST_F(Teq1FrameErrorCheck, length_mismatch) {
};

TEST_F(Teq1FrameErrorCheck, unchained_r_block) {
};

TEST_F(Teq1FrameErrorCheck, unexpected_seq) {
};

class Teq1RulesTest : public virtual Test {
 public:
  Teq1RulesTest() :
    tx_data_(INF_LEN, 'A'),
    rx_data_(INF_LEN, 'B'),
    tx_sg_({ .base = tx_data_.data(), .len = INF_LEN }),
    rx_sg_({ .base = rx_data_.data(), .len = INF_LEN }),
    card_state_({ .seq = { .card = 1, .interface = 1, }, }),
    state_(TEQ1_INIT_STATE(&tx_sg_, 1, INF_LEN,
                           &rx_sg_, 1, INF_LEN,
                           &card_state_)) {
    memset(&tx_frame_, 0, sizeof(struct Teq1Frame));
    memset(&tx_next_, 0, sizeof(struct Teq1Frame));
    memset(&rx_frame_, 0, sizeof(struct Teq1Frame));
  }
  virtual ~Teq1RulesTest() { }
  virtual void SetUp() {}
  virtual void TearDown() { }

  struct Teq1Frame tx_frame_;
  struct Teq1Frame tx_next_;
  struct Teq1Frame rx_frame_;
  std::vector<uint8_t> tx_data_;
  std::vector<uint8_t> rx_data_;
  struct EseSgBuffer tx_sg_;
  struct EseSgBuffer rx_sg_;
  struct Teq1CardState card_state_;
  struct Teq1State state_;
};

class Teq1ErrorFreeTest : public Teq1RulesTest {
};

class Teq1ErrorHandlingTest : public Teq1RulesTest {
};

class Teq1CompleteTest : public Teq1ErrorFreeTest {
 public:
  virtual void SetUp() {
    tx_frame_.header.PCB = TEQ1_I(0, 0);
    teq1_fill_info_block(&state_, &tx_frame_);
    // Check that the tx_data was fully consumed.
    EXPECT_EQ(0UL, state_.app_data.tx_total);

    rx_frame_.header.PCB = TEQ1_I(0, 0);
    rx_frame_.header.LEN = INF_LEN;
    ASSERT_EQ(static_cast<unsigned long>(INF_LEN), tx_data_.size());  // Catch fixture changes.
    // Supply TX data and make sure it overwrites RX data on consumption.
    memcpy(rx_frame_.INF, tx_data_.data(), INF_LEN);
    rx_frame_.INF[INF_LEN] = teq1_compute_LRC(&rx_frame_);
  }

  virtual void RunRules() {
    teq1_trace_header();
    teq1_trace_transmit(tx_frame_.header.PCB, tx_frame_.header.LEN);
    teq1_trace_receive(rx_frame_.header.PCB, rx_frame_.header.LEN);

    enum RuleResult result = teq1_rules(&state_,  &tx_frame_, &rx_frame_, &tx_next_);
    EXPECT_EQ(0, state_.errors);
    EXPECT_EQ(NULL,  state_.last_error_message)
      << "Last error: " << state_.last_error_message;
    EXPECT_EQ(0, tx_next_.header.PCB)
      << "Actual next TX: " << teq1_pcb_to_name(tx_next_.header.PCB);
    EXPECT_EQ(kRuleResultComplete, result)
     << "Actual result name: " << teq1_rule_result_to_name(result);
  }
};

TEST_F(Teq1CompleteTest, I00_I00_empty) {
  // No data.
  state_.app_data.tx_total = 0;
  state_.app_data.rx_total = 0;
  // Re-zero the prepared frames.
  teq1_fill_info_block(&state_, &tx_frame_);
  rx_frame_.header.LEN = 0;
  rx_frame_.INF[0] = teq1_compute_LRC(&rx_frame_);
  RunRules();
  EXPECT_EQ(0U, rx_frame_.header.LEN);
};

TEST_F(Teq1CompleteTest, I00_I00_data) {
  RunRules();
  // Ensure that the rx_frame data was copied out to rx_data.
  EXPECT_EQ(0UL, state_.app_data.rx_total);
  EXPECT_EQ(tx_data_, rx_data_);
};

TEST_F(Teq1CompleteTest, I10_I10_data) {
  tx_frame_.header.PCB = TEQ1_I(1, 0);
  rx_frame_.header.PCB = TEQ1_I(0, 0);
  rx_frame_.INF[INF_LEN] = teq1_compute_LRC(&rx_frame_);
  RunRules();
  // Ensure that the rx_frame data was copied out to rx_data.
  EXPECT_EQ(INF_LEN, rx_frame_.header.LEN);
  EXPECT_EQ(0UL, state_.app_data.rx_total);
  EXPECT_EQ(tx_data_, rx_data_);
};

// Note, IFS is not tested as it is not supported on current hardware.

TEST_F(Teq1ErrorFreeTest, I00_WTX0_WTX1_data) {
  tx_frame_.header.PCB = TEQ1_I(0, 0);
  teq1_fill_info_block(&state_, &tx_frame_);
  // Check that the tx_data was fully consumed.
  EXPECT_EQ(0UL, state_.app_data.tx_total);

  rx_frame_.header.PCB = TEQ1_S_WTX(0);
  rx_frame_.header.LEN = 1;
  rx_frame_.INF[0] = 2; /* Wait x 2 */
  rx_frame_.INF[1] = teq1_compute_LRC(&rx_frame_);

  teq1_trace_header();
  teq1_trace_transmit(tx_frame_.header.PCB, tx_frame_.header.LEN);
  teq1_trace_receive(rx_frame_.header.PCB, rx_frame_.header.LEN);

  enum RuleResult result = teq1_rules(&state_,  &tx_frame_, &rx_frame_, &tx_next_);
  teq1_trace_transmit(tx_next_.header.PCB, tx_next_.header.LEN);

  EXPECT_EQ(0, state_.errors);
  EXPECT_EQ(NULL,  state_.last_error_message)
    << "Last error: " << state_.last_error_message;
  EXPECT_EQ(TEQ1_S_WTX(1), tx_next_.header.PCB)
    << "Actual next TX: " << teq1_pcb_to_name(tx_next_.header.PCB);
  EXPECT_EQ(state_.wait_mult, 2);
  EXPECT_EQ(state_.wait_mult, rx_frame_.INF[0]);
  // Ensure the next call will use the original TX frame.
  EXPECT_EQ(kRuleResultSingleShot, result)
   << "Actual result name: " << teq1_rule_result_to_name(result);
};

class Teq1ErrorFreeChainingTest : public Teq1ErrorFreeTest {
 public:
  virtual void RunRules() {
    tx_data_.resize(oversized_data_len_, 'C');
    const_cast<struct EseSgBuffer *>(state_.app_data.tx)->base = tx_data_.data();
    const_cast<struct EseSgBuffer *>(state_.app_data.tx)->len = oversized_data_len_;
    state_.app_data.tx_total = oversized_data_len_;
    teq1_fill_info_block(&state_, &tx_frame_);
    // Ensure More bit was set.
    EXPECT_EQ(1, bs_get(PCB.I.more_data, tx_frame_.header.PCB));
    // Check that the tx_data was fully consumed.
    EXPECT_EQ(static_cast<uint32_t>(oversized_data_len_ - INF_LEN),
              state_.app_data.tx_total);
    // No one is checking the TX LRC since there is no card present.

    rx_frame_.header.LEN = 0;
    rx_frame_.INF[0] = teq1_compute_LRC(&rx_frame_);

    teq1_trace_header();
    teq1_trace_transmit(tx_frame_.header.PCB, tx_frame_.header.LEN);
    teq1_trace_receive(rx_frame_.header.PCB, rx_frame_.header.LEN);

    enum RuleResult result = teq1_rules(&state_,  &tx_frame_, &rx_frame_, &tx_next_);
    teq1_trace_transmit(tx_next_.header.PCB, tx_next_.header.LEN);
    EXPECT_EQ(0, state_.errors);
    EXPECT_EQ(NULL,  state_.last_error_message)
      << "Last error: " << state_.last_error_message;
    EXPECT_EQ(kRuleResultContinue, result)
      << "Actual result name: " << teq1_rule_result_to_name(result);
    // Check that the tx_buf was drained already for the next frame.
    // ...
    EXPECT_EQ(static_cast<uint32_t>(oversized_data_len_ - (2 * INF_LEN)),
              state_.app_data.tx_total);
    // Belt and suspenders: make sure no RX buf was used.
    EXPECT_EQ(rx_data_.size(), state_.app_data.rx_total);
  }
  int oversized_data_len_;
};

TEST_F(Teq1ErrorFreeChainingTest, I01_R1_I11_chaining) {
  oversized_data_len_ = INF_LEN * 3;
  tx_frame_.header.PCB = TEQ1_I(0, 0);
  rx_frame_.header.PCB = TEQ1_R(1, 0, 0);
  RunRules();
  EXPECT_EQ(TEQ1_I(1, 1), tx_next_.header.PCB)
    << "Actual next TX: " << teq1_pcb_to_name(tx_next_.header.PCB);
};

TEST_F(Teq1ErrorFreeChainingTest, I11_R0_I01_chaining) {
  oversized_data_len_ = INF_LEN * 3;
  tx_frame_.header.PCB = TEQ1_I(1, 0);
  rx_frame_.header.PCB = TEQ1_R(0, 0, 0);
  RunRules();
  EXPECT_EQ(TEQ1_I(0, 1), tx_next_.header.PCB)
    << "Actual next TX: " << teq1_pcb_to_name(tx_next_.header.PCB);
};

TEST_F(Teq1ErrorFreeChainingTest, I11_R0_I00_chaining) {
  oversized_data_len_ = INF_LEN * 2;  // Exactly 2 frames worth.
  tx_frame_.header.PCB = TEQ1_I(1, 0);
  rx_frame_.header.PCB = TEQ1_R(0, 0, 0);
  RunRules();
  EXPECT_EQ(TEQ1_I(0, 0), tx_next_.header.PCB)
    << "Actual next TX: " << teq1_pcb_to_name(tx_next_.header.PCB);
};

//
// Error handling tests
//
//

class Teq1Retransmit : public Teq1ErrorHandlingTest {
 public:
  virtual void SetUp() {
    // No data.
    state_.app_data.rx_total = 0;
    state_.app_data.tx_total = 0;

    tx_frame_.header.PCB = TEQ1_I(0, 0);
    teq1_fill_info_block(&state_, &tx_frame_);
    // No one is checking the TX LRC since there is no card present.

    // Assume the card may not even set the error bit.
    rx_frame_.header.LEN = 0;
    rx_frame_.header.PCB = TEQ1_R(0, 0, 0);
    rx_frame_.INF[0] = teq1_compute_LRC(&rx_frame_);
  }
  virtual void TearDown() {
    teq1_trace_header();
    teq1_trace_transmit(tx_frame_.header.PCB, tx_frame_.header.LEN);
    teq1_trace_receive(rx_frame_.header.PCB, rx_frame_.header.LEN);

    enum RuleResult result = teq1_rules(&state_,  &tx_frame_, &rx_frame_, &tx_next_);
    // Not counted as an error as it was on the card-side.
    EXPECT_EQ(0, state_.errors);
    const char *kNull = NULL;
    EXPECT_EQ(kNull, state_.last_error_message) << state_.last_error_message;
    EXPECT_EQ(kRuleResultRetransmit, result)
     << "Actual result name: " << teq1_rule_result_to_name(result);
  }
};

TEST_F(Teq1Retransmit, I00_R000_I00) {
  rx_frame_.header.PCB = TEQ1_R(0, 0, 0);
  rx_frame_.INF[0] = teq1_compute_LRC(&rx_frame_);
};

TEST_F(Teq1Retransmit, I00_R001_I00) {
  rx_frame_.header.PCB = TEQ1_R(0, 0, 1);
  rx_frame_.INF[0] = teq1_compute_LRC(&rx_frame_);
};

TEST_F(Teq1Retransmit, I00_R010_I00) {
  rx_frame_.header.PCB = TEQ1_R(0, 1, 0);
  rx_frame_.INF[0] = teq1_compute_LRC(&rx_frame_);
};

TEST_F(Teq1Retransmit, I00_R011_I00) {
  rx_frame_.header.PCB = TEQ1_R(0, 1, 1);
  rx_frame_.INF[0] = teq1_compute_LRC(&rx_frame_);
}

TEST_F(Teq1ErrorHandlingTest, I00_I00_bad_lrc) {
  // No data.
  state_.app_data.rx_total = 0;
  state_.app_data.tx_total = 0;

  tx_frame_.header.PCB = TEQ1_I(0, 0);
  teq1_fill_info_block(&state_, &tx_frame_);
  // No one is checking the TX LRC since there is no card present.

  rx_frame_.header.PCB = TEQ1_I(0, 0);
  rx_frame_.header.LEN = 0;
  rx_frame_.INF[0] = teq1_compute_LRC(&rx_frame_) - 1;

  teq1_trace_header();
  teq1_trace_transmit(tx_frame_.header.PCB, tx_frame_.header.LEN);
  teq1_trace_receive(rx_frame_.header.PCB, rx_frame_.header.LEN);

  enum RuleResult result = teq1_rules(&state_,  &tx_frame_, &rx_frame_, &tx_next_);
  EXPECT_EQ(1, state_.errors);
  const char *kNull = NULL;
  EXPECT_NE(kNull, state_.last_error_message);
  EXPECT_STREQ("Invalid frame received", state_.last_error_message);
  EXPECT_EQ(TEQ1_R(0, 0, 1), tx_next_.header.PCB)
    << "Actual next TX: " << teq1_pcb_to_name(tx_next_.header.PCB);
  EXPECT_EQ(kRuleResultSingleShot, result)
   << "Actual result name: " << teq1_rule_result_to_name(result);
};

static const struct Teq1ProtocolOptions kTeq1Options = {
      .host_address = 0xA5,
      .node_address = 0x5A,
      .bwt = 1.624f,
      .etu = 0.00015f, /* elementary time unit, in seconds */
      .preprocess = NULL,
};

std::string to_hex(const std::vector<uint8_t>& data) {
    static constexpr char hex[] = "0123456789ABCDEF";
    std::string out;
    out.reserve(data.size() * 2);
    for (uint8_t c : data) {
        out.push_back(hex[c / 16]);
        out.push_back(hex[c % 16]);
    }
    return out;
}

class EseWireFake : public EseOperationsInterface {
 public:
  EseWireFake() : tx_cursor_(0), rx_cursor_(0) { }
  virtual ~EseWireFake() = default;

  virtual int EseOpen(struct EseInterface *UNUSED(ese), void *UNUSED(data)) {
    return 0;
  }
  virtual int EseReset(struct EseInterface *UNUSED(ese)) {
    ALOGI("EseReset called!"); // Add to invocations
    // Using the RX cursor, check for a reset expected.
    // This is on RX because the s(resync) global counter is on session resets.
    EXPECT_EQ(1, invocations.at(tx_cursor_).expect_reset);
    return 0;
  }
  virtual int EsePoll(struct EseInterface *UNUSED(ese), uint8_t UNUSED(poll_for),
                      float UNUSED(timeout), int UNUSED(complete)) {
    return 0;
  }
  virtual void EseClose(struct EseInterface *UNUSED(ese)) { };

  virtual uint32_t EseTransceive(struct EseInterface *ese, const struct EseSgBuffer *tx_sg, uint32_t tx_nsg,
                                 struct EseSgBuffer *rx_sg, uint32_t rx_nsg) {
    rx_cursor_ = 0;
    return teq1_transceive(ese, &kTeq1Options, tx_sg, tx_nsg, rx_sg, rx_nsg);
  }

  virtual uint32_t EseHwTransmit(struct EseInterface *UNUSED(ese), const uint8_t *data,
                                 uint32_t len, int UNUSED(complete)) {
    EXPECT_GT(invocations.size(), tx_cursor_);
    if (invocations.size() <= tx_cursor_) {
      return 0;
    }
    if (!len) {
      return 0;
    }
    if (!invocations.size()) {
      return 0;
    }
    // Just called once per teq1_transmit -- no partials.
    const struct Invocation &invocation = invocations.at(tx_cursor_++);

    EXPECT_EQ(invocation.expected_tx.size(), len);
    int eq = memcmp(data, invocation.expected_tx.data(), len);
    const std::vector<uint8_t> vec_data(data, data + len);
    EXPECT_EQ(0, eq)
        << "Got: '" << to_hex(vec_data) << "' "
        << "Expected: '" << to_hex(invocation.expected_tx) << "'";

    return len;
  }

  virtual uint32_t EseHwReceive(struct EseInterface *UNUSED(ese), uint8_t *data,
                                uint32_t len, int UNUSED(complete)) {
    if (!len) {
      return 0;
    }
    // Get this calls expected data.
    EXPECT_GT(invocations.size(), rx_cursor_);
    if (!invocations.size())
      return 0;
    struct Invocation &invocation = invocations.at(rx_cursor_);

    // Supply the golden return data and pop off the invocation.
    // Allows partial reads from the invocation stack.
    uint32_t rx_total = 0;
    if (len <= invocation.rx.size()) {
      rx_total = len;
      memcpy(data, invocation.rx.data(), invocation.rx.size());
    }
    uint32_t remaining = invocation.rx.size() - rx_total;
    if (remaining && rx_total) {
      invocation.rx.erase(invocation.rx.begin(),
                          invocation.rx.begin() + rx_total);
    } else {
      rx_cursor_++;
      // RX shouldn't get ahead of TX.
      EXPECT_GE(tx_cursor_, rx_cursor_);
      // We could delete, but this make test bugs a little easier to see.
    }
    return rx_total;
  }

  struct Invocation {
    std::vector<uint8_t> rx;
    std::vector<uint8_t> expected_tx;
    int expect_reset;
  };

  std::vector<Invocation> invocations;
 private:
  uint32_t tx_cursor_;
  uint32_t rx_cursor_;
};

class Teq1TransceiveTest : public virtual Test {
 public:
  Teq1TransceiveTest() { }
  virtual ~Teq1TransceiveTest() { }

  void SetUp() {
    // Configure ese with our internal ops.
    EseOperationsWrapper::InitializeEse(&ese_, &wire_);
    // Start with normal seq's.
    TEQ1_INIT_CARD_STATE((struct Teq1CardState *)(&(ese_.pad[0])));
  }

  void TearDown() {
    wire_.invocations.resize(0);
  }

 protected:
  EseWireFake wire_;
  EseInterface ese_;
};


TEST_F(Teq1TransceiveTest, NormalTransceiveUnchained) {
  EXPECT_EQ(0, ese_open(&ese_, NULL));

  // I(0,0) ->
  //        <- I(0, 0)
  wire_.invocations.resize(1);
  struct Teq1Frame frame;
  size_t frame_size = 0;
  frame.header.NAD = kTeq1Options.node_address;
  frame.header.PCB = TEQ1_I(0, 0);
  frame.header.LEN = 4;
  frame.INF[0] = 'A';
  frame.INF[1] = 'B';
  frame.INF[2] = 'C';
  frame.INF[3] = 'D';
  frame.INF[frame.header.LEN] = teq1_compute_LRC(&frame);
  frame_size = sizeof(frame.header) + frame.header.LEN + 1;
  wire_.invocations[0].expected_tx.resize(frame_size);
  memcpy(wire_.invocations[0].expected_tx.data(), &frame.val[0], frame_size);
  ALOGI("Planning to send:");
  teq1_trace_transmit(frame.header.PCB, frame.header.LEN);

  frame.header.LEN = 0;
  frame.header.NAD = kTeq1Options.host_address;
  frame.INF[frame.header.LEN] = teq1_compute_LRC(&frame);
  frame_size = sizeof(frame.header) + frame.header.LEN + 1;
  wire_.invocations[0].rx.resize(frame_size);
  memcpy(wire_.invocations[0].rx.data(), &frame, frame_size);
  ALOGI("Expecting to receive:");
  teq1_trace_receive(frame.header.PCB, frame.header.LEN);

  const uint8_t payload[] = { 'A', 'B', 'C', 'D' };
  uint8_t reply[5];  // Should stay empty.
  EXPECT_EQ(0, ese_transceive(&ese_, payload, sizeof(payload), reply, sizeof(reply)));
};


TEST_F(Teq1TransceiveTest, NormalUnchainedRetransmitRecovery) {
  EXPECT_EQ(0, ese_open(&ese_, NULL));

  // I(0,0) [4] ->
  //            <- R(0, 1, 0)
  // I(0,0) [4] ->
  //            <- I(0, 0)
  wire_.invocations.resize(2);
  struct Teq1Frame frame;
  size_t frame_size = 0;
  frame.header.NAD = kTeq1Options.node_address;
  frame.header.PCB = TEQ1_I(0, 0);
  frame.header.LEN = 4;
  frame.INF[0] = 'A';
  frame.INF[1] = 'B';
  frame.INF[2] = 'C';
  frame.INF[3] = 'D';
  frame.INF[frame.header.LEN] = teq1_compute_LRC(&frame);
  frame_size = sizeof(frame.header) + frame.header.LEN + 1;
  wire_.invocations[0].expected_tx.resize(frame_size);
  memcpy(wire_.invocations[0].expected_tx.data(), &frame.val[0], frame_size);
  wire_.invocations[1].expected_tx.resize(frame_size);
  memcpy(wire_.invocations[1].expected_tx.data(), &frame.val[0], frame_size);

  frame.header.LEN = 0;
  frame.header.NAD = kTeq1Options.host_address;
  frame.header.PCB = TEQ1_R(0, 1, 0);
  frame.INF[frame.header.LEN] = teq1_compute_LRC(&frame);
  frame_size = sizeof(frame.header) + frame.header.LEN + 1;
  wire_.invocations[0].rx.resize(frame_size);
  memcpy(wire_.invocations[0].rx.data(), &frame, frame_size);

  frame.header.LEN = 0;
  frame.header.NAD = kTeq1Options.host_address;
  frame.header.PCB = TEQ1_I(0, 0);
  frame.INF[frame.header.LEN] = teq1_compute_LRC(&frame);
  frame_size = sizeof(frame.header) + frame.header.LEN + 1;
  wire_.invocations[1].rx.resize(frame_size);
  memcpy(wire_.invocations[1].rx.data(), &frame, frame_size);

  const uint8_t payload[] = { 'A', 'B', 'C', 'D' };
  uint8_t reply[5];  // Should stay empty.
  EXPECT_EQ(0, ese_transceive(&ese_, payload, sizeof(payload), reply, sizeof(reply)));
};

TEST_F(Teq1TransceiveTest, RetransmitResyncRecovery) {
  EXPECT_EQ(0, ese_open(&ese_, NULL));

  // I(0,0) [4] ->
  //            <- R(0, 1, 0)
  // I(0,0) [4] ->
  //            <- R(0, 1, 0)
  // I(0,0) [4] ->
  //            <- R(0, 1, 0)
  // I(0,0) [4] ->
  //            <- R(0, 1, 0)
  // S(RESYNC, REQUEST) -> (retran this is another case)
  //            <- S(RESYNC, RESPONSE)
  // I(0, 0) [4] ->
  //            <- I(0, 0) [0]
  wire_.invocations.resize(6);
  struct Teq1Frame frame;
  size_t frame_size = 0;
  frame.header.NAD = kTeq1Options.node_address;
  frame.header.PCB = TEQ1_I(0, 0);
  frame.header.LEN = 4;
  frame.INF[0] = 'A';
  frame.INF[1] = 'B';
  frame.INF[2] = 'C';
  frame.INF[3] = 'D';
  frame.INF[frame.header.LEN] = teq1_compute_LRC(&frame);
  frame_size = sizeof(frame.header) + frame.header.LEN + 1;
  wire_.invocations[0].expected_tx.resize(frame_size);
  memcpy(wire_.invocations[0].expected_tx.data(), &frame.val[0], frame_size);
  wire_.invocations[1].expected_tx.resize(frame_size);
  memcpy(wire_.invocations[1].expected_tx.data(), &frame.val[0], frame_size);
  wire_.invocations[2].expected_tx.resize(frame_size);
  memcpy(wire_.invocations[2].expected_tx.data(), &frame.val[0], frame_size);
  wire_.invocations[3].expected_tx.resize(frame_size);
  memcpy(wire_.invocations[3].expected_tx.data(), &frame.val[0], frame_size);
  wire_.invocations[5].expected_tx.resize(frame_size);
  memcpy(wire_.invocations[5].expected_tx.data(), &frame.val[0], frame_size);

  frame.header.LEN = 0;
  frame.header.NAD = kTeq1Options.node_address;
  frame.header.PCB = TEQ1_S_RESYNC(0);
  frame.INF[frame.header.LEN] = teq1_compute_LRC(&frame);
  frame_size = sizeof(frame.header) + frame.header.LEN + 1;
  wire_.invocations[4].expected_tx.resize(frame_size);
  memcpy(wire_.invocations[4].expected_tx.data(), &frame, frame_size);

  frame.header.LEN = 0;
  frame.header.NAD = kTeq1Options.host_address;
  frame.header.PCB = TEQ1_R(0, 1, 0);
  frame.INF[frame.header.LEN] = teq1_compute_LRC(&frame);
  frame_size = sizeof(frame.header) + frame.header.LEN + 1;
  wire_.invocations[0].rx.resize(frame_size);
  memcpy(wire_.invocations[0].rx.data(), &frame, frame_size);
  wire_.invocations[1].rx.resize(frame_size);
  memcpy(wire_.invocations[1].rx.data(), &frame, frame_size);
  wire_.invocations[2].rx.resize(frame_size);
  memcpy(wire_.invocations[2].rx.data(), &frame, frame_size);
  wire_.invocations[3].rx.resize(frame_size);
  memcpy(wire_.invocations[3].rx.data(), &frame, frame_size);

  frame.header.LEN = 0;
  frame.header.NAD = kTeq1Options.host_address;
  frame.header.PCB = TEQ1_S_RESYNC(1);
  frame.INF[frame.header.LEN] = teq1_compute_LRC(&frame);
  frame_size = sizeof(frame.header) + frame.header.LEN + 1;
  wire_.invocations[4].rx.resize(frame_size);
  memcpy(wire_.invocations[4].rx.data(), &frame, frame_size);

  frame.header.LEN = 0;
  frame.header.NAD = kTeq1Options.host_address;
  frame.header.PCB = TEQ1_I(0, 0);
  frame.INF[frame.header.LEN] = teq1_compute_LRC(&frame);
  frame_size = sizeof(frame.header) + frame.header.LEN + 1;
  wire_.invocations[5].rx.resize(frame_size);
  memcpy(wire_.invocations[5].rx.data(), &frame, frame_size);

  const uint8_t payload[] = { 'A', 'B', 'C', 'D' };
  uint8_t reply[5];  // Should stay empty.
  EXPECT_EQ(0, ese_transceive(&ese_, payload, sizeof(payload), reply, sizeof(reply)));
};

// Error case described in b/63546784
TEST_F(Teq1TransceiveTest, RetransmitResyncLoop) {
  EXPECT_EQ(0, ese_open(&ese_, NULL));

  // I(0,0) [4] ->
  //            <- R(0, 1, 0)
  // I(0,0) [4] ->
  //            <- R(0, 1, 0)
  // I(0,0) [4] ->
  //            <- R(0, 1, 0)
  // I(0,0) [4] ->
  //            <- R(0, 1, 0)
  // S(RESYNC, REQUEST) ->
  //            <- S(RESYNC, RESPONSE)
  // I(0,0) [4] ->
  //            <- R(0, 1, 0)
  // I(0,0) [4] ->
  //            <- R(0, 1, 0)
  // I(0,0) [4] ->
  //            <- R(0, 1, 0)
  // I(0,0) [4] ->
  //            <- R(0, 1, 0)
  // S(RESYNC, REQUEST) ->
  //            <- S(RESYNC, RESPONSE)
  // ...
  // 6 failure loops before a reset then 6 more before a hard failure.
  wire_.invocations.resize(5 * 12);
  struct Teq1Frame frame;
  size_t frame_size = 0;

  frame.header.NAD = kTeq1Options.node_address;
  frame.header.PCB = TEQ1_I(0, 0);
  frame.header.LEN = 4;
  frame.INF[0] = 'A';
  frame.INF[1] = 'B';
  frame.INF[2] = 'C';
  frame.INF[3] = 'D';
  frame.INF[frame.header.LEN] = teq1_compute_LRC(&frame);
  frame_size = sizeof(frame.header) + frame.header.LEN + 1;
  // Initialize all invocations to I/R then overwrite with resyncs.
  for (auto &invocation : wire_.invocations) {
    invocation.expected_tx.resize(frame_size);
    memcpy(invocation.expected_tx.data(), &frame.val[0], frame_size);
  }

  frame.header.LEN = 0;
  frame.header.NAD = kTeq1Options.host_address;
  frame.header.PCB = TEQ1_R(0, 1, 0);
  frame.INF[frame.header.LEN] = teq1_compute_LRC(&frame);
  frame_size = sizeof(frame.header) + frame.header.LEN + 1;
  for (auto &invocation : wire_.invocations) {
    invocation.rx.resize(frame_size);
    memcpy(invocation.rx.data(), &frame.val[0], frame_size);
  }

  frame.header.LEN = 0;
  frame.header.NAD = kTeq1Options.node_address;
  frame.header.PCB = TEQ1_S_RESYNC(0);
  frame.INF[frame.header.LEN] = teq1_compute_LRC(&frame);
  frame_size = sizeof(frame.header) + frame.header.LEN + 1;
  int count = 0;
  for (auto &invocation : wire_.invocations) {
    if (++count % 5 == 0) {
      invocation.expected_tx.resize(frame_size);
      memcpy(invocation.expected_tx.data(), &frame, frame_size);
    }
  }

  frame.header.LEN = 0;
  frame.header.NAD = kTeq1Options.host_address;
  frame.header.PCB = TEQ1_S_RESYNC(1);
  frame.INF[frame.header.LEN] = teq1_compute_LRC(&frame);
  frame_size = sizeof(frame.header) + frame.header.LEN + 1;
  count = 0;
  for (auto &invocation : wire_.invocations) {
    if (++count % 5 == 0) {
      invocation.rx.resize(frame_size);
      memcpy(invocation.rx.data(), &frame, frame_size);
    }
  }

  wire_.invocations[30].expect_reset = 1;

  const uint8_t payload[] = { 'A', 'B', 'C', 'D' };
  uint8_t reply[5];  // Should stay empty.
  EXPECT_EQ(-1, ese_transceive(&ese_, payload, sizeof(payload), reply, sizeof(reply)));
  EXPECT_NE(0, ese_error(&ese_));
};


