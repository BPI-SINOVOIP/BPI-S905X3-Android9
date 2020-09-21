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

#include "import_wrapped_key.h"
#include "macros.h"
#include "proto_utils.h"

#include <android-base/logging.h>

#include <openssl/bytestring.h>
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
using ::android::hardware::keymaster::V4_0::BlockMode;
using ::android::hardware::keymaster::V4_0::Digest;
using ::android::hardware::keymaster::V4_0::EcCurve;
using ::android::hardware::keymaster::V4_0::HardwareAuthenticatorType;
using ::android::hardware::keymaster::V4_0::KeyFormat;
using ::android::hardware::keymaster::V4_0::KeyPurpose;
using ::android::hardware::keymaster::V4_0::PaddingMode;
using ::android::hardware::keymaster::V4_0::Tag;
using ::android::hardware::keymaster::V4_0::TagType;

// BoringSSL
using bssl::UniquePtr;

// std
using std::function;
using std::unique_ptr;

using parse_asn1_fn = function<ErrorCode(CBS *cbs, Tag tag,
                                         ImportWrappedKeyRequest *request)>;

#define KM_WRAPPER_FORMAT_VERSION    0
#define KM_WRAPPER_GCM_IV_SIZE       12
#define KM_WRAPPER_GCM_TAG_SIZE      16
#define KM_WRAPPER_WRAPPED_KEY_SIZE  32
#define KM_TAG_MASK                  0x0FFFFFFF

// BoringSSL helpers.
static int CBS_get_optional_asn1_set(CBS *cbs, CBS *out, int *out_present,
                                     unsigned tag) {
  CBS child;
  int present;
  if (!CBS_get_optional_asn1(cbs, &child, &present, tag)) {
    return 0;
  }
  if (present) {
    if (!CBS_get_asn1(&child, out, CBS_ASN1_SET) ||
        CBS_len(&child) != 0) {
      return 0;
    }
  } else {
    CBS_init(out, NULL, 0);
  }
  if (out_present) {
    *out_present = present;
  }
  return 1;
}

static int CBS_get_optional_asn1_null(CBS *cbs, CBS *out, int *out_present,
                                     unsigned tag) {
  CBS child;
  int present;
  if (!CBS_get_optional_asn1(cbs, &child, &present, tag)) {
    return 0;
  }
  if (present) {
    if (!CBS_get_asn1(&child, out, CBS_ASN1_NULL) ||
        CBS_len(&child) != 0) {
      return 0;
    }
  } else {
    CBS_init(out, NULL, 0);
  }
  if (out_present) {
    *out_present = present;
  }
  return 1;
}

static ErrorCode parse_asn1_set(CBS *auth, Tag tag,
                                ImportWrappedKeyRequest *request)
{
    CBS set;
    int present;
    if (!CBS_get_optional_asn1_set(
            auth, &set, &present,
            CBS_ASN1_CONTEXT_SPECIFIC | CBS_ASN1_CONSTRUCTED |
            (tag & 0x0fffffff))) {
        return ErrorCode::INVALID_ARGUMENT;
    }
    if (!present) {
        // This is optional parameter, so ok to be missing.
        return ErrorCode::OK;
    }
    // TODO: are empty sets acceptable?

    while (CBS_len(&set)) {
        uint64_t val;
        KeyParameter kp;
        if (!CBS_get_asn1_uint64(&set, &val)) {
            return ErrorCode::INVALID_ARGUMENT;
        }

        kp.tag = tag;
        switch (tag) {
        case Tag::PURPOSE:
            kp.f.purpose = (KeyPurpose)val;
            break;
        case Tag::BLOCK_MODE:
            kp.f.blockMode = (BlockMode)val;
            break;
        case Tag::DIGEST:
            kp.f.digest = (Digest)val;
            break;
        case Tag::PADDING:
            kp.f.paddingMode = (PaddingMode)val;
            break;
        case Tag::USER_SECURE_ID:
            kp.f.longInteger = val;
            break;
        default:
            return ErrorCode::INVALID_ARGUMENT;
        }
        nosapp::KeyParameter *param = request->mutable_params()->add_params();
        if (key_parameter_to_pb(kp, param) != ErrorCode::OK) {
            return ErrorCode::INVALID_ARGUMENT;
        }
    }

    return ErrorCode::OK;
}

static ErrorCode parse_asn1_integer(CBS *auth, Tag tag,
                                    ImportWrappedKeyRequest *request)
{
    uint64_t val;
    if (!CBS_get_optional_asn1_uint64(
            auth, &val,
            CBS_ASN1_CONTEXT_SPECIFIC | CBS_ASN1_CONSTRUCTED |
            (tag & 0x0fffffff), (uint64_t)-1)) {
        return ErrorCode::INVALID_ARGUMENT;
    }
    if (val == (uint64_t)-1) {
        // This is optional parameter, so ok to be missing.
        return ErrorCode::OK;
    }

    KeyParameter kp;
    kp.tag = tag;
    switch (tag) {
    case Tag::ALGORITHM:
        kp.f.algorithm = (Algorithm)val;
        break;
    case Tag::KEY_SIZE:
    case Tag::MIN_MAC_LENGTH:
    case Tag::RSA_PUBLIC_EXPONENT:
    case Tag::MIN_SECONDS_BETWEEN_OPS:
    case Tag::MAX_USES_PER_BOOT:
    case Tag::AUTH_TIMEOUT:
        if (val > UINT32_MAX) {
            return ErrorCode::INVALID_ARGUMENT;
        }
        kp.f.integer = (uint32_t)val;
        break;
    case Tag::EC_CURVE:
        kp.f.ecCurve = (EcCurve)val;
        break;
    case Tag::ACTIVE_DATETIME:
    case Tag::ORIGINATION_EXPIRE_DATETIME:
    case Tag::USAGE_EXPIRE_DATETIME:
    case Tag::CREATION_DATETIME:
        kp.f.longInteger = val;
        break;
    case Tag::USER_AUTH_TYPE:
        kp.f.hardwareAuthenticatorType = (HardwareAuthenticatorType)val;
        break;
    default:
        return ErrorCode::INVALID_ARGUMENT;
    }

    nosapp::KeyParameter *param = request->mutable_params()->add_params();
    if (key_parameter_to_pb(kp, param) != ErrorCode::OK) {
        return ErrorCode::INVALID_ARGUMENT;
    }

    return ErrorCode::OK;
}

static ErrorCode parse_asn1_boolean(CBS *auth, Tag tag,
                                    ImportWrappedKeyRequest *request)
{
    CBS null;
    int present;
    if (!CBS_get_optional_asn1_null(
            auth, &null, &present,
            CBS_ASN1_CONTEXT_SPECIFIC | CBS_ASN1_CONSTRUCTED |
            (tag & 0x0fffffff))) {
        return ErrorCode::INVALID_ARGUMENT;
    }
    if (!present) {
        // This is optional parameter, so ok to be missing.
        return ErrorCode::OK;
    }
    if (CBS_len(&null) != 0) {
        // NULL type should be empty.
        return ErrorCode::INVALID_ARGUMENT;
    }

    KeyParameter kp;
    kp.tag = tag;
    switch (tag) {
    case Tag::CALLER_NONCE:
    case Tag::BOOTLOADER_ONLY:
    case Tag::NO_AUTH_REQUIRED:
        kp.f.boolValue = true;
        break;
    default:
        return ErrorCode::INVALID_ARGUMENT;
    }

    nosapp::KeyParameter *param = request->mutable_params()->add_params();
    if (key_parameter_to_pb(kp, param) != ErrorCode::OK) {
        return ErrorCode::INVALID_ARGUMENT;
    }

    return ErrorCode::OK;
}

static ErrorCode parse_asn1_octet_string(CBS *auth, Tag tag,
                                         ImportWrappedKeyRequest *request)
{
    CBS str;
    int present;
    if (!CBS_get_optional_asn1_octet_string(
            auth, &str, &present,
            CBS_ASN1_CONTEXT_SPECIFIC | CBS_ASN1_CONSTRUCTED |
            (tag & 0x0fffffff))) {
        return ErrorCode::INVALID_ARGUMENT;
    }
    if (!present) {
        // This is optional parameter, so ok to be missing.
        return ErrorCode::OK;
    }
    if (CBS_len(&str) == 0) {
        return ErrorCode::INVALID_ARGUMENT;
    }

    KeyParameter kp;
    kp.tag = tag;
    switch (tag) {
    case Tag::APPLICATION_DATA:
    case Tag::APPLICATION_ID:
        kp.blob.setToExternal(
                const_cast<uint8_t*>(CBS_data(&str)),
            CBS_len(&str), false);
        break;
    default:
        return ErrorCode::INVALID_ARGUMENT;
    }

    nosapp::KeyParameter *param = request->mutable_params()->add_params();
    if (key_parameter_to_pb(kp, param) != ErrorCode::OK) {
        return ErrorCode::INVALID_ARGUMENT;
    }

    return ErrorCode::OK;
}

ErrorCode import_wrapped_key_request(const hidl_vec<uint8_t>& wrappedKeyData,
                                     const hidl_vec<uint8_t>& wrappingKeyBlob,
                                     const hidl_vec<uint8_t>& maskingKey,
                                     ImportWrappedKeyRequest *request)
{
    /*
     * Unwrap an ASN.1 DER wrapped key, as specified here:
     * https://docs.google.com/document/d/165Jd6jumhOD2yB6MygKhqjOLuHm3YVZGTgFHmGc8HzA/edit#heading=h.ut6j2przg4ra
     *
     * AuthorizationList ::= SEQUENCE {
     *     purpose  [1] EXPLICIT SET OF INTEGER OPTIONAL,
     *     algorithm  [2] EXPLICIT INTEGER OPTIONAL,
     *     keySize  [3] EXPLICIT INTEGER OPTIONAL,
     *     blockMode [4] EXPLICIT SET OF INTEGER OPTIONAL,
     *     digest  [5] EXPLICIT SET OF INTEGER OPTIONAL,
     *     padding  [6] EXPLICIT SET OF INTEGER OPTIONAL,
     *     callerNonce [7] EXPLICIT NULL OPTIONAL,
     *     minMacLength [8] EXPLICIT INTEGER OPTIONAL,
     *     ecCurve  [10] EXPLICIT INTEGER OPTIONAL,
     *     rsaPublicExponent  [200] EXPLICIT INTEGER OPTIONAL,
     *     bootloaderOnly [302]  EXPLICIT NULL OPTIONAL,
     *     activeDateTime  [400] EXPLICIT INTEGER OPTIONAL,
     *     originationExpireDateTime  [401] EXPLICIT INTEGER OPTIONAL,
     *     usageExpireDateTime  [402] EXPLICIT INTEGER OPTIONAL,
     *     minSecondsBetweenOps  [403] EXPLICIT INTEGER OPTIONAL,
     *     maxUsesPerBoot  [404] EXPLICIT INTEGER OPTIONAL,
     *     userSecureId [502]  EXPLICIT SET OF INTEGER OPTIONAL,
     *     noAuthRequired  [503] EXPLICIT NULL OPTIONAL,
     *     userAuthType  [504] EXPLICIT INTEGER OPTIONAL,
     *     authTimeout  [505] EXPLICIT INTEGER OPTIONAL,
     *     allApplications  [600] EXPLICIT NULL OPTIONAL,
     *     applicationId  [601] EXPLICIT OCTET_STRING OPTIONAL,
     *     applicationData  [700]  EXPLICIT OCTET_STRING OPTIONAL,
     *     creationDateTime  [701] EXPLICIT INTEGER OPTIONAL,
     * }
     *
     * KeyDescription ::= SEQUENCE {
     *    keyFormat INTEGER,
     *    authorizationList AuthorizationList
     * }
     *
     * SecureKeyWrapper ::= SEQUENCE {
     *    wrapperFormatVersion INTEGER,
     *    encryptedTransportKey OCTET_STRING,
     *    initializationVector OCTET_STRING,
     *    keyDescription KeyDescription,
     *    secureKey OCTET_STRING,
     *    tag OCTET_STRING,
     * }
     */

    CBS cbs;
    CBS child;
    uint64_t wrapperFormatVersion;
    CBS encryptedTransportKey;
    CBS initializationVector;

    CBS_init(&cbs, wrappedKeyData.data(), wrappedKeyData.size());
    if (!CBS_get_asn1(&cbs, &child, CBS_ASN1_SEQUENCE) ||
        !CBS_get_asn1_uint64(&child, &wrapperFormatVersion)) {
        LOG(ERROR) << "Failed to wrap outer sequence or wrapperFormatVersion";
        return ErrorCode::INVALID_ARGUMENT;
    }
    if (wrapperFormatVersion != KM_WRAPPER_FORMAT_VERSION) {
        LOG(ERROR) << "Invalid wrapper format version" << wrapperFormatVersion;
        return ErrorCode::INVALID_ARGUMENT;
    }
    if (!CBS_get_asn1(&child, &encryptedTransportKey, CBS_ASN1_OCTETSTRING) ||
        !CBS_get_asn1(&child, &initializationVector, CBS_ASN1_OCTETSTRING)) {
        LOG(ERROR) <<
            "Failed to parse encryptedTransportKey or initializationVector";
        return ErrorCode::INVALID_ARGUMENT;
    }

    /* TODO: if the RSA key-size is known from the blob, use it to
     * validate encryptedTransportKey (i.e. the RSA envelope) length.
     */

    if (CBS_len(&initializationVector) != KM_WRAPPER_GCM_IV_SIZE) {
        LOG(ERROR) << "Invalid AAD length";
        return ErrorCode::INVALID_ARGUMENT;
    }

    CBS aad;
    if (!CBS_get_asn1_element(&child, &aad, CBS_ASN1_SEQUENCE)) {
        LOG(ERROR) << "Failed to parse aad";
        return ErrorCode::INVALID_ARGUMENT;
    }
    /* Assign over AAD before CBS gets consumed. */
    request->set_aad(CBS_data(&aad), CBS_len(&aad));

    CBS keyDescription;
    if (!CBS_get_asn1(&aad, &keyDescription, CBS_ASN1_SEQUENCE)) {
        LOG(ERROR) << "Failed to parse keyDescription";
        return ErrorCode::INVALID_ARGUMENT;
    }

    uint64_t keyFormat;
    if (!CBS_get_asn1_uint64(&keyDescription, &keyFormat)) {
        LOG(ERROR) << "Failed to parse keyFormat";
        return ErrorCode::INVALID_ARGUMENT;
    }

    CBS authorizationList;
    if (!CBS_get_asn1(&keyDescription, &authorizationList, CBS_ASN1_SEQUENCE) ||
        CBS_len(&keyDescription) != 0) {
        LOG(ERROR) << "Failed to parse keyDescription, remaining length: "
                   << CBS_len(&keyDescription);
        return ErrorCode::INVALID_ARGUMENT;
    }

    // TODO: the list below may not be complete,
    // update once the AuthorizationList spec final.
    struct tag_parser_entry {
        Tag tag;
        const parse_asn1_fn fn;
    };
    const struct tag_parser_entry parser_table[] = {
        {Tag::PURPOSE, parse_asn1_set},
        {Tag::ALGORITHM, parse_asn1_integer},
        {Tag::KEY_SIZE, parse_asn1_integer},
        {Tag::BLOCK_MODE, parse_asn1_set},
        {Tag::DIGEST, parse_asn1_set},
        {Tag::PADDING, parse_asn1_set},
        {Tag::CALLER_NONCE, parse_asn1_boolean},
        {Tag::MIN_MAC_LENGTH, parse_asn1_integer},
        {Tag::EC_CURVE, parse_asn1_integer},
        {Tag::RSA_PUBLIC_EXPONENT, parse_asn1_integer},
        {Tag::BOOTLOADER_ONLY, parse_asn1_boolean},
        {Tag::ACTIVE_DATETIME, parse_asn1_integer},
        {Tag::ORIGINATION_EXPIRE_DATETIME, parse_asn1_integer},
        {Tag::USAGE_EXPIRE_DATETIME, parse_asn1_integer},
        {Tag::MIN_SECONDS_BETWEEN_OPS, parse_asn1_integer},
        {Tag::MAX_USES_PER_BOOT, parse_asn1_integer},
        {Tag::USER_SECURE_ID, parse_asn1_set},
        {Tag::NO_AUTH_REQUIRED, parse_asn1_boolean},
        {Tag::USER_AUTH_TYPE, parse_asn1_integer},
        {Tag::AUTH_TIMEOUT, parse_asn1_integer},
        {Tag::APPLICATION_ID, parse_asn1_octet_string},
        {Tag::APPLICATION_DATA, parse_asn1_octet_string},
        {Tag::CREATION_DATETIME, parse_asn1_octet_string},
    };

    for (size_t i = 0; i < ARRAYSIZE(parser_table); i++) {
        const struct tag_parser_entry *entry = &parser_table[i];

        if (entry->fn(&authorizationList, entry->tag, request)
            != ErrorCode::OK) {
          LOG(ERROR) <<
              "ImportWrappedKey authorization list parse failure: tag: "
                     << (uint32_t)entry->tag;
            return ErrorCode::INVALID_ARGUMENT;
        }
    }

    if (CBS_len(&authorizationList) != 0) {
        LOG(ERROR) << "Authorization list not fully consumed, "
                   << CBS_len(&authorizationList)
                   << " bytes remaining";
            return ErrorCode::INVALID_ARGUMENT;
    }

    CBS secureKey;
    CBS tag;
    if (!CBS_get_asn1(&child, &secureKey, CBS_ASN1_OCTETSTRING)) {
        LOG(ERROR) << "Failed to parse secure key";
        return ErrorCode::INVALID_ARGUMENT;
    }
    if (CBS_len(&secureKey) != KM_WRAPPER_WRAPPED_KEY_SIZE) {
        LOG(ERROR) << "Secure key len, expected: "
                   << KM_WRAPPER_WRAPPED_KEY_SIZE
                   << " got: "
                   << CBS_len(&secureKey);
        return ErrorCode::INVALID_ARGUMENT;
    }
    if (!CBS_get_asn1(&child, &tag, CBS_ASN1_OCTETSTRING)) {
        LOG(ERROR) << "Failed to parse gcm tag";
        return ErrorCode::INVALID_ARGUMENT;
    }
    if (CBS_len(&tag) != KM_WRAPPER_GCM_TAG_SIZE) {
        LOG(ERROR) << "GCM tag len, expected: "
                   << KM_WRAPPER_GCM_TAG_SIZE
                   << " got: "
                   << CBS_len(&tag);
        return ErrorCode::INVALID_ARGUMENT;
    }

    request->set_key_format(keyFormat);
    request->set_rsa_envelope(CBS_data(&encryptedTransportKey),
                              CBS_len(&encryptedTransportKey));
    request->set_initialization_vector(CBS_data(&initializationVector),
                                       CBS_len(&initializationVector));
    request->set_encrypted_import_key(CBS_data(&secureKey),
                                      CBS_len(&secureKey));
    request->set_gcm_tag(CBS_data(&tag), CBS_len(&tag));
    request->mutable_wrapping_key_blob()->set_blob(wrappingKeyBlob.data(),
                                                 wrappingKeyBlob.size());

    request->set_masking_key(maskingKey.data(), maskingKey.size());

    return ErrorCode::OK;
}

}  // namespace keymaster
}  // hardware
}  // android
