/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <../proto_utils.h>

#include <nugget/app/keymaster/keymaster.pb.h>

#include <MockKeymaster.client.h>
#include <KeymasterDevice.h>

#include <keymasterV4_0/keymaster_tags.h>

#include <android-base/logging.h>
#include <gtest/gtest.h>

#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/pkcs8.h>

#include <string>

using ::std::string;
using ::std::unique_ptr;

using ::testing::_;
using ::testing::DoAll;
using ::testing::Eq;
using ::testing::Matcher;
using ::testing::MatcherInterface;
using ::testing::MatchResultListener;
using ::testing::Return;
using ::testing::SetArgPointee;

// Hardware
using ::android::hardware::hidl_vec;
using ::android::hardware::keymaster::KeymasterDevice;

// HAL
using ::android::hardware::keymaster::V4_0::Algorithm;
using ::android::hardware::keymaster::V4_0::EcCurve;
using ::android::hardware::keymaster::V4_0::ErrorCode;
using ::android::hardware::keymaster::V4_0::KeyFormat;
using ::android::hardware::keymaster::V4_0::KeyParameter;
using ::android::hardware::keymaster::V4_0::KeyCharacteristics;
using ::android::hardware::keymaster::V4_0::Tag;
using android::hardware::keymaster::pb_to_hidl_params;

// App
//using namespace ::nugget::app::keymaster;
namespace nosapp = ::nugget::app::keymaster;
using ::nugget::app::keymaster::MockKeymaster;
using ::nugget::app::keymaster::ImportWrappedKeyRequest;
using ::nugget::app::keymaster::ImportKeyResponse;

// ImportWrappedKey
static const hidl_vec<uint8_t> WRAPPED_KEY_DER{
  0x30, 0x82, 0x01, 0x5f, 0x02, 0x01, 0x00, 0x04, 0x82, 0x01, 0x00, 0x99,
  0xb0, 0xad, 0xd4, 0xe4, 0x0c, 0x82, 0x37, 0x33, 0x0c, 0x12, 0xe1, 0x2a,
  0x5c, 0x22, 0x5b, 0xdc, 0xb4, 0x36, 0xae, 0xb5, 0xb1, 0x90, 0xbb, 0xc7,
  0x09, 0x13, 0xe4, 0x12, 0xf2, 0x5b, 0x20, 0x3e, 0xe5, 0xd2, 0x6d, 0x69,
  0x25, 0xa8, 0x3e, 0x59, 0x43, 0x31, 0x3a, 0x29, 0x06, 0x97, 0xae, 0x0f,
  0x30, 0x38, 0x18, 0x6e, 0x3a, 0xac, 0x6a, 0xb7, 0xa4, 0x36, 0x87, 0xc4,
  0xde, 0xdb, 0xa3, 0x46, 0x78, 0x64, 0xd7, 0x2b, 0x51, 0x51, 0x34, 0x36,
  0x98, 0x66, 0x72, 0xd2, 0x48, 0x98, 0x61, 0x67, 0x87, 0xcf, 0x29, 0xba,
  0x9b, 0xf7, 0xcd, 0x14, 0x33, 0xd2, 0x67, 0x9b, 0x9c, 0x55, 0x3b, 0xf0,
  0x72, 0x13, 0x75, 0xbc, 0x55, 0x95, 0xd9, 0x0d, 0xb3, 0xe5, 0x6b, 0x88,
  0x4a, 0xae, 0xe5, 0xc0, 0xf2, 0x17, 0x01, 0x92, 0xfb, 0x68, 0x08, 0x8e,
  0x91, 0x96, 0x5f, 0x2f, 0x19, 0x63, 0xeb, 0x95, 0xb2, 0xd2, 0x89, 0x5b,
  0xb5, 0x96, 0xa6, 0x6f, 0x50, 0x63, 0x6d, 0x05, 0x9f, 0x06, 0x29, 0x81,
  0xc2, 0x85, 0x3a, 0xd0, 0x63, 0x78, 0xc8, 0x78, 0x95, 0xde, 0x49, 0xa1,
  0xb7, 0xdd, 0xde, 0xaf, 0x6a, 0xa2, 0xf6, 0xb5, 0xe2, 0x51, 0x21, 0xad,
  0x5e, 0x81, 0xa3, 0x2c, 0xf4, 0xb5, 0x5d, 0x1f, 0x7e, 0x45, 0xe8, 0xdc,
  0x7c, 0xab, 0x3b, 0xaa, 0x49, 0xee, 0xa9, 0xd5, 0x9d, 0xe1, 0x78, 0x39,
  0xe9, 0xb4, 0x91, 0xf7, 0x2e, 0xbf, 0xc5, 0xbc, 0xb5, 0x26, 0x48, 0x05,
  0x9f, 0x49, 0x31, 0xa7, 0xa2, 0x56, 0xea, 0x79, 0x61, 0x28, 0x23, 0x67,
  0x8e, 0x12, 0xbd, 0x4b, 0xe7, 0xbd, 0x8f, 0x10, 0x45, 0xbc, 0x3c, 0xd0,
  0x4b, 0xa9, 0x28, 0xd2, 0xf3, 0x59, 0xfb, 0x10, 0x08, 0xd0, 0x91, 0x74,
  0xd8, 0xd1, 0x89, 0x6c, 0xda, 0xc7, 0x6e, 0x4f, 0x44, 0x09, 0x89, 0x4f,
  0x2d, 0x7c, 0xa7, 0x04, 0x0c, 0xd7, 0x96, 0xb0, 0x2c, 0x37, 0x0f, 0x1f,
  0xa4, 0xcc, 0x01, 0x24, 0xf1, 0x30, 0x14, 0x02, 0x01, 0x03, 0x30, 0x0f,
  0xa1, 0x02, 0x31, 0x00, 0xa2, 0x03, 0x02, 0x01, 0x20, 0xa3, 0x04, 0x02,
  0x02, 0x01, 0x00, 0x04, 0x20, 0xcc, 0xd5, 0x40, 0x85, 0x5f, 0x83, 0x3a,
  0x5e, 0x14, 0x80, 0xbf, 0xd2, 0xd3, 0x6f, 0xaf, 0x3a, 0xee, 0xe1, 0x5d,
  0xf5, 0xbe, 0xab, 0xe2, 0x69, 0x1b, 0xc8, 0x2d, 0xde, 0x2a, 0x7a, 0xa9,
  0x10, 0x04, 0x10, 0x0a, 0xa4, 0x6a, 0x14, 0xa0, 0x24, 0x90, 0xea, 0xf5,
  0xef, 0x32, 0x86, 0x2e, 0x4c, 0x03, 0x4e
};

static const hidl_vec<uint8_t> RSA_ENVELOPE{
  0x99, 0xb0, 0xad, 0xd4, 0xe4, 0x0c, 0x82, 0x37, 0x33, 0x0c, 0x12, 0xe1,
  0x2a, 0x5c, 0x22, 0x5b, 0xdc, 0xb4, 0x36, 0xae, 0xb5, 0xb1, 0x90, 0xbb,
  0xc7, 0x09, 0x13, 0xe4, 0x12, 0xf2, 0x5b, 0x20, 0x3e, 0xe5, 0xd2, 0x6d,
  0x69, 0x25, 0xa8, 0x3e, 0x59, 0x43, 0x31, 0x3a, 0x29, 0x06, 0x97, 0xae,
  0x0f, 0x30, 0x38, 0x18, 0x6e, 0x3a, 0xac, 0x6a, 0xb7, 0xa4, 0x36, 0x87,
  0xc4, 0xde, 0xdb, 0xa3, 0x46, 0x78, 0x64, 0xd7, 0x2b, 0x51, 0x51, 0x34,
  0x36, 0x98, 0x66, 0x72, 0xd2, 0x48, 0x98, 0x61, 0x67, 0x87, 0xcf, 0x29,
  0xba, 0x9b, 0xf7, 0xcd, 0x14, 0x33, 0xd2, 0x67, 0x9b, 0x9c, 0x55, 0x3b,
  0xf0, 0x72, 0x13, 0x75, 0xbc, 0x55, 0x95, 0xd9, 0x0d, 0xb3, 0xe5, 0x6b,
  0x88, 0x4a, 0xae, 0xe5, 0xc0, 0xf2, 0x17, 0x01, 0x92, 0xfb, 0x68, 0x08,
  0x8e, 0x91, 0x96, 0x5f, 0x2f, 0x19, 0x63, 0xeb, 0x95, 0xb2, 0xd2, 0x89,
  0x5b, 0xb5, 0x96, 0xa6, 0x6f, 0x50, 0x63, 0x6d, 0x05, 0x9f, 0x06, 0x29,
  0x81, 0xc2, 0x85, 0x3a, 0xd0, 0x63, 0x78, 0xc8, 0x78, 0x95, 0xde, 0x49,
  0xa1, 0xb7, 0xdd, 0xde, 0xaf, 0x6a, 0xa2, 0xf6, 0xb5, 0xe2, 0x51, 0x21,
  0xad, 0x5e, 0x81, 0xa3, 0x2c, 0xf4, 0xb5, 0x5d, 0x1f, 0x7e, 0x45, 0xe8,
  0xdc, 0x7c, 0xab, 0x3b, 0xaa, 0x49, 0xee, 0xa9, 0xd5, 0x9d, 0xe1, 0x78,
  0x39, 0xe9, 0xb4, 0x91, 0xf7, 0x2e, 0xbf, 0xc5, 0xbc, 0xb5, 0x26, 0x48,
  0x05, 0x9f, 0x49, 0x31, 0xa7, 0xa2, 0x56, 0xea, 0x79, 0x61, 0x28, 0x23,
  0x67, 0x8e, 0x12, 0xbd, 0x4b, 0xe7, 0xbd, 0x8f, 0x10, 0x45, 0xbc, 0x3c,
  0xd0, 0x4b, 0xa9, 0x28, 0xd2, 0xf3, 0x59, 0xfb, 0x10, 0x08, 0xd0, 0x91,
  0x74, 0xd8, 0xd1, 0x89, 0x6c, 0xda, 0xc7, 0x6e, 0x4f, 0x44, 0x09, 0x89,
  0x4f, 0x2d, 0x7c, 0xa7
};

static const hidl_vec<uint8_t> INITIALIZATION_VECTOR{
  0xd7, 0x96, 0xb0, 0x2c, 0x37, 0x0f, 0x1f, 0xa4, 0xcc, 0x01, 0x24, 0xf1
};

static const hidl_vec<uint8_t> ENCRYPTED_IMPORT_KEY{
  0xcc, 0xd5, 0x40, 0x85, 0x5f, 0x83, 0x3a, 0x5e, 0x14, 0x80, 0xbf, 0xd2,
  0xd3, 0x6f, 0xaf, 0x3a, 0xee, 0xe1, 0x5d, 0xf5, 0xbe, 0xab, 0xe2, 0x69,
  0x1b, 0xc8, 0x2d, 0xde, 0x2a, 0x7a, 0xa9, 0x10
};

static const hidl_vec<uint8_t> AAD{
  0x30, 0x14, 0x02, 0x01, 0x03, 0x30, 0x0f, 0xa1, 0x02, 0x31, 0x00, 0xa2,
  0x03, 0x02, 0x01, 0x20, 0xa3, 0x04, 0x02, 0x02, 0x01, 0x00
};

static const hidl_vec<uint8_t> GCM_TAG{
    0x0a, 0xa4, 0x6a, 0x14, 0xa0, 0x24, 0x90, 0xea, 0xf5, 0xef, 0x32, 0x86,
    0x2e, 0x4c, 0x03, 0x4e
};

class ImportWrappedKeyRequestMatcher
    : public MatcherInterface<const ImportWrappedKeyRequest&> {
  public:
    explicit ImportWrappedKeyRequestMatcher(
    KeyFormat key_format,
    const hidl_vec<KeyParameter>& params,
    const hidl_vec<uint8_t>& rsa_envelope,
    const hidl_vec<uint8_t>& initialization_vector,
    const hidl_vec<uint8_t>& encrypted_import_key,
    const hidl_vec<uint8_t>& aad,
    const hidl_vec<uint8_t>& gcm_tag,
    const hidl_vec<uint8_t>& wrapping_key_blob,
    const hidl_vec<uint8_t>& masking_key)
        : key_format_(key_format),
          params_(params),
          rsa_envelope_(rsa_envelope),
          initialization_vector_(initialization_vector),
          encrypted_import_key_(encrypted_import_key),
          aad_(aad),
          gcm_tag_(gcm_tag),
          wrapping_key_blob_(wrapping_key_blob),
          masking_key_(masking_key) {
    }

    virtual void DescribeTo(::std::ostream* os) const {
        *os << "ImportWrappedKeyRequest matched expectation";
    }

    virtual void DescribeNegationTo(::std::ostream* os) const {
        *os << "ImportWrappedKeyRequest mis-matched expectation";
    }

    virtual bool MatchAndExplain(const ImportWrappedKeyRequest& request,
                                 MatchResultListener* listener) const {
        if ((uint32_t)key_format_ != request.key_format()) {
            *listener << "Expected key_format: " << (uint32_t)key_format_
                      << " got: " << request.key_format();
            return false;
        }

        (void)params_;

        if (!MatchByteArray(
                "RSA envelope",
                listener,
                rsa_envelope_.data(),
                rsa_envelope_.size(),
                (const uint8_t *)request.rsa_envelope().data(),
                request.rsa_envelope().size())) {
            return false;
        }

        if (!MatchByteArray(
                "Initialization vector",
                listener,
                initialization_vector_.data(),
                initialization_vector_.size(),
                (const uint8_t *)request.initialization_vector().data(),
                request.initialization_vector().size())) {
            return false;
        }

        if (!MatchByteArray(
                "Encrypted import key",
                listener,
                encrypted_import_key_.data(),
                encrypted_import_key_.size(),
                (const uint8_t *)request.encrypted_import_key().data(),
                request.encrypted_import_key().size())) {
            return false;
        }

        if (!MatchByteArray(
                "AAD",
                listener,
                aad_.data(),
                aad_.size(),
                (const uint8_t *)request.aad().data(),
                request.aad().size())) {
            return false;
        }

        if (!MatchByteArray(
                "GCM TAG",
                listener,
                gcm_tag_.data(),
                gcm_tag_.size(),
                (const uint8_t *)request.gcm_tag().data(),
                request.gcm_tag().size())) {
            return false;
        }

        if (!MatchByteArray(
                "Wrapping key blob",
                listener,
                wrapping_key_blob_.data(),
                wrapping_key_blob_.size(),
                (const uint8_t *)request.wrapping_key_blob().blob().data(),
                request.wrapping_key_blob().blob().size())) {
            return false;
        }

        if (!MatchByteArray(
                "Masking key",
                listener,
                masking_key_.data(),
                masking_key_.size(),
                (const uint8_t *)request.masking_key().data(),
                request.masking_key().size())) {
            return false;
        }

        return true;
    }

  private:

    bool MatchByteArray(const char *label, MatchResultListener *listener,
                        const uint8_t *expected, size_t expected_len,
                        const uint8_t *got, size_t got_len) const {
        if (expected_len != got_len) {
            *listener << label << ": expected len: " << expected_len
                      << " got len: " << got_len;
            return false;
        }

        for (size_t i = 0; i < expected_len; i++) {
            if (expected[i] != got[i]) {
                *listener << label << ": mismatch at index: " << i
                          << " expected: " << std::hex << (int)expected[i]
                          << " got: " << std::hex << (int)got[i];
                return false;
            }
        }

        return true;
    }

    KeyFormat key_format_;
    const hidl_vec<KeyParameter>& params_;
    const hidl_vec<uint8_t>& rsa_envelope_;
    const hidl_vec<uint8_t>& initialization_vector_;
    const hidl_vec<uint8_t>& encrypted_import_key_;
    const hidl_vec<uint8_t>& aad_;
    const hidl_vec<uint8_t>& gcm_tag_;
    const hidl_vec<uint8_t>& wrapping_key_blob_;
    const hidl_vec<uint8_t>& masking_key_;
};

static Matcher<const ImportWrappedKeyRequest&> ImportWrappedKeyRequestEq(
    KeyFormat key_format,
    const hidl_vec<KeyParameter>& params,
    const hidl_vec<uint8_t>& rsa_envelope,
    const hidl_vec<uint8_t>& initialization_vector,
    const hidl_vec<uint8_t>& encrypted_import_key,
    const hidl_vec<uint8_t>& aad,
    const hidl_vec<uint8_t>& gcm_tag,
    const hidl_vec<uint8_t>& wrapping_key_blob,
    const hidl_vec<uint8_t>& masking_key) {
    return MakeMatcher(new ImportWrappedKeyRequestMatcher(
                           key_format, params,
                           rsa_envelope, initialization_vector,
                           encrypted_import_key, aad, gcm_tag,
                           wrapping_key_blob, masking_key));
}

TEST(KeymasterHalTest, importWrappedKeyRAWSuccess) {
    MockKeymaster mockService;
    ImportKeyResponse response;

    // request
    KeyParameter kp1;
    kp1.tag = Tag::ALGORITHM;
    kp1.f.algorithm = Algorithm::AES;
    KeyParameter kp2;
    kp2.tag = Tag::KEY_SIZE;
    kp2.f.integer = 256;

    uint8_t masking_key_data[32] = {
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    };
    hidl_vec<uint8_t> masking_key;
    masking_key.setToExternal(
        masking_key_data, sizeof(masking_key_data), false);

    uint8_t wrapping_key_blob_data[32] = {
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    };
    hidl_vec<uint8_t> wrapping_key_blob;
    wrapping_key_blob.setToExternal(
        wrapping_key_blob_data, sizeof(wrapping_key_blob_data), false);

    // response
    string response_key_blob("Dummy-rkb");
    response.mutable_blob()->set_blob(response_key_blob);
    nosapp::KeyParameter *noskp =
        response.mutable_characteristics()->
        mutable_tee_enforced()->add_params();
    noskp->set_tag(nosapp::Tag::ALGORITHM);
    noskp->set_integer((uint32_t)nosapp::Algorithm::AES);

    noskp = response.mutable_characteristics()->
        mutable_tee_enforced()->add_params();
    noskp->set_tag(nosapp::Tag::KEY_SIZE);
    noskp->set_integer(256);

    EXPECT_CALL(
        mockService,
        ImportWrappedKey(ImportWrappedKeyRequestEq(
                             KeyFormat::RAW,
                             hidl_vec<KeyParameter>{kp1, kp2},
                             RSA_ENVELOPE,
                             INITIALIZATION_VECTOR,
                             ENCRYPTED_IMPORT_KEY,
                             AAD,
                             GCM_TAG,
                             wrapping_key_blob,
                             masking_key), _))
        .WillOnce(DoAll(SetArgPointee<1>(response), Return(APP_SUCCESS)));

    KeymasterDevice hal{mockService};
    /*
     * TODO: extend test cases & matchers to include unwrappingParams,
     * passwordSid, biometricSid.
     */
    hal.importWrappedKey(
        WRAPPED_KEY_DER,
        wrapping_key_blob,
        masking_key,
        {}, /* unwrappingParams */
        0,  /* passwordSid */
        0,  /* biometricSid */
        [&](ErrorCode error, hidl_vec<uint8_t> blob,
          KeyCharacteristics characteristics) {

          EXPECT_EQ(error, ErrorCode::OK);

          EXPECT_THAT(blob, Eq(hidl_vec<uint8_t>{'D', 'u', 'm', 'm', 'y',
                            '-', 'r', 'k', 'b'}));;

          EXPECT_EQ(characteristics.hardwareEnforced.size(), 2u);
          EXPECT_EQ(characteristics.hardwareEnforced[0].tag, Tag::ALGORITHM);
          EXPECT_EQ(characteristics.hardwareEnforced[0].f.algorithm,
                    Algorithm::AES);
          EXPECT_EQ(characteristics.hardwareEnforced[1].tag, Tag::KEY_SIZE);
          EXPECT_EQ(characteristics.hardwareEnforced[1].f.integer,
                    256u);
    });
}
