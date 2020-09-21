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
#define LOG_TAG "KeystoreOperation"

#include "operation_proto_handler.h"

#include <android/os/DropBoxManager.h>
#include <google/protobuf/message_lite.h>
#include <keymasterV4_0/Keymaster.h>
#include <keystore/keymaster_types.h>
#include <keystore/keystore_hidl_support.h>
#include <utils/String16.h>

#include "operation_config.pb.h"

namespace keystore {

void determinePurpose(KeyPurpose purpose, OperationConfig* operationConfig) {
    switch (purpose) {
    case KeyPurpose::VERIFY:
        operationConfig->set_purpose("verify");
        break;
    case KeyPurpose::ENCRYPT:
        operationConfig->set_purpose("encrypt");
        break;
    case KeyPurpose::SIGN:
        operationConfig->set_purpose("sign");
        break;
    case KeyPurpose::DECRYPT:
        operationConfig->set_purpose("decrypt");
        break;
    case KeyPurpose::WRAP_KEY:
        operationConfig->set_purpose("wrap");
        break;
    default:
        break;
    }
}

void checkKeyCharacteristics(const hidl_vec<KeyParameter>& characteristics,
                             OperationConfig* operationConfig) {
    for (auto& opParam : characteristics) {
        switch (opParam.tag) {
        case Tag::ALGORITHM:
            operationConfig->set_algorithm(toString(accessTagValue(TAG_ALGORITHM, opParam)));
            break;
        case Tag::KEY_SIZE:
            operationConfig->set_key_size(accessTagValue(TAG_KEY_SIZE, opParam));
            break;
        case Tag::EC_CURVE:
            operationConfig->set_ec_curve(toString(accessTagValue(TAG_EC_CURVE, opParam)));
            break;
        case Tag::AUTH_TIMEOUT:
            operationConfig->set_user_auth_key_timeout(accessTagValue(TAG_AUTH_TIMEOUT, opParam));
            break;
        case Tag::ORIGIN:
            operationConfig->set_origin(toString(accessTagValue(TAG_ORIGIN, opParam)));
            break;
        case Tag::BLOB_USAGE_REQUIREMENTS:
            operationConfig->set_key_blob_usage_reqs(
                toString(accessTagValue(TAG_BLOB_USAGE_REQUIREMENTS, opParam)));
            break;
        case Tag::USER_AUTH_TYPE:
            operationConfig->set_user_auth_type(
                toString(accessTagValue(TAG_USER_AUTH_TYPE, opParam)));
            break;
        default:
            break;
        }
    }
}

void checkOpCharacteristics(const hidl_vec<KeyParameter>& characteristics,
                            OperationConfig* operationConfig) {
    for (auto& opParam : characteristics) {
        switch (opParam.tag) {
        case Tag::BLOCK_MODE:
            operationConfig->set_block_mode(toString(accessTagValue(TAG_BLOCK_MODE, opParam)));
            break;
        case Tag::PADDING:
            operationConfig->set_padding(toString(accessTagValue(TAG_PADDING, opParam)));
            break;
        case Tag::DIGEST:
            operationConfig->set_digest(toString(accessTagValue(TAG_DIGEST, opParam)));
            break;
        default:
            break;
        }
    }
}

void uploadOpAsProto(Operation& op, bool wasOpSuccessful) {
    OperationConfig operationConfig;
    determinePurpose(op.purpose, &operationConfig);
    checkKeyCharacteristics(op.characteristics.softwareEnforced, &operationConfig);
    checkKeyCharacteristics(op.characteristics.hardwareEnforced, &operationConfig);
    checkOpCharacteristics(op.params, &operationConfig);
    auto dropbox = std::make_unique<android::os::DropBoxManager>();
    operationConfig.set_was_op_successful(wasOpSuccessful);

    size_t size = operationConfig.ByteSize();
    auto data = std::make_unique<uint8_t[]>(size);
    operationConfig.SerializeWithCachedSizesToArray(data.get());
    dropbox->addData(android::String16("keymaster"), data.get(), size, 0);
}

}  // namespace keystore
