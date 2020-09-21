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

#include "openssl_ops.h"

#include <libatap/libatap.h>
#include <openssl/aead.h>
#include <openssl/curve25519.h>
#include <openssl/digest.h>
#include <openssl/ec.h>
#include <openssl/ecdh.h>
#include <openssl/hkdf.h>
#include <openssl/obj_mac.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

namespace atap {

OpensslOps::OpensslOps() {}
OpensslOps::~OpensslOps() {}

AtapResult OpensslOps::get_random_bytes(uint8_t* buf, uint32_t buf_size) {
  if (RAND_bytes(buf, buf_size) != 1) {
    atap_error("Error getting random bytes");
    return ATAP_RESULT_ERROR_IO;
  }
  return ATAP_RESULT_OK;
}

AtapResult OpensslOps::ecdh_shared_secret_compute(
    AtapCurveType curve,
    const uint8_t other_public_key[ATAP_ECDH_KEY_LEN],
    uint8_t public_key[ATAP_ECDH_KEY_LEN],
    uint8_t shared_secret[ATAP_ECDH_SHARED_SECRET_LEN]) {
  AtapResult result = ATAP_RESULT_OK;
  EC_GROUP* group = NULL;
  EC_POINT* other_point = NULL;
  EC_KEY* pkey = NULL;
  if (curve == ATAP_CURVE_TYPE_X25519) {
    uint8_t x25519_priv_key[32];
    uint8_t x25519_pub_key[32];
    if (test_key_size_ == 32) {
      atap_memcpy(x25519_priv_key, test_key_, 32);
      X25519_public_from_private(x25519_pub_key, x25519_priv_key);
    } else {
      // Generate an ephemeral key pair.
      X25519_keypair(x25519_pub_key, x25519_priv_key);
    }
    atap_memset(public_key, 0, ATAP_ECDH_KEY_LEN);
    atap_memcpy(public_key, x25519_pub_key, 32);
    X25519(shared_secret, x25519_priv_key, other_public_key);
  } else if (curve == ATAP_CURVE_TYPE_P256) {
    group = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
    other_point = EC_POINT_new(group);
    if (!EC_POINT_oct2point(group,
                            other_point,
                            other_public_key,
                            ATAP_ECDH_KEY_LEN,
                            NULL)) {
      atap_error("Deserializing other_public_key failed");
      result = ATAP_RESULT_ERROR_CRYPTO;
      goto out;
    }

    if (test_key_size_ > 0) {
      const uint8_t* buf_ptr = test_key_;
      pkey = d2i_ECPrivateKey(nullptr, &buf_ptr, test_key_size_);
      EC_KEY_set_group(pkey, group);
    } else {
      pkey = EC_KEY_new();
      if (!pkey) {
        atap_error("Error allocating EC key");
        result = ATAP_RESULT_ERROR_OOM;
        goto out;
      }
      if (1 != EC_KEY_set_group(pkey, group)) {
        atap_error("EC_KEY_set_group failed");
        result = ATAP_RESULT_ERROR_CRYPTO;
        goto out;
      }
      if (1 != EC_KEY_generate_key(pkey)) {
        atap_error("EC_KEY_generate_key failed");
        result = ATAP_RESULT_ERROR_CRYPTO;
        goto out;
      }
    }
    const EC_POINT* public_point = EC_KEY_get0_public_key(pkey);
    if (!EC_POINT_point2oct(group,
                            public_point,
                            POINT_CONVERSION_COMPRESSED,
                            public_key,
                            ATAP_ECDH_KEY_LEN,
                            NULL)) {
      atap_error("Serializing public_key failed");
      result = ATAP_RESULT_ERROR_CRYPTO;
      goto out;
    }

    if (-1 == ECDH_compute_key(shared_secret,
                               ATAP_ECDH_SHARED_SECRET_LEN,
                               other_point,
                               pkey,
                               NULL)) {
      atap_error("Error computing shared secret");
      result = ATAP_RESULT_ERROR_CRYPTO;
      goto out;
    }
  } else {
    atap_error("Unsupported ECDH curve");
    result = ATAP_RESULT_ERROR_UNSUPPORTED_OPERATION;
    goto out;
  }

out:
  if (group) EC_GROUP_free(group);
  if (other_point) EC_POINT_free(other_point);
  if (pkey) EC_KEY_free(pkey);
  return result;
}

AtapResult OpensslOps::aes_gcm_128_encrypt(
    const uint8_t* plaintext,
    uint32_t len,
    const uint8_t iv[ATAP_GCM_IV_LEN],
    const uint8_t key[ATAP_AES_128_KEY_LEN],
    uint8_t* ciphertext,
    uint8_t tag[ATAP_GCM_TAG_LEN]) {
  AtapResult ret = ATAP_RESULT_OK;
  EVP_AEAD_CTX ctx;
  if (!EVP_AEAD_CTX_init(&ctx,
                         EVP_aead_aes_128_gcm(),
                         key,
                         ATAP_AES_128_KEY_LEN,
                         ATAP_GCM_TAG_LEN,
                         NULL)) {
    atap_error("Error initializing EVP_AEAD_CTX");
    return ATAP_RESULT_ERROR_CRYPTO;
  }
  uint8_t* out_buf = (uint8_t*)atap_malloc(len + ATAP_GCM_TAG_LEN);
  size_t out_len = len + ATAP_GCM_TAG_LEN;
  if (!EVP_AEAD_CTX_seal(&ctx,
                         out_buf,
                         &out_len,
                         len + ATAP_GCM_TAG_LEN,
                         iv,
                         ATAP_GCM_IV_LEN,
                         plaintext,
                         len,
                         NULL,
                         0)) {
    atap_error("Error encrypting");
    ret = ATAP_RESULT_ERROR_CRYPTO;
    goto out;
  }
  atap_memcpy(ciphertext, out_buf, len);
  atap_memcpy(tag, out_buf + len, ATAP_GCM_TAG_LEN);

out:
  atap_free(out_buf);
  EVP_AEAD_CTX_cleanup(&ctx);
  return ret;
}

AtapResult OpensslOps::aes_gcm_128_decrypt(
    const uint8_t* ciphertext,
    uint32_t len,
    const uint8_t iv[ATAP_GCM_IV_LEN],
    const uint8_t key[ATAP_AES_128_KEY_LEN],
    const uint8_t tag[ATAP_GCM_TAG_LEN],
    uint8_t* plaintext) {
  AtapResult ret = ATAP_RESULT_OK;
  EVP_AEAD_CTX ctx;
  if (!EVP_AEAD_CTX_init(&ctx,
                         EVP_aead_aes_128_gcm(),
                         key,
                         ATAP_AES_128_KEY_LEN,
                         ATAP_GCM_TAG_LEN,
                         NULL)) {
    atap_error("Error initializing EVP_AEAD_CTX");
    return ATAP_RESULT_ERROR_CRYPTO;
  }
  uint8_t* in_buf = (uint8_t*)atap_malloc(len + ATAP_GCM_TAG_LEN);
  atap_memcpy(in_buf, ciphertext, len);
  atap_memcpy(in_buf + len, tag, ATAP_GCM_TAG_LEN);
  size_t out_len = len;
  if (!EVP_AEAD_CTX_open(&ctx,
                         plaintext,
                         &out_len,
                         len,
                         iv,
                         ATAP_GCM_IV_LEN,
                         in_buf,
                         len + ATAP_GCM_TAG_LEN,
                         NULL,
                         0)) {
    atap_error("Error decrypting");
    ret = ATAP_RESULT_ERROR_CRYPTO;
    goto out;
  }
out:
  atap_free(in_buf);
  EVP_AEAD_CTX_cleanup(&ctx);
  return ret;
}

AtapResult OpensslOps::sha256(const uint8_t* plaintext,
                              uint32_t plaintext_len,
                              uint8_t hash[ATAP_SHA256_DIGEST_LEN]) {
  SHA256(plaintext, plaintext_len, hash);
  return ATAP_RESULT_OK;
}

AtapResult OpensslOps::hkdf_sha256(const uint8_t* salt,
                                   uint32_t salt_len,
                                   const uint8_t* ikm,
                                   uint32_t ikm_len,
                                   const uint8_t* info,
                                   uint32_t info_len,
                                   uint8_t* okm,
                                   int32_t okm_len) {
  if (!HKDF(okm,
            okm_len,
            EVP_sha256(),
            ikm,
            ikm_len,
            salt,
            salt_len,
            info,
            info_len)) {
    atap_error("Error in key derivation");
    return ATAP_RESULT_ERROR_CRYPTO;
  }
  return ATAP_RESULT_OK;
}

void OpensslOps::SetEcdhKeyForTesting(const void* key_data,
                                      size_t size_in_bytes) {
  if (size_in_bytes > sizeof(test_key_)) {
    size_in_bytes = sizeof(test_key_);
  }
  if (size_in_bytes > 0) {
    atap_memcpy(test_key_, key_data, size_in_bytes);
  }
  test_key_size_ = size_in_bytes;
}

}  // namespace atap
