/*
 * Copyright 2017 The Android Open Source Project
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

#include "atap_ops.h"
#include "atap_util.h"
#include "libatap.h"

static uint8_t session_shared_secret[ATAP_ECDH_SHARED_SECRET_LEN];
static uint8_t session_key[ATAP_AES_128_KEY_LEN];
static AtapOperation operation;

static AtapResult auth_key_signature_generate(
    AtapOps* ops,
    uint8_t device_pubkey[ATAP_ECDH_KEY_LEN],
    uint8_t ca_pubkey[ATAP_ECDH_KEY_LEN],
    uint8_t** signature,
    uint32_t* signature_len) {
  AtapResult ret = 0;
  uint8_t nonce[ATAP_NONCE_LEN];
  *signature = (uint8_t*)atap_malloc(ATAP_SIGNATURE_LEN_MAX);
  if (*signature == NULL) {
    return ATAP_RESULT_ERROR_OOM;
  }
  /* deriving nonce uses same HKDF as deriving session key */
  ret = derive_session_key(ops,
                           device_pubkey,
                           ca_pubkey,
                           session_shared_secret,
                           "SIGN",
                           nonce,
                           ATAP_NONCE_LEN);
  if (ret != ATAP_RESULT_OK) {
    return ret;
  }
  return ops->auth_key_sign(
      ops, nonce, ATAP_NONCE_LEN, *signature, signature_len);
}

static AtapResult read_available_public_keys(
    AtapOps* ops, AtapInnerCaRequest* inner_ca_request) {
  AtapResult ret = 0;

  inner_ca_request->RSA_pubkey.data = (uint8_t*)atap_malloc(ATAP_KEY_LEN_MAX);
  inner_ca_request->RSA_pubkey.data_length = ATAP_KEY_LEN_MAX;
  inner_ca_request->ECDSA_pubkey.data = (uint8_t*)atap_malloc(ATAP_KEY_LEN_MAX);
  inner_ca_request->ECDSA_pubkey.data_length = ATAP_KEY_LEN_MAX;
  inner_ca_request->edDSA_pubkey.data = (uint8_t*)atap_malloc(ATAP_KEY_LEN_MAX);
  inner_ca_request->edDSA_pubkey.data_length = ATAP_KEY_LEN_MAX;

  if (inner_ca_request->RSA_pubkey.data == NULL ||
      inner_ca_request->ECDSA_pubkey.data == NULL ||
      inner_ca_request->edDSA_pubkey.data == NULL) {
    return ATAP_RESULT_ERROR_OOM;
  }
  ret = ops->read_attestation_public_key(
      ops,
      ATAP_KEY_TYPE_RSA,
      inner_ca_request->RSA_pubkey.data,
      &inner_ca_request->RSA_pubkey.data_length);
  if (ret != ATAP_RESULT_OK) {
    return ret;
  }
  ret = ops->read_attestation_public_key(
      ops,
      ATAP_KEY_TYPE_ECDSA,
      inner_ca_request->ECDSA_pubkey.data,
      &inner_ca_request->ECDSA_pubkey.data_length);
  if (ret != ATAP_RESULT_OK) {
    return ret;
  }
  ret = ops->read_attestation_public_key(
      ops,
      ATAP_KEY_TYPE_edDSA,
      inner_ca_request->edDSA_pubkey.data,
      &inner_ca_request->edDSA_pubkey.data_length);
  /* edDSA support is not required in the initial version */
  if (ret == ATAP_RESULT_ERROR_UNSUPPORTED_OPERATION) {
    atap_free(inner_ca_request->edDSA_pubkey.data);
    inner_ca_request->edDSA_pubkey.data = NULL;
    inner_ca_request->edDSA_pubkey.data_length = 0;
    ret = ATAP_RESULT_OK;
  }
  return ret;
}

static AtapResult initialize_session(AtapOps* ops,
                                     const uint8_t* operation_start,
                                     uint32_t operation_start_size,
                                     AtapKeyType* auth_key_type,
                                     uint8_t ca_pubkey[ATAP_ECDH_KEY_LEN],
                                     AtapCaRequest* ca_request,
                                     AtapInnerCaRequest* inner_ca_request) {
  uint32_t message_length = 0;
  AtapCurveType curve_type = 0;
  AtapResult ret = 0;
  uint8_t* buf_ptr = (uint8_t*)operation_start + 4;

  atap_memset(ca_request, 0, sizeof(AtapCaRequest));
  atap_memset(inner_ca_request, 0, sizeof(AtapInnerCaRequest));

  if (operation_start_size != ATAP_OPERATION_START_LEN) {
    return ATAP_RESULT_ERROR_INVALID_INPUT;
  }
  if (operation_start[0] != ATAP_PROTOCOL_VERSION) {
    return ATAP_RESULT_ERROR_INVALID_INPUT;
  }
  copy_uint32_from_buf(&buf_ptr, &message_length);
  if (message_length != ATAP_ECDH_KEY_LEN + 2) {
    return ATAP_RESULT_ERROR_INVALID_INPUT;
  }
  curve_type = operation_start[ATAP_HEADER_LEN];
  operation = operation_start[ATAP_HEADER_LEN + 1];
  if (!validate_operation(operation)) {
    return ATAP_RESULT_ERROR_UNSUPPORTED_OPERATION;
  }
  if (!validate_curve(curve_type)) {
    return ATAP_RESULT_ERROR_UNSUPPORTED_ALGORITHM;
  }

  atap_memcpy(ca_pubkey, &operation_start[10], ATAP_ECDH_KEY_LEN);

  ret = ops->get_auth_key_type(ops, auth_key_type);
  if (ret != ATAP_RESULT_OK) {
    return ret;
  }

  ret = ops->ecdh_shared_secret_compute(ops,
                                        curve_type,
                                        ca_pubkey,
                                        ca_request->device_pubkey,
                                        session_shared_secret);
  if (ret != ATAP_RESULT_OK) {
    return ret;
  }

  return derive_session_key(ops,
                            ca_request->device_pubkey,
                            ca_pubkey,
                            session_shared_secret,
                            "KEY",
                            session_key,
                            ATAP_AES_128_KEY_LEN);
}

static AtapResult compute_auth_signature(
    AtapOps* ops,
    uint8_t ca_pubkey[ATAP_ECDH_KEY_LEN],
    uint8_t device_pubkey[ATAP_ECDH_KEY_LEN],
    AtapInnerCaRequest* inner_ca_request) {
  AtapResult ret = 0;
  /* read auth key cert chain */
  ret = ops->read_auth_key_cert_chain(ops,
                                      &inner_ca_request->auth_key_cert_chain);
  if (ret != ATAP_RESULT_OK) {
    return ret;
  }
  /* generate auth key signature */
  return auth_key_signature_generate(ops,
                                     device_pubkey,
                                     ca_pubkey,
                                     &inner_ca_request->signature.data,
                                     &inner_ca_request->signature.data_length);
}

static AtapResult read_product_id_hash(AtapOps* ops,
                                       AtapInnerCaRequest* inner_ca_request) {
  AtapResult ret = 0;
  uint8_t product_id[ATAP_PRODUCT_ID_LEN];

  ret = ops->read_product_id(ops, product_id);
  if (ret != ATAP_RESULT_OK) {
    return ret;
  }
  return ops->sha256(
      ops, product_id, ATAP_PRODUCT_ID_LEN, inner_ca_request->product_id_hash);
}

static AtapResult encrypt_inner_ca_request(
    AtapOps* ops,
    const AtapInnerCaRequest* inner_ca_request,
    AtapCaRequest* ca_request) {
  AtapResult ret = 0;
  uint32_t inner_ca_request_len = 0;
  uint8_t* inner_ca_request_buf = NULL;

  inner_ca_request_len = inner_ca_request_serialized_size(inner_ca_request);
  inner_ca_request_buf = (uint8_t*)atap_malloc(inner_ca_request_len);
  ca_request->encrypted_inner_ca_request.data =
      (uint8_t*)atap_malloc(inner_ca_request_len);
  ca_request->encrypted_inner_ca_request.data_length = inner_ca_request_len;
  if (inner_ca_request_buf == NULL ||
      ca_request->encrypted_inner_ca_request.data == NULL) {
    return ATAP_RESULT_ERROR_OOM;
  }
  append_inner_ca_request_to_buf(inner_ca_request_buf, inner_ca_request);

  /* generate IV */
  ret = ops->get_random_bytes(ops, ca_request->iv, ATAP_GCM_IV_LEN);
  if (ret != ATAP_RESULT_OK) {
    goto out;
  }

  /* encrypt inner CA request with shared key */
  ret = ops->aes_gcm_128_encrypt(ops,
                                 inner_ca_request_buf,
                                 inner_ca_request_len,
                                 ca_request->iv,
                                 session_key,
                                 ca_request->encrypted_inner_ca_request.data,
                                 ca_request->tag);
out:
  atap_free(inner_ca_request_buf);
  return ret;
}

static AtapResult decrypt_encrypted_message(AtapOps* ops,
                                            const uint8_t* buf,
                                            uint32_t buf_size,
                                            uint8_t* key,
                                            uint8_t** plaintext,
                                            uint32_t* plaintext_len) {
  const uint8_t *iv = NULL, *tag = NULL, *ciphertext = NULL;
  uint32_t encrypted_len = 0;
  uint8_t* buf_ptr = (uint8_t*)buf + ATAP_HEADER_LEN;

  if (!validate_encrypted_message(buf, buf_size)) {
    return ATAP_RESULT_ERROR_INVALID_INPUT;
  }

  iv = buf_ptr;
  buf_ptr += ATAP_GCM_IV_LEN;
  copy_uint32_from_buf(&buf_ptr, &encrypted_len);
  ciphertext = buf_ptr;
  buf_ptr += encrypted_len;
  tag = buf_ptr;

  *plaintext = (uint8_t*)atap_malloc(encrypted_len);
  if (*plaintext == NULL) {
    return ATAP_RESULT_ERROR_OOM;
  }
  *plaintext_len = encrypted_len;

  return ops->aes_gcm_128_decrypt(
      ops, ciphertext, *plaintext_len, iv, session_key, tag, *plaintext);
}

static AtapResult write_attestation_data(AtapOps* ops,
                                         uint8_t** buf_ptr,
                                         AtapKeyType key_type) {
  AtapBlob key;
  AtapCertChain cert_chain;
  AtapResult ret = ATAP_RESULT_OK;

  atap_memset(&key, 0, sizeof(key));
  atap_memset(&cert_chain, 0, sizeof(cert_chain));

  if (!copy_cert_chain_from_buf(buf_ptr, &cert_chain)) {
    ret = ATAP_RESULT_ERROR_OOM;
    goto out;
  }
  if (!copy_blob_from_buf(buf_ptr, &key)) {
    ret = ATAP_RESULT_ERROR_OOM;
    goto out;
  }
  if (key.data_length == 0 && cert_chain.entry_count) {
    ret = ops->write_attestation_key(ops, key_type, NULL, &cert_chain);
  } else if (key.data_length && cert_chain.entry_count) {
    ret = ops->write_attestation_key(ops, key_type, &key, &cert_chain);
  } else if (key.data_length && cert_chain.entry_count == 0) {
    /* We never issue a key without a certificate chain */
    ret = ATAP_RESULT_ERROR_INVALID_INPUT;
  } else if (key_type != ATAP_KEY_TYPE_edDSA &&
             key_type != ATAP_KEY_TYPE_SPECIAL &&
             key_type != ATAP_KEY_TYPE_EPID) {
    /* edDSA, EPID, and special purpose key are optional in version 1*/
    ret = ATAP_RESULT_ERROR_INVALID_INPUT;
  }

out:
  free_blob(key);
  free_cert_chain(cert_chain);
  return ret;
}

static AtapResult write_inner_ca_response(AtapOps* ops,
                                          uint8_t* inner_ca_resp_ptr,
                                          uint32_t inner_ca_resp_len) {
  AtapResult ret = 0;
  uint8_t** buf_ptr = &inner_ca_resp_ptr;

  if (!validate_inner_ca_response(
          inner_ca_resp_ptr, inner_ca_resp_len, operation)) {
    return ATAP_RESULT_ERROR_INVALID_INPUT;
  }
  ret = ops->write_hex_uuid(ops, &inner_ca_resp_ptr[ATAP_HEADER_LEN]);
  if (ret != ATAP_RESULT_OK) {
    return ret;
  }
  *buf_ptr += (ATAP_HEADER_LEN + ATAP_HEX_UUID_LEN);
  ret = write_attestation_data(ops, buf_ptr, ATAP_KEY_TYPE_RSA);
  if (ret != ATAP_RESULT_OK) {
    return ret;
  }
  ret = write_attestation_data(ops, buf_ptr, ATAP_KEY_TYPE_ECDSA);
  if (ret != ATAP_RESULT_OK) {
    return ret;
  }
  ret = write_attestation_data(ops, buf_ptr, ATAP_KEY_TYPE_edDSA);
  /* Device may not support edDSA */
  if (ret != ATAP_RESULT_OK && ret != ATAP_RESULT_ERROR_UNSUPPORTED_ALGORITHM) {
    return ret;
  }
  ret = write_attestation_data(ops, buf_ptr, ATAP_KEY_TYPE_EPID);
  if (ret != ATAP_RESULT_OK) {
    return ret;
  }
  /* Device may not support Special Cast key */
  ret = write_attestation_data(ops, buf_ptr, ATAP_KEY_TYPE_SPECIAL);
  if (ret != ATAP_RESULT_OK) {
    return ret;
  }
  return ATAP_RESULT_OK;
}

AtapResult atap_get_ca_request(AtapOps* ops,
                               const uint8_t* operation_start,
                               uint32_t operation_start_size,
                               uint8_t** ca_request_p,
                               uint32_t* ca_request_size_p) {
  AtapResult ret = 0;
  AtapKeyType auth_key_type = 0;
  AtapCaRequest ca_request;
  AtapInnerCaRequest inner_ca_request;
  uint8_t ca_pubkey[ATAP_ECDH_KEY_LEN];

  ret = initialize_session(ops,
                           operation_start,
                           operation_start_size,
                           &auth_key_type,
                           ca_pubkey,
                           &ca_request,
                           &inner_ca_request);
  if (ret != ATAP_RESULT_OK) {
    goto err;
  }

  if (auth_key_type != ATAP_KEY_TYPE_NONE) {
    ret = compute_auth_signature(
        ops, ca_pubkey, ca_request.device_pubkey, &inner_ca_request);
    if (ret != ATAP_RESULT_OK) {
      goto err;
    }
  }

  if (operation == ATAP_OPERATION_CERTIFY) {
    ret = read_available_public_keys(ops, &inner_ca_request);
    if (ret != ATAP_RESULT_OK) {
      goto err;
    }
  }

  ret = read_product_id_hash(ops, &inner_ca_request);
  if (ret != ATAP_RESULT_OK) {
    goto err;
  }

  ret = encrypt_inner_ca_request(ops, &inner_ca_request, &ca_request);
  if (ret != ATAP_RESULT_OK) {
    goto err;
  }

  *ca_request_size_p = ca_request_serialized_size(&ca_request);
  *ca_request_p = (uint8_t*)atap_malloc(*ca_request_size_p);
  if (*ca_request_p == NULL) {
    *ca_request_size_p = 0;
    ret = ATAP_RESULT_ERROR_OOM;
    goto err;
  }
  append_ca_request_to_buf(*ca_request_p, &ca_request);
  goto out;

err:
  /* clear global secrets */
  atap_memset(session_shared_secret, 0, ATAP_ECDH_SHARED_SECRET_LEN);
  atap_memset(session_key, 0, ATAP_AES_128_KEY_LEN);
  *ca_request_p = NULL;
  *ca_request_size_p = 0;
out:
  free_inner_ca_request(inner_ca_request);
  free_ca_request(ca_request);
  return ret;
}

AtapResult atap_set_ca_response(AtapOps* ops,
                                const uint8_t* ca_response,
                                uint32_t ca_response_size) {
  AtapResult ret = 0;
  uint8_t* inner_ca_resp = NULL;
  uint8_t* inner_inner_ca_resp = NULL;
  uint8_t* inner_ca_resp_ptr = NULL;
  uint32_t inner_ca_resp_len = 0;
  uint32_t inner_inner_ca_resp_len = 0;
  uint8_t soc_global_key[ATAP_AES_128_KEY_LEN];

  ret = decrypt_encrypted_message(ops,
                                  ca_response,
                                  ca_response_size,
                                  session_key,
                                  &inner_ca_resp,
                                  &inner_ca_resp_len);
  if (ret != ATAP_RESULT_OK) {
    goto out;
  }
  inner_ca_resp_ptr = inner_ca_resp;
  if (operation == ATAP_OPERATION_ISSUE_ENCRYPTED) {
    /* Decrypt Encrypted Inner CA Response (encrypted) with SoC global key */
    ret = ops->read_soc_global_key(ops, soc_global_key);
    if (ret != ATAP_RESULT_OK) {
      goto out;
    }
    ret = decrypt_encrypted_message(ops,
                                    inner_ca_resp,
                                    inner_ca_resp_len,
                                    soc_global_key,
                                    &inner_inner_ca_resp,
                                    &inner_inner_ca_resp_len);
    if (ret != ATAP_RESULT_OK) {
      goto out;
    }
    inner_ca_resp_ptr = inner_inner_ca_resp;
    inner_ca_resp_len = inner_inner_ca_resp_len;
  }
  ret = write_inner_ca_response(ops, inner_ca_resp_ptr, inner_ca_resp_len);

out:
  if (inner_ca_resp) {
    atap_free(inner_ca_resp);
  }
  if (inner_inner_ca_resp) {
    atap_free(inner_inner_ca_resp);
  }
  return ret;
}
