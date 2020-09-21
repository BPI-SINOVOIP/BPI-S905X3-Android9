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

#include <apdu/apdu.h>

#include <limits>

namespace android {

CommandApdu::CommandApdu(const uint8_t cla, const uint8_t ins, const uint8_t p1, const uint8_t p2,
        const size_t lc, const size_t le) {
    constexpr size_t headerSize = 4;
    constexpr size_t shortLcMax = std::numeric_limits<uint8_t>::max();
    constexpr size_t shortLeMax = std::numeric_limits<uint8_t>::max() + 1;
    //constexpr size_t extendedLcMax = std::numeric_limits<uint16_t>::max();
    constexpr size_t extendedLeMax = std::numeric_limits<uint16_t>::max() + 1;

    const bool extended = lc > shortLcMax || le > shortLeMax;
    const bool hasLc = lc > 0;
    const bool hasLe = le > 0;
    const size_t lcSize = hasLc ? (extended ? 3 : 1) : 0;
    const size_t leSize = hasLe ? (extended ? (hasLc ? 2 : 3) : 1) : 0;
    const size_t commandSize = headerSize + lcSize + lc + leSize;
    mCommand.resize(commandSize, 0);

    // All cases have the header
    auto it = mCommand.begin();
    *it++ = cla;
    *it++ = ins;
    *it++ = p1;
    *it++ = p2;

    // Cases 3 & 4 send data
    if (hasLc) {
        if (extended) {
            *it++ = 0;
            *it++ = 0xff & (lc >> 8);
        }
        *it++ = 0xff & lc;
        mDataBegin = it;
        it += lc;
        mDataEnd = it;
    } else {
        mDataBegin = mDataEnd = mCommand.end();
    }

    // Cases 2 & 4 expect data back
    if (hasLe) {
        if (extended) {
            if (!hasLc) {
                *it++ = 0;
            }
            const bool isLeMax = le == extendedLeMax;
            *it++ = (isLeMax ? 0 : 0xff & (le >> 8));
            *it++ = (isLeMax ? 0 : 0xff & le);
        } else {
            *it++ = (le == shortLeMax ? 0 : 0xff & le);
        }
    }
}

} // namespace android
