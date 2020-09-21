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

#include "atap_util.h"

#include "atap_ops.h"

const char* atap_basename(const char* str) {
  int64_t n = 0;
  size_t len = atap_strlen(str);

  if (len >= 2) {
    for (n = len - 2; n >= 0; n--) {
      if (str[n] == '/') {
        return str + n + 1;
      }
    }
  }
  return str;
}

uint8_t* append_to_buf(uint8_t* buf, const void* data, uint32_t data_size) {
  atap_memcpy(buf, data, data_size);
  return buf + data_size;
}

uint8_t* append_uint32_to_buf(uint8_t* buf, uint32_t x) {
  return append_to_buf(buf, &x, sizeof(uint32_t));
}

uint8_t* append_header_to_buf(uint8_t* buf, uint32_t message_length) {
  buf[0] = ATAP_PROTOCOL_VERSION;
  atap_memset(&buf[1], 0, 3);  // reserved
  buf += 4;
  return append_uint32_to_buf(buf, message_length);
}

uint8_t* append_blob_to_buf(uint8_t* buf, const AtapBlob* blob) {
  atap_assert(blob->data_length == 0 || blob->data);
  buf = append_uint32_to_buf(buf, blob->data_length);
  if (blob->data_length > 0) {
    buf = append_to_buf(buf, blob->data, blob->data_length);
  }
  return buf;
}

uint8_t* append_cert_chain_to_buf(uint8_t* buf,
                                  const AtapCertChain* cert_chain) {
  uint32_t cert_chain_size =
      cert_chain_serialized_size(cert_chain) - sizeof(uint32_t);
  uint32_t i = 0;
  uint8_t* local_buf = buf;

  /* Append size of cert chain, as it is a Variable field. */
  local_buf = append_uint32_to_buf(local_buf, cert_chain_size);

  for (i = 0; i < cert_chain->entry_count; ++i) {
    local_buf = append_blob_to_buf(local_buf, &cert_chain->entries[i]);
  }
  atap_assert(local_buf == (buf + cert_chain_size + sizeof(uint32_t)));
  return local_buf;
}

uint8_t* append_ca_request_to_buf(uint8_t* buf,
                                  const AtapCaRequest* ca_request) {
  uint32_t ca_request_len = ca_request_serialized_size(ca_request);

  buf = append_header_to_buf(buf, ca_request_len - ATAP_HEADER_LEN);
  buf = append_to_buf(buf, ca_request->device_pubkey, ATAP_ECDH_KEY_LEN);
  buf = append_to_buf(buf, ca_request->iv, ATAP_GCM_IV_LEN);
  buf = append_blob_to_buf(buf, &ca_request->encrypted_inner_ca_request);
  return append_to_buf(buf, ca_request->tag, ATAP_GCM_TAG_LEN);
}

uint8_t* append_inner_ca_request_to_buf(
    uint8_t* buf, const AtapInnerCaRequest* inner_ca_request) {
  uint32_t inner_ca_request_len =
      inner_ca_request_serialized_size(inner_ca_request);

  buf = append_header_to_buf(buf, inner_ca_request_len - ATAP_HEADER_LEN);
  buf = append_cert_chain_to_buf(buf, &inner_ca_request->auth_key_cert_chain);
  buf = append_blob_to_buf(buf, &inner_ca_request->signature);
  buf = append_to_buf(
      buf, inner_ca_request->product_id_hash, ATAP_SHA256_DIGEST_LEN);
  buf = append_blob_to_buf(buf, &inner_ca_request->RSA_pubkey);
  buf = append_blob_to_buf(buf, &inner_ca_request->ECDSA_pubkey);
  return append_blob_to_buf(buf, &inner_ca_request->edDSA_pubkey);
}

void copy_from_buf(uint8_t** buf_ptr, void* data, uint32_t data_size) {
  atap_memcpy(data, *buf_ptr, data_size);
  *buf_ptr += data_size;
}

void copy_uint32_from_buf(uint8_t** buf_ptr, uint32_t* x) {
  copy_from_buf(buf_ptr, x, sizeof(uint32_t));
}

bool copy_blob_from_buf(uint8_t** buf_ptr, AtapBlob* blob) {
  atap_memset(blob, 0, sizeof(AtapBlob));
  copy_uint32_from_buf(buf_ptr, &blob->data_length);
  if (blob->data_length > ATAP_BLOB_LEN_MAX) {
    return false;
  }
  if (blob->data_length) {
    blob->data = (uint8_t*)atap_malloc(blob->data_length);
    if (blob->data == NULL) {
      return false;
    }
    copy_from_buf(buf_ptr, blob->data, blob->data_length);
  }
  return true;
}

bool copy_cert_chain_from_buf(uint8_t** buf_ptr, AtapCertChain* cert_chain) {
  uint32_t cert_chain_size = 0;
  int32_t bytes_remaining = 0;
  size_t i = 0;
  bool retval = true;

  atap_memset(cert_chain, 0, sizeof(AtapCertChain));

  /* Copy size of cert chain, as it is a Variable field. */
  copy_from_buf(buf_ptr, &cert_chain_size, sizeof(cert_chain_size));

  if (cert_chain_size > ATAP_CERT_CHAIN_LEN_MAX) {
    return false;
  }
  if (cert_chain_size == 0) {
    return true;
  }
  bytes_remaining = cert_chain_size;
  for (i = 0; i < ATAP_CERT_CHAIN_ENTRIES_MAX; ++i) {
    if (!copy_blob_from_buf(buf_ptr, &cert_chain->entries[i])) {
      retval = false;
      break;
    }
    ++cert_chain->entry_count;
    bytes_remaining -= (sizeof(uint32_t) + cert_chain->entries[i].data_length);
    if (bytes_remaining <= 0) {
      retval = (bytes_remaining == 0);
      break;
    }
  }
  if (retval == false) {
    free_cert_chain(*cert_chain);
  }
  return retval;
}

uint32_t blob_serialized_size(const AtapBlob* blob) {
  return sizeof(uint32_t) + blob->data_length;
}

uint32_t cert_chain_serialized_size(const AtapCertChain* cert_chain) {
  uint32_t size = sizeof(uint32_t);
  uint32_t i = 0;

  for (i = 0; i < cert_chain->entry_count; ++i) {
    size += sizeof(uint32_t) + cert_chain->entries[i].data_length;
  }
  return size;
}

uint32_t ca_request_serialized_size(const AtapCaRequest* ca_request) {
  uint32_t size =
      ATAP_HEADER_LEN + ATAP_ECDH_KEY_LEN + ATAP_GCM_IV_LEN + ATAP_GCM_TAG_LEN;

  size += blob_serialized_size(&ca_request->encrypted_inner_ca_request);
  return size;
}

uint32_t inner_ca_request_serialized_size(
    const AtapInnerCaRequest* inner_ca_request) {
  uint32_t size = ATAP_HEADER_LEN + ATAP_SHA256_DIGEST_LEN;

  size += cert_chain_serialized_size(&inner_ca_request->auth_key_cert_chain);
  size += blob_serialized_size(&inner_ca_request->signature);
  size += blob_serialized_size(&inner_ca_request->RSA_pubkey);
  size += blob_serialized_size(&inner_ca_request->ECDSA_pubkey);
  size += blob_serialized_size(&inner_ca_request->edDSA_pubkey);
  return size;
}

void free_blob(AtapBlob blob) {
  if (blob.data) {
    atap_free(blob.data);
  }
  blob.data_length = 0;
}

void free_cert_chain(AtapCertChain cert_chain) {
  unsigned int i = 0;

  for (i = 0; i < cert_chain.entry_count; ++i) {
    if (cert_chain.entries[i].data) {
      atap_free(cert_chain.entries[i].data);
    }
    cert_chain.entries[i].data_length = 0;
  }
  atap_memset(&cert_chain, 0, sizeof(AtapCertChain));
}

void free_ca_request(AtapCaRequest ca_request) {
  free_blob(ca_request.encrypted_inner_ca_request);
}

void free_inner_ca_request(AtapInnerCaRequest inner_ca_request) {
  free_cert_chain(inner_ca_request.auth_key_cert_chain);
  free_blob(inner_ca_request.signature);
  free_blob(inner_ca_request.RSA_pubkey);
  free_blob(inner_ca_request.ECDSA_pubkey);
  free_blob(inner_ca_request.edDSA_pubkey);
}

bool validate_operation(AtapOperation operation) {
  if (operation != ATAP_OPERATION_CERTIFY &&
      operation != ATAP_OPERATION_ISSUE &&
      operation != ATAP_OPERATION_ISSUE_ENCRYPTED) {
    return false;
  }
  return true;
}

bool validate_curve(AtapCurveType curve) {
  if (curve != ATAP_CURVE_TYPE_P256 && curve != ATAP_CURVE_TYPE_X25519) {
    return false;
  }
  return true;
}

bool validate_encrypted_message(const uint8_t* buf, uint32_t buf_size) {
  uint32_t encrypted_len, message_len;
  uint8_t* buf_ptr = (uint8_t*)buf;

  if (buf_size <
      ATAP_HEADER_LEN + ATAP_GCM_IV_LEN + sizeof(uint32_t) + ATAP_GCM_TAG_LEN) {
    return false;
  }
  if (buf[0] != ATAP_PROTOCOL_VERSION) {
    return false;
  }
  buf_ptr += 4;
  copy_uint32_from_buf(&buf_ptr, &message_len);
  if (message_len != buf_size - ATAP_HEADER_LEN) {
    return false;
  }
  buf_ptr += ATAP_GCM_IV_LEN;
  copy_uint32_from_buf(&buf_ptr, &encrypted_len);
  if (encrypted_len !=
      message_len - ATAP_GCM_IV_LEN - sizeof(uint32_t) - ATAP_GCM_TAG_LEN) {
    return false;
  }
  return true;
}

bool validate_inner_ca_response(const uint8_t* buf,
                                uint32_t buf_size,
                                AtapOperation operation) {
  uint32_t cert_chain_len = 0, len = 0;
  size_t i = 0;
  int tmp = 0;
  uint8_t* buf_ptr = (uint8_t*)buf;

  if (buf_size < ATAP_HEADER_LEN + ATAP_HEX_UUID_LEN +
                     ATAP_INNER_CA_RESPONSE_FIELDS * sizeof(uint32_t)) {
    return false;
  }
  if (buf[0] != ATAP_PROTOCOL_VERSION) {
    return false;
  }
  buf_ptr += 4;
  copy_uint32_from_buf(&buf_ptr, &len);
  if (len != buf_size - ATAP_HEADER_LEN) {
    return false;
  }
  buf_ptr += ATAP_HEX_UUID_LEN;
  for (i = 0; i < ATAP_INNER_CA_RESPONSE_FIELDS; ++i) {
    /* Odd indices are private keys */
    if (i % 2) {
      copy_uint32_from_buf(&buf_ptr, &len);
      /* Product keys (non special purpose) are omitted on certify operation. */
      if (operation == ATAP_OPERATION_CERTIFY &&
          i != ATAP_INNER_CA_RESPONSE_FIELDS - 1 && len != 0) {
        return false;
      }
      if (len > ATAP_KEY_LEN_MAX) {
        return false;
      }
      buf_ptr += len;
    } else {
      copy_uint32_from_buf(&buf_ptr, &cert_chain_len);
      if (cert_chain_len > ATAP_CERT_CHAIN_LEN_MAX) {
        return false;
      }
      tmp = cert_chain_len;
      while (tmp) {
        copy_uint32_from_buf(&buf_ptr, &len);
        if (len > ATAP_CERT_LEN_MAX) {
          return false;
        }
        buf_ptr += len;
        tmp -= (sizeof(uint32_t) + len);
        /* Length value list was not constructed correctly. */
        if (tmp < 0) {
          return false;
        }
      }
    }
  }
  if (buf_ptr != buf + buf_size) {
    return false;
  }
  return true;
}

AtapResult derive_session_key(
    AtapOps* ops,
    const uint8_t device_pubkey[ATAP_ECDH_KEY_LEN],
    const uint8_t ca_pubkey[ATAP_ECDH_KEY_LEN],
    const uint8_t shared_secret[ATAP_ECDH_SHARED_SECRET_LEN],
    const char* info,
    uint8_t* okm,
    uint32_t okm_len) {
  uint8_t salt[2 * ATAP_ECDH_KEY_LEN];

  atap_memcpy(salt, ca_pubkey, ATAP_ECDH_KEY_LEN);
  atap_memcpy(salt + ATAP_ECDH_KEY_LEN, device_pubkey, ATAP_ECDH_KEY_LEN);

  return ops->hkdf_sha256(ops,
                          salt,
                          sizeof(salt),
                          shared_secret,
                          ATAP_ECDH_SHARED_SECRET_LEN,
                          (const uint8_t*)info,
                          atap_strlen(info),
                          okm,
                          okm_len);
}
