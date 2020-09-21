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
 * Implement a simple T=1 echo endpoint.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../libese-teq1/include/ese/teq1.h"
#include "../libese/include/ese/ese.h"
#include "../libese/include/ese/log.h"

struct EchoState {
  struct Teq1Frame frame;
  uint8_t *rx_fill;
  uint8_t *tx_sent;
  int recvd;
};

#define ECHO_STATE(ese) (*(struct EchoState **)(&ese->pad[1]))

static int echo_open(struct EseInterface *ese, void *hw_opts) {
  struct EchoState *es = hw_opts; /* shorter than __attribute */
  struct EchoState **es_ptr;
  if (sizeof(ese->pad) < sizeof(struct EchoState *)) {
    /* This is a compile-time correctable error only. */
    ALOGE("Pad size too small to use Echo HW (%zu < %zu)", sizeof(ese->pad),
          sizeof(struct EchoState *));
    return -1;
  }
  es_ptr = &ECHO_STATE(ese);
  *es_ptr = malloc(sizeof(struct EchoState));
  if (!*es_ptr) {
    return -1;
  }
  es = ECHO_STATE(ese);
  es->rx_fill = &es->frame.header.NAD;
  es->tx_sent = es->rx_fill;
  es->recvd = 0;
  return 0;
}

static void echo_close(struct EseInterface *ese) {
  struct EchoState *es;
  es = ECHO_STATE(ese);
  if (!es) {
    return;
  }
  free(es);
  es = NULL;
}

static uint32_t echo_receive(struct EseInterface *ese, uint8_t *buf,
                             uint32_t len, int complete) {
  struct EchoState *es = ECHO_STATE(ese);
  ALOGV("interface attempting to read data");
  if (!es->recvd) {
    return 0;
  }

  if (len > sizeof(es->frame) - (es->tx_sent - &es->frame.header.NAD)) {
    return 0;
  }

  /* NAD was polled for so skip it. */
  memcpy(buf, es->tx_sent, len);
  es->tx_sent += len;
  if (complete) {
    es->tx_sent = &es->frame.header.NAD;
    es->recvd = 0;
    ALOGV("card sent a frame");
  }
  return sizeof(es->frame.header) + es->frame.header.LEN;
}

static uint32_t echo_transmit(struct EseInterface *ese, const uint8_t *buf,
                              uint32_t len, int complete) {
  struct EchoState *es = ECHO_STATE(ese);
  ALOGV("interface transmitting data");
  if (len > sizeof(es->frame) - (es->rx_fill - &es->frame.header.NAD)) {
    return 0;
  }
  memcpy(es->rx_fill, buf, len);
  es->rx_fill += len;
  es->recvd = complete;
  if (complete) {
    es->frame.header.NAD = 0x00;
    if (teq1_compute_LRC(&es->frame) != es->frame.INF[es->frame.header.LEN]) {
      ALOGV("card received frame with bad LRC");
      return 0;
    }
    ALOGV("card received valid frame");
    es->rx_fill = &es->frame.header.NAD;
  }
  return len;
}

static int echo_poll(struct EseInterface *ese, uint8_t poll_for, float timeout,
                     int complete) {
  struct EchoState *es = ECHO_STATE(ese);
  const struct Teq1ProtocolOptions *opts = ese->ops->opts;
  ALOGV("interface polling for start of frame/host node address: %x", poll_for);
  /* In reality, we should be polling at intervals up to the timeout. */
  if (timeout > 0.0) {
    usleep(timeout * 1000);
  }
  if (poll_for == opts->host_address) {
    ALOGV("interface received NAD");
    if (!complete) {
      es->tx_sent++; /* Consume the polled byte: NAD */
    }
    return 1;
  }
  return -1;
}

int echo_preprocess(const struct Teq1ProtocolOptions *const opts,
                    struct Teq1Frame *frame, int tx) {
  if (tx) {
    /* Recompute the LRC with the NAD of 0x00 */
    frame->header.NAD = 0x00;
    frame->INF[frame->header.LEN] = teq1_compute_LRC(frame);
    frame->header.NAD = opts->node_address;
    ALOGV("interface is preprocessing outbound frame");
  } else {
    /* Replace the NAD with 0x00 so the LRC check passes. */
    frame->header.NAD = 0x00;
    ALOGV("interface is preprocessing inbound frame");
  }
  return 0;
}

static const struct Teq1ProtocolOptions kTeq1Options = {
    .host_address = 0xAA,
    .node_address = 0xBB,
    .bwt = 3.14152f,
    .etu = 1.0f,
    .preprocess = &echo_preprocess,
};

uint32_t echo_transceive(struct EseInterface *ese,
                         const struct EseSgBuffer *tx_buf, uint32_t tx_len,
                         struct EseSgBuffer *rx_buf, uint32_t rx_len) {
  return teq1_transceive(ese, &kTeq1Options, tx_buf, tx_len, rx_buf, rx_len);
}

static const char *kErrorMessages[] = {
    "T=1 hard failure.",        /* TEQ1_ERROR_HARD_FAIL */
    "T=1 abort.",               /* TEQ1_ERROR_ABORT */
    "T=1 device reset failed.", /* TEQ1_ERROR_DEVICE_ABORT */
};

static const struct EseOperations ops = {
    .name = "eSE Echo Hardware (fake)",
    .open = &echo_open,
    .hw_receive = &echo_receive,
    .hw_transmit = &echo_transmit,
    .transceive = &echo_transceive,
    .poll = &echo_poll,
    .close = &echo_close,
    .opts = &kTeq1Options,
    .errors = kErrorMessages,
    .errors_count = sizeof(kErrorMessages),
};
ESE_DEFINE_HW_OPS(ESE_HW_ECHO, ops);
