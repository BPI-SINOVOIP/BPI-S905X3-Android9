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

#include "include/ese/app/weaver.h"

/* Non-static, but visibility=hidden so they can be used in test. */
const uint8_t kManageChannelOpen[] = {0x00, 0x70, 0x00, 0x00, 0x01};
const uint32_t kManageChannelOpenLength = (uint32_t)sizeof(kManageChannelOpen);
const uint8_t kManageChannelClose[] = {0x00, 0x70, 0x80, 0x00, 0x00};
const uint8_t kSelectApplet[] = {0x00, 0xA4, 0x04, 0x00, 0x0D, 0xA0,
                                 0x00, 0x00, 0x04, 0x76, 0x57, 0x56,
                                 0x52, 0x43, 0x4F, 0x4D, 0x4D, 0x30};
const uint32_t kSelectAppletLength = (uint32_t)sizeof(kSelectApplet);
// Supported commands.
const uint8_t kGetNumSlots[] = {0x80, 0x02, 0x00, 0x00, 0x04};
const uint8_t kWrite[] = {0x80, 0x04, 0x00, 0x00,
                          4 + kEseWeaverKeySize +
                              kEseWeaverValueSize}; // slotid + key + value
const uint8_t kRead[] = {0x80, 0x06, 0x00, 0x00,
                         4 + kEseWeaverKeySize}; // slotid + key
const uint8_t kEraseValue[] = {0x80, 0x08, 0x00, 0x00, 4}; // slotid
const uint8_t kEraseAll[] = {0x80, 0x0a, 0x00, 0x00};

// Build 32-bit int from big endian bytes
static uint32_t get_uint32(uint8_t buf[4]) {
  uint32_t x = buf[3];
  x |= buf[2] << 8;
  x |= buf[1] << 16;
  x |= buf[0] << 24;
  return x;
}

static void put_uint32(uint8_t buf[4], uint32_t val) {
  buf[0] = 0xff & (val >> 24);
  buf[1] = 0xff & (val >> 16);
  buf[2] = 0xff & (val >> 8);
  buf[3] = 0xff & val;
}

EseAppResult check_apdu_status(uint8_t code[2]) {
  if (code[0] == 0x90 && code[1] == 0x00) {
    return ESE_APP_RESULT_OK;
  }
  if (code[0] == 0x66 && code[1] == 0xA5) {
    return ESE_APP_RESULT_ERROR_COOLDOWN;
  }
  if (code[0] == 0x6A && code[1] == 0x83) {
    return ESE_APP_RESULT_ERROR_UNCONFIGURED;
  }
  /* TODO(wad) Bubble up the error code if needed. */
  ALOGE("unhandled response %.2x %.2x", code[0], code[1]);
  return ese_make_os_result(code[0], code[1]);
}

ESE_API void ese_weaver_session_init(struct EseWeaverSession *session) {
  session->ese = NULL;
  session->active = false;
  session->channel_id = 0;
}

ESE_API EseAppResult ese_weaver_session_open(struct EseInterface *ese,
                                             struct EseWeaverSession *session) {
  struct EseSgBuffer tx[2];
  struct EseSgBuffer rx;
  uint8_t rx_buf[32];
  int rx_len;
  if (!ese || !session) {
    ALOGE("Invalid |ese| or |session|");
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }
  if (session->active == true) {
    ALOGE("|session| is already active");
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }
  /* Instantiate a logical channel */
  rx_len = ese_transceive(ese, kManageChannelOpen, sizeof(kManageChannelOpen),
                          rx_buf, sizeof(rx_buf));
  if (ese_error(ese)) {
    ALOGE("transceive error: code:%d message:'%s'", ese_error_code(ese),
          ese_error_message(ese));
    return ESE_APP_RESULT_ERROR_COMM_FAILED;
  }
  if (rx_len < 0) {
    ALOGE("transceive error: rx_len: %d", rx_len);
    return ESE_APP_RESULT_ERROR_COMM_FAILED;
  }
  if (rx_len < 2) {
    ALOGE("transceive error: reply too short");
    return ESE_APP_RESULT_ERROR_COMM_FAILED;
  }
  EseAppResult ret;
  ret = check_apdu_status(&rx_buf[rx_len - 2]);
  if (ret != ESE_APP_RESULT_OK) {
    ALOGE("MANAGE CHANNEL OPEN failed with error code: %x %x",
          rx_buf[rx_len - 2], rx_buf[rx_len - 1]);
    return ret;
  }
  if (rx_len < 3) {
    ALOGE("transceive error: successful reply unexpectedly short");
    return ESE_APP_RESULT_ERROR_COMM_FAILED;
  }
  session->ese = ese;
  session->channel_id = rx_buf[rx_len - 3];

  /* Select Weaver Applet. */
  uint8_t chan = kSelectApplet[0] | session->channel_id;
  tx[0].base = &chan;
  tx[0].len = 1;
  tx[1].base = (uint8_t *)&kSelectApplet[1];
  tx[1].len = sizeof(kSelectApplet) - 1;
  rx.base = &rx_buf[0];
  rx.len = sizeof(rx_buf);
  rx_len = ese_transceive_sg(ese, tx, 2, &rx, 1);
  if (rx_len < 0 || ese_error(ese)) {
    ALOGE("transceive error: caller should check ese_error()");
    return ESE_APP_RESULT_ERROR_COMM_FAILED;
  }
  if (rx_len < 2) {
    ALOGE("transceive error: reply too short");
    return ESE_APP_RESULT_ERROR_COMM_FAILED;
  }
  ret = check_apdu_status(&rx_buf[rx_len - 2]);
  if (ret != ESE_APP_RESULT_OK) {
    ALOGE("SELECT failed with error code: %x %x", rx_buf[rx_len - 2],
          rx_buf[rx_len - 1]);
    return ret;
  }
  session->active = true;
  return ESE_APP_RESULT_OK;
}

ESE_API EseAppResult
ese_weaver_session_close(struct EseWeaverSession *session) {
  uint8_t rx_buf[32];
  int rx_len;
  if (!session || !session->ese) {
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }
  if (!session->active || session->channel_id == 0) {
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }
  /* Release the channel */
  uint8_t close_channel[sizeof(kManageChannelClose)];
  ese_memcpy(close_channel, kManageChannelClose, sizeof(kManageChannelClose));
  close_channel[0] |= session->channel_id;
  close_channel[3] |= session->channel_id;
  rx_len = ese_transceive(session->ese, close_channel, sizeof(close_channel),
                          rx_buf, sizeof(rx_buf));
  if (rx_len < 0 || ese_error(session->ese)) {
    return ESE_APP_RESULT_ERROR_COMM_FAILED;
  }
  if (rx_len < 2) {
    return ESE_APP_RESULT_ERROR_COMM_FAILED;
  }
  EseAppResult ret;
  ret = check_apdu_status(&rx_buf[rx_len - 2]);
  if (ret != ESE_APP_RESULT_OK) {
    return ret;
  }
  session->channel_id = 0;
  session->active = false;
  return ESE_APP_RESULT_OK;
}

ESE_API EseAppResult ese_weaver_get_num_slots(struct EseWeaverSession *session,
                                              uint32_t *numSlots) {
  if (!session || !session->ese || !session->active) {
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }
  if (!session->active || session->channel_id == 0) {
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }
  if (!numSlots) {
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }

  // Prepare command
  uint8_t get_num_slots[sizeof(kGetNumSlots)];
  ese_memcpy(get_num_slots, kGetNumSlots, sizeof(kGetNumSlots));
  get_num_slots[0] |= session->channel_id;

  // Send command
  uint8_t rx_buf[6];
  const int rx_len =
      ese_transceive(session->ese, get_num_slots, sizeof(get_num_slots), rx_buf,
                     sizeof(rx_buf));

  // Check for errors
  if (rx_len < 2 || ese_error(session->ese)) {
    ALOGE("Failed to get num slots");
    return ESE_APP_RESULT_ERROR_COMM_FAILED;
  }
  if (rx_len == 2) {
    ALOGE("ese_weaver_get_num_slots: SE exception");
    EseAppResult ret = check_apdu_status(rx_buf);
    return ret;
  }
  if (rx_len != 6) {
    ALOGE("Unexpected response from Weaver applet");
    return ESE_APP_RESULT_ERROR_COMM_FAILED;
  }

  *numSlots = get_uint32(rx_buf);
  return ESE_APP_RESULT_OK;
}

ESE_API EseAppResult ese_weaver_write(struct EseWeaverSession *session,
                                      uint32_t slotId, const uint8_t *key,
                                      const uint8_t *value) {
  if (!session || !session->ese || !session->active) {
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }
  if (!session->active || session->channel_id == 0) {
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }
  if (!key || !value) {
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }

  // Prepare data to send
  struct EseSgBuffer tx[5];
  uint8_t chan = kWrite[0] | session->channel_id;
  tx[0].base = &chan;
  tx[0].len = 1;
  tx[1].base = (uint8_t *)&kWrite[1];
  tx[1].len = sizeof(kWrite) - 1;

  // Slot ID in big endian
  uint8_t slot_id[4];
  put_uint32(slot_id, slotId);
  tx[2].base = slot_id;
  tx[2].len = sizeof(slot_id);

  // Key and value
  tx[3].c_base = key;
  tx[3].len = kEseWeaverKeySize;
  tx[4].c_base = value;
  tx[4].len = kEseWeaverValueSize;

  // Prepare buffer for result
  struct EseSgBuffer rx;
  uint8_t rx_buf[2];
  rx.base = rx_buf;
  rx.len = sizeof(rx_buf);

  // Send the command
  const int rx_len = ese_transceive_sg(session->ese, tx, 5, &rx, 1);

  // Check for errors
  if (rx_len < 2 || ese_error(session->ese)) {
    ALOGE("Failed to write to slot");
    return ESE_APP_RESULT_ERROR_COMM_FAILED;
  }
  if (rx_len > 2) {
    ALOGE("Unexpected response from Weaver applet");
    return ESE_APP_RESULT_ERROR_COMM_FAILED;
  }
  return check_apdu_status(rx_buf);
}

ESE_API EseAppResult ese_weaver_read(struct EseWeaverSession *session,
                                     uint32_t slotId, const uint8_t *key,
                                     uint8_t *value, uint32_t *timeout) {
  if (!session || !session->ese || !session->active) {
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }
  if (!session->active || session->channel_id == 0) {
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }
  if (!key || !value || !timeout) {
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }

  // Prepare data to send
  struct EseSgBuffer tx[5];
  uint8_t chan = kRead[0] | session->channel_id;
  tx[0].base = &chan;
  tx[0].len = 1;
  tx[1].base = (uint8_t *)&kRead[1];
  tx[1].len = sizeof(kRead) - 1;

  // Slot ID in big endian
  uint8_t slot_id[4];
  put_uint32(slot_id, slotId);
  tx[2].base = slot_id;
  tx[2].len = sizeof(slot_id);

  // Key of 16 bytes
  tx[3].c_base = key;
  tx[3].len = kEseWeaverKeySize;

  // Value response is 16 bytes
  const uint8_t maxResponse = 1 + kEseWeaverValueSize;
  tx[4].c_base = &maxResponse;
  tx[4].len = 1;

  // Prepare buffer for result
  struct EseSgBuffer rx[3];
  uint8_t appletStatus;
  rx[0].base = &appletStatus;
  rx[0].len = 1;
  rx[1].base = value;
  rx[1].len = kEseWeaverValueSize;
  uint8_t rx_buf[2];
  rx[2].base = rx_buf;
  rx[2].len = sizeof(rx_buf);

  // Send the command
  const int rx_len = ese_transceive_sg(session->ese, tx, 5, rx, 3);

  // Check for errors
  if (rx_len < 2 || ese_error(session->ese)) {
    ALOGE("Failed to write to slot");
    return ESE_APP_RESULT_ERROR_COMM_FAILED;
  }
  if (rx_len == 2) {
    rx_buf[0] = appletStatus;
    rx_buf[1] = value[0];
    ALOGE("ese_weaver_read: SE exception");
    EseAppResult ret = check_apdu_status(rx_buf);
    return ret;
  }
  if (rx_len < 7) {
    ALOGE("Unexpected response from Weaver applet");
    return ESE_APP_RESULT_ERROR_COMM_FAILED;
  }
  const uint8_t READ_SUCCESS = 0x00;
  const uint8_t READ_WRONG_KEY = 0x7f;
  const uint8_t READ_BACK_OFF = 0x76;
  const uint32_t millisInSecond = 1000;
  // wrong key
  if (appletStatus == READ_WRONG_KEY) {
    ALOGI("ese_weaver_read wrong key provided");
    *timeout = get_uint32(value) * millisInSecond;
    return ESE_WEAVER_READ_WRONG_KEY;
  }
  // backoff
  if (appletStatus == READ_BACK_OFF) {
    ALOGI("ese_weaver_read wrong key provided");
    *timeout = get_uint32(value) * millisInSecond;
    return ESE_WEAVER_READ_TIMEOUT;
  }
  if (rx_len != 19) {
    ALOGE("Unexpected response from Weaver applet");
    return ESE_APP_RESULT_ERROR_COMM_FAILED;
  }
  return ESE_APP_RESULT_OK;
}

ESE_API EseAppResult ese_weaver_erase_value(struct EseWeaverSession *session,
                                            uint32_t slotId) {
  if (!session || !session->ese || !session->active) {
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }
  if (!session->active || session->channel_id == 0) {
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }

  // Prepare data to send
  struct EseSgBuffer tx[3];
  uint8_t chan = kEraseValue[0] | session->channel_id;
  tx[0].base = &chan;
  tx[0].len = 1;
  tx[1].base = (uint8_t *)&kEraseValue[1];
  tx[1].len = sizeof(kEraseValue) - 1;

  // Slot ID in big endian
  uint8_t slot_id[4];
  put_uint32(slot_id, slotId);
  tx[2].base = slot_id;
  tx[2].len = sizeof(slot_id);

  // Prepare buffer for result
  struct EseSgBuffer rx;
  uint8_t rx_buf[2];
  rx.base = rx_buf;
  rx.len = sizeof(rx_buf);

  // Send the command
  const int rx_len = ese_transceive_sg(session->ese, tx, 3, &rx, 1);

  // Check for errors
  if (rx_len < 2 || ese_error(session->ese)) {
    ALOGE("Failed to write to slot");
    return ESE_APP_RESULT_ERROR_COMM_FAILED;
  }
  if (rx_len > 2) {
    ALOGE("Unexpected response from Weaver applet");
    return ESE_APP_RESULT_ERROR_COMM_FAILED;
  }
  return check_apdu_status(rx_buf);
}

// TODO: erase all, not currently used
