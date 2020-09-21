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
 * Support SPI communication with NXP PN553/PN80T secure element.
 */

#include "include/ese/hw/nxp/pn80t/common.h"

int nxp_pn80t_preprocess(const struct Teq1ProtocolOptions *const opts,
                         struct Teq1Frame *frame, int tx) {
  if (tx) {
    /* Recompute the LRC with the NAD of 0x00 */
    frame->header.NAD = 0x00;
    frame->INF[frame->header.LEN] = teq1_compute_LRC(frame);
    frame->header.NAD = opts->node_address;
    ALOGV("interface is preprocessing outbound frame");
  } else {
    /* Replace the NAD with 0x00 so the LRC check passes. */
    ALOGV("interface is preprocessing inbound frame (%x->%x)",
          frame->header.NAD, 0x00);
    if (frame->header.NAD != opts->host_address) {
      ALOGV("Rewriting from unknown NAD: %x", frame->header.NAD);
    }
    frame->header.NAD = 0x00;
    ALOGV("Frame length: %x", frame->header.LEN);
  }
  return 0;
}

static const struct Teq1ProtocolOptions kTeq1Options = {
    .host_address = 0xA5,
    .node_address = 0x5A,
    .bwt = 1.624f, /* cwt by default would be ~8k * 1.05s */
    /* 1.05ms is the vendor defined ETU.  However, we use this
     * for polling and 7 * etu (7ms) is a long time to wait
     * between poll attempts so we divided by 7. */
    .etu = 0.00015f, /* elementary time unit, in seconds */
    .preprocess = &nxp_pn80t_preprocess,
};

int nxp_pn80t_open(struct EseInterface *ese, void *board) {
  struct NxpState *ns;
  const struct Pn80tPlatform *platform;
  _static_assert(sizeof(ese->pad) >= sizeof(struct NxpState *),
                 "Pad size too small to use NXP HW");
  platform = ese->ops->opts;

  /* Ensure all required functions exist */
  if (!platform->initialize || !platform->release || !platform->toggle_reset ||
      !platform->wait) {
    ALOGE("Required functions not implemented in supplied platform");
    ese_set_error(ese, kNxpPn80tErrorPlatformInit);
    return -1;
  }

  ns = NXP_PN80T_STATE(ese);
  TEQ1_INIT_CARD_STATE((struct Teq1CardState *)(&ese->pad[0]));
  ns->handle = platform->initialize(board);
  if (!ns->handle) {
    ALOGE("platform initialization failed");
    ese_set_error(ese, kNxpPn80tErrorPlatformInit);
    return -1;
  }
  /* Toggle all required power GPIOs.
   * Each platform may prefer to handle the power
   * muxing specific. E.g., if NFC is in use, it would
   * be unwise to unset VEN.  However, the implementation
   * here will attempt it if supported.
   */
  if (platform->toggle_ven) {
    platform->toggle_ven(ns->handle, 1);
  }
  if (platform->toggle_power_req) {
    platform->toggle_power_req(ns->handle, 1);
  }
  /* Power on eSE */
  platform->toggle_reset(ns->handle, 1);
  return 0;
}

/* Used for soft-reset when possible. */
uint32_t nxp_pn80t_send_cooldown(struct EseInterface *ese, bool end);

int nxp_pn80t_reset(struct EseInterface *ese) {
  const struct Pn80tPlatform *platform = ese->ops->opts;
  struct NxpState *ns = NXP_PN80T_STATE(ese);

  /* If there is no error, perform a soft reset.
   * If there is no cooldown time associated, go ahead and do a real
   * reset as there is no other interface to trigger a hard reset.
   *
   * This avoids pulling the power when a cooldown is in progress
   * if it is at all possible to avoid.
   */
  if (!ese_error(ese)) {
    const uint32_t cooldownSec = nxp_pn80t_send_cooldown(ese, false);
    if (!ese_error(ese) && cooldownSec > 0) {
      return 0;
    }
  }

  if (platform->toggle_reset(ns->handle, 0) < 0) {
    ese_set_error(ese, kNxpPn80tErrorResetToggle);
    return -1;
  }
  if (platform->toggle_reset(ns->handle, 1) < 0) {
    ese_set_error(ese, kNxpPn80tErrorResetToggle);
    return -1;
  }

  /* Start fresh with the reset. */
  ese->error.is_err = false;
  return 0;
}

int nxp_pn80t_poll(struct EseInterface *ese, uint8_t poll_for, float timeout,
                   int complete) {
  struct NxpState *ns = NXP_PN80T_STATE(ese);
  const struct Pn80tPlatform *platform = ese->ops->opts;
  /* Attempt to read a 8-bit character once per 8-bit character transmission
   * window (in seconds).
   */
  int intervals = (int)(0.5f + timeout / (7.0f * kTeq1Options.etu));
  uint8_t byte = 0xff;
  ALOGV("interface polling for start of frame/host node address: %x", poll_for);
  /* If we had interrupts, we could just get notified by the driver. */
  do {
    /*
     * In practice, if complete=true, then no transmission
     * should attempt again until after 1000usec.
     */
    if (ese->ops->hw_receive(ese, &byte, 1, complete) != 1) {
      ALOGE("failed to read one byte");
      ese_set_error(ese, kNxpPn80tErrorPollRead);
      return -1;
    }
    if (byte == poll_for) {
      ALOGV("Polled for byte seen: %x with %d intervals remaining.", poll_for,
            intervals);
      ALOGV("RX[0]: %.2X", byte);
      return 1;
    } else {
      ALOGV("No match (saw %x)", byte);
    }
    platform->wait(ns->handle,
                   7.0f * kTeq1Options.etu * 1000000.0f); /* s -> us */
    ALOGV("poll interval %d: no match.", intervals);
  } while (intervals-- > 0);
  ALOGW("polling timed out.");
  return -1;
}

/* Returns the seconds the chip has requested to stay powered for internal
 * maintenance. This is not expected during normal operation, but it is still
 * a possible operating response.
 *
 * There are three timers reserved for internal state usage which are
 * not reliable API. As such, this function returns the maximum time
 * in seconds that the chip would like to stay powered-on.
 */
#define SECURE_TIMER 0xF1
#define ATTACK_COUNTER 0xF2
#define RESTRICTED_MODE_PENALTY 0xF3
uint32_t nxp_pn80t_send_cooldown(struct EseInterface *ese, bool end) {
  struct NxpState *ns = NXP_PN80T_STATE(ese);
  const struct Pn80tPlatform *platform = ese->ops->opts;
  const static uint8_t kEndofApduSession[] = {0x5a, 0xc5, 0x00, 0xc5};
  const static uint8_t kResetSession[] = {0x5a, 0xc4, 0x00, 0xc4};
  const uint8_t *const message = end ? kEndofApduSession : kResetSession;
  const uint32_t message_len =
      end ? sizeof(kEndofApduSession) : sizeof(kResetSession);

  if (ese_error(ese)) {
    return 0;
  }

  ese->ops->hw_transmit(ese, message, message_len, 1);
  if (ese_error(ese)) {
    ALOGE("failed to transmit cooldown check");
    return 0;
  }

  nxp_pn80t_poll(ese, kTeq1Options.host_address, 5.0f, 0);
  if (ese_error(ese)) {
    ALOGE("failed to poll during cooldown");
    return 0;
  }
  uint8_t rx_buf[32];
  const uint32_t bytes_read =
      ese->ops->hw_receive(ese, rx_buf, sizeof(rx_buf), 1);
  if (ese_error(ese)) {
    ALOGE("failed to receive cooldown response");
    return 0;
  }

  ALOGI("Requested power-down delay times (sec):");
  /* For each tag type, walk the response to extract the value. */
  uint32_t max_wait = 0;
  if (bytes_read >= 0x8 && rx_buf[0] == 0xe5 && rx_buf[1] == 0x12) {
    uint8_t *tag_ptr = &rx_buf[2];
    while (tag_ptr < (rx_buf + bytes_read)) {
      const uint8_t tag = *tag_ptr;
      const uint8_t length = *(tag_ptr + 1);

      /* The cooldown timers are 32-bit values. */
      if (length == sizeof(uint32_t)) {
        const uint32_t *const value_ptr = (uint32_t *)(tag_ptr + 2);
        uint32_t cooldown = ese_be32toh(*value_ptr);
        switch (tag) {
        case RESTRICTED_MODE_PENALTY:
          /* This timer is in minutes, so convert it to seconds. */
          cooldown *= 60;
        /* Fallthrough */
        case SECURE_TIMER:
        case ATTACK_COUNTER:
          ALOGI("- Timer 0x%.2X: %d", tag, cooldown);
          if (cooldown > max_wait) {
            max_wait = cooldown;
            /* Wait 25ms Guard time to make sure eSE is in DPD mode */
            platform->wait(ns->handle, 25000);
          }
          break;
        default:
          /* Ignore -- not a known tag. */
          break;
        }
      }
      tag_ptr += 2 + length;
    }
  }
  return max_wait;
}

uint32_t nxp_pn80t_handle_interface_call(struct EseInterface *ese,
                                         const struct EseSgBuffer *tx_buf,
                                         uint32_t tx_len,
                                         struct EseSgBuffer *rx_buf,
                                         uint32_t rx_len) {
  /* Catch proprietary, host-targeted calls FF XX 00 00 */
  const struct Pn80tPlatform *platform = ese->ops->opts;
  static const uint32_t kCommandLength = 4;
  static const uint8_t kResetCommand = 0x01;
  static const uint8_t kGpioToggleCommand = 0xe0;
  static const uint8_t kCooldownCommand = 0xe1;
  uint8_t buf[kCommandLength + 1];
  uint8_t ok[2] = {0x90, 0x00};
  struct NxpState *ns = NXP_PN80T_STATE(ese);
  /* Over-copy by one to make sure the command length matches. */
  if (ese_sg_to_buf(tx_buf, tx_len, 0, sizeof(buf), buf) != kCommandLength) {
    return 0;
  }
  /* Let 3 change as an argument. */
  if (buf[0] != 0xff || buf[2] != 0x00) {
    return 0;
  }
  switch (buf[1]) {
  case kResetCommand:
    ALOGI("interface command received: reset");
    /* Force a hard reset by setting an error on the hw. */
    ese_set_error(ese, 0);
    if (nxp_pn80t_reset(ese) < 0) {
      /* Warning, state unchanged error. */
      ok[0] = 0x62;
    }
    return ese_sg_from_buf(rx_buf, rx_len, 0, sizeof(ok), ok);
  case kGpioToggleCommand:
    ALOGI("interface command received: gpio toggle");
    if (platform->toggle_bootloader) {
      int ret = platform->toggle_bootloader(ns->handle, buf[3]);
      if (ret) {
        /* Grab the bottom two bytes. */
        ok[0] = (ret >> 8) & 0xff;
        ok[1] = ret & 0xff;
      }
    } else {
      /* Not found. */
      ok[0] = 0x6a;
      ok[1] = 0x82;
    }
    return ese_sg_from_buf(rx_buf, rx_len, 0, sizeof(ok), ok);
  case kCooldownCommand:
    ALOGI("interface command received: cooldown");
    uint8_t reply[6] = {0, 0, 0, 0, 0x90, 0x00};
    const uint32_t cooldownSec = nxp_pn80t_send_cooldown(ese, false);
    *(uint32_t *)(&reply[0]) = ese_htole32(cooldownSec);
    if (ese_error(ese)) {
      /* Return SW_UKNOWN on an ese failure. */
      reply[sizeof(reply) - 2] = 0x6f;
    }
    return ese_sg_from_buf(rx_buf, rx_len, 0, sizeof(reply), reply);
  }
  return 0;
}

uint32_t nxp_pn80t_transceive(struct EseInterface *ese,
                              const struct EseSgBuffer *tx_buf, uint32_t tx_len,
                              struct EseSgBuffer *rx_buf, uint32_t rx_len) {

  const uint32_t recvd =
      nxp_pn80t_handle_interface_call(ese, tx_buf, tx_len, rx_buf, rx_len);
  if (recvd > 0) {
    return recvd;
  }
  return teq1_transceive(ese, &kTeq1Options, tx_buf, tx_len, rx_buf, rx_len);
}

void nxp_pn80t_close(struct EseInterface *ese) {
  ALOGV("%s: called", __func__);
  struct NxpState *ns = NXP_PN80T_STATE(ese);
  const struct Pn80tPlatform *platform = ese->ops->opts;
  const uint32_t wait_sec = nxp_pn80t_send_cooldown(ese, true);

  /* After the cooldown, the device should go to sleep.
   * If not post-use time is required, power down to ensure
   * that the device is powered down when the OS is not on.
   */
  if (ese_error(ese) || wait_sec == 0) {
    platform->toggle_reset(ns->handle, 0);
    if (platform->toggle_power_req) {
      platform->toggle_power_req(ns->handle, 0);
    }
    if (platform->toggle_ven) {
      platform->toggle_ven(ns->handle, 0);
    }
  }

  platform->release(ns->handle);
  ns->handle = NULL;
}

const char *kNxpPn80tErrorMessages[] = {
    /* The first three are required by teq1_transceive use. */
    TEQ1_ERROR_MESSAGES,
    /* The rest are pn80t impl specific. */
    [kNxpPn80tErrorPlatformInit] = "unable to initialize platform",
    [kNxpPn80tErrorPollRead] = "failed to read one byte",
    [kNxpPn80tErrorReceive] = "failed to read",
    [kNxpPn80tErrorReceiveSize] = "attempted to receive too much data",
    [kNxpPn80tErrorTransmitSize] = "attempted to transfer too much data",
    [kNxpPn80tErrorTransmit] = "failed to transmit",
    [kNxpPn80tErrorResetToggle] = "failed to toggle reset",
};
