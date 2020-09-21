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
 */

#ifndef ESE_TEQ1_H_
#define ESE_TEQ1_H_ 1

#ifdef __cplusplus
extern "C" {
#endif

#include "../../../libese/include/ese/ese.h"
#include "../../../libese/include/ese/bit_spec.h"

/* Reserved codes for T=1 devices in EseOperation­>errors. */
enum Teq1Error {
 kTeq1ErrorHardFail = 0,
 kTeq1ErrorAbort,
 kTeq1ErrorDeviceReset,
 kTeq1ErrorMax,
};

/* For use in constant initializers libese-hw errors. */
#define TEQ1_ERROR_MESSAGES \
  [kTeq1ErrorHardFail] = "T=1 suffered hard failure", \
  [kTeq1ErrorAbort] = "T=1 aborting due to errors", \
  [kTeq1ErrorDeviceReset] = "T=1 unable to recover even after device reset"

enum pcb_type {
  kPcbTypeInfo0 = 0x0,
  kPcbTypeInfo1 = 0x1,
  kPcbTypeReceiveReady = 0x2,
  kPcbTypeSupervisory = 0x3,
};

enum super_type {
  kSuperTypeResync = 0x0,
  kSuperTypeIFS = 0x1,
  kSuperTypeAbort = 0x2,
  kSuperTypeWTX = 0x3,
};

struct PcbSpec {
  struct bit_spec type;
  struct bit_spec data;
  struct {
    struct bit_spec more_data;
    struct bit_spec send_seq;
  } I;
  struct {
    struct bit_spec parity_err;
    struct bit_spec other_err;
    struct bit_spec next_seq;
  } R;
  struct {
    struct bit_spec type;
    struct bit_spec response;
  } S;
};

const static struct PcbSpec PCB = {
  .type = { .value = 3, .shift = 6, },
  .data = { .value = 63, .shift = 0, },
  .I = {
    .more_data = { .value = 1, .shift = 5, },
    .send_seq = { .value = 1, .shift = 6, },
  },
  .R = {
    /* char parity or redundancy code err */
    .parity_err = { .value = 1, .shift = 0, },
    /* any other errors */
    .other_err = { .value = 1, .shift = 1, },
    /* If the same seq as last frame, then err even if other bits are 0. */
    .next_seq = { .value = 1, .shift = 4, },
  },
  .S = {
    .type = { .value = 3, .shift = 0, },
    .response = { .value = 1, .shift = 5, },
  },
};

struct Teq1Header {
  uint8_t NAD;
  uint8_t PCB;
  uint8_t LEN;
};
#define TEQ1HEADER_SIZE 3
#define TEQ1FRAME_SIZE INF_LEN + 1 + TEQ1HEADER_SIZE

#define INF_LEN 254
#define IFSC 254
struct Teq1Frame {
  union {
    uint8_t val[sizeof(struct Teq1Header) + INF_LEN + 1];
    struct {
      struct Teq1Header header;
      union {
        uint8_t INF[INF_LEN + 1]; /* Up to 254 with trailing LRC byte. */
      };
      /* If CRC was supported, it would be uint16_t. */
    };
  };
};


/*
 * Required to be the header for all EseInterface pad[]s for
 * cards implementing T=1.
 */
struct Teq1CardState {
  union {
    struct {
      int card:1;
      int interface:1;
    };
    uint8_t seq_bits;
  } seq;
};

/* Set "last sent" to 1 so we start at 0. */
#define TEQ1_INIT_CARD_STATE(CARD) \
  (CARD)->seq.card = 1; \
  (CARD)->seq.interface = 1;

/*
 * Used by devices implementing T=1 to set specific options
 * or callback behavior.
 */
struct Teq1ProtocolOptions;
typedef int (teq1_protocol_preprocess_op_t)(const struct Teq1ProtocolOptions *const, struct Teq1Frame *, int);

struct Teq1ProtocolOptions {
  uint8_t host_address;  /* NAD to listen for */
  uint8_t node_address;  /* NAD to send to */
  float bwt;
  float etu;
  /*
   * If not NULL, is called immediately before transmit (1)
   * and immediately after receive.
   */
  teq1_protocol_preprocess_op_t *preprocess;
};

/* PCB bits */
#define kTeq1PcbType (3 << 6)

/* I-block bits */
#define kTeq1InfoType        (0 << 6)
#define kTeq1InfoMoreBit     (1 << 5)
#define kTeq1InfoSeqBit      (1 << 6)

/* R-block bits */
#define kTeq1RrType         (1 << 7)
#define kTeq1RrSeqBit       (1 << 4)
#define kTeq1RrParityError  (1)
#define kTeq1RrOtherError   (1 << 1)

/* S-block bits */
#define kTeq1SuperType      (3 << 6)
#define kTeq1SuperRequestBit (0)
#define kTeq1SuperResponseBit (1 << 5)
#define kTeq1SuperResyncBit (0)
#define kTeq1SuperIfsBit (1)
#define kTeq1SuperAbortBit (1 << 1)
#define kTeq1SuperWtxBit (3)

/* I(Seq, More-bit) */
#define TEQ1_I(S, M) ((S) << 6) | ((M) << 5)

/* R(Seq, Other Error, Parity Error) */
#define TEQ1_R(S, O, P) (kTeq1RrType | ((S) << 4) | (P) | ((O) << 1))
/* S_<TYPE>(response) */
#define TEQ1_S_RESYNC(R) (kTeq1SuperType | ((R) << 5) | kTeq1SuperResyncBit)
#define TEQ1_S_WTX(R)  (kTeq1SuperType | ((R) << 5) | kTeq1SuperWtxBit)
#define TEQ1_S_ABORT(R) (kTeq1SuperType | ((R) << 5) | kTeq1SuperAbortBit)
#define TEQ1_S_IFS(R) (kTeq1SuperType | ((R) << 5) | kTeq1SuperIfsBit)

uint32_t teq1_transceive(struct EseInterface *ese,
                         const struct Teq1ProtocolOptions *opts,
                         const struct EseSgBuffer *tx_bufs, uint8_t tx_segs,
                         struct EseSgBuffer *rx_bufs, uint8_t rx_segs);

uint8_t teq1_compute_LRC(const struct Teq1Frame *frame);

#define teq1_trace_header() ALOGI("%-20s --- %20s", "Interface", "Card")
#define teq1_trace_transmit(PCB, LEN) ALOGI("%-20s --> %20s [%3hhu]", teq1_pcb_to_name(PCB), "", LEN)
#define teq1_trace_receive(PCB, LEN) ALOGI("%-20s <-- %20s [%3hhu]", "", teq1_pcb_to_name(PCB), LEN)

#ifdef __cplusplus
}  /* extern "C" */
#endif
#endif  /* ESE_TEQ1_H_ */
