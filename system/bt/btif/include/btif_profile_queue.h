/******************************************************************************
 *
 *  Copyright 2009-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/*******************************************************************************
 *
 *  Filename:      btif_profile_queue.h
 *
 *  Description:   Bluetooth remote device connection queuing
 *
 ******************************************************************************/

#ifndef BTIF_PROFILE_QUEUE_H
#define BTIF_PROFILE_QUEUE_H

#include <hardware/bluetooth.h>

typedef bt_status_t (*btif_connect_cb_t)(RawAddress* bda, uint16_t uuid);

bt_status_t btif_queue_connect(uint16_t uuid, const RawAddress* bda,
                               btif_connect_cb_t connect_cb);
void btif_queue_cleanup(uint16_t uuid);
void btif_queue_advance();

/**
 * Dispatch the next pending connect request.
 * NOTE: Must be called on the JNI thread.
 *
 * @return BT_STATUS_SUCCESS on success, otherwise the corresponding error
 * code
 */
bt_status_t btif_queue_connect_next(void);

void btif_queue_release();

#endif
