/*
 * Copyright (C) 2014 The Android Open Source Project
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

#ifndef _BDROID_BUILDCFG_H
#define _BDROID_BUILDCFG_H

#define BLE_VND_INCLUDED TRUE

#define BTA_DM_COD {0x20, BTM_COD_MAJOR_AUDIO, BTM_COD_MINOR_SET_TOP_BOX}


// Turn off BLE_PRIVACY_SPT.  Remote reconnect fails on
// often if this is enabled.
#define BLE_PRIVACY_SPT FALSE

#define BTM_BLE_CONN_INT_MIN_DEF     0x18
#define BTM_BLE_CONN_INT_MAX_DEF     0x28
#define BTM_BLE_CONN_TIMEOUT_DEF     200
/* minimum acceptable connection interval */
#define BTM_BLE_CONN_INT_MIN_LIMIT 0x0006  /*7.5ms=6*1.25*/

#define BTM_BLE_SCAN_SLOW_INT_1 512

/*fix r311&r321 bt not open*/
#define KERNEL_MISSING_CLOCK_BOOTTIME_ALARM TRUE

#endif
