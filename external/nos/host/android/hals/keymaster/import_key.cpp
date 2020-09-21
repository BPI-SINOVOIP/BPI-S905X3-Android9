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

#include "import_key.h"
#include "proto_utils.h"

#include <android-base/logging.h>

#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/mem.h>
#include <openssl/rsa.h>
#include <openssl/pkcs8.h>

namespace android {
namespace hardware {
namespace keymaster {

// HAL
using ::android::hardware::keymaster::V4_0::Algorithm;
using ::android::hardware::keymaster::V4_0::EcCurve;
using ::android::hardware::keymaster::V4_0::KeyFormat;
using ::android::hardware::keymaster::V4_0::Tag;
using ::android::hardware::keymaster::V4_0::TagType;

// App
using ::nugget::app::keymaster::ECKey;

// BoringSSL
using bssl::UniquePtr;

// std
using std::unique_ptr;

static EVP_PKEY *evp_from_pkcs8_bytes(const uint8_t *bytes, size_t len)
{
    bssl::UniquePtr<PKCS8_PRIV_KEY_INFO> pkcs8(
        d2i_PKCS8_PRIV_KEY_INFO(NULL, &bytes, len));
    if (pkcs8.get() == NULL) {
        // Translate error.
        return nullptr;
    }

    return EVP_PKCS82PKEY(pkcs8.get());
}

static ErrorCode import_key_rsa(const tag_map_t& params,
                                const hidl_vec<uint8_t>& keyData,
                                ImportKeyRequest *request)
{
    const uint32_t *keySize = nullptr;
    if (params.find(Tag::KEY_SIZE) != params.end()) {
        const vector<KeyParameter>& v = params.find(Tag::KEY_SIZE)->second;
        keySize = &v[0].f.integer;
    }
    const uint64_t *publicExponent = nullptr;
    if (params.find(Tag::RSA_PUBLIC_EXPONENT) != params.end()) {
        const vector<KeyParameter>& v =
            params.find(Tag::RSA_PUBLIC_EXPONENT)->second;
        publicExponent = &v[0].f.longInteger;
    }

    bssl::UniquePtr<EVP_PKEY> pkey;
    pkey.reset(evp_from_pkcs8_bytes(&keyData[0], keyData.size()));
    if (pkey.get() == nullptr) {
        // Parse error.
        LOG(ERROR) << "ImportKey request: failed to parse PKCS8";
        return ErrorCode::INVALID_ARGUMENT;
    }

    const RSA *rsa = EVP_PKEY_get0_RSA(pkey.get());
    if (rsa == nullptr) {
        LOG(ERROR) << "ImportKey request: PKCS8 key is not RSA";
        return ErrorCode::INVALID_ARGUMENT;
    }

    size_t parsedKeySize = RSA_size(rsa) * 8;
    if (keySize != nullptr && parsedKeySize != *keySize) {
        // If specified, key size must match the PKCS8 blob.
        LOG(ERROR) << "ImportKey request: key size parameter mis-match";
        return ErrorCode::IMPORT_PARAMETER_MISMATCH;
    }

    const BIGNUM *n;
    const BIGNUM *e;
    const BIGNUM *d;
    RSA_get0_key(rsa, &n, &e, &d);
    if (publicExponent != nullptr && BN_get_word(e) != *publicExponent) {
        // If specified, the public exponent must match the PKCS8 blob.
        LOG(ERROR) << "ImportKey request: invalid publicExponent tag: "
                   << *publicExponent
                   << " expected: "
                   << BN_get_word(e);
        return ErrorCode::IMPORT_PARAMETER_MISMATCH;
    }

    // Key data may be invalid, and will be validated on the device
    // anyway, so avoid duplicate work here.

    // Public exponent.
    request->mutable_rsa()->set_e(BN_get_word(e));

    // Private exponent, zero-pad upto size of n.
    bssl::UniquePtr<uint8_t> d_buf(
        reinterpret_cast<uint8_t *>(OPENSSL_malloc(BN_num_bytes(n))));
    if (!BN_bn2le_padded(d_buf.get(), BN_num_bytes(n), d)) {
        LOG(ERROR) << "ImportKey request: bn2le failed";
        return ErrorCode::UNKNOWN_ERROR;
    }
    request->mutable_rsa()->set_d(d_buf.get(), BN_num_bytes(n));

    // Modulus.
    bssl::UniquePtr<uint8_t> n_buf(
        reinterpret_cast<uint8_t *>(OPENSSL_malloc(BN_num_bytes(n))));
    if (!BN_bn2le_padded(n_buf.get(), BN_num_bytes(n), n)) {
        LOG(ERROR) << "ImportKey request: bn2le_padded failed";
        return ErrorCode::UNKNOWN_ERROR;
    }
    request->mutable_rsa()->set_n(n_buf.get(), BN_num_bytes(n));

    return ErrorCode::OK;
}

static ErrorCode import_key_ec(const tag_map_t& params,
                               const hidl_vec<uint8_t>& keyData,
                               ImportKeyRequest *request)
{
    const EcCurve *curve_id = nullptr;
    if (params.find(Tag::EC_CURVE) != params.end()) {
        const vector<KeyParameter>& v = params.find(Tag::EC_CURVE)->second;
        curve_id = &v[0].f.ecCurve;
    }
    const uint32_t *key_size = nullptr;
    if (params.find(Tag::KEY_SIZE) != params.end()) {
        const vector<KeyParameter>& v = params.find(Tag::KEY_SIZE)->second;
        key_size = &v[0].f.integer;
    }

    bssl::UniquePtr<EVP_PKEY> pkey;
    pkey.reset(evp_from_pkcs8_bytes(&keyData[0], keyData.size()));
    if (pkey.get() == nullptr) {
        // Parse error.
        LOG(ERROR) << "ImportKey request: failed to parse PKCS8";
        return ErrorCode::INVALID_ARGUMENT;
    }

    const EC_KEY *ec_key = EVP_PKEY_get0_EC_KEY(pkey.get());
    if (ec_key == nullptr) {
        LOG(ERROR) << "ImportKey request: PKCS8 key is not EC";
        return ErrorCode::INVALID_ARGUMENT;
    }

    EcCurve parsed_curve_id;
    size_t parsed_key_size;
    const EC_GROUP *group = EC_KEY_get0_group(ec_key);
    switch (EC_GROUP_get_curve_name(group)) {
    case NID_secp224r1:
        parsed_curve_id = EcCurve::P_224;
        parsed_key_size = 224;
        break;
    case NID_X9_62_prime256v1:
        parsed_curve_id = EcCurve::P_256;
        parsed_key_size = 256;
        break;
    case NID_secp384r1:
        parsed_curve_id = EcCurve::P_384;
        parsed_key_size = 384;
        break;
    case NID_secp521r1:
        parsed_curve_id = EcCurve::P_521;
        parsed_key_size = 521;
        break;
    default:
        // Unsupported curve.
        return ErrorCode::INVALID_ARGUMENT;
    }

    if (curve_id != nullptr && *curve_id != parsed_curve_id) {
        // Parameter mis-match.
        LOG(ERROR) << "ImportKey: curve-id does not match PKCS8";
        return ErrorCode::IMPORT_PARAMETER_MISMATCH;
    }
    if (key_size != nullptr && *key_size != parsed_key_size) {
        // Parameter mis-match.
        LOG(ERROR) << "ImportKey: key-size does not match PKCS8";
        return ErrorCode::IMPORT_PARAMETER_MISMATCH;
    }

    const BIGNUM *d = EC_KEY_get0_private_key(ec_key);
    const EC_POINT *pub_key = EC_KEY_get0_public_key(ec_key);
    bssl::UniquePtr<BIGNUM> x(BN_new());
    bssl::UniquePtr<BIGNUM> y(BN_new());

    if (!EC_POINT_get_affine_coordinates_GFp(EC_KEY_get0_group(ec_key),
                                             pub_key, x.get(), y.get(), NULL)) {
        LOG(ERROR) << "ImportKey: failed to get public key in affine coordinates";
        return ErrorCode::INVALID_ARGUMENT;
    }

    // Key data may be invalid, and will be validated on the device
    // anyway, so avoid duplicate work here.

    // Curve parameter.
    request->mutable_ec()->set_curve_id((uint32_t)parsed_curve_id);

    // Private key.
    const size_t field_size = (parsed_key_size + 7) >> 3;
    unique_ptr<uint8_t[]> d_buf(new uint8_t[field_size]);
    if (!BN_bn2le_padded(d_buf.get(), field_size, d)) {
        LOG(ERROR) << "ImportKey request: bn2le(d) failed";
        return ErrorCode::UNKNOWN_ERROR;
    }
    request->mutable_ec()->set_d(d_buf.get(), field_size);

    // Public key.
    unique_ptr<uint8_t[]> x_buf(new uint8_t[field_size]);
    if (!BN_bn2le_padded(x_buf.get(), field_size, x.get())) {
        LOG(ERROR) << "ImportKey request: bn2le(x) failed";
        return ErrorCode::UNKNOWN_ERROR;
    }
    request->mutable_ec()->set_x(x_buf.get(), field_size);

    unique_ptr<uint8_t[]> y_buf(new uint8_t[field_size]);
    if (!BN_bn2le_padded(y_buf.get(), field_size, y.get())) {
        LOG(ERROR) << "ImportKey request: bn2le(y) failed";
        return ErrorCode::UNKNOWN_ERROR;
    }
    request->mutable_ec()->set_y(y_buf.get(), field_size);

    return ErrorCode::OK;
}

static ErrorCode import_key_raw(const tag_map_t& params,
                                Algorithm algorithm,
                                const hidl_vec<uint8_t>& keyData,
                                ImportKeyRequest *request)
{
  if (algorithm != Algorithm::AES && algorithm != Algorithm::TRIPLE_DES &&
      algorithm != Algorithm::HMAC) {
        LOG(ERROR) << "ImportKey request: unsupported algorithm";
        return ErrorCode::UNSUPPORTED_ALGORITHM;
    }

    const uint32_t *key_size = nullptr;
    if (params.find(Tag::KEY_SIZE) != params.end()) {
        const vector<KeyParameter>& v = params.find(Tag::KEY_SIZE)->second;
        key_size = &v[0].f.integer;
    }

    if (algorithm != Algorithm::TRIPLE_DES) {
        if (key_size != nullptr && *key_size != keyData.size() * 8) {
            LOG(ERROR) << "ImportKey request: mis-matched KEY_SIZE tag: "
                       << ((key_size == NULL) ? -1 : *key_size)
                       << " provided data size: "
                       << keyData.size();
            return ErrorCode::IMPORT_PARAMETER_MISMATCH;
        }
    } else {
        if ((key_size != nullptr && *key_size != 168) ||
            keyData.size() != 24) {
            LOG(ERROR) << "ImportKey request: mis-matched DES KEY_SIZE tag: "
                       << ((key_size == NULL) ? -1 : *key_size)
                       << " provided data size: "
                       << keyData.size();
            return ErrorCode::IMPORT_PARAMETER_MISMATCH;
        }
        LOG(ERROR) << "ImportKey request: DES OK: ";
    }

    if (keyData.size() == 0) {
        LOG(ERROR) << "ImportKey request: invalid key size 0";
        return ErrorCode::IMPORT_PARAMETER_MISMATCH;
    }

    request->mutable_symmetric_key()->set_material(
        keyData.data(), keyData.size());

    return ErrorCode::OK;
}

ErrorCode import_key_request(const hidl_vec<KeyParameter>& params,
                             KeyFormat keyFormat,
                             const hidl_vec<uint8_t>& keyData,
                             ImportKeyRequest *request) {
    const enum Algorithm *algorithm;

    if (keyFormat != KeyFormat::PKCS8 && keyFormat != KeyFormat::RAW) {
        return ErrorCode::UNSUPPORTED_KEY_FORMAT;
    }

    ErrorCode error;
    tag_map_t tag_map;
    error = hidl_params_to_map(params, &tag_map);
    if (error != ErrorCode::OK) {
        return error;
    }

    if (tag_map.find(Tag::ALGORITHM) != tag_map.end()) {
        // Algorithm is a required parameter.
        const vector<KeyParameter>& v = tag_map.find(Tag::ALGORITHM)->second;
        algorithm = &v[0].f.algorithm;
    } else {
        LOG(ERROR) << "ImportKey request: Algorithm Tag missing";
        return ErrorCode::INVALID_ARGUMENT;
    }

    if (keyFormat == KeyFormat::PKCS8) {
        switch (*algorithm) {
        case Algorithm::RSA:
            error = import_key_rsa(tag_map, keyData, request);
            break;
        case Algorithm::EC:
            error = import_key_ec(tag_map, keyData, request);
            break;
        default:
            LOG(ERROR) << "ImportKey request: unsupported algoritm: "
                       << (uint32_t)*algorithm;
            return ErrorCode::UNSUPPORTED_ALGORITHM;
            break;
        }
    } else {
        error = import_key_raw(tag_map, *algorithm, keyData, request);
    }

    if (error != ErrorCode::OK) {
        return error;
    }

    error = map_params_to_pb(tag_map, request->mutable_params());
    if (error != ErrorCode::OK) {
        LOG(ERROR) << "ImportKey request: failed to map params to pb: "
                   << (uint32_t) error;
        return error;
    }

    return ErrorCode::OK;
}

}  // namespace keymaster
}  // hardware
}  // android
