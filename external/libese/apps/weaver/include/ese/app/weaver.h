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

#ifndef ESE_APP_WEAVER_H_
#define ESE_APP_WEAVER_H_ 1

#include "../../../../../libese/include/ese/ese.h"
#include "../../../../../libese/include/ese/log.h"
#include "../../../../../libese-sysdeps/include/ese/sysdeps.h"

#include "../../../../include/ese/app/result.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * EseWeaverSession carries the necessary start for interfacing
 * with the methods below.
 *
 * Its usage follows a lifecycle like:
 *
 *   EseAppResult res;
 *   EseWeaverSession session;
 *   ese_weaver_session_init(&session);
 *   res = ese_weaver_session_open(ese, &session);
 *   if (res != ESE_APP_RESULT_OK) {
 *     ... handle error (especially cooldown) ...
 *   }
 *   ... ese_weaver_* ...
 *   ese_weaver_session_close(&session);
 *
 */
struct EseWeaverSession {
  struct EseInterface *ese;
  bool active;
  uint8_t channel_id;
};

/** The keys are 16 bytes */
const uint8_t kEseWeaverKeySize = 16;

/** The values are 16 bytes */
const uint8_t kEseWeaverValueSize = 16;


const int ESE_WEAVER_READ_WRONG_KEY = ese_make_app_result(0x6a, 0x85);
const int ESE_WEAVER_READ_TIMEOUT = ese_make_app_result(0x6a, 0x87);

/**
 * Initializes a pre-allocated |session| for use.
 */
void ese_weaver_session_init(struct EseWeaverSession *session);

/**
 * Configures a communication session with the Storage applet using a logical
 * channel on an already open |ese| object.
 *
 * @returns ESE_APP_RESULT_OK on success.
 */
EseAppResult ese_weaver_session_open(struct EseInterface *ese, struct EseWeaverSession *session);

/**
 * Shuts down the logical channel with the Storage applet and invalidates
 * the |session| internal state.
 *
 * @returns ESE_APP_RESULT_OK on success.
 */
EseAppResult ese_weaver_session_close(struct EseWeaverSession *session);

/**
 * Retreives the number of slots available.
 * @returns ESE_APP_RESULT_OK if |numSlots| contains a valid value.
 */
EseAppResult ese_weaver_get_num_slots(struct EseWeaverSession *session, uint32_t *numSlots);

/**
 * Writes a new key-value pair into the slot.
 *
 * |key| and |value| must be buffers of sizes |kEseWeaverKeySize| and
 * |kEseWeaverValueSize| respectively.
 *
 * @returns ESE_APP_RESULT_OK on success.
 */
EseAppResult ese_weaver_write(struct EseWeaverSession *session, uint32_t slotId,
                              const uint8_t *key, const uint8_t *value);


/**
 * Reads the value in the slot provided the correct key was passed.
 *
 * |key| and |value| must be buffers of sizes |kEseWeaverKeySize| and
 * |kEseWeaverValueSize| respectively.
 *
 * @returns ESE_APP_RESULT_OK if |value| was filled with the value.
 *          ESE_WEAVER_READ_WRONG_KEY if |key| was wrong and |timeout| contains
 *          a valid timeout.
 *          ESE_WEAVER_READ_TIMEOUT if Weaver is in backoff mode and |timeout|
 *          contains a valid timeout.
 */
EseAppResult ese_weaver_read(struct EseWeaverSession *session, uint32_t slotId,
                             const uint8_t *key, uint8_t *value, uint32_t *timeout);

/**
 * Erase the value of a slot whilst maintaining the same key.
 *
 * @returns ESE_APP_RESULT_OK on success.
 */
EseAppResult ese_weaver_erase_value(struct EseWeaverSession *session, uint32_t slotId);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* ESE_APP_WEAVER_H_ */
