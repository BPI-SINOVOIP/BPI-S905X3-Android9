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

#include "include/ese/app/boot.h"
#include "boot_private.h"

const uint8_t kBootStateVersion = 0x1;
const uint16_t kBootStorageLength = 4096;
/* Non-static, but visibility=hidden so they can be used in test. */
const uint8_t kManageChannelOpen[] = {0x00, 0x70, 0x00, 0x00, 0x01};
const uint32_t kManageChannelOpenLength = (uint32_t)sizeof(kManageChannelOpen);
const uint8_t kManageChannelClose[] = {0x00, 0x70, 0x80, 0x00, 0x00};

const uint8_t kSelectApplet[] = {0x00, 0xA4, 0x04, 0x00, 0x0d, 0xA0,
                                 0x00, 0x00, 0x04, 0x76, 0x50, 0x49,
                                 0x58, 0x4C, 0x42, 0x4F, 0x4F, 0x54};
const uint32_t kSelectAppletLength = (uint32_t)sizeof(kSelectApplet);
// Supported commands.
const uint8_t kGetState[] = {0x80, 0x00, 0x00, 0x00, 0x00};
const uint8_t kLoadCmd[] = {0x80, 0x02};
const uint8_t kStoreCmd[] = {0x80, 0x04};
const uint8_t kGetLockState[] = {0x80, 0x06, 0x00, 0x00, 0x00};
const uint8_t kSetLockState[] = {0x80, 0x08, 0x00, 0x00, 0x00};
const uint8_t kSetProduction[] = {0x80, 0x0a};
const uint8_t kCarrierLockTest[] = {0x80, 0x0c, 0x00, 0x00};
const uint8_t kFactoryReset[] = {0x80, 0x0e, 0x00, 0x00};
const uint8_t kLockReset[] = {0x80, 0x0e, 0x01, 0x00};
const uint8_t kLoadMetaClear[] = {0x80, 0x10, 0x00, 0x00};
const uint8_t kLoadMetaAppend[] = {0x80, 0x10, 0x01, 0x00};
static const uint16_t kMaxMetadataLoadSize = 1024;

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

ESE_API void ese_boot_session_init(struct EseBootSession *session) {
  session->ese = NULL;
  session->active = false;
  session->channel_id = 0;
}

ESE_API EseAppResult ese_boot_session_open(struct EseInterface *ese,
                                           struct EseBootSession *session) {
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

  /* Select Boot Applet. */
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

ESE_API EseAppResult ese_boot_session_close(struct EseBootSession *session) {
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

ESE_API EseAppResult ese_boot_lock_xget(struct EseBootSession *session,
                                        EseBootLockId lock, uint8_t *lockData,
                                        uint16_t maxSize, uint16_t *length) {
  struct EseSgBuffer tx[4];
  struct EseSgBuffer rx[3];
  int rx_len;
  if (!session || !session->ese || !session->active) {
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }
  if (lock > kEseBootLockIdMax) {
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }
  if (maxSize < 1 || maxSize > 4096) {
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }
  uint8_t chan = kGetLockState[0] | session->channel_id;
  tx[0].base = &chan;
  tx[0].len = 1;
  tx[1].base = (uint8_t *)&kGetLockState[1];
  tx[1].len = 1;

  uint8_t p1p2[] = {lock, 0x01};
  tx[2].base = &p1p2[0];
  tx[2].len = sizeof(p1p2);

  // Accomodate the applet 2 byte status code.
  uint8_t max_reply[] = {0x0, ((maxSize + 2) >> 8), ((maxSize + 2) & 0xff)};
  tx[3].base = &max_reply[0];
  tx[3].len = sizeof(max_reply);

  uint8_t reply[2]; // App reply or APDU error.
  rx[0].base = &reply[0];
  rx[0].len = sizeof(reply);
  // Applet data
  rx[1].base = lockData;
  rx[1].len = maxSize;
  // Only used if the full maxSize is used.
  uint8_t apdu_status[2];
  rx[2].base = &apdu_status[0];
  rx[2].len = sizeof(apdu_status);

  rx_len = ese_transceive_sg(session->ese, tx, 4, rx, 3);
  if (rx_len < 2 || ese_error(session->ese)) {
    ALOGE("ese_boot_lock_xget: failed to read lock state (%d)", lock);
    return ESE_APP_RESULT_ERROR_COMM_FAILED;
  }
  if (rx_len == 2) {
    ALOGE("ese_boot_lock_xget: SE exception");
    EseAppResult ret = check_apdu_status(&reply[0]);
    return ret;
  }
  // Expect the full payload plus the aplet status and the completion code.
  *length = (uint16_t)(rx_len - 4);
  if (rx_len == 4) {
    ALOGE("ese_boot_lock_xget: received applet error code %x %x", lockData[0],
          lockData[1]);
    return ese_make_app_result(lockData[0], lockData[1]);
  }
  return ESE_APP_RESULT_OK;
}

ESE_API EseAppResult ese_boot_lock_get(struct EseBootSession *session,
                                       EseBootLockId lock, uint8_t *lockVal) {
  struct EseSgBuffer tx[3];
  struct EseSgBuffer rx[1];
  int rx_len;
  if (!session || !session->ese || !session->active) {
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }
  if (lock > kEseBootLockIdMax) {
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }
  uint8_t chan = kGetLockState[0] | session->channel_id;
  tx[0].base = &chan;
  tx[0].len = 1;
  tx[1].base = (uint8_t *)&kGetLockState[1];
  tx[1].len = 1;

  uint8_t p1p2[] = {lock, 0x0};
  tx[2].base = &p1p2[0];
  tx[2].len = sizeof(p1p2);

  uint8_t reply[6];
  rx[0].base = &reply[0];
  rx[0].len = sizeof(reply);

  rx_len = ese_transceive_sg(session->ese, tx, 3, rx, 1);
  if (rx_len < 2 || ese_error(session->ese)) {
    ALOGE("ese_boot_lock_get: failed to read lock state (%d).", lock);
    return ESE_APP_RESULT_ERROR_COMM_FAILED;
  }
  EseAppResult ret = check_apdu_status(&reply[rx_len - 2]);
  if (ret != ESE_APP_RESULT_OK) {
    ALOGE("ese_boot_lock_get: SE OS error.");
    return ret;
  }
  if (rx_len < 5) {
    ALOGE("ese_boot_lock_get: communication error");
    return ESE_APP_RESULT_ERROR_COMM_FAILED;
  }
  // TODO: unify in the applet, then map them here.
  if (reply[0] != 0x0 && reply[1] != 0x0) {
    ALOGE("ese_boot_lock_get: Applet error: %x %x", reply[0], reply[1]);
    return ese_make_app_result(reply[0], reply[1]);
  }
  if (lockVal) {
    *lockVal = reply[2];
    return ESE_APP_RESULT_OK;
  }

  if (reply[2] != 0) {
    return ESE_APP_RESULT_TRUE;
  }
  return ESE_APP_RESULT_FALSE;
}

EseAppResult ese_boot_meta_clear(struct EseBootSession *session) {
  struct EseSgBuffer tx[2];
  struct EseSgBuffer rx[1];
  int rx_len;
  if (!session || !session->ese || !session->active) {
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }

  uint8_t chan = kLoadMetaClear[0] | session->channel_id;
  tx[0].base = &chan;
  tx[0].len = 1;
  tx[1].base = (uint8_t *)&kLoadMetaClear[1];
  tx[1].len = sizeof(kLoadMetaClear) - 1;

  uint8_t reply[4]; // App reply or APDU error.
  rx[0].base = &reply[0];
  rx[0].len = sizeof(reply);

  rx_len = ese_transceive_sg(session->ese, tx, 2, rx, 1);
  if (rx_len < 2 || ese_error(session->ese)) {
    ALOGE("ese_boot_meta_clear: communication failure");
    return ESE_APP_RESULT_ERROR_COMM_FAILED;
  }
  // Expect the full payload plus the applet status and the completion code.
  if (rx_len < 4) {
    ALOGE("ese_boot_meta_clear: SE exception");
    EseAppResult ret = check_apdu_status(&reply[rx_len - 2]);
    return ret;
  }
  if (reply[0] != 0x0 || reply[1] != 0x0) {
    ALOGE("ese_boot_meta_clear: received applet error code %.2x %.2x", reply[0],
          reply[1]);
    return ese_make_app_result(reply[0], reply[1]);
  }
  return ESE_APP_RESULT_OK;
}

EseAppResult ese_boot_meta_append(struct EseBootSession *session,
                                  const uint8_t *data, uint16_t dataLen) {
  struct EseSgBuffer tx[4];
  struct EseSgBuffer rx[1];
  int rx_len;
  if (!session || !session->ese || !session->active) {
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }
  if (dataLen > kMaxMetadataLoadSize) {
    ALOGE("ese_boot_meta_append: too much data provided");
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }

  uint8_t chan = kLoadMetaAppend[0] | session->channel_id;
  tx[0].base = &chan;
  tx[0].len = 1;
  tx[1].base = (uint8_t *)&kLoadMetaAppend[1];
  tx[1].len = sizeof(kLoadMetaAppend) - 1;

  uint8_t apdu_len[] = {0x0, (dataLen >> 8), (dataLen & 0xff)};
  tx[2].base = &apdu_len[0];
  tx[2].len = sizeof(apdu_len);
  tx[3].c_base = data;
  tx[3].len = dataLen;

  uint8_t reply[4]; // App reply or APDU error.
  rx[0].base = &reply[0];
  rx[0].len = sizeof(reply);

  rx_len = ese_transceive_sg(session->ese, tx, 4, rx, 1);
  if (rx_len < 2 || ese_error(session->ese)) {
    ALOGE("ese_boot_meta_append: communication failure");
    return ESE_APP_RESULT_ERROR_COMM_FAILED;
  }
  // Expect the full payload plus the applet status and the completion code.
  if (rx_len < 4) {
    ALOGE("ese_boot_meta_append: SE exception");
    EseAppResult ret = check_apdu_status(&reply[rx_len - 2]);
    return ret;
  }
  if (reply[0] != 0x0 || reply[1] != 0x0) {
    ALOGE("ese_boot_meta_append: received applet error code %.2x %.2x",
          reply[0], reply[1]);
    return ese_make_app_result(reply[0], reply[1]);
  }
  return ESE_APP_RESULT_OK;
}

ESE_API EseAppResult ese_boot_lock_xset(struct EseBootSession *session,
                                        EseBootLockId lockId,
                                        const uint8_t *lockData,
                                        uint16_t dataLen) {
  struct EseSgBuffer tx[3];
  struct EseSgBuffer rx[1];
  int rx_len;
  if (!session || !session->ese || !session->active) {
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }
  if (lockId > kEseBootLockIdMax) {
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }
  if (dataLen < 1 || dataLen > kEseBootOwnerKeyMax + 1) {
    ALOGE("ese_boot_lock_xset: too much data: %hu > %d", dataLen,
          kEseBootOwnerKeyMax + 1);
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }

  // Locks with metadata require a multi-step upload to meet the
  // constraints of the transport.
  EseAppResult res = ese_boot_meta_clear(session);
  if (res != ESE_APP_RESULT_OK) {
    ALOGE("ese_boot_lock_xset: unable to clear scratch metadata");
    return res;
  }
  // The first byte is the lock value itself, so we skip it.
  const uint8_t *cursor = &lockData[1];
  uint16_t remaining = dataLen - 1;
  while (remaining > 0) {
    uint16_t chunk = (512 < remaining) ? 512 : remaining;
    res = ese_boot_meta_append(session, cursor, chunk);
    ALOGI("ese_boot_lock_xset: sending chunk %x", remaining);
    if (res != ESE_APP_RESULT_OK) {
      ALOGE("ese_boot_lock_xset: unable to upload metadata");
      return res;
    }
    remaining -= chunk;
    cursor += chunk;
  }

  uint8_t chan = kSetLockState[0] | session->channel_id;
  tx[0].base = &chan;
  tx[0].len = 1;
  tx[1].base = (uint8_t *)&kSetLockState[1];
  tx[1].len = 1;

  uint8_t lockIdLockValueUseMeta[] = {lockId, lockData[0], 0x1, 0x1};
  tx[2].base = &lockIdLockValueUseMeta[0];
  tx[2].len = sizeof(lockIdLockValueUseMeta);

  uint8_t reply[4]; // App reply or APDU error.
  rx[0].base = &reply[0];
  rx[0].len = sizeof(reply);

  rx_len = ese_transceive_sg(session->ese, tx, 3, rx, 1);
  if (rx_len < 2 || ese_error(session->ese)) {
    ALOGE("ese_boot_lock_xset: failed to set lock state (%d).", lockId);
    return ESE_APP_RESULT_ERROR_COMM_FAILED;
  }
  if (rx_len == 2) {
    ALOGE("ese_boot_lock_xset: SE exception");
    EseAppResult ret = check_apdu_status(&reply[0]);
    return ret;
  }
  // Expect the full payload plus the applet status and the completion code.
  if (rx_len != 4) {
    ALOGE("ese_boot_lock_xset: communication error");
    return ESE_APP_RESULT_ERROR_COMM_FAILED;
  }
  if (reply[0] != 0x0 || reply[1] != 0x0) {
    ALOGE("ese_boot_lock_xset: received applet error code %x %x", reply[0],
          reply[1]);
    return ese_make_app_result(reply[0], reply[1]);
  }
  return ESE_APP_RESULT_OK;
}

ESE_API EseAppResult ese_boot_lock_set(struct EseBootSession *session,
                                       EseBootLockId lockId,
                                       uint8_t lockValue) {
  struct EseSgBuffer tx[3];
  struct EseSgBuffer rx[1];
  int rx_len;
  if (!session || !session->ese || !session->active) {
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }
  if (lockId > kEseBootLockIdMax) {
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }

  uint8_t chan = kSetLockState[0] | session->channel_id;
  tx[0].base = &chan;
  tx[0].len = 1;
  tx[1].base = (uint8_t *)&kSetLockState[1];
  tx[1].len = 1;

  uint8_t lockIdLockValueNoMeta[] = {lockId, lockValue, 0x1, 0x0};
  tx[2].base = &lockIdLockValueNoMeta[0];
  tx[2].len = sizeof(lockIdLockValueNoMeta);

  uint8_t reply[4]; // App reply or APDU error.
  rx[0].base = &reply[0];
  rx[0].len = sizeof(reply);

  rx_len = ese_transceive_sg(session->ese, tx, 3, rx, 1);
  if (rx_len < 2 || ese_error(session->ese)) {
    ALOGE("Failed to set lock state (%d).", lockId);
    return ESE_APP_RESULT_ERROR_COMM_FAILED;
  }
  // Expect the full payload plus the applet status and the completion code.
  if (rx_len < 4) {
    ALOGE("ese_boot_lock_set: SE exception");
    EseAppResult ret = check_apdu_status(&reply[rx_len - 2]);
    return ret;
  }
  if (reply[0] != 0x0 || reply[1] != 0x0) {
    ALOGE("Received applet error code %x %x", reply[0], reply[1]);
    return ese_make_app_result(reply[0], reply[1]);
  }
  return ESE_APP_RESULT_OK;
}

ESE_API EseAppResult ese_boot_rollback_index_write(
    struct EseBootSession *session, uint8_t slot, uint64_t value) {
  struct EseSgBuffer tx[5];
  struct EseSgBuffer rx[1];
  uint8_t chan;
  if (!session || !session->ese || !session->active) {
    ALOGE("ese_boot_rollback_index_write: invalid session");
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }
  if (slot >= kEseBootRollbackSlotCount) {
    ALOGE("ese_boot_rollback_index_write: slot invalid");
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }

  // APDU CLA
  chan = kStoreCmd[0] | session->channel_id;
  tx[0].base = &chan;
  tx[0].len = 1;
  // APDU INS
  tx[1].base = (uint8_t *)&kStoreCmd[1];
  tx[1].len = 1;
  // APDU P1 - P2
  const uint8_t p1p2[] = {slot, 0x0};
  tx[2].c_base = &p1p2[0];
  tx[2].len = sizeof(p1p2);
  // APDU Lc
  uint8_t len = (uint8_t)sizeof(value);
  tx[3].base = &len;
  tx[3].len = sizeof(len);
  // APDU data
  tx[4].base = (uint8_t *)&value;
  tx[4].len = sizeof(value);

  uint8_t rx_buf[4];
  rx[0].base = &rx_buf[0];
  rx[0].len = sizeof(rx_buf);

  int rx_len = ese_transceive_sg(session->ese, tx, 5, rx, 1);
  if (rx_len < 0 || ese_error(session->ese)) {
    ALOGE("ese_boot_rollback_index_write: comm error");
    return ESE_APP_RESULT_ERROR_COMM_FAILED;
  }
  if (rx_len < 2) {
    ALOGE("ese_boot_rollback_index_write: too few bytes recieved.");
    return ESE_APP_RESULT_ERROR_COMM_FAILED;
  }
  if (rx_len < 4) {
    ALOGE("ese_boot_rollback_index_write: APDU Error");
    return check_apdu_status(&rx_buf[rx_len - 2]);
  }

  if (rx_buf[0] != 0 || rx_buf[1] != 0) {
    ALOGE("ese_boot_rollback_index_write: applet error code %x %x", rx_buf[0],
          rx_buf[1]);
    return ese_make_app_result(rx_buf[0], rx_buf[1]);
  }
  return ESE_APP_RESULT_OK;
}

ESE_API EseAppResult ese_boot_rollback_index_read(
    struct EseBootSession *session, uint8_t slot, uint64_t *value) {
  struct EseSgBuffer tx[4];
  struct EseSgBuffer rx[1];
  uint8_t chan;
  if (!session || !session->ese || !session->active) {
    ALOGE("ese_boot_rollback_index_write: invalid session");
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }
  if (!value) {
    ALOGE("ese_boot_rollback_index_write: NULL value supplied");
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }
  if (slot >= kEseBootRollbackSlotCount) {
    ALOGE("ese_boot_rollback_index_write: slot invalid");
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }

  // APDU CLA
  chan = kLoadCmd[0] | session->channel_id;
  tx[0].base = &chan;
  tx[0].len = 1;
  // APDU INS
  tx[1].base = (uint8_t *)&kLoadCmd[1];
  tx[1].len = 1;
  // APDU P1 - P2
  const uint8_t p1p2[] = {slot, 0x0};
  tx[2].c_base = &p1p2[0];
  tx[2].len = sizeof(p1p2);
  // APDU Lc
  uint8_t len = 0;
  tx[3].base = &len;
  tx[3].len = sizeof(len);

  uint8_t rx_buf[4 + sizeof(*value)];
  rx[0].base = &rx_buf[0];
  rx[0].len = sizeof(rx_buf);

  int rx_len = ese_transceive_sg(session->ese, tx, 4, rx, 1);
  if (rx_len < 0 || ese_error(session->ese)) {
    ALOGE("ese_boot_rollback_index_read: comm error");
    return ESE_APP_RESULT_ERROR_COMM_FAILED;
  }
  if (rx_len < 2) {
    ALOGE("ese_boot_rollback_index_read: too few bytes recieved.");
    return ESE_APP_RESULT_ERROR_COMM_FAILED;
  }
  // TODO(wad) We should check the APDU status anyway.
  if (rx_len < 4) {
    ALOGE("ese_boot_rollback_index_read: APDU Error");
    return check_apdu_status(&rx_buf[rx_len - 2]);
  }
  if (rx_buf[0] != 0 || rx_buf[1] != 0) {
    ALOGE("ese_boot_rollback_index_read: applet error code %x %x", rx_buf[0],
          rx_buf[1]);
    return ese_make_app_result(rx_buf[0], rx_buf[1]);
  }
  if (rx_len != (int)sizeof(rx_buf)) {
    ALOGE("ese_boot_rollback_index_read: unexpected partial reply (%d)",
          rx_len);
    return ESE_APP_RESULT_ERROR_COMM_FAILED;
  }
  *value = *((uint64_t *)&rx_buf[2]);
  return ESE_APP_RESULT_OK;
}

ESE_API EseAppResult ese_boot_carrier_lock_test(struct EseBootSession *session,
                                                const uint8_t *testdata,
                                                uint16_t len) {
  struct EseSgBuffer tx[5];
  struct EseSgBuffer rx[1];
  int rx_len;
  if (!session || !session->ese || !session->active) {
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }
  if (len > 2048) {
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }

  uint8_t chan = kCarrierLockTest[0] | session->channel_id;
  tx[0].base = &chan;
  tx[0].len = 1;
  tx[1].base = (uint8_t *)&kCarrierLockTest[1];
  tx[1].len = 1;

  uint8_t p1p2[] = {0, 0};
  tx[2].base = &p1p2[0];
  tx[2].len = sizeof(p1p2);

  uint8_t apdu_len[] = {0x0, (len >> 8), (len & 0xff)};
  tx[3].base = &apdu_len[0];
  tx[3].len = sizeof(apdu_len);

  tx[4].c_base = testdata;
  tx[4].len = len;

  uint8_t reply[4]; // App reply or APDU error.
  rx[0].base = &reply[0];
  rx[0].len = sizeof(reply);

  rx_len = ese_transceive_sg(session->ese, tx, 5, rx, 1);
  if (rx_len < 2 || ese_error(session->ese)) {
    ALOGE("ese_boot_carrier_lock_test: failed to test carrier vector");
    return ESE_APP_RESULT_ERROR_COMM_FAILED;
  }
  if (rx_len < 4) {
    ALOGE("ese_boot_carrier_lock_test: SE exception");
    EseAppResult ret = check_apdu_status(&reply[rx_len - 2]);
    return ret;
  }
  if (reply[0] != 0x0 || reply[1] != 0x0) {
    ALOGE("ese_boot_carrier_lock_test: applet error %x %x", reply[0], reply[1]);
    return ese_make_app_result(reply[0], reply[1]);
  }
  return ESE_APP_RESULT_OK;
}

ESE_API EseAppResult ese_boot_set_production(struct EseBootSession *session,
                                             bool production_mode) {
  struct EseSgBuffer tx[3];
  struct EseSgBuffer rx[1];
  int rx_len;
  uint8_t prodVal = production_mode ? 0x1 : 0x00;
  if (!session || !session->ese || !session->active) {
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }

  uint8_t chan = kSetProduction[0] | session->channel_id;
  tx[0].base = &chan;
  tx[0].len = 1;
  tx[1].base = (uint8_t *)&kSetProduction[1];
  tx[1].len = 1;

  uint8_t p1p2[] = {prodVal, 0x0};
  tx[2].base = &p1p2[0];
  tx[2].len = sizeof(p1p2);

  uint8_t reply[4]; // App reply or APDU error.
  rx[0].base = &reply[0];
  rx[0].len = sizeof(reply);

  rx_len = ese_transceive_sg(session->ese, tx, 3, rx, 1);
  if (rx_len < 2 || ese_error(session->ese)) {
    ALOGE("ese_boot_set_production: comms failure.");
    return ESE_APP_RESULT_ERROR_COMM_FAILED;
  }
  if (rx_len == 2) {
    ALOGE("ese_boot_set_production: SE exception");
    EseAppResult ret = check_apdu_status(&reply[0]);
    return ret;
  }
  // Expect the full payload plus the aplet status and the completion code.
  if (rx_len != 4) {
    ALOGE("ese_boot_set_production: not enough data (%d)", rx_len);
    return ese_make_app_result(reply[0], reply[1]);
  }
  if (reply[0] != 0x0 || reply[1] != 0x0) {
    ALOGE("ese_boot_set_production: applet error code %x %x", reply[0],
          reply[1]);
    return ese_make_app_result(reply[0], reply[1]);
  }
  return ESE_APP_RESULT_OK;
}

ESE_API EseAppResult ese_boot_reset_locks(struct EseBootSession *session) {
  struct EseSgBuffer tx[2];
  struct EseSgBuffer rx[1];
  int rx_len;
  if (!session || !session->ese || !session->active) {
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }

  uint8_t chan = kLockReset[0] | session->channel_id;
  tx[0].base = &chan;
  tx[0].len = 1;
  tx[1].base = (uint8_t *)&kLockReset[1];
  tx[1].len = sizeof(kLockReset) - 1;

  uint8_t reply[4]; // App reply or APDU error.
  rx[0].base = &reply[0];
  rx[0].len = sizeof(reply);

  rx_len = ese_transceive_sg(session->ese, tx, 2, rx, 1);
  if (rx_len < 2 || ese_error(session->ese)) {
    ALOGE("ese_boot_reset_locks: comms failure.");
    return ESE_APP_RESULT_ERROR_COMM_FAILED;
  }
  if (rx_len == 2) {
    ALOGE("ese_boot_reset_locks: SE exception");
    EseAppResult ret = check_apdu_status(&reply[0]);
    return ret;
  }
  // Expect the full payload plus the aplet status and the completion code.
  if (rx_len != 4) {
    ALOGE("ese_boot_reset_locks: not enough data (%d)", rx_len);
    return ese_make_app_result(reply[0], reply[1]);
  }
  if (reply[0] != 0x0 || reply[1] != 0x0) {
    ALOGE("ese_boot_reset_locks: applet error code %x %x", reply[0], reply[1]);
    return ese_make_app_result(reply[0], reply[1]);
  }
  return ESE_APP_RESULT_OK;
}

ESE_API EseAppResult ese_boot_get_state(struct EseBootSession *session,
                                        uint8_t *state, uint16_t maxSize) {
  struct EseSgBuffer tx[4];
  struct EseSgBuffer rx[3];
  int rx_len;
  if (!session || !session->ese || !session->active) {
    return ESE_APP_RESULT_ERROR_ARGUMENTS;
  }
  uint8_t chan = kGetState[0] | session->channel_id;
  tx[0].base = &chan;
  tx[0].len = 1;
  tx[1].base = (uint8_t *)&kGetState[1];
  tx[1].len = 1;

  uint8_t p1p2[] = {0x0, 0x0};
  tx[2].base = &p1p2[0];
  tx[2].len = sizeof(p1p2);

  // Accomodate the applet 2 byte status code.
  uint8_t max_reply[] = {0x0, ((maxSize + 2) >> 8), ((maxSize + 2) & 0xff)};
  tx[3].base = &max_reply[0];
  tx[3].len = sizeof(max_reply);

  uint8_t reply[2]; // App reply or APDU error.
  rx[0].base = &reply[0];
  rx[0].len = sizeof(reply);
  // Applet data
  rx[1].base = state;
  rx[1].len = maxSize;
  // Just in case the maxSize is used. That is unlikely.
  // TODO(wad) clean this up.
  uint8_t apdu_status[2];
  rx[2].base = &apdu_status[0];
  rx[2].len = sizeof(apdu_status);

  rx_len = ese_transceive_sg(session->ese, tx, 4, rx, 3);
  if (rx_len < 2 || ese_error(session->ese)) {
    ALOGE("ese_boot_get_state: comm failure");
    return ESE_APP_RESULT_ERROR_COMM_FAILED;
  }
  if (rx_len == 2) {
    ALOGE("ese_boot_get_state: SE exception");
    EseAppResult ret = check_apdu_status(&reply[0]);
    return ret;
  }
  // Expect the full payload plus the aplet status and the completion code.
  if (rx_len < 3 + 4) {
    ALOGE("ese_boot_get_state: did not receive enough data: %d", rx_len);
    if (rx_len == 4) {
      ALOGE("Received applet error code %x %x", reply[0], reply[1]);
    }
    return ese_make_app_result(reply[0], reply[1]);
  }
  // Well known version (for now).
  if (state[0] == kBootStateVersion) {
    uint16_t expected = (state[1] << 8) | (state[2]);
    // Reduce for version (1), status (2).
    if ((rx_len - 3) != expected) {
      ALOGE("ese_boot_get_state: may be truncated: %d != %d", rx_len - 5,
            expected);
    }
    return ESE_APP_RESULT_OK;
  }
  ALOGE("ese_boot_get_state: missing version tag");
  return ESE_APP_RESULT_ERROR_OS;
}
