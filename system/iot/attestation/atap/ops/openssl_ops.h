/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef OPENSSL_OPS_H_
#define OPENSSL_OPS_H_

#include "atap_ops_delegate.h"

namespace atap {

// A partial delegate implementation which implements all crypto ops with
// openssl. All instances of this class must be created on the same thread.
class OpensslOps : public AtapOpsDelegate {
 public:
  OpensslOps();
  ~OpensslOps() override;

  // Overridden AtapOpsDelegate methods.
  AtapResult get_random_bytes(uint8_t* buf, uint32_t buf_size) override;

  AtapResult ecdh_shared_secret_compute(
      AtapCurveType curve,
      const uint8_t other_public_key[ATAP_ECDH_KEY_LEN],
      uint8_t public_key[ATAP_ECDH_KEY_LEN],
      uint8_t shared_secret[ATAP_ECDH_KEY_LEN]) override;

  AtapResult aes_gcm_128_encrypt(const uint8_t* plaintext,
                                 uint32_t len,
                                 const uint8_t iv[ATAP_GCM_IV_LEN],
                                 const uint8_t key[ATAP_AES_128_KEY_LEN],
                                 uint8_t* ciphertext,
                                 uint8_t tag[ATAP_GCM_TAG_LEN]) override;

  AtapResult aes_gcm_128_decrypt(const uint8_t* ciphertext,
                                 uint32_t len,
                                 const uint8_t iv[ATAP_GCM_IV_LEN],
                                 const uint8_t key[ATAP_AES_128_KEY_LEN],
                                 const uint8_t tag[ATAP_GCM_TAG_LEN],
                                 uint8_t* plaintext) override;

  AtapResult sha256(const uint8_t* plaintext,
                    uint32_t plaintext_len,
                    uint8_t hash[ATAP_SHA256_DIGEST_LEN]) override;

  AtapResult hkdf_sha256(const uint8_t* salt,
                         uint32_t salt_len,
                         const uint8_t* ikm,
                         uint32_t ikm_len,
                         const uint8_t* info,
                         uint32_t info_len,
                         uint8_t* okm,
                         int32_t okm_len) override;

  // Can be used during testing to get predictable 'ephemeral' ECDH keys. This
  // must never be called except during testing. For X25519, the expected format
  // is a 32-byte private key. For P256, the expected format is X9.62 DER.
  void SetEcdhKeyForTesting(const void* key_data, size_t size_in_bytes);

 private:
  uint8_t test_key_[512];
  size_t test_key_size_{0};
};

}  // namespace atap

#endif /* OPENSSL_OPS_H_ */
