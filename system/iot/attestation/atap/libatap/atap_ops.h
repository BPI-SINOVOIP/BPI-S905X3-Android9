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

#ifndef ATAP_OPS_H_
#define ATAP_OPS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "atap_sysdeps.h"
#include "atap_types.h"

/* High-level operations/functions/methods that are platform
 * dependent.
 */
struct AtapOps {
  /* This pointer can be used by the application/TEE and is typically
   * used in each operation to get a pointer to platform-specific
   * resources. It cannot be used by this library.
   */
  void* user_data;

  /* Reads the Product Id from permanent attibutes, and outputs
   * ATAP_PRODUCT_ID_LEN bytes to |product_id|. On success, returns
   * ATAP_RESULT_OK.
   */
  AtapResult (*read_product_id)(AtapOps* ops,
                                uint8_t product_id[ATAP_PRODUCT_ID_LEN]);

  /* Outputs the type of the authentication key to |key_type|. On success,
   * returns ATAP_RESULT_OK.
   */
  AtapResult (*get_auth_key_type)(AtapOps* ops, AtapKeyType* key_type);

  /* Reads the stored authentication key certificate chain. On success,
   * returns ATAP_RESULT_OK and populates |cert_chain|. New memory will be
   * allocated for the certificate chain entries, and caller takes
   * ownership. Entries for |cert_chain| must be allocated using
   * atap_malloc(), and cert_chain->entry_count must be valid.
   */
  AtapResult (*read_auth_key_cert_chain)(AtapOps* ops,
                                         AtapCertChain* cert_chain);

  /* Writes the |key_type| attestation |key| and |cert_chain|. The data
   * MUST be stored in a location that cannot be read or written to by
   * Android. For certify operations, |key| will be NULL. On success,
   * returns ATAP_RESULT_OK.
   */
  AtapResult (*write_attestation_key)(AtapOps* ops,
                                      AtapKeyType key_type,
                                      const AtapBlob* key,
                                      const AtapCertChain* cert_chain);

  /* Reads an asymmetric public key of type |key_type| to be certified
   * for attestation. The keypair to be certified may either be generated
   * on the fly or provisioned and securely stored at an earlier stage. On
   * success, returns ATAP_RESULT_OK and writes at most ATAP_KEY_LEN_MAX
   * bytes to |pubkey|, and the size of the public  key is written to
   * |*pubkey_len|.
   */
  AtapResult (*read_attestation_public_key)(AtapOps* ops,
                                            AtapKeyType key_type,
                                            uint8_t pubkey[ATAP_KEY_LEN_MAX],
                                            uint32_t* pubkey_len);

  /* Reads the SoC global key. If an SoC global key is not supported,
   * ATAP_RESULT_ERROR_UNSUPPORTED_OPERATION is returned and nothing is
   * written to |global_key|. On success, returns ATAP_RESULT_OK and writes
   * ATAP_AES_128_KEY_LEN bytes to |global_key|.
   */
  AtapResult (*read_soc_global_key)(AtapOps* ops,
                                    uint8_t global_key[ATAP_AES_128_KEY_LEN]);

  /* Writes the hex encoded UUID that appears in the subjectName of the
   * Product Key Certificate to storage. The UUID will be read by
   * invoking a fastboot command. On success, returns ATAP_RESULT_OK.
   */
  AtapResult (*write_hex_uuid)(AtapOps* ops,
                               const uint8_t uuid[ATAP_HEX_UUID_LEN]);

  /* Outputs |buf_size| random bytes to |buf|. Bytes should be
   * generated with either a hardware random number generator, or a
   * pseudorandom function seeded with hardware entropy.
   */
  AtapResult (*get_random_bytes)(AtapOps* ops, uint8_t* buf, uint32_t buf_size);

  /* Signs |nonce_len| bytes of |nonce| with the authentication key. On
   * success, returns ATAP_RESULT_OK and writes at most
   * ATAP_SIGNATURE_LEN_MAX bytes to |signature|, and the size of the
   * signature is written to |*signature_len|.
   */
  AtapResult (*auth_key_sign)(AtapOps* ops,
                              const uint8_t* nonce,
                              uint32_t nonce_len,
                              uint8_t signature[ATAP_SIGNATURE_LEN_MAX],
                              uint32_t* signature_len);

  /* Generates a new ECDH keypair using curve |curve|, and writes
   * ATAP_ECDH_KEY_LEN bytes of the public key to |pubkey|. Computes the
   * ECDH shared secret using |other_pubkey| and the generated private key
   * as inputs. Writes |ATAP_ECDH_SHARED_SECRET_LEN| bytes to
   * |shared_secret|. The raw private key material is not exposed to the
   * caller. On success, returns ATAP_RESULT_OK.
   */
  AtapResult (*ecdh_shared_secret_compute)(
      AtapOps* ops,
      AtapCurveType curve,
      const uint8_t other_pubkey[ATAP_ECDH_KEY_LEN],
      uint8_t pubkey[ATAP_ECDH_KEY_LEN],
      uint8_t shared_secret[ATAP_ECDH_SHARED_SECRET_LEN]);

  /* Encrypts |len| bytes of |plaintext| using |key| and |iv|, and outputs
   * |len| bytes to |ciphertext| and ATAP_GCM_TAG_LEN bytes to |tag|. On
   * success, returns ATAP_RESULT_OK.
   */
  AtapResult (*aes_gcm_128_encrypt)(AtapOps* ops,
                                    const uint8_t* plaintext,
                                    uint32_t len,
                                    const uint8_t iv[ATAP_GCM_IV_LEN],
                                    const uint8_t key[ATAP_AES_128_KEY_LEN],
                                    uint8_t* ciphertext,
                                    uint8_t tag[ATAP_GCM_TAG_LEN]);

  /* Decrypts |len| bytes of |ciphertext| using |key|, |iv|, and |tag|, and
   * outputs |len| bytes to |plaintext|. On success, returns ATAP_RESULT_OK.
   */
  AtapResult (*aes_gcm_128_decrypt)(AtapOps* ops,
                                    const uint8_t* ciphertext,
                                    uint32_t len,
                                    const uint8_t iv[ATAP_GCM_IV_LEN],
                                    const uint8_t key[ATAP_AES_128_KEY_LEN],
                                    const uint8_t tag[ATAP_GCM_TAG_LEN],
                                    uint8_t* plaintext);

  /* Computes a SHA256 hash of the |input|, and outputs
   * ATAP_SHA256_DIGEST_LEN bytes to |HASH|. On success, returns
   * ATAP_RESULT_OK.
   */
  AtapResult (*sha256)(AtapOps* ops,
                       const uint8_t* input,
                       uint32_t input_len,
                       uint8_t hash[ATAP_SHA256_DIGEST_LEN]);

  /* Computes HKDF (as specificed by RFC 5868) of initial keying
   * material |ikm| with |salt| and |info| using SHA256 as the digest,
   * and outputs |okm_len| bytes to |okm|.
   */
  AtapResult (*hkdf_sha256)(AtapOps* ops,
                            const uint8_t* salt,
                            uint32_t salt_len,
                            const uint8_t* ikm,
                            uint32_t ikm_len,
                            const uint8_t* info,
                            uint32_t info_len,
                            uint8_t* okm,
                            uint32_t okm_len);
};

#ifdef __cplusplus
}
#endif

#endif /* ATAP_OPS_H_ */
