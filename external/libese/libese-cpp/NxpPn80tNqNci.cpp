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

#include <esecpp/NxpPn80tNqNci.h>
ESE_INCLUDE_HW(ESE_HW_NXP_PN80T_NQ_NCI);

namespace android {

void NxpPn80tNqNci::init() {
    if (mEse == nullptr) {
        mEse = new ::EseInterface;
    }
    ese_init(mEse, ESE_HW_NXP_PN80T_NQ_NCI);
}

int NxpPn80tNqNci::open() {
    const int ret = ese_open(mEse, nullptr);
    mOpen = !(ret < 0);
    return ret;
}

void NxpPn80tNqNci::close() {
    if (mOpen) {
        ese_close(mEse);
        mOpen = false;
    }
    delete mEse;
    mEse = nullptr;
}

} // namespace android
