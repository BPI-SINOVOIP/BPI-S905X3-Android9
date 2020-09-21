/*
 * Copyright (C) 2014 The Android Open Source Project
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
#include <memory>
#include <string>
#include <vector>

#include <openssl/evp.h>
#include <openssl/x509.h>

#include <hardware/keymaster0.h>

#include <keymaster/android_keymaster.h>
#include <keymaster/attestation_record.h>
#include <keymaster/contexts/pure_soft_keymaster_context.h>
#include <keymaster/contexts/soft_keymaster_context.h>
#include <keymaster/key_factory.h>
#include <keymaster/km_openssl/hmac_key.h>
#include <keymaster/km_openssl/openssl_utils.h>
#include <keymaster/km_openssl/soft_keymaster_enforcement.h>
#include <keymaster/legacy_support/keymaster0_engine.h>
#include <keymaster/soft_keymaster_device.h>

#include "android_keymaster_test_utils.h"

using std::ifstream;
using std::istreambuf_iterator;
using std::ofstream;
using std::string;
using std::unique_ptr;
using std::vector;

extern "C" {
int __android_log_print(int prio, const char* tag, const char* fmt);
int __android_log_print(int prio, const char* tag, const char* fmt) {
    (void)prio, (void)tag, (void)fmt;
    return 0;
}
}  // extern "C"

namespace {

// For some reason std::make_unique isn't available.  Define make_unique.
template <typename T, typename... Args> std::unique_ptr<T> make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

}  // namespace

namespace keymaster {
namespace test {

const uint32_t kOsVersion = 060000;
const uint32_t kOsPatchLevel = 201603;

StdoutLogger logger;

template <typename T> vector<T> make_vector(const T* array, size_t len) {
    return vector<T>(array, array + len);
}

/**
 * KeymasterEnforcement class for use in testing.  It's permissive in the sense that it doesn't
 * check cryptoperiods, but restrictive in the sense that the clock never advances (so rate-limited
 * keys will only work once).
 */
class TestKeymasterEnforcement : public SoftKeymasterEnforcement {
  public:
    TestKeymasterEnforcement() : SoftKeymasterEnforcement(3, 3) {}

    virtual bool activation_date_valid(uint64_t /* activation_date */) const { return true; }
    virtual bool expiration_date_passed(uint64_t /* expiration_date */) const { return false; }
    virtual bool auth_token_timed_out(const hw_auth_token_t& /* token */,
                                      uint32_t /* timeout */) const {
        return false;
    }
    virtual uint32_t get_current_time() const { return 0; }
    virtual bool ValidateTokenSignature(const hw_auth_token_t& /* token */) const { return true; }
};

/**
 * Variant of SoftKeymasterContext that provides a TestKeymasterEnforcement.
 */
class TestKeymasterContext : public SoftKeymasterContext {
  public:
    TestKeymasterContext() {}
    explicit TestKeymasterContext(const string& root_of_trust)
        : SoftKeymasterContext(root_of_trust) {}

    KeymasterEnforcement* enforcement_policy() override { return &test_policy_; }

  private:
    TestKeymasterEnforcement test_policy_;
};

/**
 * Test instance creator that builds a pure software keymaster2 implementation.
 */
class SoftKeymasterTestInstanceCreator : public Keymaster2TestInstanceCreator {
  public:
    keymaster2_device_t* CreateDevice() const override {
        std::cerr << "Creating software-only device" << std::endl;
        context_ = new TestKeymasterContext;
        SoftKeymasterDevice* device = new SoftKeymasterDevice(context_);
        AuthorizationSet version_info(AuthorizationSetBuilder()
                                          .Authorization(TAG_OS_VERSION, kOsVersion)
                                          .Authorization(TAG_OS_PATCHLEVEL, kOsPatchLevel));
        device->keymaster2_device()->configure(device->keymaster2_device(), &version_info);
        return device->keymaster2_device();
    }

    bool algorithm_in_km0_hardware(keymaster_algorithm_t) const override { return false; }
    int keymaster0_calls() const override { return 0; }
    bool is_keymaster1_hw() const override { return false; }
    KeymasterContext* keymaster_context() const override { return context_; }
    string name() const override { return "Soft Keymaster2"; }

  private:
    mutable TestKeymasterContext* context_;
};

/**
 * Test instance creator that builds a SoftKeymasterDevice which wraps a fake hardware keymaster1
 * instance, with minimal digest support.
 */
class Sha256OnlyKeymaster1TestInstanceCreator : public Keymaster2TestInstanceCreator {
    keymaster2_device_t* CreateDevice() const override {
        std::cerr << "Creating keymaster1-backed device that supports only SHA256";

        // fake_device doesn't leak because device (below) takes ownership of it.
        keymaster1_device_t* fake_device = make_device_sha256_only(
            (new SoftKeymasterDevice(new TestKeymasterContext("PseudoHW")))->keymaster_device());

        // device doesn't leak; it's cleaned up by device->keymaster_device()->common.close().
        context_ = new TestKeymasterContext;
        SoftKeymasterDevice* device = new SoftKeymasterDevice(context_);
        device->SetHardwareDevice(fake_device);

        AuthorizationSet version_info(AuthorizationSetBuilder()
                                          .Authorization(TAG_OS_VERSION, kOsVersion)
                                          .Authorization(TAG_OS_PATCHLEVEL, kOsPatchLevel));
        device->keymaster2_device()->configure(device->keymaster2_device(), &version_info);
        return device->keymaster2_device();
    }

    bool algorithm_in_km0_hardware(keymaster_algorithm_t) const override { return false; }
    int keymaster0_calls() const override { return 0; }
    int minimal_digest_set() const override { return true; }
    bool is_keymaster1_hw() const override { return true; }
    KeymasterContext* keymaster_context() const override { return context_; }
    string name() const override { return "Wrapped fake keymaster1 w/minimal digests"; }

  private:
    mutable TestKeymasterContext* context_;
};

/**
 * Test instance creator that builds a SoftKeymasterDevice which wraps a fake hardware keymaster1
 * instance, with full digest support
 */
class Keymaster1TestInstanceCreator : public Keymaster2TestInstanceCreator {
    keymaster2_device_t* CreateDevice() const override {
        std::cerr << "Creating keymaster1-backed device";

        // fake_device doesn't leak because device (below) takes ownership of it.
        keymaster1_device_t* fake_device =
            (new SoftKeymasterDevice(new TestKeymasterContext("PseudoHW")))->keymaster_device();

        // device doesn't leak; it's cleaned up by device->keymaster_device()->common.close().
        context_ = new TestKeymasterContext;
        SoftKeymasterDevice* device = new SoftKeymasterDevice(context_);
        device->SetHardwareDevice(fake_device);

        AuthorizationSet version_info(AuthorizationSetBuilder()
                                          .Authorization(TAG_OS_VERSION, kOsVersion)
                                          .Authorization(TAG_OS_PATCHLEVEL, kOsPatchLevel));
        device->keymaster2_device()->configure(device->keymaster2_device(), &version_info);
        return device->keymaster2_device();
    }

    bool algorithm_in_km0_hardware(keymaster_algorithm_t) const override { return false; }
    int keymaster0_calls() const override { return 0; }
    int minimal_digest_set() const override { return false; }
    bool is_keymaster1_hw() const override { return true; }
    KeymasterContext* keymaster_context() const override { return context_; }
    string name() const override { return "Wrapped fake keymaster1 w/full digests"; }

  private:
    mutable TestKeymasterContext* context_;
};

static auto test_params = testing::Values(
    InstanceCreatorPtr(new SoftKeymasterTestInstanceCreator),
    InstanceCreatorPtr(new Keymaster1TestInstanceCreator),
    InstanceCreatorPtr(new Sha256OnlyKeymaster1TestInstanceCreator));

class NewKeyGeneration : public Keymaster2Test {
  protected:
    void CheckBaseParams() {
        AuthorizationSet auths = sw_enforced();
        EXPECT_GT(auths.SerializedSize(), 12U);

        EXPECT_TRUE(contains(auths, TAG_PURPOSE, KM_PURPOSE_SIGN));
        EXPECT_TRUE(contains(auths, TAG_PURPOSE, KM_PURPOSE_VERIFY));
        EXPECT_TRUE(contains(auths, TAG_USER_ID, 7));
        EXPECT_TRUE(contains(auths, TAG_USER_AUTH_TYPE, HW_AUTH_PASSWORD));
        EXPECT_TRUE(contains(auths, TAG_AUTH_TIMEOUT, 300));

        // Verify that App ID, App data and ROT are NOT included.
        EXPECT_FALSE(contains(auths, TAG_ROOT_OF_TRUST));
        EXPECT_FALSE(contains(auths, TAG_APPLICATION_ID));
        EXPECT_FALSE(contains(auths, TAG_APPLICATION_DATA));

        // Just for giggles, check that some unexpected tags/values are NOT present.
        EXPECT_FALSE(contains(auths, TAG_PURPOSE, KM_PURPOSE_ENCRYPT));
        EXPECT_FALSE(contains(auths, TAG_PURPOSE, KM_PURPOSE_DECRYPT));
        EXPECT_FALSE(contains(auths, TAG_AUTH_TIMEOUT, 301));

        // Now check that unspecified, defaulted tags are correct.
        EXPECT_TRUE(contains(auths, KM_TAG_CREATION_DATETIME));
        if (GetParam()->is_keymaster1_hw()) {
            // If the underlying (faked) HW is KM1, it will not have version info.
            EXPECT_FALSE(auths.Contains(TAG_OS_VERSION));
            EXPECT_FALSE(auths.Contains(TAG_OS_PATCHLEVEL));
        } else {
            // In all othe cases; SoftKeymasterDevice keys, or keymaster0 keys wrapped by
            // SoftKeymasterDevice, version information will be present and up to date.
            EXPECT_TRUE(contains(auths, TAG_OS_VERSION, kOsVersion));
            EXPECT_TRUE(contains(auths, TAG_OS_PATCHLEVEL, kOsPatchLevel));
        }
    }
};
INSTANTIATE_TEST_CASE_P(AndroidKeymasterTest, NewKeyGeneration, test_params);

TEST_P(NewKeyGeneration, Rsa) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .RsaSigningKey(256, 3)
                                           .Digest(KM_DIGEST_NONE)
                                           .Padding(KM_PAD_NONE)));
    CheckBaseParams();

    // Check specified tags are all present, and in the right set.
    AuthorizationSet crypto_params;
    AuthorizationSet non_crypto_params;
    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA)) {
        EXPECT_NE(0U, hw_enforced().size());
        EXPECT_NE(0U, sw_enforced().size());
        crypto_params.push_back(hw_enforced());
        non_crypto_params.push_back(sw_enforced());
    } else {
        EXPECT_EQ(0U, hw_enforced().size());
        EXPECT_NE(0U, sw_enforced().size());
        crypto_params.push_back(sw_enforced());
    }

    EXPECT_TRUE(contains(crypto_params, TAG_ALGORITHM, KM_ALGORITHM_RSA));
    EXPECT_FALSE(contains(non_crypto_params, TAG_ALGORITHM, KM_ALGORITHM_RSA));
    EXPECT_TRUE(contains(crypto_params, TAG_KEY_SIZE, 256));
    EXPECT_FALSE(contains(non_crypto_params, TAG_KEY_SIZE, 256));
    EXPECT_TRUE(contains(crypto_params, TAG_RSA_PUBLIC_EXPONENT, 3));
    EXPECT_FALSE(contains(non_crypto_params, TAG_RSA_PUBLIC_EXPONENT, 3));

    EXPECT_EQ(KM_ERROR_OK, DeleteKey());

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(2, GetParam()->keymaster0_calls());
}

TEST_P(NewKeyGeneration, RsaDefaultSize) {
    ASSERT_EQ(KM_ERROR_UNSUPPORTED_KEY_SIZE,
              GenerateKey(AuthorizationSetBuilder()
                              .Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA)
                              .Authorization(TAG_RSA_PUBLIC_EXPONENT, 3)
                              .SigningKey()));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(NewKeyGeneration, Ecdsa) {
    ASSERT_EQ(KM_ERROR_OK,
              GenerateKey(AuthorizationSetBuilder().EcdsaSigningKey(224).Digest(KM_DIGEST_NONE)));
    CheckBaseParams();

    // Check specified tags are all present, and in the right set.
    AuthorizationSet crypto_params;
    AuthorizationSet non_crypto_params;
    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_EC)) {
        EXPECT_NE(0U, hw_enforced().size());
        EXPECT_NE(0U, sw_enforced().size());
        crypto_params.push_back(hw_enforced());
        non_crypto_params.push_back(sw_enforced());
    } else {
        EXPECT_EQ(0U, hw_enforced().size());
        EXPECT_NE(0U, sw_enforced().size());
        crypto_params.push_back(sw_enforced());
    }

    EXPECT_TRUE(contains(crypto_params, TAG_ALGORITHM, KM_ALGORITHM_EC));
    EXPECT_FALSE(contains(non_crypto_params, TAG_ALGORITHM, KM_ALGORITHM_EC));
    EXPECT_TRUE(contains(crypto_params, TAG_KEY_SIZE, 224));
    EXPECT_FALSE(contains(non_crypto_params, TAG_KEY_SIZE, 224));

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_EC))
        EXPECT_EQ(1, GetParam()->keymaster0_calls());
}

TEST_P(NewKeyGeneration, EcdsaDefaultSize) {
    ASSERT_EQ(KM_ERROR_UNSUPPORTED_KEY_SIZE,
              GenerateKey(AuthorizationSetBuilder()
                              .Authorization(TAG_ALGORITHM, KM_ALGORITHM_EC)
                              .SigningKey()
                              .Digest(KM_DIGEST_NONE)));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(NewKeyGeneration, EcdsaInvalidSize) {
    ASSERT_EQ(KM_ERROR_UNSUPPORTED_KEY_SIZE,
              GenerateKey(AuthorizationSetBuilder().EcdsaSigningKey(190).Digest(KM_DIGEST_NONE)));
    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(NewKeyGeneration, EcdsaMismatchKeySize) {
    ASSERT_EQ(KM_ERROR_INVALID_ARGUMENT,
              GenerateKey(AuthorizationSetBuilder()
                              .EcdsaSigningKey(224)
                              .Authorization(TAG_EC_CURVE, KM_EC_CURVE_P_256)
                              .Digest(KM_DIGEST_NONE)));
}

TEST_P(NewKeyGeneration, EcdsaAllValidSizes) {
    size_t valid_sizes[] = {224, 256, 384, 521};
    for (size_t size : valid_sizes) {
        EXPECT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder().EcdsaSigningKey(size).Digest(
                                   KM_DIGEST_NONE)))
            << "Failed to generate size: " << size;
    }

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_EC))
        EXPECT_EQ(4, GetParam()->keymaster0_calls());
}

TEST_P(NewKeyGeneration, HmacSha256) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .HmacKey(128)
                                           .Digest(KM_DIGEST_SHA_2_256)
                                           .Authorization(TAG_MIN_MAC_LENGTH, 256)));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(NewKeyGeneration, CheckKeySizes) {
    for (size_t key_size = 0; key_size <= kMaxHmacKeyLengthBits + 10; ++key_size) {
        if (key_size < kMinHmacKeyLengthBits || key_size > kMaxHmacKeyLengthBits ||
            key_size % 8 != 0) {
            EXPECT_EQ(KM_ERROR_UNSUPPORTED_KEY_SIZE,
                      GenerateKey(AuthorizationSetBuilder()
                                      .HmacKey(key_size)
                                      .Digest(KM_DIGEST_SHA_2_256)
                                      .Authorization(TAG_MIN_MAC_LENGTH, 256)))
                << "HMAC key size " << key_size << " invalid.";
        } else {
            EXPECT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                                   .HmacKey(key_size)
                                                   .Digest(KM_DIGEST_SHA_2_256)
                                                   .Authorization(TAG_MIN_MAC_LENGTH, 256)));
        }
    }
    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(NewKeyGeneration, HmacMultipleDigests) {
    ASSERT_EQ(KM_ERROR_UNSUPPORTED_DIGEST,
              GenerateKey(AuthorizationSetBuilder()
                              .HmacKey(128)
                              .Digest(KM_DIGEST_SHA1)
                              .Digest(KM_DIGEST_SHA_2_256)
                              .Authorization(TAG_MIN_MAC_LENGTH, 128)));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(NewKeyGeneration, HmacDigestNone) {
    ASSERT_EQ(KM_ERROR_UNSUPPORTED_DIGEST,
              GenerateKey(AuthorizationSetBuilder()
                              .HmacKey(128)
                              .Digest(KM_DIGEST_NONE)
                              .Authorization(TAG_MIN_MAC_LENGTH, 128)));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(NewKeyGeneration, HmacSha256TooShortMacLength) {
    ASSERT_EQ(KM_ERROR_UNSUPPORTED_MIN_MAC_LENGTH,
              GenerateKey(AuthorizationSetBuilder()
                              .HmacKey(128)
                              .Digest(KM_DIGEST_SHA_2_256)
                              .Authorization(TAG_MIN_MAC_LENGTH, 48)));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(NewKeyGeneration, HmacSha256NonIntegralOctetMacLength) {
    ASSERT_EQ(KM_ERROR_UNSUPPORTED_MIN_MAC_LENGTH,
              GenerateKey(AuthorizationSetBuilder()
                              .HmacKey(128)
                              .Digest(KM_DIGEST_SHA_2_256)
                              .Authorization(TAG_MIN_MAC_LENGTH, 130)));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(NewKeyGeneration, HmacSha256TooLongMacLength) {
    ASSERT_EQ(KM_ERROR_UNSUPPORTED_MIN_MAC_LENGTH,
              GenerateKey(AuthorizationSetBuilder()
                              .HmacKey(128)
                              .Digest(KM_DIGEST_SHA_2_256)
                              .Authorization(TAG_MIN_MAC_LENGTH, 384)));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

typedef Keymaster2Test GetKeyCharacteristics;
INSTANTIATE_TEST_CASE_P(AndroidKeymasterTest, GetKeyCharacteristics, test_params);

TEST_P(GetKeyCharacteristics, SimpleRsa) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .RsaSigningKey(256, 3)
                                           .Digest(KM_DIGEST_NONE)
                                           .Padding(KM_PAD_NONE)));
    AuthorizationSet original(sw_enforced());

    ASSERT_EQ(KM_ERROR_OK, GetCharacteristics());
    EXPECT_EQ(original, sw_enforced());

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(1, GetParam()->keymaster0_calls());
}

typedef Keymaster2Test SigningOperationsTest;
INSTANTIATE_TEST_CASE_P(AndroidKeymasterTest, SigningOperationsTest, test_params);

TEST_P(SigningOperationsTest, RsaSuccess) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .RsaSigningKey(256, 3)
                                           .Digest(KM_DIGEST_NONE)
                                           .Padding(KM_PAD_NONE)));
    string message = "12345678901234567890123456789012";
    string signature;
    SignMessage(message, &signature, KM_DIGEST_NONE, KM_PAD_NONE);

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(3, GetParam()->keymaster0_calls());
}

TEST_P(SigningOperationsTest, RsaPssSha256Success) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .RsaSigningKey(768, 3)
                                           .Digest(KM_DIGEST_SHA_2_256)
                                           .Padding(KM_PAD_RSA_PSS)));
    // Use large message, which won't work without digesting.
    string message(1024, 'a');
    string signature;
    SignMessage(message, &signature, KM_DIGEST_SHA_2_256, KM_PAD_RSA_PSS);

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(3, GetParam()->keymaster0_calls());
}

TEST_P(SigningOperationsTest, RsaPaddingNoneDoesNotAllowOther) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .RsaSigningKey(512, 3)
                                           .Digest(KM_DIGEST_NONE)
                                           .Padding(KM_PAD_NONE)));
    string message = "12345678901234567890123456789012";
    string signature;

    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_DIGEST, KM_DIGEST_NONE);
    begin_params.push_back(TAG_PADDING, KM_PAD_RSA_PKCS1_1_5_SIGN);
    EXPECT_EQ(KM_ERROR_INCOMPATIBLE_PADDING_MODE, BeginOperation(KM_PURPOSE_SIGN, begin_params));

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(2, GetParam()->keymaster0_calls());
}

TEST_P(SigningOperationsTest, RsaPkcs1Sha256Success) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .RsaSigningKey(512, 3)
                                           .Digest(KM_DIGEST_SHA_2_256)
                                           .Padding(KM_PAD_RSA_PKCS1_1_5_SIGN)));
    string message(1024, 'a');
    string signature;
    SignMessage(message, &signature, KM_DIGEST_SHA_2_256, KM_PAD_RSA_PKCS1_1_5_SIGN);

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(3, GetParam()->keymaster0_calls());
}

TEST_P(SigningOperationsTest, RsaPkcs1NoDigestSuccess) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .RsaSigningKey(512, 3)
                                           .Digest(KM_DIGEST_NONE)
                                           .Padding(KM_PAD_RSA_PKCS1_1_5_SIGN)));
    string message(53, 'a');
    string signature;
    SignMessage(message, &signature, KM_DIGEST_NONE, KM_PAD_RSA_PKCS1_1_5_SIGN);

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(3, GetParam()->keymaster0_calls());
}

TEST_P(SigningOperationsTest, RsaPkcs1NoDigestTooLarge) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .RsaSigningKey(512, 3)
                                           .Digest(KM_DIGEST_NONE)
                                           .Padding(KM_PAD_RSA_PKCS1_1_5_SIGN)));
    string message(54, 'a');

    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_DIGEST, KM_DIGEST_NONE);
    begin_params.push_back(TAG_PADDING, KM_PAD_RSA_PKCS1_1_5_SIGN);
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_SIGN, begin_params));
    string result;
    string signature;
    EXPECT_EQ(KM_ERROR_INVALID_INPUT_LENGTH, FinishOperation(message, "", &signature));

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(2, GetParam()->keymaster0_calls());
}

TEST_P(SigningOperationsTest, RsaPssSha256TooSmallKey) {
    // Key must be at least 10 bytes larger than hash, to provide eight bytes of random salt, so
    // verify that nine bytes larger than hash won't work.
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .RsaSigningKey(256 + 9 * 8, 3)
                                           .Digest(KM_DIGEST_SHA_2_256)
                                           .Padding(KM_PAD_RSA_PSS)));
    string message(1024, 'a');
    string signature;

    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_DIGEST, KM_DIGEST_SHA_2_256);
    begin_params.push_back(TAG_PADDING, KM_PAD_RSA_PSS);
    EXPECT_EQ(KM_ERROR_INCOMPATIBLE_DIGEST, BeginOperation(KM_PURPOSE_SIGN, begin_params));
}

TEST_P(SigningOperationsTest, RsaNoPaddingHugeData) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .RsaSigningKey(256, 3)
                                           .Digest(KM_DIGEST_NONE)
                                           .Padding(KM_PAD_RSA_PKCS1_1_5_SIGN)));
    string message(64 * 1024, 'a');
    string signature;
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_DIGEST, KM_DIGEST_NONE);
    begin_params.push_back(TAG_PADDING, KM_PAD_RSA_PKCS1_1_5_SIGN);
    ASSERT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_SIGN, begin_params));
    string result;
    size_t input_consumed;
    EXPECT_EQ(KM_ERROR_INVALID_INPUT_LENGTH, UpdateOperation(message, &result, &input_consumed));

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(2, GetParam()->keymaster0_calls());
}

TEST_P(SigningOperationsTest, RsaAbort) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .RsaSigningKey(256, 3)
                                           .Digest(KM_DIGEST_NONE)
                                           .Padding(KM_PAD_NONE)));
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_DIGEST, KM_DIGEST_NONE);
    begin_params.push_back(TAG_PADDING, KM_PAD_NONE);
    ASSERT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_SIGN, begin_params));
    EXPECT_EQ(KM_ERROR_OK, AbortOperation());
    // Another abort should fail
    EXPECT_EQ(KM_ERROR_INVALID_OPERATION_HANDLE, AbortOperation());

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(2, GetParam()->keymaster0_calls());
}

TEST_P(SigningOperationsTest, RsaUnsupportedPadding) {
    GenerateKey(AuthorizationSetBuilder()
                    .RsaSigningKey(256, 3)
                    .Digest(KM_DIGEST_SHA_2_256 /* supported digest */)
                    .Padding(KM_PAD_PKCS7));
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_DIGEST, KM_DIGEST_SHA_2_256);
    ASSERT_EQ(KM_ERROR_UNSUPPORTED_PADDING_MODE, BeginOperation(KM_PURPOSE_SIGN, begin_params));

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(2, GetParam()->keymaster0_calls());
}

TEST_P(SigningOperationsTest, RsaNoDigest) {
    // PSS requires a digest.
    GenerateKey(AuthorizationSetBuilder()
                    .RsaSigningKey(256, 3)
                    .Digest(KM_DIGEST_NONE)
                    .Padding(KM_PAD_RSA_PSS));
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_DIGEST, KM_DIGEST_NONE);
    begin_params.push_back(TAG_PADDING, KM_PAD_RSA_PSS);
    ASSERT_EQ(KM_ERROR_INCOMPATIBLE_DIGEST, BeginOperation(KM_PURPOSE_SIGN, begin_params));

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(2, GetParam()->keymaster0_calls());
}

TEST_P(SigningOperationsTest, RsaNoPadding) {
    // Padding must be specified
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder().RsaKey(256, 3).SigningKey().Digest(
                               KM_DIGEST_NONE)));
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_DIGEST, KM_DIGEST_NONE);
    ASSERT_EQ(KM_ERROR_UNSUPPORTED_PADDING_MODE, BeginOperation(KM_PURPOSE_SIGN, begin_params));

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(2, GetParam()->keymaster0_calls());
}

TEST_P(SigningOperationsTest, RsaTooShortMessage) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .RsaSigningKey(256, 3)
                                           .Digest(KM_DIGEST_NONE)
                                           .Padding(KM_PAD_NONE)));
    string message = "1234567890123456789012345678901";
    string signature;
    SignMessage(message, &signature, KM_DIGEST_NONE, KM_PAD_NONE);

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(3, GetParam()->keymaster0_calls());
}

TEST_P(SigningOperationsTest, RsaSignWithEncryptionKey) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .RsaEncryptionKey(256, 3)
                                           .Digest(KM_DIGEST_NONE)
                                           .Padding(KM_PAD_NONE)));
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_PADDING, KM_PAD_NONE);
    begin_params.push_back(TAG_DIGEST, KM_DIGEST_NONE);
    ASSERT_EQ(KM_ERROR_INCOMPATIBLE_PURPOSE, BeginOperation(KM_PURPOSE_SIGN, begin_params));

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(2, GetParam()->keymaster0_calls());
}

TEST_P(SigningOperationsTest, RsaSignTooLargeMessage) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .RsaSigningKey(256, 3)
                                           .Digest(KM_DIGEST_NONE)
                                           .Padding(KM_PAD_NONE)));
    string message(256 / 8, static_cast<char>(0xff));
    string signature;
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_PADDING, KM_PAD_NONE);
    begin_params.push_back(TAG_DIGEST, KM_DIGEST_NONE);
    ASSERT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_SIGN, begin_params));
    string result;
    size_t input_consumed;
    ASSERT_EQ(KM_ERROR_OK, UpdateOperation(message, &result, &input_consumed));
    ASSERT_EQ(message.size(), input_consumed);
    string output;
    ASSERT_EQ(KM_ERROR_INVALID_ARGUMENT, FinishOperation(&output));

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(3, GetParam()->keymaster0_calls());
}

TEST_P(SigningOperationsTest, EcdsaSuccess) {
    ASSERT_EQ(KM_ERROR_OK,
              GenerateKey(AuthorizationSetBuilder().EcdsaSigningKey(224).Digest(KM_DIGEST_NONE)));
    string message(224 / 8, 'a');
    string signature;
    SignMessage(message, &signature, KM_DIGEST_NONE);

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_EC))
        EXPECT_EQ(3, GetParam()->keymaster0_calls());
}

TEST_P(SigningOperationsTest, EcdsaSha256Success) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder().EcdsaSigningKey(224).Digest(
                               KM_DIGEST_SHA_2_256)));
    string message(1024, 'a');
    string signature;
    SignMessage(message, &signature, KM_DIGEST_SHA_2_256);

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_EC))
        EXPECT_EQ(3, GetParam()->keymaster0_calls());
}

TEST_P(SigningOperationsTest, EcdsaSha384Success) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder().EcdsaSigningKey(224).Digest(
                               KM_DIGEST_SHA_2_384)));
    string message(1024, 'a');
    string signature;
    SignMessage(message, &signature, KM_DIGEST_SHA_2_384);

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_EC))
        EXPECT_EQ(3, GetParam()->keymaster0_calls());
}

TEST_P(SigningOperationsTest, EcdsaNoPaddingHugeData) {
    ASSERT_EQ(KM_ERROR_OK,
              GenerateKey(AuthorizationSetBuilder().EcdsaSigningKey(224).Digest(KM_DIGEST_NONE)));
    string message(64 * 1024, 'a');
    string signature;
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_DIGEST, KM_DIGEST_NONE);
    ASSERT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_SIGN, begin_params));
    string result;
    size_t input_consumed;
    EXPECT_EQ(KM_ERROR_OK, UpdateOperation(message, &result, &input_consumed));

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_EC))
        EXPECT_EQ(2, GetParam()->keymaster0_calls());
}

TEST_P(SigningOperationsTest, EcdsaAllSizesAndHashes) {
    vector<int> key_sizes = {224, 256, 384, 521};
    vector<keymaster_digest_t> digests = {
        KM_DIGEST_SHA1,      KM_DIGEST_SHA_2_224, KM_DIGEST_SHA_2_256,
        KM_DIGEST_SHA_2_384, KM_DIGEST_SHA_2_512,
    };

    for (int key_size : key_sizes) {
        for (keymaster_digest_t digest : digests) {
            ASSERT_EQ(
                KM_ERROR_OK,
                GenerateKey(AuthorizationSetBuilder().EcdsaSigningKey(key_size).Digest(digest)));

            string message(1024, 'a');
            string signature;
            if (digest == KM_DIGEST_NONE)
                message.resize(key_size / 8);
            SignMessage(message, &signature, digest);
        }
    }

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_EC))
        EXPECT_EQ(digests.size() * key_sizes.size() * 3,
                  static_cast<size_t>(GetParam()->keymaster0_calls()));
}

TEST_P(SigningOperationsTest, AesEcbSign) {
    ASSERT_EQ(KM_ERROR_OK,
              GenerateKey(AuthorizationSetBuilder().AesEncryptionKey(128).Authorization(
                  TAG_BLOCK_MODE, KM_MODE_ECB)));
    ASSERT_EQ(KM_ERROR_UNSUPPORTED_PURPOSE, BeginOperation(KM_PURPOSE_SIGN));
    ASSERT_EQ(KM_ERROR_UNSUPPORTED_PURPOSE, BeginOperation(KM_PURPOSE_VERIFY));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(SigningOperationsTest, HmacSha1Success) {
    GenerateKey(AuthorizationSetBuilder()
                    .HmacKey(128)
                    .Digest(KM_DIGEST_SHA1)
                    .Authorization(TAG_MIN_MAC_LENGTH, 160));
    string message = "12345678901234567890123456789012";
    string signature;
    MacMessage(message, &signature, 160);
    ASSERT_EQ(20U, signature.size());

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(SigningOperationsTest, HmacSha224Success) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .HmacKey(128)
                                           .Digest(KM_DIGEST_SHA_2_224)
                                           .Authorization(TAG_MIN_MAC_LENGTH, 160)));
    string message = "12345678901234567890123456789012";
    string signature;
    MacMessage(message, &signature, 224);
    ASSERT_EQ(28U, signature.size());

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(SigningOperationsTest, HmacSha256Success) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .HmacKey(128)
                                           .Digest(KM_DIGEST_SHA_2_256)
                                           .Authorization(TAG_MIN_MAC_LENGTH, 256)));
    string message = "12345678901234567890123456789012";
    string signature;
    MacMessage(message, &signature, 256);
    ASSERT_EQ(32U, signature.size());

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(SigningOperationsTest, HmacSha384Success) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .HmacKey(128)
                                           .Digest(KM_DIGEST_SHA_2_384)
                                           .Authorization(TAG_MIN_MAC_LENGTH, 384)));

    string message = "12345678901234567890123456789012";
    string signature;
    MacMessage(message, &signature, 384);
    ASSERT_EQ(48U, signature.size());

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(SigningOperationsTest, HmacSha512Success) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .HmacKey(128)
                                           .Digest(KM_DIGEST_SHA_2_512)
                                           .Authorization(TAG_MIN_MAC_LENGTH, 384)));
    string message = "12345678901234567890123456789012";
    string signature;
    MacMessage(message, &signature, 512);
    ASSERT_EQ(64U, signature.size());

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(SigningOperationsTest, HmacLengthInKey) {
    // TODO(swillden): unified API should generate an error on key generation.
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .HmacKey(128)
                                           .Digest(KM_DIGEST_SHA_2_256)
                                           .Authorization(TAG_MIN_MAC_LENGTH, 128)));
    string message = "12345678901234567890123456789012";
    string signature;
    MacMessage(message, &signature, 160);
    ASSERT_EQ(20U, signature.size());

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(SigningOperationsTest, HmacRfc4231TestCase3) {
    string key(20, 0xaa);
    string message(50, 0xdd);
    uint8_t sha_224_expected[] = {
        0x7f, 0xb3, 0xcb, 0x35, 0x88, 0xc6, 0xc1, 0xf6, 0xff, 0xa9, 0x69, 0x4d, 0x7d, 0x6a,
        0xd2, 0x64, 0x93, 0x65, 0xb0, 0xc1, 0xf6, 0x5d, 0x69, 0xd1, 0xec, 0x83, 0x33, 0xea,
    };
    uint8_t sha_256_expected[] = {
        0x77, 0x3e, 0xa9, 0x1e, 0x36, 0x80, 0x0e, 0x46, 0x85, 0x4d, 0xb8,
        0xeb, 0xd0, 0x91, 0x81, 0xa7, 0x29, 0x59, 0x09, 0x8b, 0x3e, 0xf8,
        0xc1, 0x22, 0xd9, 0x63, 0x55, 0x14, 0xce, 0xd5, 0x65, 0xfe,
    };
    uint8_t sha_384_expected[] = {
        0x88, 0x06, 0x26, 0x08, 0xd3, 0xe6, 0xad, 0x8a, 0x0a, 0xa2, 0xac, 0xe0,
        0x14, 0xc8, 0xa8, 0x6f, 0x0a, 0xa6, 0x35, 0xd9, 0x47, 0xac, 0x9f, 0xeb,
        0xe8, 0x3e, 0xf4, 0xe5, 0x59, 0x66, 0x14, 0x4b, 0x2a, 0x5a, 0xb3, 0x9d,
        0xc1, 0x38, 0x14, 0xb9, 0x4e, 0x3a, 0xb6, 0xe1, 0x01, 0xa3, 0x4f, 0x27,
    };
    uint8_t sha_512_expected[] = {
        0xfa, 0x73, 0xb0, 0x08, 0x9d, 0x56, 0xa2, 0x84, 0xef, 0xb0, 0xf0, 0x75, 0x6c,
        0x89, 0x0b, 0xe9, 0xb1, 0xb5, 0xdb, 0xdd, 0x8e, 0xe8, 0x1a, 0x36, 0x55, 0xf8,
        0x3e, 0x33, 0xb2, 0x27, 0x9d, 0x39, 0xbf, 0x3e, 0x84, 0x82, 0x79, 0xa7, 0x22,
        0xc8, 0x06, 0xb4, 0x85, 0xa4, 0x7e, 0x67, 0xc8, 0x07, 0xb9, 0x46, 0xa3, 0x37,
        0xbe, 0xe8, 0x94, 0x26, 0x74, 0x27, 0x88, 0x59, 0xe1, 0x32, 0x92, 0xfb,
    };

    CheckHmacTestVector(key, message, KM_DIGEST_SHA_2_224, make_string(sha_224_expected));
    CheckHmacTestVector(key, message, KM_DIGEST_SHA_2_256, make_string(sha_256_expected));
    CheckHmacTestVector(key, message, KM_DIGEST_SHA_2_384, make_string(sha_384_expected));
    CheckHmacTestVector(key, message, KM_DIGEST_SHA_2_512, make_string(sha_512_expected));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(SigningOperationsTest, HmacRfc4231TestCase4) {
    uint8_t key_data[25] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,
        0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
    };
    string key = make_string(key_data);
    string message(50, 0xcd);
    uint8_t sha_224_expected[] = {
        0x6c, 0x11, 0x50, 0x68, 0x74, 0x01, 0x3c, 0xac, 0x6a, 0x2a, 0xbc, 0x1b, 0xb3, 0x82,
        0x62, 0x7c, 0xec, 0x6a, 0x90, 0xd8, 0x6e, 0xfc, 0x01, 0x2d, 0xe7, 0xaf, 0xec, 0x5a,
    };
    uint8_t sha_256_expected[] = {
        0x82, 0x55, 0x8a, 0x38, 0x9a, 0x44, 0x3c, 0x0e, 0xa4, 0xcc, 0x81,
        0x98, 0x99, 0xf2, 0x08, 0x3a, 0x85, 0xf0, 0xfa, 0xa3, 0xe5, 0x78,
        0xf8, 0x07, 0x7a, 0x2e, 0x3f, 0xf4, 0x67, 0x29, 0x66, 0x5b,
    };
    uint8_t sha_384_expected[] = {
        0x3e, 0x8a, 0x69, 0xb7, 0x78, 0x3c, 0x25, 0x85, 0x19, 0x33, 0xab, 0x62,
        0x90, 0xaf, 0x6c, 0xa7, 0x7a, 0x99, 0x81, 0x48, 0x08, 0x50, 0x00, 0x9c,
        0xc5, 0x57, 0x7c, 0x6e, 0x1f, 0x57, 0x3b, 0x4e, 0x68, 0x01, 0xdd, 0x23,
        0xc4, 0xa7, 0xd6, 0x79, 0xcc, 0xf8, 0xa3, 0x86, 0xc6, 0x74, 0xcf, 0xfb,
    };
    uint8_t sha_512_expected[] = {
        0xb0, 0xba, 0x46, 0x56, 0x37, 0x45, 0x8c, 0x69, 0x90, 0xe5, 0xa8, 0xc5, 0xf6,
        0x1d, 0x4a, 0xf7, 0xe5, 0x76, 0xd9, 0x7f, 0xf9, 0x4b, 0x87, 0x2d, 0xe7, 0x6f,
        0x80, 0x50, 0x36, 0x1e, 0xe3, 0xdb, 0xa9, 0x1c, 0xa5, 0xc1, 0x1a, 0xa2, 0x5e,
        0xb4, 0xd6, 0x79, 0x27, 0x5c, 0xc5, 0x78, 0x80, 0x63, 0xa5, 0xf1, 0x97, 0x41,
        0x12, 0x0c, 0x4f, 0x2d, 0xe2, 0xad, 0xeb, 0xeb, 0x10, 0xa2, 0x98, 0xdd,
    };

    CheckHmacTestVector(key, message, KM_DIGEST_SHA_2_224, make_string(sha_224_expected));
    CheckHmacTestVector(key, message, KM_DIGEST_SHA_2_256, make_string(sha_256_expected));
    CheckHmacTestVector(key, message, KM_DIGEST_SHA_2_384, make_string(sha_384_expected));
    CheckHmacTestVector(key, message, KM_DIGEST_SHA_2_512, make_string(sha_512_expected));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(SigningOperationsTest, HmacRfc4231TestCase5) {
    string key(20, 0x0c);
    string message = "Test With Truncation";

    uint8_t sha_224_expected[] = {
        0x0e, 0x2a, 0xea, 0x68, 0xa9, 0x0c, 0x8d, 0x37,
        0xc9, 0x88, 0xbc, 0xdb, 0x9f, 0xca, 0x6f, 0xa8,
    };
    uint8_t sha_256_expected[] = {
        0xa3, 0xb6, 0x16, 0x74, 0x73, 0x10, 0x0e, 0xe0,
        0x6e, 0x0c, 0x79, 0x6c, 0x29, 0x55, 0x55, 0x2b,
    };
    uint8_t sha_384_expected[] = {
        0x3a, 0xbf, 0x34, 0xc3, 0x50, 0x3b, 0x2a, 0x23,
        0xa4, 0x6e, 0xfc, 0x61, 0x9b, 0xae, 0xf8, 0x97,
    };
    uint8_t sha_512_expected[] = {
        0x41, 0x5f, 0xad, 0x62, 0x71, 0x58, 0x0a, 0x53,
        0x1d, 0x41, 0x79, 0xbc, 0x89, 0x1d, 0x87, 0xa6,
    };

    CheckHmacTestVector(key, message, KM_DIGEST_SHA_2_224, make_string(sha_224_expected));
    CheckHmacTestVector(key, message, KM_DIGEST_SHA_2_256, make_string(sha_256_expected));
    CheckHmacTestVector(key, message, KM_DIGEST_SHA_2_384, make_string(sha_384_expected));
    CheckHmacTestVector(key, message, KM_DIGEST_SHA_2_512, make_string(sha_512_expected));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(SigningOperationsTest, HmacRfc4231TestCase6) {
    string key(131, 0xaa);
    string message = "Test Using Larger Than Block-Size Key - Hash Key First";

    uint8_t sha_224_expected[] = {
        0x95, 0xe9, 0xa0, 0xdb, 0x96, 0x20, 0x95, 0xad, 0xae, 0xbe, 0x9b, 0x2d, 0x6f, 0x0d,
        0xbc, 0xe2, 0xd4, 0x99, 0xf1, 0x12, 0xf2, 0xd2, 0xb7, 0x27, 0x3f, 0xa6, 0x87, 0x0e,
    };
    uint8_t sha_256_expected[] = {
        0x60, 0xe4, 0x31, 0x59, 0x1e, 0xe0, 0xb6, 0x7f, 0x0d, 0x8a, 0x26,
        0xaa, 0xcb, 0xf5, 0xb7, 0x7f, 0x8e, 0x0b, 0xc6, 0x21, 0x37, 0x28,
        0xc5, 0x14, 0x05, 0x46, 0x04, 0x0f, 0x0e, 0xe3, 0x7f, 0x54,
    };
    uint8_t sha_384_expected[] = {
        0x4e, 0xce, 0x08, 0x44, 0x85, 0x81, 0x3e, 0x90, 0x88, 0xd2, 0xc6, 0x3a,
        0x04, 0x1b, 0xc5, 0xb4, 0x4f, 0x9e, 0xf1, 0x01, 0x2a, 0x2b, 0x58, 0x8f,
        0x3c, 0xd1, 0x1f, 0x05, 0x03, 0x3a, 0xc4, 0xc6, 0x0c, 0x2e, 0xf6, 0xab,
        0x40, 0x30, 0xfe, 0x82, 0x96, 0x24, 0x8d, 0xf1, 0x63, 0xf4, 0x49, 0x52,
    };
    uint8_t sha_512_expected[] = {
        0x80, 0xb2, 0x42, 0x63, 0xc7, 0xc1, 0xa3, 0xeb, 0xb7, 0x14, 0x93, 0xc1, 0xdd,
        0x7b, 0xe8, 0xb4, 0x9b, 0x46, 0xd1, 0xf4, 0x1b, 0x4a, 0xee, 0xc1, 0x12, 0x1b,
        0x01, 0x37, 0x83, 0xf8, 0xf3, 0x52, 0x6b, 0x56, 0xd0, 0x37, 0xe0, 0x5f, 0x25,
        0x98, 0xbd, 0x0f, 0xd2, 0x21, 0x5d, 0x6a, 0x1e, 0x52, 0x95, 0xe6, 0x4f, 0x73,
        0xf6, 0x3f, 0x0a, 0xec, 0x8b, 0x91, 0x5a, 0x98, 0x5d, 0x78, 0x65, 0x98,
    };

    CheckHmacTestVector(key, message, KM_DIGEST_SHA_2_224, make_string(sha_224_expected));
    CheckHmacTestVector(key, message, KM_DIGEST_SHA_2_256, make_string(sha_256_expected));
    CheckHmacTestVector(key, message, KM_DIGEST_SHA_2_384, make_string(sha_384_expected));
    CheckHmacTestVector(key, message, KM_DIGEST_SHA_2_512, make_string(sha_512_expected));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(SigningOperationsTest, HmacRfc4231TestCase7) {
    string key(131, 0xaa);
    string message = "This is a test using a larger than block-size key and a larger than "
                     "block-size data. The key needs to be hashed before being used by the HMAC "
                     "algorithm.";

    uint8_t sha_224_expected[] = {
        0x3a, 0x85, 0x41, 0x66, 0xac, 0x5d, 0x9f, 0x02, 0x3f, 0x54, 0xd5, 0x17, 0xd0, 0xb3,
        0x9d, 0xbd, 0x94, 0x67, 0x70, 0xdb, 0x9c, 0x2b, 0x95, 0xc9, 0xf6, 0xf5, 0x65, 0xd1,
    };
    uint8_t sha_256_expected[] = {
        0x9b, 0x09, 0xff, 0xa7, 0x1b, 0x94, 0x2f, 0xcb, 0x27, 0x63, 0x5f,
        0xbc, 0xd5, 0xb0, 0xe9, 0x44, 0xbf, 0xdc, 0x63, 0x64, 0x4f, 0x07,
        0x13, 0x93, 0x8a, 0x7f, 0x51, 0x53, 0x5c, 0x3a, 0x35, 0xe2,
    };
    uint8_t sha_384_expected[] = {
        0x66, 0x17, 0x17, 0x8e, 0x94, 0x1f, 0x02, 0x0d, 0x35, 0x1e, 0x2f, 0x25,
        0x4e, 0x8f, 0xd3, 0x2c, 0x60, 0x24, 0x20, 0xfe, 0xb0, 0xb8, 0xfb, 0x9a,
        0xdc, 0xce, 0xbb, 0x82, 0x46, 0x1e, 0x99, 0xc5, 0xa6, 0x78, 0xcc, 0x31,
        0xe7, 0x99, 0x17, 0x6d, 0x38, 0x60, 0xe6, 0x11, 0x0c, 0x46, 0x52, 0x3e,
    };
    uint8_t sha_512_expected[] = {
        0xe3, 0x7b, 0x6a, 0x77, 0x5d, 0xc8, 0x7d, 0xba, 0xa4, 0xdf, 0xa9, 0xf9, 0x6e,
        0x5e, 0x3f, 0xfd, 0xde, 0xbd, 0x71, 0xf8, 0x86, 0x72, 0x89, 0x86, 0x5d, 0xf5,
        0xa3, 0x2d, 0x20, 0xcd, 0xc9, 0x44, 0xb6, 0x02, 0x2c, 0xac, 0x3c, 0x49, 0x82,
        0xb1, 0x0d, 0x5e, 0xeb, 0x55, 0xc3, 0xe4, 0xde, 0x15, 0x13, 0x46, 0x76, 0xfb,
        0x6d, 0xe0, 0x44, 0x60, 0x65, 0xc9, 0x74, 0x40, 0xfa, 0x8c, 0x6a, 0x58,
    };

    CheckHmacTestVector(key, message, KM_DIGEST_SHA_2_224, make_string(sha_224_expected));
    CheckHmacTestVector(key, message, KM_DIGEST_SHA_2_256, make_string(sha_256_expected));
    CheckHmacTestVector(key, message, KM_DIGEST_SHA_2_384, make_string(sha_384_expected));
    CheckHmacTestVector(key, message, KM_DIGEST_SHA_2_512, make_string(sha_512_expected));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(SigningOperationsTest, HmacSha256TooLargeMacLength) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .HmacKey(128)
                                           .Digest(KM_DIGEST_SHA_2_256)
                                           .Authorization(TAG_MIN_MAC_LENGTH, 256)));
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_MAC_LENGTH, 264);
    begin_params.push_back(TAG_DIGEST, KM_DIGEST_SHA_2_256);
    ASSERT_EQ(KM_ERROR_UNSUPPORTED_MAC_LENGTH,
              BeginOperation(KM_PURPOSE_SIGN, begin_params, nullptr /* output_params */));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(SigningOperationsTest, HmacSha256TooSmallMacLength) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .HmacKey(128)
                                           .Digest(KM_DIGEST_SHA_2_256)
                                           .Authorization(TAG_MIN_MAC_LENGTH, 128)));
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_MAC_LENGTH, 120);
    begin_params.push_back(TAG_DIGEST, KM_DIGEST_SHA_2_256);
    ASSERT_EQ(KM_ERROR_INVALID_MAC_LENGTH,
              BeginOperation(KM_PURPOSE_SIGN, begin_params, nullptr /* output_params */));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

// TODO(swillden): Add more verification failure tests.

typedef Keymaster2Test VerificationOperationsTest;
INSTANTIATE_TEST_CASE_P(AndroidKeymasterTest, VerificationOperationsTest, test_params);

TEST_P(VerificationOperationsTest, RsaSuccess) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .RsaSigningKey(256, 3)
                                           .Digest(KM_DIGEST_NONE)
                                           .Padding(KM_PAD_NONE)));
    string message = "12345678901234567890123456789012";
    string signature;
    SignMessage(message, &signature, KM_DIGEST_NONE, KM_PAD_NONE);
    VerifyMessage(message, signature, KM_DIGEST_NONE, KM_PAD_NONE);

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(4, GetParam()->keymaster0_calls());
}

TEST_P(VerificationOperationsTest, RsaPssSha256Success) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .RsaSigningKey(768, 3)
                                           .Digest(KM_DIGEST_SHA_2_256)
                                           .Padding(KM_PAD_RSA_PSS)));
    // Use large message, which won't work without digesting.
    string message(1024, 'a');
    string signature;
    SignMessage(message, &signature, KM_DIGEST_SHA_2_256, KM_PAD_RSA_PSS);
    VerifyMessage(message, signature, KM_DIGEST_SHA_2_256, KM_PAD_RSA_PSS);

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(4, GetParam()->keymaster0_calls());
}

TEST_P(VerificationOperationsTest, RsaPssSha224Success) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .RsaSigningKey(512, 3)
                                           .Digest(KM_DIGEST_SHA_2_224)
                                           .Padding(KM_PAD_RSA_PSS)));
    // Use large message, which won't work without digesting.
    string message(1024, 'a');
    string signature;
    SignMessage(message, &signature, KM_DIGEST_SHA_2_224, KM_PAD_RSA_PSS);
    VerifyMessage(message, signature, KM_DIGEST_SHA_2_224, KM_PAD_RSA_PSS);

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(4, GetParam()->keymaster0_calls());

    // Verify with OpenSSL.
    string pubkey;
    EXPECT_EQ(KM_ERROR_OK, ExportKey(KM_KEY_FORMAT_X509, &pubkey));

    const uint8_t* p = reinterpret_cast<const uint8_t*>(pubkey.data());
    unique_ptr<EVP_PKEY, EVP_PKEY_Delete> pkey(
        d2i_PUBKEY(nullptr /* alloc new */, &p, pubkey.size()));
    ASSERT_TRUE(pkey.get());

    EVP_MD_CTX digest_ctx;
    EVP_MD_CTX_init(&digest_ctx);
    EVP_PKEY_CTX* pkey_ctx;
    EXPECT_EQ(1, EVP_DigestVerifyInit(&digest_ctx, &pkey_ctx, EVP_sha224(), nullptr /* engine */,
                                      pkey.get()));
    EXPECT_EQ(1, EVP_PKEY_CTX_set_rsa_padding(pkey_ctx, RSA_PKCS1_PSS_PADDING));
    EXPECT_EQ(1, EVP_DigestVerifyUpdate(&digest_ctx, message.data(), message.size()));
    EXPECT_EQ(1,
              EVP_DigestVerifyFinal(&digest_ctx, reinterpret_cast<const uint8_t*>(signature.data()),
                                    signature.size()));
    EVP_MD_CTX_cleanup(&digest_ctx);
}

TEST_P(VerificationOperationsTest, RsaPssSha256CorruptSignature) {
    GenerateKey(AuthorizationSetBuilder()
                    .RsaSigningKey(768, 3)
                    .Digest(KM_DIGEST_SHA_2_256)
                    .Padding(KM_PAD_RSA_PSS));
    string message(1024, 'a');
    string signature;
    SignMessage(message, &signature, KM_DIGEST_SHA_2_256, KM_PAD_RSA_PSS);
    ++signature[signature.size() / 2];

    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_DIGEST, KM_DIGEST_SHA_2_256);
    begin_params.push_back(TAG_PADDING, KM_PAD_RSA_PSS);
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_VERIFY, begin_params));

    string result;
    EXPECT_EQ(KM_ERROR_VERIFICATION_FAILED, FinishOperation(message, signature, &result));

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(4, GetParam()->keymaster0_calls());
}

TEST_P(VerificationOperationsTest, RsaPssSha256CorruptInput) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .RsaSigningKey(768, 3)
                                           .Digest(KM_DIGEST_SHA_2_256)
                                           .Padding(KM_PAD_RSA_PSS)));
    // Use large message, which won't work without digesting.
    string message(1024, 'a');
    string signature;
    SignMessage(message, &signature, KM_DIGEST_SHA_2_256, KM_PAD_RSA_PSS);
    ++message[message.size() / 2];

    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_DIGEST, KM_DIGEST_SHA_2_256);
    begin_params.push_back(TAG_PADDING, KM_PAD_RSA_PSS);
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_VERIFY, begin_params));

    string result;
    EXPECT_EQ(KM_ERROR_VERIFICATION_FAILED, FinishOperation(message, signature, &result));

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(4, GetParam()->keymaster0_calls());
}

TEST_P(VerificationOperationsTest, RsaPkcs1Sha256Success) {
    GenerateKey(AuthorizationSetBuilder()
                    .RsaSigningKey(512, 3)
                    .Digest(KM_DIGEST_SHA_2_256)
                    .Padding(KM_PAD_RSA_PKCS1_1_5_SIGN));
    string message(1024, 'a');
    string signature;
    SignMessage(message, &signature, KM_DIGEST_SHA_2_256, KM_PAD_RSA_PKCS1_1_5_SIGN);
    VerifyMessage(message, signature, KM_DIGEST_SHA_2_256, KM_PAD_RSA_PKCS1_1_5_SIGN);

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(4, GetParam()->keymaster0_calls());
}

TEST_P(VerificationOperationsTest, RsaPks1Sha224Success) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .RsaSigningKey(512, 3)
                                           .Digest(KM_DIGEST_SHA_2_224)
                                           .Padding(KM_PAD_RSA_PKCS1_1_5_SIGN)));
    // Use large message, which won't work without digesting.
    string message(1024, 'a');
    string signature;
    SignMessage(message, &signature, KM_DIGEST_SHA_2_224, KM_PAD_RSA_PKCS1_1_5_SIGN);
    VerifyMessage(message, signature, KM_DIGEST_SHA_2_224, KM_PAD_RSA_PKCS1_1_5_SIGN);

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(4, GetParam()->keymaster0_calls());

    // Verify with OpenSSL.
    string pubkey;
    EXPECT_EQ(KM_ERROR_OK, ExportKey(KM_KEY_FORMAT_X509, &pubkey));

    const uint8_t* p = reinterpret_cast<const uint8_t*>(pubkey.data());
    unique_ptr<EVP_PKEY, EVP_PKEY_Delete> pkey(
        d2i_PUBKEY(nullptr /* alloc new */, &p, pubkey.size()));
    ASSERT_TRUE(pkey.get());

    EVP_MD_CTX digest_ctx;
    EVP_MD_CTX_init(&digest_ctx);
    EVP_PKEY_CTX* pkey_ctx;
    EXPECT_EQ(1, EVP_DigestVerifyInit(&digest_ctx, &pkey_ctx, EVP_sha224(), nullptr /* engine */,
                                      pkey.get()));
    EXPECT_EQ(1, EVP_DigestVerifyUpdate(&digest_ctx, message.data(), message.size()));
    EXPECT_EQ(1,
              EVP_DigestVerifyFinal(&digest_ctx, reinterpret_cast<const uint8_t*>(signature.data()),
                                    signature.size()));
    EVP_MD_CTX_cleanup(&digest_ctx);
}

TEST_P(VerificationOperationsTest, RsaPkcs1Sha256CorruptSignature) {
    GenerateKey(AuthorizationSetBuilder()
                    .RsaSigningKey(512, 3)
                    .Digest(KM_DIGEST_SHA_2_256)
                    .Padding(KM_PAD_RSA_PKCS1_1_5_SIGN));
    string message(1024, 'a');
    string signature;
    SignMessage(message, &signature, KM_DIGEST_SHA_2_256, KM_PAD_RSA_PKCS1_1_5_SIGN);
    ++signature[signature.size() / 2];

    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_DIGEST, KM_DIGEST_SHA_2_256);
    begin_params.push_back(TAG_PADDING, KM_PAD_RSA_PKCS1_1_5_SIGN);
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_VERIFY, begin_params));

    string result;
    EXPECT_EQ(KM_ERROR_VERIFICATION_FAILED, FinishOperation(message, signature, &result));

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(4, GetParam()->keymaster0_calls());
}

TEST_P(VerificationOperationsTest, RsaPkcs1Sha256CorruptInput) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .RsaSigningKey(512, 3)
                                           .Digest(KM_DIGEST_SHA_2_256)
                                           .Padding(KM_PAD_RSA_PKCS1_1_5_SIGN)));
    // Use large message, which won't work without digesting.
    string message(1024, 'a');
    string signature;
    SignMessage(message, &signature, KM_DIGEST_SHA_2_256, KM_PAD_RSA_PKCS1_1_5_SIGN);
    ++message[message.size() / 2];

    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_DIGEST, KM_DIGEST_SHA_2_256);
    begin_params.push_back(TAG_PADDING, KM_PAD_RSA_PKCS1_1_5_SIGN);
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_VERIFY, begin_params));

    string result;
    EXPECT_EQ(KM_ERROR_VERIFICATION_FAILED, FinishOperation(message, signature, &result));

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(4, GetParam()->keymaster0_calls());
}

TEST_P(VerificationOperationsTest, RsaAllDigestAndPadCombinations) {
    vector<keymaster_digest_t> digests = {
        KM_DIGEST_NONE,      KM_DIGEST_MD5,       KM_DIGEST_SHA1,      KM_DIGEST_SHA_2_224,
        KM_DIGEST_SHA_2_256, KM_DIGEST_SHA_2_384, KM_DIGEST_SHA_2_512,
    };

    vector<keymaster_padding_t> padding_modes{
        KM_PAD_NONE, KM_PAD_RSA_PKCS1_1_5_SIGN, KM_PAD_RSA_PSS,
    };

    int trial_count = 0;
    for (keymaster_padding_t padding_mode : padding_modes) {
        for (keymaster_digest_t digest : digests) {
            if (digest != KM_DIGEST_NONE && padding_mode == KM_PAD_NONE)
                // Digesting requires padding
                continue;

            // Compute key & message size that will work.
            size_t key_bits = 0;
            size_t message_len = 1000;

            if (digest == KM_DIGEST_NONE) {
                key_bits = 256;
                switch (padding_mode) {
                case KM_PAD_NONE:
                    // Match key size.
                    message_len = key_bits / 8;
                    break;
                case KM_PAD_RSA_PKCS1_1_5_SIGN:
                    message_len = key_bits / 8 - 11;
                    break;
                case KM_PAD_RSA_PSS:
                    // PSS requires a digest.
                    continue;
                default:
                    FAIL() << "Missing padding";
                    break;
                }
            } else {
                size_t digest_bits;
                switch (digest) {
                case KM_DIGEST_MD5:
                    digest_bits = 128;
                    break;
                case KM_DIGEST_SHA1:
                    digest_bits = 160;
                    break;
                case KM_DIGEST_SHA_2_224:
                    digest_bits = 224;
                    break;
                case KM_DIGEST_SHA_2_256:
                    digest_bits = 256;
                    break;
                case KM_DIGEST_SHA_2_384:
                    digest_bits = 384;
                    break;
                case KM_DIGEST_SHA_2_512:
                    digest_bits = 512;
                    break;
                default:
                    FAIL() << "Missing digest";
                }

                switch (padding_mode) {
                case KM_PAD_RSA_PKCS1_1_5_SIGN:
                    key_bits = digest_bits + 8 * (11 + 19);
                    break;
                case KM_PAD_RSA_PSS:
                    key_bits = digest_bits * 2 + 2 * 8;
                    break;
                default:
                    FAIL() << "Missing padding";
                    break;
                }
            }

            // Round up to the nearest multiple of 128.
            key_bits = (key_bits + 127) / 128 * 128;

            GenerateKey(AuthorizationSetBuilder()
                            .RsaSigningKey(key_bits, 3)
                            .Digest(digest)
                            .Padding(padding_mode));
            string message(message_len, 'a');
            string signature;
            SignMessage(message, &signature, digest, padding_mode);
            VerifyMessage(message, signature, digest, padding_mode);
            ++trial_count;
        }
    }

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(trial_count * 4, GetParam()->keymaster0_calls());
}

TEST_P(VerificationOperationsTest, EcdsaSuccess) {
    ASSERT_EQ(KM_ERROR_OK,
              GenerateKey(AuthorizationSetBuilder().EcdsaSigningKey(256).Digest(KM_DIGEST_NONE)));
    string message = "12345678901234567890123456789012";
    string signature;
    SignMessage(message, &signature, KM_DIGEST_NONE);
    VerifyMessage(message, signature, KM_DIGEST_NONE);

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_EC))
        EXPECT_EQ(4, GetParam()->keymaster0_calls());
}

TEST_P(VerificationOperationsTest, EcdsaTooShort) {
    ASSERT_EQ(KM_ERROR_OK,
              GenerateKey(AuthorizationSetBuilder().EcdsaSigningKey(256).Digest(KM_DIGEST_NONE)));
    string message = "12345678901234567890";
    string signature;
    SignMessage(message, &signature, KM_DIGEST_NONE);
    VerifyMessage(message, signature, KM_DIGEST_NONE);

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_EC))
        EXPECT_EQ(4, GetParam()->keymaster0_calls());
}

TEST_P(VerificationOperationsTest, EcdsaSlightlyTooLong) {
    ASSERT_EQ(KM_ERROR_OK,
              GenerateKey(AuthorizationSetBuilder().EcdsaSigningKey(521).Digest(KM_DIGEST_NONE)));

    string message(66, 'a');
    string signature;
    SignMessage(message, &signature, KM_DIGEST_NONE);
    VerifyMessage(message, signature, KM_DIGEST_NONE);

    // Modifying low-order bits doesn't matter, because they didn't get signed.  Ugh.
    message[65] ^= 7;
    VerifyMessage(message, signature, KM_DIGEST_NONE);

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_EC))
        EXPECT_EQ(5, GetParam()->keymaster0_calls());
}

TEST_P(VerificationOperationsTest, EcdsaSha256Success) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .EcdsaSigningKey(256)
                                           .Digest(KM_DIGEST_SHA_2_256)
                                           .Digest(KM_DIGEST_NONE)));
    string message = "12345678901234567890123456789012";
    string signature;
    SignMessage(message, &signature, KM_DIGEST_SHA_2_256);
    VerifyMessage(message, signature, KM_DIGEST_SHA_2_256);

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_EC))
        EXPECT_EQ(4, GetParam()->keymaster0_calls());

    // Just for giggles, try verifying with the wrong digest.
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_DIGEST, KM_DIGEST_NONE);
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_VERIFY, begin_params));

    string result;
    EXPECT_EQ(KM_ERROR_VERIFICATION_FAILED, FinishOperation(message, signature, &result));
}

TEST_P(VerificationOperationsTest, EcdsaSha224Success) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder().EcdsaSigningKey(256).Digest(
                               KM_DIGEST_SHA_2_224)));

    string message = "12345678901234567890123456789012";
    string signature;
    SignMessage(message, &signature, KM_DIGEST_SHA_2_224);
    VerifyMessage(message, signature, KM_DIGEST_SHA_2_224);

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_EC))
        EXPECT_EQ(4, GetParam()->keymaster0_calls());

    // Just for giggles, try verifying with the wrong digest.
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_DIGEST, KM_DIGEST_NONE);
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_VERIFY, begin_params));

    string result;
    EXPECT_EQ(KM_ERROR_VERIFICATION_FAILED, FinishOperation(message, signature, &result));
}

TEST_P(VerificationOperationsTest, EcdsaAllDigestsAndKeySizes) {
    keymaster_digest_t digests[] = {
        KM_DIGEST_SHA1,      KM_DIGEST_SHA_2_224, KM_DIGEST_SHA_2_256,
        KM_DIGEST_SHA_2_384, KM_DIGEST_SHA_2_512,
    };
    size_t key_sizes[] = {224, 256, 384, 521};

    string message = "1234567890";
    string signature;

    for (auto key_size : key_sizes) {
        SCOPED_TRACE(testing::Message() << "Key size: " << key_size);
        AuthorizationSetBuilder builder;
        builder.EcdsaSigningKey(key_size);
        for (auto digest : digests)
            builder.Digest(digest);
        ASSERT_EQ(KM_ERROR_OK, GenerateKey(builder));

        for (auto digest : digests) {
            SCOPED_TRACE(testing::Message() << "Digest: " << digest);
            SignMessage(message, &signature, digest);
            VerifyMessage(message, signature, digest);
        }
    }

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_EC))
        EXPECT_EQ(static_cast<int>(array_length(key_sizes) * (1 + 3 * array_length(digests))),
                  GetParam()->keymaster0_calls());
}

TEST_P(VerificationOperationsTest, HmacSha1Success) {
    GenerateKey(AuthorizationSetBuilder()
                    .HmacKey(128)
                    .Digest(KM_DIGEST_SHA1)
                    .Authorization(TAG_MIN_MAC_LENGTH, 128));
    string message = "123456789012345678901234567890123456789012345678";
    string signature;
    MacMessage(message, &signature, 160);
    VerifyMac(message, signature);

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(VerificationOperationsTest, HmacSha224Success) {
    GenerateKey(AuthorizationSetBuilder()
                    .HmacKey(128)
                    .Digest(KM_DIGEST_SHA_2_224)
                    .Authorization(TAG_MIN_MAC_LENGTH, 128));
    string message = "123456789012345678901234567890123456789012345678";
    string signature;
    MacMessage(message, &signature, 224);
    VerifyMac(message, signature);

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(VerificationOperationsTest, HmacSha256Success) {
    GenerateKey(AuthorizationSetBuilder()
                    .HmacKey(128)
                    .Digest(KM_DIGEST_SHA_2_256)
                    .Authorization(TAG_MIN_MAC_LENGTH, 128));
    string message = "123456789012345678901234567890123456789012345678";
    string signature;
    MacMessage(message, &signature, 256);
    VerifyMac(message, signature);

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(VerificationOperationsTest, HmacSha256TooShortMac) {
    GenerateKey(AuthorizationSetBuilder()
                    .HmacKey(128)
                    .Digest(KM_DIGEST_SHA_2_256)
                    .Authorization(TAG_MIN_MAC_LENGTH, 128));
    string message = "123456789012345678901234567890123456789012345678";
    string signature;
    MacMessage(message, &signature, 256);

    // Shorten to 128 bits, should still work.
    signature.resize(128 / 8);
    VerifyMac(message, signature);

    // Drop one more byte.
    signature.resize(signature.length() - 1);

    AuthorizationSet begin_params(client_params());
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_VERIFY, begin_params));
    string result;
    EXPECT_EQ(KM_ERROR_INVALID_MAC_LENGTH, FinishOperation(message, signature, &result));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(VerificationOperationsTest, HmacSha384Success) {
    GenerateKey(AuthorizationSetBuilder()
                    .HmacKey(128)
                    .Digest(KM_DIGEST_SHA_2_384)
                    .Authorization(TAG_MIN_MAC_LENGTH, 128));
    string message = "123456789012345678901234567890123456789012345678";
    string signature;
    MacMessage(message, &signature, 384);
    VerifyMac(message, signature);

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(VerificationOperationsTest, HmacSha512Success) {
    GenerateKey(AuthorizationSetBuilder()
                    .HmacKey(128)
                    .Digest(KM_DIGEST_SHA_2_512)
                    .Authorization(TAG_MIN_MAC_LENGTH, 128));
    string message = "123456789012345678901234567890123456789012345678";
    string signature;
    MacMessage(message, &signature, 512);
    VerifyMac(message, signature);

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

typedef Keymaster2Test ExportKeyTest;
INSTANTIATE_TEST_CASE_P(AndroidKeymasterTest, ExportKeyTest, test_params);

TEST_P(ExportKeyTest, RsaSuccess) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .RsaSigningKey(256, 3)
                                           .Digest(KM_DIGEST_NONE)
                                           .Padding(KM_PAD_NONE)));
    string export_data;
    ASSERT_EQ(KM_ERROR_OK, ExportKey(KM_KEY_FORMAT_X509, &export_data));
    EXPECT_GT(export_data.length(), 0U);

    // TODO(swillden): Verify that the exported key is actually usable to verify signatures.

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(2, GetParam()->keymaster0_calls());
}

TEST_P(ExportKeyTest, EcdsaSuccess) {
    ASSERT_EQ(KM_ERROR_OK,
              GenerateKey(AuthorizationSetBuilder().EcdsaSigningKey(224).Digest(KM_DIGEST_NONE)));
    string export_data;
    ASSERT_EQ(KM_ERROR_OK, ExportKey(KM_KEY_FORMAT_X509, &export_data));
    EXPECT_GT(export_data.length(), 0U);

    // TODO(swillden): Verify that the exported key is actually usable to verify signatures.

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_EC))
        EXPECT_EQ(2, GetParam()->keymaster0_calls());
}

TEST_P(ExportKeyTest, RsaUnsupportedKeyFormat) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .RsaSigningKey(256, 3)
                                           .Digest(KM_DIGEST_NONE)
                                           .Padding(KM_PAD_NONE)));
    string export_data;
    ASSERT_EQ(KM_ERROR_UNSUPPORTED_KEY_FORMAT, ExportKey(KM_KEY_FORMAT_PKCS8, &export_data));

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(2, GetParam()->keymaster0_calls());
}

TEST_P(ExportKeyTest, RsaCorruptedKeyBlob) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .RsaSigningKey(256, 3)
                                           .Digest(KM_DIGEST_NONE)
                                           .Padding(KM_PAD_NONE)));
    corrupt_key_blob();
    string export_data;
    ASSERT_EQ(KM_ERROR_INVALID_KEY_BLOB, ExportKey(KM_KEY_FORMAT_X509, &export_data));

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(2, GetParam()->keymaster0_calls());
}

TEST_P(ExportKeyTest, AesKeyExportFails) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder().AesEncryptionKey(128)));
    string export_data;

    EXPECT_EQ(KM_ERROR_UNSUPPORTED_KEY_FORMAT, ExportKey(KM_KEY_FORMAT_X509, &export_data));
    EXPECT_EQ(KM_ERROR_UNSUPPORTED_KEY_FORMAT, ExportKey(KM_KEY_FORMAT_PKCS8, &export_data));
    EXPECT_EQ(KM_ERROR_UNSUPPORTED_KEY_FORMAT, ExportKey(KM_KEY_FORMAT_RAW, &export_data));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

static string read_file(const string& file_name) {
    ifstream file_stream(file_name, std::ios::binary);
    istreambuf_iterator<char> file_begin(file_stream);
    istreambuf_iterator<char> file_end;
    return string(file_begin, file_end);
}

typedef Keymaster2Test ImportKeyTest;
INSTANTIATE_TEST_CASE_P(AndroidKeymasterTest, ImportKeyTest, test_params);

TEST_P(ImportKeyTest, RsaSuccess) {
    string pk8_key = read_file("rsa_privkey_pk8.der");
    ASSERT_EQ(633U, pk8_key.size());

    ASSERT_EQ(KM_ERROR_OK, ImportKey(AuthorizationSetBuilder()
                                         .RsaSigningKey(1024, 65537)
                                         .Digest(KM_DIGEST_NONE)
                                         .Padding(KM_PAD_NONE),
                                     KM_KEY_FORMAT_PKCS8, pk8_key));

    // Check values derived from the key.
    EXPECT_TRUE(contains(GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA) ? hw_enforced()
                                                                                 : sw_enforced(),
                         TAG_ALGORITHM, KM_ALGORITHM_RSA));
    EXPECT_TRUE(contains(GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA) ? hw_enforced()
                                                                                 : sw_enforced(),
                         TAG_KEY_SIZE, 1024));
    EXPECT_TRUE(contains(GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA) ? hw_enforced()
                                                                                 : sw_enforced(),
                         TAG_RSA_PUBLIC_EXPONENT, 65537U));

    // And values provided by AndroidKeymaster
    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_TRUE(contains(hw_enforced(), TAG_ORIGIN, KM_ORIGIN_UNKNOWN));
    else
        EXPECT_TRUE(contains(sw_enforced(), TAG_ORIGIN, KM_ORIGIN_IMPORTED));
    EXPECT_TRUE(contains(sw_enforced(), KM_TAG_CREATION_DATETIME));

    string message(1024 / 8, 'a');
    string signature;
    SignMessage(message, &signature, KM_DIGEST_NONE, KM_PAD_NONE);
    VerifyMessage(message, signature, KM_DIGEST_NONE, KM_PAD_NONE);

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(4, GetParam()->keymaster0_calls());
}

TEST_P(ImportKeyTest, RsaKeySizeMismatch) {
    string pk8_key = read_file("rsa_privkey_pk8.der");
    ASSERT_EQ(633U, pk8_key.size());
    ASSERT_EQ(KM_ERROR_IMPORT_PARAMETER_MISMATCH,
              ImportKey(AuthorizationSetBuilder()
                            .RsaSigningKey(2048 /* Doesn't match key */, 3)
                            .Digest(KM_DIGEST_NONE)
                            .Padding(KM_PAD_NONE),
                        KM_KEY_FORMAT_PKCS8, pk8_key));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(ImportKeyTest, RsaPublicExponenMismatch) {
    string pk8_key = read_file("rsa_privkey_pk8.der");
    ASSERT_EQ(633U, pk8_key.size());
    ASSERT_EQ(KM_ERROR_IMPORT_PARAMETER_MISMATCH,
              ImportKey(AuthorizationSetBuilder()
                            .RsaSigningKey(256, 3 /* Doesnt' match key */)
                            .Digest(KM_DIGEST_NONE)
                            .Padding(KM_PAD_NONE),
                        KM_KEY_FORMAT_PKCS8, pk8_key));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(ImportKeyTest, EcdsaSuccess) {
    string pk8_key = read_file("ec_privkey_pk8.der");
    ASSERT_EQ(138U, pk8_key.size());

    ASSERT_EQ(KM_ERROR_OK,
              ImportKey(AuthorizationSetBuilder().EcdsaSigningKey(256).Digest(KM_DIGEST_NONE),
                        KM_KEY_FORMAT_PKCS8, pk8_key));

    // Check values derived from the key.
    EXPECT_TRUE(contains(GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_EC) ? hw_enforced()
                                                                                : sw_enforced(),
                         TAG_ALGORITHM, KM_ALGORITHM_EC));
    EXPECT_TRUE(contains(GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_EC) ? hw_enforced()
                                                                                : sw_enforced(),
                         TAG_KEY_SIZE, 256));

    // And values provided by AndroidKeymaster
    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_EC))
        EXPECT_TRUE(contains(hw_enforced(), TAG_ORIGIN, KM_ORIGIN_UNKNOWN));
    else
        EXPECT_TRUE(contains(sw_enforced(), TAG_ORIGIN, KM_ORIGIN_IMPORTED));
    EXPECT_TRUE(contains(sw_enforced(), KM_TAG_CREATION_DATETIME));

    string message(32, 'a');
    string signature;
    SignMessage(message, &signature, KM_DIGEST_NONE);
    VerifyMessage(message, signature, KM_DIGEST_NONE);

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_EC))
        EXPECT_EQ(4, GetParam()->keymaster0_calls());
}

TEST_P(ImportKeyTest, EcdsaSizeSpecified) {
    string pk8_key = read_file("ec_privkey_pk8.der");
    ASSERT_EQ(138U, pk8_key.size());

    ASSERT_EQ(KM_ERROR_OK,
              ImportKey(AuthorizationSetBuilder().EcdsaSigningKey(256).Digest(KM_DIGEST_NONE),
                        KM_KEY_FORMAT_PKCS8, pk8_key));

    // Check values derived from the key.
    EXPECT_TRUE(contains(GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_EC) ? hw_enforced()
                                                                                : sw_enforced(),
                         TAG_ALGORITHM, KM_ALGORITHM_EC));
    EXPECT_TRUE(contains(GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_EC) ? hw_enforced()
                                                                                : sw_enforced(),
                         TAG_KEY_SIZE, 256));

    // And values provided by AndroidKeymaster
    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_EC))
        EXPECT_TRUE(contains(hw_enforced(), TAG_ORIGIN, KM_ORIGIN_UNKNOWN));
    else
        EXPECT_TRUE(contains(sw_enforced(), TAG_ORIGIN, KM_ORIGIN_IMPORTED));
    EXPECT_TRUE(contains(sw_enforced(), KM_TAG_CREATION_DATETIME));

    string message(32, 'a');
    string signature;
    SignMessage(message, &signature, KM_DIGEST_NONE);
    VerifyMessage(message, signature, KM_DIGEST_NONE);

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_EC))
        EXPECT_EQ(4, GetParam()->keymaster0_calls());
}

TEST_P(ImportKeyTest, EcdsaSizeMismatch) {
    string pk8_key = read_file("ec_privkey_pk8.der");
    ASSERT_EQ(138U, pk8_key.size());
    ASSERT_EQ(KM_ERROR_IMPORT_PARAMETER_MISMATCH,
              ImportKey(AuthorizationSetBuilder()
                            .EcdsaSigningKey(224 /* Doesn't match key */)
                            .Digest(KM_DIGEST_NONE),
                        KM_KEY_FORMAT_PKCS8, pk8_key));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(ImportKeyTest, AesKeySuccess) {
    char key_data[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    string key(key_data, sizeof(key_data));
    ASSERT_EQ(KM_ERROR_OK,
              ImportKey(AuthorizationSetBuilder().AesEncryptionKey(128).EcbMode().Authorization(
                            TAG_PADDING, KM_PAD_PKCS7),
                        KM_KEY_FORMAT_RAW, key));

    EXPECT_TRUE(contains(sw_enforced(), TAG_ORIGIN, KM_ORIGIN_IMPORTED));
    EXPECT_TRUE(contains(sw_enforced(), KM_TAG_CREATION_DATETIME));

    string message = "Hello World!";
    string ciphertext = EncryptMessage(message, KM_MODE_ECB, KM_PAD_PKCS7);
    string plaintext = DecryptMessage(ciphertext, KM_MODE_ECB, KM_PAD_PKCS7);
    EXPECT_EQ(message, plaintext);

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(ImportKeyTest, HmacSha256KeySuccess) {
    char key_data[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    string key(key_data, sizeof(key_data));
    ASSERT_EQ(KM_ERROR_OK, ImportKey(AuthorizationSetBuilder()
                                         .HmacKey(sizeof(key_data) * 8)
                                         .Digest(KM_DIGEST_SHA_2_256)
                                         .Authorization(TAG_MIN_MAC_LENGTH, 256),
                                     KM_KEY_FORMAT_RAW, key));

    EXPECT_TRUE(contains(sw_enforced(), TAG_ORIGIN, KM_ORIGIN_IMPORTED));
    EXPECT_TRUE(contains(sw_enforced(), KM_TAG_CREATION_DATETIME));

    string message = "Hello World!";
    string signature;
    MacMessage(message, &signature, 256);
    VerifyMac(message, signature);

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

string wrapped_key = hex2str(
    "3082017302010004820100A2B7988012A043CE83E762A4E4D3C86D578B2E1EA5E04138353114A816951308E3222AFA"
    "D86CA141C581198E65BC56D9EEDC5B555713BE2C20948DD076AB4980305871317F89DD3A7A67FBBB7AACF6941C06B2"
    "65396D894A6DCC7B9FB152FFA8CBF44FF8063748795F3FB506DF8718535289E759075A13A5DDC83EF8470549AA7794"
    "3AFBACF6CF82DCE3751E05BFE05F30B998D73E23611E6EAEFC2A497E097A895C4242607B472AE8F19DA77A9A5A4786"
    "75541FC813C9213B5CE8C2E598BFBBCD1B369E3D7AEC0274F9B79118D8AA5FBB7634EBD3C4C2AF3B5DA483DF2CFDF1"
    "E68A3BFC7B6C0D503AF88E82C9EE841A278B144FF8D39F2DB2ACE9415C120190040CD796B02C370F1FA4CC0124F130"
    "280201033023A1083106020100020101A203020120A30402020100A4053103020101A60531030201400420CCD54085"
    "5F833A5E1480BFD2D36FAF3AEEE15DF5BEABE2691BC82DDE2A7AA91004107CB81BDDCD09E8F4DF575726279F3229");

string wrapped_key_masked = hex2str(
    "30820173020100048201008CBEE0DC600215FFC85FC26B57DD2331DDF5D3E106C0A68BFEF167AFD428041D9B7C3316"
    "110BBB914A86FC24D4EF5C6A4673C9B3CC914C7806453650753B5130C4FE72264A52C1A270286032513F24EB3E033A"
    "BCC26A9D6AEFD0D0AD3E922E4E737ECDAD3C4DF2ABDB416378E67381BE0391175EC8F05FDFBC3794B7D0D88298010F"
    "E9B6F788BC049D874575D2D4C33DB582B113694738A9151BBC7603D3556B26FEC0279EE1C1CA44D6F7F91F4C424912"
    "7F9CC3232DE8B0AEFFD5AFAD4C3D5B846FD26873315606F6457BC19447FD7C6431550D6E6592A0555E61C7A021D149"
    "BCEE7A858DD6D4A8E230C6015EEDF0A58F4CAA8A6D0E3A1E3794CAEE7854CE92040C6D9721D08589581AB49204A330"
    "280201033023A1083106020100020101A203020120A30402020100A4053103020101A60531030201400420A61C6E24"
    "7E25B3E6E69AA78EB03C2D4AC20D1F99A9A024A76F35C8E2CAB9B68D04101FF7A0E793B9EE4AECEBB9AC4C545254");

string wrapping_key = hex2str(
    "308204be020100300d06092a864886f70d0101010500048204a8308204a40201000282010100aec367931d8900ce56"
    "b0067f7d70e1fc653f3f34d194c1fed50018fb43db937b06e673a837313d56b1c725150a3fef86acbddc41bb759c28"
    "54eae32d35841efb5c18d82bc90a1cb5c1d55adf245b02911f0b7cda88c421ff0ebafe7c0d23be312d7bd5921ffaea"
    "1347c157406fef718f682643e4e5d33c6703d61c0cf7ac0bf4645c11f5c1374c3886427411c449796792e0bef75dec"
    "858a2123c36753e02a95a96d7c454b504de385a642e0dfc3e60ac3a7ee4991d0d48b0172a95f9536f02ba13cecccb9"
    "2b727db5c27e5b2f5cec09600b286af5cf14c42024c61ddfe71c2a8d7458f185234cb00e01d282f10f8fc6721d2aed"
    "3f4833cca2bd8fa62821dd55020301000102820100431447b6251908112b1ee76f99f3711a52b6630960046c2de70d"
    "e188d833f8b8b91e4d785caeeeaf4f0f74414e2cda40641f7fe24f14c67a88959bdb27766df9e710b630a03adc683b"
    "5d2c43080e52bee71e9eaeb6de297a5fea1072070d181c822bccff087d63c940ba8a45f670feb29fb4484d1c95e6d2"
    "579ba02aae0a00900c3ebf490e3d2cd7ee8d0e20c536e4dc5a5097272888cddd7e91f228b1c4d7474c55b8fcd618c4"
    "a957bbddd5ad7407cc312d8d98a5caf7e08f4a0d6b45bb41c652659d5a5ba05b663737a8696281865ba20fbdd7f851"
    "e6c56e8cbe0ddbbf24dc03b2d2cb4c3d540fb0af52e034a2d06698b128e5f101e3b51a34f8d8b4f8618102818100de"
    "392e18d682c829266cc3454e1d6166242f32d9a1d10577753e904ea7d08bff841be5bac82a164c5970007047b8c517"
    "db8f8f84e37bd5988561bdf503d4dc2bdb38f885434ae42c355f725c9a60f91f0788e1f1a97223b524b5357fdf72e2"
    "f696bab7d78e32bf92ba8e1864eab1229e91346130748a6e3c124f9149d71c743502818100c95387c0f9d35f137b57"
    "d0d65c397c5e21cc251e47008ed62a542409c8b6b6ac7f8967b3863ca645fcce49582a9aa17349db6c4a95affdae0d"
    "ae612e1afac99ed39a2d934c880440aed8832f9843163a47f27f392199dc1202f9a0f9bd08308007cb1e4e7f583093"
    "66a7de25f7c3c9b880677c068e1be936e81288815252a8a102818057ff8ca1895080b2cae486ef0adfd791fb0235c0"
    "b8b36cd6c136e52e4085f4ea5a063212a4f105a3764743e53281988aba073f6e0027298e1c4378556e0efca0e14ece"
    "1af76ad0b030f27af6f0ab35fb73a060d8b1a0e142fa2647e93b32e36d8282ae0a4de50ab7afe85500a16f43a64719"
    "d6e2b9439823719cd08bcd03178102818100ba73b0bb28e3f81e9bd1c568713b101241acc607976c4ddccc90e65b65"
    "56ca31516058f92b6e09f3b160ff0e374ec40d78ae4d4979fde6ac06a1a400c61dd31254186af30b22c10582a8a43e"
    "34fe949c5f3b9755bae7baa7b7b7a6bd03b38cef55c86885fc6c1978b9cee7ef33da507c9df6b9277cff1e6aaa5d57"
    "aca528466102818100c931617c77829dfb1270502be9195c8f2830885f57dba869536811e6864236d0c4736a0008a1"
    "45af36b8357a7c3d139966d04c4e00934ea1aede3bb6b8ec841dc95e3f579751e2bfdfe27ae778983f959356210723"
    "287b0affcc9f727044d48c373f1babde0724fa17a4fd4da0902c7c9b9bf27ba61be6ad02dfddda8f4e6822");

string zero_masking_key =
    hex2str("0000000000000000000000000000000000000000000000000000000000000000");
string masking_key = hex2str("D796B02C370F1FA4CC0124F14EC8CBEBE987E825246265050F399A51FD477DFC");

class ImportWrappedKeyTest : public testing::Test {
  public:
    ImportWrappedKeyTest() : keymaster_(new PureSoftKeymasterContext(), 16) {}

  protected:
    void SetUp() override {
        ConfigureRequest configReq;
        configReq.os_version = kOsVersion;
        configReq.os_patchlevel = kOsPatchLevel;
        ConfigureResponse configRsp;
        keymaster_.Configure(configReq, &configRsp);
        EXPECT_EQ(KM_ERROR_OK, configRsp.error);
    }

    keymaster_error_t BeginOperation(keymaster_purpose_t purpose,
                                     const AuthorizationSet& input_set) {
        BeginOperationRequest req;
        req.purpose = purpose;
        req.SetKeyMaterial(blob_);
        req.additional_params = input_set;

        BeginOperationResponse rsp;
        keymaster_.BeginOperation(req, &rsp);
        op_handle_ = rsp.op_handle;

        return rsp.error;
    }

    keymaster_error_t FinishOperation(const string& input, string* output) {
        FinishOperationRequest req;
        req.op_handle = op_handle_;
        req.input.Reinitialize(input.data(), input.size());

        FinishOperationResponse rsp;
        keymaster_.FinishOperation(req, &rsp);

        if (output) {
            *output = string(reinterpret_cast<const char*>(rsp.output.peek_read()),
                             rsp.output.available_read());
        }

        return rsp.error;
    }

    string ProcessMessage(keymaster_purpose_t purpose, const string& message,
                          const AuthorizationSet& begin_params) {
        EXPECT_EQ(KM_ERROR_OK, BeginOperation(purpose, begin_params));

        string result;
        EXPECT_EQ(KM_ERROR_OK, FinishOperation(message, &result));
        return result;
    }

    AndroidKeymaster keymaster_;
    KeymasterKeyBlob blob_;
    uint64_t op_handle_;
};

TEST_F(ImportWrappedKeyTest, GoldenKeySuccess) {
    ImportKeyRequest import_request;

    auto import_params = AuthorizationSetBuilder()
                             .RsaEncryptionKey(2048, 65537)
                             .Digest(KM_DIGEST_SHA1)
                             .Padding(KM_PAD_RSA_OAEP)
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_WRAP)
                             .build();
    import_request.key_description.Reinitialize(import_params);
    import_request.SetKeyMaterial(reinterpret_cast<const uint8_t*>(wrapping_key.c_str()),
                                  wrapping_key.size());
    import_request.key_format = KM_KEY_FORMAT_PKCS8;
    ImportKeyResponse import_response;
    keymaster_.ImportKey(import_request, &import_response);
    ASSERT_EQ(import_response.error, KM_ERROR_OK);

    ImportWrappedKeyRequest request;
    KeymasterKeyBlob wrapped_key_blob(reinterpret_cast<const uint8_t*>(wrapped_key.c_str()),
                                      wrapped_key.size());
    request.SetKeyMaterial(wrapped_key_blob, import_response.key_blob);
    request.SetMaskingKeyMaterial(reinterpret_cast<const uint8_t*>(zero_masking_key.c_str()),
                                  zero_masking_key.size());
    ImportWrappedKeyResponse response;

    keymaster_.ImportWrappedKey(request, &response);

    EXPECT_EQ(response.error, KM_ERROR_OK);
    // Check that the tags from the wrapped auth list were imported correctly
    ASSERT_EQ(response.key_blob.key_material_size > 0, true);
    ASSERT_EQ(response.unenforced.Contains(TAG_ALGORITHM), true);
    ASSERT_EQ(response.unenforced.Contains(TAG_KEY_SIZE), true);
    ASSERT_EQ(response.unenforced.Contains(TAG_PURPOSE), true);
    ASSERT_EQ(response.unenforced.Contains(TAG_BLOCK_MODE), true);

    blob_ = move(response.key_blob);

    string message = "Hello World!";
    auto params = AuthorizationSetBuilder().BlockMode(KM_MODE_ECB).Padding(KM_PAD_PKCS7).build();
    string ciphertext = ProcessMessage(KM_PURPOSE_ENCRYPT, message, params);
    string plaintext = ProcessMessage(KM_PURPOSE_DECRYPT, ciphertext, params);

    EXPECT_EQ(message, plaintext);
}

TEST_F(ImportWrappedKeyTest, SuccessMaskingKey) {
    ImportKeyRequest import_request;

    auto import_params = AuthorizationSetBuilder()
                             .RsaEncryptionKey(2048, 65537)
                             .Digest(KM_DIGEST_SHA1)
                             .Padding(KM_PAD_RSA_OAEP)
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_WRAP)
                             .build();
    import_request.key_description.Reinitialize(import_params);
    import_request.SetKeyMaterial(reinterpret_cast<const uint8_t*>(wrapping_key.c_str()),
                                  wrapping_key.size());

    import_request.key_format = KM_KEY_FORMAT_PKCS8;
    ImportKeyResponse import_response;
    keymaster_.ImportKey(import_request, &import_response);
    EXPECT_EQ(import_response.error, KM_ERROR_OK);

    if (import_response.error != KM_ERROR_OK) return;

    ImportWrappedKeyRequest request;
    KeymasterKeyBlob wrapped_key_blob(reinterpret_cast<const uint8_t*>(wrapped_key_masked.c_str()),
                                      wrapped_key_masked.size());
    request.SetKeyMaterial(wrapped_key_blob, import_response.key_blob);
    request.SetMaskingKeyMaterial(reinterpret_cast<const uint8_t*>(masking_key.c_str()),
                                  masking_key.size());
    ImportWrappedKeyResponse response;

    keymaster_.ImportWrappedKey(request, &response);
    EXPECT_EQ(response.error, KM_ERROR_OK);
}

TEST_F(ImportWrappedKeyTest, WrongMaskingKey) {
    ImportKeyRequest import_request;

    auto import_params = AuthorizationSetBuilder()
                             .RsaEncryptionKey(2048, 65537)
                             .Digest(KM_DIGEST_SHA1)
                             .Padding(KM_PAD_RSA_OAEP)
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_WRAP)
                             .build();
    import_request.key_description.Reinitialize(import_params);
    import_request.SetKeyMaterial(reinterpret_cast<const uint8_t*>(wrapping_key.c_str()),
                                  wrapping_key.size());

    import_request.key_format = KM_KEY_FORMAT_PKCS8;
    ImportKeyResponse import_response;
    keymaster_.ImportKey(import_request, &import_response);
    EXPECT_EQ(import_response.error, KM_ERROR_OK);

    if (import_response.error != KM_ERROR_OK) return;

    ImportWrappedKeyRequest request;
    KeymasterKeyBlob wrapped_key_blob(reinterpret_cast<const uint8_t*>(wrapped_key_masked.c_str()),
                                      wrapped_key_masked.size());
    request.SetKeyMaterial(wrapped_key_blob, import_response.key_blob);
    request.SetMaskingKeyMaterial(reinterpret_cast<const uint8_t*>(zero_masking_key.c_str()),
                                  zero_masking_key.size());
    ImportWrappedKeyResponse response;

    keymaster_.ImportWrappedKey(request, &response);
    EXPECT_EQ(response.error, KM_ERROR_VERIFICATION_FAILED);
}

TEST_F(ImportWrappedKeyTest, WrongPurpose) {
    ImportKeyRequest import_request;

    auto import_params = AuthorizationSetBuilder()
                             .RsaEncryptionKey(2048, 65537)
                             .Digest(KM_DIGEST_SHA1)
                             .Padding(KM_PAD_RSA_OAEP)
                             .build();
    import_request.key_description.Reinitialize(import_params);
    import_request.SetKeyMaterial(reinterpret_cast<const uint8_t*>(wrapping_key.c_str()),
                                  wrapping_key.size());
    import_request.key_format = KM_KEY_FORMAT_PKCS8;
    ImportKeyResponse import_response;
    keymaster_.ImportKey(import_request, &import_response);
    EXPECT_EQ(import_response.error, KM_ERROR_OK);

    if (import_response.error != KM_ERROR_OK) return;

    ImportWrappedKeyRequest request;
    KeymasterKeyBlob wrapped_key_blob(reinterpret_cast<const uint8_t*>(wrapped_key.c_str()),
                                      wrapped_key.size());
    request.SetKeyMaterial(wrapped_key_blob, import_response.key_blob);
    ImportWrappedKeyResponse response;

    keymaster_.ImportWrappedKey(request, &response);
    EXPECT_EQ(response.error, KM_ERROR_INCOMPATIBLE_PURPOSE);
}

typedef Keymaster2Test EncryptionOperationsTest;
INSTANTIATE_TEST_CASE_P(AndroidKeymasterTest, EncryptionOperationsTest, test_params);

TEST_P(EncryptionOperationsTest, RsaNoPaddingSuccess) {
    ASSERT_EQ(KM_ERROR_OK,
              GenerateKey(AuthorizationSetBuilder().RsaEncryptionKey(256, 3).Padding(KM_PAD_NONE)));

    string message = "12345678901234567890123456789012";
    string ciphertext1 = EncryptMessage(string(message), KM_PAD_NONE);
    EXPECT_EQ(256U / 8, ciphertext1.size());

    string ciphertext2 = EncryptMessage(string(message), KM_PAD_NONE);
    EXPECT_EQ(256U / 8, ciphertext2.size());

    // Unpadded RSA is deterministic
    EXPECT_EQ(ciphertext1, ciphertext2);

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(3, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, RsaNoPaddingTooShort) {
    ASSERT_EQ(KM_ERROR_OK,
              GenerateKey(AuthorizationSetBuilder().RsaEncryptionKey(256, 3).Padding(KM_PAD_NONE)));

    string message = "1";

    string ciphertext = EncryptMessage(message, KM_PAD_NONE);
    EXPECT_EQ(256U / 8, ciphertext.size());

    string expected_plaintext = string(256 / 8 - 1, 0) + message;
    string plaintext = DecryptMessage(ciphertext, KM_PAD_NONE);

    EXPECT_EQ(expected_plaintext, plaintext);

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(4, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, RsaNoPaddingTooLong) {
    ASSERT_EQ(KM_ERROR_OK,
              GenerateKey(AuthorizationSetBuilder().RsaEncryptionKey(256, 3).Padding(KM_PAD_NONE)));

    string message = "123456789012345678901234567890123";

    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_PADDING, KM_PAD_NONE);
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_ENCRYPT, begin_params));

    string result;
    size_t input_consumed;
    EXPECT_EQ(KM_ERROR_INVALID_INPUT_LENGTH, UpdateOperation(message, &result, &input_consumed));

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(2, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, RsaNoPaddingLargerThanModulus) {
    ASSERT_EQ(KM_ERROR_OK,
              GenerateKey(AuthorizationSetBuilder().RsaEncryptionKey(256, 3).Padding(KM_PAD_NONE)));

    string exported;
    ASSERT_EQ(KM_ERROR_OK, ExportKey(KM_KEY_FORMAT_X509, &exported));

    const uint8_t* p = reinterpret_cast<const uint8_t*>(exported.data());
    unique_ptr<EVP_PKEY, EVP_PKEY_Delete> pkey(
        d2i_PUBKEY(nullptr /* alloc new */, &p, exported.size()));
    unique_ptr<RSA, RSA_Delete> rsa(EVP_PKEY_get1_RSA(pkey.get()));

    size_t modulus_len = BN_num_bytes(rsa->n);
    ASSERT_EQ(256U / 8, modulus_len);
    unique_ptr<uint8_t[]> modulus_buf(new uint8_t[modulus_len]);
    BN_bn2bin(rsa->n, modulus_buf.get());

    // The modulus is too big to encrypt.
    string message(reinterpret_cast<const char*>(modulus_buf.get()), modulus_len);

    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_PADDING, KM_PAD_NONE);
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_ENCRYPT, begin_params));

    string result;
    size_t input_consumed;
    EXPECT_EQ(KM_ERROR_OK, UpdateOperation(message, &result, &input_consumed));
    EXPECT_EQ(KM_ERROR_INVALID_ARGUMENT, FinishOperation(&result));

    // One smaller than the modulus is okay.
    BN_sub(rsa->n, rsa->n, BN_value_one());
    modulus_len = BN_num_bytes(rsa->n);
    ASSERT_EQ(256U / 8, modulus_len);
    BN_bn2bin(rsa->n, modulus_buf.get());
    message = string(reinterpret_cast<const char*>(modulus_buf.get()), modulus_len);
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_ENCRYPT, begin_params));
    EXPECT_EQ(KM_ERROR_OK, FinishOperation(message, "", &result));

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(4, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, RsaOaepSuccess) {
    size_t key_size = 768;
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .RsaEncryptionKey(key_size, 3)
                                           .Padding(KM_PAD_RSA_OAEP)
                                           .Digest(KM_DIGEST_SHA_2_256)));

    string message = "Hello";
    string ciphertext1 = EncryptMessage(string(message), KM_DIGEST_SHA_2_256, KM_PAD_RSA_OAEP);
    EXPECT_EQ(key_size / 8, ciphertext1.size());

    string ciphertext2 = EncryptMessage(string(message), KM_DIGEST_SHA_2_256, KM_PAD_RSA_OAEP);
    EXPECT_EQ(key_size / 8, ciphertext2.size());

    // OAEP randomizes padding so every result should be different.
    EXPECT_NE(ciphertext1, ciphertext2);

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(3, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, RsaOaepSha224Success) {
    size_t key_size = 768;
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .RsaEncryptionKey(key_size, 3)
                                           .Padding(KM_PAD_RSA_OAEP)
                                           .Digest(KM_DIGEST_SHA_2_224)));

    string message = "Hello";
    string ciphertext1 = EncryptMessage(string(message), KM_DIGEST_SHA_2_224, KM_PAD_RSA_OAEP);
    EXPECT_EQ(key_size / 8, ciphertext1.size());

    string ciphertext2 = EncryptMessage(string(message), KM_DIGEST_SHA_2_224, KM_PAD_RSA_OAEP);
    EXPECT_EQ(key_size / 8, ciphertext2.size());

    // OAEP randomizes padding so every result should be different.
    EXPECT_NE(ciphertext1, ciphertext2);

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(3, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, RsaOaepRoundTrip) {
    size_t key_size = 768;
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .RsaEncryptionKey(key_size, 3)
                                           .Padding(KM_PAD_RSA_OAEP)
                                           .Digest(KM_DIGEST_SHA_2_256)));
    string message = "Hello World!";
    string ciphertext = EncryptMessage(string(message), KM_DIGEST_SHA_2_256, KM_PAD_RSA_OAEP);
    EXPECT_EQ(key_size / 8, ciphertext.size());

    string plaintext = DecryptMessage(ciphertext, KM_DIGEST_SHA_2_256, KM_PAD_RSA_OAEP);
    EXPECT_EQ(message, plaintext);

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(4, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, RsaOaepSha224RoundTrip) {
    size_t key_size = 768;
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .RsaEncryptionKey(key_size, 3)
                                           .Padding(KM_PAD_RSA_OAEP)
                                           .Digest(KM_DIGEST_SHA_2_224)));
    string message = "Hello World!";
    string ciphertext = EncryptMessage(string(message), KM_DIGEST_SHA_2_224, KM_PAD_RSA_OAEP);
    EXPECT_EQ(key_size / 8, ciphertext.size());

    string plaintext = DecryptMessage(ciphertext, KM_DIGEST_SHA_2_224, KM_PAD_RSA_OAEP);
    EXPECT_EQ(message, plaintext);

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(4, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, RsaOaepInvalidDigest) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .RsaEncryptionKey(512, 3)
                                           .Padding(KM_PAD_RSA_OAEP)
                                           .Digest(KM_DIGEST_NONE)));
    string message = "Hello World!";

    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_PADDING, KM_PAD_RSA_OAEP);
    begin_params.push_back(TAG_DIGEST, KM_DIGEST_NONE);
    EXPECT_EQ(KM_ERROR_INCOMPATIBLE_DIGEST, BeginOperation(KM_PURPOSE_ENCRYPT, begin_params));

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(2, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, RsaOaepUnauthorizedDigest) {
    if (GetParam()->minimal_digest_set())
        // We don't have two supported digests, so we can't try authorizing one and using another.
        return;

    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .RsaEncryptionKey(512, 3)
                                           .Padding(KM_PAD_RSA_OAEP)
                                           .Digest(KM_DIGEST_SHA_2_256)));
    string message = "Hello World!";
    // Works because encryption is a public key operation.
    EncryptMessage(string(message), KM_DIGEST_SHA1, KM_PAD_RSA_OAEP);

    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_PADDING, KM_PAD_RSA_OAEP);
    begin_params.push_back(TAG_DIGEST, KM_DIGEST_SHA1);
    EXPECT_EQ(KM_ERROR_INCOMPATIBLE_DIGEST, BeginOperation(KM_PURPOSE_DECRYPT, begin_params));

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(3, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, RsaOaepDecryptWithWrongDigest) {
    if (GetParam()->minimal_digest_set())
        // We don't have two supported digests, so we can't try encrypting with one and decrypting
        // with another.
        return;

    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .RsaEncryptionKey(768, 3)
                                           .Padding(KM_PAD_RSA_OAEP)
                                           .Digest(KM_DIGEST_SHA_2_256)
                                           .Digest(KM_DIGEST_SHA_2_384)));
    string message = "Hello World!";
    string ciphertext = EncryptMessage(string(message), KM_DIGEST_SHA_2_256, KM_PAD_RSA_OAEP);

    string result;
    size_t input_consumed;
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_PADDING, KM_PAD_RSA_OAEP);
    begin_params.push_back(TAG_DIGEST, KM_DIGEST_SHA_2_384);
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_DECRYPT, begin_params));
    EXPECT_EQ(KM_ERROR_OK, UpdateOperation(ciphertext, &result, &input_consumed));
    EXPECT_EQ(KM_ERROR_UNKNOWN_ERROR, FinishOperation(&result));
    EXPECT_EQ(0U, result.size());

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(4, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, RsaOaepTooLarge) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .RsaEncryptionKey(512, 3)
                                           .Padding(KM_PAD_RSA_OAEP)
                                           .Digest(KM_DIGEST_SHA1)));
    string message = "12345678901234567890123";
    string result;
    size_t input_consumed;

    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_PADDING, KM_PAD_RSA_OAEP);
    begin_params.push_back(TAG_DIGEST, KM_DIGEST_SHA1);
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_ENCRYPT, begin_params));
    EXPECT_EQ(KM_ERROR_OK, UpdateOperation(message, &result, &input_consumed));
    EXPECT_EQ(KM_ERROR_INVALID_ARGUMENT, FinishOperation(&result));
    EXPECT_EQ(0U, result.size());

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(2, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, RsaOaepCorruptedDecrypt) {
    size_t key_size = 768;
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .RsaEncryptionKey(768, 3)
                                           .Padding(KM_PAD_RSA_OAEP)
                                           .Digest(KM_DIGEST_SHA_2_256)));
    string message = "Hello World!";
    string ciphertext = EncryptMessage(string(message), KM_DIGEST_SHA_2_256, KM_PAD_RSA_OAEP);
    EXPECT_EQ(key_size / 8, ciphertext.size());

    // Corrupt the ciphertext
    ciphertext[key_size / 8 / 2]++;

    string result;
    size_t input_consumed;
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_PADDING, KM_PAD_RSA_OAEP);
    begin_params.push_back(TAG_DIGEST, KM_DIGEST_SHA_2_256);
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_DECRYPT, begin_params));
    EXPECT_EQ(KM_ERROR_OK, UpdateOperation(ciphertext, &result, &input_consumed));
    EXPECT_EQ(KM_ERROR_UNKNOWN_ERROR, FinishOperation(&result));
    EXPECT_EQ(0U, result.size());

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(4, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, RsaPkcs1Success) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder().RsaEncryptionKey(512, 3).Padding(
                               KM_PAD_RSA_PKCS1_1_5_ENCRYPT)));
    string message = "Hello World!";
    string ciphertext1 = EncryptMessage(message, KM_PAD_RSA_PKCS1_1_5_ENCRYPT);
    EXPECT_EQ(512U / 8, ciphertext1.size());

    string ciphertext2 = EncryptMessage(message, KM_PAD_RSA_PKCS1_1_5_ENCRYPT);
    EXPECT_EQ(512U / 8, ciphertext2.size());

    // PKCS1 v1.5 randomizes padding so every result should be different.
    EXPECT_NE(ciphertext1, ciphertext2);

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(3, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, RsaPkcs1RoundTrip) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder().RsaEncryptionKey(512, 3).Padding(
                               KM_PAD_RSA_PKCS1_1_5_ENCRYPT)));
    string message = "Hello World!";
    string ciphertext = EncryptMessage(message, KM_PAD_RSA_PKCS1_1_5_ENCRYPT);
    EXPECT_EQ(512U / 8, ciphertext.size());

    string plaintext = DecryptMessage(ciphertext, KM_PAD_RSA_PKCS1_1_5_ENCRYPT);
    EXPECT_EQ(message, plaintext);

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(4, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, RsaRoundTripAllCombinations) {
    size_t key_size = 2048;
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .RsaEncryptionKey(key_size, 3)
                                           .Padding(KM_PAD_RSA_PKCS1_1_5_ENCRYPT)
                                           .Padding(KM_PAD_RSA_OAEP)
                                           .Digest(KM_DIGEST_NONE)
                                           .Digest(KM_DIGEST_MD5)
                                           .Digest(KM_DIGEST_SHA1)
                                           .Digest(KM_DIGEST_SHA_2_224)
                                           .Digest(KM_DIGEST_SHA_2_256)
                                           .Digest(KM_DIGEST_SHA_2_384)
                                           .Digest(KM_DIGEST_SHA_2_512)));

    string message = "Hello World!";

    keymaster_padding_t padding_modes[] = {KM_PAD_RSA_OAEP, KM_PAD_RSA_PKCS1_1_5_ENCRYPT};
    keymaster_digest_t digests[] = {
        KM_DIGEST_NONE,      KM_DIGEST_MD5,       KM_DIGEST_SHA1,      KM_DIGEST_SHA_2_224,
        KM_DIGEST_SHA_2_256, KM_DIGEST_SHA_2_384, KM_DIGEST_SHA_2_512,
    };

    for (auto padding : padding_modes)
        for (auto digest : digests) {
            if (padding == KM_PAD_RSA_OAEP && digest == KM_DIGEST_NONE)
                // OAEP requires a digest.
                continue;

            string ciphertext = EncryptMessage(message, digest, padding);
            EXPECT_EQ(key_size / 8, ciphertext.size());

            string plaintext = DecryptMessage(ciphertext, digest, padding);
            EXPECT_EQ(message, plaintext);
        }

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(40, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, RsaPkcs1TooLarge) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder().RsaEncryptionKey(512, 3).Padding(
                               KM_PAD_RSA_PKCS1_1_5_ENCRYPT)));
    string message = "123456789012345678901234567890123456789012345678901234";
    string result;
    size_t input_consumed;

    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_PADDING, KM_PAD_RSA_PKCS1_1_5_ENCRYPT);
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_ENCRYPT, begin_params));
    EXPECT_EQ(KM_ERROR_OK, UpdateOperation(message, &result, &input_consumed));
    EXPECT_EQ(KM_ERROR_INVALID_ARGUMENT, FinishOperation(&result));
    EXPECT_EQ(0U, result.size());

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(2, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, RsaPkcs1CorruptedDecrypt) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder().RsaEncryptionKey(512, 3).Padding(
                               KM_PAD_RSA_PKCS1_1_5_ENCRYPT)));
    string message = "Hello World!";
    string ciphertext = EncryptMessage(string(message), KM_PAD_RSA_PKCS1_1_5_ENCRYPT);
    EXPECT_EQ(512U / 8, ciphertext.size());

    // Corrupt the ciphertext
    ciphertext[512 / 8 / 2]++;

    string result;
    size_t input_consumed;
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_PADDING, KM_PAD_RSA_PKCS1_1_5_ENCRYPT);
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_DECRYPT, begin_params));
    EXPECT_EQ(KM_ERROR_OK, UpdateOperation(ciphertext, &result, &input_consumed));
    EXPECT_EQ(KM_ERROR_UNKNOWN_ERROR, FinishOperation(&result));
    EXPECT_EQ(0U, result.size());

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(4, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, RsaEncryptWithSigningKey) {
    ASSERT_EQ(KM_ERROR_OK,
              GenerateKey(AuthorizationSetBuilder().RsaSigningKey(256, 3).Padding(KM_PAD_NONE)));

    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_PADDING, KM_PAD_NONE);
    ASSERT_EQ(KM_ERROR_INCOMPATIBLE_PURPOSE, BeginOperation(KM_PURPOSE_DECRYPT, begin_params));

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(2, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, EcdsaEncrypt) {
    ASSERT_EQ(KM_ERROR_OK,
              GenerateKey(AuthorizationSetBuilder().EcdsaSigningKey(224).Digest(KM_DIGEST_NONE)));
    ASSERT_EQ(KM_ERROR_UNSUPPORTED_PURPOSE, BeginOperation(KM_PURPOSE_ENCRYPT));
    ASSERT_EQ(KM_ERROR_UNSUPPORTED_PURPOSE, BeginOperation(KM_PURPOSE_DECRYPT));

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_EC))
        EXPECT_EQ(3, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, HmacEncrypt) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .HmacKey(128)
                                           .Digest(KM_DIGEST_SHA_2_256)
                                           .Padding(KM_PAD_NONE)
                                           .Authorization(TAG_MIN_MAC_LENGTH, 128)));
    ASSERT_EQ(KM_ERROR_UNSUPPORTED_PURPOSE, BeginOperation(KM_PURPOSE_ENCRYPT));
    ASSERT_EQ(KM_ERROR_UNSUPPORTED_PURPOSE, BeginOperation(KM_PURPOSE_DECRYPT));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, AesEcbRoundTripSuccess) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .AesEncryptionKey(128)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_ECB)
                                           .Padding(KM_PAD_NONE)));
    // Two-block message.
    string message = "12345678901234567890123456789012";
    string ciphertext1 = EncryptMessage(message, KM_MODE_ECB, KM_PAD_NONE);
    EXPECT_EQ(message.size(), ciphertext1.size());

    string ciphertext2 = EncryptMessage(string(message), KM_MODE_ECB, KM_PAD_NONE);
    EXPECT_EQ(message.size(), ciphertext2.size());

    // ECB is deterministic.
    EXPECT_EQ(ciphertext1, ciphertext2);

    string plaintext = DecryptMessage(ciphertext1, KM_MODE_ECB, KM_PAD_NONE);
    EXPECT_EQ(message, plaintext);

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, AesEcbNotAuthorized) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .AesEncryptionKey(128)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_CBC)
                                           .Padding(KM_PAD_NONE)));
    // Two-block message.
    string message = "12345678901234567890123456789012";
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_BLOCK_MODE, KM_MODE_ECB);
    begin_params.push_back(TAG_PADDING, KM_PAD_NONE);
    EXPECT_EQ(KM_ERROR_INCOMPATIBLE_BLOCK_MODE, BeginOperation(KM_PURPOSE_ENCRYPT, begin_params));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, AesEcbNoPaddingWrongInputSize) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .AesEncryptionKey(128)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_ECB)
                                           .Padding(KM_PAD_NONE)));
    // Message is slightly shorter than two blocks.
    string message = "1234567890123456789012345678901";

    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_BLOCK_MODE, KM_MODE_ECB);
    begin_params.push_back(TAG_PADDING, KM_PAD_NONE);
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_ENCRYPT, begin_params));
    string ciphertext;
    EXPECT_EQ(KM_ERROR_INVALID_INPUT_LENGTH, FinishOperation(message, "", &ciphertext));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, AesEcbPkcs7Padding) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .AesEncryptionKey(128)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_ECB)
                                           .Authorization(TAG_PADDING, KM_PAD_PKCS7)));

    // Try various message lengths; all should work.
    for (size_t i = 0; i < 32; ++i) {
        string message(i, 'a');
        string ciphertext = EncryptMessage(message, KM_MODE_ECB, KM_PAD_PKCS7);
        EXPECT_EQ(i + 16 - (i % 16), ciphertext.size());
        string plaintext = DecryptMessage(ciphertext, KM_MODE_ECB, KM_PAD_PKCS7);
        EXPECT_EQ(message, plaintext);
    }

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, AesEcbNoPaddingKeyWithPkcs7Padding) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .AesEncryptionKey(128)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_ECB)
                                           .Authorization(TAG_PADDING, KM_PAD_NONE)));

    // Try various message lengths; all should fail.
    for (size_t i = 0; i < 32; ++i) {
        AuthorizationSet begin_params(client_params());
        begin_params.push_back(TAG_BLOCK_MODE, KM_MODE_ECB);
        begin_params.push_back(TAG_PADDING, KM_PAD_PKCS7);
        EXPECT_EQ(KM_ERROR_INCOMPATIBLE_PADDING_MODE,
                  BeginOperation(KM_PURPOSE_ENCRYPT, begin_params));
    }

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, AesEcbPkcs7PaddingCorrupted) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .AesEncryptionKey(128)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_ECB)
                                           .Authorization(TAG_PADDING, KM_PAD_PKCS7)));

    string message = "a";
    string ciphertext = EncryptMessage(message, KM_MODE_ECB, KM_PAD_PKCS7);
    EXPECT_EQ(16U, ciphertext.size());
    EXPECT_NE(ciphertext, message);
    ++ciphertext[ciphertext.size() / 2];

    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_BLOCK_MODE, KM_MODE_ECB);
    begin_params.push_back(TAG_PADDING, KM_PAD_PKCS7);
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_DECRYPT, begin_params));
    string plaintext;
    size_t input_consumed;
    EXPECT_EQ(KM_ERROR_OK, UpdateOperation(ciphertext, &plaintext, &input_consumed));
    EXPECT_EQ(ciphertext.size(), input_consumed);
    EXPECT_EQ(KM_ERROR_INVALID_ARGUMENT, FinishOperation(&plaintext));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, AesCtrRoundTripSuccess) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .AesEncryptionKey(128)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_CTR)
                                           .Padding(KM_PAD_NONE)));
    string message = "123";
    string iv1;
    string ciphertext1 = EncryptMessage(message, KM_MODE_CTR, KM_PAD_NONE, &iv1);
    EXPECT_EQ(message.size(), ciphertext1.size());
    EXPECT_EQ(16U, iv1.size());

    string iv2;
    string ciphertext2 = EncryptMessage(message, KM_MODE_CTR, KM_PAD_NONE, &iv2);
    EXPECT_EQ(message.size(), ciphertext2.size());
    EXPECT_EQ(16U, iv2.size());

    // IVs should be random, so ciphertexts should differ.
    EXPECT_NE(iv1, iv2);
    EXPECT_NE(ciphertext1, ciphertext2);

    string plaintext = DecryptMessage(ciphertext1, KM_MODE_CTR, KM_PAD_NONE, iv1);
    EXPECT_EQ(message, plaintext);

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, AesCtrIncremental) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .AesEncryptionKey(128)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_CTR)
                                           .Padding(KM_PAD_NONE)));

    int increment = 15;
    string message(239, 'a');
    AuthorizationSet input_params(client_params());
    input_params.push_back(TAG_BLOCK_MODE, KM_MODE_CTR);
    input_params.push_back(TAG_PADDING, KM_PAD_NONE);
    AuthorizationSet output_params;
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_ENCRYPT, input_params, &output_params));

    string ciphertext;
    size_t input_consumed;
    for (size_t i = 0; i < message.size(); i += increment)
        EXPECT_EQ(KM_ERROR_OK,
                  UpdateOperation(message.substr(i, increment), &ciphertext, &input_consumed));
    EXPECT_EQ(KM_ERROR_OK, FinishOperation(&ciphertext));
    EXPECT_EQ(message.size(), ciphertext.size());

    // Move TAG_NONCE into input_params
    input_params.Reinitialize(output_params);
    input_params.push_back(client_params());
    input_params.push_back(TAG_BLOCK_MODE, KM_MODE_CTR);
    input_params.push_back(TAG_PADDING, KM_PAD_NONE);
    output_params.Clear();

    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_DECRYPT, input_params, &output_params));
    string plaintext;
    for (size_t i = 0; i < ciphertext.size(); i += increment)
        EXPECT_EQ(KM_ERROR_OK,
                  UpdateOperation(ciphertext.substr(i, increment), &plaintext, &input_consumed));
    EXPECT_EQ(KM_ERROR_OK, FinishOperation(&plaintext));
    EXPECT_EQ(ciphertext.size(), plaintext.size());
    EXPECT_EQ(message, plaintext);

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

struct AesCtrSp80038aTestVector {
    const char* key;
    const char* nonce;
    const char* plaintext;
    const char* ciphertext;
};

// These test vectors are taken from
// http://csrc.nist.gov/publications/nistpubs/800-38a/sp800-38a.pdf, section F.5.
static const AesCtrSp80038aTestVector kAesCtrSp80038aTestVectors[] = {
    // AES-128
    {
        "2b7e151628aed2a6abf7158809cf4f3c", "f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff",
        "6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e51"
        "30c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710",
        "874d6191b620e3261bef6864990db6ce9806f66b7970fdff8617187bb9fffdff"
        "5ae4df3edbd5d35e5b4f09020db03eab1e031dda2fbe03d1792170a0f3009cee",
    },
    // AES-192
    {
        "8e73b0f7da0e6452c810f32b809079e562f8ead2522c6b7b", "f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff",
        "6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e51"
        "30c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710",
        "1abc932417521ca24f2b0459fe7e6e0b090339ec0aa6faefd5ccc2c6f4ce8e94"
        "1e36b26bd1ebc670d1bd1d665620abf74f78a7f6d29809585a97daec58c6b050",
    },
    // AES-256
    {
        "603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4",
        "f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff",
        "6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e51"
        "30c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710",
        "601ec313775789a5b7a7f504bbf3d228f443e3ca4d62b59aca84e990cacaf5c5"
        "2b0930daa23de94ce87017ba2d84988ddfc9c58db67aada613c2dd08457941a6",
    },
};

TEST_P(EncryptionOperationsTest, AesCtrSp80038aTestVector) {
    for (size_t i = 0; i < 3; i++) {
        const AesCtrSp80038aTestVector& test(kAesCtrSp80038aTestVectors[i]);
        const string key = hex2str(test.key);
        const string nonce = hex2str(test.nonce);
        const string plaintext = hex2str(test.plaintext);
        const string ciphertext = hex2str(test.ciphertext);
        CheckAesCtrTestVector(key, nonce, plaintext, ciphertext);
    }

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, AesCtrInvalidPaddingMode) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .AesEncryptionKey(128)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_CTR)
                                           .Authorization(TAG_PADDING, KM_PAD_PKCS7)));
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_BLOCK_MODE, KM_MODE_CTR);
    begin_params.push_back(TAG_PADDING, KM_PAD_NONE);
    EXPECT_EQ(KM_ERROR_INCOMPATIBLE_PADDING_MODE, BeginOperation(KM_PURPOSE_ENCRYPT, begin_params));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, AesCtrInvalidCallerNonce) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .AesEncryptionKey(128)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_CTR)
                                           .Authorization(TAG_CALLER_NONCE)
                                           .Padding(KM_PAD_NONE)));

    AuthorizationSet input_params(client_params());
    input_params.push_back(TAG_BLOCK_MODE, KM_MODE_CTR);
    input_params.push_back(TAG_PADDING, KM_PAD_NONE);
    input_params.push_back(TAG_NONCE, "123", 3);
    EXPECT_EQ(KM_ERROR_INVALID_NONCE, BeginOperation(KM_PURPOSE_ENCRYPT, input_params));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, AesCbcRoundTripSuccess) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .AesEncryptionKey(128)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_CBC)
                                           .Padding(KM_PAD_NONE)));
    // Two-block message.
    string message = "12345678901234567890123456789012";
    string iv1;
    string ciphertext1 = EncryptMessage(message, KM_MODE_CBC, KM_PAD_NONE, &iv1);
    EXPECT_EQ(message.size(), ciphertext1.size());

    string iv2;
    string ciphertext2 = EncryptMessage(message, KM_MODE_CBC, KM_PAD_NONE, &iv2);
    EXPECT_EQ(message.size(), ciphertext2.size());

    // IVs should be random, so ciphertexts should differ.
    EXPECT_NE(iv1, iv2);
    EXPECT_NE(ciphertext1, ciphertext2);

    string plaintext = DecryptMessage(ciphertext1, KM_MODE_CBC, KM_PAD_NONE, iv1);
    EXPECT_EQ(message, plaintext);

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, AesCallerNonce) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .AesEncryptionKey(128)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_CBC)
                                           .Authorization(TAG_CALLER_NONCE)
                                           .Padding(KM_PAD_NONE)));
    string message = "12345678901234567890123456789012";
    string iv1;
    // Don't specify nonce, should get a random one.
    string ciphertext1 = EncryptMessage(message, KM_MODE_CBC, KM_PAD_NONE, &iv1);
    EXPECT_EQ(message.size(), ciphertext1.size());
    EXPECT_EQ(16U, iv1.size());

    string plaintext = DecryptMessage(ciphertext1, KM_MODE_CBC, KM_PAD_NONE, iv1);
    EXPECT_EQ(message, plaintext);

    // Now specify a nonce, should also work.
    AuthorizationSet input_params(client_params());
    AuthorizationSet update_params;
    AuthorizationSet output_params;
    input_params.push_back(TAG_NONCE, "abcdefghijklmnop", 16);
    input_params.push_back(TAG_BLOCK_MODE, KM_MODE_CBC);
    input_params.push_back(TAG_PADDING, KM_PAD_NONE);
    string ciphertext2 =
        ProcessMessage(KM_PURPOSE_ENCRYPT, message, input_params, update_params, &output_params);

    // Decrypt with correct nonce.
    plaintext = ProcessMessage(KM_PURPOSE_DECRYPT, ciphertext2, input_params, update_params,
                               &output_params);
    EXPECT_EQ(message, plaintext);

    // Now try with wrong nonce.
    input_params.Reinitialize(client_params());
    input_params.push_back(TAG_BLOCK_MODE, KM_MODE_CBC);
    input_params.push_back(TAG_PADDING, KM_PAD_NONE);
    input_params.push_back(TAG_NONCE, "aaaaaaaaaaaaaaaa", 16);
    plaintext = ProcessMessage(KM_PURPOSE_DECRYPT, ciphertext2, input_params, update_params,
                               &output_params);
    EXPECT_NE(message, plaintext);

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, AesCallerNonceProhibited) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .AesEncryptionKey(128)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_CBC)
                                           .Padding(KM_PAD_NONE)));

    string message = "12345678901234567890123456789012";
    string iv1;
    // Don't specify nonce, should get a random one.
    string ciphertext1 = EncryptMessage(message, KM_MODE_CBC, KM_PAD_NONE, &iv1);
    EXPECT_EQ(message.size(), ciphertext1.size());
    EXPECT_EQ(16U, iv1.size());

    string plaintext = DecryptMessage(ciphertext1, KM_MODE_CBC, KM_PAD_NONE, iv1);
    EXPECT_EQ(message, plaintext);

    // Now specify a nonce, should fail.
    AuthorizationSet input_params(client_params());
    AuthorizationSet update_params;
    AuthorizationSet output_params;
    input_params.push_back(TAG_NONCE, "abcdefghijklmnop", 16);
    input_params.push_back(TAG_BLOCK_MODE, KM_MODE_CBC);
    input_params.push_back(TAG_PADDING, KM_PAD_NONE);

    EXPECT_EQ(KM_ERROR_CALLER_NONCE_PROHIBITED,
              BeginOperation(KM_PURPOSE_ENCRYPT, input_params, &output_params));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, AesCbcIncrementalNoPadding) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .AesEncryptionKey(128)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_CBC)
                                           .Padding(KM_PAD_NONE)));

    int increment = 15;
    string message(240, 'a');
    AuthorizationSet input_params(client_params());
    input_params.push_back(TAG_BLOCK_MODE, KM_MODE_CBC);
    input_params.push_back(TAG_PADDING, KM_PAD_NONE);
    AuthorizationSet output_params;
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_ENCRYPT, input_params, &output_params));

    string ciphertext;
    size_t input_consumed;
    for (size_t i = 0; i < message.size(); i += increment)
        EXPECT_EQ(KM_ERROR_OK,
                  UpdateOperation(message.substr(i, increment), &ciphertext, &input_consumed));
    EXPECT_EQ(KM_ERROR_OK, FinishOperation(&ciphertext));
    EXPECT_EQ(message.size(), ciphertext.size());

    // Move TAG_NONCE into input_params
    input_params.Reinitialize(output_params);
    input_params.push_back(client_params());
    input_params.push_back(TAG_BLOCK_MODE, KM_MODE_CBC);
    input_params.push_back(TAG_PADDING, KM_PAD_NONE);
    output_params.Clear();

    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_DECRYPT, input_params, &output_params));
    string plaintext;
    for (size_t i = 0; i < ciphertext.size(); i += increment)
        EXPECT_EQ(KM_ERROR_OK,
                  UpdateOperation(ciphertext.substr(i, increment), &plaintext, &input_consumed));
    EXPECT_EQ(KM_ERROR_OK, FinishOperation(&plaintext));
    EXPECT_EQ(ciphertext.size(), plaintext.size());
    EXPECT_EQ(message, plaintext);

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, AesCbcPkcs7Padding) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .AesEncryptionKey(128)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_CBC)
                                           .Authorization(TAG_PADDING, KM_PAD_PKCS7)));

    // Try various message lengths; all should work.
    for (size_t i = 0; i < 32; ++i) {
        string message(i, 'a');
        string iv;
        string ciphertext = EncryptMessage(message, KM_MODE_CBC, KM_PAD_PKCS7, &iv);
        EXPECT_EQ(i + 16 - (i % 16), ciphertext.size());
        string plaintext = DecryptMessage(ciphertext, KM_MODE_CBC, KM_PAD_PKCS7, iv);
        EXPECT_EQ(message, plaintext);
    }

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, AesGcmRoundTripSuccess) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .AesEncryptionKey(128)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_GCM)
                                           .Authorization(TAG_PADDING, KM_PAD_NONE)
                                           .Authorization(TAG_MIN_MAC_LENGTH, 128)));
    string aad = "foobar";
    string message = "123456789012345678901234567890123456";
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_BLOCK_MODE, KM_MODE_GCM);
    begin_params.push_back(TAG_PADDING, KM_PAD_NONE);
    begin_params.push_back(TAG_MAC_LENGTH, 128);

    AuthorizationSet update_params;
    update_params.push_back(TAG_ASSOCIATED_DATA, aad.data(), aad.size());

    // Encrypt
    AuthorizationSet begin_out_params;
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_ENCRYPT, begin_params, &begin_out_params));
    string ciphertext;
    size_t input_consumed;
    AuthorizationSet update_out_params;
    EXPECT_EQ(KM_ERROR_OK, UpdateOperation(update_params, message, &update_out_params, &ciphertext,
                                           &input_consumed));
    EXPECT_EQ(message.size(), input_consumed);
    EXPECT_EQ(KM_ERROR_OK, FinishOperation(&ciphertext));

    // Grab nonce
    EXPECT_NE(-1, begin_out_params.find(TAG_NONCE));
    begin_params.push_back(begin_out_params);

    // Decrypt.
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_DECRYPT, begin_params));
    string plaintext;
    EXPECT_EQ(KM_ERROR_OK, UpdateOperation(update_params, ciphertext, &update_out_params,
                                           &plaintext, &input_consumed));
    EXPECT_EQ(ciphertext.size(), input_consumed);
    EXPECT_EQ(KM_ERROR_OK, FinishOperation(&plaintext));

    EXPECT_EQ(message, plaintext);
    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, AesGcmTooShortTag) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .AesEncryptionKey(128)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_GCM)
                                           .Authorization(TAG_PADDING, KM_PAD_NONE)
                                           .Authorization(TAG_MIN_MAC_LENGTH, 128)));
    string aad = "foobar";
    string message = "123456789012345678901234567890123456";
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_BLOCK_MODE, KM_MODE_GCM);
    begin_params.push_back(TAG_PADDING, KM_PAD_NONE);
    begin_params.push_back(TAG_MAC_LENGTH, 96);

    AuthorizationSet update_params;
    update_params.push_back(TAG_ASSOCIATED_DATA, aad.data(), aad.size());

    AuthorizationSet begin_out_params;
    EXPECT_EQ(KM_ERROR_INVALID_MAC_LENGTH,
              BeginOperation(KM_PURPOSE_ENCRYPT, begin_params, &begin_out_params));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, AesGcmTooShortTagOnDecrypt) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .AesEncryptionKey(128)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_GCM)
                                           .Authorization(TAG_PADDING, KM_PAD_NONE)
                                           .Authorization(TAG_MIN_MAC_LENGTH, 128)));
    string aad = "foobar";
    string message = "123456789012345678901234567890123456";
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_BLOCK_MODE, KM_MODE_GCM);
    begin_params.push_back(TAG_PADDING, KM_PAD_NONE);
    begin_params.push_back(TAG_MAC_LENGTH, 128);

    AuthorizationSet update_params;
    update_params.push_back(TAG_ASSOCIATED_DATA, aad.data(), aad.size());

    // Encrypt
    AuthorizationSet begin_out_params;
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_ENCRYPT, begin_params, &begin_out_params));
    string ciphertext;
    size_t input_consumed;
    AuthorizationSet update_out_params;
    EXPECT_EQ(KM_ERROR_OK, UpdateOperation(update_params, message, &update_out_params, &ciphertext,
                                           &input_consumed));
    EXPECT_EQ(message.size(), input_consumed);
    EXPECT_EQ(KM_ERROR_OK, FinishOperation(&ciphertext));

    // Grab nonce
    EXPECT_NE(-1, begin_out_params.find(TAG_NONCE));
    begin_params.Reinitialize(client_params());
    begin_params.push_back(begin_out_params);
    begin_params.push_back(TAG_BLOCK_MODE, KM_MODE_GCM);
    begin_params.push_back(TAG_PADDING, KM_PAD_NONE);
    begin_params.push_back(TAG_MAC_LENGTH, 96);

    // Decrypt.
    EXPECT_EQ(KM_ERROR_INVALID_MAC_LENGTH, BeginOperation(KM_PURPOSE_DECRYPT, begin_params));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, AesGcmCorruptKey) {
    uint8_t nonce[] = {
        0xb7, 0x94, 0x37, 0xae, 0x08, 0xff, 0x35, 0x5d, 0x7d, 0x8a, 0x4d, 0x0f,
    };
    uint8_t ciphertext[] = {
        0xb3, 0xf6, 0x79, 0x9e, 0x8f, 0x93, 0x26, 0xf2, 0xdf, 0x1e, 0x80, 0xfc, 0xd2, 0xcb, 0x16,
        0xd7, 0x8c, 0x9d, 0xc7, 0xcc, 0x14, 0xbb, 0x67, 0x78, 0x62, 0xdc, 0x6c, 0x63, 0x9b, 0x3a,
        0x63, 0x38, 0xd2, 0x4b, 0x31, 0x2d, 0x39, 0x89, 0xe5, 0x92, 0x0b, 0x5d, 0xbf, 0xc9, 0x76,
        0x76, 0x5e, 0xfb, 0xfe, 0x57, 0xbb, 0x38, 0x59, 0x40, 0xa7, 0xa4, 0x3b, 0xdf, 0x05, 0xbd,
        0xda, 0xe3, 0xc9, 0xd6, 0xa2, 0xfb, 0xbd, 0xfc, 0xc0, 0xcb, 0xa0,
    };
    string ciphertext_str(reinterpret_cast<char*>(ciphertext), sizeof(ciphertext));

    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_BLOCK_MODE, KM_MODE_GCM);
    begin_params.push_back(TAG_PADDING, KM_PAD_NONE);
    begin_params.push_back(TAG_MAC_LENGTH, 128);
    begin_params.push_back(TAG_NONCE, nonce, sizeof(nonce));

    string plaintext;
    size_t input_consumed;

    // Import correct key and decrypt
    uint8_t good_key[] = {
        0xba, 0x76, 0x35, 0x4f, 0x0a, 0xed, 0x6e, 0x8d,
        0x91, 0xf4, 0x5c, 0x4f, 0xf5, 0xa0, 0x62, 0xdb,
    };
    string good_key_str(reinterpret_cast<char*>(good_key), sizeof(good_key));
    ASSERT_EQ(KM_ERROR_OK, ImportKey(AuthorizationSetBuilder()
                                         .AesEncryptionKey(128)
                                         .Authorization(TAG_BLOCK_MODE, KM_MODE_GCM)
                                         .Authorization(TAG_PADDING, KM_PAD_NONE)
                                         .Authorization(TAG_CALLER_NONCE)
                                         .Authorization(TAG_MIN_MAC_LENGTH, 128),
                                     KM_KEY_FORMAT_RAW, good_key_str));
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_DECRYPT, begin_params));
    EXPECT_EQ(KM_ERROR_OK, UpdateOperation(ciphertext_str, &plaintext, &input_consumed));
    EXPECT_EQ(KM_ERROR_OK, FinishOperation(&plaintext));

    // Import bad key and decrypt
    uint8_t bad_key[] = {
        0xbb, 0x76, 0x35, 0x4f, 0x0a, 0xed, 0x6e, 0x8d,
        0x91, 0xf4, 0x5c, 0x4f, 0xf5, 0xa0, 0x62, 0xdb,
    };
    string bad_key_str(reinterpret_cast<char*>(bad_key), sizeof(bad_key));
    ASSERT_EQ(KM_ERROR_OK, ImportKey(AuthorizationSetBuilder()
                                         .AesEncryptionKey(128)
                                         .Authorization(TAG_BLOCK_MODE, KM_MODE_GCM)
                                         .Authorization(TAG_PADDING, KM_PAD_NONE)
                                         .Authorization(TAG_MIN_MAC_LENGTH, 128),
                                     KM_KEY_FORMAT_RAW, bad_key_str));
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_DECRYPT, begin_params));
    EXPECT_EQ(KM_ERROR_OK, UpdateOperation(ciphertext_str, &plaintext, &input_consumed));
    EXPECT_EQ(KM_ERROR_VERIFICATION_FAILED, FinishOperation(&plaintext));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, AesGcmAadNoData) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .AesEncryptionKey(128)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_GCM)
                                           .Authorization(TAG_PADDING, KM_PAD_NONE)
                                           .Authorization(TAG_MIN_MAC_LENGTH, 128)));
    string aad = "123456789012345678";
    string empty_message;
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_BLOCK_MODE, KM_MODE_GCM);
    begin_params.push_back(TAG_PADDING, KM_PAD_NONE);
    begin_params.push_back(TAG_MAC_LENGTH, 128);

    AuthorizationSet update_params;
    update_params.push_back(TAG_ASSOCIATED_DATA, aad.data(), aad.size());

    // Encrypt
    AuthorizationSet begin_out_params;
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_ENCRYPT, begin_params, &begin_out_params));
    string ciphertext;
    size_t input_consumed;
    AuthorizationSet update_out_params;
    EXPECT_EQ(KM_ERROR_OK, UpdateOperation(update_params, empty_message, &update_out_params,
                                           &ciphertext, &input_consumed));
    EXPECT_EQ(0U, input_consumed);
    EXPECT_EQ(KM_ERROR_OK, FinishOperation(&ciphertext));

    // Grab nonce
    EXPECT_NE(-1, begin_out_params.find(TAG_NONCE));
    begin_params.push_back(begin_out_params);

    // Decrypt.
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_DECRYPT, begin_params));
    string plaintext;
    EXPECT_EQ(KM_ERROR_OK, UpdateOperation(update_params, ciphertext, &update_out_params,
                                           &plaintext, &input_consumed));
    EXPECT_EQ(ciphertext.size(), input_consumed);
    EXPECT_EQ(KM_ERROR_OK, FinishOperation(&plaintext));

    EXPECT_EQ(empty_message, plaintext);
    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, AesGcmIncremental) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .AesEncryptionKey(128)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_GCM)
                                           .Authorization(TAG_PADDING, KM_PAD_NONE)
                                           .Authorization(TAG_MIN_MAC_LENGTH, 128)));
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_BLOCK_MODE, KM_MODE_GCM);
    begin_params.push_back(TAG_PADDING, KM_PAD_NONE);
    begin_params.push_back(TAG_MAC_LENGTH, 128);

    AuthorizationSet update_params;
    update_params.push_back(TAG_ASSOCIATED_DATA, "b", 1);

    // Encrypt
    AuthorizationSet begin_out_params;
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_ENCRYPT, begin_params, &begin_out_params));
    string ciphertext;
    size_t input_consumed;
    AuthorizationSet update_out_params;

    // Send AAD, incrementally
    for (int i = 0; i < 1000; ++i) {
        EXPECT_EQ(KM_ERROR_OK, UpdateOperation(update_params, "", &update_out_params, &ciphertext,
                                               &input_consumed));
        EXPECT_EQ(0U, input_consumed);
        EXPECT_EQ(0U, ciphertext.size());
    }

    // Now send data, incrementally, no data.
    AuthorizationSet empty_params;
    for (int i = 0; i < 1000; ++i) {
        EXPECT_EQ(KM_ERROR_OK, UpdateOperation(empty_params, "a", &update_out_params, &ciphertext,
                                               &input_consumed));
        EXPECT_EQ(1U, input_consumed);
    }
    EXPECT_EQ(1000U, ciphertext.size());

    // And finish.
    EXPECT_EQ(KM_ERROR_OK, FinishOperation(&ciphertext));
    EXPECT_EQ(1016U, ciphertext.size());

    // Grab nonce
    EXPECT_NE(-1, begin_out_params.find(TAG_NONCE));
    begin_params.push_back(begin_out_params);

    // Decrypt.
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_DECRYPT, begin_params));
    string plaintext;

    // Send AAD, incrementally, no data
    for (int i = 0; i < 1000; ++i) {
        EXPECT_EQ(KM_ERROR_OK, UpdateOperation(update_params, "", &update_out_params, &plaintext,
                                               &input_consumed));
        EXPECT_EQ(0U, input_consumed);
        EXPECT_EQ(0U, plaintext.size());
    }

    // Now send data, incrementally.
    for (size_t i = 0; i < ciphertext.length(); ++i) {
        EXPECT_EQ(KM_ERROR_OK, UpdateOperation(empty_params, string(ciphertext.data() + i, 1),
                                               &update_out_params, &plaintext, &input_consumed));
        EXPECT_EQ(1U, input_consumed);
    }
    EXPECT_EQ(1000U, plaintext.size());
    EXPECT_EQ(KM_ERROR_OK, FinishOperation(&plaintext));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, AesGcmMultiPartAad) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .AesEncryptionKey(128)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_GCM)
                                           .Authorization(TAG_PADDING, KM_PAD_NONE)
                                           .Authorization(TAG_MIN_MAC_LENGTH, 128)));
    string message = "123456789012345678901234567890123456";
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_BLOCK_MODE, KM_MODE_GCM);
    begin_params.push_back(TAG_PADDING, KM_PAD_NONE);
    begin_params.push_back(TAG_MAC_LENGTH, 128);
    AuthorizationSet begin_out_params;

    AuthorizationSet update_params;
    update_params.push_back(TAG_ASSOCIATED_DATA, "foo", 3);

    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_ENCRYPT, begin_params, &begin_out_params));

    // No data, AAD only.
    string ciphertext;
    size_t input_consumed;
    AuthorizationSet update_out_params;
    EXPECT_EQ(KM_ERROR_OK, UpdateOperation(update_params, "" /* message */, &update_out_params,
                                           &ciphertext, &input_consumed));
    EXPECT_EQ(0U, input_consumed);

    // AAD and data.
    EXPECT_EQ(KM_ERROR_OK, UpdateOperation(update_params, message, &update_out_params, &ciphertext,
                                           &input_consumed));
    EXPECT_EQ(message.size(), input_consumed);
    EXPECT_EQ(KM_ERROR_OK, FinishOperation(&ciphertext));

    // Grab nonce.
    EXPECT_NE(-1, begin_out_params.find(TAG_NONCE));
    begin_params.push_back(begin_out_params);

    // Decrypt
    update_params.Clear();
    update_params.push_back(TAG_ASSOCIATED_DATA, "foofoo", 6);

    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_DECRYPT, begin_params));
    string plaintext;
    EXPECT_EQ(KM_ERROR_OK, UpdateOperation(update_params, ciphertext, &update_out_params,
                                           &plaintext, &input_consumed));
    EXPECT_EQ(ciphertext.size(), input_consumed);
    EXPECT_EQ(KM_ERROR_OK, FinishOperation(&plaintext));

    EXPECT_EQ(message, plaintext);
    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, AesGcmBadAad) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .AesEncryptionKey(128)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_GCM)
                                           .Authorization(TAG_PADDING, KM_PAD_NONE)
                                           .Authorization(TAG_MIN_MAC_LENGTH, 128)));
    string message = "12345678901234567890123456789012";
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_BLOCK_MODE, KM_MODE_GCM);
    begin_params.push_back(TAG_PADDING, KM_PAD_NONE);
    begin_params.push_back(TAG_MAC_LENGTH, 128);

    AuthorizationSet update_params;
    update_params.push_back(TAG_ASSOCIATED_DATA, "foobar", 6);

    AuthorizationSet finish_params;
    AuthorizationSet finish_out_params;

    // Encrypt
    AuthorizationSet begin_out_params;
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_ENCRYPT, begin_params, &begin_out_params));
    AuthorizationSet update_out_params;
    string ciphertext;
    size_t input_consumed;
    EXPECT_EQ(KM_ERROR_OK, UpdateOperation(update_params, message, &update_out_params, &ciphertext,
                                           &input_consumed));
    EXPECT_EQ(message.size(), input_consumed);
    EXPECT_EQ(KM_ERROR_OK, FinishOperation(&ciphertext));

    // Grab nonce
    EXPECT_NE(-1, begin_out_params.find(TAG_NONCE));
    begin_params.push_back(begin_out_params);

    update_params.Clear();
    update_params.push_back(TAG_ASSOCIATED_DATA, "barfoo" /* Wrong AAD */, 6);

    // Decrypt.
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_DECRYPT, begin_params, &begin_out_params));
    string plaintext;
    EXPECT_EQ(KM_ERROR_OK, UpdateOperation(update_params, ciphertext, &update_out_params,
                                           &plaintext, &input_consumed));
    EXPECT_EQ(ciphertext.size(), input_consumed);
    EXPECT_EQ(KM_ERROR_VERIFICATION_FAILED, FinishOperation(&plaintext));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, AesGcmWrongNonce) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .AesEncryptionKey(128)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_GCM)
                                           .Authorization(TAG_PADDING, KM_PAD_NONE)
                                           .Authorization(TAG_MIN_MAC_LENGTH, 128)));
    string message = "12345678901234567890123456789012";
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_BLOCK_MODE, KM_MODE_GCM);
    begin_params.push_back(TAG_PADDING, KM_PAD_NONE);
    begin_params.push_back(TAG_MAC_LENGTH, 128);

    AuthorizationSet update_params;
    update_params.push_back(TAG_ASSOCIATED_DATA, "foobar", 6);

    // Encrypt
    AuthorizationSet begin_out_params;
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_ENCRYPT, begin_params, &begin_out_params));
    AuthorizationSet update_out_params;
    string ciphertext;
    size_t input_consumed;
    EXPECT_EQ(KM_ERROR_OK, UpdateOperation(update_params, message, &update_out_params, &ciphertext,
                                           &input_consumed));
    EXPECT_EQ(message.size(), input_consumed);
    EXPECT_EQ(KM_ERROR_OK, FinishOperation(&ciphertext));

    begin_params.push_back(TAG_NONCE, "123456789012", 12);

    // Decrypt
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_DECRYPT, begin_params, &begin_out_params));
    string plaintext;
    EXPECT_EQ(KM_ERROR_OK, UpdateOperation(update_params, ciphertext, &update_out_params,
                                           &plaintext, &input_consumed));
    EXPECT_EQ(ciphertext.size(), input_consumed);
    EXPECT_EQ(KM_ERROR_VERIFICATION_FAILED, FinishOperation(&plaintext));

    // With wrong nonce, should have gotten garbage plaintext.
    EXPECT_NE(message, plaintext);
    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, AesGcmCorruptTag) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .AesEncryptionKey(128)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_GCM)
                                           .Authorization(TAG_PADDING, KM_PAD_NONE)
                                           .Authorization(TAG_MIN_MAC_LENGTH, 128)));
    string aad = "foobar";
    string message = "123456789012345678901234567890123456";
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_BLOCK_MODE, KM_MODE_GCM);
    begin_params.push_back(TAG_PADDING, KM_PAD_NONE);
    begin_params.push_back(TAG_MAC_LENGTH, 128);
    AuthorizationSet begin_out_params;

    AuthorizationSet update_params;
    update_params.push_back(TAG_ASSOCIATED_DATA, aad.data(), aad.size());

    // Encrypt
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_ENCRYPT, begin_params, &begin_out_params));
    AuthorizationSet update_out_params;
    string ciphertext;
    size_t input_consumed;
    EXPECT_EQ(KM_ERROR_OK, UpdateOperation(update_params, message, &update_out_params, &ciphertext,
                                           &input_consumed));
    EXPECT_EQ(message.size(), input_consumed);
    EXPECT_EQ(KM_ERROR_OK, FinishOperation(&ciphertext));

    // Corrupt tag
    (*ciphertext.rbegin())++;

    // Grab nonce.
    EXPECT_NE(-1, begin_out_params.find(TAG_NONCE));
    begin_params.push_back(begin_out_params);

    // Decrypt.
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_DECRYPT, begin_params, &begin_out_params));
    string plaintext;
    EXPECT_EQ(KM_ERROR_OK, UpdateOperation(update_params, ciphertext, &update_out_params,
                                           &plaintext, &input_consumed));
    EXPECT_EQ(ciphertext.size(), input_consumed);
    EXPECT_EQ(KM_ERROR_VERIFICATION_FAILED, FinishOperation(&plaintext));

    EXPECT_EQ(message, plaintext);
    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, TripleDesEcbRoundTripSuccess) {
    auto auths = AuthorizationSetBuilder()
                     .TripleDesEncryptionKey(112)
                     .Authorization(TAG_BLOCK_MODE, KM_MODE_ECB)
                     .Padding(KM_PAD_NONE);

    ASSERT_EQ(KM_ERROR_OK, GenerateKey(auths));
    // Two-block message.
    string message = "1234567890123456";
    string ciphertext1 = EncryptMessage(message, KM_MODE_ECB, KM_PAD_NONE);
    EXPECT_EQ(message.size(), ciphertext1.size());

    string ciphertext2 = EncryptMessage(string(message), KM_MODE_ECB, KM_PAD_NONE);
    EXPECT_EQ(message.size(), ciphertext2.size());

    // ECB is deterministic.
    EXPECT_EQ(ciphertext1, ciphertext2);

    string plaintext = DecryptMessage(ciphertext1, KM_MODE_ECB, KM_PAD_NONE);
    EXPECT_EQ(message, plaintext);
}

TEST_P(EncryptionOperationsTest, TripleDesEcbNotAuthorized) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .TripleDesEncryptionKey(112)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_CBC)
                                           .Padding(KM_PAD_NONE)));
    // Two-block message.
    string message = "1234567890123456";
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_BLOCK_MODE, KM_MODE_ECB);
    begin_params.push_back(TAG_PADDING, KM_PAD_NONE);
    EXPECT_EQ(KM_ERROR_INCOMPATIBLE_BLOCK_MODE, BeginOperation(KM_PURPOSE_ENCRYPT, begin_params));
}

TEST_P(EncryptionOperationsTest, TripleDesEcbNoPaddingWrongInputSize) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .TripleDesEncryptionKey(112)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_ECB)
                                           .Padding(KM_PAD_NONE)));
    // Message is slightly shorter than two blocks.
    string message = "123456789012345";

    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_BLOCK_MODE, KM_MODE_ECB);
    begin_params.push_back(TAG_PADDING, KM_PAD_NONE);
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_ENCRYPT, begin_params));
    string ciphertext;
    EXPECT_EQ(KM_ERROR_INVALID_INPUT_LENGTH, FinishOperation(message, "", &ciphertext));
}

TEST_P(EncryptionOperationsTest, TripleDesEcbPkcs7Padding) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .TripleDesEncryptionKey(112)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_ECB)
                                           .Authorization(TAG_PADDING, KM_PAD_PKCS7)));

    // Try various message lengths; all should work.
    for (size_t i = 0; i < 32; ++i) {
        string message(i, 'a');
        string ciphertext = EncryptMessage(message, KM_MODE_ECB, KM_PAD_PKCS7);
        EXPECT_EQ(i + 8 - (i % 8), ciphertext.size());
        string plaintext = DecryptMessage(ciphertext, KM_MODE_ECB, KM_PAD_PKCS7);
        EXPECT_EQ(message, plaintext);
    }
}

TEST_P(EncryptionOperationsTest, TripleDesEcbNoPaddingKeyWithPkcs7Padding) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .TripleDesEncryptionKey(112)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_ECB)
                                           .Authorization(TAG_PADDING, KM_PAD_NONE)));

    // Try various message lengths; all should fail.
    for (size_t i = 0; i < 32; ++i) {
        AuthorizationSet begin_params(client_params());
        begin_params.push_back(TAG_BLOCK_MODE, KM_MODE_ECB);
        begin_params.push_back(TAG_PADDING, KM_PAD_PKCS7);
        EXPECT_EQ(KM_ERROR_INCOMPATIBLE_PADDING_MODE,
                  BeginOperation(KM_PURPOSE_ENCRYPT, begin_params));
    }
}

TEST_P(EncryptionOperationsTest, TripleDesEcbPkcs7PaddingCorrupted) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .TripleDesEncryptionKey(112)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_ECB)
                                           .Authorization(TAG_PADDING, KM_PAD_PKCS7)));

    string message = "a";
    string ciphertext = EncryptMessage(message, KM_MODE_ECB, KM_PAD_PKCS7);
    EXPECT_EQ(8U, ciphertext.size());
    EXPECT_NE(ciphertext, message);
    ++ciphertext[ciphertext.size() / 2];

    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_BLOCK_MODE, KM_MODE_ECB);
    begin_params.push_back(TAG_PADDING, KM_PAD_PKCS7);
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_DECRYPT, begin_params));
    string plaintext;
    size_t input_consumed;
    EXPECT_EQ(KM_ERROR_OK, UpdateOperation(ciphertext, &plaintext, &input_consumed));
    EXPECT_EQ(ciphertext.size(), input_consumed);
    EXPECT_EQ(KM_ERROR_INVALID_ARGUMENT, FinishOperation(&plaintext));
}

struct TripleDesTestVector {
    const char* name;
    const keymaster_purpose_t purpose;
    const keymaster_block_mode_t block_mode;
    const keymaster_padding_t padding_mode;
    const char* key;
    const char* iv;
    const char* input;
    const char* output;
};

// These test vectors are from NIST CAVP, plus a few custom variants to test padding, since all of
// the NIST vectors are multiples of the block size.
static const TripleDesTestVector kTripleDesTestVectors[] = {
    {
        "TECBMMT2 Encrypt 0", KM_PURPOSE_ENCRYPT, KM_MODE_ECB, KM_PAD_NONE,
        "ad192fd064b5579e7a4fb3c8f794f22a",  // key
        "",                                  // IV
        "13bad542f3652d67",                  // input
        "908e543cf2cb254f",                  // output
    },
    {
        "TECBMMT2 Encrypt 0 PKCS7", KM_PURPOSE_ENCRYPT, KM_MODE_ECB, KM_PAD_PKCS7,
        "ad192fd064b5579e7a4fb3c8f794f22a",  // key
        "",                                  // IV
        "13bad542f3652d6700",                // input
        "908e543cf2cb254fc40165289a89008c",  // output
    },
    {
        "TECBMMT2 Encrypt 0 PKCS7 decrypted", KM_PURPOSE_DECRYPT, KM_MODE_ECB, KM_PAD_PKCS7,
        "ad192fd064b5579e7a4fb3c8f794f22a",  // key
        "",                                  // IV
        "908e543cf2cb254fc40165289a89008c",  // input
        "13bad542f3652d6700",                // output
    },
    {
        "TECBMMT2 Encrypt 1", KM_PURPOSE_ENCRYPT, KM_MODE_ECB, KM_PAD_NONE,
        "259df16e7af804fe83b90e9bf7c7e557",  // key
        "",                                  // IV
        "a4619c433bbd6787c07c81728f9ac9fa",  // input
        "9e06de155c483c6bcfd834dbc8bd5830",  // output
    },
    {
        "TECBMMT2 Decrypt 0", KM_PURPOSE_DECRYPT, KM_MODE_ECB, KM_PAD_NONE,
        "b32ff42092024adf2076b9d3d9f19e6d",  // key
        "",                                  // IV
        "2f3f2a49bba807a5",                  // input
        "2249973fa135fb52",                  // output
    },
    {
        "TECBMMT2 Decrypt 1", KM_PURPOSE_DECRYPT, KM_MODE_ECB, KM_PAD_NONE,
        "023dfbe6621aa17cc219eae9cdecd923",  // key
        "",                                  // IV
        "54045dc71d8d565b227ec19f06fef912",  // input
        "9b071622181e6412de6066429401410d",  // output
    },
    {
        "TECBMMT3 Encrypt 0", KM_PURPOSE_ENCRYPT, KM_MODE_ECB, KM_PAD_NONE,
        "a2b5bc67da13dc92cd9d344aa238544a0e1fa79ef76810cd",  // key
        "",                                                  // IV
        "329d86bdf1bc5af4",                                  // input
        "d946c2756d78633f",                                  // output
    },
    {
        "TECBMMT3 Encrypt 1", KM_PURPOSE_ENCRYPT, KM_MODE_ECB, KM_PAD_NONE,
        "49e692290d2a5e46bace79b9648a4c5d491004c262dc9d49",  // key
        "",                                                  // IV
        "6b1540781b01ce1997adae102dbf3c5b",                  // input
        "4d0dc182d6e481ac4a3dc6ab6976ccae",                  // output
    },
    {
        "TECBMMT3 Decrypt 0", KM_PURPOSE_DECRYPT, KM_MODE_ECB, KM_PAD_NONE,
        "52daec2ac7dc1958377392682f37860b2cc1ea2304bab0e9",  // key
        "",                                                  // IV
        "6daad94ce08acfe7",                                  // input
        "660e7d32dcc90e79",                                  // output
    },
    {
        "TECBMMT3 Decrypt 1", KM_PURPOSE_DECRYPT, KM_MODE_ECB, KM_PAD_NONE,
        "7f8fe3d3f4a48394fb682c2919926d6ddfce8932529229ce",  // key
        "",                                                  // IV
        "e9653a0a1f05d31b9acd12d73aa9879d",                  // input
        "9b2ae9d998efe62f1b592e7e1df8ff38",                  // output
    },
    {
        "TCBCMMT2 Encrypt 0", KM_PURPOSE_ENCRYPT, KM_MODE_CBC, KM_PAD_NONE,
        "34a41a8c293176c1b30732ecfe38ae8a",  // key
        "f55b4855228bd0b4",                  // IV
        "7dd880d2a9ab411c",                  // input
        "c91892948b6cadb4",                  // output
    },
    {
        "TCBCMMT2 Encrypt 1", KM_PURPOSE_ENCRYPT, KM_MODE_CBC, KM_PAD_NONE,
        "70a88fa1dfb9942fa77f40157ffef2ad",  // key
        "ece08ce2fdc6ce80",                  // IV
        "bc225304d5a3a5c9918fc5006cbc40cc",  // input
        "27f67dc87af7ddb4b68f63fa7c2d454a",  // output
    },
    {
        "TCBCMMT2 Decrypt 0", KM_PURPOSE_DECRYPT, KM_MODE_CBC, KM_PAD_NONE,
        "4ff47fda89209bda8c85f7fe80192007",  // key
        "d5bc4891dabe48b9",                  // IV
        "7e154b28c353adef",                  // input
        "712b961ea9a1d0af",                  // output
    },
    {
        "TCBCMMT2 Decrypt 1", KM_PURPOSE_DECRYPT, KM_MODE_CBC, KM_PAD_NONE,
        "464092cdbf736d38fb1fe6a12a94ae0e",  // key
        "5423455f00023b01",                  // IV
        "3f6050b74ed64416bc23d53b0469ed7a",  // input
        "9cbe7d1b5cdd1864c3095ba810575960",  // output
    },
    {
        "TCBCMMT3 Encrypt 0", KM_PURPOSE_ENCRYPT, KM_MODE_CBC, KM_PAD_NONE,
        "b5cb1504802326c73df186e3e352a20de643b0d63ee30e37",  // key
        "43f791134c5647ba",                                  // IV
        "dcc153cef81d6f24",                                  // input
        "92538bd8af18d3ba",                                  // output
    },
    {
        "TCBCMMT3 Encrypt 1", KM_PURPOSE_ENCRYPT, KM_MODE_CBC, KM_PAD_NONE,
        "a49d7564199e97cb529d2c9d97bf2f98d35edf57ba1f7358",  // key
        "c2e999cb6249023c",                                  // IV
        "c689aee38a301bb316da75db36f110b5",                  // input
        "e9afaba5ec75ea1bbe65506655bb4ecb",                  // output
    },
    {
        "TCBCMMT3 Encrypt 1 PKCS7 variant", KM_PURPOSE_ENCRYPT, KM_MODE_CBC, KM_PAD_PKCS7,
        "a49d7564199e97cb529d2c9d97bf2f98d35edf57ba1f7358",  // key
        "c2e999cb6249023c",                                  // IV
        "c689aee38a301bb316da75db36f110b500",                // input
        "e9afaba5ec75ea1bbe65506655bb4ecb825aa27ec0656156",  // output
    },
    {
        "TCBCMMT3 Encrypt 1 PKCS7 decrypted", KM_PURPOSE_DECRYPT, KM_MODE_CBC, KM_PAD_PKCS7,
        "a49d7564199e97cb529d2c9d97bf2f98d35edf57ba1f7358",  // key
        "c2e999cb6249023c",                                  // IV
        "e9afaba5ec75ea1bbe65506655bb4ecb825aa27ec0656156",  // input
        "c689aee38a301bb316da75db36f110b500",                // output
    },
    {
        "TCBCMMT3 Decrypt 0", KM_PURPOSE_DECRYPT, KM_MODE_CBC, KM_PAD_NONE,
        "5eb6040d46082c7aa7d06dfd08dfeac8c18364c1548c3ba1",  // key
        "41746c7e442d3681",                                  // IV
        "c53a7b0ec40600fe",                                  // input
        "d4f00eb455de1034",                                  // output
    },
    {
        "TCBCMMT3 Decrypt 1", KM_PURPOSE_DECRYPT, KM_MODE_CBC, KM_PAD_NONE,
        "5b1cce7c0dc1ec49130dfb4af45785ab9179e567f2c7d549",  // key
        "3982bc02c3727d45",                                  // IV
        "6006f10adef52991fcc777a1238bbb65",                  // input
        "edae09288e9e3bc05746d872b48e3b29",                  // output
    },
};

TEST_P(EncryptionOperationsTest, TripleDesTestVector) {
    for (auto& test : array_range(kTripleDesTestVectors)) {
        SCOPED_TRACE(test.name);
        CheckTripleDesTestVector(test.purpose, test.block_mode, test.padding_mode,
                                 hex2str(test.key), hex2str(test.iv), hex2str(test.input),
                                 hex2str(test.output));
    }
}

TEST_P(EncryptionOperationsTest, TripleDesCbcRoundTripSuccess) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .TripleDesEncryptionKey(112)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_CBC)
                                           .Padding(KM_PAD_NONE)));
    // Two-block message.
    string message = "1234567890123456";
    string iv1;
    string ciphertext1 = EncryptMessage(message, KM_MODE_CBC, KM_PAD_NONE, &iv1);
    EXPECT_EQ(message.size(), ciphertext1.size());

    string iv2;
    string ciphertext2 = EncryptMessage(message, KM_MODE_CBC, KM_PAD_NONE, &iv2);
    EXPECT_EQ(message.size(), ciphertext2.size());

    // IVs should be random, so ciphertexts should differ.
    EXPECT_NE(iv1, iv2);
    EXPECT_NE(ciphertext1, ciphertext2);

    string plaintext = DecryptMessage(ciphertext1, KM_MODE_CBC, KM_PAD_NONE, iv1);
    EXPECT_EQ(message, plaintext);

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(EncryptionOperationsTest, TripleDesCallerIv) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .TripleDesEncryptionKey(112)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_CBC)
                                           .Authorization(TAG_CALLER_NONCE)
                                           .Padding(KM_PAD_NONE)));
    string message = "1234567890123456";
    string iv1;
    // Don't specify IV, should get a random one.
    string ciphertext1 = EncryptMessage(message, KM_MODE_CBC, KM_PAD_NONE, &iv1);
    EXPECT_EQ(message.size(), ciphertext1.size());
    EXPECT_EQ(8U, iv1.size());

    string plaintext = DecryptMessage(ciphertext1, KM_MODE_CBC, KM_PAD_NONE, iv1);
    EXPECT_EQ(message, plaintext);

    // Now specify an IV, should also work.
    AuthorizationSet input_params(client_params());
    AuthorizationSet update_params;
    AuthorizationSet output_params;
    input_params.push_back(TAG_NONCE, "abcdefgh", 8);
    input_params.push_back(TAG_BLOCK_MODE, KM_MODE_CBC);
    input_params.push_back(TAG_PADDING, KM_PAD_NONE);
    string ciphertext2 =
        ProcessMessage(KM_PURPOSE_ENCRYPT, message, input_params, update_params, &output_params);

    // Decrypt with correct IV.
    plaintext = ProcessMessage(KM_PURPOSE_DECRYPT, ciphertext2, input_params, update_params,
                               &output_params);
    EXPECT_EQ(message, plaintext);

    // Now try with wrong IV.
    input_params.Reinitialize(client_params());
    input_params.push_back(TAG_BLOCK_MODE, KM_MODE_CBC);
    input_params.push_back(TAG_PADDING, KM_PAD_NONE);
    input_params.push_back(TAG_NONCE, "aaaaaaaa", 8);
    plaintext = ProcessMessage(KM_PURPOSE_DECRYPT, ciphertext2, input_params, update_params,
                               &output_params);
    EXPECT_NE(message, plaintext);
}

TEST_P(EncryptionOperationsTest, TripleDesCallerNonceProhibited) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .TripleDesEncryptionKey(112)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_CBC)
                                           .Padding(KM_PAD_NONE)));

    string message = "12345678901234567890123456789012";
    string iv1;
    // Don't specify nonce, should get a random one.
    string ciphertext1 = EncryptMessage(message, KM_MODE_CBC, KM_PAD_NONE, &iv1);
    EXPECT_EQ(message.size(), ciphertext1.size());
    EXPECT_EQ(8U, iv1.size());

    string plaintext = DecryptMessage(ciphertext1, KM_MODE_CBC, KM_PAD_NONE, iv1);
    EXPECT_EQ(message, plaintext);

    // Now specify a nonce, should fail.
    AuthorizationSet input_params(client_params());
    AuthorizationSet update_params;
    AuthorizationSet output_params;
    input_params.push_back(TAG_NONCE, "abcdefgh", 8);
    input_params.push_back(TAG_BLOCK_MODE, KM_MODE_CBC);
    input_params.push_back(TAG_PADDING, KM_PAD_NONE);

    EXPECT_EQ(KM_ERROR_CALLER_NONCE_PROHIBITED,
              BeginOperation(KM_PURPOSE_ENCRYPT, input_params, &output_params));
}

TEST_P(EncryptionOperationsTest, TripleDesCbcNotAuthorized) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .TripleDesEncryptionKey(112)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_ECB)
                                           .Padding(KM_PAD_NONE)));
    // Two-block message.
    string message = "1234567890123456";
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_BLOCK_MODE, KM_MODE_CBC);
    begin_params.push_back(TAG_PADDING, KM_PAD_NONE);
    EXPECT_EQ(KM_ERROR_INCOMPATIBLE_BLOCK_MODE, BeginOperation(KM_PURPOSE_ENCRYPT, begin_params));
}

TEST_P(EncryptionOperationsTest, TripleDesCbcNoPaddingWrongInputSize) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .TripleDesEncryptionKey(112)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_CBC)
                                           .Padding(KM_PAD_NONE)));
    // Message is slightly shorter than two blocks.
    string message = "123456789012345";

    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_BLOCK_MODE, KM_MODE_CBC);
    begin_params.push_back(TAG_PADDING, KM_PAD_NONE);
    AuthorizationSet output_params;
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_ENCRYPT, begin_params, &output_params));
    string ciphertext;
    EXPECT_EQ(KM_ERROR_INVALID_INPUT_LENGTH, FinishOperation(message, "", &ciphertext));
}

TEST_P(EncryptionOperationsTest, TripleDesCbcPkcs7Padding) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .TripleDesEncryptionKey(112)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_CBC)
                                           .Authorization(TAG_PADDING, KM_PAD_PKCS7)));

    // Try various message lengths; all should work.
    for (size_t i = 0; i < 32; ++i) {
        string message(i, 'a');
        string iv;
        string ciphertext = EncryptMessage(message, KM_MODE_CBC, KM_PAD_PKCS7, &iv);
        EXPECT_EQ(i + 8 - (i % 8), ciphertext.size());
        string plaintext = DecryptMessage(ciphertext, KM_MODE_CBC, KM_PAD_PKCS7, iv);
        EXPECT_EQ(message, plaintext);
    }
}

TEST_P(EncryptionOperationsTest, TripleDesCbcNoPaddingKeyWithPkcs7Padding) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .TripleDesEncryptionKey(112)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_CBC)
                                           .Authorization(TAG_PADDING, KM_PAD_NONE)));

    // Try various message lengths; all should fail.
    for (size_t i = 0; i < 32; ++i) {
        AuthorizationSet begin_params(client_params());
        begin_params.push_back(TAG_BLOCK_MODE, KM_MODE_CBC);
        begin_params.push_back(TAG_PADDING, KM_PAD_PKCS7);
        EXPECT_EQ(KM_ERROR_INCOMPATIBLE_PADDING_MODE,
                  BeginOperation(KM_PURPOSE_ENCRYPT, begin_params));
    }
}

TEST_P(EncryptionOperationsTest, TripleDesCbcPkcs7PaddingCorrupted) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .TripleDesEncryptionKey(112)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_CBC)
                                           .Authorization(TAG_PADDING, KM_PAD_PKCS7)));

    string message = "a";
    string iv;
    string ciphertext = EncryptMessage(message, KM_MODE_CBC, KM_PAD_PKCS7, &iv);
    EXPECT_EQ(8U, ciphertext.size());
    EXPECT_NE(ciphertext, message);
    ++ciphertext[ciphertext.size() / 2];

    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_BLOCK_MODE, KM_MODE_CBC);
    begin_params.push_back(TAG_PADDING, KM_PAD_PKCS7);
    begin_params.push_back(TAG_NONCE, iv.data(), iv.size());
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_DECRYPT, begin_params));
    string plaintext;
    size_t input_consumed;
    EXPECT_EQ(KM_ERROR_OK, UpdateOperation(ciphertext, &plaintext, &input_consumed));
    EXPECT_EQ(ciphertext.size(), input_consumed);
    EXPECT_EQ(KM_ERROR_INVALID_ARGUMENT, FinishOperation(&plaintext));
}

TEST_P(EncryptionOperationsTest, TripleDesCbcIncrementalNoPadding) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .TripleDesEncryptionKey(112)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_CBC)
                                           .Padding(KM_PAD_NONE)));

    int increment = 7;
    string message(240, 'a');
    AuthorizationSet input_params(client_params());
    input_params.push_back(TAG_BLOCK_MODE, KM_MODE_CBC);
    input_params.push_back(TAG_PADDING, KM_PAD_NONE);
    AuthorizationSet output_params;
    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_ENCRYPT, input_params, &output_params));

    string ciphertext;
    size_t input_consumed;
    for (size_t i = 0; i < message.size(); i += increment)
        EXPECT_EQ(KM_ERROR_OK,
                  UpdateOperation(message.substr(i, increment), &ciphertext, &input_consumed));
    EXPECT_EQ(KM_ERROR_OK, FinishOperation(&ciphertext));
    EXPECT_EQ(message.size(), ciphertext.size());

    // Move TAG_NONCE into input_params
    input_params.Reinitialize(output_params);
    input_params.push_back(client_params());
    input_params.push_back(TAG_BLOCK_MODE, KM_MODE_CBC);
    input_params.push_back(TAG_PADDING, KM_PAD_NONE);
    output_params.Clear();

    EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_DECRYPT, input_params, &output_params));
    string plaintext;
    for (size_t i = 0; i < ciphertext.size(); i += increment)
        EXPECT_EQ(KM_ERROR_OK,
                  UpdateOperation(ciphertext.substr(i, increment), &plaintext, &input_consumed));
    EXPECT_EQ(KM_ERROR_OK, FinishOperation(&plaintext));
    EXPECT_EQ(ciphertext.size(), plaintext.size());
    EXPECT_EQ(message, plaintext);

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

typedef Keymaster2Test MaxOperationsTest;
INSTANTIATE_TEST_CASE_P(AndroidKeymasterTest, MaxOperationsTest, test_params);

TEST_P(MaxOperationsTest, TestLimit) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .AesEncryptionKey(128)
                                           .EcbMode()
                                           .Authorization(TAG_PADDING, KM_PAD_NONE)
                                           .Authorization(TAG_MAX_USES_PER_BOOT, 3)));

    string message = "1234567890123456";
    string ciphertext1 = EncryptMessage(message, KM_MODE_ECB, KM_PAD_NONE);
    string ciphertext2 = EncryptMessage(message, KM_MODE_ECB, KM_PAD_NONE);
    string ciphertext3 = EncryptMessage(message, KM_MODE_ECB, KM_PAD_NONE);

    // Fourth time should fail.
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_BLOCK_MODE, KM_MODE_ECB);
    begin_params.push_back(TAG_PADDING, KM_PAD_NONE);
    EXPECT_EQ(KM_ERROR_KEY_MAX_OPS_EXCEEDED, BeginOperation(KM_PURPOSE_ENCRYPT, begin_params));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(MaxOperationsTest, TestAbort) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .AesEncryptionKey(128)
                                           .EcbMode()
                                           .Authorization(TAG_PADDING, KM_PAD_NONE)
                                           .Authorization(TAG_MAX_USES_PER_BOOT, 3)));

    string message = "1234567890123456";
    string ciphertext1 = EncryptMessage(message, KM_MODE_ECB, KM_PAD_NONE);
    string ciphertext2 = EncryptMessage(message, KM_MODE_ECB, KM_PAD_NONE);
    string ciphertext3 = EncryptMessage(message, KM_MODE_ECB, KM_PAD_NONE);

    // Fourth time should fail.
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_BLOCK_MODE, KM_MODE_ECB);
    begin_params.push_back(TAG_PADDING, KM_PAD_NONE);
    EXPECT_EQ(KM_ERROR_KEY_MAX_OPS_EXCEEDED, BeginOperation(KM_PURPOSE_ENCRYPT, begin_params));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

typedef Keymaster2Test AddEntropyTest;
INSTANTIATE_TEST_CASE_P(AndroidKeymasterTest, AddEntropyTest, test_params);

TEST_P(AddEntropyTest, AddEntropy) {
    // There's no obvious way to test that entropy is actually added, but we can test that the API
    // doesn't blow up or return an error.
    EXPECT_EQ(KM_ERROR_OK,
              device()->add_rng_entropy(device(), reinterpret_cast<const uint8_t*>("foo"), 3));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

typedef Keymaster2Test AttestationTest;
INSTANTIATE_TEST_CASE_P(AndroidKeymasterTest, AttestationTest, test_params);

static X509* parse_cert_blob(const keymaster_blob_t& blob) {
    const uint8_t* p = blob.data;
    return d2i_X509(nullptr, &p, blob.data_length);
}

static bool verify_chain(const keymaster_cert_chain_t& chain) {
    for (size_t i = 0; i < chain.entry_count - 1; ++i) {
        keymaster_blob_t& key_cert_blob = chain.entries[i];
        keymaster_blob_t& signing_cert_blob = chain.entries[i + 1];

        X509_Ptr key_cert(parse_cert_blob(key_cert_blob));
        X509_Ptr signing_cert(parse_cert_blob(signing_cert_blob));
        EXPECT_TRUE(!!key_cert.get() && !!signing_cert.get());
        if (!key_cert.get() || !signing_cert.get())
            return false;

        EVP_PKEY_Ptr signing_pubkey(X509_get_pubkey(signing_cert.get()));
        EXPECT_TRUE(!!signing_pubkey.get());
        if (!signing_pubkey.get())
            return false;

        EXPECT_EQ(1, X509_verify(key_cert.get(), signing_pubkey.get()))
            << "Verification of certificate " << i << " failed";
    }

    return true;
}

// Extract attestation record from cert. Returned object is still part of cert; don't free it
// separately.
static ASN1_OCTET_STRING* get_attestation_record(X509* certificate) {
    ASN1_OBJECT_Ptr oid(OBJ_txt2obj(kAttestionRecordOid, 1 /* dotted string format */));
    EXPECT_TRUE(!!oid.get());
    if (!oid.get())
        return nullptr;

    int location = X509_get_ext_by_OBJ(certificate, oid.get(), -1 /* search from beginning */);
    EXPECT_NE(-1, location);
    if (location == -1)
        return nullptr;

    X509_EXTENSION* attest_rec_ext = X509_get_ext(certificate, location);
    EXPECT_TRUE(!!attest_rec_ext);
    if (!attest_rec_ext)
        return nullptr;

    ASN1_OCTET_STRING* attest_rec = X509_EXTENSION_get_data(attest_rec_ext);
    EXPECT_TRUE(!!attest_rec);
    return attest_rec;
}

static bool verify_attestation_record(const string& challenge,
                                      AuthorizationSet expected_sw_enforced,
                                      AuthorizationSet expected_tee_enforced,
                                      uint32_t expected_keymaster_version,
                                      keymaster_security_level_t expected_keymaster_security_level,
                                      const keymaster_blob_t& attestation_cert) {

    X509_Ptr cert(parse_cert_blob(attestation_cert));
    EXPECT_TRUE(!!cert.get());
    if (!cert.get())
        return false;

    ASN1_OCTET_STRING* attest_rec = get_attestation_record(cert.get());
    EXPECT_TRUE(!!attest_rec);
    if (!attest_rec)
        return false;

    AuthorizationSet att_sw_enforced;
    AuthorizationSet att_tee_enforced;
    uint32_t att_attestation_version;
    uint32_t att_keymaster_version;
    keymaster_security_level_t att_attestation_security_level;
    keymaster_security_level_t att_keymaster_security_level;
    keymaster_blob_t att_challenge = {};
    keymaster_blob_t att_unique_id = {};
    EXPECT_EQ(KM_ERROR_OK, parse_attestation_record(
                               attest_rec->data, attest_rec->length, &att_attestation_version,
                               &att_attestation_security_level, &att_keymaster_version,
                               &att_keymaster_security_level, &att_challenge, &att_sw_enforced,
                               &att_tee_enforced, &att_unique_id));

    EXPECT_EQ(2U, att_attestation_version);
    EXPECT_EQ(KM_SECURITY_LEVEL_SOFTWARE, att_attestation_security_level);
    EXPECT_EQ(expected_keymaster_version, att_keymaster_version);
    EXPECT_EQ(expected_keymaster_security_level, att_keymaster_security_level);

    EXPECT_EQ(challenge.length(), att_challenge.data_length);
    EXPECT_EQ(0, memcmp(challenge.data(), att_challenge.data, challenge.length()));

    // Add TAG_USER_ID to the relevant attestation list, because user IDs are not included in
    // attestations, since they're meaningless off-device.
    uint32_t user_id;
    if (expected_sw_enforced.GetTagValue(TAG_USER_ID, &user_id))
        att_sw_enforced.push_back(TAG_USER_ID, user_id);
    if (expected_tee_enforced.GetTagValue(TAG_USER_ID, &user_id))
        att_tee_enforced.push_back(TAG_USER_ID, user_id);

    // Add TAG_INCLUDE_UNIQUE_ID to the relevant attestation list, because that tag is not included
    // in the attestation.
    if (expected_sw_enforced.GetTagValue(TAG_INCLUDE_UNIQUE_ID))
        att_sw_enforced.push_back(TAG_INCLUDE_UNIQUE_ID);
    if (expected_tee_enforced.GetTagValue(TAG_INCLUDE_UNIQUE_ID))
        att_tee_enforced.push_back(TAG_INCLUDE_UNIQUE_ID);

    att_sw_enforced.Sort();
    expected_sw_enforced.Sort();
    EXPECT_EQ(expected_sw_enforced, att_sw_enforced);

    att_tee_enforced.Sort();
    expected_tee_enforced.Sort();
    EXPECT_EQ(expected_tee_enforced, att_tee_enforced);

    delete[] att_challenge.data;
    delete[] att_unique_id.data;

    return true;
}

TEST_P(AttestationTest, RsaAttestation) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .RsaSigningKey(256, 3)
                                           .Digest(KM_DIGEST_NONE)
                                           .Padding(KM_PAD_NONE)
                                           .Authorization(TAG_INCLUDE_UNIQUE_ID)));

    keymaster_cert_chain_t cert_chain;
    EXPECT_EQ(KM_ERROR_OK, AttestKey("challenge", "attest_app_id", &cert_chain));
    ASSERT_EQ(3U, cert_chain.entry_count);
    EXPECT_TRUE(verify_chain(cert_chain));

    uint32_t expected_keymaster_version;
    keymaster_security_level_t expected_keymaster_security_level;
    // TODO(swillden): Add a test KM1 that claims to be hardware.
    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA)) {
        expected_keymaster_version = 0;
        expected_keymaster_security_level = KM_SECURITY_LEVEL_TRUSTED_ENVIRONMENT;
    } else {
        expected_keymaster_version = 3;
        expected_keymaster_security_level = KM_SECURITY_LEVEL_SOFTWARE;
    }

    EXPECT_TRUE(verify_attestation_record(
        "challenge", sw_enforced(), hw_enforced(), expected_keymaster_version,
        expected_keymaster_security_level, cert_chain.entries[0]));

    keymaster_free_cert_chain(&cert_chain);
}

TEST_P(AttestationTest, EcAttestation) {
    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder().EcdsaSigningKey(256).Digest(
                               KM_DIGEST_SHA_2_256)));

    uint32_t expected_keymaster_version;
    keymaster_security_level_t expected_keymaster_security_level;
    // TODO(swillden): Add a test KM1 that claims to be hardware.
    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_EC)) {
        expected_keymaster_version = 0;
        expected_keymaster_security_level = KM_SECURITY_LEVEL_TRUSTED_ENVIRONMENT;
    } else {
        expected_keymaster_version = 3;
        expected_keymaster_security_level = KM_SECURITY_LEVEL_SOFTWARE;
    }

    keymaster_cert_chain_t cert_chain;
    EXPECT_EQ(KM_ERROR_OK, AttestKey("challenge", "attest_app_id", &cert_chain));
    ASSERT_EQ(3U, cert_chain.entry_count);
    EXPECT_TRUE(verify_chain(cert_chain));
    EXPECT_TRUE(verify_attestation_record(
        "challenge", sw_enforced(), hw_enforced(), expected_keymaster_version,
        expected_keymaster_security_level, cert_chain.entries[0]));

    keymaster_free_cert_chain(&cert_chain);
}

typedef Keymaster2Test KeyUpgradeTest;
INSTANTIATE_TEST_CASE_P(AndroidKeymasterTest, KeyUpgradeTest, test_params);

TEST_P(KeyUpgradeTest, AesVersionUpgrade) {
    GetParam()->keymaster_context()->SetSystemVersion(1, 1);

    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder()
                                           .AesEncryptionKey(128)
                                           .Authorization(TAG_BLOCK_MODE, KM_MODE_ECB)
                                           .Padding(KM_PAD_NONE)));

    // Key should operate fine.
    string message = "1234567890123456";
    string ciphertext = EncryptMessage(message, KM_MODE_ECB, KM_PAD_NONE);
    EXPECT_EQ(message, DecryptMessage(ciphertext, KM_MODE_ECB, KM_PAD_NONE));

    // Increase patch level.  Key usage should fail with KM_ERROR_KEY_REQUIRES_UPGRADE.
    GetParam()->keymaster_context()->SetSystemVersion(1, 2);
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_BLOCK_MODE, KM_MODE_ECB);
    begin_params.push_back(TAG_PADDING, KM_PAD_NONE);
    if (GetParam()->is_keymaster1_hw()) {
        // Keymaster1 hardware can't support version binding.  The key will work regardless
        // of system version.  Just abort the remainder of the test.
        EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_ENCRYPT, begin_params));
        EXPECT_EQ(KM_ERROR_OK, AbortOperation());
        return;
    }
    EXPECT_EQ(KM_ERROR_KEY_REQUIRES_UPGRADE, BeginOperation(KM_PURPOSE_ENCRYPT, begin_params));

    // Getting characteristics should also fail
    EXPECT_EQ(KM_ERROR_KEY_REQUIRES_UPGRADE, GetCharacteristics());

    // Upgrade key.
    EXPECT_EQ(KM_ERROR_OK, UpgradeKey(client_params()));

    // Key should work again
    ciphertext = EncryptMessage(message, KM_MODE_ECB, KM_PAD_NONE);
    EXPECT_EQ(message, DecryptMessage(ciphertext, KM_MODE_ECB, KM_PAD_NONE));

    // Decrease patch level.  Key usage should fail with KM_ERROR_INVALID_KEY_BLOB.
    GetParam()->keymaster_context()->SetSystemVersion(1, 1);
    EXPECT_EQ(KM_ERROR_INVALID_KEY_BLOB, BeginOperation(KM_PURPOSE_ENCRYPT, begin_params));
    EXPECT_EQ(KM_ERROR_INVALID_KEY_BLOB, GetCharacteristics());

    // Upgrade should fail
    EXPECT_EQ(KM_ERROR_INVALID_ARGUMENT, UpgradeKey(client_params()));

    EXPECT_EQ(0, GetParam()->keymaster0_calls());
}

TEST_P(KeyUpgradeTest, RsaVersionUpgrade) {
    GetParam()->keymaster_context()->SetSystemVersion(1, 1);

    ASSERT_EQ(KM_ERROR_OK,
              GenerateKey(AuthorizationSetBuilder().RsaEncryptionKey(256, 3).Padding(KM_PAD_NONE)));

    // Key should operate fine.
    string message = "12345678901234567890123456789012";
    string ciphertext = EncryptMessage(message, KM_PAD_NONE);
    EXPECT_EQ(message, DecryptMessage(ciphertext, KM_PAD_NONE));

    // Increase patch level.  Key usage should fail with KM_ERROR_KEY_REQUIRES_UPGRADE.
    GetParam()->keymaster_context()->SetSystemVersion(1, 2);
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_PADDING, KM_PAD_NONE);
    if (GetParam()->is_keymaster1_hw()) {
        // Keymaster1 hardware can't support version binding.  The key will work regardless
        // of system version.  Just abort the remainder of the test.
        EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_ENCRYPT, begin_params));
        EXPECT_EQ(KM_ERROR_OK, AbortOperation());
        return;
    }
    EXPECT_EQ(KM_ERROR_KEY_REQUIRES_UPGRADE, BeginOperation(KM_PURPOSE_ENCRYPT, begin_params));

    // Getting characteristics should also fail
    EXPECT_EQ(KM_ERROR_KEY_REQUIRES_UPGRADE, GetCharacteristics());

    // Upgrade key.
    EXPECT_EQ(KM_ERROR_OK, UpgradeKey(client_params()));

    // Key should work again
    ciphertext = EncryptMessage(message, KM_PAD_NONE);
    EXPECT_EQ(message, DecryptMessage(ciphertext, KM_PAD_NONE));

    // Decrease patch level.  Key usage should fail with KM_ERROR_INVALID_KEY_BLOB.
    GetParam()->keymaster_context()->SetSystemVersion(1, 1);
    EXPECT_EQ(KM_ERROR_INVALID_KEY_BLOB, BeginOperation(KM_PURPOSE_ENCRYPT, begin_params));
    EXPECT_EQ(KM_ERROR_INVALID_KEY_BLOB, GetCharacteristics());

    // Upgrade should fail
    EXPECT_EQ(KM_ERROR_INVALID_ARGUMENT, UpgradeKey(client_params()));

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_RSA))
        EXPECT_EQ(7, GetParam()->keymaster0_calls());
}

TEST_P(KeyUpgradeTest, EcVersionUpgrade) {
    GetParam()->keymaster_context()->SetSystemVersion(1, 1);

    ASSERT_EQ(KM_ERROR_OK, GenerateKey(AuthorizationSetBuilder().EcdsaSigningKey(256).Digest(
                               KM_DIGEST_SHA_2_256)));

    // Key should operate fine.
    string message = "1234567890123456";
    string signature;
    SignMessage(message, &signature, KM_DIGEST_SHA_2_256);
    VerifyMessage(message, signature, KM_DIGEST_SHA_2_256);

    // Increase patch level.  Key usage should fail with KM_ERROR_KEY_REQUIRES_UPGRADE.
    GetParam()->keymaster_context()->SetSystemVersion(1, 2);
    AuthorizationSet begin_params(client_params());
    begin_params.push_back(TAG_DIGEST, KM_DIGEST_SHA_2_256);
    if (GetParam()->is_keymaster1_hw()) {
        // Keymaster1 hardware can't support version binding.  The key will work regardless
        // of system version.  Just abort the remainder of the test.
        EXPECT_EQ(KM_ERROR_OK, BeginOperation(KM_PURPOSE_SIGN, begin_params));
        EXPECT_EQ(KM_ERROR_OK, AbortOperation());
        return;
    }
    EXPECT_EQ(KM_ERROR_KEY_REQUIRES_UPGRADE, BeginOperation(KM_PURPOSE_SIGN, begin_params));

    // Getting characteristics should also fail
    EXPECT_EQ(KM_ERROR_KEY_REQUIRES_UPGRADE, GetCharacteristics());

    // Upgrade key.
    EXPECT_EQ(KM_ERROR_OK, UpgradeKey(client_params()));

    // Key should work again
    SignMessage(message, &signature, KM_DIGEST_SHA_2_256);
    VerifyMessage(message, signature, KM_DIGEST_SHA_2_256);

    // Decrease patch level.  Key usage should fail with KM_ERROR_INVALID_KEY_BLOB.
    GetParam()->keymaster_context()->SetSystemVersion(1, 1);
    EXPECT_EQ(KM_ERROR_INVALID_KEY_BLOB, BeginOperation(KM_PURPOSE_ENCRYPT, begin_params));
    EXPECT_EQ(KM_ERROR_INVALID_KEY_BLOB, GetCharacteristics());

    // Upgrade should fail
    EXPECT_EQ(KM_ERROR_INVALID_ARGUMENT, UpgradeKey(client_params()));

    if (GetParam()->algorithm_in_km0_hardware(KM_ALGORITHM_EC))
        EXPECT_EQ(7, GetParam()->keymaster0_calls());
}

TEST(SoftKeymasterWrapperTest, CheckKeymaster2Device) {
    // Make a good fake device, and wrap it.
    SoftKeymasterDevice* good_fake(new SoftKeymasterDevice(new TestKeymasterContext));

    // Wrap it and check it.
    SoftKeymasterDevice* good_fake_wrapper(new SoftKeymasterDevice(new TestKeymasterContext));
    good_fake_wrapper->SetHardwareDevice(good_fake->keymaster_device());
    EXPECT_TRUE(good_fake_wrapper->Keymaster1DeviceIsGood());

    // Close and clean up wrapper and wrapped
    good_fake_wrapper->keymaster_device()->common.close(good_fake_wrapper->hw_device());

    // Make a "bad" (doesn't support all digests) device;
    keymaster1_device_t* sha256_only_fake = make_device_sha256_only(
        (new SoftKeymasterDevice(new TestKeymasterContext("256")))->keymaster_device());

    // Wrap it and check it.
    SoftKeymasterDevice* sha256_only_fake_wrapper(
        (new SoftKeymasterDevice(new TestKeymasterContext)));
    sha256_only_fake_wrapper->SetHardwareDevice(sha256_only_fake);
    EXPECT_FALSE(sha256_only_fake_wrapper->Keymaster1DeviceIsGood());

    // Close and clean up wrapper and wrapped
    sha256_only_fake_wrapper->keymaster_device()->common.close(
        sha256_only_fake_wrapper->hw_device());
}

class HmacKeySharingTest : public ::testing::Test {
  protected:
    using KeymasterVec = std::vector<std::unique_ptr<AndroidKeymaster>>;
    using ParamsVec = std::vector<HmacSharingParameters>;
    using ByteString = std::basic_string<uint8_t>;
    using NonceVec = std::vector<ByteString>;
    using ResponseVec = std::vector<ComputeSharedHmacResponse>;

    KeymasterVec CreateKeymasters(size_t count) {
        KeymasterVec keymasters;
        for (size_t i = 0; i < count; ++i) {
            keymasters.push_back(make_unique<AndroidKeymaster>(new TestKeymasterContext, 16));
        }
        return keymasters;
    }

    ParamsVec GetHmacSharingParameters(const KeymasterVec& keymasters) {
        ParamsVec paramsVec;
        for (auto& keymaster : keymasters) {
            auto result = keymaster->GetHmacSharingParameters();
            EXPECT_EQ(KM_ERROR_OK, result.error);
            if (result.error == KM_ERROR_OK) paramsVec.push_back(move(result.params));
        }
        return paramsVec;
    }

    template <size_t N> ByteString ToByteString(const uint8_t (&a)[N]) { return ByteString(a, N); }

    ByteString ToByteString(const keymaster_blob_t& b) { return ByteString(b.data, b.data_length); }

    NonceVec CopyNonces(const ParamsVec& paramsVec) {
        NonceVec nonces;
        for (auto& param : paramsVec) {
            nonces.push_back(ToByteString(param.nonce));
        }
        return nonces;
    }

    ResponseVec ComputeSharedHmac(const KeymasterVec& keymasters, const ParamsVec& paramsVec) {
        ComputeSharedHmacRequest req;
        req.params_array.params_array = const_cast<HmacSharingParameters*>(paramsVec.data());
        auto prevent_deletion_of_paramsVec_data =
            finally([&]() { req.params_array.params_array = nullptr; });
        req.params_array.num_params = paramsVec.size();

        ResponseVec responses;
        for (auto& keymaster : keymasters) {
            responses.push_back(keymaster->ComputeSharedHmac(req));
        }

        return responses;
    }

    bool VerifyResponses(const ByteString& expected, const ResponseVec& responses) {
        for (auto& response : responses) {
            EXPECT_EQ(KM_ERROR_OK, response.error);
            auto this_sharing_check = ToByteString(response.sharing_check);
            EXPECT_EQ(expected, this_sharing_check) << "Sharing check values should match.";
            if (response.error != KM_ERROR_OK || expected != this_sharing_check) {
                return false;
            }
        }
        return true;
    }
};

TEST_F(HmacKeySharingTest, GetParametersIdempotency) {
    AndroidKeymaster keymaster(new TestKeymasterContext, 16);

    ParamsVec paramsVec;

    auto result1 = keymaster.GetHmacSharingParameters();
    EXPECT_EQ(KM_ERROR_OK, result1.error);
    paramsVec.push_back(std::move(result1.params));

    auto result2 = keymaster.GetHmacSharingParameters();
    EXPECT_EQ(KM_ERROR_OK, result2.error);
    paramsVec.push_back(std::move(result2.params));

    ASSERT_EQ(ToByteString(paramsVec[0].seed), ToByteString(paramsVec[1].seed))
        << "A given keymaster should always return the same seed.";
    EXPECT_EQ(ToByteString(paramsVec[0].nonce), ToByteString(paramsVec[1].nonce))
        << "A given keymaster should always return the same nonce until restart.";
}

TEST_F(HmacKeySharingTest, ComputeSharedHmac) {
    // ComputeSharedHmac should work with any number of participants; we just test 1 through 4.
    for (size_t keymaster_count = 1; keymaster_count <= 4; ++keymaster_count) {
        SCOPED_TRACE(testing::Message() << keymaster_count << " keymaster instances");

        auto keymasters = CreateKeymasters(keymaster_count);
        auto params = GetHmacSharingParameters(keymasters);
        ASSERT_EQ(keymaster_count, params.size())
            << "One or more keymasters failed to provide parameters.";

        auto nonces = CopyNonces(params);
        EXPECT_EQ(keymaster_count, nonces.size()) << "We should have a nonce per keymaster.";
        std::sort(nonces.begin(), nonces.end());
        std::unique(nonces.begin(), nonces.end());
        EXPECT_EQ(keymaster_count, nonces.size()) << "Nonces should all be unique.";

        auto responses = ComputeSharedHmac(keymasters, params);
        ASSERT_EQ(keymaster_count, responses.size());

        ASSERT_TRUE(VerifyResponses(ToByteString(responses[0].sharing_check), responses));
    }
}

TEST_F(HmacKeySharingTest, ComputeSharedHmacTwice) {
    for (size_t keymaster_count = 1; keymaster_count <= 4; ++keymaster_count) {
        SCOPED_TRACE(testing::Message() << keymaster_count << " keymaster instances");

        auto keymasters = CreateKeymasters(keymaster_count);
        auto params = GetHmacSharingParameters(keymasters);
        ASSERT_EQ(keymaster_count, params.size())
            << "One or more keymasters failed to provide parameters.";

        auto responses = ComputeSharedHmac(keymasters, params);
        ASSERT_EQ(keymaster_count, responses.size());

        ByteString sharing_check_value = ToByteString(responses[0].sharing_check);
        ASSERT_TRUE(VerifyResponses(sharing_check_value, responses));

        params = GetHmacSharingParameters(keymasters);
        ASSERT_EQ(keymaster_count, params.size())
            << "One or more keymasters failed to provide parameters.";
        responses = ComputeSharedHmac(keymasters, params);

        // Verify against first check value; we should get the same one every time, because each
        // keymaster instance returns the same seed every time, and the same nonce until restart.
        ASSERT_TRUE(VerifyResponses(sharing_check_value, responses));
    }
}

TEST_F(HmacKeySharingTest, ComputeSharedHmacCorruptNonce) {
    constexpr size_t keymaster_count = 4;
    auto keymasters = CreateKeymasters(keymaster_count);

    auto params = GetHmacSharingParameters(keymasters);
    ASSERT_EQ(keymaster_count, params.size())
        << "One or more keymasters failed to provide parameters.";

    // All should be well in the normal case
    auto responses = ComputeSharedHmac(keymasters, params);
    ASSERT_EQ(keymaster_count, responses.size());
    ByteString sharing_check_value = ToByteString(responses[0].sharing_check);
    ASSERT_TRUE(VerifyResponses(sharing_check_value, responses));

    // Pick a random param, a random byte within the param's nonce, and a random bit within
    // the byte.  Flip that bit.
    size_t param_to_tweak = rand() % params.size();
    uint8_t byte_to_tweak = rand() % sizeof(params[param_to_tweak].nonce);
    uint8_t bit_to_tweak = rand() % 8;
    params[param_to_tweak].nonce[byte_to_tweak] ^= (1 << bit_to_tweak);

    responses = ComputeSharedHmac(keymasters, params);
    EXPECT_EQ(KM_ERROR_INVALID_ARGUMENT, responses[param_to_tweak].error)
        << "Keymaster that provided tweaked response should fail to compute HMAC key";
    for (size_t i = 0; i < responses.size(); ++i) {
        if (i != param_to_tweak) {
            EXPECT_EQ(KM_ERROR_OK, responses[i].error) << "Others should succeed";
            EXPECT_NE(sharing_check_value, ToByteString(responses[i].sharing_check))
                << "Others should calculate a different HMAC key, due to the tweaked nonce.";
        }
    }
}

TEST_F(HmacKeySharingTest, ComputeSharedHmacCorruptSeed) {
    constexpr size_t keymaster_count = 4;
    auto keymasters = CreateKeymasters(keymaster_count);

    auto params = GetHmacSharingParameters(keymasters);
    ASSERT_EQ(keymaster_count, params.size())
        << "One or more keymasters failed to provide parameters.";

    // All should be well in the normal case
    auto responses = ComputeSharedHmac(keymasters, params);
    ASSERT_EQ(keymaster_count, responses.size());
    ByteString sharing_check_value = ToByteString(responses[0].sharing_check);
    ASSERT_TRUE(VerifyResponses(sharing_check_value, responses));

    // Pick a random param and modify the seed.
    auto param_to_tweak = rand() & params.size();
    constexpr uint8_t wrong_seed_value[] = {0xF, 0x0, 0x0};
    params[param_to_tweak].SetSeed({wrong_seed_value, sizeof(wrong_seed_value)});
    auto prevent_deletion_of_wrong_seed =
        finally([&]() { params[param_to_tweak].seed.data = nullptr; });

    responses = ComputeSharedHmac(keymasters, params);
    EXPECT_EQ(KM_ERROR_INVALID_ARGUMENT, responses[param_to_tweak].error)
        << "Keymaster that provided tweaked response should fail to compute HMAC key";
    for (size_t i = 0; i < responses.size(); ++i) {
        if (i != param_to_tweak) {
            EXPECT_EQ(KM_ERROR_OK, responses[i].error) << "Others should succeed";
            EXPECT_NE(sharing_check_value, ToByteString(responses[i].sharing_check))
                << "Others should calculate a different HMAC key, due to the tweaked seed.";
        }
    }
}

}  // namespace test
}  // namespace keymaster
