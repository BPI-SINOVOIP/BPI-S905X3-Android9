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
#ifndef ANDROID_HARDWARE_BROADCASTRADIO_COMMON_UTILS_1X_H
#define ANDROID_HARDWARE_BROADCASTRADIO_COMMON_UTILS_1X_H

#include <android/hardware/broadcastradio/1.1/types.h>
#include <chrono>
#include <queue>
#include <thread>

namespace android {
namespace hardware {
namespace broadcastradio {
namespace utils {

enum class HalRevision : uint32_t {
    V1_0 = 1,
    V1_1,
};

/**
 * Checks, if {@code pointer} tunes to {@channel}.
 *
 * For example, having a channel {AMFM_FREQUENCY = 103.3}:
 * - selector {AMFM_FREQUENCY = 103.3, HD_SUBCHANNEL = 0} can tune to this channel;
 * - selector {AMFM_FREQUENCY = 103.3, HD_SUBCHANNEL = 1} can't.
 *
 * @param pointer selector we're trying to match against channel.
 * @param channel existing channel.
 */
bool tunesTo(const V1_1::ProgramSelector& pointer, const V1_1::ProgramSelector& channel);

V1_1::ProgramType getType(const V1_1::ProgramSelector& sel);
bool isAmFm(const V1_1::ProgramType type);

bool isAm(const V1_0::Band band);
bool isFm(const V1_0::Band band);

bool hasId(const V1_1::ProgramSelector& sel, const V1_1::IdentifierType type);

/**
 * Returns ID (either primary or secondary) for a given program selector.
 *
 * If the selector does not contain given type, returns 0 and emits a warning.
 */
uint64_t getId(const V1_1::ProgramSelector& sel, const V1_1::IdentifierType type);

/**
 * Returns ID (either primary or secondary) for a given program selector.
 *
 * If the selector does not contain given type, returns default value.
 */
uint64_t getId(const V1_1::ProgramSelector& sel, const V1_1::IdentifierType type, uint64_t defval);

V1_1::ProgramSelector make_selector(V1_0::Band band, uint32_t channel, uint32_t subChannel = 0);

bool getLegacyChannel(const V1_1::ProgramSelector& sel, uint32_t* channelOut,
                      uint32_t* subChannelOut);

bool isDigital(const V1_1::ProgramSelector& sel);

}  // namespace utils

namespace V1_0 {

bool operator==(const BandConfig& l, const BandConfig& r);

}  // namespace V1_0

}  // namespace broadcastradio
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_BROADCASTRADIO_COMMON_UTILS_1X_H
