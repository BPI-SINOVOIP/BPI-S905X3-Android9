/*
 * Copyright 2018 The Android Open Source Project
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

#include "buffer.h"
#include "proto_utils.h"

#include <keymasterV4_0/key_param_output.h>

#include <android-base/logging.h>

#include <openssl/rsa.h>

#include <map>
#include <vector>

namespace android {
namespace hardware {
namespace keymaster {

// HAL
using ::android::hardware::keymaster::V4_0::Algorithm;
using ::android::hardware::keymaster::V4_0::ErrorCode;

// std
using std::map;
using std::pair;
using std::vector;

static const size_t kMaxChunkSize = 256;

class Operation {
public:
    Operation(Algorithm algorithm) : _algorithm(algorithm), _buffer{} {
        switch (_algorithm) {
        case Algorithm::AES:
            _blockSize = 16;
            break;
        case Algorithm::TRIPLE_DES:
            _blockSize = 8;
            break;
        case Algorithm::RSA:
        case Algorithm::EC:
        case Algorithm::HMAC:
            _blockSize = 0;
        default:
            break;
        }
    }

    size_t remaining() const {
        return _buffer.size();
    }

    void append(const hidl_vec<uint8_t>& input, uint32_t *consumed) {
        _buffer.insert(_buffer.end(), input.begin(), input.end());
        *consumed = input.size();
    }

    void peek(hidl_vec<uint8_t> *data) {
        // Retain at least one full block; this is done so that when
        // either GCM mode or PKCS7 padding are in use, the last block
        // will be available to be consumed by final().
        if (_buffer.size() <= _blockSize) {
            *data = vector<uint8_t>();
            return;
        }

        size_t retain;
        if (_blockSize == 0) {
            retain = 0;
        } else {
            retain = (_buffer.size() % _blockSize) + _blockSize;
        }
        const size_t count = std::min(_buffer.size() - retain, kMaxChunkSize);
        *data = vector<uint8_t>(_buffer.begin(), _buffer.begin() + count);
    }

    ErrorCode advance(size_t count) {
        if (count > _buffer.size()) {
            LOG(ERROR) << "Attempt to advance " << count
                       << " bytes, where occupancy is " << _buffer.size();
            return ErrorCode::UNKNOWN_ERROR;
        }
        _buffer.erase(_buffer.begin(), _buffer.begin() + count);
        return ErrorCode::OK;
    }

    void final(hidl_vec<uint8_t> *data) {
        if (data != NULL) {
            *data = _buffer;
        }
        _buffer.clear();
    }

private:
    Algorithm _algorithm;
    size_t _blockSize;
    vector<uint8_t> _buffer;
};

static map<uint64_t, Operation>  buffer_map;
typedef map<uint64_t, Operation>::iterator  buffer_item;

ErrorCode buffer_begin(uint64_t handle, Algorithm algorithm)
{
    if (buffer_map.find(handle) != buffer_map.end()) {
        LOG(ERROR) << "Duplicate operation handle " << handle
                   << "returned by begin()";
        // Final the existing op to potential mishandling of data.
        buffer_final(handle, NULL);
        return ErrorCode::UNKNOWN_ERROR;
    }

    buffer_map.insert(
        pair<uint64_t, Operation>(handle, Operation(algorithm)));
    return ErrorCode::OK;
}

size_t buffer_remaining(uint64_t handle) {
    if (buffer_map.find(handle) == buffer_map.end()) {
        LOG(ERROR) << "Remaining requested on absent operation: " << handle;
        return 0;
    }

    const Operation &op = buffer_map.find(handle)->second;
    return op.remaining();
}

ErrorCode buffer_append(uint64_t handle,
                        const hidl_vec<uint8_t>& input,
                        uint32_t *consumed)
{
    if (buffer_map.find(handle) == buffer_map.end()) {
        LOG(ERROR) << "Append requested on absent operation: " << handle;
        return ErrorCode::UNKNOWN_ERROR;
    }

    Operation *op = &buffer_map.find(handle)->second;
    op->append(input, consumed);
    return ErrorCode::OK;
}

ErrorCode buffer_peek(uint64_t handle,
                      hidl_vec<uint8_t> *data)
{
    if (buffer_map.find(handle) == buffer_map.end()) {
        LOG(ERROR) << "Read requested on absent operation: " << handle;
        return ErrorCode::UNKNOWN_ERROR;
    }

    Operation *op = &buffer_map.find(handle)->second;
    op->peek(data);
    return ErrorCode::OK;
}

ErrorCode buffer_advance(uint64_t handle, size_t count)
{
    if (buffer_map.find(handle) == buffer_map.end()) {
        LOG(ERROR) << "Read requested on absent operation: " << handle;
        return ErrorCode::UNKNOWN_ERROR;
    }

    Operation *op = &buffer_map.find(handle)->second;
    return op->advance(count);
}

ErrorCode buffer_final(uint64_t handle,
                   hidl_vec<uint8_t> *data)
{
    if (buffer_map.find(handle) == buffer_map.end()) {
        LOG(ERROR) << "Final requested on absent operation: " << handle;
        return ErrorCode::UNKNOWN_ERROR;
    }
    Operation *op = &buffer_map.find(handle)->second;
    op->final(data);
    buffer_map.erase(handle);
    return ErrorCode::OK;
}

}  // namespace keymaster
}  // hardware
}  // android
