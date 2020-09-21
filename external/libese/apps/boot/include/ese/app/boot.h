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
 * See the README.md in the parent (../../..) directory.
 */

#ifndef ESE_APP_BOOT_H_
#define ESE_APP_BOOT_H_ 1

#include "../../../../../libese/include/ese/ese.h"
#include "../../../../../libese/include/ese/log.h"
#include "../../../../../libese-sysdeps/include/ese/sysdeps.h"

#include "../../../../include/ese/app/result.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * EseBootSession carries the necessary start for interfacing
 * with the methods below.
 *
 * Its usage follows a lifecycle like:
 *
 *   EseAppResult res;
 *   EseBootSession session;
 *   ese_boot_session_init(&session);
 *   res = ese_boot_session_open(ese, &session);
 *   if (res != ESE_APP_RESULT_OK) {
 *     ... handle error (especially cooldown) ...
 *   }
 *   ... ese_boot_* ...
 *   ese_boot_session_close(&session);
 *
 */
struct EseBootSession {
  struct EseInterface *ese;
  bool active;
  uint8_t channel_id;
};

/**
 * The Storage applet supports up to 8 64-bit storage slots for storing
 * rollback protection indices.
 */
const uint8_t kEseBootRollbackSlotCount = 8;
/**
 * When using the LOCK_OWNER, a key, or other relevant value, must be supplied.
 * It may be at most OWNER_LOCK_METADATA_SIZE as defined in
 * card/src/com/android/verifiedboot/storage/Storage.java.
 */
const uint16_t kEseBootOwnerKeyMax = 2048;

/* Keep in sync with card/src/com/android/verifiedboot/storage/Storage.java */
/**
 * This enum reflects the types of Locks that are supported by
 * the ese_boot_lock_* calls.
 */
typedef enum {
  kEseBootLockIdCarrier = 0,
  kEseBootLockIdDevice,
  kEseBootLockIdBoot,
  kEseBootLockIdOwner,
  kEseBootLockIdMax = kEseBootLockIdOwner,
} EseBootLockId;


/**
 * Initializes a pre-allocated |session| for use.
 */
void ese_boot_session_init(struct EseBootSession *session);

/**
 * Configures a communication session with the Storage applet using a logical
 * channel on an already open |ese| object.
 *
 * @returns ESE_APP_RESULT_OK on success.
 */
EseAppResult ese_boot_session_open(struct EseInterface *ese, struct EseBootSession *session);

/**
 * Shuts down the logical channel with the Storage applet and invalidates
 * the |session| internal state.
 *
 * @returns ESE_APP_RESULT_OK on success.
 */
EseAppResult ese_boot_session_close(struct EseBootSession *session);

/**
 * Retrieves the uint8_t value stored for the lock specified by |lockId|.
 * On success, the value is stored in |lockVal|.  If the byte is 0x0, then
 * the lock is cleared (or unlocked).  If it is any non-zero value, then it
 * is locked.  Any specific byte value may have additional meaning to the
 * caller.
 *
 * @returns ESE_APP_RESULT_OK if |lockVal| contains a valid byte.
 */
EseAppResult ese_boot_lock_get(struct EseBootSession *session, EseBootLockId lockId, uint8_t *lockVal);
/**
 * Retrieves extended lock data for the lock specified by |lockId|.
 *
 * |maxSize| specifies how many bytes may be written to |lockData|. |dataLen|
 * will be updated to hold the length of the data received from the applet on
 * success.
 *
 * The first byte of |lockData| will be the lock's value.  The remaining bytes
 * are the associated metadata.  See the README.md for more details
 * on each lock's behavior.
 *
 * @returns ESE_APP_RESULT_OK on success.
 */
EseAppResult ese_boot_lock_xget(
    struct EseBootSession *session, EseBootLockId lockId, uint8_t *lockData,
    uint16_t maxSize, uint16_t *dataLen);

/**
 * Sets the lock specified by |lockId| to |lockVal|.
 *
 * @returns ESE_APP_RESULT_OK on success.
 */

EseAppResult ese_boot_lock_set(struct EseBootSession *session, EseBootLockId lockId, uint8_t lockVal);
/**
 * Sets the lock and its metadata specified by |lockId| and |lockData|,
 * respectively.  |dataLen| indicates the length of |lockData|.
 *
 * The first byte of |lockData| will be treated as the new value for the lock.
 *
 * @returns ESE_APP_RESULT_OK on success.
 */
EseAppResult ese_boot_lock_xset(struct EseBootSession *session, EseBootLockId lockId, const uint8_t *lockData, uint16_t dataLen);

/**
 * Performs a test of the carrier unlock code by allowing the caller to specify
 * a fake internal nonce value, fake internal device data, as well as an actual
 * unlock token (made up of a nonce and signature).
 *
 * @returns ESE_APP_RESULT_OK on success.  On failure, it is worthwhile to
 *          check the upper two bytes in the result code if the lower two bytes
 *          are ESE_APP_RESULT_ERROR_APPLET as it will provide an error
 *          specific to the code path.  These applet codes are not (yet)
 *          considered API and should be relied on for debugging.
 */
EseAppResult ese_boot_carrier_lock_test(struct EseBootSession *session, const uint8_t *testdata, uint16_t len);

/**
 * Transitions the applet from "factory" mode to "production" mode.
 * This can only be done if the bootloader gpio has not been cleared.
 *
 * When not in production mode, the applet will ignore the bootloader gpio
 * and allow for all the locks to be provisioned.  Once |mode| is set
 * to true, LOCK_CARRIER can not be "lock"ed once cleared and any locks
 * that depend on being in the bootloader (gpio not cleared) will respect
 * that value.
 */
EseAppResult ese_boot_set_production(struct EseBootSession *session, bool production_mode);

/**
 * Debugging helper that emits the internal value of production, bootloader gpio,
 * and lock initialization and storage.  It is not insecure in the field, but
 * it is not expected to be needed during normal operation.
 */
EseAppResult ese_boot_get_state(struct EseBootSession *session, uint8_t *state, uint16_t maxSize);

/**
 * Stores |value| in the specified |slot| in the applet.
 *
 * @returns ESE_APP_RESULT_OK on success
 */
EseAppResult ese_boot_rollback_index_write(struct EseBootSession *session, uint8_t slot, uint64_t value);

/**
 * Reads a uint64_t from |slot| into |value|.
 *
 * @returns ESE_APP_RESULT_OK on success.
 */
EseAppResult ese_boot_rollback_index_read(struct EseBootSession *session, uint8_t slot, uint64_t *value);


/**
 * Resets all lock state -- including internal metadata.
 * This should only be called in factory or under test.
 *
 * @returns ESE_APP_RESULT_OK on success.
 */
EseAppResult ese_boot_reset_locks(struct EseBootSession *session);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* ESE_APP_BOOT_H_ */
