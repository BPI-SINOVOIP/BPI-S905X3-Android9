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

#include "atap_unittest_util.h"

#include <base/files/file_util.h>
#include <gtest/gtest.h>
#include <libatap/libatap.h>

/* These tests verify serialization, deserialization, and validation functions
 * in atap_util.c.
 */
namespace atap {

// Subclass BaseAtapToolTest to check for memory leaks.
class UtilTest : public BaseAtapTest {
 public:
  UtilTest() {}
};

namespace {

static void validate_blob(const uint8_t* buf, AtapBlob* blob) {
  EXPECT_EQ(blob->data_length, *(uint32_t*)&buf[0]);
  EXPECT_EQ(0, memcmp(&buf[4], blob->data, blob->data_length));
}

static void alloc_test_blob(AtapBlob* blob) {
  blob->data_length = 20;
  blob->data = (uint8_t*)atap_malloc(20);
  atap_memset(blob->data, 0x77, blob->data_length);
}

static void validate_cert_chain(const uint8_t* buf, AtapCertChain* chain) {
  uint32_t chain_size = cert_chain_serialized_size(chain) - sizeof(uint32_t);
  EXPECT_EQ(*(uint32_t*)&buf[0], chain_size);
  size_t index = 4;
  for (size_t i = 0; i < chain->entry_count; ++i) {
    validate_blob(&buf[index], &chain->entries[i]);
    index += sizeof(uint32_t) + chain->entries[i].data_length;
  }
}

static void alloc_test_cert_chain(AtapCertChain* chain) {
  chain->entry_count = 3;
  for (size_t i = 0; i < chain->entry_count; ++i) {
    alloc_test_blob(&chain->entries[i]);
  }
}

static void validate_header(uint8_t* buf,
                            uint32_t serialized_size,
                            uint32_t* index) {
  uint8_t protocol_version = *next(buf, index, 4);
  EXPECT_EQ(ATAP_PROTOCOL_VERSION, protocol_version);
  uint32_t message_len = *(uint32_t*)next(buf, index, sizeof(uint32_t));
  EXPECT_EQ(serialized_size - ATAP_HEADER_LEN, message_len);
}

}  // unnamed namespace

TEST_F(UtilTest, AppendToBuf) {
  uint64_t x = 0x1122334455667788;
  uint8_t buf[sizeof(uint64_t)];
  uint8_t* res = append_to_buf(buf, &x, sizeof(uint64_t));
  EXPECT_EQ(buf + sizeof(uint64_t), res);
  EXPECT_EQ(*(uint64_t*)buf, x);
}

TEST_F(UtilTest, CopyFromBuf) {
  uint8_t buf[sizeof(uint64_t)];
  uint8_t* buf_ptr = &buf[0];
  uint64_t x = 0x1122334455667788;
  atap_memcpy(buf, &x, sizeof(uint64_t));
  x = 0;
  copy_from_buf(&buf_ptr, &x, sizeof(uint64_t));
  EXPECT_EQ(buf + sizeof(uint64_t), buf_ptr);
  EXPECT_EQ(*(uint64_t*)buf, x);
}

TEST_F(UtilTest, AppendHeaderToBuf) {
  uint8_t buf[ATAP_HEADER_LEN];
  uint32_t message_len = 100;
  uint8_t* res = append_header_to_buf(buf, message_len);
  EXPECT_EQ(buf + ATAP_HEADER_LEN, res);
  EXPECT_EQ(ATAP_PROTOCOL_VERSION, buf[0]);
  EXPECT_EQ(message_len, *(uint32_t*)&buf[4]);
}

TEST_F(UtilTest, SerializeBlob) {
  AtapBlob blob;
  alloc_test_blob(&blob);

  uint8_t buf[32];
  uint8_t* end = append_blob_to_buf(buf, &blob);
  EXPECT_EQ(buf + blob_serialized_size(&blob), end);
  validate_blob(buf, &blob);
  free_blob(blob);

  end = &buf[0];
  copy_blob_from_buf(&end, &blob);
  EXPECT_EQ(buf + blob_serialized_size(&blob), end);
  validate_blob(buf, &blob);
  free_blob(blob);
}

TEST_F(UtilTest, SerializeCertChain) {
  AtapCertChain chain;
  alloc_test_cert_chain(&chain);

  uint8_t buf[128];
  uint8_t* end = append_cert_chain_to_buf(buf, &chain);
  EXPECT_EQ(buf + cert_chain_serialized_size(&chain), end);
  validate_cert_chain(buf, &chain);
  free_cert_chain(chain);

  end = &buf[0];
  EXPECT_TRUE(copy_cert_chain_from_buf(&end, &chain));
  EXPECT_EQ(buf + cert_chain_serialized_size(&chain), end);
  validate_cert_chain(buf, &chain);
  free_cert_chain(chain);

  // malformed serialized certificate chains
  buf[0] -= 7;
  end = &buf[0];
  EXPECT_FALSE(copy_cert_chain_from_buf(&end, &chain));

  buf[0] += 7;
  buf[7] -= 7;
  end = &buf[0];
  EXPECT_FALSE(copy_cert_chain_from_buf(&end, &chain));
}

TEST_F(UtilTest, InnerCaRequestCertifyWithAuth) {
  AtapInnerCaRequest req;
  atap_memset(&req, 0, sizeof(AtapInnerCaRequest));
  alloc_test_cert_chain(&req.auth_key_cert_chain);
  alloc_test_blob(&req.signature);
  atap_memset(req.product_id_hash, 0x66, ATAP_SHA256_DIGEST_LEN);
  alloc_test_blob(&req.RSA_pubkey);
  alloc_test_blob(&req.ECDSA_pubkey);
  alloc_test_blob(&req.edDSA_pubkey);
  uint32_t size = inner_ca_request_serialized_size(&req);

  uint8_t buf[4096];
  uint8_t* end = append_inner_ca_request_to_buf(buf, &req);
  EXPECT_EQ(buf + size, end);
  uint32_t i = 0;
  validate_header(buf, size, &i);
  uint8_t* auth_cert_chain_buf =
      next(buf, &i, cert_chain_serialized_size(&req.auth_key_cert_chain));
  validate_cert_chain(auth_cert_chain_buf, &req.auth_key_cert_chain);
  uint8_t* auth_signature_buf =
      next(buf, &i, blob_serialized_size(&req.signature));
  validate_blob(auth_signature_buf, &req.signature);
  uint8_t* product_id_hash = next(buf, &i, ATAP_SHA256_DIGEST_LEN);
  EXPECT_EQ(
      0, memcmp(req.product_id_hash, product_id_hash, ATAP_SHA256_DIGEST_LEN));
  uint8_t* RSA_pubkey_buf =
      next(buf, &i, blob_serialized_size(&req.RSA_pubkey));
  validate_blob(RSA_pubkey_buf, &req.RSA_pubkey);
  uint8_t* ECDSA_pubkey_buf =
      next(buf, &i, blob_serialized_size(&req.ECDSA_pubkey));
  validate_blob(ECDSA_pubkey_buf, &req.ECDSA_pubkey);
  uint8_t* edDSA_pubkey_buf =
      next(buf, &i, blob_serialized_size(&req.edDSA_pubkey));
  validate_blob(edDSA_pubkey_buf, &req.edDSA_pubkey);
  free_inner_ca_request(req);
}

TEST_F(UtilTest, InnerCaRequestCertifyNoAuth) {
  AtapInnerCaRequest req;
  atap_memset(&req, 0, sizeof(AtapInnerCaRequest));
  atap_memset(req.product_id_hash, 0x66, ATAP_SHA256_DIGEST_LEN);
  alloc_test_blob(&req.RSA_pubkey);
  alloc_test_blob(&req.ECDSA_pubkey);
  alloc_test_blob(&req.edDSA_pubkey);
  uint32_t size = inner_ca_request_serialized_size(&req);

  uint8_t buf[4096];
  uint8_t* end = append_inner_ca_request_to_buf(buf, &req);
  EXPECT_EQ(buf + size, end);
  uint32_t i = 0;
  validate_header(buf, size, &i);
  int32_t auth_cert_chain_len = *(int32_t*)next(buf, &i, sizeof(int32_t));
  EXPECT_EQ(0, auth_cert_chain_len);
  int32_t auth_signature_len = *(int32_t*)next(buf, &i, sizeof(int32_t));
  EXPECT_EQ(0, auth_signature_len);
  uint8_t* product_id_hash = next(buf, &i, ATAP_SHA256_DIGEST_LEN);
  EXPECT_EQ(
      0, memcmp(req.product_id_hash, product_id_hash, ATAP_SHA256_DIGEST_LEN));
  uint8_t* RSA_pubkey_buf =
      next(buf, &i, blob_serialized_size(&req.RSA_pubkey));
  validate_blob(RSA_pubkey_buf, &req.RSA_pubkey);
  uint8_t* ECDSA_pubkey_buf =
      next(buf, &i, blob_serialized_size(&req.ECDSA_pubkey));
  validate_blob(ECDSA_pubkey_buf, &req.ECDSA_pubkey);
  uint8_t* edDSA_pubkey_buf =
      next(buf, &i, blob_serialized_size(&req.edDSA_pubkey));
  validate_blob(edDSA_pubkey_buf, &req.edDSA_pubkey);
  free_inner_ca_request(req);
}

TEST_F(UtilTest, InnerCaRequestIssueWithAuth) {
  AtapInnerCaRequest req;
  atap_memset(&req, 0, sizeof(AtapInnerCaRequest));
  alloc_test_cert_chain(&req.auth_key_cert_chain);
  alloc_test_blob(&req.signature);
  atap_memset(req.product_id_hash, 0x66, ATAP_SHA256_DIGEST_LEN);
  uint32_t size = inner_ca_request_serialized_size(&req);

  uint8_t buf[4096];
  uint8_t* end = append_inner_ca_request_to_buf(buf, &req);
  EXPECT_EQ(buf + size, end);
  uint32_t i = 0;
  validate_header(buf, size, &i);
  uint8_t* auth_cert_chain_buf =
      next(buf, &i, cert_chain_serialized_size(&req.auth_key_cert_chain));
  validate_cert_chain(auth_cert_chain_buf, &req.auth_key_cert_chain);
  uint8_t* auth_signature_buf =
      next(buf, &i, blob_serialized_size(&req.signature));
  validate_blob(auth_signature_buf, &req.signature);
  uint8_t* product_id_hash = next(buf, &i, ATAP_SHA256_DIGEST_LEN);
  EXPECT_EQ(
      0, memcmp(req.product_id_hash, product_id_hash, ATAP_SHA256_DIGEST_LEN));
  int32_t RSA_pubkey_size = *(int32_t*)next(buf, &i, sizeof(int32_t));
  EXPECT_EQ(0, RSA_pubkey_size);
  int32_t ECDSA_pubkey_size = *(int32_t*)next(buf, &i, sizeof(int32_t));
  EXPECT_EQ(0, ECDSA_pubkey_size);
  int32_t edDSA_pubkey_size = *(int32_t*)next(buf, &i, sizeof(int32_t));
  EXPECT_EQ(0, edDSA_pubkey_size);
  free_inner_ca_request(req);
}

TEST_F(UtilTest, InnerCaRequestIssueNoAuth) {
  AtapInnerCaRequest req;
  atap_memset(&req, 0, sizeof(AtapInnerCaRequest));
  atap_memset(req.product_id_hash, 0x66, ATAP_SHA256_DIGEST_LEN);
  uint32_t size = inner_ca_request_serialized_size(&req);

  uint8_t buf[4096];
  uint8_t* end = append_inner_ca_request_to_buf(buf, &req);
  EXPECT_EQ(buf + size, end);
  uint32_t i = 0;
  validate_header(buf, size, &i);
  int32_t auth_cert_chain_len = *(int32_t*)next(buf, &i, sizeof(int32_t));
  EXPECT_EQ(0, auth_cert_chain_len);
  int32_t auth_signature_len = *(int32_t*)next(buf, &i, sizeof(int32_t));
  EXPECT_EQ(0, auth_signature_len);
  uint8_t* product_id_hash = next(buf, &i, ATAP_SHA256_DIGEST_LEN);
  EXPECT_EQ(
      0, memcmp(req.product_id_hash, product_id_hash, ATAP_SHA256_DIGEST_LEN));
  int32_t RSA_pubkey_size = *(int32_t*)next(buf, &i, sizeof(int32_t));
  EXPECT_EQ(0, RSA_pubkey_size);
  int32_t ECDSA_pubkey_size = *(int32_t*)next(buf, &i, sizeof(int32_t));
  EXPECT_EQ(0, ECDSA_pubkey_size);
  int32_t edDSA_pubkey_size = *(int32_t*)next(buf, &i, sizeof(int32_t));
  EXPECT_EQ(0, edDSA_pubkey_size);
  free_inner_ca_request(req);
}

TEST_F(UtilTest, CaRequest) {
  AtapCaRequest req;
  atap_memset(&req, 0, sizeof(AtapCaRequest));
  atap_memset(req.device_pubkey, 0x66, ATAP_ECDH_KEY_LEN);
  atap_memset(req.iv, 0x55, ATAP_GCM_IV_LEN);
  alloc_test_blob(&req.encrypted_inner_ca_request);
  atap_memset(req.tag, 0x44, ATAP_GCM_TAG_LEN);
  uint32_t size = ca_request_serialized_size(&req);

  uint8_t buf[4096];
  uint8_t* end = append_ca_request_to_buf(buf, &req);
  EXPECT_EQ(buf + size, end);
  uint32_t i = 0;
  validate_header(buf, size, &i);
  uint8_t* device_pubkey = next(buf, &i, ATAP_ECDH_KEY_LEN);
  EXPECT_EQ(0, memcmp(req.device_pubkey, device_pubkey, ATAP_ECDH_KEY_LEN));
  uint8_t* iv = next(buf, &i, ATAP_GCM_IV_LEN);
  EXPECT_EQ(0, memcmp(req.iv, iv, ATAP_GCM_IV_LEN));
  uint8_t* encrypted_inner_buf =
      next(buf, &i, blob_serialized_size(&req.encrypted_inner_ca_request));
  validate_blob(encrypted_inner_buf, &req.encrypted_inner_ca_request);
  uint8_t* tag = next(buf, &i, ATAP_GCM_TAG_LEN);
  EXPECT_EQ(0, memcmp(req.tag, tag, ATAP_GCM_TAG_LEN));
  free_ca_request(req);
}

TEST_F(UtilTest, ValidateEncryptedMessage) {
  uint8_t buf[128];
  uint32_t message_len = 128 - ATAP_HEADER_LEN;
  uint32_t encrypted_len =
      message_len - ATAP_GCM_IV_LEN - sizeof(uint32_t) - ATAP_GCM_TAG_LEN;

  append_header_to_buf(buf, message_len);
  *(uint32_t*)&buf[ATAP_HEADER_LEN + ATAP_GCM_IV_LEN] = encrypted_len;
  EXPECT_EQ(true, validate_encrypted_message(buf, 128));
  EXPECT_EQ(false, validate_encrypted_message(buf, 8));
  *(uint32_t*)&buf[ATAP_HEADER_LEN + ATAP_GCM_IV_LEN] = 4;
  EXPECT_EQ(false, validate_encrypted_message(buf, 128));
}

TEST_F(UtilTest, ValidateInnerCaResponse) {
  std::string inner_ca_resp;

  ASSERT_TRUE(base::ReadFileToString(
      base::FilePath(kIssueX25519InnerCaResponsePath), &inner_ca_resp));
  ASSERT_EQ(6954, (int)inner_ca_resp.size());
  EXPECT_EQ(true,
            validate_inner_ca_response((uint8_t*)&inner_ca_resp[0],
                                       inner_ca_resp.size(),
                                       ATAP_OPERATION_ISSUE));
  // Invalid InnerCaResponse message format
  *(uint32_t*)&inner_ca_resp[4] = 100;
  EXPECT_EQ(false,
            validate_inner_ca_response((uint8_t*)&inner_ca_resp[0],
                                       inner_ca_resp.size(),
                                       ATAP_OPERATION_ISSUE));
}

}  // namespace atap
