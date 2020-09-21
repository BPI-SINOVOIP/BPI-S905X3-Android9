/*
 * Copyright 2016 The Android Open Source Project
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

#include <fstream>

#include <gtest/gtest.h>

#include <keymaster/keymaster_context.h>

#include "android_keymaster_test_utils.h"
#include <keymaster/attestation_record.h>

namespace keymaster {
namespace test {

class TestContext : public AttestationRecordContext {
  public:
    keymaster_security_level_t GetSecurityLevel() const override {
        return KM_SECURITY_LEVEL_SOFTWARE;
    }
    keymaster_error_t GenerateUniqueId(uint64_t /* creation_date_time */,
                                       const keymaster_blob_t& /* application_id */,
                                       bool /* reset_since_rotation */, Buffer* unique_id) const override {
        // Finally, the reason for defining this class:
        unique_id->Reinitialize("foo", 3);
        return KM_ERROR_OK;
    }
};

TEST(AttestTest, Simple) {
    AuthorizationSet hw_set(AuthorizationSetBuilder()
                                .RsaSigningKey(512, 3)
                                .Digest(KM_DIGEST_SHA_2_256)
                                .Digest(KM_DIGEST_SHA_2_384)
                                .Authorization(TAG_OS_VERSION, 60000)
                                .Authorization(TAG_OS_PATCHLEVEL, 201512)
                                .Authorization(TAG_APPLICATION_ID, "bar", 3));
    AuthorizationSet sw_set(AuthorizationSetBuilder().Authorization(TAG_ACTIVE_DATETIME, 10));

    UniquePtr<uint8_t[]> asn1;
    size_t asn1_len;
    AuthorizationSet attest_params(
        AuthorizationSetBuilder()
            .Authorization(TAG_ATTESTATION_CHALLENGE, "hello", 5)
            .Authorization(TAG_ATTESTATION_APPLICATION_ID, "hello again", 11));
    EXPECT_EQ(KM_ERROR_OK, build_attestation_record(attest_params, sw_set, hw_set, TestContext(),
                                                    &asn1, &asn1_len));
    EXPECT_GT(asn1_len, 0U);

    std::ofstream output("attest.der",
                         std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
    if (output)
        output.write(reinterpret_cast<const char*>(asn1.get()), asn1_len);
    output.close();

    AuthorizationSet parsed_hw_set;
    AuthorizationSet parsed_sw_set;
    uint32_t attestation_version;
    uint32_t keymaster_version;
    keymaster_security_level_t attestation_security_level;
    keymaster_security_level_t keymaster_security_level;
    keymaster_blob_t attestation_challenge = {};
    keymaster_blob_t unique_id = {};
    EXPECT_EQ(KM_ERROR_OK,
              parse_attestation_record(asn1.get(), asn1_len, &attestation_version,
                                       &attestation_security_level, &keymaster_version,
                                       &keymaster_security_level, &attestation_challenge,
                                       &parsed_sw_set, &parsed_hw_set, &unique_id));

    delete[] attestation_challenge.data;
    delete[] unique_id.data;

    hw_set.Sort();
    sw_set.Sort();
    parsed_hw_set.Sort();
    parsed_sw_set.Sort();
    EXPECT_EQ(hw_set, parsed_hw_set);
    EXPECT_EQ(sw_set, parsed_sw_set);
}

}  // namespace test
}  // namespace keymaster
