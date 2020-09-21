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

#include "permission_helpers.h"

#include <base/logging.h>
#include <base/strings/stringprintf.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <pwd.h>
#include <sys/types.h>
#include "IUserManager.h"

using ::android::binder::Status;

namespace android {
namespace bluetooth {

uid_t foregroundUserId;
uid_t systemUiUid;
static uid_t SYSTEM_UID = 1000;
constexpr int PER_USER_RANGE = 100000;

Status checkPermission(const char* permission) {
  int32_t pid;
  int32_t uid;

  if (android::checkCallingPermission(String16(permission), &pid, &uid)) {
    return Status::ok();
  }

  auto err = ::base::StringPrintf("UID %d / PID %d lacks permission %s", uid,
                                  pid, permission);
  return Status::fromExceptionCode(Status::EX_SECURITY, String8(err.c_str()));
}

bool isCallerActiveUser() {
  IPCThreadState* ipcState = IPCThreadState::selfOrNull();
  if (!ipcState) return true;  // It's a local call

  uid_t callingUid = ipcState->getCallingUid();
  uid_t callingUser = callingUid / PER_USER_RANGE;
  if (callingUid == getuid()) return true;  // It's a local call

  return (foregroundUserId == callingUser) || (systemUiUid == callingUid) ||
         (SYSTEM_UID == callingUid);
}

bool isCallerActiveUserOrManagedProfile() {
  IPCThreadState* ipcState = IPCThreadState::selfOrNull();
  if (!ipcState) return true;  // It's a local call

  uid_t callingUid = ipcState->getCallingUid();
  uid_t callingUser = callingUid / PER_USER_RANGE;
  if (callingUid == getuid()) return true;  // It's a local call

  if ((foregroundUserId == callingUser) || (systemUiUid == callingUid) ||
      (SYSTEM_UID == callingUid))
    return true;

  uid_t parentUser = callingUser;

  sp<IServiceManager> sm = defaultServiceManager();
  sp<IBinder> binder = sm->getService(String16("user"));
  sp<IUserManager> um = interface_cast<IUserManager>(binder);
  if (um != NULL) {
    // Must use Bluetooth process identity when making call to get parent user
    int64_t ident = ipcState->clearCallingIdentity();
    parentUser = um->getProfileParentId(callingUser);
    ipcState->restoreCallingIdentity(ident);
  }

  return foregroundUserId == parentUser;
}

}  // namespace bluetooth
}  // namespace android
