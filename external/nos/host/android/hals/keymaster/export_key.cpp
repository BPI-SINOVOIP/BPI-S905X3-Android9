/*
 * Copyright 2018 The Android Open Source Project
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

#include "export_key.h"
#include "proto_utils.h"

#include <android-base/logging.h>

#include <openssl/bn.h>
#include <openssl/bytestring.h>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>

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
namespace nosapp = nugget::app::keymaster;

// BoringSSL
using bssl::UniquePtr;

// std
using std::unique_ptr;

ErrorCode export_key_der(const ExportKeyResponse& response,
                          hidl_vec<uint8_t> *der) {

  if (response.algorithm() == nosapp::Algorithm::RSA) {
      bssl::UniquePtr<BIGNUM> n(BN_new());
      BN_le2bn(
          reinterpret_cast<const uint8_t *>(response.rsa().n().data()),
          response.rsa().n().size(), n.get());

      bssl::UniquePtr<BIGNUM> e(BN_new());
      if (!BN_set_word(e.get(), response.rsa().e())) {
          LOG(ERROR) << "ExportKey: BN_set_word failed";
          return ErrorCode::UNKNOWN_ERROR;
      }

      bssl::UniquePtr<RSA> rsa(RSA_new());
      if (!RSA_set0_key(rsa.get(), n.get(), e.get(), NULL)) {
          LOG(ERROR) << "ExportKey: RSA_set0_key failed";
          return ErrorCode::UNKNOWN_ERROR;
      }

      CBB cbb;
      uint8_t *data = new uint8_t[1024];  /* Plenty for RSA 4-k. */
      CBB_init_fixed(&cbb, data, 1024);
      bssl::UniquePtr<EVP_PKEY> pkey(EVP_PKEY_new());
      EVP_PKEY_set1_RSA(pkey.get(), rsa.get());
      if (!EVP_marshal_public_key(&cbb, pkey.get())) {
          LOG(ERROR) << "ExportKey: EVP_marshal_public_key failed";
          return ErrorCode::UNKNOWN_ERROR;
      }

      der->setToExternal(
          const_cast<uint8_t*>(
              data), CBB_len(&cbb), true /* Transfer ownership. */);
      for (size_t i = 0; i < der->size(); i++) {
          LOG(ERROR) << "exporting: " << std::hex << (int)der->data()[i];
      }
      CBB_cleanup(&cbb);
  } else {
      EcCurve ec_curve;
      /* TODO: eliminate this type-cast. */
      if (translate_ec_curve(
              (nosapp::EcCurve)response.ec().curve_id(),
              &ec_curve) != ErrorCode::OK) {
          LOG(ERROR) << "Failed to parse response device ec curve_id: "
                     << response.ec().curve_id();
          return ErrorCode::UNKNOWN_ERROR;
      }
    int curve_nid;
    switch (ec_curve) {
    case EcCurve::P_224:
        curve_nid = NID_secp224r1;
        break;
    case EcCurve::P_256:
        curve_nid = NID_X9_62_prime256v1;
        break;
    case EcCurve::P_384:
        curve_nid = NID_secp384r1;
        break;
    case EcCurve::P_521:
        curve_nid = NID_secp521r1;
        break;
    default:
        LOG(ERROR) << "ExportKey: received invalid EcCurve id: "
                   << (uint32_t) ec_curve;
        return ErrorCode::UNKNOWN_ERROR;
    }

    bssl::UniquePtr<EC_GROUP> ec_group(EC_GROUP_new_by_curve_name(curve_nid));
    if (!ec_group.get()) {
        LOG(ERROR) << "EC_GROUP_new_by_name("
                   << (uint32_t)curve_nid << ") failed";
        return ErrorCode::UNKNOWN_ERROR;
    }
    bssl::UniquePtr<EC_POINT> ec_point(EC_POINT_new(ec_group.get()));
    if (!ec_point.get()) {
        LOG(ERROR) << "EC_POINT_new() failed";
        return ErrorCode::UNKNOWN_ERROR;
    }
    bssl::UniquePtr<BIGNUM> x(BN_new());
    BN_le2bn(
        reinterpret_cast<const uint8_t *>(response.ec().x().data()),
        response.ec().x().size(), x.get());
    bssl::UniquePtr<BIGNUM> y(BN_new());
    BN_le2bn(
        reinterpret_cast<const uint8_t *>(response.ec().y().data()),
        response.ec().y().size(), y.get());
    if (!EC_POINT_set_affine_coordinates_GFp(
            ec_group.get(), ec_point.get(), x.get(), y.get(), NULL)) {
        LOG(ERROR) << "EC_POINT_set_affine_coordinates() failed";
        return ErrorCode::UNKNOWN_ERROR;
    }

    bssl::UniquePtr<EC_KEY> ec_key(EC_KEY_new_by_curve_name(curve_nid));
    if (!ec_key.get()) {
        LOG(ERROR) << "EC_KEY_new() failed";
        return ErrorCode::UNKNOWN_ERROR;
    }

    if (!EC_KEY_set_public_key(ec_key.get(), ec_point.get())) {
        LOG(ERROR) << "EC_KEY_set_public_key() failed";
        return ErrorCode::UNKNOWN_ERROR;
    }

    bssl::UniquePtr<EVP_PKEY> pkey(EVP_PKEY_new());
    if (!pkey.get()) {
        LOG(ERROR) << "EVP_PKEY_new() failed";
        return ErrorCode::UNKNOWN_ERROR;
    }
    if (!EVP_PKEY_set1_EC_KEY(pkey.get(), ec_key.get())) {
        LOG(ERROR) << "EVP_PKEY_set1_EC_KEY() failed";
        return ErrorCode::UNKNOWN_ERROR;
    }
    CBB cbb;
    uint8_t *data = new uint8_t[256];  /* Plenty for EC-521. */
    CBB_init_fixed(&cbb, data, 256);
    if (!EVP_marshal_public_key(&cbb, pkey.get())) {
      LOG(ERROR) << "ExportKey: EVP_marshal_public_key failed";
      return ErrorCode::UNKNOWN_ERROR;
    }

    der->setToExternal(
            const_cast<uint8_t*>(
                data), CBB_len(&cbb), true /* Transfer ownership. */);
    for (size_t i = 0; i < der->size(); i++) {
      LOG(ERROR) << "exporting: " << std::hex << (int)der->data()[i];
    }
    CBB_cleanup(&cbb);
  }

  return ErrorCode::OK;
}

}  // namespace keymaster
}  // hardware
}  // android
