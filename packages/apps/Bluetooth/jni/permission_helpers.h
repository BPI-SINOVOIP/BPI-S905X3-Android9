/*
 * Copyright 2017 The Android Open Source Project
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
#pragma once

#include <binder/Status.h>

namespace android {
namespace bluetooth {

const char PERMISSION_BLUETOOTH[] = "android.permission.BLUETOOTH";
const char PERMISSION_BLUETOOTH_ADMIN[] = "android.permission.BLUETOOTH_ADMIN";
const char PERMISSION_BLUETOOTH_PRIVILEGED[] =
    "android.permission.BLUETOOTH_PRIVILEGED";

extern uid_t foregroundUserId;
extern uid_t systemUiUid;

android::binder::Status checkPermission(const char* permission);
bool isCallerActiveUser();
bool isCallerActiveUserOrManagedProfile();

}  // namespace bluetooth
}  // namespace android

#define ENFORCE_PERMISSION(permission)                     \
  {                                                        \
    android::binder::Status status =                       \
        android::bluetooth::checkPermission((permission)); \
    if (!status.isOk()) {                                  \
      return status;                                       \
    }                                                      \
  }