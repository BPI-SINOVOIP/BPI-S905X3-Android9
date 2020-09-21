/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef CLEARKEY_SESSION_H_
#define CLEARKEY_SESSION_H_

#include <utils/Mutex.h>
#include <utils/RefBase.h>
#include <vector>

#include "ClearKeyTypes.h"


namespace android {
namespace hardware {
namespace drm {
namespace V1_1 {
namespace clearkey {

using ::android::hardware::drm::V1_0::Status;
using ::android::hardware::drm::V1_0::SubSample;

class Session : public RefBase {
public:
    explicit Session(const std::vector<uint8_t>& sessionId)
            : mSessionId(sessionId) {}
    virtual ~Session() {}

    const std::vector<uint8_t>& sessionId() const { return mSessionId; }

    Status getKeyRequest(
            const std::vector<uint8_t>& mimeType,
            const std::string& initDataType,
            std::vector<uint8_t>* keyRequest) const;

    Status provideKeyResponse(
            const std::vector<uint8_t>& response);

    Status decrypt(
            const KeyId keyId, const Iv iv, const uint8_t* srcPtr,
            uint8_t* dstPtr, const std::vector<SubSample> subSamples,
            size_t* bytesDecryptedOut);

private:
    CLEARKEY_DISALLOW_COPY_AND_ASSIGN(Session);

    const std::vector<uint8_t> mSessionId;
    KeyMap mKeyMap;
    Mutex mMapLock;
};

} // namespace clearkey
} // namespace V1_1
} // namespace drm
} // namespace hardware
} // namespace android

#endif // CLEARKEY_SESSION_H_
