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

#include "fake_atap_ops.h"

#include <memory>

namespace atap {

FakeAtapOps::FakeAtapOps() {}
FakeAtapOps::~FakeAtapOps() {}

AtapResult FakeAtapOps::read_product_id(
    uint8_t product_id[ATAP_PRODUCT_ID_LEN]) {
  memset(product_id, 0x00, ATAP_PRODUCT_ID_LEN);
  return ATAP_RESULT_OK;
}

AtapResult FakeAtapOps::get_auth_key_type(AtapKeyType* key_type) {
  *key_type = ATAP_KEY_TYPE_NONE;
  return ATAP_RESULT_OK;
}

AtapResult FakeAtapOps::read_auth_key_cert_chain(AtapCertChain* cert_chain) {
  return ATAP_RESULT_ERROR_UNSUPPORTED_OPERATION;
}

AtapResult FakeAtapOps::write_attestation_key(AtapKeyType key_type,
                                              const AtapBlob* key,
                                              const AtapCertChain* cert_chain) {
  return ATAP_RESULT_OK;
}

AtapResult FakeAtapOps::read_attestation_public_key(

    AtapKeyType key_type,
    uint8_t pubkey[ATAP_KEY_LEN_MAX],
    uint32_t* pubkey_len) {
  return ATAP_RESULT_ERROR_UNSUPPORTED_OPERATION;
}

AtapResult FakeAtapOps::read_soc_global_key(
    uint8_t global_key[ATAP_AES_128_KEY_LEN]) {
  return ATAP_RESULT_ERROR_UNSUPPORTED_OPERATION;
}

AtapResult FakeAtapOps::write_hex_uuid(const uint8_t uuid[ATAP_HEX_UUID_LEN]) {
  return ATAP_RESULT_OK;
}

AtapResult FakeAtapOps::auth_key_sign(const uint8_t* nonce,
                                      uint32_t nonce_len,
                                      uint8_t sig[ATAP_SIGNATURE_LEN_MAX],
                                      uint32_t* sig_len) {
  return ATAP_RESULT_ERROR_UNSUPPORTED_OPERATION;
}

}  // namespace atap
