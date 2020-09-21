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
#ifndef ESE_TEQ1_PRIVATE_H__
#define ESE_TEQ1_PRIVATE_H__ 1

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Enable T=1 format to reduce to case integers.
 * Ensure there are tests to map TEQ1_X() to the shorthand below.
 */
#define I(S, M) I_##S##_##M
#define I_0_0 0
#define I_0_1 32
#define I_1_0 64
#define I_1_1 96
#define R(S, O, P) R_ ## S ##_## O ##_## P
#define R_0_0_0 128
#define R_0_0_1 129
#define R_0_1_0 130
#define R_0_1_1 131
#define R_1_0_0 144
#define R_1_0_1 145
#define R_1_1_0 146
#define R_1_1_1 147

#define _S(x) x
#define S(N, R) S_##N##_##R
#define S_RESYNC_REQUEST 192
#define S_IFS_REQUEST 193
#define S_ABORT_REQUEST 194
#define S_WTX_REQUEST 195
#define S_RESYNC_RESPONSE 224
#define S_IFS_RESPONSE 225
#define S_ABORT_RESPONSE 226
#define S_WTX_RESPONSE 227

#define TEQ1_RULE(TX, RX) (((TX & 255) << 8)|(RX & 255))

struct Teq1State {
  uint8_t wait_mult;
  uint8_t ifs;
  uint8_t errors;
  int retransmits;
  const char *last_error_message;
  struct Teq1CardState *card_state;
  struct {
    const struct EseSgBuffer *tx;
    struct EseSgBuffer *rx;
    uint32_t tx_offset;
    uint32_t tx_count;
    uint32_t tx_total;
    uint32_t rx_offset;
    uint32_t rx_count;
    uint32_t rx_total;
  } app_data;
};

#define TEQ1_INIT_STATE(TX_BUFS, TX_LEN, TX_TOTAL_LEN, RX_BUFS, RX_LEN, RX_TOTAL_LEN, CSTATE) \
  { \
    .wait_mult = 1, \
    .ifs = IFSC, \
    .errors = 0, \
    .retransmits = 0, \
    .last_error_message = NULL, \
    .card_state = (CSTATE), \
    .app_data = { \
      .tx = (TX_BUFS), \
      .rx = (RX_BUFS), \
      .tx_offset = 0, \
      .tx_count = (TX_LEN), \
      .tx_total = (TX_TOTAL_LEN), \
      .rx_offset = 0, \
      .rx_count = (RX_LEN), \
      .rx_total = (RX_TOTAL_LEN), \
    }, \
  }

enum RuleResult {
  kRuleResultComplete,
  kRuleResultAbort,
  kRuleResultContinue,
  kRuleResultHardFail,
  kRuleResultResetDevice,
  kRuleResultResetSession,
  kRuleResultRetransmit,
  kRuleResultSingleShot,
};


const char *teq1_rule_result_to_name(enum RuleResult result);
const char *teq1_pcb_to_name(uint8_t pcb);
int teq1_transmit(struct EseInterface *ese,
                  const struct Teq1ProtocolOptions *opts,
                  struct Teq1Frame *frame);
int teq1_receive(struct EseInterface *ese,
                 const struct Teq1ProtocolOptions *opts,
                 float timeout,
                 struct Teq1Frame *frame);
uint8_t teq1_fill_info_block(struct Teq1State *state, struct Teq1Frame *frame);
void teq1_get_app_data(struct Teq1State *state, const struct Teq1Frame *frame);
uint8_t teq1_frame_error_check(struct Teq1State *state,
                               struct Teq1Frame *tx_frame,
                               struct Teq1Frame *rx_frame);
enum RuleResult teq1_rules(struct Teq1State *state,
                           struct Teq1Frame *tx_frame,
                           struct Teq1Frame *rx_frame,
                           struct Teq1Frame *next_tx);

#define teq1_dump_transmit(_B, _L) teq1_dump_buf("TX", (_B), (_L))
#define teq1_dump_receive(_B, _L) teq1_dump_buf("RX", (_B), (_L))

#ifdef __cplusplus
}  /* extern "C" */
#endif
#endif  /* ESE_TEQ1_PRIVATE_H__ */
