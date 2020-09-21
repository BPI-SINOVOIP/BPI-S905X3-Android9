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
using ::nugget::app::keymaster::ImportKeyRequest;
using ::nugget::app::keymaster::ImportKeyResponse;

// ImportKey

static uint8_t TEST_RSA1024_KEY[] = {
    0x30, 0x82, 0x02, 0x5c, 0x02, 0x01, 0x00, 0x02, 0x81, 0x81, 0x00, 0xba,
    0x9f, 0x5e, 0xf0, 0x58, 0xda, 0x49, 0xbe, 0x6b, 0x3c, 0xef, 0x17, 0x6f,
    0x2f, 0x4f, 0x81, 0x18, 0x21, 0x4f, 0xbd, 0x6c, 0x17, 0xaa, 0x74, 0x99,
    0xcc, 0x55, 0x94, 0x0f, 0xee, 0xa4, 0x0e, 0xae, 0x68, 0x4c, 0xaf, 0x11,
    0x86, 0xdd, 0x50, 0xbf, 0x5c, 0x0f, 0x22, 0xad, 0xfa, 0xd1, 0x42, 0xab,
    0xb8, 0x01, 0x7c, 0x2f, 0x54, 0xc7, 0xd9, 0xac, 0x47, 0xe3, 0xe1, 0x6b,
    0x51, 0x48, 0xb6, 0xbb, 0x33, 0xab, 0xba, 0x9f, 0x86, 0x1b, 0xdf, 0xea,
    0x9f, 0x2d, 0xb6, 0x40, 0xe3, 0x22, 0xbe, 0x90, 0xd6, 0x0d, 0xca, 0x2e,
    0xb7, 0x8d, 0x8f, 0xf9, 0xd7, 0xeb, 0xa5, 0x74, 0x69, 0xc0, 0x87, 0x9b,
    0x73, 0x17, 0x65, 0xde, 0x90, 0xb9, 0x5a, 0xde, 0xe0, 0xa4, 0xf3, 0xde,
    0xeb, 0x9d, 0x3d, 0xb3, 0x12, 0x84, 0xf0, 0xdb, 0x43, 0x82, 0x0a, 0x8c,
    0x64, 0x48, 0x4e, 0x9d, 0xd8, 0x13, 0x6d, 0x02, 0x03, 0x01, 0x00, 0x01,
    0x02, 0x81, 0x80, 0x48, 0x20, 0xc7, 0x8f, 0x4a, 0x30, 0x17, 0xf7, 0x5e,
    0x38, 0x1f, 0x4a, 0x65, 0xe1, 0x19, 0xaf, 0xd1, 0xd5, 0x32, 0x1e, 0x0a,
    0x74, 0x7d, 0x1f, 0x72, 0xbf, 0xdc, 0x45, 0x8d, 0x00, 0xd5, 0x6c, 0x8d,
    0x30, 0xe7, 0x8e, 0x74, 0x4e, 0x35, 0x24, 0x7b, 0xc9, 0x47, 0x5a, 0x46,
    0x76, 0xdd, 0xc1, 0x10, 0x60, 0x5e, 0x46, 0x92, 0x7e, 0x88, 0x7d, 0x53,
    0x4e, 0x37, 0xbf, 0x8c, 0x7c, 0x4e, 0x87, 0x12, 0x97, 0x5a, 0xbc, 0x4a,
    0x13, 0x56, 0xf4, 0x6f, 0xd7, 0x85, 0xd8, 0xe9, 0xed, 0x86, 0x6e, 0xc0,
    0x7b, 0xe5, 0x9e, 0x0e, 0x2a, 0xd2, 0x55, 0x60, 0x5c, 0x79, 0x36, 0xe0,
    0x09, 0x46, 0xf2, 0xf3, 0x61, 0x87, 0xc5, 0x53, 0xb0, 0x83, 0x6c, 0x9b,
    0x16, 0x37, 0x6e, 0x23, 0xb7, 0xfb, 0x37, 0xef, 0x53, 0xd8, 0x2b, 0xca,
    0x2d, 0x6e, 0x53, 0x91, 0xb4, 0x79, 0xf0, 0x1c, 0x5b, 0xa8, 0x5b, 0x02,
    0x41, 0x00, 0xfe, 0x93, 0x73, 0x43, 0xab, 0xd9, 0x38, 0xbe, 0x6e, 0x54,
    0x38, 0x2b, 0x01, 0x45, 0x77, 0x18, 0xa6, 0x40, 0x84, 0xa0, 0x7c, 0x74,
    0x76, 0x6d, 0x9e, 0x1d, 0xa0, 0x36, 0xc6, 0xb2, 0x3d, 0x6f, 0xd4, 0xa8,
    0xb0, 0xac, 0xfc, 0x8e, 0x3d, 0xfd, 0x81, 0xea, 0x52, 0x0f, 0x1b, 0x08,
    0x3f, 0xe5, 0x32, 0x9e, 0xa2, 0x77, 0x7d, 0x3b, 0x7f, 0x41, 0x24, 0x74,
    0xea, 0xce, 0xd2, 0x80, 0xb8, 0x77, 0x02, 0x41, 0x00, 0xbb, 0xaa, 0x9c,
    0xb2, 0x83, 0x5a, 0x29, 0xae, 0xfb, 0x90, 0x20, 0xcd, 0x39, 0x0d, 0x5a,
    0x1a, 0x8c, 0xb0, 0xc5, 0xc7, 0x62, 0x7a, 0xf4, 0xa5, 0xd8, 0x80, 0xac,
    0xd1, 0x5f, 0xb9, 0xea, 0xea, 0xeb, 0x53, 0xae, 0x27, 0x45, 0x07, 0x0d,
    0xd7, 0x5b, 0x68, 0x55, 0x53, 0x64, 0x83, 0xa0, 0xf0, 0xd7, 0x4a, 0xdf,
    0x0e, 0x7f, 0xe6, 0xe7, 0xf2, 0xba, 0x30, 0x4e, 0x13, 0x4a, 0x32, 0xf0,
    0x3b, 0x02, 0x40, 0x6f, 0xce, 0x74, 0x8e, 0x20, 0xf8, 0x6b, 0x0a, 0x7f,
    0xcc, 0x2f, 0x4a, 0xfb, 0xe8, 0xf5, 0x50, 0x77, 0x1b, 0xd8, 0xe3, 0xdf,
    0x25, 0x0b, 0x2a, 0x43, 0x8a, 0x41, 0x66, 0x2d, 0x47, 0xf4, 0xe1, 0x9b,
    0xa5, 0x66, 0xca, 0xe2, 0xb4, 0xda, 0x16, 0xef, 0xaa, 0xe8, 0xd5, 0x47,
    0x8b, 0x0c, 0xfc, 0xed, 0x89, 0x6c, 0x53, 0x4c, 0x46, 0x08, 0x32, 0xa4,
    0xff, 0x50, 0x6c, 0xfb, 0x58, 0x9b, 0x2b, 0x02, 0x40, 0x49, 0x49, 0x3a,
    0x42, 0x38, 0x2b, 0x68, 0xa5, 0xcd, 0xd5, 0x9e, 0x09, 0xa6, 0xa3, 0x01,
    0x31, 0xe7, 0x09, 0x4d, 0x63, 0x2c, 0xa1, 0x29, 0x92, 0xee, 0x76, 0x69,
    0x86, 0xa6, 0x24, 0x5b, 0x89, 0xfb, 0xf6, 0x44, 0xc7, 0x4f, 0x1c, 0x8f,
    0x1a, 0x2f, 0xb7, 0x11, 0xc3, 0x2c, 0x38, 0x7f, 0x0c, 0x2e, 0x77, 0x2d,
    0x9e, 0x62, 0xf2, 0x50, 0x58, 0x28, 0xbf, 0x9e, 0x6d, 0xc8, 0x07, 0x16,
    0x6b, 0x02, 0x41, 0x00, 0xe8, 0x72, 0x7b, 0xce, 0x08, 0x27, 0x0a, 0xf5,
    0x1b, 0xf6, 0x1a, 0xec, 0x66, 0x5b, 0x65, 0x60, 0x7f, 0xcc, 0x5e, 0xcf,
    0x8e, 0xa6, 0xe4, 0x48, 0xcd, 0x82, 0x87, 0x59, 0xee, 0x9f, 0x22, 0x9e,
    0x81, 0x32, 0x34, 0x0c, 0x14, 0x7f, 0x89, 0xab, 0xc7, 0xe8, 0xab, 0x0a,
    0x1f, 0xdd, 0x99, 0x2c, 0xa9, 0xa3, 0xf1, 0x64, 0xa2, 0x1a, 0x93, 0xa6,
    0x2a, 0xfe, 0x68, 0x42, 0xea, 0x5f, 0xab, 0x93
};

class ImportKeyRequestMatcher
    : public MatcherInterface<const ImportKeyRequest&> {
  public:
    explicit ImportKeyRequestMatcher(
        const hidl_vec<KeyParameter>& params, const RSA *rsa)
        : params_(params),
          rsa_(rsa),
          ec_key_(NULL),
          symmetric_key_(NULL),
          symmetric_key_size_(0) {
    }

    explicit ImportKeyRequestMatcher(
        const hidl_vec<KeyParameter>& params, const EC_KEY *ec)
        : params_(params),
          rsa_(NULL),
          ec_key_(ec),
          symmetric_key_(NULL),
          symmetric_key_size_(0) {
    }

    explicit ImportKeyRequestMatcher(
        const hidl_vec<KeyParameter>& params,
        const uint8_t *symmetric_key, size_t symmetric_key_size)
        : params_(params),
          rsa_(NULL),
          ec_key_(NULL),
          symmetric_key_(symmetric_key),
          symmetric_key_size_(symmetric_key_size) {
    }

    virtual void DescribeTo(::std::ostream* os) const {
        *os << "ImportKeyRequest matched expectation";
    }

    virtual void DescribeNegationTo(::std::ostream* os) const {
        *os << "ImportKeyRequest mis-matched expectation";
    }

    virtual bool MatchAndExplain(const ImportKeyRequest& request,
                                 MatchResultListener* listener) const {
        (void)listener;

        if (rsa_ != nullptr) {
            return this->MatchAndExplainRSA(request);
        } else if (ec_key_ != nullptr) {
            return this->MatchAndExplainEC(request);
        } else {
            return this->MatchAndExplainSymmetricKey(request);
        }
    }

  private:
    bool MatchAndExplainRSA(const ImportKeyRequest& request) const {
        if (params_.size() != (size_t)request.params().params().size()) {
            LOG(ERROR) << "test: KeyParameters len mis-match";
            return false;
        }

        hidl_vec<KeyParameter> params;
        if (pb_to_hidl_params(request.params(), &params) != ErrorCode::OK) {
            LOG(ERROR) << "test: pb_to_hidl_params failed";
            return false;
        }

        for (size_t i = 0; i < params_.size(); i++) {
            if(!(params[i] == params_[i])) {
                LOG(ERROR) << "test: KeyParameters mis-match";
                return false;
            }
        }

        const BIGNUM *n;
        const BIGNUM *e;
        const BIGNUM *d;
        RSA_get0_key(rsa_, &n, &e, &d);

        if (request.rsa().e() != BN_get_word(e)) {
            LOG(ERROR) << "e mismatch, expected: "
                       << BN_get_word(e)
                       << " got "
                       << request.rsa().e();
            return false;
        }

        unique_ptr<uint8_t[]> d_buf(new uint8_t[BN_num_bytes(n)]);
        if (!BN_bn2le_padded(d_buf.get(), BN_num_bytes(n), d)) {
            LOG(ERROR) << "bn2le(d) failed";
            return false;
        }
        // Expect d to be zero-padded to sizeof n if necessary.
        if (request.rsa().d().size() != BN_num_bytes(n) &&
            memcmp(request.rsa().d().data(), d_buf.get(),
                   BN_num_bytes(n)) != 0) {
            LOG(ERROR) << "d-bytes mis-matched";
            return false;
        }

        unique_ptr<uint8_t[]> n_buf(new uint8_t[BN_num_bytes(n)]);
        if (!BN_bn2le_padded(n_buf.get(), BN_num_bytes(n), n)) {
            LOG(ERROR) << "bn2le(n) failed";
            return false;
        }
        if (request.rsa().n().size() != BN_num_bytes(n) &&
            memcmp(request.rsa().n().data(), n_buf.get(),
                   BN_num_bytes(n)) != 0) {
            LOG(ERROR) << "n-bytes mis-matched";
            return false;
        }

        return true;
    }

    bool MatchAndExplainEC(const ImportKeyRequest& request) const {
        // Curve parameter.
        uint32_t expected_curve_id = (uint32_t)-1;
        const EC_GROUP *group = EC_KEY_get0_group(ec_key_);
        switch (EC_GROUP_get_curve_name(group)) {
        case NID_secp224r1:
            expected_curve_id = (uint32_t)EcCurve::P_224;
            break;
        case NID_X9_62_prime256v1:
            expected_curve_id = (uint32_t)EcCurve::P_256;
            break;
        case NID_secp384r1:
            expected_curve_id = (uint32_t)EcCurve::P_384;
            break;
        case NID_secp521r1:
            expected_curve_id = (uint32_t)EcCurve::P_521;
            break;
        }

        if (request.ec().curve_id() != expected_curve_id) {
            LOG(ERROR) << "Curve-id mismatch, got: "
                       << request.ec().curve_id()
                       << " expected "
                       << expected_curve_id;
            return false;
        }

        const BIGNUM *d = EC_KEY_get0_private_key(ec_key_);
        const EC_POINT *pub_key = EC_KEY_get0_public_key(ec_key_);
        bssl::UniquePtr<BIGNUM> x(BN_new());
        bssl::UniquePtr<BIGNUM> y(BN_new());

        if (!EC_POINT_get_affine_coordinates_GFp(
                EC_KEY_get0_group(ec_key_),
                pub_key, x.get(), y.get(), NULL)) {
            LOG(ERROR) << "failed to get public key in affine coordinates";
            return false;
        }

        // Private key.
        const size_t field_size = (EC_GROUP_get_degree(group) + 7) >> 3;
        unique_ptr<uint8_t[]> d_buf(new uint8_t[field_size]);
        if (!BN_bn2le_padded(d_buf.get(), field_size, d)) {
            LOG(ERROR) << "bn2le(d) failed";
            return false;
        }
        if (request.ec().d().size() != field_size ||
            memcmp(request.ec().d().data(), d_buf.get(), field_size) != 0) {
            LOG(ERROR) << "d-bytes mis-matched";
            return false;
        }

        // Public key.
        unique_ptr<uint8_t[]> x_buf(new uint8_t[field_size]);
        if (!BN_bn2le_padded(x_buf.get(), field_size, x.get())) {
            LOG(ERROR) << "bn2le(x) failed";
            return false;
        }
        if (request.ec().x().size() != field_size ||
            memcmp(request.ec().x().data(), x_buf.get(), field_size) != 0) {
            LOG(ERROR) << "x-bytes mis-matched";
            return false;
        }

        unique_ptr<uint8_t[]> y_buf(new uint8_t[field_size]);
        if (!BN_bn2le_padded(y_buf.get(), field_size, y.get())) {
            LOG(ERROR) << "bn2le(y) failed";
            return false;
        }
        if (request.ec().y().size() != field_size ||
            memcmp(request.ec().y().data(), y_buf.get(), field_size) != 0) {
            LOG(ERROR) << "y-bytes mis-matched";
            return false;
        }

        return false;
    }

    bool MatchAndExplainSymmetricKey(const ImportKeyRequest& request) const {
        if (request.symmetric_key().material().size() != symmetric_key_size_ ||
            memcmp(request.symmetric_key().material().data(),
                   symmetric_key_, symmetric_key_size_) != 0) {
            LOG(ERROR) << "symmetric key bytes mis-match";
            return false;
        }

        return true;
    }

  private:
    const hidl_vec<KeyParameter> params_;
    const RSA *rsa_;
    const EC_KEY *ec_key_;
    const uint8_t *symmetric_key_;
    size_t symmetric_key_size_;
};

static Matcher<const ImportKeyRequest&> ImportKeyRequestEq(
    const hidl_vec<KeyParameter>& params, const RSA *rsa) {
    return MakeMatcher(new ImportKeyRequestMatcher(params, rsa));
}

TEST(KeymasterHalTest, importKeyPKCS8Success) {
    MockKeymaster mockService;
    ImportKeyResponse response;

    KeyParameter kp;
    kp.tag = Tag::ALGORITHM;
    kp.f.algorithm = Algorithm::RSA;

    bssl::UniquePtr<RSA> rsa;
    bssl::UniquePtr<EVP_PKEY> pkey(EVP_PKEY_new());
    bssl::UniquePtr<PKCS8_PRIV_KEY_INFO> pkcs8;

    // TODO: Avoid parsing in the validator, use known values instead.
    rsa.reset(RSA_private_key_from_bytes(TEST_RSA1024_KEY,
                                         sizeof(TEST_RSA1024_KEY)));
    EXPECT_NE(rsa.get(), nullptr);
    EXPECT_EQ(EVP_PKEY_set1_RSA(pkey.get(), rsa.get()), 1);
    pkcs8.reset(EVP_PKEY2PKCS8(pkey.get()));
    EXPECT_NE(pkcs8.get(), nullptr);

    int len = i2d_PKCS8_PRIV_KEY_INFO(pkcs8.get(), NULL);
    std::unique_ptr<uint8_t[]> der(new uint8_t[len]);
    uint8_t *tmp = der.get();
    EXPECT_EQ(i2d_PKCS8_PRIV_KEY_INFO(pkcs8.get(), &tmp), len);

    hidl_vec<uint8_t> der_vec;
    der_vec.setToExternal(const_cast<uint8_t*>(der.get()), len);

    // response
    string dummy_blob("Dummy");
    response.mutable_blob()->set_blob(dummy_blob);
    nosapp::KeyParameter *noskp =
        response.mutable_characteristics()->
        mutable_tee_enforced()->add_params();
    noskp->set_tag(nosapp::Tag::ALGORITHM);
    noskp->set_integer((uint32_t)nosapp::Algorithm::RSA);

    EXPECT_CALL(
        mockService,
        ImportKey(ImportKeyRequestEq(hidl_vec<KeyParameter>{kp}, rsa.get()), _))
        .WillOnce(DoAll(SetArgPointee<1>(response), Return(APP_SUCCESS)));

    KeymasterDevice hal{mockService};
    hal.importKey(
        hidl_vec<KeyParameter>{kp}, KeyFormat::PKCS8, der_vec,
        [&](ErrorCode error, hidl_vec<uint8_t> blob,
            KeyCharacteristics characteristics) {
            EXPECT_EQ(error, ErrorCode::OK);
            EXPECT_THAT(blob, Eq(hidl_vec<uint8_t>{'D', 'u', 'm', 'm', 'y'}));;

            EXPECT_EQ(characteristics.hardwareEnforced.size(), 1u);
            EXPECT_EQ(characteristics.hardwareEnforced[0].tag, Tag::ALGORITHM);
            EXPECT_EQ(characteristics.hardwareEnforced[0].f.algorithm,
                      Algorithm::RSA);
    });
}

// TODO: add negative tests for PKCS8 RSA
// TODO: add tests for EC
// TODO: add tests for AES / HMAC
