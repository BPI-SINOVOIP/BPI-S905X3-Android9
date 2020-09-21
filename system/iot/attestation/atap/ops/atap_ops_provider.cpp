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

#include "atap_ops_provider.h"

namespace {

using atap::AtapOpsProvider;

AtapResult forward_read_product_id(AtapOps* ops,
                                   uint8_t product_id[ATAP_PRODUCT_ID_LEN]) {
  return AtapOpsProvider::GetInstanceFromAtapOps(ops)
      ->delegate()
      ->read_product_id(product_id);
}

AtapResult forward_get_auth_key_type(AtapOps* ops, AtapKeyType* key_type) {
  return AtapOpsProvider::GetInstanceFromAtapOps(ops)
      ->delegate()
      ->get_auth_key_type(key_type);
}

AtapResult forward_read_auth_key_cert_chain(AtapOps* ops,
                                            AtapCertChain* cert_chain) {
  return AtapOpsProvider::GetInstanceFromAtapOps(ops)
      ->delegate()
      ->read_auth_key_cert_chain(cert_chain);
}

AtapResult forward_write_attestation_key(AtapOps* ops,
                                         AtapKeyType key_type,
                                         const AtapBlob* key,
                                         const AtapCertChain* cert_chain) {
  return AtapOpsProvider::GetInstanceFromAtapOps(ops)
      ->delegate()
      ->write_attestation_key(key_type, key, cert_chain);
}

AtapResult forward_read_attestation_public_key(AtapOps* ops,
                                               AtapKeyType key_type,
                                               uint8_t pubkey[ATAP_KEY_LEN_MAX],
                                               uint32_t* pubkey_len) {
  return AtapOpsProvider::GetInstanceFromAtapOps(ops)
      ->delegate()
      ->read_attestation_public_key(key_type, pubkey, pubkey_len);
}

AtapResult forward_read_soc_global_key(
    AtapOps* ops, uint8_t global_key[ATAP_AES_128_KEY_LEN]) {
  return AtapOpsProvider::GetInstanceFromAtapOps(ops)
      ->delegate()
      ->read_soc_global_key(global_key);
}

AtapResult forward_write_hex_uuid(AtapOps* ops,
                                  const uint8_t uuid[ATAP_HEX_UUID_LEN]) {
  return AtapOpsProvider::GetInstanceFromAtapOps(ops)
      ->delegate()
      ->write_hex_uuid(uuid);
}

AtapResult forward_get_random_bytes(AtapOps* ops,
                                    uint8_t* buf,
                                    uint32_t buf_size) {
  return AtapOpsProvider::GetInstanceFromAtapOps(ops)
      ->delegate()
      ->get_random_bytes(buf, buf_size);
}

AtapResult forward_auth_key_sign(AtapOps* ops,
                                 const uint8_t* nonce,
                                 uint32_t nonce_len,
                                 uint8_t sig[ATAP_SIGNATURE_LEN_MAX],
                                 uint32_t* sig_len) {
  return AtapOpsProvider::GetInstanceFromAtapOps(ops)
      ->delegate()
      ->auth_key_sign(nonce, nonce_len, sig, sig_len);
}

AtapResult forward_ecdh_shared_secret_compute(
    AtapOps* ops,
    AtapCurveType curve,
    const uint8_t other_public_key[ATAP_ECDH_KEY_LEN],
    uint8_t public_key[ATAP_ECDH_KEY_LEN],
    uint8_t shared_secret[ATAP_ECDH_SHARED_SECRET_LEN]) {
  return AtapOpsProvider::GetInstanceFromAtapOps(ops)
      ->delegate()
      ->ecdh_shared_secret_compute(
          curve, other_public_key, public_key, shared_secret);
}

AtapResult forward_aes_gcm_128_encrypt(AtapOps* ops,
                                       const uint8_t* plaintext,
                                       uint32_t len,
                                       const uint8_t iv[ATAP_GCM_IV_LEN],
                                       const uint8_t key[ATAP_AES_128_KEY_LEN],
                                       uint8_t* ciphertext,
                                       uint8_t tag[ATAP_GCM_TAG_LEN]) {
  return AtapOpsProvider::GetInstanceFromAtapOps(ops)
      ->delegate()
      ->aes_gcm_128_encrypt(plaintext, len, iv, key, ciphertext, tag);
}

AtapResult forward_aes_gcm_128_decrypt(AtapOps* ops,
                                       const uint8_t* ciphertext,
                                       uint32_t len,
                                       const uint8_t iv[ATAP_GCM_IV_LEN],
                                       const uint8_t key[ATAP_AES_128_KEY_LEN],
                                       const uint8_t tag[ATAP_GCM_TAG_LEN],
                                       uint8_t* plaintext) {
  return AtapOpsProvider::GetInstanceFromAtapOps(ops)
      ->delegate()
      ->aes_gcm_128_decrypt(ciphertext, len, iv, key, tag, plaintext);
}

AtapResult forward_sha256(AtapOps* ops,
                          const uint8_t* plaintext,
                          uint32_t plaintext_len,
                          uint8_t hash[ATAP_SHA256_DIGEST_LEN]) {
  return AtapOpsProvider::GetInstanceFromAtapOps(ops)->delegate()->sha256(
      plaintext, plaintext_len, hash);
}

AtapResult forward_hkdf_sha256(AtapOps* ops,
                               const uint8_t* salt,
                               uint32_t salt_len,
                               const uint8_t* ikm,
                               uint32_t ikm_len,
                               const uint8_t* info,
                               uint32_t info_len,
                               uint8_t* okm,
                               uint32_t okm_len) {
  return AtapOpsProvider::GetInstanceFromAtapOps(ops)->delegate()->hkdf_sha256(
      salt, salt_len, ikm, ikm_len, info, info_len, okm, okm_len);
}

}  // namespace

namespace atap {

AtapOpsProvider::AtapOpsProvider() {
  setup_ops();
}

AtapOpsProvider::AtapOpsProvider(AtapOpsDelegate* delegate)
    : delegate_(delegate) {
  setup_ops();
}

AtapOpsProvider::~AtapOpsProvider() {}

void AtapOpsProvider::setup_ops() {
  atap_ops_.user_data = this;
  atap_ops_.read_product_id = forward_read_product_id;
  atap_ops_.get_auth_key_type = forward_get_auth_key_type;
  atap_ops_.read_auth_key_cert_chain = forward_read_auth_key_cert_chain;
  atap_ops_.write_attestation_key = forward_write_attestation_key;
  atap_ops_.read_attestation_public_key = forward_read_attestation_public_key;
  atap_ops_.read_soc_global_key = forward_read_soc_global_key;
  atap_ops_.write_hex_uuid = forward_write_hex_uuid;
  atap_ops_.get_random_bytes = forward_get_random_bytes;
  atap_ops_.auth_key_sign = forward_auth_key_sign;
  atap_ops_.ecdh_shared_secret_compute = forward_ecdh_shared_secret_compute;
  atap_ops_.aes_gcm_128_encrypt = forward_aes_gcm_128_encrypt;
  atap_ops_.aes_gcm_128_decrypt = forward_aes_gcm_128_decrypt;
  atap_ops_.sha256 = forward_sha256;
  atap_ops_.hkdf_sha256 = forward_hkdf_sha256;
}

}  // namespace atap
