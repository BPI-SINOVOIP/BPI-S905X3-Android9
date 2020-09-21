/*
 * Copyright (C) 2009 The Android Open Source Project
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

#include "rsDevice.h"
#include "rsContext.h"

namespace android {
namespace renderscript {

Device::Device() {
    mForceSW = false;
}

Device::~Device() {
}

void Device::addContext(Context *rsc) {
    mContexts.push_back(rsc);
}

void Device::removeContext(Context *rsc) {
    for (size_t idx=0; idx < mContexts.size(); idx++) {
        if (mContexts[idx] == rsc) {
            mContexts.erase(mContexts.begin() + idx);
            break;
        }
    }
}

} // namespace renderscript
} // namespace android
