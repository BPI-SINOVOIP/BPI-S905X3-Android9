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
#ifndef ANDROID_ESED_SCOPED_ESE_CONNECTION_H
#define ANDROID_ESED_SCOPED_ESE_CONNECTION_H

#include <android-base/logging.h>
#include <esecpp/EseInterface.h>

namespace android {
namespace esed {

using ::android::EseInterface;

class ScopedEseConnection {
 public:
  ScopedEseConnection(EseInterface& ese) : mEse_(ese) {}
  ~ScopedEseConnection() { mEse_.close(); }

  bool init() {
    mEse_.init();
    int res = mEse_.open();
    if (res != 0) {
      LOG(ERROR) << "Failed to open eSE connection: " << res;
      return false;
    }
    return true;
  }

 private:
  EseInterface& mEse_;
};

}  // namespace esed
}  // namespace android

#endif  // ANDROID_ESED_SCOPED_ESE_CONNECTION_H
