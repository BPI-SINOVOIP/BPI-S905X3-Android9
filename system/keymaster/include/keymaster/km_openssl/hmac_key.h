/*
 * Copyright 2014 The Android Open Source Project
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

#ifndef SYSTEM_KEYMASTER_HMAC_KEY_H_
#define SYSTEM_KEYMASTER_HMAC_KEY_H_

#include "symmetric_key.h"

namespace keymaster {

const size_t kMinHmacLengthBits = 64;
const size_t kMinHmacKeyLengthBits = 64;
const size_t kMaxHmacKeyLengthBits = 2048;  // Some RFC test cases require >1024-bit keys

class HmacKeyFactory : public SymmetricKeyFactory {
  public:
    explicit HmacKeyFactory(const SoftwareKeyBlobMaker* blob_maker,
                            const RandomSource* random_source) :
                                    SymmetricKeyFactory(blob_maker, random_source) {}

    keymaster_error_t LoadKey(KeymasterKeyBlob&& key_material,
                              const AuthorizationSet& additional_params,
                              AuthorizationSet&& hw_enforced,
                              AuthorizationSet&& sw_enforced,
                              UniquePtr<Key>* key) const override;

    OperationFactory* GetOperationFactory(keymaster_purpose_t purpose) const override;

  private:
    bool key_size_supported(size_t key_size_bits) const override {
        return key_size_bits > 0 && key_size_bits % 8 == 00 &&
               key_size_bits >= kMinHmacKeyLengthBits && key_size_bits <= kMaxHmacKeyLengthBits;
    }
    keymaster_error_t validate_algorithm_specific_new_key_params(
        const AuthorizationSet& key_description) const override;
};

class HmacKey : public SymmetricKey {
  public:
    HmacKey(KeymasterKeyBlob&& key_material, AuthorizationSet&& hw_enforced,
            AuthorizationSet&& sw_enforced, const KeyFactory* key_factory)
        : SymmetricKey(move(key_material), move(hw_enforced), move(sw_enforced), key_factory) {}
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_HMAC_KEY_H_
