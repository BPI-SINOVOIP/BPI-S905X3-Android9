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

#ifndef ATAP_UTIL_H_
#define ATAP_UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "atap_ops.h"
#include "atap_sysdeps.h"
#include "atap_types.h"

#define ATAP_STRINGIFY(x) #x
#define ATAP_TO_STRING(x) ATAP_STRINGIFY(x)

#ifdef ATAP_ENABLE_DEBUG
/* Aborts the program if |expr| is false.
 *
 * This has no effect unless ATAP_ENABLE_DEBUG is defined.
 */
#define atap_assert(expr)                     \
  do {                                        \
    if (!(expr)) {                            \
      atap_fatal("assert fail: " #expr "\n"); \
    }                                         \
  } while (0)
#else
#define atap_assert(expr)
#endif

/* Aborts the program if reached.
 *
 * This has no effect unless ATAP_ENABLE_DEBUG is defined.
 */
#ifdef ATAP_ENABLE_DEBUG
#define atap_assert_not_reached()         \
  do {                                    \
    atap_fatal("assert_not_reached()\n"); \
  } while (0)
#else
#define atap_assert_not_reached()
#endif

/* Aborts the program if |addr| is not word-aligned.
 *
 * This has no effect unless ATAP_ENABLE_DEBUG is defined.
 */
#define atap_assert_aligned(addr) \
  atap_assert((((uintptr_t)addr) & (ATAP_ALIGNMENT_SIZE - 1)) == 0)

#ifdef ATAP_ENABLE_DEBUG
/* Print functions, used for diagnostics.
 *
 * These have no effect unless ATAP_ENABLE_DEBUG is defined.
 */
#define atap_debug(message)               \
  do {                                    \
    atap_printv(atap_basename(__FILE__),  \
                ":",                      \
                ATAP_TO_STRING(__LINE__), \
                ": DEBUG: ",              \
                message,                  \
                NULL);                    \
  } while (0)
#define atap_debugv(message, ...)         \
  do {                                    \
    atap_printv(atap_basename(__FILE__),  \
                ":",                      \
                ATAP_TO_STRING(__LINE__), \
                ": DEBUG: ",              \
                message,                  \
                ##__VA_ARGS__);           \
  } while (0)
#else
#define atap_debug(message)
#define atap_debugv(message, ...)
#endif

/* Prints out a message. This is typically used if a runtime-error
 * occurs.
 */
#define atap_error(message)               \
  do {                                    \
    atap_printv(atap_basename(__FILE__),  \
                ":",                      \
                ATAP_TO_STRING(__LINE__), \
                ": ERROR: ",              \
                message,                  \
                NULL);                    \
  } while (0)
#define atap_errorv(message, ...)         \
  do {                                    \
    atap_printv(atap_basename(__FILE__),  \
                ":",                      \
                ATAP_TO_STRING(__LINE__), \
                ": ERROR: ",              \
                message,                  \
                ##__VA_ARGS__);           \
  } while (0)

/* Prints out a message and calls atap_abort().
 */
#define atap_fatal(message)               \
  do {                                    \
    atap_printv(atap_basename(__FILE__),  \
                ":",                      \
                ATAP_TO_STRING(__LINE__), \
                ": FATAL: ",              \
                message,                  \
                NULL);                    \
    atap_abort();                         \
  } while (0)
#define atap_fatalv(message, ...)         \
  do {                                    \
    atap_printv(atap_basename(__FILE__),  \
                ":",                      \
                ATAP_TO_STRING(__LINE__), \
                ": FATAL: ",              \
                message,                  \
                ##__VA_ARGS__);           \
    atap_abort();                         \
  } while (0)

/* Returns the basename of |str|. This is defined as the last path
 * component, assuming the normal POSIX separator '/'. If there are no
 * separators, returns |str|.
 */
const char* atap_basename(const char* str);

/* These write serialized structures to |buf|, and return
 * |buf| + [number of bytes written].
 */
uint8_t* append_to_buf(uint8_t* buf, const void* data, uint32_t data_size);
uint8_t* append_uint32_to_buf(uint8_t* buf, uint32_t x);
uint8_t* append_header_to_buf(uint8_t* buf, uint32_t message_length);
uint8_t* append_blob_to_buf(uint8_t* buf, const AtapBlob* blob);
uint8_t* append_cert_chain_to_buf(uint8_t* buf,
                                  const AtapCertChain* cert_chain);
uint8_t* append_ca_request_to_buf(uint8_t* buf,
                                  const AtapCaRequest* ca_request);
uint8_t* append_inner_ca_request_to_buf(
    uint8_t* buf, const AtapInnerCaRequest* inner_ca_request);

/* These copy serialized data from |*buf_ptr| to the output structure, and
 * increment |*buf_ptr| by [number of bytes copied]. Returns true on success,
 * false when the serialized format is invalid.
 */
void copy_from_buf(uint8_t** buf_ptr, void* data, uint32_t data_size);
void copy_uint32_from_buf(uint8_t** buf_ptr, uint32_t* x);
bool copy_blob_from_buf(uint8_t** buf_ptr, AtapBlob* blob);
bool copy_cert_chain_from_buf(uint8_t** buf_ptr, AtapCertChain* cert_chain);

/* Returns the serialized size of structures. For AtapCaRequest and
 * AtapInnerCaRequest, this includes the header.
 */
uint32_t blob_serialized_size(const AtapBlob* blob);
uint32_t cert_chain_serialized_size(const AtapCertChain* cert_chain);
uint32_t ca_request_serialized_size(const AtapCaRequest* ca_request);
uint32_t inner_ca_request_serialized_size(
    const AtapInnerCaRequest* inner_ca_request);

/* Frees all data allocated for a structure. */
void free_blob(AtapBlob blob);
void free_cert_chain(AtapCertChain cert_chain);
void free_inner_ca_request(AtapInnerCaRequest inner_ca_request);
void free_ca_request(AtapCaRequest ca_request);

/* These return true if the inputs are valid. For complicated message
 * structures, each expected field is parsed and validated.
 */
bool validate_operation(AtapOperation operation);
bool validate_curve(AtapCurveType curve);
bool validate_encrypted_message(const uint8_t* buf, uint32_t buf_size);
bool validate_inner_ca_response(const uint8_t* buf,
                                uint32_t buf_size,
                                AtapOperation operation);

/* Derives the session key to |okm| using HKDF-SHA256 as the KDF. The input
 * keying material (IKM) is |shared_secret|. The salt is the concatenation
 * |ca_pubkey| + |device_pubkey|. |info| is "KEY" (without trailing NUL
 * character) when deriving the encryption key, and "SIGN" when deriving the
 * 16-byte nonce to be signed by the authentication key.
 */
AtapResult derive_session_key(
    AtapOps* ops,
    const uint8_t device_pubkey[ATAP_ECDH_KEY_LEN],
    const uint8_t ca_pubkey[ATAP_ECDH_KEY_LEN],
    const uint8_t shared_secret[ATAP_ECDH_SHARED_SECRET_LEN],
    const char* info,
    uint8_t* okm,
    uint32_t okm_len);

#ifdef __cplusplus
}
#endif

#endif /* ATAP_UTIL_H_ */
