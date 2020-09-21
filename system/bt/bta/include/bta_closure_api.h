/******************************************************************************
 *
 *  Copyright 2016 The Android Open Source Project
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

#ifndef BTA_CLOSURE_API_H
#define BTA_CLOSURE_API_H

#include <base/bind.h>
#include <base/callback_forward.h>
#include <base/location.h>

#include <hardware/bluetooth.h>

/*
 * This method post a closure for execution on bta thread. Please see
 * documentation at
 * https://www.chromium.org/developers/coding-style/important-abstractions-and-data-structures
 * for how to handle dynamic memory ownership/smart pointers with base::Owned(),
 * base::Passed(), base::ConstRef() and others.
 */
bt_status_t do_in_bta_thread(const tracked_objects::Location& from_here,
                             const base::Closure& task);

#endif /* BTA_CLOSURE_API_H */
