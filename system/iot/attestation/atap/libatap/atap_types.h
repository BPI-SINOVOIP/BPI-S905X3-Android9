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

#ifndef ATAP_TYPES_H_
#define ATAP_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "atap_sysdeps.h"

struct AtapOps;
typedef struct AtapOps AtapOps;

/* Return codes used for all operations.
 *
 * ATAP_RESULT_OK is returned if the requested operation was
 * successful.
 *
 * ATAP_RESULT_ERROR_IO is returned if the underlying hardware (disk
 * or other subsystem) encountered an I/O error.
 *
 * ATAP_RESULT_ERROR_OOM is returned if unable to allocate memory.
 *
 * ATAP_RESULT_ERROR_INVALID_INPUT is returned if inputs are invalid.
 *
 * ATAP_RESULT_ERROR_UNSUPPORTED_ALGORITHM is returned if the device does
 * not support the requested algorithm.
 *
 * ATAP_RESULT_ERROR_UNSUPPORTED_OPERATION is returned if the device does
 * not support the requested operation.
 *
 * ATAP_RESULT_ERROR_CRYPTO is returned if a crypto operation failed.
 */
typedef enum {
  ATAP_RESULT_OK,
  ATAP_RESULT_ERROR_IO,
  ATAP_RESULT_ERROR_OOM,
  ATAP_RESULT_ERROR_INVALID_INPUT,
  ATAP_RESULT_ERROR_UNSUPPORTED_ALGORITHM,
  ATAP_RESULT_ERROR_UNSUPPORTED_OPERATION,
  ATAP_RESULT_ERROR_CRYPTO,
  ATAP_RESULT_ERROR_STORAGE,
} AtapResult;

typedef enum {
  ATAP_KEY_TYPE_NONE = 0,
  ATAP_KEY_TYPE_RSA = 1,
  ATAP_KEY_TYPE_ECDSA = 2,
  ATAP_KEY_TYPE_edDSA = 3,
  ATAP_KEY_TYPE_EPID = 4,
  ATAP_KEY_TYPE_SPECIAL = 5 /* in protocol v1, this is always the "cast" key
                             * persisted by the TEE */
} AtapKeyType;

typedef enum {
  ATAP_CURVE_TYPE_NONE = 0,
  ATAP_CURVE_TYPE_P256 = 1,
  ATAP_CURVE_TYPE_X25519 = 2,
} AtapCurveType;

typedef enum {
  ATAP_OPERATION_NONE = 0,
  ATAP_OPERATION_CERTIFY = 1,
  ATAP_OPERATION_ISSUE = 2,
  ATAP_OPERATION_ISSUE_ENCRYPTED = 3
} AtapOperation;

#define ATAP_PROTOCOL_VERSION 1
#define ATAP_HEADER_LEN 8
#define ATAP_ECDH_KEY_LEN 33
#define ATAP_ECDH_SHARED_SECRET_LEN 32
#define ATAP_OPERATION_START_LEN (ATAP_HEADER_LEN + 2 + ATAP_ECDH_KEY_LEN)
#define ATAP_AES_128_KEY_LEN 16
#define ATAP_GCM_IV_LEN 12
#define ATAP_GCM_TAG_LEN 16
#define ATAP_SHA256_DIGEST_LEN 32
#define ATAP_PRODUCT_ID_LEN 16
#define ATAP_NONCE_LEN 16
#define ATAP_KEY_LEN_MAX 2048
#define ATAP_CERT_LEN_MAX 2048
#define ATAP_CERT_CHAIN_LEN_MAX 8192
#define ATAP_CERT_CHAIN_ENTRIES_MAX 8
#define ATAP_BLOB_LEN_MAX ATAP_CERT_CHAIN_LEN_MAX
#define ATAP_SIGNATURE_LEN_MAX 512
#define ATAP_HEX_UUID_LEN 32
#define ATAP_INNER_CA_RESPONSE_FIELDS 10

typedef struct {
  uint8_t* data;
  uint32_t data_length;
} AtapBlob;

typedef struct {
  AtapBlob entries[ATAP_CERT_CHAIN_ENTRIES_MAX];
  uint32_t entry_count;
} AtapCertChain;

typedef struct {
  uint8_t header[ATAP_HEADER_LEN];
  AtapCertChain auth_key_cert_chain;
  AtapBlob signature;
  uint8_t product_id_hash[ATAP_SHA256_DIGEST_LEN];
  AtapBlob RSA_pubkey;
  AtapBlob ECDSA_pubkey;
  AtapBlob edDSA_pubkey;
} AtapInnerCaRequest;

typedef struct {
  uint8_t header[ATAP_HEADER_LEN];
  uint8_t device_pubkey[ATAP_ECDH_KEY_LEN];
  uint8_t iv[ATAP_GCM_IV_LEN];
  AtapBlob encrypted_inner_ca_request;
  uint8_t tag[ATAP_GCM_TAG_LEN];
} AtapCaRequest;

typedef struct {
  uint8_t header[ATAP_HEADER_LEN];
  uint8_t iv[ATAP_GCM_IV_LEN];
  AtapBlob encrypted;
  uint8_t tag[ATAP_GCM_TAG_LEN];
} AtapEncryptedMessage;

#ifdef __cplusplus
}
#endif

#endif /* ATAP_TYPES_H_ */
