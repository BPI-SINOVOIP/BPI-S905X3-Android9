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

//#define LOG_NDEBUG 0
#define LOG_TAG "hidl_ClearKeyCryptoPlugin"
#include <utils/Log.h>

#include "CryptoPlugin.h"
#include "SessionLibrary.h"
#include "TypeConvert.h"

#include <hidlmemory/mapping.h>

namespace android {
namespace hardware {
namespace drm {
namespace V1_1 {
namespace clearkey {

using ::android::hardware::drm::V1_0::BufferType;

Return<void> CryptoPlugin::setSharedBufferBase(
        const hidl_memory& base, uint32_t bufferId) {
    sp<IMemory> hidlMemory = mapMemory(base);
    ALOGE_IF(hidlMemory == nullptr, "mapMemory returns nullptr");

    // allow mapMemory to return nullptr
    mSharedBufferMap[bufferId] = hidlMemory;
    return Void();
}

// Returns negative values for error code and positive values for the size of
// decrypted data.  In theory, the output size can be larger than the input
// size, but in practice this will never happen for AES-CTR.
Return<void> CryptoPlugin::decrypt(
        bool secure,
        const hidl_array<uint8_t, KEY_ID_SIZE>& keyId,
        const hidl_array<uint8_t, KEY_IV_SIZE>& iv,
        Mode mode,
        const Pattern& pattern,
        const hidl_vec<SubSample>& subSamples,
        const SharedBuffer& source,
        uint64_t offset,
        const DestinationBuffer& destination,
        decrypt_cb _hidl_cb) {
    UNUSED(pattern);

    if (secure) {
        _hidl_cb(Status::ERROR_DRM_CANNOT_HANDLE, 0,
            "Secure decryption is not supported with ClearKey.");
        return Void();
    }

    if (mSharedBufferMap.find(source.bufferId) == mSharedBufferMap.end()) {
      _hidl_cb(Status::ERROR_DRM_CANNOT_HANDLE, 0,
               "source decrypt buffer base not set");
      return Void();
    }

    if (destination.type == BufferType::SHARED_MEMORY) {
      const SharedBuffer& dest = destination.nonsecureMemory;
      if (mSharedBufferMap.find(dest.bufferId) == mSharedBufferMap.end()) {
        _hidl_cb(Status::ERROR_DRM_CANNOT_HANDLE, 0,
                 "destination decrypt buffer base not set");
        return Void();
      }
    }

    sp<IMemory> sourceBase = mSharedBufferMap[source.bufferId];
    if (sourceBase == nullptr) {
        _hidl_cb(Status::ERROR_DRM_CANNOT_HANDLE, 0, "source is a nullptr");
        return Void();
    }

    if (source.offset + offset + source.size > sourceBase->getSize()) {
        _hidl_cb(Status::ERROR_DRM_CANNOT_HANDLE, 0, "invalid buffer size");
        return Void();
    }

    uint8_t *base = static_cast<uint8_t *>
            (static_cast<void *>(sourceBase->getPointer()));
    uint8_t* srcPtr = static_cast<uint8_t *>(base + source.offset + offset);
    void* destPtr = NULL;
    if (destination.type == BufferType::SHARED_MEMORY) {
        const SharedBuffer& destBuffer = destination.nonsecureMemory;
        sp<IMemory> destBase = mSharedBufferMap[destBuffer.bufferId];
        if (destBase == nullptr) {
            _hidl_cb(Status::ERROR_DRM_CANNOT_HANDLE, 0, "destination is a nullptr");
            return Void();
        }

        if (destBuffer.offset + destBuffer.size > destBase->getSize()) {
            _hidl_cb(Status::ERROR_DRM_CANNOT_HANDLE, 0, "invalid buffer size");
            return Void();
        }
        destPtr = static_cast<void *>(base + destination.nonsecureMemory.offset);
    } else if (destination.type == BufferType::NATIVE_HANDLE) {
        native_handle_t *handle = const_cast<native_handle_t *>(
        destination.secureMemory.getNativeHandle());
        destPtr = static_cast<void *>(handle);
    }

    // Calculate the output buffer size and determine if any subsamples are
    // encrypted.
    size_t destSize = 0;
    bool haveEncryptedSubsamples = false;
    for (size_t i = 0; i < subSamples.size(); i++) {
        const SubSample &subSample = subSamples[i];
        destSize += subSample.numBytesOfClearData;
        destSize += subSample.numBytesOfEncryptedData;
        if (subSample.numBytesOfEncryptedData > 0) {
        haveEncryptedSubsamples = true;
        }
    }

    if (mode == Mode::UNENCRYPTED) {
        if (haveEncryptedSubsamples) {
            _hidl_cb(Status::ERROR_DRM_CANNOT_HANDLE, 0,
                    "Encrypted subsamples found in allegedly unencrypted data.");
            return Void();
        }

        size_t offset = 0;
        for (size_t i = 0; i < subSamples.size(); ++i) {
            const SubSample& subSample = subSamples[i];
            if (subSample.numBytesOfClearData != 0) {
                memcpy(reinterpret_cast<uint8_t*>(destPtr) + offset,
                       reinterpret_cast<const uint8_t*>(srcPtr) + offset,
                       subSample.numBytesOfClearData);
                offset += subSample.numBytesOfClearData;
            }
        }

        _hidl_cb(Status::OK, static_cast<ssize_t>(offset), "");
        return Void();
    } else if (mode == Mode::AES_CTR) {
        size_t bytesDecrypted;
        Status res = mSession->decrypt(keyId.data(), iv.data(), srcPtr,
                static_cast<uint8_t*>(destPtr), toVector(subSamples), &bytesDecrypted);
        if (res == Status::OK) {
            _hidl_cb(Status::OK, static_cast<ssize_t>(bytesDecrypted), "");
            return Void();
        } else {
            _hidl_cb(Status::ERROR_DRM_DECRYPT, static_cast<ssize_t>(res),
                    "Decryption Error");
            return Void();
        }
    } else {
        _hidl_cb(Status::ERROR_DRM_CANNOT_HANDLE, 0,
                "Selected encryption mode is not supported by the ClearKey DRM Plugin.");
        return Void();
    }
}

Return<Status> CryptoPlugin::setMediaDrmSession(
        const hidl_vec<uint8_t>& sessionId) {
    if (!sessionId.size()) {
        mSession = nullptr;
    } else {
        mSession = SessionLibrary::get()->findSession(sessionId);
        if (!mSession.get()) {
            return Status::ERROR_DRM_SESSION_NOT_OPENED;
        }
    }
    return Status::OK;
}

} // namespace clearkey
} // namespace V1_1
} // namespace drm
} // namespace hardware
} // namespace android
