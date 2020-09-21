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

#ifndef ATAP_UNITTEST_UTIL_H_
#define ATAP_UNITTEST_UTIL_H_

#include <gtest/gtest.h>

namespace atap {

// These two functions are in atap_sysdeps_posix_testing.cc and is
// used for finding memory leaks.
void testing_memory_reset();
size_t testing_memory_all_freed();

// Base-class used for unit test.
class BaseAtapTest : public ::testing::Test {
 public:
  BaseAtapTest() {}

 protected:
  virtual ~BaseAtapTest() {}

  virtual void SetUp() override {
    // Reset memory leak tracing
    atap::testing_memory_reset();
  }

  virtual void TearDown() override {
    // Ensure all memory has been freed.
    EXPECT_TRUE(atap::testing_memory_all_freed());
  }
};

const char kCaP256PrivateKey[] = "test/data/ca_p256_private.bin";
const char kCaX25519PrivateKey[] = "test/data/ca_x25519_private.bin";
const char kCaX25519PublicKey[] = "test/data/ca_x25519_public.bin";
const char kIssueP256OperationStartPath[] =
    "test/data/issue_p256_operation_start.bin";
const char kIssueX25519InnerCaResponsePath[] =
    "test/data/issue_x25519_inner_ca_response.bin";
const char kIssueX25519OperationStartPath[] =
    "test/data/issue_x25519_operation_start.bin";
const char kProductIdHash[] = "test/data/product_id_hash.bin";

// Returns |buf| + |*index|, then increments |*index| by |value|. Used for
// validating the contents of a serialized structure.
inline uint8_t* next(const uint8_t* buf, uint32_t* index, uint32_t value) {
  uint8_t* retval = (uint8_t*)&buf[*index];
  *index += value;
  return retval;
}

}  // namespace atap

#endif /* ATAP_UNITTEST_UTIL_H_ */
