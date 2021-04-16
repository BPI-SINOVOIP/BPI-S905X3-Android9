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

// Turn off BLE_PRIVACY_SPT.  Remote reconnect fails on
// often if this is enabled.
#define BLE_PRIVACY_SPT TRUE

/* minimum acceptable connection interval */
#define BTM_BLE_CONN_INT_MIN_LIMIT 0x0006  /*7.5ms=6*1.25*/

/*fix bt crash about init */
#define KERNEL_MISSING_CLOCK_BOOTTIME_ALARM TRUE

#endif
